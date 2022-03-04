/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "plat_types.h"
#include "cmsis.h"
#include "hal_mcu2sens.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "sens_trc_msg.h"
#include "sens_trc_to_mcu.h"
#include "string.h"

#ifndef SENS_TRC_BUF_SIZE
#define SENS_TRC_BUF_SIZE                   (1024 * 4)
#endif
#ifndef SENS_TRC_ENTRY_CNT
#define SENS_TRC_ENTRY_CNT                  100
#endif
#ifndef SENS_TRC_INTVL_MS
#define SENS_TRC_INTVL_MS                   100
#endif

#define SENS_CRASH_WAIT_TIMEOUT_MS          200

#define VAR_MAX_VALUE(a)                    ((1 << (sizeof(a) * 8)) - 1)

STATIC_ASSERT(SENS_TRC_BUF_SIZE < VAR_MAX_VALUE(((struct SENS_TRC_CTRL_T *)0)->buf_size), "SENS_TRC_BUF_SIZE too large");
STATIC_ASSERT(SENS_TRC_ENTRY_CNT < VAR_MAX_VALUE(((struct SENS_TRC_CTRL_T *)0)->entry_cnt), "SENS_TRC_ENTRY_CNT too large");
STATIC_ASSERT(SENS_TRC_ENTRY_CNT < VAR_MAX_VALUE(((struct SENS_TRC_CTRL_T *)0)->entry_rpos), "SENS_TRC_ENTRY_CNT too large");
STATIC_ASSERT(SENS_TRC_ENTRY_CNT < VAR_MAX_VALUE(((struct SENS_TRC_CTRL_T *)0)->entry_wpos), "SENS_TRC_ENTRY_CNT too large");

static uint8_t trc_buf[SENS_TRC_BUF_SIZE];
static uint16_t trc_entry_list[SENS_TRC_ENTRY_CNT];
static struct SENS_TRC_CTRL_T ctrl;
static unsigned int last_seq;
static bool in_crash;

static const struct SENS_TRC_MSG_T trc_msg = {
    .id = SENS_TRC_MSG_ID_TRACE,
    .param = &ctrl,
};

static void sens_wait_tx_msg_done(void)
{
    uint32_t time;

    time = hal_sys_timer_get();
    while (hal_mcu2sens_tx_active(HAL_MCU2SENS_ID_1, last_seq) &&
            (hal_sys_timer_get() - time < MS_TO_TICKS(SENS_CRASH_WAIT_TIMEOUT_MS))) {
        hal_mcu2sens_tx_irq_run(HAL_MCU2SENS_ID_1);
    }
}

static void sens_send_trace_msg(void)
{
    uint32_t lock;

    lock = int_lock();

#if (SENS_TRC_INTVL_MS > 0)
    if (!in_crash && hal_mcu2sens_tx_active(HAL_MCU2SENS_ID_1, last_seq) && ctrl.seq_r != ctrl.seq_w) {
        goto _exit;
    }
#endif

    ctrl.seq_w++;
    hal_mcu2sens_send_seq(HAL_MCU2SENS_ID_1, &trc_msg, sizeof(trc_msg), &last_seq);

    if (in_crash) {
        sens_wait_tx_msg_done();
    }

_exit: POSSIBLY_UNUSED;
    int_unlock(lock);
}

#if (SENS_TRC_INTVL_MS > 0)
static HWTIMER_ID msg_timer;

static void msg_timer_handler(void *param)
{
    sens_send_trace_msg();
}
#endif

static void sens_trace_notify_handler(enum HAL_TRACE_STATE_T state)
{
    int ret;
    struct SENS_TRC_MSG_T crash_msg;

    in_crash = true;

    if (state == HAL_TRACE_STATE_CRASH_ASSERT_START) {
        crash_msg.id = SENS_TRC_MSG_ID_CRASH_ASSERT_START;
    } else if (state == HAL_TRACE_STATE_CRASH_FAULT_START) {
        crash_msg.id = SENS_TRC_MSG_ID_CRASH_FAULT_START;
    } else {
        crash_msg.id = SENS_TRC_MSG_ID_CRASH_END;
    }
    crash_msg.param = &ctrl;

#if (SENS_TRC_INTVL_MS > 0)
    hwtimer_stop(msg_timer);
#endif
    sens_wait_tx_msg_done();

    ctrl.seq_w++;
    ret = hal_mcu2sens_send_seq(HAL_MCU2SENS_ID_1, &crash_msg, sizeof(crash_msg), &last_seq);
    if (ret == 0) {
        sens_wait_tx_msg_done();
    }
}

static int sens_global_tag_handler(char *buf, unsigned int buf_len)
{
    const char tag[] = "[SENS]";
    unsigned int len;

    len = sizeof(tag) - 1;
    if (buf_len > len) {
        memcpy(buf, tag, len);
        return len;
    }

    return 0;
}

static void sens_trace_output_handler(const unsigned char *buf, unsigned int buf_len)
{
    uint32_t entry_rpos, entry_wpos;
    uint32_t entry_avail;
    uint32_t buf_rpos, buf_wpos;
    uint32_t buf_avail;
    uint32_t len;

    if (buf_len == 0) {
        return;
    }

    entry_rpos = ctrl.entry_rpos;
    entry_wpos = ctrl.entry_wpos;

    if (entry_wpos >= entry_rpos) {
        entry_avail = ARRAY_SIZE(trc_entry_list) - (entry_wpos - entry_rpos) - 1;
    } else {
        entry_avail = (entry_rpos - entry_wpos) - 1;
    }

    if (entry_avail < 1) {
        ctrl.discard_cnt_w++;
        goto _tell_mcu;
    }

    buf_rpos = trc_entry_list[entry_rpos];
    buf_wpos = trc_entry_list[entry_wpos];
    if (buf_wpos >= buf_rpos) {
        // Not to wrap buffer -- simplify the atomic trace operation on MCU
        buf_avail = sizeof(trc_buf) - buf_wpos;
        if (buf_avail < buf_len && buf_rpos) {
            buf_wpos = 0;
            buf_avail = buf_rpos;
        }
    } else {
        buf_avail = (buf_rpos - buf_wpos);
    }

    if (buf_avail < buf_len) {
        ctrl.discard_cnt_w++;
        goto _tell_mcu;
    }

    len = buf_len;
    memcpy(trc_buf + buf_wpos, buf, len);
    buf_wpos += buf_len;

    entry_wpos++;
    if (entry_wpos >= ARRAY_SIZE(trc_entry_list)) {
        entry_wpos -= ARRAY_SIZE(trc_entry_list);
    }

    trc_entry_list[entry_wpos] = buf_wpos;
    ctrl.entry_wpos = entry_wpos;

_tell_mcu:
    // Send message to MCU
    if (in_crash) {
        if ((entry_avail < ARRAY_SIZE(trc_entry_list) / 2) || (buf_avail < sizeof(trc_buf) / 2)) {
            sens_send_trace_msg();
        }
        return;
    }

#if (SENS_TRC_INTVL_MS > 0)
    if ((entry_avail < ARRAY_SIZE(trc_entry_list) / 2) || (buf_avail < sizeof(trc_buf) / 2)) {
        hwtimer_stop(msg_timer);
        sens_send_trace_msg();
        return;
    }

    if (!hwtimer_active(msg_timer)) {
        hwtimer_start(msg_timer, MS_TO_TICKS(SENS_TRC_INTVL_MS));
    }
#else
    sens_send_trace_msg();
#endif
}

void sens_trace_to_mcu(void)
{
    int ret;

    ctrl.buf_start = trc_buf;
    ctrl.entry_start = trc_entry_list;
    ctrl.buf_size = sizeof(trc_buf);
    ctrl.entry_cnt = ARRAY_SIZE(trc_entry_list);
    ctrl.entry_rpos = 0;
    ctrl.entry_wpos = 0;
    ctrl.discard_cnt_r = 0;
    ctrl.discard_cnt_w = 0;

    trc_entry_list[ctrl.entry_rpos] = 0;
    trc_entry_list[ctrl.entry_wpos] = 0;

    last_seq = 0;
    in_crash = false;

#if (SENS_TRC_INTVL_MS > 0)
    msg_timer = hwtimer_alloc(msg_timer_handler, NULL);
    ASSERT(msg_timer, "%s: Failed to alloc msg_timer", __func__);
#endif

    ret = hal_mcu2sens_open(HAL_MCU2SENS_ID_1, NULL, NULL, false);
    ASSERT(ret == 0, "hal_mcu2sens_open failed: %d", ret);

    hal_trace_global_tag_register(sens_global_tag_handler);
    hal_trace_app_register(HAL_TRACE_APP_REG_ID_0, sens_trace_notify_handler, sens_trace_output_handler);
}
