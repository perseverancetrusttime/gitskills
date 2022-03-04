/**
 * @file ancs_task.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-26
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

#ifndef __ANCS_TASK_H__
#define __ANCS_TASK_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "rwapp_config.h"

#if (ANCS_PROXY_ENABLE)
#include "rwip_task.h"


/******************************macro defination*****************************/

/******************************type defination******************************/
enum ancs_proxy_msg_id {
    ANCS_IND_EVT = TASK_FIRST_MSG(TASK_ID_ANCSP),
    ANCS_WRITE_CFM,
};

/****************************function declearation**************************/
/**
 * @brief Initialize the profile task
 * 
 * @param task_desc     ANC proxy task descriptor
 */
void ancs_task_init(struct ke_task_desc *task_desc, void *p_env);

const void* ancs_task_get_srv_cbs(void);

void ancs_proxy_set_ready_flag(uint8_t conidx,
                               int handle_ns_char, int handle_ns_val, int handle_ns_cfg,
                               int handle_ds_char, int handle_ds_val,  int handle_ds_cfg,
                               int handle_cp_char, int handle_cp_val);

#endif /// #if (ANCS_PROXY_ENABLE)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __ANCS_TASK_H__ */