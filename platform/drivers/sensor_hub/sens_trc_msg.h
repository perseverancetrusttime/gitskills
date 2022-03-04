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
#ifndef __SENS_TRC_MSG_H__
#define __SENS_TRC_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

enum SENS_TRC_MSG_ID_T {
    SENS_TRC_MSG_ID_TRACE,
    SENS_TRC_MSG_ID_CRASH_ASSERT_START,
    SENS_TRC_MSG_ID_CRASH_FAULT_START,
    SENS_TRC_MSG_ID_CRASH_END,
};

struct SENS_TRC_CTRL_T {
    const uint8_t *buf_start;
    const uint16_t *entry_start;
    uint16_t buf_size;
    uint16_t entry_cnt;
    uint16_t entry_rpos;
    uint16_t entry_wpos;
    uint16_t discard_cnt_r;
    uint16_t discard_cnt_w;
    uint8_t seq_r;
    uint8_t seq_w;
};

struct SENS_TRC_MSG_T {
    enum SENS_TRC_MSG_ID_T id;
    void *param;
};

#ifdef __cplusplus
}
#endif

#endif

