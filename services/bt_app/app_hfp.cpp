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
//#include "mbed.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_trace_rx.h"
#include "app_status_ind.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "audio_policy.h"
#include "hfp_api.h"
#include "besbt.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "bt_if.h"
#include "app_hfp.h"
#if defined(BT_SOURCE)
#include "app_hfp_ag.h"
#endif
#include "app_fp_rfcomm.h"
#include "os_api.h"
#include "besbt_cfg.h"
#include "app_bt_func.h"
#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#include "bt_drv_interface.h"

#ifdef __IAG_BLE_INCLUDE__
#include "app.h"
#include "app_ble_mode_switch.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_if.h"
#endif

#ifdef __XSPACE_UI__
#include "xspace_ui.h"
#endif
#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "app_ibrt_hf.h"
#include "besaud_api.h"
#include "app_tws_profile_sync.h"
#endif

#include "bt_drv_reg_op.h"

#ifdef BISTO_ENABLED
#include "gsound_custom_bt.h"
#endif

#if defined(BT_MAP_SUPPORT)
#include "app_btmap_sms.h"
#endif

#define ATCMD_SZ 42

#ifdef __INTERCONNECTION__
#define HF_COMMAND_HUAWEI_BATTERY_HEAD              "AT+HUAWEIBATTERY="
#define BATTERY_REPORT_NUM                          2
#define BATTERY_REPORT_TYPE_BATTERY_LEVEL           1
#define BATTERY_REPORT_KEY_LEFT_BATTERY_LEVEL       2
#define BATTERY_REPORT_KEY_LEFT_CHARGE_STATE        3
static const char* huawei_self_defined_command_response = "+HUAWEIBATTERY=OK";
#endif

/* hfp */
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);

int store_voicecvsd_buffer(unsigned char *buf, unsigned int len);
int store_voicemsbc_buffer(unsigned char *buf, unsigned int len);

void btapp_hfp_mic_need_skip_frame_set(int32_t skip_frame);

extern "C" bool bt_media_cur_is_bt_stream_media(void);
bool bt_is_sco_media_open();
extern bool app_audio_list_playback_exist(void);
#ifdef GFPS_ENABLED
extern "C" void app_exit_fastpairing_mode(void);
#endif

app_hfp_hf_callback_t g_app_bt_hfp_hf_cb = NULL;

#define APP_BT_HFP_HF_CB(event,param) \
    do { \
        if (g_app_bt_hfp_hf_cb != NULL) \
            g_app_bt_hfp_hf_cb(event, param); \
    } while (0)

extern void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id, btif_hf_channel_t* Chan, struct hfp_context *ctx);
extern void hfcall_next_sta_handler(uint8_t device_id, hf_event_t event);
#ifndef _SCO_BTPCM_CHANNEL_
struct hf_sendbuff_control  hf_sendbuff_ctrl;
#endif
#ifdef __INTERACTION__
const char* oppo_self_defined_command_response = "+VDSF:7";
#endif
#if defined(SCO_LOOP)
#define HF_LOOP_CNT (20)
#define HF_LOOP_SIZE (360)

static uint8_t hf_loop_buffer[HF_LOOP_CNT*HF_LOOP_SIZE];
static uint32_t hf_loop_buffer_len[HF_LOOP_CNT];
static uint32_t hf_loop_buffer_valid = 1;
static uint32_t hf_loop_buffer_size = 0;
static char hf_loop_buffer_w_idx = 0;
#endif

#ifdef __IAG_BLE_INCLUDE__
static void app_hfp_resume_ble_adv(void);
#endif

#define HFP_AUDIO_CLOSED_DELAY_RESUME_ADV_IN_MS     1500
static void app_hfp_audio_closed_delay_resume_ble_adv_timer_cb(void const *n);
osTimerDef (APP_HFP_AUDIO_CLOSED_DELAY_RESUME_BLE_ADV_TIMER,
                        app_hfp_audio_closed_delay_resume_ble_adv_timer_cb);
osTimerId   app_hfp_audio_closed_delay_resume_ble_adv_timer_id = NULL;

#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)

static uint8_t report_battery_level = 0xff;

void app_hfp_set_battery_level(uint8_t level)
{
    if (report_battery_level == 0xff) {
        report_battery_level = level;
        osapi_notify_evm();
    }
}

int app_hfp_battery_report_reset(uint8_t bt_device_id)
{
    app_bt_get_device(bt_device_id)->battery_level = 0xff;
    return 0;
}


#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
#define HF_COMMAND_BATTERY_HEAD              		"AT+VDBTY="
#define HF_COMMAND_VERSION_HEAD              		"AT+VDRV="
#define HF_COMMAND_FEATURE_HEAD              		"AT+VDSF="
#define REPORT_NUM                          		3
#define LEFT_UNIT_REPORT					       	1
#define RIGHT_UNIT_REPORT      						2
#define BOX_REPORT						        	3

bt_status_t Send_customer_battery_report_AT_command(btif_hf_channel_t* chan_h, uint8_t level)
{
    char at_cmd[ATCMD_SZ] = {'\0'};
    char buf[20] = {'\0'};
    uint8_t len;

    TRACE(0,"Send battery report at commnad.");
    /// head and keyNumber
    sprintf(at_cmd, "%s%d", HF_COMMAND_BATTERY_HEAD, REPORT_NUM);
    /// keys and corresponding values
    sprintf(buf, ",%d,%d,%d,%d,%d,%d\r", LEFT_UNIT_REPORT, level,
            RIGHT_UNIT_REPORT, level, BOX_REPORT, 9);

    len = strlen(at_cmd);
    if ((len + strlen(buf)) > ATCMD_SZ)
        return -1;

    strcpy(&at_cmd[len], buf);
    /// send AT command
    return btif_hf_send_at_cmd(chan_h, at_cmd);
}

bt_status_t Send_customer_phone_feature_support_AT_command(btif_hf_channel_t* chan_h, uint8_t val)
{
    char at_cmd[ATCMD_SZ] = {'\0'};

    TRACE(0,"Send_customer_phone_feature_support_AT_command.");
	/// keys and corresponding values
    sprintf(at_cmd, "%s%d\r", HF_COMMAND_FEATURE_HEAD, val);
	/// send AT command
    return btif_hf_send_at_cmd(chan_h, at_cmd);
}

bt_status_t Send_customer_version_report_AT_command(btif_hf_channel_t* chan_h)
{
    char at_cmd[ATCMD_SZ] = {'\0'};
    char buf[30] = {'\0'};
    uint8_t len;

    TRACE(0,"Send version report at commnad.");
    /// head and keyNumber
    sprintf(at_cmd, "%s%d", HF_COMMAND_VERSION_HEAD, REPORT_NUM);
    /// keys and corresponding values
    sprintf(buf, ",%d,%d,%d,%d,%d,%d\r", LEFT_UNIT_REPORT, 0x1111,
            RIGHT_UNIT_REPORT, 0x2222, BOX_REPORT, 0x3333);
    len = strlen(at_cmd);
    if ((len + strlen(buf)) > ATCMD_SZ)
        return -1;

    strcpy(&at_cmd[len], buf);
    /// send AT command
    return btif_hf_send_at_cmd(chan_h, at_cmd);
}

#endif

#ifdef __INTERCONNECTION__
uint8_t ask_is_selfdefined_battery_report_AT_command_support(void)
{
    char at_cmd[ATCMD_SZ] = {'\0'};

    TRACE(0,"ask if mobile support self-defined at commnad.");
    uint8_t *support = app_battery_get_mobile_support_self_defined_command_p();
    *support = 0;

    sprintf(at_cmd, "%s?\r", HF_COMMAND_HUAWEI_BATTERY_HEAD);
    btif_hf_send_at_cmd((btif_hf_channel_t*)app_bt_get_device(BT_DEVICE_ID_1)->hf_channel, at_cmd);

    return 0;
}

uint8_t send_selfdefined_battery_report_AT_command(uint8_t device_id)
{
    uint8_t *support = app_battery_get_mobile_support_self_defined_command_p();
    uint8_t batteryInfo = 0;
    char at_cmd[ATCMD_SZ] = {'\0'};
    char buf[30] = {'\0'};

    if(*support)
    {
        app_battery_get_info(NULL, &batteryInfo, NULL);

        /// head and keyNumber
        sprintf(at_cmd, "%s%d", HF_COMMAND_HUAWEI_BATTERY_HEAD, BATTERY_REPORT_NUM);

        /// keys and corresponding values
        sprintf(buf, ",%d,%d,%d,%d\r", BATTERY_REPORT_KEY_LEFT_BATTERY_LEVEL,
                batteryInfo & 0x7f,
                BATTERY_REPORT_KEY_LEFT_CHARGE_STATE,
                (batteryInfo & 0x80) ? 1 : 0);
        uint8_t len;

        len = strlen(at_cmd);

        if ((len + strlen(buf)) > ATCMD_SZ)
            return -1;

        strcpy(&at_cmd[len], buf);

        /// send AT command
        btif_hf_send_at_cmd((btif_hf_channel_t*)app_bt_get_device(device_id)->hf_channel, at_cmd);
    }
    return 0;
}
#endif

extern bool get_force_report_battery_flag(uint8_t hfp_device);

int app_hfp_battery_report(uint8_t level)
{
    bt_status_t status = BT_STS_LAST_CODE;
    btif_hf_channel_t* chan;

    uint8_t i;
    int nRet = 0;

    if (level>9)
        return -1;

    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        chan = app_bt_get_device(i)->hf_channel;
        if (btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN)
        {
            if (!besbt_cfg.apple_hf_at_support || btif_hf_is_hf_indicators_support(chan))
            {
                if (app_bt_get_device(i)->battery_level != level)
                {
                    status = btif_hf_update_indicators_batt_level(chan, level*10); //battery range 0~100
                }
            }
            else if (btif_hf_is_batt_report_support(chan))
            {
                if (app_bt_get_device(i)->battery_level != level)
                {
                #ifdef GFPS_ENABLED
                    app_fp_msg_send_battery_levels(i);
                #endif
#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
                    status = Send_customer_battery_report_AT_command(chan, level);
#endif
                    status = btif_hf_batt_report(chan, level); //iphoneaccev battery range 0~9
                }
                else if(get_force_report_battery_flag(i))
                {
                    TRACE(1, "d(%d) force report battery level: %d", i, level);
                    status = btif_hf_batt_report(chan, level);
                }
            }

            if (BT_STS_PENDING == status){
                app_bt_get_device(i)->battery_level = level;
            }
            else
            {
                nRet = -1;
            }
        }
        else
        {
             app_bt_get_device(i)->battery_level = 0xff;
             nRet = -1;
        }
    }
#if defined(BT_SOURCE)
    app_hfp_ag_battery_report(level);
#endif
    return nRet;
}

extern bool get_force_report_battery_flag(uint8_t hfp_device);
extern void set_force_report_battery_flag(uint8_t hfp_device, bool flag);

void app_hfp_battery_report_proc(void)
{
	uint8_t i = 0;
    static uint8_t last_battery_level = 0xff;
    
    if(last_battery_level != 0xff)
    {
        for(i = 0; i < BT_DEVICE_NUM; i++)
        {
            if(get_force_report_battery_flag(i) == true)
            {
                app_hfp_battery_report(last_battery_level);
                set_force_report_battery_flag(i, false);
                return;
            }
        }
    }

    if(report_battery_level != 0xff)
    {
        app_hfp_battery_report(report_battery_level);
        last_battery_level = report_battery_level;
        report_battery_level = 0xff;
    }
}

bt_status_t app_hfp_send_at_command(const char *cmd)
{
    bt_status_t ret = 0;
    //send AT command
    ret = btif_hf_send_at_cmd((btif_hf_channel_t*)app_bt_get_device(BT_DEVICE_ID_1)->hf_channel, cmd);

    return ret;
}

#endif

static void app_hfp_handle_call_hold(btif_hf_channel_t* chan_h, uint32_t action)
{
    btif_hf_call_hold(chan_h, (btif_hf_hold_call_t)action, 0);
}

void app_hfp_send_call_hold_request(uint8_t device_id, btif_hf_hold_call_t action)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->hf_channel,
            (uint32_t)action, (uint32_t)(uintptr_t)app_hfp_handle_call_hold);
    }
}

void app_hfp_report_battery_hf_indicator(uint8_t device_id, uint8_t level)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->hf_channel,
            (uint32_t)level, (uint32_t)(uintptr_t)btif_hf_update_indicators_batt_level);
    }
}

void app_hfp_report_enhanced_safety(uint8_t device_id, uint8_t value)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->hf_channel,
            (uint32_t)value, (uint32_t)(uintptr_t)btif_hf_report_enhanced_safety);
    }
}

void app_bt_hf_send_at_command(uint8_t device_id, const char* at_str)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (!curr_device || !curr_device->hf_conn_flag)
    {
        TRACE(2, "(d%x) %s invalid device", device_id, __func__);
        return;
    }
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->hf_channel, (uint32_t)at_str, (uint32_t)(uintptr_t)btif_hf_send_at_cmd);
}

void app_bt_hf_create_sco_directly(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (!curr_device || !curr_device->hf_conn_flag)
    {
        TRACE(2, "(d%x) %s invalid device", device_id, __func__);
        return;
    }
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->hf_channel, (uint32_t)NULL, (uint32_t)(uintptr_t)btif_hf_create_sco_directly);
}

bool app_bt_is_hfp_disconnected(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    return curr_device && (0 == curr_device->hf_conn_flag);
}

void a2dp_get_curStream_remDev(btif_remote_device_t   **p_remDev);

bool app_hfp_curr_audio_up(btif_hf_channel_t* hfp_chnl)
{
    int i = 0;
    struct BT_DEVICE_T* curr_device = NULL;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device->hf_channel == hfp_chnl) {
            return curr_device->hf_conn_flag && curr_device->hf_audio_state == BTIF_HF_AUDIO_CON;
        }
    }
    return false;
}

uint8_t  app_hfp_get_chnl_via_remDev(btif_hf_channel_t** p_hfp_chnl)
{
    uint8_t found_device_id = BT_DEVICE_INVALID_ID;

    if (BT_DEVICE_NUM > 1)
    {
        btif_remote_device_t   *p_a2dp_remDev;
        btif_remote_device_t   *p_hfp_remDev;
        a2dp_get_curStream_remDev(&p_a2dp_remDev);
        for(int i=0; i< BT_DEVICE_NUM; i++)
        {
            p_hfp_remDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_get_device(i)->hf_channel);
            if(p_hfp_remDev == p_a2dp_remDev)
            {
                found_device_id = i;
                break;
            }
        }
    }
    else
    {
        found_device_id = BT_DEVICE_ID_1;
    }

    if (found_device_id != BT_DEVICE_INVALID_ID)
    {
        *p_hfp_chnl = app_bt_get_device(found_device_id)->hf_channel;
    }

    return found_device_id;
}

#ifdef SUPPORT_SIRI
extern int open_siri_flag ;
int app_hfp_siri_voice(bool en)
{
    static enum BT_DEVICE_ID_T hf_id = (enum BT_DEVICE_ID_T)BT_DEVICE_INVALID_ID;
    bt_status_t res = BT_STS_LAST_CODE;

    btif_hf_channel_t* POSSIBLY_UNUSED hf_siri_chnl = NULL;
    if(open_siri_flag == 1){
        if(btif_hf_is_voice_rec_active(app_bt_get_device(hf_id)->hf_channel) == false){
            open_siri_flag = 0;
            TRACE(0,"end auto");
        }else{
            TRACE(0,"need close");
            en = false;
        }
    }
    if(open_siri_flag == 0){
        int i = 0;
        for (; i < BT_DEVICE_NUM; i += 1)
        {
            if(btif_hf_is_voice_rec_active(app_bt_get_device(i)->hf_channel))
            {
                TRACE(1, "%d ->close", i+1);
                hf_id = (enum BT_DEVICE_ID_T)i;
                en = false;
                hf_siri_chnl = app_bt_get_device(i)->hf_channel;
                break;
            }
        }
        if (i == BT_DEVICE_NUM)
        {
            open_siri_flag =1;
            en = true;
            hf_id = (enum BT_DEVICE_ID_T)app_hfp_get_chnl_via_remDev(&hf_siri_chnl);
            TRACE(1,"a2dp id = %d",hf_id);
        }
    }
    TRACE(4,"[%s]id =%d/%d/%d",__func__,hf_id,open_siri_flag,en);
    if(hf_id == BT_DEVICE_INVALID_ID)
        hf_id = BT_DEVICE_ID_1;

    if((btif_get_hf_chan_state(app_bt_get_device(hf_id)->hf_channel) == BTIF_HF_STATE_OPEN)) {
        res = btif_hf_enable_voice_recognition(app_bt_get_device(hf_id)->hf_channel, en);
    }

    TRACE(3,"[%s] Line =%d, res = %d", __func__, __LINE__, res);

    return 0;
}
#endif



uint8_t hfp_volume_local_get(enum BT_DEVICE_ID_T id)
{
    uint8_t localVol = AUDIO_OUTPUT_VOLUME_DEFAULT;
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && !nv_record_btdevicerecord_find(&curr_device->remote, &record))
    {
        localVol = record->device_vol.hfp_vol;
    }

    return localVol;
}

uint8_t hfp_convert_local_vol_to_bt_vol(uint8_t localVol)
{
    return unsigned_range_value_map(localVol, TGT_VOLUME_LEVEL_MUTE, TGT_VOLUME_LEVEL_MAX, 0, MAX_HFP_VOL);
}

uint8_t hfp_volume_get(enum BT_DEVICE_ID_T id)
{
    int8_t localVol = hfp_volume_local_get(id);

    return hfp_convert_local_vol_to_bt_vol(localVol);
}

void hfp_volume_local_set(enum BT_DEVICE_ID_T id, uint8_t vol)
{
    nvrec_btdevicerecord *record = NULL;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(id);

    if (curr_device && curr_device->acl_is_connected)
    {
        TRACE(3, "(d%x) %s hfp_vol local %d", id, __func__, vol);

        if (!nv_record_btdevicerecord_find(&curr_device->remote, &record) && record->device_vol.hfp_vol != vol)
        {
            nv_record_btdevicerecord_set_hfp_vol(record, vol);
#ifndef FPGA
            nv_record_touch_cause_flush();
#endif
        }
    }
}

uint8_t hfp_convert_bt_vol_to_local_vol(uint8_t btVol)
{
    return unsigned_range_value_map(btVol, 0, MAX_HFP_VOL, TGT_VOLUME_LEVEL_MUTE, TGT_VOLUME_LEVEL_MAX);
}

void hfp_update_local_volume(enum BT_DEVICE_ID_T id, uint8_t localVol)
{
    // update nv record
    hfp_volume_local_set(id, localVol);

    // update codec if the codec is active
    if (app_audio_manager_hfp_is_active(id))
    {
        app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, localVol);
    }
}

void hfp_volume_set(enum BT_DEVICE_ID_T id, uint8_t btVol)
{
    uint8_t localVol = hfp_convert_bt_vol_to_local_vol(btVol);

    hfp_update_local_volume(id, localVol);

    TRACE(2, "Set hfp bt vol %d local vol %d", btVol, localVol);
}

void hfp_remote_not_support_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    TRACE(1,"(d%x) ::HF_EVENT_REMOTE_NOT_SUPPORT\n", device_id);

    app_bt_profile_connect_manager_hf((enum BT_DEVICE_ID_T)device_id, chan, ctx);
}

static void hfp_connected_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    bt_bdaddr_t peer_addr;
    app_hfp_hf_event_param_t param;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    TRACE(1,"(d%x) ::HF_EVENT_SERVICE_CONNECTED\n", device_id);

#ifdef GFPS_ENABLED
    app_exit_fastpairing_mode();
#endif

    app_bt_clear_connecting_profiles_state(device_id);

    curr_device->switch_sco_to_earbud = 1;
    curr_device->hf_conn_flag = 1;

#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
    app_hfp_battery_report_reset(device_id);
#if defined(__XSPACE_BATTERY_MANAGER__)
    xspace_ui_battery_report_remote_device();
#else
    uint8_t battery_level;
    app_battery_get_info(NULL, &battery_level, NULL);
    app_hfp_set_battery_level(battery_level);
#endif
#endif

    //app_bt_stream_hfpvolume_reset();
    btif_hf_report_speaker_volume(chan, hfp_volume_get((enum BT_DEVICE_ID_T)device_id));

#if defined(HFP_DISABLE_NREC)
    btif_hf_disable_nrec(chan);
#endif

#if defined(BT_MAP_SUPPORT)&&defined(BTIF_DIP_DEVICE)
    if ((btif_dip_get_process_status(app_bt_get_remoteDev(device_id)))&& (app_btmap_check_is_idle((BT_DEVICE_ID_T)device_id)))
    {
        app_btmap_sms_open((BT_DEVICE_ID_T)device_id, &ctx->remote_dev_bdaddr);
    }
#endif

    app_bt_profile_connect_manager_hf((enum BT_DEVICE_ID_T)device_id, chan, ctx);

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_SERVICE_CONNECTED, 0);

    btif_hf_get_remote_bdaddr(chan, &peer_addr);
    param.p.service_connected.device_id = device_id;
    param.p.service_connected.addr      = &peer_addr;
    param.p.service_connected.channel   = chan;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_SERVICE_CONNECTED, &param);

#ifdef __INTERCONNECTION__
    ask_is_selfdefined_battery_report_AT_command_support();
#endif

#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
    Send_customer_phone_feature_support_AT_command(chan,7);
    Send_customer_battery_report_AT_command(chan, battery_level);
#endif
}

static void hfp_disconnected_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    app_hfp_hf_event_param_t param;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    uint32_t sco_is_connected = (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON);

    TRACE(2,"(d%x) ::HF_EVENT_SERVICE_DISCONNECTED reason=%x\n", device_id, ctx->disc_reason);

    btif_hf_set_negotiated_codec(chan, BTIF_HF_SCO_CODEC_CVSD);

    curr_device->hf_conn_flag = 0;
    curr_device->hfchan_call = 0;
    curr_device->hfchan_callSetup = 0;
    curr_device->hf_callheld = 0;
    curr_device->hf_audio_state = BTIF_HF_AUDIO_DISCON;

    app_bt_profile_connect_manager_hf((enum BT_DEVICE_ID_T)device_id, chan, ctx);

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_SERVICE_DISCONNECTED, sco_is_connected);

    param.p.service_disconnected.device_id = device_id;
    param.p.service_disconnected.channel   = chan;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_SERVICE_DISCONNECTED, &param);
}

static void hfp_audio_data_sent_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
#if defined(SCO_LOOP)
    hf_loop_buffer_valid = 1;
#endif
}

static void hfp_audio_data_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    if(app_bt_audio_get_curr_playing_sco() == device_id)
    {
#ifndef _SCO_BTPCM_CHANNEL_
        uint32_t idx = 0;
        if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){
            store_voicebtpcm_m2p_buffer(ctx->audio_data, ctx->audio_data_len);

            idx = hf_sendbuff_ctrl.index % HF_SENDBUFF_MEMPOOL_NUM;
            get_voicebtpcm_p2m_frame(&(hf_sendbuff_ctrl.mempool[idx].buffer[0]), ctx->audio_data_len);
            hf_sendbuff_ctrl.mempool[idx].packet.data = &(hf_sendbuff_ctrl.mempool[idx].buffer[0]);
            hf_sendbuff_ctrl.mempool[idx].packet.dataLen = ctx->audio_data_len;
            hf_sendbuff_ctrl.mempool[idx].packet.flags = BTIF_BTP_FLAG_NONE;
            if(!app_bt_manager.hf_tx_mute_flag) {
                btif_hf_send_audio_data(chan, &hf_sendbuff_ctrl.mempool[idx].packet);
            }
            hf_sendbuff_ctrl.index++;
        }
#endif
    }

#if defined(SCO_LOOP)
    memcpy(hf_loop_buffer + hf_loop_buffer_w_idx*HF_LOOP_SIZE, Info->p.audioData->data, Info->p.audioData->len);
    hf_loop_buffer_len[hf_loop_buffer_w_idx] = Info->p.audioData->len;
    hf_loop_buffer_w_idx = (hf_loop_buffer_w_idx+1)%HF_LOOP_CNT;
    ++hf_loop_buffer_size;

    if (hf_loop_buffer_size >= 18 && hf_loop_buffer_valid == 1) {
        hf_loop_buffer_valid = 0;
        idx = hf_loop_buffer_w_idx-17<0?(HF_LOOP_CNT-(17-hf_loop_buffer_w_idx)):hf_loop_buffer_w_idx-17;
        pkt.flags = BTP_FLAG_NONE;
        pkt.dataLen = hf_loop_buffer_len[idx];
        pkt.data = hf_loop_buffer + idx*HF_LOOP_SIZE;
        HF_SendAudioData(Chan, &pkt);
    }
#endif
}

static void hfp_call_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    uint8_t prev_hf_call = 0;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    TRACE(4,"(d%x) ::HF_EVENT_CALL_IND %d curr_hfp_device %x\n", device_id, ctx->call, app_bt_audio_get_curr_hfp_device());

    prev_hf_call = curr_device->hfchan_call;
    curr_device->hfchan_call = ctx->call;

    TRACE(2, "(d%x) hfp state: %s\n", device_id, app_bt_hf_get_all_device_state());

    if(ctx->call == BTIF_HF_CALL_NONE)
    {
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("Call hangup ok.");
#endif
        curr_device->hf_callheld = BTIF_HF_CALL_HELD_NONE;
    }
    else if(ctx->call == BTIF_HF_CALL_ACTIVE)
    {
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("Call setup ok.");
#endif
    }
    else
    {
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("Call hangup ok.");
#endif
    }

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_CALL_IND, prev_hf_call);
}

static void hfp_callsetup_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    uint8_t prev_hf_callsetup = 0;

    if (BTIF_HF_CALL_SETUP_NONE != ctx->call_setup)
    {
        // exit sniff mode and stay active
        app_bt_active_mode_set(ACTIVE_MODE_KEEPEER_SCO_STREAMING, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
    }
    else
    {
        // resume sniff mode
        app_bt_active_mode_clear(ACTIVE_MODE_KEEPEER_SCO_STREAMING, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
    }

    prev_hf_callsetup = curr_device->hfchan_callSetup;
    curr_device->hfchan_callSetup = ctx->call_setup;

    TRACE(3, "(d%x) ::HF_EVENT_CALLSETUP_IND %d %s\n", device_id, ctx->call_setup, app_bt_hf_get_all_device_state());

    /////call is alert so remember this time
    if(curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_ALERT) {
        TRACE(1,"HF CALLSETUP TIME=%d",hal_sys_timer_get());
        //curr_device->hf_callsetup_alert_time = hal_sys_timer_get();
    }

    if(curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN) {
        btif_hf_list_current_calls(chan);
    }

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_CALLSETUP_IND, prev_hf_callsetup);
}

static void hfp_call_held_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    uint8_t prev_hf_callheld = 0;

    prev_hf_callheld = curr_device->hf_callheld;
    curr_device->hf_callheld = ctx->call_held;

    TRACE(3,"(d%x) ::HF_EVENT_CALLHELD_IND %d %s\n", device_id, ctx->call_held, app_bt_hf_get_all_device_state());

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_CALLHELD_IND, prev_hf_callheld);
}

static void hfp_ring_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    app_hfp_hf_event_param_t param;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    TRACE(2,"(d%x) ::HF_EVENT_RING_IND in-band-ring-tone %d\n", device_id, btif_hf_is_inbandring_enabled(curr_device->hf_channel));

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_RING_IND, 0);

    param.p.ring_ind.channel   = chan;
    param.p.ring_ind.device_id = device_id;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_RING_IND, &param);
}

static void hfp_current_call_state_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    app_hfp_hf_event_param_t param;
    TRACE(2,"(d%x) ::HF_EVENT_CURRENT_CALL_STATE call_number:%s\n", device_id, ctx->call_number);

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_CLCC_IND, 0);

    param.p.caller_id_ind.number           = ctx->call_number;
    if (ctx->call_number == NULL)
        param.p.caller_id_ind.number_len   = 0;
    else
        param.p.caller_id_ind.number_len   = strlen(ctx->call_number);
    param.p.caller_id_ind.channel          = chan;
    param.p.caller_id_ind.device_id        = device_id;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_CALLER_ID_IND, &param);
}

void app_hfp_mute_upstream(uint8_t devId, bool isMute);

void app_hfp_receive_peer_sco_codec(uint8_t device_id, void *chan, uint8_t codec_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    ASSERT(device_id < BT_DEVICE_NUM && curr_device->hf_channel == chan, "hfp device channel must match");
    btif_hf_set_negotiated_codec(curr_device->hf_channel, (hfp_sco_codec_t)codec_id);
    app_audio_manager_set_scocodecid((enum BT_DEVICE_ID_T)device_id, codec_id);
    app_bt_audio_peer_sco_codec_received(device_id);
}

static void hfp_audio_connected_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    app_hfp_hf_event_param_t param;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    if(ctx->status != BT_STS_SUCCESS)
        return;

#ifdef __AI_VOICE__
    app_ai_if_hfp_connected_handler(device_id);
#endif

    btapp_hfp_mic_need_skip_frame_set(2);
#if defined(IBRT)
    uint8_t codec_id = 0;
    if (ctx->sco_codec != 0xff) {
        codec_id = ctx->sco_codec;
        btif_hf_set_negotiated_codec(curr_device->hf_channel, (hfp_sco_codec_t)codec_id);
        TRACE(2,"(d%x) ::HF_EVENT_AUDIO_CONNECTED codec_id:%d\n", device_id, codec_id);
    } else {
        codec_id = btif_hf_get_negotiated_codec(curr_device->hf_channel);
        TRACE(2,"(d%x) ::HF_EVENT_AUDIO_CONNECTED codec_id:%d\n", device_id, codec_id);
    }
    app_audio_manager_set_scocodecid((enum BT_DEVICE_ID_T)device_id, codec_id);
#else
    uint16_t codec_id = btif_hf_get_negotiated_codec(curr_device->hf_channel);
    TRACE(2,"(d%x) ::HF_EVENT_AUDIO_CONNECTED codec_id:%d\n", device_id, codec_id);
    app_audio_manager_set_scocodecid((enum BT_DEVICE_ID_T)device_id, codec_id);
#endif

    curr_device->switch_sco_to_earbud = 0;
    curr_device->hf_audio_state = BTIF_HF_AUDIO_CON;

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_SCO_CONNECTED, 0);

    param.p.audio_connected.device_id = device_id;
    param.p.audio_connected.channel   = chan;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_AUDIO_CONNECTED, &param);

#if defined(__FORCE_REPORTVOLUME_SOCON__)
    btif_hf_report_speaker_volume(chan, hfp_volume_get((enum BT_DEVICE_ID_T)device_id));
#endif

#ifdef __IAG_BLE_INCLUDE__
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_HFP_ON, true);
#endif
}

static void hfp_audio_disconnected_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    app_hfp_hf_event_param_t param;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    TRACE(1,"(d%x) ::HF_EVENT_AUDIO_DISCONNECTED\n", device_id);


#ifdef BT_USB_AUDIO_DUAL_MODE
    if(!btusb_is_bt_mode())
    {
        TRACE(0,"btusb_usbaudio_open doing.");
        btusb_usbaudio_open();
    }
#endif

    if(curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE) {
        curr_device->switch_sco_to_earbud = 1;
    }

    curr_device->hf_audio_state = BTIF_HF_AUDIO_DISCON;

    //app_hfp_mute_upstream(device_id, true);

    /* Dont clear callsetup status when audio disc: press iphone volume button
       will disc audio link, but the iphone incoming call is still exist. The
       callsetup status will be reported after call rejected or answered. */
    //curr_device->hfchan_callSetup = BTIF_HF_CALL_SETUP_NONE;

    app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_SCO_DISCONNECTED, 0);

    param.p.audio_disconnected.device_id = device_id;
    param.p.audio_disconnected.channel   = chan;
    APP_BT_HFP_HF_CB(APP_HFP_HF_EVENT_AUDIO_DISCONNECTED, &param);

#ifdef __IAG_BLE_INCLUDE__
    if (!app_bt_is_in_reconnecting()) {
        app_hfp_resume_ble_adv();
    }
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_HFP_ON, false);
#endif

    app_bt_active_mode_clear(ACTIVE_MODE_KEEPEER_SCO_STREAMING, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
}

void hfp_speak_volume_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    TRACE(2,"::HF_EVENT_SPEAKER_VOLUME  chan_id:%d,speaker gain = %d\n", device_id, ctx->speaker_volume);
    hfp_volume_set((enum BT_DEVICE_ID_T)device_id, (uint8_t)ctx->speaker_volume);
}

static void hfp_voice_rec_state_ind_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    TRACE(2,"::HF_EVENT_VOICE_REC_STATE  chan_id:%d,voice_rec_state = %d\n",
                                    device_id, ctx->voice_rec_state);
}

static void hfp_bes_test_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    //TRACE(0,"HF_EVENT_BES_TEST content =d", Info->p.ptr);
}

static void hfp_read_ag_ind_status_handler(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    TRACE(1,"HF_EVENT_READ_AG_INDICATORS_STATUS %s\n", __func__);
}

static uint8_t skip_frame_cnt = 0;
void app_hfp_set_skip_frame(uint8_t frames)
{
    skip_frame_cnt = frames;
}
uint8_t app_hfp_run_skip_frame(void)
{
    if(skip_frame_cnt >0){
        skip_frame_cnt--;
        return 1;
    }
    return 0;
}
uint8_t hfp_is_service_connected(uint8_t device_id)
{
    return app_bt_get_device(device_id)->hf_conn_flag;
}

static uint8_t upstreamMute = 0xff;
void app_hfp_mute_upstream(uint8_t devId, bool isMute)
{
    if (upstreamMute != isMute){
        TRACE(3,"%s devId %d isMute %d",__func__,devId,isMute);
        upstreamMute = isMute;
        if (isMute){
            btdrv_set_bt_pcm_en(0);
        }else{
            btdrv_set_bt_pcm_en(1);
        }
    }
}

static void app_hfp_audio_closed_delay_resume_ble_adv_timer_cb(void const *n)
{
#ifdef __IAG_BLE_INCLUDE__
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}

#ifdef __IAG_BLE_INCLUDE__
static void app_hfp_resume_ble_adv(void)
{
    if (!app_hfp_audio_closed_delay_resume_ble_adv_timer_id) {
        app_hfp_audio_closed_delay_resume_ble_adv_timer_id =
            osTimerCreate(osTimer(APP_HFP_AUDIO_CLOSED_DELAY_RESUME_BLE_ADV_TIMER),
            osTimerOnce, NULL);
    }

    osTimerStart(app_hfp_audio_closed_delay_resume_ble_adv_timer_id,
        HFP_AUDIO_CLOSED_DELAY_RESUME_ADV_IN_MS);
}
#endif

void app_hfp_bt_driver_callback(uint8_t device_id, hf_event_t event)
{
    struct bt_cb_tag* bt_drv_func_cb = bt_drv_get_func_cb_ptr();
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if(curr_device == NULL)
    {
        ASSERT(0, "hfp device error");
        return;
    }

    switch(event)
    {
        case BTIF_HF_EVENT_AUDIO_CONNECTED:
            if(bt_drv_func_cb->bt_switch_agc != NULL)
            {
                bt_drv_func_cb->bt_switch_agc(BT_HFP_WORK_MODE);
            }
            break;

        case BTIF_HF_EVENT_AUDIO_DISCONNECTED:
            {
                if(bt_drv_func_cb->bt_switch_agc != NULL)
                {
                    bt_drv_func_cb->bt_switch_agc(BT_IDLE_MODE);
                }

                if(app_bt_audio_count_connected_sco() == 0)
                {
                    uint16_t codec_id = btif_hf_get_negotiated_codec(curr_device->hf_channel);
                    bt_drv_reg_op_sco_txfifo_reset(codec_id);
                }

                bt_drv_reg_op_clean_flags_of_ble_and_sco();
            }
            break;

        default:
            break;
    }
}

void app_hfp_event_callback(uint8_t device_id, btif_hf_channel_t* chan, struct hfp_context *ctx)
{
    if (device_id == BT_DEVICE_INVALID_ID && ctx->event == BTIF_HF_EVENT_SERVICE_DISCONNECTED)
    {
        // hfp profile is closed due to acl created fail
        TRACE(1,"::HF_EVENT_SERVICE_DISCONNECTED acl created error=%x\n", ctx->disc_reason);
        return;
    }

    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    ASSERT(device_id < BT_DEVICE_NUM && curr_device->hf_channel == chan, "hfp device channel must match");

    TRACE(3,"(d%x) %s event %d", device_id, __func__, ctx->event);
    switch(ctx->event) {
    case BTIF_HF_EVENT_SERVICE_CONNECTED:
        curr_device->mock_hfp_after_force_disc = false;
        curr_device->ibrt_slave_force_disc_hfp = false;
        hfp_connected_ind_handler(device_id, chan, ctx);
        if (app_bt_is_a2dp_connected(device_id))
        {
            curr_device->profiles_connected_before = true;
        }
        break;
    case BTIF_HF_EVENT_SERVICE_MOCK_CONNECTED:
        if (app_bt_is_a2dp_connected(device_id))
        {
            curr_device->profiles_connected_before = true;
        }
        break;
    case BTIF_HF_EVENT_AUDIO_DATA_SENT:
        hfp_audio_data_sent_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_AUDIO_DATA:
        hfp_audio_data_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_SERVICE_DISCONNECTED:
        hfp_disconnected_ind_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_AUDIO_CONNECTED:
        hfp_audio_connected_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_AUDIO_DISCONNECTED:
        hfp_audio_disconnected_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_BES_TEST:
        hfp_bes_test_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_READ_AG_INDICATORS_STATUS:
        hfp_read_ag_ind_status_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_RING_IND:
        hfp_ring_ind_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_CURRENT_CALL_STATE:
        hfp_current_call_state_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_WAIT_NOTIFY:
        break;
    case BTIF_HF_EVENT_CALL_IND:
        hfp_call_ind_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_CALLSETUP_IND:
        hfp_callsetup_ind_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_CALLHELD_IND:
        hfp_call_held_ind_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_SERVICE_IND:
        break;
    case BTIF_HF_EVENT_SIGNAL_IND:
        break;
    case BTIF_HF_EVENT_ROAM_IND:
        break;
    case BTIF_HF_EVENT_BATTERY_IND:
        break;
    case BTIF_HF_EVENT_SPEAKER_VOLUME:
        hfp_speak_volume_handler(device_id, chan, ctx);
        break;
    case BTIF_HF_EVENT_MIC_VOLUME:
        break;
    case BTIF_HF_EVENT_VOICE_REC_STATE:
        hfp_voice_rec_state_ind_handler(device_id, chan, ctx);
        break;
#ifdef SUPPORT_SIRI
    case BTIF_HF_EVENT_SIRI_STATUS:
        break;
#endif
    case BTIF_HF_EVENT_COMMAND_COMPLETE:
        break;
    case BTIF_HF_EVENT_AT_RESULT_DATA:
        TRACE(2,"(d%x) received AT command: %s", device_id, ctx->ptr);
#ifdef __INTERACTION__
        if(!memcmp(oppo_self_defined_command_response, ctx->ptr, strlen(oppo_self_defined_command_response)))
        {
            for(int i=0; i<BT_DEVICE_NUM; i++)
            {
                chan = app_bt_get_device(i)->hf_channel;
                {
                    TRACE(2,"hf state=%p %d",chan, btif_get_hf_chan_state(chan));
                    if (btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN)
                    {
                        //char firmwareversion[] = "AT+VDRV=3,1,9,2,9,3,9";
                        //sprintf(&firmwareversion[27], "%d", (int)NeonFwVersion[0]);
                        //sprintf(&firmwareversion[28], "%d", (int)NeonFwVersion[1]);
                        //sprintf(&firmwareversion[29], "%d", (int)NeonFwVersion[2]);
                        //btif_hf_send_at_cmd(chan,firmwareversion);
                    }
                }
            }
            TRACE(0,"oppo_self_defined_command_response");
        }
#endif
#ifdef __INTERCONNECTION__
        if (!memcmp(huawei_self_defined_command_response, ctx->ptr, strlen(huawei_self_defined_command_response)+1))
        {
            uint8_t *support = app_battery_get_mobile_support_self_defined_command_p();
            *support = 1;

            TRACE(0,"send self defined AT command to mobile.");
            send_selfdefined_battery_report_AT_command(device_id);
        }
#endif
        break;

    case BTIF_HF_EVENT_REMOTE_NOT_SUPPORT:
#if defined(IBRT) && defined(IBRT_V2_MULTIPOINT)
        bt_bdaddr_t bdaddr;
        hfp_remote_not_support_ind_handler(device_id, chan, ctx);
        ctx->event = BTIF_HF_EVENT_SERVICE_DISCONNECTED;
        if (btif_hf_get_remote_bdaddr(chan, &bdaddr) == true)
        {
            app_tws_profile_remove_from_basic_profiles(&bdaddr,BTIF_APP_HFP_PROFILE_ID);
        }
#endif
	break;
    default:
        break;
    }

    app_hfp_bt_driver_callback(device_id, ctx->event);

    hfcall_next_sta_handler(device_id, ctx->event);

#if defined(IBRT)
    app_tws_ibrt_profile_callback(device_id, BTIF_APP_HFP_PROFILE_ID,(void *)chan, (void *)ctx,&curr_device->remote);
#endif
}

struct btif_hf_cind_value app_bt_hf_get_cind_service_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_service_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_call_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_call_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_callsetup_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_callsetup_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_callheld_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_callheld_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_signal_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_signal_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_roam_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_roam_value(curr_device->hf_channel);
    }
    return value;
}

struct btif_hf_cind_value app_bt_hf_get_cind_battchg_value(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct btif_hf_cind_value value = {0};
    if (curr_device)
    {
        value = btif_hf_get_cind_battchg_value(curr_device->hf_channel);
    }
    return value;
}

uint32_t app_bt_hf_get_ag_features(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    uint32_t ag_features = 0;
    if (curr_device)
    {
        ag_features = btif_hf_get_ag_features(curr_device->hf_channel);
    }
    return ag_features;
}

uint8_t btapp_hfp_get_call_state(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++) {
        if (app_bt_get_device(i)->hfchan_call == BTIF_HF_CALL_ACTIVE) {
            return 1;
        }
    }
    return 0;
}

uint8_t btapp_hfp_get_call_setup(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++){
        if ((app_bt_get_device(i)->hfchan_callSetup != BTIF_HF_CALL_SETUP_NONE)){
            return (app_bt_get_device(i)->hfchan_callSetup);
        }
    }
    return 0;
}

uint8_t btapp_hfp_incoming_calls(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++) {
        if (app_bt_get_device(i)->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN) {
            return 1;
        }
    }
    return 0;
}

bool btapp_hfp_is_call_active(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++){
        if (app_bt_get_device(i)->hf_audio_state == BTIF_HF_AUDIO_CON) {
            return true;
        }
    }
    return false;
}

bool btapp_hfp_is_sco_active(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++) {
        if (app_bt_get_device(i)->hf_audio_state == BTIF_HF_AUDIO_CON) {
            return true;
        }
    }
    return false;
}

uint8_t btapp_hfp_get_call_active(void)
{
    uint8_t i;
    for (i = 0; i < BT_DEVICE_NUM; i++) {
        if ((app_bt_get_device(i)->hfchan_call == BTIF_HF_CALL_ACTIVE) ||
            (app_bt_get_device(i)->hfchan_callSetup == BTIF_HF_CALL_SETUP_ALERT)){

            return 1;
        }
    }
    return 0;
}

bool btapp_hfp_is_dev_sco_connected(uint8_t devId)
{
    return (app_bt_get_device(devId)->hf_audio_state == BTIF_HF_AUDIO_CON);
}

void btapp_hfp_report_speak_gain(void)
{
    uint8_t i;
    btif_remote_device_t *remDev = NULL;
    btif_link_mode_t mode = BTIF_BLM_SNIFF_MODE;
    btif_hf_channel_t* chan;

    for(i = 0; i < BT_DEVICE_NUM; i++) {
        remDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_get_device(i)->hf_channel);
        if (remDev){
            mode = btif_me_get_current_mode(remDev);
        } else {
            mode = BTIF_BLM_SNIFF_MODE;
        }
        chan = app_bt_get_device(i)->hf_channel;
        if ((btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN) &&
                                (mode == BTIF_BLM_ACTIVE_MODE)) {
            btif_hf_report_speaker_volume(chan, hfp_volume_get((enum BT_DEVICE_ID_T)i));
        }
    }
}

uint8_t btapp_hfp_need_mute(void)
{
    return app_bt_manager.hf_tx_mute_flag;
}

int32_t hfp_mic_need_skip_frame_cnt = 0;

bool btapp_hfp_mic_need_skip_frame(void)
{
    bool nRet;

    if (hfp_mic_need_skip_frame_cnt > 0) {
        hfp_mic_need_skip_frame_cnt--;
        nRet = true;
    } else {
        app_hfp_mute_upstream(0, false);
        nRet = false;
    }
    return nRet;
}

void btapp_hfp_mic_need_skip_frame_set(int32_t skip_frame)
{
    hfp_mic_need_skip_frame_cnt = skip_frame;
}

bool app_hfp_sco_request_handler(uint8_t device_id, const uint8_t *remote)
{
    return app_bt_audio_event_handler(device_id, APP_BT_AUDIO_EVENT_HFP_SCO_CONNECT_REQ, (uint32_t)(uintptr_t)remote);
}

static bool g_hfp_is_inited = false;

void app_hfp_init(void)
{
#ifdef IBRT
    btif_hfp_register_peer_sco_codec_receive_handler(app_hfp_receive_peer_sco_codec);
#endif

    btif_hfp_initialize();

    if (g_hfp_is_inited)
    {
        return;
    }

    g_hfp_is_inited = true;

    if (besbt_cfg.bt_sink_enable)
    {
        for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
            struct BT_DEVICE_T* curr_device = app_bt_get_device(i);
            curr_device->hf_channel = btif_hf_alloc_channel();
            curr_device->profile_mgr.chan = curr_device->hf_channel;
            if (!curr_device->hf_channel) {
                ASSERT(0, "Serious error: cannot alloc hf channel\n");
            }
            btif_hf_init_channel(curr_device->hf_channel);
            curr_device->hfchan_call = 0;
            curr_device->hfchan_callSetup = 0;
            curr_device->hf_callheld = 0;
            curr_device->hf_audio_state = BTIF_HF_AUDIO_DISCON,
            curr_device->hf_conn_flag = false;
            curr_device->battery_level = 0xff;
        }

        btif_hf_register_callback(app_hfp_event_callback);

        btif_me_register_sco_request_handler(app_hfp_sco_request_handler);
    }

#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
    Besbt_hook_handler_set(BESBT_HOOK_USER_3, app_hfp_battery_report_proc);
#endif

    app_bt_manager.curr_hf_channel_id = BT_DEVICE_ID_1;
    app_bt_manager.hf_tx_mute_flag = 0;

    app_hfp_mute_upstream(app_bt_manager.curr_hf_channel_id, true);
}

void app_hfp_enable_audio_link(bool isEnable)
{
    return;
}

void app_hfp_hf_register_callback(app_hfp_hf_callback_t cb)
{
    g_app_bt_hfp_hf_cb = cb;
}

#if defined(IBRT)
int hfp_ibrt_service_connected_mock(uint8_t device_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if (btif_get_hf_chan_state(curr_device->hf_channel) == BTIF_HF_STATE_OPEN){
        TRACE(1,"(d%x) ::HF_EVENT_SERVICE_CONNECTED mock", device_id);
        curr_device->hf_conn_flag = 1;
        struct hfp_context ctx = {0};
        ctx.status = BT_STS_SUCCESS;
        ctx.remote_dev_bdaddr = curr_device->remote;
        ctx.event = BTIF_HF_EVENT_SERVICE_MOCK_CONNECTED;
        ctx.call = curr_device->hfchan_call;
        ctx.call_setup = curr_device->hfchan_callSetup;
        ctx.state = BTIF_HF_STATE_OPEN;
        app_hfp_event_callback(device_id, curr_device->hf_channel, &ctx);
    }else{
        TRACE(2,"(d%x) ::HF_EVENT_SERVICE_CONNECTED mock need check chan_state:%d", device_id, btif_get_hf_chan_state(curr_device->hf_channel));
    }

    return 0;
}

#ifdef IBRT_UI_V1
int hfp_ibrt_sync_get_status(ibrt_hfp_status_t *hfp_status)
{
    hfp_status->audio_state = (uint8_t)app_bt_get_device(BT_DEVICE_ID_1)->hf_audio_state;
    hfp_status->localVolume = hfp_volume_local_get(BT_DEVICE_ID_1);
    hfp_status->lmp_sco_hdl = 0;

    if (hfp_status->audio_state == BTIF_HF_AUDIO_CON &&
        app_tws_ibrt_mobile_link_connected())
    {
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        uint16_t sco_connhdl = btif_me_get_scohdl_by_connhdl(p_ibrt_ctrl->mobile_conhandle);
        hfp_status->lmp_sco_hdl = bt_drv_reg_op_lmp_sco_hdl_get(sco_connhdl);
        TRACE(2,"ibrt_ui_log:get sco lmp hdl %04x %02x\n", sco_connhdl, hfp_status->lmp_sco_hdl);
    }

    return 0;
}

int hfp_ibrt_sync_set_status(ibrt_hfp_status_t *hfp_status)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    btif_remote_device_t *remDev = NULL;

    TRACE(5,"%s audio_state:%d localVolume:%d lmp_scohdl:%02x sync_ctx %d", __func__, hfp_status->audio_state,
        hfp_status->localVolume, hfp_status->lmp_sco_hdl, hfp_status->sync_ctx);

    /* ibrt slave also set this state when HF_EVENT_AUDIO_CONNECTED/DISCONNECTED
       so it is no need to sync this state, and if sync there is a problem:
       1. ibrt master send the audio state whent its sco connected
       2. but when slave received this command, the sco is already disc
       3. so reset this state to BTIF_HF_AUDIO_CON is a mistake
     */

    if (app_tws_ibrt_mobile_link_connected()){
        remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle);
    }else if (app_tws_ibrt_slave_ibrt_link_connected()){
        remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->ibrt_conhandle);
    }

    if (remDev){
        app_bt_stream_volume_ptr_update((uint8_t *) btif_me_get_remote_device_bdaddr(remDev));
    }else{
        app_bt_stream_volume_ptr_update(NULL);
    }

    /* only sync volume when profile exchanged done, dont sync volume when sco connected
       in case the synced volume override the volume phone send:
       ---
       (ibrt master)
        82819/  t | ::HF_EVENT_AUDIO_CONNECTED  chan_id:0, codec_id:2
        82847/  t | ibrt_ui_log:app_ibrt_sync_hfp_status
        82907/  t | ibrt_ui_log:TXDONE:00c8 cmdcode=SYNC_HFP_STATUS
        82967/  t | #### HSHF_SPK_VOL_IND
        82967/  t | ::HF_EVENT_SPEAKER_VOLUME  chan_id:0,speaker gain = 7
        82968/  t | app_bt_stream_volumeset vol=9
        82969/  t | hfp put vol raw:7 loc:9
        83332/  t | ibrt_ui_log:<-----------RCV_RSP:00c8 cmdcode=SYNC_HFP_STATUS-------

       (ibrt slave)
        85678/  t | #### HSHF_SPK_VOL_IND
        85678/  t | ::HF_EVENT_SPEAKER_VOLUME  chan_id:0,speaker gain = 7
        85678/  t | app_bt_stream_volumeset vol=9
        85679/  t | hfp put vol raw:7 loc:9
        85838/  t | ibrt_ui_log:<-----------RCV:00c8 cmdcode=SYNC_HFP_STATUS-------
        85838/  t | hfp_ibrt_sync_set_status audio_state:1 volume:15 lmp_scohdl:00
        85839/  t | app_bt_stream_volumeset vol=17
        85840/  t | hfp put vol raw:15 loc:17
        85840/  t | ibrt_ui_log:-------TRS:00c8 RSP cmdcode=SYNC_HFP_STATUS------->
     */

    if(hfp_status->sync_ctx == HFP_SYNC_CTX_PROFILE_TXDONE)
    {
        hfp_update_local_volume(BT_DEVICE_ID_1, hfp_status->localVolume);
    }

    p_ibrt_ctrl->ibrt_sco_lmphdl = 0;

    if (hfp_status->audio_state == BTIF_HF_AUDIO_CON &&
        hfp_status->lmp_sco_hdl != 0 &&
        app_tws_ibrt_slave_ibrt_link_connected())
    {
        uint16_t sco_connhdl = 0x0100; //SYNC_HFP_STATUS arrive before mock sniffer_sco, so use 0x0100 directly
        if (bt_drv_reg_op_lmp_sco_hdl_set(sco_connhdl, hfp_status->lmp_sco_hdl))
        {
            TRACE(2,"ibrt_ui_log:set sco %04x lmp hdl %02x\n", sco_connhdl, hfp_status->lmp_sco_hdl);
        }
        else
        {
            // SYNC_HFP_STATUS may so much faster lead bt_drv_reg_op_lmp_sco_hdl_set fail, backup the value
            p_ibrt_ctrl->ibrt_sco_lmphdl = hfp_status->lmp_sco_hdl;
        }
    }

    return 0;
}

int hfp_ibrt_sco_audio_connected(hfp_sco_codec_t codec, uint16_t sco_connhdl)
{
    int ret = BT_STS_SUCCESS;
    struct BT_DEVICE_T* curr_device = app_bt_get_device(BT_DEVICE_ID_1);
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    TRACE(0,"::HF_EVENT_AUDIO_CONNECTED mock");

    curr_device->hf_audio_state = BTIF_HF_AUDIO_CON;
    btif_hf_set_negotiated_codec(curr_device->hf_channel, codec);
    app_audio_manager_set_scocodecid(BT_DEVICE_ID_1,codec);
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_VOICE,BT_DEVICE_ID_1,0);
    app_ibrt_if_sniff_checker_start(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);

    app_tws_ibrt_hfp_bt_driver_callback_v1(BTIF_HF_EVENT_AUDIO_MOCK_CONNECTED, p_ibrt_ctrl->ibrt_conhandle);

    if (p_ibrt_ctrl->ibrt_sco_lmphdl != 0)
    {
        // set backuped lmphdl if it is failed in hfp_ibrt_sync_set_status
        TRACE(2,"ibrt_ui_log:set sco %04x lmp hdl %02x\n", sco_connhdl, p_ibrt_ctrl->ibrt_sco_lmphdl);
        bt_drv_reg_op_lmp_sco_hdl_set(sco_connhdl, p_ibrt_ctrl->ibrt_sco_lmphdl);
        p_ibrt_ctrl->ibrt_sco_lmphdl = 0;
    }
    return ret;
}

int hfp_ibrt_sco_audio_disconnected(void)
{
    int ret = BT_STS_SUCCESS;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    struct BT_DEVICE_T* curr_device = app_bt_get_device(BT_DEVICE_ID_1);
    TRACE(0,"::HF_EVENT_AUDIO_DISCONNECTED mock");
    curr_device->hf_audio_state = BTIF_HF_AUDIO_DISCON;
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_VOICE,BT_DEVICE_ID_1,0);
    app_ibrt_if_sniff_checker_stop(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);

    app_tws_ibrt_hfp_bt_driver_callback_v1(BTIF_HF_EVENT_AUDIO_MOCK_DISCONNECTED, p_ibrt_ctrl->ibrt_conhandle);

    return ret;
}

#else //IBRT_V2

int hfp_ibrt_sync_get_status_v2(uint8_t device_id, uint16_t conn_handle, ibrt_hfp_status_t *hfp_status)
{
    hfp_status->audio_state = (uint8_t)app_bt_get_device(device_id)->hf_audio_state;
    hfp_status->localVolume = hfp_volume_local_get((enum BT_DEVICE_ID_T)device_id);
    return 0;
}

int hfp_ibrt_sync_set_status_v2(ibrt_hfp_status_t *hfp_status)
{
    uint8_t device_id = 0;

    TRACE(4,"%s audio_state:%d localVolume:%d  sync_ctx %d", __func__, hfp_status->audio_state,
        hfp_status->localVolume, hfp_status->sync_ctx);

    device_id = btif_me_get_device_id_from_addr(&hfp_status->mobile_addr);

    /* ibrt slave also set this state when HF_EVENT_AUDIO_CONNECTED/DISCONNECTED
       so it is no need to sync this state, and if sync there is a problem:
       1. ibrt master send the audio state whent its sco connected
       2. but when slave received this command, the sco is already disc
       3. so reset this state to BTIF_HF_AUDIO_CON is a mistake
     */

    app_bt_stream_volume_ptr_update((uint8_t *)&hfp_status->mobile_addr);

    /* only sync volume when profile exchanged done, dont sync volume when sco connected
       in case the synced volume override the volume phone send:
       ---
       (ibrt master)
        82819/  t | ::HF_EVENT_AUDIO_CONNECTED  chan_id:0, codec_id:2
        82847/  t | ibrt_ui_log:app_ibrt_sync_hfp_status
        82907/  t | ibrt_ui_log:TXDONE:00c8 cmdcode=SYNC_HFP_STATUS
        82967/  t | #### HSHF_SPK_VOL_IND
        82967/  t | ::HF_EVENT_SPEAKER_VOLUME  chan_id:0,speaker gain = 7
        82968/  t | app_bt_stream_volumeset vol=9
        82969/  t | hfp put vol raw:7 loc:9
        83332/  t | ibrt_ui_log:<-----------RCV_RSP:00c8 cmdcode=SYNC_HFP_STATUS-------

       (ibrt slave)
        85678/  t | #### HSHF_SPK_VOL_IND
        85678/  t | ::HF_EVENT_SPEAKER_VOLUME  chan_id:0,speaker gain = 7
        85678/  t | app_bt_stream_volumeset vol=9
        85679/  t | hfp put vol raw:7 loc:9
        85838/  t | ibrt_ui_log:<-----------RCV:00c8 cmdcode=SYNC_HFP_STATUS-------
        85838/  t | hfp_ibrt_sync_set_status audio_state:1 volume:15 lmp_scohdl:00
        85839/  t | app_bt_stream_volumeset vol=17
        85840/  t | hfp put vol raw:15 loc:17
        85840/  t | ibrt_ui_log:-------TRS:00c8 RSP cmdcode=SYNC_HFP_STATUS------->
     */

    if(hfp_status->sync_ctx == HFP_SYNC_CTX_PROFILE_TXDONE)
    {
        hfp_update_local_volume((enum BT_DEVICE_ID_T)device_id, hfp_status->localVolume);
    }

    return 0;
}
#endif
#endif

