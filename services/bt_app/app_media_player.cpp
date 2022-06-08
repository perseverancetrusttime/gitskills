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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "list.h"

#ifdef MEDIA_PLAYER_SUPPORT

#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "resources.h"
#include "app_media_player.h"
#include "res_audio_ring.h"
#include "audio_prompt_sbc.h"
#include "app_bt.h"
#include "besbt.h"
#if defined(ANC_ASSIST_ENABLED)
#include "app_voice_assist_prompt_leak_detect.h"
#endif
#if defined(BT_SOURCE)
#include "bt_source.h"
#endif
#include "cqueue.h"
#include "btapp.h"
#include "app_bt_media_manager.h"
#include "a2dp_decoder.h"
#ifdef PROMPT_IN_FLASH
#include "nvrecord_prompt.h"
#else
#include "res_audio_data.h"
#include "res_audio_data_cn.h"
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#include"anc_process.h"
#include "hal_codec.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#if defined(IBRT)
#include "app_tws_ibrt.h"
#include "app_ibrt_if.h"
#include "app_ibrt_voice_report.h"
#endif

// #if defined(ANC_ASSIST_ENABLED)
// #include "app_anc_assist.h"
// #endif

// Customized warning tone volume should be defined in tgt_hardware.h
#ifndef MEDIA_VOLUME_LEVEL_WARNINGTONE
#define MEDIA_VOLUME_LEVEL_WARNINGTONE TGT_VOLUME_LEVEL_12
#endif

#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
#include "iir_process.h"
#endif

#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
#define MEDIA_PLAYER_USE_CODEC2
#endif

#define MEDIA_PLAYER_CHANNEL_NUM (AUD_CHANNEL_NUM_2)

#ifdef MEDIA_PLAYER_USE_CODEC2
#define MEDIA_PLAYER_OUTPUT_DEVICE (AUD_STREAM_USE_INT_CODEC2)
#define AF_CODEC_TUNE af_codec_tune_dac2
#else
#define MEDIA_PLAYER_OUTPUT_DEVICE (AUD_STREAM_USE_INT_CODEC)
#define AF_CODEC_TUNE af_codec_tune
#endif

#ifdef __INTERACTION__
uint8_t g_findme_fadein_vol = TGT_VOLUME_LEVEL_1;
#endif
static char need_init_decoder = 1;
static btif_sbc_decoder_t *media_sbc_decoder = NULL;

#define SBC_TEMP_BUFFER_SIZE 64
#define SBC_QUEUE_SIZE (SBC_TEMP_BUFFER_SIZE*16)
static CQueue media_sbc_queue;

#define CFG_HW_AUD_EQ_NUM_BANDS (8)
static float * media_sbc_eq_band_gain = NULL;

// 128 * 2: 16ms for 16k sample rate
#define APP_AUDIO_PLAYBACK_BUFF_SIZE        (128 * 2 * sizeof(int16_t) * MEDIA_PLAYER_CHANNEL_NUM * 2)

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static enum AUD_BITS_T sample_size_play_bt;
static enum AUD_SAMPRATE_T sample_rate_play_bt;
static uint32_t data_size_play_bt;

static uint8_t *playback_buf_bt;
static uint32_t playback_size_bt;
static int32_t playback_samplerate_ratio_bt;

static uint8_t *playback_buf_mc;
static uint32_t playback_size_mc;
static enum AUD_CHANNEL_NUM_T  playback_ch_num_bt;
#endif

static const AUD_ID_ENUM app_prompt_continous_id[] =
    {AUDIO_ID_FIND_MY_BUDS };
static AUD_ID_ENUM app_prompt_on_going_prompt_id;

#if defined(__XSPACE_UI__)
static AUD_ID_ENUM app_prompt_on_going_prompt_id2 = AUD_ID_INVALID;
#endif
typedef struct {
    uint8_t  reqType;    // one of APP_PROMPT_PLAY_REQ_TYPE_E
    uint8_t  isLocalPlaying;
    uint8_t  deviceId;
    uint16_t promptId;

} APP_PROMPT_PLAY_REQ_T;

/**< signal of the prompt handler thread */
#define PROMPT_HANDLER_SIGNAL_NEW_PROMPT_REQ        0x01
#define PROMPT_HANDLER_SIGNAL_CLEAR_REQ             0x02
#define PROMPT_HANDLER_SIGNAL_PLAYING_COMPLETED     0x04

static osThreadId app_prompt_handler_tid;
static void app_prompt_handler_thread(void const *argument);
osThreadDef(app_prompt_handler_thread, osPriorityHigh, 1, 1536, "app_prompt_handler");

#define  SBC_FRAME_LEN  64 //0x5c   /* pcm 512 bytes*/
static U8* g_app_audio_data = NULL;
static uint32_t g_app_audio_length = 0;
static uint32_t g_app_audio_read = 0;

static uint32_t g_play_continue_mark = 0;

#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
extern IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_dac_iir_cfg_list[EQ_HW_DAC_IIR_LIST_NUM];
#endif

static uint8_t app_play_sbc_stop_proc_cnt = 0;

static uint16_t g_prompt_chnlsel = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;

//for continue play

#define MAX_SOUND_NUMBER 10

typedef struct tMediaSoundMap
{
    U8* data;  //total files
    uint32_t fsize; //file index

}_tMediaSoundMap;

const tMediaSoundMap*  media_sound_map;

#ifdef PROMPT_IN_FLASH
tMediaSoundMap media_number_sound_map[MAX_SOUND_NUMBER] = {0};
#else

const tMediaSoundMap media_sound_map_en[MAX_SOUND_NUMBER] =
{
    {(U8*)EN_SOUND_ZERO,    sizeof(EN_SOUND_ZERO) },
    {(U8*)EN_SOUND_ONE,    sizeof(EN_SOUND_ONE) },
    {(U8*)EN_SOUND_TWO,     sizeof(EN_SOUND_TWO) },
    {(U8*)EN_SOUND_THREE,     sizeof(EN_SOUND_THREE) },
    {(U8*)EN_SOUND_FOUR,     sizeof(EN_SOUND_FOUR) },
    {(U8*)EN_SOUND_FIVE,     sizeof(EN_SOUND_FIVE) },
    {(U8*)EN_SOUND_SIX,     sizeof(EN_SOUND_SIX) },
    {(U8*)EN_SOUND_SEVEN,     sizeof(EN_SOUND_SEVEN) },
    {(U8*)EN_SOUND_EIGHT,     sizeof(EN_SOUND_EIGHT) },
    {(U8*)EN_SOUND_NINE,    sizeof(EN_SOUND_NINE) },
};

const uint8_t BT_MUTE[] = {
#include "res/SOUND_MUTE.txt"
};
#endif

char Media_player_number[MAX_PHB_NUMBER];

typedef struct tPlayContContext
{
    uint32_t g_play_continue_total; //total files
    uint32_t g_play_continue_n; //file index

    uint32_t g_play_continue_fread; //per file have readed

    U8 g_play_continue_array[MAX_PHB_NUMBER];

}_tPlayContContext;

tPlayContContext pCont_context;

APP_AUDIO_STATUS MSG_PLAYBACK_STATUS;
APP_AUDIO_STATUS*  ptr_msg_playback = &MSG_PLAYBACK_STATUS;

static int g_language = MEDIA_DEFAULT_LANGUAGE;
#ifdef AUDIO_LINEIN
static enum AUD_SAMPRATE_T app_play_audio_sample_rate = AUD_SAMPRATE_16000;
#endif

#define PREFIX_AUDIO(name)  ((g_language==MEDIA_DEFAULT_LANGUAGE) ? EN_##name : CN_##name)

#define PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index)  \
    ((PROMPT_MIX_PROPERTY_T *)((uint32_t)__mixprompt_property_table_start + \
    (index)*sizeof(PROMPT_MIX_PROPERTY_T)))

#if defined(MEDIA_PLAYER_USE_CODEC2)
#undef app_audio_mempool_get_buff
#define app_audio_mempool_get_buff app_media_mempool_get_buff
#define MEDIA_PLAY_STREAM_ID (AUD_STREAM_ID_3)
#else
#define MEDIA_PLAY_STREAM_ID (AUD_STREAM_ID_0)
#endif

typedef int (*app_media_mempool_get_buff_cb)(uint8_t **buff, uint32_t size);

int media_audio_init(void)
{
    uint8_t *buff = NULL;
    uint8_t i;
    
    app_media_mempool_get_buff_cb get_buff_cb = NULL;
#ifdef MEDIA_PLAYER_USE_CODEC2
    get_buff_cb = app_media_mempool_get_buff;
    if(is_play_audio_prompt_use_dac1()){
        get_buff_cb = syspool_get_buff;
    }
#else
    get_buff_cb = app_audio_mempool_get_buff;
#endif
    get_buff_cb((uint8_t **)&media_sbc_eq_band_gain, CFG_HW_AUD_EQ_NUM_BANDS*sizeof(float));

    for (i=0; i<CFG_HW_AUD_EQ_NUM_BANDS; i++){
        media_sbc_eq_band_gain[i] = 1.0;
    }

    get_buff_cb(&buff, SBC_QUEUE_SIZE);
    memset(buff, 0, SBC_QUEUE_SIZE);

    LOCK_APP_AUDIO_QUEUE();
    APP_AUDIO_InitCQueue(&media_sbc_queue, SBC_QUEUE_SIZE, buff);
    UNLOCK_APP_AUDIO_QUEUE();

    get_buff_cb((uint8_t **)&media_sbc_decoder, sizeof(btif_sbc_decoder_t) + 4);

    need_init_decoder = 1;

    app_play_sbc_stop_proc_cnt = 0;
    
    return 0;
}
static int decode_sbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    uint8_t underflow = 0;

    int r = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    unsigned int peek_len = 0;

    static btif_sbc_pcm_data_t pcm_data;
    bt_status_t ret = BT_STS_SUCCESS;
    unsigned short byte_decode = 0;

    pcm_data.data = (unsigned char*)pcm_buffer;

    LOCK_APP_AUDIO_QUEUE();
again:
    if(need_init_decoder) {
        pcm_data.data = (unsigned char*)pcm_buffer;
        pcm_data.dataLen = 0;
        btif_sbc_init_decoder(media_sbc_decoder);
    }

get_again:
    len1 = len2 = 0;
    peek_len = MIN(SBC_TEMP_BUFFER_SIZE, APP_AUDIO_LengthOfCQueue(&media_sbc_queue));
    if (peek_len == 0) {
        need_init_decoder = 1;
        underflow = 1;
        r = pcm_data.dataLen;
        TRACE(0,"last chunk of prompt");
        goto exit;
    }

    r = APP_AUDIO_PeekCQueue(&media_sbc_queue, peek_len, &e1, &len1, &e2, &len2);
    ASSERT(r == CQ_OK, "[%s] peek queue should not failed", __FUNCTION__);

    if (!len1){
        TRACE(2,"len1 %d/%d\n", len1, len2);
        goto get_again;
    }

    ret = btif_sbc_decode_frames(media_sbc_decoder, (unsigned char *)e1,
                                len1, &byte_decode,
                                &pcm_data, pcm_len,
                                media_sbc_eq_band_gain);

    if(ret == BT_STS_CONTINUE) {
        need_init_decoder = 0;
        APP_AUDIO_DeCQueue(&media_sbc_queue, 0, len1);
        goto again;

        /* back again */
    }
    else if(ret == BT_STS_SUCCESS) {
        need_init_decoder = 0;
        r = pcm_data.dataLen;
        pcm_data.dataLen = 0;

        APP_AUDIO_DeCQueue(&media_sbc_queue, 0, byte_decode);

        //TRACE(1,"p %d\n", pcm_data.sampleFreq);

        /* leave */
    }
    else if(ret == BT_STS_FAILED) {
        need_init_decoder = 1;
        r = pcm_data.dataLen;
        TRACE(0,"err\n");

        APP_AUDIO_DeCQueue(&media_sbc_queue, 0, byte_decode);

        /* leave */
    }
    else if(ret == BT_STS_NO_RESOURCES) {
        need_init_decoder = 0;

        TRACE(0,"no\n");

        /* leav */
        r = 0;
    }

exit:
    if (underflow){
        TRACE(1,"media_sbc_decoder len:%d\n ", pcm_len);
    }
    UNLOCK_APP_AUDIO_QUEUE();
    return r;
}

static int app_media_store_sbc_buffer(uint8_t device_id, unsigned char *buf, unsigned int len)
{
    int nRet;
    LOCK_APP_AUDIO_QUEUE();
    nRet = APP_AUDIO_EnCQueue(&media_sbc_queue, buf, len);
    UNLOCK_APP_AUDIO_QUEUE();

    return nRet;
}

#if defined(IBRT)

#define PENDING_SYNC_PROMPT_BUFFER_CNT  8
// cleared when tws is disconnected
static uint16_t pendingSyncPromptId[PENDING_SYNC_PROMPT_BUFFER_CNT];
static uint8_t pending_sync_prompt_in_index = 0;
static uint8_t pending_sync_prompt_out_index = 0;
static uint8_t pending_sync_prompt_cnt = 0;

void app_tws_sync_prompt_manager_reset(void)
{
    pending_sync_prompt_in_index = 0;
    pending_sync_prompt_out_index = 0;
    pending_sync_prompt_cnt = 0;
}

void app_tws_sync_prompt_check(void)
{
    if (0 == pending_sync_prompt_cnt)
    {
        app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_SYNC_VOICE_PROMPT, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
    }

    if (IBRT_ACTIVE_MODE != app_ibrt_if_get_bt_ctrl_ctx()->tws_mode)
    {
        return;
    }

    bool isPlayPendingPrompt = false;
    uint16_t promptIdToPlay = 0;

    uint32_t lock = int_lock_global();
    if (pending_sync_prompt_cnt > 0)
    {
        isPlayPendingPrompt = true;
        promptIdToPlay = pendingSyncPromptId[pending_sync_prompt_out_index];
        pending_sync_prompt_out_index++;
        if (PENDING_SYNC_PROMPT_BUFFER_CNT == pending_sync_prompt_out_index)
        {
            pending_sync_prompt_out_index = 0;
        }
        pending_sync_prompt_cnt--;
    }
    int_unlock_global(lock);

    if (isPlayPendingPrompt)
    {
        TRACE(1,"pop pending prompt 0x%x to play", promptIdToPlay);
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
            BT_STREAM_MEDIA, 0, promptIdToPlay);
    }
}
#endif

void trigger_media_play(AUD_ID_ENUM id, uint8_t device_id, uint16_t aud_pram)
{
    uint16_t convertedId = (uint8_t)id;
    convertedId |= aud_pram;
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_MEDIA,device_id,convertedId);
}

void trigger_media_stop(AUD_ID_ENUM id, uint8_t device_id)
{
    /* Only the stop loop mode is supported */
    if (id == AUDIO_ID_FIND_MY_BUDS)
        app_play_sbc_stop_proc_cnt = 1;
}

#if defined(IBRT)
static bool isTriggerPromptLocal = false;
void trigger_prompt_local(bool isTriggerLocal)
{
    isTriggerPromptLocal = isTriggerLocal;
}

bool is_trigger_prompt_local(void)
{
    return isTriggerPromptLocal;
}

static bool is_prompt_playing_handling_locally(AUD_ID_ENUM promptId)
{
    if ((AUDIO_ID_BT_MUTE != promptId) && isTriggerPromptLocal)
    {
        return true;
    }
    switch ((uint16_t)PROMPT_ID_FROM_ID_VALUE(promptId))
    {
        case AUDIO_ID_FIND_MY_BUDS:
        case AUD_ID_POWER_OFF:
            return true;
        default:
            return false;
    }
}
#endif

static bool media_PlayAudio_handler(AUD_ID_ENUM id,uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_pram = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
    if (isLocalPlaying)
    {
        aud_pram |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_pram);

    return true;
}

void media_PlayAudio(AUD_ID_ENUM id,uint8_t device_id)
{
    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_NORMAL_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_NORMAL_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

static bool media_PlayAudio_standalone_handler(AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_pram = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
    if (isLocalPlaying)
    {
        aud_pram |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_pram);
    return true;
}

void media_PlayAudio_standalone(AUD_ID_ENUM id, uint8_t device_id)
{
    if (AUDIO_ID_BT_MUTE == id)
    {
        app_start_post_chopping_timer();
    }

    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_STANDALONE_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_STANDALONE_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

static bool media_PlayAudio_locally_handler(AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_param;
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (A2DP_AUDIO_CHANNEL_SELECT_LCHNL == p_ibrt_ctrl->audio_chnl_sel)
    {
        aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_LCHNL;
    }
    else
    {
        aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_RCHNL;
    }
#else
    aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
#endif

    if (isLocalPlaying)
    {
        aud_param |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_param);
    return true;
}

void media_PlayAudio_locally(AUD_ID_ENUM id, uint8_t device_id)
{

    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_LOCAL_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_LOCAL_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

static bool media_PlayAudio_standalone_locally_handler(AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_param;
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (A2DP_AUDIO_CHANNEL_SELECT_LCHNL == p_ibrt_ctrl->audio_chnl_sel)
    {
        aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_LCHNL;
    }
    else
    {
        aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_RCHNL;
    }
#else
    aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
#endif

    if (isLocalPlaying)
    {
        aud_param |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_param);
    return true;
}

void media_PlayAudio_standalone_locally(AUD_ID_ENUM id, uint8_t device_id)
{
    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_STANDALONE_LOCAL_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_STANDALONE_LOCAL_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

static bool media_PlayAudio_remotely_handler(AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_param;
    
#if defined(IBRT)    
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (A2DP_AUDIO_CHANNEL_SELECT_LCHNL == p_ibrt_ctrl->audio_chnl_sel)
    {
        aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_RCHNL;
    }
    else
    {
        aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_LCHNL;
    }
#else
    aud_param = PROMOT_ID_BIT_MASK_MERGING|PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
#endif

    if (isLocalPlaying)
    {
        aud_param |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_param);
    return true;
}

void media_PlayAudio_remotely(AUD_ID_ENUM id, uint8_t device_id)
{
    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_REMOTE_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_REMOTE_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

static bool media_PlayAudio_standalone_remotely_handler(AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    uint16_t aud_param;
    
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (A2DP_AUDIO_CHANNEL_SELECT_LCHNL == p_ibrt_ctrl->audio_chnl_sel)
    {
        aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_RCHNL;
    }
    else
    {
        aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_LCHNL;
    }
#else
    aud_param = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
#endif

    if (isLocalPlaying)
    {
        aud_param |= PROMOT_ID_BIT_MASK_LOCAL_PLAYING;
    }

    trigger_media_play(id, device_id, aud_param);
    return true;
}

void media_PlayAudio_standalone_remotely(AUD_ID_ENUM id, uint8_t device_id)
{
    bool isLocalPlaying = true;
    
#ifdef IBRT
    isLocalPlaying = is_prompt_playing_handling_locally(id);

    if (app_tws_ibrt_tws_link_connected()&&(!isLocalPlaying))
    {
        voice_report_role_t report_role =  app_ibrt_voice_report_get_role();
        if (VOICE_REPORT_SLAVE == report_role)
        {
            app_tws_let_peer_device_play_audio_prompt((uint16_t)id, APP_PROMPT_STANDALONE_REMOTE_PLAY, device_id);
            return;
        }
    }
#endif

    app_prompt_push_request(APP_PROMPT_STANDALONE_REMOTE_PLAY, id, device_id, isLocalPlaying);
    trigger_prompt_local(false);
}

AUD_ID_ENUM app_get_current_standalone_promptId(void)
{
    return AUD_ID_INVALID;
}

AUD_ID_ENUM media_GetCurrentPrompt(uint8_t device_id)
{
    AUD_ID_ENUM currentPromptId = AUD_ID_INVALID;
    if (app_bt_stream_isrun(APP_PLAY_BACK_AUDIO))
    {
        currentPromptId = app_get_current_standalone_promptId();
    }
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    else if (audio_prompt_is_playing_ongoing())
    {
        currentPromptId = (AUD_ID_ENUM)audio_prompt_get_prompt_id();
    }
#endif
    return currentPromptId;
}

#define IsDigit(c) (((c)>='0')&&((c)<='9'))
void media_Set_IncomingNumber(const char* pNumber)
{
    char *p_num = Media_player_number;
    uint8_t cnt = 0;
    for(uint8_t idx = 0; idx < MAX_PHB_NUMBER; idx++) {
        if(*(pNumber + idx) == 0)
            break;

        if(IsDigit(*(pNumber + idx))) {
            *(p_num + cnt) = *(pNumber + idx);
            TRACE(2,"media_Set_IncomingNumber: cnt %d ,p_num  %d", cnt, *(p_num + cnt));
            cnt ++;
        }
    }
}

PROMPT_MIX_PROPERTY_T* get_prompt_mix_property(uint16_t promptId)
{
    for (uint32_t index = 0;
         index < ((uint32_t)__mixprompt_property_table_end -
         (uint32_t)__mixprompt_property_table_start) / sizeof(PROMPT_MIX_PROPERTY_T);
         index++)
    {
        if (PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index)->promptId == promptId)
        {
            return PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    return NULL;
}
/*
Reference information for how to pass
parameters into PROMPT_MIX_PROPERTY_TO_ADD:

PROMPT_MIX_PROPERTY_TO_ADD(
promptId,
volume_level_override,
coeff_for_mix_prompt_for_music,
coeff_for_mix_music_for_music,
coeff_for_mix_prompt_for_call,
coeff_for_mix_call_for_call)
*/
void media_runtime_audio_prompt_update(uint16_t id, uint8_t** ptr, uint32_t* len)
{
#ifdef PROMPT_IN_FLASH
    nv_record_prompt_get_prompt_info(g_language, id, &g_app_audio_data, &g_app_audio_length);
#else
    switch (id)
    {
        case AUD_ID_POWER_ON:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_POWER_ON:  (U8*)EN_POWER_ON; //aud_get_reouce((AUD_ID_ENUM)id, &g_app_audio_length, &type);
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_POWER_ON): sizeof(EN_POWER_ON);
            break;
        case AUD_ID_POWER_OFF:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_POWER_OFF: (U8*)EN_POWER_OFF;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_POWER_OFF): sizeof(EN_POWER_OFF);
            break;
        case AUD_ID_BT_PAIR_ENABLE:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PAIR_ENABLE: (U8*)EN_BT_PAIR_ENABLE;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PAIR_ENABLE): sizeof(EN_BT_PAIR_ENABLE);
            break;
        case AUD_ID_BT_PAIRING:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_BT_PAIRING: (U8*)EN_BT_PAIRING;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PAIRING): sizeof(EN_BT_PAIRING);
            break;
        case AUD_ID_BT_PAIRING_SUC:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PAIRING_SUCCESS: (U8*)EN_BT_PAIRING_SUCCESS;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PAIRING_SUCCESS): sizeof(EN_BT_PAIRING_SUCCESS);
            break;
        case AUD_ID_BT_PAIRING_FAIL:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PAIRING_FAIL: (U8*)EN_BT_PAIRING_FAIL;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PAIRING_FAIL): sizeof(EN_BT_PAIRING_FAIL);
            break;
        case AUD_ID_BT_CALL_REFUSE:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_REFUSE: (U8*)EN_BT_REFUSE;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_REFUSE): sizeof(EN_BT_REFUSE);
            break;
        case AUD_ID_BT_CALL_OVER:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_OVER: (U8*)EN_BT_OVER;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_OVER): sizeof(EN_BT_OVER);
            break;
        case AUD_ID_BT_CALL_ANSWER:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_ANSWER: (U8*)EN_BT_ANSWER;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_ANSWER): sizeof(EN_BT_ANSWER);
            break;
        case AUD_ID_BT_CALL_HUNG_UP:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_HUNG_UP: (U8*)EN_BT_HUNG_UP;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_HUNG_UP): sizeof(EN_BT_HUNG_UP);
            break;
        case AUD_ID_BT_CALL_INCOMING_CALL:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_INCOMING_CALL: (U8*)EN_BT_INCOMING_CALL;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_INCOMING_CALL): sizeof(EN_BT_INCOMING_CALL);
            break;
        case AUD_ID_BT_CHARGE_PLEASE:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_CHARGE_PLEASE: (U8*)EN_CHARGE_PLEASE;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_CHARGE_PLEASE): sizeof(EN_CHARGE_PLEASE);
            break;
        case AUD_ID_BT_CHARGE_FINISH:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_CHARGE_FINISH: (U8*)EN_CHARGE_FINISH;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_CHARGE_FINISH): sizeof(EN_CHARGE_FINISH);
            break;
        case AUD_ID_BT_CONNECTED:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_CONNECTED: (U8*)EN_BT_CONNECTED;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_CONNECTED): sizeof(EN_BT_CONNECTED);
            break;
        case AUD_ID_BT_DIS_CONNECT:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_DIS_CONNECT: (U8*)EN_BT_DIS_CONNECT;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_DIS_CONNECT): sizeof(EN_BT_DIS_CONNECT);
            break;
        case AUD_ID_BT_WARNING:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_WARNING: (U8*)EN_BT_WARNING;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_WARNING): sizeof(EN_BT_WARNING);
            break;
        case AUDIO_ID_BT_ALEXA_START:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_ALEXA_START: (U8*)EN_BT_ALEXA_START;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_ALEXA_START): sizeof(EN_BT_ALEXA_START);
            break;
        case AUDIO_ID_BT_ALEXA_STOP:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_ALEXA_STOP: (U8*)EN_BT_ALEXA_STOP;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_ALEXA_STOP): sizeof(EN_BT_ALEXA_STOP);
            break;
        case AUDIO_ID_FIND_MY_BUDS:
        case AUDIO_ID_FIND_TILE:
			g_app_audio_data = (U8*)EN_FIND_MY_BUDS;
            g_app_audio_length =  sizeof(EN_FIND_MY_BUDS);
            break;
            break;
    #if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
        case AUDIO_ID_BT_GSOUND_MIC_OPEN:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_GSOUND_MIC_OPEN: (U8*)EN_BT_GSOUND_MIC_OPEN;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_GSOUND_MIC_OPEN): sizeof(EN_BT_GSOUND_MIC_OPEN);
            break;
        case AUDIO_ID_BT_GSOUND_MIC_CLOSE:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_GSOUND_MIC_CLOSE: (U8*)EN_BT_GSOUND_MIC_CLOSE;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_GSOUND_MIC_CLOSE): sizeof(EN_BT_GSOUND_MIC_CLOSE);
            break;
        case AUDIO_ID_BT_GSOUND_NC:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_GSOUND_NC: (U8*)EN_BT_GSOUND_NC;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_GSOUND_NC): sizeof(EN_BT_GSOUND_NC);
            break;
    #endif
        case AUD_ID_LANGUAGE_SWITCH:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_LANGUAGE_SWITCH: (U8*)EN_LANGUAGE_SWITCH;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_LANGUAGE_SWITCH): sizeof(EN_LANGUAGE_SWITCH);
            break;
        case AUDIO_ID_BT_MUTE:
            g_app_audio_data = (U8*)BT_MUTE;
            g_app_audio_length = sizeof(BT_MUTE);
            break;
        case AUD_ID_NUM_0:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_ZERO:  (U8*)EN_SOUND_ZERO;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_ZERO): sizeof(EN_SOUND_ZERO);
            break;
        case AUD_ID_NUM_1:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_ONE:  (U8*)EN_SOUND_ONE;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_ONE): sizeof(EN_SOUND_ONE);
            break;
        case AUD_ID_NUM_2:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_TWO:  (U8*)EN_SOUND_TWO;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_TWO): sizeof(EN_SOUND_TWO);
            break;
        case AUD_ID_NUM_3:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_THREE:  (U8*)EN_SOUND_THREE;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_THREE): sizeof(EN_SOUND_THREE);
            break;
        case AUD_ID_NUM_4:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_FOUR:  (U8*)EN_SOUND_FOUR;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_FOUR): sizeof(EN_SOUND_FOUR);
            break;
        case AUD_ID_NUM_5:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_FIVE:  (U8*)EN_SOUND_FIVE;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_FIVE): sizeof(EN_SOUND_FIVE);
            break;
        case AUD_ID_NUM_6:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_SIX:  (U8*)EN_SOUND_SIX;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_SIX): sizeof(EN_SOUND_SIX);
            break;
        case AUD_ID_NUM_7:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_SEVEN:  (U8*)EN_SOUND_SEVEN;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_SEVEN): sizeof(EN_SOUND_SEVEN);
            break;
        case AUD_ID_NUM_8:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_EIGHT:  (U8*)EN_SOUND_EIGHT;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_EIGHT): sizeof(EN_SOUND_EIGHT);
            break;
        case AUD_ID_NUM_9:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)EN_SOUND_NINE:  (U8*)EN_SOUND_NINE;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(EN_SOUND_NINE): sizeof(EN_SOUND_NINE);
            break;
        case AUD_ID_ANC_PROMPT:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)CN_PROMPT_ADAPTIVE:  (U8*)CN_PROMPT_ADAPTIVE;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(CN_PROMPT_ADAPTIVE): sizeof(CN_PROMPT_ADAPTIVE);
            break;
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
        case AUD_ID_RING_WARNING:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)? (U8*)RES_AUD_RING_SAMPRATE_16000:  (U8*)RES_AUD_RING_SAMPRATE_16000;
            g_app_audio_length =  (g_language==MEDIA_DEFAULT_LANGUAGE)? sizeof(RES_AUD_RING_SAMPRATE_16000): sizeof(RES_AUD_RING_SAMPRATE_16000);
            break;
#endif

#ifdef __INTERACTION__
        case AUD_ID_BT_FINDME:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_FINDME: (U8*)EN_BT_FINDME;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_FINDME): sizeof(EN_BT_FINDME);
            break;
#endif

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
        case AUD_ID_BT_PRESSURE_DOWN:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PROSSURE_DOWN: (U8*)EN_BT_PROSSURE_DOWN;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PROSSURE_DOWN): sizeof(EN_BT_PROSSURE_DOWN);
            break;

        case AUD_ID_BT_PRESSURE_UP:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PROSSURE_UP: (U8*)EN_BT_PROSSURE_UP;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PROSSURE_UP): sizeof(EN_BT_PROSSURE_UP);
            break;
#endif

        case AUD_ID_BT_SEALING_AUDIO:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_SEALING_AUDIO: (U8*)EN_BT_SEALING_AUDIO;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_SEALING_AUDIO): sizeof(EN_BT_SEALING_AUDIO);
            break;

//jinyao_learning:播放welcome提示音

#ifdef jinyao_learning
        case AUDIO_ID_BT_WELCOME:
            g_app_audio_data = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_WELCOME: (U8*)EN_BT_WELCOME;
            g_app_audio_length = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_WELCOME): sizeof(EN_BT_WELCOME);
            break;
#endif



        default:
            g_app_audio_length = 0;
            break;
        }
#endif

      *ptr = g_app_audio_data;
      *len = g_app_audio_length;
}

static void _get_number_sound_map(int language)
{
#ifdef PROMPT_IN_FLASH
    for (uint8_t promptId = AUD_ID_NUM_0; promptId <= AUD_ID_NUM_9; promptId++)
    {
        nv_record_prompt_get_prompt_info(language, promptId,
                                         &media_number_sound_map[promptId - AUD_ID_NUM_0].data,
                                         &media_number_sound_map[promptId - AUD_ID_NUM_0].fsize);
    }

    media_sound_map = media_number_sound_map;
#else
    if (language == MEDIA_DEFAULT_LANGUAGE)
    {
        media_sound_map = media_sound_map_en;
    }
    else
    {
        media_sound_map = media_sound_map_en;
    }
#endif
}

void media_Play_init_audio(uint16_t aud_id)
{
    if (aud_id == AUD_ID_BT_CALL_INCOMING_NUMBER)
    {
        g_play_continue_mark = 1;
        _get_number_sound_map(g_language);

        memset(&pCont_context, 0x0, sizeof(pCont_context));

        pCont_context.g_play_continue_total = strlen((const char *)Media_player_number);

        for (uint32_t i = 0; (i < pCont_context.g_play_continue_total) && (i < MAX_PHB_NUMBER); i++)
        {
            pCont_context.g_play_continue_array[i] = Media_player_number[i] - '0';

            TRACE(3, "media_PlayNumber, pCont_context.g_play_continue_array[%d] = %d, total =%d",
                  i, pCont_context.g_play_continue_array[i], pCont_context.g_play_continue_total);
        }
    }
    else
    {
        g_app_audio_read = 0;
        g_play_continue_mark = 0;

        media_runtime_audio_prompt_update(aud_id, &g_app_audio_data, &g_app_audio_length);
    }
}

uint32_t app_play_sbc_more_data_fadeout(int16_t *buf, uint32_t len)
{
    uint32_t i;
    uint32_t j = 0;

    for (i = len; i > 0; i--){
        *(buf+j) = *(buf+j)*i/len;
        j++;
    }

    return len;
}

static uint32_t need_fadein_len = 0;
static uint32_t need_fadein_len_processed = 0;

int app_play_sbc_more_data_fadein_config(uint32_t len)
{
    TRACE(1,"fadein_config l:%d", len);
    need_fadein_len = len;
    need_fadein_len_processed = 0;
    return 0;
}
uint32_t app_play_sbc_more_data_fadein(int16_t *buf, uint32_t len)
{
    uint32_t i;
    uint32_t j = 0;
    uint32_t base;
    uint32_t dest;

    base = need_fadein_len_processed;
    dest = need_fadein_len_processed + len < need_fadein_len ?
           need_fadein_len_processed + len :
           need_fadein_len_processed;

    if (base >= dest){
//        TRACE(0,"skip fadein");
        return len;
    }
//    TRACE(3,"fadein l:%d base:%d dest:%d", len, base, dest);
//    DUMP16("%5d ", buf, 20);
//    DUMP16("%5d ", buf+len-19, 20);

    for (i = base; i < dest; i++){
        *(buf+j) = *(buf+j)*i/need_fadein_len;
        j++;
    }

    need_fadein_len_processed += j;
//    DUMP16("%05d ", buf, 20);
//    DUMP16("%5d ", buf+len-19, 20);
    return len;
}

uint32_t app_play_single_sbc_more_data(uint8_t device_id, uint8_t *buf, uint32_t len)
{
    //int32_t stime, etime;
    //U16 byte_decode;
    uint32_t l = 0;

     //TRACE(2,"app_play_sbc_more_data : %d, %d", g_app_audio_read, g_app_audio_length);

    if (g_app_audio_read < g_app_audio_length){
        unsigned int available_len = 0;
        unsigned int store_len = 0;

        available_len = AvailableOfCQueue(&media_sbc_queue);
        store_len = (g_app_audio_length - g_app_audio_read) > available_len ? available_len :(g_app_audio_length - g_app_audio_read);
        app_media_store_sbc_buffer(device_id, (unsigned char *)(g_app_audio_data + g_app_audio_read), store_len);
        g_app_audio_read += store_len;
    }


        l = decode_sbc_frame(buf, len);

        if (l != len)
        {
            g_app_audio_read = g_app_audio_length;
            //af_stream_stop(AUD_STREAM_PLAYBACK);
            //af_stream_close(AUD_STREAM_PLAYBACK);
            TRACE(4,"[%s]-->need close, length:%d len:%d l:%d", __func__,g_app_audio_length, len, l);
        }

    return l;
}


/* play continue sound */
uint32_t app_play_continue_sbc_more_data(uint8_t device_id, uint8_t *buf, uint32_t len)
{

    uint32_t l, n, fsize = 0;

    U8*  pdata;

// store data
    unsigned int available_len = 0;
    unsigned int store_len = 0;

    if (pCont_context.g_play_continue_n < pCont_context.g_play_continue_total){
        do {
            n = pCont_context.g_play_continue_n;
            pdata = media_sound_map[pCont_context.g_play_continue_array[n]].data;
            fsize = media_sound_map[pCont_context.g_play_continue_array[n]].fsize;

            available_len = AvailableOfCQueue(&media_sbc_queue);
            if (!available_len)
                break;

            store_len = (fsize - pCont_context.g_play_continue_fread) > available_len ? available_len :(fsize - pCont_context.g_play_continue_fread);
            app_media_store_sbc_buffer(device_id, (unsigned char *)(pdata + pCont_context.g_play_continue_fread), store_len);
            pCont_context.g_play_continue_fread += store_len;
            if (pCont_context.g_play_continue_fread == fsize){
                pCont_context.g_play_continue_n++;
                pCont_context.g_play_continue_fread = 0;
            }
        }while(pCont_context.g_play_continue_n < pCont_context.g_play_continue_total);
    }

    l = decode_sbc_frame(buf, len);

    if (l !=len){
        TRACE(0,"continue sbc decode ---> APP_BT_SETTING_CLOSE");
    }

    return l;
}

uint32_t app_play_sbc_channel_select(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
#if defined(IBRT)
    if (IS_PROMPT_CHNLSEl_ALL(g_prompt_chnlsel) || app_ibrt_voice_report_is_me(PROMPT_CHNLSEl_FROM_ID_VALUE(g_prompt_chnlsel)))
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
        }
    }
    else
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = 0;
        }
    }
#else
    if (IS_PROMPT_CHNLSEl_ALL(g_prompt_chnlsel))
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
        }
    }
    else if (IS_PROMPT_CHNLSEl_LCHNL(g_prompt_chnlsel))
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = src_buf[i];
            dst_buf[i*2 + 1] = 0;
        }
    }
    else if (IS_PROMPT_CHNLSEl_RCHNL(g_prompt_chnlsel))
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = 0;
            dst_buf[i*2 + 1] = src_buf[i];
        }
    }
    else
    {
        // Copy from tail so that it works even if dst_buf == src_buf
        for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = 0;
        }
    }
#endif
    return 0;
}

#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
static uint32_t g_prompt_upsampling_ratio = 3;
extern void us_fir_init(uint32_t upsampling_ratio);
extern uint32_t us_fir_run(short *src_buf, short *dst_buf, uint32_t in_samp_num);
#endif

uint32_t g_cache_buff_sz = 0;

static int16_t *app_play_sbc_cache = NULL;
uint32_t app_play_sbc_more_data(uint8_t *buf, uint32_t len)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
#if defined(IBRT)
    app_ibrt_voice_report_trigger_checker();
#endif
#if defined(ANC_ASSIST_ENABLED)     
    app_voice_assist_prompt_leak_detect_set_working_status(1);
#endif

    uint32_t l = 0;

    memset(buf, 0, len);

#if defined(MEDIA_PLAY_24BIT)
    len /= 2;
#endif

#ifdef APP_MIC_CAPTURE_LOOPBACK
        if(app_play_audio_pending_stream_get() == STREAM_PENDING_OF_TYPE_CAPTURE){
            app_play_audio_pending_stream_set(STREAM_PENDING_OF_TYPE_CAPTURE,false);
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
#endif

#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
    uint32_t dec_len = len/g_prompt_upsampling_ratio;
#endif

    if(app_play_sbc_cache)
        memset(app_play_sbc_cache, 0, g_cache_buff_sz);

    if (app_play_sbc_stop_proc_cnt) {
        if (app_play_sbc_stop_proc_cnt == 1) {
            app_play_sbc_stop_proc_cnt = 2;
        } else if (app_play_sbc_stop_proc_cnt == 2) {
            app_play_sbc_stop_proc_cnt = 3;

            // For 8K sample rate data, it takes about 4ms (or 12ms if h/w resample in use) from codec to DAC PA.
            // The playback stream should be stopped after the last data arrives at DAC PA, otherwise there
            // might be some pop sound.
            app_play_audio_stop();
        }
    } else {
        if (app_play_sbc_cache) {
#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
            len = dec_len;
#endif
            if (g_play_continue_mark) {
                l = app_play_continue_sbc_more_data(device_id, (uint8_t *)app_play_sbc_cache, len/MEDIA_PLAYER_CHANNEL_NUM);
            } else {
                l = app_play_single_sbc_more_data(device_id, (uint8_t *)app_play_sbc_cache, len/MEDIA_PLAYER_CHANNEL_NUM);
            }
            if (l != len / MEDIA_PLAYER_CHANNEL_NUM) {
#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
                len = dec_len*3;
#endif
                memset(app_play_sbc_cache+l, 0, len/MEDIA_PLAYER_CHANNEL_NUM-l);
                app_play_sbc_stop_proc_cnt = 1;
            }
#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
            len = dec_len*3;
            l = l*3;
            us_fir_run((short*)app_play_sbc_cache, (short*)buf, dec_len/MEDIA_PLAYER_CHANNEL_NUM/sizeof(int16_t));
            if (MEDIA_PLAYER_CHANNEL_NUM == AUD_CHANNEL_NUM_2) {
                app_play_sbc_channel_select((int16_t*)buf, (int16_t*)buf, len/MEDIA_PLAYER_CHANNEL_NUM/sizeof(int16_t));
            }
#else
            if (MEDIA_PLAYER_CHANNEL_NUM == AUD_CHANNEL_NUM_2) {
                app_play_sbc_channel_select((int16_t *)buf, app_play_sbc_cache, len/MEDIA_PLAYER_CHANNEL_NUM/sizeof(int16_t));
            } else {
                memcpy(buf, app_play_sbc_cache, len);
            }
#endif

#if defined(MEDIA_PLAY_24BIT)
            int32_t *buf32 = (int32_t *)buf;
            int16_t *buf16 = (int16_t *)buf;

            for (int16_t i = len/2 - 1; i >= 0; i--) {
                buf32[i] = ((int32_t)buf16[i] << 8);
            }
            len *= 2;
#endif
        } else {
#if defined(MEDIA_PLAY_24BIT)
            len *= 2;
#endif
            memset(buf, 0, len);
        }
    }
#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
#if defined(MEDIA_PLAY_24BIT)
    iir_run(buf, len/sizeof(int32_t));
#else
    iir_run(buf, len/sizeof(int16_t));
#endif
#endif

    return l;
}

#ifdef AUDIO_LINEIN
static uint8_t app_play_lineinmode_merge = 0;
static uint8_t app_play_lineinmode_mode = 0;

inline static void app_play_audio_lineinmode_mono_merge(int16_t *aud_buf_mono, int16_t *ring_buf_mono, uint32_t aud_buf_len)
{
    uint32_t i = 0;
    for (i = 0; i < aud_buf_len; i++) {
        aud_buf_mono[i] = (aud_buf_mono[i]>>1) + (ring_buf_mono[i]>>1);
    }
}

inline static void app_play_audio_lineinmode_stereo_merge(int16_t *aud_buf_stereo, int16_t *ring_buf_mono, uint32_t aud_buf_len)
{
    uint32_t aud_buf_stereo_offset = 0;
    uint32_t ring_buf_mono_offset = 0;
    for (aud_buf_stereo_offset = 0; aud_buf_stereo_offset < aud_buf_len; ) {
        aud_buf_stereo[aud_buf_stereo_offset] = aud_buf_stereo[aud_buf_stereo_offset] + (ring_buf_mono[ring_buf_mono_offset]>>1);
        aud_buf_stereo_offset++;
        aud_buf_stereo[aud_buf_stereo_offset] = aud_buf_stereo[aud_buf_stereo_offset] + (ring_buf_mono[ring_buf_mono_offset]>>1);
        aud_buf_stereo_offset++;
        ring_buf_mono_offset++;
    }
}

uint32_t app_play_audio_lineinmode_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    if (app_play_lineinmode_merge && app_play_sbc_cache){
        TRACE(1,"line in mode:%d ", len);
        if (app_play_lineinmode_mode == 1){
            if (g_play_continue_mark){
                l = app_play_continue_sbc_more_data((uint8_t *)app_play_sbc_cache, len);
            }else{
                l = app_play_single_sbc_more_data((uint8_t *)app_play_sbc_cache, len);
            }
            if (l != len){
                memset(app_play_sbc_cache+l, 0, len-l);
                app_play_lineinmode_merge = 0;
            }
            app_play_audio_lineinmode_mono_merge((int16_t *)buf, (int16_t *)app_play_sbc_cache, len/2);
        }else if (app_play_lineinmode_mode == 2){
            if (g_play_continue_mark){
                l = app_play_continue_sbc_more_data((uint8_t *)app_play_sbc_cache, len/2);
            }else{
                l = app_play_single_sbc_more_data((uint8_t *)app_play_sbc_cache, len/2);
            }
            if (l != len/2){
                memset(app_play_sbc_cache+l, 0, len/2-l);
                app_play_lineinmode_merge = 0;
            }
            app_play_audio_lineinmode_stereo_merge((int16_t *)buf, (int16_t *)app_play_sbc_cache, len/2);
        }
    }

    return l;
}

int app_play_audio_lineinmode_init(uint8_t mode, uint32_t buff_len)
{
    TRACE(1,"lapp_play_audio_lineinmode_init:%d ", buff_len);
    app_play_lineinmode_mode = mode;
    app_audio_mempool_get_buff((uint8_t **)&app_play_sbc_cache, buff_len);
    media_audio_init();
    return 0;
}

int app_play_audio_lineinmode_start(APP_AUDIO_STATUS* status)
{
    if (app_play_audio_sample_rate == AUD_SAMPRATE_44100){
        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_DeCQueue(&media_sbc_queue, 0, APP_AUDIO_LengthOfCQueue(&media_sbc_queue));
        UNLOCK_APP_AUDIO_QUEUE();
        app_play_lineinmode_merge = 1;
        need_init_decoder = 1;
        media_Play_init_audio(status->aud_id);
    }
    return 0;
}

int app_play_audio_lineinmode_stop(APP_AUDIO_STATUS* status)
{
    app_play_lineinmode_merge = 0;
    return 0;
}
#endif


#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#define DELAY_SAMPLE_MC (33*2)     //  2:ch
static int32_t delay_buf_media[DELAY_SAMPLE_MC];
static uint32_t audio_mc_data_playback_media(uint8_t *buf, uint32_t mc_len_bytes)
{
   uint32_t begin_time;
   //uint32_t end_time;
   begin_time = hal_sys_timer_get();
   TRACE(1,"media cancel: %d",begin_time);

   float left_gain;
   float right_gain;
   int32_t playback_len_bytes,mc_len_bytes_8;
   int32_t i,j,k;
   int delay_sample;

   mc_len_bytes_8=mc_len_bytes/8;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   //TRACE(1,"playback_samplerate_ratio:  %d",playback_samplerate_ratio);

  // TRACE(1,"left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE(1,"right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_media[i];
            mid_p[j++]=delay_buf_media[i+1];
        }

        for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

        for(j=0;i<playback_len_bytes/2;i=i+2)
        {
            delay_buf_media[j++]=sour_p[i];
            delay_buf_media[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(8/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_16);

        for(i=0,j=0;i<(mc_len_bytes_8)/2;i=i+2)
        {
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8[i];
                dest_p[j++]=mid_p_8[i+1];
            }
        }


    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == (playback_buf_mc))
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_media[i];
            mid_p[j++]=delay_buf_media[i+1];

        }

         for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

         for(j=0;i<playback_len_bytes/4;i=i+2)
        {
            delay_buf_media[j++]=sour_p[i];
            delay_buf_media[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(8/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_24);

        for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+2)
        {
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8[i];
                dest_p[j++]=mid_p_8[i+1];
            }
        }

    }

  //  end_time = hal_sys_timer_get();

 //   TRACE(2,"%s:run time: %d", __FUNCTION__, end_time-begin_time);

    return 0;
}
#endif

void app_audio_playback_done(void)
{
#if defined(IBRT)
    app_tws_sync_prompt_check();
#endif
}

extern void bt_media_clear_media_type(uint16_t media_type,enum BT_DEVICE_ID_T device_id);

int app_play_audio_stop(void)
{
    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
    bt_media_clear_current_media(BT_STREAM_MEDIA);
    for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
    {
        bt_media_clear_media_type(BT_STREAM_MEDIA, (enum BT_DEVICE_ID_T)i);
    }
    return 0;
}


uint32_t g_active_aud_id = MAX_RECORD_NUM;

int app_play_audio_set_aud_id(uint32_t aud_id)
{
    g_active_aud_id = aud_id;
    return 0;
}

int app_play_audio_get_aud_id(void)
{
    return g_active_aud_id;
}

#ifdef MEDIA_PLAYER_USE_CODEC2
bool is_play_audio_prompt_use_dac1()
{
    TRACE(1, "g_active_aud_id 0x%x", g_active_aud_id);

    if(g_active_aud_id == AUD_ID_BT_SEALING_AUDIO)
        return true;
    else
        return false;
}
#endif

void app_play_audio_set_lang(int L)
{
    g_language = L;
    TRACE(1, "language is set to: %d", g_language);
}

int app_play_audio_get_lang()
{
    return g_language;
}

#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
static int copy_prompt_sw_eq_band(enum AUD_SAMPRATE_T sample_rate, IIR_CFG_T *prompt_sw_eq_cfg, IIR_CFG_T *a2dp_eq_cfg)
{
    prompt_sw_eq_cfg->gain0 = a2dp_eq_cfg->gain0;
    prompt_sw_eq_cfg->gain1 = a2dp_eq_cfg->gain1;
    prompt_sw_eq_cfg->num = 0;
    for(int i =0, j=0; i< a2dp_eq_cfg->num; i++){
        if(a2dp_eq_cfg->param[i].fc < ((float)sample_rate/2)){
            prompt_sw_eq_cfg->param[j++] = a2dp_eq_cfg->param[i];
            prompt_sw_eq_cfg->num++;
        }
    }

    return 0;
}
#endif

#ifdef APP_MIC_CAPTURE_LOOPBACK
static uint16_t app_play_audio_pending_stream_types ;
void app_play_audio_pending_stream_set(uint16_t type , bool set)
{
    if(set == true){
        app_play_audio_pending_stream_types |= (1<<type);
    }else{
        app_play_audio_pending_stream_types &= (~(1<<type) & 0xffff);
    }
}
uint16_t app_play_audio_pending_stream_get(void)
{
    uint16_t type = 0;
    uint8_t i = 0;
    
    for(i=0;i<STREAM_PENDING_OF_TYPE_MAX;i++){
        type = 1<<i;
        if(type == app_play_audio_pending_stream_types){
            return i;
        }
    }

    return STREAM_PENDING_OF_TYPE_MAX;
}
#endif

int app_play_audio_onoff(bool onoff, APP_AUDIO_STATUS* status)
{
    static bool isRun =  false;
    enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_32K;
    struct AF_STREAM_CONFIG_T stream_cfg;
    uint8_t* bt_audio_buff = NULL;
    uint16_t bytes_parsed = 0;
    enum AUD_SAMPRATE_T sample_rate POSSIBLY_UNUSED = AUD_SAMPRATE_16000;
    uint16_t aud_id = 0;
    uint16_t aud_pram = 0;
    int ret = 0;
    AUD_STREAM_ID_T media_play_stream_id = MEDIA_PLAY_STREAM_ID;
    POSSIBLY_UNUSED app_media_mempool_get_buff_cb get_buff_cb = NULL;
    POSSIBLY_UNUSED static bool media_use_dac2 = false;

    TRACE(2,"Audio prompt stream state %s, to %s it",
        isRun?"Running":"Idle", onoff?"start":"stop");

    if (isRun == onoff)
    {
        return 0;
    }

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);

    if (onoff ) {
// #if defined(ANC_ASSIST_ENABLED)
//         app_anc_assist_set_mode(ANC_ASSIST_MODE_NONE);
// #endif

#if defined(__THIRDPARTY)
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_STOP, 0);
#endif

#if defined(IBRT)
        bool isPlayingLocally = false;
        if (IS_PROMPT_PLAYED_LOCALLY(status->aud_id))
        {
            isPlayingLocally = true;
        }
#endif

        aud_id = PROMPT_ID_FROM_ID_VALUE(status->aud_id);
        aud_pram = PROMPT_PRAM_FROM_ID_VALUE(status->aud_id);
        g_prompt_chnlsel = PROMPT_CHNLSEl_FROM_ID_VALUE(aud_pram);
        TRACE(4,"aud_id:%04x %s aud_pram:%04x chnlsel:%d", status->aud_id, aud_id2str(aud_id), aud_pram, g_prompt_chnlsel);
#if defined(__XSPACE_UI__)
        app_prompt_on_going_prompt_id2 = (AUD_ID_ENUM)aud_id;
#endif
        media_Play_init_audio(aud_id);//配置音频文本文件
        if(aud_id == AUD_ID_ANC_PROMPT){         
            app_voice_assist_prompt_leak_detect_open();
            app_voice_assist_prompt_leak_detect_set_working_status(0);
        }
#endif
        app_play_audio_set_aud_id(aud_id);
        if (!g_app_audio_length){
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
            return 0;
        }

        freq = APP_SYSFREQ_104M;

#if defined(MEDIA_PLAYER_USE_CODEC2)
        media_use_dac2 = true;
        get_buff_cb = app_media_mempool_get_buff;
        if(is_play_audio_prompt_use_dac1()){ 
            media_use_dac2 = false;
            get_buff_cb = syspool_get_buff;
            media_play_stream_id = AUD_STREAM_ID_0;
        }
#endif

        af_set_priority(AF_USER_AUDIO, osPriorityHigh);
#if defined(MEDIA_PLAYER_USE_CODEC2)
        if(media_use_dac2){
            app_media_mempool_init();
        }else
#endif
        {
            app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
        }

        media_audio_init();

        btif_sbc_init_decoder(media_sbc_decoder);
        btif_sbc_decode_frames(media_sbc_decoder,
                               g_app_audio_data, g_app_audio_length,
                               &bytes_parsed,
                               NULL, 0,
                               NULL);
        switch (media_sbc_decoder->streamInfo.sampleFreq)
        {
            case BTIF_SBC_CHNL_SAMPLE_FREQ_16:
                sample_rate = AUD_SAMPRATE_16000;
                break;
            case BTIF_SBC_CHNL_SAMPLE_FREQ_32:
                sample_rate = AUD_SAMPRATE_32000;
                break;
            case BTIF_SBC_CHNL_SAMPLE_FREQ_44_1:
                sample_rate = AUD_SAMPRATE_44100;
                break;
            case BTIF_SBC_CHNL_SAMPLE_FREQ_48:
                sample_rate = AUD_SAMPRATE_48000;
                break;
            default:
                sample_rate = AUD_SAMPRATE_16000;
                break;
        }

#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_disable();
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

#if defined(MEDIA_PLAY_24BIT)
        stream_cfg.bits = AUD_BITS_24;
#else
        stream_cfg.bits = AUD_BITS_16;
#endif
        stream_cfg.channel_num = MEDIA_PLAYER_CHANNEL_NUM;
#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
        stream_cfg.sample_rate =  AUD_SAMPRATE_48000;
#else
        stream_cfg.sample_rate =  AUD_SAMPRATE_16000;
#endif

#ifdef PLAYBACK_USE_I2S
        stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
        stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        }else
#endif
        {
            stream_cfg.device = MEDIA_PLAYER_OUTPUT_DEVICE;
        }

        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif
#ifdef __INTERACTION__
        if(aud_id == AUD_ID_BT_FINDME)
        {
            stream_cfg.vol = g_findme_fadein_vol;
        }
        else
#endif
        {
            stream_cfg.vol = MEDIA_VOLUME_LEVEL_WARNINGTONE;
        }
        stream_cfg.handler = app_play_sbc_more_data;

#ifdef APP_MIC_CAPTURE_LOOPBACK
        if(aud_id == AUD_ID_BT_SEALING_AUDIO){
        /*
            same with :
#define APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN   (128*2*2)
        */
            stream_cfg.data_size = APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN * 2;
        }else
#endif
        {
            stream_cfg.data_size = APP_AUDIO_PLAYBACK_BUFF_SIZE;
        }
        g_cache_buff_sz = stream_cfg.data_size/MEDIA_PLAYER_CHANNEL_NUM/2 ;

#if defined(MEDIA_PLAY_24BIT)
        stream_cfg.data_size *= 2;
#endif

#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            syspool_get_buff((uint8_t **)&app_play_sbc_cache, g_cache_buff_sz);
            syspool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        }else
#endif
        {
            app_audio_mempool_get_buff((uint8_t **)&app_play_sbc_cache, g_cache_buff_sz);
            app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        }
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
        us_fir_init(g_prompt_upsampling_ratio);
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;
#if defined(__BT_ANC__) || defined(PSAP_FORCE_STREAM_48K)
        playback_samplerate_ratio_bt=8;
#else
        playback_samplerate_ratio_bt=8*3;
#endif
        playback_ch_num_bt=stream_cfg.channel_num;
#endif

        TRACE(0, "[%s]: sample rate: %d, bits: %d, channel number: %d, data size:%d",
            __func__,
            stream_cfg.sample_rate,
            stream_cfg.bits,
            stream_cfg.channel_num,
            stream_cfg.data_size);

#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
        IIR_CFG_T prompt_iir_cfg;
        copy_prompt_sw_eq_band(stream_cfg.sample_rate, &prompt_iir_cfg, audio_eq_hw_dac_iir_cfg_list[0]);
        iir_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num);
        iir_set_cfg(&prompt_iir_cfg);
#endif
        af_stream_open(media_play_stream_id, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
        stream_cfg.handler = audio_mc_data_playback_media;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            syspool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        }else
#endif
        {
            app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        }
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*playback_samplerate_ratio_bt;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));
        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
#endif

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            af_codec_tune(AUD_STREAM_PLAYBACK, 0);
        }else
#endif
        {
            AF_CODEC_TUNE(AUD_STREAM_PLAYBACK, 0);
        }
#endif

#ifdef APP_MIC_CAPTURE_LOOPBACK
        if (app_mic_capture_procedure_enable_flag_set_filter((uint8_t*)&(aud_id),0))
        {
            app_mic_capture_procedure_algorithm_init();
            app_mic_capture_procedure_struct_t mic_capture_config;
            app_mic_capture_procedure_configure_set((void*)&mic_capture_config);
            app_mic_capture_procedure_onoff(true,(void *)&mic_capture_config);
            app_play_audio_pending_stream_set(STREAM_PENDING_OF_TYPE_CAPTURE,true);
        }
#endif

#if defined(IBRT)
        if(!isPlayingLocally)
        {
#if defined MEDIA_PLAYER_USE_CODEC2
            app_ibrt_voice_resport_trigger_device_t trigger_device = {
                .device = AUD_STREAM_USE_INT_CODEC2,
                .trigger_channel = 1,
            };
            if(!media_use_dac2){
                trigger_device = {
                .device = AUD_STREAM_USE_INT_CODEC,
                .trigger_channel = 0,
                };
            }   
#else
            app_ibrt_voice_resport_trigger_device_t trigger_device = {
                .device = AUD_STREAM_USE_INT_CODEC,
                .trigger_channel = 0,
            };
#endif

            ret = app_ibrt_voice_report_trigger_init(aud_id, aud_pram, &trigger_device);
        }
#endif
        if (0 == ret)
        {
            af_stream_start(media_play_stream_id, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
            af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
        }
    }
    else
    {
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            af_codec_tune(AUD_STREAM_PLAYBACK, 0);
        }else
#endif
        AF_CODEC_TUNE(AUD_STREAM_PLAYBACK, 0);
#endif

#ifdef MEDIA_PLAYER_USE_CODEC2
        if(!media_use_dac2){
            media_play_stream_id = AUD_STREAM_ID_0;
        }
#endif
        af_stream_stop(media_play_stream_id, AUD_STREAM_PLAYBACK);
        af_stream_close(media_play_stream_id, AUD_STREAM_PLAYBACK);
        
#if defined(__SW_IIR_PROMPT_EQ_PROCESS__)
        iir_close();
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
#if defined(IBRT)
        app_ibrt_voice_report_trigger_deinit();
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();
#endif
#endif
#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_enable();
#endif

        app_play_sbc_cache = NULL;
        g_cache_buff_sz = 0;
        g_prompt_chnlsel = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
        app_play_audio_set_aud_id(MAX_RECORD_NUM);
        af_set_priority(AF_USER_AUDIO, osPriorityAboveNormal);
        freq = APP_SYSFREQ_32K;

        app_audio_playback_done();
#if defined(__THIRDPARTY)
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START, 0);
#endif
#ifdef APP_MIC_CAPTURE_LOOPBACK
        app_play_audio_pending_stream_set(STREAM_PENDING_OF_TYPE_AUDIO,false);
        app_mic_capture_procedure_onoff(false,NULL);
        app_mic_capture_procedure_enabled_clear();
#endif

#if defined(ANC_ASSIST_ENABLED)  
        app_voice_assist_prompt_leak_detect_close();
#endif

// #if defined(ANC_ASSIST_ENABLED)
//         app_anc_assist_set_mode(ANC_ASSIST_MODE_STANDALONE);
// #endif
    }

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] sysfreq calc : %d", __FUNCTION__, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

    isRun = onoff;

    return 0;
}

static void app_stop_local_prompt_playing(void)
{
    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, APP_BT_SETTING_CLOSE, 0);
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_stop_playing();
#endif
}

void app_stop_both_prompt_playing(void)
{
    app_stop_local_prompt_playing();
    app_tws_stop_peer_prompt();
}

void app_tws_cmd_stop_prompt_handler(uint8_t* ptrParam, uint16_t paramLen)
{
    TRACE(1,"%s", __func__);
    app_stop_local_prompt_playing();
}

static void app_prompt_protector_timer_handler(void const *param);
osTimerDef(app_prompt_protector_timer,
    app_prompt_protector_timer_handler);
static osTimerId app_prompt_protector_timer_id = NULL;

// shall cover the longest prompt, if a prompt is longer than this expire time,
// increase it
#define APP_PROMPT_PROTECTOR_EXPIRE_TIMEOUT_MS  15000

static void app_prompt_start_protector(void)
{
    if (NULL == app_prompt_protector_timer_id)
    {
        app_prompt_protector_timer_id = 
            osTimerCreate(osTimer(app_prompt_protector_timer), osTimerOnce, NULL);
    }

    osTimerStart(app_prompt_protector_timer_id, APP_PROMPT_PROTECTOR_EXPIRE_TIMEOUT_MS);
}

static void app_prompt_protector_timer_handler(void const *param)
{
    TRACE(0, "a wild prompt has blocked system for %d ms!", 
        APP_PROMPT_PROTECTOR_EXPIRE_TIMEOUT_MS);
    // timeout, clear the prompt list
    for (uint32_t i = 0;i < sizeof(app_prompt_continous_id)/sizeof(AUD_ID_ENUM);i++)
    {
        if ((app_prompt_on_going_prompt_id == app_prompt_continous_id[i])&&
            app_bt_stream_isrun(APP_PLAY_BACK_AUDIO))
        {
            app_prompt_start_protector();
            return;
        }
    }
    app_prompt_stop_all();
}

static void app_prompt_stop_protector(void)
{
    if (app_prompt_protector_timer_id)
    {
        osTimerStop(app_prompt_protector_timer_id);
    }
}

#define APP_PROMPT_MAXIMUM_QUEUED_CNT   8

osPoolDef (app_prompt_request_mempool, APP_PROMPT_MAXIMUM_QUEUED_CNT, APP_PROMPT_PLAY_REQ_T);
static osPoolId app_prompt_request_mempool = NULL;

static list_t *app_prompt_list;
static bool app_prompt_is_on_going = false;

static list_node_t *app_prompt_list_begin(const list_t *list)
{
    uint32_t lock = int_lock_global();
    list_node_t *node = list_begin(list);
    int_unlock_global(lock);
    return node;
}

static APP_PROMPT_PLAY_REQ_T *app_prompt_list_node(const list_node_t *node)
{
    uint32_t lock = int_lock_global();
    void *data = list_node(node);
    int_unlock_global(lock);
    return (APP_PROMPT_PLAY_REQ_T *)data;
}

static bool app_prompt_list_remove(list_t *list, void *data)
{
    uint32_t lock = int_lock_global();
    bool nRet = list_remove(list, data);
    int_unlock_global(lock);
    return nRet;
}

static bool app_prompt_list_append(list_t *list, APP_PROMPT_PLAY_REQ_T *req)
{
    APP_PROMPT_PLAY_REQ_T* reqToAppend = 
        (APP_PROMPT_PLAY_REQ_T *)osPoolCAlloc (app_prompt_request_mempool);
    if (NULL == reqToAppend)
    {
        TRACE(0, "prompt request memory pool is full.");
        return false;
    }

    memcpy((uint8_t *)reqToAppend,  (uint8_t *)req, sizeof(APP_PROMPT_PLAY_REQ_T));
    
    uint32_t lock = int_lock_global();
    bool nRet = list_append(list, (void *)reqToAppend);
    int_unlock_global(lock);
    return nRet;
}

static void app_prompt_list_clear(list_t *list)
{
    uint32_t lock = int_lock_global();
    list_clear(list);
    int_unlock_global(lock);
}

static void app_prompt_list_free(void* data)
{
    osPoolFree (app_prompt_request_mempool, data);
}

void app_prompt_list_init(void)
{    
    if (NULL == app_prompt_request_mempool)
    {
        app_prompt_request_mempool = 
            osPoolCreate(osPool(app_prompt_request_mempool));
    }

    app_prompt_list = list_new(app_prompt_list_free, NULL, NULL);

    app_prompt_handler_tid = 
        osThreadCreate(osThread(app_prompt_handler_thread), NULL);
}

static void app_prompt_clear_prompt_list(void);

void app_prompt_push_request(APP_PROMPT_PLAY_REQ_TYPE_E reqType,
    AUD_ID_ENUM id, uint8_t device_id, bool isLocalPlaying)
{
    APP_PROMPT_PLAY_REQ_T req;
    req.deviceId = device_id;
    req.promptId = (uint16_t)id;
    req.reqType = reqType;
    req.isLocalPlaying = isLocalPlaying;

    TRACE(0, "push prompt request type %d devId %d promptId 0x%x isLocalPlaying %d",
        reqType, device_id, id, isLocalPlaying);

    if (AUDIO_ID_BT_MUTE == id)
    {
        app_prompt_clear_prompt_list();
    }
#ifdef APP_MIC_CAPTURE_LOOPBACK
    if(id  == AUD_ID_BT_SEALING_AUDIO)
    {
        app_prompt_clear_prompt_list();
    }
#endif

    bool ret = app_prompt_list_append(app_prompt_list, &req);
    if (ret)
    {
        osSignalSet(app_prompt_handler_tid, PROMPT_HANDLER_SIGNAL_NEW_PROMPT_REQ);
    }
}

static bool app_prompt_refresh_list(void)
{
    if (app_prompt_is_on_going)
    {
        TRACE(0, "A prompt playing is on-going.");
        return false;
    }

    list_t *list = app_prompt_list;
    list_node_t *node = NULL;
    APP_PROMPT_PLAY_REQ_T* pPlayReq = NULL;

    if ((node = app_prompt_list_begin(list)) != NULL) {
        pPlayReq = app_prompt_list_node(node);
        app_prompt_is_on_going = true;
        app_prompt_on_going_prompt_id = (AUD_ID_ENUM)pPlayReq->promptId;
        app_prompt_start_protector();
        bool isSuccessfullySubmitted = true;
        switch (pPlayReq->reqType)
        {
            case APP_PROMPT_NORMAL_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            case APP_PROMPT_LOCAL_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_locally_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            case APP_PROMPT_REMOTE_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_remotely_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            case APP_PROMPT_STANDALONE_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_standalone_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            case APP_PROMPT_STANDALONE_LOCAL_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_standalone_locally_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            case APP_PROMPT_STANDALONE_REMOTE_PLAY:
                isSuccessfullySubmitted = media_PlayAudio_standalone_remotely_handler(
                    (AUD_ID_ENUM)pPlayReq->promptId, pPlayReq->deviceId, pPlayReq->isLocalPlaying);
                break;
            default:
                ASSERT(false, "Invalid prompt req type %d.", pPlayReq->reqType);
                break;
        }

        app_prompt_list_remove(list, pPlayReq);

        if (!isSuccessfullySubmitted)
        {
            TRACE(0, "Prompt is not successfully submitted because of tws disconnection.");
            app_prompt_inform_completed_event();
        }
        return true;
    }else {
        TRACE(0, "Prompt list is empty");
    }

    return false;
}

void app_prompt_inform_completed_event(void)
{
    osSignalSet(app_prompt_handler_tid, PROMPT_HANDLER_SIGNAL_PLAYING_COMPLETED);
}

void app_prompt_stop_all(void)
{
    osSignalSet(app_prompt_handler_tid, PROMPT_HANDLER_SIGNAL_CLEAR_REQ);
}

// assure that all pending prompts especailly power-off can be played
// at the time of power-off, after that, disconnect all of the links,
// to avoid the possible issue that only one-earbud plays power-off prompt
#define APP_PROMPT_FLUSH_TIMEOUT_MS APP_PROMPT_PROTECTOR_EXPIRE_TIMEOUT_MS
#define APP_PROMPT_FLUSH_CHECK_INTERVAL_MS  100
#define APP_PROMPT_FLUSH_CHECK_COUNT        ((APP_PROMPT_FLUSH_TIMEOUT_MS)/(APP_PROMPT_FLUSH_CHECK_INTERVAL_MS))
void app_prompt_flush_pending_prompts(void)
{
    uint32_t checkCounter = 0;
    while (checkCounter++ < APP_PROMPT_FLUSH_CHECK_COUNT)
    {
        if (!app_prompt_is_on_going)
        {
            app_prompt_stop_all();
            break;
        }
    }
}

static void app_prompt_clear_prompt_list(void)
{
    // clear the request list
    app_prompt_list_clear(app_prompt_list);

    // stop the on-going prompt
    
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_stop_playing();
#endif

    for (uint8_t devId = 0;devId < BT_DEVICE_NUM;devId++)
    {    
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, 
            (uint8_t)APP_BT_SETTING_CLOSE, devId);
    }
}

#if defined(__XSPACE_UI__)
AUD_ID_ENUM app_prompt_checker(void)
{
    return app_prompt_on_going_prompt_id2;

}
#endif

static void app_prompt_handler_thread(void const *argument)
{
    TRACE(0, "enter prompt handler thread.");
    while(1) 
    {    
        osEvent evt;
        // wait any signal
        evt = osSignalWait(0x0, osWaitForever);

        // get role from signal value
        if (evt.status == osEventSignal)
        {
            TRACE(0, "prompt handler thread gets signal 0x%x", evt.value.signals);
            
            if (evt.value.signals & PROMPT_HANDLER_SIGNAL_NEW_PROMPT_REQ)
            {
                app_prompt_refresh_list();
            }
            
            if (evt.value.signals & PROMPT_HANDLER_SIGNAL_CLEAR_REQ)
            {
                app_prompt_clear_prompt_list();
            }
            
            if (evt.value.signals & PROMPT_HANDLER_SIGNAL_PLAYING_COMPLETED)
            {
                app_prompt_is_on_going = false;
                app_prompt_on_going_prompt_id = AUD_ID_INVALID;
                app_prompt_stop_protector();
                app_prompt_refresh_list();
            }
        }
    }
}

#if 1
static char need_init_decoder_press = 1;
static uint8_t app_play_sbc_stop_proc_cnt_press = 0;
static CQueue media_sbc_queue_press;
static btif_sbc_decoder_t *media_sbc_decoder_press = NULL;
static uint32_t g_app_audio_read_press_dwon_up = 0;
static U8* g_app_audio_data_press_dwon_up = NULL;
static uint32_t g_app_audio_length_press_dwon_up = 0;
static float * media_press_sbc_eq_band_gain = NULL;
#define APP_MEDIA_BUFFER_SIZE_PRESS (8*1024)
static uint8_t media_buffer_press[APP_MEDIA_BUFFER_SIZE_PRESS];
static uint32_t media_buff_size_used_press;
uint32_t g_cache_buff_sz_press = 0;
static int16_t *app_play_sbc_cache_press = NULL;
int app_media_press_mempool_init(void)
{
    TRACE(1, "app_media_press_mempool_init %d.", APP_MEDIA_BUFFER_SIZE_PRESS);
    media_buff_size_used_press = 0;
    memset((uint8_t *)media_buffer_press, 0, APP_MEDIA_BUFFER_SIZE_PRESS);

    return 0;
}
uint32_t app_media_press_mempool_free_buff_size()
{
    return APP_MEDIA_BUFFER_SIZE_PRESS - media_buff_size_used_press;
}

int app_media_press_mempool_get_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;
    uint8_t* media_buf_addr = (uint8_t *)media_buffer_press;
    buff_size_free = app_media_press_mempool_free_buff_size();

    if (size % 4){
        size = size + (4 - size % 4);
    }

    TRACE(2,"Get media buf, current free %d to allocate %d", buff_size_free, size);

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    *buff = media_buf_addr + media_buff_size_used_press;

    media_buff_size_used_press += size;
    TRACE(3,"Allocate %d, now used %d left %d",
        size, media_buff_size_used_press, app_media_press_mempool_free_buff_size());

    return 0;
}
void media_runtime_audio_prompt_update_press_dwon_up(uint16_t id){
        if( id == AUD_ID_BT_PRESSURE_DOWN){
#if defined __GESTURE_MANAGER_USE_PRESSURE__
            g_app_audio_data_press_dwon_up = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PROSSURE_DOWN: (U8*)EN_BT_PROSSURE_DOWN;
            g_app_audio_length_press_dwon_up = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PROSSURE_DOWN): sizeof(EN_BT_PROSSURE_DOWN);
#endif
        }
        else if(id ==  AUD_ID_BT_PRESSURE_UP){
#if defined __GESTURE_MANAGER_USE_PRESSURE__
            g_app_audio_data_press_dwon_up = (g_language==MEDIA_DEFAULT_LANGUAGE)?(U8*)EN_BT_PROSSURE_UP: (U8*)EN_BT_PROSSURE_UP;
            g_app_audio_length_press_dwon_up = (g_language==MEDIA_DEFAULT_LANGUAGE)?sizeof(EN_BT_PROSSURE_UP): sizeof(EN_BT_PROSSURE_UP);
#endif
        }else{
		g_app_audio_data_press_dwon_up = NULL;
		g_app_audio_length_press_dwon_up = 0;
        }

}

void media_Play_init_audio_press_dwon_up(uint16_t aud_id){
        g_app_audio_read_press_dwon_up = 0;
        media_runtime_audio_prompt_update_press_dwon_up(aud_id);
}

int media_audio_press_init(void)
{
    uint8_t *buff = NULL;
    uint8_t i;

    app_media_press_mempool_get_buff((uint8_t **)&media_press_sbc_eq_band_gain, CFG_HW_AUD_EQ_NUM_BANDS*sizeof(float));

    for (i=0; i<CFG_HW_AUD_EQ_NUM_BANDS; i++){
        media_press_sbc_eq_band_gain[i] = 1.0;
    }

    app_media_press_mempool_get_buff(&buff, SBC_QUEUE_SIZE);
    memset(buff, 0, SBC_QUEUE_SIZE);

    LOCK_APP_AUDIO_QUEUE();
    APP_AUDIO_InitCQueue(&media_sbc_queue_press, SBC_QUEUE_SIZE, buff);
    UNLOCK_APP_AUDIO_QUEUE();

    app_media_press_mempool_get_buff((uint8_t **)&media_sbc_decoder_press, sizeof(btif_sbc_decoder_t) + 4);

    need_init_decoder_press = 1;

    app_play_sbc_stop_proc_cnt_press = 0;

    return 0;
}
int decode_sbc_frame_press(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    uint8_t underflow = 0;

    int r = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    static btif_sbc_pcm_data_t pcm_data;
    bt_status_t ret = BT_STS_SUCCESS;
    unsigned short byte_decode = 0;

    pcm_data.data = (unsigned char*)pcm_buffer;

    LOCK_APP_AUDIO_QUEUE();
again:
    if(need_init_decoder_press) {
        pcm_data.data = (unsigned char*)pcm_buffer;
        pcm_data.dataLen = 0;
        btif_sbc_init_decoder(media_sbc_decoder_press);
    }

get_again:
    len1 = len2 = 0;
    r = APP_AUDIO_PeekCQueue(&media_sbc_queue_press, SBC_TEMP_BUFFER_SIZE, &e1, &len1, &e2, &len2);

    if(r == CQ_ERR) {
            need_init_decoder = 1;
            underflow = 1;
            r = pcm_data.dataLen;
            TRACE(0,"cache underflow");
            goto exit;
    }
    if (!len1){
        TRACE(2,"len1 underflow %d/%d\n", len1, len2);
        goto get_again;
    }

    ret = btif_sbc_decode_frames(media_sbc_decoder_press, (unsigned char *)e1,
                                len1, &byte_decode,
                                &pcm_data, pcm_len,
                                media_press_sbc_eq_band_gain);

    if(ret == BT_STS_CONTINUE) {
        need_init_decoder_press = 0;
        APP_AUDIO_DeCQueue(&media_sbc_queue_press, 0, len1);
        goto again;

        /* back again */
    }
    else if(ret == BT_STS_SUCCESS) {
        need_init_decoder_press = 0;
        r = pcm_data.dataLen;
        pcm_data.dataLen = 0;

        APP_AUDIO_DeCQueue(&media_sbc_queue_press, 0, byte_decode);

        //TRACE(1,"p %d\n", pcm_data.sampleFreq);

        /* leave */
    }
    else if(ret == BT_STS_FAILED) {
        need_init_decoder_press = 1;
        r = pcm_data.dataLen;
        TRACE(0,"err\n");

        APP_AUDIO_DeCQueue(&media_sbc_queue_press, 0, byte_decode);

        /* leave */
    }
    else if(ret == BT_STS_NO_RESOURCES) {
        need_init_decoder_press = 0;

        TRACE(0,"no\n");

        /* leav */
        r = 0;
    }

exit:
    if (underflow){
        TRACE(1,"decode_sbc_frame_press underflow len:%d\n ", pcm_len);
    }
    UNLOCK_APP_AUDIO_QUEUE();
    return r;
}
int app_media_store_sbc_buffer_press(unsigned char *buf, unsigned int len)
{
    int nRet;
    LOCK_APP_AUDIO_QUEUE();
    nRet = APP_AUDIO_EnCQueue(&media_sbc_queue_press, buf, len);
    UNLOCK_APP_AUDIO_QUEUE();

    return nRet;
}
uint32_t app_play_single_sbc_more_data_press(uint8_t *buf, uint32_t len)
{

    uint32_t l = 0;

     //TRACE(2,"app_play_sbc_more_data : %d, %d", g_app_audio_read, g_app_audio_length);

    if (g_app_audio_read_press_dwon_up < g_app_audio_length_press_dwon_up){
        unsigned int available_len = 0;
        unsigned int store_len = 0;

        available_len = AvailableOfCQueue(&media_sbc_queue_press);
        store_len = (g_app_audio_length_press_dwon_up - g_app_audio_read_press_dwon_up) > available_len ? available_len :(g_app_audio_length_press_dwon_up - g_app_audio_read_press_dwon_up);
        app_media_store_sbc_buffer_press((unsigned char *)(g_app_audio_data_press_dwon_up + g_app_audio_read_press_dwon_up), store_len);
        g_app_audio_read_press_dwon_up+= store_len;
    }


        l = decode_sbc_frame_press(buf, len);

        if (l != len)
        {
            g_app_audio_read_press_dwon_up = g_app_audio_length_press_dwon_up;
            //af_stream_stop(AUD_STREAM_PLAYBACK);
            //af_stream_close(AUD_STREAM_PLAYBACK);
            TRACE(4,"[%s]-->need close, length:%d len:%d l:%d", __func__,g_app_audio_length, len, l);
        }

    return l;
}
uint32_t app_play_sbc_channel_select_press(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{	
	for (int i = (int)(src_len - 1); i >= 0; i--)
        {
            dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
        }


    return 0;
}
uint32_t app_play_sbc_more_data_press(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    memset(buf, 0, len);


    if(app_play_sbc_cache_press)
        memset(app_play_sbc_cache_press, 0, g_cache_buff_sz_press);

    if (app_play_sbc_stop_proc_cnt_press) {
        if (app_play_sbc_stop_proc_cnt_press == 1) {
            app_play_sbc_stop_proc_cnt_press = 2;
        } else if (app_play_sbc_stop_proc_cnt_press == 2) {
            app_play_sbc_stop_proc_cnt_press = 3;
            app_play_audio_press_dwon_up_onoff(false,0);
        }
    } else {
        if (app_play_sbc_cache_press) {

            l = app_play_single_sbc_more_data_press( (uint8_t *)app_play_sbc_cache_press, len/2);

            if (l != len/2 ) {
                memset(app_play_sbc_cache_press+l, 0, len/2-l);
                app_play_sbc_stop_proc_cnt_press = 1;
            }
           app_play_sbc_channel_select_press((int16_t *)buf, app_play_sbc_cache_press, len/2/2);

        } else {
            memset(buf, 0, len);
        }
    }

    return l;
}
int app_play_audio_press_dwon_up_onoff(bool onoff, uint16_t aud_id)
{
    static bool isRun =  false;
    struct AF_STREAM_CONFIG_T stream_cfg;
    uint8_t* bt_audio_buff = NULL;
    enum AUD_SAMPRATE_T sample_rate POSSIBLY_UNUSED = AUD_SAMPRATE_16000;
    if (isRun == onoff)
    {
        return 0;
    }

    app_sysfreq_req(APP_SYSFREQ_USER_PRESS, APP_SYSFREQ_104M);

    if (onoff ) {
        media_Play_init_audio_press_dwon_up(aud_id);
        if (!g_app_audio_length_press_dwon_up){
            app_sysfreq_req(APP_SYSFREQ_USER_PRESS, APP_SYSFREQ_32K);
		isRun = false;
            return 0;
        }
        af_set_priority(AF_USER_PRESS, osPriorityHigh);
        app_media_press_mempool_init();
        media_audio_press_init();
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate =  AUD_SAMPRATE_16000;
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = MEDIA_VOLUME_LEVEL_WARNINGTONE;     
        stream_cfg.handler = app_play_sbc_more_data_press;
        stream_cfg.data_size = APP_AUDIO_PLAYBACK_BUFF_SIZE;
        g_cache_buff_sz_press = stream_cfg.data_size/2/2 ;
        app_media_press_mempool_get_buff((uint8_t **)&app_play_sbc_cache_press, g_cache_buff_sz_press);
        app_media_press_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        TRACE(0, "[%s]: sample rate: %d, bits: %d, channel number: %d, data size:%d",
            __func__,
            stream_cfg.sample_rate,
            stream_cfg.bits,
            stream_cfg.channel_num,
            stream_cfg.data_size);

        af_stream_open(AUD_STREAM_ID_3, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_3, AUD_STREAM_PLAYBACK);

    }
    else{
	af_stream_stop(AUD_STREAM_ID_3, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_3, AUD_STREAM_PLAYBACK);
        app_play_sbc_cache_press = NULL;
        g_cache_buff_sz_press = 0;
        af_set_priority(AF_USER_PRESS, osPriorityAboveNormal);
	app_sysfreq_req(APP_SYSFREQ_USER_PRESS, APP_SYSFREQ_32K);

    }
    isRun = onoff;

    return 0;
}
#endif
#endif