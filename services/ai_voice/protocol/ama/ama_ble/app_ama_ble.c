#ifdef __AI_VOICE_BLE_ENABLE__
#ifndef BLE_V2
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_task.h"                // application task definitions         
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"

#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_gatt_server.h"
#include "app_ai_ble.h"
#include "app_ai_if_ble.h"
#include "app_ama_ble.h"                  // ama Voice Application Definitions
#include "ama_gatt_server.h"
#include "ai_manager.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ai_add_ama(void)
{
    BLE_APP_DBG("%s", __func__);
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM,
                                                  TASK_APP,
                                                  gapm_profile_task_add_cmd,
                                                  0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, UNAUTH) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, UNAUTH);
#endif
    req->prf_task_id = TASK_ID_AI;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Send the message
    ke_msg_send(req);
}

bool app_ama_send_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t dest_id)
{
    //TRACE(2,"%s len= %d", __func__, length);
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(AMA_SEND_VIA_NOTIFICATION,
                                                prf_get_task_from_id(TASK_ID_AI),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
    return true;
}

void app_ama_send_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t dest_id)
{
    TRACE(2,"%s len= %d", __func__, length);
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(AMA_SEND_VIA_INDICATION,
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

static int app_ama_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(1,"%s", __func__);
    // Do nothing
    return (KE_MSG_CONSUMED);
}

static int app_ama_cmd_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    TRACE(3,"%s %d index %d", __func__, param->isNotificationEnabled, conidx);
    uint8_t connect_index = app_ai_set_ble_connect_info(AI_TRANSPORT_BLE, AI_SPEC_AMA, conidx);

    if (app_ai_ble_is_connected(connect_index) && (conidx != app_ai_env[connect_index].connectionIndex))
    {
        TRACE(2,"conidx error %d %d", conidx, app_ai_env[connect_index].connectionIndex);
        return (KE_MSG_CONSUMED);
    }

    app_ai_env[connect_index].isCmdNotificationEnabled = param->isNotificationEnabled;
    if (app_ai_env[connect_index].isCmdNotificationEnabled)
    {
        if (BLE_INVALID_CONNECTION_INDEX == app_ai_env[connect_index].connectionIndex)
        {
            app_ai_connected_evt_handler(conidx, connect_index);
            if (app_ai_env[connect_index].mtu[conidx] > 0)
            {
                ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env[connect_index].mtu[conidx], AI_SPEC_AMA, 0);
            }
        }
    }
    ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE, AI_SPEC_AMA, 0);

    return (KE_MSG_CONSUMED);
}

static int app_ama_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, AI_SPEC_AMA, 0);

    return (KE_MSG_CONSUMED);
}

static int app_ama_rx_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(1,"%s ======>", __func__);
    DUMP8("%02x ",param->data, param->length);
    ai_function_handle(CALLBACK_CMD_RECEIVE, param->data, param->length, AI_SPEC_AMA, 0);
    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const ai_ble_ke_msg_handler_t app_ama_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ai_ble_ke_msg_func_t)app_ama_msg_handler},
    {AMA_CCC_CHANGED,           (ai_ble_ke_msg_func_t)app_ama_cmd_ccc_changed_handler},
    {AMA_TX_DATA_SENT,          (ai_ble_ke_msg_func_t)app_ama_tx_data_sent_handler},//ama ble data sent callback
    {AMA_RECEIVED,              (ai_ble_ke_msg_func_t)app_ama_rx_received_handler},
};

const ai_ble_ke_state_handler_t app_ama_table_handler =
    {&app_ama_msg_handler_list[0], (sizeof(app_ama_msg_handler_list)/sizeof(ai_ble_ke_msg_handler_t))};

AI_BLE_HANDLER_TO_ADD(AI_SPEC_AMA, &app_ama_table_handler)

#endif //BLE_V2
#endif //__AI_VOICE_BLE_ENABLE__

