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
#include "apps.h"
#include "gapm_msg.h"               // GAP Manager Task API
#include "gapc_msg.h"               // GAP Controller Task API
#include "app.h"
#include "app_sec.h"
#include "app_ble_include.h"
#include "nvrecord.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#include "nvrecord_ble.h"
#include "app_sec.h"
#ifdef IBRT
#if defined(IBRT_UI_V1)
#include "app_ibrt_ui.h"
#endif
#include "app_ibrt_if.h"
#endif

static void app_ble_customif_connect_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_connect_bond_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection bond complete
}

static void app_ble_customif_stopped_connecting_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_connecting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble start connecting failed. Mostly it's param invalid.
}

static void app_ble_customif_disconnect_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_conn_param_update_req_event_handler(ble_event_t *event, void *output)
{
}

static void app_ble_customif_conn_param_update_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update failed
}

static void app_ble_customif_conn_param_update_successful_event_handler(ble_event_t *event, void *output)
{
    // Indicate that ble connection param update successful
}

static void app_ble_customif_set_random_bd_addr_event_handler(ble_event_t *event, void *output)
{
    // Indicate that a new random BD address set in lower layers
}

static void app_ble_customif_adv_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been started success
}

static void app_ble_customif_adv_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv starting failed
}

static void app_ble_customif_adv_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that adv has been stopped success
}

static void app_ble_customif_scan_started_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been started success
}

static void app_ble_customif_scan_data_report_event_handler(ble_event_t *event, void *output)
{
    // Scan data report
}

static void app_ble_customif_scan_starting_failed_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan starting failed
}

static void app_ble_customif_scan_stopped_event_handler(ble_event_t *event, void *output)
{
    // Indicate that scan has been stopped success
}

static const ble_event_handler_t app_ble_customif_event_handler_tab[] =
{
    {BLE_CONNECT_EVENT, app_ble_customif_connect_event_handler},
    {BLE_CONNECT_BOND_EVENT, app_ble_customif_connect_bond_event_handler},
    {BLE_CONNECTING_STOPPED_EVENT, app_ble_customif_stopped_connecting_event_handler},
    {BLE_CONNECTING_FAILED_EVENT, app_ble_customif_connecting_failed_event_handler},
    {BLE_DISCONNECT_EVENT, app_ble_customif_disconnect_event_handler},
    {BLE_CONN_PARAM_UPDATE_REQ_EVENT, app_ble_customif_conn_param_update_req_event_handler},
    {BLE_CONN_PARAM_UPDATE_FAILED_EVENT, app_ble_customif_conn_param_update_failed_event_handler},
    {BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT, app_ble_customif_conn_param_update_successful_event_handler},
    {BLE_SET_RANDOM_BD_ADDR_EVENT, app_ble_customif_set_random_bd_addr_event_handler},
    {BLE_ADV_STARTED_EVENT, app_ble_customif_adv_started_event_handler},
    {BLE_ADV_STARTING_FAILED_EVENT, app_ble_customif_adv_starting_failed_event_handler},
    {BLE_ADV_STOPPED_EVENT, app_ble_customif_adv_stopped_event_handler},
    {BLE_SCAN_STARTED_EVENT, app_ble_customif_scan_started_event_handler},
    {BLE_SCAN_DATA_REPORT_EVENT, app_ble_customif_scan_data_report_event_handler},
    {BLE_SCAN_STARTING_FAILED_EVENT, app_ble_customif_scan_starting_failed_event_handler},
    {BLE_SCAN_STOPPED_EVENT, app_ble_customif_scan_stopped_event_handler},
};

//handle the event that from ble lower layers
void app_ble_customif_global_handler_ind(ble_event_t *event, void *output)
{
    uint8_t evt_type = event->evt_type;
    uint16_t index = 0;
    const ble_event_handler_t *p_ble_event_hand = NULL;

    for (index=0; index<BLE_EVENT_NUM_MAX; index++)
    {
        p_ble_event_hand = &app_ble_customif_event_handler_tab[index];
        if (p_ble_event_hand->evt_type == evt_type)
        {
            p_ble_event_hand->func(event, output);
            break;
        }
    }
}

static void app_ble_customif_callback_roleswitch_start_handler(ble_callback_event_t *event, void *output)
{
}

static void app_ble_customif_callback_roleswitch_complete_handler(ble_callback_event_t *event, void *output)
{
}

static void app_ble_customif_callback_role_update_handler(ble_callback_event_t *event, void *output)
{
}

static void app_ble_customif_callback_ibrt_event_entry_handler(ble_callback_event_t *event, void *output)
{
}

static const ble_callback_event_handler_t app_ble_customif_callback_event_handler_tab[] =
{
    {BLE_CALLBACK_RS_START, app_ble_customif_callback_roleswitch_start_handler},
    {BLE_CALLBACK_RS_COMPLETE, app_ble_customif_callback_roleswitch_complete_handler},
    {BLE_CALLBACK_ROLE_UPDATE, app_ble_customif_callback_role_update_handler},
    {BLE_CALLBACK_IBRT_EVENT_ENTRY, app_ble_customif_callback_ibrt_event_entry_handler},
};

//handle the event that from other module
void app_ble_customif_global_callback_handler_ind(ble_callback_event_t *event, void *output)
{
   uint8_t evt_type = event->evt_type;
   uint16_t index = 0;
   const ble_callback_event_handler_t *p_ble_callback_event_hand = NULL;
   
   for (index=0; index<BLE_EVENT_NUM_MAX; index++)
   {
       p_ble_callback_event_hand = &app_ble_customif_callback_event_handler_tab[index];
       if (p_ble_callback_event_hand->evt_type == evt_type)
       {
           p_ble_callback_event_hand->func(event, output);
           break;
       }
   }
}

void app_ble_customif_init(void)
{
    LOG_I("%s", __func__);
    app_ble_core_register_global_handler_ind(app_ble_customif_global_handler_ind);
    app_ble_core_register_global_callback_handle_ind(app_ble_customif_global_callback_handler_ind);
}

