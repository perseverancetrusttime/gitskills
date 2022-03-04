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
#include "app_audio.h"
#include "speech_cfg.h"
#include "stereo_record_process.h"

// #define VOICE_COMPEXP
// #define VOICE_EQ

#define VOICE_HEAP_BUF_SIZE     (STEREO_RECORD_PROCESS_BUF_SIZE)
static uint8_t *voice_heap_buf = NULL;

#if defined(VOICE_COMPEXP)
static MultiCompexpState *voice_compexp_st = NULL;
MultiCompexpConfig voice_compexp_cfg = {
    .bypass = 0,
    .num = 2,
    .xover_freq = {5000},
    .order = 4,
    .params = {
        {
            .bypass             = 0,
            .comp_threshold     = -10.f,
            .comp_ratio         = 2.f,
            .expand_threshold   = -60.f,
            .expand_ratio       = 0.5556f,
            .attack_time        = 0.01f,
            .release_time       = 0.6f,
            .makeup_gain        = 0,
            .delay              = 128,
        },
        {
            .bypass             = 0,
            .comp_threshold     = -10.f,
            .comp_ratio         = 2.f,
            .expand_threshold   = -60.f,
            .expand_ratio       = 0.5556f,
            .attack_time        = 0.01f,
            .release_time       = 0.6f,
            .makeup_gain        = 0,
            .delay              = 128,
        }
    }
};
#endif

#if defined(VOICE_EQ)
static EqState *voice_eq_st = NULL;
static EqConfig voice_eq_cfg = {
    .bypass = 0,
    .gain   = 0.0f,
    .num    = 1,
    .params = {
        {IIR_BIQUARD_PEAKINGEQ, {{1000,   0,  0.707}}},
    },
};
#endif

int32_t stereo_record_process_init(uint32_t sample_rate, uint32_t frame_len, _record_get_buf_t record_get_buf)
{
    TRACE(0, "[%s] sample_rate: %d, frame_len: %d", __func__, sample_rate, frame_len);

    record_get_buf(&voice_heap_buf, VOICE_HEAP_BUF_SIZE);
    speech_heap_init(voice_heap_buf, VOICE_HEAP_BUF_SIZE);

#if defined(VOICE_COMPEXP)
    voice_compexp_st = multi_compexp_create(sample_rate, frame_len, &voice_compexp_cfg);
#endif

#if defined(VOICE_EQ)
    voice_eq_st = eq_init(sample_rate, frame_len, &voice_eq_cfg);
#endif

    return 0;
}

int32_t stereo_record_process_deinit(void)
{
#if defined(VOICE_EQ)
    eq_destroy(voice_eq_st);
#endif

#if defined(VOICE_COMPEXP)
    multi_compexp_destroy(voice_compexp_st);
#endif

    voice_heap_buf = NULL;

    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(0, "[%s] total - %d, used - %d, max_used - %d.", __func__, total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);

    return 0;
}

int32_t stereo_record_process_run(short *pcm_buf, uint32_t pcm_len)
{
#if defined(VOICE_COMPEXP)
    multi_compexp_process(voice_compexp_st, pcm_buf, pcm_len);
#endif

#if defined(VOICE_EQ)
    eq_process(voice_eq_st, pcm_buf, pcm_len);
#endif

    return 0;
}