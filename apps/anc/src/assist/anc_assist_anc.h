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
#ifndef __ANC_ASSIST_ANC_H__
#define __ANC_ASSIST_ANC_H__

#include "anc_assist_defs.h"
#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t anc_assist_anc_init(void);
int32_t anc_assist_anc_deinit(void);
int32_t anc_assist_anc_reset(void);

// API for app_anc_assist.
int32_t anc_assist_anc_switch_curve(anc_assist_algo_id_t id, uint32_t index);
int32_t anc_assist_anc_set_gain_coef(anc_assist_algo_id_t id, float gain_coef);

// API for app_anc_assist_tws_sync .
int32_t anc_assist_anc_switch_curve_impl(uint32_t index);
int32_t anc_assist_anc_set_gain_coef_impl(float gain_coef, enum ANC_TYPE_T type);

#ifdef __cplusplus
}
#endif

#endif