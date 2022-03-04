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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration
#include "app_ble_cmd_handler.h"

#if (BLE_APP_DATAPATH_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gapc_msg.h" 
#include "app.h"
#include "app_datapath_server.h"                  // Data Path Application Definitions
#include "app_task.h"                // application task definitions
#include "datapathps_task.h"              
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"
#if defined(IBRT) && defined(IBRT_UI_V1)
#include "app_ibrt_ui.h"
#endif
 
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// health thermometer application environment structure
struct app_datapath_server_env_tag app_datapath_server_env =
{
    BLE_INVALID_CONNECTION_INDEX,
    false
};

static app_datapath_server_tx_done_t tx_done_callback = NULL;
static app_datapath_server_data_received_callback_func_t rx_done_callback = NULL;
static app_datapath_server_disconnected_done_t disconnected_done_callback = NULL;
static app_datapath_server_connected_done_t connected_done_callback = NULL;
static app_datapath_server_mtuexchanged_done_t mtuexchanged_done_callback = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_datapath_server_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu)
{
    if (NULL != mtuexchanged_done_callback)
    {
        mtuexchanged_done_callback(conidx, mtu);
    }
}

void app_datapath_server_connected_evt_handler(uint8_t conidx)
{
    TRACE(0,"app datapath server connected.");
    app_datapath_server_env.connectionIndex = conidx;
#if defined(IBRT) && defined(IBRT_UI_V1)
    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        app_ibrt_ui_set_master_notify_flag(TRUE);
    }
#endif
    if (NULL != connected_done_callback)
    {
        connected_done_callback(conidx);
    }
}

void app_datapath_server_disconnected_evt_handler(uint8_t conidx)
{
    if (conidx == app_datapath_server_env.connectionIndex)
    {
        TRACE(0,"app datapath server dis-connected.");
        app_datapath_server_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
        app_datapath_server_env.isNotificationEnabled = false;

        tx_done_callback = NULL;
    }
    if (NULL != disconnected_done_callback)
    {
        disconnected_done_callback(conidx);
    }
}

void app_datapath_server_init(void)
{
    // Reset the environment
    app_datapath_server_env.connectionIndex =  BLE_INVALID_CONNECTION_INDEX;
    app_datapath_server_env.isNotificationEnabled = false;
}

void app_datapath_add_datapathps(void)
{
    TRACE(1, "app_datapath_add_datapathps %d", TASK_ID_DATAPATHPS);
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, 0);
    
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC);
    req->user_prio = 0;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    req->prf_api_id = TASK_ID_DATAPATHPS;

    // Send the message
    ke_msg_send(req);
}

void app_datapath_server_send_data_via_notification(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPS_SEND_DATA_VIA_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPS),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = app_datapath_server_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

void app_datapath_server_send_data_via_indication(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPS_SEND_DATA_VIA_INDICATION,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPS),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = app_datapath_server_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

void app_datapath_server_send_data_via_write_command(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPS_SEND_DATA_VIA_WRITE_COMMAND,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPS),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = app_datapath_server_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);
    ke_msg_send(req);
}

void app_datapath_server_send_data_via_write_request(uint8_t* ptrData, uint32_t length)
{
    struct ble_datapath_send_data_req_t * req = KE_MSG_ALLOC_DYN(DATAPATHPS_SEND_DATA_VIA_WRITE_REQUEST,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPS),
                                                TASK_APP,
                                                ble_datapath_send_data_req_t,
                                                length);
    req->connecionIndex = app_datapath_server_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);
    ke_msg_send(req);
}

void app_datapath_server_control_notification(uint8_t conidx,bool isEnable)
{
    struct ble_datapath_control_notification_t * req = KE_MSG_ALLOC(DATAPATHPS_CONTROL_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_DATAPATHPS),
                                                TASK_APP,
                                                ble_datapath_control_notification_t);
    req->isEnable = isEnable;
    req->connecionIndex = conidx;
    ke_msg_send(req);
}


/**
 ****************************************************************************************
 * @brief Handles health thermometer timer
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_datapath_server_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing

    return (KE_MSG_CONSUMED);
}

static int app_datapath_server_tx_ccc_changed_handler(ke_msg_id_t const msgid,
        struct ble_datapath_tx_notif_config_t *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    app_datapath_server_env.isNotificationEnabled = param->isNotificationEnabled;

    if (app_datapath_server_env.isNotificationEnabled) 
    {
        // the app datapath server is connected when receiving the first enable CCC request
        if (BLE_INVALID_CONNECTION_INDEX == app_datapath_server_env.connectionIndex)
        {
            uint8_t conidx = KE_IDX_GET(src_id);
            app_datapath_server_connected_evt_handler(conidx);
        }
    }

    return (KE_MSG_CONSUMED);
}

static int app_datapath_server_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_datapath_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    if (NULL != tx_done_callback)
    {
        tx_done_callback();
    }

    return (KE_MSG_CONSUMED);
}

static int app_datapath_server_rx_data_received_handler(ke_msg_id_t const msgid,
                              struct ble_datapath_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // loop back the received data
    app_datapath_server_send_data_via_notification(param->data, param->length);

    TRACE(2,"%s length %d", __func__, param->length);
    //DUMP8("%02x ", (param->data+i), len);
    
#ifndef __INTERCONNECTION__
    BLE_custom_command_receive_data(param->data, param->length);
#endif
    
#if defined(IBRT) && defined(IBRT_UI_V1)
    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        app_ibrt_ui_snoop_info_handler(param->data, param->length);
    }
#endif

    if (NULL != rx_done_callback)
    {
        rx_done_callback(param->data, param->length);
    }

    return (KE_MSG_CONSUMED);
}

void app_datapath_server_register_tx_done(app_datapath_server_tx_done_t callback)
{
    tx_done_callback = callback;
}

void app_datapath_server_register_rx_done(app_datapath_server_data_received_callback_func_t callback)
{
    rx_done_callback = callback;
}

void app_datapath_server_register_disconnected_done(app_datapath_server_disconnected_done_t callback)
{
    disconnected_done_callback = callback;
}

void app_datapath_server_register_connected_done(app_datapath_server_connected_done_t callback)
{
    connected_done_callback = callback;
}

void app_datapath_server_register_mtu_exchanged_done(app_datapath_server_mtuexchanged_done_t callback)
{
    mtuexchanged_done_callback = callback;
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_datapath_server_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_datapath_server_msg_handler},

    {DATAPATHPS_TX_CCC_CHANGED,     (ke_msg_func_t)app_datapath_server_tx_ccc_changed_handler},
    {DATAPATHPS_TX_DATA_SENT,       (ke_msg_func_t)app_datapath_server_tx_data_sent_handler},
    {DATAPATHPS_RX_DATA_RECEIVED,   (ke_msg_func_t)app_datapath_server_rx_data_received_handler},
    {DATAPATHPS_NOTIFICATION_RECEIVED, (ke_msg_func_t)app_datapath_server_rx_data_received_handler},
};

const struct app_subtask_handlers app_datapath_server_table_handler =
    {&app_datapath_server_msg_handler_list[0], (sizeof(app_datapath_server_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_APP_DATAPATH_SERVER

/// @} APP
