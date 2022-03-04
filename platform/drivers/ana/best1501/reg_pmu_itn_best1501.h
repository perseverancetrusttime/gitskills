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
#ifndef __REG_PMU_ITN_BEST1501_H__
#define __REG_PMU_ITN_BEST1501_H__

#ifdef __cplusplus
extern "C" {
#endif

// REG_00
#define REVID_SHIFT                         0
#define REVID_MASK                          (0xF << REVID_SHIFT)
#define REVID(n)                            BITFIELD_VAL(REVID, n)
#define CHIPID_SHIFT                        4
#define CHIPID_MASK                         (0xFFF << CHIPID_SHIFT)
#define CHIPID(n)                           BITFIELD_VAL(CHIPID, n)

// REG_01
#define LPO_OFF_CNT_SHIFT                   12
#define LPO_OFF_CNT_MASK                    (0xF << LPO_OFF_CNT_SHIFT)
#define LPO_OFF_CNT(n)                      BITFIELD_VAL(LPO_OFF_CNT, n)
#define POWER_ON_DB_CNT_SHIFT               8
#define POWER_ON_DB_CNT_MASK                (0xF << POWER_ON_DB_CNT_SHIFT)
#define POWER_ON_DB_CNT(n)                  BITFIELD_VAL(POWER_ON_DB_CNT, n)
#define POWER_OFF_CNT_SHIFT                 4
#define POWER_OFF_CNT_MASK                  (0xF << POWER_OFF_CNT_SHIFT)
#define POWER_OFF_CNT(n)                    BITFIELD_VAL(POWER_OFF_CNT, n)
#define CLK_STABLE_CNT_SHIFT                0
#define CLK_STABLE_CNT_MASK                 (0xF << CLK_STABLE_CNT_SHIFT)
#define CLK_STABLE_CNT(n)                   BITFIELD_VAL(CLK_STABLE_CNT, n)

// REG_02
#define REG_PU_VBAT_DIV                     (1 << 15)
#define PU_LPO_DR                           (1 << 14)
#define PU_LPO_REG                          (1 << 13)
#define POWERKEY_WAKEUP_OSC_EN              (1 << 12)
#define RTC_POWER_ON_EN                     (1 << 11)
#define PU_ALL_REG                          (1 << 10)
#define RESERVED_ANA_16                     (1 << 9)
#define REG_CLK_32K_SEL                     (1 << 8)
#define DEEPSLEEP_MODE_DIGI_DR              (1 << 7)
#define DEEPSLEEP_MODE_DIGI_REG             (1 << 6)
#define RESERVED_ANA_18_17_SHIFT            4
#define RESERVED_ANA_18_17_MASK             (0x3 << RESERVED_ANA_18_17_SHIFT)
#define RESERVED_ANA_18_17(n)               BITFIELD_VAL(RESERVED_ANA_18_17, n)
#define RESETN_ANA_DR                       (1 << 3)
#define RESETN_ANA_REG                      (1 << 2)
#define RESETN_DIG_DR                       (1 << 1)
#define RESETN_DIG_REG                      (1 << 0)

// REG_03
#define RESERVED_ANA_19                     (1 << 15)
#define PU_LP_BIAS_LDO_DSLEEP_MSK           (1 << 14)
#define PU_LP_BIAS_LDO_DR                   (1 << 13)
#define PU_LP_BIAS_LDO_REG                  (1 << 12)
#define PU_BIAS_LDO_DR                      (1 << 11)
#define PU_BIAS_LDO_REG                     (1 << 10)
#define BG_VBG_SEL_DR                       (1 << 9)
#define BG_VBG_SEL_REG                      (1 << 8)
#define BG_CONSTANT_GM_BIAS_DR              (1 << 7)
#define BG_CONSTANT_GM_BIAS_REG             (1 << 6)
#define BG_CORE_EN_DR                       (1 << 5)
#define BG_CORE_EN_REG                      (1 << 4)
#define BG_VTOI_EN_DR                       (1 << 3)
#define BG_VTOI_EN_REG                      (1 << 2)
#define BG_NOTCH_EN_DR                      (1 << 1)
#define BG_NOTCH_EN_REG                     (1 << 0)

// REG_04
#define BG_NOTCH_LPF_LOW_BW                 (1 << 15)
#define BG_OPX2                             (1 << 14)
#define BG_PTATX2                           (1 << 13)
#define BG_VBG_RES_SHIFT                    9
#define BG_VBG_RES_MASK                     (0xF << BG_VBG_RES_SHIFT)
#define BG_VBG_RES(n)                       BITFIELD_VAL(BG_VBG_RES, n)
#define BG_CONSTANT_GM_BIAS_BIT_SHIFT       7
#define BG_CONSTANT_GM_BIAS_BIT_MASK        (0x3 << BG_CONSTANT_GM_BIAS_BIT_SHIFT)
#define BG_CONSTANT_GM_BIAS_BIT(n)          BITFIELD_VAL(BG_CONSTANT_GM_BIAS_BIT, n)
#define BG_VTOI_IABS_BIT_SHIFT              2
#define BG_VTOI_IABS_BIT_MASK               (0x1F << BG_VTOI_IABS_BIT_SHIFT)
#define BG_VTOI_IABS_BIT(n)                 BITFIELD_VAL(BG_VTOI_IABS_BIT, n)
#define BG_VTOI_VCAS_BIT_SHIFT              0
#define BG_VTOI_VCAS_BIT_MASK               (0x3 << BG_VTOI_VCAS_BIT_SHIFT)
#define BG_VTOI_VCAS_BIT(n)                 BITFIELD_VAL(BG_VTOI_VCAS_BIT, n)

// REG_05
#define LDO_VCORE_L_VBIT_DSLEEP_SHIFT       8
#define LDO_VCORE_L_VBIT_DSLEEP_MASK        (0xFF << LDO_VCORE_L_VBIT_DSLEEP_SHIFT)
#define LDO_VCORE_L_VBIT_DSLEEP(n)          BITFIELD_VAL(LDO_VCORE_L_VBIT_DSLEEP, n)
#define LDO_VCORE_L_VBIT_NORMAL_SHIFT       0
#define LDO_VCORE_L_VBIT_NORMAL_MASK        (0xFF << LDO_VCORE_L_VBIT_NORMAL_SHIFT)
#define LDO_VCORE_L_VBIT_NORMAL(n)          BITFIELD_VAL(LDO_VCORE_L_VBIT_NORMAL, n)
#define MAX_LDO_VCORE_L_VBIT_VAL            (LDO_VCORE_L_VBIT_NORMAL_MASK << LDO_VCORE_L_VBIT_NORMAL_SHIFT)

// REG_06
#define REG_BYPASS_VBG_RF_BUFFER_DR         (1 << 15)
#define REG_BYPASS_VBG_RF_BUFFER_REG        (1 << 14)
#define PU_VBG_RF_BUFFER_DR                 (1 << 13)
#define PU_VBG_RF_BUFFER_REG                (1 << 12)
#define RESERVED_ANA_21_20_SHIFT            10
#define RESERVED_ANA_21_20_MASK             (0x3 << RESERVED_ANA_21_20_SHIFT)
#define RESERVED_ANA_21_20(n)               BITFIELD_VAL(RESERVED_ANA_21_20, n)
#define REG_LPO_KTRIM_SHIFT                 4
#define REG_LPO_KTRIM_MASK                  (0x3F << REG_LPO_KTRIM_SHIFT)
#define REG_LPO_KTRIM(n)                    BITFIELD_VAL(REG_LPO_KTRIM, n)
#define REG_LPO_ITRIM_SHIFT                 0
#define REG_LPO_ITRIM_MASK                  (0xF << REG_LPO_ITRIM_SHIFT)
#define REG_LPO_ITRIM(n)                    BITFIELD_VAL(REG_LPO_ITRIM, n)

// REG_07
#define BG_TRIM_VBG_SHIFT                   13
#define BG_TRIM_VBG_MASK                    (0x7 << BG_TRIM_VBG_SHIFT)
#define BG_TRIM_VBG(n)                      BITFIELD_VAL(BG_TRIM_VBG, n)
#define BG_R_TEMP_SHIFT                     10
#define BG_R_TEMP_MASK                      (0x7 << BG_R_TEMP_SHIFT)
#define BG_R_TEMP(n)                        BITFIELD_VAL(BG_R_TEMP, n)
#define REG_LPO_FREQ_TRIM_SHIFT             5
#define REG_LPO_FREQ_TRIM_MASK              (0x1F << REG_LPO_FREQ_TRIM_SHIFT)
#define REG_LPO_FREQ_TRIM(n)                BITFIELD_VAL(REG_LPO_FREQ_TRIM, n)
#define REG_LIGHT_LOAD_OPTION_VCORE_V       (1 << 4)
#define REG_LOOP_CTRL_VCORE_L_LDO           (1 << 3)
#define REG_PULLDOWN_VCORE_L                (1 << 2)
#define REG_LOOP_CTRL_VCORE_V_LDO           (1 << 1)
#define REG_PULLDOWN_VCORE_V                (1 << 0)

// REG_08
#define LP_EN_VCORE_L_LDO_DSLEEP_EN         (1 << 15)
#define LP_EN_VCORE_L_LDO_DR                (1 << 14)
#define LP_EN_VCORE_L_LDO_REG               (1 << 13)
#define REG_PU_VCORE_L_LDO_DSLEEP_MSK       (1 << 12)
#define REG_PU_VCORE_L_LDO_DR               (1 << 11)
#define REG_PU_VCORE_L_LDO_REG              (1 << 10)
#define REG_LDO_VCORE_L_VOUT_HV             (1 << 8)
#define REG_LDO_VCORE_V_VOUT_HV             (1 << 7)

// REG_09
#define LP_EN_VCORE_V_LDO_DSLEEP_EN         (1 << 15)
#define LP_EN_VCORE_V_LDO_DR                (1 << 14)
#define LP_EN_VCORE_V_LDO_REG               (1 << 13)
#define REG_PU_VCORE_V_LDO_DSLEEP_MSK       (1 << 12)
#define REG_PU_VCORE_V_LDO_DR               (1 << 11)
#define REG_PU_VCORE_V_LDO_REG              (1 << 10)

// REG_0A
#define LP_EN_MIC_LDO_DSLEEP_EN             (1 << 15)
#define LP_EN_MIC_LDO_DR                    (1 << 14)
#define LP_EN_MIC_LDO_REG                   (1 << 13)
#define REG_PU_MIC_LDO_DSLEEP_MSK           (1 << 12)
#define REG_PU_MIC_LDO_DR                   (1 << 11)
#define REG_PU_MIC_LDO_REG                  (1 << 10)
#define REG_VCORE_ON_DELAY_DR               (1 << 7)
#define REG_VCORE_ON_DELAY                  (1 << 6)
#define RESETN_DPA_DR                       (1 << 5)
#define RESETN_DPA_REG                      (1 << 4)
#define RESETN_USB_DR                       (1 << 3)
#define RESETN_USB_REG                      (1 << 2)
#define RESETN_RF_DR                        (1 << 1)
#define RESETN_RF_REG                       (1 << 0)

// REG_0B
#define LP_EN_MIC_BIASA_DSLEEP_EN           (1 << 15)
#define LP_EN_MIC_BIASA_DR                  (1 << 14)
#define LP_EN_MIC_BIASA_REG                 (1 << 13)
#define REG_PU_MIC_BIASA_DSLEEP_MSK         (1 << 12)
#define REG_PU_MIC_BIASA_DR                 (1 << 11)
#define REG_PU_MIC_BIASA_REG                (1 << 10)
#define LP_EN_MIC_BIASB_DSLEEP_EN           (1 << 9)
#define LP_EN_MIC_BIASB_DR                  (1 << 8)
#define LP_EN_MIC_BIASB_REG                 (1 << 7)
#define REG_PU_MIC_BIASB_DSLEEP_MSK         (1 << 6)
#define REG_PU_MIC_BIASB_DR                 (1 << 5)
#define REG_PU_MIC_BIASB_REG                (1 << 4)
#define REG_CLK_32K_INT_EXT_SEL             (1 << 0)

// REG_0C
#define REG_VCORE_SSTIME_MODE_SHIFT         14
#define REG_VCORE_SSTIME_MODE_MASK          (0x3 << REG_VCORE_SSTIME_MODE_SHIFT)
#define REG_VCORE_SSTIME_MODE(n)            BITFIELD_VAL(REG_VCORE_SSTIME_MODE, n)
#define REG_STON_SW_VBG_TIME_SHIFT          6
#define REG_STON_SW_VBG_TIME_MASK           (0x1F << REG_STON_SW_VBG_TIME_SHIFT)
#define REG_STON_SW_VBG_TIME(n)             BITFIELD_VAL(REG_STON_SW_VBG_TIME, n)
#define LP_EN_VTOI_DSLEEP_EN                (1 << 5)
#define LP_EN_VTOI_DR                       (1 << 4)
#define LP_EN_VTOI_REG                      (1 << 3)
#define REG_PU_VTOI_DSLEEP_MSK              (1 << 2)
#define REG_PU_VTOI_DR                      (1 << 1)
#define REG_PU_VTOI_REG                     (1 << 0)

// REG_0D
#define REG_BG_SLEEP_MSK                    (1 << 12)
#define REG_MIC_BIASA_PULL_DOWN_DR          (1 << 11)
#define REG_MIC_BIASA_PULL_DOWN             (1 << 10)
#define REG_MIC_BIASB_PULL_DOWN_DR          (1 << 9)
#define REG_MIC_BIASB_PULL_DOWN             (1 << 8)
#define REG_LP_BIAS_SEL_LDO_SHIFT           5
#define REG_LP_BIAS_SEL_LDO_MASK            (0x7 << REG_LP_BIAS_SEL_LDO_SHIFT)
#define REG_LP_BIAS_SEL_LDO(n)              BITFIELD_VAL(REG_LP_BIAS_SEL_LDO, n)
#define EN_VBG_TEST                         (1 << 3)
#define REG_PU_AVDD25_ANA                   (1 << 2)
#define REG_BYPASS_VCORE_V                  (1 << 1)
#define REG_BYPASS_VCORE_L                  (1 << 0)

// REG_0E
#define ISO_EN_INTERFACE_USB                (1 << 15)
#define ISO_EN_INTERFACE_TX                 (1 << 14)
#define ISO_EN_INTERFACE_RF                 (1 << 13)
#define ISO_EN_INTERFACE_ANA                (1 << 12)
#define PU_INTERFACE_USB                    (1 << 11)
#define PU_INTERFACE_TX                     (1 << 10)
#define PU_INTERFACE_RF                     (1 << 9)
#define PU_INTERFACE_ANA                    (1 << 8)
#define BG_AUX_EN_DR                        (1 << 1)
#define BG_AUX_EN_REG                       (1 << 0)

// REG_0F
#define RESERVED_ANA_22                     (1 << 15)
#define REG_UVLO_SEL_SHIFT                  12
#define REG_UVLO_SEL_MASK                   (0x3 << REG_UVLO_SEL_SHIFT)
#define REG_UVLO_SEL(n)                     BITFIELD_VAL(REG_UVLO_SEL, n)
#define POWER_UP_SOFT_CNT_SHIFT             8
#define POWER_UP_SOFT_CNT_MASK              (0xF << POWER_UP_SOFT_CNT_SHIFT)
#define POWER_UP_SOFT_CNT(n)                BITFIELD_VAL(POWER_UP_SOFT_CNT, n)
#define POWER_UP_BIAS_CNT_SHIFT             4
#define POWER_UP_BIAS_CNT_MASK              (0xF << POWER_UP_BIAS_CNT_SHIFT)
#define POWER_UP_BIAS_CNT(n)                BITFIELD_VAL(POWER_UP_BIAS_CNT, n)

// REG_10
#define POWER_UP_MOD1_CNT_SHIFT             0
#define POWER_UP_MOD1_CNT_MASK              (0xFF << POWER_UP_MOD1_CNT_SHIFT)
#define POWER_UP_MOD1_CNT(n)                BITFIELD_VAL(POWER_UP_MOD1_CNT, n)
#define POWER_UP_MOD2_CNT_SHIFT             8
#define POWER_UP_MOD2_CNT_MASK              (0xFF << POWER_UP_MOD2_CNT_SHIFT)
#define POWER_UP_MOD2_CNT(n)                BITFIELD_VAL(POWER_UP_MOD2_CNT, n)

// REG_11
#define POWER_UP_MOD3_CNT_SHIFT             0
#define POWER_UP_MOD3_CNT_MASK              (0xFF << POWER_UP_MOD3_CNT_SHIFT)
#define POWER_UP_MOD3_CNT(n)                BITFIELD_VAL(POWER_UP_MOD3_CNT, n)
#define POWER_UP_MOD4_CNT_SHIFT             8
#define POWER_UP_MOD4_CNT_MASK              (0xFF << POWER_UP_MOD4_CNT_SHIFT)
#define POWER_UP_MOD4_CNT(n)                BITFIELD_VAL(POWER_UP_MOD4_CNT, n)

// REG_12
#define POWER_UP_MOD5_CNT_SHIFT             0
#define POWER_UP_MOD5_CNT_MASK              (0xFF << POWER_UP_MOD5_CNT_SHIFT)
#define POWER_UP_MOD5_CNT(n)                BITFIELD_VAL(POWER_UP_MOD5_CNT, n)
#define POWER_UP_MOD6_CNT_SHIFT             8
#define POWER_UP_MOD6_CNT_MASK              (0xFF << POWER_UP_MOD6_CNT_SHIFT)
#define POWER_UP_MOD6_CNT(n)                BITFIELD_VAL(POWER_UP_MOD6_CNT, n)

// REG_13
#define POWER_UP_MOD7_CNT_SHIFT             0
#define POWER_UP_MOD7_CNT_MASK              (0xFF << POWER_UP_MOD7_CNT_SHIFT)
#define POWER_UP_MOD7_CNT(n)                BITFIELD_VAL(POWER_UP_MOD7_CNT, n)
#define POWER_UP_MOD8_CNT_SHIFT             8
#define POWER_UP_MOD8_CNT_MASK              (0xFF << POWER_UP_MOD8_CNT_SHIFT)
#define POWER_UP_MOD8_CNT(n)                BITFIELD_VAL(POWER_UP_MOD8_CNT, n)

// REG_14
#define TEST_MODE_SHIFT                     13
#define TEST_MODE_MASK                      (0x7 << TEST_MODE_SHIFT)
#define TEST_MODE(n)                        BITFIELD_VAL(TEST_MODE, n)
#define REG_EFUSE_REDUNDANCY_DATA_OUT_SHIFT 2
#define REG_EFUSE_REDUNDANCY_DATA_OUT_MASK  (0xF << REG_EFUSE_REDUNDANCY_DATA_OUT_SHIFT)
#define REG_EFUSE_REDUNDANCY_DATA_OUT(n)    BITFIELD_VAL(REG_EFUSE_REDUNDANCY_DATA_OUT, n)
#define REG_EFUSE_REDUNDANCY_DATA_OUT_DR    (1 << 1)
#define REG_EFUSE_DATA_OUT_DR               (1 << 0)

// REG_15
#define REG_EFUSE_DATA_OUT_HI_SHIFT         0
#define REG_EFUSE_DATA_OUT_HI_MASK          (0xFFFF << REG_EFUSE_DATA_OUT_HI_SHIFT)
#define REG_EFUSE_DATA_OUT_HI(n)            BITFIELD_VAL(REG_EFUSE_DATA_OUT_HI, n)

// REG_16
#define REG_EFUSE_DATA_OUT_LO_SHIFT         0
#define REG_EFUSE_DATA_OUT_LO_MASK          (0xFFFF << REG_EFUSE_DATA_OUT_LO_SHIFT)
#define REG_EFUSE_DATA_OUT_LO(n)            BITFIELD_VAL(REG_EFUSE_DATA_OUT_LO, n)

// REG_17
#define REG_SS_VCORE_EN                     (1 << 15)
#define REG_PWR_SEL_REG                     (1 << 14)
#define REG_PULLDOWN_VANA2VCORE             (1 << 13)
#define REG_BYPASS_VANA2VCORE               (1 << 12)
#define POWER_OFF_ENABLE                    (1 << 0)

// REG_18
#define LP_EN_VANA2VCORE_LDO_DSLEEP_EN      (1 << 15)
#define LP_EN_VANA2VCORE_LDO_DR             (1 << 14)
#define LP_EN_VANA2VCORE_LDO_REG            (1 << 13)
#define REG_PU_VANA2VCORE_LDO_DSLEEP_MSK    (1 << 12)
#define REG_PU_VANA2VCORE_LDO_DR            (1 << 11)
#define REG_PU_VANA2VCORE_LDO_REG           (1 << 10)
#define LDO_VANA2VCORE_VBIT_DSLEEP_SHIFT    5
#define LDO_VANA2VCORE_VBIT_DSLEEP_MASK     (0x1F << LDO_VANA2VCORE_VBIT_DSLEEP_SHIFT)
#define LDO_VANA2VCORE_VBIT_DSLEEP(n)       BITFIELD_VAL(LDO_VANA2VCORE_VBIT_DSLEEP, n)
#define LDO_VANA2VCORE_VBIT_NORMAL_SHIFT    0
#define LDO_VANA2VCORE_VBIT_NORMAL_MASK     (0x1F << LDO_VANA2VCORE_VBIT_NORMAL_SHIFT)
#define LDO_VANA2VCORE_VBIT_NORMAL(n)       BITFIELD_VAL(LDO_VANA2VCORE_VBIT_NORMAL, n)


// REG_19
#define REG_PU_SHARE_MEM8                   (1 << 8)
#define REG_PU_SHARE_MEM7                   (1 << 7)
#define REG_PU_SHARE_MEM6                   (1 << 6)
#define REG_PU_SHARE_MEM5                   (1 << 5)
#define REG_PU_SHARE_MEM4                   (1 << 4)
#define REG_PU_CODEC_MEM                    (1 << 3)
#define REG_PU_BT_MEM                       (1 << 2)
#define REG_PU_SENS_MEM                     (1 << 1)
#define REG_PU_MCU_MEM                      (1 << 0)

// REG_1B
#define REG_PU_SHARE_MEM8_DR                (1 << 8)
#define REG_PU_SHARE_MEM7_DR                (1 << 7)
#define REG_PU_SHARE_MEM6_DR                (1 << 6)
#define REG_PU_SHARE_MEM5_DR                (1 << 5)
#define REG_PU_SHARE_MEM4_DR                (1 << 4)
#define REG_PU_CODEC_MEM_DR                 (1 << 3)
#define REG_PU_BT_MEM_DR                    (1 << 2)
#define REG_PU_SENS_MEM_DR                  (1 << 1)
#define REG_PU_MCU_MEM_DR                   (1 << 0)


// REG_1A
#define REG_PMU_VSEL1_SHIFT                 13
#define REG_PMU_VSEL1_MASK                  (0x7 << REG_PMU_VSEL1_SHIFT)
#define REG_PMU_VSEL1(n)                    BITFIELD_VAL(REG_PMU_VSEL1, n)
#define REG_POWER_SEL_CNT_SHIFT             8
#define REG_POWER_SEL_CNT_MASK              (0x1F << REG_POWER_SEL_CNT_SHIFT)
#define REG_POWER_SEL_CNT(n)                BITFIELD_VAL(REG_POWER_SEL_CNT, n)
#define REG_PWR_SEL_DR                      (1 << 7)
#define REG_PWR_SEL                         (1 << 6)
#define CLK_BG_EN                           (1 << 5)
#define CLK_BG_DIV_VALUE_SHIFT              0
#define CLK_BG_DIV_VALUE_MASK               (0x1F << CLK_BG_DIV_VALUE_SHIFT)
#define CLK_BG_DIV_VALUE(n)                 BITFIELD_VAL(CLK_BG_DIV_VALUE, n)


// REG_1D
#define CHIP_ADDR_I2C_SHIFT                 8
#define CHIP_ADDR_I2C_MASK                  (0x7F << CHIP_ADDR_I2C_SHIFT)
#define CHIP_ADDR_I2C(n)                    BITFIELD_VAL(CHIP_ADDR_I2C, n)
#define SLEEP_ALLOW                         (1 << 15)

// REG_1E
#define RESERVED_ANA_24                     (1 << 15)

// REG_1F
#define EXT_INTR_MSK                        (1 << 15)
#define RTC_INT1_MSK                        (1 << 14)
#define RTC_INT0_MSK                        (1 << 13)

// REG_20
#define EXT_INTR_EN                         (1 << 15)
#define RTC_INT_EN_1                        (1 << 14)
#define RTC_INT_EN_0                        (1 << 13)

// REG_21
#define RESERVED_ANA_26_25_SHIFT            14
#define RESERVED_ANA_26_25_MASK             (0x3 << RESERVED_ANA_26_25_SHIFT)
#define RESERVED_ANA_26_25(n)               BITFIELD_VAL(RESERVED_ANA_26_25, n)
#define REG_VCORE_V_RAMP_EN                 (1 << 9)
#define REG_VCORE_L_RAMP_EN                 (1 << 8)
#define REG_VANA2VCORE_RAMP_EN              (1 << 7)
#define REG_VCORE_RAMP_STEP_SHIFT           0
#define REG_VCORE_RAMP_STEP_MASK            (0x1F << REG_VCORE_RAMP_STEP_SHIFT)
#define REG_VCORE_RAMP_STEP(n)              BITFIELD_VAL(REG_VCORE_RAMP_STEP, n)

// REG_22
#define CFG_DIV_RTC_1HZ_SHIFT               0
#define CFG_DIV_RTC_1HZ_MASK                (0xFFFF << CFG_DIV_RTC_1HZ_SHIFT)
#define CFG_DIV_RTC_1HZ(n)                  BITFIELD_VAL(CFG_DIV_RTC_1HZ, n)

// REG_23
#define RTC_LOAD_VALUE_15_0_SHIFT           0
#define RTC_LOAD_VALUE_15_0_MASK            (0xFFFF << RTC_LOAD_VALUE_15_0_SHIFT)
#define RTC_LOAD_VALUE_15_0(n)              BITFIELD_VAL(RTC_LOAD_VALUE_15_0, n)

// REG_24
#define RTC_LOAD_VALUE_31_16_SHIFT          0
#define RTC_LOAD_VALUE_31_16_MASK           (0xFFFF << RTC_LOAD_VALUE_31_16_SHIFT)
#define RTC_LOAD_VALUE_31_16(n)             BITFIELD_VAL(RTC_LOAD_VALUE_31_16, n)

// REG_25
#define RTC_MATCH_VALUE_0_15_0_SHIFT        0
#define RTC_MATCH_VALUE_0_15_0_MASK         (0xFFFF << RTC_MATCH_VALUE_0_15_0_SHIFT)
#define RTC_MATCH_VALUE_0_15_0(n)           BITFIELD_VAL(RTC_MATCH_VALUE_0_15_0, n)

// REG_26
#define RTC_MATCH_VALUE_0_31_16_SHIFT       0
#define RTC_MATCH_VALUE_0_31_16_MASK        (0xFFFF << RTC_MATCH_VALUE_0_31_16_SHIFT)
#define RTC_MATCH_VALUE_0_31_16(n)          BITFIELD_VAL(RTC_MATCH_VALUE_0_31_16, n)

// REG_27
#define RTC_MATCH_VALUE_1_15_0_SHIFT        0
#define RTC_MATCH_VALUE_1_15_0_MASK         (0xFFFF << RTC_MATCH_VALUE_1_15_0_SHIFT)
#define RTC_MATCH_VALUE_1_15_0(n)           BITFIELD_VAL(RTC_MATCH_VALUE_1_15_0, n)

// REG_28
#define RTC_MATCH_VALUE_1_31_16_SHIFT       0
#define RTC_MATCH_VALUE_1_31_16_MASK        (0xFFFF << RTC_MATCH_VALUE_1_31_16_SHIFT)
#define RTC_MATCH_VALUE_1_31_16(n)          BITFIELD_VAL(RTC_MATCH_VALUE_1_31_16, n)

// REG_29
#define REG_MIC_BIASA_CHANSEL_SHIFT         14
#define REG_MIC_BIASA_CHANSEL_MASK          (0x3 << REG_MIC_BIASA_CHANSEL_SHIFT)
#define REG_MIC_BIASA_CHANSEL(n)            BITFIELD_VAL(REG_MIC_BIASA_CHANSEL, n)
#define REG_MIC_BIASA_EN                    (1 << 13)
#define REG_MIC_BIASA_ENLPF                 (1 << 12)
#define REG_MIC_BIASA_LPFSEL_SHIFT          9
#define REG_MIC_BIASA_LPFSEL_MASK           (0x7 << REG_MIC_BIASA_LPFSEL_SHIFT)
#define REG_MIC_BIASA_LPFSEL(n)             BITFIELD_VAL(REG_MIC_BIASA_LPFSEL, n)
#define REG_MIC_BIASA_VSEL_SHIFT            3
#define REG_MIC_BIASA_VSEL_MASK             (0x3F << REG_MIC_BIASA_VSEL_SHIFT)
#define REG_MIC_BIASA_VSEL(n)               BITFIELD_VAL(REG_MIC_BIASA_VSEL, n)
#define RESERVED_ANA_28_27_SHIFT            1
#define RESERVED_ANA_28_27_MASK             (0x3 << RESERVED_ANA_28_27_SHIFT)
#define RESERVED_ANA_28_27(n)               BITFIELD_VAL(RESERVED_ANA_28_27, n)
#define RESERVED_ANA_23                     (1 << 0)

// REG_2A
#define REG_MIC_BIASB_CHANSEL_SHIFT         14
#define REG_MIC_BIASB_CHANSEL_MASK          (0x3 << REG_MIC_BIASB_CHANSEL_SHIFT)
#define REG_MIC_BIASB_CHANSEL(n)            BITFIELD_VAL(REG_MIC_BIASB_CHANSEL, n)
#define REG_MIC_BIASB_EN                    (1 << 13)
#define REG_MIC_BIASB_ENLPF                 (1 << 12)
#define REG_MIC_BIASB_LPFSEL_SHIFT          9
#define REG_MIC_BIASB_LPFSEL_MASK           (0x7 << REG_MIC_BIASB_LPFSEL_SHIFT)
#define REG_MIC_BIASB_LPFSEL(n)             BITFIELD_VAL(REG_MIC_BIASB_LPFSEL, n)
#define REG_MIC_BIASB_VSEL_SHIFT            3
#define REG_MIC_BIASB_VSEL_MASK             (0x3F << REG_MIC_BIASB_VSEL_SHIFT)
#define REG_MIC_BIASB_VSEL(n)               BITFIELD_VAL(REG_MIC_BIASB_VSEL, n)
#define REG_MIC_LDO_EN                      (1 << 2)
#define REG_MIC_LDO_PULLDOWN                (1 << 1)
#define REG_MIC_LDO_LOOPCTRL                (1 << 0)

// REG_2B
#define REG_CLK_26M_DIV_SHIFT               0
#define REG_CLK_26M_DIV_MASK                (0x3FF << REG_CLK_26M_DIV_SHIFT)
#define REG_CLK_26M_DIV(n)                  BITFIELD_VAL(REG_CLK_26M_DIV, n)

// REG_2C
#define REG_PU_VCORE_V_LDO_DIG_DR           (1 << 9)
#define REG_VCORE_V_VOLTAGE_SEL_DR          (1 << 8)
#define REG_VCORE_V_VOLTAGE_SEL_SHIFT       0
#define REG_VCORE_V_VOLTAGE_SEL_MASK        (0xFF << REG_VCORE_V_VOLTAGE_SEL_SHIFT)
#define REG_VCORE_V_VOLTAGE_SEL(n)          BITFIELD_VAL(REG_VCORE_V_VOLTAGE_SEL, n)

// REG_2D
#define LDO_VCORE_V_VBIT_DSLEEP_SHIFT       8
#define LDO_VCORE_V_VBIT_DSLEEP_MASK        (0xFF << LDO_VCORE_V_VBIT_DSLEEP_SHIFT)
#define LDO_VCORE_V_VBIT_DSLEEP(n)          BITFIELD_VAL(LDO_VCORE_V_VBIT_DSLEEP, n)
#define LDO_VCORE_V_VBIT_NORMAL_SHIFT       0
#define LDO_VCORE_V_VBIT_NORMAL_MASK        (0xFF << LDO_VCORE_V_VBIT_NORMAL_SHIFT)
#define LDO_VCORE_V_VBIT_NORMAL(n)          BITFIELD_VAL(LDO_VCORE_V_VBIT_NORMAL, n)
#define MAX_LDO_VCORE_V_VBIT_VAL            (LDO_VCORE_V_VBIT_NORMAL_MASK >> LDO_VCORE_V_VBIT_NORMAL_SHIFT)

// REG_30
#define RESERVED_ANA_29                     (1 << 15)
#define RESETN_DB_NUMBER_SHIFT              11
#define RESETN_DB_NUMBER_MASK               (0xF << RESETN_DB_NUMBER_SHIFT)
#define RESETN_DB_NUMBER(n)                 BITFIELD_VAL(RESETN_DB_NUMBER, n)
#define RESETN_DB_EN                        (1 << 10)
#define REG_MIC_LDO_RES_SHIFT               0
#define REG_MIC_LDO_RES_MASK                (0x1F << REG_MIC_LDO_RES_SHIFT)
#define REG_MIC_LDO_RES(n)                  BITFIELD_VAL(REG_MIC_LDO_RES, n)

// REG_34
#define RESERVED_ANA_30                     (1 << 15)
#define CLK_32K_COUNT_NUM_SHIFT             11
#define CLK_32K_COUNT_NUM_MASK              (0xF << CLK_32K_COUNT_NUM_SHIFT)
#define CLK_32K_COUNT_NUM(n)                BITFIELD_VAL(CLK_32K_COUNT_NUM, n)
#define DEEPSLEEP_DLY_DR                    (1 << 7)
#define PU_OSC_DLY_VALUE_SHIFT              1
#define PU_OSC_DLY_VALUE_MASK               (0x3F << PU_OSC_DLY_VALUE_SHIFT)
#define PU_OSC_DLY_VALUE(n)                 BITFIELD_VAL(PU_OSC_DLY_VALUE, n)
#define PU_OSC_DLY_DR                       (1 << 0)

// REG_35
#define REG_DIV_HW_RESET_SHIFT              0
#define REG_DIV_HW_RESET_MASK               (0xFFFF << REG_DIV_HW_RESET_SHIFT)
#define REG_DIV_HW_RESET(n)                 BITFIELD_VAL(REG_DIV_HW_RESET, n)

// REG_36
#define REG_WDT_TIMER_SHIFT                 0
#define REG_WDT_TIMER_MASK                  (0xFFFF << REG_WDT_TIMER_SHIFT)
#define REG_WDT_TIMER(n)                    BITFIELD_VAL(REG_WDT_TIMER, n)

// REG_37
#define RESERVED_ANA_32_31_SHIFT            14
#define RESERVED_ANA_32_31_MASK             (0x3 << RESERVED_ANA_32_31_SHIFT)
#define RESERVED_ANA_32_31(n)               BITFIELD_VAL(RESERVED_ANA_32_31, n)
#define CLK_32K_COUNT_EN_TRIGGER            (1 << 13)
#define CLK_32K_COUNT_EN                    (1 << 12)
#define CLK_32K_COUNT_CLOCK_EN              (1 << 11)
#define POWERON_DETECT_EN                   (1 << 10)
#define REG_WDT_EN                          (1 << 8)
#define REG_WDT_RESET_EN                    (1 << 7)
#define REG_HW_RESET_TIME_SHIFT             1
#define REG_HW_RESET_TIME_MASK              (0x3F << REG_HW_RESET_TIME_SHIFT)
#define REG_HW_RESET_TIME(n)                BITFIELD_VAL(REG_HW_RESET_TIME, n)
#define REG_HW_RESET_EN                     (1 << 0)

// REG_38
#define RESERVED_ANA_15_0_SHIFT             0
#define RESERVED_ANA_15_0_MASK              (0xFFFF << RESERVED_ANA_15_0_SHIFT)
#define RESERVED_ANA_15_0(n)                BITFIELD_VAL(RESERVED_ANA_15_0, n)

// REG_39
#define RESERVED_DIG_15_0_SHIFT             0
#define RESERVED_DIG_15_0_MASK              (0xFFFF << RESERVED_DIG_15_0_SHIFT)
#define RESERVED_DIG_15_0(n)                BITFIELD_VAL(RESERVED_DIG_15_0, n)

// REG_3A
#define SOFT_MODE_SHIFT                     0
#define SOFT_MODE_MASK                      (0xFFFF << SOFT_MODE_SHIFT)
#define SOFT_MODE(n)                        BITFIELD_VAL(SOFT_MODE, n)

// REG_3B
#define EFUSE_REDUNDANCY_DATA_OUT_SHIFT     0
#define EFUSE_REDUNDANCY_DATA_OUT_MASK      (0xF << EFUSE_REDUNDANCY_DATA_OUT_SHIFT)
#define EFUSE_REDUNDANCY_DATA_OUT(n)        BITFIELD_VAL(EFUSE_REDUNDANCY_DATA_OUT, n)

// REG_3F
#define REG_SLEEP                           (1 << 1)
#define REG_POWER_ON                        (1 << 0)

// REG_43
#define EFUSE_REDUNDANCY_INFO_ROW_SEL_DR    (1 << 15)
#define EFUSE_REDUNDANCY_INFO_ROW_SEL_REG   (1 << 14)


// REG_45
#define EFUSE_CHIP_SEL_ENB_DR               (1 << 15)
#define EFUSE_CHIP_SEL_ENB_REG              (1 << 14)
#define EFUSE_STROBE_DR                     (1 << 13)
#define EFUSE_STROBE_REG                    (1 << 12)
#define EFUSE_LOAD_DR                       (1 << 11)
#define EFUSE_LOAD_REG                      (1 << 10)
#define EFUSE_PGM_ENB_DR                    (1 << 9)
#define EFUSE_PGM_ENB_REG                   (1 << 8)
#define EFUSE_PGM_SEL_DR                    (1 << 7)
#define EFUSE_PGM_SEL_REG                   (1 << 6)
#define EFUSE_READ_MODE_DR                  (1 << 5)
#define EFUSE_READ_MODE_REG                 (1 << 4)
#define EFUSE_PWR_DN_ENB_DR                 (1 << 3)
#define EFUSE_PWR_DN_ENB_REG                (1 << 2)
#define EFUSE_REDUNDANCY_EN_DR              (1 << 1)
#define EFUSE_REDUNDANCY_EN_REG             (1 << 0)



// REG_46
#define RESERVED_ANA_33                     (1 << 15)
#define REG_EFUSE_ADDRESS_SHIFT             6
#define REG_EFUSE_ADDRESS_MASK              (0x1FF << REG_EFUSE_ADDRESS_SHIFT)
#define REG_EFUSE_ADDRESS(n)                BITFIELD_VAL(REG_EFUSE_ADDRESS, n)
#define REG_EFUSE_STROBE_TRIGGER            (1 << 5)
#define REG_EFUSE_TURN_ON                   (1 << 4)
#define REG_EFUSE_CLK_EN                    (1 << 3)
#define REG_EFUSE_READ_MODE                 (1 << 2)
#define REG_EFUSE_PGM_MODE                  (1 << 1)
#define REG_EFUSE_PGM_READ_SEL              (1 << 0)

// REG_47
#define REG_EFUSE_TIME_CSB_ADDR_VALUE_SHIFT 10
#define REG_EFUSE_TIME_CSB_ADDR_VALUE_MASK  (0xF << REG_EFUSE_TIME_CSB_ADDR_VALUE_SHIFT)
#define REG_EFUSE_TIME_CSB_ADDR_VALUE(n)    BITFIELD_VAL(REG_EFUSE_TIME_CSB_ADDR_VALUE, n)
#define REG_EFUSE_TIME_PS_CSB_VALUE_SHIFT   6
#define REG_EFUSE_TIME_PS_CSB_VALUE_MASK    (0xF << REG_EFUSE_TIME_PS_CSB_VALUE_SHIFT)
#define REG_EFUSE_TIME_PS_CSB_VALUE(n)      BITFIELD_VAL(REG_EFUSE_TIME_PS_CSB_VALUE, n)
#define REG_EFUSE_TIME_PD_PS_VALUE_SHIFT    0
#define REG_EFUSE_TIME_PD_PS_VALUE_MASK     (0x3F << REG_EFUSE_TIME_PD_PS_VALUE_SHIFT)
#define REG_EFUSE_TIME_PD_PS_VALUE(n)       BITFIELD_VAL(REG_EFUSE_TIME_PD_PS_VALUE, n)


// REG_48
#define REG_EFUSE_TIME_PGM_STROBING_VALUE_SHIFT 4
#define REG_EFUSE_TIME_PGM_STROBING_VALUE_MASK (0x1FF << REG_EFUSE_TIME_PGM_STROBING_VALUE_SHIFT)
#define REG_EFUSE_TIME_PGM_STROBING_VALUE(n) BITFIELD_VAL(REG_EFUSE_TIME_PGM_STROBING_VALUE, n)
#define REG_EFUSE_TIME_ADDR_STROBE_VALUE_SHIFT 0
#define REG_EFUSE_TIME_ADDR_STROBE_VALUE_MASK (0xF << REG_EFUSE_TIME_ADDR_STROBE_VALUE_SHIFT)
#define REG_EFUSE_TIME_ADDR_STROBE_VALUE(n) BITFIELD_VAL(REG_EFUSE_TIME_ADDR_STROBE_VALUE, n)

// REG_49
#define REG_EFUSE_TIME_READ_STROBING_VALUE_SHIFT 0
#define REG_EFUSE_TIME_READ_STROBING_VALUE_MASK (0x1FF << REG_EFUSE_TIME_READ_STROBING_VALUE_SHIFT)
#define REG_EFUSE_TIME_READ_STROBING_VALUE(n) BITFIELD_VAL(REG_EFUSE_TIME_READ_STROBING_VALUE, n)

// REG_4A
#define REG_EFUSE_TIME_PS_PD_VALUE_SHIFT    10
#define REG_EFUSE_TIME_PS_PD_VALUE_MASK     (0x3F << REG_EFUSE_TIME_PS_PD_VALUE_SHIFT)
#define REG_EFUSE_TIME_PS_PD_VALUE(n)       BITFIELD_VAL(REG_EFUSE_TIME_PS_PD_VALUE, n)
#define REG_EFUSE_TIME_CSB_PS_VALUE_SHIFT   6
#define REG_EFUSE_TIME_CSB_PS_VALUE_MASK    (0xF << REG_EFUSE_TIME_CSB_PS_VALUE_SHIFT)
#define REG_EFUSE_TIME_CSB_PS_VALUE(n)      BITFIELD_VAL(REG_EFUSE_TIME_CSB_PS_VALUE, n)
#define REG_EFUSE_TIME_STROBE_CSB_VALUE_SHIFT 0
#define REG_EFUSE_TIME_STROBE_CSB_VALUE_MASK (0x3F << REG_EFUSE_TIME_STROBE_CSB_VALUE_SHIFT)
#define REG_EFUSE_TIME_STROBE_CSB_VALUE(n)  BITFIELD_VAL(REG_EFUSE_TIME_STROBE_CSB_VALUE, n)

// REG_4B
#define REG_EFUSE_TIME_PD_OFF_VALUE_SHIFT   0
#define REG_EFUSE_TIME_PD_OFF_VALUE_MASK    (0x3F << REG_EFUSE_TIME_PD_OFF_VALUE_SHIFT)
#define REG_EFUSE_TIME_PD_OFF_VALUE(n)      BITFIELD_VAL(REG_EFUSE_TIME_PD_OFF_VALUE, n)

// REG_4C
#define EFUSE_DATA_OUT_HI_SHIFT             0
#define EFUSE_DATA_OUT_HI_MASK              (0xFFFF << EFUSE_DATA_OUT_HI_SHIFT)
#define EFUSE_DATA_OUT_HI(n)                BITFIELD_VAL(EFUSE_DATA_OUT_HI, n)

// REG_4D
#define EFUSE_DATA_OUT_LO_SHIFT             0
#define EFUSE_DATA_OUT_LO_MASK              (0xFFFF << EFUSE_DATA_OUT_LO_SHIFT)
#define EFUSE_DATA_OUT_LO(n)                BITFIELD_VAL(EFUSE_DATA_OUT_LO, n)

// REG_4E
#define CLK_32K_COUNTER_26M_SHIFT           0
#define CLK_32K_COUNTER_26M_MASK            (0x7FFF << CLK_32K_COUNTER_26M_SHIFT)
#define CLK_32K_COUNTER_26M(n)              BITFIELD_VAL(CLK_32K_COUNTER_26M, n)
#define CLK_32K_COUNTER_26M_READY           (1 << 15)

// REG_4F
#define EXT_INTR_IN                         (1 << 15)
#define HARDWARE_POWER_OFF_EN               (1 << 1)
#define SOFT_POWER_OFF                      (1 << 0)

// REG_50
#define EXT_INTR                            (1 << 15)
#define RTC_INT_1                           (1 << 14)
#define RTC_INT_0                           (1 << 13)


// REG_51
#define EXT_INTR_MSKED                      (1 << 15)
#define RTC_INT1_MSKED                      (1 << 14)
#define RTC_INT0_MSKED                      (1 << 13)

#define EXT_INTR_CLR                        (1 << 15)
#define RTC_INT_CLR_1                       (1 << 14)
#define RTC_INT_CLR_0                       (1 << 13)

// REG_52
#define PMU_LDO_ON_SOURCE_SHIFT             4
#define PMU_LDO_ON_SOURCE_MASK              (0x3 << PMU_LDO_ON_SOURCE_SHIFT)
#define PMU_LDO_ON_SOURCE(n)                BITFIELD_VAL(PMU_LDO_ON_SOURCE, n)
#define RTC_LOAD                            (1 << 3)
#define WDT_LOAD                            (1 << 2)
#define POWER_ON                            (1 << 0)

// REG_53
#define REG_VTOI_POUT_IDLE_15_0_SHIFT       0
#define REG_VTOI_POUT_IDLE_15_0_MASK        (0xFFFF << REG_VTOI_POUT_IDLE_15_0_SHIFT)
#define REG_VTOI_POUT_IDLE_15_0(n)          BITFIELD_VAL(REG_VTOI_POUT_IDLE_15_0, n)

// REG_54
#define REG_VTOI_POUT_IDLE_31_16_SHIFT      0
#define REG_VTOI_POUT_IDLE_31_16_MASK       (0xFFFF << REG_VTOI_POUT_IDLE_31_16_SHIFT)
#define REG_VTOI_POUT_IDLE_31_16(n)         BITFIELD_VAL(REG_VTOI_POUT_IDLE_31_16, n)

// REG_55
#define REG_VTOI_POUT_IDLE_39_32_SHIFT      0
#define REG_VTOI_POUT_IDLE_39_32_MASK       (0xFF << REG_VTOI_POUT_IDLE_39_32_SHIFT)
#define REG_VTOI_POUT_IDLE_39_32(n)         BITFIELD_VAL(REG_VTOI_POUT_IDLE_39_32, n)

// REG_56
#define REG_VTOI_POUT_IDLE_49_40_SHIFT      0
#define REG_VTOI_POUT_IDLE_49_40_MASK       (0x3FF << REG_VTOI_POUT_IDLE_49_40_SHIFT)
#define REG_VTOI_POUT_IDLE_49_40(n)         BITFIELD_VAL(REG_VTOI_POUT_IDLE_49_40, n)

// REG_57
#define REG_VTOI_POUT_NORMAL_15_0_SHIFT     0
#define REG_VTOI_POUT_NORMAL_15_0_MASK      (0xFFFF << REG_VTOI_POUT_NORMAL_15_0_SHIFT)
#define REG_VTOI_POUT_NORMAL_15_0(n)        BITFIELD_VAL(REG_VTOI_POUT_NORMAL_15_0, n)

// REG_58
#define REG_VTOI_POUT_NORMAL_31_16_SHIFT    0
#define REG_VTOI_POUT_NORMAL_31_16_MASK     (0xFFFF << REG_VTOI_POUT_NORMAL_31_16_SHIFT)
#define REG_VTOI_POUT_NORMAL_31_16(n)       BITFIELD_VAL(REG_VTOI_POUT_NORMAL_31_16, n)

// REG_59
#define REG_VTOI_POUT_NORMAL_39_32_SHIFT    0
#define REG_VTOI_POUT_NORMAL_39_32_MASK     (0xFF << REG_VTOI_POUT_NORMAL_39_32_SHIFT)
#define REG_VTOI_POUT_NORMAL_39_32(n)       BITFIELD_VAL(REG_VTOI_POUT_NORMAL_39_32, n)

// REG_5A
#define REG_VTOI_POUT_NORMAL_49_40_SHIFT    0
#define REG_VTOI_POUT_NORMAL_49_40_MASK     (0x3FF << REG_VTOI_POUT_NORMAL_49_40_SHIFT)
#define REG_VTOI_POUT_NORMAL_49_40(n)       BITFIELD_VAL(REG_VTOI_POUT_NORMAL_49_40, n)


// REG_5B
#define REG_RTC_EXT_INTR_SEL                (1 << 9)
#define REG_VTOI_IABS_POUT_SHIFT            6
#define REG_VTOI_IABS_POUT_MASK             (0x7 << REG_VTOI_IABS_POUT_SHIFT)
#define REG_VTOI_IABS_POUT(n)               BITFIELD_VAL(REG_VTOI_IABS_POUT, n)
#define REG_VTOI_IABS_SHIFT                 1
#define REG_VTOI_IABS_MASK                  (0x1F << REG_VTOI_IABS_SHIFT)
#define REG_VTOI_IABS(n)                    BITFIELD_VAL(REG_VTOI_IABS, n)
#define REG_VTOI_IABS_EN                    (1 << 0)



// REG_5C
#define RTC_VALUE_31_16_SHIFT               0
#define RTC_VALUE_31_16_MASK                (0xFFFF << RTC_VALUE_31_16_SHIFT)
#define RTC_VALUE_31_16(n)                  BITFIELD_VAL(RTC_VALUE_31_16, n)

// REG_5D
#define RTC_VALUE_15_0_SHIFT                0
#define RTC_VALUE_15_0_MASK                 (0xFFFF << RTC_VALUE_15_0_SHIFT)
#define RTC_VALUE_15_0(n)                   BITFIELD_VAL(RTC_VALUE_15_0, n)

// REG_5E
#define POWER_ON_RELEASE                    (1 << 15)
#define POWER_ON_PRESS                      (1 << 14)
#define PU_OSC_OUT                          (1 << 13)
#define DIG_PU_VCORE_L_LDO                  (1 << 12)
#define DIG_LP_EN_VCORE_L_LDO               (1 << 11)
#define DIG_PU_VCORE_V_LDO                  (1 << 10)
#define DIG_LP_EN_VCORE_V_LDO               (1 << 9)
#define DIG_MIC_LDO_EN                      (1 << 8)
#define DIG_MIC_LP_ENABLE                   (1 << 7)
#define DIG_MIC_BIASA_EN                    (1 << 6)
#define DIG_MIC_BIASB_EN                    (1 << 5)
#define DIG_VTOI_EN                         (1 << 4)
#define DIG_VTOI_LP_EN                      (1 << 3)
#define LDO_BIAS_ENABLE                     (1 << 2)
#define LDO_LP_BIAS_ENABLE                  (1 << 1)
#define LP_MODE_RTC                         (1 << 0)

#if 0
// REG_5F
#define DEEPSLEEP_MODE                      (1 << 13)
#define UVLO_LV                             (1 << 11)
#define BG_AUX_EN                           (1 << 5)
#define BG_VBG_SEL                          (1 << 4)
#define PMU_LDO_ON                          (1 << 3)
#define BG_NOTCH_EN                         (1 << 2)
#define BG_CORE_EN                          (1 << 1)
#endif

#ifdef __cplusplus
}
#endif

#endif

