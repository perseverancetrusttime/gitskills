#if defined(__XSPACE_INEAR_DETECT_MANAGER__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_pmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_touch.h"
#include "dev_thread.h"
#include "xspace_interface.h"
#include "xspace_inear_detect_manager.h"
#include "xspace_inout_box_manager.h"

static inear_detect_manager_env_s inear_detect_manager_env = {
    .detect_status = false,
    .inear_status = INEAR_DETECT_MG_UNKNOWN,
};

static inear_detect_manager_status_change_func   inear_detect_manager_status_change_cb = NULL;
static inear_detect_manager_ui_need_execute_func inear_detect_manager_ui_need_execute = NULL;

static void xspace_inear_detect_manager_timeout_handler(void const *param);
osTimerDef (INEAR_DETECT_MANAGE_DEBOUNCE_TIMER, xspace_inear_detect_manager_timeout_handler);
static osTimerId inear_detect_manager_debounce_timer = NULL;


bool xspace_inear_detect_manager_is_inear(void)
{
    if (INEAR_DETECT_MG_IN_EAR == inear_detect_manager_env.inear_status &&
        xspace_inear_detect_manager_is_workon()) {
        return true;
    }

    return false;
}

bool xspace_inear_detect_manager_is_workon(void)
{
    return inear_detect_manager_env.detect_status;
}

inear_detect_manager_status_e xspace_inear_detect_manager_inear_status(void)
{
    return inear_detect_manager_env.inear_status;
}

static void xspace_inear_detect_manager_start_if(void)
{
    if(inear_detect_manager_env.detect_status) {
        INEAR_TRACE(0, " Inear detect manager is already started !!!");
        return;
    }

    inear_detect_manager_env.detect_status = true;
    osTimerStop(inear_detect_manager_debounce_timer);

#if defined(__INEAR_DETECTION_USE_IR__)
    hal_infrared_start_measure();
#endif

#if defined(__INEAR_DETECTION_USE_TOUCH__)
    hal_touch_start_inear_detection();
#endif

    INEAR_TRACE(0, " Inear detect Manager Start !!!");
}

void xspace_inear_detect_manager_start(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_inear_detect_manager_start_if, 0, 0, 0);
}

static void xspace_inear_detect_manager_stop_if(void)
{
    if(!inear_detect_manager_env.detect_status) {
        INEAR_TRACE(0, "Wear manage is already stoped !!!");
        return;
    }

    inear_detect_manager_env.detect_status = false;
    osTimerStop(inear_detect_manager_debounce_timer);

#if defined(__INEAR_DETECTION_USE_IR__)
    hal_infrared_stop_measure();
#endif

#if defined(__INEAR_DETECTION_USE_TOUCH__)
    hal_touch_stop_inear_detection();
#endif

    INEAR_TRACE(0, "Wear manage Stop !!!");
}

void xspace_inear_detect_manager_stop(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_inear_detect_manager_stop_if, 0, 0, 0);
}

static void xspace_inear_detect_manager_timeout_handler(void const *param)
{
    if (NULL != inear_detect_manager_ui_need_execute) {
        if (!inear_detect_manager_ui_need_execute(inear_detect_manager_env.inear_status)) {
            return;
        }
    }

    if (!inear_detect_manager_env.detect_status) {
        INEAR_TRACE(0, " inear detect manager stop work!!!");
        return;
    }

    if(INEAR_DETECT_MG_DETACH_EAR == inear_detect_manager_env.inear_status) {

    } else if(INEAR_DETECT_MG_IN_EAR == inear_detect_manager_env.inear_status) {

    } else {
        INEAR_TRACE(1, " Invaild Event!!!");
        return ;
    }

    INEAR_TRACE(1, " Inear_status:%s!!!",
        inear_detect_manager_env.inear_status == INEAR_DETECT_MG_IN_EAR ? ("In_ear"):("Detech_ear"));

    if(NULL == inear_detect_manager_status_change_cb) {
        return;
    }

    xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
        (uint32_t)inear_detect_manager_status_change_cb, inear_detect_manager_env.inear_status, 0, 0);
}

void xspace_inear_detect_manager_register_ui_cb(inear_detect_manager_status_change_func cb)
{
    if (NULL == cb) {
        return;
    }

    inear_detect_manager_status_change_cb = cb;
}

static void xspace_inear_detect_manager_status_changed_cb(hal_touch_inear_status_e status)
{
    if (!inear_detect_manager_env.detect_status) {
        INEAR_TRACE(0, "App wear manage stop work!!!");
        return;
    }

    uint32_t debounce_time = 0;
    INEAR_TRACE(0, " status:%d", status);
    if(HAL_TOUCH_DETACH_EAR == status) {
        inear_detect_manager_env.inear_status = INEAR_DETECT_MG_DETACH_EAR;
        debounce_time = INEAR_DETECT_MANAGER_EAR_DETECH_DEBOUNCE_MS;
    } else if(HAL_TOUCH_IN_EAR == status) {
        inear_detect_manager_env.inear_status = INEAR_DETECT_MG_IN_EAR;
        debounce_time = INEAR_DETECT_MANAGER_EAR_IN_DEBOUNCE_MS;
    }

    osTimerStop(inear_detect_manager_debounce_timer);
    osTimerStart(inear_detect_manager_debounce_timer, debounce_time);
}

#if defined(__INEAR_DETECTION_USE_IR__)
static int32_t xspace_inear_detect_manager_ir_init(void)
{
    int32_t ret = 0;
    int32_t retry_count = 0;
    uint8_t ir_5mmValue = 0;
    uint8_t ir_12mmValue = 0;
    uint8_t ir_25mmValue = 0;
    uint8_t ir_nullValue = 0;

    while (retry_count < 5) {
        ret = hal_infrared_init();
        if (HAL_RUN_SUCCESS != ret) {
            osDelay(100);
            retry_count++;
            INEAR_TRACE(2, " Init failed,%d,%d.", ret, retry_count);
        } else {
            break;
        }
    }

    if (HAL_RUN_SUCCESS != ret) {
        INEAR_TRACE(0, " Retry Init faild!!!");
        return -1;
    }

    xspace_section_get_ir_calib_value(PT_IR_MOVE, &ir_25mmValue);
    xspace_section_get_ir_calib_value(PT_IR_NEAR, &ir_12mmValue);
    xspace_section_get_ir_calib_value(PT_IR_5MM,  &ir_5mmValue);
    xspace_section_get_ir_calib_value(PT_IR_NULL, &ir_nullValue);
    ret = hal_infrared_auto_calib(ir_25mmValue, ir_12mmValue, ir_5mmValue, ir_nullValue);
    if (ret != 0) {
        INEAR_TRACE(0, "Infrared auto calib fail, used default calib value!!!");
    }

    hal_infrared_set_status_changed_cb(xspace_inear_detect_manager_status_changed_cb);

    return 0;
}
#endif

#if defined(__INEAR_DETECTION_USE_TOUCH__)
static int32_t xspace_inear_detect_manager_touch_reset(void)
{
    INEAR_TRACE(0, "enter function");
    hal_touch_reset();

    return 0;
}

int32_t xspace_inear_detect_manager_reset(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_inear_detect_manager_touch_reset, 0, 0, 0);

    return 0;
}

static int32_t xspace_inear_detect_manager_touch_init(void)
{
    int32_t ret = 0;
    int32_t retry_count = 0;

    while (retry_count < 5) {
        ret = hal_touch_init();
        if (HAL_RUN_SUCCESS != ret) {
            osDelay(100);
            retry_count++;
            INEAR_TRACE(2, "Init Touch failed,%d,%d.", ret, retry_count);
        } else {
            break;
        }
    }

    if (HAL_RUN_SUCCESS != ret) {
        INEAR_TRACE(0, "Retry Touch Init faild!!!");
        return -1;
    }

    hal_touch_register_inear_status_changed_cb(xspace_inear_detect_manager_status_changed_cb);

    return 0;
}

#endif

void xspace_inear_off_set_self_cali(void)
{
#if defined(__INEAR_DETECTION_USE_TOUCH__)
	hal_touch_off_set_self_cali();
#endif
}

void xspace_inear_detect_manager_register_ui_need_execute_cb(inear_detect_manager_ui_need_execute_func cb)
{
    if (NULL == cb) {
        return;
    }

    inear_detect_manager_ui_need_execute = cb;
}

#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
extern "C" uint8_t gh621xenablePowerOnOffset;
static void xspace_inear_detect_out_box_start_check(void)
{
	static uint16_t check_cnt = 0;
    INEAR_TRACE(0, "/n");
	if (hal_pmu_force_get_charger_status() == HAL_PMU_PLUGOUT){
		check_cnt++;
		if(check_cnt >= 4)
		{
			gh621xenablePowerOnOffset = false;
	        //xspace_inear_detect_manager_start();
			xspace_interface_delay_timer_stop((uint32_t)xspace_inear_detect_manager_start);
			xspace_interface_delay_timer_start(50, (uint32_t)xspace_inear_detect_manager_start, 0, 0, 0);
			return;
		}
    }
	else{
		check_cnt = 0;
	}
	
	if(XIOB_UNKNOWN == xspace_inout_box_manager_get_curr_status())
	{
		xspace_interface_delay_timer_stop((uint32_t)xspace_inear_detect_out_box_start_check);
		xspace_interface_delay_timer_start(100, (uint32_t)xspace_inear_detect_out_box_start_check, 0, 0, 0);
	}

}
#endif

static int32_t xspace_inear_detect_manager_init_if(void)
{
    int32_t ret = -1;

#if defined(__INEAR_DETECTION_USE_TOUCH__)
    ret = xspace_inear_detect_manager_touch_init();
#endif

#if defined(__INEAR_DETECTION_USE_IR__)
    ret = xspace_inear_detect_manager_ir_init();
#endif

    if (ret) {
        INEAR_TRACE(0, " Inear detect manager IC init Fail!!!");
        return ret;
    }

    if (NULL == inear_detect_manager_debounce_timer) {
        inear_detect_manager_debounce_timer = osTimerCreate (osTimer(INEAR_DETECT_MANAGE_DEBOUNCE_TIMER), osTimerOnce, NULL);
    }
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
	xspace_interface_delay_timer_stop((uint32_t)xspace_inear_detect_out_box_start_check);
	xspace_interface_delay_timer_start(20, (uint32_t)xspace_inear_detect_out_box_start_check, 0, 0, 0);
#endif
    return ret;
}


int32_t xspace_inear_detect_manager_init(void)
{
    INEAR_TRACE(0, "xspace_inear_detect_manager_init");
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_inear_detect_manager_init_if, 0, 0, 0);

    return 0;
}

#endif  // (XSPACE_INEAR_DETECTION_MANAGER)

