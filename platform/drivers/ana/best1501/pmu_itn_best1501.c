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
#include "pmu.h"
#include CHIP_SPECIFIC_HDR(pmu_itn)
#include CHIP_SPECIFIC_HDR(reg_pmu_itn)
#include "analog.h"
#include "cmsis.h"
#include "cmsis_nvic.h"
#include "hal_aud.h"
#include "hal_cache.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "tgt_hardware.h"
#include "usbphy.h"

#define PMU_VCORE_STABLE_TIME_US            10

#define PMU_VDIG_1_05V                      PMU_ITN_VDIG_1_05V
#define PMU_VDIG_1_025V                     PMU_ITN_VDIG_1_025V
#define PMU_VDIG_1_0V                       PMU_ITN_VDIG_1_0V
#define PMU_VDIG_0_975V                     PMU_ITN_VDIG_0_975V
#define PMU_VDIG_0_95V                      PMU_ITN_VDIG_0_95V
#define PMU_VDIG_0_925V                     PMU_ITN_VDIG_0_925V
#define PMU_VDIG_0_9V                       PMU_ITN_VDIG_0_9V
#define PMU_VDIG_0_875V                     PMU_ITN_VDIG_0_875V
#define PMU_VDIG_0_85V                      PMU_ITN_VDIG_0_85V
#define PMU_VDIG_0_825V                     PMU_ITN_VDIG_0_825V
#define PMU_VDIG_0_8V                       PMU_ITN_VDIG_0_8V
#define PMU_VDIG_0_775V                     PMU_ITN_VDIG_0_775V
#define PMU_VDIG_0_75V                      PMU_ITN_VDIG_0_75V
#define PMU_VDIG_0_725V                     PMU_ITN_VDIG_0_725V
#define PMU_VDIG_0_7V                       PMU_ITN_VDIG_0_7V
#define PMU_VDIG_0_675V                     PMU_ITN_VDIG_0_675V
#define PMU_VDIG_0_65V                      PMU_ITN_VDIG_0_65V
#define PMU_VDIG_0_625V                     PMU_ITN_VDIG_0_625V
#define PMU_VDIG_0_6V                       PMU_ITN_VDIG_0_6V
#define PMU_VDIG_MAX                        PMU_ITN_VDIG_1_05V

enum PMU_REG_T {
    PMU_REG_METAL_ID            = ITNPMU_REG(0x00),
    PMU_REG_POWER_KEY_CFG       = ITNPMU_REG(0x02),
    PMU_REG_VBG_CFG             = ITNPMU_REG(0x03),
    PMU_REG_DIG1_VOLT           = ITNPMU_REG(0x05),
    PMU_REG_DIG1_CFG            = ITNPMU_REG(0x08),
    PMU_REG_DIG2_CFG            = ITNPMU_REG(0x09),
    PMU_REG_MIC_LDO_EN          = ITNPMU_REG(0x0A),
    PMU_REG_MIC_BIAS_EN         = ITNPMU_REG(0x0B),
    PMU_REG_ITF_POWER_EN        = ITNPMU_REG(0x0E),
    PMU_REG_VCORE_BYPASS        = ITNPMU_REG(0x0D),
    PMU_REG_VANA2VCORE_CFG      = ITNPMU_REG(0x18),
    PMU_REG_MEM_PU_DR           = ITNPMU_REG(0x1B),
    PMU_REG_SLEEP_CFG           = ITNPMU_REG(0x1D),
    PMU_REG_VCORE_VOLT_SEL      = ITNPMU_REG(0x2C),
    PMU_REG_DIG2_VOLT           = ITNPMU_REG(0x2D),
    PMU_REG_EFUSE_CTRL          = ITNPMU_REG(0x46),
    PMU_REG_EFUSE_DATA_HIGH     = ITNPMU_REG(0x4C),
    PMU_REG_EFUSE_DATA_LOW      = ITNPMU_REG(0x4D),

#if 0
    PMU_REG_VTOI_EN             = ITNPMU_REG(0x0C),
    PMU_REG_MEM_PU_CTRL         = ITNPMU_REG(0x19),
    PMU_REG_PWR_SEL             = ITNPMU_REG(0x1A),
    PMU_REG_INT_MASK            = ITNPMU_REG(0x1F),
    PMU_REG_INT_EN              = ITNPMU_REG(0x20),
    PMU_REG_RTC_DIV_1HZ         = ITNPMU_REG(0x22),
    PMU_REG_RTC_LOAD_LOW        = ITNPMU_REG(0x23),
    PMU_REG_RTC_LOAD_HIGH       = ITNPMU_REG(0x24),
    PMU_REG_RTC_MATCH1_LOW      = ITNPMU_REG(0x27),
    PMU_REG_RTC_MATCH1_HIGH     = ITNPMU_REG(0x28),
    PMU_REG_MIC_BIAS_A          = ITNPMU_REG(0x29),
    PMU_REG_MIC_BIAS_B          = ITNPMU_REG(0x2A),
    PMU_REG_MIC_LDO_RES         = ITNPMU_REG(0x30),
    PMU_REG_WDT_TIMER           = ITNPMU_REG(0x36),
    PMU_REG_WDT_CFG             = ITNPMU_REG(0x37),
    PMU_REG_POWER_OFF           = ITNPMU_REG(0x4F),
    PMU_REG_INT_STATUS          = ITNPMU_REG(0x50),
    PMU_REG_INT_MSKED_STATUS    = ITNPMU_REG(0x51),
    PMU_REG_INT_CLR             = ITNPMU_REG(0x51),
    PMU_REG_RTC_VAL_HIGH        = ITNPMU_REG(0x5C),
    PMU_REG_RTC_VAL_LOW         = ITNPMU_REG(0x5D),

    PMU_REG_ANA_60              = ANA_REG(0x60),
    PMU_REG_ANA_75              = ANA_REG(0x75),
    PMU_REG_ANA_26E             = ANA_REG(0x26E),
    PMU_REG_ANA_271             = ANA_REG(0x271),
    PMU_REG_ANA_272             = ANA_REG(0x272),
    PMU_REG_ANA_273             = ANA_REG(0x273),
    PMU_REG_ANA_274             = ANA_REG(0x274),
    PMU_REG_ANA_275             = ANA_REG(0x275),
    PMU_REG_ANA_276             = ANA_REG(0x276),
    PMU_REG_ANA_277             = ANA_REG(0x277),
#endif
};

enum PMU_VCORE_REQ_T {
    PMU_VCORE_FLASH_WRITE_ENABLED   = (1 << 0),
    PMU_VCORE_FLASH_FREQ_HIGH       = (1 << 1),
    PMU_VCORE_PSRAM_FREQ_HIGH       = (1 << 2),
    PMU_VCORE_USB_HS_ENABLED        = (1 << 3),
    PMU_VCORE_RS_FREQ_HIGH          = (1 << 4),
    PMU_VCORE_SYS_FREQ_MEDIUM       = (1 << 5),
    PMU_VCORE_SYS_FREQ_HIGH         = (1 << 6),
};

union BOOT_SETTINGS_T {
    struct {
        unsigned short usb_dld_dis      :1;
        unsigned short uart_dld_en      :1;
        unsigned short uart_trace_en    :1;
        unsigned short pll_dis          :1;
        unsigned short uart_baud_div2   :1;
        unsigned short sec_freq_div2    :1;
        unsigned short crystal_freq     :2;
        unsigned short reserved         :4;
        unsigned short chksum           :4;
    };
    unsigned short reg;
};

enum PMU_MODUAL_T {
    PMU_DIG1,
    PMU_DIG2,
};

#ifdef NO_EXT_PMU

static enum HAL_CHIP_METAL_ID_T BOOT_BSS_LOC pmu_metal_id;

static enum PMU_VCORE_REQ_T BOOT_BSS_LOC pmu_vcore_req;

static PMU_IRQ_UNIFIED_HANDLER_T pmu_irq_hdlrs[PMU_IRQ_TYPE_QTY];

static const uint8_t dig_lp_ldo = PMU_VDIG_0_8V;

static uint16_t wdt_timer;

#if defined(MCU_HIGH_PERFORMANCE_MODE)
static const uint16_t high_perf_freq_mhz =
#if defined(MTEST_ENABLED) && defined(MTEST_CLK_MHZ)
    MTEST_CLK_MHZ;
#else
    300;
#endif
static bool high_perf_on;
#endif

#ifdef RTC_ENABLE
struct PMU_RTC_CTX_T {
    bool enabled;
    bool alarm_set;
    uint32_t alarm_val;
};

static struct PMU_RTC_CTX_T BOOT_BSS_LOC rtc_ctx;

static PMU_RTC_IRQ_HANDLER_T rtc_irq_handler;

static void BOOT_TEXT_SRAM_LOC pmu_rtc_save_context(void)
{
    if (pmu_rtc_enabled()) {
        rtc_ctx.enabled = true;
        if (pmu_rtc_alarm_status_set()) {
            rtc_ctx.alarm_set = true;
            rtc_ctx.alarm_val = pmu_rtc_get_alarm();
        }
    } else {
        rtc_ctx.enabled = false;
    }
}

static void pmu_rtc_restore_context(void)
{
    uint32_t rtc_val;

    if (rtc_ctx.enabled) {
        pmu_rtc_enable();
        if (rtc_ctx.alarm_set) {
            rtc_val = pmu_rtc_get();
            if (rtc_val - rtc_ctx.alarm_val <= 1 || rtc_ctx.alarm_val - rtc_val < 5) {
                rtc_ctx.alarm_val = rtc_val + 5;
            }
            pmu_rtc_set_alarm(rtc_ctx.alarm_val);
        }
    }
}
#endif

uint32_t BOOT_TEXT_FLASH_LOC read_hw_metal_id(void)
{
    uint16_t val;
    uint32_t metal_id;

#ifdef RTC_ENABLE
    // RTC will be restored in pmu_open()
    pmu_rtc_save_context();
#endif

#ifdef __WATCHER_DOG_RESET__
    pmu_wdt_save_context();
#endif

    // Reset PMU (to recover from a possible insane state, e.g., ESD reset)
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));

#ifdef __WATCHER_DOG_RESET__
    pmu_wdt_restore_context();
#else
    pmu_wdt_stop();
#endif

    pmu_rf_ana_init();

    pmu_read(PMU_REG_METAL_ID, &val);
    pmu_metal_id = GET_BITFIELD(val, REVID);

    metal_id = hal_cmu_get_aon_revision_id();

    return metal_id;
}

#endif // NO_EXT_PMU

int BOOT_TEXT_SRAM_LOC pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse)
{
    int ret;
    unsigned short val;
    unsigned short tmp[2];

#ifdef NO_EXT_PMU
    //hal_cmu_pmu_fast_clock_enable();
#endif

    // Enable CLK_EN
    val = REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Enable TURN_ON
    val |= REG_EFUSE_TURN_ON;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Write Address
#ifdef PMU_EFUSE_NO_REDUNDANCY
    val |= REG_EFUSE_ADDRESS(page / 2);
#else
    val |= REG_EFUSE_ADDRESS(page); //redundancy
#endif
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Set Strobe Trigger = 1
    val |= REG_EFUSE_STROBE_TRIGGER;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // set Strobe Trigger = 0
    val &= ~REG_EFUSE_STROBE_TRIGGER;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    // Read Efuse High 16 bits
    ret = pmu_read(PMU_REG_EFUSE_DATA_LOW, &tmp[0]);
    if (ret) {
        goto _exit;
    }

    // Read Efuse Low 16 bits
    ret = pmu_read(PMU_REG_EFUSE_DATA_HIGH, &tmp[1]);
    if (ret) {
        goto _exit;
    }
#ifdef PMU_EFUSE_NO_REDUNDANCY
    *efuse = tmp[page % 2];
#else
    *efuse = (tmp[0] | tmp[1]); //redundancy
#endif

_exit:
    // Disable TURN_ON
    val &= ~(REG_EFUSE_TURN_ON | REG_EFUSE_ADDRESS_MASK);
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);

    // Disable CLK_EN
    val &= ~REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);

#ifdef NO_EXT_PMU
    //hal_cmu_pmu_fast_clock_disable();
#endif

    return ret;
}

#ifdef PMU_LDO_VCORE_CALIB
union LDO_VCORE_COMP_T {
    struct LDO_VCORE_COMP_FIELD_T {
        uint16_t dig2_v: 5;   //vcore0p4: bit[4:0]: 0 ~ 31
        uint16_t dig2_f: 1;   //bit[5]  : 1: negative, 0: positive
        uint16_t dig1_v: 4;   //vcore0p6: bit[9:6]: 0 ~ 15
        uint16_t dig1_f: 1;   //bit[10] : 1: negative, 0: positive
        uint16_t cal_flag:1;  //bit[11] : calib flag: 1:new calib, 0: old calib
        uint16_t dig1_v_append: 1;   //ldo vcore0p6 (expanded to bit4)
        uint16_t reserved:4;
    } f;
    uint16_t v;
};

extern unsigned short pmu_reg_val_add(unsigned short val, int delta, unsigned short max);
extern int pmu_ext_get_efuse(enum PMU_EXT_EFUSE_PAGE_T page, unsigned short *efuse);

static int8_t pmu_ldo_vcore_l_comp = 0;
static int8_t pmu_ldo_vcore_v_comp = 0;
static int8_t pmu_ldo_vcorel_lp_comp = 0;
static int8_t pmu_ldo_vcorev_lp_comp = 0;

void pmu_get_ldo_vcore_calib_value(void)
{
    union LDO_VCORE_COMP_T lv;

    //normal voltage
    pmu_get_efuse(PMU_EFUSE_PAGE_BT_POWER, &lv.v);

    if (lv.f.dig1_f) {
        pmu_ldo_vcore_l_comp = -(int8_t)(lv.f.dig1_v);
    } else {
        pmu_ldo_vcore_l_comp = (int8_t)(lv.f.dig1_v);
    }

    if (lv.f.dig2_f) {
        pmu_ldo_vcore_v_comp = -(int8_t)(lv.f.dig2_v);
    } else {
        pmu_ldo_vcore_v_comp = (int8_t)(lv.f.dig2_v);
    }

    if(0 == lv.f.cal_flag)
    {
        pmu_ldo_vcore_l_comp += 3;
        pmu_ldo_vcore_v_comp += 10;
    }

    //sleep voltage
    pmu_ext_get_efuse(PMU_EXT_EFUSE_PAGE_LDO_VCORE, &lv.v);

    if (lv.f.dig1_f) {
        pmu_ldo_vcorel_lp_comp = -(int8_t)(lv.f.dig1_v | (lv.f.dig1_v_append << 4));
    } else {
        pmu_ldo_vcorel_lp_comp = (int8_t)(lv.f.dig1_v | (lv.f.dig1_v_append << 4));
    }

    if (lv.f.dig2_f) {
        pmu_ldo_vcorev_lp_comp = -(int8_t)(lv.f.dig2_v);
    } else {
        pmu_ldo_vcorev_lp_comp = (int8_t)(lv.f.dig2_v);
    }
    pmu_ldo_vcorev_lp_comp += 1;

#ifdef ITN_PMU_FORCE_LP_MODE
    pmu_ldo_vcore_l_comp = pmu_ldo_vcorel_lp_comp;
    pmu_ldo_vcore_v_comp = pmu_ldo_vcorev_lp_comp;
#endif
}
#endif // PMU_LDO_VCORE_CALIB

void BOOT_TEXT_FLASH_LOC pmu_itn_init(void)
{
    uint16_t val;

    // Reset PMU (to recover from a possible insane state, e.g., ESD reset)
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(US_TO_TICKS(500));

    // Set memory power mode to auto
    pmu_write(PMU_REG_MEM_PU_DR, 0);

    // Power down DPA interface
    pmu_read(PMU_REG_ITF_POWER_EN, &val);
    val = (val & ~PU_INTERFACE_TX) | ISO_EN_INTERFACE_TX;
    pmu_write(PMU_REG_ITF_POWER_EN, val);

    // Allow PMU to sleep when power key is pressed
    pmu_read(PMU_REG_POWER_KEY_CFG, &val);
    val &= ~POWERKEY_WAKEUP_OSC_EN;
    pmu_write(PMU_REG_POWER_KEY_CFG, val);

#ifdef NO_EXT_PMU
    // Set vcore voltage to lowest
    pmu_read(PMU_REG_DIG1_CFG, &val);
    val = (val & ~(LDO_VCORE1_VBIT_DSLEEP_MASK | LDO_VCORE1_VBIT_NORMAL_MASK)) |
        LDO_VCORE1_VBIT_DSLEEP(0) | LDO_VCORE1_VBIT_NORMAL(0);
    pmu_write(PMU_REG_DIG1_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Force LP mode
    pmu_read(PMU_REG_VBG_CFG, &val);
    val = (val & ~BG_VBG_SEL_REG) | BG_VBG_SEL_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    val = (val & ~(PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG)) |
        PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
#else // !NO_EXT_PMU
    // Close MIC LDO 0x0A
    pmu_read(PMU_REG_MIC_LDO_EN, &val);
    val = (val & ~(REG_PU_MIC_LDO_REG)) | REG_PU_MIC_LDO_DR;
    pmu_write(PMU_REG_MIC_LDO_EN, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Close MIC BIAS 0x0B
    pmu_read(PMU_REG_MIC_BIAS_EN, &val);
    val = (val & ~(REG_PU_MIC_BIASA_REG | REG_PU_MIC_BIASB_REG)) | REG_PU_MIC_BIASA_DR | REG_PU_MIC_BIASB_DR;
    pmu_write(PMU_REG_MIC_BIAS_EN, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Close VANA2VCORE
    pmu_read(PMU_REG_VANA2VCORE_CFG, &val);
    val = (val & ~(REG_PU_VANA2VCORE_LDO_REG)) | REG_PU_VANA2VCORE_LDO_DR;
    pmu_write(PMU_REG_VANA2VCORE_CFG, val);

    // Enable logical vcore (nominal 0.6V)
    // Default is bypassed and same as memory vcore (0.9V)
    pmu_itn_logic_dig_init_volt(PMU_ITN_VDIG_0_9V, PMU_ITN_VDIG_0_9V);

    pmu_read(PMU_REG_DIG1_CFG, &val);
    val = (val & ~(REG_PU_VCORE_L_LDO_DR)) | REG_PU_VCORE_L_LDO_REG;
#ifdef ITN_PMU_FORCE_LP_MODE
    val |= LP_EN_VCORE_L_LDO_DSLEEP_EN | LP_EN_VCORE_L_LDO_DR | LP_EN_VCORE_L_LDO_REG | REG_PU_VCORE_L_LDO_DSLEEP_MSK;
#endif
    pmu_write(PMU_REG_DIG1_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    pmu_read(PMU_REG_VCORE_BYPASS, &val);
    val &= ~REG_BYPASS_VCORE_L;
    pmu_write(PMU_REG_VCORE_BYPASS, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Ramp down for stability
    pmu_itn_logic_dig_init_volt(PMU_ITN_VDIG_0_875V, PMU_ITN_VDIG_0_875V);
    pmu_itn_logic_dig_init_volt(PMU_ITN_VDIG_0_85V, PMU_ITN_VDIG_0_85V);
    pmu_itn_logic_dig_init_volt(PMU_ITN_VDIG_0_825V, PMU_ITN_VDIG_0_825V);
    pmu_itn_logic_dig_init_volt(PMU_ITN_VDIG_0_8V, PMU_ITN_VDIG_0_8V);

    // Enable Minima vcore (nominal 0.4V)
    pmu_itn_sens_dig_init_volt(PMU_ITN_VDIG_0_6V, PMU_ITN_VDIG_0_6V);

    pmu_read(PMU_REG_DIG2_CFG, &val);
    val = (val & ~(REG_PU_VCORE_V_LDO_DR)) | REG_PU_VCORE_V_LDO_REG;
#ifdef ITN_PMU_FORCE_LP_MODE
    val |= LP_EN_VCORE_V_LDO_DSLEEP_EN | LP_EN_VCORE_V_LDO_DR | LP_EN_VCORE_V_LDO_REG | REG_PU_VCORE_V_LDO_DSLEEP_MSK;
#endif
    pmu_write(PMU_REG_DIG2_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    pmu_read(PMU_REG_VCORE_BYPASS, &val);
    val &= ~REG_BYPASS_VCORE_V;
    pmu_write(PMU_REG_VCORE_BYPASS, val);

    pmu_read(PMU_REG_VCORE_VOLT_SEL, &val);
    val |= REG_PU_VCORE_V_LDO_DIG_DR;
    pmu_write(PMU_REG_VCORE_VOLT_SEL, val);

#ifdef ITN_PMU_FORCE_LP_MODE
    // Force LP mode
    pmu_read(PMU_REG_VBG_CFG, &val);
    val = (val & ~BG_VBG_SEL_REG) | BG_VBG_SEL_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    val = (val & ~(PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG)) |
        PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
#else
    // Auto sleep control
    pmu_read(PMU_REG_VBG_CFG, &val);
    val = (val & ~BG_VBG_SEL_DR) | BG_VBG_SEL_REG;
    pmu_write(PMU_REG_VBG_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));
#endif
#endif // !NO_EXT_PMU
#ifdef SENSOR_HUB_MINIMA
    pmu_vcorev_dig_enable();
#endif
}

void pmu_vcorev_dig_enable(void)
{
    uint16_t val;

    pmu_read(PMU_REG_VCORE_VOLT_SEL, &val);
    pmu_write(PMU_REG_VCORE_VOLT_SEL, val | REG_VCORE_V_VOLTAGE_SEL_DR);
}

void pmu_vcorev_dig_disable(void)
{
    uint16_t val;

    pmu_read(PMU_REG_VCORE_VOLT_SEL, &val);
    pmu_write(PMU_REG_VCORE_VOLT_SEL, val & (~ REG_VCORE_V_VOLTAGE_SEL_DR));
}

void pmu_itn_sleep_en(unsigned char sleep_en)
{
    unsigned short val;

    pmu_read(PMU_REG_SLEEP_CFG, &val);
    if (sleep_en) {
        val |= SLEEP_ALLOW;
    } else {
        val &= ~SLEEP_ALLOW;
    }
    pmu_write(PMU_REG_SLEEP_CFG, val);
}

static void pmu_module_set_volt(unsigned char module, unsigned short sleep_v,unsigned short normal_v)
{
    unsigned short val;
    unsigned short module_address;

    if (module == PMU_DIG1) {
        module_address = PMU_REG_DIG1_VOLT;
    } else {
        module_address = PMU_REG_DIG2_VOLT;
    }

#ifdef PMU_LDO_VCORE_CALIB
    if (module == PMU_DIG1) {
        normal_v = pmu_reg_val_add(normal_v, pmu_ldo_vcore_l_comp, MAX_LDO_VCORE_L_VBIT_VAL);
        sleep_v = pmu_reg_val_add(sleep_v, pmu_ldo_vcorel_lp_comp, MAX_LDO_VCORE_L_VBIT_VAL);
    } else {
        normal_v = pmu_reg_val_add(normal_v, pmu_ldo_vcore_v_comp, MAX_LDO_VCORE_V_VBIT_VAL);
        sleep_v = pmu_reg_val_add(sleep_v, pmu_ldo_vcorev_lp_comp, MAX_LDO_VCORE_V_VBIT_VAL);
    }
#endif

    pmu_read(module_address, &val);
    val = (val & ~(LDO_VCORE_L_VBIT_DSLEEP_MASK | LDO_VCORE_L_VBIT_NORMAL_MASK)) |
        LDO_VCORE_L_VBIT_DSLEEP(sleep_v) | LDO_VCORE_L_VBIT_NORMAL(normal_v);
    pmu_write(module_address, val);
}

static int pmu_module_get_volt(unsigned char module, unsigned short *sleep_vp,unsigned short *normal_vp)
{
    unsigned short val;
    unsigned short module_address;

    if (module == PMU_DIG1) {
        module_address = PMU_REG_DIG1_VOLT;
    } else {
        module_address = PMU_REG_DIG2_VOLT;
    }

    pmu_read(module_address, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, LDO_VCORE_L_VBIT_NORMAL);
    }
    if (sleep_vp) {
        *sleep_vp = GET_BITFIELD(val, LDO_VCORE_L_VBIT_DSLEEP);
    }

#ifdef PMU_LDO_VCORE_CALIB
    if (module == PMU_DIG1) {
        if (normal_vp){
            *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_ldo_vcore_l_comp, MAX_LDO_VCORE_L_VBIT_VAL);
        }
        if (sleep_vp){
            *sleep_vp = pmu_reg_val_add(*sleep_vp, -pmu_ldo_vcorel_lp_comp, MAX_LDO_VCORE_L_VBIT_VAL);
        }
    } else {
        if (normal_vp){
            *normal_vp = pmu_reg_val_add(*normal_vp, -pmu_ldo_vcore_v_comp, MAX_LDO_VCORE_V_VBIT_VAL);
        }
        if (sleep_vp){
            *sleep_vp = pmu_reg_val_add(*sleep_vp, -pmu_ldo_vcorev_lp_comp, MAX_LDO_VCORE_V_VBIT_VAL);
        }
    }
#endif

    return 0;
}

static void pmu_module_ramp_volt(unsigned char module, unsigned short sleep_v, unsigned short normal_v)
{
    uint16_t old_normal_v;
    uint16_t old_sleep_v;

    pmu_module_get_volt(module, &old_sleep_v, &old_normal_v);

    if (old_normal_v < normal_v) {
        while (old_normal_v++ < normal_v) {
            pmu_module_set_volt(module, sleep_v, old_normal_v);
        }
    } else if (old_normal_v != normal_v || old_sleep_v != sleep_v) {
        pmu_module_set_volt(module, sleep_v, normal_v);
    }
}

void pmu_itn_logic_dig_set_volt(unsigned short normal, unsigned short sleep)
{
    pmu_module_ramp_volt(PMU_DIG1, sleep, normal);
}

void pmu_itn_sens_dig_set_volt(unsigned short normal, unsigned short sleep)
{
    pmu_module_ramp_volt(PMU_DIG2, sleep, normal);
}

void BOOT_TEXT_FLASH_LOC pmu_itn_logic_dig_init_volt(unsigned short normal, unsigned short sleep)
{
    uint16_t val;

    val = LDO_VCORE_L_VBIT_DSLEEP(sleep) | LDO_VCORE_L_VBIT_NORMAL(normal);
    pmu_write(PMU_REG_DIG1_VOLT, val);
}

void BOOT_TEXT_FLASH_LOC pmu_itn_sens_dig_init_volt(unsigned short normal, unsigned short sleep)
{
    uint16_t val;

    val = LDO_VCORE_L_VBIT_DSLEEP(sleep) | LDO_VCORE_L_VBIT_NORMAL(normal);
    pmu_write(PMU_REG_DIG2_VOLT, val);
}

#ifdef NO_EXT_PMU

static void pmu_sys_ctrl(bool shutdown)
{
    uint16_t val;
    uint32_t lock = int_lock();

    PMU_INFO_TRACE_IMM(0, "Start pmu %s", shutdown ? "shutdown" : "reboot");

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    // Default vcore might not be high enough to support high performance mode
    pmu_high_performance_mode_enable(false);
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);
#endif
#endif

#ifdef RTC_ENABLE
    pmu_rtc_save_context();
#endif

    // Reset PMU
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(4);

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#endif

    if (shutdown) {
        // Power off
        pmu_read(PMU_REG_POWER_OFF,&val);
        val |= SOFT_POWER_OFF;
        for (int i = 0; i < 100; i++) {
            pmu_write(PMU_REG_POWER_OFF,val);
            hal_sys_timer_delay(MS_TO_TICKS(5));
        }

        hal_sys_timer_delay(MS_TO_TICKS(50));

        //can't reach here
        PMU_INFO_TRACE_IMM(0, "\nError: pmu shutdown failed!\n");
        hal_sys_timer_delay(MS_TO_TICKS(5));
    } else {
#if defined(PMU_FULL_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
        // CAUTION:
        // 1) Never reset RF because system or flash might be using X2/X4, which are off by default
        // 2) Never reset RF/ANA because system or flash might be using PLL, and the reset might cause clock glitch
        // TODO:
        // Restore BBPLL settings in RF
#endif
    }

    hal_cmu_sys_reboot();

    int_unlock(lock);
}

void pmu_shutdown(void)
{
    pmu_sys_ctrl(true);
}

void pmu_reboot(void)
{
    pmu_sys_ctrl(false);
}

void pmu_module_config(enum PMU_MODUAL_T module,unsigned short is_manual,unsigned short ldo_on,unsigned short lp_mode,unsigned short dpmode)
{
    unsigned short val;
    unsigned short module_address;

    if (module == PMU_DIG1) {
        module_address = PMU_REG_DIG1_CFG;
    } else {
        module_address = PMU_REG_DIG2_CFG;
    }

    pmu_read(module_address, &val);
    if (is_manual) {
        val |= REG_PU_VCORE1_LDO_DR;
    } else {
        val &= ~REG_PU_VCORE1_LDO_DR;
    }
    if (ldo_on) {
        val |= REG_PU_VCORE1_LDO_REG;
    } else {
        val &= ~REG_PU_VCORE1_LDO_REG;
    }
    if (lp_mode) {
        val &= ~LP_EN_VCORE1_LDO_DR;
    } else {
        val = (val & ~LP_EN_VCORE1_LDO_REG) | LP_EN_VCORE1_LDO_DR;
    }
    if (dpmode) {
        val |= LP_EN_VCORE1_LDO_DSLEEP_EN;
    } else {
        val &= ~LP_EN_VCORE1_LDO_DSLEEP_EN;
    }
    pmu_write(module_address, val);
}

static void BOOT_TEXT_SRAM_LOC pmu_dig_get_target_volt(uint16_t *ldo)
{
    uint16_t ldo_volt;

    if (0) {
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    } else if (pmu_vcore_req & (PMU_VCORE_SYS_FREQ_HIGH)) {
        if (high_perf_freq_mhz <= 300) {
            ldo_volt = PMU_VDIG_1_05V;
        } else if (high_perf_freq_mhz <= 350) {
            ldo_volt = PMU_VDIG_1_05V;
        }
#endif
    } else if (pmu_vcore_req & (PMU_VCORE_USB_HS_ENABLED | PMU_VCORE_RS_FREQ_HIGH | PMU_VCORE_SYS_FREQ_MEDIUM)) {
        ldo_volt = PMU_VDIG_0_9V;
    } else if (pmu_vcore_req & (PMU_VCORE_FLASH_FREQ_HIGH | PMU_VCORE_PSRAM_FREQ_HIGH |
            PMU_VCORE_FLASH_WRITE_ENABLED)) {
        ldo_volt = PMU_VDIG_0_8V;
    } else {
        // Common cases
        ldo_volt = PMU_VDIG_0_8V;
    }

#if defined(MTEST_ENABLED) && defined(MTEST_VOLT)
    ldo_volt  = MTEST_VOLT;
#endif

    if (ldo) {
        *ldo = ldo_volt;
    }
}

static void pmu_dig_set_volt(void)
{
    uint32_t lock;
    uint16_t ldo_volt, old_act_ldo, old_lp_ldo;
    bool volt_inc = false;

    lock = int_lock();

    pmu_dig_get_target_volt(&ldo_volt);

    pmu_module_get_volt(PMU_DIG1, &old_lp_ldo, &old_act_ldo);

    if (old_act_ldo < ldo_volt) {
        volt_inc = true;
    }
    pmu_module_ramp_volt(PMU_DIG1, dig_lp_ldo, ldo_volt);
    if (volt_inc) {
        hal_sys_timer_delay_us(PMU_VCORE_STABLE_TIME_US);
    }

    int_unlock(lock);
}

void pmu_mode_change(enum PMU_POWER_MODE_T mode)
{
    pmu_module_config(PMU_DIG1,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    pmu_dig_set_volt();
    pmu_module_config(PMU_DIG2,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
}

void pmu_sleep_en(unsigned char sleep_en)  __attribute__((alias("pmu_itn_sleep_en")));

int BOOT_TEXT_FLASH_LOC pmu_open(void)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))

    uint16_t val;

    // Disable and clear all PMU irqs by default
    pmu_write(PMU_REG_INT_MASK, 0);
    pmu_write(PMU_REG_INT_EN, 0);
    // PMU irqs cannot be cleared by PMU soft reset
    pmu_read(PMU_REG_INT_STATUS, &val);
    pmu_write(PMU_REG_INT_CLR, val);

#ifndef NO_SLEEP
    pmu_sleep_en(true);
#endif

    pmu_codec_mic_bias_enable(0);

    pmu_mode_change(PMU_POWER_MODE_LDO);

#ifdef FORCE_LP_MODE
    pmu_read(PMU_REG_DIG1_CFG, &val);
    val |= LP_EN_VCORE1_LDO_DR | LP_EN_VCORE1_LDO_REG;
    pmu_write(PMU_REG_DIG1_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    pmu_read(PMU_REG_VBG_CFG, &val);
    val = (val & ~BG_VBG_SEL_REG) | BG_VBG_SEL_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
    hal_sys_timer_delay(MS_TO_TICKS(1));

    val = (val & ~(PU_BIAS_LDO_REG | BG_CONSTANT_GM_BIAS_REG | BG_CORE_EN_REG | BG_VTOI_EN_REG | BG_NOTCH_EN_REG)) |
        PU_BIAS_LDO_DR | BG_CONSTANT_GM_BIAS_DR | BG_CORE_EN_DR | BG_VTOI_EN_DR | BG_NOTCH_EN_DR;
    pmu_write(PMU_REG_VBG_CFG, val);
#endif

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#endif

#if defined(MCU_HIGH_PERFORMANCE_MODE)
    analog_read(PMU_REG_ANA_277, &val);
    val = (val & ~(REG_BT_BBPLL_LDO_CP_MASK | REG_BT_BBPLL_LDO_DIG_MASK | REG_BT_BBPLL_LDO_VCO_MASK)) |
        REG_BT_BBPLL_LDO_CP(7) | REG_BT_BBPLL_LDO_DIG(7) | REG_BT_BBPLL_LDO_VCO(7);
    analog_write(PMU_REG_ANA_277, val);
    analog_read(PMU_REG_ANA_75, &val);
    val = (val & ~(REG_AUDPLL_VCO_SPD_MASK | REG_AUDPLL_VCO_SWRC_MASK)) |
        REG_AUDPLL_VCO_SPD(7) | REG_AUDPLL_VCO_SWRC(3);
    analog_write(PMU_REG_ANA_75, val);

    pmu_high_performance_mode_enable(true);
#endif

#endif // PMU_INIT || (!FPGA && !PROGRAMMER)

    return 0;
}

void pmu_sleep(void)
{
}

void pmu_wakeup(void)
{
}

void pmu_codec_mic_bias_enable(uint32_t map)
{
    uint16_t val;

    pmu_read(PMU_REG_MIC_BIAS_A, &val);
    if (map & AUD_VMIC_MAP_VMIC1) {
        val |= REG_MIC_BIASA_EN;
    } else {
        val &= ~REG_MIC_BIASA_EN;
    }
    pmu_write(PMU_REG_MIC_BIAS_A, val);

    pmu_read(PMU_REG_MIC_BIAS_B, &val);
    if (map & AUD_VMIC_MAP_VMIC2) {
        val |= REG_MIC_BIASB_EN;
    } else {
        val &= ~REG_MIC_BIASB_EN;
    }
    if (map & (AUD_VMIC_MAP_VMIC1 | AUD_VMIC_MAP_VMIC2)) {
        val |= REG_MIC_LDO_EN;
    } else {
        val &= ~REG_MIC_LDO_EN;
    }
    pmu_write(PMU_REG_MIC_BIAS_B, val);

    pmu_read(PMU_REG_MIC_BIAS_EN, &val);
    val |= REG_PU_MIC_BIASA_DR | REG_PU_MIC_BIASB_DR;
    if (map & AUD_VMIC_MAP_VMIC1) {
        val |= REG_PU_MIC_BIASA_REG;
    } else {
        val &= ~REG_PU_MIC_BIASA_REG;
    }
    if (map & AUD_VMIC_MAP_VMIC2) {
        val |= REG_PU_MIC_BIASB_REG;
    } else {
        val &= ~REG_PU_MIC_BIASB_REG;
    }
    pmu_write(PMU_REG_MIC_BIAS_EN, val);

    pmu_read(PMU_REG_MIC_LDO_EN, &val);
    val |= REG_PU_MIC_LDO_DR;
    if (map & (AUD_VMIC_MAP_VMIC1 | AUD_VMIC_MAP_VMIC2)) {
        val |= REG_PU_MIC_LDO_REG;
    } else {
        val &= ~REG_PU_MIC_LDO_REG;
    }
    pmu_write(PMU_REG_MIC_LDO_EN, val);
}

void pmu_codec_mic_bias_lowpower_mode(uint32_t map)
{
}

void pmu_flash_write_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if (pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req |= PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt();
#endif
}

void pmu_flash_read_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if ((pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) == 0) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req &= ~PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt();
#endif
}

void BOOT_TEXT_FLASH_LOC pmu_flash_freq_config(uint32_t freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    uint32_t lock;

    lock = int_lock();
    if (freq > 52000000) {
        // The real max freq is 120M
        //pmu_vcore_req |= PMU_VCORE_FLASH_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_FLASH_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt();
#endif
}

void pmu_anc_config(int enable)
{
}

void pmu_fir_high_speed_config(int enable)
{
}

void pmu_iir_freq_config(uint32_t freq)
{
}

void pmu_rs_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    if (freq >= 60000000) {
        pmu_vcore_req |= PMU_VCORE_RS_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_RS_FREQ_HIGH;
    }
    int_unlock(lock);

    pmu_dig_set_volt();
}

void BOOT_TEXT_SRAM_LOC pmu_sys_freq_config(enum HAL_CMU_FREQ_T freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#if defined(MCU_HIGH_PERFORMANCE_MODE) || defined(ULTRA_LOW_POWER) || !defined(OSC_26M_X4_AUD2BB)
    uint32_t lock;
    enum PMU_VCORE_REQ_T old_req;
    bool update = false;

    lock = int_lock();
    old_req = pmu_vcore_req;
    pmu_vcore_req &= ~(PMU_VCORE_SYS_FREQ_HIGH | PMU_VCORE_SYS_FREQ_MEDIUM);
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    if (freq > HAL_CMU_FREQ_104M) {
        if (high_perf_on) {
            // The real freq is 350M
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_HIGH;
        } else {
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
        }
    } else {
#ifndef OSC_26M_X4_AUD2BB
        if (freq == HAL_CMU_FREQ_104M) {
            // The real freq is 200M
            pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
        }
#endif
    }
#else
    if (freq > HAL_CMU_FREQ_104M) {
        pmu_vcore_req |= PMU_VCORE_SYS_FREQ_MEDIUM;
    }
#endif
    if (old_req != pmu_vcore_req) {
        update = true;
    }
    int_unlock(lock);

    if (!update) {
        // Nothing changes
        return;
    }

    pmu_dig_set_volt();
#endif
#endif
}

void pmu_high_performance_mode_enable(bool enable)
{
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    if (high_perf_on == enable) {
        return;
    }
    high_perf_on = enable;

    if (!enable) {
        if (high_perf_freq_mhz > 300) {
#ifdef NO_X2_X4
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);
#else
            // Switch to 52M to avoid using PLL
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
#endif
            // Restore the default div
            analog_aud_pll_set_dig_div(2);
            // Restore the sys freq
            hal_cmu_sys_set_freq(hal_sysfreq_get_hw_freq());
        }
        // Restore the default PLL freq (393M)
        analog_aud_freq_pll_config(CODEC_FREQ_24P576M, 16);
    }

    pmu_sys_freq_config(hal_sysfreq_get_hw_freq());

    if (enable) {
        uint32_t pll_freq;

        // Change freq first, and then change divider.
        // Otherwise there will be an instant very high freq sent to digital domain.

        if (high_perf_freq_mhz <= 300) {
            pll_freq = high_perf_freq_mhz * 1000000 * 2;
        } else {
            pll_freq = high_perf_freq_mhz * 1000000;
        }
        analog_aud_freq_pll_config(pll_freq / 16, 16);

        if (high_perf_freq_mhz > 300) {
#ifdef NO_X2_X4
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);
#else
            // Switch to 52M to avoid using PLL
            hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
#endif
            // Change the dig div
            analog_aud_pll_set_dig_div(1);
            // Restore the sys freq
            hal_cmu_sys_set_freq(hal_sysfreq_get_hw_freq());
        }
    }
#endif
}

void pmu_usb_config(enum PMU_USB_CONFIG_TYPE_T type)
{
}

#ifdef RTC_ENABLE
void pmu_rtc_enable(void)
{
    uint16_t readval;
    uint32_t lock;

#ifdef SIMU
    // Set RTC counter to 1KHz
    pmu_write(PMU_REG_RTC_DIV_1HZ, 32 - 2);
#else
    // Set RTC counter to 1Hz
    pmu_write(PMU_REG_RTC_DIV_1HZ, CONFIG_SYSTICK_HZ * 2 - 2);
#endif

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval |= RTC_POWER_ON_EN | PU_LPO_DR | PU_LPO_REG;
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);
    int_unlock(lock);
}

void pmu_rtc_disable(void)
{
    uint16_t readval;
    uint32_t lock;

    pmu_rtc_clear_alarm();

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval &= ~(RTC_POWER_ON_EN | PU_LPO_DR);
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);
    int_unlock(lock);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_enabled(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);

    return !!(readval & RTC_POWER_ON_EN);
}

void pmu_rtc_set(uint32_t seconds)
{
    uint16_t high, low;

    // Need 3 seconds to load a new value
    seconds += 3;

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_RTC_LOAD_LOW, low);
    pmu_write(PMU_REG_RTC_LOAD_HIGH, high);
}

uint32_t pmu_rtc_get(void)
{
    uint16_t high, low, high2;

    pmu_read(PMU_REG_RTC_VAL_HIGH, &high);
    pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    // Handle counter wrap
    pmu_read(PMU_REG_RTC_VAL_HIGH, &high2);
    if (high != high2) {
        high = high2;
        pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    }

    return (high << 16) | low;
}

void pmu_rtc_set_alarm(uint32_t seconds)
{
    uint16_t readval;
    uint16_t high, low;
    uint32_t lock;

    // Need 1 second to raise the interrupt
    if (seconds > 0) {
        seconds -= 1;
    }

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_INT_CLR, RTC_INT1_MSKED);

    pmu_write(PMU_REG_RTC_MATCH1_LOW, low);
    pmu_write(PMU_REG_RTC_MATCH1_HIGH, high);

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval |= RTC_INT_EN_1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);
}

uint32_t BOOT_TEXT_SRAM_LOC pmu_rtc_get_alarm(void)
{
    uint16_t high, low;

    pmu_read(PMU_REG_RTC_MATCH1_LOW, &low);
    pmu_read(PMU_REG_RTC_MATCH1_HIGH, &high);

    // Compensate the alarm offset
    return (uint32_t)((high << 16) | low) + 1;
}

void pmu_rtc_clear_alarm(void)
{
    uint16_t readval;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval &= ~RTC_INT_EN_1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);

    pmu_write(PMU_REG_INT_CLR, RTC_INT1_MSKED);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_alarm_status_set(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_EN, &readval);

    return !!(readval & RTC_INT_EN_1);
}

int pmu_rtc_alarm_alerted()
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_STATUS, &readval);

    return !!(readval & RTC_INT_1);
}

static void pmu_rtc_irq_handler(uint16_t irq_status)
{
    uint32_t seconds;

    if (irq_status & RTC_INT1_MSKED) {
        pmu_rtc_clear_alarm();

        if (rtc_irq_handler) {
            seconds = pmu_rtc_get();
            rtc_irq_handler(seconds);
        }
    }
}

void pmu_rtc_set_irq_handler(PMU_RTC_IRQ_HANDLER_T handler)
{
    uint16_t readval;
    uint32_t lock;

    rtc_irq_handler = handler;

    lock = int_lock();
    pmu_read(PMU_REG_INT_MASK, &readval);
    if (handler) {
        readval |= RTC_INT1_MSK;
    } else {
        readval &= ~RTC_INT1_MSK;
    }
    pmu_write(PMU_REG_INT_MASK, readval);
    pmu_set_irq_unified_handler(PMU_IRQ_TYPE_RTC, handler ? pmu_rtc_irq_handler : NULL);
    int_unlock(lock);
}
#endif

static void pmu_general_irq_handler(void)
{
    uint32_t lock;
    uint16_t val;
    bool rtc;

    rtc = false;

    lock = int_lock();
    pmu_read(PMU_REG_INT_MSKED_STATUS, &val);
    if (val & (RTC_INT1_MSKED | RTC_INT0_MSKED)) {
        rtc = true;
    }
    if (rtc) {
        pmu_write(PMU_REG_INT_CLR, val);
    }
    int_unlock(lock);

    if (rtc) {
        if (pmu_irq_hdlrs[PMU_IRQ_TYPE_RTC]) {
            pmu_irq_hdlrs[PMU_IRQ_TYPE_RTC](val);
        }
    }
}

int pmu_set_irq_unified_handler(enum PMU_IRQ_TYPE_T type, PMU_IRQ_UNIFIED_HANDLER_T hdlr)
{
    bool update;
    uint32_t lock;
    int i;

    if (type >= PMU_IRQ_TYPE_QTY) {
        return 1;
    }

    update = false;

    lock = int_lock();

    for (i = 0; i < PMU_IRQ_TYPE_QTY; i++) {
        if (pmu_irq_hdlrs[i]) {
            break;
        }
    }

    pmu_irq_hdlrs[type] = hdlr;

    if (hdlr) {
        update = (i >= PMU_IRQ_TYPE_QTY);
    } else {
        if (i == type) {
            for (; i < PMU_IRQ_TYPE_QTY; i++) {
                if (pmu_irq_hdlrs[i]) {
                    break;
                }
            }
            update = (i >= PMU_IRQ_TYPE_QTY);
        }
    }

    if (update) {
        if (hdlr) {
            NVIC_SetVector(PMU_IRQn, (uint32_t)pmu_general_irq_handler);
            NVIC_SetPriority(PMU_IRQn, IRQ_PRIORITY_NORMAL);
            NVIC_ClearPendingIRQ(PMU_IRQn);
            NVIC_EnableIRQ(PMU_IRQn);
        } else {
            NVIC_DisableIRQ(PMU_IRQn);
        }
    }

    int_unlock(lock);

    return 0;
}

int pmu_debug_config_ana(uint16_t volt)
{
    return 0;
}

int pmu_debug_config_codec(uint16_t volt)
{
#ifdef ANC_PROD_TEST
    if (volt == 1600 || volt == 1700 || volt == 1800 || volt == 1900 || volt == 1950) {
        vhppa_mv = vcodec_mv = volt;
        vcodec_off = true;
    } else {
        vcodec_off = false;
        return 1;
    }
#endif
    return 0;
}

int pmu_debug_config_vcrystal(bool on)
{
    return 0;
}

int pmu_debug_config_audio_output(bool diff)
{
    return 0;
}

void pmu_debug_reliability_test(int stage)
{
}

#ifdef __WATCHER_DOG_RESET__
struct PMU_WDT_CTX_T {
    bool enabled;
    uint16_t wdt_timer;
    uint16_t wdt_cfg;
};

static struct PMU_WDT_CTX_T BOOT_BSS_LOC wdt_ctx;

void BOOT_TEXT_SRAM_LOC pmu_wdt_save_context(void)
{
    uint16_t wdt_cfg = 0, wdt_timer = 0;
    pmu_read(PMU_REG_WDT_CFG, &wdt_cfg);
    if (wdt_cfg & (REG_WDT_RESET_EN | REG_WDT_EN)){
        wdt_ctx.enabled = true;
        wdt_ctx.wdt_cfg = wdt_cfg;
        pmu_read(PMU_REG_WDT_TIMER, &wdt_timer);
        wdt_ctx.wdt_timer = wdt_timer;
    }
}

void BOOT_TEXT_SRAM_LOC pmu_wdt_restore_context(void)
{
    if (wdt_ctx.enabled) {
        pmu_write(PMU_REG_WDT_TIMER, wdt_ctx.wdt_timer);
        pmu_write(PMU_REG_WDT_CFG, wdt_ctx.wdt_cfg);
    }
}
#endif

void pmu_wdt_set_irq_handler(PMU_WDT_IRQ_HANDLER_T handler)
{
}

int pmu_wdt_config(uint32_t irq_ms, uint32_t reset_ms)
{
    // No wdt irq on best3008
    if (irq_ms + reset_ms > 0xFFFF) {
        return 1;
    }
    wdt_timer = irq_ms + reset_ms;

    pmu_write(PMU_REG_WDT_TIMER, wdt_timer);

    return 0;
}

void pmu_wdt_start(void)
{
    uint16_t val;

    if (wdt_timer == 0) {
        return;
    }

    pmu_read(PMU_REG_WDT_CFG, &val);
    val |= (REG_WDT_RESET_EN | REG_WDT_EN);
    pmu_write(PMU_REG_WDT_CFG, val);
}

#ifndef __WATCHER_DOG_RESET__
BOOT_TEXT_SRAM_LOC
#endif
void pmu_wdt_stop(void)
{
    uint16_t val;

    pmu_read(PMU_REG_WDT_CFG, &val);
    val &= ~(REG_WDT_RESET_EN | REG_WDT_EN);
    pmu_write(PMU_REG_WDT_CFG, val);
}

void pmu_wdt_feed(void)
{
    if (wdt_timer == 0) {
        return;
    }

    pmu_write(PMU_REG_WDT_TIMER, wdt_timer);
}

#endif

