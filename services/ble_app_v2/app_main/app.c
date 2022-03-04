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
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)

#include <string.h>

#include "app_task.h"                // Application task Definition
#include "gap.h"                     // GAP Definition
#include "tgt_hardware.h"
#include "gapm_msg.h"               // GAP Manager Task API
#include "gapc_msg.h"               // GAP Controller Task API

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "app.h"                     // Application Definition

#include "nvrecord_ble.h"
#include "prf.h"

#include "nvrecord_env.h"
#ifndef _BLE_NVDS_
#include "tgt_hardware.h"
#endif
#ifdef __AI_VOICE__
#include "app_ai_ble.h"                  // AI Voice Application Definitions
#endif //(__AI_VOICE__)

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"                  // Data Path Server Application Definitions
#endif //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_DIS)
#include "app_dis.h"                 // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                 // HID Application Definitions
#endif //(BLE_APP_HID)

#if (BLE_APP_VOICEPATH)
#include "app_voicepath_ble.h"         // Voice Path Application Definitions
#endif //(BLE_APP_VOICEPATH)

#if (BLE_APP_OTA)
#include "app_ota.h"                  // OTA Application Definitions
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h"                  // OTA Application Definitions
#endif //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h"                  // ANCC Application Definitions
#endif //(BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "app_amsc.h"               // AMS Module Definition
#endif // (BLE_APP_AMS)

#if (BLE_APP_GFPS)
#include "app_gfps.h"               // Google Fast Pair Service Definitions
#endif
#if (DISPLAY_SUPPORT)
#include "app_display.h"             // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"                 // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)

#include "cmsis_os.h"
#include "ke_timer.h"
#include "nvrecord.h"
#include "ble_app_dbg.h"

#include "app_bt.h"
#include "app_ble_mode_switch.h"
#include "apps.h"
#include "crc16_c.h"
#include "besbt.h"

#ifdef BISTO_ENABLED
#include "gsound_service.h"
#endif

#if defined(__INTERCONNECTION__)
#include "app_bt.h"
#include "app_battery.h"
#endif

#if (BLE_APP_TILE)
#include "tile_target_ble.h"
#include "tile_service.h"
#endif

#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif

#ifdef FPGA
#include "hal_timer.h"
#endif

#include "bt_if.h"
#include "btgatt_api.h"

#if BLE_BUDS
#include "buds.h"
#endif

#include "gapm.h"
#if BLE_V2
#include "gap_int.h"
#endif
#include "bt_drv_reg_op.h"

#ifdef SWIFT_ENABLED
#include "app_swift.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

/// Default Device Name
#define APP_DFLT_DEVICE_NAME            ("BES_BLE")
#define APP_DFLT_DEVICE_NAME_LEN        (sizeof(APP_DFLT_DEVICE_NAME))


#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME        "Hid Mouse"
#else
#define DEVICE_NAME        "RW DEVICE"
#endif

#define DEVICE_NAME_SIZE    sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID        "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN    (4)
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)

#if (BLE_APP_DATAPATH_SERVER)
/*
 * x11 - Length
 * x07 - Complete list of 16-bit UUIDs available
 * .... the 128 bit UUIDs
 */
#define APP_DATAPATH_SERVER_ADV_DATA_UUID        "\x11\x07\x9e\x34\x9B\x5F\x80\x00\x00\x80\x00\x10\x00\x00\x00\x01\x00\x01"
#define APP_DATAPATH_SERVER_ADV_DATA_UUID_LEN    (18)
#endif //(BLE_APP_HT)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE    "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)

/**
 * Default Scan response data
 * --------------------------------------------------------------------------------------
 * x09                             - Length
 * xFF                             - Vendor specific advertising type
 * x00\x60\x52\x57\x2D\x42\x4C\x45 - "RW-BLE"
 * --------------------------------------------------------------------------------------
 */
#define APP_SCNRSP_DATA         "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LEN     (10)

/**
 * Advertising Parameters
 */
#if (BLE_APP_HID)
/// Default Advertising duration - 30s (in multiple of 10ms)
#define APP_DFLT_ADV_DURATION   (3000)
#endif //(BLE_APP_HID)
/// Advertising channel map - 37, 38, 39
#define APP_ADV_CHMAP           (0x07)
/// Advertising minimum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MIN         (64)
/// Advertising maximum interval - 40ms (64*0.625ms)
#define APP_ADV_INT_MAX         (64)
/// Fast advertising interval
#define APP_ADV_FAST_INT        (32)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
    #if (BLE_APP_HT)
    APPM_SVC_HTS,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    APPM_SVC_DIS,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    APPM_SVC_BATT,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    APPM_SVC_HIDS,
    #endif //(BLE_APP_HID)
    #ifdef BLE_APP_AM0
    APPM_SVC_AM0_HAS,
    #endif //defined(BLE_APP_AM0)
    #if (BLE_APP_HR)
    APPM_SVC_HRP,
    #endif
    #if (BLE_APP_DATAPATH_SERVER)
    APPM_SVC_DATAPATH_SERVER,
    #endif //(BLE_APP_DATAPATH_SERVER)
    #if (BLE_APP_VOICEPATH)
    APPM_SVC_VOICEPATH,
    #if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
    APPM_SVC_BMS,
    #endif
    #endif //(BLE_APP_VOICEPATH)
    #if (ANCS_PROXY_ENABLE)
    APPM_SVC_ANCSP,
    APPM_SVC_AMSP,
    #endif
    #if (BLE_APP_ANCC)
    APPM_SVC_ANCC,
    #endif //(BLE_APP_ANCC)
    #if (BLE_APP_AMS)
    APPM_SVC_AMSC,
    #endif //(BLE_APP_AMS)
    #if (BLE_APP_OTA)
    APPM_SVC_OTA,
    #endif //(BLE_APP_OTA)
    #if (BLE_APP_GFPS)
    APPM_SVC_GFPS,
    #endif //(BLE_APP_GFPS)
    #if (BLE_AMA_VOICE)
    APPM_AI_AMA,
    #endif //(BLE_AI_VOICE)

    #if (BLE_DMA_VOICE)
    APPM_AI_DMA,
    #endif //(BLE_AI_VOICE)

    #if (BLE_GMA_VOICE)
    APPM_AI_GMA,
    #endif //(BLE_AI_VOICE)

    #if (BLE_SMART_VOICE)
    APPM_AI_SMARTVOICE,
    #endif //(BLE_AI_VOICE)

    #if (BLE_TENCENT_VOICE)
    APPM_AI_TENCENT,
    #endif //(BLE_AI_VOICE)
    #if (BLE_APP_TOTA)
    APPM_SVC_TOTA,
    #endif //(BLE_APP_TOTA)

    #if (BLE_APP_TILE)
    APPM_SVC_TILE,
    #endif //(BLE_APP_TILE)

    #if (BLE_BUDS)
    APPM_SVC_BUDS,
    #endif //(BLE_BUDS)

    APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    #if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_AM0)
    (appm_add_svc_func_t)app_am0_add_has,
    #endif //(BLE_APP_AM0)
    #if (BLE_APP_HR)
    (appm_add_svc_func_t)app_hrps_add_profile,
    #endif
    #if (BLE_APP_DATAPATH_SERVER)
    (appm_add_svc_func_t)app_datapath_add_datapathps,
    #endif //(BLE_APP_DATAPATH_SERVER)
    #if (BLE_APP_VOICEPATH)
    (appm_add_svc_func_t)app_ble_voicepath_add_svc,
    #if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
    (appm_add_svc_func_t)app_ble_bms_add_svc,
    #endif
    #endif //(BLE_APP_VOICEPATH)
    #if (ANCS_PROXY_ENABLE)
    (appm_add_svc_func_t)app_ble_ancsp_add_svc,
    (appm_add_svc_func_t)app_ble_amsp_add_svc,
    #endif
    #if (BLE_APP_ANCC)
    (appm_add_svc_func_t)app_ancc_add_ancc,
    #endif
    #if (BLE_APP_AMS)
    (appm_add_svc_func_t)app_amsc_add_amsc,
    #endif
    #if (BLE_APP_OTA)
    (appm_add_svc_func_t)app_ota_add_ota,
    #endif //(BLE_APP_OTA)
    #if (BLE_APP_GFPS)
    (appm_add_svc_func_t)app_gfps_add_gfps,
    #endif
    #if (BLE_APP_AMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_ama,
    #endif //(BLE_APP_AMA_VOICE)
    #if (BLE_APP_DMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_dma,
    #endif //(BLE_APP_DMA_VOICE)
    #if (BLE_APP_GMA_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_gma,
    #endif
    #if (BLE_APP_SMART_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_smartvoice,
    #endif
    #if (BLE_APP_TENCENT_VOICE)
    (appm_add_svc_func_t)app_ai_ble_add_tencent,
    #endif
    #if (BLE_APP_TOTA)
    (appm_add_svc_func_t)app_tota_add_tota,
    #endif //(BLE_APP_TOTA)
    #if (BLE_APP_TILE)
    (appm_add_svc_func_t)app_ble_tile_add_svc,
    #endif
    #if (BLE_BUDS)
    (appm_add_svc_func_t)app_add_buds,
    #endif //(BLE_BUDS)
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

#ifdef CFG_SEC_CON
uint8_t ble_public_key[64];
uint8_t ble_private_key[32];
#endif

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void appm_refresh_ble_irk(void)
{
    nv_record_blerec_get_local_irk(app_env.loc_irk);
    LOG_I("[APPM]local irk:");
    DUMP8("0x%02x ", app_env.loc_irk, 16);
    // TODO: update to use the new API
    gapm_update_irk(app_env.loc_irk);
}

uint8_t *appm_get_current_ble_irk(void)
{
    return app_env.loc_irk;
}

void appm_set_white_list(gap_bdaddr_t *bdaddr, uint8_t size)
{
    uint8_t actv_idx = 0xFF;
    if (size > WHITE_LIST_MAX_NUM)
    {
        size = WHITE_LIST_MAX_NUM;
    }

    app_env.white_list_size = size;
    memcpy(&app_env.white_list_addr[0], bdaddr, sizeof(gap_bdaddr_t)*size);

    for (uint8_t i=0; i<BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        actv_idx = app_env.adv_actv_idx[i];
        if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
                APP_ADV_STATE_STARTED == app_env.state[actv_idx])
        {
            appm_stop_advertising(actv_idx);
            app_env.need_set_white_list = true;
            return;
        }
    }

    if (app_env.scan_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_SCAN_STATE_STARTED == app_env.state[app_env.scan_actv_idx])
    {
        appm_stop_scanning();
        app_env.need_set_white_list = true;
    }
    else if (app_env.connect_actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_CONNECT_STATE_STARTED == app_env.state[app_env.connect_actv_idx])
    {
        appm_stop_connecting();
        app_env.need_set_white_list = true;
    }
    else
    {
        app_env.need_set_white_list = false;
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_list_set_wl_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_LIST_SET_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_list_set_wl_cmd,
                                                             sizeof(gap_bdaddr_t)*size);

        p_cmd->operation = GAPM_SET_WL;
        p_cmd->size = size;
        memcpy(&p_cmd->wl_info[0], bdaddr, sizeof(gap_bdaddr_t)*size);

        // Send the message
        ke_msg_send(p_cmd);
    }
}

static void appm_kick_off_advertising(uint8_t actv_idx)
{
    LOG_I("%s actv_idx %d", __func__, actv_idx);
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_APP,
                                                         gapm_activity_start_cmd);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = actv_idx;
    p_cmd->u_param.adv_add_param.duration = 0;
    p_cmd->u_param.adv_add_param.max_adv_evt = 0;

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
}

void appm_stop_advertising(uint8_t actv_idx)
{
    LOG_I("%s actv_idx %d", __func__, actv_idx);
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_ADV_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update advertising state
        appm_update_actv_state(actv_idx, APP_ADV_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_ADV_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_ADV_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
    }
}

static void appm_kick_off_scanning(void)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_APP,
                                                         gapm_activity_start_cmd);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.scan_actv_idx;
    p_cmd->u_param.scan_param.prop = 
        GAPM_SCAN_PROP_PHY_1M_BIT|GAPM_SCAN_PROP_ACTIVE_1M_BIT;
    if (app_env.scanFiltPolicy & BLE_SCAN_ALLOW_ADV_WLST)
    {
        p_cmd->u_param.scan_param.type = GAPM_SCAN_TYPE_SEL_CONN_DISC;
    }
    else
    {
        p_cmd->u_param.scan_param.type = GAPM_SCAN_TYPE_GEN_DISC;
    }

    if (app_env.scanFiltPolicy & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
    {
        p_cmd->u_param.scan_param.prop |= GAPM_SCAN_PROP_ACCEPT_RPA_BIT;
    }

    p_cmd->u_param.scan_param.dup_filt_pol = GAPM_DUP_FILT_DIS;
    p_cmd->u_param.scan_param.scan_param_1m.scan_intv = app_env.scanIntervalInMs*1000/625;
    p_cmd->u_param.scan_param.scan_param_1m.scan_wd = app_env.scanWindowInMs*1000/625;
    p_cmd->u_param.scan_param.duration = 0;
    p_cmd->u_param.scan_param.period = 0;
    if (p_cmd->u_param.scan_param.duration && p_cmd->u_param.scan_param.period)
    {
        ASSERT(p_cmd->u_param.scan_param.duration < p_cmd->u_param.scan_param.period,
            "%s duration %d is greater than or equal to period %d", __func__,
            p_cmd->u_param.scan_param.duration, p_cmd->u_param.scan_param.period);
    }

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(app_env.scan_actv_idx, APP_SCAN_STATE_STARTING);
}

void appm_stop_scanning(void)
{
    uint8_t actv_idx = app_env.scan_actv_idx;
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_SCAN_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update advertising state
        appm_update_actv_state(actv_idx, APP_SCAN_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_SCAN_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_SCAN_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_SCAN, actv_idx);
    }
}

static void appm_kick_off_connecting(void)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_APP,
                                                         gapm_activity_start_cmd);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.connect_actv_idx;
    p_cmd->u_param.init_param.type = app_env.ble_conn_param.gapm_init_type;
    p_cmd->u_param.init_param.prop = GAPM_INIT_PROP_1M_BIT;
    p_cmd->u_param.init_param.conn_to = app_env.ble_conn_param.conn_to; //!< 10s timeout

    /// scan param
    p_cmd->u_param.init_param.scan_param_1m.scan_intv = 0x0040;
    p_cmd->u_param.init_param.scan_param_1m.scan_wd =  0x0030;

    /// 1M phy connect param
    p_cmd->u_param.init_param.conn_param_1m.conn_intv_min = 102;
    p_cmd->u_param.init_param.conn_param_1m.conn_intv_max = 102;
    p_cmd->u_param.init_param.conn_param_1m.conn_latency = 0;
    p_cmd->u_param.init_param.conn_param_1m.supervision_to = 0x01F4;
    p_cmd->u_param.init_param.conn_param_1m.ce_len_min = 0;
    p_cmd->u_param.init_param.conn_param_1m.ce_len_max = 8;

    if (GAPM_INIT_TYPE_DIRECT_CONN_EST == p_cmd->u_param.init_param.type)
    {
        memcpy(p_cmd->u_param.init_param.peer_addr.addr, app_env.ble_conn_param.peer_addr.addr, BD_ADDR_LEN);
        p_cmd->u_param.init_param.peer_addr.addr_type = app_env.ble_conn_param.peer_addr.addr_type;
    }

    // Send the message
    ke_msg_send(p_cmd);

    // Update connecting state
    appm_update_actv_state(app_env.connect_actv_idx, APP_CONNECT_STATE_STARTING);
}

void appm_stop_connecting(void)
{
    uint8_t actv_idx = app_env.connect_actv_idx;
    if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
            APP_CONNECT_STATE_STARTED == app_env.state[actv_idx])
    {
        // Prepare the GAPM_ACTIVITY_STOP_CMD message
        struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                          TASK_GAPM, TASK_APP,
                                                          gapm_activity_stop_cmd);

        // Fill the allocated kernel message
        cmd->operation = GAPM_STOP_ACTIVITY;
        cmd->actv_idx = actv_idx;

        // Send the message
        ke_msg_send(cmd);

        // Update connecting state
        //appm_update_actv_state(app_env.connect_actv_idx, APP_CONNECT_STATE_STOPPING);
    }
    else if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
             app_env.state[actv_idx] > APP_CONNECT_STATE_STARTED &&
             app_env.state[actv_idx] <= APP_CONNECT_STATE_DELETING)
    {
        LOG_I("%s stopping now state %d", __func__, app_env.state[actv_idx]);
    }
    else
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_STOP_CONNECTING, actv_idx);
    }
}

static void appm_set_adv_data(uint8_t actv_idx)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_ADV_DATA;
    p_cmd->actv_idx = actv_idx;

    p_cmd->length = app_env.advParam.advDataLen;
    memcpy(p_cmd->data, app_env.advParam.advData, p_cmd->length);

    // Send the message
    ke_msg_send(p_cmd);

    if (false == app_env.need_set_rsp_data)
    {
        if (app_env.need_update_adv)
        {
            app_env.need_update_adv = false;
            appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
        }
        else //if (GAPM_ADV_NON_CONN != app_env.advParam.advType)
        {
            // Update advertising state
            appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_SCAN_RSP_DATA);
        }
    }
    else //if (GAPM_ADV_NON_CONN != app_env.advParam.advType)
    {
        // Update advertising state
        appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_ADV_DATA);
    }
}

static void appm_set_scan_rsp_data(uint8_t actv_idx)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_SCAN_RSP_DATA;
    p_cmd->actv_idx = actv_idx;

    p_cmd->length = app_env.advParam.scanRspDataLen;
    memcpy(p_cmd->data, app_env.advParam.scanRspData, p_cmd->length);

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    if (app_env.need_update_adv)
    {
        app_env.need_update_adv = false;
        appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTING);
    }
    else
    {
        appm_update_actv_state(actv_idx, APP_ADV_STATE_SETTING_SCAN_RSP_DATA);
    }
}

static void appm_send_gapm_reset_cmd(void)
{
    // Reset the stack
    struct gapm_reset_cmd *p_cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_reset_cmd);

    p_cmd->operation = GAPM_RESET;

    ke_msg_send(p_cmd);
}

void appm_adv_activity_index_init(void)
{
    for (uint8_t i=0; i<BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        app_env.adv_actv_idx[i] = INVALID_BLE_ACTIVITY_INDEX;
    }
}

bool appm_check_adv_activity_index(uint8_t actv_idx)
{
    for (uint8_t i=0; i<BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        if (app_env.adv_actv_idx[i] == actv_idx)
        {
            return true;
        }
    }

    LOG_I("%s %d %d %d %d", __func__,
                            actv_idx,
                            app_env.adv_actv_idx[0],
                            app_env.adv_actv_idx[1],
                            app_env.adv_actv_idx[2]);
    return false;
}

void appm_clear_adv_activity_index(uint8_t actv_idx)
{
    for (uint8_t i=0; i<BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        if (app_env.adv_actv_idx[i] == actv_idx)
        {
            app_env.adv_actv_idx[i] = INVALID_BLE_ACTIVITY_INDEX;
            return;
        }
    }
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
//MSB->LSB
const uint8_t bes_demo_Public_key[64] = {
    0x3E,0x08,0x3B,0x0A,0x5C,0x04,0x78,0x84,0xBE,0x41,
    0xBE,0x7E,0x52,0xD1,0x0C,0x68,0x64,0x6C,0x4D,0xB6,
    0xD9,0x20,0x95,0xA7,0x32,0xE9,0x42,0x40,0xAC,0x02,
    0x54,0x48,0x99,0x49,0xDA,0xE1,0x0D,0x9C,0xF5,0xEB,
    0x29,0x35,0x7F,0xB1,0x70,0x55,0xCB,0x8C,0x8F,0xBF,
    0xEB,0x17,0x15,0x3F,0xA0,0xAA,0xA5,0xA2,0xC4,0x3C,
    0x1B,0x48,0x60,0xDA
};

//MSB->LSB
const uint8_t bes_demo_private_key[32]= {
     0xCD,0xF8,0xAA,0xC0,0xDF,0x4C,0x93,0x63,0x2F,0x48,
     0x20,0xA6,0xD8,0xAB,0x22,0xF3,0x3A,0x94,0xBF,0x8E,
     0x4C,0x90,0x25,0xB3,0x44,0xD2,0x2E,0xDE,0x0F,0xB7,
     0x22,0x1F
 };
 
void appm_init()
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    appm_adv_activity_index_init();
    app_env.scan_actv_idx = INVALID_BLE_ACTIVITY_INDEX;
    app_env.connect_actv_idx = INVALID_BLE_ACTIVITY_INDEX;

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

#ifdef _BLE_NVDS_
    nv_record_blerec_init();
    nv_record_blerec_get_local_irk(app_env.loc_irk);
#else
    uint8_t counter;

    //avoid ble irk collision low probability
    uint32_t generatedSeed = hal_sys_timer_get();
    for (uint8_t index = 0; index < sizeof(bt_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
    }
    srand(generatedSeed);

    // generate a new IRK
    for (counter = 0; counter < KEY_LEN; counter++)
    {
        app_env.loc_irk[counter]    = (uint8_t)co_rand_word();
    }
#endif

#ifdef CFG_SEC_CON
    // Note: for the product that needs secure connection feature while google
    // fastpair is not included, the ble private/public key should be
    // loaded from custom parameter section
    memcpy(ble_public_key, bes_demo_Public_key, sizeof(ble_public_key));
    memcpy(ble_private_key, bes_demo_private_key, sizeof(ble_private_key));
#endif

    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/

    // load device information:

    #if (DISPLAY_SUPPORT)
    app_display_init();
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
    #endif // (BLE_APP_SEC)

    #if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
    #endif //(BLE_APP_HT)

    #if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
    #endif //(BLE_APP_DIS)

    #if (BLE_APP_HID)
    // HID Module
    app_hid_init();
    #endif //(BLE_APP_HID)

    #if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
    #endif //(BLE_APP_BATT)

    #if (BLE_APP_AM0)
    // Audio Mode 0 Module
    app_am0_init();
    #endif //(BLE_APP_AM0)

    #if (BLE_APP_VOICEPATH)
    // Voice Path Module
    app_ble_voicepath_init();
    #endif //(BLE_APP_VOICEPATH)

    #if (BLE_APP_TILE)
    app_ble_tile_init();
    #endif
    
    #if (BLE_APP_DATAPATH_SERVER)
    // Data Path Server Module
    app_datapath_server_init();
    #endif //(BLE_APP_DATAPATH_SERVER)

    #if (BLE_APP_OTA)
    // OTA Module
    app_ota_init();
    #endif //(BLE_APP_OTA)

    #if (BLE_APP_TOTA)
    // TOTA Module
    app_tota_ble_init();
    #endif //(BLE_APP_TOTA)

    #if (BLE_APP_GFPS)
    //
    app_gfps_init();
    #endif

    #ifdef SWIFT_ENABLED
    app_swift_init();
    #endif

    #if (BLE_APP_TILE)
    app_tile_init();
    #endif 

    // Reset the stack
    appm_send_gapm_reset_cmd();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }

    return more_svc;
}

uint16_t appm_get_conhdl_from_conidx(uint8_t conidx)
{
    if (BLE_INVALID_CONNECTION_INDEX == conidx)
    {
        return 0xFFFF;
    }

    return app_env.context[conidx].conhdl;
}

void appm_disconnect(uint8_t conidx)
{
    ASSERT(conidx<BLE_CONNECTION_MAX, "%s conidx 0x%x", __func__, conidx);
    if (BLE_CONNECTED == app_env.context[conidx].connectStatus)
    {
        app_env.context[conidx].connectStatus = BLE_DISCONNECTING;
        struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, conidx),
                                                   TASK_APP,
                                                   gapc_disconnect_cmd);

        cmd->operation = GAPC_DISCONNECT;
        cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

        // Send the message
        ke_msg_send(cmd);
    }
    else
    {
        TRACE(0, "will not execute disconnect since state is:%d",
              app_env.context[conidx].connectStatus);
    }

    ble_execute_pending_op(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);
}

uint8_t appm_is_connected(void)
{
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (BLE_CONNECTED == app_env.context[index].connectStatus)
        {
            return 1;
        }
    }
    return 0;
}

// may not be applied as current ble host stack doesn't support 
// multiple ble activities working at the same time?
void appm_delete_activity(uint8_t actv_idx)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ACTIVITY;
    p_cmd->actv_idx = actv_idx;

    // Send the message
    ke_msg_send(p_cmd);
}

void appm_update_actv_state(uint8_t actv_idx, enum app_actv_state newState)
{
    ASSERT(actv_idx<BLE_ACTIVITY_MAX, "%s actv_idx %d newState %d call %p", __func__, actv_idx, newState, __builtin_return_address(0));
    enum app_actv_state oldState = app_env.state[actv_idx];
    if (oldState == APP_ACTV_STATE_IDLE &&
        (newState == APP_ADV_STATE_CREATING ||
         newState == APP_SCAN_STATE_CREATING ||
         newState == APP_CONNECT_STATE_CREATING))
    {
    }
    else if (newState == APP_ACTV_STATE_IDLE &&
            (oldState == APP_ADV_STATE_DELETING ||
             oldState == APP_SCAN_STATE_DELETING ||
             oldState == APP_CONNECT_STATE_DELETING))
    {
    }
    else if (oldState == APP_ADV_STATE_STARTED &&
            (newState == APP_ADV_STATE_SETTING_ADV_DATA ||
             newState == APP_ADV_STATE_SETTING_SCAN_RSP_DATA ||
             newState == APP_ADV_STATE_STARTING))
    {
    }
    else if (newState == APP_ADV_STATE_STARTING && oldState == APP_ADV_STATE_SETTING_ADV_DATA)
    {
    }
    else if (oldState == APP_ADV_STATE_STARTING && newState == APP_ADV_STATE_DELETING)
    {
    }
    else if (oldState == APP_SCAN_STATE_STARTING && newState == APP_SCAN_STATE_DELETING)
    {
    }
    else if (oldState == APP_ADV_STATE_CREATING &&
            (newState == APP_ADV_STATE_STARTING ||
             newState == APP_ADV_STATE_SETTING_SCAN_RSP_DATA))
    {
    }
    else if (oldState == APP_SCAN_STATE_STARTED && newState == APP_SCAN_STATE_STARTING)
    {
    }
    else if(oldState == APP_CONNECT_STATE_STARTING && newState == APP_CONNECT_STATE_DELETING)
    {
    }
    else if ((newState - oldState) == 1)
    {
    }
    else
    {
        ASSERT(0, "%s actv_idx %d oldState %d newState %d call %p", __func__, actv_idx, oldState, newState, __builtin_return_address(0));
    }

    TRACE(0, "actv_idx %d actv state from %d to %d", actv_idx, app_env.state[actv_idx], newState);
    app_env.state[actv_idx] = newState;
}

void appm_actv_fsm_next(uint8_t actv_idx, uint8_t status)
{
    ASSERT(actv_idx<BLE_ACTIVITY_MAX, "%s actv_idx %d call %p", __func__, actv_idx, __builtin_return_address(0));
    switch (app_env.state[actv_idx])
    {
        case (APP_ADV_STATE_CREATING):
        {
            if ((app_env.advParam.advType == ADV_TYPE_DIRECT_LDC) || 
                (app_env.advParam.advType == ADV_TYPE_DIRECT_HDC))
            {
                // kick off advertising activity
                appm_kick_off_advertising(actv_idx);
            }
            else
            {
                // Set advertising data
                appm_set_adv_data(actv_idx);
            }
        } break;

        case (APP_ADV_STATE_SETTING_ADV_DATA):
        {
            // Set scan response data
            appm_set_scan_rsp_data(actv_idx);
        } break;

        case (APP_ADV_STATE_SETTING_SCAN_RSP_DATA):
        {
            // kick off advertising activity
            appm_kick_off_advertising(actv_idx);
        } break;

        case (APP_ADV_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_ADV_STATE_STARTED);
                app_advertising_started(actv_idx);
                ble_execute_pending_op(BLE_ACTV_ACTION_STARTING_ADV, actv_idx);
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_ADV_STATE_DELETING);
                app_advertising_starting_failed(actv_idx, status);
                appm_delete_activity(actv_idx);
            }
        } break;

        case (APP_ADV_STATE_STOPPING):
        {
            ASSERT(appm_check_adv_activity_index(actv_idx), "%s error actv_idx %d", __func__, actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_ADV_STATE_DELETING);
            appm_delete_activity(actv_idx);
        } break;
        case (APP_ADV_STATE_DELETING):
        {
            // Go next state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_advertising_stopped(actv_idx);
            appm_clear_adv_activity_index(actv_idx);
            if (app_env.need_restart_adv)
            {
                app_env.need_restart_adv = false;
                appm_start_advertising(&app_env.advParam);
            }
            else if (app_env.need_set_white_list)
            {
                appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
            }
        } break;

        case (APP_SCAN_STATE_CREATING):
        {
            // kick off scanning activity
            appm_kick_off_scanning();
        } break;

        case (APP_SCAN_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_SCAN_STATE_STARTED);
                app_scanning_started();
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_SCAN_STATE_DELETING);
                appm_delete_activity(actv_idx);
                app_scanning_starting_failed(actv_idx, status);
            }
            ble_execute_pending_op(BLE_ACTV_ACTION_STARTING_SCAN, actv_idx);
        } break;

        case (APP_SCAN_STATE_STOPPING):
        {
            ASSERT(actv_idx==app_env.scan_actv_idx, "%s actv_idx %d scan_idx %d", __func__, actv_idx, app_env.scan_actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_SCAN_STATE_DELETING);
            appm_delete_activity(actv_idx);
        } break;
        case (APP_SCAN_STATE_DELETING):
        {
            // Go created state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_env.scan_actv_idx = INVALID_BLE_ACTIVITY_INDEX;
            if (app_env.need_restart_scan)
            {
                app_env.need_restart_scan = false;
                appm_start_scanning(app_env.scanIntervalInMs, app_env.scanWindowInMs, app_env.scanFiltPolicy);
            }
            else if (app_env.need_set_white_list)
            {
                appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOPPING_SCAN, actv_idx);
                app_scanning_stopped();
            }
        } break;

        case (APP_CONNECT_STATE_CREATING):
        {
            // kick off connecting activity
            appm_kick_off_connecting();
        } break;
        case (APP_CONNECT_STATE_STARTING):
        {
            if (GAP_ERR_NO_ERROR == status)
            {
                // Go to started state
                appm_update_actv_state(actv_idx, APP_CONNECT_STATE_STARTED);
            }
            else
            {
                appm_update_actv_state(actv_idx, APP_CONNECT_STATE_DELETING);
                appm_delete_activity(actv_idx);
                app_connecting_failed(actv_idx, status);
            }
            ble_execute_pending_op(BLE_ACTV_ACTION_CONNECTING, actv_idx);
        } break;
        case (APP_CONNECT_STATE_STOPPING):
        {
            ASSERT(actv_idx==app_env.connect_actv_idx, "%s actv_idx %d conn_idx %d", __func__, actv_idx, app_env.connect_actv_idx);
            // Go next state
            appm_update_actv_state(actv_idx, APP_CONNECT_STATE_DELETING);
            appm_delete_activity(actv_idx);
        } break;
        case (APP_CONNECT_STATE_DELETING):
        {
            // Go created state
            appm_update_actv_state(actv_idx, APP_ACTV_STATE_IDLE);
            app_env.connect_actv_idx = INVALID_BLE_ACTIVITY_INDEX;

            if (app_env.need_restart_connect)
            {
                app_env.need_restart_connect = false;
                appm_start_connecting(&app_env.ble_conn_param);
            }
            else if (app_env.need_set_white_list)
            {
                appm_set_white_list(&app_env.white_list_addr[0], app_env.white_list_size);
            }
            else
            {
                ble_execute_pending_op(BLE_ACTV_ACTION_STOP_CONNECTING, actv_idx);
            }
        } break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }
}

/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */
void appm_start_advertising(void *param)
{
    BLE_APP_FUNC_ENTER();

    BLE_ADV_PARAM_T* pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_idx = app_env.adv_actv_idx[pAdvParam->adv_actv_user];
    app_env.advParam = *pAdvParam;
    app_env.need_set_rsp_data = false;
    if (actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

        // Fill the allocated kernel message
    #ifdef BLE_USE_RPA
        p_cmd->own_addr_type = GAPM_GEN_RSLV_ADDR;
        p_cmd->adv_param.randomAddrRenewIntervalInSecond = (uint16_t)(60*15);
    #else
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
    #endif
        p_cmd->adv_param.type = GAPM_ADV_TYPE_LEGACY;
        p_cmd->adv_param.max_tx_pwr = btdrv_reg_op_txpwr_idx_to_rssidbm(pAdvParam->advTxPwr);
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
        p_cmd->adv_param.with_flags = pAdvParam->withFlags;

        switch (pAdvParam->advType)
        {
            case ADV_TYPE_UNDIRECT:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
                break;
            case ADV_TYPE_DIRECT_LDC:
                // Device must be non discoverable and non scannable if directed advertising is used
                p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
                p_cmd->adv_param.prop = GAPM_ADV_PROP_DIR_CONN_LDC_MASK;
                break;
            case ADV_TYPE_DIRECT_HDC:
                // Device must be non discoverable and non scannable if directed advertising is used
                p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
                p_cmd->adv_param.prop = GAPM_ADV_PROP_DIR_CONN_HDC_MASK;
                break;
            case ADV_TYPE_NON_CONN_SCAN:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_NON_CONN_SCAN_MASK;
                break;
            case ADV_TYPE_NON_CONN_NON_SCAN:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK;
                break;
            default:
                p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
                break;
        }

        memcpy(p_cmd->adv_param.local_addr.addr, pAdvParam->localAddr, GAP_BD_ADDR_LEN);
        memcpy(p_cmd->adv_param.peer_addr.addr, pAdvParam->peerAddr.addr, GAP_BD_ADDR_LEN);
        p_cmd->adv_param.peer_addr.addr_type = pAdvParam->peerAddr.addr_type;

        p_cmd->adv_param.filter_pol = pAdvParam->filter_pol;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;
        p_cmd->adv_param.prim_cfg.adv_intv_min = pAdvParam->advInterval * 8 / 5;
        p_cmd->adv_param.prim_cfg.adv_intv_max = pAdvParam->advInterval * 8 / 5;

        if (app_env.advParam.scanRspDataLen)
        {
            app_env.need_set_rsp_data = true;
        }

        if (pAdvParam->withFlags)
        {
            ASSERT(pAdvParam->advDataLen <= BLE_ADV_DATA_WITH_FLAG_LEN, "adv data exceed");
        }
        else
        {
            ASSERT(pAdvParam->advDataLen <= BLE_ADV_DATA_WITHOUT_FLAG_LEN, "adv data exceed");
        }
        ASSERT(pAdvParam->scanRspDataLen <= SCAN_RSP_DATA_LEN, "scan rsp data exceed");

        LOG_I("[ADDR_TYPE]:%d", p_cmd->own_addr_type);
        if (GAPM_GEN_RSLV_ADDR == p_cmd->own_addr_type)
        {
            LOG_I("[IRK]:");
            DUMP8("0x%02x ", appm_get_current_ble_irk(), KEY_LEN);
        }
        else if (GAPM_STATIC_ADDR == p_cmd->own_addr_type)
        {
            LOG_I("[ADDR]:");
            DUMP8("%02x ", bt_get_ble_local_address(), BT_ADDR_OUTPUT_PRINT_NUM);
        }

        LOG_I("[ADV_TYPE]:%d", pAdvParam->advType);
        LOG_I("[ADV_INTERVAL]:%d", pAdvParam->advInterval);

        // Send the message
        ke_msg_send(p_cmd);

        // Set the state of the task to APPM_ADVERTISING
        ke_state_set(TASK_APP, APPM_ADVERTISING);
    }
    else
    {
        app_env.need_restart_adv = true;
        appm_stop_advertising(actv_idx);
    }

    BLE_APP_FUNC_LEAVE();
}

void appm_start_connecting(BLE_CONN_PARAM_T *conn_param)
{
    if (GAPM_INIT_TYPE_DIRECT_CONN_EST == conn_param->gapm_init_type && 
        app_ble_is_connection_on_by_addr(conn_param->peer_addr.addr))
    {
        LOG_I("%s ble has connect", __func__);
        ble_execute_pending_op(BLE_ACTV_ACTION_CONNECTING, 0xFF);
        return;
    }

    app_env.ble_conn_param = *conn_param;

    if (app_env.connect_actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_INIT_ACTIVITY;

        // Fill the allocated kernel message
#ifdef BLE_USE_RPA
        p_cmd->own_addr_type = GAPM_GEN_RSLV_ADDR;
#else
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
#endif
        app_env.ble_conn_param.peer_addr.addr_type = p_cmd->own_addr_type;

        // Send the message
        ke_msg_send(p_cmd);
    }
    else if (APP_CONNECT_STATE_STARTED == app_env.state[app_env.connect_actv_idx])
    {
        app_env.need_restart_connect = true;
        appm_stop_connecting();
    }
    else if(APP_CONNECT_STATE_STOPPING == app_env.state[app_env.connect_actv_idx] || 
             APP_CONNECT_STATE_DELETING == app_env.state[app_env.connect_actv_idx])
    {
        app_env.need_restart_connect = true;
    }
}

void appm_start_scanning(uint16_t intervalInMs, uint16_t windowInMs, uint32_t filtPolicy)
{
    uint32_t oldFiltPolicy = app_env.scanFiltPolicy;
    app_env.scanIntervalInMs = intervalInMs;
    app_env.scanWindowInMs = windowInMs;
    app_env.scanFiltPolicy = filtPolicy;
    if (app_env.scan_actv_idx == INVALID_BLE_ACTIVITY_INDEX)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;

        // Fill the allocated kernel message
        if (app_env.scanFiltPolicy & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
        {
            p_cmd->own_addr_type = GAPM_GEN_RSLV_ADDR;
        }
        else
        {
            p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        }

        // Send the message
        ke_msg_send(p_cmd);
    }
    else
    {
        if ((app_env.scanFiltPolicy ^ oldFiltPolicy) & BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA)
        {
            app_env.need_restart_scan = true;
            appm_stop_scanning();
        }
        else
        {
            appm_kick_off_scanning();
        }
    }
}

void appm_update_param(uint8_t conidx, struct gapc_conn_param *conn_param)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, conidx),
                                                     TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = conn_param->intv_min;
    cmd->intv_max   = conn_param->intv_max;
    cmd->latency    = conn_param->latency;
    cmd->time_out   = conn_param->time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

void l2cap_update_param(uint8_t  conidx,
                        uint32_t min_interval_in_ms,
                        uint32_t max_interval_in_ms,
                        uint32_t supervision_timeout_in_ms,
                        uint8_t  slaveLatency)
{
    if (btif_is_gatt_over_br_edr_enabled() && btif_btgatt_is_connected_by_conidx(conidx))
    {
        LOG_I("l2cap_update_param failed because of btgatt conn!");
        return;
    }

    // TODO: update to new API
    // restore the param
    uint32_t lock = int_lock();
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);
    if (pContext->connPendindParam.con_interval != min_interval_in_ms ||
        pContext->connPendindParam.sup_to != supervision_timeout_in_ms ||
        pContext->connPendindParam.con_latency != slaveLatency)
    {
        app_env.context[conidx].conn_param_update_times = 0;
        pContext->connPendindParam.con_interval = min_interval_in_ms;
        pContext->connPendindParam.sup_to = supervision_timeout_in_ms;
        pContext->connPendindParam.con_latency = slaveLatency;
    }
    int_unlock(lock);

    // Requested connection parameters
    struct gapc_conn_param conn_param;

    /* fill up the parameters */
    conn_param.intv_min = (uint16_t)(min_interval_in_ms/1.25);
    conn_param.intv_max = (uint16_t)(max_interval_in_ms/1.25);
    conn_param.latency  = slaveLatency;
    conn_param.time_out = supervision_timeout_in_ms/10;

    LOG_I("%s val: 0x%x 0x%x 0x%x 0x%x",
          __func__,
          max_interval_in_ms,
          min_interval_in_ms,
          slaveLatency,
          supervision_timeout_in_ms);
    appm_update_param(conidx, &conn_param);
}

uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

void appm_exchange_mtu(uint8_t conidx)
{
    // TODO: update to new API
    /*
    struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
                                                 KE_BUILD_ID(TASK_GATTC, conidx),
                                                 TASK_APP,
                                                 gattc_exc_mtu_cmd);

    cmd->operation = GATTC_MTU_EXCH;
    cmd->seq_num= 0;

    ke_msg_send(cmd);
    */
}

void appm_check_and_resolve_ble_address(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T* pContext = &(app_env.context[conidx]);

    if (btif_is_gatt_over_br_edr_enabled() && btif_btgatt_is_connected_by_conidx(conidx))
    {
        pContext->isGotSolvedBdAddr = true;
        pContext->isBdAddrResolvingInProgress = false;
        btif_btgatt_get_device_address(conidx,pContext->solvedBdAddr);
    }

    // solved already, return
    if (pContext->isGotSolvedBdAddr)
    {
        LOG_I("Already get solved bd addr.");
        return;
    }
    // not solved yet and the solving is in progress, return and wait
    else if (app_is_resolving_ble_bd_addr())
    {
        LOG_I("Random bd addr solving on going.");
        return;
    }

    if (BLE_RANDOM_ADDR == pContext->peerAddrType)
    {
        memset(pContext->solvedBdAddr, 0, BD_ADDR_LEN);
        bool isSuccessful = appm_resolve_random_ble_addr_from_nv(conidx, pContext->bdAddr);
        LOG_I("%s isSuccessful %d", __func__, isSuccessful);
        if (isSuccessful)
        {
            pContext->isBdAddrResolvingInProgress = true;
        }
        else
        {
            pContext->isGotSolvedBdAddr = true;
            pContext->isBdAddrResolvingInProgress = false;
        }
    }
    else
    {
        pContext->isGotSolvedBdAddr = true;
        pContext->isBdAddrResolvingInProgress = false;
        memcpy(pContext->solvedBdAddr, pContext->bdAddr, BD_ADDR_LEN);
    }

}

bool appm_resolve_random_ble_addr_from_nv(uint8_t conidx, uint8_t* randomAddr)
{
#ifdef _BLE_NVDS_
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                                        KE_BUILD_ID(TASK_GAPM, conidx),
                                                        TASK_APP,
                                                        gapm_resolv_addr_cmd,
                                                        BLE_RECORD_NUM * GAP_KEY_LEN);

    uint8_t irkeyNum = nv_record_ble_fill_irk((uint8_t *)(cmd->irk));
    if (0 == irkeyNum)
    {
        LOG_I("No history irk, cannot solve bd addr.");
        KE_MSG_FREE(cmd);
        return false;
    }

    LOG_I("Start random bd addr solving.");

    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = irkeyNum;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    ke_msg_send(cmd);
    return true;
#else
    return false;
#endif

}

void appm_resolve_random_ble_addr_with_sepcific_irk(uint8_t conidx, uint8_t* randomAddr, uint8_t* pIrk)
{
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                       KE_BUILD_ID(TASK_GAPM, conidx), TASK_APP,
                                       gapm_resolv_addr_cmd,
                                       GAP_KEY_LEN);
    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = 1;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    memcpy(cmd->irk, pIrk, GAP_KEY_LEN);
    ke_msg_send(cmd);
}

void appm_random_ble_addr_solved(bool isSolvedSuccessfully, uint8_t* irkUsedForSolving)
{
    APP_BLE_CONN_CONTEXT_T* pContext;
    uint32_t conidx;
    for (conidx = 0;conidx < BLE_CONNECTION_MAX;conidx++)
    {
        pContext = &(app_env.context[conidx]);
        if (pContext->isBdAddrResolvingInProgress)
        {
            break;
        }
    }

    if (conidx < BLE_CONNECTION_MAX)
    {
        pContext->isBdAddrResolvingInProgress = false;
        pContext->isGotSolvedBdAddr = true;

        LOG_I("%s conidx %d isSolvedSuccessfully %d", __func__, conidx, isSolvedSuccessfully);
        if (isSolvedSuccessfully)
        {
#ifdef _BLE_NVDS_
            bool isSuccessful = nv_record_blerec_get_bd_addr_from_irk(app_env.context[conidx].solvedBdAddr, irkUsedForSolving);
            if (isSuccessful)
            {
                LOG_I("[CONNECT]Connected random address's original addr is:");
                DUMP8("%02x ", app_env.context[conidx].solvedBdAddr, BT_ADDR_OUTPUT_PRINT_NUM);
            }
            else
#endif
            {
                LOG_I("[CONNECT]Resolving of the connected BLE random addr failed.");
            }
        }
        else
        {
            LOG_I("[CONNECT]random resolving failed.");
        }
    }

#if defined(CFG_VOICEPATH)
    ke_task_msg_retrieve(prf_get_task_from_id(TASK_ID_VOICEPATH));
#endif
    ke_task_msg_retrieve(TASK_GAPC);
    ke_task_msg_retrieve(TASK_APP);

    app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

uint8_t app_ble_connection_count(void)
{
    return app_env.conn_cnt;
}

bool app_is_arrive_at_max_ble_connections(void)
{
    LOG_I("connection count %d", app_env.conn_cnt);
    
#if defined(TILE_DATAPATH)
    return (app_env.conn_cnt >= 1)
#else
    return (app_env.conn_cnt >= BLE_CONNECTION_MAX);
#endif
}

bool app_is_resolving_ble_bd_addr(void)
{
    for (uint32_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (app_env.context[index].isBdAddrResolvingInProgress)
        {
            return true;
        }
    }

    return false;
}

void app_trigger_ble_service_discovery(uint8_t conidx, uint16_t shl, uint16_t ehl)
{
    // TODO: update to new API
    /*
    struct gattc_send_svc_changed_cmd *cmd= KE_MSG_ALLOC(GATTC_SEND_SVC_CHANGED_CMD,\
                                                            KE_BUILD_ID(TASK_GATTC, conidx),\
                                                            TASK_APP,\
                                                            gattc_send_svc_changed_cmd);
    cmd->operation = GATTC_SVC_CHANGED;
    cmd->svc_shdl = shl;
    cmd->svc_ehdl = ehl;
    ke_msg_send(cmd);
    */
}

void appm_update_adv_data(void *param)
{
    // TODO: update to new API
    BLE_ADV_PARAM_T* pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_idx = app_env.adv_actv_idx[pAdvParam->adv_actv_user];
    app_env.need_set_rsp_data = false;

    ASSERT(actv_idx != INVALID_BLE_ACTIVITY_INDEX,
            "%s adv idx %d", __func__, actv_idx);
    ASSERT(app_env.state[actv_idx] == APP_ADV_STATE_STARTED,
            "%s state %d", __func__, app_env.state[actv_idx]);

    // if scan rsp data change, need set rsp data again
    if (pAdvParam->scanRspDataLen != app_env.advParam.scanRspDataLen ||
        memcmp(pAdvParam->scanRspData, app_env.advParam.scanRspData, pAdvParam->scanRspDataLen))
    {
        app_env.need_set_rsp_data = true;
    }

    app_env.advParam = *pAdvParam;
    app_env.need_update_adv = true;

    appm_set_adv_data(actv_idx);
}

__attribute__((weak)) void app_ble_connected_evt_handler(uint8_t conidx, const uint8_t* pPeerBdAddress)
{

}

__attribute__((weak)) void app_ble_disconnected_evt_handler(uint8_t conidx, uint8_t errCode)
{

}

__attribute__((weak)) void app_advertising_stopped(uint8_t actv_idx)
{

}

__attribute__((weak)) void app_advertising_started(uint8_t actv_idx)
{

}

__attribute__((weak)) void app_connecting_started(void)
{

}

__attribute__((weak)) void app_scanning_stopped(void)
{

}

__attribute__((weak)) void app_connecting_stopped(gap_bdaddr_t *peer_addr)
{

}


__attribute__((weak)) void app_scanning_started(void)
{

}

__attribute__((weak)) void app_ble_system_ready(void)
{

}

__attribute__((weak)) void app_adv_reported_scanned(struct gapm_ext_adv_report_ind* ptInd)
{

}

__attribute__((weak)) void app_ble_update_param_failed(uint8_t conidx, uint8_t errCode)
{

}

__attribute((weak)) void app_ble_on_bond_success(uint8_t conidx)
{

}

uint8_t* appm_get_current_ble_addr(void)
{
#ifdef BLE_USE_RPA
    return (uint8_t *)gapm_get_bdaddr();
#else
    return ble_addr;
#endif
}

uint8_t* appm_get_local_identity_ble_addr(void)
{
    return ble_addr;
}

// bit mask of the existing conn param modes
static uint32_t existingBleConnParamModes[BLE_CONNECTION_MAX] = {0};

static BLE_CONN_PARAM_CONFIG_T ble_conn_param_config[] =
{
    // default value: for the case of BLE just connected and the BT idle state
    {BLE_CONN_PARAM_MODE_DEFAULT, BLE_CONN_PARAM_PRIORITY_NORMAL, 36},

    {BLE_CONN_PARAM_MODE_AI_STREAM_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL1, 36},

    //{BLE_CONN_PARAM_MODE_A2DP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL0, 36},

    {BLE_CONN_PARAM_MODE_HFP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 36},

    {BLE_CONN_PARAM_MODE_OTA, BLE_CONN_PARAM_PRIORITY_HIGH, 12},

    {BLE_CONN_PARAM_MODE_OTA_SLOWER, BLE_CONN_PARAM_PRIORITY_HIGH, 20},

    {BLE_CONN_PARAM_MODE_SNOOP_EXCHANGE, BLE_CONN_PARAM_PRIORITY_HIGH, 8},

    {BLE_CONN_PARAM_MODE_BLE_AUDIO, BLE_CONN_PARAM_PRIORITY_HIGH, 360},

    // TODO: add mode cases if needed
};

void app_ble_reset_conn_param_mode(uint8_t conidx)
{
    uint32_t lock = int_lock_global();
    existingBleConnParamModes[conidx] = 0;
    int_unlock_global(lock);
}

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {        
        if (BLE_CONNECTED == app_env.context[index].connectStatus)
        {
            app_ble_update_conn_param_mode_of_specific_connection(
                index, mode, isEnable);
        }
    }
}

void app_ble_update_conn_param_mode_of_specific_connection(uint8_t conidx, BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
    ASSERT(mode < BLE_CONN_PARAM_MODE_NUM, "Wrong ble conn param mode %d!", mode);

    if (btif_is_gatt_over_br_edr_enabled() && btif_btgatt_is_connected_by_conidx(conidx))
    {
        LOG_I("conn param mode update failed because of btgatt conn!");
        return;
    }

    uint32_t lock = int_lock_global();

    // locate the conn param mode
    BLE_CONN_PARAM_CONFIG_T *pConfig = NULL;
    uint8_t index;

    for (index = 0;
         index < sizeof(ble_conn_param_config) / sizeof(BLE_CONN_PARAM_CONFIG_T);
         index++)
    {
        if (mode == ble_conn_param_config[index].ble_conn_param_mode)
        {
            pConfig = &ble_conn_param_config[index];
            break;
        }
    }

    if (NULL == pConfig)
    {
        int_unlock_global(lock);
        LOG_I("conn param mode %d not defined!", mode);
        return;
    }

    if (isEnable)
    {
        if (0 == existingBleConnParamModes[conidx])
        {
            // no other params existing, just configure this one
            existingBleConnParamModes[conidx] = 1 << mode;
        }
        else
        {
            // already existing, directly return
            if (existingBleConnParamModes[conidx]&(1 << mode))
            {
                int_unlock_global(lock);
                return;
            }
            else
            {
                // update the bit-mask 
                existingBleConnParamModes[conidx] |= (1 << mode);

                // not existing yet, need to go throuth the existing params to see whether
                // we need to update the param
                for (index = 0;
                    index < sizeof(ble_conn_param_config)/sizeof(BLE_CONN_PARAM_CONFIG_T);
                    index++)
                {
                    if ((( uint32_t )1 << ( uint8_t )ble_conn_param_config[index].ble_conn_param_mode) &
                        existingBleConnParamModes[conidx])
                    {
                        if (ble_conn_param_config[index].priority > pConfig->priority)
                        {
                            // one of the exiting param has higher priority than this one,
                            // so do nothing but update the bit-mask
                            int_unlock_global(lock);
                            return;
                        }
                    }
                }

                // no higher priority conn param existing, so we need to apply this one
            }
        }
    }
    else
    {
        if (0 == existingBleConnParamModes[conidx])
        {
            // no other params existing, just return
            int_unlock_global(lock);
            return;
        }
        else 
        {
            // doesn't exist, directly return
            if (!(existingBleConnParamModes[conidx]&(1 << mode)))
            {
                int_unlock_global(lock);
                return;
            }
            else
            {
                // update the bit-mask 
                existingBleConnParamModes[conidx] &= (~(1 << mode));

                if (0 == existingBleConnParamModes[conidx])
                {
                    int_unlock_global(lock);
                    return;
                }

                pConfig = NULL;

                // existing, need to apply for the highest priority conn param
                for (index = 0;
                     index < sizeof(ble_conn_param_config) / sizeof(BLE_CONN_PARAM_CONFIG_T);
                     index++)
                {
                    if ((( uint32_t )1 << ( uint8_t )ble_conn_param_config[index].ble_conn_param_mode) &
                        existingBleConnParamModes[conidx])
                    {
                        if (NULL != pConfig)
                        {
                            if (ble_conn_param_config[index].priority > pConfig->priority)
                            {
                                pConfig = &ble_conn_param_config[index];
                            }
                        }
                        else
                        {
                            pConfig = &ble_conn_param_config[index];
                        }
                    }
                }
            }
        }
    }

    int_unlock_global(lock);

    // if we can arrive here, it means we have got one config to apply
    ASSERT(NULL != pConfig, "It's strange that config pointer is still NULL.");

    APP_BLE_CONN_CONTEXT_T* pContext = &(app_env.context[conidx]);

    if (pContext->connParam.con_interval != pConfig->conn_param_interval)
    {
        l2cap_update_param(conidx, pConfig->conn_param_interval*10/8,
                            pConfig->conn_param_interval*10/8,
                            BLE_CONN_PARAM_SUPERVISE_TIMEOUT_MS,
                            BLE_CONN_PARAM_SLAVE_LATENCY_CNT);
        LOG_I("new conn interval %d", pConfig->conn_param_interval);
    }

    LOG_I("conn param mode switched to %d:", mode);

}

bool app_ble_get_conn_param(uint8_t conidx,  APP_BLE_CONN_PARAM_T* pConnParam)
{
    if ((conidx < BLE_CONNECTION_MAX) &&
        (BLE_CONNECTED == app_env.context[conidx].connectStatus))
    {
        *pConnParam = app_env.context[conidx].connParam;
        return true;
    }
    else
    {
        return false;
    }    
}

#if GFPS_ENABLED
uint8_t delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
#define FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE      (10000)
osTimerId fp_update_ble_param_timer = NULL;
static void fp_update_ble_connect_param_timer_handler(void const *param);
osTimerDef (FP_UPDATE_BLE_CONNECT_PARAM_TIMER, (void (*)(void const *))fp_update_ble_connect_param_timer_handler);
extern uint8_t is_sco_mode (void);
static void fp_update_ble_connect_param_timer_handler(void const *param)
{
    LOG_I("fp_update_ble_connect_param_timer_handler");
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if ((BLE_CONNECTED == app_env.context[index].connectStatus) &&
            (index == delay_update_conidx))
        {
            LOG_I("update connection interval of conidx %d", delay_update_conidx);

            if (is_sco_mode())
            {
                app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_HFP_ON, true);
            }
            else
            {
                app_ble_update_conn_param_mode_of_specific_connection(delay_update_conidx, BLE_CONN_PARAM_MODE_DEFAULT, true);
            }
            break;
        }
    }
    delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
}

void fp_update_ble_connect_param_start(uint8_t ble_conidx)
{
    if (fp_update_ble_param_timer == NULL)
    {
        fp_update_ble_param_timer = osTimerCreate(osTimer(FP_UPDATE_BLE_CONNECT_PARAM_TIMER), osTimerOnce, NULL);
        return;
    }

    delay_update_conidx = ble_conidx;
    if (fp_update_ble_param_timer)
        osTimerStart(fp_update_ble_param_timer, FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE);
}

void fp_update_ble_connect_param_stop(uint8_t ble_conidx)
{
    if (delay_update_conidx == ble_conidx)
    {
        if (fp_update_ble_param_timer)
            osTimerStop(fp_update_ble_param_timer);
        delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
    }
}
#endif

bool app_ble_is_connection_on_by_addr(uint8_t *addr)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (!memcmp(app_env.context[i].bdAddr, addr, BD_ADDR_LEN))
        {
            if (BLE_CONNECTED == app_env.context[i].connectStatus)
            {
                return true;
            }
        }
    }

    return false;
}

uint8_t* app_ble_get_peer_addr(uint8_t conidx)
{
    return app_env.context[conidx].bdAddr;
}

bool app_ble_is_connection_on_by_index(uint8_t conidx)
{
    return (BLE_CONNECTED == app_env.context[conidx].connectStatus);
}

void app_init_ble_name(const char *name)
{
    ASSERT(name, "%s null name ptr received", __func__);

    /// Load the device name from NVDS
    uint32_t nameLen = co_min(strlen(name) + 1, APP_DEVICE_NAME_MAX_LEN);

    // Get default Device Name (No name if not enough space)
    memcpy(app_env.dev_name, name, nameLen);
    app_env.dev_name[nameLen - 1] = '\0';
    app_env.dev_name_len = nameLen;
    LOG_I("device ble name:%s", app_env.dev_name);
}

__attribute__((weak)) uint32_t app_sec_get_P256_key(uint8_t * out_public_key,uint8_t * out_private_key)
{
    TRACE(0, "%s", __func__);
#ifdef CFG_SEC_CON
    memcpy(out_private_key, ble_private_key, sizeof(ble_private_key));
    memcpy(out_public_key, ble_public_key, sizeof(ble_public_key));
#endif
    return 0;
}

#endif //(BLE_APP_PRESENT)
/// @} APP
