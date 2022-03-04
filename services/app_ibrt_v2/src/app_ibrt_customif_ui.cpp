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
#include "apps.h"
#include "hal_trace.h"
#include "app_ibrt_if.h"
#include "app_ibrt_customif_ui.h"
#include "me_api.h"
#include "app_vendor_cmd_evt.h"
#include "besaud_api.h"
#include "app_battery.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_bt_media_manager.h"
#include "app_spp.h"
#include "app_dip.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#endif
#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif
#include "btgatt_api.h"
#include "app_bt.h"
#include "audio_policy.h"
#include "app_ibrt_configure.h"
#include "app_tws_ibrt_conn_api.h"
#include "dip_api.h"

#ifdef IBRT_UI_V2
#include "ibrt_mgr.h"
#endif

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#include "app_fp_rfcomm.h"
#endif
#ifdef __AI_VOICE__
#include "ai_spp.h"
#endif
#if defined(__XSPACE_UI__)
#include "xspace_ui.h"
#endif
//typedef
void app_ibrt_customif_global_state_callback(ibrt_global_state_change_event *state);
void app_ibrt_customif_a2dp_callback(const bt_bdaddr_t* addr, ibrt_conn_a2dp_state_change *state);
void app_ibrt_customif_hfp_callback(const bt_bdaddr_t* addr, ibrt_conn_hfp_state_change *state);
void app_ibrt_customif_avrcp_callback(const bt_bdaddr_t* addr, ibrt_conn_avrcp_state_change *state);
void app_ibrt_customif_tws_on_paring_state_changed(ibrt_conn_pairing_state state,uint8_t reason_code);
void app_ibrt_customif_tws_on_acl_state_changed(ibrt_conn_tws_conn_state_event *state,uint8_t reason_code);
void app_ibrt_customif_on_mobile_acl_state_changed(const bt_bdaddr_t *addr, ibrt_mobile_conn_state_event *state, uint8_t reason_code);
void app_ibrt_customif_sco_state_changed(const bt_bdaddr_t *addr, ibrt_sco_conn_state_event *state, uint8_t reason_code);
void app_ibrt_customif_on_tws_role_switch_status_ind(const bt_bdaddr_t *addr,ibrt_conn_role_change_state state,ibrt_role_e role);
void app_ibrt_customif_on_ibrt_state_changed(const bt_bdaddr_t *addr, ibrt_connection_state_event* state,ibrt_role_e role, uint8_t reason_code);
void app_ibrt_customif_on_access_mode_changed(btif_accessible_mode_t newAccessibleMode);
void app_ibrt_customif_case_open_run_complete_callback();
void app_ibrt_customif_case_close_run_complete_callback();

void app_ibrt_customif_pairing_mode_entry();
void app_ibrt_customif_pairing_mode_exit();

uint32_t app_ibrt_customif_set_profile_delaytime_on_spp_connect(rfcomm_uuid_t uuid);

#ifdef IBRT_UI_V2
static APP_IBRT_IF_LINK_STATUS_CHANGED_CALLBACK custom_ui_status_changed_cb = {
        .ibrt_global_state_changed      = app_ibrt_customif_global_state_callback,
        .ibrt_a2dp_state_changed        = app_ibrt_customif_a2dp_callback,
        .ibrt_hfp_state_changed         = app_ibrt_customif_hfp_callback,
        .ibrt_avrcp_state_changed       = app_ibrt_customif_avrcp_callback,
        .ibrt_tws_pairing_changed       = app_ibrt_customif_tws_on_paring_state_changed,
        .ibrt_tws_acl_state_changed     = app_ibrt_customif_tws_on_acl_state_changed,
        .ibrt_mobile_acl_state_changed  = app_ibrt_customif_on_mobile_acl_state_changed,
        .ibrt_sco_state_changed         = app_ibrt_customif_sco_state_changed,
        .ibrt_tws_role_switch_status_ind = app_ibrt_customif_on_tws_role_switch_status_ind,
        .ibrt_ibrt_state_changed        = app_ibrt_customif_on_ibrt_state_changed,
        .ibrt_access_mode_changed       = app_ibrt_customif_on_access_mode_changed,
    };

static APP_IBRT_IF_PAIRING_MODE_CHANGED_CALLBACK custom_ui_pairing_mode_cb = {
    .ibrt_pairing_mode_entry_func = app_ibrt_customif_pairing_mode_entry,
    .ibrt_pairing_mode_exit_func = app_ibrt_customif_pairing_mode_exit,
};

static APP_IBRT_IF_CASE_EVENT_COMPLETE_CALLBACK case_event_complete_cb = {
    .ibrt_case_open_run_complete_callback = app_ibrt_customif_case_open_run_complete_callback,
    .ibrt_case_close_run_complete_callback = app_ibrt_customif_case_close_run_complete_callback,
};

static uint8_t g_device_id_need_resume_sco = BT_DEVICE_INVALID_ID;

#endif

const app_ibrt_customif_remote_device_t no_hfp_device_list[NO_HFP_DEVICE_LIST_MAX_NUM] = 
{
    {0x46, 0x00, "Sony A55"},
};

typedef bool (*select_disc_dev_func)(const bt_bdaddr_t* address_0,const bt_bdaddr_t* address_1);

bool set_disc_dev_on_streaming_changed(const bt_bdaddr_t* address_0,const bt_bdaddr_t* address_1)
{
    if(memcmp((uint8_t*)&address_0->address[0],(uint8_t*)&address_1->address[0],BD_ADDR_LEN))
    {
        TRACE(0,"custom_ui:set disc dev on streaming changed");
        DUMP8("%02x ", address_1->address, BT_ADDR_OUTPUT_PRINT_NUM);
        return true;
    }
    else
    {
        return false;
    }
}

bool set_disc_dev_on_status_changed(const bt_bdaddr_t* address_0,const bt_bdaddr_t* address_1)
{
    if(memcmp((uint8_t*)&address_0->address[0],(uint8_t*)&address_1->address[0],BD_ADDR_LEN))
    {
        TRACE(0,"custom_ui:set disc dev on status changed");
        DUMP8("%02x ", address_1->address, BT_ADDR_OUTPUT_PRINT_NUM);
        return true;
    }
    else
    {
        return false;
    }
}

void app_ibrt_customif_ui_vender_event_handler_ind(uint8_t evt_type, uint8_t *buffer, uint8_t length)
{
    uint8_t subcode = evt_type;

    switch (subcode)
    {
        case HCI_DBG_SNIFFER_INIT_CMP_EVT_SUBCODE:
            break;

        case HCI_DBG_IBRT_CONNECTED_EVT_SUBCODE:
            break;

        case HCI_DBG_IBRT_DISCONNECTED_EVT_SUBCODE:
            break;

        case HCI_DBG_IBRT_SWITCH_COMPLETE_EVT_SUBCODE:
            break;

        case HCI_NOTIFY_CURRENT_ADDR_EVT_CODE:
            break;

        case HCI_DBG_TRACE_WARNING_EVT_CODE:
            break;

        case HCI_SCO_SNIFFER_STATUS_EVT_CODE:
            break;

        case HCI_DBG_RX_SEQ_ERROR_EVT_SUBCODE:
            break;

        case HCI_LL_MONITOR_EVT_CODE:
            app_ibrt_if_update_link_monitor_info(&buffer[3]);
            break;

        case HCI_GET_TWS_SLAVE_MOBILE_RSSI_CODE:
            break;

        default:
            break;
    }
}

void app_ibrt_customif_update_steal_device(select_disc_dev_func func,const bt_bdaddr_t* addr)
{
    uint8_t device_count = 0;
    bt_bdaddr_t device_list[BT_DEVICE_NUM];
    bt_bdaddr_t active_device_addr;

    device_count = app_ibrt_if_get_mobile_connected_dev_list(device_list);
    if(device_count == app_ibrt_if_get_support_max_remote_link())
    {
        if(!app_ibrt_if_get_active_device(&active_device_addr))
        {
            TRACE(0,"custom_ui:no get active device,addr parameter no changed");
            memcpy((uint8_t*)&active_device_addr.address[0],(uint8_t*)&addr->address[0],BD_ADDR_LEN);
        }

        for(uint8_t idx = 0;idx < device_count;idx++)
        {
            if(func(&active_device_addr,&device_list[idx]))
            {
                TRACE(0,"custom_ui:set next disconnect dev:");
                DUMP8("%02x ", active_device_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
                app_ibrt_if_set_disc_dev_if_3rd_dev_request(&device_list[idx]);
            }
        }
    }
    else
    {
        app_ibrt_if_set_disc_dev_if_3rd_dev_request(NULL);
    }
}
#ifdef IBRT_UI_V2
/*****************************************************************************
 Prototype    : app_ibrt_customif_pairing_mode_entry
 Description  : indicate custom ui TWS pairing state entry
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/15
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_customif_pairing_mode_entry()
{
    ibrt_mgr_config_t* ibrt_mgr_config = ibrt_mgr_get_config();
    uint8_t select_sco_device = app_bt_audio_select_streaming_sco();
    struct BT_DEVICE_T *curr_device = NULL;

    TRACE(1,"custom_ui pairing mode entry: disc_sco_during_paring %d", ibrt_mgr_config->disc_sco_during_pairing);

    if (ibrt_mgr_config->disc_sco_during_pairing && select_sco_device != BT_DEVICE_INVALID_ID)
    {
        g_device_id_need_resume_sco = select_sco_device;

        for (int i = 0; i < BT_DEVICE_NUM; ++i)
        {
            curr_device = app_bt_get_device(i);
            if (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON)
            {
                app_ibrt_if_hf_disc_audio_link(i);
            }
        }
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_customif_pairing_mode_exit
 Description  : indicate custom ui TWS pairing state exit
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/15
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_customif_pairing_mode_exit()
{
    ibrt_mgr_config_t* ibrt_mgr_config = ibrt_mgr_get_config();
    uint8_t resume_sco_device = g_device_id_need_resume_sco;

    TRACE(1,"custom_ui pairing mode exit: resume_sco_device %x", resume_sco_device);

    if (ibrt_mgr_config->disc_sco_during_pairing && resume_sco_device != BT_DEVICE_INVALID_ID)
    {
        app_ibrt_if_hf_create_audio_link(resume_sco_device);
    }

    g_device_id_need_resume_sco = BT_DEVICE_INVALID_ID;
}
#endif
void app_ibrt_customif_global_state_callback(ibrt_global_state_change_event *state)
{
    TRACE(2, "custom_ui global_status changed = %d", state->state);

    switch (state->state)
    {
        case IBRT_BLUETOOTH_ENABLED:
            break;
        case IBRT_BLUETOOTH_DISABLED:
            break;
        default:
            break;
    }
}

void app_ibrt_customif_a2dp_callback(const bt_bdaddr_t* addr, ibrt_conn_a2dp_state_change *state)
{
    TRACE(2, "(d%x) custom_ui a2dp_status changed = %d", state->device_id, state->a2dp_state);
    DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);

#ifdef __IAG_BLE_INCLUDE__
    if (state->a2dp_state == IBRT_CONN_A2DP_IDLE || state->a2dp_state == IBRT_CONN_A2DP_CLOSED)
    {
        if (app_bt_is_a2dp_disconnected(state->device_id) && app_bt_is_hfp_disconnected(state->device_id) &&
            app_bt_is_profile_connected_before(state->device_id)){

            if(btif_is_gatt_over_br_edr_enabled() &&
                btif_btgatt_is_connected(state->device_id))
            {
                btif_btgatt_disconnect(state->device_id);
            }
#ifdef __AI_VOICE__
        ai_spp_enumerate_disconnect_service((uint8_t*)addr,state->device_id);
#endif
#ifdef GFPS_ENABLED
        app_fp_disconnect_rfcomm();
#endif
        }
    }
#endif

    switch(state->a2dp_state)
    {
        case IBRT_CONN_A2DP_IDLE:
            break;
        case IBRT_CONN_A2DP_CLOSED:
            break;
        case IBRT_CONN_A2DP_OPEN:
            break;
        case IBRT_CONN_A2DP_CODEC_CONFIGURED:
            TRACE(1, "custom_ui delay report support %d", state->delay_report_support);
            break;
        case IBRT_CONN_A2DP_STREAMING:
            app_ibrt_customif_update_steal_device(set_disc_dev_on_streaming_changed,addr);

            app_ibrt_if_exit_pairing_mode();
            //just add Qos setting interface for music state, if need, uncomment it.
            //app_ibrt_if_update_mobile_link_qos(state->device_id, 40);
            break;
        default:
            break;
    }
}

void app_ibrt_customif_hfp_callback(const bt_bdaddr_t* addr, ibrt_conn_hfp_state_change *state)
{
    TRACE(1, "(d%x) custom_ui hfp_status changed = %d", state->device_id, state->hfp_state);
    DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);

    switch(state->hfp_state)
    {
        case IBRT_CONN_HFP_SLC_DISCONNECTED:
#ifdef __IAG_BLE_INCLUDE__
            if (app_bt_is_a2dp_disconnected(state->device_id) && app_bt_is_hfp_disconnected(state->device_id) &&
                app_bt_is_profile_connected_before(state->device_id)){
    
                if(btif_is_gatt_over_br_edr_enabled() &&
                    btif_btgatt_is_connected(state->device_id))
                {
                    btif_btgatt_disconnect(state->device_id);
                }
#ifdef __AI_VOICE__
            ai_spp_enumerate_disconnect_service((uint8_t*)addr,state->device_id);
#endif
#ifdef GFPS_ENABLED
            app_fp_disconnect_rfcomm();
#endif
            }
#endif
            break;
        case IBRT_CONN_HFP_SLC_OPEN:
            break;
        case IBRT_CONN_HFP_SCO_OPEN:
            app_ibrt_customif_update_steal_device(set_disc_dev_on_streaming_changed,addr);
            app_ibrt_if_exit_pairing_mode();
            break;
        case IBRT_CONN_HFP_SCO_CLOSED:
            break;

        case IBRT_CONN_HFP_RING_IND:
            break;

        case IBRT_CONN_HFP_CALL_IND:
            break;

        case IBRT_CONN_HFP_CALLSETUP_IND:
            break;

        case IBRT_CONN_HFP_CALLHELD_IND:
            break;

        case IBRT_CONN_HFP_CIEV_SERVICE_IND:
            break;

        case IBRT_CONN_HFP_CIEV_SIGNAL_IND:
            break;

        case IBRT_CONN_HFP_CIEV_ROAM_IND:
            break;

        case IBRT_CONN_HFP_CIEV_BATTCHG_IND:
            break;

        case IBRT_CONN_HFP_SPK_VOLUME_IND:
            break;

        case IBRT_CONN_HFP_MIC_VOLUME_IND:
            break;

        case IBRT_CONN_HFP_IN_BAND_RING_IND:
            TRACE(2, "%s is in-band ring %d",__func__, state->in_band_ring_enable);
            app_ibrt_if_BSIR_command_event(state->in_band_ring_enable);
            break;

        case IBRT_CONN_HFP_VR_STATE_IND:
            break;

        case IBRT_CONN_HFP_AT_CMD_COMPLETE:
            break;

        default:
            break;
    }
}

void app_ibrt_customif_avrcp_callback(const bt_bdaddr_t* addr, ibrt_conn_avrcp_state_change *state)
{
    TRACE(2,"(d%x) custom_ui avrcp_status changed = %d", state->device_id, state->avrcp_state);
    DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);

    switch(state->avrcp_state)
    {
        case IBRT_CONN_AVRCP_DISCONNECTED:
        #if defined(__XSPACE_UI__)
            xspace_ui_mobile_connect_status_change_handle(state->device_id, false, 0);
        #endif 
            break;
        case IBRT_CONN_AVRCP_CONNECTED:
        #if defined(__XSPACE_UI__)
			xspace_ui_mobile_connect_status_change_handle(state->device_id, true, 0);
		#endif
            break;
        case IBRT_CONN_AVRCP_VOLUME_UPDATED:
            TRACE(2, "custom_ui volume %d %d%%", state->volume, state->volume*100/128);
            break;
        case IBRT_CONN_AVRCP_REMOTE_CT_0104:
            TRACE(2, "custom_ui remote ct 1.4 support %d volume %d", state->support, state->volume);
            break;
        case IBRT_CONN_AVRCP_REMOTE_SUPPORT_PLAYBACK_STATUS_CHANGED_EVENT:
            TRACE(1, "custom_ui playback status support %d", state->support);
            break;
        case IBRT_CONN_AVRCP_PLAYBACK_STATUS_CHANGED:
            TRACE(1, "custom_ui playback status %d", state->playback_status);
            break;
        default:
            break;
    }
}

void app_ibrt_customif_tws_on_paring_state_changed(ibrt_conn_pairing_state state,uint8_t reason_code)
{
    TWS_UI_ROLE_E ui_role = app_ibrt_conn_get_ui_role();

    TRACE(2,"custom_ui tws pairing_state changed = %d with reason 0x%x,role=%d",state,reason_code, ui_role);

    switch(state)
    {
        case IBRT_CONN_PAIRING_IDLE:
            break;
        case IBRT_CONN_PAIRING_IN_PROGRESS:
            break;
        case IBRT_CONN_PAIRING_COMPLETE:
#ifdef MEDIA_PLAYER_SUPPORT
            if (app_ibrt_if_is_ui_slave() && (btif_besaud_is_connected()))
            {
                app_voice_report(APP_STATUS_INDICATION_PAIRSUCCEED, 0);
            }
#endif
            break;
        case IBRT_CONN_PAIRING_TIMEOUT:
            break;
        default:
            break;
    }
}

void app_ibrt_customif_tws_on_acl_state_changed(ibrt_conn_tws_conn_state_event *state, uint8_t reason_code)
{
    TRACE(3,"custom_ui tws acl state changed = %d with reason 0x%x role %d", state->state.acl_state, reason_code, state->current_role);

    switch (state->state.acl_state)
    {
        case IBRT_CONN_ACL_CONNECTED:
            break;
        case IBRT_CONN_ACL_DISCONNECTED:
            break;
        case IBRT_CONN_ACL_CONNECTING_CANCELED:
            break;
        case IBRT_CONN_ACL_CONNECTING_FAILURE:
            break;
        case IBRT_CONN_ACL_PROFILES_CONNECTED:
        //Note(Mike.Cheng):sync anc status when acl profiles connected
#if defined(IBRT) && defined(ANC_APP)
            //xspace_ui_let_peer_run_func(1000, XUI_APP_ANC_STATUS_SYNC, 0, 0, 0);
            //xspace_interface_delay_timer_stop((uint32_t)app_anc_sync_status);
            //xspace_interface_delay_timer_start(1000, (uint32_t)app_anc_sync_status, 0, 0, 0);
#endif
            break;
        default:
            break;
    }
}

void app_ibrt_customif_on_mobile_acl_state_changed(const bt_bdaddr_t *addr, ibrt_mobile_conn_state_event *state, uint8_t reason_code)
{
    TRACE(3, "(d%x) custom_ui mobile acl state changed = %d reason 0x%x", state->device_id, state->state.acl_state, reason_code);
    DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);

    switch (state->state.acl_state)
    {
        case IBRT_CONN_ACL_DISCONNECTED:            
#ifdef GFPS_ENABLED
            if (app_gfps_is_last_response_pending())
            {
                app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
            }
#endif      
            app_ibrt_customif_update_steal_device(set_disc_dev_on_status_changed, addr);
            break;
        case IBRT_CONN_ACL_CONNECTING:
            break;
        case IBRT_CONN_ACL_CONNECTING_CANCELED:
            break;
        case IBRT_CONN_ACL_CONNECTING_FAILURE:
            break;
        case IBRT_CONN_ACL_CONNECTED:
            if(app_dip_function_enable() == true){
                
                ibrt_mobile_info_t *p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(addr);
                rem_ver_t * rem_ver = (rem_ver_t *)btif_me_get_remote_device_version(p_mobile_info->p_mobile_remote_dev);
            
                if ((rem_ver->compid == 0xa) && (rem_ver->subvers == 0x2918)) //Sony S313
                    TRACE(1, "<Sony S313>: don't send dip request to avoid profile connection failure");
                else
                    app_dip_get_remote_info(state->device_id);
            }
            app_ibrt_customif_update_steal_device(set_disc_dev_on_status_changed, addr);
            break;
        case IBRT_CONN_ACL_PROFILES_CONNECTED:
            break;
        case IBRT_CONN_ACL_AUTH_COMPLETE:
            break;
        case IBRT_CONN_ACL_DISCONNECTING:
            break;
        case IBRT_CONN_ACL_UNKNOWN:
            break;
        default:
            break;
    }
}

void app_ibrt_customif_sco_state_changed(const bt_bdaddr_t *addr, ibrt_sco_conn_state_event *state, uint8_t reason_code)
{
    TRACE(3,"custom_ui sco state changed = %d with reason 0x%x", state->state.sco_state, reason_code);

    switch (state->state.sco_state)
    {
        case IBRT_CONN_SCO_CONNECTED:
            break;
        case IBRT_CONN_SCO_DISCONNECTED:
            app_ibrt_if_sco_disconnect((uint8_t *)addr, reason_code);
            break;
        default:
            break;
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_customif_on_tws_role_changed
 Description  : role state changed callabck
 Input        : bt_bdaddr_t *addr
                ibrt_conn_role_change_state state
                ibrt_role_e role
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/02
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_customif_on_tws_role_switch_status_ind(const bt_bdaddr_t *addr,ibrt_conn_role_change_state state,ibrt_role_e role)
{
    TRACE(2,"custom_ui tws role switch changed = %d with role 0x%x",state,role);
    if(addr != NULL)
    {
        DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);
    }

    switch (state)
    {
        case IBRT_CONN_ROLE_SWAP_INITIATED: //this msg will be notified if initiate role switch
            break;
        case IBRT_CONN_ROLE_SWAP_COMPLETE: //role switch complete,both sides will receice this msg
            app_ibrt_middleware_role_switch_complete_handler(role);
            break;
        case IBRT_CONN_ROLE_CHANGE_COMPLETE://nv role changed
            break;
        default:
            break;
    }
}

void app_ibrt_customif_tws_ui_role_updated(uint8_t newRole)
{
    app_ibrt_middleware_ui_role_updated_handler(newRole);

    // to add custom implementation here
}

void app_ibrt_customif_on_ibrt_state_changed(const bt_bdaddr_t *addr, ibrt_connection_state_event* state,ibrt_role_e role,uint8_t reason_code)
{
    TRACE(3,"(d%x) custom_ui ibrt state changed = %d with reason 0x%x", state->device_id, state->state.ibrt_state, reason_code);
    DUMP8("%02x ",addr->address, BT_ADDR_OUTPUT_PRINT_NUM);

    switch(state->state.ibrt_state)
    {
        case IBRT_CONN_IBRT_DISCONNECTED:
        {
#if 0
            if(app_ibrt_conn_get_snoop_connected_link_num() == 0)
            {
                uint8_t link_id = btif_me_get_device_id_from_addr((bt_bdaddr_t *)addr);
                ibrt_mgr_update_ui_role_from_box_state(ibrt_mgr_get_local_box_state(link_id), link_id);
            }
#endif
        }
            break;
        case IBRT_CONN_IBRT_CONNECTED:
            if(IBRT_MASTER == role)
            {
                app_ibrt_customif_update_steal_device(set_disc_dev_on_status_changed,addr);
            }
            else
            {
                if(app_dip_function_enable() == true){
                    btif_remote_device_t *rdev = btif_me_get_remote_device_by_bdaddr((bt_bdaddr_t *)addr);
                    btif_me_set_remote_dip_queried(rdev, false);
                }
            }
            break;
        case IBRT_CONN_IBRT_START_FAIL:
            break;
        default:
            break;
    }
}

void app_ibrt_customif_on_access_mode_changed(btif_accessible_mode_t newAccessibleMode)
{
    TRACE(0, "Access mode changed to %d", newAccessibleMode);
}

/*****************************************************************************
 Prototype    : app_ibrt_customif_case_open_run_complete_callback
 Description  : called after case open event run complete
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/02
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_customif_case_open_run_complete_callback()
{
    TRACE(0,"custom_ui:case open run complete");
#ifdef __IAG_BLE_INCLUDE__
    app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, true);
#endif
}

/*****************************************************************************
 Prototype    : app_ibrt_customif_case_close_run_complete_callback
 Description  : called after case close event run complete
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/02
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_customif_case_close_run_complete_callback()
{
    TRACE(0,"custom_ui:case close run complete");
#ifdef __IAG_BLE_INCLUDE__
    app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, false);
#endif
    app_ibrt_if_case_is_closed_complete();
}

/*
* custom tws switch interface
* tws switch cmd send sucess, return true, else return false
*/
void app_ibrt_customif_ui_tws_switch(void)
{
    app_ibrt_if_tws_role_switch_request();
}

/*
* custom tws switching check interface
* whether doing tws switch now, return true, else return false
*/
bool app_ibrt_customif_ui_is_tws_switching(void)
{
    return app_ibrt_if_is_tws_role_switch_on();
}

/*
* custom reconfig bd_addr
*/
void app_ibrt_customif_ui_reconfig_bd_addr(bt_bdaddr_t local_addr, bt_bdaddr_t peer_addr, ibrt_role_e nv_role)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    p_ibrt_ctrl->local_addr = local_addr;
    p_ibrt_ctrl->peer_addr  = peer_addr;
    p_ibrt_ctrl->nv_role    = nv_role;

    if (!p_ibrt_ctrl->is_ibrt_search_ui)
    {
        if (IBRT_MASTER == p_ibrt_ctrl->nv_role)
        {
            p_ibrt_ctrl->peer_addr = local_addr;
            btif_me_set_bt_address(p_ibrt_ctrl->local_addr.address);
        }
        else if (IBRT_SLAVE == p_ibrt_ctrl->nv_role)
        {
            p_ibrt_ctrl->local_addr = peer_addr;
            btif_me_set_bt_address(p_ibrt_ctrl->local_addr.address);
        }
        else
        {
            ASSERT(0, "%s nv_role error", __func__);
        }
    }
}

/*custom can block connect mobile if needed*/
bool app_ibrt_customif_connect_mobile_needed_ind(void)
{
    return true;
}

void app_ibrt_customif_get_tws_side_handler(APP_TWS_SIDE_T* twsSide)
{
    // TODO: update the tws side
}

uint32_t app_ibrt_customif_set_profile_delaytime_on_spp_connect(rfcomm_uuid_t uuid)
{
    // While spp connect and no basic profile established, this func will be called.
    // Add custom implementation here, if you need to make peer buds connects to the spp as soon as possible,
    // just return a none-zero value. For example, "return 1" will be the fastest.
    // return t : wait t ms to start profile exchange
    // return 0 : will not change the process of "profile exchange"
    TRACE(1, "spp_uuid len=%d bytes, uuid:", uuid.len);
    DUMP8("0x%x ", uuid.pdata, uuid.len);

    //    return 1;
    //BES LN : 4000 means 4s
    return 4000;
}

bool app_ibrt_customif_keep_pairing_after_tws_disconnected_needed_ind()
{
    return true;
}

bool app_ibrt_customif_tws_role_switch_allowed()
{
    return true;
}

#ifdef IBRT_UI_V2
int app_ibrt_customif_ui_start(void)
{
    ibrt_mgr_config_t config;

    // zero init the config
    memset(&config, 0, sizeof(ibrt_mgr_config_t));

    // support max remote link count
    config.support_max_remote_link                  = 2;

    //just for lenovo UI, not role siwtch when wear up/down, default should be false
    config.ibrt_role_switch_control_by_customer     = false;

    //freeman mode config, default should be false
    config.freeman_enable                           = false;

    //tws earphone set the same addr, UI will be flexible, default should be true
    config.tws_use_same_addr                        = true;

    /* following cases the reconnect will be fail for freeman, please set to true if you want to reconnect successful:
    * 1. freeman has link key but mobile deleted the link key
    * 2. freeman changed its bt address after reboot and use the new address to reconnect mobile
    */
    config.freeman_accept_mobile_new_pairing        = false;

    //for some proj no box key, default should be false;
    config.enter_pairing_on_empty_mobile_addr       = true;

    //for some proj no box key, default should be false
    config.enter_pairing_on_reconnect_mobile_failed = true;

    //sync pairing mode on tws connected
    config.sync_pairing_mode_on_tws_connect         = true;

    //for some proj no box key, default should be false
    config.enter_pairing_on_mobile_disconnect       = false;

    //do tws switch when RSII value change, default should be true
    config.tws_switch_according_to_rssi_value       = false;

    //do tws switch when RSII value change, timer threshold
    config.role_switch_timer_threshold                    = IBRT_UI_ROLE_SWITCH_TIME_THRESHOLD;

    //do tws switch when rssi value change over threshold
    config.rssi_threshold                                 = IBRT_UI_ROLE_SWITCH_THRESHOLD_WITH_RSSI;

    //close box debounce time config
    config.close_box_event_wait_response_timeout          = IBRT_UI_CLOSE_BOX_EVENT_WAIT_RESPONSE_TIMEOUT;

    //wait time before launch reconnect event
    config.reconnect_mobile_wait_response_timeout         = IBRT_UI_RECONNECT_MOBILE_WAIT_RESPONSE_TIMEOUT;

    //reconnect event internal config wait timer when tws disconnect
    config.reconnect_wait_ready_timeout                   = IBRT_UI_MOBILE_RECONNECT_WAIT_READY_TIMEOUT;//inquiry scan enable timeout when enter paring

    config.tws_conn_failed_wait_time                      = TWS_CONN_FAILED_WAIT_TIME;
    config.wait_time_before_disc_tws                      = 3000;

    config.reconnect_mobile_wait_ready_timeout            = IBRT_UI_MOBILE_RECONNECT_WAIT_READY_TIMEOUT;
    config.reconnect_tws_wait_ready_timeout               = IBRT_UI_TWS_RECONNECT_WAIT_READY_TIMEOUT;
    config.reconnect_ibrt_wait_response_timeout           = IBRT_UI_RECONNECT_IBRT_WAIT_RESPONSE_TIMEOUT;
    config.nv_master_reconnect_tws_wait_response_timeout  = IBRT_UI_NV_MASTER_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT;
    config.nv_slave_reconnect_tws_wait_response_timeout   = IBRT_UI_NV_SLAVE_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT;

    config.check_plugin_excute_closedbox_event            = true;
    config.ibrt_with_ai                                   = false;
    config.suppport_io_capability                         = false;
    config.no_profile_stop_ibrt                           = true;

    config.paring_mode_support                            = true;

    config.single_earbud_entry_pairing_mode               = true;

    //pairing mode timeout config
    config.disable_bt_scan_timeout                        = IBRT_UI_DISABLE_BT_SCAN_TIMEOUT;

    //open box reconnect mobile times config
    config.open_reconnect_mobile_max_times                = IBRT_UI_OPEN_RECONNECT_MOBILE_MAX_TIMES;

    //open box reconnect tws times config
    config.open_reconnect_tws_max_times                   = IBRT_UI_OPEN_RECONNECT_TWS_MAX_TIMES;

    //connection timeout reconnect mobile times config
    config.reconnect_mobile_max_times                     = IBRT_UI_RECONNECT_MOBILE_MAX_TIMES;

    //connection timeout reconnect tws times config
    config.reconnect_tws_max_times                        = IBRT_UI_RECONNECT_TWS_MAX_TIMES;

    //connection timeout reconnect ibrt times config
    config.reconnect_ibrt_max_times                       = IBRT_UI_RECONNECT_IBRT_MAX_TIMES;

    //controller basband monitor
    config.lowlayer_monitor_enable                        = true;
    config.llmonitor_report_format                  = REP_FORMAT_PACKET;
    config.llmonitor_report_count                 = 1000;
  //  config.llmonitor_report_format                  = REP_FORMAT_TIME;
   // config.llmonitor_report_count                 = 1600;////625us uint

    config.mobile_page_timeout                            = IBRT_MOBILE_PAGE_TIMEOUT;
    //tws connection supervision timeout
    config.tws_connection_timeout                         = IBRT_UI_TWS_CONNECTION_TIMEOUT;

    config.rx_seq_error_timeout                           = IBRT_UI_RX_SEQ_ERROR_TIMEOUT;
    config.rx_seq_error_threshold                         = IBRT_UI_RX_SEQ_ERROR_THRESHOLD;
    config.rx_seq_recover_wait_timeout                    = IBRT_UI_RX_SEQ_ERROR_RECOVER_TIMEOUT;

    config.rssi_monitor_timeout                           = IBRT_UI_RSSI_MONITOR_TIMEOUT;

    config.wear_updown_detect_supported                   = false;

    config.radical_scan_interval_nv_slave                 = IBRT_UI_RADICAL_SAN_INTERVAL_NV_SLAVE;
    config.radical_scan_interval_nv_master                = IBRT_UI_RADICAL_SAN_INTERVAL_NV_MASTER;

    config.scan_interval_in_sco_tws_disconnected          = IBRT_UI_SCAN_INTERVAL_IN_SCO_TWS_DISCONNECTED;
    config.scan_window_in_sco_tws_disconnected            = IBRT_UI_SCAN_WINDOW_IN_SCO_TWS_DISCONNECTED;

    config.scan_interval_in_sco_tws_connected             = IBRT_UI_SCAN_INTERVAL_IN_SCO_TWS_CONNECTED;
    config.scan_window_in_sco_tws_connected               = IBRT_UI_SCAN_WINDOW_IN_SCO_TWS_CONNECTED;

    config.scan_interval_in_a2dp_tws_disconnected         = IBRT_UI_SCAN_INTERVAL_IN_A2DP_TWS_DISCONNECTED;
    config.scan_window_in_a2dp_tws_disconnected           = IBRT_UI_SCAN_WINDOW_IN_A2DP_TWS_DISCONNECTED;

    config.scan_interval_in_a2dp_tws_connected            = IBRT_UI_SCAN_INTERVAL_IN_A2DP_TWS_CONNECTED;
    config.scan_window_in_a2dp_tws_connected              = IBRT_UI_SCAN_WINDOW_IN_A2DP_TWS_CONNECTED;

    config.connect_no_03_timeout                          = CONNECT_NO_03_TIMEOUT;
    config.disconnect_no_05_timeout                       = DISCONNECT_NO_05_TIMEOUT;

    config.tws_switch_tx_data_protect                     = true;

    config.tws_cmd_send_timeout                           = IBRT_UI_TWS_CMD_SEND_TIMEOUT;
    config.tws_cmd_send_counter_threshold                 = IBRT_UI_TWS_COUNTER_THRESHOLD;

    config.profile_concurrency_supported                  = false;

    config.audio_sync_mismatch_resume_version             = 2;

    config.support_steal_connection                       = true;
    config.support_steal_connection_in_sco                = false;

    config.allow_sniff_in_sco                             = false;

    config.always_interlaced_scan                         = true;

    config.paring_with_disc_remote_dev                    = false;

    config.disallow_reconnect_in_streaming_state          = false;

	//if local open box, there is no reason and force earbuds enter pairing mode
    config.open_reconnect_fail_force_enter_pairing          = true;
    // disconnect sco or not when accepting phone connection in pairing mode
    config.disc_sco_during_pairing                        = false;

    config.without_reconnect_when_fetch_out_wear_up       = false;

#ifdef IBRT_UI_MASTER_ON_TWS_DISCONNECTED
    config.is_changed_to_ui_master_on_tws_disconnected    = true;
#else
    config.is_changed_to_ui_master_on_tws_disconnected    = false;
#endif
    config.allow_mobile_con_req_with_tws_connecting       = false;

    config.both_side_start_page = true;

    config.dock_with_stop_ibrt = false;

    config.one_earbud_in_case_also_need_entry_pairing_mode = true;
 
    app_ibrt_if_register_custom_ui_callback(&custom_ui_status_changed_cb);
    app_ibrt_if_register_pairing_mode_callback(&custom_ui_pairing_mode_cb);
    app_ibrt_if_register_case_event_complete_callback(&case_event_complete_cb);
    app_ibrt_if_register_vender_handler_ind(app_ibrt_customif_ui_vender_event_handler_ind);

    app_ibrt_if_config(&config);

#ifdef BT_ALWAYS_IN_DISCOVERABLE_MODE
    btif_me_configure_keeping_both_scan(true);
#endif

    app_spp_is_connected_register(app_ibrt_customif_set_profile_delaytime_on_spp_connect);

    return 0;
}
#endif
