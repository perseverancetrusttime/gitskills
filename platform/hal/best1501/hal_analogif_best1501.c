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
#ifndef NO_ISPI

#include "plat_types.h"
#include "cmsis.h"
#include "hal_analogif.h"
#include "hal_location.h"
#include "hal_spi.h"
#include "hal_trace.h"
#include "hal_iomux.h"

#define ISPI_ANA_CS                     1
#define ISPI_RF_CS                      2

#define ISPI_ANA_PAGE_REG               0x60
#define ISPI_RF_PAGE_REG                0x80
#define ISPI_NORMAL_PAGE_REG            0x00

#define ITNPMU_REG(r)                   (((r) & 0xFFF) | 0x3000)

#define ANA_REG_CHIP_ID                 ITNPMU_REG(0x00)

#define ANA_CHIP_ID_SHIFT               (4)
#define ANA_CHIP_ID_MASK                (0xFFF << ANA_CHIP_ID_SHIFT)
#define ANA_CHIP_ID(n)                  BITFIELD_VAL(ANA_CHIP_ID, n)
#define ANA_VAL_CHIP_ID                 0x151

// ISPI_ARBITRATOR_ENABLE should be defined when:
// 1) BT and MCU will access RF register at the same time; or
// 2) BT can access PMU/ANA, and BT will access RF register at the same time
//    when MCU is accessing PMU/ANA register

#ifdef ISPI_ARBITRATOR_ENABLE
// Min padding OSC cycles needed: BT=0 MCU=6
// When OSC=26M and SPI=6.5M, min padding SPI cycles is BT=0 MCU=2
#define PADDING_CYCLES                  2
#else
#define PADDING_CYCLES                  0
#endif

#define ANA_READ_CMD(r)                 (((1 << 24) | (((r) & 0xFF) << 16)) << PADDING_CYCLES)
#define ANA_WRITE_CMD(r, v)             (((((r) & 0xFF) << 16) | ((v) & 0xFFFF)) << PADDING_CYCLES)
#define ANA_READ_VAL(v)                 (((v) >> PADDING_CYCLES) & 0xFFFF)
#ifdef ISPI_ARBITRATOR_ENABLE
#define DPD_MEM_READ_VAL(v)             (((v) >> (PADDING_CYCLES - 1)) & 0xFFFF)
#endif

#define ANA_PAGE_0                      (0xA000)
#define ANA_PAGE_1                      (0xA010)
#define ANA_PAGE_2                      (0xA020)
#define ANA_PAGE_3                      (0xA030)
#define ANA_PAGE_4                      (0xA040)
#define ANA_PAGE_QTY                    5

#define ISPI_REG_CS(r)                  ((r) >> 12)
#define ISPI_REG_PAGE(r)                (((r) >> 8) & 0xF)
#define ISPI_REG_OFFSET(r)              ((r) & 0xFF)

static uint8_t BOOT_BSS_LOC ana_cs;
#ifdef ISPI_CHECK_RF_ACCESS
static enum HAL_ANALOGIF_RF_ACCESS_T BOOT_BSS_LOC rf_access;
#endif

static const struct HAL_SPI_CFG_T spi_cfg = {
    .clk_delay_half = false,
    .clk_polarity = false,
    .slave = false,
    .dma_rx = false,
    .dma_tx = false,
    .rx_sep_line = true,
    .cs = 0,
    .rate = 6500000,
    .tx_bits = 25 + PADDING_CYCLES,
    .rx_bits = 25 + PADDING_CYCLES,
    .rx_frame_bits = 0,
};

// Max page number is 4
static const uint16_t page_val[ANA_PAGE_QTY] = { ANA_PAGE_0, ANA_PAGE_1, ANA_PAGE_2, ANA_PAGE_3, ANA_PAGE_4, };

enum HAL_ANALOGIF_RF_ACCESS_T hal_analogif_rf_access_config(enum HAL_ANALOGIF_RF_ACCESS_T type)
{
    enum HAL_ANALOGIF_RF_ACCESS_T old_val = HAL_ANALOGIF_RF_ACCESS_ALL;

#ifdef ISPI_CHECK_RF_ACCESS
    old_val = rf_access;
    rf_access = type;
#endif

    return old_val;
}

__STATIC_FORCEINLINE void hal_analogif_check_rf_access(uint8_t cs, uint8_t page, uint8_t reg)
{
#ifdef ISPI_CHECK_RF_ACCESS
    if (cs == ISPI_RF_CS) {
        ASSERT(rf_access != HAL_ANALOGIF_RF_ACCESS_NONE,
            "RF-ISPI: page=%u reg=0x%X caller=%p",
            page, reg, __builtin_return_address(0));
        ASSERT(rf_access != HAL_ANALOGIF_RF_ACCESS_NO_PAGE || page == 0,
            "RF-ISPI P: page=%u reg=0x%X caller=%p",
            page, reg, __builtin_return_address(0));
    }
#endif
}

static int hal_analogif_rawread(unsigned short reg, unsigned short *val)
{
    int ret;
    unsigned int data;
    unsigned int cmd;

    data = 0;
    cmd = ANA_READ_CMD(reg);
    ret = hal_ispi_recv(&cmd, &data, 4);
    if (ret) {
        return ret;
    }
#ifdef ISPI_ARBITRATOR_ENABLE
    if (ana_cs == ISPI_RF_CS && (reg <= 0x7F)) {
        *val = DPD_MEM_READ_VAL(data);
    }
    else
#endif
    {
        *val = ANA_READ_VAL(data);
    }
    return 0;
}

static int hal_analogif_rawwrite(unsigned short reg, unsigned short val)
{
    int ret;
    unsigned int cmd;

    cmd = ANA_WRITE_CMD(reg, val);
    ret = hal_ispi_send(&cmd, 4);
    if (ret) {
        return ret;
    }
    return 0;
}

static uint8_t hal_analogif_page_switch(uint8_t cs, uint8_t page)
{
    uint8_t page_reg = 0;

    if (cs == ISPI_ANA_CS) {
        page_reg = ISPI_ANA_PAGE_REG;
    } else if (cs == ISPI_RF_CS) {
        page_reg = ISPI_RF_PAGE_REG;
    } else {
        page_reg = ISPI_NORMAL_PAGE_REG;
    }
    hal_analogif_rawwrite(page_reg, page_val[page]);

    return page_reg;
}

static void hal_analogif_page_restore(uint8_t page_reg)
{
    hal_analogif_rawwrite(page_reg, page_val[0]);
}

int hal_analogif_reg_read(unsigned short reg, unsigned short *val)
{
    int ret;
    uint32_t lock;
    uint8_t cs;
    uint8_t page;
    uint8_t page_reg = 0;

    cs = ISPI_REG_CS(reg);
    page = ISPI_REG_PAGE(reg);
    reg = ISPI_REG_OFFSET(reg);

    hal_analogif_check_rf_access(cs, page, reg);

    if (page >= ANA_PAGE_QTY) {
        return -1;
    }

    lock = int_lock();
    if(cs == ISPI_RF_CS && page > 0){
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_BT_RF);
    }
    if (cs != ana_cs) {
        hal_ispi_activate_cs(cs);
        ana_cs = cs;
    }
    if (page) {
        page_reg = hal_analogif_page_switch(cs, page);
    }
    ret = hal_analogif_rawread(reg, val);
    if (page) {
        hal_analogif_page_restore(page_reg);
    }
    if(cs == ISPI_RF_CS && page > 0){
        hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_BT_RF);
    }
    int_unlock(lock);

    return ret;
}

int hal_analogif_reg_write(unsigned short reg, unsigned short val)
{
    int ret;
    uint32_t lock;
    uint8_t cs;
    uint8_t page;
    uint8_t page_reg = 0;

    cs = ISPI_REG_CS(reg);
    page = ISPI_REG_PAGE(reg);
    reg = ISPI_REG_OFFSET(reg);

    hal_analogif_check_rf_access(cs, page, reg);

    if (page >= ANA_PAGE_QTY) {
        return -1;
    }

    lock = int_lock();
    if(cs == ISPI_RF_CS && page > 0){
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_BT_RF);
    }
    if (cs != ana_cs) {
        hal_ispi_activate_cs(cs);
        ana_cs = cs;
    }
    if (page) {
        page_reg = hal_analogif_page_switch(cs, page);
    }
    ret = hal_analogif_rawwrite(reg, val);
    if (page) {
        hal_analogif_page_restore(page_reg);
    }
    if(cs == ISPI_RF_CS && page > 0){
        hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_BT_RF);
    }
    int_unlock(lock);

    return ret;
}

int BOOT_TEXT_FLASH_LOC hal_analogif_open(void)
{
    int ret;

    ret = hal_ispi_open(&spi_cfg);
    if (ret) {
        return ret;
    }

    ana_cs = 0;
    hal_ispi_activate_cs(ana_cs);

#ifndef CHIP_SUBSYS_SENS
    unsigned short chip_id;

    ret = hal_analogif_reg_read(ANA_REG_CHIP_ID, &chip_id);
    if (ret) {
        return ret;
    }

    if (GET_BITFIELD(chip_id, ANA_CHIP_ID) != ANA_VAL_CHIP_ID) {
        return -1;
    }
#endif

    return 0;
}

#endif

