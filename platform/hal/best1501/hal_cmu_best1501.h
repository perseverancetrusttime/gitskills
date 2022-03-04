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
#ifndef __HAL_CMU_BEST1501_H__
#define __HAL_CMU_BEST1501_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_CMU_DEFAULT_CRYSTAL_FREQ        24000000
#define HAL_CMU_VALID_CRYSTAL_FREQ          { 24000000, 26000000, }

#define HAL_CMU_USB_ROM_SELECT_CLOCK_SOURCE

enum HAL_CMU_MOD_ID_T {
    // HCLK/HRST
    HAL_CMU_MOD_H_MCU,          // 0
    HAL_CMU_MOD_H_ROM0,         // 1
    HAL_CMU_MOD_H_RAM8,         // 2
    HAL_CMU_MOD_H_REAL_FLASH1,  // 3
    HAL_CMU_MOD_H_RAM0,         // 4
    HAL_CMU_MOD_H_RAM1,         // 5
    HAL_CMU_MOD_H_RAM2,         // 6
    HAL_CMU_MOD_H_RAM7,         // 7
    HAL_CMU_MOD_H_AHB0,         // 8
    HAL_CMU_MOD_H_AHB1,         // 9
    HAL_CMU_MOD_H_AH2H_BT,      // 10
    HAL_CMU_MOD_H_ADMA,         // 11
    HAL_CMU_MOD_H_GDMA,         // 12
    HAL_CMU_MOD_H_PSPY,         // 13
    HAL_CMU_MOD_H_REAL_FLASH,   // 14
    HAL_CMU_MOD_H_SDMMC,        // 15
    HAL_CMU_MOD_H_USBC,         // 16
    HAL_CMU_MOD_H_CODEC,        // 17
    HAL_CMU_MOD_H_MCU_CP0,      // 18
    HAL_CMU_MOD_H_BT_TPORT,     // 19
    HAL_CMU_MOD_H_USBH,         // 20
    HAL_CMU_MOD_H_LCDC,         // 21
    HAL_CMU_MOD_H_BT_DUMP,      // 22
    HAL_CMU_MOD_H_CP,           // 23
    HAL_CMU_MOD_H_RAM3,         // 24
    HAL_CMU_MOD_H_RAM4,         // 25
    HAL_CMU_MOD_H_RAM5,         // 26
    HAL_CMU_MOD_H_RAM6,         // 27
    HAL_CMU_MOD_H_SEC_ENG,      // 28
    HAL_CMU_MOD_H_CACHE0,       // 29
    HAL_CMU_MOD_H_AH2H_SENS,    // 30
    HAL_CMU_MOD_H_SPI_AHB,      // 31
    // PCLK/PRST
    HAL_CMU_MOD_P_CMU,          // 32
    HAL_CMU_MOD_P_WDT,          // 33
    HAL_CMU_MOD_P_TIMER0,       // 34
    HAL_CMU_MOD_P_TIMER1,       // 35
    HAL_CMU_MOD_P_TIMER2,       // 36
    HAL_CMU_MOD_P_I2C0,         // 37
    HAL_CMU_MOD_P_I2C1,         // 38
    HAL_CMU_MOD_P_SPI,          // 39
    HAL_CMU_MOD_P_TRNG,         // 40
    HAL_CMU_MOD_P_SPI_ITN,      // 41
    HAL_CMU_MOD_P_UART0,        // 42
    HAL_CMU_MOD_P_UART1,        // 43
    HAL_CMU_MOD_P_UART2,        // 44
    HAL_CMU_MOD_P_PCM,          // 45
    HAL_CMU_MOD_P_I2S0,         // 46
    HAL_CMU_MOD_P_SPDIF0,       // 47
    HAL_CMU_MOD_P_I2S1,         // 48
    HAL_CMU_MOD_P_SEC_ENG,      // 49
    HAL_CMU_MOD_P_TZC,          // 50
    HAL_CMU_MOD_P_DIS,          // 51
    HAL_CMU_MOD_P_PSPY,         // 52
    // OCLK/ORST
    HAL_CMU_MOD_O_SLEEP,        // 53
    HAL_CMU_MOD_O_REAL_FLASH,   // 54
    HAL_CMU_MOD_O_USB,          // 55
    HAL_CMU_MOD_O_SDMMC,        // 56
    HAL_CMU_MOD_O_WDT,          // 57
    HAL_CMU_MOD_O_TIMER0,       // 58
    HAL_CMU_MOD_O_TIMER1,       // 59
    HAL_CMU_MOD_O_TIMER2,       // 60
    HAL_CMU_MOD_O_I2C0,         // 61
    HAL_CMU_MOD_O_I2C1,         // 62
    HAL_CMU_MOD_O_SPI,          // 63
    HAL_CMU_MOD_O_SPI_ITN,      // 64
    HAL_CMU_MOD_O_UART0,        // 65
    HAL_CMU_MOD_O_UART1,        // 66
    HAL_CMU_MOD_O_UART2,        // 67
    HAL_CMU_MOD_O_I2S0,         // 68
    HAL_CMU_MOD_O_SPDIF0,       // 69
    HAL_CMU_MOD_O_PCM,          // 70
    HAL_CMU_MOD_O_USB32K,       // 71
    HAL_CMU_MOD_O_I2S1,         // 72
    HAL_CMU_MOD_O_REAL_FLASH1,  // 73
    HAL_CMU_MOD_O_DISPIX,       // 74
    HAL_CMU_MOD_O_DISDSI,       // 75

    // AON ACLK/ARST
    HAL_CMU_AON_A_CMU,          // 76
    HAL_CMU_AON_A_GPIO0,        // 77
    HAL_CMU_AON_A_GPIO0_INT,    // 78
    HAL_CMU_AON_A_WDT,          // 79
    HAL_CMU_AON_A_PWM,          // 80
    HAL_CMU_AON_A_TIMER0,       // 81
    HAL_CMU_AON_A_PSC,          // 82
    HAL_CMU_AON_A_IOMUX,        // 83
    HAL_CMU_AON_A_APBC,         // 84
    HAL_CMU_AON_A_H2H_MCU,      // 85
    HAL_CMU_AON_A_I2C_SLV,      // 86
    HAL_CMU_AON_A_TZC,          // 87
    HAL_CMU_AON_A_GPIO1,        // 88
    HAL_CMU_AON_A_GPIO1_INT,    // 89
    HAL_CMU_AON_A_GPIO2,        // 90
    HAL_CMU_AON_A_GPIO2_INT,    // 91
    HAL_CMU_AON_A_TIMER1,       // 92
    // AON OCLK/ORST
    HAL_CMU_AON_O_WDT,          // 93
    HAL_CMU_AON_O_TIMER0,       // 94
    HAL_CMU_AON_O_GPIO0,        // 95
    HAL_CMU_AON_O_PWM0,         // 96
    HAL_CMU_AON_O_PWM1,         // 97
    HAL_CMU_AON_O_PWM2,         // 98
    HAL_CMU_AON_O_PWM3,         // 99
    HAL_CMU_AON_O_IOMUX,        // 100
    HAL_CMU_AON_O_GPIO1,        // 101
    HAL_CMU_AON_O_BTAON,        // 102
    HAL_CMU_AON_O_PSC,          // 103
    HAL_CMU_AON_O_GPIO2,        // 104
    HAL_CMU_AON_O_TIMER1,       // 105
    // AON SUBSYS
    HAL_CMU_AON_MCU,            // 106
    HAL_CMU_AON_CODEC,          // 107
    HAL_CMU_AON_SENS,           // 108
    HAL_CMU_AON_BT,             // 109
    HAL_CMU_AON_MCUCPU,         // 110
    HAL_CMU_AON_RESERVED5,      // 111
    HAL_CMU_AON_BTCPU,          // 112
    HAL_CMU_AON_GLOBAL,         // 113

    HAL_CMU_MOD_QTY,

    HAL_CMU_MOD_GLOBAL = HAL_CMU_AON_GLOBAL,
    HAL_CMU_MOD_BT = HAL_CMU_AON_BT,
    HAL_CMU_MOD_BTCPU = HAL_CMU_AON_BTCPU,

    HAL_CMU_MOD_P_PWM = HAL_CMU_AON_A_PWM,
    HAL_CMU_MOD_O_PWM0 = HAL_CMU_AON_O_PWM0,
    HAL_CMU_MOD_O_PWM1 = HAL_CMU_AON_O_PWM1,
    HAL_CMU_MOD_O_PWM2 = HAL_CMU_AON_O_PWM2,
    HAL_CMU_MOD_O_PWM3 = HAL_CMU_AON_O_PWM3,

    HAL_CMU_H_ICACHECP = HAL_CMU_MOD_QTY,
    HAL_CMU_H_DCACHECP = HAL_CMU_MOD_QTY,

#ifdef ALT_BOOT_FLASH
    HAL_CMU_MOD_H_FLASH  = HAL_CMU_MOD_H_REAL_FLASH1,
    HAL_CMU_MOD_H_FLASH1 = HAL_CMU_MOD_H_REAL_FLASH,
    HAL_CMU_MOD_O_FLASH  = HAL_CMU_MOD_O_REAL_FLASH1,
    HAL_CMU_MOD_O_FLASH1 = HAL_CMU_MOD_O_REAL_FLASH,
#else
    HAL_CMU_MOD_H_FLASH  = HAL_CMU_MOD_H_REAL_FLASH,
    HAL_CMU_MOD_H_FLASH1 = HAL_CMU_MOD_H_REAL_FLASH1,
    HAL_CMU_MOD_O_FLASH  = HAL_CMU_MOD_O_REAL_FLASH,
    HAL_CMU_MOD_O_FLASH1 = HAL_CMU_MOD_O_REAL_FLASH1,
#endif
};

enum HAL_CMU_CLOCK_OUT_ID_T {
    HAL_CMU_CLOCK_OUT_AON_32K           = 0x00,
    HAL_CMU_CLOCK_OUT_AON_OSC           = 0x01,
    HAL_CMU_CLOCK_OUT_AON_OSCX2         = 0x02,
    HAL_CMU_CLOCK_OUT_AON_DIG_OSCX2     = 0x03,
    HAL_CMU_CLOCK_OUT_AON_DIG_OSCX4     = 0x04,
    HAL_CMU_CLOCK_OUT_AON_SYS           = 0x05,
    HAL_CMU_CLOCK_OUT_AON_SYS_2         = 0x06,
    HAL_CMU_CLOCK_OUT_AON_DCDC          = 0x07,

    HAL_CMU_CLOCK_OUT_BT_NONE           = 0x40,
    HAL_CMU_CLOCK_OUT_BT_32K            = 0x41,
    HAL_CMU_CLOCK_OUT_BT_SYS            = 0x42,
    HAL_CMU_CLOCK_OUT_BT_48M            = 0x43,
    HAL_CMU_CLOCK_OUT_BT_12M            = 0x44,
    HAL_CMU_CLOCK_OUT_BT_ADC            = 0x45,
    HAL_CMU_CLOCK_OUT_BT_ADCD3          = 0x46,
    HAL_CMU_CLOCK_OUT_BT_DAC            = 0x47,
    HAL_CMU_CLOCK_OUT_BT_DACD2          = 0x48,
    HAL_CMU_CLOCK_OUT_BT_DACD4          = 0x49,
    HAL_CMU_CLOCK_OUT_BT_DACD8          = 0x50,

    HAL_CMU_CLOCK_OUT_MCU_32K           = 0x60,
    HAL_CMU_CLOCK_OUT_MCU_SYS           = 0x61,
    HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH0   = 0x62,
    HAL_CMU_CLOCK_OUT_MCU_USB           = 0x63,
    HAL_CMU_CLOCK_OUT_MCU_PCLK          = 0x64,
    HAL_CMU_CLOCK_OUT_MCU_I2S0          = 0x65,
    HAL_CMU_CLOCK_OUT_MCU_PCM           = 0x66,
    HAL_CMU_CLOCK_OUT_MCU_SPDIF0        = 0x67,
    HAL_CMU_CLOCK_OUT_MCU_SDMMC         = 0x68,
    HAL_CMU_CLOCK_OUT_MCU_SPI2          = 0x69,
    HAL_CMU_CLOCK_OUT_MCU_SPI1          = 0x6A,
    HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH1   = 0x6B,
    HAL_CMU_CLOCK_OUT_MCU_I2S1          = 0x6C,
    HAL_CMU_CLOCK_OUT_MCU_PIX           = 0x6D,
    HAL_CMU_CLOCK_OUT_MCU_DSI           = 0x6E,

#ifdef ALT_BOOT_FLASH
    HAL_CMU_CLOCK_OUT_MCU_FLASH0        = HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH1,
    HAL_CMU_CLOCK_OUT_MCU_FLASH1        = HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH0,
#else
    HAL_CMU_CLOCK_OUT_MCU_FLASH0        = HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH0,
    HAL_CMU_CLOCK_OUT_MCU_FLASH1        = HAL_CMU_CLOCK_OUT_MCU_REAL_FLASH1,
#endif

    HAL_CMU_CLOCK_OUT_CODEC_ADC_ANA     = 0x80,
    HAL_CMU_CLOCK_OUT_CODEC_CODEC       = 0x81,
    HAL_CMU_CLOCK_OUT_CODEC_ANC_IIR     = 0x82,
    HAL_CMU_CLOCK_OUT_CODEC_RS_DAC      = 0x83,
    HAL_CMU_CLOCK_OUT_CODEC_RS_ADC      = 0x84,
    HAL_CMU_CLOCK_OUT_CODEC_FIR         = 0x85,
    HAL_CMU_CLOCK_OUT_CODEC_PSAP        = 0x86,
    HAL_CMU_CLOCK_OUT_CODEC_EQ_IIR      = 0x87,
    HAL_CMU_CLOCK_OUT_CODEC_HCLK        = 0x88,

    HAL_CMU_CLOCK_OUT_SENS_32K          = 0xA0,
    HAL_CMU_CLOCK_OUT_SENS_SYS          = 0xA1,
    HAL_CMU_CLOCK_OUT_SENS_PCLK         = 0xA2,
    HAL_CMU_CLOCK_OUT_SENS_PCM          = 0xA3,
    HAL_CMU_CLOCK_OUT_SENS_SPI2         = 0xA4,
    HAL_CMU_CLOCK_OUT_SENS_SPI0         = 0xA5,
    HAL_CMU_CLOCK_OUT_SENS_SPI1         = 0xA6,
    HAL_CMU_CLOCK_OUT_SENS_I2C          = 0xA7,
    HAL_CMU_CLOCK_OUT_SENS_UART0        = 0xA8,
    HAL_CMU_CLOCK_OUT_SENS_JTAG         = 0xA9,
    HAL_CMU_CLOCK_OUT_SENS_CAP          = 0xAA,
    HAL_CMU_CLOCK_OUT_SENS_ADC_ANA      = 0xAB,
    HAL_CMU_CLOCK_OUT_SENS_SAR_ADC_ANA  = 0xAC,
    HAL_CMU_CLOCK_OUT_SENS_CODEC        = 0xAD,
    HAL_CMU_CLOCK_OUT_SENS_CODEC_HCLK   = 0xAE,
    HAL_CMU_CLOCK_OUT_SENS_I2S          = 0xAF,
};

enum HAL_CMU_I2S_MCLK_ID_T {
    HAL_CMU_I2S_MCLK_PER                = 0x00,
    HAL_CMU_I2S_MCLK_PLLPCM             = 0x01,
    HAL_CMU_I2S_MCLK_PLLI2S0            = 0x02,
    HAL_CMU_I2S_MCLK_PLLI2S1            = 0x03,
    HAL_CMU_I2S_MCLK_PLLSPDIF0          = 0x04,
};

enum HAL_I2S_ID_T {
    HAL_I2S_ID_0 = 0,
    HAL_I2S_ID_1,

    HAL_I2S_ID_QTY,
};
#define HAL_I2S_ID_T                        HAL_I2S_ID_T

enum HAL_CMU_ANC_CLK_USER_T {
    HAL_CMU_ANC_CLK_USER_ANC,

    HAL_CMU_ANC_CLK_USER_QTY
};

enum HAL_CMU_FREQ_T {
    HAL_CMU_FREQ_32K,
    HAL_CMU_FREQ_6P5M,
    HAL_CMU_FREQ_13M,
    HAL_CMU_FREQ_26M,
    HAL_CMU_FREQ_52M,
    HAL_CMU_FREQ_78M,
    HAL_CMU_FREQ_104M,
    HAL_CMU_FREQ_208M,

    HAL_CMU_FREQ_QTY
};
#define HAL_CMU_FREQ_T                      HAL_CMU_FREQ_T

enum HAL_CMU_RAM_CFG_SEL_T {
    HAL_CMU_RAM_CFG_SEL_MCU = 0,
    HAL_CMU_RAM_CFG_SEL_SENS,
    HAL_CMU_RAM_CFG_SEL_BT,
};

enum HAL_CMU_PLL_T {
    HAL_CMU_PLL_BB = 0,
    HAL_CMU_PLL_DSI,
    HAL_CMU_PLL_AUD,
    HAL_CMU_PLL_USB,

    HAL_CMU_PLL_QTY
};
#define HAL_CMU_PLL_T                       HAL_CMU_PLL_T

enum HAL_CMU_PLL_USER_T {
    HAL_CMU_PLL_USER_SYS,
    HAL_CMU_PLL_USER_AUD,
    HAL_CMU_PLL_USER_USB,
    HAL_CMU_PLL_USER_FLASH,
    HAL_CMU_PLL_USER_FLASH1,

    HAL_CMU_PLL_USER_QTY,
    HAL_CMU_PLL_USER_ALL = HAL_CMU_PLL_USER_QTY,
};
#define HAL_CMU_PLL_USER_T                  HAL_CMU_PLL_USER_T

enum HAL_CMU_DCDC_CLOCK_USER_T {
    HAL_CMU_DCDC_CLOCK_USER_GPADC,
    HAL_CMU_DCDC_CLOCK_USER_ADCKEY,
    HAL_CMU_DCDC_CLOCK_USER_EFUSE,

    HAL_CMU_DCDC_CLOCK_USER_QTY,
};

enum HAL_CMU_BT_TRIGGER_SRC_T {
    HAL_CMU_BT_TRIGGER_SRC_0,
    HAL_CMU_BT_TRIGGER_SRC_1,
    HAL_CMU_BT_TRIGGER_SRC_2,
    HAL_CMU_BT_TRIGGER_SRC_3,

    HAL_CMU_BT_TRIGGER_SRC_QTY,
};

typedef void (*HAL_CMU_BT_TRIGGER_HANDLER_T)(enum HAL_CMU_BT_TRIGGER_SRC_T src);

int hal_cmu_fast_timer_offline(void);

enum HAL_CMU_PLL_T hal_cmu_get_audio_pll(void);

void hal_cmu_anc_enable(enum HAL_CMU_ANC_CLK_USER_T user);

void hal_cmu_anc_disable(enum HAL_CMU_ANC_CLK_USER_T user);

int hal_cmu_anc_get_status(enum HAL_CMU_ANC_CLK_USER_T user);

void hal_cmu_mcu_pdm_clock_out(uint32_t clk_map);

void hal_cmu_sens_pdm_clock_out(uint32_t clk_map);

uint32_t hal_cmu_get_aon_chip_id(void);

uint32_t hal_cmu_get_aon_revision_id(void);

void hal_cmu_cp_enable(uint32_t sp, uint32_t entry);

void hal_cmu_cp_disable(void);

uint32_t hal_cmu_cp_get_entry_addr(void);

uint32_t hal_cmu_get_ram_boot_addr(void);

void hal_cmu_sens_clock_enable(void);

void hal_cmu_sens_clock_disable(void);

void hal_cmu_sens_reset_set(void);

void hal_cmu_sens_reset_clear(void);

void hal_cmu_sens_start_cpu(uint32_t sp, uint32_t entry);

void hal_cmu_ram_cfg_sel_update(uint32_t map, enum HAL_CMU_RAM_CFG_SEL_T sel);

int hal_cmu_sys_is_using_audpll(void);

int hal_cmu_flash_is_using_audpll(void);

void hal_cmu_beco_enable(void);

void hal_cmu_beco_disable(void);

void hal_cmu_dcdc_clock_enable(enum HAL_CMU_DCDC_CLOCK_USER_T user);

void hal_cmu_dcdc_clock_disable(enum HAL_CMU_DCDC_CLOCK_USER_T user);

void hal_cmu_auto_switch_rc_enable(void);

void hal_cmu_auto_switch_rc_disable(void);

void hal_cmu_select_rc_clock(void);

void hal_cmu_select_osc_clock(void);

uint32_t hal_cmu_get_osc_ready_cycle_cnt(void);

uint32_t hal_cmu_get_osc_switch_overhead(void);

void hal_cmu_boot_dcdc_clock_enable(void);

void hal_cmu_boot_dcdc_clock_disable(void);

int hal_cmu_bt_trigger_set_handler(enum HAL_CMU_BT_TRIGGER_SRC_T src, HAL_CMU_BT_TRIGGER_HANDLER_T hdlr);

int hal_cmu_bt_trigger_enable(enum HAL_CMU_BT_TRIGGER_SRC_T src);

int hal_cmu_bt_trigger_disable(enum HAL_CMU_BT_TRIGGER_SRC_T src);

#ifdef __cplusplus
}
#endif

#endif

