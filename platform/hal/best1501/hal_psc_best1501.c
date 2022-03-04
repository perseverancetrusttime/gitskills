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
#include "plat_addr_map.h"
#include "cmsis.h"
#include "hal_gpio.h"
#include "hal_location.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include CHIP_SPECIFIC_HDR(reg_psc)

#define PSC_WRITE_ENABLE                    0xCAFE0000

enum PSC_INTR_MASK3_T {
    PSC_INTR_MASK3_GPIO2_15_0   = (0xFFFF << 0),
    PSC_INTR_MASK3_TIMER        = (1 << 16),
    PSC_INTR_MASK3_INTPMU       = (1 << 17),
    PSC_INTR_MASK3_WDT          = (1 << 18),
    PSC_INTR_MASK3_EXTPMU       = (1 << 19),
    PSC_INTR_MASK3_VAD          = (1 << 20),
    PSC_INTR_MASK3_CODEC        = (1 << 21),
    PSC_INTR_MASK3_WAKEUP_BT    = (1 << 22),
    PSC_INTR_MASK3_WAKEUP_SENS  = (1 << 23),
    PSC_INTR_MASK3_WAKEUP_MCU   = (1 << 24),
};

static struct AONPSC_T * const psc = (struct AONPSC_T *)AON_PSC_BASE;

void hal_psc_jtag_select_mcu(void)
{
    psc->REG_064 &= ~PSC_AON_CODEC_RESERVED(1 << 3);
    psc->REG_064 |= PSC_AON_CODEC_RESERVED(1 << 2);
}

void hal_psc_jtag_select_bt(void)
{
    psc->REG_064 &= ~PSC_AON_CODEC_RESERVED(1 << 3);
    psc->REG_064 &= ~PSC_AON_CODEC_RESERVED(1 << 2);
}

void hal_psc_jtag_select_sens(void)
{
    psc->REG_064 |= PSC_AON_CODEC_RESERVED(1 << 3);
}

void BOOT_TEXT_FLASH_LOC hal_psc_init(void)
{
    uint32_t mcu_wake_mask;
    uint32_t sens_wake_mask;

    mcu_wake_mask = 0;
    sens_wake_mask = 0;

#ifdef INTPMU_IRQ_TO_SENS
    sens_wake_mask |= PSC_INTR_MASK3_INTPMU;
#else
    mcu_wake_mask |= PSC_INTR_MASK3_INTPMU;
#endif
#ifdef WDT_IRQ_TO_SENS
    sens_wake_mask |= PSC_INTR_MASK3_WDT;
#else
    mcu_wake_mask |= PSC_INTR_MASK3_WDT;
#endif
#ifdef EXTPMU_IRQ_TO_SENS
    sens_wake_mask |= PSC_INTR_MASK3_EXTPMU;
#else
    mcu_wake_mask |= PSC_INTR_MASK3_EXTPMU;
#endif
#ifdef CODEC_IRQ_TO_SENS
    sens_wake_mask |= PSC_INTR_MASK3_CODEC;
#else
    mcu_wake_mask |= PSC_INTR_MASK3_CODEC;
#endif

    // Setup MCU wakeup mask
    psc->REG_080 = 0;
    psc->REG_084 = 0;
    psc->REG_088 = PSC_INTR_MASK3_TIMER | PSC_INTR_MASK3_WAKEUP_MCU | mcu_wake_mask;

    // Setup BT wakeup mask
    psc->REG_098 = 0;
    psc->REG_09C = 0;
    psc->REG_0A0 = PSC_INTR_MASK3_WAKEUP_BT;

    // Setup SENS wakeup mask
    psc->REG_0B0 = 0;
    psc->REG_0B4 = 0;
    psc->REG_0B8 = PSC_INTR_MASK3_VAD | PSC_INTR_MASK3_WAKEUP_SENS | sens_wake_mask;

    // Setup power down/up timer
    psc->REG_008 = PSC_AON_MCU_POWERDN_TIMER5(4);
    psc->REG_00C = PSC_AON_MCU_POWERUP_TIMER2(4);

#ifndef MCU_I2C_SLAVE
    // Power down sens
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR | PSC_AON_SENSCPU_PSW_EN_REG |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR | PSC_AON_SENSCPU_RESETN_ASSERT_REG |
        PSC_AON_SENSCPU_ISO_EN_DR | PSC_AON_SENSCPU_ISO_EN_REG |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR | PSC_AON_SENS_MEM_PSW_EN_REG |
        PSC_AON_SENS_PSW_EN_DR | PSC_AON_SENS_PSW_EN_REG |
        PSC_AON_SENS_RESETN_ASSERT_DR | PSC_AON_SENS_RESETN_ASSERT_REG |
        PSC_AON_SENS_ISO_EN_DR | PSC_AON_SENS_ISO_EN_REG |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
#endif

#if defined(CORE_SLEEP_POWER_DOWN) && defined(SMALL_RET_RAM)
    psc->REG_07C = PSC_WRITE_ENABLE |
        PSC_AON_SRAM4_ISO_EN_REG | PSC_AON_SRAM5_ISO_EN_REG |
        PSC_AON_SRAM4_ISO_EN_DR | PSC_AON_SRAM5_ISO_EN_DR;
    psc->REG_0FC = PSC_WRITE_ENABLE |
        PSC_AON_SRAM4_PE_PSW_EN_REG | PSC_AON_SRAM5_PE_PSW_EN_REG |
        PSC_AON_SRAM4_CE_PSW_EN_REG | PSC_AON_SRAM5_CE_PSW_EN_REG |
        PSC_AON_SRAM4_PSW_EN_DR | PSC_AON_SRAM5_PSW_EN_DR;
#endif
}

uint32_t hal_psc_get_power_loop_cycle_cnt(void)
{
    return 6 + 14 + 14; // 1.5 + 4.5 + 14 + 14
}

void SRAM_TEXT_LOC hal_psc_mcu_auto_power_down(void)
{
    psc->REG_018 = PSC_WRITE_ENABLE | 0;
    psc->REG_000 = PSC_WRITE_ENABLE | PSC_AON_MCU_PG_AUTO_EN;
    psc->REG_010 = PSC_WRITE_ENABLE | PSC_AON_MCU_POWERDN_START;
}

void SRAM_TEXT_LOC hal_psc_mcu_auto_power_up(void)
{
    psc->REG_014 = PSC_WRITE_ENABLE | PSC_AON_MCU_POWERUP_START;
}

void BOOT_TEXT_FLASH_LOC hal_psc_codec_enable(void)
{
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR |
        PSC_AON_CODEC_PSW_EN_DR | PSC_AON_CODEC_PSW_EN_REG |
        PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_RESETN_ASSERT_REG |
        PSC_AON_CODEC_ISO_EN_DR | PSC_AON_CODEC_ISO_EN_REG |
        PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR |
        PSC_AON_CODEC_PSW_EN_DR |
        PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_RESETN_ASSERT_REG |
        PSC_AON_CODEC_ISO_EN_DR | PSC_AON_CODEC_ISO_EN_REG |
        PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR |
        PSC_AON_CODEC_PSW_EN_DR |
        PSC_AON_CODEC_RESETN_ASSERT_DR |
        PSC_AON_CODEC_ISO_EN_DR | PSC_AON_CODEC_ISO_EN_REG |
        PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR |
        PSC_AON_CODEC_PSW_EN_DR |
        PSC_AON_CODEC_RESETN_ASSERT_DR |
        PSC_AON_CODEC_ISO_EN_DR |
        PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR |
        PSC_AON_CODEC_PSW_EN_DR |
        PSC_AON_CODEC_RESETN_ASSERT_DR |
        PSC_AON_CODEC_ISO_EN_DR |
        PSC_AON_CODEC_CLK_STOP_DR;
}

void BOOT_TEXT_FLASH_LOC hal_psc_codec_disable(void)
{
    psc->REG_078 = PSC_WRITE_ENABLE |
        PSC_AON_CODEC_MEM_PSW_EN_DR | PSC_AON_CODEC_MEM_PSW_EN_REG |
        PSC_AON_CODEC_PSW_EN_DR | PSC_AON_CODEC_PSW_EN_REG |
        PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_RESETN_ASSERT_REG |
        PSC_AON_CODEC_ISO_EN_DR | PSC_AON_CODEC_ISO_EN_REG |
        PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
}

void BOOT_TEXT_FLASH_LOC hal_psc_bt_enable(void)
{
    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR |
        PSC_AON_BT_PSW_EN_DR | PSC_AON_BT_PSW_EN_REG |
        PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_RESETN_ASSERT_REG |
        PSC_AON_BT_ISO_EN_DR | PSC_AON_BT_ISO_EN_REG |
        PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR |
        PSC_AON_BT_PSW_EN_DR |
        PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_RESETN_ASSERT_REG |
        PSC_AON_BT_ISO_EN_DR | PSC_AON_BT_ISO_EN_REG |
        PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR |
        PSC_AON_BT_PSW_EN_DR |
        PSC_AON_BT_RESETN_ASSERT_DR |
        PSC_AON_BT_ISO_EN_DR | PSC_AON_BT_ISO_EN_REG |
        PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR |
        PSC_AON_BT_PSW_EN_DR |
        PSC_AON_BT_RESETN_ASSERT_DR |
        PSC_AON_BT_ISO_EN_DR |
        PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR |
        PSC_AON_BT_PSW_EN_DR |
        PSC_AON_BT_RESETN_ASSERT_DR |
        PSC_AON_BT_ISO_EN_DR |
        PSC_AON_BT_CLK_STOP_DR;

#ifdef JTAG_BT
    hal_psc_jtag_select_bt();
#endif
}

void BOOT_TEXT_FLASH_LOC hal_psc_bt_disable(void)
{
#ifdef JTAG_BT
    hal_psc_jtag_select_mcu();
#endif

    psc->REG_038 = PSC_WRITE_ENABLE |
        PSC_AON_BT_MEM_PSW_EN_DR | PSC_AON_BT_MEM_PSW_EN_REG |
        PSC_AON_BT_PSW_EN_DR | PSC_AON_BT_PSW_EN_REG |
        PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_RESETN_ASSERT_REG |
        PSC_AON_BT_ISO_EN_DR | PSC_AON_BT_ISO_EN_REG |
        PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
}

void hal_psc_bt_enable_auto_power_down(void)
{
//#ifdef BT_SLEEP_POWER_DOWN
    psc->REG_020 = PSC_WRITE_ENABLE | PSC_AON_BT_PG_AUTO_EN | PSC_AON_BT_PG_HW_EN;
    psc->REG_038 = PSC_WRITE_ENABLE | 0;

    psc->REG_024 |= PSC_AON_BT_SLEEP_NO_WFI;
//#endif
}

void BOOT_TEXT_FLASH_LOC hal_psc_sens_enable(void)
{
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR |
        PSC_AON_SENS_PSW_EN_DR | PSC_AON_SENS_PSW_EN_REG |
        PSC_AON_SENS_RESETN_ASSERT_DR | PSC_AON_SENS_RESETN_ASSERT_REG |
        PSC_AON_SENS_ISO_EN_DR | PSC_AON_SENS_ISO_EN_REG |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR |
        PSC_AON_SENS_PSW_EN_DR |
        PSC_AON_SENS_RESETN_ASSERT_DR | PSC_AON_SENS_RESETN_ASSERT_REG |
        PSC_AON_SENS_ISO_EN_DR | PSC_AON_SENS_ISO_EN_REG |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR |
        PSC_AON_SENS_PSW_EN_DR |
        PSC_AON_SENS_RESETN_ASSERT_DR |
        PSC_AON_SENS_ISO_EN_DR | PSC_AON_SENS_ISO_EN_REG |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR |
        PSC_AON_SENS_PSW_EN_DR |
        PSC_AON_SENS_RESETN_ASSERT_DR |
        PSC_AON_SENS_ISO_EN_DR |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR |
        PSC_AON_SENS_PSW_EN_DR |
        PSC_AON_SENS_RESETN_ASSERT_DR |
        PSC_AON_SENS_ISO_EN_DR |
        PSC_AON_SENS_CLK_STOP_DR;

    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR | PSC_AON_SENSCPU_PSW_EN_REG |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR | PSC_AON_SENSCPU_RESETN_ASSERT_REG |
        PSC_AON_SENSCPU_ISO_EN_DR | PSC_AON_SENSCPU_ISO_EN_REG |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR | PSC_AON_SENSCPU_RESETN_ASSERT_REG |
        PSC_AON_SENSCPU_ISO_EN_DR | PSC_AON_SENSCPU_ISO_EN_REG |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    hal_sys_timer_delay(MS_TO_TICKS(1));
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR |
        PSC_AON_SENSCPU_ISO_EN_DR | PSC_AON_SENSCPU_ISO_EN_REG |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR |
        PSC_AON_SENSCPU_ISO_EN_DR |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR |
        PSC_AON_SENSCPU_ISO_EN_DR |
        PSC_AON_SENSCPU_CLK_STOP_DR;
}

void BOOT_TEXT_FLASH_LOC hal_psc_sens_disable(void)
{
    psc->REG_0F8 = PSC_WRITE_ENABLE |
        PSC_AON_SENSCPU_PSW_EN_DR | PSC_AON_SENSCPU_PSW_EN_REG |
        PSC_AON_SENSCPU_RESETN_ASSERT_DR | PSC_AON_SENSCPU_RESETN_ASSERT_REG |
        PSC_AON_SENSCPU_ISO_EN_DR | PSC_AON_SENSCPU_ISO_EN_REG |
        PSC_AON_SENSCPU_CLK_STOP_DR | PSC_AON_SENSCPU_CLK_STOP_REG;
    psc->REG_058 = PSC_WRITE_ENABLE |
        PSC_AON_SENS_MEM_PSW_EN_DR | PSC_AON_SENS_MEM_PSW_EN_REG |
        PSC_AON_SENS_PSW_EN_DR | PSC_AON_SENS_PSW_EN_REG |
        PSC_AON_SENS_RESETN_ASSERT_DR | PSC_AON_SENS_RESETN_ASSERT_REG |
        PSC_AON_SENS_ISO_EN_DR | PSC_AON_SENS_ISO_EN_REG |
        PSC_AON_SENS_CLK_STOP_DR | PSC_AON_SENS_CLK_STOP_REG;
}

void SRAM_TEXT_LOC hal_psc_sens_auto_power_down(void)
{
    psc->REG_058 = PSC_WRITE_ENABLE | 0;
    psc->REG_040 = PSC_WRITE_ENABLE | PSC_AON_SENS_PG_AUTO_EN | PSC_AON_SENS_PG_HW_EN;

    psc->REG_0F8 = PSC_WRITE_ENABLE | 0;
    psc->REG_0E8 = PSC_WRITE_ENABLE | PSC_AON_SENSCPU_PG_AUTO_EN;
    psc->REG_0F0 = PSC_WRITE_ENABLE | PSC_AON_SENSCPU_POWERDN_START;
}

void SRAM_TEXT_LOC hal_psc_dslp_force_on_bt_enable(void)
{
}

void SRAM_TEXT_LOC hal_psc_dslp_force_on_bt_disable(void)
{
}

void SRAM_TEXT_LOC hal_psc_core_auto_power_down(void)
{
#ifdef CHIP_SUBSYS_SENS
    hal_psc_sens_auto_power_down();
#else
    hal_psc_mcu_auto_power_down();
#endif
}

void hal_psc_mcu_gpio_irq_enable(enum HAL_GPIO_PIN_T pin)
{
    if (pin < HAL_GPIO_PIN_P4_0) {
        psc->REG_080 |= (1 << pin);
    } else if (pin < HAL_GPIO_PIN_P8_0) {
        psc->REG_084 |= (1 << (pin - HAL_GPIO_PIN_P4_0));
    } else if (pin < HAL_GPIO_PIN_NUM) {
        psc->REG_088 |= (1 << (pin - HAL_GPIO_PIN_P8_0));
    }
}

void hal_psc_mcu_gpio_irq_disable(enum HAL_GPIO_PIN_T pin)
{
    if (pin < HAL_GPIO_PIN_P4_0) {
        psc->REG_080 &= ~(1 << pin);
    } else if (pin < HAL_GPIO_PIN_P8_0) {
        psc->REG_084 &= ~(1 << (pin - HAL_GPIO_PIN_P4_0));
    } else if (pin < HAL_GPIO_PIN_NUM) {
        psc->REG_088 &= ~(1 << (pin - HAL_GPIO_PIN_P8_0));
    }
}

uint32_t hal_psc_mcu_gpio_irq_get_status(uint32_t *status, uint32_t cnt)
{
    volatile uint32_t * const irq_status[] = {
        &psc->REG_08C,
        &psc->REG_090,
        &psc->REG_094,
    };
    const uint32_t irq_tab_sz = ARRAY_SIZE(irq_status);
    uint32_t i;

    if (cnt > irq_tab_sz) {
        cnt = irq_tab_sz;
    }
    for (i = 0; i < cnt; i++) {
        status[i] = *irq_status[i];
        if (i == (irq_tab_sz - 1))
            status[i] &= PSC_INTR_MASK3_GPIO2_15_0;
    }

    return cnt;
}

void hal_psc_sens_gpio_irq_enable(enum HAL_GPIO_PIN_T pin)
{
    if (pin < HAL_GPIO_PIN_P4_0) {
        psc->REG_0B0 |= (1 << pin);
    } else if (pin < HAL_GPIO_PIN_P8_0) {
        psc->REG_0B4 |= (1 << (pin - HAL_GPIO_PIN_P4_0));
    } else if (pin < HAL_GPIO_PIN_NUM) {
        psc->REG_0B8 |= (1 << (pin - HAL_GPIO_PIN_P8_0));
    }
}

void hal_psc_sens_gpio_irq_disable(enum HAL_GPIO_PIN_T pin)
{
    if (pin < HAL_GPIO_PIN_P4_0) {
        psc->REG_0B0 &= ~(1 << pin);
    } else if (pin < HAL_GPIO_PIN_P8_0) {
        psc->REG_0B4 &= ~(1 << (pin - HAL_GPIO_PIN_P4_0));
    } else if (pin < HAL_GPIO_PIN_NUM) {
        psc->REG_0B8 &= ~(1 << (pin - HAL_GPIO_PIN_P8_0));
    }
}

uint32_t hal_psc_sens_gpio_irq_get_status(uint32_t *status, uint32_t cnt)
{
    volatile uint32_t * const irq_status[] = {
        &psc->REG_0BC,
        &psc->REG_0C0,
        &psc->REG_0C4,
    };
    const uint32_t irq_tab_sz = ARRAY_SIZE(irq_status);
    uint32_t i;

    if (cnt > irq_tab_sz) {
        cnt = irq_tab_sz;
    }
    for (i = 0; i < cnt; i++) {
        status[i] = *irq_status[i];
        if (i == (irq_tab_sz - 1))
            status[i] &= PSC_INTR_MASK3_GPIO2_15_0;
    }
    return cnt;
}

void hal_psc_gpio_irq_enable(enum HAL_GPIO_PIN_T pin)
{
#ifdef CHIP_SUBSYS_SENS
    hal_psc_sens_gpio_irq_enable(pin);
#else
    hal_psc_mcu_gpio_irq_enable(pin);
#endif
}

void hal_psc_gpio_irq_disable(enum HAL_GPIO_PIN_T pin)
{
#ifdef CHIP_SUBSYS_SENS
    hal_psc_sens_gpio_irq_disable(pin);
#else
    hal_psc_mcu_gpio_irq_disable(pin);
#endif
}

uint32_t hal_psc_gpio_irq_get_status(uint32_t *status, uint32_t cnt)
{
#ifdef CHIP_SUBSYS_SENS
    return hal_psc_sens_gpio_irq_get_status(status, cnt);
#else
    return hal_psc_mcu_gpio_irq_get_status(status, cnt);
#endif
}


