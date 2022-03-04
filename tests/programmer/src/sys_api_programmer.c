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
#include "sys_api_programmer.h"
#include "cmsis_nvic.h"
#include "export_fn_rom.h"
#include "flash_programmer.h"
#include "hal_bootmode.h"
#include "hal_cache.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_iomux.h"
#include "hal_norflash.h"
#include "hal_sysfreq.h"
#include "hal_uart.h"
#include "hal_wdt.h"
#include "hwtimer_list.h"
#include "plat_addr_map.h"
#include "pmu.h"
#include "stdint.h"
#include "string.h"
#include "task_schedule.h"
#include "tool_msg.h"
#ifdef CHIP_HAS_USB
#include "usb_cdc.h"
#endif
#ifdef __ARMCC_VERSION
#include "link_sym_armclang.h"
#endif
#include "hal_key.h"
#include "tgt_hardware.h"

#define WATCHDOG_SECONDS     (10)
#define PGM_TIMEOUT_INFINITE ((uint32_t)-1)

#ifdef UART_DEVICE_UART2
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_2;
#else
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_1;
#endif
extern uint32_t __download_uart_bandrate__[];

extern int ProgrammerInflashEnterApp(void);

enum PGM_DOWNLOAD_TRANSPORT {
    PGM_TRANSPORT_USB,
    PGM_TRANSPORT_UART,
};

enum UART_DMA_STATE {
    UART_DMA_IDLE,
    UART_DMA_START,
    UART_DMA_DONE,
    UART_DMA_ERROR,
};

static void setup_download_task(void);
static void setup_flash_task(void);

static const unsigned int default_recv_timeout_short = MS_TO_TICKS(500);
static const unsigned int default_recv_timeout_idle =
#if defined(PROGRAMMER_INFLASH)
    MS_TO_TICKS(1000);
#else
    PGM_TIMEOUT_INFINITE;
#endif
static const unsigned int default_recv_timeout_4k_data = MS_TO_TICKS(500);
static const unsigned int default_send_timeout = MS_TO_TICKS(500);

static const uint32_t default_sync_timeout =
#ifdef SINGLE_WIRE_DOWNLOAD
    MS_TO_TICKS(60000);
#else
    MS_TO_TICKS(1 * 60 * 1000);
#endif

static const uint32_t uart_baud_cfg_timeout = MS_TO_TICKS(10);

static uint32_t send_timeout;
static uint32_t recv_timeout;

static struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE,   //HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
    .baud = 921600,
    .dma_rx = true,
    .dma_tx = false,
    .dma_rx_stop_on_err = false,
};

static const enum HAL_UART_ID_T dld_uart =
#if (CHIP_HAS_UART >= 2) && defined(SINGLE_WIRE_DOWNLOAD)
    HAL_UART_ID_1;
#else
    HAL_UART_ID_0;
#endif

static volatile enum UART_DMA_STATE uart_dma_rx_state = UART_DMA_IDLE;
static volatile uint32_t uart_dma_rx_size = 0;
static uint32_t uart_baud;
static bool uart_baud_cfg = false;
static bool uart_opened = false;

#ifdef CHIP_HAS_USB
static bool usb_opened = false;
static HWTIMER_ID usb_send_timerid = NULL;
static HWTIMER_ID usb_rcv_timerid = NULL;
#endif

static volatile bool cancel_xfer = false;
static enum PGM_DOWNLOAD_TRANSPORT dld_transport;

#ifdef FLASH_REMAP
static bool burn_remap_both;
#endif

#ifdef CHIP_HAS_USB
static void usb_recv_timeout(uint32_t elapsed);
static void usb_send_timeout(uint32_t elapsed);
#endif

static void uart_break_handler(void)
{
    TRACE(0, "****** Handle break ******");
    cancel_xfer = true;
}

static void uart_rx_dma_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    if (status.BE) {
        uart_break_handler();
        return;
    }

    // The DMA transfer has been cancelled
    if (uart_dma_rx_state != UART_DMA_START) {
        return;
    }

    uart_dma_rx_size = xfer_size;
    if (dma_error || status.FE || status.OE || status.PE || status.RT || status.BE) {
        TRACE(2, "UART-RX Error: xfer_size=%u dma_error=%d, status=0x%08x", xfer_size, dma_error, status.reg);
        uart_dma_rx_state = UART_DMA_ERROR;
    } else {
        uart_dma_rx_state = UART_DMA_DONE;
    }
}

#ifdef CHIP_HAS_USB
static void usb_serial_break_handler(uint16_t ms)
{
    if (ms) {
        TRACE(0, "****** Handle break ******");

        cancel_xfer = true;
        hwtimer_stop(usb_rcv_timerid);
        usb_serial_cancel_recv();
        usb_serial_cancel_send();
    }
}

static void reopen_usb_serial(void)
{
    usb_serial_reopen(usb_serial_break_handler);
}
#endif

int send_debug_event(const void *buf, size_t len)
{
    int ret;

    // TODO:
    // 1) Transport must have been initialized.
    // 2) If debug events can be sent in flash task, task scheduler should be locked
    //    to avoid conflicts with main task.
    //    Moreover, the global sending msg should be protected too.

    ret = send_notif_msg(buf, len);

    if (dld_transport == PGM_TRANSPORT_UART) {
        // Make sure the data in uart fifo has been sent out
        // (transport/iomux might be reset later)
        hal_sys_timer_delay(MS_TO_TICKS(1));
    }

    return ret;
}

static void init_flash(void)
{
    int ret;
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    uint8_t event[HAL_NORFLASH_DEVICE_ID_LEN + 1];

    static const struct HAL_NORFLASH_CONFIG_T cfg = {
#ifdef FPGA
        .source_clk = HAL_NORFLASH_SPEED_13M * 2,
        .speed = HAL_NORFLASH_SPEED_13M,
#else
#if (defined(CHIP_BEST1400) || defined(CHIP_BEST1402)) && !defined(FLASH_DTR)
        .source_clk = HAL_NORFLASH_SPEED_52M,
        .speed = HAL_NORFLASH_SPEED_52M,
#else
        .source_clk = HAL_NORFLASH_SPEED_104M * 2,
        .speed = HAL_NORFLASH_SPEED_104M,
#endif
#endif
        .mode = HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI | HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO
            | HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
#ifdef FLASH_DTR
            HAL_NORFLASH_OP_MODE_DTR |
#endif
            HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM
            | HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | 0,
        .override_config = 0,
    };

    // Workaround for reboot: controller in SPI mode while FLASH in QUAD mode
    *(volatile uint32_t *)FLASH_NC_BASE;

    ret = hal_norflash_open(HAL_FLASH_ID_0, &cfg);

    hal_norflash_get_id(HAL_FLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TRACE(3, "FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
    hal_norflash_show_calib_result(HAL_FLASH_ID_0);

    if (ret) {
        event[0] = ret;
        memcpy(&event[1], &flash_id[0], sizeof(flash_id));
        send_debug_event(&event[0], sizeof(event));
    }

    ASSERT(ret == 0, "Failed to open norflash: %d", ret);
}

static void set_pgm_download_transport(enum PGM_DOWNLOAD_TRANSPORT transport)
{
    union HAL_UART_IRQ_T mask;

    dld_transport = transport;

    if (transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
        if (!usb_opened) {
            if (usb_send_timerid == NULL) {
                usb_send_timerid = hwtimer_alloc((HWTIMER_CALLBACK_T)usb_send_timeout, NULL);
            }
            if (usb_rcv_timerid == NULL) {
                usb_rcv_timerid = hwtimer_alloc((HWTIMER_CALLBACK_T)usb_recv_timeout, NULL);
            }
            reopen_usb_serial();
            usb_opened = true;
        }
#endif
    } else if (transport == PGM_TRANSPORT_UART) {
        if (!uart_opened) {
            mask.reg = 0;
#ifdef SINGLE_WIRE_DOWNLOAD
            mask.FE = 1;
            mask.OE = 1;
            mask.PE = 1;
            uart_cfg.baud = *__download_uart_bandrate__;
            TRACE(1, "uart baud=%d", uart_cfg.baud);
            hal_uart_open(dld_uart, &uart_cfg);
            hal_iomux_single_wire_uart_rx(comm_uart);
#else
            mask.BE = 1;
            hal_uart_reopen(dld_uart, &uart_cfg);
#endif
            hal_uart_irq_set_dma_handler(dld_uart, uart_rx_dma_handler, NULL);
            hal_uart_irq_set_mask(dld_uart, mask);
            uart_opened = true;
        }
    }
}

static int uart_download_enabled(void)
{
#ifdef SINGLE_WIRE_DOWNLOAD
    return true;
#endif
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_DLD_TRANS_UART);
}

static void init_download_transport(void)
{
    enum PGM_DOWNLOAD_TRANSPORT transport;

    transport = uart_download_enabled() ? PGM_TRANSPORT_UART : PGM_TRANSPORT_USB;
    set_pgm_download_transport(transport);
}

void init_download_context(void)
{
    init_download_transport();
    setup_download_task();
    setup_flash_task();
    init_flash();
}

void init_cust_cmd(void)
{
    extern uint32_t __cust_cmd_init_tbl_start[];
    extern uint32_t __cust_cmd_init_tbl_end[];

    CUST_CMD_INIT_T *start = (CUST_CMD_INIT_T *)__cust_cmd_init_tbl_start;
    CUST_CMD_INIT_T *end = (CUST_CMD_INIT_T *)__cust_cmd_init_tbl_end;

    while (start < end) {
        (*start)();
        start++;
    }
}

enum ERR_CODE handle_cust_cmd(struct CUST_CMD_PARAM *param)
{
    enum ERR_CODE errcode = ERR_TYPE_INVALID;

    extern uint32_t __cust_cmd_hldr_tbl_start[];
    extern uint32_t __cust_cmd_hldr_tbl_end[];

    CUST_CMD_HANDLER_T *start = (CUST_CMD_HANDLER_T *)__cust_cmd_hldr_tbl_start;
    CUST_CMD_HANDLER_T *end = (CUST_CMD_HANDLER_T *)__cust_cmd_hldr_tbl_end;

    while (errcode == ERR_TYPE_INVALID && start < end) {
        errcode = (*start)(param);
        start++;
    }

    return errcode;
}

void reset_transport(void)
{
    cancel_xfer = false;

    if (dld_transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
        usb_serial_flush_recv_buffer();
        usb_serial_init_xfer();
#endif
    } else {
        hal_uart_flush(dld_uart, 0);
        uart_dma_rx_state = UART_DMA_IDLE;
    }

    set_recv_timeout(get_pgm_timeout(PGM_TIMEOUT_RECV_SHORT));
    set_send_timeout(get_pgm_timeout(PGM_TIMEOUT_SEND));
}

void set_recv_timeout(unsigned int timeout)
{
    recv_timeout = timeout;
}

void set_send_timeout(unsigned int timeout)
{
    send_timeout = timeout;
}

unsigned int get_pgm_timeout(enum PGM_DOWNLOAD_TIMEOUT timeout)
{
    if (timeout == PGM_TIMEOUT_RECV_SHORT) {
        return default_recv_timeout_short;
    } else if (timeout == PGM_TIMEOUT_RECV_IDLE) {
        return default_recv_timeout_idle;
    } else if (timeout == PGM_TIMEOUT_RECV_4K_DATA) {
        return default_recv_timeout_4k_data;
    } else if (timeout == PGM_TIMEOUT_SEND) {
        return default_send_timeout;
    } else if (timeout == PGM_TIMEOUT_SYNC) {
        return default_sync_timeout;
    } else {
        return 0;
    }
}

unsigned int get_current_time(void)
{
    return hal_sys_timer_get();
}

int debug_read_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_READ_ENABLED);
}

int debug_write_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_WRITE_ENABLED);
}

#ifdef CHIP_HAS_USB
static void usb_send_timeout(uint32_t elapsed)
{
    usb_serial_cancel_send();
}

static void usb_send_timer_start(void)
{
    if (send_timeout == PGM_TIMEOUT_INFINITE) {
        return;
    }
    hwtimer_start(usb_send_timerid, send_timeout);
}

static void usb_send_timer_stop(void)
{
    hwtimer_stop(usb_send_timerid);
}

static int usb_send_data(const unsigned char *buf, size_t len)
{
    int ret;

    usb_send_timer_start();
    ret = usb_serial_send(buf, len);
    usb_send_timer_stop();
    return ret;
}
#endif

static int uart_send_data(const unsigned char *buf, size_t len)
{
    uint32_t start;
    uint32_t sent = 0;

#ifdef SINGLE_WIRE_DOWNLOAD
    hal_iomux_single_wire_uart_tx(comm_uart);
#endif

    start = hal_sys_timer_get();
    while (sent < len) {
        while (!cancel_xfer && !hal_uart_writable(dld_uart) && (send_timeout == PGM_TIMEOUT_INFINITE || hal_sys_timer_get() - start < send_timeout)
               && TASK_SCHEDULE)
            ;
        if (cancel_xfer) {
            break;
        }
        if (hal_uart_writable(dld_uart)) {
            hal_uart_putc(dld_uart, buf[sent++]);
        } else {
            break;
        }
    }

#ifdef SINGLE_WIRE_DOWNLOAD
    while (hal_uart_get_flag(dld_uart).BUSY)
        ;
    hal_iomux_single_wire_uart_rx(comm_uart);
#endif

    if (sent != len) {
        return 1;
    }

    return 0;
}

int send_data(const unsigned char *buf, size_t len)
{
    int nRet;

    if (cancel_xfer) {
        nRet = -1;
        goto _exit;
    }

    if (dld_transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
        nRet = usb_send_data(buf, len);
#else
        nRet = 0;
#endif
    } else {
        nRet = uart_send_data(buf, len);
    }

_exit:
    return nRet;
}

#ifdef CHIP_HAS_USB
static void usb_recv_timeout(uint32_t elapsed)
{
    usb_serial_cancel_recv();
}

static void usb_recv_timer_start(void)
{
    if (recv_timeout == PGM_TIMEOUT_INFINITE) {
        return;
    }
    hwtimer_start(usb_rcv_timerid, recv_timeout);
}

static void usb_recv_timer_stop(void)
{
    hwtimer_stop(usb_rcv_timerid);
}

static int usb_recv_data(unsigned char *buf, size_t len, size_t *rlen)
{
    int ret;

    usb_recv_timer_start();
    ret = usb_serial_recv(buf, len);
    usb_recv_timer_stop();
    if (ret == 0) {
        *rlen = len;
    }
    return ret;
}

static int usb_recv_data_dma(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
    int ret;
    uint32_t recv_len;

    usb_recv_timer_start();
    ret = usb_serial_direct_recv(buf, len, expect, &recv_len, NULL);
    usb_recv_timer_stop();
    if (rlen) {
        *rlen = recv_len;
    }
    return ret;
}
#endif

static int uart_recv_data_dma(unsigned char *buf, size_t expect, size_t *rlen)
{
    int ret;
    union HAL_UART_IRQ_T mask;
    uint32_t start;
    struct HAL_DMA_DESC_T dma_desc[17];
    uint32_t desc_cnt = ARRAY_SIZE(dma_desc);

    if (uart_baud_cfg) {
        uart_baud_cfg = false;
        // TODO: Refactor rx codes to reduce the CPU freq requirement
        if (uart_baud <= 1625000) {
            hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_52M);
        } else {
            hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_104M);
        }
        start = hal_sys_timer_get();
        do {
            ret = hal_uart_change_baud_rate(dld_uart, uart_baud);
        } while (ret && (hal_sys_timer_get() - start < uart_baud_cfg_timeout));
    }

    if (uart_dma_rx_state != UART_DMA_IDLE) {
        ret = -3;
        goto _no_state_exit;
    }

    uart_dma_rx_state = UART_DMA_START;
    uart_dma_rx_size = 0;

    ret = hal_uart_dma_recv(dld_uart, buf, expect, &dma_desc[0], &desc_cnt);
    if (ret) {
        goto _exit;
    }

    mask.reg = 0;
#ifndef SINGLE_WIRE_DOWNLOAD
    mask.BE = 1;
    mask.FE = 1;
#endif
    mask.OE = 1;
    mask.PE = 1;
    hal_uart_irq_set_mask(dld_uart, mask);

    start = hal_sys_timer_get();
    while (uart_dma_rx_state == UART_DMA_START && !cancel_xfer && (recv_timeout == PGM_TIMEOUT_INFINITE || hal_sys_timer_get() - start < recv_timeout)
           && TASK_SCHEDULE)
        ;
    if (cancel_xfer) {
        ret = -14;
        goto _exit;
    }
    if (uart_dma_rx_state == UART_DMA_START) {
        ret = -15;
        goto _exit;
    }
    if (uart_dma_rx_state != UART_DMA_DONE) {
        ret = -2;
        goto _exit;
    }

    ret = 0;
    *rlen = uart_dma_rx_size;

_exit:
    mask.reg = 0;
#ifndef SINGLE_WIRE_DOWNLOAD
    mask.BE = 1;
#endif
    hal_uart_irq_set_mask(dld_uart, mask);

    if (uart_dma_rx_state != UART_DMA_DONE) {
        hal_uart_stop_dma_recv(dld_uart);
    }
    uart_dma_rx_state = UART_DMA_IDLE;

_no_state_exit:
    return ret;
}

int recv_data_ex(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
#ifdef SINGLE_WIRE_DOWNLOAD
    programmer_watchdog_ping();
#endif

    if (cancel_xfer) {
        return -1;
    }

    if (dld_transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
        if (len == 0) {
            return usb_recv_data(buf, expect, rlen);
        } else {
            return usb_recv_data_dma(buf, len, expect, rlen);
        }
#else
        return 0;
#endif
    } else {
        return uart_recv_data_dma(buf, expect, rlen);
    }
}

#ifdef CHIP_HAS_USB
static int usb_handle_error(void)
{
    int ret;

    TRACE(0, "****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    ret = usb_serial_send_break();
    if (ret) {
        TRACE(1, "Sending break failed: %d", ret);
    }
    return ret;
}
#endif

static int uart_handle_error(void)
{
    TRACE(0, "****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    cancel_output();
    hal_sys_timer_delay(MS_TO_TICKS(1));
    // Send a normal byte to ensure the peer can detect the following break signal
    if (hal_uart_writable(dld_uart)) {
        hal_uart_putc(dld_uart, 0);
        hal_sys_timer_delay(MS_TO_TICKS(2));
    }
    hal_uart_break_set(dld_uart);
    hal_sys_timer_delay(MS_TO_TICKS(2));
    hal_uart_break_clear(dld_uart);

    return 0;
}

int handle_error(void)
{
    int ret = 0;

    hal_sys_timer_delay(MS_TO_TICKS(50));

    if (!cancel_xfer) {
        if (dld_transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
            ret = usb_handle_error();
#endif
        } else {
            ret = uart_handle_error();
        }
    }

#ifdef PROGRAMMER_ERROR_ASSERT
    ASSERT(false, "Programmer Error");
#endif

    // Wait for the host to start the normal rx
    hal_sys_timer_delay(MS_TO_TICKS(100));

    return ret;
}

#ifdef CHIP_HAS_USB
static int usb_cancel_input(void)
{
    return usb_serial_flush_recv_buffer();
}
#endif

static int uart_cancel_transfer(void)
{
    hal_uart_flush(dld_uart, 0);
    return 0;
}

int cancel_input(void)
{
    if (dld_transport == PGM_TRANSPORT_USB) {
#ifdef CHIP_HAS_USB
        return usb_cancel_input();
#else
        return 0;
#endif
    } else {
        return uart_cancel_transfer();
    }
}

int cancel_output(void)
{
    if (dld_transport == PGM_TRANSPORT_USB) {
        return 0;
    } else {
        return uart_cancel_transfer();
    }
}

void system_reboot(void)
{
    hal_sys_timer_delay(MS_TO_TICKS(10));
    hal_cmu_sys_reboot();
}

void system_shutdown(void)
{
#if 0
    if (dld_transport == PGM_TRANSPORT_USB) {
        // Avoid PC usb serial driver hanging
        usb_serial_close();
    }
#endif
    hal_sys_timer_delay(MS_TO_TICKS(10));
    pmu_shutdown();
}

void system_flash_boot(void)
{
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FLASH_BOOT);
    hal_cmu_sys_reboot();
}

void system_set_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_set(bootmode);
}

void system_clear_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_clear(bootmode);
}

unsigned int system_get_bootmode(void)
{
    return hal_sw_bootmode_get();
}

void system_set_download_rate(unsigned int rate)
{
    uart_baud = rate;
    uart_baud_cfg = true;
}

int get_sector_info(unsigned int addr, unsigned int *sector_addr, unsigned int *sector_len)
{
    int ret;
    uint32_t base;
    uint32_t size;

    if (sector_addr) {
        ret = hal_norflash_get_boundary(HAL_FLASH_ID_0, addr, NULL, &base);
        if (ret) {
            return ret;
        }
        *sector_addr = base;
    }
    if (sector_len) {
        ret = hal_norflash_get_size(HAL_FLASH_ID_0, NULL, NULL, &size, NULL);
        *sector_len = size;
    }

    return 0;
}

int get_flash_id(unsigned char id, unsigned char cs, unsigned char *buf, unsigned int len)
{
    return hal_norflash_get_id(HAL_FLASH_ID_0, buf, len);
}

#ifdef FLASH_UNIQUE_ID
int get_flash_unique_id(unsigned char id, unsigned char cs, unsigned char *buf, unsigned int len)
{
    return hal_norflash_get_unique_id(HAL_FLASH_ID_0, buf, len);
}
#endif

int get_flash_size(unsigned char id, unsigned char cs, unsigned int *size)
{
    int ret;
    uint32_t total_size;

    ret = hal_norflash_get_size(HAL_FLASH_ID_0, &total_size, NULL, NULL, NULL);
    *size = total_size;

    return ret;
}

int erase_sector(unsigned int sector_addr, unsigned int sector_len)
{
    int ret;

    ret = hal_norflash_erase(HAL_FLASH_ID_0, sector_addr, sector_len);
#ifdef FLASH_REMAP
    if (ret == 0 && burn_remap_both) {
        hal_norflash_disable_remap(HAL_FLASH_ID_0);

        ret = hal_norflash_erase(HAL_FLASH_ID_0, sector_addr, sector_len);

        hal_norflash_enable_remap(HAL_FLASH_ID_0);
    }
#endif
    return ret;
}

int erase_chip(unsigned char id, unsigned char cs)
{
    return hal_norflash_erase_chip(HAL_FLASH_ID_0);
}

int burn_data(unsigned int addr, const unsigned char *data, unsigned int len)
{
    int ret;

    ret = hal_norflash_write(HAL_FLASH_ID_0, addr, data, len);
#ifdef FLASH_REMAP
    if (ret == 0 && burn_remap_both) {
        hal_norflash_disable_remap(HAL_FLASH_ID_0);

        ret = hal_norflash_write(HAL_FLASH_ID_0, addr, data, len);

        hal_norflash_enable_remap(HAL_FLASH_ID_0);
    }
#endif
    return ret;
}

int verify_flash_data(unsigned int addr, const unsigned char *data, unsigned int len)
{
    const unsigned char *fdata;
    const unsigned char *mdata;
    int i;

    fdata = (unsigned char *)addr;
    mdata = data;
    for (i = 0; i < len; i++) {
        if (*fdata++ != *mdata++) {
            --fdata;
            --mdata;
            TRACE(4, "*** Verify flash data failed: 0x%02X @ %p != 0x%02X @ %p", *fdata, fdata, *mdata, mdata);
            return *fdata - *mdata;
        }
    }

#ifdef FLASH_REMAP
    if (burn_remap_both) {
        hal_norflash_disable_remap(HAL_FLASH_ID_0);
        hal_cache_invalidate(HAL_CACHE_ID_I_CACHE, addr, len);
        hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, addr, len);

        fdata = (unsigned char *)addr;
        mdata = data;
        for (i = 0; i < len; i++) {
            if (*fdata++ != *mdata++) {
                --fdata;
                --mdata;
                TRACE(4, "*** Verify flash data2 failed: 0x%02X @ %p != 0x%02X @ %p", *fdata, fdata, *mdata, mdata);
                return *fdata - *mdata;
            }
        }

        hal_norflash_enable_remap(HAL_FLASH_ID_0);
        hal_cache_invalidate(HAL_CACHE_ID_I_CACHE, addr, len);
        hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, addr, len);
    }
#endif

    return 0;
}

int enable_flash_remap(unsigned int addr, unsigned int len, unsigned int offset, int burn_both)
{
#ifdef FLASH_REMAP
    int ret;

    burn_remap_both = false;
    ret = hal_norflash_config_remap(HAL_FLASH_ID_0, addr, len, offset);
    if (ret) {
        return ret;
    }
    ret = hal_norflash_enable_remap(HAL_FLASH_ID_0);
    if (ret) {
        return ret;
    }
    burn_remap_both = !!burn_both;
    return 0;
#else
    return 1;
#endif
}

int disable_flash_remap(void)
{
#ifdef FLASH_REMAP
    burn_remap_both = false;
    return hal_norflash_disable_remap(HAL_FLASH_ID_0);
#else
    return 1;
#endif
}

int wait_data_buf_finished(void)
{
    while (free_data_buf()) {
        task_yield();
    }

    return 0;
}

int wait_all_data_buf_finished(void)
{
    while (data_buf_in_use()) {
        while (free_data_buf() == 0)
            ;
        task_yield();
    }

    return 0;
}

int abort_all_data_buf(void)
{
    set_data_buf_abort_flag(1);
    while (data_buf_in_burn()) {
        task_yield();
    }
    set_data_buf_abort_flag(0);

    return 0;
}

#ifdef FLASH_SECURITY_REGISTER
int read_security_register(unsigned int addr, unsigned char *data, unsigned int len)
{
    return hal_norflash_security_register_read(HAL_FLASH_ID_0, addr, data, len);
}

int erase_security_register(unsigned int addr, unsigned int len)
{
    return hal_norflash_security_register_erase(HAL_FLASH_ID_0, addr, len);
}

int get_security_register_lock_status(unsigned int addr, unsigned int len)
{
    return hal_norflash_security_register_is_locked(HAL_FLASH_ID_0, addr, len);
}

int lock_security_register(unsigned int addr, unsigned int len)
{
    return hal_norflash_security_register_lock(HAL_FLASH_ID_0, addr, len);
}

int burn_security_register(unsigned int addr, const unsigned char *data, unsigned int len)
{
    return hal_norflash_security_register_write(HAL_FLASH_ID_0, addr, data, len);
}

int verify_security_register_data(unsigned int addr, unsigned char *load_buf, const unsigned char *data, unsigned int len)
{
    enum HAL_NORFLASH_RET_T ret;
    const unsigned char *fdata;
    const unsigned char *mdata;
    int i;

    ret = hal_norflash_security_register_read(HAL_FLASH_ID_0, addr, load_buf, len);
    if (ret) {
        TRACE(1, "*** Failed to read sec reg: %d", ret);
        return 1;
    }

    fdata = load_buf;
    mdata = data;
    for (i = 0; i < len; i++) {
        if (*fdata++ != *mdata++) {
            --fdata;
            --mdata;
            TRACE(4, "*** Verify sec reg data failed: 0x%02X @ %p != 0x%02X @ %p", *fdata, (void *)(addr + (fdata - load_buf)), *mdata, (void *)mdata);
            return *fdata - *mdata;
        }
    }

    return 0;
}
#endif

// ========================================================================
// Programmer Task

extern uint32_t __StackTop[];
extern uint32_t __StackLimit[];
extern uint32_t __bss_start__[];
extern uint32_t __bss_end__[];

#define EXEC_STRUCT_LOC __attribute__((section(".exec_struct")))

static void programmer_tgt_hardware_configre(void)
{
    uint8_t len = sizeof(ota_bootloader_pin_cfg) / sizeof(struct HAL_KEY_GPIOKEY_CFG_T);
    uint8_t pin_out_val;
    for (uint8_t i = 0; i < len; i++)
        if (ota_bootloader_pin_cfg[i].key_code != HAL_KEY_CODE_NONE) {
            if (ota_bootloader_pin_cfg[i].key_config.pin != HAL_IOMUX_PIN_NUM) {
                if (ota_bootloader_pin_cfg[i].key_down == HAL_KEY_GPIOKEY_VAL_LOW) {
                    pin_out_val = 0;
                } else {
                    pin_out_val = 1;
                }
                hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&ota_bootloader_pin_cfg[i], 1);
                hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)ota_bootloader_pin_cfg[i].key_config.pin, HAL_GPIO_DIR_OUT, pin_out_val);
            }
        }

    //NOTE(Mike.Cheng):Do more HardWare Setup here!
}

void programmer_main(void)
{
    uint32_t *dst;

    for (dst = __bss_start__; dst < __bss_end__; dst++) {
        *dst = 0;
    }

#ifdef PROGRAMMER_LOAD_SIMU

    hal_cmu_simu_pass();

#else   // !PROGRAMMER_LOAD_SIMU

#ifdef DEBUG
    for (dst = (uint32_t *)(__get_MSP() - 4); dst >= __StackLimit; dst--) {
        *dst = 0xCCCCCCCC;
    }
#endif

    NVIC_InitVectors();
#ifdef UNALIGNED_ACCESS
    SystemInit();
#endif
    hal_cmu_programmer_setup();
    hal_dma_open();
    hwtimer_init();

    bool uart_dld;
    enum HAL_TRACE_TRANSPORT_T transport;
    enum HAL_TRACE_TRANSPORT_T main_trace_transport;
    enum HAL_TRACE_TRANSPORT_T alt_trace_transport;

    programmer_tgt_hardware_configre();

#if (CHIP_HAS_UART >= 3) && (DEBUG_PORT == 3)
    main_trace_transport = HAL_TRACE_TRANSPORT_UART2;
#elif (CHIP_HAS_UART >= 2) && (DEBUG_PORT == 2)
    main_trace_transport = HAL_TRACE_TRANSPORT_UART1;
#elif (DEBUG_PORT == 1)
    main_trace_transport = HAL_TRACE_TRANSPORT_UART0;
#else
    main_trace_transport = HAL_TRACE_TRANSPORT_QTY;
#endif

#if (CHIP_HAS_UART >= 2)
    if ((main_trace_transport - HAL_TRACE_TRANSPORT_UART0) == (dld_uart - HAL_UART_ID_0)) {
        alt_trace_transport = (dld_uart == HAL_UART_ID_0) ? HAL_TRACE_TRANSPORT_UART1 : HAL_TRACE_TRANSPORT_UART0;
    } else {
        alt_trace_transport = main_trace_transport;
    }
#else
    alt_trace_transport = HAL_TRACE_TRANSPORT_QTY;
#endif

    uart_dld = uart_download_enabled();
    transport = uart_dld ? alt_trace_transport : main_trace_transport;

#ifndef NO_UART_IOMUX
    if (transport == HAL_TRACE_TRANSPORT_UART0) {
        hal_iomux_set_uart0();
#if (CHIP_HAS_UART >= 2)
    } else {
        if (!uart_dld) {
            hal_iomux_set_analog_i2c();
        }
        if (0) {
        }
        if (transport == HAL_TRACE_TRANSPORT_UART1) {
            hal_iomux_set_uart1();
#if (CHIP_HAS_UART >= 3)
        }
        if (transport == HAL_TRACE_TRANSPORT_UART2) {
            hal_iomux_set_uart2();
#endif
        }
#endif
    }
#endif
    hal_trace_open(transport);

#ifdef FORCE_ANALOG_I2C
    hal_iomux_set_analog_i2c();
#endif

#ifdef SINGLE_WIRE_DOWNLOAD
    programmer_wdt_timeout_threshold_set(programmer_op_wdt_timeout_ms);
    programmer_watchdog_init();
#endif
#if defined(_OTA_BOOT_FAST_COPY_)
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_208M);
#else
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_52M);
#endif
    TRACE(1, "------ Enter %s ------", __FUNCTION__);

    programmer_loop();
#ifdef UNCONDITIONAL_ENTER_SINGLE_WIRE_DOWNLOAD
    if (ProgrammerInflashEnterApp() < 0) {
        //enter app failed,try reboot.
        system_reboot();
    }
#endif
    TRACE(1, "------ Exit %s ------", __FUNCTION__);

#endif   // !PROGRAMMER_LOAD_SIMU

#if !defined(PROGRAMMER_INFLASH)
    SAFE_PROGRAM_STOP();
#endif
}

void programmer_start(void)
{
#if !defined(PROGRAMMER_INFLASH)
    // Init Got Base
    GotBaseInit();
#endif

    // Set stack pointer
#ifdef __ARM_ARCH_8M_MAIN__
    __set_MSPLIM((uint32_t)__StackLimit);
#endif
    __set_MSP((uint32_t)__StackTop);

    // Jump to main
    programmer_main();
}

const struct exec_struct_t EXEC_STRUCT_LOC exec_struct = {
    .entry = (uint32_t)programmer_start,
    .param = 0,
    .sp = 0,
    .exec_addr = (uint32_t)&exec_struct,
};

// ========================================================================
// Flash Task

#define STACK_ALIGN           ALIGNED(8)
#define FLASH_TASK_STACK_SIZE (2048)

enum TASK_ID_T {
    TASK_ID_MAIN = 0,
    TASK_ID_FLASH,
};

static unsigned char STACK_ALIGN flash_task_stack[FLASH_TASK_STACK_SIZE + 16];

static void setup_download_task(void)
{
    int ret;

    // Fake entry for main task
    ret = task_setup(TASK_ID_MAIN, 0, (uint32_t)__StackLimit, TASK_STATE_ACTIVE, NULL);

    ASSERT(ret == 0, "Failed to setup download task: %d", ret);
}

static void flash_task(void)
{
    unsigned int index;

    while (1) {
        while (get_burn_data_buf(&index)) {
            task_yield();
        }

        handle_data_buf(index);
    }
}

static void setup_flash_task(void)
{
    int ret;

#ifdef DEBUG
    memset(&flash_task_stack[0], 0xcc, sizeof(flash_task_stack));
#endif

    ret = task_setup(TASK_ID_FLASH, (uint32_t)flash_task_stack + FLASH_TASK_STACK_SIZE, (uint32_t)flash_task_stack, TASK_STATE_PENDING, flash_task);

    ASSERT(ret == 0, "Failed to setup flash task: %d", ret);
}

void programmer_watchdog_ping(void)
{
    hal_wdt_ping(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_feed();
#endif
}

static uint32_t programmer_wdt_timeout_threshold = WATCHDOG_SECONDS;
void programmer_wdt_timeout_threshold_set(uint32_t val)
{
    programmer_wdt_timeout_threshold = val;
}

void programmer_watchdog_reconfigure(uint32_t seconds)
{
    uint32_t lock = int_lock();

    hal_wdt_stop(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_stop();
#endif

    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);
    hal_wdt_start(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_config(seconds * 1100, seconds * 1100);
    pmu_wdt_start();
#endif

    int_unlock(lock);
}
void programmer_watchdog_init(void)
{
    hal_wdt_set_irq_callback(HAL_WDT_ID_0, NULL);
    hal_wdt_set_timeout(HAL_WDT_ID_0, programmer_wdt_timeout_threshold);
    hal_wdt_start(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_config(programmer_wdt_timeout_threshold * 1100, programmer_wdt_timeout_threshold * 1100);
    pmu_wdt_start();
#endif
}
