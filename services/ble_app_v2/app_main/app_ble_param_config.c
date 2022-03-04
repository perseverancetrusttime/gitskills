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
#include "app_a2dp.h"
#include "app_thread.h"
#include "app_utils.h"
#include "bluetooth.h"
#include "apps.h"
#include "gapc_msg.h"            // GAP Controller Task API
#include "gapm_msg.h"            // GAP Manager Task API
#include "app.h"
#include "app_sec.h"
#include "app_ble_mode_switch.h"
#include "app_ble_param_config.h"

extern uint8_t is_sco_mode (void);
#define APP_DEMO_UUID0            "\x03\x17\x03\xFE"
#define APP_DEMO_UUID0_LEN        (4)
#define APP_DEMO_UUID1            "\x03\x18\x03\xFE"
#define APP_DEMO_UUID1_LEN        (4)

// common used function
#ifdef USE_LOG_I_ID
char *ble_adv_user2str(BLE_ADV_USER_E user)
{
    return (char *)(uint32_t)user;
}

char *ble_adv_intv_user2str(BLE_ADV_INTERVALREQ_USER_E user)
{
    return (char *)(uint32_t)user;
}

#else
#define CASE_S(s) \
    case s:       \
        return "[" #s "]";
#define CASE_D() \
    default:     \
        return "[INVALID]";

char *ble_adv_user2str(BLE_ADV_USER_E user)
{
    switch (user)
    {
        CASE_S(USER_STUB)
        CASE_S(USER_GFPS)
        CASE_S(USER_SWIFT)
        CASE_S(USER_GSOUND)
        CASE_S(USER_AI)
        CASE_S(USER_INTERCONNECTION)
        CASE_S(USER_TILE)
        CASE_S(USER_OTA)
        CASE_S(USER_BLE_AUDIO)
        CASE_S(USER_BLE_CUSTOMER_0)
        CASE_S(USER_BLE_CUSTOMER_1)
        CASE_S(USER_BLE_CUSTOMER_2)
        CASE_S(USER_BLE_DEMO0)
        CASE_S(USER_BLE_DEMO1)
        CASE_D()
    }
}

char *g_log_ble_adv_intv_user[BLE_ADV_INTERVALREQ_USER_NUM] = 
{
    "[A2DP]",
    "[SCO]",
    "[TWS_STM]",
};

char *ble_adv_intv_user2str(BLE_ADV_INTERVALREQ_USER_E user)
{
    if (g_log_ble_adv_intv_user[user])
    {
        return g_log_ble_adv_intv_user[user];
    }

    return "[INVALID]";
}

#endif

BLE_ADV_FILL_PARAM_T ble_adv_fill_param[BLE_ADV_ACTIVITY_USER_NUM] = 
{
    {
        //idle adv interval, music adv interval, sco adv interval
        {BLE_ADV_INVALID_INTERVAL},
        {USER_BLE_CUSTOMER_0, USER_STUB, USER_GFPS, USER_GSOUND, USER_AI, USER_INTERCONNECTION, USER_TILE,
         USER_OTA, USER_BLE_AUDIO},
         BLE_ADV_TX_POWER_LEVEL_0
    },
    {
        //idle adv interval, music adv interval, sco adv interval
        {BLE_ADV_INVALID_INTERVAL},
        {USER_SWIFT,USER_BLE_CUSTOMER_1, USER_BLE_DEMO0, USER_INUSE},
        BLE_ADV_TX_POWER_LEVEL_1
    },
    {
        //idle adv interval, music adv interval, sco adv interval
        {BLE_ADV_INVALID_INTERVAL},
        {USER_BLE_CUSTOMER_2, USER_BLE_DEMO1, USER_INUSE},
        BLE_ADV_TX_POWER_LEVEL_2
    }
};

BLE_ADV_FILL_PARAM_T *app_ble_param_get_ctx(void)
{
    return ble_adv_fill_param;
}

BLE_ADV_ACTIVITY_USER_E app_ble_param_get_actv_user_from_adv_user(BLE_ADV_USER_E user)
{
    uint8_t actv_user;
    for (actv_user=BLE_ADV_ACTIVITY_USER_0; actv_user<BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        for (uint8_t i=0; i<BLE_ADV_USER_NUM; i++)
        {
            if (ble_adv_fill_param[actv_user].adv_user[i] == user)
            {
                return actv_user;
            }
        }
    }

    return BLE_ADV_ACTIVITY_USER_NUM;
}

uint16_t app_ble_param_get_adv_interval(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    uint16_t adv_interval = BLE_ADV_INVALID_INTERVAL;
    uint16_t adv_actv_user_interval = BLE_ADV_INVALID_INTERVAL;
    for (uint8_t adv_intv_user = BLE_ADV_INTERVALREQ_USER_A2DP; adv_intv_user < BLE_ADV_INTERVALREQ_USER_NUM; adv_intv_user++)
    {
        adv_actv_user_interval = ble_adv_fill_param[actv_user].adv_interval[adv_intv_user];
        if ((BLE_ADV_INVALID_INTERVAL != adv_actv_user_interval) && (adv_actv_user_interval > adv_interval))
        {
            adv_interval = adv_actv_user_interval;
        }
    }

    LOG_I("%s actv_user %d %d", __func__, actv_user, adv_interval);
    return adv_interval;
}

void app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_E adv_intv_user,
                                        BLE_ADV_USER_E adv_user,
                                        uint16_t interval)
{
    uint8_t actv_user = 0;
    LOG_I("%s %d%s %d%s %d", __func__,
                        adv_intv_user, ble_adv_intv_user2str(adv_intv_user),
                        adv_user, ble_adv_user2str(adv_user),
                        interval);

    if (USER_ALL == adv_user)
    {
        for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            ble_adv_fill_param[actv_user].adv_interval[adv_intv_user] = interval;
        }
    }
    else
    {
        for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            for (uint8_t i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
            {
                if (ble_adv_fill_param[actv_user].adv_user[i] == adv_user)
                {
                    ble_adv_fill_param[actv_user].adv_interval[adv_intv_user] = interval;
                    return;
                }
            }
        }

        ASSERT(0, "%s %d %d %d", __func__, adv_intv_user, adv_user, interval);
    }

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

BLE_ADV_TX_POWER_LEVEL_E app_ble_param_get_adv_tx_power_level(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    return ble_adv_fill_param[actv_user].adv_tx_power_level;
}

static void app_ble_demo0_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T*)param;

    memcpy(&cmd->advData[cmd->advDataLen],
        APP_DEMO_UUID0, APP_DEMO_UUID0_LEN);
    cmd->advDataLen += APP_DEMO_UUID0_LEN;

    cmd->advUserInterval[USER_BLE_DEMO0] = BLE_ADVERTISING_INTERVAL;

    app_ble_data_fill_enable(USER_BLE_DEMO0, true);
}

void app_ble_demo0_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_DEMO0, (BLE_DATA_FILL_FUNC_T)app_ble_demo0_user_data_fill_handler, false);
}

static void app_ble_demo1_user_data_fill_handler(void *param)
{
    LOG_I("%s", __func__);
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T*)param;

    memcpy(&cmd->advData[cmd->advDataLen],
        APP_DEMO_UUID1, APP_DEMO_UUID1_LEN);
    cmd->advDataLen += APP_DEMO_UUID1_LEN;

    app_ble_data_fill_enable(USER_BLE_DEMO1, true);
}

void app_ble_demo1_user_init(void)
{
    LOG_I("%s", __func__);
    app_ble_register_data_fill_handle(USER_BLE_DEMO1, (BLE_DATA_FILL_FUNC_T)app_ble_demo1_user_data_fill_handler, false);
}

