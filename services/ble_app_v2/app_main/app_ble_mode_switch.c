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
#include "gapc_msg.h"
#include "nvrecord_ble.h"
#include "app_sec.h"
#include "besbt.h"
#include "bt_drv_interface.h"

#if BLE_AUDIO_ENABLED
#include "app_bapc_uc_msg.h"
#include "app_arc_micc_msg.h"
#endif
#ifdef AOB_UX_ENABLED
#include "aob_stm_generic.h"
#include "aob_tws_conn_mgr_stm.h"
#include "aob_mobile_conn_mgr_stm.h"
#endif

/************************private macro defination***************************/
#define APP_BLE_UPDATE_CONN_PARAM_TIMEOUT_INTERVEL          (2000)
#define APP_BLE_UPDATE_CONN_PARAM_MAX_TIMES                 (3)

/************************private type defination****************************/
typedef struct
{
    uint8_t conidx;
    uint8_t errCode;
} BLE_UPDATE_TIMEOUT_CONN_PARAM_T;

BLE_UPDATE_TIMEOUT_CONN_PARAM_T ble_update_timeout_conn_param[BLE_CONNECTION_MAX];

static void app_ble_update_conn_param_timeout_cb(void const *n);
osTimerDef (APP_BLE_UPDATE_CONN_PARAM_TIMEOUT, app_ble_update_conn_param_timeout_cb);
osTimerId   app_ble_update_conn_param_timer_id = NULL;

/************************extern function declearation***********************/
extern uint8_t bt_addr[6];
extern bool app_is_ux_mobile(void);

#if BLE_AUDIO_ENABLED
//const char *ble_audio_test_name = "FREDDIE_THE_FOX";
#endif

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            ble_adv_is_allowed
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE advertisment is allowed or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - if advertisement is allowed
 *    flase -  if adverstisement is not allowed
 */
static bool ble_adv_is_allowed(void);

/*---------------------------------------------------------------------------
 *            ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    param - see @BLE_ADV_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_adv(void *param);

/*---------------------------------------------------------------------------
 *            app_ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static bool app_ble_start_adv(void);

/*---------------------------------------------------------------------------
 *            ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan api
 *
 * Parameters:
 *    param - see @BLE_SCAN_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_scan(void);

/*---------------------------------------------------------------------------
 *            ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE connection
 *
 * Parameters:
 *    bleBdAddr - address of BLE device to connect
 *
 * Return:
 *    void
 */
static void ble_start_connect(void);

/*---------------------------------------------------------------------------
 *            app_ble_pending_to_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE pending to connect
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true means ble pending to connect
 */
static bool app_ble_pending_to_connect(void);

/*---------------------------------------------------------------------------
 *            app_ble_index_pending_to_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the index of BLE pending to disconnect
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t -- the index of ble pending to disconnect
 */
static uint8_t app_ble_index_pending_to_disconnect(void);
static void app_ble_set_all_white_list(void);
static bool app_ble_need_set_white_list(void);


/************************private variable defination************************/
static BLE_MODE_ENV_T bleModeEnv;

/****************************function defination****************************/
void app_ble_mode_init(void)
{
    LOG_I("%s", __func__);

    uint8_t actv_user;
    memset(&bleModeEnv, 0, sizeof(bleModeEnv));

    bleModeEnv.bleEnv = &app_env;

    bleModeEnv.advPendingInterval = BLE_ADVERTISING_INTERVAL;
    bleModeEnv.advPendingType = ADV_TYPE_UNDIRECT;

    for (actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        bleModeEnv.advCurrentTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
        bleModeEnv.advPendingTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
    }

    if(NULL == app_ble_update_conn_param_timer_id)
    {
        app_ble_update_conn_param_timer_id = osTimerCreate(osTimer(APP_BLE_UPDATE_CONN_PARAM_TIMEOUT), osTimerOnce, NULL);
    }
    for (uint8_t i=0; i<BLE_CONNECTION_MAX; i++)
    {
        ble_update_timeout_conn_param[i].conidx = 0xFF;
        ble_update_timeout_conn_param[i].errCode = 0x00;
    }
}

static void ble_set_actv_action(BLE_ACTV_ACTION_E actv_action, uint8_t actv_idx)
{
    LOG_I("%s %d -> %d idx 0x%02x", __func__, bleModeEnv.ble_actv_action, actv_action, actv_idx);
    bleModeEnv.ble_op_actv_idx = actv_idx;
    bleModeEnv.ble_actv_action = actv_action;
}

static bool ble_clear_actv_action(BLE_ACTV_ACTION_E actv_action, uint8_t actv_idx)
{
    LOG_I("actv_action now %d clear %d idx 0x%02x", bleModeEnv.ble_actv_action, actv_action, actv_idx);
    if (actv_action == bleModeEnv.ble_actv_action)
    {
        if ((BLE_ACTV_ACTION_STOPPING_ADV == actv_action) &&
            bleModeEnv.ble_op_actv_idx != actv_idx)
        {
            return false;
        }

        LOG_I("%s %d -> %d", __func__, bleModeEnv.ble_actv_action, BLE_ACTV_ACTION_IDLE);
        bleModeEnv.ble_op_actv_idx = 0xFF;
        bleModeEnv.ble_actv_action = BLE_ACTV_ACTION_IDLE;
        return true;
    }

    return false;
}

// ble advertisement used functions
static void ble_adv_config_param(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    uint8_t i = 0;
    POSSIBLY_UNUSED uint8_t avail_space = 0;
    uint16_t adv_user_interval = BLE_ADV_INVALID_INTERVAL;
    uint8_t empty_addr[BTIF_BD_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    BLE_ADV_FILL_PARAM_T *p_ble_param = app_ble_param_get_ctx();

    // reset adv param info
    memset(&bleModeEnv.advParamInfo, 0, sizeof(bleModeEnv.advParamInfo));
    bleModeEnv.adv_user_enable[actv_user] = 0;
    bleModeEnv.advParamInfo.advType = bleModeEnv.advPendingType;
    bleModeEnv.advParamInfo.advTxPwr = app_ble_param_get_adv_tx_power_level(actv_user);
    bleModeEnv.advParamInfo.withFlags = bleModeEnv.withPendingFlag;
    if (memcmp(bleModeEnv.advPendingLocalAddr[actv_user], empty_addr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(bleModeEnv.advParamInfo.localAddr, bleModeEnv.advPendingLocalAddr[actv_user], BTIF_BD_ADDR_SIZE);
    }
    else
    {
        memcpy(bleModeEnv.advParamInfo.localAddr, bt_get_ble_local_address(), BTIF_BD_ADDR_SIZE);
    }

    for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
    {
        bleModeEnv.advParamInfo.advUserInterval[i] = BLE_ADV_INVALID_INTERVAL;
    }

    // scenarios interval
    bleModeEnv.advParamInfo.advInterval = app_ble_param_get_adv_interval(actv_user);

    // fill adv user data
    for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
    {
        BLE_ADV_USER_E user = p_ble_param[actv_user].adv_user[i];
        if (USER_INUSE != user)
        {
            if (bleModeEnv.bleDataFillFunc[user])
            {
                bleModeEnv.bleDataFillFunc[user]((void *)&bleModeEnv.advParamInfo);

                // check if the adv/scan_rsp data length is legal
                if(bleModeEnv.advParamInfo.withFlags)
                {
                    ASSERT(BLE_ADV_DATA_WITH_FLAG_LEN >= bleModeEnv.advParamInfo.advDataLen, "[BLE][ADV]adv data exceed");
                }
                else
                {
                    ASSERT(BLE_ADV_DATA_WITHOUT_FLAG_LEN >= bleModeEnv.advParamInfo.advDataLen, "[BLE][ADV]adv data exceed");
                }
                ASSERT(SCAN_RSP_DATA_LEN >= bleModeEnv.advParamInfo.scanRspDataLen, "[BLE][ADV]scan response data exceed");
            }
        }
    }

    // adv param judge
    if (BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval)
    {
        // adv user interval
        for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
        {
            // adv interval
            adv_user_interval = bleModeEnv.advParamInfo.advUserInterval[i];
            if ((BLE_ADV_INVALID_INTERVAL != adv_user_interval) && 
                ((BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval) ||
                 (adv_user_interval < bleModeEnv.advParamInfo.advInterval)))
            {
                bleModeEnv.advParamInfo.advInterval = adv_user_interval;
            }
        }
    }

    // adv interval judge
    if (BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval)
    {
        bleModeEnv.advParamInfo.advInterval = BLE_ADVERTISING_INTERVAL;
    }

    // adv type judge
    // connectable adv is not allowed if max connection reaches
    if (app_is_arrive_at_max_ble_connections() && 
        (ADV_TYPE_NON_CONN_SCAN != bleModeEnv.advParamInfo.advType) &&
        (ADV_TYPE_NON_CONN_NON_SCAN != bleModeEnv.advParamInfo.advType))
    {
        LOG_W("will change adv type to none-connectable because max ble connection reaches");
        bleModeEnv.advParamInfo.advType = ADV_TYPE_NON_CONN_SCAN;
    }

#ifndef CUSTOMER_DEFINE_ADV_DATA
    // adv with flag
    if(bleModeEnv.advParamInfo.withFlags)
    {
        avail_space = BLE_ADV_DATA_WITH_FLAG_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
    }
    else
    {
        avail_space = BLE_ADV_DATA_WITHOUT_FLAG_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
    }

    // Check if data can be added to the adv Data
    if (avail_space > 2)
    {
        avail_space = co_min(avail_space, bleModeEnv.bleEnv->dev_name_len);
        bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen++] = avail_space + 1;
        // Fill Device Name Flag
        bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen++] =
            (avail_space == bleModeEnv.bleEnv->dev_name_len) ? '\x08' : '\x09';
        // Copy device name
        memcpy(&bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen], bleModeEnv.bleEnv->dev_name, avail_space);
        // Update adv Data Length
        bleModeEnv.advParamInfo.advDataLen += avail_space;
    }
#endif
}

static bool ble_adv_is_allowed(void)
{
    bool allowed_adv = true;
    if (!app_is_stack_ready())
    {
        LOG_I("reason: stack not ready");
        allowed_adv = false;
    }

    if (app_is_power_off_in_progress())
    {
        LOG_I("reason: in power off mode");
        allowed_adv = false;
    }

    if (bleModeEnv.advSwitch)
    {
        LOG_I("adv switched off:%d", bleModeEnv.advSwitch);
        allowed_adv = false;
    }

    if (btapp_hfp_is_sco_active())
    {
        LOG_I("SCO ongoing");
        allowed_adv = false;
    }

    if (app_is_arrive_at_max_ble_connections())
    {
        LOG_I("arrive at max connection");
        allowed_adv = false;
    }

    return allowed_adv;
}

/*---------------------------------------------------------------------------
 *            ble_stop_advertising
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop advertising
 *
 * Parameters:
 *    actv_idx - activity index
 *
 * Return:
 *    void
 */
static void ble_stop_advertising(uint8_t actv_idx)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
    appm_stop_advertising(actv_idx);
}

static void ble_start_adv(void *param)
{
    BLE_ADV_PARAM_T* pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_user = pAdvParam->adv_actv_user;
    BLE_ADV_PARAM_T *pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
    if (!ble_adv_is_allowed())
    {
        LOG_I("[ADV] not allowed.");
        ble_stop_advertising(bleModeEnv.bleEnv->adv_actv_idx[pAdvParam->adv_actv_user]);
        return;
    }

    memcpy(pCurrentAdvParam, param, sizeof(BLE_ADV_PARAM_T));
    ble_set_actv_action(BLE_ACTV_ACTION_STARTING_ADV, 0xFF);
    appm_start_advertising(pCurrentAdvParam);
}

static void ble_start_scan(void)
{
    if (INVALID_BLE_ACTIVITY_INDEX != bleModeEnv.bleEnv->scan_actv_idx &&
        APP_SCAN_STATE_STARTED == bleModeEnv.bleEnv->state[bleModeEnv.bleEnv->scan_actv_idx] &&
        !memcmp(&bleModeEnv.scanCurrentInfo, &bleModeEnv.scanPendingInfo, sizeof(bleModeEnv.scanCurrentInfo)))
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
        LOG_I("reason: scan param not changed");
        LOG_I("[SCAN] not allowed.");
        return;
    }
    memcpy(&bleModeEnv.scanCurrentInfo, &bleModeEnv.scanPendingInfo, sizeof(BLE_SCAN_PARAM_T));
    ble_set_actv_action(BLE_ACTV_ACTION_STARTING_SCAN, 0xFF);
    appm_start_scanning(bleModeEnv.scanCurrentInfo.scanInterval,
                        bleModeEnv.scanCurrentInfo.scanWindow,
                        bleModeEnv.scanCurrentInfo.scanType);
}

/*---------------------------------------------------------------------------
 *            ble_stop_scanning
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop BLE scanning
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_scanning(void)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOPPING_SCAN, 0xFF);
    appm_stop_scanning();
}

static void ble_start_connect(void)
{
    uint8_t index = 0;
    BLE_CONN_PARAM_T conn_param;
    memset(&conn_param, 0, sizeof(BLE_CONN_PARAM_T));
    conn_param.gapm_init_type = bleModeEnv.bleToConnectInfo[index].gapm_init_type;
    conn_param.conn_to = bleModeEnv.bleToConnectInfo[index].conn_to;

    for (index=0; index<BLE_CONNECTION_MAX; index++)
    {
        if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            break;
        }
    }
    ASSERT(index<BLE_CONNECTION_MAX, "%s has no ble to connect, index %d", __func__, index);

    bleModeEnv.bleToConnectInfo[index].pendingConnect = false;
    memcpy(conn_param.peer_addr.addr, bleModeEnv.bleToConnectInfo[index].addrToConnect, BTIF_BD_ADDR_SIZE);

    LOG_I("%s", __func__);
    ble_set_actv_action(BLE_ACTV_ACTION_CONNECTING, 0xFF);
    appm_start_connecting(&conn_param);
}

/*---------------------------------------------------------------------------
 *            ble_stop_connecting
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop BLE connecting
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_connecting(void)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOP_CONNECTING, 0xFF);
    appm_stop_connecting();
}

/*---------------------------------------------------------------------------
 *            ble_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE disconnect
 *
 * Parameters:
 *    idx - connection index
 *
 * Return:
 *    void
 */
static void ble_disconnect(uint8_t idx)
{
    ble_set_actv_action(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);
    appm_disconnect(idx);
}

void ble_execute_pending_op(BLE_ACTV_ACTION_E finish_action, uint8_t actv_idx)
{
    LOG_I("%s finish_action %d actv_action %d", __func__, finish_action, bleModeEnv.ble_actv_action);
    uint8_t index_of_pending_disconnect = app_ble_index_pending_to_disconnect();
    if (false == ble_clear_actv_action(finish_action, actv_idx))
    {
        LOG_I("can't execute pending op");
        return;
    }

    if (bleModeEnv.pending_stop_connecting)
    {
        bleModeEnv.pending_stop_connecting = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)ble_stop_connecting);
    }
    else if (index_of_pending_disconnect != BLE_CONNECTION_MAX)
    {
        bleModeEnv.pendingDisconnect[index_of_pending_disconnect] = false;
        app_bt_start_custom_function_in_bt_thread((uint32_t)index_of_pending_disconnect,
                                                  0,
                                                  (uint32_t)ble_disconnect);
    }
    else if(app_ble_need_set_white_list())
    {
        app_ble_set_all_white_list();
    }
    else if (app_ble_pending_to_connect())
    {
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)ble_start_connect);
    }
    else if (bleModeEnv.scan_has_pending_op)
    {
        bleModeEnv.scan_has_pending_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)ble_start_scan);
    }
    else if (bleModeEnv.scan_has_pending_stop_op)
    {
        bleModeEnv.scan_has_pending_stop_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)ble_stop_scanning);
    }
    else if (bleModeEnv.adv_has_pending_op)
    {
        bleModeEnv.adv_has_pending_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)app_ble_start_adv);
    }
    else
    {
        bleModeEnv.ble_is_busy = false;
    }
}

static void app_ble_update_param_timeout_handle(void)
{
    uint8_t conidx = 0xFF;
    uint8_t errCode = 0x00;

    for (uint8_t i=0; i < BLE_CONNECTION_MAX; i++)
    {
        if (0xFF != ble_update_timeout_conn_param[i].conidx)
        {
            conidx = ble_update_timeout_conn_param[i].conidx;
            errCode = ble_update_timeout_conn_param[i].errCode;
            LOG_I("%s conidx 0x%02x errCode 0x%02x", __func__, conidx, errCode);

            //here maybe need re-update ble connect param
            APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);
            l2cap_update_param(conidx, pContext->connPendindParam.con_interval,
                                pContext->connPendindParam.con_interval,
                                pContext->connPendindParam.sup_to,
                                pContext->connPendindParam.con_latency);

            ble_update_timeout_conn_param[i].conidx = 0xFF;
            ble_update_timeout_conn_param[i].errCode = 0x00;
        }
    }
}

static void app_ble_update_conn_param_timeout_cb(void const *n)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              (uint32_t)app_ble_update_param_timeout_handle);
}

void app_ble_update_param_failed(uint8_t conidx, uint8_t errCode)
{
    LOG_I("%s conidx 0x%02x errCode 0x%02x", __func__, conidx, errCode);

    if (app_env.context[conidx].conn_param_update_times++ > APP_BLE_UPDATE_CONN_PARAM_MAX_TIMES)
    {
        LOG_I("param updated too much times %d", app_env.context[conidx].conn_param_update_times);
        app_env.context[conidx].conn_param_update_times = 0;

        ble_event_t event;
        event.evt_type = BLE_CONN_PARAM_UPDATE_FAILED_EVENT;
        event.p.conn_param_update_failed_handled.conidx = conidx;
        event.p.conn_param_update_failed_handled.err_code = errCode;
        app_ble_core_global_handle(&event, NULL);
        return;
    }

    for (uint8_t i=0; i<BLE_CONNECTION_MAX; i++)
    {
        if (ble_update_timeout_conn_param[i].conidx == 0xFF || ble_update_timeout_conn_param[i].conidx == conidx)
        {
            ble_update_timeout_conn_param[i].conidx = conidx;
            ble_update_timeout_conn_param[i].errCode = errCode;
            break;
        }
    }

    osTimerStop(app_ble_update_conn_param_timer_id);
    osTimerStart(app_ble_update_conn_param_timer_id, APP_BLE_UPDATE_CONN_PARAM_TIMEOUT_INTERVEL);
}

void app_ble_update_param_successful(uint8_t conidx, APP_BLE_CONN_PARAM_T* pConnParam)
{
    LOG_I("%s conidx 0x%02x", __func__, conidx);

    bool need_stop_timer = true;

    app_env.context[conidx].conn_param_update_times = 0;

    for (uint8_t i=0; i<BLE_CONNECTION_MAX; i++)
    {
        if (ble_update_timeout_conn_param[i].conidx != 0xFF)
        {
            if (ble_update_timeout_conn_param[i].conidx == conidx)
            {
                ble_update_timeout_conn_param[i].conidx = 0xFF;
                ble_update_timeout_conn_param[i].errCode = 0x00;
            }
            else
            {
                need_stop_timer = false;
            }
        }
    }

    if (need_stop_timer)
    {
        osTimerStop(app_ble_update_conn_param_timer_id);
    }

    ble_event_t event;
    event.evt_type = BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT;
    event.p.conn_param_update_successful_handled.conidx = conidx;
    event.p.conn_param_update_successful_handled.con_interval = pConnParam->con_interval;
    event.p.conn_param_update_successful_handled.con_latency = pConnParam->con_latency;
    event.p.conn_param_update_successful_handled.sup_to = pConnParam->sup_to;
    app_ble_core_global_handle(&event, NULL);
}

/**
 * @brief : callback function of BLE connect failed
 *
 */
static void app_ble_connecting_failed_handler(uint8_t actv_idx, uint8_t err_code)
{
    LOG_I("%s", __func__);

    ble_event_t event;
    event.evt_type = BLE_CONNECTING_FAILED_EVENT;
    event.p.connecting_failed_handled.actv_idx = actv_idx;
    event.p.connecting_failed_handled.err_code = err_code;
    app_ble_core_global_handle(&event, NULL);
}

void app_connecting_failed(uint8_t actv_idx, uint8_t err_code)
{
    app_bt_start_custom_function_in_bt_thread(actv_idx,
                                              err_code,
                                              (uint32_t)app_ble_connecting_failed_handler);
}

void app_ble_on_bond_success(uint8_t conidx)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_CONNECT_BOND_EVENT;
    event.p.connect_bond_handled.conidx = conidx;
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_connected_evt_handler(uint8_t conidx, const uint8_t *pPeerBdAddress)
{
    ble_event_t event;

    event.evt_type = BLE_CONNECT_EVENT;
    event.p.connect_handled.conidx = conidx;
    event.p.connect_handled.peer_bdaddr = pPeerBdAddress;
    app_ble_core_global_handle(&event, NULL);

    app_stop_fast_connectable_ble_adv_timer();
    //app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);

#ifdef AOB_UX_ENABLED
    if (app_is_ux_mobile())
    {
        #if BLE_AUDIO_ENABLED
        app_bapc_uc_start(conidx);
        //app_arc_micc_start(conidx);
        #endif

        if (bleModeEnv.bleEnv->conn_cnt < BLE_AUDIO_CONNECTION_CNT)
        {
            /// start scan if connection count doesn't match the requirement
            app_ble_start_scan(BLE_DEFAULT_SCAN_POLICY, 10, 50);
        }
    }
#endif
}

void app_ble_disconnected_evt_handler(uint8_t conidx, uint8_t errCode)
{
    ble_event_t event;
    ble_execute_pending_op(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);

    event.evt_type = BLE_DISCONNECT_EVENT;
    event.p.disconnect_handled.conidx = conidx;
    event.p.disconnect_handled.errCode = errCode;
    app_ble_core_global_handle(&event, NULL);

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);

#ifdef AOB_UX_ENABLED
    if (app_is_ux_mobile())
    {
        app_ble_start_scan(BLE_DEFAULT_SCAN_POLICY, 10, 50);
    }
#endif
}

void app_connecting_stopped(gap_bdaddr_t *peer_addr)
{
    LOG_I("%s peer addr:", __func__);
    DUMP8("%2x ", peer_addr->addr, BT_ADDR_OUTPUT_PRINT_NUM);

    ble_event_t event;

    event.evt_type = BLE_CONNECTING_STOPPED_EVENT;
    event.p.stopped_connecting_handled.peer_bdaddr = peer_addr->addr;
    app_ble_core_global_handle(&event, NULL);
}

void app_advertising_started(uint8_t actv_idx)
{
    LOG_I("%s", __func__);

    uint8_t actv_user;
    for (actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        if (actv_idx == bleModeEnv.bleEnv->adv_actv_idx[actv_user])
        {
            if (bleModeEnv.advCurrentTxpwr[actv_user] != bleModeEnv.advPendingTxpwr[actv_user])
            {
                app_ble_set_adv_txpwr_by_actv_user(actv_user, bleModeEnv.advPendingTxpwr[actv_user]);
            }
            break;
        }
    }

    ble_event_t event;

    event.evt_type = BLE_ADV_STARTED_EVENT;
    event.p.adv_started_handled.actv_idx = actv_idx;
    app_ble_core_global_handle(&event, NULL);

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_advertising_stopped(uint8_t actv_idx)
{
    LOG_I("%s", __func__);

    uint8_t actv_user;
    for (actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        if (actv_idx == bleModeEnv.bleEnv->adv_actv_idx[actv_user])
        {
            bleModeEnv.advCurrentTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
            break;
        }
    }

    ble_event_t event;

    event.evt_type = BLE_ADV_STOPPED_EVENT;
    event.p.adv_stopped_handled.actv_idx = actv_idx;
    app_ble_core_global_handle(&event, NULL);

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

static void app_advertising_starting_failed_handle(uint8_t actv_idx, uint8_t err_code)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_ADV_STARTING_FAILED_EVENT;
    event.p.adv_starting_failed_handled.actv_idx = actv_idx;
    event.p.adv_starting_failed_handled.err_code = err_code;
    app_ble_core_global_handle(&event, NULL);
}

void app_advertising_starting_failed(uint8_t actv_idx, uint8_t err_code)
{
    app_bt_start_custom_function_in_bt_thread(actv_idx,
                                              err_code,
                                              (uint32_t)app_advertising_starting_failed_handle);
}

// BLE APIs for external use
void app_ble_data_fill_enable(BLE_ADV_USER_E user, bool enable)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s user %d", __func__, user);
    LOG_I("%s user %d%s enable %d", __func__, user, ble_adv_user2str(user), enable);

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    if (enable)
    {
        bleModeEnv.adv_user_enable[actv_user] |= (1 << user);
    }
    else
    {
        bleModeEnv.adv_user_enable[actv_user] &= ~(1 << user);
    }
}


// BLE APIs for external use
void app_ble_register_data_fill_handle(BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable)
{
    LOG_I("%s user %d", __func__, user);
    if (BLE_ADV_USER_NUM <= user)
    {
        LOG_W("invalid user");
    }
    else
    {
        if (func != bleModeEnv.bleDataFillFunc[user] &&
            NULL != func)
        {
            bleModeEnv.bleDataFillFunc[user] = func;
        }
    }

    bleModeEnv.adv_user_register |= (1 << user);
}

void app_ble_system_ready(void)
{

#if defined(ENHANCED_STACK)
    app_notify_stack_ready(STACK_READY_BLE);
#else
    app_notify_stack_ready(STACK_READY_BLE | STACK_READY_BT);
#endif
}

static void ble_adv_refreshing(void *param)
{
    BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_user = pAdvParam->adv_actv_user;
    uint8_t actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
    BLE_ADV_PARAM_T *pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
    // four conditions that we just need to update the ble adv data instead of restarting ble adv
    // 1. BLE advertising is on
    // 2. No on-going BLE operation
    // 3. BLE adv type is the same
    // 4. BLE adv interval is the same
    // TODO: to be re-enabled when appm_update_adv_data is ready
    if ((APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]) && \
        pCurrentAdvParam->advType == pAdvParam->advType && \
        pCurrentAdvParam->advInterval == pAdvParam->advInterval && \
        pCurrentAdvParam->advTxPwr == pAdvParam->advTxPwr && \
        pCurrentAdvParam->filter_pol == pAdvParam->filter_pol && \
        !memcmp(pCurrentAdvParam->localAddr, pAdvParam->localAddr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(pCurrentAdvParam, param, sizeof(BLE_ADV_PARAM_T));
        ble_set_actv_action(BLE_ACTV_ACTION_STARTING_ADV, 0xFF);
        appm_update_adv_data(pCurrentAdvParam);
    }
    else
    {
        // otherwise, restart ble adv
        ble_start_adv(param);
    }
}

static bool app_ble_adv_param_is_different(BLE_ADV_PARAM_T *p_cur_adv, BLE_ADV_PARAM_T *p_dst_adv)
{
    uint8_t ret = 0;

    if ((p_cur_adv->withFlags != p_dst_adv->withFlags) ||
        (p_cur_adv->filter_pol != p_dst_adv->filter_pol) ||
        (p_cur_adv->advType != p_dst_adv->advType) ||
        (p_cur_adv->advInterval != p_dst_adv->advInterval) ||
        (p_cur_adv->advTxPwr != p_dst_adv->advTxPwr) ||
        (p_cur_adv->advDataLen != p_dst_adv->advDataLen) ||
        (p_cur_adv->scanRspDataLen != p_dst_adv->scanRspDataLen))
    {
        ret = 1;
    }
    else if (memcmp(p_cur_adv->advData, p_dst_adv->advData, ADV_DATA_LEN))
    {
        ret = 2;
    }
    else if (memcmp(p_cur_adv->scanRspData, p_dst_adv->scanRspData, ADV_DATA_LEN))
    {
        ret = 3;
    }
    else if (((p_cur_adv->advType == ADV_TYPE_DIRECT_LDC) ||
              (p_cur_adv->advType == ADV_TYPE_DIRECT_HDC)) &&
             (memcmp((uint8_t *)&p_cur_adv->peerAddr, (uint8_t *)&p_dst_adv->peerAddr, sizeof(ble_bdaddr_t))))
    {
        ret = 4;
    }
    else if (memcmp(p_cur_adv->localAddr, p_dst_adv->localAddr, BTIF_BD_ADDR_SIZE))
    {
        ret = 5;
    }

    if (ret)
    {
        LOG_I("%s ret %d", __func__, ret);
        return true;
    }

    return false;
}

static bool app_ble_start_adv(void)
{
    uint32_t adv_user_enable = 0;
    uint8_t actv_idx = 0xFF;
    BLE_ADV_PARAM_T *pCurrentAdvParam;
    BLE_ADV_PARAM_T *pAdvParamInfo;
    //LOG_I("[ADV] interval:%d ca:%p", bleModeEnv.advPendingInterval, __builtin_return_address(0));

    if (!ble_adv_is_allowed())
    {
        LOG_I("[ADV] not allowed.");
        for (uint8_t actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
            if (INVALID_BLE_ACTIVITY_INDEX != actv_idx)
            {
                ble_stop_advertising(actv_idx);
                return false;
            }
        }
        ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
        return false;
    }

    for (uint8_t actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
        adv_user_enable = bleModeEnv.adv_user_enable[actv_user];
        ble_adv_config_param(actv_user);

        LOG_I("%s old_user_enable 0x%x new 0x%x %d %d", __func__, adv_user_enable, bleModeEnv.adv_user_enable[actv_user],
                                                            actv_user, BLE_ADV_ACTIVITY_USER_NUM);
        if (!bleModeEnv.adv_user_enable[actv_user])
        {
            LOG_I("no adv user enable");

            if ((INVALID_BLE_ACTIVITY_INDEX != actv_idx) &&
                (APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]))
            {
                ble_stop_advertising(actv_idx);
                return false;
            }
            continue;
        }

        if (app_ble_is_white_list_enable())
        {
            LOG_I("white list mode");
            bleModeEnv.advParamInfo.filter_pol = ADV_ALLOW_SCAN_WLST_CON_WLST;
        }
        else
        {
            bleModeEnv.advParamInfo.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        }

        pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
        pAdvParamInfo = &bleModeEnv.advParamInfo;
        pAdvParamInfo->adv_actv_user = actv_user;

        // param of adv request is exactly same as current adv
        if ((INVALID_BLE_ACTIVITY_INDEX != actv_idx) &&
            (APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]) &&
            (!app_ble_adv_param_is_different(pCurrentAdvParam, pAdvParamInfo)))
        {
            LOG_I("adv param not changed, do nothing");
            //ble_execute_pending_op(BLE_ACTV_ACTION_ADV);
            //continue;
        }
        else
        {
            LOG_I("[ADV_LEN] %d [DATA]:", bleModeEnv.advParamInfo.advDataLen);
            DUMP8("%02x ", bleModeEnv.advParamInfo.advData, bleModeEnv.advParamInfo.advDataLen);
            LOG_I("[SCAN_RSP_LEN] %d [DATA]:", bleModeEnv.advParamInfo.scanRspDataLen);
            DUMP8("%02x ", bleModeEnv.advParamInfo.scanRspData, bleModeEnv.advParamInfo.scanRspDataLen);

            ble_adv_refreshing(pAdvParamInfo);
            return true;
        }
    }

    ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
    return true;
}

void app_ble_start_connectable_adv(uint16_t advInterval)
{
    LOG_D("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    bleModeEnv.withPendingFlag = true;

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.adv_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)app_ble_start_adv);
    }
}

void app_ble_refresh_adv_state(uint16_t advInterval)
{
    LOG_D("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    bleModeEnv.withPendingFlag = true;

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.adv_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)app_ble_start_adv);
    }
}

void app_ble_set_adv_type(BLE_ADV_TYPE_E advType, ble_bdaddr_t *peer_addr)
{
    LOG_I("%s %d -> %d", __func__, bleModeEnv.advPendingType, advType);
    bleModeEnv.advPendingType = advType;
    if ((advType == ADV_TYPE_DIRECT_LDC) ||
        (advType == ADV_TYPE_DIRECT_HDC))
    {
        memcpy((uint8_t *)&bleModeEnv.advPendingPeerAddr, (uint8_t *)peer_addr, sizeof(ble_bdaddr_t));
    }

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_ble_set_adv_txpwr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, int8_t txpwr_dbm)
{
    ASSERT(actv_user < BLE_ADV_ACTIVITY_USER_NUM, "%s user %d", __func__, actv_user);

    uint8_t adv_hdl = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
    uint8_t txpwr_level = app_ble_param_get_adv_tx_power_level(actv_user);
    LOG_I("%s %d hdl 0x%02x level %d dbm %d", __func__, actv_user, adv_hdl, txpwr_level, txpwr_dbm);

    bleModeEnv.advPendingTxpwr[actv_user] = txpwr_dbm;
    if (INVALID_BLE_ACTIVITY_INDEX != adv_hdl && 
        txpwr_dbm != bleModeEnv.advCurrentTxpwr[actv_user])
    {
        bleModeEnv.advCurrentTxpwr[actv_user] = txpwr_dbm;
        bt_drv_ble_adv_txpwr_via_advhdl(adv_hdl, txpwr_level, txpwr_dbm);
    }
}

void app_ble_set_adv_txpwr_by_adv_user(BLE_ADV_USER_E user, int8_t txpwr_dbm)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s %d", __func__, user);
    LOG_I("%s %d%s", __func__, user, ble_adv_user2str(user));

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    app_ble_set_adv_txpwr_by_actv_user(actv_user, txpwr_dbm);
}

void app_ble_set_all_adv_txpwr(int8_t txpwr_dbm)
{
    uint8_t actv_user;
    for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        app_ble_set_adv_txpwr_by_actv_user(actv_user, txpwr_dbm);
    }
}

void app_ble_set_adv_local_addr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, uint8_t *addr)
{
    ASSERT(actv_user < BLE_ADV_ACTIVITY_USER_NUM, "%s user %d", __func__, actv_user);
    LOG_I("%s %d", __func__, actv_user);
    DUMP8("0x%02x ", addr, BT_ADDR_OUTPUT_PRINT_NUM);

    memcpy((uint8_t *)&bleModeEnv.advPendingLocalAddr[actv_user][0], addr, BTIF_BD_ADDR_SIZE);
}

void app_ble_set_adv_local_addr_by_adv_user(BLE_ADV_USER_E user, uint8_t *addr)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s %d", __func__, user);
    LOG_I("%s %d%s", __func__, user, ble_adv_user2str(user));

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    app_ble_set_adv_local_addr_by_actv_user(actv_user, addr);
}

void app_ble_start_scan(enum BLE_SCAN_FILTER_POLICY scanFilterPolicy, uint16_t scanWindow, uint16_t scanInterval)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    bleModeEnv.scanPendingInfo.scanWindow = scanWindow;
    bleModeEnv.scanPendingInfo.scanInterval = scanInterval;
    bleModeEnv.scanPendingInfo.scanType = scanFilterPolicy; //BLE_SCAN_ALLOW_ADV_WLST

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.scan_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_start_scan);
    }
}

void app_ble_stop_scan(void)
{
    LOG_D("%s", __func__);
    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.scan_has_pending_stop_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_stop_scanning);
    }
}

void app_scanning_started(void)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_SCAN_STARTED_EVENT;
    app_ble_core_global_handle(&event, NULL);
}

void app_scanning_stopped(void)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_SCAN_STOPPED_EVENT;
    app_ble_core_global_handle(&event, NULL);
}

/**
 * @brief : callback function of BLE scan starting failed
 *
 */
static void app_ble_scanning_starting_failed_handler(uint8_t actv_idx, uint8_t err_code)
{
    LOG_I("%s", __func__);

    ble_event_t event;
    event.evt_type = BLE_SCAN_STARTING_FAILED_EVENT;
    event.p.scan_starting_failed_handled.actv_idx = actv_idx;
    event.p.scan_starting_failed_handled.err_code = err_code;
    app_ble_core_global_handle(&event, NULL);
}

void app_scanning_starting_failed(uint8_t actv_idx, uint8_t err_code)
{
    app_bt_start_custom_function_in_bt_thread(actv_idx,
                                              err_code,
                                              (uint32_t)app_ble_scanning_starting_failed_handler);
}

static bool app_ble_pending_to_connect(void)
{
    uint8_t index = 0;
    for (index=0; index<BLE_CONNECTION_MAX; index++)
    {
        if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            return true;
        }
    }

    return false;
}

void app_ble_start_connect(uint8_t *addr)
{
    app_ble_start_connect_with_init_type(addr, APP_BLE_INIT_TYPE_DIRECT_CONN_EST, 0);
}

void app_ble_start_connect_with_init_type(uint8_t *addr, BLE_INIT_TYPE_E init_type, uint16_t conn_to)
{
    uint8_t index = 0;

    if (APP_BLE_INIT_TYPE_DIRECT_CONN_EST == init_type)
    {
        for (index=0; index<BLE_CONNECTION_MAX; index++)
        {
            if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
            {
                if (!memcmp(bleModeEnv.bleToConnectInfo[index].addrToConnect, addr, BTIF_BD_ADDR_SIZE))
                {
                    TRACE(0, "%s already record", __func__);
                    return;
                }
            }
        }
    }

    for (index=0; index<BLE_CONNECTION_MAX; index++)
    {
        if (!bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            bleModeEnv.bleToConnectInfo[index].pendingConnect = true;
            if (APP_BLE_INIT_TYPE_DIRECT_CONN_EST == init_type)
            {
                memcpy(bleModeEnv.bleToConnectInfo[index].addrToConnect, addr, BTIF_BD_ADDR_SIZE);
            }
            bleModeEnv.bleToConnectInfo[index].gapm_init_type = init_type;
            bleModeEnv.bleToConnectInfo[index].conn_to = conn_to;
            break;
        }
    }
    ASSERT(index<BLE_CONNECTION_MAX, "%s has too many ble to connect, index %d", __func__, index);

    if (!bleModeEnv.ble_is_busy)
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_start_connect);
    }
}

void app_ble_cancel_connecting(void)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.pending_stop_connecting = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
            (uint32_t)ble_stop_connecting);
    }
}

bool app_ble_is_connection_on(uint8_t index)
{
    return app_ble_is_connection_on_by_index(index);
}

bool app_ble_is_any_connection_exist(void)
{
    bool ret = false;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (app_ble_is_connection_on_by_index(i))
        {
            ret = true;
        }
    }

    return ret;
}

static uint8_t app_ble_index_pending_to_disconnect(void)
{
    uint8_t index = 0;
    for (index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (bleModeEnv.pendingDisconnect[index])
        {
            return index;
        }
    }

    return BLE_CONNECTION_MAX;
}

void app_ble_start_disconnect(uint8_t conIdx)
{
    LOG_I("%s conidx %d", __func__, conIdx);

    if (!bleModeEnv.ble_is_busy)
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread((uint32_t)conIdx,
                                                  0,
                                                  (uint32_t)ble_disconnect);
    }
    else
    {
        bleModeEnv.pendingDisconnect[conIdx] = true;
    }
}

void app_ble_disconnect_all(void)
{
    uint8_t index = 0;
    for (index=0; index<BLE_CONNECTION_MAX; index++)
    {
        bleModeEnv.bleToConnectInfo[index].pendingConnect = false;
        app_ble_start_disconnect(index);
    }
}

void app_ble_force_switch_adv(enum BLE_ADV_SWITCH_USER_E user, bool onOff)
{
    ASSERT(user < BLE_SWITCH_USER_NUM, "ble switch user exceed");

    if (onOff)
    {
        bleModeEnv.advSwitch &= ~(1 << user);
    }
    else if ((bleModeEnv.advSwitch & (1 << user)) == 0)
    {
        bleModeEnv.advSwitch |= (1 << user);

        // disconnect all of the BLE connections if box is closed
        if (BLE_SWITCH_USER_BOX == user)
        {
            app_ble_disconnect_all();
        }
    }

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    LOG_I("%s user %d onoff %d switch 0x%x", __func__, user, onOff, bleModeEnv.advSwitch);
}

bool app_ble_is_in_advertising_state(void)
{
    uint8_t actv_idx = 0xFF;

    for (uint8_t i=0; i<BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        actv_idx = app_env.adv_actv_idx[i];
        if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx])
        {
            return true;
        }
    }

    return false;
}

uint32_t POSSIBLY_UNUSED ble_get_manufacture_data_ptr(uint8_t *advData,
                                                             uint32_t dataLength,
                                                             uint8_t *manufactureData)
{
    uint8_t followingDataLengthOfSection;
    uint8_t rawContentDataLengthOfSection;
    uint8_t flag;
    while (dataLength > 0)
    {
        followingDataLengthOfSection = *advData++;
        dataLength--;
        if (dataLength < followingDataLengthOfSection)
        {
            return 0; // wrong adv data format
        }

        if (followingDataLengthOfSection > 0)
        {
            flag = *advData++;
            dataLength--;

            rawContentDataLengthOfSection = followingDataLengthOfSection - 1;
            if (BLE_ADV_MANU_FLAG == flag)
            {
                uint32_t lengthToCopy;
                if (dataLength < rawContentDataLengthOfSection)
                {
                    lengthToCopy = dataLength;
                }
                else
                {
                    lengthToCopy = rawContentDataLengthOfSection;
                }

                memcpy(manufactureData, advData - 2, lengthToCopy + 2);
                return lengthToCopy + 2;
            }
            else
            {
                advData += rawContentDataLengthOfSection;
                dataLength -= rawContentDataLengthOfSection;
            }
        }
    }

    return 0;
}

__attribute__((unused)) static void _ble_audio_handle_adv_data(struct gapm_ext_adv_report_ind *ptInd)
{
#if BLE_AUDIO_ENABLED
    BLE_ADV_DATA_T *particle;

    for (uint8_t offset = 0; offset < ptInd->length;)
    {
        /// get the particle of adv data
        particle = (BLE_ADV_DATA_T *)(ptInd->data + offset);

        if ((GAP_AD_TYPE_SHORTENED_NAME == particle->type) &&
            (!memcmp(particle->value, app_env.dev_name, particle->length-1)))
        {
            LOG_I("Found the fox!");
            app_ble_stop_scan();
            app_ble_start_connect(ptInd->trans_addr.addr);
            break;
        }
        else
        {
            offset += (ADV_PARTICLE_HEADER_LEN + particle->length);
        }
    }
#else
    uint8_t _master_ble_addr[BLE_ADDR_SIZE];
    uint8_t _slave_ble_addr[BLE_ADDR_SIZE];

    memcpy(_master_ble_addr, ble_addr, BLE_ADDR_SIZE);
    memcpy(_slave_ble_addr, ble_addr, BLE_ADDR_SIZE);
    _master_ble_addr[0] -= 1;
    _slave_ble_addr[0] -= 2;

    if (!memcmp(_master_ble_addr, ptInd->trans_addr.addr, BLE_ADDR_SIZE) ||
        !memcmp(_slave_ble_addr, ptInd->trans_addr.addr, BLE_ADDR_SIZE))
    {
        LOG_I("Found the earphone!");
        app_ble_stop_scan();
        app_ble_start_connect(ptInd->trans_addr.addr);
    }
#endif
}

//received adv data
void app_adv_reported_scanned(struct gapm_ext_adv_report_ind *ptInd)
{
    //LOG_I("Scanned adv len %d data:", ptInd->length);
    //DUMP8("0x%02x ", ptInd->data, ptInd->length);
#ifdef AOB_UX_ENABLED
    if (app_is_ux_mobile())
    {
        _ble_audio_handle_adv_data(ptInd);
    }
    else
#endif
    {
        ble_adv_data_parse(ptInd->trans_addr.addr,
                           (int8_t)ptInd->rssi,
                           ptInd->data,
                           (unsigned char)ptInd->length);

        ble_event_t event;
        event.evt_type = BLE_SCAN_DATA_REPORT_EVENT;
        memcpy(&event.p.scan_data_report_handled.trans_addr, &ptInd->trans_addr, sizeof(ble_bdaddr_t));
        event.p.scan_data_report_handled.rssi = ptInd->rssi;
        event.p.scan_data_report_handled.length = (ptInd->length>ADV_DATA_LEN)?ADV_DATA_LEN:ptInd->length;
        memcpy(event.p.scan_data_report_handled.data, ptInd->data, event.p.scan_data_report_handled.length);
        app_ble_core_global_handle(&event, NULL);
    }
}

void app_ibrt_ui_disconnect_ble(void)
{
    app_ble_disconnect_all();
}

uint32_t app_ble_get_user_register(void)
{
    return bleModeEnv.adv_user_register;
}

void app_ble_get_runtime_adv_param(uint8_t *pAdvType, uint16_t *pAdvIntervalMs)
{
    *pAdvType = bleModeEnv.advParamInfo.advType;
    *pAdvIntervalMs = bleModeEnv.advParamInfo.advInterval;
}

void app_ble_set_white_list_complete(void)
{
    LOG_I("%s", __func__);

    ble_execute_pending_op(BLE_ACTV_ACTION_SET_WHITE_LIST, 0xFF);
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);

#if BLE_AUDIO_ENABLED
    if (app_is_ux_mobile())
    {
        app_ble_start_connect_with_init_type(NULL,
                              APP_BLE_INIT_TYPE_AUTO_CONN_EST,
                              0);
    }
#endif
}

#if BLE_AUDIO_ENABLED
void mobile_connect_earphone(void)
{
    LOG_I("%s", __func__);
    gap_bdaddr_t connect_addr[2];

    //master addr
    connect_addr[0].addr_type = GAPM_STATIC_ADDR;
    for (uint8_t i=0; i<6; i++)
    {
        connect_addr[0].addr[i] = ble_addr[i] + 1;
    }

    //slave addr
    connect_addr[1].addr_type = GAPM_STATIC_ADDR;
    for (uint8_t i=0; i<6; i++)
    {
        connect_addr[1].addr[i] = ble_addr[i] + 1;
    }
    connect_addr[1].addr[0] = ble_addr[0];
    appm_set_white_list(&connect_addr[0], 2);
}
#endif

static void _app_ble_set_all_white_list(void)
{
    LOG_I("%s", __func__);
    uint8_t white_list_num = 0;
    uint8_t size = 0;
    ble_bdaddr_t bdaddr[8];

    for (uint8_t i=0; i<BLE_WHITE_LIST_USER_NUM; i++)
    {
        bleModeEnv.ble_white_list[i].pending_set_white_list = false;
        if (bleModeEnv.ble_white_list[i].enable)
        {
            size = bleModeEnv.ble_white_list[i].size;
            memcpy(&bdaddr[white_list_num], &bleModeEnv.ble_white_list[i].bdaddr[0], sizeof(ble_bdaddr_t)*size);
            white_list_num += size;
        }
    }
    ble_set_actv_action(BLE_ACTV_ACTION_SET_WHITE_LIST, 0xFF);
    appm_set_white_list((gap_bdaddr_t *)&bdaddr[0], white_list_num);
}

static void app_ble_set_all_white_list(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              (uint32_t)_app_ble_set_all_white_list);
}

static bool app_ble_need_set_white_list(void)
{
    for (uint8_t i=0; i<BLE_WHITE_LIST_USER_NUM; i++)
    {
        if (bleModeEnv.ble_white_list[i].pending_set_white_list)
        {
            return true;
        }
    }

    return false;
}

bool app_ble_is_white_list_enable(void)
{
    for (uint8_t i=0; i<BLE_WHITE_LIST_USER_NUM; i++)
    {
        if (bleModeEnv.ble_white_list[i].enable)
        {
            return true;
        }
    }

    return false;
}

void app_ble_set_white_list(BLE_WHITE_LIST_USER_E user, uint8_t *bdaddr, uint8_t size)
{
    LOG_I("%s user %d size %d", __func__, user, size);
    if (size > WHITE_LIST_MAX_NUM)
    {
        size = WHITE_LIST_MAX_NUM;
    }
    bleModeEnv.ble_white_list[user].enable = true;
    bleModeEnv.ble_white_list[user].size = size;

    for (uint8_t index=0; index<size; index++)
    {
        bleModeEnv.ble_white_list[user].bdaddr[index].addr_type = GAPM_STATIC_ADDR;
        memcpy(bleModeEnv.ble_white_list[user].bdaddr[index].addr, (bdaddr+index*6), 6);
    }

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.ble_white_list[user].pending_set_white_list = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_ble_set_all_white_list();
    }
}

void app_ble_clear_white_list(BLE_WHITE_LIST_USER_E user)
{
    LOG_I("%s user %d", __func__, user);

    memset(&bleModeEnv.ble_white_list[user], 0, sizeof(BLE_WHITE_LIST_PARAM_T));
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_ble_clear_all_white_list(void)
{
    LOG_I("%s", __func__);

    for (uint8_t i=0; i<BLE_WHITE_LIST_USER_NUM; i++)
    {
        app_ble_clear_white_list(i);
    }
}

void app_ble_refresh_irk(void)
{
    appm_refresh_ble_irk();
}

