/***************************************************************************
*
*Copyright 2015-2021 BES.
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
#include "app_a2dp.h"
#include "app_thread.h"
#include "app_utils.h"
#include "bluetooth.h"
#include "bt_drv_interface.h"
#include "apps.h"
#include "gapc_msg.h"            // GAP Controller Task API
#include "gapm_msg.h"            // GAP Manager Task API
#include "app.h"
#include "app_sec.h"
#include "app_ble_mode_switch.h"
#include "app_ble_param_config.h"
#include "app_ble_custom_api.h"

typedef struct
{
    bool user_enable;
    BLE_ADV_PARAM_T adv_param;
} CUSTOMER_ADV_PARAM_T;

CUSTOMER_ADV_PARAM_T customer_adv_param[BLE_ADV_ACTIVITY_USER_NUM];

void app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_E actv_user,
                    bool is_custom_adv_flags,
                    uint8_t *local_addr,
                    ble_bdaddr_t *peer_addr,
                    uint16_t adv_interval,
                    BLE_ADV_TYPE_E adv_type,
                    int8_t tx_power_dbm,
                    uint8_t *adv_data, uint8_t adv_data_size,
                    uint8_t *scan_rsp_data, uint8_t scan_rsp_data_size)
{
    BLE_ADV_PARAM_T *adv_param_p = NULL;

    LOG_I("%s user %d", __func__, actv_user);
    if (actv_user > BLE_ADV_ACTIVITY_USER_NUM) {
        LOG_E("%s actv_user shouled be less than %d ", __func__, BLE_ADV_ACTIVITY_USER_NUM);
        return;
    }
    app_ble_set_adv_txpwr_by_actv_user(actv_user, tx_power_dbm);

    adv_param_p = &customer_adv_param[actv_user].adv_param;
    memset(adv_param_p, 0, sizeof(BLE_ADV_PARAM_T));

    adv_param_p->adv_actv_user = actv_user;
    adv_param_p->withFlags = is_custom_adv_flags?false:true;
    if (local_addr)
    {
        memcpy(adv_param_p->localAddr, local_addr, BTIF_BD_ADDR_SIZE);
    }
    if(peer_addr)
    {
        memcpy(&adv_param_p->peerAddr, peer_addr, sizeof(ble_bdaddr_t));
    }
    adv_param_p->advInterval = adv_interval;
    adv_param_p->advType = adv_type;
    if (adv_data && adv_data_size)
    {
        memcpy(adv_param_p->advData, adv_data, adv_data_size);
        adv_param_p->advDataLen = adv_data_size;
    }
    if (scan_rsp_data && scan_rsp_data_size)
    {
        memcpy(adv_param_p->scanRspData, scan_rsp_data, scan_rsp_data_size);
        adv_param_p->scanRspDataLen = scan_rsp_data_size;
    }
}

void app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    LOG_I("%s user %d", __func__, actv_user);
    customer_adv_param[actv_user].user_enable = true;
    app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

void app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    LOG_I("%s user %d", __func__, actv_user);
    customer_adv_param[actv_user].user_enable = false;
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

static void app_ble_custom_fill_adv_param(BLE_ADV_PARAM_T *dst, BLE_ADV_PARAM_T *src)
{
    uint8_t empty_addr[BTIF_BD_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    dst->adv_actv_user = src->adv_actv_user;
    dst->withFlags = src->withFlags;
    dst->filter_pol = src->filter_pol;
    dst->advType = src->advType;
    dst->advInterval = src->advInterval;

    memcpy(&dst->advData[dst->advDataLen],
        src->advData, src->advDataLen);
    dst->advDataLen += src->advDataLen;
    memcpy(&dst->scanRspData[dst->scanRspDataLen],
        src->scanRspData, src->scanRspDataLen);
    dst->scanRspDataLen += src->scanRspDataLen;

    if (memcmp(src->localAddr, empty_addr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(dst->localAddr, src->localAddr, BTIF_BD_ADDR_SIZE);
    }

    if (memcmp(src->peerAddr.addr, empty_addr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(&dst->peerAddr, &src->peerAddr, sizeof(ble_bdaddr_t));
    }
}

static void app_ble_custom_0_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_0];

    if (custom_adv_param_p->user_enable)
    {
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, &custom_adv_param_p->adv_param);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_0, custom_adv_param_p->user_enable);
}

static void app_ble_custom_0_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_0, (BLE_DATA_FILL_FUNC_T)app_ble_custom_0_user_data_fill_handler, false);
}

static void app_ble_custom_1_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_1];

    if (custom_adv_param_p->user_enable)
    {
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, &custom_adv_param_p->adv_param);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_1, custom_adv_param_p->user_enable);
}

static void app_ble_custom_1_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_1, (BLE_DATA_FILL_FUNC_T)app_ble_custom_1_user_data_fill_handler, false);
}

static void app_ble_custom_2_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    CUSTOMER_ADV_PARAM_T *custom_adv_param_p = &customer_adv_param[BLE_ADV_ACTIVITY_USER_2];

    if (custom_adv_param_p->user_enable)
    {
        app_ble_custom_fill_adv_param((BLE_ADV_PARAM_T *)param, &custom_adv_param_p->adv_param);
    }

    app_ble_data_fill_enable(USER_BLE_CUSTOMER_2, custom_adv_param_p->user_enable);
}

static void app_ble_custom_2_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_CUSTOMER_2, (BLE_DATA_FILL_FUNC_T)app_ble_custom_2_user_data_fill_handler, false);
}

void app_ble_custom_init(void)
{
    static bool ble_custom_inited = false;

    if (!ble_custom_inited)
    {
        LOG_I("%s", __func__);
        ble_custom_inited = true;
        for (uint8_t actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            memset(&customer_adv_param[actv_user], 0, sizeof(CUSTOMER_ADV_PARAM_T));
        }
        app_ble_custom_0_user_init();
        app_ble_custom_1_user_init();
        app_ble_custom_2_user_init();
    }
}

