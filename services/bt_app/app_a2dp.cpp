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
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#ifdef __IAG_BLE_INCLUDE__
#include "app.h"
#endif
#include "app_audio.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "hal_location.h"
#include "a2dp_api.h"
#include "app_a2dp.h"
#include "app_trace_rx.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#if defined(A2DP_LHDC_ON)
//#include "../liblhdc-dec/lhdcUtil.h"
#include "lhdcUtil.h"
#endif

#if defined(A2DP_LDAC_ON)
#include"ldacBT.h"
#endif

#include "a2dp_api.h"
#include "avrcp_api.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"
#include "bt_drv_interface.h"
#include "hci_api.h"
#include "hal_bootmode.h"
#include "bt_drv_reg_op.h"

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif

#if defined(BT_MAP_SUPPORT)
#include "app_btmap_sms.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_voice.h"
#endif

#define _FILE_TAG_ "A2DP"
#include "color_log.h"
#include "app_bt_func.h"
#include "os_api.h"

#if (A2DP_DECODER_VER >= 2)
#include "a2dp_decoder.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#ifdef IBRT_CORE_V2_ENABLE
#include "app_tws_ibrt_conn_api.h"
#endif
#include "app_tws_besaud.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom_bt.h"
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "app.h"
#include "app_ble_mode_switch.h"
#endif

#include "app_ibrt_customif_cmd.h"

#define APP_A2DP_STRM_FLAG_QUERY_CODEC  0x08

#define APP_A2DP__DEBUG

#ifdef  APP_A2DP__DEBUG
#define APP_A2DP_TRACE(str, ...)   TRACE(str, ##__VA_ARGS__)
#else
#define APP_A2DP_TRACE(str, ...)
#endif

#ifdef GFPS_ENABLED
extern "C" void app_exit_fastpairing_mode(void);
#endif
extern int app_bt_stream_volumeset(int8_t vol);
extern int store_sbc_buffer(uint8_t device_id, unsigned char *buf, unsigned int len);

static void app_AVRCP_sendCustomCmdRsp(uint8_t device_id, btif_avrcp_channel_t *chnl, uint8_t isAccept, uint8_t transId);
static void app_AVRCP_CustomCmd_Received(uint8_t* ptrData, uint32_t len);

bool is_bd_addr_valid(bt_bdaddr_t * addr)
{
    uint8_t addr_empty[6];
    memset(addr_empty, 0, sizeof(addr_empty));
    if (memcmp(addr,addr_empty,6)){
        return TRUE;
    }else{
        return FALSE;
   }

}

#ifndef IBRT
static void app_avrcp_reconnect_timeout_timer_handler(struct BT_DEVICE_T *curr_device)
{
    TRACE(4,"(d%x) %s a2dp state %d avrcp state %d", curr_device->device_id,
            __func__, curr_device->a2dp_conn_flag, btif_get_avrcp_state(curr_device->avrcp_channel));
    if ((!app_is_disconnecting_all_bt_connections()) && curr_device->a2dp_conn_flag &&
        (btif_get_avrcp_state(curr_device->avrcp_channel) != BTIF_AVRCP_STATE_CONNECTED))
    {
        btif_remote_device_t *rdev = NULL;
        if (curr_device->a2dp_connected_stream && (rdev = btif_a2dp_get_stream_conn_remDev(curr_device->a2dp_connected_stream)))
        {
            bt_bdaddr_t *bd_addr = btif_me_get_remote_device_bdaddr(rdev);
            if (is_bd_addr_valid(bd_addr)) {
                app_bt_reconnect_avrcp_profile(bd_addr);
            } else {
                TRACE(1,"%s bd_addr is empty ",__func__);
            }
        }
    }
}

static void app_avrcp_reconnect_timeout_timer_callback(void const *param)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)param ,0,
        (uint32_t)app_avrcp_reconnect_timeout_timer_handler);
}
#endif

#define MAX_AVRCP_CONNECT_TRY_TIME 3
extern void app_bt_audio_avrcp_play_status_wait_timer_callback(void const *param);
extern void app_bt_audio_a2dp_stream_recheck_timer_callback(void const *param);
extern void app_bt_audio_avrcp_status_quick_switch_filter_timer_callback(void const *param);
extern void app_bt_reconnect_a2dp_after_role_switch_callback(void const *param);
#ifndef IBRT
osTimerDef (APP_AVRCP_RECONNECT_TIMER0, app_avrcp_reconnect_timeout_timer_callback);
#endif
osTimerDef (APP_AVRCP_QUICK_SWITCH_FILTER_TIMER0, app_bt_audio_avrcp_status_quick_switch_filter_timer_callback);
osTimerDef (APP_AVRCP_PLAY_STATUS_WAIT_TIMER0, app_bt_audio_avrcp_play_status_wait_timer_callback);
osTimerDef (APP_A2DP_STREAM_RECHECK_TIMER0, app_bt_audio_a2dp_stream_recheck_timer_callback);
osTimerDef(APP_A2DP_DELAY_RECONNECT_TIMER,app_bt_reconnect_a2dp_after_role_switch_callback);
#if BT_DEVICE_NUM > 1
#ifndef IBRT
osTimerDef (APP_AVRCP_RECONNECT_TIMER1, app_avrcp_reconnect_timeout_timer_callback);
#endif
osTimerDef (APP_AVRCP_QUICK_SWITCH_FILTER_TIMER1, app_bt_audio_avrcp_status_quick_switch_filter_timer_callback);
osTimerDef (APP_AVRCP_PLAY_STATUS_WAIT_TIMER1, app_bt_audio_avrcp_play_status_wait_timer_callback);
osTimerDef (APP_A2DP_STREAM_RECHECK_TIMER1, app_bt_audio_a2dp_stream_recheck_timer_callback);
#endif
#if BT_DEVICE_NUM > 2
#ifndef IBRT
osTimerDef (APP_AVRCP_RECONNECT_TIMER2, app_avrcp_reconnect_timeout_timer_callback);
#endif
osTimerDef (APP_AVRCP_QUICK_SWITCH_FILTER_TIMER2, app_bt_audio_avrcp_status_quick_switch_filter_timer_callback);
osTimerDef (APP_AVRCP_PLAY_STATUS_WAIT_TIMER2, app_bt_audio_avrcp_play_status_wait_timer_callback);
osTimerDef (APP_A2DP_STREAM_RECHECK_TIMER2, app_bt_audio_a2dp_stream_recheck_timer_callback);
#endif

void get_value1_pos(U8 mask,U8 *start_pos, U8 *end_pos)
{
    U8 num = 0;

    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask){
            *start_pos = i;//start_pos,end_pos stands for the start and end position of value 1 in mask
            break;
        }
    }
    for(U8 i=0;i<8;i++){
        if((0x01<<i) & mask)
            num++;//number of value1 in mask
    }
    *end_pos = *start_pos + num - 1;
}
U8 a2dp_codec_sbc_get_valid_bit(U8 elements, U8 mask)
{
    U8 start_pos,end_pos;

    get_value1_pos(mask,&start_pos,&end_pos);
//    TRACE(2,"!!!start_pos:%d,end_pos:%d\n",start_pos,end_pos);
    for(U8 i = start_pos; i <= end_pos; i++){
        if((0x01<<i) & elements){
            elements = ((0x01<<i) | (elements & (~mask)));
            break;
        }
    }
    return elements;
}

void app_avrcp_get_capabilities_start(int device_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    APP_A2DP_TRACE(0,"::AVRCP_GET_CAPABILITY\n");

    btif_avrcp_ct_get_capabilities(curr_device->avrcp_channel, BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED);
}

static void a2dp_set_codec_info(uint8_t dev_num, const uint8_t *codec)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(dev_num);
    curr_device->codec_type = codec[0];
    curr_device->sample_bit = codec[1];
    curr_device->sample_rate = codec[2];
#if defined(A2DP_LHDC_ON)
    curr_device->a2dp_lhdc_llc = codec[3];
#endif
#if defined(A2DP_LDAC_ON)
    app_ibrt_restore_ldac_info(dev_num, curr_device->sample_rate);
#endif

}

static void a2dp_get_codec_info(uint8_t dev_num, uint8_t *codec)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(dev_num);
    codec[0] = curr_device->codec_type;
    codec[1] = curr_device->sample_bit;
    codec[2] = curr_device->sample_rate;
#if defined(A2DP_LHDC_ON)
    codec[3] = curr_device->a2dp_lhdc_llc;
#endif
}

static bool g_a2dp_is_inited = false;

void a2dp_init(void)
{
    if (g_a2dp_is_inited)
    {
        return;
    }

    g_a2dp_is_inited = true;
#ifdef CUSTOM_BITRATE
    a2dp_avdtpcodec_sbc_user_configure_set(nv_record_get_extension_entry_ptr()->codec_user_info.sbc_bitpool, true);
    a2dp_avdtpcodec_aac_user_configure_set(nv_record_get_extension_entry_ptr()->codec_user_info.aac_bitrate, true); 
    app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_SBC, (nv_record_get_extension_entry_ptr()->codec_user_info.audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/3, true);//sbc
    app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC, (nv_record_get_extension_entry_ptr()->codec_user_info.audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/23, true);//aac
#endif

    btif_a2dp_init();

    btif_a2dp_get_codec_info_func(a2dp_get_codec_info);

    btif_a2dp_set_codec_info_func(a2dp_set_codec_info);

    if (app_bt_sink_is_enabled())
    {
#if BT_DEVICE_NUM > 1
        btif_a2dp_register_multi_link_connect_not_allowed_callback(app_bt_audio_a2dp_disconnect_self_check);
#endif

        for(int i =0; i< BT_DEVICE_NUM; i++ )
        {
            struct BT_DEVICE_T* curr_device = app_bt_get_device(i);
            curr_device->btif_a2dp_stream = btif_a2dp_alloc_sink_stream();
            curr_device->profile_mgr.stream = curr_device->btif_a2dp_stream->a2dp_stream;
            curr_device->a2dp_connected_stream = curr_device->btif_a2dp_stream->a2dp_stream;
            curr_device->channel_mode = 0;
            curr_device->a2dp_channel_num = 2;
            curr_device->a2dp_conn_flag = 0;
            curr_device->a2dp_streamming = 0;
            curr_device->a2dp_play_pause_flag = 0;
            curr_device->avrcp_play_status_wait_to_handle = false;
            curr_device->a2dp_disc_on_process = 0;

#ifdef __A2DP_AVDTP_CP__
            curr_device->avdtp_cp = 0;
            memset(curr_device->a2dp_avdtp_cp_security_data, 0, sizeof(curr_device->a2dp_avdtp_cp_security_data));
#endif
            btif_a2dp_stream_init(curr_device->btif_a2dp_stream, BTIF_A2DP_STREAM_TYPE_SINK);

            a2dp_codec_sbc_init(i);

#if defined(A2DP_AAC_ON)
            a2dp_codec_aac_init(i);
#endif
#if defined(A2DP_LDAC_ON)
            a2dp_codec_ldac_init(i);
#endif
#if defined(A2DP_LHDC_ON)
            a2dp_codec_lhdc_init(i);
#endif
#if defined(MASTER_USE_OPUS) || defined(ALL_USE_OPUS)
            a2dp_codec_opus_init(i);
#endif
#if defined(A2DP_SCALABLE_ON)
            a2dp_codec_scalable_init(i);
#endif

            if (i == 0)
            {
            #ifndef IBRT
                curr_device->avrcp_reconnect_timer = osTimerCreate(osTimer(APP_AVRCP_RECONNECT_TIMER0), osTimerOnce, curr_device);
            #endif
                curr_device->avrcp_pause_play_quick_switch_filter_timer = osTimerCreate(osTimer(APP_AVRCP_QUICK_SWITCH_FILTER_TIMER0), osTimerOnce, curr_device);
                curr_device->avrcp_play_status_wait_timer = osTimerCreate(osTimer(APP_AVRCP_PLAY_STATUS_WAIT_TIMER0), osTimerOnce, curr_device);
                curr_device->a2dp_stream_recheck_timer = osTimerCreate(osTimer(APP_A2DP_STREAM_RECHECK_TIMER0), osTimerOnce, curr_device);
                curr_device->a2dp_delay_reconnect_timer = osTimerCreate(osTimer(APP_A2DP_DELAY_RECONNECT_TIMER),osTimerOnce,curr_device);
            }
#if BT_DEVICE_NUM > 1
            else if (i == 1)
            {
            #ifndef IBRT
                curr_device->avrcp_reconnect_timer = osTimerCreate(osTimer(APP_AVRCP_RECONNECT_TIMER1), osTimerOnce, curr_device);
            #endif
                curr_device->avrcp_pause_play_quick_switch_filter_timer = osTimerCreate(osTimer(APP_AVRCP_QUICK_SWITCH_FILTER_TIMER1), osTimerOnce, curr_device);
                curr_device->avrcp_play_status_wait_timer = osTimerCreate(osTimer(APP_AVRCP_PLAY_STATUS_WAIT_TIMER1), osTimerOnce, curr_device);
                curr_device->a2dp_stream_recheck_timer = osTimerCreate(osTimer(APP_A2DP_STREAM_RECHECK_TIMER1), osTimerOnce, curr_device);
                curr_device->a2dp_delay_reconnect_timer = osTimerCreate(osTimer(APP_A2DP_DELAY_RECONNECT_TIMER),osTimerOnce,curr_device);
            }
#endif
#if BT_DEVICE_NUM > 2
            else if (i == 2)
            {
            #ifndef IBRT
                curr_device->avrcp_reconnect_timer = osTimerCreate(osTimer(APP_AVRCP_RECONNECT_TIMER2), osTimerOnce, curr_device);
            #endif
                curr_device->avrcp_pause_play_quick_switch_filter_timer = osTimerCreate(osTimer(APP_AVRCP_QUICK_SWITCH_FILTER_TIMER2), osTimerOnce, curr_device);
                curr_device->avrcp_play_status_wait_timer = osTimerCreate(osTimer(APP_AVRCP_PLAY_STATUS_WAIT_TIMER2), osTimerOnce, curr_device);
                curr_device->a2dp_stream_recheck_timer = osTimerCreate(osTimer(APP_A2DP_STREAM_RECHECK_TIMER2), osTimerOnce, curr_device);
                curr_device->a2dp_delay_reconnect_timer = osTimerCreate(osTimer(APP_A2DP_DELAY_RECONNECT_TIMER),osTimerOnce,curr_device);
            }
#endif
        }

        app_bt_manager.curr_a2dp_stream_id = BT_DEVICE_ID_1;
    }
}

static bool a2dp_bdaddr_from_id(uint8_t id, bt_bdaddr_t *bd_addr) {
  btif_remote_device_t *remDev = NULL;
  ASSERT(id < BT_DEVICE_NUM, "invalid bt device id");
  if(NULL != bd_addr) {
    remDev = btif_a2dp_get_remote_device(app_bt_get_device(id)->a2dp_connected_stream);
    memset(bd_addr, 0, sizeof(bt_bdaddr_t));
    if (NULL != remDev) {
      memcpy(bd_addr, btif_me_get_remote_device_bdaddr(remDev), sizeof(bt_bdaddr_t));
      return true;
    }
  }
  return false;
}

static bool a2dp_bdaddr_cmp(bt_bdaddr_t *bd_addr_1, bt_bdaddr_t *bd_addr_2) {
  if((NULL == bd_addr_1) || (NULL == bd_addr_2)) {
    return false;
  }
  return (memcmp(bd_addr_1->address,
      bd_addr_2->address, BTIF_BD_ADDR_SIZE) == 0);
}

bool a2dp_id_from_bdaddr(bt_bdaddr_t *bd_addr, uint8_t *id)
{
    bt_bdaddr_t curr_addr = {0};
    uint8_t curr_id = BT_DEVICE_NUM;
    int i = 0;

    for (; i < BT_DEVICE_NUM; ++i) {
        if (app_bt_is_device_profile_connected(i)) {
            a2dp_bdaddr_from_id(i, &curr_addr);
            if(a2dp_bdaddr_cmp(&curr_addr, bd_addr)) {
                curr_id = i;
                break;
            }
        }
    }

    if(id) {
        *id = curr_id;
    }

    return (curr_id < BT_DEVICE_NUM);
}

#define APP_BT_PAUSE_MEDIA_PLAYER_DELAY 300
osTimerId app_bt_pause_media_player_delay_timer_id = NULL;
static uint8_t deviceIdPendingForMediaPlayerPause = 0xff;
static void app_bt_pause_media_player_delay_timer_handler(void const *n);
osTimerDef (APP_BT_PAUSE_MEDIA_PLAYER_DELAY_TIMER, app_bt_pause_media_player_delay_timer_handler);
static void app_bt_pause_media_player_delay_timer_handler(void const *n)
{
    app_bt_start_custom_function_in_bt_thread(deviceIdPendingForMediaPlayerPause, 0, (uint32_t)app_bt_pause_music_player);
}

bool app_bt_pause_music_player(uint8_t deviceId)
{
    struct BT_DEVICE_T* curr_device = NULL;

    if (!app_bt_is_music_player_working(deviceId))
    {
        return false;
    }

    curr_device = app_bt_get_device(deviceId);

    TRACE(1,"Pause music player of device %d", deviceId);
    app_bt_suspend_a2dp_streaming(deviceId);

    btif_avrcp_set_panel_key(curr_device->avrcp_channel, BTIF_AVRCP_POP_PAUSE, TRUE);
    btif_avrcp_set_panel_key(curr_device->avrcp_channel, BTIF_AVRCP_POP_PAUSE, FALSE);

    curr_device->a2dp_play_pause_flag = 0;
    curr_device->a2dp_need_resume_flag = 1;

    deviceIdPendingForMediaPlayerPause = 0xff;

    return true;
}

void app_bt_pause_media_player_again(uint8_t deviceId)
{
    if (NULL == app_bt_pause_media_player_delay_timer_id)
    {
        app_bt_pause_media_player_delay_timer_id = osTimerCreate(
            osTimer(APP_BT_PAUSE_MEDIA_PLAYER_DELAY_TIMER), osTimerOnce, NULL);
    }

    if (deviceIdPendingForMediaPlayerPause == 0xff)
    {
        TRACE(1,"(d%x) The media player is resumed before it's allowed, pause again", deviceId);
        deviceIdPendingForMediaPlayerPause = deviceId;
        osTimerStart(app_bt_pause_media_player_delay_timer_id, APP_BT_PAUSE_MEDIA_PLAYER_DELAY);
    }
    else
    {
        TRACE(1,"(d%x) %s another device %d is pending", deviceId, __func__, deviceIdPendingForMediaPlayerPause);
    }
}

bool app_bt_is_music_player_working(uint8_t deviceId)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(deviceId);
    TRACE(3,"device %d a2dp streaming %d playback state %d", deviceId, app_bt_is_a2dp_streaming(deviceId), curr_device->avrcp_palyback_status);
    return (app_bt_is_a2dp_streaming(deviceId) && 0x01 == curr_device->avrcp_palyback_status);
}

void app_bt_suspend_a2dp_streaming(uint8_t deviceId)
{
    if (!app_bt_is_a2dp_streaming(deviceId))
    {
        return;
    }

    TRACE(1,"Suspend a2dp streaming of device %d", deviceId);
    btif_a2dp_suspend_stream(app_bt_get_device(deviceId)->a2dp_connected_stream);
}

void app_bt_resume_music_player(uint8_t deviceId)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(deviceId);

    curr_device->a2dp_need_resume_flag = 0;

    if (app_bt_is_music_player_working(deviceId))
    {
        return;
    }

    TRACE(1,"Resume music player of device %d", deviceId);

    btif_avrcp_set_panel_key(curr_device->avrcp_channel,BTIF_AVRCP_POP_PLAY,TRUE);
    btif_avrcp_set_panel_key(curr_device->avrcp_channel,BTIF_AVRCP_POP_PLAY,FALSE);

    curr_device->a2dp_play_pause_flag = 1;
}

bool app_bt_is_a2dp_streaming(uint8_t deviceId)
{
    return app_bt_get_device(deviceId)->a2dp_streamming;
}

bool app_bt_is_a2dp_disconnected(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    return curr_device && (0 == curr_device->a2dp_conn_flag);
}

void app_bt_a2dp_disable_aac_codec(bool disable)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)disable, 0, (uint32_t)(uintptr_t)btif_a2dp_disable_aac_codec);
}

void app_bt_a2dp_disable_vendor_codec(bool disable)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)disable, 0, (uint32_t)(uintptr_t)btif_a2dp_disable_vendor_codec);
}

void app_bt_a2dp_reconfig_to_sbc(a2dp_stream_t *stream)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)stream, 0, (uint32_t)(uintptr_t)btif_a2dp_reconfig_codec_to_sbc);
}

FRAM_TEXT_LOC uint8_t bt_sbc_player_get_codec_type(void)
{
    uint8_t curr_device_id = app_bt_audio_get_curr_a2dp_device();
    btif_avdtp_codec_t *avdtp_codec = btif_a2dp_get_stream_codec_from_id(curr_device_id);

    //TRACE(3,"[%s] curr_device_id: %d, avdtp_codec->codecType:%d", __func__, curr_device_id,avdtp_codec->codecType);
    if(avdtp_codec)
    {
        return avdtp_codec->codecType;
    }
    else
    {
        return BTIF_AVDTP_CODEC_TYPE_SBC;
    }
}

FRAM_TEXT_LOC uint8_t bt_sbc_player_get_sample_bit(void) {
    return app_bt_get_device(app_bt_audio_get_curr_a2dp_device())->sample_bit;
}

uint16_t a2dp_Get_curr_a2dp_conhdl(void)
{
    return app_bt_manager.current_a2dp_conhdl;
}

void a2dp_get_curStream_remDev(btif_remote_device_t **p_remDev)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_a2dp_device());
    *p_remDev = btif_a2dp_get_remote_device(curr_device->a2dp_connected_stream);
}

void avrcp_get_current_media_status(enum BT_DEVICE_ID_T device_id)
{
    if(app_bt_get_device(device_id)->avrcp_conn_flag == 0)
        return;

    btif_avrcp_ct_get_play_status(app_bt_get_device(device_id)->avrcp_channel);
}

void btapp_a2dp_suspend_music(enum BT_DEVICE_ID_T stream_id);

extern "C" void avrcp_callback_CT(uint8_t device_id, btif_avrcp_channel_t* btif_avrcp, const avrcp_callback_parms_t* parms)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    btif_avrcp_channel_t *channel = curr_device->avrcp_channel;
    btif_avctp_event_t event = btif_avrcp_get_callback_event(parms);
    uint8_t init_local_volume = 0;
    uint8_t init_abs_volume = 0;

    if (device_id == BT_DEVICE_INVALID_ID && event == BTIF_AVRCP_EVENT_DISCONNECT)
    {
        // avrcp profile is closed due to acl created fail
        TRACE(1,"::AVRCP_EVENT_DISCONNECT acl created error=%x", btif_get_avrcp_cb_channel_error_code(parms));
        return;
    }

    TRACE(5,"(d%x) ::%s channel %p event %d%s", device_id, __func__, btif_avrcp, event, btif_avrcp_event_str(event));
    ASSERT(device_id < BT_DEVICE_NUM && curr_device->avrcp_channel == btif_avrcp, "avrcp device channel must match");

    switch(event)
    {
        case BTIF_AVRCP_EVENT_CONNECT_IND:
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Connect ok.");
#endif
            APP_A2DP_TRACE(2,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_CONNECT_IND %d", device_id, event);
            btif_avrcp_connect_rsp(channel, 1);
            break;
        case BTIF_AVRCP_EVENT_CONNECT:
#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Connect ok.");
#endif
            curr_device->avrcp_conn_flag = 1;
            curr_device->avrcp_remote_support_playback_status_change_event = false;
            curr_device->mock_avrcp_after_force_disc = false;
            curr_device->ibrt_slave_force_disc_avrcp = false;

            app_avrcp_get_capabilities_start(device_id);

            curr_device->a2dp_default_abs_volume = app_bt_manager.config.a2dp_default_abs_volume;

            init_abs_volume = a2dp_abs_volume_get((enum BT_DEVICE_ID_T)device_id);

            init_local_volume = a2dp_volume_local_get((enum BT_DEVICE_ID_T)device_id);

            TRACE(3, "(d%x) initial volume %d %d", device_id, init_local_volume, init_abs_volume);

            if (init_abs_volume == 0)
            {
                init_abs_volume = a2dp_convert_local_vol_to_bt_vol(init_local_volume);
            }

            curr_device->a2dp_initial_volume = init_abs_volume;

            app_bt_a2dp_current_abs_volume_just_set(device_id, init_abs_volume);

#ifdef _AVRCP_TRACK_CHANGED
            btif_avrcp_ct_register_track_change_notification(channel, 0);
#endif

            curr_device->avrcp_palyback_status = 0;
            avrcp_get_current_media_status((enum BT_DEVICE_ID_T)device_id);

#ifdef RESUME_MUSIC_AFTER_CRASH_REBOOT
            app_bt_resume_music_after_crash_reboot(device_id);
#endif
            curr_device->avrcp_connect_try_times = 0;
#ifndef IBRT
            osTimerStop(curr_device->avrcp_reconnect_timer);
#endif
            break;
        case BTIF_AVRCP_EVENT_CONNECT_MOCK:
            break;
        case BTIF_AVRCP_EVENT_PLAYBACK_STATUS_CHANGE_EVENT_SUPPORT:
            break;
        case BTIF_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
            break;
        case BTIF_AVRCP_EVENT_CT_SDP_INFO:
            /* avrcp_state IBRT_CONN_AVRCP_REMOTE_CT_0104 */
            break;
        case BTIF_AVRCP_EVENT_DISCONNECT:
            curr_device->avrcp_conn_flag = 0;
            curr_device->a2dp_need_resume_flag = 0;
            TRACE(1,"(d%x) ::AVRCP_EVENT_DISCONNECT", device_id);

            curr_device->avrcp_palyback_status = 0;
            curr_device->volume_report = 0;

#ifdef AVRCP_TRACK_CHANGED
            curr_device->track_changed = 0;
#endif

#ifndef IBRT
            curr_device->avrcp_connect_try_times = 0;
            osTimerStop(curr_device->avrcp_reconnect_timer);

            if(!app_is_disconnecting_all_bt_connections())
            {
                curr_device->avrcp_connect_try_times++;
                if (curr_device->avrcp_connect_try_times < MAX_AVRCP_CONNECT_TRY_TIME)
                {
                    osTimerStart(curr_device->avrcp_reconnect_timer, 3000);
                }
            }
#endif
            break;
        case BTIF_AVRCP_EVENT_RESPONSE:
            TRACE(3,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_RESPONSE op=0x%x[0x30=GET_PLAY_STATUS],status=%x %x",
                device_id, btif_get_avrcp_cb_channel_advOp(parms), btif_get_avrcp_cb_channel_state(parms), btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus);

            if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_PLAY_STATUS)
            {
                curr_device->avrcp_palyback_status = btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus;
                curr_device->a2dp_play_pause_flag = (curr_device->avrcp_palyback_status == 0x01) ? 1 : 0;
            }

            break;
         /*For Sony Compability Consideration*/
        case BTIF_AVRCP_EVENT_PANEL_PRESS:
            TRACE(3,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_PANEL_PRESS %02x %02x",
                device_id, btif_get_avrcp_panel_cnf(parms)->operation, btif_get_avrcp_panel_ind(parms)->operation);
            switch(btif_get_avrcp_panel_cnf(parms)->operation)
            {
                case BTIF_AVRCP_POP_VOLUME_UP:
                    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_UP, 0);
                    break;
                case BTIF_AVRCP_POP_VOLUME_DOWN:
                    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_DOWN, 0);
                    break;
                default :
                    break;
            }
            break;
        case BTIF_AVRCP_EVENT_PANEL_HOLD:
            TRACE(3,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_PANEL_HOLD %02x %02x",
               device_id, btif_get_avrcp_panel_cnf (parms)->operation, btif_get_avrcp_panel_ind(parms)->operation);
            break;
        case BTIF_AVRCP_EVENT_PANEL_RELEASE:
            TRACE(3,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_PANEL_RELEASE %02x %02x",
                device_id, btif_get_avrcp_panel_cnf (parms)->operation,btif_get_avrcp_panel_ind(parms)->operation);
            break;
         /*For Sony Compability Consideration End*/
        case BTIF_AVRCP_EVENT_PANEL_CNF:
            TRACE(4,"(d%x) ::AVRCP_EVENT_PANEL_CNF %02x %02x %02x",
                device_id, btif_get_avrcp_panel_cnf(parms)->operation, btif_get_avrcp_panel_cnf(parms)->press, btif_get_avrcp_panel_cnf(parms)->response);
            break;
        case BTIF_AVRCP_EVENT_ADV_TX_DONE://20
            TRACE(4,"(d%x) ::AVRCP_EVENT_ADV_TX_DONE op:%02x err_code:%d state:%d",
                device_id, btif_get_avrcp_cb_txPdu_Op(parms), btif_get_avrcp_cb_channel_error_code(parms), btif_get_avrcp_cb_channel_state(parms));
            break;
        case BTIF_AVRCP_EVENT_ADV_RESPONSE://18
            TRACE(4,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_ADV_RESPONSE role=%d op=%02x status=%d",
                device_id, btif_get_avrcp_channel_role(channel), btif_get_avrcp_cb_channel_advOp(parms), btif_get_avrcp_cb_channel_state(parms));

            if(btif_get_avrcp_cb_channel_advOp(parms)  == BTIF_AVRCP_OP_GET_CAPABILITIES && btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS)
            {
                TRACE(2,"(d%x) ::avrcp_callback_CT get_capabilities_rsp eventmask=%x", device_id, btif_get_avrcp_adv_rsp(parms)->capability.info.eventMask);

                btif_set_avrcp_adv_rem_event_mask(channel, btif_get_avrcp_adv_rsp(parms)->capability.info.eventMask);
                if(btif_get_avrcp_adv_rem_event_mask(channel) & BTIF_AVRCP_ENABLE_PLAY_STATUS_CHANGED)
                {
                    TRACE(1,"(d%x) ::avrcp_callback_CT get_capabilities_rsp support PLAY_STATUS_CHANGED", device_id);
                    btif_avrcp_ct_register_media_status_notification(channel, 0);
                    curr_device->avrcp_remote_support_playback_status_change_event = true;
                }
                if( btif_get_avrcp_adv_rem_event_mask( channel) & BTIF_AVRCP_ENABLE_PLAY_POS_CHANGED)
                {
#if 0
                    TRACE(1,"(d%x) ::avrcp_callback_CT get_capabilities_rsp support PLAY_POS_CHANGED", device_id);
                    btif_avrcp_ct_register_play_pos_notification(channel, 1);
#endif
                }
            }
            else if(btif_get_avrcp_cb_channel_advOp(parms)  == BTIF_AVRCP_OP_REGISTER_NOTIFY &&btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS)
            {
                btif_avrcp_ct_register_notify_response_check(channel, btif_get_avrcp_adv_notify(parms)->event);
                if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED)
                {
                    TRACE(2,"(d%x) ::avrcp_callback_CT NOTIFY RSP playback_changed_status =%x", device_id, btif_get_avrcp_adv_notify(parms)->p.mediaStatus);
                    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_NOTIFY_RSP, btif_get_avrcp_adv_notify(parms)->p.mediaStatus);
                    curr_device->avrcp_palyback_status = btif_get_avrcp_adv_notify(parms)->p.mediaStatus; // keep this line after app_bt_audio_event_handler
                    curr_device->a2dp_play_pause_flag = (curr_device->avrcp_palyback_status == 0x01) ? 1 : 0;
                }
                else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_PLAY_POS_CHANGED)
                {
                    TRACE(2,"(d%x) ::avrcp_callback_CT NOTIFY RSP playpos_changed_status =%x", device_id, btif_get_avrcp_adv_notify(parms)->p.position);
                }
#ifdef AVRCP_TRACK_CHANGED
                else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_TRACK_CHANGED){
                    TRACE(3,"(d%x) ::avrcp_callback_CT NOTIFY RSP track_changed_status msU32=%x, lsU32=%x", device_id,
                        btif_get_avrcp_adv_notify(parms)->p.track.msU32, btif_get_avrcp_adv_notify(parms)->p.track.lsU32);
                    curr_device->track_changed = BTIF_AVCTP_RESPONSE_INTERIM;
                    btif_avrcp_ct_get_media_Info(channel, 0x7f);
                }
#endif
            }
            else if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_PLAY_STATUS && btif_get_avrcp_cb_channel_state(parms)  == BT_STS_SUCCESS)
            {
                TRACE(2,"(d%x) ::avrcp_callback_CT get_play_status_rsp playback_changed_status =%d", device_id, btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus);
                curr_device->avrcp_palyback_status = btif_get_avrcp_adv_rsp_play_status(parms)->mediaStatus;
                curr_device->a2dp_play_pause_flag = (curr_device->avrcp_palyback_status == 0x01) ? 1 : 0;
            }
#ifdef AVRCP_TRACK_CHANGED
            else if(btif_get_avrcp_cb_channel_advOp(parms) == BTIF_AVRCP_OP_GET_MEDIA_INFO && btif_get_avrcp_cb_channel_state(parms) == BT_STS_SUCCESS){
                TRACE(2,"(d%x) ::avrcp_callback_CT get_media_info_rsp track_changed_status numid=%d", device_id, btif_get_avrcp_adv_rsp(parms)->element.numIds);
                for(uint8_t i=0;i<7;i++)
                {
                    if(btif_get_avrcp_adv_rsp(parms)->element.txt[i].length>0)
                    {
                        TRACE(2,"Id=%d,%s\n",i,btif_get_avrcp_adv_rsp(parms)->element.txt[i].string);
                    }
                }

            }
#endif
            break;
        case BTIF_AVRCP_EVENT_COMMAND:
            TRACE(8,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_COMMAND role%d ctype%02x subtp%02x subid%02x op%02x len%d more%d",
                    device_id, btif_get_avrcp_channel_role(channel),
                    btif_get_avrcp_cmd_frame(parms)->ctype,
                    btif_get_avrcp_cmd_frame(parms)->subunitType,
                    btif_get_avrcp_cmd_frame(parms)->subunitId,
                    btif_get_avrcp_cmd_frame(parms)->opcode,
                    btif_get_avrcp_cmd_frame(parms)->operandLen,
                    btif_get_avrcp_cmd_frame(parms)->more);
            if (btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVRCP_CTYPE_STATUS)
            {
                uint32_t company_id = *(btif_get_avrcp_cmd_frame(parms)->operands+2) + ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+1))<<8) + ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands))<<16);
                if(company_id == 0x001958)  //bt sig
                {
                    avrcp_operation_t op = *(btif_get_avrcp_cmd_frame(parms)->operands+3);
                    uint8_t oplen =  *(btif_get_avrcp_cmd_frame(parms)->operands+6)+ ((uint32_t)(*(btif_get_avrcp_cmd_frame(parms)->operands+5))<<8);
                    switch(op)
                    {
                        case BTIF_AVRCP_OP_GET_CAPABILITIES:
                        {
                            uint8_t event = *(btif_get_avrcp_cmd_frame(parms)->operands+7);
                            if(event==BTIF_AVRCP_CAPABILITY_COMPANY_ID)
                            {
                                TRACE(1,"(d%x) ::avrcp_callback_CT receive get_capabilities support compay id", device_id);
                                btif_avrcp_ct_send_capability_company_id_rsp(channel, btif_get_avrcp_cmd_frame(parms)->transId);
                            }
                            else if(event == BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED)
                            {
                                TRACE(2,"(d%x) ::avrcp_callback_CT receive get_capabilities support event transId:%d", device_id, btif_get_avrcp_cmd_frame(parms)->transId);
#ifdef __AVRCP_EVENT_COMMAND_VOLUME_SKIP__
                                 btif_set_avrcp_adv_rem_event_mask(channel, 0);
#else
                                 btif_set_avrcp_adv_rem_event_mask(channel, BTIF_AVRCP_ENABLE_VOLUME_CHANGED);
#endif
                                 btif_avrcp_ct_send_capability_rsp(channel, BTIF_AVRCP_CAPABILITY_EVENTS_SUPPORTED,
                                        btif_get_avrcp_adv_rem_event_mask(channel), btif_get_avrcp_cmd_frame(parms)->transId);
                            }
                            else
                            {
                                TRACE(1,"(d%x) ::avrcp_callback_CT receive get_capabilities error event value", device_id);
                            }
                        }
                        break;
                        default:
                        {
                            TRACE(2,"(d%x) ::avrcp_callback_CT receive AVRCP_EVENT_COMMAND unhandled op=%x,oplen=%x", device_id, op,oplen);
                        }
                        break;
                    }

                }
                else
                {
                    TRACE(2,"(d%x) ::avrcp_callback_CT receive AVRCP_EVENT_COMMAND unknown company_id=%x", device_id, company_id);
                }
            }
            else if(btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_CONTROL){
                if (btif_get_avrcp_cmd_frame(parms)->operands[3] == BTIF_AVRCP_OP_SET_ABSOLUTE_VOLUME){
                    uint8_t volume = 0;
                    uint8_t error_n = 0;
                    volume = btif_get_avrcp_cmd_frame(parms)->operands[7] & 0x7f;
                    TRACE(4,"(d%x) ::avrcp_callback_CT receive CONTROL set_absolute_volume %d %d%% transId:%d",
                        device_id, volume, ((int)volume)*100/128, btif_get_avrcp_cmd_frame(parms)->transId);
                    app_bt_a2dp_current_abs_volume_just_set(device_id, volume);
                    if((btif_get_avrcp_cmd_frame(parms)->operandLen != 8))//it works for BQB
                    {
                        error_n = BTIF_AVRCP_ERR_INVALID_PARM;
                        TRACE(1,"(d%x) ::avrcp_callback_CT reject invalid volume", device_id);
                    }
                    btif_avrcp_ct_send_absolute_volume_rsp(channel, btif_get_avrcp_cmd_frame(parms)->operands[7], btif_get_avrcp_cmd_frame(parms)->transId, error_n);
                } else if (BTIF_AVRCP_OP_CUSTOM_CMD == btif_get_avrcp_cmd_frame(parms)->operands[3]) {
                    TRACE(2,"(d%x) ::avrcp_callback_CT receive CONTROL CUSTOM_CMD transId:%d", device_id, btif_get_avrcp_cmd_frame(parms)->transId);
                    app_AVRCP_CustomCmd_Received(&btif_get_avrcp_cmd_frame(parms)->operands[7], btif_get_avrcp_cmd_frame(parms)->operandLen - 7);
                    app_AVRCP_sendCustomCmdRsp(device_id, channel, true,btif_get_avrcp_cmd_frame(parms)->transId);
                }
            }
            else if (btif_get_avrcp_cmd_frame(parms)->ctype == BTIF_AVCTP_CTYPE_NOTIFY){
                bt_status_t status;
                if (btif_get_avrcp_cmd_frame(parms)->operands[7] == BTIF_AVRCP_EID_VOLUME_CHANGED){
                    TRACE(2,"(d%x) ::avrcp_callback_CT receive REGISTER volume_changed_status NOTIFY transId:%d", device_id, btif_get_avrcp_cmd_frame(parms)->transId);
                    curr_device->volume_report = BTIF_AVCTP_RESPONSE_INTERIM;
                    status = btif_avrcp_ct_send_volume_change_interim_rsp(channel, curr_device->a2dp_current_abs_volume, btif_get_avrcp_cmd_frame(parms)->transId);
                }
                else if(btif_get_avrcp_cmd_frame(parms)->operands[7] == 0xff)//it works for BQB
                {
                    status = btif_avrcp_ct_send_invalid_volume_rsp(channel, btif_get_avrcp_cmd_frame(parms)->transId);
                    TRACE(3,"(d%x) ::avrcp_callback_CT receive invalid op=0xff tran id:%d status:%d", device_id, btif_get_avrcp_cmd_frame(parms)->transId, status);
                }
            }
            break;
        case BTIF_AVRCP_EVENT_ADV_NOTIFY://17
            if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_VOLUME_CHANGED)
            {
                 TRACE(2,"(d%x) ::avrcp_callback_CT NOTIFY volume_changed_status =%x", device_id, btif_get_avrcp_adv_notify(parms)->p.volume);
                 btif_avrcp_ct_register_volume_change_notification(channel, 0);
            }
            else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_MEDIA_STATUS_CHANGED)
            {
                TRACE(2,"(d%x) ::avrcp_callback_CT NOTIFY playback_changed_status =%x", device_id, btif_get_avrcp_adv_notify(parms)->p.mediaStatus);
                curr_device->avrcp_palyback_status = btif_get_avrcp_adv_notify(parms)->p.mediaStatus;
                curr_device->a2dp_play_pause_flag = (curr_device->avrcp_palyback_status == 0x01) ? 1 : 0;
                btif_avrcp_ct_register_media_status_notification(channel, 0);
                app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_CHANGED, curr_device->avrcp_palyback_status);
            }
            else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_PLAY_POS_CHANGED)
            {
                TRACE(2,"(d%x) ::avrcp_callback_CT NOTIFY playpos_changed_status =%x", device_id, btif_get_avrcp_adv_notify(parms)->p.position);
                btif_avrcp_ct_register_play_pos_notification(channel, 1);
            }
#ifdef AVRCP_TRACK_CHANGED
            else if(btif_get_avrcp_adv_notify(parms)->event == BTIF_AVRCP_EID_TRACK_CHANGED){
                TRACE(3,"(d%x) ::avrcp_callback_CT NOTIFY track_changed_status msU32=%x, lsU32=%x", device_id,
                    btif_get_avrcp_adv_notify(parms)->p.track.msU32, btif_get_avrcp_adv_notify(parms)->p.track.lsU32);
                btif_avrcp_ct_register_track_change_notification(channel, 0);
            }
#endif
            break;
        case BTIF_AVRCP_EVENT_ADV_CMD_TIMEOUT:
            TRACE(2,"(d%x) ::avrcp_callback_CT AVRCP_EVENT_ADV_CMD_TIMEOUT role=%d", device_id, btif_get_avrcp_cb_channel_role(channel));
            break;

    }

#if defined(IBRT)
    app_tws_ibrt_profile_callback(device_id, BTIF_APP_AVRCP_PROFILE_ID,(void *)btif_avrcp, (void *)parms,&curr_device->remote);
#endif

}

int a2dp_audio_sbc_set_frame_info(int rcv_len, int frame_num);

void btapp_send_pause_key(enum BT_DEVICE_ID_T stream_id)
{
    TRACE(1,"btapp_send_pause_key id = %x",stream_id);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(stream_id);
    btif_avrcp_set_panel_key(curr_device->avrcp_channel,BTIF_AVRCP_POP_PAUSE,TRUE);
    btif_avrcp_set_panel_key(curr_device->avrcp_channel,BTIF_AVRCP_POP_PAUSE,FALSE);
}

void btapp_a2dp_suspend_music(enum BT_DEVICE_ID_T stream_id)
{
    TRACE(1,"btapp_a2dp_suspend_music id = %x",stream_id);
    btapp_send_pause_key(stream_id);
}

extern enum AUD_SAMPRATE_T a2dp_sample_rate;

#define A2DP_TIMESTAMP_TRACE(s,...)

#define A2DP_TIMESTAMP_DEBOUNCE_DURATION (1000)
#define A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD (2000)

#define A2DP_TIMESTAMP_SYNC_LIMIT_CNT (100)
#define A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD (60)
#define A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD ((int64_t)a2dp_sample_rate*A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD/1000)

#define RICE_THRESHOLD
#define RICE_THRESHOLD

struct A2DP_TIMESTAMP_INFO_T{
    uint16_t rtp_timestamp;
    uint32_t loc_timestamp;
    uint16_t frame_num;
    int32_t rtp_timestamp_diff_sum;
};

enum A2DP_TIMESTAMP_MODE_T{
    A2DP_TIMESTAMP_MODE_NONE,
    A2DP_TIMESTAMP_MODE_SAMPLE,
    A2DP_TIMESTAMP_MODE_TIME,
};

enum A2DP_TIMESTAMP_MODE_T a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;

struct A2DP_TIMESTAMP_INFO_T a2dp_timestamp_pre = {0,0,0};
bool a2dp_timestamp_parser_need_sync = false;

int a2dp_timestamp_parser_init(void)
{
    a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_NONE;
    a2dp_timestamp_pre.rtp_timestamp = 0;
    a2dp_timestamp_pre.loc_timestamp = 0;
    a2dp_timestamp_pre.frame_num = 0;
    a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
    a2dp_timestamp_parser_need_sync = false;
    return 0;
}

int a2dp_timestamp_parser_needsync(void)
{
    a2dp_timestamp_parser_need_sync = true;
    return 0;
}

int a2dp_timestamp_parser_run(uint16_t timestamp, uint16_t framenum)
{
    static int skip_cnt = 0;
    struct A2DP_TIMESTAMP_INFO_T curr_timestamp;
    int skipframe = 0;
    uint16_t rtpdiff;
    int32_t locdiff;
    bool needsave_rtp_timestamp = true;
    bool needsave_loc_timestamp = true;

    curr_timestamp.rtp_timestamp = timestamp;
    curr_timestamp.loc_timestamp = hal_sys_timer_get();
    curr_timestamp.frame_num = framenum;

    switch(a2dp_timestamp_mode) {
        case A2DP_TIMESTAMP_MODE_NONE:

//            TRACE(5,"parser rtp:%d loc:%d num:%d prertp:%d preloc:%d\n", curr_timestamp.rtp_timestamp, curr_timestamp.loc_timestamp, curr_timestamp.frame_num,
//                                                   a2dp_timestamp_pre.rtp_timestamp, a2dp_timestamp_pre.loc_timestamp);
            if (a2dp_timestamp_pre.rtp_timestamp){
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                if (TICKS_TO_MS(locdiff) > A2DP_TIMESTAMP_DEBOUNCE_DURATION){
                    rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                    if (ABS((int16_t)TICKS_TO_MS(locdiff)-rtpdiff)>A2DP_TIMESTAMP_MODE_SAMPLE_THRESHOLD){
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_SAMPLE;
                        TRACE(0,"A2DP_TIMESTAMP_MODE_SAMPLE\n");
                    }else{
                        a2dp_timestamp_mode = A2DP_TIMESTAMP_MODE_TIME;
                        TRACE(0,"A2DP_TIMESTAMP_MODE_TIME\n");
                    }
                }else{
                    needsave_rtp_timestamp = false;
                    needsave_loc_timestamp = false;
                }
            }
            break;
        case A2DP_TIMESTAMP_MODE_SAMPLE:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                a2dp_timestamp_pre.rtp_timestamp_diff_sum += rtpdiff;

                A2DP_TIMESTAMP_TRACE(3,"%d-%d=%d",  curr_timestamp.rtp_timestamp, a2dp_timestamp_pre.rtp_timestamp, rtpdiff);

                A2DP_TIMESTAMP_TRACE(3,"%d-%d=%d",  curr_timestamp.loc_timestamp , a2dp_timestamp_pre.loc_timestamp, locdiff);

                A2DP_TIMESTAMP_TRACE(3,"%d-%d=%d", (int32_t)((int64_t)(TICKS_TO_MS(locdiff))*(uint32_t)a2dp_sample_rate/1000),
                                     a2dp_timestamp_pre.rtp_timestamp_diff_sum,
                                     (int32_t)((TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum));


                A2DP_TIMESTAMP_TRACE(2,"A2DP_TIMESTAMP_MODE_SAMPLE SYNC diff:%d cnt:%d\n", (int32_t)((int64_t)(TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum), skip_cnt);
                if (((int64_t)(TICKS_TO_MS(locdiff)*a2dp_sample_rate/1000) - a2dp_timestamp_pre.rtp_timestamp_diff_sum) < (int32_t)A2DP_TIMESTAMP_SYNC_SAMPLE_THRESHOLD){
                    TRACE(1,"A2DP_TIMESTAMP_MODE_SAMPLE RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){
                    TRACE(0,"A2DP_TIMESTAMP_MODE_SAMPLE RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else{
                    needsave_loc_timestamp = false;
                    skipframe = 1;
                }
            }else{
                a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
            }
            break;
        case A2DP_TIMESTAMP_MODE_TIME:
            if (a2dp_timestamp_parser_need_sync){
                skip_cnt++;
                rtpdiff = curr_timestamp.rtp_timestamp - a2dp_timestamp_pre.rtp_timestamp;
                locdiff = curr_timestamp.loc_timestamp - a2dp_timestamp_pre.loc_timestamp;
                a2dp_timestamp_pre.rtp_timestamp_diff_sum += rtpdiff;

                A2DP_TIMESTAMP_TRACE(5,"%d/%d/ %d/%d %d\n",  rtpdiff,a2dp_timestamp_pre.rtp_timestamp_diff_sum,
                                            a2dp_timestamp_pre.loc_timestamp, curr_timestamp.loc_timestamp,
                                            TICKS_TO_MS(locdiff));
                A2DP_TIMESTAMP_TRACE(2,"A2DP_TIMESTAMP_MODE_TIME SYNC diff:%d cnt:%d\n", (int32_t)ABS(TICKS_TO_MS(locdiff) - a2dp_timestamp_pre.rtp_timestamp_diff_sum), skip_cnt);
                if (((int64_t)TICKS_TO_MS(locdiff) - a2dp_timestamp_pre.rtp_timestamp_diff_sum) < A2DP_TIMESTAMP_SYNC_TIME_THRESHOLD){
                    TRACE(1,"A2DP_TIMESTAMP_MODE_TIME RESYNC OK cnt:%d\n", skip_cnt);
                    skip_cnt = 0;
                    needsave_loc_timestamp = false;
                    a2dp_timestamp_parser_need_sync = false;
                }else if (skip_cnt > A2DP_TIMESTAMP_SYNC_LIMIT_CNT){
                    TRACE(0,"A2DP_TIMESTAMP_MODE_TIME RESYNC FORCE END\n");
                    skip_cnt = 0;
                    a2dp_timestamp_parser_need_sync = false;
                }else{
                    needsave_loc_timestamp = false;
                    skipframe = 1;
                }
            }else{
                a2dp_timestamp_pre.rtp_timestamp_diff_sum = 0;
            }
            break;
    }

    if (needsave_rtp_timestamp){
        a2dp_timestamp_pre.rtp_timestamp = curr_timestamp.rtp_timestamp;
    }

    if (needsave_loc_timestamp){
        a2dp_timestamp_pre.loc_timestamp = curr_timestamp.loc_timestamp;
    }

    return skipframe;
}

#if defined(A2DP_LHDC_ON)

uint8_t bt_sbc_player_get_bitsDepth(void)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(app_bt_audio_get_curr_a2dp_device());
    return curr_device->sample_bit;
}

uint32_t lhdc_ext_flags;
#define LHDC_EXT_FLAGS_JAS    0x01
#define LHDC_EXT_FLAGS_AR     0x02
#define LHDC_EXT_FLAGS_LLAC   0x04
#define LHDC_EXT_FLAGS_MQA    0x08
#define LHDC_EXT_FLAGS_MBR    0x10
#define LHDC_EXT_FLAGS_LARC   0x20
#define LHDC_EXT_FLAGS_V4     0x40
#define LHDC_SET_EXT_FLAGS(X) (lhdc_ext_flags |= X)
#define LHDC_CLR_EXT_FLAGS(X) (lhdc_ext_flags &= ~X)
#define LHDC_CLR_ALL_EXT_FLAGS() (lhdc_ext_flags = 0)

//uint32_t llac_supported_bitrate;
void a2dp_lhdc_config(uint8_t device_id, uint8_t * elements)
{
    //uint8_t * elements = &(Info->p.configReq->codec.elements[0]);
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t config = elements[6];
    TRACE(4,"codecType: LHDC Codec, config value=0x%02x, elements[6]=0x%02x [7]=0x%02x [8]=0x%02x\n", 
            A2DP_LHDC_SR_DATA(config), elements[6], elements[7], elements[8]);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if (vendor_id == A2DP_LHDC_VENDOR_ID && codec_id == A2DP_LHDC_CODEC_ID) {
        TRACE(2,"Vendor ID = 0x%08x, Codec ID = 0x%04x, LHDC Codec\n", vendor_id, codec_id);
        switch (A2DP_LHDC_SR_DATA(config)) {
            case A2DP_LHDC_SR_96000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_96;
            TRACE(1,"%s:CodecCfg sample_rate 96000\n", __func__);
            break;
            case A2DP_LHDC_SR_48000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_48;
            TRACE(1,"%s:CodecCfg sample_rate 48000\n", __func__);
            break;
            case A2DP_LHDC_SR_44100:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
            TRACE(1,"%s:CodecCfg sample_rate 44100\n", __func__);
            break;
        }
        switch (A2DP_LHDC_FMT_DATA(config)) {
            case A2DP_LHDC_FMT_16:
            curr_device->sample_bit = 16;
            TRACE(1,"%s:CodecCfg bits per sampe = 16", __func__);
            break;
            case A2DP_LHDC_FMT_24:
            TRACE(1,"%s:CodecCfg bits per sampe = 24", __func__);
            curr_device->sample_bit = 24;
            break;
        }
        
        LHDC_CLR_ALL_EXT_FLAGS();

        if (config & A2DP_LHDC_JAS)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_JAS);

        if (config & A2DP_LHDC_AR)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_AR);

        if (elements[7] & A2DP_LHDC_LLAC)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_LLAC);

        if (elements[8] & A2DP_LHDC_MQA)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_MQA);

        if (elements[8] & A2DP_LHDC_MinBR)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_MBR);

        if (elements[8] & A2DP_LHDC_LARC)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_LARC);

        if (elements[8] & A2DP_LHDC_V4)
            LHDC_SET_EXT_FLAGS(LHDC_EXT_FLAGS_V4);

        curr_device->lhdc_ext_flags = lhdc_ext_flags;
        TRACE(2,"%s:EXT flags = 0x%08x", __func__, curr_device->lhdc_ext_flags);
        if (elements[7]&A2DP_LHDC_LLC_ENABLE){
            curr_device->a2dp_lhdc_llc = true;;
        }else{
            curr_device->a2dp_lhdc_llc = false;;
        }
    }
}

uint8_t a2dp_lhdc_config_llc_get(void)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    return app_bt_get_device(device_id)->a2dp_lhdc_llc;
}

uint32_t a2dp_lhdc_config_bitrate_get(void)
{
    return 400000;//llac_supported_bitrate; 
}

bool a2dp_lhdc_get_ext_flags(uint32_t flags)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    TRACE(3,"%s:EXT flags = 0x%08x, flag = 0x%02x", __func__, curr_device->lhdc_ext_flags, (uint8_t)flags);
    return ((curr_device->lhdc_ext_flags & flags) == flags);
}

#endif

#if defined(A2DP_SCALABLE_ON)
void a2dp_scalable_config(uint8_t device_id, uint8_t * elements){
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t config = elements[6];
    TRACE(2,"##codecType: Scalable Codec, config value = 0x%02x, elements[6]=0x%02x\n", A2DP_SCALABLE_SR_DATA(config), elements[6]);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if (vendor_id == A2DP_SCALABLE_VENDOR_ID && codec_id == A2DP_SCALABLE_CODEC_ID) {
        TRACE(2,"Vendor ID = 0x%08x, Codec ID = 0x%04x, Scalable Codec\n", vendor_id, codec_id);
        switch (A2DP_SCALABLE_SR_DATA(config)) {
            case A2DP_SCALABLE_SR_96000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_96;
            TRACE(1,"%s:CodecCfg sample_rate 96000\n", __func__);
            break;
            case A2DP_SCALABLE_SR_48000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_48;
            TRACE(1,"%s:CodecCfg sample_rate 48000\n", __func__);
            break;
            case A2DP_SCALABLE_SR_44100:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
            TRACE(1,"%s:CodecCfg sample_rate 44100\n", __func__);
            break;
            case A2DP_SCALABLE_SR_32000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_32;
            TRACE(1,"%s:CodecCfg sample_rate 32000\n", __func__);
            break;
        }
        switch (A2DP_SCALABLE_FMT_DATA(config)) {
            case A2DP_SCALABLE_FMT_16:
            curr_device->sample_bit = 16;
            TRACE(1,"%s:CodecCfg bits per sampe = 16", __func__);
            break;
            case A2DP_SCALABLE_FMT_24:
            curr_device->sample_bit = 24;
            TRACE(1,"%s:CodecCfg bits per sampe = 24", __func__);
            if (curr_device->sample_rate != A2D_SBC_IE_SAMP_FREQ_96) {
                curr_device->sample_bit = 16;
                TRACE(1,"%s:CodeCfg reset bit per sample to 16 when samplerate is not 96k", __func__);
            }
            break;
        }
    }
}
#endif

#if defined(A2DP_LDAC_ON)
int ldac_decoder_sf = 0;
int ldac_decoder_cm = 0;
void a2dp_ldac_config(uint8_t device_id, uint8_t * elements){
    //uint8_t * elements = &(Info->p.configReq->codec.elements[0]);
    uint32_t vendor_id = (uint32_t)elements[0];
    vendor_id |= ((uint32_t)elements[1]) << 8;
    vendor_id |= ((uint32_t)elements[2]) << 16;
    vendor_id |= ((uint32_t)elements[3]) << 24;
    uint16_t codec_id = (uint16_t)elements[4];
    codec_id |= ((uint16_t)elements[5]) << 8;
    uint8_t sf_config = elements[6];
    uint8_t cm_config = elements[7];
    TRACE(2,"##codecType: LDAC Codec, config value = 0x%02x, elements[6]=0x%02x\n", A2DP_LDAC_SR_DATA(sf_config), elements[6]);
    TRACE(2,"##codecType: LDAC Codec, config value = 0x%02x, elements[7]=0x%02x\n", A2DP_LDAC_CM_DATA(cm_config), elements[7]);
    TRACE(2,"Vendor ID = 0x%08x, Codec ID = 0x%04x, LHDC Codec\n", vendor_id, codec_id);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if (vendor_id == A2DP_LDAC_VENDOR_ID && codec_id == A2DP_LDAC_CODEC_ID) {
        switch (A2DP_LDAC_SR_DATA(sf_config)) {
            case A2DP_LDAC_SR_96000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_96;
            ldac_decoder_sf = 96000;
            TRACE(1,"%s:ldac CodecCfg sample_rate 96000\n", __func__);
            break;
            case A2DP_LDAC_SR_48000:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_48;
            ldac_decoder_sf = 48000;
            TRACE(1,"%s:ldac CodecCfg sample_rate 48000\n", __func__);
            break;
            case A2DP_LDAC_SR_44100:
            curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
            ldac_decoder_sf = 44100;
            TRACE(1,"%sldac :CodecCfg sample_rate 44100\n", __func__);
            break;
        }
        switch (A2DP_LDAC_CM_DATA(cm_config)) {
            case A2DP_LDAC_CM_MONO:
            curr_device->channel_mode = LDACBT_CHANNEL_MODE_MONO;
            TRACE(1,"%s:ldac CodecCfg A2DP_LDAC_CM_MONO", __func__);
            break;
            case A2DP_LDAC_CM_DUAL:
            TRACE(1,"%s:ldac CodecCfg A2DP_LDAC_CM_DUAL", __func__);
            curr_device->channel_mode = LDACBT_CHANNEL_MODE_DUAL_CHANNEL;
            break;
            case A2DP_LDAC_CM_STEREO:
            TRACE(1,"%s:ldac ldac CodecCfg A2DP_LDAC_CM_STEREO", __func__);
            curr_device->channel_mode = LDACBT_CHANNEL_MODE_STEREO;
            break;
        }
    }
}

void app_ibrt_restore_ldac_info(uint8_t device_id, uint8_t sample_freq)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    curr_device->channel_mode=LDACBT_CHANNEL_MODE_STEREO;
    switch (sample_freq)
    {
        case A2D_SBC_IE_SAMP_FREQ_96:
            ldac_decoder_sf = 96000;
        break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            ldac_decoder_sf = 48000;
        break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            ldac_decoder_sf = 44100;
        break;
    }
}


int channel_mode;
int bt_ldac_player_get_channelmode(void){
    struct BT_DEVICE_T* curr_device = app_bt_get_device(app_bt_audio_get_curr_a2dp_device());
     if (curr_device->channel_mode != channel_mode) {
        /* code */
        channel_mode = curr_device->channel_mode;
    }
    return channel_mode;
}
int bt_get_ladc_sample_rate(void){

    return ldac_decoder_sf;
}
#endif


#if defined(IBRT)
a2dp_stream_t * app_bt_get_mobile_a2dp_stream(uint32_t deviceId);
int app_bt_stream_ibrt_audio_mismatch_stopaudio(uint8_t device_id);
void app_bt_stream_ibrt_audio_mismatch_resume(uint8_t device_id);
#if defined(IBRT_V2_MULTIPOINT)
int a2dp_ibrt_sync_get_status_v2(uint8_t device_id, ibrt_mobile_info_t *mobile_info, ibrt_a2dp_status_t *a2dp_status);
#endif

int a2dp_ibrt_session_reset(uint8_t devId)
{
    app_bt_get_device(devId)->a2dp_session = 0;
    return 0;
}

int a2dp_ibrt_session_new(uint8_t devId)
{
    app_bt_get_device(devId)->a2dp_session++;
    return 0;
}

int a2dp_ibrt_session_set(uint8_t session,uint8_t devId)
{
    app_bt_get_device(devId)->a2dp_session = session;
    return 0;
}

uint32_t a2dp_ibrt_session_get(uint8_t devId)
{
    return app_bt_get_device(devId)->a2dp_session;
}

static int a2dp_ibrt_autotrigger_flag = 0;
int a2dp_ibrt_stream_need_autotrigger_set_flag(void)
{
    a2dp_ibrt_autotrigger_flag = 1;
    return 0;
}

int a2dp_ibrt_stream_need_autotrigger_getandclean_flag(void)
{
    uint32_t flag;
#if defined(IBRT_A2DP_TRIGGER_BY_MYSELF)
    flag = a2dp_ibrt_autotrigger_flag;
    a2dp_ibrt_autotrigger_flag = 0;
#else
    a2dp_ibrt_autotrigger_flag = 0;
    flag = 1;
#endif
    return flag;
}

int a2dp_ibrt_sync_get_status(uint8_t device_id, ibrt_a2dp_status_t *a2dp_status)
{
#if defined(IBRT_V2_MULTIPOINT)
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    ibrt_mobile_info_t *mobile_info = app_ibrt_conn_get_mobile_info_by_addr(&curr_device->remote);

    if (mobile_info != NULL)
    {
        return a2dp_ibrt_sync_get_status_v2(device_id, mobile_info, a2dp_status);
    }
    else
    {
        a2dp_status->state = BTIF_AVDTP_STRM_STATE_IDLE;
        TRACE(1,"%s,mobile info null",__func__);
        return 0;
    }
#else
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
    a2dp_status->codec           = p_ibrt_ctrl->a2dp_codec;
    a2dp_status->localVolume = a2dp_volume_local_get(BT_DEVICE_ID_1);
    a2dp_status->state           = btif_a2dp_get_stream_state(app_bt_get_mobile_a2dp_stream(BT_DEVICE_ID_1));
    a2dp_status->latency_factor  = a2dp_audio_latency_factor_get();
    a2dp_status->session         = a2dp_ibrt_session_get(device_id);
    TRACE(4,"%s,sync a2dp stream ac = %p ; stream_status = %d ; codec_type =  %d ",__func__,app_bt_get_device(BT_DEVICE_ID_1)->a2dp_connected_stream,a2dp_status->state,a2dp_status->codec.codec_type);
    return 0;
#endif
}

#ifdef IBRT_UI_V1
int a2dp_ibrt_sync_set_status(uint8_t device_id, ibrt_a2dp_status_t *a2dp_status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    a2dp_stream_t *Stream = curr_device->btif_a2dp_stream->a2dp_stream;
    btif_avdtp_stream_state_t old_avdtp_stream_state = btif_a2dp_get_stream_state(Stream);

    btif_a2dp_callback_parms_t info;
    btif_avdtp_config_request_t avdtp_config_req;

    TRACE(5,"%s,stream_state:[%d]->[%d], codec_type:%d localVolume:%d", __func__, old_avdtp_stream_state, a2dp_status->state, a2dp_status->codec.codec_type, a2dp_status->localVolume);

    if ((!(APP_IBRT_UI_GET_MOBILE_CONNSTATE(&curr_device->remote) & BTIF_APP_A2DP_PROFILE_ID)) &&
        (!(APP_IBRT_UI_GET_IBRT_CONNSTATE(&curr_device->remote) & BTIF_APP_A2DP_PROFILE_ID)))
    {
        TRACE(1,"%s,a2dp profile not connected",__func__);
        return 1;
    }

    btif_a2dp_set_codec_info(device_id, (uint8_t*)&a2dp_status->codec);
    info.event = BTIF_A2DP_EVENT_STREAM_OPEN_MOCK;
    app_tws_ibrt_set_a2dp_codec((a2dp_callback_parms_t *)&info);
    app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
    a2dp_volume_set_local_vol((enum BT_DEVICE_ID_T)device_id, a2dp_status->localVolume);
    a2dp_audio_latency_factor_set(a2dp_status->latency_factor);
    a2dp_ibrt_session_set(a2dp_status->session,device_id);

    if (a2dp_status->state != old_avdtp_stream_state){
        switch(a2dp_status->state) {
            case BTIF_AVDTP_STRM_STATE_STREAMING:
                app_bt_clear_connecting_profiles_state(device_id);
                a2dp_timestamp_parser_init();
                btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                info.event = BTIF_A2DP_EVENT_STREAM_STARTED_MOCK;
                TRACE(0,"::A2DP_EVENT_STREAM_STARTED mock");
                a2dp_ibrt_stream_need_autotrigger_set_flag();
#if defined(IBRT_FORCE_AUDIO_RETRIGGER)
                a2dp_callback(device_id, Stream, (a2dp_callback_parms_t *)&info);
                app_ibrt_if_force_audio_retrigger();
#else
//                app_bt_stream_ibrt_audio_mismatch_resume();
                a2dp_callback(device_id, Stream, (a2dp_callback_parms_t *)&info);
#endif
                break;
            case BTIF_AVDTP_STRM_STATE_OPEN:
                //Ignore START->OPEN transition since itslef can received SUSPEND CMD
                if (old_avdtp_stream_state != BTIF_AVDTP_STRM_STATE_STREAMING)
                {
                    TRACE(0,"::A2DP_EVENT_STREAM_OPEN mock");
                    btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                    info.event = BTIF_A2DP_EVENT_STREAM_OPEN_MOCK;
                    info.p.configReq = &avdtp_config_req;
                    info.p.configReq->codec.codecType = a2dp_status->codec.codec_type;
                    info.p.configReq->codec.elements = ((uint8_t*)btif_a2dp_get_stream_codec(Stream))+2;
                    a2dp_callback(device_id, Stream, (a2dp_callback_parms_t *)&info);
                }
                break;
            default:
                if (btif_a2dp_get_stream_state(Stream) != BTIF_AVDTP_STRM_STATE_IDLE){
                    TRACE(0,"::A2DP_EVENT_STREAM_SUSPENDED mock");
                    btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                    info.event = BTIF_A2DP_EVENT_STREAM_SUSPENDED;
                    a2dp_callback(device_id, Stream, (a2dp_callback_parms_t *)&info);
                }
                break;
        }
    }
    return 0;
}

#else

int a2dp_ibrt_sync_get_status_v2(uint8_t device_id, ibrt_mobile_info_t *mobile_info, ibrt_a2dp_status_t *a2dp_status)
{
    a2dp_status->avrcp_play_status = app_bt_get_device(device_id)->avrcp_palyback_status;
    a2dp_status->codec           = mobile_info->a2dp_codec;
    a2dp_status->localVolume     = a2dp_volume_local_get((enum BT_DEVICE_ID_T)device_id);
    a2dp_status->state           = btif_a2dp_get_stream_state(app_bt_get_mobile_a2dp_stream(device_id));
    a2dp_status->latency_factor  = a2dp_audio_latency_factor_get();
    a2dp_status->session         = a2dp_ibrt_session_get(device_id);
    return 0;
}

int a2dp_ibrt_sync_set_status_v2(uint8_t device_id, ibrt_mobile_info_t *mobile_info, ibrt_a2dp_status_t *a2dp_status)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    a2dp_stream_t *Stream = curr_device->btif_a2dp_stream->a2dp_stream;
    btif_avdtp_stream_state_t old_avdtp_stream_state = btif_a2dp_get_stream_state(Stream);
    btif_a2dp_callback_parms_t info;
    btif_avdtp_config_request_t avdtp_config_req;

    TRACE(5,"%s,stream_state:[%d]->[%d], codec_type:%d localVolume:%d", __func__, old_avdtp_stream_state, a2dp_status->state, a2dp_status->codec.codec_type, a2dp_status->localVolume);

    if ((!(APP_IBRT_UI_GET_MOBILE_CONNSTATE(&curr_device->remote) & BTIF_APP_A2DP_PROFILE_ID)) &&
        (!(APP_IBRT_UI_GET_IBRT_CONNSTATE(&curr_device->remote) & BTIF_APP_A2DP_PROFILE_ID)))
    {
        TRACE(1,"%s,a2dp profile not connected",__func__);
        return 1;
    }

    btif_a2dp_set_codec_info(device_id, (uint8_t*)&a2dp_status->codec);
    info.event = BTIF_A2DP_EVENT_STREAM_OPEN_MOCK;
    app_tws_ibrt_set_a2dp_codec_v2(mobile_info, (a2dp_callback_parms_t*)&info);
    app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
    a2dp_volume_set_local_vol((enum BT_DEVICE_ID_T)device_id, a2dp_status->localVolume);
    a2dp_audio_latency_factor_set(a2dp_status->latency_factor);
    a2dp_ibrt_session_set(a2dp_status->session,device_id);

    if (curr_device->avrcp_palyback_status != a2dp_status->avrcp_play_status)
    {
        curr_device->avrcp_palyback_status = a2dp_status->avrcp_play_status;
        app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_AVRCP_PLAY_STATUS_MOCK, curr_device->avrcp_palyback_status);
    }

    if (a2dp_status->state != old_avdtp_stream_state){
        switch(a2dp_status->state) {
            case BTIF_AVDTP_STRM_STATE_STREAMING:
                app_bt_clear_connecting_profiles_state(device_id);
                a2dp_timestamp_parser_init();
                btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                info.event = BTIF_A2DP_EVENT_STREAM_STARTED_MOCK;
                TRACE(0,"::A2DP_EVENT_STREAM_STARTED mock");
                a2dp_ibrt_stream_need_autotrigger_set_flag();
#if defined(IBRT_FORCE_AUDIO_RETRIGGER)
                a2dp_callback(device_id, Stream, &info);
                app_ibrt_if_force_audio_retrigger();
#else
//                app_bt_stream_ibrt_audio_mismatch_resume();
                a2dp_callback(device_id, Stream, (a2dp_callback_parms_t*)&info);
#endif
                break;
            case BTIF_AVDTP_STRM_STATE_OPEN:
                //Ignore START->OPEN transition since itslef can received SUSPEND CMD
                if (old_avdtp_stream_state != BTIF_AVDTP_STRM_STATE_STREAMING)
                {
                    TRACE(0,"::A2DP_EVENT_STREAM_OPEN mock");
                    btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                    info.event = BTIF_A2DP_EVENT_STREAM_OPEN_MOCK;
                    info.p.configReq = &avdtp_config_req;
                    info.p.configReq->codec.codecType = a2dp_status->codec.codec_type;
                    info.p.configReq->codec.elements = ((uint8_t*)btif_a2dp_get_stream_codec(Stream))+2;
                    a2dp_callback(device_id, Stream, (a2dp_callback_parms_t*)&info);
                }
                break;
            default:
                if (btif_a2dp_get_stream_state(Stream) != BTIF_AVDTP_STRM_STATE_IDLE){
                    TRACE(0,"::A2DP_EVENT_STREAM_SUSPENDED mock");
                    btif_a2dp_set_stream_state(Stream, a2dp_status->state);
                    info.event = BTIF_A2DP_EVENT_STREAM_SUSPENDED;
                    a2dp_callback(device_id, Stream, (a2dp_callback_parms_t*)&info);
                }
                break;
        }
    }
    return 0;
}
#endif

int a2dp_ibrt_stream_open_mock(uint8_t device_id, btif_remote_device_t *remDev)
{
    TRACE(0,"::A2DP_EVENT_STREAM_OPEN mock");

    btif_a2dp_callback_parms_t info;
    btif_avdtp_config_request_t avdtp_config_req;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    info.event = BTIF_A2DP_EVENT_STREAM_OPEN_MOCK;
    info.p.configReq = &avdtp_config_req;
    info.p.configReq->codec.codecType = curr_device->codec_type;
    info.p.configReq->codec.elements = ((uint8_t*)btif_a2dp_get_stream_codec(curr_device->btif_a2dp_stream->a2dp_stream))+2;
    a2dp_callback(device_id, curr_device->btif_a2dp_stream->a2dp_stream, (a2dp_callback_parms_t*)&info);

    if (remDev){
        btdevice_profile *btdevice_plf_p = NULL;
        btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(btif_me_get_remote_device_bdaddr(remDev)->address);
        nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_plf_p, true);
        nv_record_btdevicerecord_set_a2dp_profile_codec(btdevice_plf_p, curr_device->codec_type);
        app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(remDev));
        TRACE(2,"::A2DP_EVENT_STREAM_OPEN mock codec_type:%d vol:%d", curr_device->codec_type, app_bt_stream_volume_get_ptr()->a2dp_vol);
    }else{
        TRACE(0,"::A2DP_EVENT_STREAM_OPEN mock no find remDev");
        app_bt_stream_volume_ptr_update(NULL);
    }

    return 0;
}

#define A2DP_IBRT_STREAM_SKIP_TWS_SNIFF_STATUS         (1)
int a2dp_ibrt_stream_event_stream_data_ind_needskip(uint8_t device_id, a2dp_stream_t *Stream)
{
    int nRet = 0;
    ibrt_ctrl_t  *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
        if (APP_IBRT_UI_GET_MOBILE_MODE(&curr_device->remote) == IBRT_SNIFF_MODE){
            TRACE(0,"::A2DP_EVENT_STREAM_DATA_IND ibrt_link skip (mobile) skip sniff\n");
            nRet = 1;
        }
        if (app_tws_ibrt_tws_link_connected()){
            if (APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote) &&
                p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE){
                TRACE(0,"::A2DP_EVENT_STREAM_DATA_IND mobile_link (tws) skip sniff\n");
#ifndef A2DP_IBRT_STREAM_SKIP_TWS_SNIFF_STATUS
                nRet = 1;
#endif
            }
        }
    }else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
        if (APP_IBRT_UI_GET_MOBILE_MODE(&curr_device->remote) == IBRT_SNIFF_MODE){
            TRACE(0,"::A2DP_EVENT_STREAM_DATA_IND ibrt_link skip (mobile) skip sniff\n");
            nRet = 1;
        }
        if (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE){
            TRACE(0,"::A2DP_EVENT_STREAM_DATA_IND ibrt_link skip (tws) skip sniff\n");
#ifndef A2DP_IBRT_STREAM_SKIP_TWS_SNIFF_STATUS
            nRet = 1;
#endif
        }
    }
    return nRet;
}
#endif

void app_a2dp_bt_driver_callback(uint8_t device_id, btif_a2dp_event_t event)
{
    struct bt_cb_tag* bt_drv_func_cb = bt_drv_get_func_cb_ptr();

    switch(event)
    {
        case BTIF_A2DP_EVENT_STREAM_STARTED:
        //fall through
        case BTIF_A2DP_EVENT_STREAM_STARTED_MOCK:
            if(bt_drv_func_cb->bt_switch_agc != NULL)
            {
                bt_drv_func_cb->bt_switch_agc(BT_A2DP_WORK_MODE);
            }
            bt_drv_reg_op_afh_assess_en(true);
            break;

        case BTIF_A2DP_EVENT_STREAM_CLOSED:
        //fall through
        case BTIF_A2DP_EVENT_STREAM_IDLE:
        //fall through
        case BTIF_A2DP_EVENT_STREAM_SUSPENDED:
            if(app_bt_audio_count_streaming_a2dp() == 0)
            {
                if(bt_drv_func_cb->bt_switch_agc != NULL)
                {
                    bt_drv_func_cb->bt_switch_agc(BT_IDLE_MODE);
                }
                bt_drv_reg_op_afh_assess_en(false);
            }
            break;
        default:
            break;
    }
}

void a2dp_callback(uint8_t device_id, a2dp_stream_t *Stream, const a2dp_callback_parms_t *info)
{
    int header_len = 0;
    btif_avdtp_media_header_t header;
    btif_a2dp_callback_parms_t * Info = (btif_a2dp_callback_parms_t *)info;
    btif_avdtp_codec_t  *codec =  NULL;

    if (device_id == BT_DEVICE_INVALID_ID && Info->event == BTIF_A2DP_EVENT_STREAM_CLOSED)
    {
        // a2dp profile is closed due to acl created fail
        TRACE(1,"::A2DP_EVENT_STREAM_CLOSED acl created error %x", Info->discReason);
        return;
    }

    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    ASSERT(device_id < BT_DEVICE_NUM && curr_device->btif_a2dp_stream->a2dp_stream == Stream, "a2dp device channel must match");

    static uint8_t detect_first_packet[BT_DEVICE_NUM] = {0,};
    static btif_avdtp_codec_t   setconfig_codec;
    static u8 tmp_element[10];

    if (BTIF_A2DP_EVENT_STREAM_DATA_IND != Info->event)
    {
        TRACE(3,"(d%x) %s event %d", device_id, __func__, Info->event);
    }

    codec = btif_a2dp_get_stream_codec(Stream);

    switch(Info->event) {
        case BTIF_A2DP_REMOTE_NOT_SUPPORT:
            TRACE(2,"(d%x) ::A2DP_EVENT_REMOTE_NOT_SUPPORT %d", device_id, Info->event);
            app_bt_profile_connect_manager_a2dp((enum BT_DEVICE_ID_T)device_id, Stream, (a2dp_callback_parms_t *)Info);
            break;
        case BTIF_A2DP_EVENT_AVDTP_DISCONNECT:
            TRACE(3,"(d%x) ::A2DP_EVENT_AVDTP_DISCONNECT %d st = %p", device_id, Info->event, Stream);
            break;
        case BTIF_A2DP_EVENT_AVDTP_CONNECT:
            TRACE(3,"(d%x) ::A2DP_EVENT_AVDTP_CONNECT %d st = %p", device_id, Info->event, Stream);

#ifdef BT_USB_AUDIO_DUAL_MODE
            if(!btusb_is_bt_mode())
            {
                btusb_btaudio_close(false);
            }
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_OPEN:
            curr_device->mock_a2dp_after_force_disc = false;
            //fall through
        case BTIF_A2DP_EVENT_STREAM_OPEN_MOCK:
            TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_OPEN codec %d codec.elements %x", device_id, codec->codecType, Info->p.configReq->codec.elements[0]);

            if (app_bt_is_hfp_connected(device_id))
            {
                curr_device->profiles_connected_before = true;
            }

            curr_device->a2dp_conn_flag = 1;
            curr_device->a2dp_disc_on_process = 0;

            curr_device->ibrt_disc_a2dp_profile_only = false;
            curr_device->ibrt_slave_force_disc_a2dp = false;

            app_audio_manager_set_a2dp_codec_type(device_id, codec->codecType);

#ifdef GFPS_ENABLED
            app_exit_fastpairing_mode();
#endif

            app_bt_clear_connecting_profiles_state(device_id);

#if defined(BT_MAP_SUPPORT)&&defined(BTIF_DIP_DEVICE)
            if ((btif_dip_get_process_status(btif_a2dp_get_stream_conn_remDev(Stream)))&& (app_btmap_check_is_idle((BT_DEVICE_ID_T)device_id)))
            {
                app_btmap_sms_open((BT_DEVICE_ID_T)device_id, btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));
            }
#endif

            a2dp_timestamp_parser_init();
            app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream)));

#if defined(A2DP_LHDC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE(3,"(d%x) ##codecType: LHDC Codec, Element length = %d, AVDTP_MAX_CODEC_ELEM_SIZE = %d", device_id, Info->p.configReq->codec.elemLen, BTIF_AVDTP_MAX_CODEC_ELEM_SIZE);
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                a2dp_lhdc_config(device_id, &(Info->p.configReq->codec.elements[0]));
            }
            else
#endif
#if defined(A2DP_AAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
                TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac codec.elements %x", device_id, Info->p.configReq->codec.elements[1]);
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                curr_device->sample_bit = 16;
                // convert aac sample_rate to sbc sample_rate format
                if (Info->p.configReq->codec.elements[1] & A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100) {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac sample_rate 44100", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
                }
                else if (Info->p.configReq->codec.elements[2] & A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000) {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac sample_rate 48000", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_48;
                }
                else {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac sample_rate not 48000 or 44100, set to 44100", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
                }

                if (Info->p.configReq->codec.elements[2] & A2DP_AAC_OCTET2_CHANNELS_1) {
                    curr_device->a2dp_channel_num = 1;
                }
                else {
                    curr_device->a2dp_channel_num = 2;
                }
            }
            else
#endif
#if defined(A2DP_SCALABLE_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE(1,"(d%x) ##codecType scalable", device_id);
                a2dp_scalable_config(device_id, &(Info->p.configReq->codec.elements[0]));
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //0x75 0x00 0x00 0x00 Vid
                //0x03 0x01 Codec id
                if(Info->p.codec->elements[0]==0x75 && Info->p.codec->elements[4]==0x03 && Info->p.codec->elements[5]==0x01)
                {
                    setconfig_codec.elements = a2dp_scalable_avdtpcodec.elements;
                }
                else
                {
                    if(Info->p.codec->pstreamflags != NULL)
                    {
                        Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    }
                    else
                    {
                        ASSERT(false, "pstreamflags not init ..");
                    }
                    curr_device->a2dp_channel_num = 2;
                }
            }
            else
#endif
#if defined(A2DP_LDAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE(3,"(d%x) ##codecType: LDAC Codec, Element length = %d, AVDTP_MAX_CODEC_ELEM_SIZE = %d", device_id, Info->p.configReq->codec.elemLen, BTIF_AVDTP_MAX_CODEC_ELEM_SIZE);
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //Codec Info Element: 0x 2d 01 00 00 aa 00 34 07
                if(Info->p.codec->elements[0] == 0x2d)
                {
                    curr_device->sample_bit = 16;
                    a2dp_ldac_config(device_id, &(Info->p.configReq->codec.elements[0]));
                }
                else
                {
                    if(Info->p.codec->pstreamflags != NULL)
                    {
                        Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    }
                    else
                    {
                        ASSERT(false, "pstreamflags not init ..");
                    }
                    curr_device->a2dp_channel_num = 2;
                }
            }
            else
#endif
            {
                TRACE(6,"(d%x) sample_rate::elements[0] %d BITPOOL:%d/%d %02x/%02x", device_id,
                                    Info->p.codec->elements[0],
                                    Info->p.codec->elements[2],
                                    Info->p.codec->elements[3],
                                    Info->p.codec->elements[2],
                                    Info->p.codec->elements[3]);

                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
                curr_device->sample_bit = 16;
#if defined(A2DP_LDAC_ON)
                curr_device->sample_rate = (Info->p.configReq->codec.elements[0] & (A2D_SBC_IE_SAMP_FREQ_MSK == 0xff ? (A2D_SBC_IE_SAMP_FREQ_MSK & 0xf0) : A2D_SBC_IE_SAMP_FREQ_MSK));
#else
                curr_device->sample_rate = (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_SAMP_FREQ_MSK);
#endif

                if(Info->p.configReq->codec.elements[0] & A2D_SBC_IE_CH_MD_MONO)
                    curr_device->a2dp_channel_num  = 1;
                else
                    curr_device->a2dp_channel_num = 2;

            }

            app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_A2DP_STREAM_OPEN, 0);

            if (Info->event == BTIF_A2DP_EVENT_STREAM_OPEN) // dont auto reconnect avrcp when open mock
            {
                app_bt_reconnect_avrcp_profile(&curr_device->remote);
            }

            app_bt_profile_connect_manager_a2dp((enum BT_DEVICE_ID_T)device_id, Stream, (a2dp_callback_parms_t *)Info);

#if defined(IBRT)
            a2dp_ibrt_session_reset(device_id);
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_OPEN_IND:
            TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_OPEN_IND %d", device_id, Info->event);
            btif_a2dp_open_stream_rsp(Stream, BTIF_A2DP_ERR_NO_ERROR, BTIF_AVDTP_SRV_CAT_MEDIA_TRANSPORT);
            break;
        case BTIF_A2DP_EVENT_CODEC_INFO_MOCK:
            break;
        case BTIF_A2DP_EVENT_STREAM_STARTED:

#if defined(IBRT)
            a2dp_ibrt_session_new(device_id);
            // FALLTHROUGH
        case BTIF_A2DP_EVENT_STREAM_STARTED_MOCK:
#endif

#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Music on ok.");
#endif

            app_bt_active_mode_set(ACTIVE_MODE_KEEPER_A2DP_STREAMING, device_id);
            if (!besbt_cfg.dont_auto_report_delay_report && btif_a2dp_is_stream_device_has_delay_reporting(Stream))
            {
                btif_a2dp_set_sink_delay(Stream, 150);
            }

            a2dp_timestamp_parser_init();
#if defined(__CONNECTIVITY_LOG_REPORT__)
            app_ibrt_if_reset_acl_data_packet_check();
#endif
            curr_device->a2dp_streamming = 1;
            curr_device->a2dp_play_pause_flag = 1;
            detect_first_packet[device_id] = 1;

            TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_STARTED codec %d streaming %s",
                            device_id,
                            codec->codecType,
                            app_bt_a2dp_get_all_device_streaming_state());

            app_bt_audio_event_handler(device_id, Info->event == BTIF_A2DP_EVENT_STREAM_STARTED ? APP_BT_AUDIO_EVENT_A2DP_STREAM_START : APP_BT_AUDIO_EVENT_A2DP_STREAM_MOCK_START, 0);

#if (A2DP_DECODER_VER == 2)
#if defined(IBRT)
            if (Info->event == BTIF_A2DP_EVENT_STREAM_STARTED)
            {
                a2dp_audio_latency_factor_setlow();
            }
#else
            a2dp_audio_latency_factor_setlow();
#endif
#endif

#ifdef __IAG_BLE_INCLUDE__
            app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_A2DP_ON, true);
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_START_IND:
            TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_START_IND codec %d streaming %s",
                device_id, codec->codecType, app_bt_a2dp_get_all_device_streaming_state());
#ifdef BT_USB_AUDIO_DUAL_MODE
            if(!btusb_is_bt_mode())
            {
                btif_a2dp_start_stream_rsp(Stream, BTIF_A2DP_ERR_INSUFFICIENT_RESOURCE);
            }
            else
#endif
            {
                btif_a2dp_start_stream_rsp(Stream, BTIF_A2DP_ERR_NO_ERROR);
                curr_device->a2dp_play_pause_flag = 1;
            }
            break;
        case BTIF_A2DP_EVENT_STREAM_IDLE:
            TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_IDLE", device_id);
            // FALLTHROUGH
        case BTIF_A2DP_EVENT_STREAM_SUSPENDED:
            TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_SUSPENDED streaming %s",
                device_id, app_bt_a2dp_get_all_device_streaming_state());

#if defined(_AUTO_TEST_)
            AUTO_TEST_SEND("Music suspend ok.");
#endif
            app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_A2DP_STREAMING, device_id);
            a2dp_timestamp_parser_init();
            curr_device->a2dp_streamming = 0;
#if defined(__CONNECTIVITY_LOG_REPORT__)
            app_ibrt_if_reset_acl_data_packet_check();
#endif
            curr_device->a2dp_play_pause_flag = 0;

            app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_A2DP_STREAM_SUSPEND, 0);

#ifdef __IAG_BLE_INCLUDE__
            app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_A2DP_ON, false);
#endif
            break;
        case BTIF_A2DP_EVENT_STREAM_DATA_IND:
#ifdef __AI_VOICE__
            if(app_ai_voice_is_need2block_a2dp()) {
                TRACE(0,"AI block A2DP data");
                break;
            }
#endif
#if defined(IBRT)
            if (a2dp_ibrt_stream_event_stream_data_ind_needskip(device_id, Stream)){
                header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info,0);
                TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_DATA_IND skip seq:%d timestamp:%d", device_id, header.sequenceNumber, header.timestamp);
                break;
            }
#else
            if (btif_me_get_current_mode(btif_a2dp_get_remote_device(Stream)) == BTIF_BLM_SNIFF_MODE){
                TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_DATA_IND skip", device_id);
                break;
            }
#endif
            if (detect_first_packet[device_id]) {
                detect_first_packet[device_id] = 0;
                avrcp_get_current_media_status((enum BT_DEVICE_ID_T)device_id);
            }

            if (app_bt_audio_curr_a2dp_data_need_receive(device_id))
            {
#ifdef __A2DP_AVDTP_CP__ //zadd bug fixed sony Z5 no sound
                header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info, curr_device->avdtp_cp);
#else
                header_len = btif_avdtp_parse_mediaHeader(&header, (btif_a2dp_callback_parms_t *)Info, 0);
#endif
#ifdef __A2DP_TIMESTAMP_PARSER__
                if (a2dp_timestamp_parser_run(header.timestamp,(*(((unsigned char *)Info->p.data) + header_len))))
                {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_DATA_IND skip frame", device_id);
                }
                else
#endif
                {
#if (A2DP_DECODER_VER >= 2)
                    a2dp_audio_store_packet(device_id, &header, ((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
#else
                    a2dp_audio_sbc_set_frame_info(Info->len - header_len - 1, (*(((unsigned char *)Info->p.data) + header_len)));
#if defined(A2DP_LHDC_ON) || defined(A2DP_LDAC_ON) || defined(A2DP_SCALABLE_ON)
                    if (curr_device->codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                    {
                        store_sbc_buffer(device_id, ((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                    }
                    else
#endif
#if defined(A2DP_AAC_ON)
                    if (curr_device->codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
                    {
#ifdef BT_USB_AUDIO_DUAL_MODE
                        if(btusb_is_bt_mode())
#endif
                        {
                            store_sbc_buffer(device_id, ((unsigned char *)Info->p.data) + header_len , Info->len - header_len);
                        }
                    }
                    else
#endif
                    {
                        store_sbc_buffer(device_id, ((unsigned char *)Info->p.data) + header_len + 1 , Info->len - header_len - 1);
                    }
#endif
                }
            }
            else
            {
                // TRACE(0, "Current A2DP data not need receive");
            }
            break;
        case BTIF_A2DP_EVENT_STREAM_CLOSED:
            TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_CLOSED reason %x force_disc %d", device_id, Info->discReason, curr_device->ibrt_slave_force_disc_a2dp);

            if (btif_a2dp_is_disconnected(Stream))
            {
                curr_device->a2dp_conn_flag = 0;
                curr_device->a2dp_disc_on_process = 0;
                if (curr_device->ibrt_disc_a2dp_profile_only || curr_device->ibrt_slave_force_disc_a2dp)
                {
                    TRACE(2, "%s dont close avrcp due to disc a2dp only %d", __func__, curr_device->ibrt_disc_a2dp_profile_only);
                }
                else if (Info->a2dp_closed_due_to_sdp_fail)
                {
                    TRACE(1, "%s dont close avrcp due to sdp fail", __func__);
                }
                else
                {
                    app_bt_disconnect_avrcp_profile(curr_device->avrcp_channel);
                }

                curr_device->ibrt_disc_a2dp_profile_only = false;
                curr_device->ibrt_slave_force_disc_a2dp = false;
            }

            a2dp_timestamp_parser_init();
            curr_device->a2dp_streamming = 0;
            curr_device->a2dp_play_pause_flag = 0;
#ifdef __A2DP_AVDTP_CP__
            curr_device->avdtp_cp = 0;
#endif
            app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_A2DP_STREAM_CLOSE, 0);

            app_bt_profile_connect_manager_a2dp((enum BT_DEVICE_ID_T)device_id, Stream, info);

#if defined(IBRT)
            a2dp_ibrt_session_reset(device_id);
#endif
            break;
        case BTIF_A2DP_EVENT_CODEC_INFO:
            TRACE(2,"(d%x) ::A2DP_EVENT_CODEC_INFO %d", device_id, Info->event);
            if (!besbt_cfg.dont_auto_report_delay_report && btif_a2dp_is_stream_device_has_delay_reporting(Stream))
            {
                btif_a2dp_set_sink_delay(Stream, 150);
            }
            setconfig_codec.codecType = Info->p.codec->codecType;
            setconfig_codec.discoverable = Info->p.codec->discoverable;
            setconfig_codec.elemLen = Info->p.codec->elemLen;
            setconfig_codec.elements = tmp_element;
            memset(tmp_element, 0, sizeof(tmp_element));
            DUMP8("%02x ", (setconfig_codec.elements), 8);
            if (Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_SBC) {
                setconfig_codec.elements[0] = (Info->p.codec->elements[0]) & (a2dp_codec_elements[0]);
                setconfig_codec.elements[1] = (Info->p.codec->elements[1]) & (a2dp_codec_elements[1]);

                if (Info->p.codec->elements[2] <= a2dp_codec_elements[2])
                    setconfig_codec.elements[2] = a2dp_codec_elements[2];////[2]:MIN_BITPOOL
                else
                    setconfig_codec.elements[2] = Info->p.codec->elements[2];

                if (Info->p.codec->elements[3] >= a2dp_codec_elements[3])
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];////[3]:MAX_BITPOOL
                else
                    setconfig_codec.elements[3] = Info->p.codec->elements[3];

                ///////null set situation:
                if (setconfig_codec.elements[3] < a2dp_codec_elements[2]) {
                    setconfig_codec.elements[2] = a2dp_codec_elements[2];
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];
                }
                else if (setconfig_codec.elements[2] > a2dp_codec_elements[3]) {
                    setconfig_codec.elements[2] = a2dp_codec_elements[3];
                    setconfig_codec.elements[3] = a2dp_codec_elements[3];
                }
                TRACE(3,"(d%x) !!!setconfig_codec.elements[2]:%d,setconfig_codec.elements[3]:%d",
                    device_id,setconfig_codec.elements[2],setconfig_codec.elements[3]);

                setconfig_codec.elements[0] = a2dp_codec_sbc_get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_SAMP_FREQ_MSK);
                setconfig_codec.elements[0] = a2dp_codec_sbc_get_valid_bit(setconfig_codec.elements[0],A2D_SBC_IE_CH_MD_MSK);
                setconfig_codec.elements[1] = a2dp_codec_sbc_get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_BLOCKS_MSK);
                setconfig_codec.elements[1] = a2dp_codec_sbc_get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_SUBBAND_MSK);
                setconfig_codec.elements[1] = a2dp_codec_sbc_get_valid_bit(setconfig_codec.elements[1],A2D_SBC_IE_ALLOC_MD_MSK);
            }
#if defined(A2DP_AAC_ON)
            else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
                setconfig_codec.elements[0] = a2dp_codec_aac_elements[0];
                if (Info->p.codec->elements[1] & A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100)
                    setconfig_codec.elements[1] |= A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100;
                else if (Info->p.codec->elements[2] & A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000)
                    setconfig_codec.elements[2] |= A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000;

                if (Info->p.codec->elements[2] & A2DP_AAC_OCTET2_CHANNELS_2)
                    setconfig_codec.elements[2] |= A2DP_AAC_OCTET2_CHANNELS_2;
                else if (Info->p.codec->elements[2] & A2DP_AAC_OCTET2_CHANNELS_1)
                    setconfig_codec.elements[2] |= A2DP_AAC_OCTET2_CHANNELS_1;

                setconfig_codec.elements[3] = (Info->p.codec->elements[3]) & A2DP_AAC_OCTET3_VBR_SUPPORTED;

                if (((Info->p.codec->elements[3]) & A2DP_AAC_OCTET3_VBR_SUPPORTED) &&
                    (((a2dp_codec_aac_elements[3]) & A2DP_AAC_OCTET3_VBR_SUPPORTED) == 0))
                {
                    Info->error = BTIF_A2DP_ERR_NOT_SUPPORTED_VBR;
                    TRACE(1,"(d%x) setconfig: VBR  UNSUPPORTED!!!!!!", device_id);
                }

                uint32_t bit_rate = 0;
                bit_rate = ((Info->p.codec->elements[3]) & 0x7f) << 16;
                bit_rate |= (Info->p.codec->elements[4]) << 8;
                bit_rate |= (Info->p.codec->elements[5]);
                TRACE(2,"(d%x) bit_rate = %d", device_id, bit_rate);
                if (bit_rate == 0) {
                    bit_rate = MAX_AAC_BITRATE;
                }
                else if(bit_rate > MAX_AAC_BITRATE) {
                    bit_rate = MAX_AAC_BITRATE;
                }

                setconfig_codec.elements[3] |= (bit_rate >> 16) &0x7f;
                setconfig_codec.elements[4] = (bit_rate >> 8) & 0xff;
                setconfig_codec.elements[5] = bit_rate & 0xff;
            }
#endif
            else if(Info->p.codec->codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
#if defined(A2DP_SCALABLE_ON)
                //0x75 0x00 0x00 0x00 Vid
                //0x03 0x01 Codec id
                if(Info->p.codec->elements[0]==0x75 && Info->p.codec->elements[1]==0x00 && Info->p.codec->elements[2]==0x00 &&
                   Info->p.codec->elements[3]==0x00 && Info->p.codec->elements[4]==0x03 && Info->p.codec->elements[5]==0x01)
                {
                    memcpy(&setconfig_codec.elements[0], &a2dp_codec_scalable_elements[0], 6);
                    DUMP8("%02x ", (setconfig_codec.elements), 8);
                    DUMP8("%02x ", &(Info->p.codec->elements[0]), 8);
                    setconfig_codec.elements[6] = 0x00;
                    //Audio format setting
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
                    if (Info->p.codec->elements[6] & A2DP_SCALABLE_SR_96000) {
                        setconfig_codec.elements[6] |= A2DP_SCALABLE_SR_96000;
                    }
#endif
                    if (Info->p.codec->elements[6] & A2DP_SCALABLE_SR_32000) {
                        setconfig_codec.elements[6] |= A2DP_SCALABLE_SR_32000;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_SCALABLE_SR_44100) {
                        setconfig_codec.elements[6] |= A2DP_SCALABLE_SR_44100;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_SCALABLE_SR_48000) {
                        setconfig_codec.elements[6] |= A2DP_SCALABLE_SR_48000;
                    }

                    if (Info->p.codec->elements[6] & A2DP_SCALABLE_HQ) {
                        setconfig_codec.elements[6] |= A2DP_SCALABLE_HQ;
                    }
                    DUMP8("0x%02x ", setconfig_codec.elements, setconfig_codec.elemLen);
                }
#endif

#if defined(A2DP_LHDC_ON)
                //0x3A 0x05 0x00 0x00Vid
                //0x33 0x4c   Codec id
                if(Info->p.codec->elements[0]==0x3A && Info->p.codec->elements[1]==0x05 && Info->p.codec->elements[2]==0x00 &&
                   Info->p.codec->elements[3]==0x00)
                {
                    memcpy(&setconfig_codec.elements[0], &a2dp_codec_lhdc_elements[0], 6);
                    DUMP8("%02x ", (setconfig_codec.elements), 8);
                    DUMP8("%02x ", &(Info->p.codec->elements[0]), 8);

                    //Audio format setting
                    //(A2DP_LHDC_SR_96000|A2DP_LHDC_SR_48000 |A2DP_LHDC_SR_44100) | (A2DP_LHDC_FMT_16),
                    if (Info->p.codec->elements[6] & A2DP_LHDC_SR_96000) {
                        setconfig_codec.elements[6] |= A2DP_LHDC_SR_96000;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_LHDC_SR_48000) {
                        setconfig_codec.elements[6] |= A2DP_LHDC_SR_48000;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_LHDC_SR_44100) {
                        setconfig_codec.elements[6] |= A2DP_LHDC_SR_44100;
                    }

                    if (Info->p.codec->elements[6] & A2DP_LHDC_FMT_24) {
                        setconfig_codec.elements[6] |= A2DP_LHDC_FMT_24;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_LHDC_FMT_16) {
                        setconfig_codec.elements[6] |= A2DP_LHDC_FMT_16;
                    }
                }
#endif

#if defined(A2DP_LDAC_ON)
                //0x2d, 0x01, 0x00, 0x00, //Vendor ID
                //0xaa, 0x00,     //Codec ID
                if(Info->p.codec->elements[0]==0x2d && Info->p.codec->elements[1]==0x01 && Info->p.codec->elements[2]==0x00 &&
                   Info->p.codec->elements[3]==0x00 && Info->p.codec->elements[4]==0xaa && Info->p.codec->elements[5]==0x00)
                {
                    memcpy(&setconfig_codec.elements[0], &a2dp_codec_ldac_elements[0], 6);

                    DUMP8("%02x ", (setconfig_codec.elements), 8);
                    DUMP8("%02x ", &(Info->p.codec->elements[0]), 8);
                    //Audio format setting
                    //3c 03
                    //34 07
                    if (Info->p.codec->elements[6] & A2DP_LDAC_SR_96000) {
                        setconfig_codec.elements[6] |= A2DP_LDAC_SR_96000;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_48000) {
                        setconfig_codec.elements[6] |= A2DP_LDAC_SR_48000;
                    }
                    else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_44100) {
                        setconfig_codec.elements[6] |= A2DP_LDAC_SR_44100;
                    }
                    /*
                    else if (Info->p.codec->elements[6] & A2DP_LDAC_SR_88200) {
                        setconfig_codec.elements[6] |= A2DP_LDAC_SR_88200;
                    }
                    */

                    if (Info->p.codec->elements[7] & A2DP_LDAC_CM_MONO) {
                        setconfig_codec.elements[7] |= A2DP_LDAC_CM_MONO;
                    }
                    else if (Info->p.codec->elements[7] & A2DP_LDAC_CM_DUAL) {
                        setconfig_codec.elements[7] |= A2DP_LDAC_CM_DUAL;
                    }
                    else if (Info->p.codec->elements[7] & A2DP_LDAC_CM_STEREO) {
                        setconfig_codec.elements[7] |= A2DP_LDAC_CM_STEREO;
                    }

                    TRACE(2,"(d%x) setconfig_codec.elemLen = %d", device_id, setconfig_codec.elemLen);
                    TRACE(2,"(d%x) setconfig_codec.elements[7] = 0x%02x", device_id, setconfig_codec.elements[7]);

                    DUMP8("%02x ", (setconfig_codec.elements), 8);
                }
#endif
            }
            break;
        case BTIF_A2DP_EVENT_GET_CONFIG_IND:
            TRACE(2,"(d%x) ::A2DP_EVENT_GET_CONFIG_IND %d\n", device_id, Info->event);
            break;
        case BTIF_A2DP_EVENT_STREAM_RECONFIG_IND:
            TRACE(3,"(d%x) ::A2DP_EVENT_STREAM_RECONFIG_IND %d codec %d", device_id, Info->event, Info->p.configReq->codec.codecType);
#if defined(A2DP_SCALABLE_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE(1,"(d%x) ::##SCALABLE A2DP_EVENT_STREAM_RECONFIG_IND", device_id);
                a2dp_scalable_config(device_id, &(Info->p.configReq->codec.elements[0]));
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                //0x75 0x00 0x00 0x00Vid
                //0x03 0x01   Codec id
                if(Info->p.codec->elements[0]==0x75 && Info->p.codec->elements[4]==0x03 && Info->p.codec->elements[5]==0x01)
                {
                    setconfig_codec.elements = a2dp_scalable_avdtpcodec.elements;
                }
                else
                {
                    if(Info->p.codec->pstreamflags!=NULL) {
                        Info->p.codec->pstreamflags[0] &= ~APP_A2DP_STRM_FLAG_QUERY_CODEC;
                    }
                    else {
                        ASSERT(false, "pstreamflags not init ..");
                    }
                }
                curr_device->a2dp_channel_num = 2;
            }
            else
#endif
#if defined(A2DP_LHDC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_LHDC) {
                TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_RECONFIG_IND ##LHDC len %d", device_id, Info->p.configReq->codec.elemLen);
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_LHDC;
                curr_device->a2dp_channel_num = 2;
                a2dp_lhdc_config(device_id, &(Info->p.configReq->codec.elements[0]));
            }
            else
#endif
#if defined(A2DP_LDAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_RECONFIG_IND ##LDAC len %d", device_id, Info->p.configReq->codec.elemLen);
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                curr_device->a2dp_channel_num = 2;
                a2dp_ldac_config(device_id, &(Info->p.configReq->codec.elements[0]));
            }
            else
#endif
#if defined(A2DP_AAC_ON)
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
                TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_RECONFIG_IND ##AAC", device_id);
                if (((Info->p.codec->elements[3]) & A2DP_AAC_OCTET3_VBR_SUPPORTED) &&
                    (((a2dp_codec_aac_elements[3]) & A2DP_AAC_OCTET3_VBR_SUPPORTED) == 0))
                {
                    Info->error = BTIF_A2DP_ERR_NOT_SUPPORTED_VBR;
                    TRACE(1,"(d%x) stream reconfig: VBR  UNSUPPORTED!!!!!!", device_id);
                }
                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                curr_device->sample_bit = 16;
                // convert aac sample_rate to sbc sample_rate format
                if (Info->p.configReq->codec.elements[1] & A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100) {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN  aac sample_rate 44100", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
                }
                else if (Info->p.configReq->codec.elements[2] & A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000) {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac sample_rate 48000", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_48;
                }
                else {
                    TRACE(1,"(d%x) ::A2DP_EVENT_STREAM_OPEN aac sample_rate not 48000 or 44100, set to 44100", device_id);
                    curr_device->sample_rate = A2D_SBC_IE_SAMP_FREQ_44;
                }
                if (Info->p.configReq->codec.elements[2] & A2DP_AAC_OCTET2_CHANNELS_1){
                    curr_device->a2dp_channel_num = 1;
                }else{
                    curr_device->a2dp_channel_num = 2;
                }
            }
            else
#endif
            if (Info->p.configReq->codec.codecType == BTIF_AVDTP_CODEC_TYPE_SBC) {
                TRACE(6,"(d%x) ::A2DP_EVENT_STREAM_RECONFIG_IND ##SBC sample_rate::elements[0] %d BITPOOL:%d/%d %02x/%02x",
                        device_id,
                        Info->p.configReq->codec.elements[0],
                        Info->p.configReq->codec.elements[2],
                        Info->p.configReq->codec.elements[3],
                        Info->p.configReq->codec.elements[2],
                        Info->p.configReq->codec.elements[3]);

                curr_device->codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
                curr_device->sample_bit = 16;
                curr_device->sample_rate = (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_SAMP_FREQ_MSK);

                if (Info->p.configReq->codec.elements[0] & A2D_SBC_IE_CH_MD_MONO)
                    curr_device->a2dp_channel_num  = 1;
                else
                    curr_device->a2dp_channel_num = 2;
            }
            break;
#ifdef __A2DP_AVDTP_CP__
        case BTIF_A2DP_EVENT_CP_INFO:
            TRACE(3,"(d%x) ::A2DP_EVENT_CP_INFO %d cpType: %x", device_id, Info->event, Info->p.cp->cpType);
            if(Info->p.cp && Info->p.cp->cpType == BTIF_AVDTP_CP_TYPE_SCMS_T)
            {
                curr_device->avdtp_cp = 1;
            }
            else
            {
                curr_device->avdtp_cp = 0;
            }
            btif_a2dp_set_copy_protection_enable(Stream, curr_device->avdtp_cp);
            break;
        case BTIF_A2DP_EVENT_STREAM_SECURITY_IND:
            TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_SECURITY_IND %d", device_id, Info->event);
            DUMP8("%x ",Info->p.data,Info->len);
             btif_a2dp_security_control_rsp(Stream,&Info->p.data[1],Info->len-1,Info->error);
            break;
        case BTIF_A2DP_EVENT_STREAM_SECURITY_CNF:
            curr_device->avdtp_cp = 1;
            TRACE(2,"(d%x) ::A2DP_EVENT_STREAM_SECURITY_CNF %d", device_id, Info->event);
            break;
#endif
    }

    app_a2dp_bt_driver_callback(device_id, Info->event);

#if defined(IBRT)
    app_tws_ibrt_profile_callback(device_id, BTIF_APP_A2DP_PROFILE_ID, (void *)Stream, (void *)info,&curr_device->remote);
#endif
}


uint8_t a2dp_volume_local_get(enum BT_DEVICE_ID_T id)
{
    uint8_t localVol = AUDIO_OUTPUT_VOLUME_DEFAULT;
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && !nv_record_btdevicerecord_find(&curr_device->remote, &record))
    {
        localVol = record->device_vol.a2dp_vol;
        TRACE(3,"(d%x)%s,vol %d",id,__func__,localVol);
    }
    else
    {
        TRACE(2,"(d%x)%s,not find remDev",id,__func__);
    }

    return localVol;
}

uint8_t a2dp_convert_local_vol_to_bt_vol(uint8_t localVol)
{
    return unsigned_range_value_map(localVol, TGT_VOLUME_LEVEL_MUTE, TGT_VOLUME_LEVEL_MAX, 0, MAX_A2DP_VOL);
}

uint8_t a2dp_volume_get(enum BT_DEVICE_ID_T id)
{
    uint8_t localVol = a2dp_volume_local_get(id);

    return a2dp_convert_local_vol_to_bt_vol(localVol);
}

void a2dp_volume_local_set(enum BT_DEVICE_ID_T id, uint8_t vol)
{
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && curr_device->acl_is_connected)
    {
        if (!nv_record_btdevicerecord_find(&curr_device->remote, &record) && record->device_vol.a2dp_vol != vol)
        {
            nv_record_btdevicerecord_set_a2dp_vol(record, vol);
#ifndef FPGA
            nv_record_touch_cause_flush();
#endif
        }
    }
}

void a2dp_abs_volume_set(enum BT_DEVICE_ID_T id, uint8_t vol)
{
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && curr_device->acl_is_connected)
    {
        if (!nv_record_btdevicerecord_find(&curr_device->remote, &record) && record->device_plf.a2dp_abs_vol != vol)
        {
            nv_record_btdevicerecord_set_a2dp_abs_vol(record, vol);
#ifndef FPGA
            nv_record_touch_cause_flush();
#endif
        }
    }
}

uint8_t a2dp_abs_volume_get(enum BT_DEVICE_ID_T id)
{
    uint8_t local_abs_vol = app_bt_manager.config.a2dp_default_abs_volume;
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && !nv_record_btdevicerecord_find(&curr_device->remote, &record))
    {
        local_abs_vol = record->device_plf.a2dp_abs_vol;
        TRACE(3,"(d%x)%s,abs vol %d",id,__func__,local_abs_vol);
    }
    else
    {
        TRACE(2,"(d%x)%s,not find remDev",id,__func__);
    }

    return local_abs_vol;
}

uint8_t a2dp_convert_bt_vol_to_local_vol(uint8_t btVol)
{
    return unsigned_range_value_map(btVol, 0, MAX_A2DP_VOL, TGT_VOLUME_LEVEL_MUTE, TGT_VOLUME_LEVEL_MAX);
}

void a2dp_update_local_volume(enum BT_DEVICE_ID_T id, uint8_t localVol)
{
    a2dp_volume_local_set(id, localVol);

    if (app_audio_manager_a2dp_is_active(id))
    {
        app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, localVol);
    }
}

void a2dp_volume_set(enum BT_DEVICE_ID_T id, uint8_t bt_volume)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);
    uint8_t local_vol = 0;

    bt_volume = bt_volume > 127 ? 127 : bt_volume;

    local_vol = a2dp_convert_bt_vol_to_local_vol(bt_volume);

    if (curr_device)
    {
        TRACE(4, "(d%x) %s a2dp_vol bt %d local %d", id, __func__, bt_volume, local_vol);

        curr_device->a2dp_current_abs_volume = bt_volume;

        a2dp_abs_volume_set(id, bt_volume);

        a2dp_update_local_volume(id, local_vol);
    }
}

void a2dp_volume_set_local_vol(enum BT_DEVICE_ID_T id, uint8_t local_vol)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);
    uint8_t bt_volume = a2dp_convert_local_vol_to_bt_vol(local_vol);

    if (curr_device)
    {
        TRACE(4, "(d%x) %s a2dp_vol bt %d local %d", id, __func__, bt_volume, local_vol);

        curr_device->a2dp_current_abs_volume = bt_volume;

        a2dp_abs_volume_set(id, bt_volume);

        a2dp_update_local_volume(id, local_vol);
    }
}

bool a2dp_is_music_ongoing(void)
{
    struct BT_DEVICE_T *device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        device = app_bt_get_device(i);
        if (device->a2dp_streamming)
        {
            return true;
        }
    }

    return false;
}

static void app_bt_a2dp_send_volume_change_handler(int device_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

#if defined(IBRT)
    if (!curr_device || 
        (tws_besaud_is_connected() &&
        (IBRT_SLAVE == app_tws_get_ibrt_role(&curr_device->remote))))
#else
    if (!curr_device)
#endif
    {
        return;
    }

    TRACE(4, "(d%x) %s volume_report %02x vol %d", device_id, __func__, curr_device->volume_report, curr_device->a2dp_current_abs_volume);

    if (curr_device->volume_report == BTIF_AVCTP_RESPONSE_INTERIM)
    {
        bt_status_t ret = btif_avrcp_ct_send_volume_change_actual_rsp(curr_device->avrcp_channel,
            curr_device->a2dp_current_abs_volume);
        if (BT_STS_FAILED != ret)
        {
            curr_device->volume_report = BTIF_AVCTP_RESPONSE_CHANGED;
        }
        else
        {
            TRACE(1, "set abs vol failed.");
        }
    }
}

bool app_bt_a2dp_send_volume_change(int device_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    if (!curr_device)
    {
        return false;
    }

    app_bt_start_custom_function_in_bt_thread((uint32_t)device_id,
            0, (uint32_t)app_bt_a2dp_send_volume_change_handler);

    return true;
}

bool app_bt_a2dp_report_current_volume(int device_id)
{
    btif_remote_device_t *remDev = NULL;
    btif_link_mode_t mode = BTIF_BLM_SNIFF_MODE;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    if (!curr_device)
    {
        return false;
    }

    remDev = btif_a2dp_get_remote_device(curr_device->a2dp_connected_stream);

    if (remDev)
    {
        mode = btif_me_get_current_mode(remDev);
    }
    else
    {
        mode = BTIF_BLM_SNIFF_MODE;
    }

    if (mode != BTIF_BLM_ACTIVE_MODE || !curr_device->a2dp_streamming)
    {
        return false;
    }

    return app_bt_a2dp_send_volume_change(device_id);
}

void btapp_a2dp_report_speak_gain(void)
{
#ifdef BTIF_AVRCP_ADVANCED_CONTROLLER
    for (int i = 0; i < BT_DEVICE_NUM; i++)
    {
        app_bt_a2dp_report_current_volume(i);
    }
#endif
}

static void app_AVRCP_sendCustomCmdRsp(uint8_t device_id, btif_avrcp_channel_t *chnl, uint8_t isAccept, uint8_t transId)
{
    btif_avrcp_ct_accept_custom_cmd(chnl, isAccept, transId);
}

static void app_AVRCP_CustomCmd_Received(uint8_t* ptrData, uint32_t len)
{
    TRACE(1,"AVRCP Custom Command Received %d bytes data:", len);
    DUMP8("0x%02x ", ptrData, len);
}

#ifdef __TENCENT_VOICE__
avrcp_media_status_t    media_status = 0xff;
uint8_t avrcp_get_media_status(void)
{
    TRACE(2,"%s %d",__func__, media_status);
    return media_status;
}
 uint8_t avrcp_ctrl_music_flag;
void avrcp_set_media_status(uint8_t status)
{
    TRACE(2,"%s %d",__func__, status);
    if ((status == 1 && avrcp_ctrl_music_flag == 2) || (status == 2 && avrcp_ctrl_music_flag == 1))
        avrcp_ctrl_music_flag = 0;

#if defined(__CONNECTIVITY_LOG_REPORT__)
    if ((status == 2) || (status == 1)) {  //BTIF_AVRCP_MEDIA_PLAYING, BTIF_AVRCP_MEDIA_PAUSED
        app_ibrt_if_reset_acl_data_packet_check();
    }
#endif

    media_status = status;
}
#endif

#if defined(__FORCE_UPDATE_A2DP_VOL__)

#define FORCE_UPDATE_VOLUME_LEVEL (TGT_VOLUME_LEVEL_6)
extern void xspace_ui_tws_sync_volume_info(uint8_t *addr, uint8_t vol);

static void xspace_btapp_a2dp_report_speak_gain(uint8_t dev_id, uint8_t vol)
{
#ifdef BTIF_AVRCP_ADVANCED_CONTROLLER
    app_bt_get_device(dev_id)->a2dp_current_abs_volume = a2dp_convert_local_vol_to_bt_vol(vol);
    app_bt_get_device(dev_id)->volume_report = BTIF_AVCTP_RESPONSE_INTERIM;
    TRACE(4, "(d%x) %s volume_report %d vol %d", dev_id, __func__, app_bt_get_device(dev_id)->volume_report,
                    app_bt_get_device(dev_id)->a2dp_current_abs_volume);

    if (app_bt_get_device(dev_id)->volume_report == BTIF_AVCTP_RESPONSE_INTERIM)
    {
        app_bt_get_device(dev_id)->volume_report = BTIF_AVCTP_RESPONSE_CHANGED;
        app_bt_start_custom_function_in_bt_thread((uint32_t)app_bt_get_device(dev_id)->avrcp_channel,
                (uint32_t)app_bt_get_device(dev_id)->a2dp_current_abs_volume, (uint32_t)btif_avrcp_ct_send_volume_change_actual_rsp);
    }

#endif
}

void btapp_a2dp_vol_force_changed(uint8_t dev_id)
{
    int a2dp_vol = TGT_VOLUME_LEVEL_29;

    btif_remote_device_t *remDev = NULL;
    nvrec_btdevicerecord *record = NULL;
    remDev = btif_a2dp_get_remote_device(app_bt_get_device(dev_id)->a2dp_connected_stream);
    if (remDev && !nv_record_btdevicerecord_find(btif_me_get_remote_device_bdaddr(remDev),&record)){
        a2dp_vol = record->device_vol.a2dp_vol;
    }else if (app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)dev_id)){
        a2dp_vol = app_bt_stream_a2dpvolume_get();
    }

    TRACE(2,"(d%x) %s current a2dp vol:%d\n", dev_id, __func__, a2dp_vol);
    if (a2dp_vol < FORCE_UPDATE_VOLUME_LEVEL) {    
        a2dp_volume_local_set((enum BT_DEVICE_ID_T)dev_id, FORCE_UPDATE_VOLUME_LEVEL);
        if (app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)dev_id)){
            app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, FORCE_UPDATE_VOLUME_LEVEL);
        }
	    xspace_btapp_a2dp_report_speak_gain(dev_id, FORCE_UPDATE_VOLUME_LEVEL);
        //app_ibrt_sync_a2dp_status(&(app_bt_get_device(dev_id)->remote));
		if (IBRT_MASTER == app_ibrt_if_get_ui_role())
        	xspace_ui_tws_sync_volume_info(app_bt_get_device(dev_id)->remote.address, FORCE_UPDATE_VOLUME_LEVEL);
    }
}

void btapp_a2dp_vol_force_changed_d0(void)
{
	btapp_a2dp_vol_force_changed(0);
}

void btapp_a2dp_vol_force_changed_d1(void)
{
	btapp_a2dp_vol_force_changed(1);
}

#endif

