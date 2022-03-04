#include "pmu.h"
#include "cmsis.h"
#include "stdio.h"
#include "hal_pmu.h"
#include "cmsis_os.h"
#include "hal_error.h"
#include "hal_timer.h"
#include "xspace_interface.h"

#define HAL_PMU_REGISTER_CB_CNT                 (5)

static hal_pmu_charger_status_register_cb hal_pmu_status_change_cb_queue[HAL_PMU_REGISTER_CB_CNT];
static hal_pmu_status_e hal_pmu_status = HAL_PMU_UNKNOWN;

int32_t hal_pmu_add_charger_status_change_cb(hal_pmu_charger_status_register_cb cb)
{
    int32_t i;
    int32_t empty_index = HAL_PMU_REGISTER_CB_CNT;
    int32_t found_index = HAL_PMU_REGISTER_CB_CNT;

    if (NULL == cb) {
        return -1;
    }

    for(i = 0; i < HAL_PMU_REGISTER_CB_CNT; i++) {
        if(hal_pmu_status_change_cb_queue[i] == NULL && HAL_PMU_REGISTER_CB_CNT == empty_index) {
            empty_index = i;
        }

        if(hal_pmu_status_change_cb_queue[i] == cb) {
            found_index = i;
            break;
        }
    }

    if(found_index != HAL_PMU_REGISTER_CB_CNT) {
        HAL_PMU_TRACE(0, " register cb has been added!!!");
        return 0;
    }

    if(empty_index == HAL_PMU_REGISTER_CB_CNT) {
        HAL_PMU_TRACE(0, " No space to add cb!!!");
        return 0;
    }

    hal_pmu_status_change_cb_queue[empty_index] = cb;

    return 0;
}

int32_t hal_pmu_remove_charger_status_change_cb(hal_pmu_charger_status_register_cb cb)
{
    int32_t i;

    if (NULL == cb)
        return -1;

    for(i = 0; i < HAL_PMU_REGISTER_CB_CNT; i++) {
        if(hal_pmu_status_change_cb_queue[i] == cb) {
            hal_pmu_status_change_cb_queue[i] = NULL;
        }
    }

    return 0;
}

static int32_t hal_pmu_call_charger_status_change_cb(hal_pmu_status_e status)
{
    int32_t i;

    for(i = 0; i < HAL_PMU_REGISTER_CB_CNT; i++) {
        if(hal_pmu_status_change_cb_queue[i] != NULL) {
            hal_pmu_status_change_cb_queue[i](status);
        }
    }

    return 0;
}

hal_pmu_status_e hal_pmu_force_get_charger_status(void)
{
    hal_pmu_status_e status = HAL_PMU_UNKNOWN;
    enum PMU_CHARGER_STATUS_T charger;

    charger = pmu_charger_get_status();

    if (PMU_CHARGER_PLUGIN == charger) {
        status = HAL_PMU_PLUGIN;
        HAL_PMU_TRACE(0, " PMU_CHARGER_PLUGIN");
    } else if (PMU_CHARGER_PLUGOUT == charger) {
        status = HAL_PMU_PLUGOUT;
        HAL_PMU_TRACE(0, " PMU_CHARGER_PLUGOUT");
    } else {
        status = HAL_PMU_UNKNOWN;
        HAL_PMU_TRACE(0, " PMU_CHARGER_UNKNOWN");
    }

    return status;
}

/////////////////////////pmu_int_event begain/////////////////////////////////////////////////
static uint8_t hal_pmu_int_timerout;
static void hal_pmu_call_charger_timeout_handler(void);
static void hal_pmu_call_charger_timer_reset(void)
{
	xspace_interface_delay_timer_stop((uint32_t)hal_pmu_call_charger_timeout_handler);
	xspace_interface_delay_timer_start(50, (uint32_t)hal_pmu_call_charger_timeout_handler, 0, 0, 0);
}
static void hal_pmu_call_charger_timeout_handler(void)
{
    if(hal_pmu_int_timerout > 0)
    {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)hal_pmu_call_charger_timer_reset, 0, 0, 0);
        hal_pmu_int_timerout = 0;
    }
    else
    {
        hal_pmu_call_charger_status_change_cb(hal_pmu_force_get_charger_status());
    }
}
static void hal_pmu_call_charger_event_update(void)
{
    if(hal_pmu_int_timerout == 0)
    {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)hal_pmu_call_charger_timer_reset, 0, 0, 0);
    }
    hal_pmu_int_timerout = 1;
}
//////////////////////////pmu_int_event end/////////////////////////////////////////

static void hal_pmu_charger_handler(enum PMU_CHARGER_STATUS_T status)
{
/*
    static PMU_CHARGER_STATUS_T local_status = PMU_CHARGER_UNKNOWN;


    if (status == local_status) {
        //HAL_PMU_TRACE(2, " INT repeat!!! status=%d. local_status:%d", status, local_status);
        return;
    }
*/
    if (PMU_CHARGER_PLUGIN == status) {
        hal_pmu_status = HAL_PMU_PLUGIN;
    } else if (PMU_CHARGER_PLUGOUT == status) {
        hal_pmu_status = HAL_PMU_PLUGOUT;
    } else {
        hal_pmu_status = HAL_PMU_UNKNOWN;
        return;
    }
    
    HAL_PMU_TRACE(1, " status=%d(0:plugin,1:plugout,2:unknown)\n", status);

    uint32_t lock = int_lock();
    hal_pmu_call_charger_event_update();
	//hal_pmu_call_charger_status_change_cb(hal_pmu_status);
    int_unlock(lock);
}

static void hal_pmu_config_pmu_irq_cb(bool status)
{
    if (status == true) {
        pmu_charger_set_irq_handler(hal_pmu_charger_handler);
        HAL_PMU_TRACE(0, " Enable pmu irq\n");
    } else {
        pmu_charger_set_irq_handler(NULL);
        HAL_PMU_TRACE(0, " Disable pmu irq\n");
    }
}

static int hal_pmu_open(void)
{
    hal_pmu_status_e status = HAL_PMU_UNKNOWN;
    uint8_t cnt = 0;

    do {
        status = hal_pmu_force_get_charger_status();
        if (status == HAL_PMU_PLUGIN)
            break;
        osDelay(20);
    } while(cnt++ < 10);

    hal_pmu_status = status;

    return status;
}

void hal_pmu_init(void)
{
    static bool init_done = false;

    if (init_done) {
        HAL_PMU_TRACE(0, " Already Init Done!!!");
        return;
    }

    init_done = true;
    memset((void *)hal_pmu_status_change_cb_queue, 0x00, sizeof(hal_pmu_status_change_cb_queue));
    pmu_charger_init();
    hal_pmu_open();
    hal_pmu_config_pmu_irq_cb(true);
}

