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
#include <assert.h>

#include "cmsis_os.h"
#include "cmsis.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_chipid.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "nvrecord.h"
#if defined(IBRT)
#include "app_ibrt_if.h"
#else
#include "app_media_player.h"
#endif

#include "resources.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#endif

#include "bt_drv.h"
#include "apps.h"
#include "app_bt_func.h"
#include "app_bt.h"

#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_hfp.h"

#include "app_bt_media_manager.h"
#include "app_thread.h"

#include "app_ring_merge.h"
#include "bt_if.h"
#include "audio_prompt_sbc.h"
#ifdef __AI_VOICE__
#include "app_ai_if.h"
#include "app_ai_voice.h"
#endif
#include "app_bt_stream.h"

int bt_sco_player_forcemute(bool mic_mute, bool spk_mute);
extern enum AUD_SAMPRATE_T a2dp_sample_rate;

struct bt_media_manager {
    uint8_t media_curr_sbc;
    uint8_t media_curr_sco;
    uint16_t curr_active_media; // low 8 bits are out direciton, while high 8 bits are in direction
};

static char _strm_type_str[168];
static char *_catstr(char *dst, const char *src) {
     while(*dst) dst++;
     while((*dst++ = *src++));
     return --dst;
}
const char *strmtype2str(uint16_t stream_type) {
    const char *s = NULL;
    char _cat = 0, first = 1, *d = NULL;
    _strm_type_str[0] = '\0';
    d = _strm_type_str;
    d = _catstr(d, "[");
    if (stream_type != 0) {
        for (int i = 15 ; i >= 0; i--) {
            _cat = 1;
            //TRACE(3,"i=%d,stream_type=0x%d,stream_type&(1<<i)=0x%x", i, stream_type, stream_type&(1<<i));
            switch(stream_type&(1<<i)) {
                case 0: _cat = 0; break;
                case BT_STREAM_SBC: s = "SBC"; break;
                case BT_STREAM_MEDIA: s = "MEDIA"; break;
                case BT_STREAM_VOICE: s = "VOICE"; break;
                #ifdef RB_CODEC
                case BT_STREAM_RBCODEC: s = "RB_CODEC"; break;
                #endif
                // direction is in
                #ifdef __AI_VOICE__
                case BT_STREAM_AI_VOICE: s = "AI_VOICE"; break;
                #endif
                #ifdef AUDIO_LINEIN
                case BT_STREAM_LINEIN: s  = "LINEIN"; break;
                #endif
                #ifdef AUDIO_PCM_PLAYER
                case BT_STREAM_PCM_PLAYER: s  = "PCM_PLAYER"; break;
                #endif
                default:  s = "UNKNOWN"; break;
            }
            if (_cat) {
                if (!first)
                    d = _catstr(d, "|");
                //TRACE(2,"d=%s,s=%s", d, s);
                d = _catstr(d, s);
                first = 0;
            }
        }
    }

    _catstr(d, "]");

    return _strm_type_str;
}

static const char *handleId2str(uint8_t id) {
    #define CASE_M(s) \
        case s: return "["#s"]";

    switch(id) {
    CASE_M(APP_BT_STREAM_MANAGER_START)
    CASE_M(APP_BT_STREAM_MANAGER_STOP)
    CASE_M(APP_BT_STREAM_MANAGER_STOP_MEDIA)
    CASE_M(APP_BT_STREAM_MANAGER_CLEAR_MEDIA)
    CASE_M(APP_BT_STREAM_MANAGER_UPDATE_MEDIA)
    CASE_M(APP_BT_STREAM_MANAGER_SWAP_SCO)
    CASE_M(APP_BT_STREAM_MANAGER_CTRL_VOLUME)
    CASE_M(APP_BT_STREAM_MANAGER_TUNE_SAMPLERATE_RATIO)
    }

    return "[]";
}

extern enum AUD_SAMPRATE_T sco_sample_rate;
static struct bt_media_manager bt_meida;
static uint8_t a2dp_codec_type[BT_DEVICE_NUM] = {0,};

int app_audio_manager_set_a2dp_codec_type(uint8_t device_id, uint8_t codec_type)
{
    if(device_id < BT_DEVICE_NUM) {
        a2dp_codec_type[device_id] = codec_type;
        return 0;
    } else {
        TRACE(3, "[%s] device_id:%d is out of range:%d", __func__, device_id, BT_DEVICE_NUM);
        return -1;
    }
}

uint8_t app_audio_manager_get_a2dp_codec_type(uint8_t device_id)
{
    if(device_id < BT_DEVICE_NUM) {
        return a2dp_codec_type[device_id];
    } else {
        TRACE(3, "[%s] device_id:%d is out of range:%d,return default value", __func__, device_id, BT_DEVICE_NUM);
        return BTIF_AVDTP_CODEC_TYPE_SBC;
    }
}

uint16_t bt_media_get_media_active(enum BT_DEVICE_ID_T device_id)
{
    return app_bt_get_device(device_id)->media_active;
}

uint8_t bt_media_is_media_active_by_type(uint16_t media_type)
{
    uint8_t i;
    for(i = 0; i < BT_DEVICE_NUM; i++)
    {
        if(app_bt_get_device(i)->media_active & media_type)
            return 1;
    }
    for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i += 1)
    {
        if(app_bt_get_device(i)->media_active & media_type)
            return 1;
    }
    return 0;
}

bool bt_media_is_media_idle(void)
{
    uint8_t i;
    for(i = 0; i < BT_DEVICE_NUM; i++)
    {
        if(app_bt_get_device(i)->media_active != 0)
            return false;
    }
    for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i += 1)
    {
        if(app_bt_get_device(i)->media_active != 0)
            return false;
    }
    return true;
}
void bt_media_clean_up(void)
{
    uint8_t i = 0;
    bt_meida.curr_active_media = 0;
    for (i = 0; i < BT_DEVICE_NUM; i++)
    {
        app_bt_get_device(i)->media_active = 0;
    }
    for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i += 1)
    {
        app_bt_get_device(i)->media_active = 0;
    }
}

bool bt_media_is_media_active_by_sbc(void)
{
    uint8_t i;
    for(i = 0; i < BT_DEVICE_NUM; i++) // only a2dp sink has sbc speaker out media
    {
        if(app_bt_get_device(i)->media_active & BT_STREAM_SBC)
            return 1;
    }
    return 0;
}
bool bt_is_sco_media_open(void)
{
    return (bt_meida.curr_active_media == BT_STREAM_VOICE)?(true):(false);
}

enum BT_DEVICE_ID_T bt_media_get_active_device_by_type(uint16_t media_type)
{
    uint8_t i;
    for(i = 0; i < BT_DEVICE_NUM; i++)
    {
        if(app_bt_get_device(i)->media_active & media_type)
            return (enum BT_DEVICE_ID_T)i;
    }
    for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i += 1)
    {
        if(app_bt_get_device(i)->media_active & media_type)
            return (enum BT_DEVICE_ID_T)i;
    }
    return (enum BT_DEVICE_ID_T)BT_DEVICE_INVALID_ID;
}

uint8_t bt_media_is_media_active_by_device(uint16_t media_type,enum BT_DEVICE_ID_T device_id)
{
    if(app_bt_get_device(device_id)->media_active & media_type)
        return 1;
    return 0;
}

uint16_t bt_media_get_current_media(void)
{
    return bt_meida.curr_active_media;
}

bool bt_media_cur_is_bt_stream_sbc(void)
{
    return (BT_STREAM_SBC & bt_meida.curr_active_media)?(true):(false);
}

bool bt_media_cur_is_bt_stream_media(void)
{
    return (BT_STREAM_MEDIA & bt_meida.curr_active_media)?(true):(false);
}

bool bt_media_is_sbc_media_active(void)
{
    return (bt_media_is_media_active_by_type(BT_STREAM_SBC) == 1)?(true):(false);
}

void bt_media_current_sbc_set(uint8_t id)
{
    TRACE(0, "current sbc %d->%d", bt_meida.media_curr_sbc, id);
    bt_meida.media_curr_sbc = id;
}

uint8_t bt_media_current_sbc_get(void)
{
    return bt_meida.media_curr_sbc;
}

uint8_t bt_media_current_sco_get(void)
{
    return bt_meida.media_curr_sco;
}

void bt_media_current_sco_set(uint8_t id)
{
    TRACE(0, "current sco %d->%d", bt_meida.media_curr_sco, id);
    bt_meida.media_curr_sco = id;
}

void bt_media_set_current_media(uint16_t media_type)
{
    // out direction
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
    // BT_STREAM_MEDIA can run along with other stream
    if (media_type == BT_STREAM_MEDIA)
    {
        bt_meida.curr_active_media |= BT_STREAM_MEDIA;
    }
    else
#endif
    if (media_type < 0x100)
    {
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        uint16_t media_stream_status = BT_STREAM_MEDIA & bt_meida.curr_active_media;
#endif
        bt_meida.curr_active_media &= (~0xFF);
        bt_meida.curr_active_media |= media_type;
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        bt_meida.curr_active_media |= media_stream_status;
#endif
    }
    else
    {
        //bt_meida.curr_active_media &= (~0xFF00);
        bt_meida.curr_active_media |= media_type;
    }

    TRACE(2,"curr_active_media is set to 0x%x%s", bt_meida.curr_active_media,
            bt_meida.curr_active_media ? strmtype2str(bt_meida.curr_active_media) : "[N/A]");
}

void bt_media_clear_current_media(uint16_t media_type)
{
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
    // BT_STREAM_MEDIA can run along with other stream
    if (media_type == BT_STREAM_MEDIA)
    {
        bt_meida.curr_active_media &= (~BT_STREAM_MEDIA);
    }
    else
#endif
    if (media_type < 0x100)
    {
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        uint16_t media_stream_status = BT_STREAM_MEDIA & bt_meida.curr_active_media;
#endif
        bt_meida.curr_active_media &= (~0xFF);
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        bt_meida.curr_active_media |= media_stream_status;
#endif
    }
    else
    {
        bt_meida.curr_active_media &= (~media_type);
    }
    TRACE(2, "clear media 0x%x curr media 0x%x", media_type, bt_meida.curr_active_media);
}

static uint8_t bt_media_set_media_type(uint16_t media_type,enum BT_DEVICE_ID_T device_id)
{
    if (device_id < BT_DEVICE_NUM || (device_id >= BT_SOURCE_DEVICE_ID_BASE && device_id < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM))
    {
        TRACE(2, "Add active stream %d for device %d", media_type, device_id);
        app_bt_get_device(device_id)->media_active |= media_type;
    }
    else
    {
        TRACE(2,"%s invalid devcie_id:%d",__func__,device_id );
    }
    return 0;
}

void bt_media_clear_media_type(uint16_t media_type,enum BT_DEVICE_ID_T device_id)
{
    uint8_t i;
    TRACE(4,"%s 0x%x active_now 0x%x id %d", __func__, media_type, app_bt_get_device(device_id)->media_active, device_id);
    if (media_type == BT_STREAM_MEDIA)
    {
        for(i = 0; i < BT_DEVICE_NUM; i++)
        {
            app_bt_get_device(i)->media_active &= (~media_type);
        }
        for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i += 1)
        {
            app_bt_get_device(i)->media_active &= (~media_type);
        }
    }
    else
    {
        app_bt_get_device(device_id)->media_active &= (~media_type);
    }
}

static enum BT_DEVICE_ID_T bt_media_get_active_sbc_device(void)
{
    uint8_t i;
    for(i = 0; i < BT_DEVICE_NUM; i++) // only a2dp sink has sbc speaker out media
    {
        if((app_bt_get_device(i)->media_active & BT_STREAM_SBC) && (i == bt_meida.media_curr_sbc))
        {
            return (enum BT_DEVICE_ID_T)i;
        }
    }
    return BT_DEVICE_INVALID_ID;
}


#ifdef RB_CODEC
bool  bt_media_rbcodec_start_process(uint16_t stream_type,enum BT_DEVICE_ID_T device_id,AUD_ID_ENUM media_id, uint32_t param, uint32_t ptr)
{
    TRACE(2, "Add active stream %d for device %d", stream_type, device_id);
    int ret_SendReq2AudioThread = -1;
    app_bt_get_device(device_id)->media_active |= stream_type;

    ret_SendReq2AudioThread = app_audio_sendrequest(APP_BT_STREAM_RBCODEC, (uint8_t)APP_BT_SETTING_OPEN, media_id);
    bt_media_set_current_media(BT_STREAM_RBCODEC);
    return true;
exit:
    return false;
}
#endif

#ifdef __AI_VOICE__
static bool bt_media_ai_voice_start_process(uint16_t stream_type,
                                             enum BT_DEVICE_ID_T device_id,
                                             AUD_ID_ENUM media_id,
                                             uint32_t param,
                                             uint32_t ptr)
{
    TRACE(2, "Add active stream %d for device %d", stream_type, device_id);
    app_bt_get_device(device_id)->media_active |= stream_type;
    app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, ( uint8_t )APP_BT_SETTING_OPEN, media_id);

    bt_media_set_current_media(BT_STREAM_AI_VOICE);
    return true;
}
#endif

void app_stop_a2dp_media_stream(uint8_t devId)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,
            BT_STREAM_SBC, devId,0);
}

void app_stop_sco_media_stream(uint8_t devId)
{
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,
            BT_STREAM_VOICE, devId,0);
}

extern char *app_bt_get_global_state_buffer(void);

const char* app_bt_get_active_media_state(void)
{
    int len = 0;
    char *buffer = app_bt_get_global_state_buffer();

    len = sprintf(buffer, "curr_active_media %x%s media_active ", bt_meida.curr_active_media,
                bt_meida.curr_active_media ? strmtype2str(bt_meida.curr_active_media) : "[N/A]");

    for (int i = 0; i < BT_DEVICE_NUM; i += 1)
    {
        len += sprintf(buffer+len, "(d%x %x%s) ", i, app_bt_get_device(i)->media_active,
                    app_bt_get_device(i)->media_active ? strmtype2str(app_bt_get_device(i)->media_active) : "[N/A]");
    }

    return buffer;
}

void app_bt_audio_state_checker(void)
{
    bt_bdaddr_t curr_sco_bdaddr = {0};
    bt_bdaddr_t curr_sbc_bdaddr = {0};

    if (bt_meida.media_curr_sco != BT_DEVICE_INVALID_ID)
    {
        curr_sco_bdaddr = app_bt_get_device(bt_meida.media_curr_sco)->remote;
    }

    if (bt_meida.media_curr_sbc != BT_DEVICE_INVALID_ID)
    {
        curr_sbc_bdaddr = app_bt_get_device(bt_meida.media_curr_sbc)->remote;
    }

    TRACE(14,"audio_state: media_curr_sco %x %02x:%02x:%02x:%02x:%02x:%02x media_curr_sbc %x %02x:%02x:%02x:%02x:%02x:%02x",
            bt_meida.media_curr_sco == BT_DEVICE_INVALID_ID ? 0xff : bt_meida.media_curr_sco,
            curr_sco_bdaddr.address[0], curr_sco_bdaddr.address[1], curr_sco_bdaddr.address[2],
            curr_sco_bdaddr.address[3], curr_sco_bdaddr.address[4], curr_sco_bdaddr.address[5],
            bt_meida.media_curr_sbc == BT_DEVICE_INVALID_ID ? 0xff : bt_meida.media_curr_sbc,
            curr_sbc_bdaddr.address[0], curr_sbc_bdaddr.address[1], curr_sbc_bdaddr.address[2],
            curr_sbc_bdaddr.address[3], curr_sbc_bdaddr.address[4], curr_sbc_bdaddr.address[5]);

    TRACE(1, "audio_state: %s", app_bt_get_active_media_state());
}

//only used in iamain thread ,can't used in other thread or interrupt
void  bt_media_start(uint16_t stream_type, enum BT_DEVICE_ID_T device_id, uint16_t media_id)
{
#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    bool isMergingPrompt = true;
#endif
#endif

    bt_media_set_media_type(stream_type, device_id);

    TRACE(4,"STREAM MANAGE bt_media_start stream 0x%x%s device 0x%x media_id 0x%x",
          stream_type,
          stream_type ? strmtype2str(stream_type) : "[N/A]",
          device_id,
          media_id);

    TRACE(1, "bt_media_start %s\n", app_bt_get_active_media_state());

    switch(stream_type)
    {
#ifdef RB_CODEC
        case BT_STREAM_RBCODEC:
            if(!bt_media_rbcodec_start_process(stream_type,device_id, (AUD_ID_ENUM)media_id, NULL, NULL))
                goto exit;
            break;
#endif

#ifdef __AI_VOICE__
        case BT_STREAM_AI_VOICE:
            if (bt_media_get_current_media() & BT_STREAM_AI_VOICE)
            {
                TRACE(0,"there is a ai voice stream exist ,do nothing");
                return;
            }

            if (bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                TRACE(0,"there is a SCO stream exist ,do nothing");
                goto exit;
            }

            if (!bt_media_ai_voice_start_process(BT_STREAM_AI_VOICE, device_id, ( AUD_ID_ENUM )media_id, ( uint32_t )NULL, ( uint32_t )NULL))
                goto exit;
            break;
#endif

        case BT_STREAM_SBC://音乐流
        {
            uint8_t media_pre_sbc = bt_meida.media_curr_sbc;

            /// because voice is the highest priority and media report will stop soon
            /// so just store the sbc type
            if (bt_meida.media_curr_sbc == BT_DEVICE_INVALID_ID)
                bt_meida.media_curr_sbc = device_id;

            TRACE(2," pre/cur_sbc = %d/%d", media_pre_sbc, bt_meida.media_curr_sbc);

#ifdef MEDIA_PLAYER_SUPPORT
#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
            /// clear the pending stop flag if it is set
            audio_prompt_clear_pending_stream(PENDING_TO_STOP_A2DP_STREAMING);
#endif

            if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
#ifdef __AI_VOICE__
                if (app_ai_voice_is_need2block_a2dp())
                {
                    TRACE(0,"there is a ai voice stream exist");
                    app_ai_voice_block_a2dp();
                }
#endif
                goto exit;
            }
#endif // #ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
#endif // #ifdef MEDIA_PLAYER_SUPPORT

#ifdef RB_CODEC
            if (bt_media_is_media_active_by_type(BT_STREAM_RBCODEC))
            {
                goto exit;
            }
#endif

#ifdef __AI_VOICE__
            if (app_ai_voice_is_need2block_a2dp())
            {
                TRACE(0,"there is a ai voice stream exist");
                app_ai_voice_block_a2dp();
                goto exit;
            }
#endif

#ifdef AUDIO_LINEIN
            if(bt_media_is_media_active_by_type(BT_STREAM_LINEIN))
            {
                if(bt_media_get_current_media() & BT_STREAM_LINEIN)
                {
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, (uint8_t)BT_STREAM_LINEIN, 0,0);
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_SBC, device_id, 0);
                    return;
                }
            }
#endif

#ifdef AUDIO_PCM_PLAYER
            if(bt_media_is_media_active_by_type(BT_STREAM_PCM_PLAYER))
            {
                if(bt_media_get_current_media() & BT_STREAM_PCM_PLAYER)
                {
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, (uint8_t)BT_STREAM_PCM_PLAYER, 0,0);
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_SBC, device_id, 0);
                    return;
                }
            }
#endif

            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                /// sbc and voice is all on so set sys freq to 104m
                app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
                return;
            }

            if (!bt_media_cur_is_bt_stream_sbc())
            {
                app_audio_manager_switch_a2dp((enum BT_DEVICE_ID_T)device_id);

                bt_media_set_current_media(BT_STREAM_SBC);

                app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                      (uint8_t)(APP_BT_SETTING_SETUP),
                                      (uint32_t)(app_bt_get_device(device_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));

                if (app_bt_audio_count_streaming_a2dp() >= 2)
                {
                    app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC, ( uint8_t )APP_BT_SETTING_OPEN, 0, APP_SYSFREQ_104M);
                }
                else
                {
                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, ( uint8_t )APP_BT_SETTING_OPEN, 0);
                }
            }
        }
        break;

#ifdef MEDIA_PLAYER_SUPPORT
        case BT_STREAM_MEDIA://提示音流
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
            isMergingPrompt = IS_PROMPT_NEED_MERGING(media_id);
#endif

            if (
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                audio_prompt_is_playing_ongoing() ||
#endif
                app_audio_list_playback_exist())
            {
                if (!bt_media_cur_is_bt_stream_media())
                {
                    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                }

                APP_AUDIO_STATUS aud_status = {0};
                aud_status.id = APP_PLAY_BACK_AUDIO;
                aud_status.aud_id = media_id;
                aud_status.device_id = device_id;
                app_audio_list_append(&aud_status);
                break;
            }

#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
#ifdef AUDIO_LINEIN
            if(bt_media_is_media_active_by_type(BT_STREAM_LINEIN))
            {
                if(bt_media_get_current_media() & BT_STREAM_LINEIN)
                {
                    APP_AUDIO_STATUS aud_status = {0};
                    aud_status.id = media_id;
                    app_play_audio_lineinmode_start(&aud_status);
                    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                }
            }else
#endif
#ifdef AUDIO_PCM_PLAYER
            if(bt_media_is_media_active_by_type(BT_STREAM_PCM_PLAYER))
            {
                if(bt_media_get_current_media() & BT_STREAM_PCM_PLAYER)
                {
                    APP_AUDIO_STATUS aud_status;
                    aud_status.id = media_id;
                    app_play_audio_lineinmode_start(&aud_status);
                    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                }
            }else
#endif
            //first,if the voice is active so  mix "dudu" to the stream
            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                if(bt_media_get_current_media() & BT_STREAM_VOICE)
                {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                    // if the playback is not triggered yet, we just use the stand-alone prompt playing
                    if (!bt_is_playback_triggered())
                    {
                        isMergingPrompt = false;
                    }
#endif
                    //if call is not active so do media report
                    if((btapp_hfp_is_call_active() && !btapp_hfp_incoming_calls()) ||
                       (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)))
                    {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                        if (isMergingPrompt)
                        {
                            audio_prompt_start_playing(media_id, sco_sample_rate);
                            goto exit;
                        }
                        else
                        {
                            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                            bt_media_set_current_media(BT_STREAM_MEDIA);
                        }
#else
                        //in three way call merge "dudu"
                        TRACE(0,"BT_STREAM_VOICE-->app_ring_merge_start\n");
                        app_ring_merge_start();
                        //meida is done here
                        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
#endif
                    }
                    else
                    {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                        if (isMergingPrompt)
                        {
                            audio_prompt_start_playing(media_id, sco_sample_rate);
                            goto exit;
                        }
                        else
                        {
                            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                            bt_media_set_current_media(BT_STREAM_MEDIA);
                        }
#else
                        TRACE(0,"stop sco and do media report\n");
                        bt_media_set_current_media(BT_STREAM_MEDIA);
                        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
#endif
                    }
                }
                else if(bt_media_get_current_media() & BT_STREAM_MEDIA)
                {
                    bt_media_set_current_media(BT_STREAM_MEDIA);
                    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                }
                else
                {
                    ///if voice is active but current is not voice something is unkown
                    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);

                    TRACE(1,"STREAM MANAGE %s", app_bt_get_active_media_state());
                }
            }
            else if (btapp_hfp_is_call_active())
            {
                bt_media_set_current_media(BT_STREAM_MEDIA);
                app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
            }
            /// if sbc active so
            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                if(bt_media_get_current_media() & BT_STREAM_SBC)
                {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                    // if the playback is not triggered yet, we just use the stand-alone prompt playing
                    if (!bt_is_playback_triggered())
                    {
                        isMergingPrompt = false;
                    }

                    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                    if (isMergingPrompt)
                    {
                        audio_prompt_start_playing(media_id,a2dp_sample_rate);
                        goto exit;
                    }
                    else
                    {
                        TRACE(0,"START prompt.");
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        bt_media_clear_current_media(BT_STREAM_SBC);
                        bt_media_set_current_media(BT_STREAM_MEDIA);
                        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                    }
#else
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
                    if (PROMPT_ID_FROM_ID_VALUE(media_id) == AUD_ID_BT_WARNING)
                    {
                        TRACE(0,"BT_STREAM_SBC-->app_ring_merge_start\n");
                        app_ring_merge_start();
                        //meida is done here
                        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                    }
                    else
#endif
                    {
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        bt_media_clear_current_media(BT_STREAM_SBC);
                        bt_media_set_current_media(BT_STREAM_MEDIA);
                        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                    }
#endif // #ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                }
                else if(bt_media_get_current_media() & BT_STREAM_MEDIA)
                {
                    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                }
                else if ((bt_media_get_current_media()&0xFF) == 0)
                {
                    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                }
                else
                {
                    ASSERT(0,"media in sbc  current wrong");
                }
            }
            /// just play the media
            else
#endif // #ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
            {
                bt_media_set_current_media(BT_STREAM_MEDIA);
                app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
            }
            break;
#endif
        case BT_STREAM_VOICE:
        {
            uint8_t curr_playing_sco = app_audio_manager_get_active_sco_num();
            uint8_t curr_request_sco = device_id;

            if (curr_playing_sco == BT_DEVICE_INVALID_ID)
                app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)device_id);

#ifdef __AI_VOICE__
            if(bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE)) {
                if(bt_media_get_current_media() & BT_STREAM_AI_VOICE) {
                    app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    bt_media_clear_current_media(BT_STREAM_AI_VOICE);
                }
            }
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
            /// clear the pending stop flag if it is set
            audio_prompt_clear_pending_stream(PENDING_TO_STOP_SCO_STREAMING);
#endif

            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                //if call is active ,so disable media report
                if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
                {
                    if(bt_media_get_current_media() & BT_STREAM_MEDIA)
                    {
                        if (app_play_audio_get_aud_id() == AUD_ID_BT_CALL_INCOMING_NUMBER)
                        {
                            //if meida is open ,close media clear all media type
                            TRACE(0,"call active so start sco and stop media report\n");
#ifdef __AUDIO_QUEUE_SUPPORT__
                            app_audio_list_clear();
#endif
                            app_prompt_stop_all();

                            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                            bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
                            bt_media_set_current_media(BT_STREAM_VOICE);
                            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                        }
                    }
                }
                else
                {
                    ////call is not active so media report continue
                }
                return;
            }
#endif // #ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
#endif // #ifdef MEDIA_PLAYER_SUPPORT

            if (curr_playing_sco == BT_DEVICE_INVALID_ID)
            {
                app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)curr_request_sco);
                if (bt_media_is_media_active_by_type(BT_STREAM_SBC))
                {
                    ///if sbc is open  stop sbc
                    if(bt_media_get_current_media() & BT_STREAM_SBC)
                    {
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        bt_media_clear_current_media(BT_STREAM_SBC);
                    }

                    ////start voice stream
                    bt_media_set_current_media(BT_STREAM_VOICE);
                    app_audio_sendrequest_param(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0, APP_SYSFREQ_104M);
                }
                else
                {
                    bt_media_set_current_media(BT_STREAM_VOICE);
                    app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                }
            }
            else if (curr_playing_sco != curr_request_sco)
            {
                if (app_bt_manager.config.sco_prompt_play_mode) // prompt mode - play requested one
                {
                    app_audio_manager_swap_sco((enum BT_DEVICE_ID_T)curr_request_sco);
                    app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)curr_request_sco);
                }
                else // non-prompt mode - play current playing one
                {
                    app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)curr_playing_sco);
                    goto start_bt_stream_voice_label;
                }
            }
            else
            {
                app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)curr_request_sco);
start_bt_stream_voice_label:
                if ((bt_media_get_current_media() & BT_STREAM_VOICE) == 0)
                {
                    if (bt_media_is_media_active_by_type(BT_STREAM_SBC))
                    {
                        ///if sbc is open  stop sbc
                        if(bt_media_get_current_media() & BT_STREAM_SBC)
                        {
                            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                            bt_media_clear_current_media(BT_STREAM_SBC);
                        }

                        ////start voice stream
                        bt_media_set_current_media(BT_STREAM_VOICE);
                        app_audio_sendrequest_param(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0, APP_SYSFREQ_104M);
                    }
                    else
                    {
                        bt_media_set_current_media(BT_STREAM_VOICE);
                        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    }
                }
            }
        }
        break;
#ifdef AUDIO_LINEIN
        case BT_STREAM_LINEIN:
            if(!bt_media_is_media_active_by_type(BT_STREAM_SBC | BT_STREAM_MEDIA | BT_STREAM_VOICE))
            {
                app_audio_sendrequest(APP_PLAY_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_LINEIN);
            }
            break;
#endif
#ifdef AUDIO_PCM_PLAYER
        case BT_STREAM_PCM_PLAYER:
            if (!bt_media_is_media_active_by_type(BT_STREAM_SBC | BT_STREAM_MEDIA | BT_STREAM_VOICE)) {
                app_audio_sendrequest(APP_PLAY_PCM_PLAYER, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_PCM_PLAYER);
            }
            break;
#endif

        default:
            ASSERT(0,"bt_media_open ERROR TYPE");
            break;

    }

#if defined(RB_CODEC) || defined(VOICE_DATAPATH) || (defined(MEDIA_PLAYER_SUPPORT) && !defined(AUDIO_PROMPT_USE_DAC2_ENABLED)) || defined(__AI_VOICE__)
exit:
    return;
#endif
}

#ifdef RB_CODEC

static bool bt_media_rbcodec_stop_process(uint16_t stream_type,enum BT_DEVICE_ID_T device_id, uint32_t ptr)
{
    int ret_SendReq2AudioThread = -1;
    bt_media_clear_media_type(stream_type,device_id);
    //if current stream is the stop one ,so stop it
    if(bt_media_get_current_media() & BT_STREAM_RBCODEC ) {
        ret_SendReq2AudioThread = app_audio_sendrequest(APP_BT_STREAM_RBCODEC, (uint8_t)APP_BT_SETTING_CLOSE, ptr);
        bt_media_clear_current_media(BT_STREAM_RBCODEC);
        TRACE(0," RBCODEC STOPED! ");
    }

    if(bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
        enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
        TRACE(1,"sbc_id %d",sbc_id);
        if(sbc_id < BT_DEVICE_NUM) {
            bt_meida.media_curr_sbc = sbc_id;
        }
    } else {
        bt_meida.media_curr_sbc = BT_DEVICE_INVALID_ID;
    }

    TRACE(1,"bt_meida.media_curr_sbc %d",bt_meida.media_curr_sbc);

    if(bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
    } else if(bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
        enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
        if(sbc_id < BT_DEVICE_NUM) {
#ifdef __TWS__
            bt_media_clear_media_type(BT_STREAM_SBC,sbc_id);
            bt_media_clear_current_media(BT_STREAM_SBC);
            notify_tws_player_status(APP_BT_SETTING_OPEN);
#else
            bt_parse_store_sbc_sample_rate(app_bt_get_device(sbc_id)->sample_rate);
            ret_SendReq2AudioThread = app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
            bt_media_set_current_media(BT_STREAM_SBC);
#endif
        }
    } else if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
        //do nothing
    }
}
#endif

#ifdef __AI_VOICE__
bool bt_media_ai_voice_stop_process(uint16_t stream_type, enum BT_DEVICE_ID_T device_id)
{
    bt_media_clear_media_type(BT_STREAM_AI_VOICE, device_id);
    //if current stream is the stop one ,stop it
    if(bt_media_get_current_media() & BT_STREAM_AI_VOICE)
    {
        app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_AI_VOICE);
        TRACE(0," AI VOICE STOPED! ");

#ifdef IBRT
        if(!bt_media_is_media_active_by_type(BT_STREAM_VOICE) &&
           (VOICE_REPORT_MASTER == app_ibrt_voice_report_get_role()))
        {
            TRACE(0," AI VOICE STOPED retrigger ");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_AI_STREAM);
        }
#endif
    }

#ifndef IBRT
    enum BT_DEVICE_ID_T sbc_id = (enum BT_DEVICE_ID_T)BT_DEVICE_INVALID_ID;
    sbc_id = bt_media_get_active_device_by_type(BT_STREAM_SBC);
    TRACE(1,"sbc_id %d",sbc_id);
    bt_meida.media_curr_sbc = sbc_id;

    TRACE(1,"bt_meida.media_curr_sbc %d",bt_meida.media_curr_sbc);

    if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
    {
    }
    else if(sbc_id < BT_DEVICE_NUM)
    {
        if(!(bt_media_get_current_media() & BT_STREAM_SBC))
        {
            //bt_parse_store_sbc_sample_rate(app_bt_get_device(sbc_id)->sample_rate);
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                  (uint8_t)(APP_BT_SETTING_SETUP),
                                  (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
            bt_media_set_current_media(BT_STREAM_SBC);
        }
    }
#endif
    return true;
}
#endif

/*
   bt_media_stop function is called to stop media by app or media play callback
   sbc is just stop by a2dp stream suspend or close
   voice is just stop by hfp audio disconnect
   media is stop by media player finished call back

*/
void bt_media_stop(uint16_t stream_type,enum BT_DEVICE_ID_T device_id,uint16_t media_id)
{
    TRACE(4,"bt_media_stop stream 0x%x%s device 0x%x media_id 0x%x", stream_type, stream_type ? strmtype2str(stream_type) : "[N/A]", device_id, media_id);

    TRACE(1, "bt_media_stop %s\n", app_bt_get_active_media_state());

    if (!bt_media_is_media_active_by_device(stream_type, device_id) &&
        !(bt_media_get_current_media() & stream_type) &&
        stream_type != BT_STREAM_MEDIA)
    {
        TRACE(0, "bt_media_stop skip");
        return;
    }

    switch(stream_type)
    {
    #ifdef __AI_VOICE__
        case BT_STREAM_AI_VOICE:
            bt_media_ai_voice_stop_process(stream_type, device_id);
            break;
    #endif

        case BT_STREAM_SBC:
            {
                APP_AUDIO_STATUS info = {0};
            #ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
                if (!audio_prompt_check_on_stopping_stream(PENDING_TO_STOP_A2DP_STREAMING, device_id))
                {
                    TRACE(0,"Pending stop BT_STREAM_SBC");
                    return;
                }
            #elif defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
            #else
                if (app_ring_merge_isrun())
                {
                    TRACE(0,"bt_media_stop pending BT_STREAM_SBC");
                    app_ring_merge_save_pending_start_stream_op(PENDING_TO_STOP_A2DP_STREAMING, device_id);
                    return;
                }
            #endif

                uint8_t media_pre_sbc = bt_meida.media_curr_sbc;
                TRACE(2,"SBC STOPPING id:%d/%d", bt_meida.media_curr_sbc, device_id);

                ////if current media is sbc ,stop the sbc streaming
                bt_media_clear_media_type(stream_type, device_id);

                info.id     = APP_BT_STREAM_A2DP_SBC;
                info.aud_id = 0;

                uint32_t lock;
                lock = int_lock_global();
                app_audio_list_rmv_by_info(&info);
                int_unlock_global(lock);

                //if current stream is the stop one ,so stop it
                if ((bt_media_get_current_media() & BT_STREAM_SBC)
#if !defined(IBRT)
                    && bt_meida.media_curr_sbc  == device_id
#endif
                )
                {
                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    bt_media_clear_current_media(BT_STREAM_SBC);
                    TRACE(0,"SBC STOPED!");
#ifdef __AI_VOICE__
                    app_ai_voice_resume_blocked_a2dp();
#endif
                }

                if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
                {
                    struct BT_DEVICE_T *curr_device = NULL;
                    enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
                     if(sbc_id < BT_DEVICE_NUM)
                    {
                        TRACE(1,"want to enable sbc id= %d",sbc_id);
                        curr_device = app_bt_get_device(sbc_id);
                        if(curr_device->a2dp_streamming)
                        {
                            bt_meida.media_curr_sbc = sbc_id;
                        }
                        else
                        {
                            bt_meida.media_curr_sbc = BT_DEVICE_INVALID_ID;
                            bt_media_clear_media_type(BT_STREAM_SBC,sbc_id);
                        }
                    }
                }
                else
                {
                    bt_meida.media_curr_sbc = BT_DEVICE_INVALID_ID;
                }

                if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
                {

                }
                else if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
                {
                    //do nothing
                }
                else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
                {
                    enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
                    if (sbc_id < BT_DEVICE_NUM && (media_pre_sbc != bt_meida.media_curr_sbc))
                    {
                        app_audio_manager_switch_a2dp(sbc_id);
                        bt_media_set_current_media(BT_STREAM_SBC);
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)(APP_BT_SETTING_SETUP),
                                (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    }
                }
            }
            break;
#ifdef MEDIA_PLAYER_SUPPORT
        case BT_STREAM_MEDIA:
            bt_media_clear_media_type(BT_STREAM_MEDIA ,device_id);

            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                //also have media report so do nothing
            }
            else if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                if(bt_media_get_current_media() & BT_STREAM_VOICE)
                {
                    //do nothing
                }
                else if(bt_media_get_current_media() & BT_STREAM_MEDIA)
                {
                    ///media report is end ,so goto voice
                    uint8_t curr_sco_id;
                    curr_sco_id = app_audio_manager_get_active_sco_num();
                    if (curr_sco_id != BT_DEVICE_INVALID_ID) {
                        bt_media_set_media_type(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)curr_sco_id);
                        bt_media_set_current_media(BT_STREAM_VOICE);
#if BT_DEVICE_NUM > 1
                        app_audio_manager_swap_sco((enum BT_DEVICE_ID_T)curr_sco_id);
#endif
                        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    }
                }
                else
                {
                    //media report is end ,so goto voice
                    TRACE(2, "curr_active_media is %x%s, goto voice", 
                             bt_meida.curr_active_media, 
                             bt_meida.curr_active_media ? strmtype2str(bt_meida.curr_active_media) : "[N/A]");
                    uint8_t curr_sco_id;
                    curr_sco_id = app_audio_manager_get_active_sco_num();
                    if (curr_sco_id != BT_DEVICE_INVALID_ID) {
                        TRACE(0, "goto voice open");
                        bt_media_set_current_media(BT_STREAM_VOICE);
                        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    }
                }
            }
            else if (btapp_hfp_is_call_active())
            {
                //do nothing
            }
        #ifdef __AI_VOICE__
            else if(bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) || ai_if_is_ai_stream_mic_open())
            {
                bt_media_clear_current_media(BT_STREAM_MEDIA);
                if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) && !(bt_media_get_current_media() & BT_STREAM_AI_VOICE))
                {
                    app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_AI_VOICE);
                }
                else if(app_ai_voice_is_need2block_a2dp()){
                    app_ai_voice_block_a2dp();
                }
                else if(bt_media_is_media_active_by_type(BT_STREAM_SBC)){
                    // Do nothing when prompt use DAC2
#ifdef AUDIO_PROMPT_USE_DAC2_ENABLED
                    if (media_id == AUDIO_ID_BT_MUTE)
#endif
                    {
                        enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
                        bt_media_set_media_type(BT_STREAM_SBC, sbc_id);
                        app_audio_manager_switch_a2dp(sbc_id);
                        bt_media_set_current_media(BT_STREAM_SBC);

                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                            (uint8_t)(APP_BT_SETTING_SETUP),
                                            (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));

                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    }
                }
            }
        #endif
            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                // Do nothing when prompt use DAC2
#ifdef AUDIO_PROMPT_USE_DAC2_ENABLED
                if (media_id == AUDIO_ID_BT_MUTE)
#endif
                {
                    TRACE(0, "A2DP stream active, start it");
                    ///if another device is also in sbc mode
                    enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
                    bt_media_set_media_type(BT_STREAM_SBC, sbc_id);
                    app_audio_manager_switch_a2dp(sbc_id);
                    bt_media_set_current_media(BT_STREAM_SBC);

                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                        (uint8_t)(APP_BT_SETTING_SETUP),
                                        (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));

                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                }
            }
            else
            {
                //have no meida task,so goto idle
                bt_media_set_current_media(0);
            }
            break;
#endif
        case BT_STREAM_VOICE:
            if(!bt_media_is_media_active_by_device(BT_STREAM_VOICE,device_id)||!(bt_media_get_current_media() & BT_STREAM_VOICE))
            {
                TRACE(0,"bt_media_stop already stop");
                bt_media_clear_media_type(stream_type, device_id);
                return ;
            }

        #ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
            if (!audio_prompt_check_on_stopping_stream(PENDING_TO_STOP_SCO_STREAMING, device_id))
            {
                TRACE(0,"Pending stop BT_STREAM_SCO");
                return;
            }
        #elif defined(AUDIO_PROMPY_USE_DAC2_ENABLED)
        #else
            if (app_ring_merge_isrun())
            {
                TRACE(0,"bt_media_stop pending BT_STREAM_VOICE");
                app_ring_merge_save_pending_start_stream_op(PENDING_TO_STOP_SCO_STREAMING, device_id);
                return;
            }
        #endif

            if (device_id == app_audio_manager_get_active_sco_num())
            {
                app_audio_manager_set_active_sco_num((enum BT_DEVICE_ID_T)BT_DEVICE_INVALID_ID);
            }

            bt_media_clear_media_type(stream_type, device_id);

#ifdef MEDIA_PLAYER_SUPPORT
#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                if(bt_media_get_current_media() & BT_STREAM_MEDIA)
                {
                    //do nothing
                }
                else if (bt_media_get_current_media() & BT_STREAM_VOICE)
                {
                    TRACE(0,"!!!!! WARNING VOICE if voice and media is all on,media should be the current media");
                    if(!bt_media_is_media_active_by_type(BT_STREAM_VOICE)){
                        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    }
                }
                else if (bt_media_get_current_media() & BT_STREAM_SBC)
                {
                    TRACE(0,"!!!!! WARNING SBC if voice and media is all on,media should be the current media");
                    if(!bt_media_is_media_active_by_type(BT_STREAM_SBC)){
                        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    }
                }
            }
            else
#endif
#endif
            if (bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                TRACE(0,"!!!!!!!!!bt_media_stop voice, wait another voice start");
            }
            else if (btapp_hfp_is_call_active())
            {
                TRACE(0,"stop in HF_CALL_ACTIVE and no sco need");
                bt_media_set_current_media(0);
                app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                bt_media_clear_media_type(BT_STREAM_VOICE, device_id);
            }
            else
            {
                bt_media_set_current_media(0);
                app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
            }

#ifdef __AI_VOICE__
            if(bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE)) {
                app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_AI_VOICE);
            }
            else
#endif
            if (bt_media_is_media_active_by_type(BT_STREAM_SBC) && !bt_media_cur_is_bt_stream_sbc())
            {
                enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
                TRACE(2, "d%x voice is stopped, d%x still have sbc wait to start", device_id, sbc_id);
                if (sbc_id != BT_DEVICE_INVALID_ID)
                {
                    bt_media_set_media_type(BT_STREAM_SBC, sbc_id);
                    app_audio_manager_switch_a2dp(sbc_id);
                    bt_media_set_current_media(BT_STREAM_SBC);

                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                        (uint8_t)(APP_BT_SETTING_SETUP),
                                        (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));

                    app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                }
            }
            break;

#ifdef RB_CODEC
        case BT_STREAM_RBCODEC:
            bt_media_rbcodec_stop_process(stream_type, device_id, 0);
            break;
#endif

#ifdef AUDIO_LINEIN
        case BT_STREAM_LINEIN:
            if(bt_media_is_media_active_by_type(BT_STREAM_LINEIN))
            {
                 app_audio_sendrequest(APP_PLAY_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                 if(bt_media_get_current_media() & BT_STREAM_LINEIN)
                    bt_media_set_current_media(0);

                 bt_media_clear_media_type(stream_type,device_id);
            }
            break;
#endif

#ifdef AUDIO_PCM_PLAYER
        case BT_STREAM_PCM_PLAYER:
            if(bt_media_is_media_active_by_type(BT_STREAM_PCM_PLAYER))
            {
                 app_audio_sendrequest(APP_PLAY_PCM_PLAYER, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                 if(bt_media_get_current_media() & BT_STREAM_PCM_PLAYER)
                    bt_media_set_current_media(0);

                 bt_media_clear_media_type(stream_type,device_id);
            }
            break;
#endif

        default:
            ASSERT(0,"bt_media_close ERROR TYPE: %x", stream_type);
            break;
    }

    TRACE(1, "bt_media_stop end %s\n", app_bt_get_active_media_state());
}

void app_media_clear_media(uint16_t stream_type,enum BT_DEVICE_ID_T device_id)
{
#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();
#endif

    if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA) || app_audio_list_playback_exist())
    {
#ifdef __AUDIO_QUEUE_SUPPORT__
        ////should have no sbc
        app_audio_list_clear();
#endif
        app_prompt_stop_all();

        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
    }
#endif
}

void app_media_stop_media(uint16_t stream_type,enum BT_DEVICE_ID_T device_id)
{
#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();
#endif

    if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA) || app_audio_list_playback_exist())
    {
#ifdef __AUDIO_QUEUE_SUPPORT__
        ////should have no sbc
        app_audio_list_clear();
#endif
        app_prompt_stop_all();

        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
        if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
        {
            enum BT_DEVICE_ID_T currScoId = (enum BT_DEVICE_ID_T)BT_DEVICE_INVALID_ID;
            currScoId = (enum BT_DEVICE_ID_T)app_audio_manager_get_active_sco_num();

            if (currScoId == BT_DEVICE_INVALID_ID){
                uint8_t i = 0;
                for (i = 0; i < BT_DEVICE_NUM; i++){
                    if (bt_media_is_media_active_by_device(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)i)){
                        currScoId = (enum BT_DEVICE_ID_T)i;
                        break;
                    }
                }
                if (i == BT_DEVICE_NUM)
                {
                    for (i = BT_SOURCE_DEVICE_ID_BASE; i < BT_SOURCE_DEVICE_ID_BASE + BT_SOURCE_DEVICE_NUM; i++){
                        if (bt_media_is_media_active_by_device(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)i)){
                            currScoId = (enum BT_DEVICE_ID_T)i;
                            break;
                        }
                    }
                }
            }

            TRACE(2,"%s try to resume sco:%d", __func__, currScoId);
            if (currScoId != BT_DEVICE_INVALID_ID){
                bt_media_set_media_type(BT_STREAM_VOICE,currScoId);
                bt_media_set_current_media(BT_STREAM_VOICE);
#if BT_DEVICE_NUM > 1
                app_audio_manager_swap_sco(currScoId);
#endif
                app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
            }
        }
#ifdef __AI_VOICE__
        else if(bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE))
        {
            if ((bt_media_get_current_media() & BT_STREAM_AI_VOICE) == 0)
            {
                app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_AI_VOICE);
            }
        }
#endif
        else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
        {
            enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
            bt_media_set_media_type(BT_STREAM_SBC,device_id);
            app_audio_manager_switch_a2dp(sbc_id);
            bt_media_set_current_media(BT_STREAM_SBC);
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                  (uint8_t)(APP_BT_SETTING_SETUP),
                                  (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
        }
    }
#endif
}

void app_media_update_media(uint16_t stream_type,enum BT_DEVICE_ID_T device_id)
{
    TRACE(1,"%s ",__func__);

#ifdef MEDIA_PLAYER_SUPPORT
    if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
    {
        //do nothing
        TRACE(0,"skip BT_STREAM_MEDIA");
    }
    else
#endif
    if(bt_media_is_media_active_by_type(BT_STREAM_VOICE) || btapp_hfp_is_call_active())
    {
        //do nothing
        TRACE(0,"skip BT_STREAM_VOICE");
        TRACE(3,"DEBUG INFO actByVoc:%d %d %d",    bt_media_is_media_active_by_type(BT_STREAM_VOICE),
                btapp_hfp_is_call_active(),
                btapp_hfp_incoming_calls());
    }
    else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
    {
#ifdef __AI_VOICE__
        if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) || ai_if_is_ai_stream_mic_open())
        {
            TRACE(0,"there is a ai voice stream exist, skip");
            return;
        }
#endif
        ///if another device is also in sbc mode
        TRACE(0,"try to resume sbc");
        enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
        if (0 == (bt_media_get_current_media() & BT_STREAM_SBC)){
            app_audio_manager_switch_a2dp(sbc_id);
            bt_media_set_current_media(BT_STREAM_SBC);
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                    (uint8_t)(APP_BT_SETTING_SETUP),
                    (uint32_t)(app_bt_get_device(sbc_id)->sample_rate & A2D_STREAM_SAMP_FREQ_MSK));
            app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
        }
    }else{
        TRACE(0,"skip idle");
    }
}

int app_audio_manager_sco_status_checker(void)
{
    btif_cmgr_handler_t *cmgrHandler __attribute__((unused));

    TRACE(1,"%s enter", __func__);
#if defined(CHIP_BEST1000) || defined(CHIP_BEST2000)
    uint32_t scoTransmissionInterval_reg;
    bt_bdaddr_t bdaddr;
    if (bt_meida.media_curr_sco != BT_DEVICE_INVALID_ID){
        BTDIGITAL_REG_GET_FIELD(0xD0220120, 0xff, 24, scoTransmissionInterval_reg);
        cmgrHandler = (btif_cmgr_handler_t *)btif_hf_get_chan_manager_handler(app_bt_get_device(bt_meida.media_curr_sco)->hf_channel);
        if (  btif_cmgr_is_audio_up(cmgrHandler))
        {
            if ( btif_cmgr_get_sco_connect_sco_link_type(cmgrHandler) == BTIF_BLT_ESCO)
            {
                if ( btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(cmgrHandler) != scoTransmissionInterval_reg)
                {
                    BTDIGITAL_REG_SET_FIELD(0xD0220120, 0xff, 24,  btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(cmgrHandler));
                }
            }
        }
        TRACE(4,"curSco:%d type:%d Interval:%d Interval_reg:%d", bt_meida.media_curr_sco,
                btif_cmgr_get_sco_connect_sco_link_type(cmgrHandler),
                btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(cmgrHandler),
                scoTransmissionInterval_reg);

        btif_hf_get_remote_bdaddr(app_bt_get_device(bt_meida.media_curr_sco)->hf_channel, &bdaddr);
        DUMP8("%02x ", bdaddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        uint32_t code_type;
        uint32_t code_type_reg;
        code_type = app_audio_manager_get_scocodecid();
        code_type_reg = BTDIGITAL_REG(0xD0222000);
        if (code_type == BTIF_HF_SCO_CODEC_MSBC) {
            BTDIGITAL_REG(0xD0222000) = (code_type_reg & (~(7<<1))) | (3<<1);
            TRACE(2,"MSBC!!!!!!!!!!!!!!!!!!! REG:0xD0222000=0x%08x B:%d", BTDIGITAL_REG(0xD0222000), (BTDIGITAL_REG(0xD0222000)>>15)&1);

        }else{
            BTDIGITAL_REG(0xD0222000) = (code_type_reg & (~(7<<1))) | (2<<1);
            TRACE(2,"CVSD!!!!!!!!!!!!!!!!!!! REG:0xD0222000=0x%08x B:%d", BTDIGITAL_REG(0xD0222000), (BTDIGITAL_REG(0xD0222000)>>15)&1);
        }
    }
#else
#if defined(DEBUG)
    app_bt_audio_state_checker();
#endif
#endif
    TRACE(1,"%s exit", __func__);
    return 0;
}

int app_audio_manager_swap_sco(enum BT_DEVICE_ID_T id)
{
    uint8_t curr_sco_id;
    bt_bdaddr_t bdAdd;
    uint16_t scohandle;
    struct BT_DEVICE_T* curr_device = NULL;

    if (bt_get_max_sco_number() <= 1)
        return 0;

    curr_sco_id = app_audio_manager_get_active_sco_num();

    curr_device = app_bt_get_device(id);

    if (btif_hf_get_remote_bdaddr(curr_device->hf_channel, &bdAdd))
    {
        DUMP8("%02x ", bdAdd.address, BT_ADDR_OUTPUT_PRINT_NUM);

        app_audio_manager_set_active_sco_num(id);
        scohandle = btif_hf_get_sco_hcihandle(curr_device->hf_channel);
        if (scohandle !=  BTIF_HCI_INVALID_HANDLE)
        {
            app_bt_Me_switch_sco(scohandle);
        }
        app_bt_stream_volume_ptr_update(bdAdd.address);
        app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, app_bt_stream_volume_get_ptr()->hfp_vol);
        if(curr_sco_id != id)
        {
            TRACE(2, "%s try restart d%x", __func__, id);
            bt_sco_player_forcemute(true, true);
            bt_media_set_current_media(BT_STREAM_VOICE);
            bt_media_set_media_type(BT_STREAM_VOICE, id);
            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_RESTART, 0);
        }
        app_audio_manager_sco_status_checker();
    }

    return 0;
}

typedef void (*app_audio_local_volume_change_callback_t)(uint8_t device_id);

int app_audio_manager_ctrl_volume_handle(APP_MESSAGE_BODY *msg_body)
{
    enum APP_AUDIO_MANAGER_VOLUME_CTRL_T volume_ctrl;
    uint8_t local_volume_changed_device_id = BT_DEVICE_INVALID_ID;
    uint16_t volume_level = 0;

    volume_ctrl  = (enum APP_AUDIO_MANAGER_VOLUME_CTRL_T)msg_body->message_ptr;
    volume_level = (uint16_t)msg_body->message_Param0;

    switch (volume_ctrl) {
        case APP_AUDIO_MANAGER_VOLUME_CTRL_SET:
            app_bt_stream_volumeset(volume_level);
            app_bt_stream_volume_edge_check();
            break;
        case APP_AUDIO_MANAGER_VOLUME_CTRL_UP:
            local_volume_changed_device_id = app_bt_stream_local_volumeup();//作为改变音量的标志：local_volume_changed_device_id
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case APP_AUDIO_MANAGER_VOLUME_CTRL_DOWN:
            local_volume_changed_device_id = app_bt_stream_local_volumedown();
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        default:
            break;
    }

    if (local_volume_changed_device_id != BT_DEVICE_INVALID_ID && msg_body->message_Param1)
    {
        ((app_audio_local_volume_change_callback_t)msg_body->message_Param1)(local_volume_changed_device_id);
    }

    return 0;
}

int app_audio_manager_tune_samplerate_ratio_handle(APP_MESSAGE_BODY *msg_body)
{
    enum AUD_STREAM_T stream = AUD_STREAM_NUM;
    float ratio = 1.0f;

    stream = (enum AUD_STREAM_T)msg_body->message_ptr;
    ratio = *(float *)&msg_body->message_Param0;

    TRACE(1,"codec_tune_resample_rate:%d", (int32_t)(ratio * 10000000));
    af_codec_tune(stream, ratio);

    return 0;
}

static bool app_audio_manager_init = false;


int app_audio_manager_sendrequest(uint8_t massage_id, uint16_t stream_type, uint8_t device_id, uint16_t aud_id)
{
    uint32_t audevt;
    uint32_t msg0;
    APP_MESSAGE_BLOCK msg;

    TRACE(7,"%s %d%s %d%s d%x aud %d\n",
        __func__,
        massage_id, handleId2str(massage_id),
        stream_type, stream_type ? strmtype2str(stream_type) : "[N/A]",
        device_id, aud_id);
    TRACE(2, "%s ca %p", __func__, __builtin_return_address(0));

    if(app_audio_manager_init == false)
        return -1;

    // only allow prompt playing if powering-off is on-going
    if (app_is_power_off_in_progress())
    {
        if ((APP_BT_STREAM_MANAGER_START == massage_id) &&
            (BT_STREAM_MEDIA != stream_type))
        {
            return -1;
        }
    }

    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    APP_AUDIO_MANAGER_SET_MESSAGE(audevt, massage_id, stream_type);
    APP_AUDIO_MANAGER_SET_MESSAGE0(msg0,device_id,aud_id);
    msg.msg_body.message_id = audevt;
    msg.msg_body.message_ptr = msg0;
    msg.msg_body.message_Param0 = msg0;
    msg.msg_body.message_Param1 = 0;
    msg.msg_body.message_Param2 = 0;
    app_mailbox_put(&msg);

    return 0;
}

int app_audio_manager_sendrequest_need_callback(
                           uint8_t massage_id, uint16_t stream_type, uint8_t device_id, uint16_t aud_id, uint32_t cb, uint32_t cb_param)
{
    uint32_t audevt;
    uint32_t msg0;
    APP_MESSAGE_BLOCK msg;

    TRACE(7,"aud_mgr send req:ca=%p %d%s %d%s d%x aud %d\n",
            __builtin_return_address(0),
            massage_id, handleId2str(massage_id),
            stream_type, stream_type ? strmtype2str(stream_type) : "[N/A]",
            device_id, aud_id);

    if(app_audio_manager_init == false)
        return -1;

    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    APP_AUDIO_MANAGER_SET_MESSAGE(audevt, massage_id, stream_type);
    APP_AUDIO_MANAGER_SET_MESSAGE0(msg0,device_id,aud_id);
    msg.msg_body.message_id = audevt;
    msg.msg_body.message_ptr = msg0;
    msg.msg_body.message_Param0 = msg0;
    msg.msg_body.message_Param1 = cb;
    msg.msg_body.message_Param2 = cb_param;
    app_mailbox_put(&msg);

    return 0;
}

int app_audio_manager_ctrl_volume_with_callback(enum APP_AUDIO_MANAGER_VOLUME_CTRL_T volume_ctrl, uint16_t volume_level, void (*cb)(uint8_t device_id))
{
    uint32_t audevt;
    APP_MESSAGE_BLOCK msg;
    osThreadId currThreadId;

    if(app_audio_manager_init == false)
        return -1;

    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    APP_AUDIO_MANAGER_SET_MESSAGE(audevt, APP_BT_STREAM_MANAGER_CTRL_VOLUME, 0);
    msg.msg_body.message_id     = audevt;
    msg.msg_body.message_ptr    = (uint32_t)volume_ctrl;
    msg.msg_body.message_Param0 = (uint32_t)volume_level;
    msg.msg_body.message_Param1 = (uint32_t)(uintptr_t)cb;
    msg.msg_body.message_Param2 = 0;
    currThreadId = osThreadGetId();
    if (currThreadId == af_thread_tid_get() ||
        currThreadId == app_os_tid_get()){
        app_audio_manager_ctrl_volume_handle(&msg.msg_body);
    }else{
        app_mailbox_put(&msg);
    }
    return 0;
}

int app_audio_manager_ctrl_volume(enum APP_AUDIO_MANAGER_VOLUME_CTRL_T volume_ctrl, uint16_t volume_level)
{
    return app_audio_manager_ctrl_volume_with_callback(volume_ctrl, volume_level, NULL);
}

int app_audio_manager_tune_samplerate_ratio(enum AUD_STREAM_T stream, float ratio)
{
    uint32_t audevt;
    APP_MESSAGE_BLOCK msg;
    osThreadId currThreadId;

    if(app_audio_manager_init == false)
        return -1;

    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    APP_AUDIO_MANAGER_SET_MESSAGE(audevt, APP_BT_STREAM_MANAGER_TUNE_SAMPLERATE_RATIO, 0);
    msg.msg_body.message_id     = audevt;
    msg.msg_body.message_ptr    = (uint32_t)stream;
    msg.msg_body.message_Param0 = *(uint32_t *)&ratio;
    msg.msg_body.message_Param1 = 0;
    msg.msg_body.message_Param2 = 0;

    currThreadId = osThreadGetId();
    if (currThreadId == af_thread_tid_get() ||
        currThreadId == app_os_tid_get()){
        app_audio_manager_tune_samplerate_ratio_handle(&msg.msg_body);
    }else{
        app_mailbox_put(&msg);
    }

    return 0;
}

static int app_audio_manager_handle_process(APP_MESSAGE_BODY *msg_body)
{
    int nRet = 0;
    APP_AUDIO_MANAGER_MSG_STRUCT aud_manager_msg;
    APP_AUDIO_MANAGER_CALLBACK_T callback_fn = NULL;
    uint32_t callback_param = 0;
    if(app_audio_manager_init == false)
        return -1;
    APP_AUDIO_MANAGER_GET_ID(msg_body->message_id, aud_manager_msg.id);
    APP_AUDIO_MANAGER_GET_STREAM_TYPE(msg_body->message_id, aud_manager_msg.stream_type);
    APP_AUDIO_MANAGER_GET_DEVICE_ID(msg_body->message_Param0, aud_manager_msg.device_id);
    APP_AUDIO_MANAGER_GET_AUD_ID(msg_body->message_Param0, aud_manager_msg.aud_id);
    APP_AUDIO_MANAGER_GET_CALLBACK(msg_body->message_Param1, callback_fn);
    APP_AUDIO_MANAGER_GET_CALLBACK_PARAM(msg_body->message_Param2, callback_param);
    TRACE(7, "%s %d%s %x%s d%x aud %x", __func__,
          aud_manager_msg.id, handleId2str(aud_manager_msg.id),
          aud_manager_msg.stream_type, aud_manager_msg.stream_type ? strmtype2str(aud_manager_msg.stream_type) : "[N/A]",
          aud_manager_msg.device_id, aud_manager_msg.aud_id);

    switch (aud_manager_msg.id) {
        case APP_BT_STREAM_MANAGER_START:
            bt_media_start(aud_manager_msg.stream_type,(enum BT_DEVICE_ID_T) aud_manager_msg.device_id, aud_manager_msg.aud_id);
            break;
        case APP_BT_STREAM_MANAGER_STOP:
            bt_media_stop(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id, aud_manager_msg.aud_id);
            break;
        case APP_BT_STREAM_MANAGER_STOP_MEDIA:
            app_media_stop_media(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_CLEAR_MEDIA:
            app_media_clear_media(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_UPDATE_MEDIA:
            app_media_update_media(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_SWAP_SCO:
            app_audio_manager_swap_sco((enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_CTRL_VOLUME:
            app_audio_manager_ctrl_volume_handle(msg_body);
            break;
        case APP_BT_STREAM_MANAGER_TUNE_SAMPLERATE_RATIO:
            app_audio_manager_tune_samplerate_ratio_handle(msg_body);
            break;
        default:
            break;
    }
    if (callback_fn){
        callback_fn(aud_manager_msg.device_id, aud_manager_msg.id, callback_param);
    }
    return nRet;
}

void bt_media_volume_ptr_update_by_mediatype(uint16_t stream_type)
{
    uint8_t id = 0;
    struct BT_DEVICE_T *curr_device = NULL;

    TRACE(2,"%s %d enter", __func__, stream_type);
    if (stream_type & bt_media_get_current_media()){
        switch (stream_type) {
            case BT_STREAM_SBC:
                id = bt_meida.media_curr_sbc;
                ASSERT(id<BT_DEVICE_NUM, "INVALID_BT_DEVICE_NUM"); // only a2dp sink has speaker out sbc media
                curr_device = app_bt_get_device(id);
                app_bt_stream_volume_ptr_update(curr_device->remote.address);
                break;
            case BT_STREAM_VOICE:
                id = app_audio_manager_get_active_sco_num();
                ASSERT(id<BT_DEVICE_NUM || (id>=BT_SOURCE_DEVICE_ID_BASE && id<BT_SOURCE_DEVICE_ID_BASE+BT_SOURCE_DEVICE_NUM), "INVALID_BT_DEVICE_NUM");
                curr_device = app_bt_get_device(id);
                app_bt_stream_volume_ptr_update(curr_device->remote.address);
                break;
            case BT_STREAM_MEDIA:
            default:
                break;
        }
    }
    TRACE(1,"%s exit", __func__);
}

int app_audio_manager_set_active_sco_num(enum BT_DEVICE_ID_T id)
{
    bt_meida.media_curr_sco = id;
    return 0;
}

int app_audio_manager_get_active_sco_num(void)
{
    return bt_meida.media_curr_sco;
}

btif_hf_channel_t* app_audio_manager_get_active_sco_chnl(void)
{
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID)
    {
        return app_bt_get_device(curr_sco)->hf_channel;
    }
    return NULL;
}

int app_audio_manager_set_scocodecid(enum BT_DEVICE_ID_T dev_id, uint16_t codec_id)
{
    app_bt_get_device(dev_id)->app_audio_manage_scocodecid = codec_id;
    return 0;
}

hfp_sco_codec_t app_audio_manager_get_scocodecid(void)
{
    hfp_sco_codec_t scocodecid = BTIF_HF_SCO_CODEC_NONE;
    if (bt_meida.media_curr_sco != BT_DEVICE_INVALID_ID){
        scocodecid = (hfp_sco_codec_t)app_bt_get_device(bt_meida.media_curr_sco)->app_audio_manage_scocodecid;
    }
    return scocodecid;
}

int app_audio_manager_switch_a2dp(enum BT_DEVICE_ID_T id)
{
    bt_bdaddr_t* bdAdd;
    btif_remote_device_t *remDev = NULL;
    remDev = btif_a2dp_get_remote_device(app_bt_get_device(id)->a2dp_connected_stream);
    if (remDev)
    {
        TRACE(2,"%s switch_a2dp to id:%d", __func__, id);
        bdAdd = btif_me_get_remote_device_bdaddr(remDev);
        app_bt_stream_volume_ptr_update(bdAdd->address);
        bt_meida.media_curr_sbc = id;
    }
    return 0;
}

bool app_audio_manager_a2dp_is_active(enum BT_DEVICE_ID_T id)
{
    uint16_t media_type;
    bool nRet = false;

    media_type = bt_media_get_current_media();
    if (media_type & BT_STREAM_SBC){
        if (bt_meida.media_curr_sbc == id){
            nRet = true;
        }
    }

#ifndef BES_AUTOMATE_TEST
    TRACE(5,"%s nRet:%d type:%d %d/%d", __func__, nRet, media_type, id, bt_meida.media_curr_sbc);
#endif
    return nRet;
}

bool app_audio_manager_hfp_is_active(enum BT_DEVICE_ID_T id)
{
    uint16_t media_type;
    bool nRet = false;

    media_type = bt_media_get_current_media();
    if (media_type & BT_STREAM_VOICE){
        if (bt_meida.media_curr_sco == id){
            nRet = true;
        }
    }

#ifndef BES_AUTOMATE_TEST
    TRACE(5,"%s nRet:%d type:%d %d/%d", __func__, nRet, media_type, id, bt_meida.media_curr_sco);
#endif
    return nRet;
}

bool app_audio_manager_media_is_active(void)
{
    uint16_t media_type;
    bool nRet = false;

    media_type = bt_media_get_current_media();
    if (media_type & BT_STREAM_MEDIA){
            nRet = true;
    }

    return nRet;
}

void app_audio_manager_open(void)
{
    if(app_audio_manager_init){
        return;
    }
    bt_meida.media_curr_sbc = BT_DEVICE_INVALID_ID;
    bt_meida.media_curr_sco = BT_DEVICE_INVALID_ID;
    bt_meida.curr_active_media = 0;
    app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE, app_audio_manager_handle_process);
    app_bt_stream_register_a2dp_param_callback(app_audio_manager_get_a2dp_codec_type);
    app_audio_manager_init = true;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_init_handler();
#endif
}

void app_audio_manager_close(void)
{
    app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE, NULL);
    app_bt_stream_register_a2dp_param_callback(NULL);
    app_audio_manager_init = false;
}

#ifdef RB_CODEC

static bool app_rbcodec_play_status = false;

static bool app_rbplay_player_mode = false;

bool app_rbplay_is_localplayer_mode(void)
{
    return app_rbplay_player_mode;
}

bool app_rbplay_mode_switch(void)
{
    return (app_rbplay_player_mode = !app_rbplay_player_mode);
}

void app_rbplay_set_player_mode(bool isInPlayerMode)
{
    app_rbplay_player_mode = isInPlayerMode;
}

void app_rbcodec_ctr_play_onoff(bool on )
{
    TRACE(3,"%s %d ,turnon?%d ",__func__,app_rbcodec_play_status,on);

    if(app_rbcodec_play_status == on)
        return;
    app_rbcodec_play_status = on;
    if(on)
        app_audio_manager_sendrequest( APP_BT_STREAM_MANAGER_START, BT_STREAM_RBCODEC, 0, 0);
    else
        app_audio_manager_sendrequest( APP_BT_STREAM_MANAGER_STOP, BT_STREAM_RBCODEC, 0, 0);
}

void app_rbcodec_ctl_set_play_status(bool st)
{
    app_rbcodec_play_status = st;
    TRACE(2,"%s %d",__func__,app_rbcodec_play_status);
}

bool app_rbcodec_get_play_status(void)
{
    TRACE(2,"%s %d",__func__,app_rbcodec_play_status);
    return app_rbcodec_play_status;
}

void app_rbcodec_toggle_play_stop(void)
{
    if(app_rbcodec_get_play_status()) {
        app_rbcodec_ctr_play_onoff(false);
    } else {
        app_rbcodec_ctr_play_onoff(true);
    }
}

bool app_rbcodec_check_hfp_active(void )
{
    return (bool)bt_media_is_media_active_by_type(BT_STREAM_VOICE);
}
#endif

void app_ibrt_sync_mix_prompt_req_handler(uint8_t* ptrParam, uint16_t paramLen)
{
#if defined(TWS_PROMPT_SYNC) && !defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
    app_tws_cmd_sync_mix_prompt_req_handler(ptrParam, paramLen);
#endif
}

void app_audio_decode_err_force_trigger(void)
{
#ifndef IBRT
    media_PlayAudio_standalone(AUDIO_ID_BT_MUTE, 0);
#endif
}



