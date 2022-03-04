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

#include "hal_aud.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "apps.h"
#include "app_utils.h"
#include "app_thread.h"
#include "app_status_ind.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "me_api.h"
#include "mei_api.h"
#include "a2dp_api.h"
#include "hci_api.h"
#include "l2cap_api.h"
#include "hfp_api.h"
#include "dip_api.h"
#include "btapp.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "app_bt_func.h"
#include "bt_drv_interface.h"
#include "bt_if.h"
#include "os_api.h"
#include "bt_drv_reg_op.h"
#include "app_a2dp.h"
#include "app_dip.h"
#if defined(BT_SOURCE)
#include "bt_source.h"
#endif
#include "app_bt_media_manager.h"
#include "app_bt_stream.h"
#include "app_audio.h"
#include "audio_policy.h"
#include "a2dp_decoder.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#include "app_ibrt_a2dp.h"
#include "app_tws_besaud.h"
#ifdef IBRT_UI_V1
#include "app_ibrt_ui.h"
#endif
#ifdef IBRT_CORE_V2_ENABLE
#include "app_tws_ibrt_conn_api.h"
#include "app_ibrt_tws_ext_cmd.h"
#endif
#endif

#include "platform_deps.h"

#ifdef __AI_VOICE__
#include "app_ai_voice.h"
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif
extern uint8_t bt_media_current_sbc_get(void);
extern uint8_t bt_media_current_sco_get(void);
extern void bt_media_clear_media_type(uint16_t media_type,enum BT_DEVICE_ID_T device_id);
extern void bt_media_clear_current_media(uint16_t media_type);
static uint8_t app_bt_audio_create_sco_for_another_call_active_device(uint8_t device_id);

void app_bt_init_config_postphase(struct app_bt_config *config)
{
    config->block_2nd_sco_before_call_active = false;
    config->pause_bg_a2dp_stream = false;
    config->close_bg_a2dp_stream = false;
    config->dont_auto_play_bg_a2dp = false;
    config->a2dp_prompt_play_only_when_avrcp_play_received = false;
    config->a2dp_delay_prompt_play = false;
    config->pause_old_a2dp_when_new_sco_connected = false;
    config->mute_new_a2dp_stream = false;
    config->pause_new_a2dp_stream = false;

    config->a2dp_prompt_play_mode = false;
    config->keep_only_one_stream_close_connected_a2dp = true;
    config->close_new_a2dp_stream = true;

    TRACE(12, "%s red a2dp non-prompt %d%d%d%d%d%d%d%d sco %d%d%d", __func__,
                config->pause_bg_a2dp_stream,
                config->close_bg_a2dp_stream,
                config->dont_auto_play_bg_a2dp,
                config->a2dp_prompt_play_only_when_avrcp_play_received,
                config->mute_new_a2dp_stream,
                config->pause_new_a2dp_stream,
                config->close_new_a2dp_stream,
                config->sco_prompt_play_mode,
                config->keep_only_one_stream_close_connected_a2dp,
                config->keep_only_one_sco_link,
                config->block_2nd_sco_before_call_active);
}

uint8_t app_bt_audio_select_another_streaming_a2dp(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint32_t stream_prio = 0;
    uint8_t device_id = BT_DEVICE_INVALID_ID;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == curr_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->a2dp_streamming && !curr_device->a2dp_disc_on_process)
        {
            if (curr_device->a2dp_audio_prio > stream_prio)
            {
                stream_prio = curr_device->a2dp_audio_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_streaming_a2dp(void)
{
    return app_bt_audio_select_another_streaming_a2dp(BT_DEVICE_INVALID_ID);
}

uint8_t app_bt_audio_select_any_streaming_a2dp(void)
{
    struct BT_DEVICE_T* curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->a2dp_streamming)
        {
            return i;
        }
    }

    return BT_DEVICE_INVALID_ID;
}

uint8_t app_bt_audio_count_streaming_a2dp(void)
{
    uint8_t count = 0;
    struct BT_DEVICE_T* curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->a2dp_streamming)
        {
            count += 1;
        }
    }

    return count;
}

uint8_t app_bt_audio_count_straming_mobile_links(void)
{
    uint8_t count = 0;
    struct BT_DEVICE_T* curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device)
        {
            if ((curr_device->a2dp_streamming) || app_bt_is_sco_connected(i))
            {
                count += 1;
            }
        }
    }

    return count;
}

uint8_t app_bt_audio_count_connected_a2dp(void)
{
    uint8_t count = 0;
    struct BT_DEVICE_T* curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->a2dp_conn_flag)
        {
            count += 1;
        }
    }

    return count;
}

uint8_t app_bt_audio_get_curr_playing_a2dp(void)
{
#if 0
    uint8_t device_id = bt_media_current_sbc_get();

    if (device_id != BT_DEVICE_NUM)
    {
        return device_id;
    }

    device_id = BT_DEVICE_INVALID_ID;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (bt_media_is_media_active_by_device(BT_STREAM_SBC, (enum BT_DEVICE_ID_T)i))
        {
            device_id = i;
            break;
        }
    }

    return device_id;
#endif

    return app_bt_manager.curr_playing_a2dp_id;
}

uint8_t app_bt_audio_select_another_streaming_sco(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint32_t stream_prio = 0;
    uint8_t device_id = BT_DEVICE_INVALID_ID;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == curr_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
        {
            if (curr_device->sco_audio_prio > stream_prio)
            {
                stream_prio = curr_device->sco_audio_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_streaming_sco(void)
{
    return app_bt_audio_select_another_streaming_sco(BT_DEVICE_INVALID_ID);
}

uint8_t app_bt_audio_count_connected_sco(void)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t count = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
        {
            count += 1;
        }
    }

    return count;
}

bool app_bt_get_if_sco_audio_connected(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    bool status = false;

    if (curr_device_id < BT_DEVICE_NUM)
    {
        curr_device = app_bt_get_device(curr_device_id);
        if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
        {
            status = true;
        }
    }

    return status;
}

uint8_t app_bt_audio_count_connected_hfp(void)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t count = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_conn_flag)
        {
            count += 1;
        }
    }

    return count;
}

uint8_t app_bt_audio_get_curr_playing_sco(void)
{
#if 0
    uint8_t device_id = bt_media_current_sco_get();

    if (device_id != BT_DEVICE_NUM)
    {
        return device_id;
    }

    device_id = BT_DEVICE_INVALID_ID;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (bt_media_is_media_active_by_device(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)i))
        {
            device_id = i;
            break;
        }
    }

    return device_id;
#endif

    return app_bt_manager.curr_playing_sco_id;
}

uint8_t app_bt_audio_select_another_connected_a2dp(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    uint32_t a2dp_prio = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == curr_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->a2dp_conn_flag && !curr_device->a2dp_disc_on_process)
        {
            if (curr_device->a2dp_conn_prio > a2dp_prio)
            {
                a2dp_prio = curr_device->a2dp_conn_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_connected_a2dp(void)
{
    return app_bt_audio_select_another_connected_a2dp(BT_DEVICE_INVALID_ID);
}

uint8_t app_bt_audio_select_connected_device(void)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    uint32_t acl_conn_prio = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->acl_is_connected)
        {
            if (curr_device->acl_conn_prio > acl_conn_prio)
            {
                acl_conn_prio = curr_device->acl_conn_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_another_connected_hfp(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    uint32_t hfp_prio = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == curr_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_conn_flag)
        {
            if (curr_device->hfp_conn_prio > hfp_prio)
            {
                hfp_prio = curr_device->hfp_conn_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_connected_hfp(void)
{
    return app_bt_audio_select_another_connected_hfp(BT_DEVICE_INVALID_ID);
}

uint8_t app_bt_audio_select_another_call_setup_hfp(uint8_t curr_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    uint32_t hfp_prio = 0;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == curr_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->hfchan_callSetup)
        {
            if (curr_device->hfp_call_setup_prio > hfp_prio)
            {
                hfp_prio = curr_device->hfp_call_setup_prio;
                device_id = i;
            }
        }
    }

    return device_id;
}

uint8_t app_bt_audio_select_call_setup_hfp(void)
{
    return app_bt_audio_select_another_call_setup_hfp(BT_DEVICE_INVALID_ID);
}

static const char* app_bt_audio_get_active_calls_state(void)
{
    struct BT_DEVICE_T* curr_device = NULL;
    static char buff[64] = {0};
    int len = 0;
    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        len += sprintf(buff+len, "%d(prio %d)", curr_device->hfchan_call, curr_device->hfp_call_active_prio);
    }
    return buff;
}

static uint8_t app_bt_audio_choice_other_call_active_device(uint8_t sco_disconnected_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t found_id = BT_DEVICE_INVALID_ID;
    uint8_t first_call_active_device = BT_DEVICE_INVALID_ID;
    uint32_t hfp_prio = 0;

    TRACE(2, "(d%x) active calls state: %s", sco_disconnected_device_id, app_bt_audio_get_active_calls_state());

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == sco_disconnected_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->hfchan_call)
        {
            if (first_call_active_device == BT_DEVICE_INVALID_ID)
            {
                first_call_active_device = i;
            }
            if (curr_device->hfp_call_active_prio > hfp_prio)
            {
                hfp_prio = curr_device->hfp_call_active_prio;
                found_id = i;
            }
        }
    }

    if (found_id == BT_DEVICE_INVALID_ID)
    {
        found_id = first_call_active_device;
    }

    return found_id;
}

uint8_t app_bt_audio_select_another_device_to_create_sco(uint8_t curr_device_id)
{
    uint8_t found_id = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T* curr_device = NULL;

    found_id = app_bt_audio_choice_other_call_active_device(curr_device_id);

    if (found_id == BT_DEVICE_INVALID_ID) // continue check call callsetup status
    {
        found_id = app_bt_audio_select_another_call_setup_hfp(curr_device_id);
        if (found_id != BT_DEVICE_INVALID_ID)
        {
            curr_device = app_bt_get_device(found_id);
            if (curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN && !btif_hf_is_inbandring_enabled(curr_device->hf_channel))
            {
                found_id = BT_DEVICE_INVALID_ID; // only create sco for in-band ring incoming call
            }
        }
    }

    return found_id;
}

uint8_t app_bt_audio_select_another_call_active_hfp(uint8_t curr_device_id)
{
    return app_bt_audio_choice_other_call_active_device(curr_device_id);
}

uint8_t app_bt_audio_select_call_active_hfp(void)
{
    return app_bt_audio_select_another_call_active_hfp(BT_DEVICE_INVALID_ID);
}

void app_bt_audio_stop_media_playing(uint8_t device_id)
{
    bt_media_clear_media_type(BT_STREAM_MEDIA, (enum BT_DEVICE_ID_T)device_id);
    if (bt_media_cur_is_bt_stream_media())
    {
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_MEDIA);
    }
}

static void app_bt_a2dp_set_curr_stream(uint8_t device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    btif_remote_device_t* activeRem = NULL;

    if (device_id == BT_DEVICE_AUTO_CHOICE_ID)
    {
        /** a2dp stream priority:
         *      1. a2dp playing device
         *      2. stream started device
         *      3. a2dp connected device
         *      4. BT_DEVICE_ID_1
         */
        device_id = app_bt_audio_get_curr_playing_a2dp();
        if (device_id == BT_DEVICE_INVALID_ID)
        {
            device_id = app_bt_audio_select_streaming_a2dp();
            if (device_id == BT_DEVICE_INVALID_ID)
            {
                device_id = app_bt_audio_select_connected_a2dp();
                if(app_bt_manager.a2dp_last_paused_device != BT_DEVICE_INVALID_ID && app_bt_manager.a2dp_last_paused_device != device_id)
                {
                        if(app_bt_get_device(app_bt_manager.a2dp_last_paused_device)->a2dp_conn_flag)
                        {
                            device_id = app_bt_manager.a2dp_last_paused_device;
                            TRACE(1,"%s : select last paused device as current device!",__func__);
                        }
                }
                if (device_id == BT_DEVICE_INVALID_ID)
                {
                    device_id = BT_DEVICE_ID_1;
                }
            }
        }
    }
    
    TRACE(2, "%s as %x", __func__, device_id);

    if (device_id == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s,no valid device id found",__func__);
        return;
    }

    app_bt_manager.curr_a2dp_stream_id = (enum BT_DEVICE_ID_T)device_id;

    curr_device = app_bt_get_device(device_id);

    if(curr_device->a2dp_connected_stream) {
        activeRem = btif_a2dp_get_remote_device(curr_device->a2dp_connected_stream);
    }

    if(activeRem) {
        app_bt_manager.current_a2dp_conhdl= btif_me_get_remote_device_hci_handle(activeRem);
    } else {
        app_bt_manager.current_a2dp_conhdl= 0xffff;
    }
}

void app_bt_audio_enable_active_link(bool enable, uint8_t active_id)
{
    btif_remote_device_t* active_rem = NULL;
    uint8_t active_role = 0;
    uint16_t active_handle = 0;
    btif_remote_device_t* prev_active_rem = NULL;
    uint8_t prev_active_role = 0;
    uint16_t prev_active_handle = 0;
    uint8_t link_id[BT_DEVICE_NUM];
    uint8_t active[BT_DEVICE_NUM];
    uint8_t num = 0;
    if (BT_DEVICE_NUM <= 1)
    {
        return;
    }

    if (enable && active_id == BT_DEVICE_INVALID_ID)
    {
        return;
    }

    if (enable && active_id == BT_DEVICE_AUTO_CHOICE_ID)
    {
        if (app_bt_audio_count_connected_a2dp() >= 2 && app_bt_audio_get_curr_playing_a2dp() != BT_DEVICE_INVALID_ID)
        {
            active_id = app_bt_audio_get_curr_playing_a2dp();
        }
        else
        {
            enable = false;
            active_id = BT_DEVICE_INVALID_ID;
        }
    }

    TRACE(4, "%s %d active_id %x prev_id %x", __func__, enable, active_id, app_bt_manager.prev_active_audio_link);

    if (enable && app_bt_manager.prev_active_audio_link == active_id)
    {
        // the link already active
        return;
    }

    if (app_bt_manager.prev_active_audio_link != BT_DEVICE_INVALID_ID)
    {
        prev_active_rem = btif_a2dp_get_remote_device(app_bt_get_device(app_bt_manager.prev_active_audio_link)->a2dp_connected_stream);
        if (prev_active_rem)
        {
            prev_active_role = btif_me_get_remote_device_role(prev_active_rem);
            prev_active_handle = btif_me_get_remote_device_hci_handle(prev_active_rem);
            if (prev_active_role == 0) // MASTER
            {
                bt_drv_reg_op_set_tpoll(prev_active_handle-0x80, 0x40); // restore default tpoll
            }
            link_id[num] = prev_active_handle-0x80;
            active[num] = 0;
            num++;
        }
        app_bt_manager.prev_active_audio_link = BT_DEVICE_INVALID_ID;
    }

    if (enable)
    {
        active_rem = btif_a2dp_get_remote_device(app_bt_get_device(active_id)->a2dp_connected_stream);
        if (active_rem)
        {
            active_role = btif_me_get_remote_device_role(active_rem);
            active_handle = btif_me_get_remote_device_hci_handle(active_rem);

            bt_drv_reg_op_set_music_link(active_handle-0x80);
            bt_drv_reg_op_set_music_link_duration_extra(11);

            if (active_role == 0) // MASTER
            {
                bt_drv_reg_op_set_tpoll(active_handle-0x80, 0x10); // use smaller tpoll
            }
            link_id[num] = active_handle-0x80;
            active[num] = 1;
            num++;
            bt_drv_reg_op_multi_ibrt_music_config(link_id,active,num);
            app_bt_manager.prev_active_audio_link = active_id;
            return;
        }
    }
    if(num > 0)
    {
        bt_drv_reg_op_multi_ibrt_music_config(link_id,active,num);
    }
    bt_drv_reg_op_set_music_link(BT_DEVICE_INVALID_ID);
    app_bt_manager.prev_active_audio_link = BT_DEVICE_INVALID_ID;
}

static void app_bt_audio_start_a2dp_playing(uint8_t device_id)
{
#if defined(BT_WATCH_APP)
    return;
#endif
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    curr_device->a2dp_play_pause_flag = 1;
    app_bt_a2dp_set_curr_stream(device_id);
    app_bt_audio_enable_active_link(true, device_id); // let current playing a2dp get more timing

    if (besbt_cfg.a2dp_play_stop_media_first)
    {
#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
        for (int i = 0; i < BT_DEVICE_NUM; i += 1) // stop media if any
        {
            if (bt_media_is_media_active_by_device(BT_STREAM_MEDIA, (enum BT_DEVICE_ID_T)i)
#ifdef __AI_VOICE__
                && (!app_ai_voice_is_need2block_a2dp())
#endif
                )
            {
                app_bt_audio_stop_media_playing(i);
            }
        }
#endif
    }

    if (app_bt_manager.curr_playing_a2dp_id != device_id || app_bt_manager.sco_trigger_a2dp_replay)
    {
        app_bt_manager.sco_trigger_a2dp_replay = false;

        app_bt_manager.curr_playing_a2dp_id = device_id;

        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_SBC, device_id, MAX_RECORD_NUM);
    }
}

static void app_bt_audio_stop_a2dp_playing(uint8_t device_id)
{
#if defined(BT_WATCH_APP)
    return;
#endif

    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    curr_device->a2dp_play_pause_flag = 0;

    if (app_bt_manager.curr_playing_a2dp_id == device_id)
    {
        app_bt_manager.curr_playing_a2dp_id = BT_DEVICE_INVALID_ID;

        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_SBC, device_id, MAX_RECORD_NUM);
    }

    app_bt_a2dp_set_curr_stream(BT_DEVICE_AUTO_CHOICE_ID);
}

uint32_t app_bt_audio_create_new_prio(void)
{
    return ++app_bt_manager.audio_prio_seed;
}

#define APP_BT_AUDIO_WAIT_BLOCKED_SCO_BECOME_CONNECTED_TIME_MS (600)
static void app_bt_audio_wait_blocked_sco_handler(void const *n);
osTimerDef (APP_BT_AUDIO_WAIT_BLOCKED_SCO_BECOME_CONNECTED_TIMER, app_bt_audio_wait_blocked_sco_handler);
#define APP_BT_AUDIO_WAIT_BLOCKED_SCO_MAX_TIMES (2)
static uint8_t g_app_bt_audio_sco_connect_times_count = 0;

static void app_bt_audio_wait_this_sco_become_connected(uint8_t device_id, bool first_time)
{
    if (!app_bt_manager.wait_sco_connected_timer)
    {
        app_bt_manager.wait_sco_connected_timer = osTimerCreate(osTimer(APP_BT_AUDIO_WAIT_BLOCKED_SCO_BECOME_CONNECTED_TIMER), osTimerOnce, NULL);
    }

    if (app_bt_manager.wait_sco_connected_device_id != BT_DEVICE_INVALID_ID &&
        app_bt_manager.wait_sco_connected_device_id != device_id)
    {
        TRACE(1, "(d%x) already has a sco waiting device", device_id);
        g_app_bt_audio_sco_connect_times_count = 0;
        return;
    }

    if (first_time)
    {
        g_app_bt_audio_sco_connect_times_count = 1;
    }
    else
    {
        g_app_bt_audio_sco_connect_times_count += 1;
    }

    app_bt_manager.wait_sco_connected_device_id = device_id;

    osTimerStop(app_bt_manager.wait_sco_connected_timer);

    osTimerStart(app_bt_manager.wait_sco_connected_timer, APP_BT_AUDIO_WAIT_BLOCKED_SCO_BECOME_CONNECTED_TIME_MS);
}

static void app_bt_audio_route_sco_path_to_phone(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device)
    {
        app_bt_manager.device_routed_sco_to_phone = device_id;
        TRACE(2,"%s d%x", __func__, device_id);
        app_bt_HF_DisconnectAudioLink(curr_device->hf_channel);
    }
}

static bool app_bt_audio_route_sco_path_back_to_earbud(uint8_t device_id, bool create_sco_in_normal_way)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->hf_conn_flag)
    {
        return false;
    }

    if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
    {
        return false;
    }

    if (curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE  || curr_device->hfchan_callSetup != BTIF_HF_CALL_SETUP_NONE)
    {
        if (create_sco_in_normal_way)
        {
            app_bt_HF_CreateAudioLink(curr_device->hf_channel);
        }
        else
        {
            app_bt_hf_create_sco_directly(device_id);
        }

        if (device_id == app_bt_manager.device_routed_sco_to_phone)
        {
            app_bt_manager.device_routed_sco_to_phone = BT_DEVICE_INVALID_ID;
        }

        app_bt_manager.device_routing_sco_back = device_id;

        return true;
    }
    else
    {
        return false;
    }
}

static void app_bt_audio_wait_blocked_sco_handler(void const *n)
{
    uint8_t device_id = app_bt_manager.wait_sco_connected_device_id;
    uint8_t another_connected_sco = app_bt_audio_select_another_streaming_sco(device_id);
    struct BT_DEVICE_T *curr_device = NULL;
    bool wait_sco_conn_failed = false;

    app_bt_manager.wait_sco_connected_device_id = BT_DEVICE_INVALID_ID;

    curr_device = app_bt_get_device(device_id);

    TRACE(7, "(d%x) %s %d conn %d call %d callsetup %d sco %d", device_id, __func__, g_app_bt_audio_sco_connect_times_count,
        curr_device->hf_conn_flag, curr_device->hfchan_call, curr_device->hfchan_callSetup, curr_device->hf_audio_state);

    if (!curr_device->hf_conn_flag)
    {
        wait_sco_conn_failed = true;
        goto wait_finished;
    }

    if (curr_device->hfchan_call != BTIF_HF_CALL_ACTIVE  && curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_NONE)
    {
        wait_sco_conn_failed = true;
        goto wait_finished;
    }

    if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
    {
        goto wait_finished;
    }

    if (another_connected_sco != BT_DEVICE_INVALID_ID)
    {
        // route exist connected sco path to phone
        app_bt_audio_route_sco_path_to_phone(another_connected_sco); // after this sco disc, it will auto create sco for call active device
        goto wait_finished;
    }

    if (app_bt_audio_route_sco_path_back_to_earbud(device_id, (g_app_bt_audio_sco_connect_times_count == 1) ? true : false))
    {
        if (g_app_bt_audio_sco_connect_times_count < APP_BT_AUDIO_WAIT_BLOCKED_SCO_MAX_TIMES)
        {
            goto continue_wait;
        }
    }

wait_finished:
    g_app_bt_audio_sco_connect_times_count = 0;
    if (wait_sco_conn_failed)
    {
        /* wait blocked sco connected failed, may be due to call already hangup normal cases.
            we need check other call active device to replay because we just disc its sco link */
        app_bt_audio_create_sco_for_another_call_active_device(device_id);
    }
    return;

continue_wait:
    app_bt_audio_wait_this_sco_become_connected(device_id, false);
    return;
}

typedef enum {
    APP_BT_AUDIO_A2DP_RECHECK_CONTEXT_NULL,
    APP_BT_AUDIO_A2DP_WAIT_PHONE_AUTO_START_STREAM = 1,
    APP_BT_AUDIO_A2DP_WAIT_PAUSED_STREAM_SUSPEND,
} app_bt_audio_a2dp_recheck_enum_t;

#define APP_BT_AUDIO_A2DP_WAIT_PHONE_AUTO_START_STREAM_MS (3000)
#define APP_BT_AUDIO_A2DP_WAIT_AVRCP_PAUSED_STREAM_SUSPEND_MS (10000)
static uint8_t app_bt_audio_select_a2dp_stream_to_play(uint8_t device_id, uint8_t ctx);

static void app_bt_delay_reconnect_a2dp_timeout_handler(void const *param)
{
    BT_DEVICE_T* curr_device = NULL;
    for(uint8_t i = 0; i<BT_DEVICE_NUM; i++)
    {
        curr_device = app_bt_get_device(i);
        if(curr_device->this_is_delay_reconnect_a2dp)
        {
            curr_device->this_is_delay_reconnect_a2dp = false;
            app_bt_audio_a2dp_resume_this_profile(i);
            TRACE(0,"this is delay reconnect a2dp");
        }
    }
}

static void app_bt_audio_a2dp_streaming_recheck_timer_handler(void const *param)
{
    struct BT_DEVICE_T *recheck_device = (struct BT_DEVICE_T *)param;
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    struct BT_DEVICE_T *curr_playing_device = app_bt_get_device(curr_playing_a2dp);
    uint8_t reselected_a2dp = BT_DEVICE_INVALID_ID;

    if (app_bt_manager.a2dp_stream_play_recheck)
    {
        app_bt_manager.a2dp_stream_play_recheck = false;

        reselected_a2dp = app_bt_audio_select_streaming_a2dp();
    }
    else if (recheck_device->a2dp_stream_recheck_context == APP_BT_AUDIO_A2DP_WAIT_PAUSED_STREAM_SUSPEND)
    {
        recheck_device->a2dp_stream_recheck_context = APP_BT_AUDIO_A2DP_RECHECK_CONTEXT_NULL;

        if (curr_playing_a2dp != BT_DEVICE_INVALID_ID)
        {
            return;
        }

        if (recheck_device->a2dp_streamming)
        {
            reselected_a2dp = recheck_device->device_id;
            app_bt_get_device(reselected_a2dp)->a2dp_audio_prio = app_bt_audio_create_new_prio();
        }
    }
    else if (recheck_device->a2dp_stream_recheck_context == APP_BT_AUDIO_A2DP_WAIT_PHONE_AUTO_START_STREAM)
    {
        recheck_device->a2dp_stream_recheck_context = APP_BT_AUDIO_A2DP_RECHECK_CONTEXT_NULL;

        if (curr_playing_a2dp != BT_DEVICE_INVALID_ID)
        {
            if (curr_playing_device->a2dp_streamming)
            {
                return;
            }

            app_bt_audio_stop_a2dp_playing(curr_playing_a2dp);
            curr_playing_a2dp = BT_DEVICE_INVALID_ID;
        }

        if (curr_playing_a2dp == BT_DEVICE_INVALID_ID)
        {
            reselected_a2dp = app_bt_audio_select_streaming_a2dp();
        }
    }

    TRACE(4, "(d%x) a2dp streaming recheck: curr_playing_a2dp %x strming %d reselect %x", recheck_device->device_id,
        curr_playing_a2dp, curr_playing_device ? curr_playing_device->a2dp_streamming : 0, reselected_a2dp);

    if (reselected_a2dp != BT_DEVICE_INVALID_ID)
    {
        app_bt_audio_select_a2dp_stream_to_play(reselected_a2dp, APP_BT_AUDIO_EVENT_A2DP_STREAM_RESELECT);
    }
}

void app_bt_audio_a2dp_stream_recheck_timer_callback(void const *param)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)param, 0, (uint32_t)(uintptr_t)app_bt_audio_a2dp_streaming_recheck_timer_handler);
}

void app_bt_reconnect_a2dp_after_role_switch_callback(void const *param)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)param, 0, (uint32_t)(uintptr_t)app_bt_delay_reconnect_a2dp_timeout_handler);
}


void app_bt_audio_recheck_a2dp_streaming(void)
{
    app_bt_manager.a2dp_stream_play_recheck = true;
    app_bt_audio_a2dp_stream_recheck_timer_callback((void *)app_bt_get_device(BT_DEVICE_ID_1));
}

static void app_bt_audio_start_a2dp_recheck(uint8_t device_id, uint32_t delay_ms)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (curr_device && curr_device->a2dp_stream_recheck_timer)
    {
        osTimerStop(curr_device->a2dp_stream_recheck_timer);

        osTimerStart(curr_device->a2dp_stream_recheck_timer, delay_ms);
    }
}

static void app_bt_audio_a2dp_streaming_recheck_timer_start(uint8_t device_id, app_bt_audio_a2dp_recheck_enum_t ctx)
{
    uint32_t wait_ms = 0;

    if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && app_bt_audio_count_connected_sco())
    {
        // dont try to recheck a2dp when sco exist in case a2dp auto disc under mode
        // of keep_only_one_stream_close_connected_a2dp
        return;
    }

    if (ctx == APP_BT_AUDIO_A2DP_WAIT_PAUSED_STREAM_SUSPEND)
    {
        wait_ms = APP_BT_AUDIO_A2DP_WAIT_AVRCP_PAUSED_STREAM_SUSPEND_MS;
    }
    else
    {
        wait_ms = APP_BT_AUDIO_A2DP_WAIT_PHONE_AUTO_START_STREAM_MS;
    }

    app_bt_get_device(device_id)->a2dp_stream_recheck_context = ctx;
    app_bt_audio_start_a2dp_recheck(device_id, wait_ms);
}

#if defined(IBRT_V2_MULTIPOINT)
void app_bt_ibrt_audio_play_a2dp_stream(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", device_id, __func__, ibrt_role);
        if (IBRT_MASTER == ibrt_role)
        {
            // only ibrt master need to trigger action
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PLAY);
        }
    }
    else
    {
        app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PLAY);
    }
}

void app_bt_ibrt_audio_pause_a2dp_stream(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", device_id, __func__, ibrt_role);
        if (IBRT_MASTER == ibrt_role)
        {
            // only ibrt master need to trigger action
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PAUSE);
        }
    }
    else
    {
        app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PAUSE);
    }
}
#endif

void app_bt_audio_set_bg_a2dp_device(bool is_paused_bg_device, bt_bdaddr_t *bdaddr)
{
    uint8_t device_id = app_bt_get_device_id_byaddr(bdaddr);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    TRACE(3, "%s %d d%x", __func__, is_paused_bg_device, device_id);

    if (curr_device)
    {
        if (is_paused_bg_device)
        {
            curr_device->this_is_paused_bg_a2dp = true;
            curr_device->pause_a2dp_resume_prio = app_bt_audio_create_new_prio();
        }
        else
        {
            curr_device->this_is_closed_bg_a2dp = true;
            curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
        }
    }
}

uint8_t app_bt_audio_select_bg_a2dp_to_resume(void)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    uint32_t resume_prio = 0;
    int i = 0;

    for (i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);

        if (curr_device->this_is_closed_bg_a2dp && curr_device->close_a2dp_resume_prio > resume_prio)
        {
            resume_prio = curr_device->close_a2dp_resume_prio;
            device_id = i;
        }
    }

    TRACE(2,"app_bt_audio_select_bg_a2dp_to_resume,device_id=%d,resume_prio=%d",device_id,resume_prio);
    return device_id;
}

#if BT_DEVICE_NUM > 1
static void app_bt_audio_remove_other_bg_a2dp(uint8_t device_id, bool is_paused)
{
    struct BT_DEVICE_T* curr_device = NULL;
    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (device_id == i)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (is_paused)
        {
            curr_device->this_is_paused_bg_a2dp = false;            
            curr_device->pause_a2dp_resume_prio = 0;
        }
        else
        {
            curr_device->this_is_closed_bg_a2dp = false;
            curr_device->close_a2dp_resume_prio = 0;
        }
    }
}
#endif

typedef enum {
    APP_KILL_OLD_A2DP = 0x01,
    APP_OLD_SCO_A2DP_PLAYING_KILL_NEW_A2DP = 0x10,
    APP_OLD_SCO_PLAYING_KILL_NEW_A2DP,
    APP_OLD_A2DP_PLAYING_KILL_NEW_A2DP,
    APP_NO_A2DP_PLAYING_KILL_CONNECTED_A2DP,
    APP_OLD_A2DP_PLAYING_NEW_SCO_COME_KILL_OLD_A2DP,
    APP_NO_A2DP_PLAYING_NEW_SCO_COME_KILL_CONNECTED_A2DP,
} app_bt_audio_a2dp_behavior_ctx_enum;

void app_bt_audio_a2dp_close_this_profile(uint8_t device_id)
{
#if BT_DEVICE_NUM > 1
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    curr_device->this_is_closed_bg_a2dp = true;
    curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
    app_bt_audio_remove_other_bg_a2dp(device_id, false);
#if defined(IBRT_V2_MULTIPOINT)
    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        if (ibrt_role != IBRT_MASTER)
        {
            TRACE(2, "(d%x) %s not master", device_id, __func__);
            curr_device->this_is_closed_bg_a2dp = false; // ibrt slave only set to true when master disc profile request received
            return;
        }
        if (!app_ibrt_conn_is_profile_exchanged(&curr_device->remote))
        {
            TRACE(2, "(d%x) %s profile not exchanged", device_id, __func__);
            curr_device->this_is_closed_bg_a2dp = false;
            return;
        }
    }
    app_ibrt_if_master_disconnect_a2dp_profile(device_id);
#else
    app_bt_disconnect_a2dp_profile(curr_device->a2dp_connected_stream);
#endif
#endif
}

bool app_bt_audio_a2dp_disconnect_self_check(uint8_t device_id)
{
#if BT_DEVICE_NUM > 1
    if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
    {
        struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
        uint8_t another_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
        uint8_t another_playing_sco = app_bt_audio_get_curr_playing_sco();

        if (another_playing_sco != BT_DEVICE_INVALID_ID && device_id != another_playing_sco
#if defined(IBRT_V2_MULTIPOINT)
            && app_ibrt_conn_is_profile_exchanged(&app_bt_get_device(another_playing_sco)->remote)
#endif
            )
        {
            // another profile exchanged sco device is playing, dont allow current a2dp connect req
            curr_device->this_is_closed_bg_a2dp = true;
            curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
            TRACE(1, "(d%x) another sco device playing, not allow a2dp connect req", device_id);
            return true;
        }

        if (another_playing_a2dp != BT_DEVICE_INVALID_ID && device_id != another_playing_a2dp
#if defined(IBRT_V2_MULTIPOINT)
            && app_ibrt_conn_is_profile_exchanged(&app_bt_get_device(another_playing_a2dp)->remote)
#endif
            )
        {
            // another profile exchanged a2dp device is playing, dont allow current a2dp connect req
            curr_device->this_is_closed_bg_a2dp = true;
            curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
            TRACE(1, "(d%x) another a2dp device playing, not allow a2dp connect req", device_id);
            return true;
        }
    }
#endif

    return false;
}

void app_bt_audio_perform_a2dp_behavior(uint8_t device_id, app_bt_audio_a2dp_behavior_ctx_enum ctx)
{
#if BT_DEVICE_NUM > 1
    app_bt_audio_a2dp_close_this_profile(device_id);
#endif
}

uint8_t app_bt_audio_reselect_resume_a2dp_stream(uint8_t trigger_resume_device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == trigger_resume_device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_conn_flag && !curr_device->a2dp_conn_flag)
        {
            device_id = i;
            break;
        }
    }

    if (device_id != BT_DEVICE_INVALID_ID)
    {
        app_bt_get_device(device_id)->this_is_closed_bg_a2dp = true;
    }

    TRACE(3, "%s d%x d%x", __func__, trigger_resume_device_id, device_id);

    return device_id;
}

void app_bt_audio_a2dp_resume_this_profile(uint8_t device_id)
{
#if BT_DEVICE_NUM > 1
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    curr_device->this_is_closed_bg_a2dp = false;
    curr_device->close_a2dp_resume_prio = 0;
    curr_device->auto_make_remote_play = false;
#if defined(IBRT_V2_MULTIPOINT)
    app_ibrt_if_master_connect_a2dp_profile(device_id);
#else
    app_bt_reconnect_a2dp_profile(&curr_device->remote);
#endif
#endif
}

#define APP_BT_A2DP_DELAY_RECONNECT_MS (500)

static uint8_t app_bt_audio_resume_bg_a2dp_stream(uint8_t trigger_resume_device_id, uint8_t ctx)
{
    uint8_t device_id = BT_DEVICE_INVALID_ID;

#if BT_DEVICE_NUM > 1
    struct BT_DEVICE_T *curr_device = NULL;

    device_id = app_bt_audio_select_bg_a2dp_to_resume();
    TRACE(3,"%s:device_id=%d trigger_id=%d",__func__,device_id,trigger_resume_device_id);
    if (device_id == BT_DEVICE_INVALID_ID)
    {
        device_id = app_bt_audio_reselect_resume_a2dp_stream(trigger_resume_device_id);
        if (device_id == BT_DEVICE_INVALID_ID)
        {
            return BT_DEVICE_INVALID_ID;
        }
    }

    if ((ctx == APP_BT_AUDIO_EVENT_AVRCP_MEDIA_PAUSED || ctx == APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND) && (device_id == trigger_resume_device_id))
    {
        return BT_DEVICE_INVALID_ID; // dont resume paused or suspended device itself triggered
    }

    curr_device = app_bt_get_device(device_id);
    TRACE(5,"resume_bg_a2dp_stream:device_id=%d paused=%d closed=%d acl=%d a2dp_conn=%d",device_id,
        curr_device->this_is_paused_bg_a2dp, curr_device->this_is_closed_bg_a2dp,
        curr_device->acl_is_connected, curr_device->a2dp_conn_flag);

    if (curr_device->this_is_closed_bg_a2dp)
    {
        curr_device->this_is_closed_bg_a2dp = false;
        curr_device->close_a2dp_resume_prio = 0;

        if (!curr_device->acl_is_connected || curr_device->a2dp_conn_flag)
        {
            TRACE(4, "%s d%x acl %d a2dp %d", __func__, device_id, curr_device->acl_is_connected, curr_device->a2dp_conn_flag);

            /**
             * the bg a2dp may be reconnected successfully by the phone itself before the earbud to resume.
             * here we just need to make remote resume to paly the music.
             */

            if (curr_device->a2dp_conn_flag)
            {
                curr_device->auto_make_remote_play = false;
                if(curr_device->a2dp_disc_on_process)
                {
                    curr_device->this_is_closed_bg_a2dp = true;
                    curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
                    TRACE(0,"waiting for close a2dp and resume pre closed profile!!!");
                }

#if 0 // dont auto make remote play
#if defined(IBRT_V2_MULTIPOINT)
                app_bt_ibrt_audio_play_a2dp_stream(device_id);
#else
                app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PLAY);
#endif
#endif
            }
        }
#if defined(IBRT_V2_MULTIPOINT)        
        else if(app_ibrt_conn_any_role_switch_running() == true)
        {
            curr_device->this_is_delay_reconnect_a2dp = true;
            osTimerStop(curr_device->a2dp_delay_reconnect_timer);
            osTimerStart(curr_device->a2dp_delay_reconnect_timer,APP_BT_A2DP_DELAY_RECONNECT_MS);
            TRACE(0,"app_ibrt_conn_any_role_switch_running delay to reconnect");
        }
#endif
        else
        {
            app_bt_audio_a2dp_resume_this_profile(device_id);
        }
    }
#endif

    return device_id;
}

void app_bt_audio_update_local_a2dp_playing_device(uint8_t device_id)
{
#if defined(IBRT_V2_MULTIPOINT)
    if (app_bt_audio_count_connected_a2dp() > 1)
    {
        app_ibrt_send_ext_cmd_a2dp_playing_device(device_id, false);
    }
#endif  
}

void app_bt_audio_receive_peer_a2dp_playing_device(bool is_response, uint8_t device_id)
{
#if defined(IBRT_V2_MULTIPOINT)
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    if (!is_response)
    {
        if (app_tws_ibrt_is_profile_exchanged(&curr_device->remote) && curr_playing_a2dp != BT_DEVICE_INVALID_ID && curr_device->a2dp_streamming)
        {
            if (curr_playing_a2dp != device_id)
            {
                app_bt_audio_stop_a2dp_playing(curr_playing_a2dp);
                app_bt_audio_start_a2dp_playing(device_id);
                app_ibrt_send_ext_cmd_a2dp_playing_device(device_id, true);
            }
        }
    }
    else
    {
        if (app_bt_audio_count_connected_a2dp() > 1 && curr_playing_a2dp != BT_DEVICE_INVALID_ID && curr_playing_a2dp != device_id)
        {
            app_ibrt_send_ext_cmd_a2dp_playing_device(curr_playing_a2dp, false);
        }
    }
#endif
}

static bool app_bt_audio_this_paused_a2dp_has_callsetup(uint8_t device_id, uint8_t ctx)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (ctx == APP_BT_AUDIO_EVENT_AVRCP_MEDIA_PAUSED || ctx == APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND)
    {
        if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && curr_device->hfchan_callSetup != BTIF_HF_CALL_SETUP_NONE)
        {
            return true;
        }
    }

    return false;
}

static uint8_t app_bt_audio_select_a2dp_stream_to_play(uint8_t device_id, uint8_t ctx)
{
    uint8_t select_a2dp_stream = app_bt_audio_select_streaming_a2dp();
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    uint8_t curr_playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t resume_a2dp_id = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct BT_DEVICE_T *interrupted_a2dp_device = NULL;
    struct BT_DEVICE_T *selected_a2dp_device = NULL;

    TRACE(6,"%s %x playing_a2dp %x playing_sco %x int_a2dp %x ctx %d",
            __func__, select_a2dp_stream, curr_playing_a2dp,
            curr_playing_sco, app_bt_manager.interrupted_a2dp_id, ctx);

    if (select_a2dp_stream == BT_DEVICE_INVALID_ID)
    {
        // current no streaming a2dp
        app_bt_audio_enable_active_link(false, BT_DEVICE_INVALID_ID);
        app_bt_manager.curr_playing_a2dp_id = BT_DEVICE_INVALID_ID;
        if(app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && curr_device->avrcp_palyback_status == BTIF_AVRCP_MEDIA_PLAYING)
        {
            TRACE(1,"(%dx)don't resume other a2dp when current device in playing status",device_id);
            return 0;
        }
        /**
         * play current connected interrupted a2dp first, for example:
         * 1. d0 a2dp is in bg
         * 2. d1 interrupt d1's a2dp and playing sco
         * 3. d1 sco disconnected, then it shall choice d1's a2dp to play
         */
        if (ctx == APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED && app_bt_manager.interrupted_a2dp_id != BT_DEVICE_INVALID_ID &&
            device_id == app_bt_manager.interrupted_a2dp_id && curr_device->a2dp_conn_flag)
        {
            goto play_interrupted_a2dp_first;
        }

        // noly resume bg a2dp when no sco connected, and this paused a2dp has no callsetup activity
        TRACE(3, "%s:sco_count=%d d%x has callsetup %d", __func__, app_bt_audio_count_connected_sco(), device_id, app_bt_audio_this_paused_a2dp_has_callsetup(device_id, ctx));
        /**
         * app_bt_audio_this_paused_a2dp_has_callsetup:
         * 1. d0 a2dp profile disconnected
         * 2. d1 is playing a2dp
         * 3. d1 incoming call, callsetup status reported
         * 4. d1 a2dp suspend and avrcp paused
         * 5. d1 sco connected and start to play sco
         * at step 4, we shall not resume d0's a2dp profile
         */
        if (!app_bt_audio_count_connected_sco() && !app_bt_audio_this_paused_a2dp_has_callsetup(device_id, ctx))
        {
            resume_a2dp_id = app_bt_audio_resume_bg_a2dp_stream(device_id, ctx);
            if (resume_a2dp_id == app_bt_manager.interrupted_a2dp_id)
            {
                app_bt_manager.interrupted_a2dp_id = BT_DEVICE_INVALID_ID;
            }
            if(resume_a2dp_id == BT_DEVICE_SEND_AVRCP_PLAY)
            {
                return resume_a2dp_id;
            }
        }

        return 0;
    }

    // interrupted a2dp has higher priority than other streaming a2dp

play_interrupted_a2dp_first:
    if (ctx == APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED && app_bt_manager.interrupted_a2dp_id != BT_DEVICE_INVALID_ID)
    {
        interrupted_a2dp_device = app_bt_get_device(app_bt_manager.interrupted_a2dp_id);
        TRACE(2,"%s int_a2dp streamming state %d", __func__, interrupted_a2dp_device->a2dp_streamming);
        if (interrupted_a2dp_device->a2dp_streamming)
        {
            select_a2dp_stream = app_bt_manager.interrupted_a2dp_id;
        }
        else if (app_bt_manager.interrupted_a2dp_id == device_id)
        {
            TRACE(1, "wait phone %x auto start interrupted a2dp stream", device_id);
            select_a2dp_stream = app_bt_manager.interrupted_a2dp_id;
            app_bt_audio_a2dp_streaming_recheck_timer_start(device_id, APP_BT_AUDIO_A2DP_WAIT_PHONE_AUTO_START_STREAM);
        }
        app_bt_manager.interrupted_a2dp_id = BT_DEVICE_INVALID_ID;
    }

    selected_a2dp_device = app_bt_get_device(select_a2dp_stream);

#if defined(IBRT_V2_MULTIPOINT)
    if (btif_me_get_activeCons() > 2 && tws_besaud_is_connected() && app_bt_audio_count_streaming_a2dp() > 1 &&
        !app_tws_ibrt_is_profile_exchanged(&selected_a2dp_device->remote) && ctx != APP_BT_AUDIO_EVENT_A2DP_STREAM_MOCK_START)
    {
        uint8_t another_streaming_a2dp = app_bt_audio_select_another_streaming_a2dp(select_a2dp_stream);
        struct BT_DEVICE_T *another_streaming_device = app_bt_get_device(another_streaming_a2dp);
        if (another_streaming_device && app_tws_ibrt_is_profile_exchanged(&another_streaming_device->remote))
        {
            TRACE(2, "only 1/2 streaming a2dp profile exchanged, change d%x -> d%x", select_a2dp_stream, another_streaming_a2dp);
            select_a2dp_stream = another_streaming_a2dp;
            selected_a2dp_device = app_bt_get_device(another_streaming_a2dp);
        }
    }
#endif

    if (select_a2dp_stream == curr_playing_a2dp)
    {
        // current streaming a2dp is playing

        app_bt_audio_start_a2dp_playing(select_a2dp_stream);

        if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
        {
            uint8_t another_conn_a2dp = app_bt_audio_select_another_connected_a2dp(select_a2dp_stream);
            if (another_conn_a2dp != BT_DEVICE_INVALID_ID && false == app_bt_get_device(another_conn_a2dp)->this_is_closed_bg_a2dp)
            {
                app_bt_audio_rechceck_kill_another_connected_a2dp(select_a2dp_stream);
            }
        }

        return 0;
    }

    if (curr_playing_sco != BT_DEVICE_INVALID_ID) // sco is playing
    {
        if (curr_playing_a2dp != BT_DEVICE_INVALID_ID) // a2dp also playing
        {
            app_bt_audio_update_local_a2dp_playing_device(curr_playing_a2dp);
            app_bt_audio_start_a2dp_playing(curr_playing_a2dp);
            app_bt_audio_perform_a2dp_behavior(select_a2dp_stream, APP_OLD_SCO_A2DP_PLAYING_KILL_NEW_A2DP);
        }
        else
        {
            app_bt_audio_perform_a2dp_behavior(select_a2dp_stream, APP_OLD_SCO_PLAYING_KILL_NEW_A2DP);
        }
    }
    else
    {
        if (curr_playing_a2dp != BT_DEVICE_INVALID_ID) // a2dp is playing
        {
            app_bt_audio_update_local_a2dp_playing_device(curr_playing_a2dp);
            app_bt_audio_start_a2dp_playing(curr_playing_a2dp);
            //app_bt_audio_perform_a2dp_behavior(select_a2dp_stream, APP_OLD_A2DP_PLAYING_KILL_NEW_A2DP);
        }
        else
        {
            app_bt_audio_update_local_a2dp_playing_device(select_a2dp_stream);
            app_bt_audio_start_a2dp_playing(select_a2dp_stream);
            app_bt_audio_rechceck_kill_another_connected_a2dp(select_a2dp_stream);
        }
    }
    return 0;
}


void app_bt_audio_rechceck_kill_another_connected_a2dp(uint8_t device_id)
{
    uint8_t another_conn_a2dp = BT_DEVICE_INVALID_ID; 
    if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
    {
        another_conn_a2dp = app_bt_audio_select_another_connected_a2dp(device_id);
        if (another_conn_a2dp != BT_DEVICE_INVALID_ID)
        {
            app_bt_audio_perform_a2dp_behavior(another_conn_a2dp, APP_NO_A2DP_PLAYING_KILL_CONNECTED_A2DP);
            TRACE(2,"%s close d%x a2dp profile", __func__, another_conn_a2dp);
        }
   }
}

#if defined(IBRT_V2_MULTIPOINT)
static void app_bt_audio_a2dp_check_after_profile_exchanged(uint8_t device_id)
{
    bool has_another_profile_exchanged_streaming_a2dp = false;
    uint8_t another_streaming_a2dp = app_bt_audio_select_another_streaming_a2dp(device_id);
    struct BT_DEVICE_T *another_streaming_device = app_bt_get_device(another_streaming_a2dp);

    if (another_streaming_device && app_tws_ibrt_is_profile_exchanged(&another_streaming_device->remote))
    {
        has_another_profile_exchanged_streaming_a2dp = true;
        app_bt_audio_select_a2dp_stream_to_play(device_id, APP_BT_AUDIO_EVENT_PROFILE_EXCHANGED);
    }

    if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
    {
        uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();

        TRACE(4, "(d%x) profile exchange a2dp check: curr_playing_a2dp d%x another_streaming_a2dp d%x %d",
                device_id, curr_playing_a2dp, another_streaming_a2dp, has_another_profile_exchanged_streaming_a2dp);

        if (has_another_profile_exchanged_streaming_a2dp && curr_playing_a2dp != device_id)
        {
            app_bt_audio_a2dp_close_this_profile(device_id);
        }
    }
}
#endif

void app_bt_audio_switch_streaming_sco(void)
{
    uint8_t curr_playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t sco_count = app_bt_audio_count_connected_sco();
    uint8_t active_call_device = BT_DEVICE_INVALID_ID;

    if (curr_playing_sco != BT_DEVICE_INVALID_ID)
    {
        if (sco_count > 1)
        {
            app_bt_audio_route_sco_path_to_phone(curr_playing_sco); // after this sco disc, it will auto play other straming sco
            return;
        }

        active_call_device = app_bt_audio_select_another_device_to_create_sco(curr_playing_sco);

        if (active_call_device == BT_DEVICE_INVALID_ID)
        {
            TRACE(1,"%s no another active call", __func__);
            return;
        }

        app_bt_audio_route_sco_path_to_phone(curr_playing_sco); // after this sco disc, it will auto create sco for call active device
    }
    else
    {
        uint8_t curr_active_call = app_bt_audio_select_call_active_hfp();

        if (curr_active_call == BT_DEVICE_INVALID_ID)
        {
            return;
        }

        // consider curr_active_call is current sco, route other call active sco path back
        active_call_device = app_bt_audio_select_another_device_to_create_sco(curr_active_call);
        if (active_call_device != BT_DEVICE_INVALID_ID) 
        {
            app_bt_audio_route_sco_path_back_to_earbud(active_call_device, true);
        }
    }
}

bool app_bt_audio_switch_streaming_a2dp(void)
{
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    uint8_t selected_streaming_a2dp = BT_DEVICE_INVALID_ID;

    if (curr_playing_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no playing a2dp", __func__);
        return false;
    }

    selected_streaming_a2dp = app_bt_audio_select_another_streaming_a2dp(curr_playing_a2dp);
    if (selected_streaming_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no other streaming a2dp", __func__);
        return false;
    }

    app_bt_get_device(selected_streaming_a2dp)->a2dp_audio_prio = app_bt_audio_create_new_prio();

    app_bt_audio_stop_a2dp_playing(curr_playing_a2dp);

    app_bt_audio_select_a2dp_stream_to_play(selected_streaming_a2dp, APP_BT_AUDIO_EVENT_A2DP_STREAM_SWITCH);

    return true;
}

#define A2DP_SWITCH_DELAY_TICKS (20*32) // 200ms - 32 * 312.5 is 10ms

static void app_bt_audio_a2dp_switch_trigger(void)
{
    struct BT_DEVICE_T *curr_device = NULL;
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    uint8_t selected_streaming_a2dp = BT_DEVICE_INVALID_ID;
    uint32_t curr_btclk = 0;
    bool time_reached = false;

    if (!app_bt_manager.trigger_a2dp_switch)
    {
        return;
    }

    if (curr_playing_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no playing a2dp", __func__);
        app_bt_manager.trigger_a2dp_switch = false;
        return;
    }

    selected_streaming_a2dp = app_bt_audio_select_another_streaming_a2dp(curr_playing_a2dp);
    if (selected_streaming_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no other streaming a2dp", __func__);
        app_bt_manager.trigger_a2dp_switch = false;
        return ;
    }

    if (curr_playing_a2dp != app_bt_manager.a2dp_switch_trigger_device)
    {
        TRACE(3,"%s trigger device not match %x %x", __func__, curr_playing_a2dp, app_bt_manager.a2dp_switch_trigger_device);
        app_bt_manager.trigger_a2dp_switch = false;
        return;
    }

    curr_device = app_bt_get_device(curr_playing_a2dp);
    curr_btclk = bt_syn_get_curr_ticks(curr_device->acl_conn_hdl);

    TRACE(5,"%s playing_a2dp %x selected_a2dp %x tgtclk %x curclk %x", __func__,
            curr_playing_a2dp, selected_streaming_a2dp, app_bt_manager.a2dp_switch_trigger_btclk, curr_btclk);

    if (curr_btclk < app_bt_manager.a2dp_switch_trigger_btclk)
    {
        if (app_bt_manager.a2dp_switch_trigger_btclk - curr_btclk > A2DP_SWITCH_DELAY_TICKS * 2)
        {
            TRACE(3,"%s btclk may wrapped %d %d", __func__, app_bt_manager.a2dp_switch_trigger_btclk, curr_btclk);
            time_reached = true; // bt clock may wrapped
        }
        else
        {
            time_reached = false;
        }
    }
    else
    {
        time_reached = true;
    }

    if (time_reached)
    {
        app_bt_manager.trigger_a2dp_switch = false;

        app_bt_audio_switch_streaming_a2dp();
    }
}

uint32_t app_bt_audio_trigger_switch_streaming_a2dp(uint32_t btclk)
{
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    uint8_t selected_streaming_a2dp = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T *curr_device = NULL;
    uint32_t curr_btclk = 0;
    uint32_t trigger_btclk = 0;

    if (app_bt_manager.trigger_a2dp_switch)
    {
        TRACE(2,"%s already has a a2dp switch trigger %x", __func__, app_bt_manager.a2dp_switch_trigger_device);
        return 0;
    }

    if (curr_playing_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no playing a2dp", __func__);
        return 0;
    }

    selected_streaming_a2dp = app_bt_audio_select_another_streaming_a2dp(curr_playing_a2dp);
    if (selected_streaming_a2dp == BT_DEVICE_INVALID_ID)
    {
        TRACE(1,"%s no other streaming a2dp", __func__);
        return 0;
    }

    curr_device = app_bt_get_device(curr_playing_a2dp);
    if (btclk == 0)
    {
        curr_btclk = bt_syn_get_curr_ticks(curr_device->acl_conn_hdl);
        trigger_btclk = curr_btclk + A2DP_SWITCH_DELAY_TICKS;
    }
    else
    {
        trigger_btclk = btclk;
    }

    app_bt_manager.trigger_a2dp_switch = true;
    app_bt_manager.a2dp_switch_trigger_btclk = trigger_btclk;
    app_bt_manager.a2dp_switch_trigger_device = curr_playing_a2dp;

    app_bt_audio_a2dp_switch_trigger();

    return trigger_btclk;
}

static void app_bt_hf_set_curr_stream(uint8_t device_id)
{
    if (device_id == BT_DEVICE_AUTO_CHOICE_ID)
    {
        /** hfp channel priority:
         *      1. sco playing device
         *      2. sco connected device
         *      3. hfp connected device
         *      4. BT_DEVICE_ID_1
         */
        device_id = app_bt_audio_get_curr_playing_sco();
        if (device_id == BT_DEVICE_INVALID_ID)
        {
            device_id = app_bt_audio_select_streaming_sco();
            if (device_id == BT_DEVICE_INVALID_ID)
            {
                device_id = app_bt_audio_select_connected_hfp();
                if (device_id == BT_DEVICE_INVALID_ID)
                {
                    device_id = BT_DEVICE_ID_1;
                }
            }
        }
    }

    TRACE(2, "%s as %x", __func__, device_id);

    if (device_id == BT_DEVICE_INVALID_ID)
    {
        return;
    }

    app_bt_manager.curr_hf_channel_id = (enum BT_DEVICE_ID_T)device_id;
}

extern uint8_t bt_media_current_sbc_get(void);
extern uint8_t bt_media_current_sco_get(void);

uint8_t app_bt_audio_get_curr_a2dp_device(void)
{
    uint8_t device_id = bt_media_current_sbc_get();
    return (device_id != BT_DEVICE_INVALID_ID ? device_id : app_bt_manager.curr_a2dp_stream_id);
}

uint8_t app_bt_audio_get_curr_hfp_device(void)
{
    uint8_t device_id = app_bt_audio_get_curr_playing_sco();
    return (device_id != BT_DEVICE_INVALID_ID ? device_id : app_bt_manager.curr_hf_channel_id);
}

uint8_t app_bt_audio_get_curr_sco_device(void)
{
    uint8_t device_id = bt_media_current_sco_get();
    return (device_id != BT_DEVICE_INVALID_ID ? device_id : app_bt_manager.curr_hf_channel_id);
}

static uint8_t app_bt_audio_select_hfp_call_activity_device(void)
{
    /** device priority for user action
     *      1. device has call in setup flow    ---> answer, reject, hold
     *      2. current sco playing device       ---> hungup, hold, sco path switch
     *      3. other sco connected device       ---> hungup, hold, sco path switch
     *      4. device has active/held call      ---> hungup, hold/active switch
     */
    uint8_t device_id = app_bt_audio_select_call_setup_hfp();
    if (device_id == BT_DEVICE_INVALID_ID)
    {
        device_id = app_bt_audio_get_curr_playing_sco();
        if (device_id == BT_DEVICE_INVALID_ID)
        {
            device_id = app_bt_audio_select_streaming_sco();
            if (device_id == BT_DEVICE_INVALID_ID)
            {
                device_id = app_bt_audio_select_call_active_hfp();
            }
        }
    }
    return device_id;
}

uint8_t app_bt_audio_get_hfp_device_for_user_action(void)
{
    uint8_t device_id = app_bt_audio_select_hfp_call_activity_device();
    if (device_id == BT_DEVICE_INVALID_ID)
    {
        device_id = app_bt_audio_select_connected_hfp();
        if (device_id == BT_DEVICE_INVALID_ID)
        {
            device_id = BT_DEVICE_ID_1;
        }
    }
    return device_id;
}

uint8_t app_bt_audio_get_device_for_user_action(void)
{
    uint8_t device_id = app_bt_audio_select_hfp_call_activity_device();
    if (device_id == BT_DEVICE_INVALID_ID)
    {
        device_id = app_bt_audio_get_curr_a2dp_device();
    }
    return device_id;
}

uint8_t app_bt_audio_get_another_hfp_device_for_user_action(uint8_t curr_device_id)
{
    uint8_t device_id = BT_DEVICE_INVALID_ID;

    device_id = app_bt_audio_select_another_call_setup_hfp(curr_device_id);
    if (device_id != BT_DEVICE_INVALID_ID)
        return device_id;

    device_id = app_bt_audio_get_curr_playing_sco();
    if (device_id != BT_DEVICE_INVALID_ID && device_id != curr_device_id)
        return device_id;

    device_id = app_bt_audio_select_another_streaming_sco(curr_device_id);
    if (device_id != BT_DEVICE_INVALID_ID)
        return device_id;

    device_id = app_bt_audio_select_another_call_active_hfp(curr_device_id);
    if (device_id != BT_DEVICE_INVALID_ID)
        return device_id;

    return (curr_device_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
}


bool app_bt_audio_curr_a2dp_data_need_receive(uint8_t device_id)
{
    bool ret = false;

#if defined(BT_WATCH_APP)
    return ret;
#endif

    app_bt_audio_a2dp_switch_trigger();
    ret =
#if (A2DP_DECODER_VER < 2)
        app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) &&
#endif
        (device_id == app_bt_manager.curr_playing_a2dp_id) &&

        (BT_DEVICE_INVALID_ID == app_bt_manager.curr_playing_sco_id);

#if 0
    if (!ret)
    {
        TRACE(4, "device_id:%d|%d, sco_id:%d|%d",
              device_id, app_bt_manager.curr_playing_a2dp_id,
              BT_DEVICE_INVALID_ID, app_bt_manager.curr_playing_sco_id);
    }
#endif

    return ret;
}

static void app_bt_audio_start_sco_playing(uint8_t device_id)
{
    uint8_t other_call_active_device = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (besbt_cfg.vendor_codec_en && btif_hf_is_sco_wait_codec_sync(curr_device->hf_channel))
    {
        curr_device->this_sco_wait_to_play = true;
        return;
    }

    curr_device->this_sco_wait_to_play = false;
    
    app_bt_hf_set_curr_stream(device_id);

#ifndef AUDIO_PROMPT_USE_DAC2_ENABLED
    for (int i = 0; i < BT_DEVICE_NUM; i += 1) // stop media if any
    {
        if (bt_media_is_media_active_by_device(BT_STREAM_MEDIA, (enum BT_DEVICE_ID_T)i))
        {
            app_bt_audio_stop_media_playing(i);
        }
    }
#endif

    if (app_bt_manager.curr_playing_sco_id != device_id)
    {
        app_bt_manager.curr_playing_sco_id = device_id;

        other_call_active_device = app_bt_audio_select_another_call_active_hfp(device_id);
        if (other_call_active_device != BT_DEVICE_INVALID_ID && bt_media_is_media_active_by_device(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)other_call_active_device))
        {
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE, other_call_active_device, MAX_RECORD_NUM); // rescure for sco not stop succ corner case
        }

        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_VOICE, device_id, MAX_RECORD_NUM);
    }
}

void app_bt_audio_peer_sco_codec_received(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (besbt_cfg.vendor_codec_en && curr_device->this_sco_wait_to_play && curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
    {
        app_bt_audio_start_sco_playing(device_id);
    }

    curr_device->this_sco_wait_to_play = false;
}

static void app_bt_audio_swap_sco_playing(uint8_t device_id)
{
    app_bt_hf_set_curr_stream(device_id);

    app_bt_manager.curr_playing_sco_id = device_id;

    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWAP_SCO, BT_STREAM_VOICE, device_id, 0);
}

static void app_bt_audio_stop_sco_playing(uint8_t device_id, uint8_t ctx)
{
    if (app_bt_manager.curr_playing_sco_id == device_id)
    {
        app_bt_manager.curr_playing_sco_id = BT_DEVICE_INVALID_ID;
    }

    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE, device_id, MAX_RECORD_NUM);

    app_bt_hf_set_curr_stream(BT_DEVICE_AUTO_CHOICE_ID);
}

static uint8_t app_bt_audio_create_sco_for_another_call_active_device(uint8_t device_id)
{
    uint8_t device_with_active_call = BT_DEVICE_INVALID_ID;

    device_with_active_call = app_bt_audio_select_another_device_to_create_sco(device_id);

    if (device_with_active_call != BT_DEVICE_INVALID_ID)
    {
        if (app_bt_audio_route_sco_path_back_to_earbud(device_with_active_call, true))
        {
            TRACE(2, "(d%x) %s create sco for waiting active call", device_with_active_call, __func__);
            return device_with_active_call;
        }
    }

    return BT_DEVICE_INVALID_ID;
}

void app_bt_audio_select_sco_stream_to_play(uint8_t device_id, uint8_t ctx)
{
    uint8_t select_sco_stream = app_bt_audio_select_streaming_sco();
    uint8_t curr_playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    uint8_t another_device_creating_sco = BT_DEVICE_INVALID_ID;
    bool switch_back_sco_path = false;
    bool need_to_resume_self_a2dp = true;
    uint8_t select_a2dp_result = 0;

    TRACE(5,"%s %x playing_sco %x playing_a2dp %x routed_sco %x",
            __func__, select_sco_stream, curr_playing_sco, curr_playing_a2dp, app_bt_manager.device_routed_sco_to_phone);

    if (select_sco_stream == BT_DEVICE_INVALID_ID)
    {
        // no sco connected
        app_bt_manager.curr_playing_sco_id = BT_DEVICE_INVALID_ID;

        if (app_bt_manager.device_routed_sco_to_phone != BT_DEVICE_INVALID_ID && app_bt_manager.device_routed_sco_to_phone != device_id)
        {
            // need route sco path back to earbud
            switch_back_sco_path = app_bt_audio_route_sco_path_back_to_earbud(app_bt_manager.device_routed_sco_to_phone, true);
            app_bt_manager.device_routed_sco_to_phone = BT_DEVICE_INVALID_ID;
        }

        if (switch_back_sco_path)
        {
            TRACE(1, "%s switch back sco path to earbud", __func__);
            return;
        }

        // create sco if other devices have active call, the device_id's sco is disconnected at this point
        another_device_creating_sco = app_bt_audio_create_sco_for_another_call_active_device(device_id);
        if (another_device_creating_sco != BT_DEVICE_INVALID_ID)
        {
            return;
        }

        //reusme sniff req policy from remote dev
        bt_drv_reg_op_set_ibrt_reject_sniff_req(false);
        // no sco activities, select a2dp to play
        select_a2dp_result = app_bt_audio_select_a2dp_stream_to_play(device_id, APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED);
        if(select_a2dp_result == BT_DEVICE_SEND_AVRCP_PLAY)
        {
            need_to_resume_self_a2dp = false;
        }
        // resume current device's a2dp profile connection if needed
        if (need_to_resume_self_a2dp && ctx == APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED && app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && !(app_bt_get_device(device_id)->a2dp_conn_flag))
        {
            app_bt_audio_a2dp_resume_this_profile(device_id);
        }
        return;
    }

    //reject sniff req from remote dev
    bt_drv_reg_op_set_ibrt_reject_sniff_req(true);

    if (select_sco_stream == curr_playing_sco)
    {
        // current selected sco is playing
        app_bt_audio_start_sco_playing(select_sco_stream);
        return;
    }

    // there may be more than one sco exist at the same time
    if (curr_playing_sco != BT_DEVICE_INVALID_ID) // sco is playing
    {
        if (app_bt_manager.config.sco_prompt_play_mode)
        {
            // current selected sco has higher priority

            if (app_bt_manager.config.keep_only_one_sco_link)
            {
                // route current playing sco path to phone, and the sco will auto play after curr playing sco disconnected
                app_bt_audio_route_sco_path_to_phone(curr_playing_sco);
            }
            else
            {
                // reset a higher priority to current playing sco
                app_bt_get_device(curr_playing_sco)->sco_audio_prio = app_bt_audio_create_new_prio();

                // swap to play selected sco
                app_bt_audio_swap_sco_playing(select_sco_stream);
            }
        }
        else
        {
            // still play current playing sco
            app_bt_audio_start_sco_playing(curr_playing_sco);

            if (app_bt_manager.config.keep_only_one_sco_link)
            {
                // route current selected sco path to phone
                app_bt_audio_route_sco_path_to_phone(select_sco_stream);
            }
        }
    }
    else
    {
        if (curr_playing_a2dp != BT_DEVICE_INVALID_ID) // a2dp is playing
        {
            app_bt_audio_start_sco_playing(select_sco_stream);

            if (curr_playing_a2dp != select_sco_stream)
            {
                app_bt_audio_perform_a2dp_behavior(curr_playing_a2dp, APP_OLD_A2DP_PLAYING_NEW_SCO_COME_KILL_OLD_A2DP);
            }
        }
        else
        {
            app_bt_audio_start_sco_playing(select_sco_stream);
            if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
            {
                uint8_t another_conn_a2dp = BT_DEVICE_INVALID_ID;
                another_conn_a2dp = app_bt_audio_select_another_connected_a2dp(select_sco_stream);
                if (another_conn_a2dp != BT_DEVICE_INVALID_ID)
                {
                    app_bt_audio_perform_a2dp_behavior(another_conn_a2dp, APP_NO_A2DP_PLAYING_NEW_SCO_COME_KILL_CONNECTED_A2DP);
                }
            }
        }
    }
}

#define APP_BT_AUDIO_AVRCP_PLAY_STATUS_WAIT_TIMER_MS 500

static bool app_bt_audio_assume_play_status_notify_rsp_event_received(struct BT_DEVICE_T *curr_device, uint32_t curr_play_status, bool is_real_event, uint32_t *result_play_status)
{
    if (!is_real_event) // the timer is timeout before NOTIFY RSP event come
    {
        curr_device->avrcp_play_status_wait_to_handle = false;
        *result_play_status = curr_device->avrcp_palyback_status;
        return true;
    }
    else
    {
        // the NOTIFY RSP event come before timer timeout

        if (curr_device->avrcp_play_status_wait_to_handle)
        {
            curr_device->avrcp_play_status_wait_to_handle = false;

            // curr_device->avrcp_palyback_status is the previous play status

            if (curr_device->avrcp_palyback_status == curr_play_status)
            {
                *result_play_status = curr_play_status;
                return true;
            }

#if 0
            switch (curr_device->avrcp_palyback_status) 
            {
                case BTIF_AVRCP_MEDIA_STOPPED:
                    break;
                case BTIF_AVRCP_MEDIA_PLAYING:
                    break;
                case BTIF_AVRCP_MEDIA_PAUSED:
                    break;
                default:
                    break;
            }
#endif

            *result_play_status = curr_play_status;
            return true;
        }
        else
        {
            return false;
        }
    }
}

static void app_bt_audio_event_avrcp_play_status_changed(struct BT_DEVICE_T *curr_device, uint32_t play_status)
{
    TRACE(2, "(d%x) avrcp play status timer generated event %d", curr_device->device_id, play_status);
    app_bt_audio_event_handler(curr_device->device_id, APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS, play_status);
}

void app_bt_audio_avrcp_play_status_wait_timer_callback(void const *param)
{
    struct BT_DEVICE_T *curr_device = (struct BT_DEVICE_T *)param;
    uint32_t play_status = 0;
    if (app_bt_audio_assume_play_status_notify_rsp_event_received(curr_device, curr_device->avrcp_palyback_status, false, &play_status))
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)curr_device, play_status, (uint32_t)app_bt_audio_event_avrcp_play_status_changed);
    }
}

static bool app_bt_audio_event_judge_avrcp_play_status(uint8_t device_id, enum app_bt_audio_event_t event, uint32_t curr_play_status, uint32_t *result_play_status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    bool handle_play_status_event = false;

    if (event == APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_CHANGED)
    {
        curr_device->avrcp_play_status_wait_to_handle = false;

        if (curr_play_status == BTIF_AVRCP_MEDIA_PAUSED)
        {
            // this pause play status may be incorrectly sent by phone, wait and recheck it
            curr_device->avrcp_play_status_wait_to_handle = true;
            osTimerStart(curr_device->avrcp_play_status_wait_timer, APP_BT_AUDIO_AVRCP_PLAY_STATUS_WAIT_TIMER_MS);
            handle_play_status_event = false;
        }
        else
        {
            *result_play_status = curr_play_status;
            handle_play_status_event = true;
        }
    }
    else
    {
        // the play status register NOTIFY RSP event come, stop the timer
        osTimerStop(curr_device->avrcp_play_status_wait_timer);
        handle_play_status_event = app_bt_audio_assume_play_status_notify_rsp_event_received(curr_device, curr_play_status, true, result_play_status);
    }

    TRACE(4, "(d%x) %s %d result %d", device_id, __func__, handle_play_status_event, *result_play_status);

    return handle_play_status_event;
}

void app_bt_audio_new_sco_is_rejected_by_controller(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device || curr_device->hfchan_call != BTIF_HF_CALL_ACTIVE)
    {
        return;
    }

    TRACE(4, "(d%x) sco rejected by controller: config %d%d sco_connect_times %d",
        device_id,
        app_bt_manager.config.sco_prompt_play_mode,
        app_bt_manager.config.block_2nd_sco_before_call_active,
        g_app_bt_audio_sco_connect_times_count);
}

#define APP_BT_AUDIO_AVRCP_STATUS_QUICK_SWITCH_FILTER_TIMER_MS 2000

void app_bt_audio_avrcp_status_quick_switch_filter_timer_callback(void const *param)
{
    struct BT_DEVICE_T *curr_device = (struct BT_DEVICE_T *)param;

    curr_device->filter_avrcp_pause_play_quick_switching = false;

    TRACE(3, "(d%x) avrcp pause/play status quick switch filtered %d %d",
            curr_device->device_id, curr_device->a2dp_streamming, curr_device->avrcp_palyback_status);

    if (curr_device->a2dp_streamming && curr_device->avrcp_palyback_status != BTIF_AVRCP_MEDIA_PLAYING)
    {
        app_bt_audio_event_handler(curr_device->device_id, APP_BT_AUDIO_EVENT_AVRCP_IS_REALLY_PAUSED, 0);
    }
}

static bool app_bt_aduio_a2dp_start_filter_avrcp_quick_switch_timer(struct BT_DEVICE_T *curr_device)
{
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    bool timer_started = false;

    if (curr_playing_a2dp == curr_device->device_id)
    {
        osTimerStop(curr_device->avrcp_pause_play_quick_switch_filter_timer);

        osTimerStart(curr_device->avrcp_pause_play_quick_switch_filter_timer, APP_BT_AUDIO_AVRCP_STATUS_QUICK_SWITCH_FILTER_TIMER_MS);

        timer_started = true;
    }

    return timer_started;
}

static void app_bt_aduio_a2dp_stop_avrcp_pause_status_wait_timer(struct BT_DEVICE_T *curr_device)
{
    if (curr_device->filter_avrcp_pause_play_quick_switching)
    {
        curr_device->filter_avrcp_pause_play_quick_switching = false;
        osTimerStop(curr_device->avrcp_pause_play_quick_switch_filter_timer);
    }
}

static struct BT_DEVICE_T *app_bt_audio_a2dp_another_device_filting_avrcp_pause_status(uint8_t device_id)
{
    struct BT_DEVICE_T* curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        if (i == device_id)
        {
            continue;
        }
        curr_device = app_bt_get_device(i);
        if (curr_device->filter_avrcp_pause_play_quick_switching)
        {
            return curr_device;
        }
    }

    return NULL;
}

#define PHONE_AUTO_PAUSE_A2DP_AND_CREATE_SCO_LINK_TIME_DIFF_MS 1000
#define PHONE_AUTO_START_A2DP_AFTER_SCO_LINK_DISC_TIME_DIFF_MS 2000

int app_bt_audio_event_handler(uint8_t device_id, enum app_bt_audio_event_t event, uint32_t data)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    uint8_t curr_playing_a2dp = app_bt_audio_get_curr_playing_a2dp();
    bool a2dp_is_auto_started_after_interrupted_a2dp = false;
    bool sco_is_connected_before = false;
    uint8_t a2dp_stream_start_ctx = 0;
    uint32_t avrcp_play_status = data;
    struct BT_DEVICE_T *other_filtering_avrcp_pause_status_device = NULL;

    if (event == APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_CHANGED || event == APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_NOTIFY_RSP)
    {
        uint32_t play_status = 0;
        if (!app_bt_audio_event_judge_avrcp_play_status(device_id, event, data, &play_status))
        {
            return 0; // skip this avrcp status event
        }
        else
        {
            event = APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS;
            avrcp_play_status = play_status; // continue handle the avrcp status event
        }
    }

    switch (event)
    {
    case APP_BT_AUDIO_EVENT_A2DP_STREAM_OPEN:
        curr_device->this_is_curr_playing_a2dp_and_paused = false;
        curr_device->a2dp_audio_prio = 0;
        curr_device->a2dp_conn_prio = app_bt_audio_create_new_prio();
        app_bt_a2dp_set_curr_stream(BT_DEVICE_AUTO_CHOICE_ID);
        break;
    case APP_BT_AUDIO_EVENT_PROFILE_EXCHANGED:
#if defined(IBRT_V2_MULTIPOINT)
        app_bt_audio_a2dp_check_after_profile_exchanged(device_id);
#endif
        break;
    case APP_BT_AUDIO_EVENT_A2DP_STREAM_MOCK_START:
        a2dp_stream_start_ctx = APP_BT_AUDIO_EVENT_A2DP_STREAM_MOCK_START;
        /* fall through */
    case APP_BT_AUDIO_EVENT_A2DP_STREAM_START:
        a2dp_audio_sysfreq_update_normalfreq();
        if (a2dp_stream_start_ctx == 0)
        {
            a2dp_stream_start_ctx = APP_BT_AUDIO_EVENT_A2DP_STREAM_START;
        }
        other_filtering_avrcp_pause_status_device = app_bt_audio_a2dp_another_device_filting_avrcp_pause_status(device_id);
        if (other_filtering_avrcp_pause_status_device)
        {
            app_bt_aduio_a2dp_stop_avrcp_pause_status_wait_timer(other_filtering_avrcp_pause_status_device);
            other_filtering_avrcp_pause_status_device->a2dp_audio_prio = 0; // prio set to 0 let it skip streaming a2dp selection
            app_bt_audio_stop_a2dp_playing(other_filtering_avrcp_pause_status_device->device_id);
        }
music_player_status_is_playing:
        if (a2dp_stream_start_ctx == 0)
        {
            a2dp_stream_start_ctx = APP_BT_AUDIO_EVENT_AVRCP_MEDIA_PLAYING;
        }
        curr_device->a2dp_audio_prio = app_bt_audio_create_new_prio();
        if (a2dp_is_auto_started_after_interrupted_a2dp && device_id != curr_device->remember_interrupted_a2dp_for_a_while)
        {
            // current auto started a2dp is not the interrupted a2dp, make interrupted a2dp higher priority
            app_bt_get_device(curr_device->remember_interrupted_a2dp_for_a_while)->a2dp_audio_prio = app_bt_audio_create_new_prio();
        }
        if (a2dp_is_auto_started_after_interrupted_a2dp && device_id == curr_device->remember_interrupted_a2dp_for_a_while && event == APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS)
        {
            /**
             * d1 playing a2dp, d0 playing a2dp, d0 make a call, the interrupted a2dp is d0
             * d0 auto pause a2dp, d1 become curr playing a2dp for a while (need to fix)
             * d0 sco connected, recorrect interrupted a2dp to d0, d1 music is streaming background
             * d0 sco disconnected, retore interrupted a2dp d0 to play, but this a2dp is waiting for auto start
             * d0 auto start its a2dp, firstly d0 send avrcp playing status, and the code flow goes to here
             * because d0 is not sreaming yet (the start stream cmd is not receveid yet after avrcp playing status),
             * we need skip this avrcp playing status to avoid select the wrong a2dp stream to play.
             */
            break; // skip this avrcp play status due to the interrupted a2dp is not stremaing yet
        }
        app_bt_audio_select_a2dp_stream_to_play(device_id, a2dp_stream_start_ctx);
        break;
    case APP_BT_AUDIO_EVENT_A2DP_STREAM_CLOSE:
        if (curr_device->ibrt_slave_force_disc_a2dp)
        {
            break;
        }
        if (btif_a2dp_is_disconnected(curr_device->btif_a2dp_stream->a2dp_stream))
        {
            curr_device->a2dp_conn_prio = 0;
            curr_device->this_is_paused_bg_a2dp = false;
            curr_device->pause_a2dp_resume_prio = 0;
            curr_device->a2dp_disc_on_process = 0;
        }
        curr_device->auto_make_remote_play = false;
        /* fall through */
    case APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND:
        a2dp_audio_sysfreq_update_normalfreq();
        curr_device->remember_interrupted_a2dp_for_a_while = BT_DEVICE_INVALID_ID;
        curr_device->this_is_curr_playing_a2dp_and_paused = false;
        app_bt_aduio_a2dp_stop_avrcp_pause_status_wait_timer(curr_device);
        if (event == APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND)
        {
            curr_device->a2dp_paused_time = Plt_GetTicks();
            if (curr_playing_a2dp != BT_DEVICE_INVALID_ID && curr_playing_a2dp == device_id)
            {
                curr_device->this_is_curr_playing_a2dp_and_paused = true;
            }
        }
music_player_status_is_paused:
        curr_device->a2dp_audio_prio = 0; // prio set to 0 let it skip streaming a2dp selection
        app_bt_audio_stop_a2dp_playing(device_id);
        if (app_bt_manager.interrupted_a2dp_id == BT_DEVICE_INVALID_ID || app_bt_audio_select_bg_a2dp_to_resume() != BT_DEVICE_INVALID_ID)
        {
            app_bt_audio_select_a2dp_stream_to_play(device_id, (event == APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS) ? APP_BT_AUDIO_EVENT_AVRCP_MEDIA_PAUSED : APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND);
        }
        break;
    case APP_BT_AUDIO_EVENT_AVRCP_IS_REALLY_PAUSED:
        goto music_player_status_is_paused;
        break;
    case APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS:
        /* fall through */
    case APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_MOCK:
        if (curr_device->auto_make_remote_play)
        {
            curr_device->auto_make_remote_play = false;
#if defined(IBRT_V2_MULTIPOINT)
            app_bt_ibrt_audio_play_a2dp_stream(device_id);
#else
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PLAY);
#endif
        }
        if (avrcp_play_status == BTIF_AVRCP_MEDIA_PAUSED)
        {
            /**
             * after pause iphone music player, it takes very long to suspend a2dp stream,
             * but the playback pause status is updated very soon via avrcp, use this info
             * can speed up the switch to other playing a2dp stream.
             *
             * music_player_status_is_paused is used to switch between multiple a2dp quickly,
             * if there is only one a2dp stream, no need to use it due to the phone may send
             * wrong media paused status.
             *
             * If there is a2dp need to resume, we may trigger this event in order to
             * resume quickly: `app_bt_audio_select_bg_a2dp_to_resume() != BT_DEVICE_INVALID_ID`.
             * But there is a compatibility issue case:
             * 1. d0 play music, and d1 play music
             * 2. stop d0 playing and pause d0
             * 3. but d1 send incorrect avrcp PAUSE status
             * 4. this will resume to play d0 and d1 is still playing
             */

            if (app_bt_count_connected_device() < 2)
            {
                break;
            }

            if (app_bt_audio_count_streaming_a2dp() > 1 || app_bt_audio_select_bg_a2dp_to_resume() != BT_DEVICE_INVALID_ID ||
                (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && BT_DEVICE_INVALID_ID != app_bt_audio_select_another_connected_a2dp(device_id)))
            {
                TRACE(1, "(d%x) music_player_status_is_paused", device_id);

                // filter avrcp puase/play quick status switching, such as select to play next song on the phone
                if (app_bt_aduio_a2dp_start_filter_avrcp_quick_switch_timer(curr_device))
                {
                    curr_device->filter_avrcp_pause_play_quick_switching = true;
                }
                else
                {
                    curr_device->filter_avrcp_pause_play_quick_switching = false;
                }

                if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp)
                {
                    // If phone send wrong pause status, and the a2dp is not suspended in N seconds, this a2dp will replay
                    // The waiting time shall be long enough, because if it is too short, it will replay quickly before it suspended,
                    // d1 is try reconnecting a2dp profile, this will lead disconnect d1's connecting profile, and d1 music played on the phone 
                    app_bt_audio_a2dp_streaming_recheck_timer_start(device_id, APP_BT_AUDIO_A2DP_WAIT_PAUSED_STREAM_SUSPEND);
                }

                if (false == curr_device->filter_avrcp_pause_play_quick_switching)
                {
                    goto music_player_status_is_paused;
                }
            }
        }
        else if (avrcp_play_status == BTIF_AVRCP_MEDIA_PLAYING)
        {
            app_bt_aduio_a2dp_stop_avrcp_pause_status_wait_timer(curr_device);
            goto music_player_status_is_playing;
        }
        break;
    case APP_BT_AUDIO_EVENT_HFP_SERVICE_CONNECTED:
        curr_device->sco_audio_prio = 0;
        curr_device->hfp_conn_prio = app_bt_audio_create_new_prio();
        app_bt_hf_set_curr_stream(BT_DEVICE_AUTO_CHOICE_ID);
        break;
    case APP_BT_AUDIO_EVENT_HFP_SCO_CONNECT_REQ:
        if (!app_bt_audio_count_connected_sco())
        {
            return true;
        }
        if (!app_bt_manager.config.keep_only_one_sco_link)
        {
            return true;
        }
        if (app_bt_manager.config.sco_prompt_play_mode)
        {
            return true;
        }
        else
        {
            return false; // older sco is not interrupted, reject new sco
        }
        break;
    case APP_BT_AUDIO_EVENT_HFP_SCO_CONNECTED:
#if defined(BT_WATCH_APP)
        app_bt_source_audio_event_handler(device_id, APP_BT_SOURCE_AUDIO_EVENT_HF_SCO_CONNECTED, NULL);
#endif
        if (app_bt_manager.device_routed_sco_to_phone == device_id)
        {
            app_bt_manager.device_routed_sco_to_phone = BT_DEVICE_INVALID_ID;
        }
        if (app_bt_manager.device_routing_sco_back == device_id)
        {
            app_bt_manager.device_routing_sco_back = BT_DEVICE_INVALID_ID;
        }
        curr_device->a2dp_is_auto_paused_by_phone = false;
        if (TICKS_TO_MS(Plt_GetTicks()-curr_device->a2dp_paused_time) < PHONE_AUTO_PAUSE_A2DP_AND_CREATE_SCO_LINK_TIME_DIFF_MS)
        {
            TRACE(2,"a2dp %x is auto paused by phone, curr_playing_a2dp %x", device_id, curr_playing_a2dp);
            curr_device->a2dp_is_auto_paused_by_phone = true;
        }
        app_bt_manager.interrupted_a2dp_id = curr_playing_a2dp;
        if (curr_device->a2dp_is_auto_paused_by_phone && curr_device->this_is_curr_playing_a2dp_and_paused)
        {
            /**
             * d0 playing a2dp, d1 playing a2dp, d1 is the curr playing a2dp;
             * d1 start to make a call, it first auto pause d1 a2dp, d0 becomes curr playing a2dp;
             * d1 sco connected and goes here, the interrupted_a2dp_id is set to d0 above;
             * but it is wrong, because d1 is the real interrupted a2dp by d1 call;
             * so below, we first stop d0 a2dp palying, and then correct the interrupted a2dp to d1;
             */
            if (curr_playing_a2dp != BT_DEVICE_INVALID_ID && curr_playing_a2dp != device_id)
            {
                app_bt_audio_stop_a2dp_playing(curr_playing_a2dp);
            }
            app_bt_manager.interrupted_a2dp_id = device_id; // correct the real interrupted a2dp id
            curr_device->a2dp_audio_prio = app_bt_audio_create_new_prio(); // keep it as higher priority
        }
        curr_device->this_is_curr_playing_a2dp_and_paused = false;
        curr_device->sco_audio_prio = app_bt_audio_create_new_prio();
        app_bt_audio_select_sco_stream_to_play(device_id, APP_BT_AUDIO_EVENT_HFP_SCO_CONNECTED);
        break;
    case APP_BT_AUDIO_EVENT_HFP_SERVICE_DISCONNECTED:
        if (app_bt_manager.device_routed_sco_to_phone == device_id)
        {
            app_bt_manager.device_routed_sco_to_phone = BT_DEVICE_INVALID_ID;
        }
        if (app_bt_manager.device_routing_sco_back == device_id)
        {
            app_bt_manager.device_routing_sco_back = BT_DEVICE_INVALID_ID;
        }
        curr_device->hfp_conn_prio = 0;
        sco_is_connected_before = data ? true : false;
        /* fall through */
    case APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED:
        curr_device->remember_interrupted_a2dp_for_a_while = BT_DEVICE_INVALID_ID;
        curr_device->sco_audio_prio = 0;
        curr_device->a2dp_is_auto_paused_by_phone = false;
        if (event == APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED || sco_is_connected_before)
        {
            app_bt_manager.sco_trigger_a2dp_replay = true; // let a2dp replay (if any) after sco stopped
        }
        app_bt_audio_stop_sco_playing(device_id, event);
curr_device_callsetup_is_rejected:
        app_bt_audio_select_sco_stream_to_play(device_id, event);
#if defined(BT_WATCH_APP)
        app_bt_source_audio_event_handler(device_id, APP_BT_SOURCE_AUDIO_EVENT_HF_SCO_DISCONNECTED, NULL);
#endif
        break;
    case APP_BT_AUDIO_EVENT_HFP_CCWA_IND:
        break;
    case APP_BT_AUDIO_EVENT_HFP_RING_IND:
#ifdef IBRT
        if(APP_IBAT_UI_GET_CURRENT_ROLE(&curr_device->remote) == IBRT_SLAVE)
        {
            break;
        }
#endif
#if !defined(FPGA) && defined(MEDIA_PLAYER_SUPPORT)
        if(curr_device->hf_audio_state != BTIF_HF_AUDIO_CON)
        {
            app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL, device_id);
        }
#endif
        break;
    case APP_BT_AUDIO_EVENT_HFP_CLCC_IND:
#if !defined(FPGA) && defined(MEDIA_PLAYER_SUPPORT) && defined(__BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__)
        if (curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN)
        {
            app_voice_report(APP_STATUS_RING_WARNING, device_id);
        }
#endif
        break;
    case APP_BT_AUDIO_EVENT_HFP_CALLSETUP_IND:
        if (!app_bt_is_hfp_connected(device_id))
        {
            if (curr_device->hfchan_callSetup != BTIF_HF_CALL_SETUP_NONE)
            {
                // call alread in setup flow before creating hfp profile
                curr_device->hfp_call_setup_prio = app_bt_audio_create_new_prio();
            }
            else
            {
                curr_device->hfp_call_setup_prio = 0;
            }
            break;
        }
        if (curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN || curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_OUT)
        {
            // the start of callsetup flow
            curr_device->hfp_call_setup_prio = app_bt_audio_create_new_prio();

#ifdef BT_USB_AUDIO_DUAL_MODE
            if(!btusb_is_bt_mode())
            {
                TRACE(0,"btusb_usbaudio_close doing.");
                btusb_usbaudio_close();
            }
#endif
        }
        else if (curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_NONE)
        {
            // the end of callsetup flow
            curr_device->hfp_call_setup_prio = 0;

#if !defined(FPGA) && defined(MEDIA_PLAYER_SUPPORT) && !defined(AUDIO_PROMPT_USE_DAC2_ENABLED) // stop any voice report in callsetup flow
            if(curr_device->hfchan_call == BTIF_HF_CALL_NONE)
            {
                TRACE(1, "(d%x) REFUSECALL, stop no in-band ring\n", device_id);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA, BT_STREAM_MEDIA, device_id, 0);
            }
#endif
            if (curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE)
            {
                // call is answered
                TRACE(1,"(d%x) !!!HF_EVENT_CALLSETUP_IND ANSWERCALL\n", device_id);
            }
            else
            {
                // call is refused
                TRACE(1,"(d%x) !!!HF_EVENT_CALLSETUP_IND REFUSECALL\n", device_id);

                if (curr_device->hf_audio_state == BTIF_HF_AUDIO_DISCON)
                {
                    goto curr_device_callsetup_is_rejected;
                }
            }
        }
        break;
    case APP_BT_AUDIO_EVENT_HFP_CALL_IND:
        if (!app_bt_is_hfp_connected(device_id))
        {
            if (curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE)
            {
                // call already active before creating hfp profile
                curr_device->hfp_call_active_prio = app_bt_audio_create_new_prio();
            }
            else
            {
                curr_device->hfp_call_active_prio = 0;
            }
            break;
        }
        if (curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE)
        {
            // a call become active
            curr_device->hfp_call_active_prio = app_bt_audio_create_new_prio();

#if !defined(FPGA) && defined(MEDIA_PLAYER_SUPPORT) && !defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
            TRACE(1, "(d%x) ANSWERCALL, stop no in-band ring\n", device_id);
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA, BT_STREAM_MEDIA, device_id, 0);
#endif
        }
        else
        {
            // call is hangup
            curr_device->hfp_call_active_prio = 0;

            TRACE(1,"(d%x) !!!HF_EVENT_CALL_IND HANGUPCALL\n", device_id);

            if (curr_device->hf_audio_state != BTIF_HF_AUDIO_CON)
            {
                uint8_t device_with_active_call = app_bt_audio_select_another_device_to_create_sco(device_id);
                if (device_with_active_call != BT_DEVICE_INVALID_ID && device_with_active_call != app_bt_manager.device_routing_sco_back)
                {
                    app_bt_audio_create_sco_for_another_call_active_device(device_with_active_call);
                }
            }

#if 0 // lead compatibility issue when phone is also disconnecting sco
            if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
            {
                TRACE(1,"(d%x) call hangup but sco still exist, disconnect it", device_id);
                app_bt_HF_DisconnectAudioLink(curr_device->hf_channel);
            }
#endif
        }
        break;
    case APP_BT_AUDIO_EVENT_HFP_CALLHELD_IND:
        if (!app_bt_is_hfp_connected(device_id))
        {
            break;
        }
        if (curr_device->hf_callheld == BTIF_HF_CALL_HELD_ACTIVE)
        {
            // has call in held and also has call in active
            curr_device->hfp_call_active_prio = app_bt_audio_create_new_prio();
        }
        break;
    default:
        break;
    }

    return 0;
}

