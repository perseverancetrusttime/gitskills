/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifndef _RAMDUMP_SECTION_H
#define _RAMDUMP_SECTION_H

#if defined(__cplusplus)
extern "C" {
#endif
#include "hal_trace.h"

void ramdump_to_flash_init(void);

void ramdump_to_flash_handler(enum HAL_TRACE_STATE_T state);

#if defined(__cplusplus)
}
#endif

#endif // _RAMDUMP_SECTION_H

