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
#include "cmsis_os.h"
#include <string.h>
#include "app_ibrt_keyboard.h"
#include "hal_trace.h"
#include "app_ibrt_if.h"
#include "audio_policy.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "btapp.h"
#include "apps.h"
#include "app_bt.h"
#include "app_tws_besaud.h"
#include "app_tws_ibrt_conn_api.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#endif

#ifdef BT_HID_DEVICE
#include "app_bt_hid.h"
#endif

#if defined(IBRT)

#ifdef TILE_DATAPATH
extern "C" void app_tile_key_handler(APP_KEY_STATUS *status, void *param);
#endif
extern void app_bt_volumedown();
extern void app_bt_volumeup();
extern void app_otaMode_enter(APP_KEY_STATUS *status, void *param);

struct ibrt_ui_switch_a2dp_usser_action {
    uint8_t action;
    uint32_t trigger_btclk;
};

static void app_ibrt_ui_switch_streaming_a2dp(struct ibrt_ui_switch_a2dp_usser_action *received_action)
{
    if (app_tws_ibrt_tws_link_connected() && tws_besaud_is_connected())
    {
        if (received_action)
        {
            app_bt_audio_trigger_switch_streaming_a2dp(received_action->trigger_btclk);
        }
        else
        {
            uint32_t trigger_btclk = app_bt_audio_trigger_switch_streaming_a2dp(0);
            struct ibrt_ui_switch_a2dp_usser_action action = {IBRT_ACTION_SWITCH_A2DP, trigger_btclk};
            if (trigger_btclk != 0)
            {
                app_ibrt_conn_send_user_action_v2((uint8_t *)&action, sizeof(action));
            }
        }
    }
    else
    {
        app_bt_audio_switch_streaming_a2dp();
    }
}

void app_ibrt_ui_start_perform_a2dp_switching(void)
{
    app_ibrt_ui_switch_streaming_a2dp(NULL);
}

void app_ibrt_handle_longpress_v2(APP_KEY_STATUS *status)
{
    uint8_t playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t sco_count = app_bt_audio_count_connected_sco();
    uint8_t a2dp_count = app_bt_audio_count_streaming_a2dp();

    // switch between sco streaming devices
    if (playing_sco != BT_DEVICE_INVALID_ID && (sco_count > 1 || app_bt_audio_select_another_device_to_create_sco(playing_sco) != BT_DEVICE_INVALID_ID))
    {
        app_ibrt_if_switch_streaming_sco();
        return;
    }

    // switch between a2dp streaming devices
    if (playing_sco == BT_DEVICE_INVALID_ID && sco_count == 0 && a2dp_count > 1)
    {
        app_ibrt_ui_start_perform_a2dp_switching();
        return;
    }    
}

#ifdef IBRT_SEARCH_UI
void app_ibrt_search_ui_handle_key_v2(bt_bdaddr_t *remote, APP_KEY_STATUS *status, void *param)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();

    TRACE(3,"%s %d,%d",__func__, status->code, status->event);


    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
            case APP_KEY_EVENT_CLICK:
                app_ibrt_middleware_handle_click();
                break;

            case APP_KEY_EVENT_DOUBLECLICK:
                TRACE(0,"double kill");
                if(IBRT_UNKNOW==p_ibrt_ctrl->nv_role)
                {
                    app_ibrt_if_init_open_box_state_for_evb();
                    app_start_tws_serching_direactly();
                }
                else
                {
                    bt_key_handle_func_doubleclick();
                }
                break;

            case APP_KEY_EVENT_LONGPRESS:
                break;

            case APP_KEY_EVENT_TRIPLECLICK:
            #ifdef TILE_DATAPATH
                app_tile_key_handler(status,NULL);
            #else
                app_otaMode_enter(NULL,NULL);
            #endif
                break;
            case HAL_KEY_EVENT_LONGLONGPRESS:
                TRACE(0,"long long press");
                app_shutdown();
                break;

            case APP_KEY_EVENT_ULTRACLICK:
                TRACE(0,"ultra kill");
                break;

            case APP_KEY_EVENT_RAMPAGECLICK:
                TRACE(0,"rampage kill!you are crazy!");
                break;

            case APP_KEY_EVENT_UP:
                break;
        }
    }

#ifdef TILE_DATAPATH
    if(APP_KEY_CODE_TILE == status->code)
        app_tile_key_handler(status,NULL);
#endif
}
#else

#if 0
#define RECONNECT_MOBILE_INTERVAL      (5000)
static void app_ibrt_reconnect_mobile_timehandler(void const *param);
osTimerDef (IBRT_RECONNECT_MOBILE, app_ibrt_reconnect_mobile_timehandler);
osTimerId  app_ibrt_mobile_timer = NULL;
bt_bdaddr_t* mobile_addr=NULL;
static void app_ibrt_reconnect_mobile_timehandler(void const *param)
{
    if(mobile_addr!=NULL)
        app_ibrt_if_reconnect_moblie_device(mobile_addr);
}
#endif

void app_ibrt_normal_ui_handle_key_v2(bt_bdaddr_t *remote, APP_KEY_STATUS *status, void *param)
{
    uint8_t conn_devices = 0;
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
            case APP_KEY_EVENT_CLICK:
                TRACE(0,"first blood.");
                app_ibrt_middleware_handle_click();
                break;

            case APP_KEY_EVENT_DOUBLECLICK:
#if 1
                TRACE(0,"double kill,enter freeman mode");
                //app_ibrt_if_init_open_box_state_for_evb();
                app_ibrt_if_enter_freeman_pairing();
#else
                for (int i = 0; i < app_ibrt_conn_support_max_mobile_dev(); ++i)
                {
                    struct BT_DEVICE_T *curr_device = app_bt_get_device(i);
                    if (curr_device->acl_is_connected)
                    {
                        app_ibrt_if_disconnet_moblie_device(&curr_device->remote);
                        mobile_addr=&curr_device->remote;
                        if (app_ibrt_mobile_timer == NULL)
                        {
                            app_ibrt_mobile_timer = osTimerCreate (osTimer(IBRT_RECONNECT_MOBILE), osTimerOnce,NULL);
                        }
                        osTimerStart(app_ibrt_mobile_timer,RECONNECT_MOBILE_INTERVAL);
                    }
                    else
                        TRACE(0,"acl is not connect");
                }
#endif
                break;
            case APP_KEY_EVENT_LONGPRESS:
                conn_devices = app_bt_count_connected_device();
                TRACE(2,"%s conn_devices %d", __func__, conn_devices);
                if (conn_devices > 0)
                {
                    app_ibrt_handle_longpress_v2(status);
                }
                else
                {
                    app_ibrt_if_init_open_box_state_for_evb();
                    app_ibrt_if_enter_pairing_after_tws_connected();
                }
                break;

            case APP_KEY_EVENT_TRIPLECLICK:
#ifdef TILE_DATAPATH
                app_tile_key_handler(status,NULL);
#else
                app_ibrt_if_init_open_box_state_for_evb();
                app_ibrt_if_enter_pairing_after_tws_connected();            
#endif
                break;
                break;

            case HAL_KEY_EVENT_LONGLONGPRESS:
                TRACE(0,"long long press");
                app_shutdown();
                break;

            case APP_KEY_EVENT_ULTRACLICK:
                TRACE(0,"ultra kill");
                break;

            case APP_KEY_EVENT_RAMPAGECLICK:
                TRACE(0,"rampage kill!you are crazy!");
                break;

            case APP_KEY_EVENT_UP:
                break;
        }
    }

#ifdef TILE_DATAPATH
    if(APP_KEY_CODE_TILE == status->code)
        app_tile_key_handler(status,NULL);
#endif
}
#endif


struct ibrt_keyboard_notify_v2_t
{
    bt_bdaddr_t remote;
    APP_KEY_STATUS key_status;
};

int app_ibrt_if_keyboard_notify_v2(bt_bdaddr_t *remote, APP_KEY_STATUS *status, void *param)
{
    struct ibrt_keyboard_notify_v2_t data;
    if (remote && APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(remote)) {
        data.remote = *remote;
    }
    else {
        data.remote = {0};
    }
    
    data.key_status = *status;
    tws_ctrl_send_cmd(APP_TWS_CMD_KEYBOARD_REQUEST, (uint8_t *)&data, sizeof(struct ibrt_keyboard_notify_v2_t));

    return 0;
}

void app_ibrt_keyboard_request_handler_v2(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    struct ibrt_keyboard_notify_v2_t *req = (struct ibrt_keyboard_notify_v2_t *)p_buff;

    if (app_tws_ibrt_mobile_link_connected(&req->remote))
    {
#ifdef IBRT_SEARCH_UI
        app_ibrt_search_ui_handle_key_v2(&req->remote, &req->key_status, NULL);
#else
        app_ibrt_normal_ui_handle_key_v2(&req->remote, &req->key_status, NULL);
#endif
    }
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    app_ai_manager_key_event_handle(&req->key_status, NULL);
#endif
}

#ifdef TOTA_v2
#define IBRT_TOTA_DAT_MAX_SIZE 200
uint8_t g_tota_data_action[IBRT_TOTA_DAT_MAX_SIZE];
bool app_spp_tota_send_data(uint8_t* ptrData, uint16_t length);
#endif

struct ibrt_if_action_header
{
    uint8_t action;
    bt_bdaddr_t remote;
    uint32_t param;
    uint32_t param2;
} __attribute__ ((packed));

void app_ibrt_if_start_user_action_v2(uint8_t device_id, uint8_t action, uint32_t param, uint32_t param2)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    uint16_t action_length = 0;
    uint8_t *action_data = NULL;
    struct ibrt_if_action_header action_header;

    action_header.action = action;
    action_header.remote = curr_device->remote;
    action_header.param = param;
    action_header.param2 = param2;

    action_data = (uint8_t *)&action_header;
    action_length = sizeof(action_header);

#ifdef TOTA_v2
    if (action == IBRT_ACTION_SEND_TOTA_DATA)
    {
        memcpy(g_tota_data_action, (uint8_t *)&action_header, action_length);
        if (action_length + param2 > IBRT_TOTA_DAT_MAX_SIZE)
        {
            TRACE(1, "send tota data length too long %d", param2);
        }
        memcpy(g_tota_data_action+action_length, (void*)(uintptr_t)param, param2);
        action_data = g_tota_data_action;
        action_length += param2;
    }
#endif

    if (tws_besaud_is_connected() && IBRT_SLAVE == app_tws_get_ibrt_role(&curr_device->remote))
    {
        app_ibrt_conn_send_user_action_v2(action_data, action_length);
    }
    else
    {
        app_ibrt_ui_perform_user_action_v2(action_data, action_length);
    }
}

void app_ibrt_if_sync_volume_info_v2(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if(curr_device && curr_device->acl_is_connected && app_tws_ibrt_tws_link_connected())
    {
        TWS_VOLUME_SYNC_INFO_T_V2 volume_info;
        
        volume_info.remote = curr_device->remote;

        volume_info.a2dp_local_volume = a2dp_volume_local_get((enum BT_DEVICE_ID_T)device_id);

        volume_info.hfp_local_volume = hfp_volume_local_get((enum BT_DEVICE_ID_T)device_id);

        TRACE(4, "(d%x) %s: a2dp %d hfp %d", device_id, __func__, volume_info.a2dp_local_volume, volume_info.hfp_local_volume);

        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO, (uint8_t*)&volume_info, sizeof(volume_info));
    }
}

void app_ibrt_ui_perform_user_action_v2(uint8_t *p_buff, uint16_t length)
{
    struct ibrt_if_action_header *action_header = (struct ibrt_if_action_header *)p_buff;
    uint8_t action = action_header->action;
    uint8_t device_id = app_bt_get_device_id_byaddr(&action_header->remote);
    uint32_t param = action_header->param;
    uint32_t param2 = action_header->param2;
    struct BT_DEVICE_T *curr_device = NULL;

    if (device_id == BT_DEVICE_INVALID_ID)
    {
        TRACE(1, "%s invalid device id", __func__);
        return;
    }

    curr_device = app_bt_get_device(device_id);

    switch (action)
    {
        case IBRT_ACTION_PLAY:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PLAY);
            break;
        case IBRT_ACTION_PAUSE:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_PAUSE);
            break;
        case IBRT_ACTION_FORWARD:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_FORWARD);
            break;
        case IBRT_ACTION_BACKWARD:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_BACKWARD);
            break;
        case IBRT_ACTION_AVRCP_VOLUP:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_VOLUME_UP);
            break;
        case IBRT_ACTION_AVRCP_VOLDN:
            app_bt_a2dp_send_key_request(device_id, AVRCP_KEY_VOLUME_DOWN);
            break;
        case IBRT_ACTION_AVRCP_ABS_VOL:
            app_bt_a2dp_send_set_abs_volume(device_id, param & 0x7f);
            break;
        case IBRT_ACTION_HFSCO_CREATE:
            if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
            {
                TRACE(1,"%s cannot create audio link\n", __func__);
            }
            else
            {
                btif_hf_create_audio_link(curr_device->hf_channel);
            }
            break;
        case IBRT_ACTION_HFSCO_DISC:
            if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
            {
                btif_hf_disc_audio_link(curr_device->hf_channel);
            }
            else
            {
                TRACE(1,"%s cannot disc audio link\n", __func__);
            }
            break;
        case IBRT_ACTION_REDIAL:
            if (curr_device->hf_conn_flag)
            {
                btif_hf_redial_call(curr_device->hf_channel);
            }
            else
            {
                TRACE(1,"%s cannot redial call\n", __func__);
            }
            break;
        case IBRT_ACTION_ANSWER:
            if (curr_device->hf_conn_flag)
            {
                btif_hf_answer_call(curr_device->hf_channel);
            }
            else
            {
                TRACE(1,"%s cannot answer call\n", __func__);
            }
            break;
        case IBRT_ACTION_HANGUP:
            if (curr_device->hf_conn_flag)
            {
                btif_hf_hang_up_call(curr_device->hf_channel);
            }
            else
            {
                TRACE(1,"%s cannot hangup call\n", __func__);
            }
            break;
        case IBRT_ACTION_LOCAL_VOLUP:
            app_bt_local_volume_up(app_ibrt_if_sync_volume_info_v2);
            break;
        case IBRT_ACTION_LOCAL_VOLDN:
            app_bt_local_volume_down(app_ibrt_if_sync_volume_info_v2);
            break;
        case IBRT_ACTION_SWITCH_A2DP:
            app_ibrt_ui_switch_streaming_a2dp((struct ibrt_ui_switch_a2dp_usser_action *)p_buff);
            break;
        case IBRT_ACTION_SWITCH_SCO:
            app_bt_audio_switch_streaming_sco();
            break;
        case IBRT_ACTION_TELL_MASTER_CONN_PROFILE:
            app_ibrt_if_profile_connect(device_id, (int)param, param2);
            break;
        case IBRT_ACTION_TELL_MASTER_DISC_PROFILE:
            app_ibrt_if_profile_disconnect(device_id, param);
            break;
        case IBRT_ACTION_TELL_MASTER_DISC_RFCOMM:
            app_ibrt_if_disonnect_rfcomm((struct spp_device *)param, (uint8_t)param2);
            break;
        case IBRT_ACTION_HID_SEND_CAPTURE:
#ifdef BT_HID_DEVICE
            app_bt_hid_capture_process(device_id);
#endif
            break;
#ifdef TOTA_v2
        case IBRT_ACTION_SEND_TOTA_DATA:
            app_spp_tota_send_data((uint8_t*)p_buff+sizeof(struct ibrt_if_action_header), length-sizeof(struct ibrt_if_action_header));
            break;
#endif
        default:
            TRACE(2,"%s unknown user action %d\n", __func__, action);
            break;
    }
}

#endif

