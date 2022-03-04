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
/**
 * Usage:
 *  1. This module is related with UI.
 * 
 * TODO: 
 *  1. Complete init, get current anc gain or curve index.
 *  2. When TWS reconnect, need to sync info
 * 
 **/
#include "hal_trace.h"
#include "anc_assist_tws_sync.h"
#include "anc_assist_anc.h"

#define SYNC_INTERVAL_TIME_MS       (1000)

typedef struct {
    uint32_t    trigger_cnt;            // For trigger operation
    float       ff_gain_coef;
    float       fb_gain_coef;
    uint32_t    anc_curve_index;
} anc_assist_sync_info_t;

static anc_assist_sync_info_t g_curr_info;
static anc_assist_sync_info_t g_sync_info;

// Use heartbeat to instead of timer or thread.
#define HEARTBEAT_CNT_INVALID       (0xFFFF)
static uint32_t g_heartbeat_cnt     = HEARTBEAT_CNT_INVALID;
static uint32_t g_heartbeat_cnt_threshold   = 100;
static bool     g_sync_cache_flag   = false;
static anc_assist_sync_info_t g_sync_cache_info;

// 100 is the scale factor which is related with precision
#define _EQ_FLOAT(a, b)         ((uint32_t)((a) * 100) == (uint32_t)((b) * 100) ? true : false)

#if defined(IBRT)
#define _TWS_SYNC_INFO_ENABLED

#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_customif_cmd.h"
#include "app_tws_ctrl_thread.h"

static POSSIBLY_UNUSED bool _check_tws_connected(void)
{
    return app_tws_ibrt_tws_link_connected();
}
#endif

int32_t anc_assist_tws_sync_status_change(uint8_t *buf, uint32_t len)
{
    ASSERT(len == sizeof(anc_assist_sync_info_t), "[%s] len(%d) != sizeof(anc_assist_sync_info_t)(%d)", __func__, len, sizeof(anc_assist_sync_info_t));
	
    anc_assist_sync_info_t info = *((anc_assist_sync_info_t *)buf);

    // TRACE(2, "[%s] gain_coef: %d", __func__, (uint32_t)(info.ff_gain_coef * 100));

    if (_EQ_FLOAT(g_curr_info.ff_gain_coef, info.ff_gain_coef) == false) {
        anc_assist_anc_set_gain_coef_impl(info.ff_gain_coef, ANC_FEEDFORWARD);
        g_curr_info.ff_gain_coef = info.ff_gain_coef;
    }

    if (_EQ_FLOAT(g_curr_info.fb_gain_coef, info.fb_gain_coef) == false) {
        anc_assist_anc_set_gain_coef_impl(info.fb_gain_coef, ANC_FEEDBACK);
        g_curr_info.fb_gain_coef = info.fb_gain_coef;
    }

    if (g_curr_info.anc_curve_index != info.anc_curve_index) {
        anc_assist_anc_switch_curve_impl(info.anc_curve_index);
        g_curr_info.anc_curve_index = info.anc_curve_index;
    }

	return 0;
}

int32_t anc_assist_tws_sync_info_impl(anc_assist_sync_info_t info)
{
#if defined(IBRT)
#if defined(ANC_ASSIST_ENABLED)
    if (_check_tws_connected()) {
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_ANC_ASSIST_STATUS, (uint8_t *)&info, sizeof(info));
    }
#endif    
#endif
    anc_assist_tws_sync_status_change((uint8_t *)&info, sizeof(info));

    g_sync_cache_flag = false;
    g_heartbeat_cnt = 0;

    return 0;
}

int32_t anc_assist_tws_sync_info(anc_assist_sync_info_t info)
{
	// TRACE(2, "[%s] ff_gain_coef: %d", __func__, (uint32_t)(info.ff_gain_coef * 100));

    if (g_heartbeat_cnt < g_heartbeat_cnt_threshold) {
        g_sync_cache_info = info;
        g_sync_cache_flag = true;
        TRACE(1, "[%s] The sending interval is too short! Cache sync info.", __func__);

        return 1;
    } else {
        anc_assist_tws_sync_info_impl(info);

        return 0;
    }
}

int32_t anc_assist_tws_sync_set_anc_gain_coef(anc_assist_algo_id_t id, float gain_coef)
{
    TRACE(0, "[%s] id = 0x%x, gain = %d%%", __func__, id, (int32_t)(gain_coef * 100));

    // Diff algo set different anc channel
    if (id == ANC_ASSIST_ALGO_ID_HYBRID_HOWLING) {
        // Standalone
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDFORWARD);
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDBACK);
    } else if (id == ANC_ASSIST_ALGO_ID_FF_HOWLING) {
        // Standalone
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDFORWARD);
    } else if (id == ANC_ASSIST_ALGO_ID_FB_HOWLING) {
        // Standalone
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDBACK);
    } else if (id == ANC_ASSIST_ALGO_ID_NOISE) {
        // Both Master and Slave can make a decision 
#if defined(_TWS_SYNC_INFO_ENABLED)
        g_sync_info.fb_gain_coef = gain_coef;
        anc_assist_tws_sync_info(g_sync_info);
#else
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDBACK);
#endif
    } else if (id == ANC_ASSIST_ALGO_ID_WIND) {
        // Both Master and Slave can make a decision 
#if defined(_TWS_SYNC_INFO_ENABLED)
        g_sync_info.ff_gain_coef = gain_coef;
        anc_assist_tws_sync_info(g_sync_info);
#else
        anc_assist_anc_set_gain_coef_impl(gain_coef, ANC_FEEDFORWARD);
#endif
    } else {
        ASSERT(0, "[%s] id(%d) is invalid!", __func__, id);
    }

    return 0;
}

int32_t anc_assist_tws_sync_set_anc_curve(anc_assist_algo_id_t id, uint32_t index)
{
    TRACE(0, "[%s] Switch curve: id = 0x%x, index = %d", __func__, id, index);
#if defined(_TWS_SYNC_INFO_ENABLED)
    if (app_ibrt_if_get_ui_role() == TWS_UI_SLAVE) {
        TRACE(1, "[%s] SLAVE has not access to change anc curve!", __func__);
    } else {    // TWS_UI_MASTER or TWS_UI_UNKNOWN
        g_sync_info.anc_curve_index = index;
        anc_assist_tws_sync_info(g_sync_info);
    }
#else
    anc_assist_anc_switch_curve_impl(index);
#endif

    return 0;
}

int32_t anc_assist_tws_sync_init(float frame_ms)
{
    // TODO: Get init parameters from anc_assist_anc
    g_curr_info.trigger_cnt = 0;
    g_curr_info.ff_gain_coef = 1.0;
    g_curr_info.fb_gain_coef = 1.0;
    g_curr_info.anc_curve_index = 0;

    g_sync_info.trigger_cnt = 0;
    g_sync_info.ff_gain_coef = 1.0;
    g_sync_info.fb_gain_coef = 1.0;
    g_sync_info.anc_curve_index = 0;

    g_sync_cache_info.trigger_cnt = 0;
    g_sync_cache_info.ff_gain_coef = 1.0;
    g_sync_cache_info.fb_gain_coef = 1.0;
    g_sync_cache_info.anc_curve_index = 0;

    g_heartbeat_cnt = HEARTBEAT_CNT_INVALID;
    g_heartbeat_cnt_threshold = (uint32_t)(SYNC_INTERVAL_TIME_MS / frame_ms);

    // TRACE(0, "[%s] Heartbeat counter threshold: %d", __func__, g_heartbeat_cnt_threshold);

    return 0;
}

int32_t anc_assist_tws_sync_heartbeat(void)
{
    if (g_heartbeat_cnt != HEARTBEAT_CNT_INVALID) {
        g_heartbeat_cnt++;

        if (g_heartbeat_cnt > g_heartbeat_cnt_threshold) {
            if (g_sync_cache_flag) {
                anc_assist_tws_sync_info_impl(g_sync_info);
            } else {
                g_heartbeat_cnt = HEARTBEAT_CNT_INVALID;
            }
        }
    }

    return 0;
}