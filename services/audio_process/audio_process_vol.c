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
#include "hal_trace.h"
#include "hal_aud.h"
#include "cmsis.h"
#include "audio_process_vol.h"
#include "audio_process_sync.h"

static uint32_t g_sample_rate = AUD_SAMPRATE_44100;
static float g_gain = 1.0;

static uint32_t g_smooth_num    = 0;       // Use to check whether need to do smooth 
static uint32_t g_smooth_index  = 0;
static float g_smooth_gain  = 1.0;
static float g_smooth_step  = 0.0;

#define _EQ_FLOAT(a, b)     (ABS(a - b) < 0.000001 ? true : false)

int32_t audio_process_vol_init(uint32_t sample_rate, uint32_t bits, uint32_t ch)
{
    TRACE(0, "[%s] sample_rate: %d, bits: %d, ch: %d", __func__, sample_rate, bits, ch);

    ASSERT(bits == AUD_BITS_24, "[%s] bits(%d) != AUD_BITS_24", __func__, bits);
    ASSERT(ch == AUD_CHANNEL_NUM_2, "[%s] ch(%d) != AUD_CHANNEL_NUM_2", __func__, ch);

    uint32_t lock = int_lock();
    g_sample_rate = sample_rate;
    g_gain = 1.0;

    g_smooth_num = 0;
    g_smooth_index = 0;
    g_smooth_gain = 1.0;
    g_smooth_step = 0.0;
    int_unlock(lock);

    return 0;
}

int32_t audio_process_vol_start_impl(float gain, uint32_t ms)
{
    ASSERT(gain <= 1.0 && gain >= 0.0, "[%s] gain(%d) is invalid", __func__, (int32_t)(gain * 1000));
    ASSERT(ms != 0, "[%s] ms(%d) is invalid", __func__, ms);

    TRACE(0, "[%s] gain(x1000): %d, ns: %d", __func__, (int32_t)(gain * 1000), ms);

    if (_EQ_FLOAT(gain, g_gain)) {
        TRACE(0, "[%s] WARNING: Don't need to change gain", __func__);
        return -1;
    }

    if (g_smooth_num) {
        TRACE(0, "[%s] WARNING: Smoothing has not been finished", __func__);
        return -2;
    }

    uint32_t lock = int_lock();
    g_smooth_index  = 0;
    g_smooth_num    = ms * (g_sample_rate / 1000);

    g_smooth_step   = (gain - g_gain) / g_smooth_num;
    g_smooth_gain   = g_gain;
    g_gain = gain;
    int_unlock(lock);

    return 0;
}

int32_t audio_process_vol_run(int32_t *pcm_buf, uint32_t pcm_len)
{
    uint32_t frame_len = pcm_len / AUD_CHANNEL_NUM_2;

    uint32_t lock = int_lock();
    for (uint32_t i=0; i<frame_len; i++) {
        pcm_buf[AUD_CHANNEL_NUM_2 * i] = (int32_t)(pcm_buf[AUD_CHANNEL_NUM_2 * i] * g_smooth_gain);
        // pcm_buf[AUD_CHANNEL_NUM_2 * i + 1] = (int32_t)(pcm_buf[AUD_CHANNEL_NUM_2 * i + 1] * g_smooth_gain);
        if (g_smooth_num) {
            if (g_smooth_index >= g_smooth_num) {
                g_smooth_index = 0;
                g_smooth_num = 0;
            } else {
                g_smooth_gain += g_smooth_step;
                g_smooth_index++;
            }
        }
    }
    int_unlock(lock);

    return 0;
}


static int32_t audio_process_vol_start_sync(float gain, uint32_t ms)
{
    TRACE(0,"[%s] ...", __func__);

    uint8_t buf[12];

    *((uint32_t *)&buf[0]) = AUDIO_PROCESS_SYNC_VOL;
    *((float *)&buf[4]) = gain;
    *((uint32_t *)&buf[8]) = ms;

    audio_process_sync_send(buf, sizeof(buf));

    return 0;
}

int32_t audio_process_vol_start(float gain, uint32_t ms)
{
    audio_process_vol_start_sync(gain, ms);
    audio_process_vol_start_impl(gain, ms);

    return 0;
}