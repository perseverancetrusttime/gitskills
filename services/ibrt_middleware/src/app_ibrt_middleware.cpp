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
#include "cmsis.h"
#include "hal_trace.h"
#include "bt_drv_interface.h"
#include "hal_bootmode.h"
#include "bt_drv.h"
#include "me_api.h"
#include "besbt.h"
#include "apps.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_middleware.h"
#include "btapp.h"
#include "nvrecord_env.h"
#include "nvrecord_extension.h"
#include "nvrecord_ble.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_dip.h"
#include "app_hfp.h"
#include "btgatt_api.h"
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#if defined(IBRT_UI_V1)
#include "app_ibrt_ui.h"
#else
#include "app_ibrt_configure.h"
#include "app_tws_ibrt_conn_api.h"
#endif // #if defined(IBRT_UI_V1)

#include "app_ibrt_customif_cmd.h"
#include "app_ibrt_if.h"
#include "app_ibrt_customif_ui.h"
#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_include.h"
#endif

#ifdef AI_OTA
#include "ota_common.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_tws.h"
#endif

#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif

#if defined(BES_OTA)
#include "ota_control.h"
#endif

#ifdef GFPS_ENABLED
#include "nvrecord_fp_account_key.h"
#include "app_gfps.h"
#endif

#ifdef  __XSPACE_UI__
#include "xspace_ui.h"
#endif

#ifdef BLE_V2
extern "C" bool is_update_ble_info_to_peer();
#endif

static TWS_ENV_T twsEnv;

static uint8_t ibrt_prevent_sniff[BT_DEVICE_NUM] = {0};

osMutexId twsSyncBufMutexId = NULL;
osMutexDef(twsSyncBufMutex);

#define POST_ROLESWITCH_STATE_STAYING_MS    15000
osTimerId app_post_roleswitch_handling_timer_id = NULL;
static int app_post_roleswitch_handling_timer_handler(void const *param);
osTimerDef (APP_POST_ROLESWITCH_HANDLING_TIMER, (void (*)(void const *))app_post_roleswitch_handling_timer_handler);                      // define timers

/****************************function defination****************************/
static const char *tws_sync_user2str(TWS_SYNC_USER_E user)
{
    const char *str = NULL;
#define CASES(user)          \
    case user:               \
        str = "[" #user "]"; \
        break

    switch (user)
    {
        CASES(TWS_SYNC_USER_BLE_INFO);
        CASES(TWS_SYNC_USER_OTA);
        CASES(TWS_SYNC_USER_AI_CONNECTION);
        CASES(TWS_SYNC_USER_GFPS_INFO);
        CASES(TWS_SYNC_USER_AI_INFO);
        CASES(TWS_SYNC_USER_AI_MANAGER);
        CASES(TWS_SYNC_USER_DIP);

    default:
        str = "[INVALID]";
        break;
    }

    return str;
}

//sniff timing
//slot num (1 slot = 625 us)
#define TWS_LINK_SNIFF_DURATION                     (0xff)
#define TWS_LINK_SNIFF_POLL_INTERVAL                IBRT_UI_LONG_POLL_INTERVAL
#define TWS_LINK_SNIFF_POLL_INTERVAL_IN_SCO         (0xffff)

//ota timing
#define TWS_LINK_OTA_DURATION                       (12)
#define TWS_LINK_OTA_POLL_INTERVAL                  (IBRT_UI_SHORT_POLL_INTERVAL)
#define TWS_LINK_OTA_POLL_INTERVAL_IN_SCO           (IBRT_UI_SHORT_POLL_INTERVAL_IN_SCO)

//ai timing
#define TWS_LINK_AI_DURATION                        (8)
#define TWS_LINK_AI_POLL_INTERVAL                   (IBRT_UI_SHORT_POLL_INTERVAL)
#define TWS_LINK_AI_POLL_INTERVAL_IN_SCO            (IBRT_UI_SHORT_POLL_INTERVAL_IN_SCO)

//a2dp
#define TWS_LINK_A2DP_DURATION                      (6)

//IBRT switch
#define TWS_LINK_ROLE_SWITCH_DURATION                        (8)
#define TWS_LINK_ROLE_SWITCH_POLL_INTERVAL                   (IBRT_UI_SHORT_POLL_INTERVAL)
#define TWS_LINK_ROLE_SWITCH_POLL_INTERVAL_IN_SCO            (IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO)

static TWS_TIMING_CONTROL_INFO_T tws_timing_info[TWS_TIMING_CONTROL_USER_NUM] =
{
    {false,     TWS_LINK_SNIFF_DURATION,        TWS_LINK_SNIFF_POLL_INTERVAL,       TWS_LINK_SNIFF_POLL_INTERVAL_IN_SCO},  //SNIFF
    {false,     IBRT_TWS_LINK_DEFAULT_DURATION, IBRT_UI_DEFAULT_POLL_INTERVAL,      IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO}, //DEFAULT
    {false,     TWS_LINK_A2DP_DURATION,         IBRT_UI_DEFAULT_POLL_INTERVAL,      IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO}, //A2DP,
    {false,     TWS_LINK_OTA_DURATION,          TWS_LINK_OTA_POLL_INTERVAL,         TWS_LINK_OTA_POLL_INTERVAL_IN_SCO},    //OTA
    {false,     TWS_LINK_AI_DURATION,           TWS_LINK_AI_POLL_INTERVAL,          TWS_LINK_AI_POLL_INTERVAL_IN_SCO},     //AI_VOICE
    {false,     TWS_LINK_START_IBRT_DURATION,   TWS_LINK_START_IBRT_INTERVAL,       TWS_LINK_START_IBRT_INTERVAL_IN_SCO}, //START_IBRT
    {false,     TWS_LINK_ROLE_SWITCH_DURATION,  TWS_LINK_ROLE_SWITCH_POLL_INTERVAL, TWS_LINK_ROLE_SWITCH_POLL_INTERVAL_IN_SCO}, //IBRT_SWITCH,
};


/* definition of the bes ibrt sync info handlers */

static void app_ibrt_middleware_send_sync_info(bool isResponse, uint16_t rsp_seq)
{
    TRACE(0, "send sync %d", twsEnv.sync_data.totalLen);
    osMutexWait(twsSyncBufMutexId, osWaitForever);

    if (!isResponse)
    {
        if (twsEnv.sync_data.totalLen > 0)
        {
            tws_ctrl_send_cmd(APP_TWS_CMD_SHARE_COMMON_INFO, ( uint8_t * )&(twsEnv.sync_data),
                twsEnv.sync_data.totalLen+OFFSETOF(TWS_SYNC_DATA_T, content));
        }
    }
    else
    {
        tws_ctrl_send_rsp(APP_TWS_CMD_SHARE_COMMON_INFO,
                          rsp_seq,
                          ( uint8_t * )&(twsEnv.sync_data),
                          twsEnv.sync_data.totalLen+OFFSETOF(TWS_SYNC_DATA_T, content));
    }

    osMutexRelease(twsSyncBufMutexId);
}

static void app_ibrt_middleware_flush_rsp_info(bool isResponse, uint16_t rsp_seq)
{
    app_ibrt_middleware_send_sync_info(isResponse, rsp_seq);
    app_ibrt_if_prepare_sync_info();
}

static TWS_SYNC_FILL_RET_E app_ibrt_middleware_sync_fill_info_handler(TWS_SYNC_USER_E id, bool isResponse)
{
    TWS_SYNC_FILL_RET_E retVal = TWS_SYNC_NO_DATA_FILLED;

    TRACE(1,"[%s isRsp %d]+++", __func__, isResponse);
    TRACE(1,"user:%s", tws_sync_user2str(id));

    osMutexWait(twsSyncBufMutexId, osWaitForever);
    if ((!isResponse && twsEnv.syncUser[id].sync_info_prepare_handler) ||
        (isResponse && twsEnv.syncUser[id].sync_info_prepare_rsp_handler))
    {
        static TWS_SYNC_ENTRY_T entry;
        uint16_t length = 0;
        if (!isResponse)
        {
            twsEnv.syncUser[id].sync_info_prepare_handler(entry.info, &length);
        }
        else
        {
            twsEnv.syncUser[id].sync_info_prepare_rsp_handler(entry.info, &length);
        }

        if (length)
        {
            uint16_t validEntryLen = OFFSETOF(TWS_SYNC_ENTRY_T, info)+length;
            if ((validEntryLen+OFFSETOF(TWS_SYNC_DATA_T, content)+twsEnv.sync_data.totalLen) <=
                sizeof(TWS_SYNC_DATA_T))
            {
                entry.userId = id;
                entry.infoLen = length;
                memcpy(&twsEnv.sync_data.content[twsEnv.sync_data.totalLen], (uint8_t *)&entry,
                    validEntryLen);
                TRACE(0, "total %d + %d", twsEnv.sync_data.totalLen, validEntryLen);
                twsEnv.sync_data.totalLen += validEntryLen;
                retVal = TWS_SYNC_CONTINUE_FILLING;
            }
            else
            {
                // the buffer is full, need to send
                retVal = TWS_SYNC_BUF_FULL;
                osMutexRelease(twsSyncBufMutexId);
                TRACE(0, "TWS sync buf is full, send it to peer.");
                app_ibrt_if_flush_sync_info();
                osMutexWait(twsSyncBufMutexId, osWaitForever);
            }
        }
        else
        {
            TRACE(0,"no info to sync");
        }
    }
    else
    {
        TRACE(0,"prepare handler is null pointer");
    }
    osMutexRelease(twsSyncBufMutexId);

    TRACE(1,"[%s]---", __func__);

    return retVal;
}

static TWS_SYNC_FILL_RET_E app_ibrt_middleware_rsp_sync_info(TWS_SYNC_USER_E id)
{
    TWS_SYNC_FILL_RET_E ret = app_ibrt_middleware_sync_fill_info_handler(id, true);
    if (TWS_SYNC_BUF_FULL == ret)
    {
        ret = app_ibrt_middleware_sync_fill_info_handler(id, true);
        ASSERT(TWS_SYNC_BUF_FULL != ret, "retry sync info filling failed.");
    }

    return ret;
}

static bool app_ibrt_middleware_sync_info_check_data_validity(TWS_SYNC_DATA_T* pInfo, uint16_t length, uint16_t rsp_seq)
{
    TRACE(0, "total len %d rec len %d", pInfo->totalLen+OFFSETOF(TWS_SYNC_DATA_T, content),
        length);

    if (((pInfo->totalLen+OFFSETOF(TWS_SYNC_DATA_T, content)) > length) ||
            (pInfo->totalLen > TWS_SYNC_MAX_XFER_SIZE))
    {
        TRACE(0, "Wrong sync info length %d received !! Max len %d",
            pInfo->totalLen, TWS_SYNC_MAX_XFER_SIZE);
        tws_ctrl_send_rsp(APP_TWS_CMD_SHARE_COMMON_INFO,
                      rsp_seq,
                      NULL,
                      0);
        return false;
    }
    else
    {
        return true;
    }
}

static bool app_ibrt_middleware_sync_info_check_entry_validity(TWS_SYNC_ENTRY_T* pEntry, uint16_t rsp_seq)
{
    TRACE(0, "entry info len %d", pEntry->infoLen);

    if (pEntry->infoLen > TWS_SYNC_BUF_SIZE)
    {
        TRACE(0, "Wrong sync entry length %d received !! Max len %d",
                    pEntry->infoLen, TWS_SYNC_BUF_SIZE);
                tws_ctrl_send_rsp(APP_TWS_CMD_SHARE_COMMON_INFO,
                              rsp_seq,
                              NULL,
                              0);
        return false;
    }
    else
    {
        return true;
    }
}

void app_ibrt_middleware_sync_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"[%s]+++", __func__);

    TWS_SYNC_DATA_T *pInfo = ( TWS_SYNC_DATA_T * )p_buff;

    if (!app_ibrt_middleware_sync_info_check_data_validity(pInfo, length, rsp_seq))
    {
        return;
    }

    app_ibrt_if_prepare_sync_info();

    uint16_t contentOffset = 0;
    while (contentOffset < pInfo->totalLen)
    {
        TWS_SYNC_ENTRY_T* pEntry = (TWS_SYNC_ENTRY_T *)&(pInfo->content[contentOffset]);
        if (!(app_ibrt_middleware_sync_info_check_entry_validity(pEntry, rsp_seq)))
        {
            return;
        }
        else
        {
            TRACE(1,"user:%s", tws_sync_user2str(pEntry->userId));
            if (twsEnv.syncUser[pEntry->userId].sync_info_received_handler)
            {
                twsEnv.syncUser[pEntry->userId].sync_info_received_handler(
                    pEntry->info, pEntry->infoLen);
            }

            TWS_SYNC_FILL_RET_E retVal = app_ibrt_middleware_rsp_sync_info(pEntry->userId);

            if (TWS_SYNC_BUF_FULL == retVal)
            {
                app_ibrt_middleware_flush_rsp_info(true, rsp_seq);
            }

            contentOffset += (pEntry->infoLen+OFFSETOF(TWS_SYNC_ENTRY_T, info));
        }
    }

    // flush pending response
    app_ibrt_middleware_flush_rsp_info(true, rsp_seq);

    TRACE(1,"[%s]---", __func__);
}

void app_ibrt_middleware_sync_info_rsp_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"[%s]+++", __func__);

    TWS_SYNC_DATA_T *pInfo = ( TWS_SYNC_DATA_T * )p_buff;

    if (!app_ibrt_middleware_sync_info_check_data_validity(pInfo, length, rsp_seq))
    {
        return;
    }

    uint16_t contentOffset = 0;
    while (contentOffset < pInfo->totalLen)
    {
        TWS_SYNC_ENTRY_T* pEntry = (TWS_SYNC_ENTRY_T *)&(pInfo->content[contentOffset]);
        if (!(app_ibrt_middleware_sync_info_check_entry_validity(pEntry, rsp_seq)))
        {
            return;
        }
        else
        {
            TRACE(1,"user:%s", tws_sync_user2str(pEntry->userId));
            if (twsEnv.syncUser[pEntry->userId].sync_info_rsp_received_handler)
            {
                twsEnv.syncUser[pEntry->userId].sync_info_rsp_received_handler(
                    pEntry->info, pEntry->infoLen);
            }

            contentOffset += (pEntry->infoLen+OFFSETOF(TWS_SYNC_ENTRY_T, info));
        }
    }

    TRACE(1,"[%s]---", __func__);
}

void app_ibrt_middleware_sync_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"[%s]+++", __func__);

    TWS_SYNC_DATA_T *pInfo = ( TWS_SYNC_DATA_T * )p_buff;

    if (!app_ibrt_middleware_sync_info_check_data_validity(pInfo, length, rsp_seq))
    {
        return;
    }

    uint16_t contentOffset = 0;
    while (contentOffset < pInfo->totalLen)
    {
        TWS_SYNC_ENTRY_T* pEntry = (TWS_SYNC_ENTRY_T *)&(pInfo->content[contentOffset]);
        if (!(app_ibrt_middleware_sync_info_check_entry_validity(pEntry, rsp_seq)))
        {
            return;
        }
        else
        {
            TRACE(1,"user:%s", tws_sync_user2str(pEntry->userId));
            if (twsEnv.syncUser[pEntry->userId].sync_info_rsp_timeout_handler)
            {
                twsEnv.syncUser[pEntry->userId].sync_info_rsp_timeout_handler(
                    pEntry->info, pEntry->infoLen);
            }

            contentOffset += (pEntry->infoLen+OFFSETOF(TWS_SYNC_ENTRY_T, info));
        }
    }

    TRACE(1,"[%s]---", __func__);
}

/* definition of the bes ibrt core layer ibrt API */
void app_ibrt_middleware_master_prepare_rs(uint8_t *p_buff, uint16_t length)
{
#ifdef __IAG_BLE_INCLUDE__
    ble_callback_event_t event;
    event.evt_type = BLE_CALLBACK_RS_START;
    app_ble_core_global_callback_event(&event, NULL);
#endif
#ifdef BISTO_ENABLED
    if (*p_buff == AI_SPEC_GSOUND)
    {
        gsound_tws_update_roleswitch_initiator(IBRT_SLAVE);
        gsound_tws_request_roleswitch();
    }
#endif
#ifdef __AI_VOICE__
    if (*p_buff != AI_SPEC_GSOUND)
    {
        app_ai_tws_master_role_switch_prepare();
    }
#endif
}

void app_ibrt_middleware_slave_continue_rs(uint8_t *p_buff, uint16_t length)
{
    TRACE(2,"%s len %d", __func__, length);
    DUMP8("%x ", p_buff, length);

#ifdef __AI_VOICE__
    if (*p_buff != AI_SPEC_GSOUND)
    {
        app_ai_tws_role_switch_prepare_done();
    }
    else
#endif
    {
        app_ibrt_if_tws_switch_prepare_done_in_bt_thread(IBRT_ROLE_SWITCH_USER_AI, (uint32_t)*p_buff);
    }
}

static APP_TWS_SIDE_T app_tws_side = EAR_SIDE_UNKNOWN;

void app_ibrt_middleware_set_side(APP_TWS_SIDE_T side)
{
    ASSERT((EAR_SIDE_LEFT == side) || (EAR_SIDE_RIGHT == side), "Error: setting invalid side");
    app_tws_side = side;

    app_ibrt_customif_get_tws_side_handler(&app_tws_side);
}

#if !defined(IBRT_UI_V1)
int app_ibrt_middleware_fill_debug_info(char* buf, unsigned int buf_len)
{
    if (app_ibrt_if_is_left_side())
    {
        buf[0] = 'L';
    }
    else if (app_ibrt_if_is_right_side())
    {
        buf[0] = 'R';
    }
    else
    {
        buf[0] = 'U';
    }

    buf[1] = '-';

    TWS_UI_ROLE_E currentUIRole = app_ibrt_conn_get_ui_role();
    if (TWS_UI_MASTER == currentUIRole)
    {
        buf[2] = 'M';
    }
    else if (TWS_UI_SLAVE == currentUIRole)
    {
        buf[2] = 'S';
    }
    else
    {
        buf[2] = 'U';
    }

    buf[3] = '/';

    return 4;
}
#endif

/* definition of the bes ibrt core layer call back functions */

void app_ibrt_middleware_handle_click(void)
{
    TRACE(1,"%s", __func__);

#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
    app_ibrt_customif_ui_tws_switch();
#else
    bt_key_handle_func_click();
#endif
}

void app_ibrt_middleware_role_switch_started_handler(void)
{
    TRACE(1,"[%s]+++", __func__);
    if (NULL == app_post_roleswitch_handling_timer_id)
    {
        app_post_roleswitch_handling_timer_id =
            osTimerCreate(osTimer(APP_POST_ROLESWITCH_HANDLING_TIMER), osTimerOnce, NULL);
    }

    osTimerStop(app_post_roleswitch_handling_timer_id);

    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_ROLE_SWITCH, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);

    TRACE(1,"[%s]---", __func__);
}

void app_ibrt_middleware_ibrt_disconnected_handler(void)
{
    TRACE(0, "twsif_ibrt_disconnected");
}

void app_ibrt_middleware_ble_connected_handler(void)
{
    TRACE(0, "twsif_ble_connected");

    if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
    {
        app_ibrt_if_prepare_sync_info();
        // inform peer to update the common info
        //app_ibrt_if_sync_info(TWS_SYNC_USER_BLE_INFO);
        app_ibrt_if_sync_info(TWS_SYNC_USER_OTA);
        // app_ibrt_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
        app_ibrt_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
        app_ibrt_if_flush_sync_info();
    }
}

void app_ibrt_middleware_ble_disconnected_handler(void)
{
    TRACE(0, "twsif_ble_disconnected");

    if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
    {
        // inform peer to update the common info
        // app_ibrt_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
    }
}

void app_ibrt_middleware_tws_connected_handler(void)
{
    TRACE(1, "twsif_tws_connected, role %d", app_tws_ibrt_role_get_callback(NULL));
#if defined(__XSPACE_UI__)
	xspace_ui_tws_connect_status_change_handle(true, 0);
#endif
}

void app_ibrt_middleware_tws_disconnected_handler(void)
{
    TRACE(0, "twsif_tws_disconnected.");

    if (IBRT_SLAVE == app_tws_ibrt_role_get_callback(NULL))
    {
#ifdef BISTO_ENABLED
        gsound_set_ble_connect_state(GSOUND_CHANNEL_AUDIO, false);
        gsound_set_ble_connect_state(GSOUND_CHANNEL_CONTROL, false);
#endif
    }

#if defined(__XSPACE_UI__)
	xspace_ui_tws_connect_status_change_handle(false, 0);
#endif
}

void app_ibrt_middleware_mobile_connected_handler(uint8_t *addr)
{
    TRACE(0, "twsif_mobile_connected");

    if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
    {
        // inform peer to update the common info
        // app_ibrt_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
        app_ibrt_if_prepare_sync_info();
        app_ibrt_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
        app_ibrt_if_flush_sync_info();
    }

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    app_ai_if_mobile_connect_handle((void *)addr);
#endif
}

void app_ibrt_middleware_mobile_disconnected_handler(uint8_t *addr)
{
    TRACE(0, "twsif_mobile_disconnected");

#if defined(BISTO_ENABLED)
    gsound_on_bt_link_disconnected(addr);
#endif
}

void app_ibrt_middleware_ibrt_connected_handler(void)
{
    TRACE(0, "twsif_ibrt_connected");

    if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
    {
        // inform peer to update the common info
        app_ibrt_if_prepare_sync_info();
        app_ibrt_if_sync_info(TWS_SYNC_USER_AI_INFO);
        app_ibrt_if_sync_info(TWS_SYNC_USER_AI_MANAGER);
        app_ibrt_if_flush_sync_info();
    }
}

void app_ibrt_middleware_tws_connected_sync_info(void)
{
    TRACE(2,"[%s]+++ role %d", __func__, app_tws_ibrt_role_get_callback(NULL));
#ifdef BLE_V2
    ibrt_mobile_info_t POSSIBLY_UNUSED *p_mobile_info = app_ibrt_conn_get_mobile_info_ext();
#endif
#ifdef BLE_V2
    if (app_ibrt_conn_is_nv_master())
#else
    if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
#endif
    {
        app_ibrt_if_prepare_sync_info();
        // inform peer to update the common info
        app_ibrt_if_sync_info(TWS_SYNC_USER_BLE_INFO);
        app_ibrt_if_sync_info(TWS_SYNC_USER_OTA);
        // app_ibrt_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
        app_ibrt_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
        app_ibrt_if_sync_info(TWS_SYNC_USER_AI_INFO);
        app_ibrt_if_sync_info(TWS_SYNC_USER_AI_MANAGER);
        app_ibrt_if_sync_info(TWS_SYNC_USER_DIP);

        app_ibrt_if_flush_sync_info();
#ifdef GFPS_ENABLED
#ifdef BLE_V2
        if (!app_tws_ibrt_mobile_link_connected(&p_mobile_info->mobile_addr))

#else
        if (!app_tws_ibrt_mobile_link_connected())
#endif
        {
            app_enter_fastpairing_mode();
        }
#endif
    }
#ifdef BLE_V2
    else if(app_ibrt_conn_is_nv_slave())
    {
        if (is_update_ble_info_to_peer())
        {
            TRACE(1, "nv slave sync ble info to nv master");
            app_ibrt_if_prepare_sync_info();
            app_ibrt_if_sync_info(TWS_SYNC_USER_BLE_INFO);
            app_ibrt_if_flush_sync_info();
        }
    }
#endif    
    TRACE(1,"[%s]---", __func__);
}

static int app_post_roleswitch_handling_timer_handler(void const *param)
{
    app_bt_start_custom_function_in_bt_thread(
        ACTIVE_MODE_KEEPER_ROLE_SWITCH,
        UPDATE_ACTIVE_MODE_FOR_ALL_LINKS,
        (uint32_t)app_bt_active_mode_clear);
    return 0;
}

static void app_tws_middleware_post_roleswitch_handler(void)
{
    if (NULL == app_post_roleswitch_handling_timer_id)
    {
        app_post_roleswitch_handling_timer_id =
            osTimerCreate(osTimer(APP_POST_ROLESWITCH_HANDLING_TIMER), osTimerOnce, NULL);
    }

    osTimerStart(app_post_roleswitch_handling_timer_id, POST_ROLESWITCH_STATE_STAYING_MS);
    // keep bt link in active mode right after role switch
    app_bt_active_mode_set(ACTIVE_MODE_KEEPER_ROLE_SWITCH, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
}

void app_ibrt_middleware_role_switch_complete_handler(uint8_t newRole)
{
    TRACE(2,"[%s]+++ role %d", __func__, newRole);

#ifdef BISTO_ENABLED
    gsound_on_system_role_switch_done(newRole);
#endif

#ifdef __IAG_BLE_INCLUDE__
    // update BLE info saved in flash after role switch complete
    nv_record_tws_exchange_ble_info();

#ifdef BLE_USE_RPA
    app_ble_refresh_irk();
#else
    btif_me_set_ble_bd_address(bt_get_ble_local_address());
#endif
#endif

#ifdef __AI_VOICE__
    app_ai_tws_role_switch_complete();
#endif

#ifdef __IAG_BLE_INCLUDE__
    ble_callback_event_t event;
    event.evt_type = BLE_CALLBACK_RS_COMPLETE;
    event.p.rs_complete_handled.newRole = newRole;
    app_ble_core_global_callback_event(&event, NULL);
#endif

    app_tws_middleware_post_roleswitch_handler();

    TRACE(1,"[%s]---", __func__);

#ifdef BES_OTA
    ota_control_send_role_switch_complete();
#endif
}

void app_ibrt_middleware_ui_role_updated_handler(uint8_t newRole)
{
#ifdef __IAG_BLE_INCLUDE__
    ble_callback_event_t event;
    event.evt_type = BLE_CALLBACK_ROLE_UPDATE;
    event.p.role_update_handled.newRole = newRole;
    app_ble_core_global_callback_event(&event, NULL);
#endif

#if defined(BISTO_ENABLED) && defined(IBRT)
    gsound_on_tws_role_updated(newRole);
#endif
}

/* definition of the application layer ibrt API */
void app_ibrt_if_register_sync_user(TWS_SYNC_USER_E id, TWS_SYNC_USER_T *user)
{
    memcpy(&twsEnv.syncUser[id], user, sizeof(TWS_SYNC_USER_T));
}

void app_ibrt_if_deregister_sync_user(TWS_SYNC_USER_E id)
{
    memset(&twsEnv.syncUser[id], 0, sizeof(TWS_SYNC_USER_T));
}

void app_ibrt_if_prepare_sync_info(void)
{
    osMutexWait(twsSyncBufMutexId, osWaitForever);
    memset((uint8_t *)&(twsEnv.sync_data), 0, sizeof(twsEnv.sync_data));
    twsEnv.sync_data.totalLen = 0;
    osMutexRelease(twsSyncBufMutexId);
}

TWS_SYNC_FILL_RET_E app_ibrt_if_sync_info(TWS_SYNC_USER_E id)
{
    TWS_SYNC_FILL_RET_E ret = app_ibrt_middleware_sync_fill_info_handler(id, false);

    if (TWS_SYNC_BUF_FULL == ret)
    {
        ret = app_ibrt_middleware_sync_fill_info_handler(id, false);
        ASSERT(TWS_SYNC_BUF_FULL != ret, "retry sync info filling failed.");
    }

    return ret;
}

void app_ibrt_if_flush_sync_info(void)
{
    app_ibrt_middleware_send_sync_info(false, 0);
    app_ibrt_if_prepare_sync_info();
}

void app_ibrt_if_sync_volume_info(void)
{
    if(app_tws_ibrt_tws_link_connected())
    {
        TWS_VOLUME_SYNC_INFO_T ibrt_volume_req;
        ibrt_volume_req.a2dp_local_volume = a2dp_volume_local_get(BT_DEVICE_ID_1);
        ibrt_volume_req.hfp_local_volume = hfp_volume_local_get(BT_DEVICE_ID_1);
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO,(uint8_t*)&ibrt_volume_req, sizeof(ibrt_volume_req));
    }
}

bool app_ibrt_if_is_left_side(void)
{
    return (app_tws_side == EAR_SIDE_LEFT);
}

bool app_ibrt_if_is_right_side(void)
{
    return (app_tws_side == EAR_SIDE_RIGHT);
}

bool app_ibrt_if_unknown_side(void)
{
    return (app_tws_side == EAR_SIDE_UNKNOWN);
}

const char* app_ibrt_if_uirole2str(TWS_UI_ROLE_E uiRole)
{
#if defined(IBRT_UI_V1)
#define CASE_UI_ROLE(uiRole) \
    case uiRole:       \
        return "[" #uiRole "]";
#define CASE_INVALID_UI_ROLE() \
    default:     \
        return "[INVALID]";

    switch (uiRole)
    {
        CASE_UI_ROLE(TWS_UI_MASTER)
        CASE_UI_ROLE(TWS_UI_SLAVE)
        CASE_UI_ROLE(TWS_UI_UNKNOWN)
        CASE_INVALID_UI_ROLE()
    }
#else
    return app_ibrt_conn_uirole2str(uiRole);
#endif
}

TWS_UI_ROLE_E app_ibrt_if_get_ui_role(void)
{
#if defined(IBRT_UI_V1)
    return app_tws_ibrt_role_get_callback(NULL);
#else
    return app_ibrt_conn_get_ui_role();
#endif
}

bool app_ibrt_if_is_ui_slave(void)
{
#if defined(IBRT_UI_V1)
    return (TWS_UI_SLAVE == app_tws_ibrt_role_get_callback(NULL));
#else
    return (TWS_UI_SLAVE == app_ibrt_conn_get_ui_role());
#endif
}

static void bt_thread_app_ibrt_if_prevent_sniff_set(uint8_t *p_mobile_addr, uint16_t prv_sniff_bit)
{
    uint16_t conhdl = 0xff;
    uint8_t device_id  = BT_DEVICE_NUM;
    uint32_t lock;
    do{
        if(p_mobile_addr == NULL)
        {
            break;
        }

        bt_bdaddr_t macAddr;
        memcpy(macAddr.address, p_mobile_addr, sizeof(macAddr.address));

        uint8_t device_id = btif_me_get_device_id_from_addr(&macAddr);

        conhdl = btif_me_get_conhandle_by_remote_address(&macAddr);
        if(conhdl == 0xff || device_id >= BT_DEVICE_NUM)
        {
            break;
        }

        lock = int_lock();
        ibrt_prevent_sniff[device_id] |= prv_sniff_bit;
        int_unlock(lock);

        app_tws_if_ibrt_write_link_policy(&macAddr, BTIF_BLP_DISABLE_ALL);

        btif_remote_device_t* remDev = btif_me_get_remote_device_by_bdaddr(&macAddr);
        if (remDev && btif_me_get_current_mode(remDev) == BTIF_BLM_SNIFF_MODE)
        {
            app_bt_ME_StopSniff(remDev);
        }
    }while(0);

    TRACE(1,"ibrt_sniff_mgr:set sniff prevent,bit=0x%x, dev_id=%d, conhdl=0x%x",prv_sniff_bit,
            device_id, conhdl);
}

void app_ibrt_if_prevent_sniff_set(uint8_t *p_mobile_addr, uint16_t prv_sniff_bit)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)p_mobile_addr, (uint32_t)prv_sniff_bit, (uint32_t)bt_thread_app_ibrt_if_prevent_sniff_set);
}

static void bt_thread_app_ibrt_if_prevent_sniff_clear(uint8_t *p_mobile_addr, uint16_t prv_sniff_bit)
{
    uint16_t conhdl = 0xff;
    uint8_t device_id  = BT_DEVICE_NUM;
    uint32_t lock;

    bt_bdaddr_t macAddr;
    memcpy(macAddr.address, p_mobile_addr, sizeof(macAddr.address));

    do{
        conhdl = btif_me_get_conhandle_by_remote_address(&macAddr);

        device_id = btif_me_get_device_id_from_addr(&macAddr);
        if(conhdl == 0xff || device_id >= BT_DEVICE_NUM)
        {
            break;
        }

        lock = int_lock();
        ibrt_prevent_sniff[device_id] &= ~prv_sniff_bit;
        int_unlock(lock);
        app_tws_if_ibrt_write_link_policy(&macAddr, BTIF_BLP_SNIFF_MODE);
    }while(0);

    TRACE(1,"ibrt_sniff_mgr:clear sniff prevent,bit=0x%x, conhdl=0x%x",prv_sniff_bit, conhdl);
}

void app_ibrt_if_prevent_sniff_clear(uint8_t *p_mobile_addr, uint16_t prv_sniff_bit)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)p_mobile_addr, (uint32_t)prv_sniff_bit, (uint32_t)bt_thread_app_ibrt_if_prevent_sniff_clear);
}

bool app_ibrt_if_customer_prevent_sniff(bt_bdaddr_t *p_mobile_addr)
{
    bool prevented = false;
    uint16_t conhdl = 0xff;
    uint8_t device_id  = BT_DEVICE_NUM;

    do{
        conhdl = btif_me_get_conhandle_by_remote_address(p_mobile_addr);
        device_id = btif_me_get_device_id_from_addr(p_mobile_addr);
        if(conhdl == 0xff || device_id >= BT_DEVICE_NUM)
        {
            break;
        }

        if(ibrt_prevent_sniff[device_id] > 0)
        {
            prevented = true;
            break;
        }
    }while(0);

    return prevented;
}

void app_ibrt_middleware_exit_sniff_with_mobile(uint8_t* mobileAddr)
{
#if defined(IBRT_UI_V1)
    // mobileAddr is filled as NULL
    app_tws_ibrt_exit_sniff_with_mobile();
#else
    bt_bdaddr_t macAddr;
    memcpy(macAddr.address, mobileAddr, sizeof(macAddr.address));
    app_tws_ibrt_exit_sniff_with_mobile((const bt_bdaddr_t *)&macAddr);
#endif
}

void app_ibrt_if_resume_bdaddr_in_nv(void)
{
#if !defined(IBRT_UI_V1)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->nv_role == IBRT_SLAVE)
    {
        uint8_t *factory_addr = app_ibrt_if_get_bt_local_address();
        memcpy(p_ibrt_ctrl->local_addr.address, factory_addr, BD_ADDR_LEN);
        btif_me_set_bt_address(p_ibrt_ctrl->local_addr.address);
    }
#endif
}

void app_ibrt_reconect_mobile_after_factorty_test(void)
{
        TRACE(0,"Factory test done, reconnect mobile");
#ifdef IBRT_UI_V2
        app_ibrt_if_resume_bdaddr_in_nv();
        app_ibrt_if_event_entry(IBRT_MGR_EV_FREE_MAN_MODE);
        app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
#endif
}

void app_ibrt_if_test_enter_freeman(void)
{
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_CUSTOM_OP1_AFTER_REBOOT);
    app_reset();
}

bool app_ibrt_process_factory_test_cmd(uint8_t test_type)
{
    TWS_UI_ROLE_E role = app_ibrt_if_get_ui_role();
    bool need_block = false;

    switch (test_type)
    {
        case NV_READ_LE_TEST_RESULT:
        case NV_READ_BT_RX_TEST_RESULT:
            break;
        default:
            if(role == TWS_UI_SLAVE)
            {
                app_ibrt_if_test_enter_freeman();
                need_block = true;
            }
            break;
    }
    return need_block;
}

void app_ibrt_middleware_init(void)
{
    // reset the environment
    memset(&twsEnv, 0, sizeof(twsEnv));
    //init ibrt_prevent_sniff array parameter
    memset(ibrt_prevent_sniff, 0, BT_DEVICE_NUM);

    app_tws_ibrt_bandwidth_table_register(tws_timing_info);

    // init the tws sync buffer mutex
    if (NULL == twsSyncBufMutexId)
    {
        twsSyncBufMutexId = osMutexCreate(osMutex(twsSyncBufMutex));
    }

    // register all users
#ifdef __IAG_BLE_INCLUDE__
    // init ble tws interface
    app_ble_mode_tws_sync_init();
#endif

#ifdef AI_OTA
    ota_common_tws_sync_init();
#endif

#ifdef BISTO_ENABLED
    // init gsound tws interface
    app_ai_tws_gsound_sync_init();
#endif

#ifdef GFPS_ENABLED
    app_gfps_tws_sync_init();
#endif

#ifdef __AI_VOICE__
    app_ai_tws_sync_init();
#endif

#ifdef IS_MULTI_AI_ENABLED
    ai_manager_sync_init();
#endif

    if(app_dip_function_enable() == true){
        app_dip_sync_init();
    }

    app_factory_register_test_ind_callback(app_ibrt_process_factory_test_cmd);
}
