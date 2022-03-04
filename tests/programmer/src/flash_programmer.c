/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "string.h"
#include "stdio.h"
#include "crc32_c.h"
#include "hexdump.h"
#include "tool_msg.h"
#include "sys_api_programmer.h"
#include "flash_programmer.h"

static int send_burn_data_reply(enum ERR_CODE code, unsigned short sec_seq, unsigned char seq);

//======================================================================================================

#define FLASH_PROGRAMMER_VERSION 0x0103

#ifdef SINGLE_WIRE_DOWNLOAD
#define SLIDE_WIN_NUM 1
#else
#define SLIDE_WIN_NUM 2
#endif

#define BULK_READ_STEP_SIZE 0x1000

#define MAX_SEC_REG_BURN_LEN 0x1000

#ifdef _WIN32
#define BURN_BUFFER_LOC
#else
#ifdef __ARMCC_VERSION
#define BURN_BUFFER_LOC __attribute__((section(".bss.burn_buffer")))
#else
#define BURN_BUFFER_LOC __attribute__((section(".burn_buffer")))
#endif
#endif

enum PROGRAMMER_STATE {
    PROGRAMMER_NONE,
    PROGRAMMER_ERASE_BURN_START,
    PROGRAMMER_SEC_REG_ERASE_BURN_START,
};

enum DATA_BUF_STATE {
    DATA_BUF_FREE,
    DATA_BUF_RECV,
    DATA_BUF_BURN,
    DATA_BUF_DONE,
};

enum FLASH_CMD_TYPE {
    FLASH_CMD_GET_ID = 0x11,
    FLASH_CMD_GET_UNIQUE_ID = 0x12,
    FLASH_CMD_GET_SIZE = 0x13,
    FLASH_CMD_ERASE_SECTOR = 0x21,
    FLASH_CMD_BURN_DATA = 0x22,
    FLASH_CMD_ERASE_CHIP = 0x31,
    FLASH_CMD_SEC_REG_ERASE = 0x41,
    FLASH_CMD_SEC_REG_BURN = 0x42,
    FLASH_CMD_SEC_REG_LOCK = 0x43,
    FLASH_CMD_SEC_REG_READ = 0x44,
    FLASH_CMD_ENABLE_REMAP = 0x51,
    FLASH_CMD_DISABLE_REMAP = 0x52,
};

static enum PROGRAMMER_STATE programmer_state;

static enum PARSE_STATE parse_state;
static struct message_t recv_msg;
static struct message_t send_msg = {
    {
        PREFIX_CHAR,
    },
};
static unsigned char send_seq = 0;

static unsigned int extra_buf_len;
static unsigned int recv_extra_timeout;

static unsigned int burn_addr;
static unsigned int burn_len;
static unsigned int sector_size;
static unsigned int sector_cnt;
static unsigned int last_sector_len;
static unsigned int cur_sector_seq;

static unsigned char BURN_BUFFER_LOC data_buf[SLIDE_WIN_NUM][(32 * 1024 + BURN_DATA_MSG_OVERHEAD + 63) / 64 * 64];
static enum DATA_BUF_STATE data_buf_state[SLIDE_WIN_NUM];
static enum ERR_CODE data_buf_errcode[SLIDE_WIN_NUM];
static unsigned int cur_data_buf_idx;

#ifndef _WIN32
static volatile unsigned char burn_aborted;
#endif

#if defined(SECURE_BOOT_WORKAROUND_SOLUTION) && defined(PROGRAMMER_INFLASH)
void programmer_timeout_clr_single_mode_upgrade_from_ota_app_boot_info(void);
#endif

STATIC_ASSERT(sizeof(data_buf[0]) >= 2 * (BURN_DATA_MSG_OVERHEAD + MAX_SEC_REG_BURN_LEN), "data_buf too small");

static const unsigned int size_mask = SECTOR_SIZE_32K | SECTOR_SIZE_4K;

static unsigned int count_set_bits(unsigned int i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

static void reset_data_buf_state(void)
{
    unsigned int i;

    cur_data_buf_idx = 0;
    data_buf_state[0] = DATA_BUF_RECV;
    for (i = 1; i < SLIDE_WIN_NUM; i++) {
        data_buf_state[i] = DATA_BUF_FREE;
    }
}

static void burn_data_buf(int index)
{
    data_buf_state[index] = DATA_BUF_BURN;

#ifdef _WIN32
    write_flash_data(&data_buf[index][BURN_DATA_MSG_OVERHEAD],
                     data_buf[index][4] | (data_buf[index][5] << 8) | (data_buf[index][6] << 16) | (data_buf[index][7] << 24));

    send_burn_data_reply(ERR_BURN_OK, data_buf[index][12] | (data_buf[index][13] << 8), data_buf[index][2]);
    data_buf_state[index] = DATA_BUF_FREE;
#endif
}

static int get_free_data_buf(void)
{
    unsigned int i;
    unsigned int index;

    while (1) {
        index = cur_data_buf_idx + 1;
        if (index >= SLIDE_WIN_NUM) {
            index = 0;
        }
        for (i = 0; i < SLIDE_WIN_NUM; i++) {
            if (data_buf_state[index] == DATA_BUF_FREE) {
                data_buf_state[index] = DATA_BUF_RECV;
                cur_data_buf_idx = index;
                return 0;
            }

            if (++index >= SLIDE_WIN_NUM) {
                index = 0;
            }
        }

        if (wait_data_buf_finished()) {
            break;
        }
    }

    return 1;
}

#ifndef _WIN32
int get_burn_data_buf(unsigned int *rindex)
{
    unsigned int i;
    unsigned int index;

    if (programmer_state != PROGRAMMER_ERASE_BURN_START) {
        return 1;
    }

    index = cur_data_buf_idx + 1;
    if (index >= SLIDE_WIN_NUM) {
        index = 0;
    }
    for (i = 0; i < SLIDE_WIN_NUM; i++) {
        if (data_buf_state[index] == DATA_BUF_BURN) {
            *rindex = index;
            return 0;
        }

        if (++index >= SLIDE_WIN_NUM) {
            index = 0;
        }
    }

    return 1;
}

static void finish_data_buf(unsigned int index, enum ERR_CODE code)
{
    data_buf_errcode[index] = code;
    data_buf_state[index] = DATA_BUF_DONE;
}

int handle_data_buf(unsigned int index)
{
    unsigned int addr;
    unsigned int len;
    unsigned int sec_seq;
    //unsigned int mcrc;
    int ret;

    if (index >= SLIDE_WIN_NUM) {
        return 1;
    }

    TRACE_TIME(2, "### FLASH_TASK: %s index=%u ###", __FUNCTION__, index);

    len = data_buf[index][4] | (data_buf[index][5] << 8) | (data_buf[index][6] << 16) | (data_buf[index][7] << 24);
    //mcrc = data_buf[index][8] | (data_buf[index][9] << 8) |
    //    (data_buf[index][10] << 16) | (data_buf[index][11] << 24);
    sec_seq = data_buf[index][12] | (data_buf[index][13] << 8);
    addr = burn_addr + sec_seq * sector_size;

    TRACE(3, "### FLASH_TASK: sec_seq=%u addr=0x%08X len=%u ###", sec_seq, addr, len);

    if (burn_aborted) {
        finish_data_buf(index, ERR_INTERNAL);
    }

    ret = erase_sector(addr, len);
    if (ret) {
        TRACE(3, "### FLASH_TASK: ERASE_DATA failed: addr=0x%08X len=%u ret=%d ###", addr, len, ret);
        finish_data_buf(index, ERR_ERASE_FLSH);
        return 0;
    }
    TRACE_TIME(0, "### FLASH_TASK: Erase done ###");

    if (burn_aborted) {
        finish_data_buf(index, ERR_INTERNAL);
    }

    ret = burn_data(addr, &data_buf[index][BURN_DATA_MSG_OVERHEAD], len);
    if (ret) {
        TRACE(3, "### FLASH_TASK: BURN_DATA failed: addr=0x%08X len=%u ret=%d ###", addr, len, ret);
        finish_data_buf(index, ERR_BURN_FLSH);
        return 0;
    }
    TRACE_TIME(0, "### FLASH_TASK: Burn done ###");

    if (burn_aborted) {
        finish_data_buf(index, ERR_INTERNAL);
    }

    ret = verify_flash_data(addr, &data_buf[index][BURN_DATA_MSG_OVERHEAD], len);
    if (ret) {
        TRACE(0, "### FLASH_TASK: VERIFY_DATA failed");
        finish_data_buf(index, ERR_VERIFY_FLSH);
        return 0;
    }
    TRACE_TIME(0, "### FLASH_TASK: Verify done ###");

    finish_data_buf(index, ERR_BURN_OK);

    return 0;
}

int free_data_buf(void)
{
    unsigned int i;
    unsigned int index;
    unsigned int sec_seq;

    index = cur_data_buf_idx + 1;
    if (index >= SLIDE_WIN_NUM) {
        index = 0;
    }

    // Free one data buffer once
    for (i = 0; i < SLIDE_WIN_NUM; i++) {
        if (data_buf_state[index] == DATA_BUF_DONE) {
            sec_seq = data_buf[index][12] | (data_buf[index][13] << 8);
            send_burn_data_reply(data_buf_errcode[index], sec_seq, data_buf[index][2]);
            data_buf_state[index] = DATA_BUF_FREE;
            return 0;
        }

        if (++index >= SLIDE_WIN_NUM) {
            index = 0;
        }
    }

    return 1;
}

int data_buf_in_use(void)
{
    unsigned int index;

    for (index = 0; index < SLIDE_WIN_NUM; index++) {
        if (data_buf_state[index] != DATA_BUF_FREE) {
            return 1;
        }
    }

    return 0;
}

int data_buf_in_burn(void)
{
    unsigned int index;

    for (index = 0; index < SLIDE_WIN_NUM; index++) {
        if (data_buf_state[index] == DATA_BUF_BURN) {
            return 1;
        }
    }

    return 0;
}

void set_data_buf_abort_flag(int abort)
{
    burn_aborted = !!abort;
}
#endif

static unsigned char check_sum(const unsigned char *buf, unsigned char len)
{
    int i;
    unsigned char sum = 0;

    for (i = 0; i < len; i++) {
        sum += buf[i];
    }

    return sum;
}

int send_notif_msg(const void *buf, unsigned char len)
{
    int ret;

    if (len + 1 > sizeof(send_msg.data)) {
        len = sizeof(send_msg.data) - 1;
    }

    send_msg.hdr.type = TYPE_NOTIF;
    send_msg.hdr.seq = send_seq++;
    send_msg.hdr.len = len;
    memcpy(&send_msg.data[0], buf, len);
    send_msg.data[len] = ~check_sum((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

    ret = send_data((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg));

    return ret;
}

static int send_sector_size_msg(void)
{
    int ret;

    send_msg.hdr.type = TYPE_SECTOR_SIZE;
    send_msg.hdr.seq = send_seq++;
    send_msg.hdr.len = 6;
    send_msg.data[0] = FLASH_PROGRAMMER_VERSION & 0xFF;
    send_msg.data[1] = (FLASH_PROGRAMMER_VERSION >> 8) & 0xFF;
    send_msg.data[2] = size_mask & 0xFF;
    send_msg.data[3] = (size_mask >> 8) & 0xFF;
    send_msg.data[4] = (size_mask >> 16) & 0xFF;
    send_msg.data[5] = (size_mask >> 24) & 0xFF;
    send_msg.data[6] = ~check_sum((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

    ret = send_data((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg));

    return ret;
}

int send_reply(const void *payload, unsigned int len)
{
    int ret = 0;

    if (len + 1 > sizeof(send_msg.data)) {
        TRACE(1, "Packet length too long: %u", len);
        return -1;
    }

    send_msg.hdr.type = recv_msg.hdr.type;
    send_msg.hdr.seq = recv_msg.hdr.seq;
    send_msg.hdr.len = len;
    memcpy(&send_msg.data[0], payload, len);
    send_msg.data[len] = ~check_sum((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

    ret = send_data((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg));

    return ret;
}

static int send_burn_data_reply(enum ERR_CODE code, unsigned short sec_seq, unsigned char seq)
{
    int ret = 0;
    enum MSG_TYPE type = TYPE_INVALID;

    if (programmer_state == PROGRAMMER_ERASE_BURN_START) {
        type = TYPE_ERASE_BURN_DATA;
#ifdef FLASH_SECURITY_REGISTER
    } else {
        type = TYPE_SEC_REG_ERASE_BURN_DATA;
#endif
    }

    send_msg.hdr.type = type;
    send_msg.hdr.seq = seq;
    send_msg.hdr.len = 3;
    send_msg.data[0] = code;
    send_msg.data[1] = sec_seq & 0xFF;
    send_msg.data[2] = (sec_seq >> 8) & 0xFF;
    send_msg.data[3] = ~check_sum((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

    ret = send_data((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg));

    return ret;
}

static void reset_parse_state(unsigned char **buf, size_t *len)
{
    parse_state = PARSE_HEADER;
    memset(&recv_msg.hdr, 0, sizeof(recv_msg.hdr));

    *buf = (unsigned char *)&recv_msg.hdr;
    *len = sizeof(recv_msg.hdr);
}

static void reset_programmer_state(unsigned char **buf, size_t *len)
{
    programmer_state = PROGRAMMER_NONE;
    reset_parse_state(buf, len);
    reset_data_buf_state();
}

static enum ERR_CODE check_msg_hdr(void)
{
    enum ERR_CODE errcode = ERR_NONE;
    struct CUST_CMD_PARAM param;

    if (recv_msg.hdr.len > MAX_TOOL_MSG_DATA_LEN) {
        return ERR_LEN;
    }

    switch (recv_msg.hdr.type) {
        case TYPE_SYS:
            if (recv_msg.hdr.len != 1 && recv_msg.hdr.len != 5) {
                //TRACE(1,"SYS msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_READ:
            if (recv_msg.hdr.len != 5) {
                //TRACE(1,"READ msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_WRITE:
            if (recv_msg.hdr.len <= 4 || recv_msg.hdr.len > (4 + MAX_WRITE_DATA_LEN)) {
                //TRACE(1,"WRITE msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_BULK_READ:
            if (recv_msg.hdr.len != 8) {
                //TRACE(1,"BULK_READ msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
#ifdef FLASH_SECURITY_REGISTER
        case TYPE_SEC_REG_ERASE_BURN_START:
#endif
        case TYPE_ERASE_BURN_START:
            if (recv_msg.hdr.len != 12) {
                //TRACE(2,"BURN_START 0x%x msg length error: %u", recv_msg.hdr.type, recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
#ifdef FLASH_SECURITY_REGISTER
        case TYPE_SEC_REG_ERASE_BURN_DATA:
#endif
        case TYPE_ERASE_BURN_DATA:
            // BURN_DATA msgs are sent in extra msgs
            errcode = ERR_BURN_INFO_MISSING;
            break;
        case TYPE_FLASH_CMD:
            if (recv_msg.hdr.len > (5 + MAX_FLASH_CMD_DATA_LEN)) {
                //TRACE(1,"FLASH_CMD msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_GET_SECTOR_INFO:
            if (recv_msg.hdr.len != 4) {
                //TRACE(1,"GET_SECTOR_INFO msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        default:
            param.stage = CUST_CMD_STAGE_HEADER;
            param.msg = &recv_msg;
            errcode = handle_cust_cmd(&param);
            if (errcode == ERR_TYPE_INVALID) {
                TRACE(1, "Invalid message type: 0x%02x", recv_msg.hdr.type);
            }
            break;
    }

    if (errcode == ERR_NONE && recv_msg.hdr.len + 1 > sizeof(recv_msg.data)) {
        errcode = ERR_LEN;
    }

    return errcode;
}

static void trace_sys_cmd_info(const char *name)
{
    TRACE(1, "--- %s ---", name);
}

static enum ERR_CODE handle_sys_cmd(enum SYS_CMD_TYPE cmd, unsigned char *param, unsigned int len)
{
    unsigned char cret[5];
    unsigned int val;

    // Programmer can process system commands even when secure boot is enabled
#if 0
    if (debug_write_enabled() == 0) {
        TRACE(0,"[SYS] No access right");
        return ERR_ACCESS_RIGHT;
    }
#endif

    cret[0] = ERR_NONE;

    if (cmd == SYS_CMD_SET_BOOTMODE || cmd == SYS_CMD_CLR_BOOTMODE || cmd == SYS_CMD_SET_DLD_RATE) {
        if (len != 4) {
            TRACE(2, "Invalid SYS CMD len %u for cmd: 0x%x", len, cmd);
            return ERR_DATA_LEN;
        }
    } else {
        if (len != 0) {
            TRACE(2, "Invalid SYS CMD len %u for cmd: 0x%x", len, cmd);
            return ERR_DATA_LEN;
        }
    }

    switch (cmd) {
        case SYS_CMD_REBOOT: {
            trace_sys_cmd_info("Reboot");
            send_reply(cret, 1);
            system_reboot();
            break;
        }
        case SYS_CMD_SHUTDOWN: {
            trace_sys_cmd_info("Shutdown");
            send_reply(cret, 1);
            system_shutdown();
            break;
        }
        case SYS_CMD_FLASH_BOOT: {
            trace_sys_cmd_info("Flash boot");
            send_reply(cret, 1);
            system_flash_boot();
            break;
        }
        case SYS_CMD_SET_BOOTMODE: {
            trace_sys_cmd_info("Set bootmode");
            memcpy(&val, param, 4);
            system_set_bootmode(val);
            send_reply(cret, 1);
            break;
        }
        case SYS_CMD_CLR_BOOTMODE: {
            trace_sys_cmd_info("Clear bootmode");
            memcpy(&val, param, 4);
            system_clear_bootmode(val);
            send_reply(cret, 1);
            break;
        }
        case SYS_CMD_GET_BOOTMODE: {
            trace_sys_cmd_info("Get bootmode");
            val = system_get_bootmode();
            memcpy(&cret[1], &val, 4);
            send_reply(cret, 5);
            break;
        }
        case SYS_CMD_SET_DLD_RATE: {
            trace_sys_cmd_info("Set download rate");
            memcpy(&val, param, 4);
            system_set_download_rate(val);
            send_reply(cret, 1);
            break;
        }
        default: {
            TRACE(1, "Invalid command: 0x%x", recv_msg.data[0]);
            return ERR_SYS_CMD;
        }
    }

    return ERR_NONE;
}

static void trace_rw_info(const char *name, unsigned int addr, unsigned int len)
{
    TRACE(3, "[%s] addr=0x%08X len=%u", name, addr, len);
}

static void trace_rw_access_err(const char *name)
{
    TRACE(1, "[%s] No access right", name);
}

static void trace_rw_len_err(const char *name, unsigned int len)
{
    TRACE(2, "[%s] Length error: %u", name, len);
}

static enum ERR_CODE handle_read_cmd(unsigned int type, unsigned int addr, unsigned int len)
{
    union {
        unsigned int data[1 + (MAX_READ_DATA_LEN + 3) / 4];
        unsigned char buf[(1 + (MAX_READ_DATA_LEN + 3) / 4) * 4];
    } d;
    int i;
#ifndef _WIN32
    int cnt;
    unsigned int *p32;
    unsigned short *p16;
#endif
    const char *name = NULL;

    if (type == TYPE_READ) {
        name = "READ";
#ifdef FLASH_SECURITY_REGISTER
    } else if (type == (TYPE_FLASH_CMD | (FLASH_CMD_SEC_REG_READ << 8))) {
        name = "SEC_READ";
#endif
    } else {
        return ERR_INTERNAL;
    }

    trace_rw_info(name, addr, len);

    if (debug_read_enabled() == 0) {
        trace_rw_access_err(name);
        return ERR_ACCESS_RIGHT;
    }
    if (len > MAX_READ_DATA_LEN) {
        trace_rw_len_err(name, len);
        return ERR_DATA_LEN;
    }

#ifdef _WIN32
    for (i = 0; i < ARRAY_SIZE(d.data); i++) {
        d.data[i] = (0x11 + i) | ((0x22 + i) << 8) | ((0x33 + i) << 16) | ((0x44 + i) << 24);
    }
#else
    if (type == TYPE_READ) {
        // Handle half-word and word register reading
        if ((len & 0x03) == 0 && (addr & 0x03) == 0) {
            cnt = len / 4;
            p32 = (unsigned int *)&d.data[1];
            for (i = 0; i < cnt; i++) {
                p32[i] = *((unsigned int *)addr + i);
            }
        } else if ((len & 0x01) == 0 && (addr & 0x01) == 0) {
            cnt = len / 2;
            p16 = (unsigned short *)&d.data[1];
            for (i = 0; i < cnt; i++) {
                p16[i] = *((unsigned short *)addr + i);
            }
        } else {
            memcpy(&d.data[1], (unsigned char *)addr, len);
        }
#ifdef FLASH_SECURITY_REGISTER
    } else {
        int ret;

        ret = read_security_register(addr, (unsigned char *)&d.data[1], len);
        if (ret) {
            TRACE(2, "[%s] Failed to read: %d", name, ret);
            return ERR_DATA_ADDR;
        }
#endif
    }
#endif

    d.buf[3] = ERR_NONE;
    send_reply((unsigned char *)&d.buf[3], 1 + len);

    return ERR_NONE;
}

static enum ERR_CODE handle_write_cmd(unsigned int addr, unsigned int len, unsigned char *wdata)
{
    unsigned int data;
#ifdef _WIN32
    unsigned char d[MAX_WRITE_DATA_LEN];
#else
    int i;
    int cnt;
#endif
    const char *name = "WRITE";

    trace_rw_info(name, addr, len);

    if (debug_write_enabled() == 0) {
        trace_rw_access_err(name);
        return ERR_ACCESS_RIGHT;
    }
    if (len > MAX_WRITE_DATA_LEN) {
        trace_rw_len_err(name, len);
        return ERR_DATA_LEN;
    }

#ifdef _WIN32
    memcpy(d, wdata, len);
    dump_buffer(d, len);
#else
    // Handle half-word and word register writing
    if ((len & 0x03) == 0 && (addr & 0x03) == 0) {
        cnt = len / 4;
        for (i = 0; i < cnt; i++) {
            data = wdata[4 * i] | (wdata[4 * i + 1] << 8) | (wdata[4 * i + 2] << 16) | (wdata[4 * i + 3] << 24);
            *((unsigned int *)addr + i) = data;
        }
    } else if ((len & 0x01) == 0 && (addr & 0x01) == 0) {
        cnt = len / 2;
        for (i = 0; i < cnt; i++) {
            data = wdata[2 * i] | (wdata[2 * i + 1] << 8);
            *((unsigned short *)addr + i) = (unsigned short)data;
        }
    } else {
        memcpy((unsigned char *)addr, wdata, len);
    }
#endif

    data = ERR_NONE;
    send_reply((unsigned char *)&data, 1);

    return ERR_NONE;
}

static enum ERR_CODE handle_bulk_read_cmd(unsigned int addr, unsigned int len)
{
    int ret;
    unsigned int sent;
    unsigned char cret[1];
    const char *name = "BULK_READ";

    trace_rw_info(name, addr, len);

#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
    if (((addr >= (0x2C800000 - 0x10000)) && (addr <= 0x2C800000)) || ((addr >= (0x2C400000 - 0x10000)) && (addr <= 0x2C400000))) {
        TRACE(1, "%s allow programmer to read the last 64k data of flash.", __func__);
    } else {
        trace_rw_access_err(name);
        return ERR_ACCESS_RIGHT;
    }
#else
    if (debug_read_enabled() == 0) {
        trace_rw_access_err(name);
        return ERR_ACCESS_RIGHT;
    }
#endif

    cret[0] = ERR_NONE;
    send_reply(cret, 1);

    ret = 0;
#ifdef _WIN32
    while (ret == 0 && len > 0) {
        sent = (len > sizeof(data_buf[0])) ? sizeof(data_buf[0]) : len;
        ret = send_data(&data_buf[0][0], sent);
        len -= sent;
    }
#else
    while (ret == 0 && len > 0) {
        sent = (len > BULK_READ_STEP_SIZE) ? BULK_READ_STEP_SIZE : len;
        ret = send_data((unsigned char *)addr, sent);
        addr += sent;
        len -= sent;
    }
#endif

    if (ret) {
        TRACE(2, "[%s] Failed to send data: %d", name, ret);
        // Just let the peer timeout
    }

    return ERR_NONE;
}

static void trace_flash_cmd_info(const char *name)
{
    TRACE_TIME(1, "- %s -", name);
}

static void trace_flash_cmd_len_err(const char *name, unsigned int len)
{
    TRACE(2, "Invalid %s cmd param len: %u", name, len);
}

static void trace_flash_cmd_err(const char *name)
{
    TRACE_TIME(1, "%s failed", name);
}

static void trace_flash_cmd_done(const char *name)
{
    TRACE_TIME(1, "%s done", name);
}

static enum ERR_CODE handle_flash_cmd(enum FLASH_CMD_TYPE cmd, unsigned char *param, unsigned int len)
{
    int ret = 0;
    unsigned char cret = ERR_NONE;
    const char *name = NULL;
    unsigned char id;
    unsigned char cs;

    switch (cmd) {
        case FLASH_CMD_GET_ID: {
            uint8_t ret_id[1 + 3];

            name = "CMD_GET_ID";

            trace_flash_cmd_info(name);

            if (len != 0 && len != 2) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }
            if (len == 0) {
                id = 0;
                cs = 0;
            } else {
                id = param[0];
                cs = param[1];
            }

            get_flash_id(id, cs, &ret_id[1], (sizeof(ret_id) - 1));
            TRACE(3, "GET_FLASH_ID: %02X-%02X-%02X", ret_id[1], ret_id[2], ret_id[3]);

            trace_flash_cmd_done(name);

            ret_id[0] = ERR_NONE;
            send_reply(ret_id, sizeof(ret_id));
            break;
        }
#ifdef FLASH_UNIQUE_ID
        case FLASH_CMD_GET_UNIQUE_ID: {
            uint8_t ret_uid[1 + 16];

            name = "CMD_GET_UNIQUE_ID";

            trace_flash_cmd_info(name);

            if (len != 0 && len != 2) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }
            if (len == 0) {
                id = 0;
                cs = 0;
            } else {
                id = param[0];
                cs = param[1];
            }

            get_flash_unique_id(id, cs, &ret_uid[1], (sizeof(ret_uid) - 1));
            TRACE(0, "GET_FLASH_UNIQUE_ID:");
            DUMP8("%02X ", &ret_uid[1], (sizeof(ret_uid) - 1));

            trace_flash_cmd_done(name);

            ret_uid[0] = ERR_NONE;
            send_reply(ret_uid, sizeof(ret_uid));
            break;
        }
#endif
        case FLASH_CMD_GET_SIZE: {
            uint8_t ret_size[1 + 4];
            unsigned int size;

            name = "CMD_GET_SIZE";

            trace_flash_cmd_info(name);

            if (len != 0 && len != 2) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }
            if (len == 0) {
                id = 0;
                cs = 0;
            } else {
                id = param[0];
                cs = param[1];
            }

            get_flash_size(id, cs, &size);
            TRACE(0, "GET_FLASH_SIZE: %u", size);

            trace_flash_cmd_done(name);

            ret_size[0] = ERR_NONE;
            ret_size[1] = size & 0xFF;
            ret_size[2] = (size >> 8) & 0xFF;
            ret_size[3] = (size >> 16) & 0xFF;
            ret_size[4] = (size >> 14) & 0xFF;
            send_reply(ret_size, sizeof(ret_size));
            break;
        }
#ifdef FLASH_SECURITY_REGISTER
        case FLASH_CMD_SEC_REG_READ: {
            unsigned int addr;
            unsigned int size;

            name = "SEC_READ";

            trace_flash_cmd_info(name);

            if (len != 5) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }

            addr = param[0] | (param[1] << 8) | (param[2] << 16) | (param[3] << 24);
            size = param[4];
            TRACE(2, "addr=0x%08X size=%u", addr, size);

            ret = handle_read_cmd(TYPE_FLASH_CMD | (cmd << 8), addr, size);
            if (ret) {
                return ret;
            }
            break;
        }
        case FLASH_CMD_SEC_REG_ERASE:
        case FLASH_CMD_SEC_REG_LOCK:
#endif
        case FLASH_CMD_ERASE_SECTOR: {
            unsigned int addr;
            unsigned int size;

            if (cmd == FLASH_CMD_ERASE_SECTOR) {
                name = "ERASE_SECTOR";
#ifdef FLASH_SECURITY_REGISTER
            } else if (cmd == FLASH_CMD_SEC_REG_ERASE) {
                name = "SEC_ERASE";
            } else {
                name = "SEC_LOCK";
#endif
            }

            trace_flash_cmd_info(name);

            if (len != 8) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }

            addr = param[0] | (param[1] << 8) | (param[2] << 16) | (param[3] << 24);
            size = param[4] | (param[5] << 8) | (param[6] << 16) | (param[7] << 24);
            TRACE(2, "addr=0x%08X size=%u", addr, size);

            if (cmd == FLASH_CMD_ERASE_SECTOR) {
                ret = erase_sector(addr, size);
#ifdef FLASH_SECURITY_REGISTER
            } else if (cmd == FLASH_CMD_SEC_REG_ERASE) {
                ret = erase_security_register(addr, size);
            } else {
                ret = lock_security_register(addr, size);
#endif
            }

            if (ret) {
                trace_flash_cmd_err(name);
                return ERR_ERASE_FLSH;
            }

            trace_flash_cmd_done(name);

            send_reply(&cret, 1);
            break;
        }
#ifdef FLASH_SECURITY_REGISTER
        case FLASH_CMD_SEC_REG_BURN:
#endif
        case FLASH_CMD_BURN_DATA: {
            unsigned int addr;

            if (cmd == FLASH_CMD_BURN_DATA) {
                name = "BURN_DATA";
#ifdef FLASH_SECURITY_REGISTER
            } else {
                name = "SEC_BURN";
#endif
            }

            trace_flash_cmd_info(name);

            if (len <= 4 || len > 20) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }

            addr = param[0] | (param[1] << 8) | (param[2] << 16) | (param[3] << 24);
            TRACE(2, "addr=0x%08X len=%u", addr, len - 4);

            if (cmd == FLASH_CMD_BURN_DATA) {
                ret = burn_data(addr, &param[4], len - 4);
#ifdef FLASH_SECURITY_REGISTER
            } else {
                ret = burn_security_register(addr, &param[4], len - 4);
#endif
            }

            if (ret) {
                trace_flash_cmd_err(name);
                return ERR_BURN_FLSH;
            }

            TRACE_TIME(1, "%s verifying", name);

            if (cmd == FLASH_CMD_BURN_DATA) {
                ret = verify_flash_data(addr, &param[4], len - 4);
#ifdef FLASH_SECURITY_REGISTER
            } else {
                ret = verify_security_register_data(addr, &data_buf[0][0], &param[4], len - 4);
#endif
            }
            if (ret) {
                TRACE(1, "%s verify failed", name);
                return ERR_VERIFY_FLSH;
            }

#if defined(SECURE_BOOT_WORKAROUND_SOLUTION) && defined(PROGRAMMER_INFLASH)
            programmer_timeout_clr_single_mode_upgrade_from_ota_app_boot_info();
#endif

            trace_flash_cmd_done(name);

            send_reply(&cret, 1);
            break;
        }
        case FLASH_CMD_ERASE_CHIP: {
            name = "CMD_ERASE_CHIP";

            trace_flash_cmd_info(name);

            if (len != 0 && len != 2) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }
            if (len == 0) {
                id = 0;
                cs = 0;
            } else {
                id = param[0];
                cs = param[1];
            }

            ret = erase_chip(id, cs);
            if (ret) {
                trace_flash_cmd_err(name);
                return ERR_ERASE_FLSH;
            }

            trace_flash_cmd_done(name);

            send_reply(&cret, 1);
            break;
        }
#ifdef FLASH_REMAP
        case FLASH_CMD_ENABLE_REMAP: {
            unsigned int remap_addr, remap_len, remap_offset;
            unsigned char burn_both;

            name = "CMD_ENABLE_REMAP";

            trace_flash_cmd_info(name);

            if (len != 13) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }

            remap_addr = param[0] | (param[1] << 8) | (param[2] << 16) | (param[3] << 24);
            remap_len = param[4] | (param[5] << 8) | (param[6] << 16) | (param[7] << 24);
            remap_offset = param[8] | (param[9] << 8) | (param[10] << 16) | (param[11] << 24);
            burn_both = param[12];
            TRACE(5, "remap_addr=0x%08X remap_len=0x%08X/%u remap_offset=0x%08X burn_both=%d", remap_addr, remap_len, remap_len, remap_offset, burn_both);

            ret = enable_flash_remap(remap_addr, remap_len, remap_offset, burn_both);
            if (ret) {
                trace_flash_cmd_err(name);
                return ERR_INTERNAL;
            }

            trace_flash_cmd_done(name);

            send_reply(&cret, 1);
            break;
        }
        case FLASH_CMD_DISABLE_REMAP: {
            name = "CMD_DISABLE_REMAP";

            trace_flash_cmd_info(name);

            if (len != 0) {
                trace_flash_cmd_len_err(name, len);
                return ERR_LEN;
            }

            ret = disable_flash_remap();
            if (ret) {
                trace_flash_cmd_err(name);
                return ERR_INTERNAL;
            }

            trace_flash_cmd_done(name);

            send_reply(&cret, 1);
            break;
        }
#endif
        default:
            TRACE(1, "Unsupported flash cmd: 0x%x", cmd);
            return ERR_FLASH_CMD;
    }

    return ERR_NONE;
}

static enum ERR_CODE handle_sector_info_cmd(unsigned int addr)
{
    unsigned int sector_addr;
    unsigned int sector_len;
    unsigned char buf[9];
    int ret;

    ret = get_sector_info(addr, &sector_addr, &sector_len);
    if (ret) {
        return ERR_DATA_ADDR;
    }

    TRACE(3, "addr=0x%08X sector_addr=0x%08X sector_len=%u", addr, sector_addr, sector_len);

    buf[0] = ERR_NONE;
    memcpy(&buf[1], &sector_addr, 4);
    memcpy(&buf[5], &sector_len, 4);

    send_reply(buf, 9);

    return ERR_NONE;
}

static void trace_stage_info(const char *name)
{
    TRACE_TIME(1, "------ %s ------", name);
}

static enum ERR_CODE handle_data(unsigned char **buf, size_t *len, int *extra)
{
    enum MSG_TYPE type;
    unsigned char cret = ERR_NONE;
    enum ERR_CODE errcode;
    struct CUST_CMD_PARAM param;

    *extra = 0;

    // Checksum
    if (check_sum((unsigned char *)&recv_msg, MSG_TOTAL_LEN(&recv_msg)) != 0xFF) {
        TRACE(0, "Checksum error");
        return ERR_CHECKSUM;
    }

    type = recv_msg.hdr.type;

    switch (type) {
        case TYPE_SYS: {
            trace_stage_info("SYS CMD");
            errcode = handle_sys_cmd((enum SYS_CMD_TYPE)recv_msg.data[0], &recv_msg.data[1], recv_msg.hdr.len - 1);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        case TYPE_READ: {
            unsigned int addr;
            unsigned int len;

            trace_stage_info("READ CMD");

            addr = recv_msg.data[0] | (recv_msg.data[1] << 8) | (recv_msg.data[2] << 16) | (recv_msg.data[3] << 24);
            len = recv_msg.data[4];

            errcode = handle_read_cmd(type, addr, len);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        case TYPE_WRITE: {
            unsigned int addr;
            unsigned int len;
            unsigned char *wdata;

            trace_stage_info("WRITE CMD");

            addr = recv_msg.data[0] | (recv_msg.data[1] << 8) | (recv_msg.data[2] << 16) | (recv_msg.data[3] << 24);
            len = recv_msg.hdr.len - 4;
            wdata = &recv_msg.data[4];

            errcode = handle_write_cmd(addr, len, wdata);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        case TYPE_BULK_READ: {
            unsigned int addr;
            unsigned int len;

            trace_stage_info("BULK READ CMD");

            addr = recv_msg.data[0] | (recv_msg.data[1] << 8) | (recv_msg.data[2] << 16) | (recv_msg.data[3] << 24);
            len = recv_msg.data[4] | (recv_msg.data[5] << 8) | (recv_msg.data[6] << 16) | (recv_msg.data[7] << 24);

            errcode = handle_bulk_read_cmd(addr, len);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
#ifdef FLASH_SECURITY_REGISTER
        case TYPE_SEC_REG_ERASE_BURN_START:
#endif
        case TYPE_ERASE_BURN_START: {
            if (type == TYPE_ERASE_BURN_START) {
                trace_stage_info("ERASE_BURN_START");
#ifdef FLASH_SECURITY_REGISTER
            } else {
                trace_stage_info("SEC_ERASE_BURN_START");
#endif
            }

            burn_addr = recv_msg.data[0] | (recv_msg.data[1] << 8) | (recv_msg.data[2] << 16) | (recv_msg.data[3] << 24);
            burn_len = recv_msg.data[4] | (recv_msg.data[5] << 8) | (recv_msg.data[6] << 16) | (recv_msg.data[7] << 24);
            sector_size = recv_msg.data[8] | (recv_msg.data[9] << 8) | (recv_msg.data[10] << 16) | (recv_msg.data[11] << 24);

            TRACE(3, "burn_addr=0x%08X burn_len=%u sector_size=%u", burn_addr, burn_len, sector_size);

            if (type == TYPE_ERASE_BURN_START) {
                if ((size_mask & sector_size) == 0 || count_set_bits(sector_size) != 1) {
                    TRACE(2, "Unsupported sector_size=0x%08X mask=0x%08X", sector_size, size_mask);
                    return ERR_SECTOR_SIZE;
                }

                sector_cnt = burn_len / sector_size;
                last_sector_len = burn_len % sector_size;
                if (last_sector_len) {
                    sector_cnt++;
                } else {
                    last_sector_len = sector_size;
                }

                if (sector_cnt > 0xFFFF) {
                    TRACE(1, "Sector seq overflow: %u", sector_cnt);
                    return ERR_SECTOR_SEQ_OVERFLOW;
                }
            } else {
                if (burn_len > MAX_SEC_REG_BURN_LEN) {
                    TRACE(1, "Bad SEC burn len (should <= %u)", MAX_SEC_REG_BURN_LEN);
                    return ERR_SECTOR_DATA_LEN;
                }
            }

            send_reply(&cret, 1);

            if (burn_len == 0) {
                TRACE(0, "Burn length = 0");
                break;
            }

            reset_data_buf_state();

            if (type == TYPE_ERASE_BURN_START) {
                programmer_state = PROGRAMMER_ERASE_BURN_START;
#ifdef FLASH_SECURITY_REGISTER
            } else {
                programmer_state = PROGRAMMER_SEC_REG_ERASE_BURN_START;
#endif
            }

            *extra = 1;
            *buf = &data_buf[0][0];
            if (type == TYPE_ERASE_BURN_START) {
                *len = BURN_DATA_MSG_OVERHEAD + ((sector_cnt == 1) ? last_sector_len : sector_size);
                extra_buf_len = sizeof(data_buf[0]);
#ifdef FLASH_SECURITY_REGISTER
            } else {
                *len = BURN_DATA_MSG_OVERHEAD + burn_len;
                extra_buf_len = sizeof(data_buf[0]) / 2;
#endif
            }
            recv_extra_timeout = get_pgm_timeout(PGM_TIMEOUT_RECV_SHORT) + (*len + 4096 - 1) / 4096 * get_pgm_timeout(PGM_TIMEOUT_RECV_4K_DATA);
            break;
        }
        case TYPE_FLASH_CMD: {
            trace_stage_info("FLASH CMD");

            errcode = handle_flash_cmd((enum FLASH_CMD_TYPE)recv_msg.data[0], &recv_msg.data[1], recv_msg.hdr.len - 1);

            TRACE_TIME(1, "FLASH CMD ret=%d", errcode);

            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        case TYPE_GET_SECTOR_INFO: {
            unsigned int addr;

            trace_stage_info("GET SECTOR INFO");

            addr = recv_msg.data[0] | (recv_msg.data[1] << 8) | (recv_msg.data[2] << 16) | (recv_msg.data[3] << 24);

            errcode = handle_sector_info_cmd(addr);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        default:
            param.stage = CUST_CMD_STAGE_DATA;
            param.msg = &recv_msg;
            param.extra = 0;
            errcode = handle_cust_cmd(&param);
            if (errcode == ERR_NONE && param.extra) {
                *extra = 1;
                *buf = param.buf;
                *len = param.expect;
                extra_buf_len = param.size;
                recv_extra_timeout = param.timeout;
            }
            return errcode;
    }

    return ERR_NONE;
}

static enum ERR_CODE check_extra_msg(const char *name, enum MSG_TYPE type, unsigned char *data)
{
    struct message_t *msg = (struct message_t *)data;
    uint8_t hdr_len;
    uint8_t check_len;

    hdr_len = (BURN_DATA_MSG_OVERHEAD - (sizeof(struct msg_hdr_t) + 1));
    check_len = BURN_DATA_MSG_OVERHEAD;

    if (msg->hdr.prefix != PREFIX_CHAR) {
        TRACE(1, "Invalid prefix char: 0x%x", msg->hdr.prefix);
        return ERR_SYNC_WORD;
    }
    if (msg->hdr.type != type) {
        TRACE(2, "Invalid %s msg type: 0x%x", name, msg->hdr.type);
        return ERR_TYPE_INVALID;
    }
    if (msg->hdr.len != hdr_len) {
        TRACE(2, "%s msg length error: %u", name, msg->hdr.len);
        return ERR_LEN;
    }
    if (check_sum(data, check_len) != 0xFF) {
        TRACE(1, "%s msg checksum error", name);
        return ERR_CHECKSUM;
    }

    return ERR_NONE;
}

static enum ERR_CODE handle_extra(unsigned char **buf, size_t *len, int *extra)
{
    enum MSG_TYPE type;
    enum ERR_CODE errcode = ERR_NONE;
    enum MSG_TYPE check_type = TYPE_INVALID;
    int ret;
    struct CUST_CMD_PARAM param;
    const char *name = NULL;

    *extra = 0;

    type = recv_msg.hdr.type;

    switch (type) {
#ifdef FLASH_SECURITY_REGISTER
        case TYPE_SEC_REG_ERASE_BURN_START:
#endif
        case TYPE_ERASE_BURN_START: {
            unsigned int dlen;
            unsigned int mcrc;
            unsigned int crc;

            if (programmer_state == PROGRAMMER_ERASE_BURN_START) {
                name = "ERASE_BURN_DATA";
                check_type = TYPE_ERASE_BURN_DATA;
#ifdef FLASH_SECURITY_REGISTER
            } else {
                name = "SEC_ERASE_BURN_DATA";
                check_type = TYPE_SEC_REG_ERASE_BURN_DATA;
#endif
            }

            trace_stage_info(name);

            errcode = check_extra_msg(name, check_type, (unsigned char *)&data_buf[cur_data_buf_idx][0]);
            if (errcode != ERR_NONE) {
                send_burn_data_reply(errcode, 0, data_buf[cur_data_buf_idx][2]);
                return ERR_NONE;
            }

            dlen = data_buf[cur_data_buf_idx][4] | (data_buf[cur_data_buf_idx][5] << 8) | (data_buf[cur_data_buf_idx][6] << 16)
                | (data_buf[cur_data_buf_idx][7] << 24);
            mcrc = data_buf[cur_data_buf_idx][8] | (data_buf[cur_data_buf_idx][9] << 8) | (data_buf[cur_data_buf_idx][10] << 16)
                | (data_buf[cur_data_buf_idx][11] << 24);
            cur_sector_seq = data_buf[cur_data_buf_idx][12] | (data_buf[cur_data_buf_idx][13] << 8);
            TRACE(4, "%s: sec_seq=%u dlen=%u cur_data_buf_idx=%u", name, cur_sector_seq, dlen, cur_data_buf_idx);

            if (type == TYPE_ERASE_BURN_START) {
                if (cur_sector_seq >= sector_cnt) {
                    TRACE(3, "%s: Bad sector seq: sec_seq=%u sector_cnt=%u", name, cur_sector_seq, sector_cnt);
                    send_burn_data_reply(ERR_SECTOR_SEQ, cur_sector_seq, data_buf[cur_data_buf_idx][2]);
                    return ERR_NONE;
                }

                if (((cur_sector_seq + 1) == sector_cnt && dlen != last_sector_len) || ((cur_sector_seq + 1) != sector_cnt && dlen != sector_size)) {
                    TRACE(3, "%s: Bad data len: sec_seq=%u dlen=%u", name, cur_sector_seq, dlen);
                    send_burn_data_reply(ERR_SECTOR_DATA_LEN, cur_sector_seq, data_buf[cur_data_buf_idx][2]);
                    return ERR_NONE;
                }
#ifdef FLASH_SECURITY_REGISTER
            } else {
                if (dlen != burn_len) {
                    TRACE(2, "%s: Bad data len: dlen=%u", name, dlen);
                    send_burn_data_reply(ERR_SECTOR_DATA_LEN, cur_sector_seq, data_buf[cur_data_buf_idx][2]);
                    return ERR_NONE;
                }
#endif
            }

            crc = crc32_c(0, (unsigned char *)&data_buf[cur_data_buf_idx][BURN_DATA_MSG_OVERHEAD], dlen);
            if (crc != mcrc) {
                TRACE(1, "%s: Bad CRC", name);
                send_burn_data_reply(ERR_SECTOR_DATA_CRC, cur_sector_seq, data_buf[cur_data_buf_idx][2]);
                return ERR_NONE;
            }

            if (type == TYPE_ERASE_BURN_START) {
                burn_data_buf(cur_data_buf_idx);

                if (cur_sector_seq + 1 == sector_cnt) {
                    ret = wait_all_data_buf_finished();
                    if (ret) {
                        TRACE(1, "%s: Waiting all data buffer free failed", name);
                        return ERR_INTERNAL;
                    }
                    // Reset state
                    programmer_state = PROGRAMMER_NONE;
                } else {
                    ret = get_free_data_buf();
                    if (ret) {
                        TRACE(1, "%s: Getting free data buffer failed", name);
                        return ERR_INTERNAL;
                    }

                    TRACE_TIME(2, "%s: Recv next buffer. cur_data_buf_idx=%u", name, cur_data_buf_idx);

                    *extra = 1;
                    *buf = (unsigned char *)&data_buf[cur_data_buf_idx][0];
                    *len = BURN_DATA_MSG_OVERHEAD + ((cur_sector_seq + 2 == sector_cnt) ? last_sector_len : sector_size);
                    extra_buf_len = sizeof(data_buf[cur_data_buf_idx]);
                    recv_extra_timeout = get_pgm_timeout(PGM_TIMEOUT_RECV_SHORT) + (*len + 4096 - 1) / 4096 * get_pgm_timeout(PGM_TIMEOUT_RECV_4K_DATA);
                }
#ifdef FLASH_SECURITY_REGISTER
            } else {
                TRACE_TIME(1, "%s erasing", name);
                ret = erase_security_register(burn_addr, burn_len);
                if (ret) {
                    trace_flash_cmd_err("SEC_ERASE/EXTRA");
                    send_burn_data_reply(ERR_ERASE_FLSH, 0, data_buf[0][2]);
                    return ERR_NONE;
                }

                TRACE_TIME(1, "%s burning", name);
                ret = burn_security_register(burn_addr, &data_buf[0][BURN_DATA_MSG_OVERHEAD], burn_len);
                if (ret) {
                    trace_flash_cmd_err("SEC_BURN/EXTRA");
                    send_burn_data_reply(ERR_BURN_FLSH, 0, data_buf[0][2]);
                    return ERR_NONE;
                }

                TRACE_TIME(1, "%s verifying", name);
                ret = verify_security_register_data(burn_addr, &data_buf[0][BURN_DATA_MSG_OVERHEAD + burn_len], &data_buf[0][BURN_DATA_MSG_OVERHEAD], burn_len);
                if (ret) {
                    trace_flash_cmd_err("SEC_VERIFY/EXTRA");
                    send_burn_data_reply(ERR_VERIFY_FLSH, 0, data_buf[0][2]);
                    return ERR_NONE;
                }

                TRACE_TIME(1, "%s done", name);
                send_burn_data_reply(ERR_BURN_OK, 0, data_buf[0][2]);
                // Reset state
                programmer_state = PROGRAMMER_NONE;
#endif
            }
            break;
        }
        default:
            param.stage = CUST_CMD_STAGE_EXTRA;
            param.msg = &recv_msg;
            param.extra = 0;
            errcode = handle_cust_cmd(&param);
            if (errcode == ERR_NONE && param.extra) {
                *extra = 1;
                *buf = param.buf;
                *len = param.expect;
                extra_buf_len = param.size;
                recv_extra_timeout = param.timeout;
            }
            return errcode;
    }
    return ERR_NONE;
}

static int parse_packet(unsigned char **buf, size_t *len)
{
    enum ERR_CODE errcode;
    int rlen = *len;
    unsigned char *data;
    int i;
    int extra;
    unsigned char cret;

    switch (parse_state) {
        case PARSE_HEADER:
            ASSERT(rlen > 0 && rlen <= sizeof(recv_msg.hdr), "Invalid rlen!");

            if (recv_msg.hdr.prefix == PREFIX_CHAR) {
                errcode = check_msg_hdr();
                if (errcode != ERR_NONE) {
                    goto _err;
                }
                parse_state = PARSE_DATA;
                *buf = &recv_msg.data[0];
                *len = recv_msg.hdr.len + 1;
            } else {
                data = (unsigned char *)&recv_msg.hdr.prefix;
                for (i = 1; i < rlen; i++) {
                    if (data[i] == PREFIX_CHAR) {
                        memmove(&recv_msg.hdr.prefix, &data[i], rlen - i);
                        break;
                    }
                }
                *buf = &data[rlen - i];
                *len = sizeof(recv_msg.hdr) + i - rlen;
            }
            break;
        case PARSE_DATA:
        case PARSE_EXTRA:
            if (parse_state == PARSE_DATA) {
                errcode = handle_data(buf, len, &extra);
            } else {
                errcode = handle_extra(buf, len, &extra);
            }
            if (errcode != ERR_NONE) {
                goto _err;
            }
            if (extra) {
                parse_state = PARSE_EXTRA;
            } else {
                // Receive next message
                reset_parse_state(buf, len);
            }
            break;
        default:
            TRACE(1, "Invalid parse_state: %d", parse_state);
            break;
    }

    return 0;

_err:
    cancel_input();
    cret = (unsigned char)errcode;
    send_reply(&cret, 1);

    return 1;
}

void programmer_loop(void)
{
    int ret;
    unsigned char *buf = NULL;
    size_t len = 0;
    size_t buf_len, rlen;

#if defined(PROGRAMMER_INFLASH)
    unsigned int sync_start_time = get_current_time();
#endif

    trace_stage_info("Init download context");

    init_download_context();
    init_cust_cmd();

_sync:
    reset_transport();
    reset_programmer_state(&buf, &len);

    trace_stage_info("Send SECTOR SIZE msg");

    ret = send_sector_size_msg();
    if (ret) {
        TRACE(0, "Sending SECTOR SIZE msg failed");
        goto _err;
    }

    while (1) {
        rlen = 0;
        if (parse_state == PARSE_EXTRA) {
            set_recv_timeout(recv_extra_timeout);
            buf_len = extra_buf_len;
        } else {
            if (programmer_state == PROGRAMMER_NONE && parse_state == PARSE_HEADER) {
                set_recv_timeout(get_pgm_timeout(PGM_TIMEOUT_RECV_IDLE));
            } else {
                set_recv_timeout(get_pgm_timeout(PGM_TIMEOUT_RECV_SHORT));
            }
            buf_len = 0;
        }
        ret = recv_data_ex(buf, buf_len, len, &rlen);
        if (ret) {
            TRACE(1, "Receiving data failed: %d", ret);
            goto _err;
        }
        //dump_buffer(buf, rlen);
        if (len != rlen) {
            TRACE(2, "Receiving part of the data: expect=%u real=%u", len, rlen);
            goto _err;
        }

        ret = parse_packet(&buf, &len);
        if (ret) {
            TRACE(0, "Parsing packet failed");
            goto _err;
        }
    }

_err:
#if defined(PROGRAMMER_INFLASH)
    if (programmer_state == PROGRAMMER_NONE) {
        if (get_current_time() - sync_start_time > get_pgm_timeout(PGM_TIMEOUT_SYNC)) {
            // Timeout when waiting for the tool
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
            programmer_timeout_clr_single_mode_upgrade_from_ota_app_boot_info();
#endif
            goto _exit;
        }
    }
#endif

    if (programmer_state == PROGRAMMER_ERASE_BURN_START) {
        ret = abort_all_data_buf();
        if (ret) {
            TRACE(1, "LOOP_ERR: Failed to abort all data buffer");
        }
    }

    ret = handle_error();
    if (ret == 0) {
        TRACE(0, "Start SECTOR SIZE ...");
        goto _sync;
    }

#if defined(PROGRAMMER_INFLASH)
_exit:
#endif
    return;
}
