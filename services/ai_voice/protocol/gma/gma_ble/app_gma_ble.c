#ifdef __AI_VOICE_BLE_ENABLE__
#ifndef BLE_V2

/**
 ****************************************************************************************
 *
 * @file app_gma_ble.c
 *
 * @brief GMA Application entry point
 *
 * Copyright (C) BES
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"     // SW configuration
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"


#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_gatt_server.h"
#include "app_ai_ble.h"
#include "app_gma_ble.h"                  // ama Voice Application Definitions
#include "gma_gatt_server.h"
#include "app_gma_handle.h"
#include "app_gma_cmd_code.h"
#include "app_ai_if_ble.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
    
//extern bool g_send_via_notification;
void app_gma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id)
{
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(GMA_SEND_CMD_VIA_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

void app_gma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id)
{
   // uint16_t id = 0;
    
  //  if (true == g_send_via_notification)
   // {
  //      id = GMA_SEND_CMD_VIA_NOTIFICATION;
  //  }
 //   else
  //  {
   //     id = GMA_SEND_CMD_VIA_INDICATION;
  //  }
    
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(GMA_SEND_CMD_VIA_INDICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
    
//  g_send_via_notification = false;
}

void app_gma_send_data_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(GMA_SEND_DATA_VIA_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    //TR_DEBUG(2,"%s length %d", __func__, length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

void app_gma_send_data_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(GMA_SEND_DATA_VIA_INDICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

static int app_gma_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

static int app_gma_cmd_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(2,"%s %d", __func__, param->isNotificationEnabled);
    
    uint8_t connect_index = app_ai_set_connect_type(AI_TRANSPORT_BLE, AI_SPEC_ALI);
    app_ai_env[connect_index].isCmdNotificationEnabled = param->isNotificationEnabled;
    if (app_ai_env[connect_index].isCmdNotificationEnabled)
    {
        if (BLE_INVALID_CONNECTION_INDEX == app_ai_env[connect_index].connectionIndex)
        {
            uint8_t conidx = KE_IDX_GET(src_id);
            app_ai_connected_evt_handler(conidx,connect_index);
            TRACE(2,"%s mtu %d", __func__, app_ai_env[connect_index].mtu[conidx]);
            if (app_ai_env[connect_index].mtu[conidx] > 0)
            {
                ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env[connect_index].mtu[conidx], AI_SPEC_ALI, 0);
            }
        }
    }

    ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE,AI_SPEC_ALI,0);

    return (KE_MSG_CONSUMED);
}

static int app_gma_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, AI_SPEC_ALI, 0);

    return (KE_MSG_CONSUMED);
}

static int app_gma_rx_cmd_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(2,"%s length is %d", __func__, param->length);
    ai_function_handle(CALLBACK_CMD_RECEIVE, param->data, param->length, AI_SPEC_ALI, 0);
    return (KE_MSG_CONSUMED);
}

static int app_gma_rx_data_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    ai_function_handle(CALLBACK_DATA_RECEIVE, param->data, param->length, AI_SPEC_ALI, 0);
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_gma_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,            (ke_msg_func_t)app_gma_msg_handler},

    {GMA_CMD_CCC_CHANGED,       (ke_msg_func_t)app_gma_cmd_ccc_changed_handler},
    {GMA_TX_DATA_SENT,          (ke_msg_func_t)app_gma_tx_data_sent_handler},
    {GMA_CMD_RECEIVED,          (ke_msg_func_t)app_gma_rx_cmd_received_handler},
    {GMA_DATA_RECEIVED,             (ke_msg_func_t)app_gma_rx_data_received_handler},
};

const struct ke_state_handler app_gma_table_handler =
    {&app_gma_msg_handler_list[0], (sizeof(app_gma_msg_handler_list)/sizeof(struct ke_msg_handler))};

AI_BLE_HANDLER_TO_ADD(AI_SPEC_ALI, &app_gma_table_handler)
GMA_GATT_ADV_DATA_PAC gma_gatt_adv_data = {GMA_GATT_ADV_DATA_LENGTH, GMA_GATT_ADV_DATA_TYPE, GMA_GATT_ADV_DATA_CID, GMA_GATT_ADV_DATA_VID, GMA_GATT_ADV_DATA_FMSK, GMA_GATT_ADV_DATA_PID, };
#endif
#endif //__AI_VOICE_BLE_ENABLE__

