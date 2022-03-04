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
#include "usbphy.h"
#include "cmsis_nvic.h"
#include "hal_cmu.h"
#include "hal_phyif.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include CHIP_SPECIFIC_HDR(reg_usbphy)

//#define USBPHY_SERIAL_ITF

#define rf_read(reg, val)                   hal_analogif_reg_read(RF_REG(reg), val)
#define rf_write(reg, val)                  hal_analogif_reg_write(RF_REG(reg), val)

// RF REG_8F
#define REG_APLL_DPLL_SEL                   (1 << 0)
#define REG_DPA_SEL                         (1 << 1)
#define REG_LDO_DPA_ENABLE                  (1 << 2)
#define REG_LDO_DPA_ENABLE_DR               (1 << 3)
#define REG_BT_PADRV_PU_DR                  (1 << 4)
#define REG_BT_PADRV_PU                     (1 << 5)
#define REG_BT_MIXER_PU_DR                  (1 << 6)
#define REG_BT_MIXER_PU                     (1 << 7)
#define REG_BT_MIXER_BUF_PU_DR              (1 << 8)
#define REG_BT_MIXER_BUF_PU                 (1 << 9)

// RF REG_90
#define REG_BT_XTAL_PU_BUF_USB_ITF          (1 << 6)

// RF REG_E9
#define REG_BT_LNA_RFFLT_BUF_PU_DR          (1 << 0)
#define REG_BT_LNA_RFFLT_BUF_PU             (1 << 1)
#define REG_BT_SW_ITXB_DR                   (1 << 2)
#define REG_BT_SW_ITXB                      (1 << 3)
#define REG_PU_USB_1P3_DR                   (1 << 4)
#define REG_PU_USB_1P3                      (1 << 5)
#define REG_PU_USB_3P3_DR                   (1 << 6)
#define REG_PU_USB_3P3                      (1 << 7)
#define REG_PU_USB_1P3_DLY_VALUE_SHIFT      8
#define REG_PU_USB_1P3_DLY_VALUE_MASK       (0xF << REG_PU_USB_1P3_DLY_VALUE_SHIFT)
#define REG_PU_USB_1P3_DLY_VALUE(n)         BITFIELD_VAL(REG_PU_USB_1P3_DLY_VALUE, n)
#define REG_PU_USB_3P3_DLY_VALUE_SHIFT      12
#define REG_PU_USB_3P3_DLY_VALUE_MASK       (0xF << REG_PU_USB_3P3_DLY_VALUE_SHIFT)
#define REG_PU_USB_3P3_DLY_VALUE(n)         BITFIELD_VAL(REG_PU_USB_3P3_DLY_VALUE, n)

// RF REG_1F3
#define REG_LDO_DPA_AON_SEL                 (1 << 0)
#define REG_LDO_DPA_LP_EN_DR                (1 << 1)
#define REG_LDO_DPA_LP_EN                   (1 << 2)
#define REG_LDO_DPA_VBIT_RAMP_EN            (1 << 3)
#define REG_LDO_DPA_RES_SEL_SLEEP_SHIFT     4
#define REG_LDO_DPA_RES_SEL_SLEEP_MASK      (0x1F << REG_LDO_DPA_RES_SEL_SLEEP_SHIFT)
#define REG_LDO_DPA_RES_SEL_SLEEP(n)        BITFIELD_VAL(REG_LDO_DPA_RES_SEL_SLEEP, n)
#define REG_LDO_DPA_RES_SEL_NORMAL_SHIFT    9
#define REG_LDO_DPA_RES_SEL_NORMAL_MASK     (0x1F << REG_LDO_DPA_RES_SEL_NORMAL_SHIFT)
#define REG_LDO_DPA_RES_SEL_NORMAL(n)       BITFIELD_VAL(REG_LDO_DPA_RES_SEL_NORMAL, n)

// RF REG_281
#define REG_LDO_DPA_BYPASS                  (1 << 0)
#define REG_LDO_DPA_IFB_PBUFFER             (1 << 1)
#define REG_LDO_DPA_IFB_NBUFFER             (1 << 2)
#define REG_LDO_DPA_PULL_DOWN               (1 << 3)
#define REG_LDO_DPA_SOFT_START_CNT_SHIFT    4
#define REG_LDO_DPA_SOFT_START_CNT_MASK     (0x1F << REG_LDO_DPA_SOFT_START_CNT_SHIFT)
#define REG_LDO_DPA_SOFT_START_CNT(n)       BITFIELD_VAL(REG_LDO_DPA_SOFT_START_CNT, n)

// REG_46
#define USB_EMPHASIS_EN                     (1 << 0)
#define USB_EM_SEL_SHIFT                    1
#define USB_EM_SEL_MASK                     (0x7 << USB_EM_SEL_SHIFT)
#define USB_EM_SEL(n)                       BITFIELD_VAL(USB_EM_SEL, n)
#define USB_DCC_BPS                         (1 << 4)
#define USB_DCC_SEL_SHIFT                   5
#define USB_DCC_SEL_MASK                    (0xF << USB_DCC_SEL_SHIFT)
#define USB_DCC_SEL(n)                      BITFIELD_VAL(USB_DCC_SEL, n)
#define USB_CK60M_EN                        (1 << 9)
#define USB_CK480M_EN                       (1 << 10)
#define USB_CKIN4_EN                        (1 << 11)
#define USB_FS_RCV_PD                       (1 << 12)
#define USB_HS_TX_PD                        (1 << 13)

enum USB_LDO_REG_T {
    USB_LDO_REG_RF_8F   = 0x8F,
    USB_LDO_REG_RF_90   = 0x90,
    USB_LDO_REG_RF_E9   = 0xE9,
    USB_LDO_REG_RF_1F3  = 0x1F3,
    USB_LDO_REG_RF_281  = 0x281,
};

void usbphy_ldo_config(int enable)
{
    uint16_t val;

    if (enable) {
        rf_read(USB_LDO_REG_RF_90, &val);
        val |= REG_BT_XTAL_PU_BUF_USB_ITF;
        rf_write(USB_LDO_REG_RF_90, val);

        rf_read(USB_LDO_REG_RF_E9, &val);
        val = (val & ~(REG_PU_USB_3P3_DR)) | REG_PU_USB_1P3_DR | REG_PU_USB_1P3;
        rf_write(USB_LDO_REG_RF_E9, val);

#ifdef LDO_DPA_CTRL
        rf_read(USB_LDO_REG_RF_1F3, &val);
        val = (val & ~(REG_LDO_DPA_RES_SEL_SLEEP_MASK | REG_LDO_DPA_RES_SEL_NORMAL_MASK)) |
            REG_LDO_DPA_RES_SEL_SLEEP(0x18) | REG_LDO_DPA_RES_SEL_NORMAL(0x18);
        rf_write(USB_LDO_REG_RF_1F3, val);

        rf_read(USB_LDO_REG_RF_281, &val);
        val |= REG_LDO_DPA_IFB_PBUFFER | REG_LDO_DPA_IFB_NBUFFER;
        rf_write(USB_LDO_REG_RF_281, val);

        rf_read(USB_LDO_REG_RF_8F, &val);
        val |= REG_LDO_DPA_ENABLE_DR | REG_LDO_DPA_ENABLE;
        rf_write(USB_LDO_REG_RF_8F, val);
#endif

        hal_sys_timer_delay(MS_TO_TICKS(3));

#ifdef USB_HIGH_SPEED
        usbphy_read(0x0D, &val);
        val |= (1 << 5);
        usbphy_write(0x0D, val);
#endif
    } else {
#ifdef USB_HIGH_SPEED
        usbphy_read(0x0D, &val);
        val &= ~(1 << 5);
        usbphy_write(0x0D, val);
#endif

#ifdef LDO_DPA_CTRL
        rf_read(USB_LDO_REG_RF_8F, &val);
        val = (val & ~REG_LDO_DPA_ENABLE) | REG_LDO_DPA_ENABLE_DR;
        rf_write(USB_LDO_REG_RF_8F, val);

        rf_read(USB_LDO_REG_RF_281, &val);
        val &= ~(REG_LDO_DPA_IFB_PBUFFER | REG_LDO_DPA_IFB_NBUFFER);
        rf_write(USB_LDO_REG_RF_281, val);
#endif

        // Always pu 1P3 to avoid current leakage
        rf_read(USB_LDO_REG_RF_E9, &val);
        val = (val & ~(REG_PU_USB_3P3)) | REG_PU_USB_1P3_DR | REG_PU_USB_1P3 | REG_PU_USB_3P3_DR;
        rf_write(USB_LDO_REG_RF_E9, val);

        rf_read(USB_LDO_REG_RF_90, &val);
        val &= ~REG_BT_XTAL_PU_BUF_USB_ITF;
        rf_write(USB_LDO_REG_RF_90, val);
    }
}

void usbphy_open(void)
{
    unsigned short val_01, val;

#ifdef USB_HIGH_SPEED

    usbphy_read(0x01, &val_01);
    val_01 &= ~((1 << 0) | (1 << 11));
    usbphy_write(0x01, val_01);
    val_01 |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 13);
#ifdef USBPHY_SERIAL_ITF
    // cdr_clk polariy=1
    val_01 |= (1 << 4);
#else
    // cdr_clk polariy=0
    val_01 &= ~(1 << 4);
#endif
    usbphy_write(0x01, val_01);

    usbphy_read(0x02, &val);
    val = (val & ~(0x1F << 10)) | (0xC << 10);
    usbphy_write(0x02, val);

    usbphy_read(0x05, &val);
    val |= (1 << 8);
    usbphy_write(0x05, val);

#ifdef USBPHY_SERIAL_ITF
    usbphy_write(0x06, 0x6EE9);
#else
    usbphy_write(0x06, 0xEEE8);
#endif

    usbphy_write(0x07, 0x9913);

    usbphy_read(0x08, &val);
    usbphy_write(0x08, val);

    usbphy_read(0x09, &val);
    usbphy_write(0x09, val);

#ifdef USBPHY_SERIAL_ITF
    usbphy_write(0x0D, 0x2B1E);
#else
    usbphy_write(0x0D, 0x2A1E);
#endif

    // Disable force clocks, and disable hs son signal
    usbphy_write(0x22, 0x030F);

    usbphy_read(0x46, &val);
    val |= USB_FS_RCV_PD | USB_CK60M_EN | USB_CK480M_EN | USB_CKIN4_EN | USB_HS_TX_PD;
    usbphy_write(0x46, val);

    // Ignore all UTMI errors
    usbphy_write(0x12, 0x0003);

    hal_sys_timer_delay(MS_TO_TICKS(1));

    val_01 |= (1 << 0);
    usbphy_write(0x01, val_01);

#ifdef USB_HS_LOOPBACK_TEST
// one time test: Check 0x21 bits: rb_bist_done, rb_bist_fail
// continous test: Check 0x20 upper 8 bits: 0xA5 -- passed (default tx pattern), otherValue -- failed
#define CONTINUOUS_TEST         1
// ana phy loop (including dig phy)
// dig phy loop
#define DIG_PHY_LOOP            0

    usbphy_read(0x0A, &val);
    val |= (1 << 3) | (1 << 4) | (1 << 6);
    usbphy_write(0x0A, val);
    usbphy_read(0x0B, &val);
    val &= ~((1 << 3) | (1 << 4) | (3 << 6));
    usbphy_write(0x0B, val);
    usbphy_read(0x06, &val);
    usbphy_write(0x06, val);

    usbphy_read(0x0D, &val);
    val |= (1 << 5) | (1 << 7);
    usbphy_write(0x0D, val);
    usbphy_read(0x1A, &val);
    val |= (1 << 0);
#if (CONTINUOUS_TEST)
    val |= (1 << 10);
#endif
    usbphy_write(0x1A, val);

    hal_sys_timer_delay(MS_TO_TICKS(1));

    // Enable force clocks
    usbphy_read(0x22, &val);
    val |= (1 << 4) | (1 << 5) | (1 << 6);
#if (DIG_PHY_LOOP)
    val |= (1 << 10);
#endif
    usbphy_write(0x22, val);

    usbphy_read(0x09, &val);
    val |= (1 << 15);
    usbphy_write(0x09, val);
#endif

#else // !USB_HIGH_SPEED

    usbphy_read(0x01, &val_01);
    val_01 &= ~(1 << 0);
    usbphy_write(0x01, val_01);
    val_01 |= (1 << 3);
    usbphy_write(0x01, val_01);

#ifdef USB_USE_USBPLL
    val_01 |= (1 << 2) | (1 << 12);
    usbphy_write(0x01, val_01);

    usbphy_read(0x33, &val);
    val |= (1 << 0);
    usbphy_write(0x33, val);
#endif

    val = 0;
    usbphy_write(0x06, val);

    usbphy_read(0x07, &val);
    usbphy_write(0x07, val);

    val = 0;
    usbphy_write(0x08, val);

    val = 0;
    usbphy_write(0x09, val);

    val_01 |= (1 << 0);
    usbphy_write(0x01, val_01);

#endif // USB_HIGH_SPEED

}

void usbphy_close(void)
{
}

void usbphy_sleep(void)
{
    uint16_t val;

    usbphy_read(0x01, &val);
#ifdef USB_HIGH_SPEED
    val &= ~((1 << 1) | (1 << 2) | (1 << 3));
#elif defined(USB_USE_USBPLL)
    val &= ~((1 << 2) | (1 << 3));
#else
    val &= ~(1 << 3);
#endif
    usbphy_write(0x01, val);

#ifdef USB_HIGH_SPEED
    usbphy_read(0x0D, &val);
    val &= ~(1 << 5);
    usbphy_write(0x0D, val);
#endif
}

void usbphy_wakeup(void)
{
    uint16_t val;

#ifdef USB_HIGH_SPEED
    usbphy_read(0x0D, &val);
    val |= (1 << 5);
    usbphy_write(0x0D, val);
#endif

    usbphy_read(0x01, &val);
#ifdef USB_HIGH_SPEED
    val |= (1 << 1) | (1 << 2) | (1 << 3);
#elif defined(USB_USE_USBPLL)
    val |= (1 << 2) | (1 << 3);
#else
    val |= (1 << 3);
#endif
    usbphy_write(0x01, val);
}

//============================================================================================
// USB Pin Status Check
//============================================================================================

static enum PMU_USB_PIN_CHK_STATUS_T usb_pin_status;

static PMU_USB_PIN_CHK_CALLBACK usb_pin_callback;

static int pmu_usb_check_pin_status(enum PMU_USB_PIN_CHK_STATUS_T status)
{
    int dp, dm;

    pmu_usb_get_pin_status(&dp, &dm);

    //TRACE(5,"[%X] %s: status=%d dp=%d dm=%d", hal_sys_timer_get(), __FUNCTION__, status, dp, dm);

    // HOST_RESUME: (resume) dp == 0 && dm == 1, (reset) dp == 0 && dm == 0

    if ( (status == PMU_USB_PIN_CHK_DEV_CONN && (dp == 1 && dm == 0)) ||
            (status == PMU_USB_PIN_CHK_DEV_DISCONN && (dp == 0 && dm == 0)) ||
            (status == PMU_USB_PIN_CHK_HOST_RESUME && dp == 0) ) {
        return 1;
    }

    return 0;
}

static void pmu_usb_pin_irq_handler(void)
{
    uint16_t val;
    uint32_t lock;

    //TRACE(2,"[%X] %s", hal_sys_timer_get(), __FUNCTION__);

    lock = int_lock();
    usbphy_read(USBPHY_REG_1A, &val);
    val |= REG_1A_INTR_CLR;
    usbphy_write(USBPHY_REG_1A, val);
    int_unlock(lock);

    if (usb_pin_callback) {
        if (pmu_usb_check_pin_status(usb_pin_status)) {
            pmu_usb_disable_pin_status_check();
            usb_pin_callback(usb_pin_status);
        }
    }
}

int pmu_usb_config_pin_status_check(enum PMU_USB_PIN_CHK_STATUS_T status, PMU_USB_PIN_CHK_CALLBACK callback, int enable)
{
    uint16_t val;
    uint32_t lock;

    //TRACE(3,"[%X] %s: status=%d", hal_sys_timer_get(), __FUNCTION__, status);

    if (status >= PMU_USB_PIN_CHK_STATUS_QTY) {
        return 1;
    }

    NVIC_DisableIRQ(USB_PIN_IRQn);

    lock = int_lock();

    usb_pin_status = status;
    usb_pin_callback = callback;

    usbphy_read(USBPHY_REG_1A, &val);

    // Mask the irq
    val &= ~REG_1A_USB_INSERT_INTR_MSK;

    // Config pin check
    val |= REG_1A_DEBOUNCE_EN | REG_1A_NOLS_MODE | REG_1A_USBINSERT_DET_EN;

    val &= ~(REG_1A_POL_USB_RX_DP | REG_1A_POL_USB_RX_DM);
    if (status == PMU_USB_PIN_CHK_DEV_CONN) {
        // Check dp 0->1, dm x->0
    } else if (status == PMU_USB_PIN_CHK_DEV_DISCONN) {
        // Check dp 1->0, dm x->0
        val |= REG_1A_POL_USB_RX_DP;
    } else if (status == PMU_USB_PIN_CHK_HOST_RESUME) {
        // Check dp 1->0, dm 0->1 (resume) or dm 0->0 (reset)
        val |= REG_1A_POL_USB_RX_DP;
    }

    if (status != PMU_USB_PIN_CHK_NONE && callback) {
        val |= REG_1A_USBINSERT_INTR_EN | REG_1A_USB_INSERT_INTR_MSK | REG_1A_INTR_CLR;
    }

    usbphy_write(USBPHY_REG_1A, val);

    int_unlock(lock);

    if (enable) {
        // Wait at least 10 cycles of 32K clock for the new status when signal checking polarity is changed
        hal_sys_timer_delay(5);
        pmu_usb_enable_pin_status_check();
    }

    return 0;
}

void pmu_usb_enable_pin_status_check(void)
{
    uint16_t val;
    uint32_t lock;

    if (usb_pin_status != PMU_USB_PIN_CHK_NONE && usb_pin_callback) {
        lock = int_lock();
        usbphy_read(USBPHY_REG_1A, &val);
        val |= REG_1A_INTR_CLR;
        usbphy_write(USBPHY_REG_1A, val);
        int_unlock(lock);
        NVIC_ClearPendingIRQ(USB_PIN_IRQn);

        if (pmu_usb_check_pin_status(usb_pin_status)) {
            pmu_usb_disable_pin_status_check();
            usb_pin_callback(usb_pin_status);
            return;
        }

        NVIC_SetVector(USB_PIN_IRQn, (uint32_t)pmu_usb_pin_irq_handler);
        NVIC_SetPriority(USB_PIN_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_EnableIRQ(USB_PIN_IRQn);
    }
}

void pmu_usb_disable_pin_status_check(void)
{
    uint16_t val;
    uint32_t lock;

    NVIC_DisableIRQ(USB_PIN_IRQn);

    lock = int_lock();

    usbphy_read(USBPHY_REG_1A, &val);
    val &= ~(REG_1A_USBINSERT_INTR_EN | REG_1A_USBINSERT_DET_EN);
    usbphy_write(USBPHY_REG_1A, val);

    int_unlock(lock);
}

void pmu_usb_get_pin_status(int *dp, int *dm)
{
    uint16_t pol, val;

    usbphy_read(USBPHY_REG_1A, &pol);
    usbphy_read(USBPHY_REG_25, &val);

    *dp = (!(pol & REG_1A_POL_USB_RX_DP)) ^ (!(val & REG_25_USB_STATUS_RX_DP));
    *dm = (!(pol & REG_1A_POL_USB_RX_DM)) ^ (!(val & REG_25_USB_STATUS_RX_DM));
}

