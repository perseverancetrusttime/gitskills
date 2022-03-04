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

#if (BLE_APP_OTA)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_ota.h"                  // OTA Application Definitions
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"
#include "ota_control.h"
#include "ota_bes.h"
#include "ota.h"
#include "app_ble_rx_handler.h"
#include "app_ble_mode_switch.h"
#include "nvrecord_ble.h"

#if defined(IBRT)
#include "app_ibrt_if.h"
#if defined(IBRT_UI_V1)
#include "app_tws_ibrt.h"
#else
#include "app_tws_ibrt_conn_api.h"
extern bool app_ibrt_if_is_any_mobile_connected(void); // TODO:
#endif
#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct app_ota_env_tag app_ota_env;

static app_ota_tx_done_t ota_data_tx_done_callback = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ota_connected_evt_handler(uint8_t conidx)
{
    app_ota_env.connectionIndex = conidx;
    app_ota_connected(APP_OTA_CONNECTED);
    ota_control_set_datapath_type(DATA_PATH_BLE);
    ota_control_register_transmitter(app_ota_send_notification);
    l2cap_update_param(conidx, 10, 15, 20000, 0);
}

void app_ota_disconnected_evt_handler(uint8_t conidx)
{
    uint8_t linkType = APP_OTA_LINK_TYPE_BLE;
    if (conidx == app_ota_env.connectionIndex)
    {
        app_ota_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
        app_ota_env.isNotificationEnabled = false;
        app_ota_env.mtu[conidx] = 0;
        app_ota_disconnected(APP_OTA_DISCONNECTED, linkType);
    }
}

static void app_ota_ble_data_fill_handler(void *param)
{
    // normally we won't allow OTA owned adv when there is already
    // an existing BLE connection. For special requirement, you can
    // disable this limitation
    if (app_ble_connection_count() > BLE_CONNECTION_MAX)
    {
        app_ble_data_fill_enable(USER_OTA, false);
        return;
    }

    bool adv_enable = false;

#if defined(IBRT)
#if defined(IBRT_UI_V1)
    if (app_tws_ibrt_get_bt_ctrl_ctx()->init_done)
    {
        if (TWS_UI_MASTER != app_ibrt_if_get_ui_role())
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ibrt_if_get_ui_role());
        }
        else if (!app_tws_ibrt_mobile_link_connected())
        {
            TRACE(1,"%s don't connect mobile", __func__);
        }
        else
        {
            adv_enable = true;
        }
    }
#else
    if (app_tws_ibrt_get_bt_ctrl_ctx()->init_done)
    {
        if (TWS_UI_MASTER != app_ibrt_if_get_ui_role())
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ibrt_if_get_ui_role());
        }
        else if (!app_ibrt_if_is_any_mobile_connected())
        {
            TRACE(1,"%s don't connect mobile", __func__);
        }
        else
        {
            adv_enable = true;
        }
    }
#endif
#else
    adv_enable = true;
#endif
#if defined(BLE_RPA_OTA_WORKROUND)
        BLE_ADV_PARAM_T *advInfo = ( BLE_ADV_PARAM_T * )param;
        uint16_t rd = nv_record_get_self_ble_rand();
        TRACE(2,"%s cur random %x", __func__,rd);
        TRACE(3,"%sadv data offset:%d, scan response data offset:%d",
                   __func__,
                   advInfo->advDataLen,
                   advInfo->scanRspDataLen);
        advInfo->advData[advInfo->advDataLen++] = 0x03;
        advInfo->advData[advInfo->advDataLen++] = BLE_ADV_MANU_FLAG;
        advInfo->advData[advInfo->advDataLen++] = (uint8_t)((rd >> 8) & 0xFF);
        advInfo->advData[advInfo->advDataLen++] = (uint8_t)(rd & 0xFF);
#endif

    app_ble_data_fill_enable(USER_OTA, adv_enable);
}

void app_ota_init(void)
{
    // Reset the environment
    app_ota_env.connectionIndex =  BLE_INVALID_CONNECTION_INDEX;
    app_ota_env.isNotificationEnabled = false;
    memset((uint8_t *)&(app_ota_env.mtu), 0, sizeof(app_ota_env.mtu));  

    app_ble_register_data_fill_handle(USER_OTA, 
        ( BLE_DATA_FILL_FUNC_T )app_ota_ble_data_fill_handler, false);
}

void app_ota_add_ota(void)
{
    BLE_APP_DBG("app_ota_add_ota");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM, TASK_APP,
                         gapm_profile_task_add_cmd, 0);

    /// Fill message
    req->operation  = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl    = SVC_SEC_LVL(NOT_ENC) | SVC_UUID(128);
    req->user_prio  = 0;
    req->prf_api_id = TASK_ID_OTA;
    req->app_task   = TASK_APP;
    req->start_hdl  = 0;

    /// Send the message
    ke_msg_send(req);
}

void app_ota_send_notification(uint8_t* ptrData, uint32_t length)
{
    ota_send_ind_ntf_generic(true,
                             app_ota_env.connectionIndex,
                             ptrData,
                             length);
}

void app_ota_send_indication(uint8_t* ptrData, uint32_t length)
{
    ota_send_ind_ntf_generic(false,
                             app_ota_env.connectionIndex,
                             ptrData,
                             length);
}

void app_ota_send_rx_cfm(uint8_t conidx)
{
    if(BLE_INVALID_CONNECTION_INDEX == conidx)
    {
        return;
    }

    ota_send_rx_cfm(conidx);
}

static int app_ota_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing
    return (KE_MSG_CONSUMED);
}

static int app_ota_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_ota_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    TRACE(1,"ota data ccc changed to %d", param->ntfIndEnableFlag);
    app_ota_env.isNotificationEnabled = (param->ntfIndEnableFlag > 0);

    if (app_ota_env.isNotificationEnabled)
    {
        uint8_t conidx = app_ota_env.connectionIndex;
        TRACE(2, "%s %d", __func__, conidx);
        app_ota_connected_evt_handler(conidx);

        if (app_ota_env.mtu[conidx])
        {
            ota_control_update_MTU(app_ota_env.mtu[conidx]);
        }
    }

    return (KE_MSG_CONSUMED);
}

static int app_ota_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_ota_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    if (NULL != ota_data_tx_done_callback)
    {
        ota_data_tx_done_callback();
    }

    //ota_data_transmission_done_callback();

    return (KE_MSG_CONSUMED);
}

static int app_ota_data_received_handler(ke_msg_id_t const msgid,
                                         struct ble_ota_rx_data_ind_t *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    TRACE(2, "%s", __func__);

    app_ble_push_rx_data(BLE_RX_DATA_SELF_OTA, app_ota_env.connectionIndex, param->data, param->length);
    return (KE_MSG_CONSUMED);
}

void app_ota_register_tx_done(app_ota_tx_done_t callback)
{
    ota_data_tx_done_callback = callback;
}

void app_ota_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU)
{
    if (conidx == app_ota_env.connectionIndex)
    {
        ota_control_update_MTU(MTU);
    }
    else
    {
        app_ota_env.mtu[conidx] = MTU;
    }
}

uint8_t app_ota_get_conidx(void)
{
    TRACE(2, "%s %d", __func__, app_ota_env.connectionIndex);
    return app_ota_env.connectionIndex;
}

void app_ota_update_conidx(uint8_t conidx)
{
    app_ota_env.connectionIndex = conidx;
}
/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_ota_msg_handler_list[] =
{
    // Note: first message is last message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_ota_msg_handler},

    {OTA_CCC_CHANGED,           (ke_msg_func_t)app_ota_ccc_changed_handler},
    {OTA_TX_DATA_SENT,          (ke_msg_func_t)app_ota_tx_data_sent_handler},
    {OTA_DATA_RECEIVED,         (ke_msg_func_t)app_ota_data_received_handler},
};

const struct app_subtask_handlers app_ota_table_handler =
    {app_ota_msg_handler_list, ARRAY_SIZE(app_ota_msg_handler_list)};

#endif //BLE_APP_OTA

/// @} APP
