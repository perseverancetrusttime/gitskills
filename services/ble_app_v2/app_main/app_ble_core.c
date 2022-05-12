/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "string.h"
#include "co_math.h" // Common Maths Definition
#include "cmsis_os.h"
#include "ble_app_dbg.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "app_a2dp.h"
#include "bluetooth.h"
#include "apps.h"
#include "gapc_msg.h"
#include "gapm_msg.h"
#include "app.h"
#include "app_sec.h"
#include "app_ble_mode_switch.h"
#include "app_ble_core.h"
#include "nvrecord.h"
#include "nvrecord_ble.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#include "nvrecord_ble.h"
#include "app_sec.h"
#if defined(IBRT)
#if defined(IBRT_UI_V1)
#include "app_ibrt_ui.h"
#endif
#include "app_ibrt_if.h"
#endif
#ifdef AOB_UX_ENABLED
#include "aob_stm_generic.h"
#include "aob_tws_conn_mgr_stm.h"
#include "aob_mobile_conn_mgr_stm.h"
#endif

#ifdef TWS_SYSTEM_ENABLED
bool is_update_ble_info_to_peer();
void set_update_ble_info_to_peer(bool is_update);
#endif

extern uint8_t is_sco_mode (void);
extern bool app_is_ux_mobile(void);

static APP_BLE_CORE_GLOBAL_HANDLER_FUNC g_ble_core_global_handler_ind = NULL;
static APP_BLE_CORE_GLOBAL_CALLBACK_HANDLER_FUNC g_ble_core_global_callback_handler_ind = NULL;

void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler)
{
    g_ble_core_global_handler_ind = handler;
}

void app_ble_core_register_global_callback_handle_ind(APP_BLE_CORE_GLOBAL_CALLBACK_HANDLER_FUNC handler)
{
    g_ble_core_global_callback_handler_ind = handler;
}

extern bool nv_record_blerec_get_paired_dev_from_addr(BleDeviceinfo* record, uint8_t *pBdAddr);
static void ble_connect_event_handler(ble_event_t *event, void *output)
{
#ifdef AOB_UX_ENABLED
    uint8_t *peer_bdaddr = (uint8_t *)event->p.connect_handled.peer_bdaddr;
    BleDeviceinfo record;

    if (!app_is_ux_mobile())
    {
        if (aob_tws_check_is_peer_addr(peer_bdaddr))
        {
            aob_tws_connection_handler(event->p.connect_handled.conidx, peer_bdaddr);
#ifdef TWS_SYSTEM_ENABLED
            app_ibrt_middleware_ble_connected_handler();
#endif
        }
        else
        {
            memset(&record, 0, sizeof(BleDeviceinfo));
            if (!nv_record_blerec_get_paired_dev_from_addr(&record, peer_bdaddr))
            {
                memcpy(record.peer_bleAddr, peer_bdaddr, BTIF_BD_ADDR_SIZE);
            }

            nv_record_blerec_add(&record);
            aob_mobile_connection_handler(event->p.connect_handled.conidx, peer_bdaddr);
        }

#if defined(IBRT) && defined(IBRT_UI_V1)
        bd_addr_t *box_ble_addr = (bd_addr_t *)app_ibrt_ui_get_box_ble_addr();
        if (app_ibrt_ui_get_snoop_via_ble_enable())
        {
            if(!memcmp(box_ble_addr, event->p.connect_handled.peer_bdaddr, BTIF_BD_ADDR_SIZE))
            {
                app_ibrt_ui_set_ble_connect_index(event->p.connect_handled.conidx);
                app_ibrt_ui_set_box_connect_state(IBRT_BOX_CONNECT_MASTER, FALSE);
            }
        }
#endif
    }
#endif    
}

static void ble_connect_bond_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection bond complete
}

static void ble_stopped_connecting_event_handler(ble_event_t *event, void *output)
{
#ifdef AOB_UX_ENABLED
    uint8_t *peer_bdaddr = (uint8_t *)event->p.stopped_connecting_handled.peer_bdaddr;

    if (aob_tws_check_is_peer_addr(peer_bdaddr))
    {
        aob_tws_cancel_connection_handler();
    }
    else
    {
        aob_mobile_canceled_connection_handler(peer_bdaddr);
    }
#endif
}

static void ble_connecting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble start connecting failed. Mostly it's param invalid.
}

static void ble_disconnect_event_handler(ble_event_t *event, void *output)
{
    TRACE(0, "%s errCode 0x%x", __func__, event->p.disconnect_handled.errCode);

#ifdef AOB_UX_ENABLED
    uint8_t conidx = event->p.disconnect_handled.conidx;
    if (!app_is_ux_mobile())
    {
        if (aob_tws_check_is_peer_conidx(conidx))
        {
            aob_tws_disconnection_handler(event->p.disconnect_handled.errCode);
        }
        else
        {
            aob_mobile_disconnection_handler(event->p.disconnect_handled.errCode, conidx);
        }

#if defined(IBRT) && defined(IBRT_UI_V1)
        if (app_ibrt_ui_get_snoop_via_ble_enable())
        {
            app_ibrt_ui_set_master_notify_flag(false);
            app_ibrt_ui_clear_box_connect_state(IBRT_BOX_CONNECT_MASTER, FALSE);
        }
#endif
    }
#endif
}

static void ble_conn_param_update_req_event_handler(ble_event_t *event, void *output)
{
    if (a2dp_is_music_ongoing() || is_sco_mode())
    {
        *(bool *)output = false;
    }
}

static void ble_conn_param_update_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update failed
}

static void ble_conn_param_update_successful_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update successful
}

static void ble_set_random_bd_addr_event_handler(ble_event_t *event, void *output)
{
    // Indicate that a new random BD address set in lower layers
    app_ble_refresh_adv_state(160);
}

static void ble_adv_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been started success
}

static void ble_adv_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv starting failed
}

static void ble_adv_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been stopped success
}



static void ble_scan_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been started success
}

static void ble_scan_data_report_event_handler(ble_event_t *event, void *output)
{
    // Scan data report
}

static void ble_scan_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan starting failed
}

static void ble_scan_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static const ble_event_handler_t ble_event_handler_tab[] =
{
    {BLE_CONNECT_EVENT, ble_connect_event_handler},
    {BLE_CONNECT_BOND_EVENT, ble_connect_bond_event_handler},
    {BLE_CONNECTING_STOPPED_EVENT, ble_stopped_connecting_event_handler},
    {BLE_CONNECTING_FAILED_EVENT, ble_connecting_failed_event_handler},
    {BLE_DISCONNECT_EVENT, ble_disconnect_event_handler},
    {BLE_CONN_PARAM_UPDATE_REQ_EVENT, ble_conn_param_update_req_event_handler},
    {BLE_CONN_PARAM_UPDATE_FAILED_EVENT, ble_conn_param_update_failed_event_handler},
    {BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT, ble_conn_param_update_successful_event_handler},
    {BLE_SET_RANDOM_BD_ADDR_EVENT, ble_set_random_bd_addr_event_handler},
    {BLE_ADV_STARTED_EVENT, ble_adv_started_event_handler},
    {BLE_ADV_STARTING_FAILED_EVENT, ble_adv_starting_failed_event_handler},
    {BLE_ADV_STOPPED_EVENT, ble_adv_stopped_event_handler},
    {BLE_SCAN_STARTED_EVENT, ble_scan_started_event_handler},
    {BLE_SCAN_DATA_REPORT_EVENT, ble_scan_data_report_event_handler},
    {BLE_SCAN_STARTING_FAILED_EVENT, ble_scan_starting_failed_event_handler},
    {BLE_SCAN_STOPPED_EVENT, ble_scan_stopped_event_handler},
};

//handle the event that from ble lower layers
void app_ble_core_global_handle(ble_event_t *event, void *output)
{
    uint8_t evt_type = event->evt_type;
    uint16_t index = 0;
    const ble_event_handler_t *p_ble_event_hand = NULL;

    for (index=0; index<BLE_EVENT_NUM_MAX; index++)
    {
        p_ble_event_hand = &ble_event_handler_tab[index];
        if (p_ble_event_hand->evt_type == evt_type)
        {
            p_ble_event_hand->func(event, output);
            break;
        }
    }

    if (g_ble_core_global_handler_ind)
    {
        g_ble_core_global_handler_ind(event, output);
    }
}


static void ble_callback_roleswitch_start_handler(ble_callback_event_t *event, void *output)
{
    LOG_I("%s", __func__);
    // disable adv after role switch start
    app_ble_force_switch_adv(BLE_SWITCH_USER_RS, false);
}

static void ble_callback_roleswitch_complete_handler(ble_callback_event_t *event, void *output)
{
#if defined(IBRT)
    // enable adv after role switch complete
    uint8_t newRole = event->p.rs_complete_handled.newRole;
    LOG_I("%s newRole %d", __func__, newRole);
    app_ble_force_switch_adv(BLE_SWITCH_USER_RS, true);
    if (newRole == IBRT_SLAVE)
    {
        app_ble_disconnect_all();
    }
#endif
}

static void ble_callback_role_update_handler(ble_callback_event_t *event, void *output)
{
#if defined(IBRT)
    uint8_t newRole = event->p.role_update_handled.newRole;
    LOG_I("%s newRole %d", __func__, newRole);
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    if (newRole == IBRT_SLAVE)
    {
        app_ble_disconnect_all();
    }
#endif
}

static void ble_callback_ibrt_event_entry_handler(ble_callback_event_t *event, void *output)
{
#if defined(IBRT)&&defined(IBRT_UI_V1)
    uint8_t ibrt_evt_type = event->p.ibrt_event_entry_handled.event;
    LOG_I("%s evt_type %d", __func__, ibrt_evt_type);
    if (IBRT_OPEN_BOX_EVENT == ibrt_evt_type)
    {
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, true);
    }
    else if (IBRT_FETCH_OUT_EVENT == ibrt_evt_type)
    {
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, true);
    }
    else if (IBRT_CLOSE_BOX_EVENT == ibrt_evt_type)
    {
        app_ble_force_switch_adv(BLE_SWITCH_USER_BOX, false);
    }
#endif
}

static const ble_callback_event_handler_t ble_callback_event_handler_tab[] =
{
    {BLE_CALLBACK_RS_START, ble_callback_roleswitch_start_handler},
    {BLE_CALLBACK_RS_COMPLETE, ble_callback_roleswitch_complete_handler},
    {BLE_CALLBACK_ROLE_UPDATE, ble_callback_role_update_handler},
    {BLE_CALLBACK_IBRT_EVENT_ENTRY, ble_callback_ibrt_event_entry_handler},
};

//handle the event that from other module
void app_ble_core_global_callback_event(ble_callback_event_t *event, void *output)
{
   uint8_t evt_type = event->evt_type;
   uint16_t index = 0;
   const ble_callback_event_handler_t *p_ble_callback_event_hand = NULL;
   
   for (index=0; index<BLE_EVENT_NUM_MAX; index++)
   {
       p_ble_callback_event_hand = &ble_callback_event_handler_tab[index];
       if (p_ble_callback_event_hand->evt_type == evt_type)
       {
           p_ble_callback_event_hand->func(event, output);
           break;
       }
   }

    if (g_ble_core_global_callback_handler_ind)
    {
        g_ble_core_global_callback_handler_ind(event, output);
    }
}

static void app_ble_stub_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    bool adv_enable = false;

    do {
#if defined(IBRT)
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

        if (!p_ibrt_ctrl->init_done)
        {
            LOG_I("%s ibrt don't init", __func__);
            break;
        }
        else if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
        {
            // LOG_I("%s role %d isn't MASTER", __func__, p_ibrt_ctrl->current_role);
            break;
        }
#endif

        // ctkd needs ble adv no matter whether a mobile bt link has been established or not
#ifdef CTKD_ENABLE
        adv_enable = true;
#else
        if (app_ble_get_user_register() & ~(1 << USER_STUB))
        {
            LOG_I("%s have other user register 0x%x", __func__, app_ble_get_user_register());
        }
        else
        {
            adv_enable = true;
        }
#endif 
    } while(0);

    app_ble_data_fill_enable(USER_STUB, adv_enable);
}

void app_ble_stub_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_STUB, (BLE_DATA_FILL_FUNC_T)app_ble_stub_user_data_fill_handler, false);
}

#ifdef TWS_SYSTEM_ENABLED
static void ble_sync_info_prepare_handler(uint8_t *buf, uint16_t *length)
{
    *length = sizeof(NV_RECORD_PAIRED_BLE_DEV_INFO_T);

    NV_RECORD_PAIRED_BLE_DEV_INFO_T *pBleInfo = nv_record_blerec_get_ptr();

    memcpy(buf, pBleInfo, *length);
}

static void ble_sync_info_received_handler(uint8_t *buf, uint16_t length)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *pReceivedBleInfo = (NV_RECORD_PAIRED_BLE_DEV_INFO_T *)buf;
    
    if (is_update_ble_info_to_peer())
    {
        set_update_ble_info_to_peer(false);
        return;
    }

    // basic info
    nv_record_extension_update_tws_ble_info(pReceivedBleInfo);

    // pair info
    for (uint32_t index = 0; index < pReceivedBleInfo->saved_list_num; index++)
    {
        nv_record_blerec_add(&pReceivedBleInfo->ble_nv[index]);
    }

#ifdef CFG_APP_SEC
    app_sec_init();
#endif
}

static void ble_sync_info_rsp_received_handler(uint8_t *buf, uint16_t length)
{
    NV_RECORD_PAIRED_BLE_DEV_INFO_T *pReceivedBleInfo = (NV_RECORD_PAIRED_BLE_DEV_INFO_T *)buf;

    nv_record_extension_update_tws_ble_info(pReceivedBleInfo);
    for (uint32_t index = 0; index < pReceivedBleInfo->saved_list_num; index++)
    {
        nv_record_blerec_add(&pReceivedBleInfo->ble_nv[index]);
    }
}

void app_ble_mode_tws_sync_init(void)
{
    TWS_SYNC_USER_T userBle = {
        ble_sync_info_prepare_handler,
        ble_sync_info_received_handler,
        ble_sync_info_prepare_handler,
        ble_sync_info_rsp_received_handler,
        NULL,
    };

    app_ibrt_if_register_sync_user(TWS_SYNC_USER_BLE_INFO, &userBle);
}

void app_ble_sync_ble_info(void)
{
    app_ibrt_if_prepare_sync_info();
    app_ibrt_if_sync_info(TWS_SYNC_USER_BLE_INFO);
    app_ibrt_if_flush_sync_info();
}
#endif

void ble_adv_data_parse(uint8_t *bleBdAddr,
                               int8_t rssi,
                               unsigned char *adv_buf,
                               unsigned char len)
{
#if defined(IBRT) && defined(IBRT_UI_V1)
    bd_addr_t *box_ble_addr = (bd_addr_t *)app_ibrt_ui_get_box_ble_addr();
    LOG_I("%s", __func__);
    //DUMP8("%02x ", (uint8_t *)box_ble_addr, BT_ADDR_OUTPUT_PRINT_NUM);
    DUMP8("%02x ", bleBdAddr, BT_ADDR_OUTPUT_PRINT_NUM);

    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        if (!memcmp(box_ble_addr, bleBdAddr, BTIF_BD_ADDR_SIZE) && app_ibrt_ui_is_slave_scaning())
        {
            app_ibrt_ui_set_slave_scaning(FALSE);
            app_ble_stop_scan();
            app_ble_start_connect((uint8_t *)box_ble_addr);
        }
    }
#endif
}

