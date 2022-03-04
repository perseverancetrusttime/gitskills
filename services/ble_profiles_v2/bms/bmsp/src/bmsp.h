/**
 * @file bmsp.h
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

#ifndef __BMSP_H__
#define __BMSP_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "rwip_config.h"
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"

/******************************macro defination*****************************/

/******************************type defination******************************/
typedef struct
{
    // This field must be first since this struct
    // is used by stack as pointer to prf_hdr_t
    prf_hdr_t prf_env;

    uint16_t shdl; // Service Start Handle
    ke_state_t state;   // State of different task instances

    /// GATT User local index
    uint8_t user_lid;
} PRF_ENV_T(bmsp);;

/****************************function declearation**************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BMSP_H__ */
