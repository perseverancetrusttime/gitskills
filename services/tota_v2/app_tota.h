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
#ifndef __APP_TOTA_H__
#define __APP_TOTA_H__

#include "app_tota_cmd_code.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of the tota module
 *
 */
typedef enum
{
    APP_TOTA_AUDIO_DUMP = 0,
    APP_TOTA_ANC,

    APP_TOTA_MODULE_NONE = 0xFF
} APP_TOTA_MODULE_E;


typedef struct{
    void (*connected_cb)(void);
    void (*disconnected_cb)(void);
    void (*tx_done_cb)(void);
    void (*rx_cb)(uint8_t * buf, uint16_t len);
}app_tota_callback_func_t;

void app_tota_callback_module_register(APP_TOTA_MODULE_E module,
                        const app_tota_callback_func_t *tota_callback_func);
void app_tota_callback_module_set(APP_TOTA_MODULE_E module);
APP_TOTA_MODULE_E tota_callback_module_get();


void app_tota_init(void);
// void app_tota_update_datapath(APP_TOTA_TRANSMISSION_PATH_E dataPath);
// APP_TOTA_TRANSMISSION_PATH_E app_tota_get_datapath(void);

/* interface */
bool is_tota_connected();

/**/
void app_tota_set_shake_hand_status(bool set_status);
bool app_tota_send(uint8_t * pdata, uint16_t dataLen, APP_TOTA_CMD_CODE_E opCode );
bool app_tota_send_rsp(APP_TOTA_CMD_CODE_E rsp_opCode, APP_TOTA_CMD_RET_STATUS_E rsp_status, uint8_t * pdata, uint16_t dataLen);

void tota_printf(const char * format, ...);
void tota_print(const char * format, ...);
bool app_tota_if_customers_access_valid(uint8_t access_code);

#ifdef __cplusplus
}
#endif

#endif

