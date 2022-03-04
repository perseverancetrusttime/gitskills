/**
 * @file ancs.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-27
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

#ifndef __ANCS_H__
#define __ANCS_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"

/******************************macro defination*****************************/

/******************************type defination******************************/
typedef enum
{
    ANCS_PROXY_SVC,

    ANCS_PROXY_NS_CHAR,
    ANCS_PROXY_NS_VAL,
    ANCS_PROXY_NS_CFG,

    ANCS_PROXY_READY_CHAR,
    ANCS_PROXY_READY_VAL,
    ANCS_PROXY_READY_CFG,

    ANCS_PROXY_DS_CHAR,
    ANCS_PROXY_DS_VAL,
    ANCS_PROXY_DS_CFG,

    ANCS_PROXY_CP_CHAR,
    ANCS_PROXY_CP_VAL,

    ANCS_PROXY_NUM,
} ancs_gatt_handles;

typedef struct
{
    uint8_t conidx;
    bool ready;
    bool ready_ntf_enable;
    int client_handles[ANCS_PROXY_NUM];
} ProxySubEnv_t;

typedef PRF_ENV_TAG(ancs)
{
    /// By making this the first struct element, the proxy env can be stored in the
    /// 'env' field of prf_task_env.
    prf_hdr_t prf_env;
    /// Service Start Handle
    uint16_t shdl;
    // State of different task instances
    ke_state_t state;
    ProxySubEnv_t subEnv[BLE_CONNECTION_MAX];
    /// GATT User local index
    uint8_t user_lid;
} PRF_ENV_T(ancs);

/****************************function declearation**************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __ANCS_H__ */
