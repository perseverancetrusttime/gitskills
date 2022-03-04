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
#ifndef __STEREO_RECORD_PROCESS_H__
#define __STEREO_RECORD_PROCESS_H__

#include "stdint.h"

typedef void (*_record_get_buf_t)(uint8_t **buff, uint32_t size);

#ifdef __cplusplus
extern "C" {
#endif

int32_t stereo_record_process_init(uint32_t sample_rate, uint32_t frame_len, _record_get_buf_t record_get_buf);
int32_t stereo_record_process_deinit(void);
int32_t stereo_record_process_run(short *pcm_buf, uint32_t pcm_len);

#ifdef __cplusplus
}
#endif

#endif
