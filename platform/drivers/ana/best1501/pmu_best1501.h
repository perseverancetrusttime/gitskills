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
#ifndef __PMU_BEST1501_H__
#define __PMU_BEST1501_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

#define EXTPMU_REG(r)                       (((r) & 0xFFF) | 0x0000)
#define ANA_REG(r)                          (((r) & 0xFFF) | 0x1000)
#define RF_REG(r)                           (((r) & 0xFFF) | 0x2000)
#define ITNPMU_REG(r)                       (((r) & 0xFFF) | 0x3000)
#define DPA_REG(r)                          (((r) & 0xFFF) | 0x4000)
#define USBPHY_REG(r)                       (((r) & 0xFFF) | 0x5000)

#define MAX_ITN_VMIC_CH_NUM                 2
#define MAX_EXT_VMIC_CH_NUM                 5
#define MAX_EXT_VMIC_CH_NUM_1803            3
#define MAX_EXT_VMIC_CH_NUM_1802            2

#ifdef NO_EXT_PMU
#define MAX_VMIC_CH_NUM                     MAX_ITN_VMIC_CH_NUM
#else
#define MAX_VMIC_CH_NUM                     MAX_EXT_VMIC_CH_NUM
#endif

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY         = 0,
    PMU_EFUSE_PAGE_BOOT             = 1,
    PMU_EFUSE_PAGE_FEATURE          = 2,
    PMU_EFUSE_PAGE_BATTER_LV        = 3,

    PMU_EFUSE_PAGE_BATTER_HV        = 4,
    PMU_EFUSE_PAGE_SW_CFG           = 5,
    PMU_EFUSE_PAGE_CODE_VER         = 6,
    PMU_EFUSE_PAGE_RESERVED_7       = 7,

    PMU_EFUSE_PAGE_BT_POWER         = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L       = 9,
    PMU_EFUSE_PAGE_DCCALIB2_L_LP    = 10,
    PMU_EFUSE_PAGE_DCCALIB_L        = 11,

    PMU_EFUSE_PAGE_DCCALIB_L_LP     = 12,
    PMU_EFUSE_PAGE_RESERVED_13      = 13,
    PMU_EFUSE_PAGE_MODEL            = 14,
    PMU_EFUSE_PAGE_RESERVED_15      = 15,
};

enum PMU_EXT_EFUSE_PAGE_T {
    PMU_EXT_EFUSE_PAGE_RESERVED_0   = 0,
    PMU_EXT_EFUSE_PAGE_RESERVED_1   = 1,
    PMU_EXT_EFUSE_PAGE_RESERVED_2   = 2,
    PMU_EXT_EFUSE_PAGE_RESERVED_3   = 3,

    PMU_EXT_EFUSE_PAGE_RESERVED_4   = 4,
    PMU_EXT_EFUSE_PAGE_RESERVED_5   = 5,
    PMU_EXT_EFUSE_PAGE_RESERVED_6   = 6,
    PMU_EXT_EFUSE_PAGE_RESERVED_7   = 7,

    PMU_EXT_EFUSE_PAGE_LDO_VCORE    = 8,
    PMU_EXT_EFUSE_PAGE_RESERVED_9   = 9,
    PMU_EXT_EFUSE_PAGE_RESERVED_10  = 10,
    PMU_EXT_EFUSE_PAGE_RESERVED_11  = 11,

    PMU_EXT_EFUSE_PAGE_RESERVED_12  = 12,
    PMU_EXT_EFUSE_PAGE_RESERVED_13  = 13,
    PMU_EXT_EFUSE_PAGE_RESERVED_14  = 14,
    PMU_EXT_EFUSE_PAGE_RESERVED_15  = 15,

    PMU_EXT_EFUSE_PAGE_RESERVED_16  = 16,
    PMU_EXT_EFUSE_PAGE_RESERVED_17  = 17,
    PMU_EXT_EFUSE_PAGE_RESERVED_18  = 18,
    PMU_EXT_EFUSE_PAGE_RESERVED_19  = 19,

    PMU_EXT_EFUSE_PAGE_RESERVED_20  = 20,
    PMU_EXT_EFUSE_PAGE_RESERVED_21  = 21,
    PMU_EXT_EFUSE_PAGE_RESERVED_22  = 22,
    PMU_EXT_EFUSE_PAGE_RESERVED_23  = 23,
};

enum PMU_IRQ_TYPE_T {
    PMU_IRQ_TYPE_PWRKEY,
    PMU_IRQ_TYPE_GPADC,
    PMU_IRQ_TYPE_RTC,
    PMU_IRQ_TYPE_CHARGER,

    PMU_IRQ_TYPE_QTY
};

enum PMU_PLL_DIV_TYPE_T {
    PMU_PLL_DIV_DIG,
    PMU_PLL_DIV_CODEC,
};

enum PMU_CHIP_TYPE_T {
    PMU_CHIP_TYPE_1800,
    PMU_CHIP_TYPE_1802,
    PMU_CHIP_TYPE_1803,
	PMU_CHIP_TYPE_1807,
};

enum PMU_BIG_BANDGAP_USER_T {
    PMU_BIG_BANDGAP_USER_GPADC          = (1 << 0),
    PMU_BIG_BANDGAP_USER_VMIC1          = (1 << 1),
    PMU_BIG_BANDGAP_USER_VMIC2          = (1 << 2),
};

union SECURITY_VALUE_T {
    struct {
        unsigned short security_en      :1;
        unsigned short mode             :1;
        unsigned short sig_type         :1;
        unsigned short reserved         :1;
        unsigned short key_id           :2;
        unsigned short vendor_id        :5;
        unsigned short flash_id         :1;
        unsigned short chksum           :4;
    } root;
    struct {
        unsigned short security_en      :1;
        unsigned short mode             :1;
        unsigned short sig_type         :1;
        unsigned short skip_sig         :1;
        unsigned short key_id           :2;
        unsigned short reg_base         :2;
        unsigned short reg_size         :2;
        unsigned short reg_offset       :1;
        unsigned short flash_id         :1;
        unsigned short chksum           :4;
    } otp;
    unsigned short reg;
};
#define SECURITY_VALUE_T                    SECURITY_VALUE_T

#define PMU_BOOT_FLASH_ID

enum HAL_FLASH_ID_T pmu_get_boot_flash_id(void);

enum PMU_CHIP_TYPE_T pmu_get_chip_type(void);

void pmu_codec_hppa_enable(int enable);

uint32_t pmu_codec_mic_bias_remap(uint32_t map);

void pmu_codec_mic_bias_enable(uint32_t map, int enable);

void pmu_codec_mic_bias_noremap_enable(uint32_t map, int enable);


void pmu_codec_mic_bias_set_volt(uint32_t map, uint32_t mv);

void pmu_codec_mic_bias_lowpower_mode(uint32_t map, int enable);

void pmu_codec_mic_bias_noremap_lowpower_mode(uint32_t map, int enable);

int pmu_codec_volt_ramp_up(void);

int pmu_codec_volt_ramp_down(void);

void pmu_codec_vad_save_power(void);

void pmu_codec_vad_restore_power(void);

void pmu_lbrt_config(int enable);

void pmu_led_set_hiz(enum HAL_GPIO_PIN_T pin);

void pmu_rf_ana_init(void);

void pmu_big_bandgap_enable(enum PMU_BIG_BANDGAP_USER_T user, int enable);

void pmu_1802_uart_enable(void);

void pmu_1803_uart_enable(void);

void pmu_vcorev_dig_enable(void);

void pmu_vcorev_dig_disable(void);
#ifdef DCDC_HIGH_EFFICIENCY_CTRL
int pmu_high_dcdc_efficiency_mode_enable(bool enable);
#endif

#ifdef __cplusplus
}
#endif

#endif

