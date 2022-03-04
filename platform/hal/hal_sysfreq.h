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
#ifndef __HAL_SYSFREQ_H__
#define __HAL_SYSFREQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

enum HAL_SYSFREQ_USER_T {
    HAL_SYSFREQ_USER_INIT,
    HAL_SYSFREQ_USER_OVERLAY,
    HAL_SYSFREQ_USER_USB,
    HAL_SYSFREQ_USER_BT,
    HAL_SYSFREQ_USER_ANC,

    HAL_SYSFREQ_USER_APP_0,
    HAL_SYSFREQ_USER_APP_1,
    HAL_SYSFREQ_USER_APP_2,
    HAL_SYSFREQ_USER_APP_3,
    HAL_SYSFREQ_USER_APP_4,
    HAL_SYSFREQ_USER_APP_5,
    HAL_SYSFREQ_USER_APP_6,
    HAL_SYSFREQ_USER_APP_7,
    HAL_SYSFREQ_USER_APP_8,
    HAL_SYSFREQ_USER_APP_9,
    HAL_SYSFREQ_USER_APP_10,
    HAL_SYSFREQ_USER_APP_11,
    HAL_SYSFREQ_USER_APP_12,
    HAL_SYSFREQ_USER_APP_13,
    HAL_SYSFREQ_USER_APP_14,
    HAL_SYSFREQ_USER_APP_15,
#if defined(__XSPACE_UI__)
    HAL_SYSFREQ_USER_APP_WRITE_FLASH,
    HAL_SYSFREQ_USER_APP_DEV_THREAD,
    HAL_SYSFREQ_USER_APP_TOUCH_UTT,
#if defined(__XSPACE_XBUS_MANAGER__)
    HAL_SYSFREQ_USER_APP_XBUS,
#endif
    HAL_SYSFREQ_USER_APP_BUSY,
    HAL_SYSFREQ_USER_SCO_ALG_BOOST,
    HAL_SYSFREQ_USER_PRESS,
#endif
    HAL_SYSFREQ_USER_QTY
};

void hal_sysfreq_set_min_freq(enum HAL_CMU_FREQ_T freq);

int hal_sysfreq_req(enum HAL_SYSFREQ_USER_T user, enum HAL_CMU_FREQ_T freq);

enum HAL_CMU_FREQ_T hal_sysfreq_get(void);

enum HAL_CMU_FREQ_T hal_sysfreq_get_hw_freq(void);

int hal_sysfreq_busy(void);

void hal_sysfreq_print(void);

#ifdef __cplusplus
}
#endif

#endif

