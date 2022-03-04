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
#include "bluetooth.h"
#include "hal_cmu.h"
#include "app_tota_cmd_code.h"
#include "app_tota.h"
#include "app_tota_cmd_handler.h"
#include "cmsis.h"
#include "app_hfp.h"
#include "app_key.h"
#include "app_tota_general.h"
#include "app_spp_tota.h"
#include "nvrecord_ble.h"
#include "app_ibrt_rssi.h"
#include "app_tota_if.h"
#if defined(TOTA_EQ_TUNING)
#include "hal_cmd.h"
#endif

/*
** general info struct
**  ->  bt  name
**  ->  ble name
**  ->  bt  local/peer addr
**  ->  ble local/peer addr
**  ->  ibrt role
**  ->  crystal freq
**  ->  xtal fcap
**  ->  bat volt/level/status
**  ->  fw version
**  ->  ear location
**  ->  rssi info
*/

/*------------------------------------------------------------------------------------------------------------------------*/
typedef struct{
    uint32_t    address;
    uint32_t    length;
}TOTA_DUMP_INFO_STRUCT_T;


static void general_get_flash_dump_info(TOTA_DUMP_INFO_STRUCT_T * p_flash_info);

/*
**  general info
*/
static general_info_t general_info;


/*
**  get general info
*/
static void __get_general_info();

/*
**  handle general cmd
*/
static void __tota_general_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen);

/*------------------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------------------*/

static void __tota_general_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG(3,"[%s]: opCode:0x%x, paramLen:%d", __func__, funcCode, paramLen);
    DUMP8("%02x ", ptrParam, paramLen);
    uint8_t resData[60]={0};
    uint32_t resLen=1;
    uint8_t volume_level;
    switch (funcCode)
    {
        case OP_TOTA_GENERAL_INFO_CMD:
            __get_general_info();
            app_tota_send_rsp(funcCode,TOTA_NO_ERROR,(uint8_t*)&general_info, sizeof(general_info_t));
            return ;
        case OP_TOTA_GET_RSSI_CMD:
            app_ibrt_rssi_get_stutter(resData, &resLen);
            app_tota_send_rsp(funcCode,TOTA_NO_ERROR,resData, resLen);
            return;
        case OP_TOTA_MERIDIAN_EFFECT_CMD:
            __get_general_info();
            // resData[0] = app_meridian_eq(ptrParam[0]);
            break;
        case OP_TOTA_EQ_SELECT_CMD:
            break;
        case OP_TOTA_VOLUME_PLUS_CMD:
            app_bt_volumeup();
            volume_level = app_bt_stream_local_volume_get();
            // resData[0] = volume_level;
            TRACE(1,"volume = %d",volume_level);
            break;
        case OP_TOTA_VOLUME_DEC_CMD:
            app_bt_volumedown();
            volume_level = app_bt_stream_local_volume_get();
            // resData[0] = volume_level;
            // resLen = 1;
            TRACE(1,"volume = %d",volume_level);
            break;
        case OP_TOTA_VOLUME_SET_CMD:
            //uint8_t scolevel = ptrParam[0];
            //uint8_t a2dplevel = ptrParam[1];
            app_bt_set_volume(APP_BT_STREAM_HFP_PCM,ptrParam[0]);
            app_bt_set_volume(APP_BT_STREAM_A2DP_SBC,ptrParam[1]);
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case OP_TOTA_VOLUME_GET_CMD:
            // resData[0] = app_bt_stream_hfpvolume_get();
            // resData[1] = app_bt_stream_a2dpvolume_get();
            // resLen = 2;
            break;
        case OP_TOTA_EQ_SET_CMD: {
            TRACE(0, "OP_TOTA_EQ_SET_CMD...");
#if defined(TOTA_EQ_TUNING)
            uint16_t len = 0;
            hal_cmd_list_process(ptrParam);
            hal_cmd_tx_process(&ptrParam, &len);
            ASSERT(len <= sizeof(resData), "[%s] len(%d) > sizeof(resData)", __func__, len);
            resLen = len;
            memcpy(resData, ptrParam, resLen);
            TRACE(0, "[%s] resLen: %d", __func__, resLen);
            DUMP8("%02x ", resData, resLen);
#endif
            break;
        }
        case OP_TOTA_EQ_GET_CMD:
            TRACE(0, "OP_TOTA_EQ_GET_CMD...");
#if defined(TOTA_EQ_TUNING)

#endif
            break;
        case OP_TOTA_RAW_DATA_SET_CMD:
            // app_ibrt_debug_parse(ptrParam, paramLen);
            break;
        case OP_TOTA_GET_DUMP_INFO_CMD:
        {
            TOTA_DUMP_INFO_STRUCT_T dump_info;
            general_get_flash_dump_info(&dump_info);
            app_tota_send_rsp(funcCode,TOTA_NO_ERROR,(uint8_t*)&dump_info, sizeof(TOTA_DUMP_INFO_STRUCT_T));
            return ;
        }
        case OP_TOTA_SET_CUSTOMER_CMD:
            TRACE(0, "[%s] OP_TOTA_SET_CUSTOMER_CMD ...", __func__);
            break;
        case OP_TOTA_GET_CUSTOMER_CMD:
            TRACE(0, "[%s] OP_TOTA_GET_CUSTOMER_CMD ...", __func__);
            break;
        default:
            // TRACE(1,"wrong cmd 0x%x",funcCode);
            // resData[0] = -1;
            return;
    }
    app_tota_send_rsp(funcCode,TOTA_NO_ERROR,resData,resLen);
}

#if defined(__XSPACE_BATTERY_MANAGER__)
extern "C" uint16_t xspace_battery_manager_bat_get_voltage(void);
extern "C" uint8_t xspace_battery_manager_bat_get_level(void);
#endif

/* get general info */
static void __get_general_info()
{
    /* get bt-ble name */
    uint8_t* factory_name_ptr =factory_section_get_bt_name();
    if ( factory_name_ptr != NULL )
    {
        uint16_t valid_len = strlen((char*)factory_name_ptr) > BT_BLE_LOCAL_NAME_LEN? BT_BLE_LOCAL_NAME_LEN:strlen((char*)factory_name_ptr);
        memcpy(general_info.btName,factory_name_ptr,valid_len);
    }

    factory_name_ptr =factory_section_get_ble_name();
    if ( factory_name_ptr != NULL )
    {
        uint16_t valid_len = strlen((char*)factory_name_ptr) > BT_BLE_LOCAL_NAME_LEN? BT_BLE_LOCAL_NAME_LEN:strlen((char*)factory_name_ptr);
        memcpy(general_info.bleName,factory_name_ptr,valid_len);
    }

    /* get bt-ble peer addr */
//    ibrt_config_t addrInfo;
//    app_ibrt_ui_test_config_load(&addrInfo);
//    general_info.ibrtRole = addrInfo.nv_role;
//    memcpy(general_info.btLocalAddr.address, addrInfo.local_addr.address, 6);
//    memcpy(general_info.btPeerAddr.address, addrInfo.peer_addr.address, 6);

    #ifdef BLE
    memcpy(general_info.bleLocalAddr.address, bt_get_ble_local_address(), 6);
    memcpy(general_info.blePeerAddr.address, nv_record_tws_get_peer_ble_addr(), 6);
    #endif

    /* get crystal info */
    general_info.crystal_freq = hal_cmu_get_crystal_freq();

    /* factory_section_xtal_fcap_get */
    factory_section_xtal_fcap_get(&general_info.xtal_fcap);
    
    /* get battery info (volt level)*/
#if !defined(__XSPACE_BATTERY_MANAGER__)
    app_battery_get_info(&general_info.battery_volt,&general_info.battery_level,&general_info.battery_status);
#else
    general_info.battery_volt = xspace_battery_manager_bat_get_voltage();
    general_info.battery_level = xspace_battery_manager_bat_get_level();
#endif
    /* get firmware version */
#ifdef FIRMWARE_REV
    system_get_info(&general_info.fw_version[0],&general_info.fw_version[1],&general_info.fw_version[2],&general_info.fw_version[3]);
    TRACE(4,"firmware version = %d.%d.%d.%d",general_info.fw_version[0],general_info.fw_version[1],general_info.fw_version[2],general_info.fw_version[3]);
#endif

    /* get ear location info */
    if ( app_ibrt_if_is_right_side() )      general_info.ear_location = EAR_SIDE_RIGHT;
    else if ( app_ibrt_if_is_left_side() )  general_info.ear_location = EAR_SIDE_LEFT;
    else                                general_info.ear_location = EAR_SIDE_UNKNOWN;

    general_info.rssi[0] = app_tota_get_rssi_value();
    general_info.rssi_len = 1;
}


/* general command */
TOTA_COMMAND_TO_ADD(OP_TOTA_GENERAL_INFO_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_MERIDIAN_EFFECT_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SELECT_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_PLUS_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_DEC_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_SET_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_GET_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SET_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_GET_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_RAW_DATA_SET_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_GET_RSSI_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_GET_DUMP_INFO_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_SET_CUSTOMER_CMD, __tota_general_cmd_handle, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_GET_CUSTOMER_CMD, __tota_general_cmd_handle, false, 0, NULL );

#ifdef DUMP_LOG_ENABLE
extern uint32_t __log_dump_start[];
#endif

static void general_get_flash_dump_info(TOTA_DUMP_INFO_STRUCT_T * p_flash_info)
{
#ifdef DUMP_LOG_ENABLE
    p_flash_info->address =  (uint32_t)&__log_dump_start;
    p_flash_info->length = LOG_DUMP_SECTION_SIZE;
#else
    p_flash_info->address =  0;
    p_flash_info->length = 0;
#endif
}
