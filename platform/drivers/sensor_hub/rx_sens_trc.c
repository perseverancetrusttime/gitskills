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
#ifdef SENS_TRC_TO_MCU

#include "plat_types.h"
#include "hal_mcu2sens.h"
#include "hal_trace.h"
#include "rx_sens_trc.h"
#include "sens_trc_msg.h"

static bool first_msg;
static const uint8_t *buf_start;
static const uint16_t *entry_start;
static uint16_t buf_total_size;
static uint16_t entry_total_cnt;

static void trace_msg_handler(struct SENS_TRC_CTRL_T *ctrl)
{
    uint32_t entry_rpos, entry_wpos;
    uint32_t trace_start, trace_end;
    uint16_t discard_cnt_w;
    uint16_t discard_cnt;
    uint32_t len;
    int proc_len;

    //TRACE(0, "Rx sensor trace: %u", ctrl->seq_w);

    if (first_msg) {
        first_msg = false;
        buf_start = ctrl->buf_start;
        entry_start = ctrl->entry_start;
        buf_total_size = ctrl->buf_size;
        entry_total_cnt = ctrl->entry_cnt;
    }

    ctrl->seq_r = ctrl->seq_w;

    discard_cnt_w = ctrl->discard_cnt_w;
    discard_cnt = discard_cnt_w - ctrl->discard_cnt_r;
    ctrl->discard_cnt_r = discard_cnt_w;

    if (discard_cnt) {
        TRACE(0, "*** WARNING: SENSOR TRACE LOST %u", discard_cnt);
    }

    entry_rpos = ctrl->entry_rpos;
    entry_wpos = ctrl->entry_wpos;

    trace_start = entry_start[entry_rpos];

    while (entry_rpos != entry_wpos) {
        entry_rpos++;
        if (entry_rpos >= ctrl->entry_cnt) {
            entry_rpos = 0;
        }
        trace_end = entry_start[entry_rpos];
        if (trace_end <= trace_start) {
            // Trace starts from buffer head
            // Sens trace will never wrap buffer so that one invokation of TRACE_OUTPUT can process one trace.
            trace_start = 0;
        }
        len = trace_end - trace_start;
        if (len) {
            proc_len = TRACE_OUTPUT(buf_start + trace_start, len);
            if (proc_len == 0) {
                break;
            }
        }
        trace_start = trace_end;
    }

    ctrl->entry_rpos = entry_rpos;
}

static unsigned int msg_test_rx_handler(const void *data, unsigned int len)
{
    const struct SENS_TRC_MSG_T *trc_msg;
    struct SENS_TRC_CTRL_T *ctrl;

    ASSERT(data, "%s: data ptr null", __func__);
    ASSERT(len == sizeof(*trc_msg), "%s: Bad msg len %u (expecting %u", __func__, len, sizeof(*trc_msg));

    trc_msg = (const struct SENS_TRC_MSG_T *)data;
    switch (trc_msg->id) {
    case SENS_TRC_MSG_ID_CRASH_ASSERT_START:
    case SENS_TRC_MSG_ID_CRASH_FAULT_START:
    case SENS_TRC_MSG_ID_CRASH_END:
        TRACE_FLUSH();
        // FALL THROUGH
    case SENS_TRC_MSG_ID_TRACE:
        ctrl = (struct SENS_TRC_CTRL_T *)trc_msg->param;
        trace_msg_handler(ctrl);
        break;
    default:
        ASSERT(false, "%s: Bad trc msg id: %d", __func__, trc_msg->id)
    }

    return len;
}

void rx_sensor_hub_trace(void)
{
    int ret;

    first_msg = true;

    ret = hal_mcu2sens_open(HAL_MCU2SENS_ID_1, msg_test_rx_handler, NULL, false);
    ASSERT(ret == 0, "hal_mcu2sens_open failed: %d", ret);

    ret = hal_mcu2sens_start_recv(HAL_MCU2SENS_ID_1);
    ASSERT(ret == 0, "hal_mcu2sens_start_recv failed: %d", ret);

    TRACE(0, "Start to rx sensor trace");
}

#endif
