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
#ifdef CHIP_SUBSYS_SENS

#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_senscmu)
#include CHIP_SPECIFIC_HDR(reg_aoncmu)
#include CHIP_SPECIFIC_HDR(reg_btcmu)
#include "hal_cmu.h"
#include "hal_aud.h"
#include "hal_bootmode.h"
#include "hal_chipid.h"
#include "hal_codec.h"
#include "hal_location.h"
#include "hal_psc.h"
#include "hal_sleep_core_pd.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "cmsis_nvic.h"
#include "pmu.h"

#ifdef VOICE_DETECTOR_EN
#define CODEC_CLK_FROM_ANA

#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE != CODEC_VAD_MAX_BUF_SIZE)
#error "VAD buffer conflict. At present, this buffer can only be used for VAD!"
#endif

#endif

#ifdef AUDIO_USE_BBPLL
#define AUDIO_PLL_SEL                   HAL_CMU_PLL_BB
#else
#define AUDIO_PLL_SEL                   HAL_CMU_PLL_AUD
#endif

#define HAL_CMU_PLL_LOCKED_TIMEOUT      US_TO_TICKS(200)
#define HAL_CMU_26M_READY_TIMEOUT       MS_TO_TICKS(4)
#define HAL_CMU_LPU_EXTRA_TIMEOUT       MS_TO_TICKS(1)

enum SENS_CMU_DEBUG_REG_SEL_T {
    SENS_CMU_DEBUG_REG_SEL_MCU_PC   = 0,
    SENS_CMU_DEBUG_REG_SEL_MCU_LR   = 1,
    SENS_CMU_DEBUG_REG_SEL_MCU_SP   = 2,
    SENS_CMU_DEBUG_REG_SEL_DEBUG    = 7,
};

enum SENS_CMU_I2S_MCLK_SEL_T {
    SENS_CMU_I2S_MCLK_SEL_MCU       = 0,
    SENS_CMU_I2S_MCLK_SEL_SENS      = 1,
    SENS_CMU_I2S_MCLK_SEL_CODEC     = 2,
};

enum SENS_DMA_REQ_T {
    SENS_DMA_REQ_CODEC_RX           = 0,
    SENS_DMA_REQ_CODEC_TX           = 1,
    SENS_DMA_REQ_PCM_RX             = 2,
    SENS_DMA_REQ_PCM_TX             = 3,
    SENS_DMA_REQ_FIR_RX             = 4,
    SENS_DMA_REQ_FIR_TX             = 5,
    SENS_DMA_REQ_VAD_RX             = 6,
    SENS_DMA_REQ_CODEC_TX2          = 7,
    SENS_DMA_REQ_CODEC_MC           = 8,
    SENS_DMA_REQ_DSD_RX             = 9,
    SENS_DMA_REQ_DSD_TX             = 10,
    SENS_DMA_REQ_I2C0_RX            = 11,
    SENS_DMA_REQ_I2C0_TX            = 12,
    SENS_DMA_REQ_SPILCD0_RX         = 13,
    SENS_DMA_REQ_SPILCD0_TX         = 14,
    SENS_DMA_REQ_SPILCD1_RX         = 15,
    SENS_DMA_REQ_SPILCD1_TX         = 16,
    SENS_DMA_REQ_UART0_RX           = 17,
    SENS_DMA_REQ_UART0_TX           = 18,
    SENS_DMA_REQ_I2C1_RX            = 19,
    SENS_DMA_REQ_I2C1_TX            = 20,
    SENS_DMA_REQ_SPI_ITN_RX         = 21,
    SENS_DMA_REQ_SPI_ITN_TX         = 22,
    SENS_DMA_REQ_I2S0_RX            = 23,
    SENS_DMA_REQ_I2S0_TX            = 24,
    SENS_DMA_REQ_I2C2_RX            = 25,
    SENS_DMA_REQ_I2C2_TX            = 26,
    SENS_DMA_REQ_I2C3_RX            = 27,
    SENS_DMA_REQ_I2C3_TX            = 28,
    SENS_DMA_REQ_UART1_RX           = 29,
    SENS_DMA_REQ_UART1_TX           = 30,

    SENS_DMA_REQ_QTY,
    SENS_DMA_REQ_NULL               = SENS_DMA_REQ_QTY,
};

static struct SENSCMU_T * const senscmu = (struct SENSCMU_T *)SENS_CMU_BASE;

static struct AONCMU_T * const aoncmu = (struct AONCMU_T *)AON_CMU_BASE;

static struct BTCMU_T * const POSSIBLY_UNUSED btcmu = (struct BTCMU_T *)BT_CMU_BASE;

static uint8_t BOOT_BSS_LOC pll_user_map[HAL_CMU_PLL_QTY];
STATIC_ASSERT(HAL_CMU_PLL_USER_QTY <= sizeof(pll_user_map[0]) * 8, "Too many PLL users");

static enum HAL_CMU_PLL_T BOOT_DATA_LOC sys_pll_sel = HAL_CMU_PLL_QTY;

#ifdef LOW_SYS_FREQ
static enum HAL_CMU_FREQ_T BOOT_BSS_LOC cmu_sys_freq;
#endif

static bool anc_enabled;

#ifdef SENSOR_HUB_MINIMA
void sensor_hub_minima_setup_freq(int idx, uint8_t freq);
#endif

#ifdef __AUDIO_RESAMPLE__
static bool aud_resample_en = true;
#endif

void hal_cmu_audio_resample_enable(void)
{
#ifdef __AUDIO_RESAMPLE__
    aud_resample_en = true;
#endif
}

void hal_cmu_audio_resample_disable(void)
{
#ifdef __AUDIO_RESAMPLE__
    aud_resample_en = false;
#endif
}

int hal_cmu_get_audio_resample_status(void)
{
#ifdef __AUDIO_RESAMPLE__
    return aud_resample_en;
#else
    return false;
#endif
}

void hal_cmu_anc_enable(enum HAL_CMU_ANC_CLK_USER_T user)
{
    anc_enabled = true;
}

void hal_cmu_anc_disable(enum HAL_CMU_ANC_CLK_USER_T user)
{
    anc_enabled = false;
}

int hal_cmu_anc_get_status(enum HAL_CMU_ANC_CLK_USER_T user)
{
    return anc_enabled;
}

static inline void aocmu_reg_update_wait(void)
{
    // Make sure AOCMU (26M clock domain) write opertions finish before return
    aoncmu->CHIP_ID;
}

int hal_cmu_clock_enable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_AON_MCU) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        senscmu->HCLK_ENABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        senscmu->PCLK_ENABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        senscmu->OCLK_ENABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else {
        aoncmu->MOD_CLK_ENABLE = (1 << (id - HAL_CMU_AON_A_CMU));
        aocmu_reg_update_wait();
    }

    return 0;
}

int hal_cmu_clock_disable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_AON_MCU) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        senscmu->HCLK_DISABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        senscmu->PCLK_DISABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        senscmu->OCLK_DISABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else {
        aoncmu->MOD_CLK_DISABLE = (1 << (id - HAL_CMU_AON_A_CMU));
    }

    return 0;
}

enum HAL_CMU_CLK_STATUS_T hal_cmu_clock_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_AON_MCU) {
        return HAL_CMU_CLK_DISABLED;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = senscmu->HCLK_ENABLE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = senscmu->PCLK_ENABLE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        status = senscmu->OCLK_ENABLE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else {
        status = aoncmu->MOD_CLK_ENABLE & (1 << (id - HAL_CMU_AON_A_CMU));
    }

    return status ? HAL_CMU_CLK_ENABLED : HAL_CMU_CLK_DISABLED;
}

int hal_cmu_clock_set_mode(enum HAL_CMU_MOD_ID_T id, enum HAL_CMU_CLK_MODE_T mode)
{
    __IO uint32_t *reg;
    uint32_t val;
    uint32_t lock;

    if (id >= HAL_CMU_AON_MCU) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        reg = &senscmu->HCLK_MODE;
        val = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        reg = &senscmu->PCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        reg = &senscmu->OCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else {
        reg = &aoncmu->MOD_CLK_MODE;
        val = (1 << (id - HAL_CMU_AON_A_CMU));
    }

    lock = int_lock();
    if (mode == HAL_CMU_CLK_MANUAL) {
        *reg |= val;
    } else {
        *reg &= ~val;
    }
    int_unlock(lock);

    return 0;
}

enum HAL_CMU_CLK_MODE_T hal_cmu_clock_get_mode(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t mode;

    if (id >= HAL_CMU_AON_MCU) {
        return HAL_CMU_CLK_AUTO;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        mode = senscmu->HCLK_MODE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        mode = senscmu->PCLK_MODE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        mode = senscmu->OCLK_MODE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else {
        mode = aoncmu->MOD_CLK_MODE & (1 << (id - HAL_CMU_AON_A_CMU));
    }

    return mode ? HAL_CMU_CLK_MANUAL : HAL_CMU_CLK_AUTO;
}

int hal_cmu_reset_set(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        senscmu->HRESET_SET = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        senscmu->PRESET_SET = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        senscmu->ORESET_SET = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else if (id < HAL_CMU_AON_MCU) {
        aoncmu->RESET_SET = (1 << (id - HAL_CMU_AON_A_CMU));
    } else {
        aoncmu->GBL_RESET_SET = (1 << (id - HAL_CMU_AON_MCU));
    }

    return 0;
}

int hal_cmu_reset_clear(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        senscmu->HRESET_CLR = (1 << id);
        asm volatile("nop; nop;");
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        senscmu->PRESET_CLR = (1 << (id - HAL_CMU_MOD_P_CMU));
        asm volatile("nop; nop; nop; nop;");
    } else if (id < HAL_CMU_AON_A_CMU) {
        senscmu->ORESET_CLR = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else if (id < HAL_CMU_AON_MCU) {
        aoncmu->RESET_CLR = (1 << (id - HAL_CMU_AON_A_CMU));
        aocmu_reg_update_wait();
    } else {
        aoncmu->GBL_RESET_CLR = (1 << (id - HAL_CMU_AON_MCU));
        aocmu_reg_update_wait();
    }

    return 0;
}

enum HAL_CMU_RST_STATUS_T hal_cmu_reset_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_RST_SET;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = senscmu->HRESET_SET & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = senscmu->PRESET_SET & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        status = senscmu->ORESET_SET & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else if (id < HAL_CMU_AON_MCU) {
        status = aoncmu->RESET_SET & (1 << (id - HAL_CMU_AON_A_CMU));
    } else {
        status = aoncmu->GBL_RESET_SET & (1 << (id - HAL_CMU_AON_MCU));
    }

    return status ? HAL_CMU_RST_CLR : HAL_CMU_RST_SET;
}

int hal_cmu_reset_pulse(enum HAL_CMU_MOD_ID_T id)
{
    volatile int i;

    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (hal_cmu_reset_get_status(id) == HAL_CMU_RST_SET) {
        return hal_cmu_reset_clear(id);
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        senscmu->HRESET_PULSE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        senscmu->PRESET_PULSE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else if (id < HAL_CMU_AON_A_CMU) {
        senscmu->ORESET_PULSE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    } else if (id < HAL_CMU_AON_MCU) {
        aoncmu->RESET_PULSE = (1 << (id - HAL_CMU_AON_A_CMU));
        // Total 3 CLK-26M cycles needed
        // AOCMU runs in 26M clock domain and its read operations consume at least 1 26M-clock cycle.
        // (Whereas its write operations will finish at 1 HCLK cycle -- finish once in async bridge fifo)
        aoncmu->CHIP_ID;
        aoncmu->CHIP_ID;
        aoncmu->CHIP_ID;
    } else {
        aoncmu->GBL_RESET_PULSE = (1 << (id - HAL_CMU_AON_MCU));
        // Total 3 CLK-26M cycles needed
        // AOCMU runs in 26M clock domain and its read operations consume at least 1 26M-clock cycle.
        // (Whereas its write operations will finish at 1 HCLK cycle -- finish once in async bridge fifo)
        aoncmu->CHIP_ID;
        aoncmu->CHIP_ID;
        aoncmu->CHIP_ID;
    }
    // Delay 5+ PCLK cycles (10+ HCLK cycles)
    for (i = 0; i < 3; i++);

    return 0;
}

int hal_cmu_timer_set_div(enum HAL_CMU_TIMER_ID_T id, uint32_t div)
{
    uint32_t lock;

    if (div < 1) {
        return 1;
    }

    div -= 1;
    if ((div & (SENS_CMU_CFG_DIV_TIMER00_MASK >> SENS_CMU_CFG_DIV_TIMER00_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    if (id == HAL_CMU_TIMER_ID_00) {
        senscmu->TIMER0_CLK = SET_BITFIELD(senscmu->TIMER0_CLK, SENS_CMU_CFG_DIV_TIMER00, div);
        aoncmu->TIMER1_CLK = SET_BITFIELD(aoncmu->TIMER1_CLK, AON_CMU_CFG_DIV_TIMER10, div);
    } else if (id == HAL_CMU_TIMER_ID_01) {
        senscmu->TIMER0_CLK = SET_BITFIELD(senscmu->TIMER0_CLK, SENS_CMU_CFG_DIV_TIMER01, div);
        aoncmu->TIMER1_CLK = SET_BITFIELD(aoncmu->TIMER1_CLK, AON_CMU_CFG_DIV_TIMER11, div);
    } else if (id == HAL_CMU_TIMER_ID_10) {
        senscmu->TIMER1_CLK = SET_BITFIELD(senscmu->TIMER1_CLK, SENS_CMU_CFG_DIV_TIMER10, div);
    } else if (id == HAL_CMU_TIMER_ID_11) {
        senscmu->TIMER1_CLK = SET_BITFIELD(senscmu->TIMER1_CLK, SENS_CMU_CFG_DIV_TIMER11, div);
    } else if (id == HAL_CMU_TIMER_ID_20) {
        senscmu->TIMER2_CLK = SET_BITFIELD(senscmu->TIMER2_CLK, SENS_CMU_CFG_DIV_TIMER20, div);
    } else if (id == HAL_CMU_TIMER_ID_21) {
        senscmu->TIMER2_CLK = SET_BITFIELD(senscmu->TIMER2_CLK, SENS_CMU_CFG_DIV_TIMER21, div);
    }
    int_unlock(lock);

    return 0;
}

void hal_cmu_timer0_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    senscmu->CLK_OUT |= (1 << SENS_CMU_SEL_TIMER_FAST_SHIFT);
    // AON Timer
    aoncmu->CLK_OUT |= (1 << (AON_CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

void hal_cmu_timer0_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 16K
    senscmu->CLK_OUT &= ~(1 << SENS_CMU_SEL_TIMER_FAST_SHIFT);
    // AON Timer
    aoncmu->CLK_OUT &= ~(1 << (AON_CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

void hal_cmu_timer1_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    senscmu->CLK_OUT |= (1 << (SENS_CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

void hal_cmu_timer1_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 16K
    senscmu->CLK_OUT &= ~(1 << (SENS_CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

void hal_cmu_timer2_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    senscmu->CLK_OUT |= (1 << (SENS_CMU_SEL_TIMER_FAST_SHIFT + 2));
    int_unlock(lock);
}

void hal_cmu_timer2_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 16K
    senscmu->CLK_OUT &= ~(1 << (SENS_CMU_SEL_TIMER_FAST_SHIFT + 2));
    int_unlock(lock);
}

#ifdef LOW_SYS_FREQ
#ifdef LOW_SYS_FREQ_6P5M
int hal_cmu_fast_timer_offline(void)
{
    return (cmu_sys_freq == HAL_CMU_FREQ_6P5M);
}
#endif
#endif

int hal_cmu_sys_set_freq(enum HAL_CMU_FREQ_T freq)
{
    uint32_t enable;
    uint32_t disable;
    int div;
    uint32_t lock;

    if (freq >= HAL_CMU_FREQ_QTY) {
        return 1;
    }

#ifdef LOW_SYS_FREQ
    cmu_sys_freq = freq;
#endif

    div = -1;

    switch (freq) {
    case HAL_CMU_FREQ_32K:
        enable = SENS_CMU_SEL_OSCX2_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_SEL_OSC_2_SYS_DISABLE | SENS_CMU_SEL_OSC_4_SYS_DISABLE |
            SENS_CMU_SEL_SLOW_SYS_DISABLE | SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        break;
    case HAL_CMU_FREQ_6P5M:
#if defined(LOW_SYS_FREQ) && defined(LOW_SYS_FREQ_6P5M)
        enable = SENS_CMU_SEL_OSC_4_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_SEL_SLOW_SYS_DISABLE |
            SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        break;
#endif
    case HAL_CMU_FREQ_13M:
#ifdef LOW_SYS_FREQ
        enable = SENS_CMU_SEL_OSC_2_SYS_ENABLE | SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE |
            SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        break;
#endif
    case HAL_CMU_FREQ_26M:
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_SEL_OSC_2_SYS_DISABLE |
            SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        break;
    case HAL_CMU_FREQ_52M:
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE | SENS_CMU_SEL_FAST_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        break;
    case HAL_CMU_FREQ_78M:
#if !defined(OSC_26M_X4_AUD2BB) || defined(FREQ_78M_USE_PLL)
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_RSTN_DIV_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE | SENS_CMU_SEL_PLL_SYS_ENABLE;
        disable = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        div = 1;
        break;
#endif
    case HAL_CMU_FREQ_104M:
#ifdef OSC_26M_X4_AUD2BB
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_FAST_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_SEL_OSCX2_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE;
        break;
#else
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_RSTN_DIV_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE | SENS_CMU_SEL_PLL_SYS_ENABLE;
        disable = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        div = 0;
        break;
#endif
    case HAL_CMU_FREQ_208M:
    default:
        enable = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE | SENS_CMU_BYPASS_DIV_SYS_ENABLE | SENS_CMU_SEL_PLL_SYS_ENABLE;
        disable = SENS_CMU_RSTN_DIV_SYS_DISABLE;
        break;
    };

    if (div >= 0) {
        lock = int_lock();
        senscmu->SYS_DIV = SET_BITFIELD(senscmu->SYS_DIV, SENS_CMU_CFG_DIV_SYS, div);
        int_unlock(lock);
    }

    if (enable & SENS_CMU_SEL_PLL_SYS_ENABLE) {
        senscmu->SYS_CLK_ENABLE = SENS_CMU_RSTN_DIV_SYS_ENABLE;
        if (enable & SENS_CMU_BYPASS_DIV_SYS_ENABLE) {
            senscmu->SYS_CLK_ENABLE = SENS_CMU_BYPASS_DIV_SYS_ENABLE;
        } else {
            senscmu->SYS_CLK_DISABLE = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        }
    }
    senscmu->SYS_CLK_ENABLE = enable;
    if (enable & SENS_CMU_SEL_PLL_SYS_ENABLE) {
        senscmu->SYS_CLK_DISABLE = disable;
    } else {
        senscmu->SYS_CLK_DISABLE = disable & ~(SENS_CMU_RSTN_DIV_SYS_DISABLE | SENS_CMU_BYPASS_DIV_SYS_DISABLE);
        senscmu->SYS_CLK_DISABLE = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        senscmu->SYS_CLK_DISABLE = SENS_CMU_RSTN_DIV_SYS_DISABLE;
    }

    return 0;
}

enum HAL_CMU_FREQ_T BOOT_TEXT_SRAM_LOC hal_cmu_sys_get_freq(void)
{
    uint32_t sys_clk;
    uint32_t div;

    sys_clk = senscmu->SYS_CLK_ENABLE;

    if (sys_clk & SENS_CMU_SEL_PLL_SYS_ENABLE) {
        if (sys_clk & SENS_CMU_BYPASS_DIV_SYS_ENABLE) {
            return HAL_CMU_FREQ_208M;
        } else {
            div = GET_BITFIELD(senscmu->SYS_DIV, SENS_CMU_CFG_DIV_SYS);
            if (div == 0) {
                return HAL_CMU_FREQ_104M;
            } else if (div == 1) {
                // (div == 1): 69M
                return HAL_CMU_FREQ_78M;
            } else {
                // (div == 2): 52M
                // (div == 3): 42M
                return HAL_CMU_FREQ_52M;
            }
        }
    } else if ((sys_clk & (SENS_CMU_SEL_FAST_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE)) ==
            (SENS_CMU_SEL_FAST_SYS_ENABLE)) {
        return HAL_CMU_FREQ_104M;
    } else if ((sys_clk & (SENS_CMU_SEL_FAST_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE)) ==
            (SENS_CMU_SEL_FAST_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE)) {
        return HAL_CMU_FREQ_52M;
    } else if ((sys_clk & SENS_CMU_SEL_SLOW_SYS_ENABLE) == 0) {
        return HAL_CMU_FREQ_26M;
    } else {
        return HAL_CMU_FREQ_32K;
    }
}

int hal_cmu_sys_select_pll(enum HAL_CMU_PLL_T pll)
{
    uint32_t lock;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }

    // 0:audpll, 1:bbpll

    lock = int_lock();
    sys_pll_sel = pll;
    // For PLL clock selection
    if (pll == HAL_CMU_PLL_AUD) {
        senscmu->UART_CLK &= ~SENS_CMU_SEL_PLL_SRC;
    } else {
        senscmu->UART_CLK |= SENS_CMU_SEL_PLL_SRC;
    }
    int_unlock(lock);

    return 0;
}

int hal_cmu_audio_select_pll(enum HAL_CMU_PLL_T pll)
{
    uint32_t lock;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }

    // 0:audpll, 1:bbpll

    lock = int_lock();
    if (pll == HAL_CMU_PLL_AUD) {
        senscmu->UART_CLK &= ~SENS_CMU_SEL_PLL_AUD;
    } else {
        senscmu->UART_CLK |= SENS_CMU_SEL_PLL_AUD;
    }
    int_unlock(lock);

    return 0;
}

int hal_cmu_get_pll_status(enum HAL_CMU_PLL_T pll)
{
    if (0) {
    } else if (pll == sys_pll_sel) {
        return !!(senscmu->SYS_CLK_ENABLE & SENS_CMU_EN_PLL_ENABLE);
    } else if (pll == HAL_CMU_PLL_BB) {
        return !!(aoncmu->TOP_CLK_ENABLE & AON_CMU_EN_CLK_TOP_PLLBB_ENABLE);
    } else if (pll == HAL_CMU_PLL_AUD) {
        return !!(aoncmu->TOP_CLK_ENABLE & AON_CMU_EN_CLK_TOP_PLLAUD_ENABLE);
    }

    return 0;
}

int hal_cmu_pll_enable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    uint32_t pu_val;
    uint32_t en_val;
    uint32_t en_val1;
    uint32_t check;
    uint32_t lock;
    uint32_t start;
    uint32_t timeout;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }
    if (user >= HAL_CMU_PLL_USER_QTY && user != HAL_CMU_PLL_USER_ALL) {
        return 2;
    }

    if (pll == HAL_CMU_PLL_BB) {
        pu_val = AON_CMU_PU_PLLBB_ENABLE;
        en_val = AON_CMU_EN_CLK_TOP_PLLBB_ENABLE;
        en_val1 = AON_CMU_EN_CLK_PLLBB_SENS_ENABLE;
        check = AON_CMU_LOCK_PLLBB;
    } else {
        pu_val = AON_CMU_PU_PLLAUD_ENABLE;
        en_val = AON_CMU_EN_CLK_TOP_PLLAUD_ENABLE;
        en_val1 = AON_CMU_EN_CLK_PLLAUD_SENS_ENABLE;
        check = AON_CMU_LOCK_PLLAUD;
    }

    lock = int_lock();
    if (pll_user_map[pll] == 0 || user == HAL_CMU_PLL_USER_ALL) {
        if (pll == sys_pll_sel) {
            senscmu->SYS_CLK_ENABLE = SENS_CMU_PU_PLL_ENABLE;
        } else {
            aoncmu->TOP_CLK_ENABLE = pu_val;
        }
        // Wait at least 10us for clock ready
        hal_sys_timer_delay_us(20);
    } else {
        check = 0;
    }
    if (user < HAL_CMU_PLL_USER_QTY) {
        pll_user_map[pll] |= (1 << user);
    }
    int_unlock(lock);

    start = hal_sys_timer_get();
    timeout = HAL_CMU_PLL_LOCKED_TIMEOUT;
    do {
        if (check) {
            if (aoncmu->AON_CLK & check) {
                //break;
            }
        } else {
            if (pll == sys_pll_sel) {
                if (senscmu->SYS_CLK_ENABLE & SENS_CMU_EN_PLL_ENABLE) {
                    break;
                }
            } else {
                if (aoncmu->TOP_CLK_ENABLE & en_val) {
                    break;
                }
            }
        }
    } while ((hal_sys_timer_get() - start) < timeout);

    if (pll == sys_pll_sel) {
        senscmu->SYS_CLK_ENABLE = SENS_CMU_EN_PLL_ENABLE;
    } else {
        aoncmu->TOP_CLK_ENABLE = en_val;
        aoncmu->TOP_CLK1_ENABLE = en_val1;
    }

    return (aoncmu->AON_CLK & check) ? 0 : 2;
}

int hal_cmu_pll_disable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    uint32_t pu_val;
    uint32_t en_val;
    uint32_t en_val1;
    uint32_t lock;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }
    if (user >= HAL_CMU_PLL_USER_QTY && user != HAL_CMU_PLL_USER_ALL) {
        return 2;
    }

    if (pll == HAL_CMU_PLL_BB) {
        pu_val = AON_CMU_PU_PLLBB_DISABLE;
        en_val = AON_CMU_EN_CLK_TOP_PLLBB_DISABLE;
        en_val1 = AON_CMU_EN_CLK_PLLBB_SENS_DISABLE;
    } else {
        pu_val = AON_CMU_PU_PLLAUD_DISABLE;
        en_val = AON_CMU_EN_CLK_TOP_PLLAUD_DISABLE;
        en_val1 = AON_CMU_EN_CLK_PLLAUD_SENS_DISABLE;
    }

    lock = int_lock();
    if (user < HAL_CMU_PLL_USER_ALL) {
        pll_user_map[pll] &= ~(1 << user);
    }
    if (pll_user_map[pll] == 0 || user == HAL_CMU_PLL_USER_ALL) {
        if (pll == sys_pll_sel) {
            senscmu->SYS_CLK_DISABLE = SENS_CMU_EN_PLL_DISABLE;
            senscmu->SYS_CLK_DISABLE = SENS_CMU_PU_PLL_DISABLE;
        } else {
            aoncmu->TOP_CLK1_DISABLE = en_val1;
            aoncmu->TOP_CLK_DISABLE = en_val;
            aoncmu->TOP_CLK_DISABLE = pu_val;
        }
    }
    int_unlock(lock);

    return 0;
}

int hal_cmu_sys_is_using_audpll(void)
{
    return false;
}

void hal_cmu_low_freq_mode_enable(enum HAL_CMU_FREQ_T old_freq, enum HAL_CMU_FREQ_T new_freq)
{
    // TODO: Need to lock irq?
    enum HAL_CMU_PLL_T POSSIBLY_UNUSED pll;

#ifdef SENSOR_HUB_MINIMA
    sensor_hub_minima_setup_freq(1, new_freq);
#endif

    pll = sys_pll_sel;

#ifdef OSC_26M_X4_AUD2BB
    if (old_freq > HAL_CMU_FREQ_104M && new_freq <= HAL_CMU_FREQ_104M) {
        hal_cmu_pll_disable(pll, HAL_CMU_PLL_USER_SYS);
    }
#endif
}

void hal_cmu_low_freq_mode_disable(enum HAL_CMU_FREQ_T old_freq, enum HAL_CMU_FREQ_T new_freq)
{
    // TODO: Need to lock irq?
    enum HAL_CMU_PLL_T POSSIBLY_UNUSED pll;

#ifdef SENSOR_HUB_MINIMA
    sensor_hub_minima_setup_freq(1, new_freq);
#endif

    pll = sys_pll_sel;

#ifdef OSC_26M_X4_AUD2BB
    if (old_freq <= HAL_CMU_FREQ_104M && new_freq > HAL_CMU_FREQ_104M) {
        hal_cmu_pll_enable(pll, HAL_CMU_PLL_USER_SYS);
    }
#endif
}

void hal_cmu_init_pll_selection(void)
{
    enum HAL_CMU_PLL_T sys;

#if defined(SYS_USE_BBPLL)
    sys = HAL_CMU_PLL_BB;
#else
    sys = HAL_CMU_PLL_AUD;
#endif
    hal_cmu_sys_select_pll(sys);

#ifdef AUDIO_USE_BBPLL
    hal_cmu_audio_select_pll(HAL_CMU_PLL_BB);
#else
    hal_cmu_audio_select_pll(HAL_CMU_PLL_AUD);
#endif

#if !defined(ULTRA_LOW_POWER) && !defined(OSC_26M_X4_AUD2BB)
    hal_cmu_pll_enable(sys, HAL_CMU_PLL_USER_SYS);
#endif
}

enum HAL_CMU_PLL_T hal_cmu_get_audio_pll(void)
{
    return AUDIO_PLL_SEL;
}

void hal_cmu_codec_clock_enable(void)
{
    uint32_t lock;
    uint32_t mask;
    uint32_t val;

#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    hal_cmu_clock_enable(HAL_CMU_MOD_H_VAD);
#endif
    hal_cmu_clock_enable(HAL_CMU_MOD_O_ADC_FREE);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_ADC_CH0);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_ADC_CH1);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_ADC_ANA);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_SADC_ANA);

    mask = 0;
    val = SENS_CMU_POL_ADC_ANA | SENS_CMU_POL_SARADC_ANA;
#ifdef CODEC_CLK_FROM_ANA
    mask |= SENS_CMU_SEL_CODEC_CLK_OSC;
#else
    val |=  SENS_CMU_SEL_CODEC_CLK_OSC;
#endif

    lock = int_lock();
    senscmu->I2C_CLK = (senscmu->I2C_CLK & ~mask) | val;
    int_unlock(lock);
}

void hal_cmu_codec_clock_disable(void)
{
    hal_cmu_clock_disable(HAL_CMU_MOD_O_ADC_FREE);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_ADC_CH0);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_ADC_CH1);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_ADC_ANA);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_SADC_ANA);
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    hal_cmu_clock_disable(HAL_CMU_MOD_H_VAD);
#endif
}

void hal_cmu_codec_vad_clock_enable(int type)
{
    aoncmu->TOP_CLK1_ENABLE = AON_CMU_EN_CLK_32K_SENS_ENABLE;
    if (type == AUD_VAD_TYPE_DIG) {
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_FREE, HAL_CMU_CLK_MANUAL);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH0,  HAL_CMU_CLK_MANUAL);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH1,  HAL_CMU_CLK_MANUAL);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_ANA,  HAL_CMU_CLK_MANUAL);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_SADC_ANA, HAL_CMU_CLK_MANUAL);
    } else {
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_FREE, HAL_CMU_CLK_AUTO);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH0,  HAL_CMU_CLK_AUTO);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH1,  HAL_CMU_CLK_AUTO);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_ANA,  HAL_CMU_CLK_AUTO);
        hal_cmu_clock_set_mode(HAL_CMU_MOD_O_SADC_ANA, HAL_CMU_CLK_AUTO);
    }
}

void hal_cmu_codec_vad_clock_disable(int type)
{
    hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_FREE, HAL_CMU_CLK_MANUAL);
    hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH0,  HAL_CMU_CLK_MANUAL);
    hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_CH1,  HAL_CMU_CLK_MANUAL);
    hal_cmu_clock_set_mode(HAL_CMU_MOD_O_ADC_ANA,  HAL_CMU_CLK_MANUAL);
    hal_cmu_clock_set_mode(HAL_CMU_MOD_O_SADC_ANA, HAL_CMU_CLK_MANUAL);
}

void hal_cmu_codec_reset_set(void)
{
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    hal_cmu_reset_set(HAL_CMU_MOD_H_VAD);
#endif
    hal_cmu_reset_set(HAL_CMU_MOD_O_ADC_FREE);
    hal_cmu_reset_set(HAL_CMU_MOD_O_ADC_CH0);
    hal_cmu_reset_set(HAL_CMU_MOD_O_ADC_CH1);
    hal_cmu_reset_set(HAL_CMU_MOD_O_ADC_ANA);
    hal_cmu_reset_set(HAL_CMU_MOD_O_SADC_ANA);
}

void hal_cmu_codec_reset_clear(void)
{
    hal_cmu_reset_clear(HAL_CMU_MOD_O_ADC_FREE);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_ADC_CH0);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_ADC_CH1);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_ADC_ANA);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_SADC_ANA);
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    hal_cmu_reset_clear(HAL_CMU_MOD_H_VAD);
#endif
}

void hal_cmu_codec_set_fault_mask(uint32_t msk)
{
    uint32_t lock;

    lock = int_lock();
    // If bit set 1, DAC will be muted when some faults occur
    senscmu->CLK_OUT = SET_BITFIELD(senscmu->CLK_OUT, SENS_CMU_MASK_OBS, msk);
    int_unlock(lock);
}

void hal_cmu_i2s_clock_out_enable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;
    uint32_t val;

    val = SENS_CMU_EN_CLK_I2S_OUT;

    lock = int_lock();
    senscmu->I2C_CLK |= val;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_out_disable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;
    uint32_t val;

    val = SENS_CMU_EN_CLK_I2S_OUT;

    lock = int_lock();
    senscmu->I2C_CLK &= ~val;
    int_unlock(lock);
}

void hal_cmu_i2s_set_slave_mode(enum HAL_I2S_ID_T id)
{
    uint32_t lock;
    uint32_t val;

    val = SENS_CMU_SEL_I2S_CLKIN;

    lock = int_lock();
    senscmu->I2C_CLK |= val;
    int_unlock(lock);
}

void hal_cmu_i2s_set_master_mode(enum HAL_I2S_ID_T id)
{
    uint32_t lock;
    uint32_t val;

    val = SENS_CMU_SEL_I2S_CLKIN;

    lock = int_lock();
    senscmu->I2C_CLK &= ~val;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_enable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->UART_CLK |= SENS_CMU_EN_CLK_PLL_I2S0;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_disable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->UART_CLK &= ~SENS_CMU_EN_CLK_PLL_I2S0;
    int_unlock(lock);
}

int hal_cmu_i2s_set_div(enum HAL_I2S_ID_T id, uint32_t div)
{
    uint32_t lock;

    if (div < 2) {
        return 1;
    }

    div -= 2;
    if ((div & (SENS_CMU_CFG_DIV_I2S0_MASK >> SENS_CMU_CFG_DIV_I2S0_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    senscmu->UART_CLK = SET_BITFIELD(senscmu->UART_CLK, SENS_CMU_CFG_DIV_I2S0, div);
    int_unlock(lock);

    return 0;
}

void hal_cmu_pcm_clock_out_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->I2C_CLK |= SENS_CMU_EN_CLK_PCM_OUT;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_out_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->I2C_CLK &= ~SENS_CMU_EN_CLK_PCM_OUT;
    int_unlock(lock);
}

void hal_cmu_pcm_set_slave_mode(int clk_pol)
{
    uint32_t lock;
    uint32_t mask;
    uint32_t cfg;

    mask = SENS_CMU_SEL_PCM_CLKIN | SENS_CMU_POL_CLK_PCM_IN;

    if (clk_pol) {
        cfg = SENS_CMU_SEL_PCM_CLKIN | SENS_CMU_POL_CLK_PCM_IN;
    } else {
        cfg = SENS_CMU_SEL_PCM_CLKIN;
    }

    lock = int_lock();
    senscmu->I2C_CLK = (senscmu->I2C_CLK & ~mask) | cfg;
    int_unlock(lock);
}

void hal_cmu_pcm_set_master_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->I2C_CLK &= ~SENS_CMU_SEL_PCM_CLKIN;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->UART_CLK |= SENS_CMU_EN_CLK_PLL_PCM;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->UART_CLK &= ~SENS_CMU_EN_CLK_PLL_PCM;
    int_unlock(lock);
}

int hal_cmu_pcm_set_div(uint32_t div)
{
    uint32_t lock;

    if (div < 2) {
        return 1;
    }

    div -= 2;
    if ((div & (SENS_CMU_CFG_DIV_PCM_MASK >> SENS_CMU_CFG_DIV_PCM_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    senscmu->UART_CLK = SET_BITFIELD(senscmu->UART_CLK, SENS_CMU_CFG_DIV_PCM, div);
    int_unlock(lock);
    return 0;
}

void hal_cmu_apb_init_div(void)
{
    // Divider defaults to 2 (reg_val = div - 2)
    //senscmu->SYS_DIV = SET_BITFIELD(senscmu->SYS_DIV, SENS_CMU_CFG_DIV_PCLK, 0);
}

#define PERPH_SET_FREQ_FUNC(f, F, r) \
int hal_cmu_ ##f## _set_freq(enum HAL_CMU_PERIPH_FREQ_T freq) \
{ \
    uint32_t lock; \
    int ret = 0; \
    lock = int_lock(); \
    if (freq == HAL_CMU_PERIPH_FREQ_26M) { \
        senscmu->r &= ~(SENS_CMU_SEL_OSCX2_ ##F); \
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) { \
        senscmu->r |= (SENS_CMU_SEL_OSCX2_ ##F); \
    } else { \
        ret = 1; \
    } \
    int_unlock(lock); \
    return ret; \
}

PERPH_SET_FREQ_FUNC(uart0, UART0, UART_CLK);
PERPH_SET_FREQ_FUNC(uart1, UART1, UART_CLK);
PERPH_SET_FREQ_FUNC(spi, SPI1, SYS_DIV);
PERPH_SET_FREQ_FUNC(slcd, SPI0, SYS_DIV);
PERPH_SET_FREQ_FUNC(i2c, I2C, I2C_CLK);

int hal_cmu_ispi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock;
    int ret = 0;

    lock = int_lock();
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        senscmu->SYS_DIV &= ~SENS_CMU_SEL_OSCX2_SPI2;
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        senscmu->SYS_DIV |= SENS_CMU_SEL_OSCX2_SPI2;
    } else {
        ret = 1;
    }
    int_unlock(lock);

    return ret;
}

int hal_cmu_clock_out_enable(enum HAL_CMU_CLOCK_OUT_ID_T id)
{
#if 0
    uint32_t lock;
    uint32_t sel;
    uint32_t cfg;

    enum CMU_CLK_OUT_SEL_T {
        CMU_CLK_OUT_SEL_NONE    = 0,
        CMU_CLK_OUT_SEL_CODEC   = 1,
        CMU_CLK_OUT_SEL_BT      = 2,
        CMU_CLK_OUT_SEL_MCU     = 3,
        CMU_CLK_OUT_SEL_SENS    = 4,
        CMU_CLK_OUT_SEL_AON     = 5,

        CMU_CLK_OUT_SEL_QTY
    };

    sel = CMU_CLK_OUT_SEL_QTY;
    cfg = 0;

    if (HAL_CMU_CLOCK_OUT_AON_32K <= id && id <= HAL_CMU_CLOCK_OUT_AON_DCDC) {
        sel = CMU_CLK_OUT_SEL_AON;
        cfg = id - HAL_CMU_CLOCK_OUT_AON_32K;
    } else if (HAL_CMU_CLOCK_OUT_MCU_32K <= id && id <= HAL_CMU_CLOCK_OUT_MCU_DSI) {
        sel = CMU_CLK_OUT_SEL_MCU;
        lock = int_lock();
        cmu->PERIPH_CLK = SET_BITFIELD(cmu->PERIPH_CLK, CMU_CFG_CLK_OUT, id - HAL_CMU_CLOCK_OUT_MCU_32K);
        int_unlock(lock);
    } else if (HAL_CMU_CLOCK_OUT_CODEC_ADC_ANA <= id && id <= HAL_CMU_CLOCK_OUT_CODEC_HCLK) {
        ASSERT(false, "This operation is not supported in sensorhub!");
        return 1;
    } else if (HAL_CMU_CLOCK_OUT_BT_NONE <= id && id <= HAL_CMU_CLOCK_OUT_BT_DACD8) {
        sel = CMU_CLK_OUT_SEL_BT;
        btcmu->CLK_OUT = SET_BITFIELD(btcmu->CLK_OUT, BT_CMU_CFG_CLK_OUT, id - HAL_CMU_CLOCK_OUT_BT_NONE);
    } else if (HAL_CMU_CLOCK_OUT_SENS_32K <= id && id <= HAL_CMU_CLOCK_OUT_SENS_I2S) {
        sel = CMU_CLK_OUT_SEL_SENS;
        senscmu->CLK_OUT = SET_BITFIELD(senscmu->CLK_OUT, SENS_CMU_CFG_CLK_OUT, id - HAL_CMU_CLOCK_OUT_SENS_32K);
    }

    if (sel < CMU_CLK_OUT_SEL_QTY) {
        lock = int_lock();
        aoncmu->CLK_OUT = (aoncmu->CLK_OUT & ~(AON_CMU_SEL_CLK_OUT_MASK | AON_CMU_CFG_CLK_OUT_MASK)) |
            AON_CMU_SEL_CLK_OUT(sel) | AON_CMU_CFG_CLK_OUT(cfg) | AON_CMU_EN_CLK_OUT;
        int_unlock(lock);

        return 0;
    }
#endif
    return 1;
}

void hal_cmu_clock_out_disable(void)
{
#if 0
    uint32_t lock;

    lock = int_lock();
    aoncmu->CLK_OUT &= ~AON_CMU_EN_CLK_OUT;
    int_unlock(lock);
#endif
}

int hal_cmu_i2s_mclk_enable(enum HAL_CMU_I2S_MCLK_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->I2C_CLK |= SENS_CMU_EN_I2S_MCLK | ((id == HAL_CMU_I2S_MCLK_PLLI2S0) ? 0 : SENS_CMU_SEL_I2S_MCLK);
    aoncmu->AON_CLK = (aoncmu->AON_CLK & ~AON_CMU_SEL_I2S_MCLK_MASK) |
        AON_CMU_SEL_I2S_MCLK(SENS_CMU_I2S_MCLK_SEL_SENS) | AON_CMU_EN_I2S_MCLK;
    int_unlock(lock);

    return 0;
}

void hal_cmu_i2s_mclk_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    senscmu->I2C_CLK &= ~SENS_CMU_EN_I2S_MCLK;
    aoncmu->AON_CLK &= ~AON_CMU_EN_I2S_MCLK;
    int_unlock(lock);
}

int hal_cmu_pwm_set_freq(enum HAL_PWM_ID_T id, uint32_t freq)
{
    uint32_t lock;
    int clk_32k;
    uint32_t div;

    if (id >= HAL_PWM_ID_QTY) {
        return 1;
    }

    if (freq == 0) {
        clk_32k = 1;
        div = 0;
    } else {
        clk_32k = 0;
        div = hal_cmu_get_crystal_freq() / freq;
        if (div < 2) {
            return 1;
        }

        div -= 2;
        if ((div & (AON_CMU_CFG_DIV_PWM0_MASK >> AON_CMU_CFG_DIV_PWM0_SHIFT)) != div) {
            return 1;
        }
    }

    lock = int_lock();
    if (id == HAL_PWM_ID_0) {
        aoncmu->PWM01_CLK = (aoncmu->PWM01_CLK & ~(AON_CMU_CFG_DIV_PWM0_MASK | AON_CMU_SEL_OSC_PWM0 | AON_CMU_EN_OSC_PWM0)) |
            AON_CMU_CFG_DIV_PWM0(div) | (clk_32k ? 0 : (AON_CMU_SEL_OSC_PWM0 | AON_CMU_EN_OSC_PWM0));
    } else if (id == HAL_PWM_ID_1) {
        aoncmu->PWM01_CLK = (aoncmu->PWM01_CLK & ~(AON_CMU_CFG_DIV_PWM1_MASK | AON_CMU_SEL_OSC_PWM1 | AON_CMU_EN_OSC_PWM1)) |
            AON_CMU_CFG_DIV_PWM1(div) | (clk_32k ? 0 : (AON_CMU_SEL_OSC_PWM1 | AON_CMU_EN_OSC_PWM1));
    } else if (id == HAL_PWM_ID_2) {
        aoncmu->PWM23_CLK = (aoncmu->PWM23_CLK & ~(AON_CMU_CFG_DIV_PWM2_MASK | AON_CMU_SEL_OSC_PWM2 | AON_CMU_EN_OSC_PWM2)) |
            AON_CMU_CFG_DIV_PWM2(div) | (clk_32k ? 0 : (AON_CMU_SEL_OSC_PWM2 | AON_CMU_EN_OSC_PWM2));
    } else {
        aoncmu->PWM23_CLK = (aoncmu->PWM23_CLK & ~(AON_CMU_CFG_DIV_PWM3_MASK | AON_CMU_SEL_OSC_PWM3 | AON_CMU_EN_OSC_PWM3)) |
            AON_CMU_CFG_DIV_PWM3(div) | (clk_32k ? 0 : (AON_CMU_SEL_OSC_PWM3 | AON_CMU_EN_OSC_PWM3));
    }
    int_unlock(lock);
    return 0;
}

void hal_cmu_jtag_enable(void)
{
#if 0
    uint32_t lock;

    lock = int_lock();
    aonsec->SEC_BOOT_ACC &= ~(AON_SEC_SECURE_BOOT_JTAG | AON_SEC_SECURE_BOOT_I2C);
    int_unlock(lock);
#endif
}

void hal_cmu_jtag_disable(void)
{
#if 0
    uint32_t lock;

    lock = int_lock();
    aonsec->SEC_BOOT_ACC |= (AON_SEC_SECURE_BOOT_JTAG | AON_SEC_SECURE_BOOT_I2C);
    int_unlock(lock);
#endif
}

void hal_cmu_jtag_clock_enable(void)
{
#if 0
    aoncmu->TOP_CLK_ENABLE = AON_CMU_EN_CLK_TOP_JTAG_ENABLE;
#endif
}

void hal_cmu_jtag_clock_disable(void)
{
#if 0
    aoncmu->TOP_CLK_DISABLE = AON_CMU_EN_CLK_TOP_JTAG_DISABLE;
#endif
}

void hal_cmu_mcu_pdm_clock_out(uint32_t clk_map)
{
    uint32_t lock;
    uint32_t val = 0;

    if (clk_map & (1 << 0)) {
        val |= AON_CMU_SEL_PDM_CLKOUT0_SENS;
    }
    if (clk_map & (1 << 1)) {
        val |= AON_CMU_SEL_PDM_CLKOUT1_SENS;
    }

    lock = int_lock();
    aoncmu->AON_CLK |= val;
    int_unlock(lock);
}

void hal_cmu_sens_pdm_clock_out(uint32_t clk_map)
{
    uint32_t lock;
    uint32_t val = 0;

    if (clk_map & (1 << 0)) {
        val |= AON_CMU_SEL_PDM_CLKOUT0_SENS;
    }
    if (clk_map & (1 << 1)) {
        val |= AON_CMU_SEL_PDM_CLKOUT1_SENS;
    }

    lock = int_lock();
    aoncmu->AON_CLK |= val;
    int_unlock(lock);
}

void hal_cmu_osc_x2_enable(void)
{
    senscmu->SYS_CLK_ENABLE = SENS_CMU_PU_OSCX2_ENABLE;
    hal_sys_timer_delay(US_TO_TICKS(60));
    senscmu->SYS_CLK_ENABLE = SENS_CMU_EN_OSCX2_ENABLE;
}

void hal_cmu_osc_x4_enable(void)
{
#ifdef ANA_26M_X4_ENABLE
    senscmu->SYS_CLK_ENABLE = SENS_CMU_PU_OSCX4_ENABLE;
    hal_sys_timer_delay(US_TO_TICKS(60));
    senscmu->SYS_CLK_ENABLE = SENS_CMU_EN_OSCX4_ENABLE;
#endif
#ifdef OSC_26M_X4_AUD2BB
    aoncmu->CLK_SELECT &= ~AON_CMU_SEL_X4_DIG;
#endif
}

void hal_cmu_module_init_state(void)
{
    // DMA channel config
    senscmu->ADMA_CH0_4_REQ =
        // i2c0
        SENS_CMU_SDMA_CH0_REQ_IDX(SENS_DMA_REQ_I2C0_RX) | SENS_CMU_SDMA_CH1_REQ_IDX(SENS_DMA_REQ_I2C0_TX) |
        // i2c1
        SENS_CMU_SDMA_CH2_REQ_IDX(SENS_DMA_REQ_I2C1_RX) | SENS_CMU_SDMA_CH3_REQ_IDX(SENS_DMA_REQ_I2C1_TX) |
        // i2c2
        SENS_CMU_SDMA_CH4_REQ_IDX(SENS_DMA_REQ_I2C2_RX);
    senscmu->ADMA_CH5_9_REQ =
        // i2c2
        SENS_CMU_SDMA_CH5_REQ_IDX(SENS_DMA_REQ_I2C2_TX) |
        // spi0
        SENS_CMU_SDMA_CH6_REQ_IDX(SENS_DMA_REQ_SPILCD0_RX) | SENS_CMU_SDMA_CH7_REQ_IDX(SENS_DMA_REQ_SPILCD0_TX) |
        // spi1
        SENS_CMU_SDMA_CH8_REQ_IDX(SENS_DMA_REQ_SPILCD1_RX) | SENS_CMU_SDMA_CH9_REQ_IDX(SENS_DMA_REQ_SPILCD1_TX);
    senscmu->ADMA_CH10_14_REQ =
#ifdef BEST1501_SENS_I2S_DMA_ENABLE
        //i2s0
        SENS_CMU_SDMA_CH10_REQ_IDX(SENS_DMA_REQ_I2S0_RX) | SENS_CMU_SDMA_CH11_REQ_IDX(SENS_DMA_REQ_I2S0_TX) |
#else
        // i2c3
        SENS_CMU_SDMA_CH10_REQ_IDX(SENS_DMA_REQ_I2C3_RX) | SENS_CMU_SDMA_CH11_REQ_IDX(SENS_DMA_REQ_I2C3_TX) |
#endif
        // vad rx and uart0 tx
        SENS_CMU_SDMA_CH12_REQ_IDX(SENS_DMA_REQ_VAD_RX) | SENS_CMU_SDMA_CH13_REQ_IDX(SENS_DMA_REQ_UART0_TX) |
        // uart1
        SENS_CMU_SDMA_CH14_REQ_IDX(SENS_DMA_REQ_UART1_RX);
    senscmu->ADMA_CH15_REQ =
        // uart1
        SENS_CMU_SDMA_CH15_REQ_IDX(SENS_DMA_REQ_UART1_TX);

#ifndef SIMU
    senscmu->ORESET_SET = SENS_ORST_WDT | SENS_ORST_TIMER2 |
        SENS_ORST_I2C0 | SENS_ORST_I2C1 | SENS_ORST_SPI | SENS_ORST_SLCD |
        SENS_ORST_UART0 | SENS_ORST_UART1 | SENS_ORST_PCM | SENS_ORST_I2S |
        SENS_ORST_I2C2 | SENS_ORST_I2C3 | SENS_ORST_CAP0 | SENS_ORST_CAP1 | SENS_ORST_CAP2 |
        SENS_ORST_ADC_FREE | SENS_ORST_ADC_CH0 | SENS_ORST_ADC_CH1 | SENS_ORST_ADC_ANA | SENS_ORST_SADC_ANA;
    senscmu->PRESET_SET = SENS_PRST_WDT | SENS_PRST_TIMER2 | SENS_PRST_I2C0 | SENS_PRST_I2C1 |
        SENS_PRST_SPI | SENS_PRST_SLCD | SENS_PRST_UART0 | SENS_PRST_UART1 |
        SENS_PRST_PCM | SENS_PRST_I2S | SENS_PRST_I2C2 | SENS_PRST_I2C3 |
        SENS_PRST_CAP | SENS_PRST_PSPY;
    senscmu->HRESET_SET = SENS_HRST_SENSOR_HUB | SENS_HRST_PSPY;
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    senscmu->HRESET_SET = SENS_HRST_VAD;
#endif

#ifndef MCU_I2C_SLAVE
    senscmu->HRESET_SET = SENS_HRST_CODEC;
    senscmu->HCLK_DISABLE = SENS_HCLK_CODEC;
#endif

    senscmu->OCLK_DISABLE = SENS_OCLK_WDT | SENS_OCLK_TIMER2 |
        SENS_OCLK_I2C0 | SENS_OCLK_I2C1 | SENS_OCLK_SPI | SENS_OCLK_SLCD |
        SENS_OCLK_UART0 | SENS_OCLK_UART1 | SENS_OCLK_PCM | SENS_OCLK_I2S |
        SENS_OCLK_I2C2 | SENS_OCLK_I2C3 | SENS_OCLK_CAP0 | SENS_OCLK_CAP1 | SENS_OCLK_CAP2 |
        SENS_OCLK_ADC_FREE | SENS_OCLK_ADC_CH0 | SENS_OCLK_ADC_CH1 | SENS_OCLK_ADC_ANA | SENS_OCLK_SADC_ANA;
    senscmu->PCLK_DISABLE = SENS_PCLK_WDT | SENS_PCLK_TIMER2 | SENS_PCLK_I2C0 | SENS_PCLK_I2C1 |
        SENS_PCLK_SPI | SENS_PCLK_SLCD | SENS_PCLK_UART0 | SENS_PCLK_UART1 |
        SENS_PCLK_PCM | SENS_PCLK_I2S | SENS_PCLK_I2C2 | SENS_PCLK_I2C3 |
        SENS_PCLK_CAP | SENS_PCLK_PSPY;
    senscmu->HCLK_DISABLE = SENS_HCLK_SENSOR_HUB | SENS_HCLK_PSPY;
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE == CODEC_VAD_MAX_BUF_SIZE)
    senscmu->HCLK_DISABLE = SENS_HCLK_VAD;
#endif

    //senscmu->HCLK_MODE = 0;
    //senscmu->PCLK_MODE = SENS_PCLK_UART0 | SENS_PCLK_UART1;
    //senscmu->OCLK_MODE = 0;
#endif
}

void hal_cmu_ema_init(void)
{
    // Never change EMA
}

void hal_cmu_lpu_wait_26m_ready(void)
{
    while ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_26M) == 0);
}

int hal_cmu_lpu_busy(void)
{
    if ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_AUTO_SWITCH26) &&
        (senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_26M) == 0) {
        return 1;
    }
    if ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_AUTO_SWITCHPLL) &&
        (senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_PLL) == 0) {
        return 1;
    }
    return 0;
}

int hal_cmu_lpu_init(enum HAL_CMU_LPU_CLK_CFG_T cfg)
{
    uint32_t lpu_clk;
    uint32_t timer_26m;
    uint32_t timer_pll;

    timer_26m = LPU_TIMER_US(TICKS_TO_US(HAL_CMU_26M_READY_TIMEOUT));
    timer_pll = LPU_TIMER_US(TICKS_TO_US(HAL_CMU_PLL_LOCKED_TIMEOUT));

    if (cfg >= HAL_CMU_LPU_CLK_QTY) {
        return 1;
    }
    if ((timer_26m & (SENS_CMU_TIMER_WT26_MASK >> SENS_CMU_TIMER_WT26_SHIFT)) != timer_26m) {
        return 2;
    }
    if ((timer_pll & (SENS_CMU_TIMER_WTPLL_MASK >> SENS_CMU_TIMER_WTPLL_SHIFT)) != timer_pll) {
        return 3;
    }
    if (hal_cmu_lpu_busy()) {
        return -1;
    }

    if (cfg == HAL_CMU_LPU_CLK_26M) {
        lpu_clk = SENS_CMU_LPU_AUTO_SWITCH26;
    } else if (cfg == HAL_CMU_LPU_CLK_PLL) {
        lpu_clk = SENS_CMU_LPU_AUTO_SWITCHPLL | SENS_CMU_LPU_AUTO_SWITCH26;
    } else {
        lpu_clk = 0;
    }

    if (lpu_clk & SENS_CMU_LPU_AUTO_SWITCH26) {
        // Disable RAM wakeup early
        senscmu->SENS_TIMER &= ~SENS_CMU_RAM_RETN_UP_EARLY;
        // MCU/ROM/RAM auto clock gating (which depends on RAM gating signal)
        senscmu->HCLK_MODE &= ~(SENS_HCLK_MCU | SENS_HCLK_RAM0 | SENS_HCLK_RAM1 | SENS_HCLK_RAM2 | SENS_HCLK_RAM3 |
            SENS_HCLK_RAM4 | SENS_HCLK_RAM5);
    }

    senscmu->WAKEUP_CLK_CFG = SENS_CMU_TIMER_WT26(timer_26m) | SENS_CMU_TIMER_WTPLL(0) | lpu_clk;
    if (timer_pll) {
        hal_sys_timer_delay(US_TO_TICKS(60));
        senscmu->WAKEUP_CLK_CFG = SENS_CMU_TIMER_WT26(timer_26m) | SENS_CMU_TIMER_WTPLL(timer_pll) | lpu_clk;
    }
    return 0;
}

#ifdef CORE_SLEEP_POWER_DOWN
static void save_senscmu_regs(struct SAVED_SENSCMU_REGS_T *senssav)
{
    senssav->HCLK_ENABLE    = senscmu->HCLK_ENABLE;
    senssav->PCLK_ENABLE    = senscmu->PCLK_ENABLE;
    senssav->OCLK_ENABLE    = senscmu->OCLK_ENABLE;
    senssav->HCLK_MODE      = senscmu->HCLK_MODE;
    senssav->PCLK_MODE      = senscmu->PCLK_MODE;
    senssav->OCLK_MODE      = senscmu->OCLK_MODE;
    senssav->REG_RAM_CFG0   = senscmu->REG_RAM_CFG0;
    senssav->HRESET_CLR     = senscmu->HRESET_CLR;
    senssav->PRESET_CLR     = senscmu->PRESET_CLR;
    senssav->ORESET_CLR     = senscmu->ORESET_CLR;
    senssav->TIMER0_CLK     = senscmu->TIMER0_CLK;
    senssav->BOOTMODE       = senscmu->BOOTMODE;
    senssav->SENS_TIMER     = senscmu->SENS_TIMER;
    senssav->SLEEP          = senscmu->SLEEP;
    senssav->CLK_OUT        = senscmu->CLK_OUT;
    senssav->SYS_CLK_ENABLE = senscmu->SYS_CLK_ENABLE;
    senssav->ADMA_CH15_REQ  = senscmu->ADMA_CH15_REQ;
    senssav->REG_RAM_CFG1   = senscmu->REG_RAM_CFG1;
    senssav->UART_CLK       = senscmu->UART_CLK;
    senssav->I2C_CLK        = senscmu->I2C_CLK;
    senssav->SENS2MCU_MASK0 = senscmu->SENS2MCU_MASK0;
    senssav->SENS2MCU_MASK1 = senscmu->SENS2MCU_MASK1;
    senssav->WAKEUP_MASK0   = senscmu->WAKEUP_MASK0;
    senssav->WAKEUP_MASK1   = senscmu->WAKEUP_MASK1;
    senssav->WAKEUP_CLK_CFG = senscmu->WAKEUP_CLK_CFG;
    senssav->TIMER1_CLK     = senscmu->TIMER1_CLK;
    senssav->TIMER2_CLK     = senscmu->TIMER2_CLK;
    senssav->SYS_DIV        = senscmu->SYS_DIV;
    senssav->SENS2BT_INTMASK0 = senscmu->SENS2BT_INTMASK0;
    senssav->SENS2BT_INTMASK1 = senscmu->SENS2BT_INTMASK1;
    senssav->ADMA_CH0_4_REQ = senscmu->ADMA_CH0_4_REQ;
    senssav->ADMA_CH5_9_REQ = senscmu->ADMA_CH5_9_REQ;
    senssav->ADMA_CH10_14_REQ = senscmu->ADMA_CH10_14_REQ;
    senssav->MINIMA_CFG     = senscmu->MINIMA_CFG;
    senssav->MISC           = senscmu->MISC;
    senssav->SIMU_RES       = senscmu->SIMU_RES;
}

static void restore_senscmu_regs(const struct SAVED_SENSCMU_REGS_T *senssav)
{
    senscmu->HRESET_SET     = ~senssav->HRESET_CLR;
    senscmu->PRESET_SET     = ~senssav->PRESET_CLR;
    senscmu->ORESET_SET     = ~senssav->ORESET_CLR;
    senscmu->HCLK_DISABLE   = ~senssav->HCLK_ENABLE;
    senscmu->PCLK_DISABLE   = ~senssav->PCLK_ENABLE;
    senscmu->OCLK_DISABLE   = ~senssav->OCLK_ENABLE;

    senscmu->HCLK_ENABLE    = senssav->HCLK_ENABLE;
    senscmu->PCLK_ENABLE    = senssav->PCLK_ENABLE;
    senscmu->OCLK_ENABLE    = senssav->OCLK_ENABLE;
    senscmu->HCLK_MODE      = senssav->HCLK_MODE;
    senscmu->PCLK_MODE      = senssav->PCLK_MODE;
    senscmu->OCLK_MODE      = senssav->OCLK_MODE;
    senscmu->REG_RAM_CFG0   = senssav->REG_RAM_CFG0;
    senscmu->HRESET_CLR     = senssav->HRESET_CLR;
    senscmu->PRESET_CLR     = senssav->PRESET_CLR;
    senscmu->ORESET_CLR     = senssav->ORESET_CLR;
    senscmu->TIMER0_CLK     = senssav->TIMER0_CLK;
    senscmu->BOOTMODE       = senssav->BOOTMODE;
    senscmu->SENS_TIMER     = senssav->SENS_TIMER;
    senscmu->SLEEP          = senssav->SLEEP;
    senscmu->CLK_OUT        = senssav->CLK_OUT;
    //senscmu->SYS_CLK_ENABLE = senssav->SYS_CLK_ENABLE;
    senscmu->ADMA_CH15_REQ  = senssav->ADMA_CH15_REQ;
    senscmu->REG_RAM_CFG1   = senssav->REG_RAM_CFG1;
    senscmu->UART_CLK       = senssav->UART_CLK;
    senscmu->I2C_CLK        = senssav->I2C_CLK;
    senscmu->SENS2MCU_MASK0 = senssav->SENS2MCU_MASK0;
    senscmu->SENS2MCU_MASK1 = senssav->SENS2MCU_MASK1;
    senscmu->WAKEUP_MASK0   = senssav->WAKEUP_MASK0;
    senscmu->WAKEUP_MASK1   = senssav->WAKEUP_MASK1;
    senscmu->WAKEUP_CLK_CFG = senssav->WAKEUP_CLK_CFG;
    senscmu->TIMER1_CLK     = senssav->TIMER1_CLK;
    senscmu->TIMER2_CLK     = senssav->TIMER2_CLK;
    senscmu->SYS_DIV        = senssav->SYS_DIV;
    senscmu->SENS2BT_INTMASK0 = senssav->SENS2BT_INTMASK0;
    senscmu->SENS2BT_INTMASK1 = senssav->SENS2BT_INTMASK1;
    senscmu->ADMA_CH0_4_REQ = senssav->ADMA_CH0_4_REQ;
    senscmu->ADMA_CH5_9_REQ = senssav->ADMA_CH5_9_REQ;
    senscmu->ADMA_CH10_14_REQ = senssav->ADMA_CH10_14_REQ;
    senscmu->MINIMA_CFG     = senssav->MINIMA_CFG;
    senscmu->MISC           = senssav->MISC;
    senscmu->SIMU_RES       = senssav->SIMU_RES;
}

static int hal_cmu_lpu_sleep_pd(void)
{
    uint32_t start;
    uint32_t timeout;
    uint32_t saved_clk_cfg;
    uint32_t cpu_regs[50];
    struct SAVED_SENSCMU_REGS_T senscmu_regs;

    NVIC_PowerDownSleep(cpu_regs, ARRAY_SIZE(cpu_regs));
    save_senscmu_regs(&senscmu_regs);

    saved_clk_cfg = senscmu_regs.SYS_CLK_ENABLE;

    // Switch VAD clock to AON
    // (VAD buffer will not survive power down -- stack and sleep codes cannot be located in VAD buffer)
    senscmu->I2C_CLK |= SENS_CMU_SEL_CODEC_HCLK_IN;

    // Switch system freq to 26M
    senscmu->SYS_CLK_ENABLE = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE;
    senscmu->SYS_CLK_DISABLE = SENS_CMU_SEL_OSC_2_SYS_DISABLE | SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE;
    senscmu->SYS_CLK_DISABLE = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
    senscmu->SYS_CLK_DISABLE = SENS_CMU_RSTN_DIV_SYS_DISABLE;
    // Shutdown system PLL
    if (saved_clk_cfg & SENS_CMU_PU_PLL_ENABLE) {
        senscmu->SYS_CLK_DISABLE = SENS_CMU_EN_PLL_DISABLE;
        senscmu->SYS_CLK_DISABLE = SENS_CMU_PU_PLL_DISABLE;
    }

    // Set power down wakeup bootmode
    aoncmu->SENS_BOOTMODE |= HAL_SW_BOOTMODE_POWER_DOWN_WAKEUP;

    hal_sleep_core_power_down();

    while ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_26M) == 0);

    // Switch system freq to 26M
    senscmu->SYS_CLK_ENABLE = SENS_CMU_SEL_SLOW_SYS_ENABLE;
    senscmu->SYS_CLK_DISABLE = SENS_CMU_SEL_OSC_2_SYS_DISABLE;

    hal_sys_timer_wakeup();

    // System freq is 26M now and will be restored later
    if (saved_clk_cfg & SENS_CMU_PU_PLL_ENABLE) {
        senscmu->SYS_CLK_ENABLE = SENS_CMU_PU_PLL_ENABLE;
        start = hal_sys_timer_get();
        timeout = HAL_CMU_PLL_LOCKED_TIMEOUT;
        while ((hal_sys_timer_get() - start) < timeout);
        senscmu->SYS_CLK_ENABLE = SENS_CMU_EN_PLL_ENABLE;
    }

    // Clear power down wakeup bootmode
    aoncmu->SENS_BOOTMODE &= ~HAL_SW_BOOTMODE_POWER_DOWN_WAKEUP;

    // Restore system freq
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg & (SENS_CMU_RSTN_DIV_SYS_ENABLE);
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg & (SENS_CMU_BYPASS_DIV_SYS_ENABLE);
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg;
    senscmu->SYS_CLK_DISABLE = ~saved_clk_cfg;

    // Switch VAD clock to SENS
    senscmu->I2C_CLK &= ~SENS_CMU_SEL_CODEC_HCLK_IN;

    restore_senscmu_regs(&senscmu_regs);
    NVIC_PowerDownWakeup(cpu_regs, ARRAY_SIZE(cpu_regs));

    // TODO:
    // 1) Register pm notif handler for all hardware modules, e.g., sdmmc
    // 2) Recover system timer in rt_suspend() and rt_resume()
    // 3) Dynamically select 32K sleep or power down sleep

    return 0;
}

#endif

static int hal_cmu_lpu_sleep_normal(enum HAL_CMU_LPU_SLEEP_MODE_T mode)
{
    uint32_t start;
    uint32_t timeout;
    uint32_t saved_clk_cfg;
    uint32_t wakeup_cfg;
    uint32_t i2c_clk_cfg;

    saved_clk_cfg = senscmu->SYS_CLK_ENABLE;

    if (mode == HAL_CMU_LPU_SLEEP_MODE_CHIP) {
        wakeup_cfg = senscmu->WAKEUP_CLK_CFG;
    } else {
        wakeup_cfg = 0;
    }

    // Setup wakeup mask
    senscmu->WAKEUP_MASK0 = NVIC->ISER[0];
    senscmu->WAKEUP_MASK1 = NVIC->ISER[1];

    if (wakeup_cfg & SENS_CMU_LPU_AUTO_SWITCH26) {
        // Enable auto memory retention
        senscmu->SLEEP = (senscmu->SLEEP & ~SENS_CMU_MANUAL_RAM_RETN) |
            SENS_CMU_DEEPSLEEP_EN | SENS_CMU_DEEPSLEEP_ROMRAM_EN | SENS_CMU_DEEPSLEEP_START;
    } else {
        // Disable auto memory retention
        senscmu->SLEEP = (senscmu->SLEEP & ~SENS_CMU_DEEPSLEEP_ROMRAM_EN) |
            SENS_CMU_DEEPSLEEP_EN | SENS_CMU_MANUAL_RAM_RETN | SENS_CMU_DEEPSLEEP_START;
    }

    if (wakeup_cfg & SENS_CMU_LPU_AUTO_SWITCHPLL) {
        // Do nothing
        // Hardware will switch system freq to 32K and shutdown PLLs automatically
    } else {
        // Switch system freq to 26M
        senscmu->SYS_CLK_ENABLE = SENS_CMU_SEL_SLOW_SYS_ENABLE | SENS_CMU_SEL_OSCX2_SYS_ENABLE;
        senscmu->SYS_CLK_DISABLE = SENS_CMU_SEL_OSC_2_SYS_DISABLE | SENS_CMU_SEL_FAST_SYS_DISABLE | SENS_CMU_SEL_PLL_SYS_DISABLE;
        senscmu->SYS_CLK_DISABLE = SENS_CMU_BYPASS_DIV_SYS_DISABLE;
        senscmu->SYS_CLK_DISABLE = SENS_CMU_RSTN_DIV_SYS_DISABLE;
        // Shutdown system PLL
        if (saved_clk_cfg & SENS_CMU_PU_PLL_ENABLE) {
            senscmu->SYS_CLK_DISABLE = SENS_CMU_EN_PLL_DISABLE;
            senscmu->SYS_CLK_DISABLE = SENS_CMU_PU_PLL_DISABLE;
        }
        if (wakeup_cfg & SENS_CMU_LPU_AUTO_SWITCH26) {
            // Do nothing
            // Hardware will switch system/memory/flash freq to 32K automatically
        } else {
            // Switch system freq to 32K
            senscmu->SYS_CLK_DISABLE = SENS_CMU_SEL_OSC_4_SYS_DISABLE | SENS_CMU_SEL_SLOW_SYS_DISABLE;
        }
    }

    if (mode == HAL_CMU_LPU_SLEEP_MODE_CHIP) {
        SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
    } else {
        SCB->SCR = 0;
    }

    i2c_clk_cfg = senscmu->I2C_CLK;
#ifndef VOICE_DETECTOR_EN
    senscmu->I2C_CLK |= SENS_CMU_SEL_CODEC_CLK_OSC;
#endif
    __DSB();
    // Switch VAD clock to AON
    // (Avoid using VAD buffer from now on. If stack or codes are in VAD buffer, they should not be accessed either)
    senscmu->I2C_CLK |= SENS_CMU_SEL_CODEC_HCLK_IN;
    __DSB();
    __NOP();

    __DSB();
    __WFI();

    // Switch VAD clock to SENS
    // (Wait until the clock swith finishes)
    senscmu->I2C_CLK &= ~SENS_CMU_SEL_CODEC_HCLK_IN;
    __DSB();
    __NOP();
    __NOP();
    __NOP();
    senscmu->I2C_CLK = i2c_clk_cfg;

    if (wakeup_cfg & SENS_CMU_LPU_AUTO_SWITCHPLL) {
        start = hal_sys_timer_get();
        timeout = HAL_CMU_26M_READY_TIMEOUT + HAL_CMU_PLL_LOCKED_TIMEOUT + HAL_CMU_LPU_EXTRA_TIMEOUT;
        while ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_PLL) == 0 &&
            (hal_sys_timer_get() - start) < timeout);
        // !!! CAUTION !!!
        // Hardware will switch system freq to PLL divider and enable PLLs automatically
    } else {
        // Wait for 26M ready
        if (wakeup_cfg & SENS_CMU_LPU_AUTO_SWITCH26) {
            start = hal_sys_timer_get();
            timeout = HAL_CMU_26M_READY_TIMEOUT + HAL_CMU_LPU_EXTRA_TIMEOUT;
            while ((senscmu->WAKEUP_CLK_CFG & SENS_CMU_LPU_STATUS_26M) == 0 &&
                (hal_sys_timer_get() - start) < timeout);
            // Hardware will switch system/memory/flash freq to 26M automatically
        } else {
            if (mode == HAL_CMU_LPU_SLEEP_MODE_CHIP) {
                timeout = HAL_CMU_26M_READY_TIMEOUT;
                hal_sys_timer_delay(timeout);
            }
            // Switch system freq to 26M
            senscmu->SYS_CLK_ENABLE = SENS_CMU_SEL_SLOW_SYS_ENABLE;
            senscmu->SYS_CLK_DISABLE = SENS_CMU_SEL_OSC_2_SYS_DISABLE;
        }
        // System freq is 26M now and will be restored later
        if (saved_clk_cfg & SENS_CMU_PU_PLL_ENABLE) {
            senscmu->SYS_CLK_ENABLE = SENS_CMU_PU_PLL_ENABLE;
            start = hal_sys_timer_get();
            timeout = HAL_CMU_PLL_LOCKED_TIMEOUT;
            while ((hal_sys_timer_get() - start) < timeout);
            senscmu->SYS_CLK_ENABLE = SENS_CMU_EN_PLL_ENABLE;
        }
    }

    // Restore system/memory/flash freq
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg & (SENS_CMU_RSTN_DIV_SYS_ENABLE);
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg & (SENS_CMU_BYPASS_DIV_SYS_ENABLE);
    senscmu->SYS_CLK_ENABLE = saved_clk_cfg;
    senscmu->SYS_CLK_DISABLE = ~saved_clk_cfg;

    if (mode == HAL_CMU_LPU_SLEEP_MODE_CHIP) {
        hal_sys_timer_delay_us(2);
    }

    return 0;
}

int hal_cmu_lpu_sleep(enum HAL_CMU_LPU_SLEEP_MODE_T mode)
{
#ifdef CORE_SLEEP_POWER_DOWN
    if (mode == HAL_CMU_LPU_SLEEP_MODE_POWER_DOWN) {
        return hal_cmu_lpu_sleep_pd();
    }
#endif
    return hal_cmu_lpu_sleep_normal(mode);
}

void hal_cmu_bt_clock_enable(void)
{
    aoncmu->TOP_CLK_ENABLE = AON_CMU_EN_CLK_32K_BT_ENABLE;
    aocmu_reg_update_wait();
}

void hal_cmu_bt_clock_disable(void)
{
    aoncmu->TOP_CLK_DISABLE = AON_CMU_EN_CLK_32K_BT_DISABLE;
}

void hal_cmu_bt_reset_set(void)
{
    aoncmu->GBL_RESET_SET = AON_CMU_SOFT_RSTN_BT_SET | AON_CMU_SOFT_RSTN_BTCPU_SET;
}

void hal_cmu_bt_reset_clear(void)
{
    aoncmu->GBL_RESET_CLR = AON_CMU_SOFT_RSTN_BT_CLR | AON_CMU_SOFT_RSTN_BTCPU_CLR;
    aocmu_reg_update_wait();
}

void hal_cmu_bt_module_init(void)
{
    //btcmu->CLK_MODE = 0;
}

uint32_t hal_cmu_get_aon_chip_id(void)
{
    return aoncmu->CHIP_ID;
}

uint32_t hal_cmu_get_aon_revision_id(void)
{
    return GET_BITFIELD(aoncmu->CHIP_ID, AON_CMU_REVISION_ID);
}

void hal_senscmu_set_wakeup_vector(uint32_t vector)
{
    aoncmu->SENS_VTOR = (aoncmu->SENS_VTOR & ~AON_CMU_VTOR_CORE_SENS_MASK) | (vector & AON_CMU_VTOR_CORE_SENS_MASK);
}

void hal_senscmu_wakeup_check(void)
{
#ifdef CORE_SLEEP_POWER_DOWN
    if (aoncmu->SENS_BOOTMODE & HAL_SW_BOOTMODE_POWER_DOWN_WAKEUP) {
        hal_sleep_core_power_up();
    }
#endif
}

void hal_senscmu_setup(void)
{
    int ret;
    enum HAL_CMU_FREQ_T freq;

    hal_cmu_module_init_state();
    hal_cmu_ema_init();
    hal_cmu_timer0_select_slow();
#ifdef TIMER1_BASE
    hal_cmu_timer1_select_fast();
#endif
    hal_sys_timer_open();

    hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);

    // Set ISPI module freq
    hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    // Open analogif (ISPI)
    ret = hal_analogif_open();
    if (ret) {
        hal_cmu_simu_tag(31);
        do { volatile int i = 0; i++; } while (1);
    }

#ifdef CALIB_SLOW_TIMER
    // Calib slow timer after determining the crystal freq
    hal_sys_timer_calib();
#endif

    // Enable OSC X2/X4 in cmu after enabling their source in hal_chipid_init()
    hal_cmu_osc_x2_enable();
    hal_cmu_osc_x4_enable();

    // Init PLL selection
    hal_cmu_init_pll_selection();

    // Sleep setting
#ifdef NO_LPU_26M
    while (hal_cmu_lpu_init(HAL_CMU_LPU_CLK_NONE) == -1);
#else
    while (hal_cmu_lpu_init(HAL_CMU_LPU_CLK_26M) == -1);
#endif

    // Init system clock
    freq = HAL_CMU_FREQ_26M;
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, freq);
}

#endif

