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
#if defined(AUDIO_PCM_PLAYER)

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cmsis_os.h"

// platform related
#include "plat_types.h"
#include "hal_trace.h"
#include "tgt_hardware.h"
#include "app_status_ind.h"
#include "app_bt_stream.h"
#include "app_bt_media_manager.h"
#include "app_audio.h"
#include "audio_process.h"
#include "app_overlay.h"
#include "app_media_player.h"
#include "app_utils.h"
#include "audioflinger.h"

#include "app_bt_stream_pcm_player.h"

#define PCM_PLAYER_LOGD(...) TRACE(1, __VA_ARGS__)

#define STREAM_PCM_IDEAL_PERIOD_MS (20)

extern uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status);
extern uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index);
extern uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status);
void bt_audio_updata_eq(uint8_t index);

static int pcm_player_on = 0;
int8_t stream_pcm_player_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
int stream_pcm_ideal_period_ms = STREAM_PCM_IDEAL_PERIOD_MS;
static pcm_player_config_t __player_config;
static pcm_player_config_t *player_config = &__player_config;

// PCM queue signal
#define PCM_QUEUE_SIGNAL_ID           15
#define PCM_QUEUE_WAIT_TIMEOUT        (300)
#define PCM_QUEUE_WAIT_RETRY          (3)
osThreadId wait_thread_id = NULL;

static void pcm_player_pcm_queue_signal_wait(void)
{
    wait_thread_id = osThreadGetId();
    osSignalWait((1<<PCM_QUEUE_SIGNAL_ID), PCM_QUEUE_WAIT_TIMEOUT);
    osSignalClear(wait_thread_id, (1<<PCM_QUEUE_SIGNAL_ID));
}

static void pcm_player_pcm_queue_signal_notify(void)
{
    ASSERT(wait_thread_id!=NULL, "pcm_player_pcm_queue_signal_notify:error,wait_thread_id is NULL!");
    osSignalSet(wait_thread_id, (1<<PCM_QUEUE_SIGNAL_ID));
}

static void pcm_player_get_size(uint32_t *sample_size, uint32_t *dma_size, uint32_t *queue_size)
{
    // sample size
    if (player_config->bits == 24) {
        *sample_size = 4*player_config->channel_num;
    }
    else {
        *sample_size = (player_config->bits/8)*player_config->channel_num;
    }

    // dma size
    *dma_size = (stream_pcm_ideal_period_ms*player_config->sample_rate/1000*(*sample_size)) * 2;

    // queue size
    *queue_size = *dma_size;

    PCM_PLAYER_LOGD("pcm_player_get_size:sample_size=%d,dma_size=%d,queue_size=%d", *sample_size, *dma_size, *queue_size);
}

// API
int pcm_player_open(pcm_player_t *pcm_player)
{
    return 0;
}

int pcm_player_start(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_PCM_PLAYER, 0, 0);    
    return 0;
}

int pcm_player_stop(void)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_PCM_PLAYER, 0, 0);    
    return 0;
}

int pcm_player_setup(pcm_player_config_t *config)
{
    memcpy(&__player_config, config, sizeof(pcm_player_config_t));
    return 0;
}

static uint32_t pcm_player_need_pcm_data(uint8_t* pcm_buf, uint32_t len)
{
    app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
    pcm_player_pcm_queue_signal_notify();
    app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);    

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len/2);
#endif

#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#else
    bt_audio_updata_eq(app_audio_get_eq());
#endif

    audio_process_run(pcm_buf, len);

    return len;
}

int pcm_player_onoff(char onoff)
{
    uint32_t play_samp_size;
    uint8_t *bt_eq_buff = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    uint32_t dma_size = 0, queue_size = 0;
    uint8_t *pcm_player_audio_play_buff = 0;
    uint8_t *pcm_player_audio_loop_buf = NULL;

    (void)bt_eq_buff;
    (void)eq_buff_size;
    (void)play_samp_size;

    PCM_PLAYER_LOGD("pcm_player_onoff : now %d, onoff %d", pcm_player_on, onoff);

    if (pcm_player_on == onoff) {
        PCM_PLAYER_LOGD("pcm_player_onoff : waring,already %d, do nothing!", onoff);
        return 0;
    }

     if (onoff) {
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);

        app_audio_mempool_init();

        pcm_player_get_size(&play_samp_size, &dma_size, &queue_size);

        app_audio_mempool_get_buff(&pcm_player_audio_play_buff, dma_size);
        app_audio_mempool_get_buff(&pcm_player_audio_loop_buf, queue_size);
        app_audio_pcmbuff_init(pcm_player_audio_loop_buf, queue_size);

        app_play_audio_lineinmode_init(player_config->channel_num, dma_size/2);

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)player_config->channel_num;
#if defined(__AUDIO_RESAMPLE__)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)player_config->sample_rate;
#endif
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = stream_pcm_player_volume;
        PCM_PLAYER_LOGD("pcm_player_onoff : vol = %d", stream_pcm_player_volume);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = pcm_player_need_pcm_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(pcm_player_audio_play_buff);
        stream_cfg.data_size = dma_size;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
     else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    pcm_player_on = onoff;
    PCM_PLAYER_LOGD("pcm_player_onoff : end!\n");
    return 0;
}

#endif /* AUDIO_PCM_PLAYER */