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
#include "cmsis.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_norflash.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "main_entry.h"
#include "pmu.h"
#include "tool_msg.h"

#include <arm_cmse.h>

#include "partition_ARMCM33.h"
#include "mpc.h"

/* TZ_START_NS: Start address of non-secure application */
#ifndef TZ_START
#define TZ_START (FLASHX_BASE+FLASH_S_SIZE)
#endif

/* typedef for non-secure callback functions */
typedef void (*funcptr_void) (void) __attribute__((cmse_nonsecure_call));

// GDB can set a breakpoint on the main function only if it is
// declared as below, when linking with STD libraries.

#ifdef DEBUG
static void wait_trace_finished(void)
{
    uint32_t time;
    int idle_cnt = 0;

    time = hal_sys_timer_get();

    while (idle_cnt < 2 && hal_sys_timer_get() - time < MS_TO_TICKS(200)) {
        hal_sys_timer_delay(10);
        idle_cnt = hal_trace_busy() ? 0 : (idle_cnt + 1);
    }
}
#endif

int MAIN_ENTRY(void)
{
    int POSSIBLY_UNUSED ret;

    hal_audma_open();
    hal_gpdma_open();
#ifdef DEBUG
#if (DEBUG_PORT == 3)
    hal_iomux_set_analog_i2c();
    hal_iomux_set_uart2();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART2);
#elif (DEBUG_PORT == 2)
    hal_iomux_set_analog_i2c();
    hal_iomux_set_uart1();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#else
    hal_iomux_set_uart0();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
#endif
#endif

#if !defined(SIMU) && !defined(FPGA)
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    hal_norflash_get_id(HAL_FLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TR_INFO(TR_MOD(MAIN), "FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
    hal_norflash_show_calib_result(HAL_FLASH_ID_0);
    ASSERT(hal_norflash_opened(HAL_FLASH_ID_0), "Failed to init flash: %d", hal_norflash_get_open_state(HAL_FLASH_ID_0));
#endif

#ifndef NO_PMU
    ret = pmu_open();
    ASSERT(ret == 0, "Failed to open pmu");
#endif

    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_26M);
    TR_INFO(TR_MOD(MAIN), "CPU freq: %u", hal_sys_timer_calc_cpu_freq(5, 0));

    ret = mpc_init();
    ASSERT(ret==0, "mpc init fail. ret:%d", ret);
    TZ_SAU_Setup();
    __ISB();

    funcptr_void NonSecure_ResetHandler;

    /* Add user setup code for secure part here*/

    /* Set non-secure main stack (MSP_NS) */
    //__TZ_set_MSP_NS(*((uint32_t *)(TZ_START)));

    ///TODO: check SECURE_BOOT
    /* Get non-secure reset handler */
    NonSecure_ResetHandler = (funcptr_void)((uint32_t)(&((struct boot_struct_t *)TZ_START)->hdr + 1));

    if (*(uint32_t *)TZ_START != BOOT_MAGIC_NUMBER) {
        TRACE(0, "nonsec image magic error");
        SAFE_PROGRAM_STOP();
    }
#ifdef DEBUG
    wait_trace_finished();
    hal_trace_close();
#endif

    /* Start non-secure state software application */
    NonSecure_ResetHandler();

    /* Non-secure software does not return, this code is not executed */
    while (1) {
      __NOP();
    }

    SAFE_PROGRAM_STOP();
    return 0;
}

