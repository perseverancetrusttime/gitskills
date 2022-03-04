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

#ifdef SWIFT_ENABLED
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "cmsis_os.h"
#include "app.h"  // Application Manager Definitions
#include "apps.h"
#include "gfps_provider_task.h"  // Device Information Profile Functions
#include "prf_types.h"           // Profile Common Types Definitions
#include <string.h>
#include "gap.h"
#include "app_ble_mode_switch.h"
#include "gapm.h"
#include "me_api.h"
#include "app_bt.h"
#include "bt_if.h"
#ifdef IBRT
#include "app_tws_ibrt.h"
#include "app_ibrt_if.h"
#include "app_ibrt_customif_cmd.h"
#include "app_tws_ibrt_cmd_handler.h"
#endif

/************************private macro defination***************************/
#define DISPLAY_ICON_LEN            3
#define DISPLAY_ICON                0x040424
#define SWIFT_MS_HEADER_LEN         6
#define SWIFT_MS_VENDER_ID          0xff0600
#define SWIFT_MS_BEACON             0x030180

#define DISPLAY_NAME_LEN            4
char display_name[DISPLAY_NAME_LEN] = "RC05";

#define BLE_SWIFT_ADVERTISING_INTERVAL (160)

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            swift_ble_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE advertisement and scan response data fill handler for Google fast pair
 *
 * Parameters:
 *    param - pointer of BLE parameter to be configure
 *
 * Return:
 *    void
 */
static void swift_ble_data_fill_handler(void *param)
{
    bool adv_enable = false;
    TRACE(1,"[%s]+++", __func__);
    ASSERT(param, "invalid param");

    BLE_ADV_PARAM_T *advInfo = ( BLE_ADV_PARAM_T * )param;
    TRACE(2,"adv data offset:%d, scan response data offset:%d",
          advInfo->advDataLen,
          advInfo->scanRspDataLen);

#ifdef IBRT
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_role_e  role = app_ibrt_if_get_ui_role();
    bool tws_connected = app_ibrt_conn_is_tws_connected();
    TRACE(1,"current role:%s, tws_connected=%d", app_tws_ibrt_role2str(role),tws_connected);

    if (IBRT_MASTER == role && p_ibrt_ctrl->init_done /*&& tws_connected*/)
#endif
    {
        TRACE(0,"swift_ble_data_fill_handler");
        advInfo->advUserInterval[USER_SWIFT] = BLE_SWIFT_ADVERTISING_INTERVAL;

        advInfo->advData[advInfo->advDataLen++] = DISPLAY_NAME_LEN + sizeof(bt_bdaddr_t) + DISPLAY_ICON_LEN + SWIFT_MS_HEADER_LEN;
        advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_VENDER_ID >> 16) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_VENDER_ID >> 8) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = SWIFT_MS_VENDER_ID  & 0xFF;
        
        advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_BEACON >> 16) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = (SWIFT_MS_BEACON >> 8) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = SWIFT_MS_BEACON  & 0xFF;

        memcpy(advInfo->advData+advInfo->advDataLen,p_ibrt_ctrl->local_addr.address, sizeof(bt_bdaddr_t));
        advInfo->advDataLen += sizeof(bt_bdaddr_t);

        advInfo->advData[advInfo->advDataLen++] = (DISPLAY_ICON >> 16) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = (DISPLAY_ICON >> 8) & 0xFF;
        advInfo->advData[advInfo->advDataLen++] = DISPLAY_ICON  & 0xFF;

        memcpy(advInfo->advData+advInfo->advDataLen,(void *)display_name, DISPLAY_NAME_LEN);
        advInfo->advDataLen += DISPLAY_NAME_LEN;

        adv_enable = true;

    }
    app_ble_data_fill_enable(USER_SWIFT, adv_enable);

    TRACE(1,"[%s]---", __func__);
}


/****************************function defination****************************/
void app_swift_init(void)
{ 
    app_ble_register_data_fill_handle(USER_SWIFT, ( BLE_DATA_FILL_FUNC_T )swift_ble_data_fill_handler, false);
}


#endif
/// @} APP
