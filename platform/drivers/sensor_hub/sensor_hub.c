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
#include "sensor_hub.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_mcu2sens.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rx_sens_trc.h"
#include "string.h"

extern uint32_t __sensor_hub_code_start_flash[];
extern uint32_t __sensor_hub_code_end_flash[];

int sensor_hub_open(SENSOR_HUB_RX_IRQ_HANDLER rx_hdlr, SENSOR_HUB_TX_IRQ_HANDLER tx_hdlr)
{
    uint32_t code_start;
    struct HAL_DMA_CH_CFG_T dma_cfg;
    enum HAL_DMA_RET_T dma_ret;
    uint32_t remains;
    int ret;
    uint32_t entry;
    uint32_t sp;

    TR_INFO(0, "%s", __FUNCTION__);

    if (__sensor_hub_code_end_flash - __sensor_hub_code_start_flash < 2) {
        return 1;
    }

    hal_psc_sens_enable();
    hal_cmu_sens_clock_enable();
    hal_cmu_sens_reset_clear();

    osDelay(2);

    code_start = *(__sensor_hub_code_end_flash - 1);
    remains = __sensor_hub_code_end_flash - __sensor_hub_code_start_flash - 1;

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.ch = hal_dma_get_chan(dma_cfg.dst_periph, HAL_DMA_LOW_PRIO);
    if (dma_cfg.ch == HAL_DMA_CHAN_NONE) {
        memcpy((void *)code_start, __sensor_hub_code_start_flash, remains * sizeof(__sensor_hub_code_start_flash[0]));
    } else {
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.dst_periph = HAL_GPDMA_MEM;
        dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.handler = NULL;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
        dma_cfg.src_periph = HAL_GPDMA_MEM;
        dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.type = HAL_DMA_FLOW_M2M_DMA;
        dma_cfg.try_burst = false;

        dma_cfg.dst = code_start;
        dma_cfg.src = (uint32_t)__sensor_hub_code_start_flash;

        while (remains > 0) {
            if (remains > HAL_DMA_MAX_DESC_XFER_SIZE) {
                dma_cfg.src_tsize = HAL_DMA_MAX_DESC_XFER_SIZE;
            } else {
                dma_cfg.src_tsize = remains;
            }
            dma_ret = hal_dma_start(&dma_cfg);
            ASSERT(dma_ret == HAL_DMA_OK, "%s: Failed to start dma: %d", __func__, dma_ret);

            while (hal_dma_chan_busy(dma_cfg.ch));

            dma_cfg.dst += dma_cfg.src_tsize * 4;
            dma_cfg.src += dma_cfg.src_tsize * 4;
            remains -= dma_cfg.src_tsize;
        }

        hal_dma_free_chan(dma_cfg.ch);
    }

#if (SENS_RAMRET_BASE < (RAM8_BASE + RAM8_SIZE)) || (SENS_RAM_BASE < (RAM8_BASE + RAM8_SIZE))
    const uint32_t ram_base_list[] = { RAM4_BASE, RAM5_BASE, RAM6_BASE, RAM7_BASE, RAM8_BASE, RAMSENS_BASE, };
    const uint32_t sens_ram_start = SENS_RAMRET_BASE;
    const uint32_t sens_ram_end = sens_ram_start + SENS_RAMRET_SIZE + SENS_RAM_SIZE;
    const uint8_t ram_first_id = 4;
    uint32_t ram_map = 0;
    bool ram_inside = false;

    for (int i = 0; i < ARRAY_SIZE(ram_base_list) - 1; i++) {
        ram_inside = false;
        if (ram_base_list[i] <= sens_ram_start && sens_ram_start < ram_base_list[i + 1]) {
            ram_inside = true;
        } else if (ram_base_list[i] < sens_ram_end && sens_ram_end <= ram_base_list[i + 1]) {
            ram_inside = true;
        } else if (sens_ram_start <= ram_base_list[i] && ram_base_list[i + 1] < sens_ram_end) {
            ram_inside = true;
        }
        if (ram_inside) {
            ram_map |= (1 << (ram_first_id + i));
        }
    }
    hal_cmu_ram_cfg_sel_update(ram_map, HAL_CMU_RAM_CFG_SEL_SENS);
    TR_INFO(0, "%s: Set sensor ram map: 0x%X", __FUNCTION__, ram_map);
#endif

#ifdef ISPI_SENS_EXTPMU
    hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_SENS_PMU);
#endif
#ifdef ISPI_SENS_ANA
    hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_SENS_ANA);
#endif
#ifdef ISPI_SENS_RF
    hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_SENS_RF);
#endif
#ifdef ISPI_SENS_ITNPMU
    hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_SENS_PMU1);
#endif
#ifdef ISPI_SENS_DPA
    hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_SENS_DPA);
#endif

    ret = hal_mcu2sens_open(HAL_MCU2SENS_ID_0, rx_hdlr, tx_hdlr, false);
    ASSERT(ret == 0, "hal_mcu2sens_open failed: %d", ret);

    ret = hal_mcu2sens_start_recv(HAL_MCU2SENS_ID_0);
    ASSERT(ret == 0, "hal_mcu2sens_start_recv failed: %d", ret);

#ifdef SENS_TRC_TO_MCU
    rx_sensor_hub_trace();
#endif

    sp = SENS_NORM_MAILBOX_BASE;
    entry = RAM_TO_RAMX(code_start | 1);

    hal_cmu_sens_start_cpu(sp, entry);

    return 0;
}

int sensor_hub_close(void)
{
    TR_INFO(0, "%s", __FUNCTION__);

    hal_mcu2sens_close(HAL_MCU2SENS_ID_0);

    hal_cmu_sens_reset_set();
    hal_cmu_sens_clock_disable();
    hal_psc_sens_disable();

    hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_SENS_PMU | HAL_IOMUX_ISPI_SENS_ANA |
        HAL_IOMUX_ISPI_SENS_RF | HAL_IOMUX_ISPI_SENS_PMU1 | HAL_IOMUX_ISPI_SENS_DPA);

    return 0;
}

int sensor_hub_send_seq(const void *data, unsigned int len, unsigned int *seq)
{
    return hal_mcu2sens_send_seq(HAL_MCU2SENS_ID_0, data, len, seq);
}

int sensor_hub_send(const void *data, unsigned int len)
{
    return hal_mcu2sens_send(HAL_MCU2SENS_ID_0, data, len);
}

int sensor_hub_tx_active(unsigned int seq)
{
    return hal_mcu2sens_tx_active(HAL_MCU2SENS_ID_0, seq);
}

