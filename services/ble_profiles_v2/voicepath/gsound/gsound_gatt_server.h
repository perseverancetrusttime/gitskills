/**
 * @file gsound_gatt_server.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-28
 * 
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

#ifndef __GSOUND_GATT_SERVER_H__
#define __GSOUND_GATT_SERVER_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "rwip_config.h"

#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"
#include "gsound.h"

/******************************macro defination*****************************/

/******************************type defination******************************/
typedef PRF_ENV_TAG(gsound)
{
    // By making this the first struct element, gsound_env can be stored in the
    // 'env' field of prf_task_env.
    prf_hdr_t prf_env;
    uint8_t conidx;
    uint8_t ntfIndEnableFlag[BLE_CONNECTION_MAX][GSOUND_NUM_CHANNEL_TYPES];
    uint16_t shdl; // Service Start Handle
    ke_state_t state;   // State of different task instances
    uint8_t user_lid;   // GATT User local index
} PRF_ENV_T(gsound);

enum gsound_msg_id {
    GSOUND_CHANNEL_CONNECTED = TASK_FIRST_MSG(TASK_ID_VOICEPATH),
    GSOUND_CHANNEL_DISCONNECTED,
    GSOUND_CONTROL_RECEIVED,
    GSOUND_AUDIO_RECEIVED,
    GSOUND_SEND_DATA_REQUEST,
    GSOUND_SEND_CONNECTION_UPDATE_CFM,
    GSOUND_SEND_COMPLETE_IND,
};

enum gsound_op_type {
    GSOUND_OP_CONNECTION = 0,
    GSOUND_OP_DISCONNECTION = 1,

    GSOUND_OP_TYPE_CNT,
};

enum gsound_op_ret {
    GSOUND_OP_ACCEPT = 0,
    GSOUND_OP_REJECT = 1,

    GSOUND_OP_RET_CNT,
};

typedef struct gsound_chnl_conn_ind
{
    GSoundChannelType chnl;
    uint8_t conidx;
    uint8_t user_lid;
    uint16_t token;
    uint8_t op_type;
    uint8_t status;
} gsound_chnl_conn_ind_t;

struct gsound_gatt_rx_data_ind_t {
    uint8_t conidx;
    uint8_t user;
    uint16_t token;
    uint16_t length;
    uint8_t data[0];
};

struct gsound_gatt_tx_data_req_t {
    GSoundChannelType channel;
    uint8_t connectionIndex;
    uint16_t length;
    uint8_t data[0];
};

struct gsound_gatt_tx_complete_ind_t {
    bool success;
};

/****************************function declearation**************************/
const struct prf_task_cbs *gsound_prf_itf_get(void);

void gsound_send_control_rx_cfm(uint32_t param);

bool gsound_gatt_server_send_notification(uint8_t conidx, uint8_t chnl,
                                          const uint8_t *ptrData, uint32_t length);

void gsound_refresh_ble_database(void);


#ifdef __cplusplus
}
#endif

#endif /* GSOUND_GATT_SERVER_H */
