/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#include "main.cpp"
#include "nuttx/init.h"
#include "hal_cmu.h"
#include "hal_iomux.h"

#ifdef __cplusplus
#define EXTERN_C                        extern "C"
#else
#define EXTERN_C                        extern
#endif
EXTERN_C int hal_uart_printf_init(void);

EXTERN_C int __rt_entry(void)
{
    hal_uart_printf_init();
    hal_iomux_set_jtag();
    hal_cmu_jtag_clock_enable();
    nx_start();
    return 0;
}

osThreadDef(main, (osPriorityAboveNormal), 1, (4096), "bes_main");

EXTERN_C __attribute__((weak)) void bes_app_initialize()
{
    osThreadCreate(osThread(main), NULL);
}

EXTERN_C void btdrv_start_bt(void);
EXTERN_C int platform_trace_enable();
#ifdef NUTTX_BUILD
extern "C" bool btdrv_init_ok = false;
#endif
EXTERN_C void bes_chip_init()
{
    tgt_hardware_setup();
#if defined(ROM_UTILS_ON)
    rom_utils_init();
#endif
    hwtimer_init();

    hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
    hal_audma_open();
    hal_gpdma_open();
    platform_trace_enable();
    pmu_open();
    analog_open();
    int ret = mpu_setup(mpu_table, ARRAY_SIZE(mpu_table));
    if (ret) {
        TR_INFO(TR_MOD(MAIN), "Warning, MPU is not setup correctly!!!");
    }
    srand(hal_sys_timer_get());
    btdrv_start_bt();
    btdrv_init_ok = true;   
}


