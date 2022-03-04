/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifndef CHIP_SUBSYS_SENS
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "app_sensor_hub.h"
#include "sensor_hub.h"
#include "mcu_sensor_hub_app.h"
#include "mcu_sensor_hub_app_ai.h"

#define APP_MCU_SENSOR_HUB_WATCH_DOG_TIME_MS    15000

static uint32_t app_mcu_sensor_hub_last_ping_ms = 0;
void app_mcu_sensor_hub_watch_dog_polling_handler(void)
{
    uint32_t currentMs = GET_CURRENT_MS();
    uint32_t passedTimerMs;

    if (0 == app_mcu_sensor_hub_last_ping_ms)
    {
        passedTimerMs = currentMs;
    }
    else
    {
        if (currentMs > app_mcu_sensor_hub_last_ping_ms)
        {
            passedTimerMs = currentMs - app_mcu_sensor_hub_last_ping_ms;
        }
        else
        {
            passedTimerMs = 0xFFFFFFFF - app_mcu_sensor_hub_last_ping_ms +
                currentMs + 1;
        }

        if (passedTimerMs > APP_MCU_SENSOR_HUB_WATCH_DOG_TIME_MS)
        {
            ASSERT(0, "sensor hub watch dog time-out!");
        }
    }
}

static void app_mcu_sensor_hub_ping_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "mcu gets ping from sensorhub core.");
    app_mcu_sensor_hub_last_ping_ms = GET_CURRENT_MS();
#ifdef VAD_IF_TEST
    hal_sysfreq_print();
#endif
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_PING,
                                "ping mcu",
                                NULL,
                                app_mcu_sensor_hub_ping_received_handler,
                                0,
                                NULL,
                                NULL,
                                NULL);

#define CORE_BRIDGE_DEMO_MCU_REQ_DATA       0x55
#define CORE_BRIDGE_DEMO_MCU_REQ_RSP_DATA   0xAA

typedef struct
{
    uint8_t reqData;
} APP_MCU_SENSOR_HUB_DEMO_REQ_T;

typedef struct
{
    uint8_t rspData;
} APP_MCU_SENSOR_HUB_DEMO_RSP_T;

static void app_mcu_sensor_hub_transmit_demo_no_rsp_cmd_handler(uint8_t* ptr, uint16_t len)
{
    app_core_bridge_send_data_without_waiting_rsp(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_NO_RSP, ptr, len);
}

static void app_mcu_sensor_hub_demo_no_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "Get demo no rsp command from sensor hub core:");
    DUMP8("%02x ", ptr, len);
}

static void app_mcu_sensor_hub_demo_no_rsp_cmd_tx_done_handler(uint16_t cmdCode,
    uint8_t* ptr, uint16_t len)
{
    TRACE(0, "cmdCode 0x%x tx done", cmdCode);
}

void app_mcu_sensor_hub_send_demo_req_no_rsp(void)
{
    APP_MCU_SENSOR_HUB_DEMO_REQ_T req;
    req.reqData = CORE_BRIDGE_DEMO_MCU_REQ_DATA;
    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_NO_RSP,
        (uint8_t *)&req, sizeof(req));
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_NO_RSP,
                                "demo no rsp req to sensor hub core",
                                app_mcu_sensor_hub_transmit_demo_no_rsp_cmd_handler,
                                app_mcu_sensor_hub_demo_no_rsp_cmd_received_handler,
                                0,
                                NULL,
                                NULL,
                                app_mcu_sensor_hub_demo_no_rsp_cmd_tx_done_handler);

static void app_mcu_sensor_hub_transmit_demo_wait_rsp_cmd_handler(uint8_t* ptr, uint16_t len)
{
    app_core_bridge_send_data_with_waiting_rsp(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_WITH_RSP, ptr, len);
}

static void app_mcu_sensor_hub_demo_wait_rsp_cmd_received_handler(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "Get demo with rsp command from sensor hub:");
    DUMP8("%02x ", ptr, len);
    APP_MCU_SENSOR_HUB_DEMO_RSP_T rsp;
    rsp.rspData = CORE_BRIDGE_DEMO_MCU_REQ_RSP_DATA;
    app_core_bridge_send_rsp(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_WITH_RSP, (uint8_t *)&rsp, sizeof(rsp));
}

static void app_mcu_sensor_hub_demo_wait_rsp_cmd_wait_rsp_timeout(uint8_t* ptr, uint16_t len)
{
    TRACE(0, "mcu waiting for rsp to demo cmd timeout.");
}

static void app_mcu_sensor_hub_demo_wait_rsp_cmd_rsp_received_handler(uint8_t* ptr, uint16_t len)
{
    APP_MCU_SENSOR_HUB_DEMO_RSP_T* ptrRsp = (APP_MCU_SENSOR_HUB_DEMO_RSP_T *)ptr;
    TRACE(0, "mcu gets rsp 0x%x from sensor hub core:", ptrRsp->rspData);
}

static void app_mcu_sensor_hub_demo_with_rsp_cmd_tx_done_handler(uint16_t cmdCode,
    uint8_t* ptr, uint16_t len)
{
    TRACE(0, "cmdCode 0x%x tx done", cmdCode);
}

void app_mcu_sensor_hub_send_demo_req_with_rsp(void)
{
    APP_MCU_SENSOR_HUB_DEMO_REQ_T req;
    req.reqData = CORE_BRIDGE_DEMO_MCU_REQ_DATA;
    app_core_bridge_send_cmd(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_WITH_RSP,
        (uint8_t *)&req, sizeof(req));
}

CORE_BRIDGE_TASK_COMMAND_TO_ADD(MCU_SENSOR_HUB_TASK_CMD_DEMO_REQ_WITH_RSP,
                                "demo with rsp req to sensor hub core",
                                app_mcu_sensor_hub_transmit_demo_wait_rsp_cmd_handler,
                                app_mcu_sensor_hub_demo_wait_rsp_cmd_received_handler,
                                APP_CORE_BRIDGE_DEFAULT_WAIT_RSP_TIMEOUT_MS,
                                app_mcu_sensor_hub_demo_wait_rsp_cmd_wait_rsp_timeout,
                                app_mcu_sensor_hub_demo_wait_rsp_cmd_rsp_received_handler,
                                app_mcu_sensor_hub_demo_with_rsp_cmd_tx_done_handler);

static void app_mcu_sensor_hub_transmit_demo_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    app_core_bridge_send_instant_cmd_data(MCU_SENSOR_HUB_INSTANT_CMD_DEMO_REQ,
        ptr, len);
}

static void app_mcu_sensor_hub_demo_instant_req_handler(uint8_t* ptr, uint16_t len)
{
    // for test purpose, we add log print here.
    // but as instant cmd handler will be directly called in intersys irq context,
    // for realistic use, should never do log print
    TRACE(0, "Get demo instant req command from sensor core:");
    DUMP8("%02x ", ptr, len);
}

void app_mcu_sensor_hub_send_demo_instant_req(void)
{
    APP_MCU_SENSOR_HUB_DEMO_REQ_T req;
    req.reqData = CORE_BRIDGE_DEMO_MCU_REQ_DATA;
    app_core_bridge_send_cmd(MCU_SENSOR_HUB_INSTANT_CMD_DEMO_REQ,
        (uint8_t *)&req, sizeof(req));
}

CORE_BRIDGE_INSTANT_COMMAND_TO_ADD(MCU_SENSOR_HUB_INSTANT_CMD_DEMO_REQ,
                                app_mcu_sensor_hub_transmit_demo_instant_req_handler,
                                app_mcu_sensor_hub_demo_instant_req_handler);

#ifdef VAD_IF_TEST
void app_mcu_vad_if_test(void);
#endif

// mcu to initialize sensor hub module
void app_sensor_hub_init(void)
{
    app_core_bridge_init();

    int ret;
    ret = sensor_hub_open(app_core_bridge_data_received, app_core_bridge_data_tx_done);
    ASSERT(ret == 0, "sensor_hub_open failed: %d", ret);

#ifdef VAD_IF_TEST
    app_mcu_vad_if_test();
#endif

#ifdef  __AI_VOICE__
    sensor_hub_ai_mcu_app_init();
#endif
}
#endif

