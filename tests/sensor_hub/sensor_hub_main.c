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
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_mcu2sens.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#ifdef BT_CONN_SLEEP_TEST
#include "hwtest.h"
#endif
#include "hwtimer_list.h"
#include "main_entry.h"
#include "sens_comp_test.h"
#include "sens_trc_to_mcu.h"
#include "sensor_hub_core.h"
#include "sensor_hub_core_app.h"
#ifdef VOICE_DETECTOR_EN
#include "sensor_hub_vad_adapter.h"
#endif
#ifdef SENSOR_TEST
#include "sensor_test.h"
#endif
#ifdef I2S_TEST
#include "hwtest.h"
#endif
#ifdef VD_TEST
#include "voice_detector.h"

void vad_test_env_init(void);
#endif
#ifdef MINIMA_TEST
#include "minima_test.h"
#endif

static sensor_hub_core_rx_irq_handler_t sensorHubCoreRxIrqHandler;
static sensor_hub_core_tx_done_irq_handler_t sensorHubCoreTxDoneIrqHandler;

void sensor_hub_core_register_rx_irq_handler(sensor_hub_core_rx_irq_handler_t irqHandler)
{
    sensorHubCoreRxIrqHandler = irqHandler;
}

void sensor_hub_core_register_tx_done_irq_handler(sensor_hub_core_tx_done_irq_handler_t irqHandler)
{
    sensorHubCoreTxDoneIrqHandler = irqHandler;
}

#ifdef NOAPP
static const uint8_t handshake_msg[] = "Hi";
#endif

#if defined(SLEEP_TEST) && !defined(NO_TIMER)
static HWTIMER_ID wakeup_timer;
static uint32_t wakeup_cnt;
static const uint32_t timer_ms = 11;

static void timer_handler(void *param)
{
    hal_mcu2sens_send(HAL_MCU2SENS_ID_0, (uint8_t *)&wakeup_cnt, sizeof(wakeup_cnt));
    wakeup_cnt++;
    hwtimer_start(wakeup_timer, MS_TO_TICKS(timer_ms));
}
#endif

static unsigned int rx_handler(const void *data, unsigned int len)
{
    if (sensorHubCoreRxIrqHandler) {
        return sensorHubCoreRxIrqHandler(data, len);
    }

    return len;
}

static void tx_handler(const void *data, unsigned int len)
{
    if (sensorHubCoreTxDoneIrqHandler) {
        sensorHubCoreTxDoneIrqHandler(data, len);
        return;
    }
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

#ifdef FULL_WORKLOAD_MODE_ENABLED
static bool isSensorHubInFullWorkloadMode = false;
void sensor_hub_enter_full_workload_mode(void)
{
    isSensorHubInFullWorkloadMode = true;
}

void sensor_hub_exit_full_workload_mode(void)
{
    isSensorHubInFullWorkloadMode = false;
}

static void sensor_hub_full_workload_thread(const void *arg);
osThreadDef(sensor_hub_full_workload_thread, osPriorityNormal, 1,
    (1024), "sensor_hub_full_workload_thread");
static osThreadId sensor_hub_full_workload_thread_id;

static void sensor_hub_full_workload_thread(const void *arg)
{
    while(1)
    {
        if (!isSensorHubInFullWorkloadMode)
        {
            osDelay(1000);
        }
        else
        {
#if defined(VOICE_DETECTOR_EN)
            app_vad_adapter_run_process();
#endif
        }
    }
}

void sensor_hub_start_full_workload_thread(void)
{
    sensor_hub_full_workload_thread_id =
        osThreadCreate(osThread(sensor_hub_full_workload_thread), NULL);
}
#endif

#if (!defined(NOAPP))&&defined(OS_THREAD_TIMING_STATISTICS_ENABLE)
#if defined(KERNEL_RTX5)
extern void rtx_show_all_threads_usage(void);
#else
extern void freertos_show_all_threads(void);
#endif
static void cpu_usage_timer_handler(void const *param);
osTimerDef(cpu_usage_timer, cpu_usage_timer_handler);
static osTimerId cpu_usage_timer_id = NULL;
static void cpu_usage_timer_handler(void const *param)
{
#if defined(KERNEL_RTX5)
    rtx_show_all_threads_usage();
#else
    freertos_show_all_threads();
#endif
}
#endif

#if defined(RTOS) && defined(FAST_TIMER_COMPENSATE)
static void slow_timer_calibration_thread(const void *arg)
{
    int count = 0;
    while(1) {
        hal_sys_timer_calib();
        if (count < 10) {
            //calibrate slow time more often after power up, utils 32K stabilized
            osDelay(500);
            count ++;
        } else {
            osDelay(3000);
        }
    }
}
osThreadDef(slow_timer_calibration_thread, osPriorityBelowNormal, 1, (1024), "slow_timer_calibration_thread");
#endif

int MAIN_ENTRY(void)
{
    int ret;

    hwtimer_init();
    hal_dma_open();

#ifdef SENS_TRC_TO_MCU
    sens_trace_to_mcu();
#endif

#ifdef DEBUG
    enum HAL_TRACE_TRANSPORT_T transport;

#if (DEBUG_PORT == 1)
    hal_iomux_set_uart0();

    transport = HAL_TRACE_TRANSPORT_UART0;
#elif (DEBUG_PORT == 2)
    hal_iomux_set_uart1();

    transport = HAL_TRACE_TRANSPORT_UART1;
#else
    transport = HAL_TRACE_TRANSPORT_QTY;
#endif
    hal_trace_open(transport);
#endif

    hal_sleep_start_stats(10000, 10000);

    ret = hal_mcu2sens_open(HAL_MCU2SENS_ID_0, rx_handler, tx_handler, false);
    ASSERT(ret == 0, "hal_mcu2sens_open failed: %d", ret);

    ret = hal_mcu2sens_start_recv(HAL_MCU2SENS_ID_0);
    ASSERT(ret == 0, "hal_mcu2sens_start_recv failed: %d", ret);

#ifdef NOAPP
    ret = hal_mcu2sens_send(HAL_MCU2SENS_ID_0, handshake_msg, (sizeof(handshake_msg) - 1));
    ASSERT(ret == 0, "hal_mcu2sens_send failed: %d", ret);
#else
    sensor_hub_core_app_init();
#endif

#ifdef MCU2SENS_MSG_TEST
    mcu2sens_msg_test();
#endif

#if defined(SLEEP_TEST) && !defined(NO_TIMER)
    wakeup_timer = hwtimer_alloc(timer_handler, NULL);
    ASSERT(wakeup_timer, "Failed to alloc wakeup_timer");

    ret = hwtimer_start(wakeup_timer, MS_TO_TICKS(timer_ms));
#endif

#ifdef BT_CONN_SLEEP_TEST
    bt_conn_sleep_test();
#endif

#ifdef I2S_TEST
    i2s_test();
#endif

#ifdef SENSOR_TEST
    sensor_test();
#endif

#ifdef VAD_CODEC_TEST
    vad_codec_test();
#endif

#ifdef VD_TEST
    vad_test_env_init();
    voice_detector_test();
#endif

#if (!defined(NOAPP))&&defined(OS_THREAD_TIMING_STATISTICS_ENABLE)
    cpu_usage_timer_id = osTimerCreate(osTimer(cpu_usage_timer), osTimerPeriodic, NULL);
    if (cpu_usage_timer_id != NULL) {
        osTimerStart(cpu_usage_timer_id, OS_THREAD_TIMING_STATISTICS_PEROID_MS);
    }
#endif

#ifdef FULL_WORKLOAD_MODE_ENABLED
    sensor_hub_start_full_workload_thread();
#endif

#if defined(RTOS) && defined(FAST_TIMER_COMPENSATE)
    osThreadCreate(osThread(slow_timer_calibration_thread), NULL);
#endif

#if defined(SLEEP_TEST) || !defined(NOAPP)
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
#else
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_26M);
#endif
    hal_sysfreq_print();

#ifdef MINIMA_TEST
    minima_test();
#endif

    while (1) {
#ifdef NOAPP
#ifdef RTOS
        osDelay(2);
#else
        hal_sleep_enter_sleep();
#endif
#else
        osSignalWait(0x0, osWaitForever);
#endif
    }

    return 0;
}
