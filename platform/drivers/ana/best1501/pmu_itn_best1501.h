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
#ifndef __PMU_ITN_BEST1501_H__
#define __PMU_ITN_BEST1501_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PMU_ITN_VDIG_1_05V                  0xF0
#define PMU_ITN_VDIG_1_025V                 0xE8
#define PMU_ITN_VDIG_1_0V                   0xE0
#define PMU_ITN_VDIG_0_975V                 0xD8
#define PMU_ITN_VDIG_0_95V                  0xD0
#define PMU_ITN_VDIG_0_925V                 0xC8
#define PMU_ITN_VDIG_0_9V                   0xC0
#define PMU_ITN_VDIG_0_875V                 0xB8
#define PMU_ITN_VDIG_0_85V                  0xB0
#define PMU_ITN_VDIG_0_825V                 0xA8
#define PMU_ITN_VDIG_0_8V                   0xA0
#define PMU_ITN_VDIG_0_775V                 0x98
#define PMU_ITN_VDIG_0_75V                  0x90
#define PMU_ITN_VDIG_0_725V                 0x88
#define PMU_ITN_VDIG_0_7V                   0x80
#define PMU_ITN_VDIG_0_675V                 0x78
#define PMU_ITN_VDIG_0_65V                  0x70
#define PMU_ITN_VDIG_0_625V                 0x68
#define PMU_ITN_VDIG_0_6V                   0x60

void pmu_itn_init(void);

void pmu_itn_sleep_en(unsigned char sleep_en);

void pmu_itn_logic_dig_set_volt(unsigned short normal, unsigned short sleep);

void pmu_itn_sens_dig_set_volt(unsigned short normal, unsigned short sleep);

void pmu_itn_logic_dig_init_volt(unsigned short normal, unsigned short sleep);

void pmu_itn_sens_dig_init_volt(unsigned short normal, unsigned short sleep);

void pmu_get_ldo_vcore_calib_value(void);

#ifdef __cplusplus
}
#endif

#endif

