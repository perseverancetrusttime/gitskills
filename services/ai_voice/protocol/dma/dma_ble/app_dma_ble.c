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

#include "app_dma_handle.h"
#include "dma_gatt_server.h"
#include "app_dma_ble.h"                  // Smart Voice Application Definitions
#include "ai_manager.h"
#include "ai_gatt_server.h"
#include "app_ai_ble.h"
#include "ai_thread.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ai_add_dma(void)
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
void app_dma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(DMA_SEND_CMD_VIA_NOTIFICATION,
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

void app_dma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(DMA_SEND_CMD_VIA_INDICATION,
                                                prf_get_task_from_id((TASK_ID_AI)),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

bool app_dma_send_data_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(DMA_SEND_DATA_VIA_NOTIFICATION,
                                                prf_get_task_from_id((TASK_ID_AI)),
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

void app_dma_send_data_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    struct ble_ai_send_data_req_t * req = KE_MSG_ALLOC_DYN(DMA_SEND_DATA_VIA_INDICATION,
                                                prf_get_task_from_id((TASK_ID_AI)),
                                                TASK_APP,
                                                ble_ai_send_data_req_t,
                                                length);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    req->connecionIndex = app_ai_env[foreground_ai].connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

static int app_dma_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

static int app_dma_cmd_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(2,"%s %d", __func__, param->isNotificationEnabled);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    app_ai_env[foreground_ai].isCmdNotificationEnabled = param->isNotificationEnabled;
    return (KE_MSG_CONSUMED);
}

static int app_dma_data_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ai_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(2,"%s %d", __func__, param->isNotificationEnabled);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t connect_index = app_ai_set_ble_connect_info(AI_TRANSPORT_BLE, AI_SPEC_BAIDU, conidx);

    app_ai_env[connect_index].isCmdNotificationEnabled = param->isNotificationEnabled;
    if (app_ai_env[connect_index].isCmdNotificationEnabled)
    {
        if (BLE_INVALID_CONNECTION_INDEX == app_ai_env[connect_index].connectionIndex)
        {
            app_ai_connected_evt_handler(conidx, connect_index);

            if (app_ai_env[connect_index].mtu[conidx] > 0)
            {
                ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env[connect_index].mtu[conidx], AI_SPEC_BAIDU, 0);
            }
        }
    }

    ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE, AI_SPEC_BAIDU, 0);
    return (KE_MSG_CONSUMED);
}

static int app_dma_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, AI_SPEC_BAIDU, 0);

    return (KE_MSG_CONSUMED);
}

static int app_dma_rx_cmd_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(1,"%s", __func__);
    return (KE_MSG_CONSUMED);
}

static int app_dma_rx_data_received_handler(ke_msg_id_t const msgid,
                              struct ble_ai_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    ai_function_handle(CALLBACK_DATA_RECEIVE, param->data, param->length, AI_SPEC_BAIDU, 0);

    return (KE_MSG_CONSUMED);
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_dma_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_dma_msg_handler},

    {DMA_CMD_CCC_CHANGED,       (ke_msg_func_t)app_dma_cmd_ccc_changed_handler},
    {DMA_DATA_CCC_CHANGED,      (ke_msg_func_t)app_dma_data_ccc_changed_handler},
    {DMA_TX_DATA_SENT,          (ke_msg_func_t)app_dma_tx_data_sent_handler},
    {DMA_CMD_RECEIVED,          (ke_msg_func_t)app_dma_rx_cmd_received_handler},
    {DMA_DATA_RECEIVED,         (ke_msg_func_t)app_dma_rx_data_received_handler},
};

const struct ke_state_handler app_dma_table_handler =
    {&app_dma_msg_handler_list[0], (sizeof(app_dma_msg_handler_list)/sizeof(struct ke_msg_handler))};

AI_BLE_HANDLER_TO_ADD(AI_SPEC_BAIDU, &app_dma_table_handler)


/// @} APP
#endif
#endif //__AI_VOICE_BLE_ENABLE__

