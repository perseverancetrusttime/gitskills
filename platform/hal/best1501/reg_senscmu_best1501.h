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
#ifndef __REG_SENSCMU_BEST1501_H__
#define __REG_SENSCMU_BEST1501_H__

#include "plat_types.h"

struct SENSCMU_T {
    __IO uint32_t HCLK_ENABLE;      // 0x00
    __IO uint32_t HCLK_DISABLE;     // 0x04
    __IO uint32_t PCLK_ENABLE;      // 0x08
    __IO uint32_t PCLK_DISABLE;     // 0x0C
    __IO uint32_t OCLK_ENABLE;      // 0x10
    __IO uint32_t OCLK_DISABLE;     // 0x14
    __IO uint32_t HCLK_MODE;        // 0x18
    __IO uint32_t PCLK_MODE;        // 0x1C
    __IO uint32_t OCLK_MODE;        // 0x20
    __IO uint32_t REG_RAM_CFG0;     // 0x24
    __IO uint32_t HRESET_PULSE;     // 0x28
    __IO uint32_t PRESET_PULSE;     // 0x2C
    __IO uint32_t ORESET_PULSE;     // 0x30
    __IO uint32_t HRESET_SET;       // 0x34
    __IO uint32_t HRESET_CLR;       // 0x38
    __IO uint32_t PRESET_SET;       // 0x3C
    __IO uint32_t PRESET_CLR;       // 0x40
    __IO uint32_t ORESET_SET;       // 0x44
    __IO uint32_t ORESET_CLR;       // 0x48
    __IO uint32_t TIMER0_CLK;       // 0x4C
    __IO uint32_t BOOTMODE;         // 0x50
    __IO uint32_t SENS_TIMER;       // 0x54
    __IO uint32_t SLEEP;            // 0x58
    __IO uint32_t CLK_OUT;          // 0x5C
    __IO uint32_t SYS_CLK_ENABLE;   // 0x60
    __IO uint32_t SYS_CLK_DISABLE;  // 0x64
    __IO uint32_t ADMA_CH15_REQ;    // 0x68
    __IO uint32_t REG_RAM_CFG1;     // 0x6C
    __IO uint32_t UART_CLK;         // 0x70
    __IO uint32_t I2C_CLK;          // 0x74
    __IO uint32_t SENS2MCU_MASK0;   // 0x78
    __IO uint32_t SENS2MCU_MASK1;   // 0x7C
    __IO uint32_t WRITE_UNLOCK;     // 0x80
    __IO uint32_t WAKEUP_MASK0;     // 0x84
    __IO uint32_t WAKEUP_MASK1;     // 0x88
    __IO uint32_t WAKEUP_CLK_CFG;   // 0x8C
    __IO uint32_t TIMER1_CLK;       // 0x90
    __IO uint32_t TIMER2_CLK;       // 0x94
    __IO uint32_t SENS2MCU_IRQ_SET; // 0x98
    __IO uint32_t SENS2MCU_IRQ_CLR; // 0x9C
    __IO uint32_t ISIRQ_SET;        // 0xA0
    __IO uint32_t ISIRQ_CLR;        // 0xA4
    __IO uint32_t SYS_DIV;          // 0xA8
    __IO uint32_t RESERVED_0AC;     // 0xAC
    __IO uint32_t SENS2BT_INTMASK0; // 0xB0
    __IO uint32_t SENS2BT_INTMASK1; // 0xB4
    __IO uint32_t RESERVED_0B8;     // 0xB8
    __IO uint32_t RESERVED_0BC;     // 0xBC
    __IO uint32_t MEMSC[4];         // 0xC0
    __I  uint32_t MEMSC_STATUS;     // 0xD0
    __IO uint32_t ADMA_CH0_4_REQ;   // 0xD4
    __IO uint32_t ADMA_CH5_9_REQ;   // 0xD8
    __IO uint32_t ADMA_CH10_14_REQ; // 0xDC
    __IO uint32_t MINIMA_CFG;       // 0xE0
    __IO uint32_t RESERVED_0E4[3];  // 0xE4
    __IO uint32_t MISC;             // 0xF0
    __IO uint32_t SIMU_RES;         // 0xF4
};

struct SAVED_SENSCMU_REGS_T {
    uint32_t HCLK_ENABLE;
    uint32_t PCLK_ENABLE;
    uint32_t OCLK_ENABLE;
    uint32_t HCLK_MODE;
    uint32_t PCLK_MODE;
    uint32_t OCLK_MODE;
    uint32_t REG_RAM_CFG0;
    uint32_t HRESET_CLR;
    uint32_t PRESET_CLR;
    uint32_t ORESET_CLR;
    uint32_t TIMER0_CLK;
    uint32_t BOOTMODE;
    uint32_t SENS_TIMER;
    uint32_t SLEEP;
    uint32_t CLK_OUT;
    uint32_t SYS_CLK_ENABLE;
    uint32_t ADMA_CH15_REQ;
    uint32_t REG_RAM_CFG1;
    uint32_t UART_CLK;
    uint32_t I2C_CLK;
    uint32_t SENS2MCU_MASK0;
    uint32_t SENS2MCU_MASK1;
    uint32_t WAKEUP_MASK0;
    uint32_t WAKEUP_MASK1;
    uint32_t WAKEUP_CLK_CFG;
    uint32_t TIMER1_CLK;
    uint32_t TIMER2_CLK;
    uint32_t SYS_DIV;
    uint32_t SENS2BT_INTMASK0;
    uint32_t SENS2BT_INTMASK1;
    uint32_t ADMA_CH0_4_REQ;
    uint32_t ADMA_CH5_9_REQ;
    uint32_t ADMA_CH10_14_REQ;
    uint32_t MINIMA_CFG;
    uint32_t MISC;
    uint32_t SIMU_RES;
};

// reg_00
#define SENS_CMU_MANUAL_HCLK_ENABLE(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_HCLK_ENABLE_MASK                    (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_HCLK_ENABLE_SHIFT                   (0)

// reg_04
#define SENS_CMU_MANUAL_HCLK_DISABLE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_HCLK_DISABLE_MASK                   (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_HCLK_DISABLE_SHIFT                  (0)

// reg_08
#define SENS_CMU_MANUAL_PCLK_ENABLE(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_PCLK_ENABLE_MASK                    (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_PCLK_ENABLE_SHIFT                   (0)

// reg_0c
#define SENS_CMU_MANUAL_PCLK_DISABLE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_PCLK_DISABLE_MASK                   (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_PCLK_DISABLE_SHIFT                  (0)

// reg_10
#define SENS_CMU_MANUAL_OCLK_ENABLE(n)                      (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_OCLK_ENABLE_MASK                    (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_OCLK_ENABLE_SHIFT                   (0)

// reg_14
#define SENS_CMU_MANUAL_OCLK_DISABLE(n)                     (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MANUAL_OCLK_DISABLE_MASK                   (0xFFFFFFFF << 0)
#define SENS_CMU_MANUAL_OCLK_DISABLE_SHIFT                  (0)

// reg_18
#define SENS_CMU_MODE_HCLK(n)                               (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MODE_HCLK_MASK                             (0xFFFFFFFF << 0)
#define SENS_CMU_MODE_HCLK_SHIFT                            (0)

// reg_1c
#define SENS_CMU_MODE_PCLK(n)                               (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MODE_PCLK_MASK                             (0xFFFFFFFF << 0)
#define SENS_CMU_MODE_PCLK_SHIFT                            (0)

// reg_20
#define SENS_CMU_MODE_OCLK(n)                               (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_MODE_OCLK_MASK                             (0xFFFFFFFF << 0)
#define SENS_CMU_MODE_OCLK_SHIFT                            (0)

// reg_24
#define SENS_CMU_RF_RET1N0                                  (1 << 0)
#define SENS_CMU_RF_RET2N0                                  (1 << 1)
#define SENS_CMU_RF_PGEN0                                   (1 << 2)
#define SENS_CMU_RF_RET1N1                                  (1 << 3)
#define SENS_CMU_RF_RET2N1                                  (1 << 4)
#define SENS_CMU_RF_PGEN1                                   (1 << 5)

// reg_28
#define SENS_CMU_HRESETN_PULSE(n)                           (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_HRESETN_PULSE_MASK                         (0xFFFFFFFF << 0)
#define SENS_CMU_HRESETN_PULSE_SHIFT                        (0)

#define SENS_PRST_NUM                                       18

// reg_2c
#define SENS_CMU_PRESETN_PULSE(n)                           (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_PRESETN_PULSE_MASK                         (0xFFFFFFFF << 0)
#define SENS_CMU_PRESETN_PULSE_SHIFT                        (0)
#define SENS_CMU_GLOBAL_RESETN_PULSE                        (1 << (SENS_PRST_NUM+1-1))

// reg_30
#define SENS_CMU_ORESETN_PULSE(n)                           (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_ORESETN_PULSE_MASK                         (0xFFFFFFFF << 0)
#define SENS_CMU_ORESETN_PULSE_SHIFT                        (0)

// reg_34
#define SENS_CMU_HRESETN_SET(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_HRESETN_SET_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_HRESETN_SET_SHIFT                          (0)

// reg_38
#define SENS_CMU_HRESETN_CLR(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_HRESETN_CLR_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_HRESETN_CLR_SHIFT                          (0)

// reg_3c
#define SENS_CMU_PRESETN_SET(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_PRESETN_SET_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_PRESETN_SET_SHIFT                          (0)
#define SENS_CMU_GLOBAL_RESETN_SET                          (1 << (SENS_PRST_NUM+1-1))

// reg_40
#define SENS_CMU_PRESETN_CLR(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_PRESETN_CLR_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_PRESETN_CLR_SHIFT                          (0)
#define SENS_CMU_GLOBAL_RESETN_CLR                          (1 << (SENS_PRST_NUM+1-1))

// reg_44
#define SENS_CMU_ORESETN_SET(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_ORESETN_SET_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_ORESETN_SET_SHIFT                          (0)

// reg_48
#define SENS_CMU_ORESETN_CLR(n)                             (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_ORESETN_CLR_MASK                           (0xFFFFFFFF << 0)
#define SENS_CMU_ORESETN_CLR_SHIFT                          (0)

// reg_4c
#define SENS_CMU_CFG_DIV_TIMER00(n)                         (((n) & 0xFFFF) << 0)
#define SENS_CMU_CFG_DIV_TIMER00_MASK                       (0xFFFF << 0)
#define SENS_CMU_CFG_DIV_TIMER00_SHIFT                      (0)
#define SENS_CMU_CFG_DIV_TIMER01(n)                         (((n) & 0xFFFF) << 16)
#define SENS_CMU_CFG_DIV_TIMER01_MASK                       (0xFFFF << 16)
#define SENS_CMU_CFG_DIV_TIMER01_SHIFT                      (16)

// reg_50
#define SENS_CMU_WATCHDOG_RESET                             (1 << 0)
#define SENS_CMU_SOFT_GLOBLE_RESET                          (1 << 1)
#define SENS_CMU_RTC_INTR_H                                 (1 << 2)
#define SENS_CMU_CHG_INTR_H                                 (1 << 3)
#define SENS_CMU_SOFT_BOOT_MODE(n)                          (((n) & 0xFFFFFFF) << 4)
#define SENS_CMU_SOFT_BOOT_MODE_MASK                        (0xFFFFFFF << 4)
#define SENS_CMU_SOFT_BOOT_MODE_SHIFT                       (4)

// reg_54
#define SENS_CMU_CFG_HCLK_MCU_OFF_TIMER(n)                  (((n) & 0xFF) << 0)
#define SENS_CMU_CFG_HCLK_MCU_OFF_TIMER_MASK                (0xFF << 0)
#define SENS_CMU_CFG_HCLK_MCU_OFF_TIMER_SHIFT               (0)
#define SENS_CMU_HCLK_MCU_ENABLE                            (1 << 8)
#define SENS_CMU_RAM_RETN_UP_EARLY                          (1 << 9)
#define SENS_CMU_DEBUG_REG_SEL(n)                           (((n) & 0x7) << 11)
#define SENS_CMU_DEBUG_REG_SEL_MASK                         (0x7 << 11)
#define SENS_CMU_DEBUG_REG_SEL_SHIFT                        (11)

// reg_58
#define SENS_CMU_SLEEP_TIMER(n)                             (((n) & 0xFFFFFF) << 0)
#define SENS_CMU_SLEEP_TIMER_MASK                           (0xFFFFFF << 0)
#define SENS_CMU_SLEEP_TIMER_SHIFT                          (0)
#define SENS_CMU_SLEEP_TIMER_EN                             (1 << 24)
#define SENS_CMU_DEEPSLEEP_EN                               (1 << 25)
#define SENS_CMU_DEEPSLEEP_ROMRAM_EN                        (1 << 26)
#define SENS_CMU_MANUAL_RAM_RETN                            (1 << 27)
#define SENS_CMU_DEEPSLEEP_START                            (1 << 28)
#define SENS_CMU_DEEPSLEEP_MODE                             (1 << 29)
#define SENS_CMU_PU_OSC                                     (1 << 30)
#define SENS_CMU_WAKEUP_DEEPSLEEP_L                         (1 << 31)

// reg_5c
#define SENS_CMU_SEL_32K_TIMER(n)                           (((n) & 0x7) << 0)
#define SENS_CMU_SEL_32K_TIMER_MASK                         (0x7 << 0)
#define SENS_CMU_SEL_32K_TIMER_SHIFT                        (0)
#define SENS_CMU_SEL_32K_WDT                                (1 << 3)
#define SENS_CMU_SEL_TIMER_FAST(n)                          (((n) & 0x7) << 4)
#define SENS_CMU_SEL_TIMER_FAST_MASK                        (0x7 << 4)
#define SENS_CMU_SEL_TIMER_FAST_SHIFT                       (4)
#define SENS_CMU_CFG_CLK_OUT(n)                             (((n) & 0xF) << 7)
#define SENS_CMU_CFG_CLK_OUT_MASK                           (0xF << 7)
#define SENS_CMU_CFG_CLK_OUT_SHIFT                          (7)
#define SENS_CMU_MASK_OBS(n)                                (((n) & 0x3F) << 11)
#define SENS_CMU_MASK_OBS_MASK                              (0x3F << 11)
#define SENS_CMU_MASK_OBS_SHIFT                             (11)
#define SENS_CMU_MASK_OBS_GPIO(n)                           (((n) & 0x3F) << 17)
#define SENS_CMU_MASK_OBS_GPIO_MASK                         (0x3F << 17)
#define SENS_CMU_MASK_OBS_GPIO_SHIFT                        (17)
#define SENS_CMU_BT_PLAYTIME_STAMP_MASK                     (1 << 23)
#define SENS_CMU_BT_PLAYTIME_STAMP1_MASK                    (1 << 24)
#define SENS_CMU_BT_PLAYTIME_STAMP2_MASK                    (1 << 25)
#define SENS_CMU_BT_PLAYTIME_STAMP3_MASK                    (1 << 26)

// reg_60
#define SENS_CMU_RSTN_DIV_SYS_ENABLE                        (1 << 0)
#define SENS_CMU_SEL_OSC_2_SYS_ENABLE                       (1 << 1)
#define SENS_CMU_SEL_OSC_4_SYS_ENABLE                       (1 << 2)
#define SENS_CMU_SEL_SLOW_SYS_ENABLE                        (1 << 3)
#define SENS_CMU_SEL_OSCX2_SYS_ENABLE                       (1 << 4)
#define SENS_CMU_SEL_FAST_SYS_ENABLE                        (1 << 5)
#define SENS_CMU_SEL_PLL_SYS_ENABLE                         (1 << 6)
#define SENS_CMU_BYPASS_DIV_SYS_ENABLE                      (1 << 7)
#define SENS_CMU_EN_OSCX2_ENABLE                            (1 << 8)
#define SENS_CMU_PU_OSCX2_ENABLE                            (1 << 9)
#define SENS_CMU_EN_OSCX4_ENABLE                            (1 << 10)
#define SENS_CMU_PU_OSCX4_ENABLE                            (1 << 11)
#define SENS_CMU_EN_PLL_ENABLE                              (1 << 12)
#define SENS_CMU_PU_PLL_ENABLE                              (1 << 13)

// reg_64
#define SENS_CMU_RSTN_DIV_SYS_DISABLE                       (1 << 0)
#define SENS_CMU_SEL_OSC_2_SYS_DISABLE                      (1 << 1)
#define SENS_CMU_SEL_OSC_4_SYS_DISABLE                      (1 << 2)
#define SENS_CMU_SEL_SLOW_SYS_DISABLE                       (1 << 3)
#define SENS_CMU_SEL_OSCX2_SYS_DISABLE                      (1 << 4)
#define SENS_CMU_SEL_FAST_SYS_DISABLE                       (1 << 5)
#define SENS_CMU_SEL_PLL_SYS_DISABLE                        (1 << 6)
#define SENS_CMU_BYPASS_DIV_SYS_DISABLE                     (1 << 7)
#define SENS_CMU_EN_OSCX2_DISABLE                           (1 << 8)
#define SENS_CMU_PU_OSCX2_DISABLE                           (1 << 9)
#define SENS_CMU_EN_OSCX4_DISABLE                           (1 << 10)
#define SENS_CMU_PU_OSCX4_DISABLE                           (1 << 11)
#define SENS_CMU_EN_PLL_DISABLE                             (1 << 12)
#define SENS_CMU_PU_PLL_DISABLE                             (1 << 13)

// reg_68
#define SENS_CMU_SDMA_CH15_REQ_IDX(n)                       (((n) & 0x3F) << 0)
#define SENS_CMU_SDMA_CH15_REQ_IDX_MASK                     (0x3F << 0)
#define SENS_CMU_SDMA_CH15_REQ_IDX_SHIFT                    (0)
#define SENS_CMU_CPU_INT_MASK_EN                            (1 << 6)
#define SENS_CMU_CEN_MASK_EN                                (1 << 7)

// reg_6c
#define SENS_CMU_RF_EMA(n)                                  (((n) & 0x7) << 0)
#define SENS_CMU_RF_EMA_MASK                                (0x7 << 0)
#define SENS_CMU_RF_EMA_SHIFT                               (0)
#define SENS_CMU_RF_EMAW(n)                                 (((n) & 0x3) << 3)
#define SENS_CMU_RF_EMAW_MASK                               (0x3 << 3)
#define SENS_CMU_RF_EMAW_SHIFT                              (3)
#define SENS_CMU_RF_WABL                                    (1 << 5)
#define SENS_CMU_RF_WABLM(n)                                (((n) & 0x3) << 6)
#define SENS_CMU_RF_WABLM_MASK                              (0x3 << 6)
#define SENS_CMU_RF_WABLM_SHIFT                             (6)
#define SENS_CMU_RF_EMAS                                    (1 << 8)
#define SENS_CMU_RF_RAWL                                    (1 << 9)
#define SENS_CMU_RF_RAWLM(n)                                (((n) & 0x3) << 10)
#define SENS_CMU_RF_RAWLM_MASK                              (0x3 << 10)
#define SENS_CMU_RF_RAWLM_SHIFT                             (10)

// reg_70
#define SENS_CMU_SEL_OSCX2_UART0                            (1 << 0)
#define SENS_CMU_SEL_OSCX2_UART1                            (1 << 1)
#define SENS_CMU_SEL_PLL_SRC                                (1 << 2)
#define SENS_CMU_SEL_PLL_AUD                                (1 << 3)
#define SENS_CMU_CFG_DIV_PCM(n)                             (((n) & 0x1FFF) << 4)
#define SENS_CMU_CFG_DIV_PCM_MASK                           (0x1FFF << 4)
#define SENS_CMU_CFG_DIV_PCM_SHIFT                          (4)
#define SENS_CMU_CFG_DIV_I2S0(n)                            (((n) & 0x1FFF) << 17)
#define SENS_CMU_CFG_DIV_I2S0_MASK                          (0x1FFF << 17)
#define SENS_CMU_CFG_DIV_I2S0_SHIFT                         (17)
#define SENS_CMU_EN_CLK_PLL_I2S0                            (1 << 30)
#define SENS_CMU_EN_CLK_PLL_PCM                             (1 << 31)

// reg_74
#define SENS_CMU_SEL_OSC_I2C                                (1 << 0)
#define SENS_CMU_SEL_OSCX2_I2C                              (1 << 1)
#define SENS_CMU_POL_CLK_PCM_IN                             (1 << 2)
#define SENS_CMU_SEL_PCM_CLKIN                              (1 << 3)
#define SENS_CMU_EN_CLK_PCM_OUT                             (1 << 4)
#define SENS_CMU_POL_CLK_PCM_OUT                            (1 << 5)
#define SENS_CMU_POL_CLK_I2S_IN                             (1 << 6)
#define SENS_CMU_SEL_I2S_CLKIN                              (1 << 7)
#define SENS_CMU_EN_CLK_I2S_OUT                             (1 << 8)
#define SENS_CMU_POL_CLK_I2S_OUT                            (1 << 9)
#define SENS_CMU_FORCE_PU_OFF                               (1 << 10)
#define SENS_CMU_LOCK_CPU_EN                                (1 << 11)
#define SENS_CMU_SEL_ROM_FAST                               (1 << 12)
#define SENS_CMU_POL_CLK_CAP                                (1 << 13)
#define SENS_CMU_EN_I2S_MCLK                                (1 << 14)
#define SENS_CMU_SEL_I2S_MCLK                               (1 << 15)
#define SENS_CMU_FORCE_PU_ON                                (1 << 16)
#define SENS_CMU_CFG_DIV_CAP(n)                             (((n) & 0xF) << 17)
#define SENS_CMU_CFG_DIV_CAP_MASK                           (0xF << 17)
#define SENS_CMU_CFG_DIV_CAP_SHIFT                          (17)
#define SENS_CMU_EN_CLK_CAP                                 (1 << 21)
#define SENS_CMU_SEL_CAP_CLKIN                              (1 << 22)
#define SENS_CMU_SEL_CODEC_CLK_OSC                          (1 << 23)
#define SENS_CMU_SEL_CODEC_HCLK_IN                          (1 << 24)
#define SENS_CMU_POL_ADC_ANA                                (1 << 25)
#define SENS_CMU_POL_SARADC_ANA                             (1 << 26)

// reg_78
#define SENS_CMU_SENS2MCU_INTISR_MASK0(n)                   (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_SENS2MCU_INTISR_MASK0_MASK                 (0xFFFFFFFF << 0)
#define SENS_CMU_SENS2MCU_INTISR_MASK0_SHIFT                (0)

// reg_7c
#define SENS_CMU_SENS2MCU_INTISR_MASK1(n)                   (((n) & 0xFFFFF) << 0)
#define SENS_CMU_SENS2MCU_INTISR_MASK1_MASK                 (0xFFFFF << 0)
#define SENS_CMU_SENS2MCU_INTISR_MASK1_SHIFT                (0)

// reg_80
#define SENS_CMU_WRITE_UNLOCK_H                             (1 << 0)
#define SENS_CMU_WRITE_UNLOCK_STATUS                        (1 << 1)

// reg_84
#define SENS_CMU_WAKEUP_IRQ_MASK0(n)                        (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_WAKEUP_IRQ_MASK0_MASK                      (0xFFFFFFFF << 0)
#define SENS_CMU_WAKEUP_IRQ_MASK0_SHIFT                     (0)

// reg_88
#define SENS_CMU_WAKEUP_IRQ_MASK1(n)                        (((n) & 0xFFFFF) << 0)
#define SENS_CMU_WAKEUP_IRQ_MASK1_MASK                      (0xFFFFF << 0)
#define SENS_CMU_WAKEUP_IRQ_MASK1_SHIFT                     (0)

// reg_8c
#define SENS_CMU_TIMER_WT26(n)                              (((n) & 0xFF) << 0)
#define SENS_CMU_TIMER_WT26_MASK                            (0xFF << 0)
#define SENS_CMU_TIMER_WT26_SHIFT                           (0)
#define SENS_CMU_TIMER_WTPLL(n)                             (((n) & 0xF) << 8)
#define SENS_CMU_TIMER_WTPLL_MASK                           (0xF << 8)
#define SENS_CMU_TIMER_WTPLL_SHIFT                          (8)
#define SENS_CMU_LPU_AUTO_SWITCH26                          (1 << 12)
#define SENS_CMU_LPU_AUTO_SWITCHPLL                         (1 << 13)
#define SENS_CMU_LPU_STATUS_26M                             (1 << 14)
#define SENS_CMU_LPU_STATUS_PLL                             (1 << 15)
#define SENS_CMU_LPU_AUTO_MID                               (1 << 16)

// reg_90
#define SENS_CMU_CFG_DIV_TIMER10(n)                         (((n) & 0xFFFF) << 0)
#define SENS_CMU_CFG_DIV_TIMER10_MASK                       (0xFFFF << 0)
#define SENS_CMU_CFG_DIV_TIMER10_SHIFT                      (0)
#define SENS_CMU_CFG_DIV_TIMER11(n)                         (((n) & 0xFFFF) << 16)
#define SENS_CMU_CFG_DIV_TIMER11_MASK                       (0xFFFF << 16)
#define SENS_CMU_CFG_DIV_TIMER11_SHIFT                      (16)

// reg_94
#define SENS_CMU_CFG_DIV_TIMER20(n)                         (((n) & 0xFFFF) << 0)
#define SENS_CMU_CFG_DIV_TIMER20_MASK                       (0xFFFF << 0)
#define SENS_CMU_CFG_DIV_TIMER20_SHIFT                      (0)
#define SENS_CMU_CFG_DIV_TIMER21(n)                         (((n) & 0xFFFF) << 16)
#define SENS_CMU_CFG_DIV_TIMER21_MASK                       (0xFFFF << 16)
#define SENS_CMU_CFG_DIV_TIMER21_SHIFT                      (16)

// reg_98
#define SENS_CMU_MCU2SENS_DATA_DONE_SET                     (1 << 0)
#define SENS_CMU_MCU2SENS_DATA1_DONE_SET                    (1 << 1)
#define SENS_CMU_MCU2SENS_DATA2_DONE_SET                    (1 << 2)
#define SENS_CMU_MCU2SENS_DATA3_DONE_SET                    (1 << 3)
#define SENS_CMU_SENS2MCU_DATA_IND_SET                      (1 << 4)
#define SENS_CMU_SENS2MCU_DATA1_IND_SET                     (1 << 5)
#define SENS_CMU_SENS2MCU_DATA2_IND_SET                     (1 << 6)
#define SENS_CMU_SENS2MCU_DATA3_IND_SET                     (1 << 7)
#define SENS_CMU_MCU2SENS_DATA_MSK_SET                      (1 << 8)
#define SENS_CMU_MCU2SENS_DATA1_MSK_SET                     (1 << 9)
#define SENS_CMU_MCU2SENS_DATA2_MSK_SET                     (1 << 10)
#define SENS_CMU_MCU2SENS_DATA3_MSK_SET                     (1 << 11)
#define SENS_CMU_SENS2MCU_DATA_MSK_SET                      (1 << 12)
#define SENS_CMU_SENS2MCU_DATA1_MSK_SET                     (1 << 13)
#define SENS_CMU_SENS2MCU_DATA2_MSK_SET                     (1 << 14)
#define SENS_CMU_SENS2MCU_DATA3_MSK_SET                     (1 << 15)
#define SENS_CMU_MCU2SENS_DATA_INTR                         (1 << 16)
#define SENS_CMU_MCU2SENS_DATA1_INTR                        (1 << 17)
#define SENS_CMU_MCU2SENS_DATA2_INTR                        (1 << 18)
#define SENS_CMU_MCU2SENS_DATA3_INTR                        (1 << 19)
#define SENS_CMU_SENS2MCU_DATA_INTR                         (1 << 20)
#define SENS_CMU_SENS2MCU_DATA1_INTR                        (1 << 21)
#define SENS_CMU_SENS2MCU_DATA2_INTR                        (1 << 22)
#define SENS_CMU_SENS2MCU_DATA3_INTR                        (1 << 23)
#define SENS_CMU_MCU2SENS_DATA_INTR_MSK                     (1 << 24)
#define SENS_CMU_MCU2SENS_DATA1_INTR_MSK                    (1 << 25)
#define SENS_CMU_MCU2SENS_DATA2_INTR_MSK                    (1 << 26)
#define SENS_CMU_MCU2SENS_DATA3_INTR_MSK                    (1 << 27)
#define SENS_CMU_SENS2MCU_DATA_INTR_MSK                     (1 << 28)
#define SENS_CMU_SENS2MCU_DATA1_INTR_MSK                    (1 << 29)
#define SENS_CMU_SENS2MCU_DATA2_INTR_MSK                    (1 << 30)
#define SENS_CMU_SENS2MCU_DATA3_INTR_MSK                    (1 << 31)

// reg_9c
#define SENS_CMU_MCU2SENS_DATA_DONE_CLR                     (1 << 0)
#define SENS_CMU_MCU2SENS_DATA1_DONE_CLR                    (1 << 1)
#define SENS_CMU_MCU2SENS_DATA2_DONE_CLR                    (1 << 2)
#define SENS_CMU_MCU2SENS_DATA3_DONE_CLR                    (1 << 3)
#define SENS_CMU_SENS2MCU_DATA_IND_CLR                      (1 << 4)
#define SENS_CMU_SENS2MCU_DATA1_IND_CLR                     (1 << 5)
#define SENS_CMU_SENS2MCU_DATA2_IND_CLR                     (1 << 6)
#define SENS_CMU_SENS2MCU_DATA3_IND_CLR                     (1 << 7)
#define SENS_CMU_MCU2SENS_DATA_MSK_CLR                      (1 << 8)
#define SENS_CMU_MCU2SENS_DATA1_MSK_CLR                     (1 << 9)
#define SENS_CMU_MCU2SENS_DATA2_MSK_CLR                     (1 << 10)
#define SENS_CMU_MCU2SENS_DATA3_MSK_CLR                     (1 << 11)
#define SENS_CMU_SENS2MCU_DATA_MSK_CLR                      (1 << 12)
#define SENS_CMU_SENS2MCU_DATA1_MSK_CLR                     (1 << 13)
#define SENS_CMU_SENS2MCU_DATA2_MSK_CLR                     (1 << 14)
#define SENS_CMU_SENS2MCU_DATA3_MSK_CLR                     (1 << 15)

// reg_a0
#define SENS_CMU_BT2SENS_DATA_DONE_SET                      (1 << 0)
#define SENS_CMU_BT2SENS_DATA1_DONE_SET                     (1 << 1)
#define SENS_CMU_SENS2BT_DATA_IND_SET                       (1 << 2)
#define SENS_CMU_SENS2BT_DATA1_IND_SET                      (1 << 3)
#define SENS_CMU_BT_ALLIRQ_MASK_SET                         (1 << 4)
#define SENS_CMU_BT_PLAYTIME_STAMP_INTR                     (1 << 5)
#define SENS_CMU_BT_PLAYTIME_STAMP1_INTR                    (1 << 6)
#define SENS_CMU_BT_PLAYTIME_STAMP2_INTR                    (1 << 7)
#define SENS_CMU_BT_PLAYTIME_STAMP3_INTR                    (1 << 8)
#define SENS_CMU_BT_PLAYTIME_STAMP_INTR_MSK                 (1 << 9)
#define SENS_CMU_BT_PLAYTIME_STAMP1_INTR_MSK                (1 << 10)
#define SENS_CMU_BT_PLAYTIME_STAMP2_INTR_MSK                (1 << 11)
#define SENS_CMU_BT_PLAYTIME_STAMP3_INTR_MSK                (1 << 12)
#define SENS_CMU_BT2SENS_DATA_MSK_SET                       (1 << 13)
#define SENS_CMU_BT2SENS_DATA1_MSK_SET                      (1 << 14)
#define SENS_CMU_SENS2BT_DATA_MSK_SET                       (1 << 15)
#define SENS_CMU_SENS2BT_DATA1_MSK_SET                      (1 << 16)
#define SENS_CMU_BT2SENS_DATA_INTR                          (1 << 17)
#define SENS_CMU_BT2SENS_DATA1_INTR                         (1 << 18)
#define SENS_CMU_SENS2BT_DATA_INTR                          (1 << 19)
#define SENS_CMU_SENS2BT_DATA1_INTR                         (1 << 20)
#define SENS_CMU_BT2SENS_DATA_INTR_MSK                      (1 << 21)
#define SENS_CMU_BT2SENS_DATA1_INTR_MSK                     (1 << 22)
#define SENS_CMU_SENS2BT_DATA_INTR_MSK                      (1 << 23)
#define SENS_CMU_SENS2BT_DATA1_INTR_MSK                     (1 << 24)
#define SENS_CMU_MCU_ALLIRQ_MASK_SET                        (1 << 25)

// reg_a4
#define SENS_CMU_BT2SENS_DATA_DONE_CLR                      (1 << 0)
#define SENS_CMU_BT2SENS_DATA1_DONE_CLR                     (1 << 1)
#define SENS_CMU_SENS2BT_DATA_IND_CLR                       (1 << 2)
#define SENS_CMU_SENS2BT_DATA1_IND_CLR                      (1 << 3)
#define SENS_CMU_BT_ALLIRQ_MASK_CLR                         (1 << 4)
#define SENS_CMU_BT_PLAYTIME_STAMP_INTR_CLR                 (1 << 5)
#define SENS_CMU_BT_PLAYTIME_STAMP1_INTR_CLR                (1 << 6)
#define SENS_CMU_BT_PLAYTIME_STAMP2_INTR_CLR                (1 << 7)
#define SENS_CMU_BT_PLAYTIME_STAMP3_INTR_CLR                (1 << 8)
#define SENS_CMU_BT2SENS_DATA_MSK_CLR                       (1 << 13)
#define SENS_CMU_BT2SENS_DATA1_MSK_CLR                      (1 << 14)
#define SENS_CMU_SENS2BT_DATA_MSK_CLR                       (1 << 15)
#define SENS_CMU_SENS2BT_DATA1_MSK_CLR                      (1 << 16)
#define SENS_CMU_MCU_ALLIRQ_MASK_CLR                        (1 << 25)

// reg_a8
#define SENS_CMU_CFG_DIV_SYS(n)                             (((n) & 0x3) << 0)
#define SENS_CMU_CFG_DIV_SYS_MASK                           (0x3 << 0)
#define SENS_CMU_CFG_DIV_SYS_SHIFT                          (0)
#define SENS_CMU_SEL_SMP_SENS(n)                            (((n) & 0x3) << 2)
#define SENS_CMU_SEL_SMP_SENS_MASK                          (0x3 << 2)
#define SENS_CMU_SEL_SMP_SENS_SHIFT                         (2)
#define SENS_CMU_CFG_DIV_PCLK(n)                            (((n) & 0x3) << 4)
#define SENS_CMU_CFG_DIV_PCLK_MASK                          (0x3 << 4)
#define SENS_CMU_CFG_DIV_PCLK_SHIFT                         (4)
#define SENS_CMU_SEL_OSCX2_SPI0                             (1 << 6)
#define SENS_CMU_SEL_OSCX2_SPI1                             (1 << 7)
#define SENS_CMU_SEL_OSCX2_SPI2                             (1 << 8)

// reg_b0
#define SENS_CMU_SENS2BT_INTISR_MASK0(n)                    (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_SENS2BT_INTISR_MASK0_MASK                  (0xFFFFFFFF << 0)
#define SENS_CMU_SENS2BT_INTISR_MASK0_SHIFT                 (0)

// reg_b4
#define SENS_CMU_SENS2BT_INTISR_MASK1(n)                    (((n) & 0xFFFFF) << 0)
#define SENS_CMU_SENS2BT_INTISR_MASK1_MASK                  (0xFFFFF << 0)
#define SENS_CMU_SENS2BT_INTISR_MASK1_SHIFT                 (0)

// reg_c0
#define SENS_CMU_MEMSC0                                     (1 << 0)

// reg_c4
#define SENS_CMU_MEMSC1                                     (1 << 0)

// reg_c8
#define SENS_CMU_MEMSC2                                     (1 << 0)

// reg_cc
#define SENS_CMU_MEMSC3                                     (1 << 0)

// reg_d0
#define SENS_CMU_MEMSC_STATUS0                              (1 << 0)
#define SENS_CMU_MEMSC_STATUS1                              (1 << 1)
#define SENS_CMU_MEMSC_STATUS2                              (1 << 2)
#define SENS_CMU_MEMSC_STATUS3                              (1 << 3)

// reg_d4
#define SENS_CMU_SDMA_CH0_REQ_IDX(n)                        (((n) & 0x3F) << 0)
#define SENS_CMU_SDMA_CH0_REQ_IDX_MASK                      (0x3F << 0)
#define SENS_CMU_SDMA_CH0_REQ_IDX_SHIFT                     (0)
#define SENS_CMU_SDMA_CH1_REQ_IDX(n)                        (((n) & 0x3F) << 6)
#define SENS_CMU_SDMA_CH1_REQ_IDX_MASK                      (0x3F << 6)
#define SENS_CMU_SDMA_CH1_REQ_IDX_SHIFT                     (6)
#define SENS_CMU_SDMA_CH2_REQ_IDX(n)                        (((n) & 0x3F) << 12)
#define SENS_CMU_SDMA_CH2_REQ_IDX_MASK                      (0x3F << 12)
#define SENS_CMU_SDMA_CH2_REQ_IDX_SHIFT                     (12)
#define SENS_CMU_SDMA_CH3_REQ_IDX(n)                        (((n) & 0x3F) << 18)
#define SENS_CMU_SDMA_CH3_REQ_IDX_MASK                      (0x3F << 18)
#define SENS_CMU_SDMA_CH3_REQ_IDX_SHIFT                     (18)
#define SENS_CMU_SDMA_CH4_REQ_IDX(n)                        (((n) & 0x3F) << 24)
#define SENS_CMU_SDMA_CH4_REQ_IDX_MASK                      (0x3F << 24)
#define SENS_CMU_SDMA_CH4_REQ_IDX_SHIFT                     (24)

// reg_d8
#define SENS_CMU_SDMA_CH5_REQ_IDX(n)                        (((n) & 0x3F) << 0)
#define SENS_CMU_SDMA_CH5_REQ_IDX_MASK                      (0x3F << 0)
#define SENS_CMU_SDMA_CH5_REQ_IDX_SHIFT                     (0)
#define SENS_CMU_SDMA_CH6_REQ_IDX(n)                        (((n) & 0x3F) << 6)
#define SENS_CMU_SDMA_CH6_REQ_IDX_MASK                      (0x3F << 6)
#define SENS_CMU_SDMA_CH6_REQ_IDX_SHIFT                     (6)
#define SENS_CMU_SDMA_CH7_REQ_IDX(n)                        (((n) & 0x3F) << 12)
#define SENS_CMU_SDMA_CH7_REQ_IDX_MASK                      (0x3F << 12)
#define SENS_CMU_SDMA_CH7_REQ_IDX_SHIFT                     (12)
#define SENS_CMU_SDMA_CH8_REQ_IDX(n)                        (((n) & 0x3F) << 18)
#define SENS_CMU_SDMA_CH8_REQ_IDX_MASK                      (0x3F << 18)
#define SENS_CMU_SDMA_CH8_REQ_IDX_SHIFT                     (18)
#define SENS_CMU_SDMA_CH9_REQ_IDX(n)                        (((n) & 0x3F) << 24)
#define SENS_CMU_SDMA_CH9_REQ_IDX_MASK                      (0x3F << 24)
#define SENS_CMU_SDMA_CH9_REQ_IDX_SHIFT                     (24)

// reg_dc
#define SENS_CMU_SDMA_CH10_REQ_IDX(n)                       (((n) & 0x3F) << 0)
#define SENS_CMU_SDMA_CH10_REQ_IDX_MASK                     (0x3F << 0)
#define SENS_CMU_SDMA_CH10_REQ_IDX_SHIFT                    (0)
#define SENS_CMU_SDMA_CH11_REQ_IDX(n)                       (((n) & 0x3F) << 6)
#define SENS_CMU_SDMA_CH11_REQ_IDX_MASK                     (0x3F << 6)
#define SENS_CMU_SDMA_CH11_REQ_IDX_SHIFT                    (6)
#define SENS_CMU_SDMA_CH12_REQ_IDX(n)                       (((n) & 0x3F) << 12)
#define SENS_CMU_SDMA_CH12_REQ_IDX_MASK                     (0x3F << 12)
#define SENS_CMU_SDMA_CH12_REQ_IDX_SHIFT                    (12)
#define SENS_CMU_SDMA_CH13_REQ_IDX(n)                       (((n) & 0x3F) << 18)
#define SENS_CMU_SDMA_CH13_REQ_IDX_MASK                     (0x3F << 18)
#define SENS_CMU_SDMA_CH13_REQ_IDX_SHIFT                    (18)
#define SENS_CMU_SDMA_CH14_REQ_IDX(n)                       (((n) & 0x3F) << 24)
#define SENS_CMU_SDMA_CH14_REQ_IDX_MASK                     (0x3F << 24)
#define SENS_CMU_SDMA_CH14_REQ_IDX_SHIFT                    (24)

// reg_e0
#define SENS_CMU_MINIMA_BYPASS                              (1 << 0)
#define SENS_CMU_MINIMA_EXTEND_180DEG                       (1 << 1)
#define SENS_CMU_MINIMA_ENABLE_FCLK                         (1 << 2)

// reg_f0
#define SENS_CMU_RESERVED(n)                                (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_RESERVED_MASK                              (0xFFFFFFFF << 0)
#define SENS_CMU_RESERVED_SHIFT                             (0)

// reg_f4
#define SENS_CMU_DEBUG(n)                                   (((n) & 0xFFFFFFFF) << 0)
#define SENS_CMU_DEBUG_MASK                                 (0xFFFFFFFF << 0)
#define SENS_CMU_DEBUG_SHIFT                                (0)

// Sensor System AHB Clocks:
#define SENS_HCLK_MCU                                       (1 << 0)
#define SENS_HRST_MCU                                       (1 << 0)
#define SENS_HCLK_RAM0                                      (1 << 1)
#define SENS_HRST_RAM0                                      (1 << 1)
#define SENS_HCLK_RAM1                                      (1 << 2)
#define SENS_HRST_RAM1                                      (1 << 2)
#define SENS_HCLK_RAM2                                      (1 << 3)
#define SENS_HRST_RAM2                                      (1 << 3)
#define SENS_HCLK_RAM3                                      (1 << 4)
#define SENS_HRST_RAM3                                      (1 << 4)
#define SENS_HCLK_RAM4                                      (1 << 5)
#define SENS_HRST_RAM4                                      (1 << 5)
#define SENS_HCLK_AHB0                                      (1 << 6)
#define SENS_HRST_AHB0                                      (1 << 6)
#define SENS_HCLK_AHB1                                      (1 << 7)
#define SENS_HRST_AHB1                                      (1 << 7)
#define SENS_HCLK_AH2H_BT                                   (1 << 8)
#define SENS_HRST_AH2H_BT                                   (1 << 8)
#define SENS_HCLK_SDMA                                      (1 << 9)
#define SENS_HRST_SDMA                                      (1 << 9)
#define SENS_HCLK_CODEC                                     (1 << 10)
#define SENS_HRST_CODEC                                     (1 << 10)
#define SENS_HCLK_SENSOR_HUB                                (1 << 11)
#define SENS_HRST_SENSOR_HUB                                (1 << 11)
#define SENS_HCLK_TEP                                       (1 << 12)
#define SENS_HRST_TEP                                       (1 << 12)
#define SENS_HCLK_AVS                                       (1 << 13)
#define SENS_HRST_AVS                                       (1 << 13)
#define SENS_HCLK_A2A                                       (1 << 14)
#define SENS_HRST_A2A                                       (1 << 14)
#define SENS_HCLK_PSPY                                      (1 << 15)
#define SENS_HRST_PSPY                                      (1 << 15)
#define SENS_HCLK_VAD                                       (1 << 16)
#define SENS_HRST_VAD                                       (1 << 16)
#define SENS_HCLK_RAM5                                      (1 << 17)
#define SENS_HRST_RAM5                                      (1 << 17)

// Sensor System APB Clocks:
#define SENS_PCLK_CMU                                       (1 << 0)
#define SENS_PRST_CMU                                       (1 << 0)
#define SENS_PCLK_WDT                                       (1 << 1)
#define SENS_PRST_WDT                                       (1 << 1)
#define SENS_PCLK_TIMER0                                    (1 << 2)
#define SENS_PRST_TIMER0                                    (1 << 2)
#define SENS_PCLK_TIMER1                                    (1 << 3)
#define SENS_PRST_TIMER1                                    (1 << 3)
#define SENS_PCLK_TIMER2                                    (1 << 4)
#define SENS_PRST_TIMER2                                    (1 << 4)
#define SENS_PCLK_I2C0                                      (1 << 5)
#define SENS_PRST_I2C0                                      (1 << 5)
#define SENS_PCLK_I2C1                                      (1 << 6)
#define SENS_PRST_I2C1                                      (1 << 6)
#define SENS_PCLK_SPI                                       (1 << 7)
#define SENS_PRST_SPI                                       (1 << 7)
#define SENS_PCLK_SLCD                                      (1 << 8)
#define SENS_PRST_SLCD                                      (1 << 8)
#define SENS_PCLK_SPI_ITN                                   (1 << 9)
#define SENS_PRST_SPI_ITN                                   (1 << 9)
#define SENS_PCLK_UART0                                     (1 << 10)
#define SENS_PRST_UART0                                     (1 << 10)
#define SENS_PCLK_UART1                                     (1 << 11)
#define SENS_PRST_UART1                                     (1 << 11)
#define SENS_PCLK_PCM                                       (1 << 12)
#define SENS_PRST_PCM                                       (1 << 12)
#define SENS_PCLK_I2S                                       (1 << 13)
#define SENS_PRST_I2S                                       (1 << 13)
#define SENS_PCLK_I2C2                                      (1 << 14)
#define SENS_PRST_I2C2                                      (1 << 14)
#define SENS_PCLK_I2C3                                      (1 << 15)
#define SENS_PRST_I2C3                                      (1 << 15)
#define SENS_PCLK_CAP                                       (1 << 16)
#define SENS_PRST_CAP                                       (1 << 16)
#define SENS_PCLK_PSPY                                      (1 << 17)
#define SENS_PRST_PSPY                                      (1 << 17)

// Sensor System Other Clocks:
#define SENS_OCLK_SLEEP                                     (1 << 0)
#define SENS_ORST_SLEEP                                     (1 << 0)
#define SENS_OCLK_WDT                                       (1 << 1)
#define SENS_ORST_WDT                                       (1 << 1)
#define SENS_OCLK_TIMER0                                    (1 << 2)
#define SENS_ORST_TIMER0                                    (1 << 2)
#define SENS_OCLK_TIMER1                                    (1 << 3)
#define SENS_ORST_TIMER1                                    (1 << 3)
#define SENS_OCLK_TIMER2                                    (1 << 4)
#define SENS_ORST_TIMER2                                    (1 << 4)
#define SENS_OCLK_I2C0                                      (1 << 5)
#define SENS_ORST_I2C0                                      (1 << 5)
#define SENS_OCLK_I2C1                                      (1 << 6)
#define SENS_ORST_I2C1                                      (1 << 6)
#define SENS_OCLK_SPI                                       (1 << 7)
#define SENS_ORST_SPI                                       (1 << 7)
#define SENS_OCLK_SLCD                                      (1 << 8)
#define SENS_ORST_SLCD                                      (1 << 8)
#define SENS_OCLK_SPI_ITN                                   (1 << 9)
#define SENS_ORST_SPI_ITN                                   (1 << 9)
#define SENS_OCLK_UART0                                     (1 << 10)
#define SENS_ORST_UART0                                     (1 << 10)
#define SENS_OCLK_UART1                                     (1 << 11)
#define SENS_ORST_UART1                                     (1 << 11)
#define SENS_OCLK_PCM                                       (1 << 12)
#define SENS_ORST_PCM                                       (1 << 12)
#define SENS_OCLK_I2S                                       (1 << 13)
#define SENS_ORST_I2S                                       (1 << 13)
#define SENS_OCLK_I2C2                                      (1 << 14)
#define SENS_ORST_I2C2                                      (1 << 14)
#define SENS_OCLK_I2C3                                      (1 << 15)
#define SENS_ORST_I2C3                                      (1 << 15)
#define SENS_OCLK_CAP0                                      (1 << 16)
#define SENS_ORST_CAP0                                      (1 << 16)
#define SENS_OCLK_CAP1                                      (1 << 17)
#define SENS_ORST_CAP1                                      (1 << 17)
#define SENS_OCLK_CAP2                                      (1 << 18)
#define SENS_ORST_CAP2                                      (1 << 18)
#define SENS_OCLK_ADC_FREE                                  (1 << 19)
#define SENS_ORST_ADC_FREE                                  (1 << 19)
#define SENS_OCLK_ADC_CH0                                   (1 << 20)
#define SENS_ORST_ADC_CH0                                   (1 << 20)
#define SENS_OCLK_ADC_CH1                                   (1 << 21)
#define SENS_ORST_ADC_CH1                                   (1 << 21)
#define SENS_OCLK_ADC_ANA                                   (1 << 22)
#define SENS_ORST_ADC_ANA                                   (1 << 22)
#define SENS_OCLK_SADC_ANA                                  (1 << 23)
#define SENS_ORST_SADC_ANA                                  (1 << 23)

#endif

