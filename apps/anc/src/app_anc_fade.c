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
#include "plat_types.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "hal_timer.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "anc_process.h"
#include "app_anc_utils.h"

typedef enum {
    APP_ANC_FADE_IDLE = 0,
    APP_ANC_FADE_IN,
    APP_ANC_FADE_OUT,
    APP_ANC_FADE_SWITCH_COEF,
} app_anc_fade_t;

#define ANC_FADE_MS         (10)
#define ANC_FADE_CNT        (512)

#define ANC_FADE_STACK_SIZE (1 * 1024)

static osThreadId anc_fade_thread_tid;
static void anc_fade_thread(void const *argument);
osThreadDef(anc_fade_thread, osPriorityBelowNormal, 1, ANC_FADE_STACK_SIZE, "anc_fade");

static uint32_t g_anc_fade_types = 0;

extern void app_anc_update_coef(void);

static void anc_set_fade_gain(uint32_t types, float gain)
{
    enum ANC_TYPE_T type = ANC_NOTYPE;

    for (uint32_t index=0; index<ANC_TYPE_NUM; index++) {
        type = types & (0x01 << index);
        if (type) {
            app_anc_set_gain_f32(ANC_GAIN_USER_APP_ANC_FADE, type, gain, gain);
        }
    }
}

static void anc_fadein_impl(uint32_t types)
{
    float gain = 0.0;
    float gain_step = 1.0 / ANC_FADE_CNT;
    uint32_t delay_us = (ANC_FADE_MS * 1000) / ANC_FADE_CNT;

    TRACE(0, "[%s] types: 0x%x", __func__, types);

    // Reset all user gain to 1.0
    for (uint32_t index=0; index<ANC_TYPE_NUM; index++) {
        app_anc_reset_gain(0x01 << index, 1.0, 1.0);
    }

    for (uint32_t cnt=0; cnt<ANC_FADE_CNT; cnt++) {
        gain += gain_step;
        anc_set_fade_gain(types, gain);
        hal_sys_timer_delay_us(delay_us);
    }

    anc_set_fade_gain(types, 1.0);

    TRACE(1, "[%s] OK", __func__);
}

static void anc_fadeout_impl(uint32_t types)
{
    float gain = 1.0;
    float gain_step = 1.0 / ANC_FADE_CNT;
    uint32_t delay_us = (ANC_FADE_MS * 1000) / ANC_FADE_CNT;

    TRACE(0, "[%s] types: 0x%x", __func__, types);

    for (uint32_t cnt=0; cnt<ANC_FADE_CNT; cnt++) {
        gain -= gain_step;
        anc_set_fade_gain(types, gain);
        hal_sys_timer_delay_us(delay_us);
    }

    anc_set_fade_gain(types, 0.0);

    TRACE(1, "[%s] OK", __func__);
}

static void anc_fade_thread(void const *argument)
{
    osEvent evt;
    evt.status = 0;
    uint32_t singles = 0;

    while(1) {
        evt = osSignalWait(0,osWaitForever);
        
        singles = evt.value.signals;
        TRACE(0, "[%s] singles: %d", __func__, singles);

        if(evt.status == osEventSignal) {
            uint32_t anc_fade_types = g_anc_fade_types;
            switch (singles) {
                case APP_ANC_FADE_IN:
                    anc_fadein_impl(anc_fade_types);
                    break;
                case APP_ANC_FADE_OUT:
                    anc_fadeout_impl(anc_fade_types);
                    break;
                 case APP_ANC_FADE_SWITCH_COEF:
                    anc_fadeout_impl(anc_fade_types);
                    app_anc_update_coef();
                    anc_fadein_impl(anc_fade_types);
                    // TODO: Recommend to play "ANC SWITCH" prompt here...

                    break;
                default:
                    break;
            }
        } else {
            TRACE(0,"anc fade thread evt error");
            continue;
        }
    }
}

int32_t app_anc_fade_init(void)
{
    TRACE(0, "[%s] ...", __func__);

    g_anc_fade_types = 0;
    anc_fade_thread_tid = osThreadCreate(osThread(anc_fade_thread),NULL);

    return 0;
}

int32_t app_anc_fade_deinit(void)
{
    TRACE(0, "[%s] ...", __func__);

    return 0;
}

int32_t app_anc_fadein(uint32_t types)
{
    TRACE(0, "[%s] types: 0x%0x, APP_ANC_FADE_IN ...", __func__, types);

    g_anc_fade_types = types;
    osSignalSet(anc_fade_thread_tid, APP_ANC_FADE_IN);

    return 0;
}

int32_t app_anc_fadeout(uint32_t types)
{
    TRACE(0, "[%s] types: 0x%0x, APP_ANC_FADE_OUT ...", __func__, types);

    g_anc_fade_types = types;
    osSignalSet(anc_fade_thread_tid, APP_ANC_FADE_OUT);

    return 0;
}

int32_t app_anc_fade_switch_coef(uint32_t types)
{
    TRACE(0, "[%s] types: 0x%0x, APP_ANC_FADE_SWITCH", __func__, types);

    g_anc_fade_types = types;
    osSignalSet(anc_fade_thread_tid, APP_ANC_FADE_SWITCH_COEF);

    return 0;
}
