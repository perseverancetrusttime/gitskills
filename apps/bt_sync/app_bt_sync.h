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

#ifndef __APP_BT_STAMP_H__
#define __APP_BT_STAMP_H__
#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************macro defination*****************************/
#define APP_BT_SYNC_THREAD_STACK_SIZE (1024) //This is default value. If your cmd handler has large arrays. You should increase it.
#define APP_BT_SYNC_INVALID_CHANNEL (0xFF)
#define APP_BT_SYNC_CHANNEL_MAX (3)
#define APP_BT_SYNC_SENT_TRIGGER_INFO_TIMEOUT_IN_MS (3000)

extern uint32_t __app_bt_sync_command_handler_table_start[];
extern uint32_t __app_bt_sync_command_handler_table_end[];

#define APP_BT_SYNC_COMMAND_TO_ADD(opCode, cmdhandler, statusNotify)                          \
    static const app_bt_sync_instance_t syncOp##opCode##_entry                          \
    __attribute__((used, section(".app_bt_sync_command_handler_table"))) =      \
    {(opCode), (cmdhandler), (statusNotify)};

#define APP_BT_SYNC_COMMAND_PTR_FROM_ENTRY_INDEX(index)    \
    ((app_bt_sync_instance_t *)((uint32_t)__app_bt_sync_command_handler_table_start + (index)*sizeof(app_bt_sync_instance_t)))

/******************************type defination******************************/
typedef void (*APP_BT_SYNC_HANDLER_T)(void);
typedef void (*APP_BT_SYNC_STATUS_NOTIFY_T)(uint32_t opCode, bool triStatus, bool triInfoSentStatus);

typedef struct{
    uint32_t opCode;
    uint8_t triSrc;
    bool used;
    uint32_t triTick;
} __attribute__((packed)) bt_trigger_info_s;

typedef struct {
    uint8_t triSrc;
    uint32_t opCode;
} __attribute__((packed)) APP_SYNC_MSG_BLOCK;

typedef struct
{
    uint32_t opCode;
    APP_BT_SYNC_HANDLER_T syncCallback;
    APP_BT_SYNC_STATUS_NOTIFY_T statusNotify;
} __attribute__((packed)) app_bt_sync_instance_t;

typedef struct{
    bool initiator;
    bool twsCmdSendDone;
    uint32_t opCode; /**< The command waiting for the irq */
    uint32_t msTillTimeout; /**< run-time timeout left milliseconds */
} __attribute__((packed)) app_bt_sync_status_info_s;

typedef struct
{
    app_bt_sync_status_info_s statusInstance[APP_BT_SYNC_CHANNEL_MAX];
    uint32_t lastSysTicks;
    uint8_t timeoutSupervisorCount;
} __attribute__((packed)) app_bt_sync_env;

/**
 ****************************************************************************************
 * @brief Enable the both buds trigger function
 *
 * @param[out] none
 * @param[in]  opCode             Point to the expected callback handler
 *
 * @return Execute result status
 ****************************************************************************************/
bool app_bt_sync_enable(uint32_t opCode);


/**
 ****************************************************************************************
 * @brief the slave device to process the sync command
 *
 * @param[out] none
 * @param[in]  triInfo            trigger info from master device
 *
 * @return none
 ****************************************************************************************/
void app_bt_sync_tws_cmd_handler(uint8_t *p_buff, uint16_t length);

/**
 ****************************************************************************************
 * @brief trigger info send done
 ****************************************************************************************/
void app_bt_sync_send_tws_cmd_done(uint8_t *ptrParam, uint16_t paramLen);

#ifdef __cplusplus
}
#endif
#endif

