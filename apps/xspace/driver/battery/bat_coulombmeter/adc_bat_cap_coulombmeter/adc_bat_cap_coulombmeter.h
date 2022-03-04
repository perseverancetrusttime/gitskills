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
#ifndef __BATTERY_ALGORITHM_H__
#define __BATTERY_ALGORITHM_H__

#if defined(__ADC_BAT_CAP_COULOMBMETER__)

#ifdef __cplusplus
extern "C" {
#endif

uint16_t bat_algorithm_poweroff_volt_get(bat_adc_status_e bat_status);
uint8_t bat_capacity_conversion_interface(bat_adc_status_e bat_status,bat_adc_voltage adc_data);

#ifdef __cplusplus
}
#endif

#endif

#endif

