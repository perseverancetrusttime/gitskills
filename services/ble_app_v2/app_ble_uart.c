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
//#include "mbed.h"
#include "plat_types.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hal_uart.h"
#include "hal_trace.h"
#include "app_trace_rx.h"
#include "ble_app_dbg.h"
#include "app_ble_include.h"
#include "gapm_msg.h"
#include "app_ble_uart.h"
#include "app_bt_func.h"

#define APP_BLE_UART_BUFF_SIZE     50  //16
#define APP_DEMO_UUID0            "\x02\x01\x06\x1b\xFF\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x03\xFE\x00\x00\x00"
#define APP_DEMO_UUID0_LEN        (31)
#define APP_DEMO_UUID1            "\x03\x18\x03\xFE"
#define APP_DEMO_UUID1_LEN        (4)
#define APP_DEMO_UUID2            "\x02\x01\xFF\x03\x19\x03\xFE"
#define APP_DEMO_UUID2_LEN        (7)

uint8_t adv_addr_set[6]  = {0x66, 0x34, 0x33, 0x23, 0x22, 0x11};
uint8_t adv_addr_set2[6] = {0x77, 0x34, 0x33, 0x23, 0x22, 0x11};
uint8_t adv_addr_set3[6] = {0x88, 0x34, 0x33, 0x23, 0x22, 0x11};


void ble_start_three_adv(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_UNDIRECT,
                            12,
                            (uint8_t *)APP_DEMO_UUID0, APP_DEMO_UUID0_LEN,
                            (uint8_t *)APP_DEMO_UUID0, APP_DEMO_UUID0_LEN);
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_1,
                            false,
                            adv_addr_set2,
                            NULL,
                            200,
                            ADV_TYPE_UNDIRECT,
                            -5,
                            (uint8_t *)APP_DEMO_UUID1, APP_DEMO_UUID1_LEN,
                            NULL, 0);
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_2,
                            true,
                            adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_UNDIRECT,
                            12,
                            (uint8_t *)APP_DEMO_UUID2, APP_DEMO_UUID2_LEN,
                            NULL, 0);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_1);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_2);
}

void ble_stop_all_adv(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_0);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_1);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_2);
}

void ble_start_adv_1(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_UNDIRECT,
                            12,
                            (uint8_t *)APP_DEMO_UUID0, APP_DEMO_UUID0_LEN,
                            (uint8_t *)APP_DEMO_UUID0, APP_DEMO_UUID0_LEN);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);
}

void ble_stop_adv_1(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_0);
}

void ble_start_adv_2(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_1,
                            false,
                            adv_addr_set2,
                            NULL,
                            200,
                            ADV_TYPE_UNDIRECT,
                            5,
                            (uint8_t *)APP_DEMO_UUID1, APP_DEMO_UUID1_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_1);
}

void ble_stop_adv_2(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_1);
}

void ble_start_adv_3(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_2,
                            true,
                            adv_addr_set3,
                            NULL,
                            200,
                            ADV_TYPE_NON_CONN_SCAN,
                            12,
                            (uint8_t *)APP_DEMO_UUID2, APP_DEMO_UUID2_LEN,
                            NULL, 0);
    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_2);
}

void ble_stop_adv_3(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_adv_stop(BLE_ADV_ACTIVITY_USER_2);
}

void ble_update_adv_data_1_to_2(void)
{
    LOG_I("%s", __func__);
    app_ble_custom_init();
    app_ble_custom_adv_write_data(BLE_ADV_ACTIVITY_USER_0,
                            true,
                            adv_addr_set,
                            NULL,
                            160,
                            ADV_TYPE_UNDIRECT,
                            12,
                            (uint8_t *)APP_DEMO_UUID1, APP_DEMO_UUID1_LEN,
                            (uint8_t *)APP_DEMO_UUID1, APP_DEMO_UUID1_LEN);

    app_ble_custom_adv_start(BLE_ADV_ACTIVITY_USER_0);
}

void ble_update_adv_data_1_back_to_1(void)
{
    LOG_I("%s", __func__);
    ble_start_adv_1();
}

void ble_set_adv_txpwr_to_min(void)
{
    app_ble_set_all_adv_txpwr(-21);
}

void ble_set_adv_txpwr_to_0(void)
{
    app_ble_set_all_adv_txpwr(0);
}

void ble_set_adv_txpwr_to_max(void)
{
    app_ble_set_all_adv_txpwr(16);
}

void ble_open_scan(void)
{
    LOG_I("%s", __func__);
    app_ble_start_scan(BLE_DEFAULT_SCAN_POLICY, 10, 60);
}

void ble_close_scan(void)
{
    LOG_I("%s", __func__);
    app_ble_stop_scan();
}

void ble_start_connect(void)
{
    LOG_I("%s", __func__);
    app_ble_start_connect(adv_addr_set);
}

void ble_stop_connect(void)
{
    LOG_I("%s", __func__);
    app_ble_cancel_connecting();
}

void ble_disconnect_all(void)
{
    LOG_I("%s", __func__);
    app_ble_disconnect_all();
}


const ble_uart_handle_t ble_uart_test_handle[]=
{
    {"start_three_adv", ble_start_three_adv},
    {"stop_all_adv", ble_stop_all_adv},
    {"start_adv_1", ble_start_adv_1},
    {"start_adv_2", ble_start_adv_2},
    {"start_adv_3", ble_start_adv_3},
    {"stop_adv_1", ble_stop_adv_1},
    {"stop_adv_2", ble_stop_adv_2},
    {"stop_adv_3", ble_stop_adv_3},
    {"update_adv_data_1_to_2", ble_update_adv_data_1_to_2},
    {"update_adv_data_1_back_to_1", ble_update_adv_data_1_back_to_1},
    {"set_adv_txpwr_to_min", ble_set_adv_txpwr_to_min},
    {"set_adv_txpwr_to_0", ble_set_adv_txpwr_to_0},
    {"set_adv_txpwr_to_max", ble_set_adv_txpwr_to_max},
    {"open_scan", ble_open_scan},
    {"close_scan", ble_close_scan},
    {"start_connect", ble_start_connect},
    {"stop_connect", ble_stop_connect},
    {"disconnect_all", ble_disconnect_all},
};

/*****************************************************************************
 Prototype    : ble_test_find_uart_handle
 Description  : find the test cmd handle
 Input        : uint8_t* buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/11/15
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
ble_uart_test_function_handle ble_test_find_uart_handle(unsigned char* buf)
{
    uint8_t i = 0;
    ble_uart_test_function_handle p = NULL;
    for (i = 0; i<sizeof(ble_uart_test_handle)/sizeof(ble_uart_handle_t); i++)
    {
        if ((strncmp((char*)buf, ble_uart_test_handle[i].string, strlen(ble_uart_test_handle[i].string)) == 0) ||
            strstr(ble_uart_test_handle[i].string, (char*)buf))
        {
            p = ble_uart_test_handle[i].function;
            break;
        }
    }
    return p;
}

int ble_uart_cmd_handler(unsigned char *buf, unsigned int length)
{
    int ret = 0;

    ble_uart_test_function_handle handl_function = ble_test_find_uart_handle(buf);
    if(handl_function)
    {
        //handl_function();
        app_bt_start_custom_function_in_bt_thread(0, 0, 
            (uint32_t)handl_function);
    }
    else
    {
        ret = -1;
        TRACE(0,"can not find handle function");
    }
    return ret;
}

unsigned int ble_uart_cmd_callback(unsigned char *buf, unsigned int len)
{
    // Check len
    TRACE(2,"[%s] len = %d", __func__, len);
    ble_uart_cmd_handler((unsigned char*)buf,strlen((char*)buf));
    return 0;
}

void ble_uart_cmd_init(void)
{
#ifdef APP_TRACE_RX_ENABLE
    TRACE(0,"ble_uart_cmd_init");
    app_trace_rx_register("BLE", ble_uart_cmd_callback);
#endif
}

