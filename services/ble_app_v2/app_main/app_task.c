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
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration


#if (BLE_APP_PRESENT)

#include "app_task.h"             // Application Manager Task API
#include "gapc_msg.h"            // GAP Controller Task API
#include "gapm_msg.h"            // GAP Manager Task API

#include "arch.h"                 // Platform Definitions
#include <string.h>
#include "co_utils.h"
#include "ke_timer.h"             // Kernel timer
#include "app_ble_include.h"
#include "app.h"                  // Application Manager Definition

#if (BLE_APP_SEC)
#include "app_sec.h"              // Security Module Definition
#endif //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h"               // Health Thermometer Module Definition
#include "htpt_msg.h"
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h"              // HID Module Definition
#include "hogpd_task.h"
#endif //(BLE_APP_HID)


#if (BLE_APP_VOICEPATH)
#include "app_voicepath_ble.h"       // Voice Path Module Definition
#endif // (BLE_APP_VOICEPATH)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"       // Data Path Server Module Definition
#include "datapathps_task.h"
#endif // (BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_AI_VOICE)
#include "app_ai_ble.h"       // ama Voice Module Definition
#endif // (BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
#include "app_ota.h"       // OTA Module Definition
#endif // (BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h"       // TOTA Module Definition
#endif // (BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h"       // ANC Module Definition
#include "app_ancc_task.h"
#include "ancc_task.h"
#endif // (BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "app_amsc.h"       // AMS Module Definition
#include "app_amsc_task.h"
#include "amsc_task.h"
#endif // (BLE_APP_AMS)
#if (BLE_APP_GFPS)
#include "app_gfps.h"       // google fast pair service provider
#include "gfps_provider_task.h"
#endif // (BLE_APP_GFPS)

#if (DISPLAY_SUPPORT)
#include "app_display.h"          // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#include "ble_app_dbg.h"
#include "nvrecord_ble.h"
#include "app_fp_rfcomm.h"
#if (BLE_APP_TILE)
#include "tile_target_ble.h"
#include "tile_gatt_server.h"
#endif
#if BLE_AUDIO_ENABLED
#include "app_gaf.h"
#endif

#if (BLE_BUDS)
#include "buds.h"
#endif

void app_adv_reported_scanned(struct gapm_ext_adv_report_ind* ptInd);

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#define APP_CONN_PARAM_INTERVEL_MIN    (20)

uint8_t ble_stack_ready = 0;

extern bool app_factorymode_get(void);

static uint8_t app_get_handler(const struct app_subtask_handlers *handler_list_desc,
                               ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list_desc->msg_cnt; 0 < counter; counter--)
    {
        struct ke_msg_handler handler
                = (struct ke_msg_handler)(*(handler_list_desc->p_msg_handler_tab + counter - 1));

        if ((handler.id == msgid) ||
            (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_CREATED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_created_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    LOG_I("adv actv index %d", p_param->actv_idx);
    return (KE_MSG_CONSUMED);
}

int hci_no_operation_cmd_cmp_evt_handler(uint16_t opcode, void const *param)

{
    if (ble_stack_ready)
    {
        return KE_MSG_CONSUMED;
    }

#ifdef __FACTORY_MODE_SUPPORT__
    if (app_factorymode_get())
    {
        return (KE_MSG_CONSUMED);
    }
#endif

    BLE_APP_FUNC_ENTER();

    ble_stack_ready = 1;

    // Reset the stack
    struct gapm_reset_cmd *cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                              TASK_GAPM, TASK_APP,
                                              gapm_reset_cmd);

    cmd->operation = GAPM_RESET;

    ke_msg_send(cmd);
    BLE_APP_FUNC_LEAVE();

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_STOPPED_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_stopped_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    enum app_actv_state state = app_env.state[p_param->actv_idx];
    enum app_actv_state new_state = state;

    if (state == APP_ADV_STATE_STARTED)
    {
        new_state = APP_ADV_STATE_STOPPING;
    }
    else if (state == APP_SCAN_STATE_STARTED)
    {
        new_state = APP_SCAN_STATE_STOPPING;
    }
    else if (state == APP_CONNECT_STATE_STARTED)
    {
        new_state = APP_CONNECT_STATE_STOPPING;
        app_connecting_stopped((void *)&p_param->peer_addr);
    }

    LOG_I("%s actv_idx %d state %d new_state %d", __func__, p_param->actv_idx, state, new_state);
    if (new_state != state)
    {
        // Act as if activity had been stopped by the application
        appm_update_actv_state(p_param->actv_idx, new_state);
        // Perform next operation
        appm_actv_fsm_next(p_param->actv_idx, GAP_ERR_NO_ERROR);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    BLE_APP_FUNC_ENTER();    
    LOG_I("param->operation: 0x%x status is 0x%x actv_idx is %d",
        param->operation, param->status, param->actv_idx);

    switch(param->operation)
    {
        // Reset completed
        case (GAPM_RESET):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                #if (BLE_APP_HID)
                app_hid_start_mouse();
                #endif //(BLE_APP_HID)

                // Set Device configuration
                struct gapm_set_dev_config_cmd* cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;
                // Set the device role - Peripheral
                cmd->role      = GAP_ROLE_ALL;
                // Set Data length parameters
                cmd->sugg_max_tx_octets = APP_MAX_TX_OCTETS;
                cmd->sugg_max_tx_time   = APP_MAX_TX_TIME;
                cmd->pairing_mode = GAPM_PAIRING_LEGACY;
                #ifdef CFG_SEC_CON
                cmd->pairing_mode |= GAPM_PAIRING_SEC_CON;
                #endif
                cmd->att_cfg = 0;
                cmd->privacy_cfg = GAPM_PRIV_CFG_PRIV_ADDR_BIT;

                // load IRK
                memcpy(cmd->irk.key, app_env.loc_irk, KEY_LEN);

                // Send message
                ke_msg_send(cmd);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;

        case (GAPM_PROFILE_TASK_ADD):
        {
            bool status = !appm_add_svc();
            // ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);
            // Add the next requested service
            if (status)
            {
                // Go to the ready state
                ke_state_set(TASK_APP, APPM_READY);

                // No more service to add
                app_ble_system_ready();
            }
        }
        break;

        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            // Go to the create db state
            ke_state_set(TASK_APP, APPM_CREATE_DB);

            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND
            appm_add_svc();
        }
        break;


        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the advertising activity index
                    app_env.adv_actv_idx[app_env.advParam.adv_actv_user] = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_ADV_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_ADV_ACTIVITY status 0x%x", __func__, param->status);
            }
            break;
        }
        case (GAPM_CREATE_SCAN_ACTIVITY):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the scanning activity index
                    app_env.scan_actv_idx = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_SCAN_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_SCAN_ACTIVITY status %d", __func__, param->status);
            }
            break;
        }
        case (GAPM_CREATE_INIT_ACTIVITY):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                if (APP_ACTV_STATE_IDLE == app_env.state[param->actv_idx])
                {
                    // Store the connecting activity index
                    app_env.connect_actv_idx = param->actv_idx;
                }
                appm_update_actv_state(param->actv_idx, APP_CONNECT_STATE_CREATING);
                appm_actv_fsm_next(param->actv_idx, param->status);
            }
            else
            {
                ASSERT(0, "%s GAPM_CREATE_INIT_ACTIVITY status %d", __func__, param->status);
            }
            break;
        }
        case (GAPM_STOP_ACTIVITY):
        case (GAPM_START_ACTIVITY):
        case (GAPM_DELETE_ACTIVITY):
        case (GAPM_SET_ADV_DATA):
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            // Perform next operation
            appm_actv_fsm_next(param->actv_idx, param->status);
        } break;

        case (GAPM_DELETE_ALL_ACTIVITIES) :
        {
            //don't use this function!!!

            // Re-Invoke Advertising
            //appm_update_actv_state(param->actv_idx, APP_ACTV_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);

            //appm_update_actv_state(param->actv_idx, APP_SCAN_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);

            //appm_update_actv_state(param->actv_idx, APP_CONNECT_STATE_IDLE);
            //appm_actv_fsm_next(param->actv_idx);
        } break;

        case GAPM_RESOLV_ADDR:
        {
            LOG_I("Resolve result 0x%x", param->status);
            if (GAP_ERR_NOT_FOUND == param->status)
            {
                appm_random_ble_addr_solved(false, NULL);
            }
            break;
        }
        case GAPM_SET_WL:
        {
            LOG_I("GAPM_SET_WL result 0x%x", param->status);
            if (GAP_ERR_NO_ERROR == param->status)
            {
                app_ble_set_white_list_complete();
            }
            break;
        }
        default:
        {
            // Drop the message
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
                                             struct gapc_get_dev_info_req_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    LOG_I("%s req %d token 0x%x", __FUNCTION__, param->req, param->token);
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                                 src_id, dest_id,
                                                                 gapc_get_dev_info_cfm,
                                                                 APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->status = GAP_ERR_NO_ERROR;
            cfm->token = param->token;
            cfm->info.name.value_length = appm_get_dev_name(cfm->info.name.value);
            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            // Set the device appearance
            #if (BLE_APP_HT)
            // Generic Thermometer - TODO: Use a flag
            cfm->info.appearance = 728;
            #elif (BLE_APP_HID)
            // HID Mouse
            cfm->info.appearance = 962;
            #else
            // No appearance
            cfm->info.appearance = 0;
            #endif

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    src_id, dest_id,
                                                            gapc_get_dev_info_cfm);
            cfm->req = param->req;
            cfm->token = param->token;
            // Slave preferred Connection interval Min
            cfm->info.slv_pref_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_pref_params.con_intv_max = 10;
            // Slave preferred Connection latency
            cfm->info.slv_pref_params.slave_latency  = 0;
            // Slave preferred Link supervision timeout
            cfm->info.slv_pref_params.conn_timeout    = 200;  // 2s (500*10ms)

            // Send message
            ke_msg_send(cfm);
        } break;

        default: /* Do Nothing */ break;
    }


    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Set Device configuration
    struct gapc_set_dev_info_cfm* cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                     gapc_set_dev_info_cfm);
    // Reject to change parameters
    cfm->status = GAP_ERR_REJECTED;
    cfm->req = param->req;
    // Send message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static void POSSIBLY_UNUSED gapc_refresh_remote_dev_feature(uint8_t conidx)
{
    // Send a GAPC_GET_INFO_CMD in order to read the device name characteristic value
    struct gapc_get_info_cmd *p_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                                   gapc_get_info_cmd);

    // request peer device name.
    p_cmd->operation = GAPC_GET_PEER_FEATURES;

    // send command
    ke_msg_send(p_cmd);
}

static void POSSIBLY_UNUSED gpac_exchange_data_packet_length(uint8_t conidx)
{
    LOG_I("%s", __func__);
    struct gapc_set_le_pkt_size_cmd *set_le_pakt_size_req = KE_MSG_ALLOC(GAPC_SET_LE_PKT_SIZE_CMD,
                                                                         KE_BUILD_ID(TASK_GAPC, conidx),
                                                                         TASK_APP,
                                                                         gapc_set_le_pkt_size_cmd);

    set_le_pakt_size_req->operation = GAPC_SET_LE_PKT_SIZE;
    set_le_pakt_size_req->tx_octets = APP_MAX_TX_OCTETS;
    set_le_pakt_size_req->tx_time = APP_MAX_TX_TIME;
    // Send message
    ke_msg_send(set_le_pakt_size_req);
}

static int gapc_peer_features_ind_handler(ke_msg_id_t const msgid,
                                          struct gapc_peer_features_ind* param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    LOG_I("Peer dev feature is:");
    DUMP8("0x%02x ", param->features, GAP_LE_FEATS_LEN);
    uint8_t conidx = KE_IDX_GET(src_id);

    if (param->features[0] & (1<<BLE_FEAT_DATA_PKT_LEN_EXT))
    {
        gpac_exchange_data_packet_length(conidx);
    }
    return (KE_MSG_CONSUMED);
}

void app_exchange_remote_feature(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

    LOG_I("connectStatus:%d, isFeatureExchanged:%d",
          pContext->connectStatus,
          pContext->isFeatureExchanged);

    if ((BLE_CONNECTED == pContext->connectStatus) && !pContext->isFeatureExchanged)
    {
        gapc_refresh_remote_dev_feature(conidx);
        pContext->isFeatureExchanged = true;
    }
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    // Check if the received Connection Handle was valid
    ASSERT(conidx != GAP_INVALID_CONIDX, "%s invalid conidx 0x%x", __func__, conidx);

    APP_BLE_CONN_CONTEXT_T* pContext = &(app_env.context[conidx]);
    
    pContext->connectStatus = BLE_CONNECTED;

    ke_state_set(dest_id, APPM_CONNECTED);

    app_env.conn_cnt++;

    if (app_is_resolving_ble_bd_addr())
    {
        LOG_I("A ongoing ble addr solving is in progress, refuse the new connection.");
        app_ble_start_disconnect(conidx);
        return KE_MSG_CONSUMED;
    }

    LOG_I("[CONNECT]device info:");
    LOG_I("peer addr:");
    DUMP8("%02x ", param->peer_addr.addr, BT_ADDR_OUTPUT_PRINT_NUM);
    LOG_I("peer addr type:%d", param->peer_addr_type);
    LOG_I("connection index:%d, isGotSolvedBdAddr:%d", conidx, pContext->isGotSolvedBdAddr);
    LOG_I("conn interval:%d, timeout:%d", param->con_interval, param->sup_to);

    BLE_APP_FUNC_ENTER();

    pContext->peerAddrType = param->peer_addr_type;
    memcpy(pContext->bdAddr, param->peer_addr.addr, BD_ADDR_LEN);
    
    // Retrieve the connection info from the parameters
    pContext->conhdl = param->conhdl;

    if (BLE_RANDOM_ADDR == pContext->peerAddrType)
    {
        pContext->isGotSolvedBdAddr = false;
    }
    else
    {
        pContext->isGotSolvedBdAddr = true;
    }

    app_env.context[conidx].connParam.con_interval = param->con_interval;
    app_env.context[conidx].connParam.con_latency = param->con_latency;
    app_env.context[conidx].connParam.sup_to = param->sup_to;
    app_env.context[conidx].conn_param_update_times = 0;

#if (BLE_APP_SEC)
    app_sec_reset_env_on_connection();
#endif

    // Send connection confirmation
    struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
            KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
            gapc_connection_cfm);

#if(BLE_APP_SEC)
    cfm->pairing_lvl      = app_sec_get_bond_status() ? BLE_AUTHENTICATION_LEVEL : GAP_AUTH_REQ_NO_MITM_NO_BOND;
#else // !(BLE_APP_SEC)
    cfm->pairing_lvl      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
#endif // (BLE_APP_SEC)
    // Send the message
    ke_msg_send(cfm);

    // We are now in connected State
    ke_state_set(dest_id, APPM_CONNECTED);
    app_exchange_remote_feature(conidx);

    app_ble_connected_evt_handler(conidx, param->peer_addr.addr);

#if (BLE_APP_TILE)
    app_tile_connected_evt_handler(conidx, param);
#endif
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_DEFAULT, true);
    BLE_APP_FUNC_LEAVE();

#if (BLE_BUDS)
    buds_register_cli_event(conidx);
#endif

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    LOG_I("%s conidx 0x%02x op 0x%02x", __func__, conidx, param->operation);
    switch(param->operation)
    {
        case (GAPC_UPDATE_PARAMS):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                app_ble_update_param_failed(conidx, param->status);
//                appm_disconnect();
            }
        } break;

        default:
        {
        } break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    LOG_I("[DISCONNECT] device info:");
    uint8_t conidx = KE_IDX_GET(src_id);
    LOG_I("connection index:%d, reason:0x%x", conidx, param->reason);

    app_env.context[conidx].isBdAddrResolvingInProgress = false;
    app_env.context[conidx].isGotSolvedBdAddr = false;

    app_env.context[conidx].connectStatus = BLE_DISCONNECTED;
    app_env.context[conidx].isFeatureExchanged = false;
    app_env.context[conidx].connParam.con_interval = 0;
    app_env.context[conidx].connParam.con_latency = 0;
    app_env.context[conidx].connParam.sup_to = 0;
    app_env.context[conidx].conn_param_update_times = 0;
    //l2cm_buffer_reset(conidx);

    // Go to the ready state
    ke_state_set(TASK_APP, APPM_READY);

    app_env.conn_cnt--;

    #if (BLE_VOICEPATH)
    app_voicepath_disconnected_evt_handler(conidx);
    #endif

    #if (BLE_DATAPATH_SERVER)
    app_datapath_server_disconnected_evt_handler(conidx);
    #endif

    #if (BLE_OTA)
    app_ota_disconnected_evt_handler(conidx);
    #endif

    #if (BLE_APP_TOTA)
    app_tota_disconnected_evt_handler(conidx);
    #endif

    #if(BLE_APP_GFPS)
    app_gfps_disconnected_evt_handler(conidx);
    #endif
    

    #if (BLE_AI_VOICE)
    app_ai_disconnected_evt_handler(conidx);
    #endif

    #if (BLE_APP_TILE)
    app_tile_disconnected_evt_handler(conidx);
    #endif

    app_ble_disconnected_evt_handler(conidx, param->reason);

    app_ble_reset_conn_param_mode(conidx);
    return (KE_MSG_CONSUMED);
}

static int gapc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_mtu_changed_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    LOG_I("MTU has been negotiated as %d conidx %d", param->mtu, conidx);


#if (BLE_APP_DATAPATH_SERVER)
    app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_VOICEPATH)
    app_voicepath_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_OTA)
    app_ota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_APP_TOTA)
    app_tota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_AI_VOICE)
    app_ai_mtu_exchanged_handler(conidx, param->mtu);
#endif

    return (KE_MSG_CONSUMED);
}

static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    LOG_I("Profile prf_task_id %d is added.", param->prf_task_id);

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol = KE_MSG_CONSUMED;

    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            #if (BLE_APP_SEC)
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_BOND_DATA_UPDATE_IND))
            {
                // Call the Security Module
                msg_pol = app_get_handler(&app_sec_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_SEC)
            // else drop the message
        } break;

        #if BLE_AUDIO_ENABLED
        case (TASK_ID_GAF):
        {
            msg_pol = app_get_handler(&app_gaf_handlers, msgid, param, src_id);
        } break;
        #endif

        case (TASK_ID_GATT):
        {
            // Service Changed - Drop
        } break;

        #if (BLE_APP_HT)
        case (TASK_ID_HTPT):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HT)

        #if (BLE_APP_DIS)
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = app_get_handler(&app_dis_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_DIS)

        #if (BLE_APP_HID)
        case (TASK_ID_HOGPD):
        {
            // Call the HID Module
            msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HID)

        #if (BLE_APP_BATT)
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = app_get_handler(&app_batt_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_BATT)

        #if (BLE_APP_AM0)
        case (TASK_ID_AM0):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_handlers, msgid, param, src_id);
        } break;

        case (TASK_ID_AM0_HAS):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_has_handlers, msgid, param, src_id);
        } break;
        #endif // defined(BLE_APP_AM0)

        #if (BLE_APP_HR)
        case (TASK_ID_HRPS):
        {
            // Call the HRPS Module
            msg_pol = app_get_handler(&app_hrps_table_handler, msgid, param, src_id);
        } break;
        #endif
        
        #if (BLE_APP_VOICEPATH)
        case (TASK_ID_VOICEPATH):
        {
            // Call the Voice Path Module
            msg_pol = app_get_handler(app_voicepath_ble_get_msg_handler_table(), msgid, param, src_id);
        } break;
        #endif //(BLE_APP_VOICEPATH)

        #if (BLE_APP_DATAPATH_SERVER)
        case (TASK_ID_DATAPATHPS):
        {
            // Call the Data Path Module
            msg_pol = app_get_handler(&app_datapath_server_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_DATAPATH_SERVER)
        #if (BLE_APP_TILE)
        case (TASK_ID_TILE):
        {
        // Call the TILE Module
            msg_pol = app_get_handler(&app_tile_table_handler, msgid, param, src_id);
        } 
        break;
        #endif

        #if (BLE_APP_AI_VOICE)
        case (TASK_ID_AI):
        {
            // Call the AI Voice
            msg_pol = app_get_handler(app_ai_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_AI_VOICE)

        #if (BLE_APP_OTA)
        case (TASK_ID_OTA):
        {
            // Call the OTA
            msg_pol = app_get_handler(&app_ota_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_OTA)

        #if (BLE_APP_TOTA)
        case (TASK_ID_TOTA):
        {
            // Call the TOTA
            msg_pol = app_get_handler(&app_tota_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_TOTA)
        
        #if (BLE_APP_ANCC)
        case (TASK_ID_ANCC):
        {
            // Call the ANCC
            msg_pol = app_get_handler(&app_ancc_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_ANCC)

        #if (BLE_APP_AMS)
        case (TASK_ID_AMSC):
        {
            // Call the AMS
            msg_pol = app_get_handler(&app_amsc_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_AMS)
        #if (BLE_APP_GFPS)
        case (TASK_ID_GFPSP):
        {
            msg_pol = app_get_handler(&app_gfps_table_handler, msgid, param, src_id);
        } break;
        #endif
        default:
        {
            #if (BLE_APP_HT)
            if (msgid == APP_HT_MEAS_INTV_TIMER)
            {
                msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HT)

            #if (BLE_APP_HID)
            if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER)
            {
                msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HID)
        } break;
    }

    return (msg_pol);
}

static int gapm_adv_report_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_ext_adv_report_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)    
{
    app_adv_reported_scanned(param);
    return KE_MSG_CONSUMED;
}

static int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_addr_solved_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)    
{
    /// Indicate that resolvable random address has been solved
    appm_random_ble_addr_solved(true, param->irk.key);
    return KE_MSG_CONSUMED;
}

// TODO: update to use new API
#if 0
__STATIC int gattc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                        struct gattc_mtu_changed_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    
    LOG_I("MTU has been negotiated as %d conidx %d", param->mtu, conidx);

#if (BLE_APP_DATAPATH_SERVER)
    app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_VOICEPATH)
    app_voicepath_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_OTA)
    app_ota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_APP_TOTA)
    app_tota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_AI_VOICE)
    app_ai_mtu_exchanged_handler(conidx, param->mtu);
#endif

    return (KE_MSG_CONSUMED);
}
#endif

#define APP_CONN_PARAM_INTERVEL_MAX    (30)
__STATIC int gapc_conn_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_param_update_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    bool accept = true;
    ble_event_t event;

    LOG_I("Receive the conn param update request: min %d max %d latency %d timeout %d",
          param->intv_min,
          param->intv_max,
          param->latency,
          param->time_out);

    struct gapc_param_update_cfm* cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM, src_id, dest_id,
                                            gapc_param_update_cfm);

    event.evt_type = BLE_CONN_PARAM_UPDATE_REQ_EVENT;
    event.p.conn_param_update_req_handled.intv_min = param->intv_min;
    event.p.conn_param_update_req_handled.intv_max = param->intv_max;
    event.p.conn_param_update_req_handled.latency  = param->latency;
    event.p.conn_param_update_req_handled.time_out = param->time_out;
    
    app_ble_core_global_handle(&event, &accept);
    LOG_I("%s ret %d ", __func__, accept);

    cfm->accept = true;

#ifdef GFPS_ENABLED
    //make sure ble cnt interval isnot less than 15ms, in order to prevent bt collision
    if (param->intv_min < (uint16_t)(15/1.25))
    {
        LOG_I("accept");
        fp_update_ble_connect_param_start(KE_IDX_GET(src_id));
    }
    else
    {
        fp_update_ble_connect_param_stop(KE_IDX_GET(src_id));
    }
#else

    if (!accept)
    {
        // when a2dp or sco streaming is on-going
        //make sure ble cnt interval isnot less than 15ms, in order to prevent bt collision
        if (param->intv_min <= (uint16_t)(15/1.25))
        {
            cfm->accept = false;
        }
    }
#endif

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int gapc_conn_param_updated_handler(ke_msg_id_t const msgid,
                                          struct gapc_param_updated_ind* param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    LOG_I("Conidx %d BLE conn parameter is updated as interval %d timeout %d", 
        conidx, param->con_interval, param->sup_to);

#if (BLE_VOICEPATH)
    app_voicepath_ble_conn_parameter_updated(conidx, param->con_interval, param->con_latency);
#endif
#if BLE_APP_TILE
    app_tile_ble_conn_parameter_updated(conidx, param);
#endif

    app_env.context[conidx].connParam.con_interval = param->con_interval;
    app_env.context[conidx].connParam.con_latency = param->con_latency;
    app_env.context[conidx].connParam.sup_to = param->sup_to;
    app_ble_update_param_successful(conidx, &app_env.context[conidx].connParam);
    return (KE_MSG_CONSUMED);
}

static int gapm_dev_addr_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_dev_bdaddr_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    ble_event_t event;
    // Indicate that a new random BD address set in lower layers
    LOG_I("New dev addr:");
    DUMP8("%02x ", param->addr.addr, BT_ADDR_OUTPUT_PRINT_NUM);

#ifdef GFPS_ENABLED
    app_fp_msg_send_updated_ble_addr(0,false);
    app_gfps_update_random_salt();
#endif
    event.evt_type = BLE_SET_RANDOM_BD_ADDR_EVENT;
    event.p.set_random_bd_addr_handled.new_bdaddr = param->addr.addr;
    app_ble_core_global_handle(&event, NULL);

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of random number generated message
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_gen_rand_nb_ind_handler(ke_msg_id_t const msgid, struct gapm_gen_rand_nb_ind *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

    return KE_MSG_CONSUMED;
}
/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(appm)
{
    // GAPM messages
    {GAPM_CMP_EVT,              (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPM_DEV_BDADDR_IND,       (ke_msg_func_t)gapm_dev_addr_ind_handler},
    {GAPM_ADDR_SOLVED_IND,      (ke_msg_func_t)gapm_addr_solved_ind_handler},
    {GAPM_GEN_RAND_NB_IND,      (ke_msg_func_t)gapm_gen_rand_nb_ind_handler},
    {GAPM_ACTIVITY_CREATED_IND, (ke_msg_func_t)gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND, (ke_msg_func_t)gapm_activity_stopped_ind_handler},
    {GAPM_EXT_ADV_REPORT_IND,   (ke_msg_func_t)gapm_adv_report_ind_handler},
    {GAPM_PROFILE_ADDED_IND,    (ke_msg_func_t)gapm_profile_added_ind_handler},

    // GAPC messages
    {GAPC_CMP_EVT,              (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_CONNECTION_REQ_IND,   (ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GAPC_MTU_CHANGED_IND,      (ke_msg_func_t)gapc_mtu_changed_ind_handler},
    {GAPC_PEER_FEATURES_IND,    (ke_msg_func_t)gapc_peer_features_ind_handler},
    {GAPC_GET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_set_dev_info_req_ind_handler},

    // TODO: update to use new API
    // {GATTC_MTU_CHANGED_IND,   (ke_msg_func_t)gattc_mtu_changed_ind_handler},
    {GAPC_PARAM_UPDATE_REQ_IND, (ke_msg_func_t)gapc_conn_param_update_req_ind_handler},
    {GAPC_PARAM_UPDATED_IND,    (ke_msg_func_t)gapc_conn_param_updated_handler},

    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)appm_msg_handler},
};

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

// Application task descriptor
const struct ke_task_desc TASK_DESC_APP = {appm_msg_handler_tab, appm_state, APP_IDX_MAX, ARRAY_LEN(appm_msg_handler_tab)};

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
