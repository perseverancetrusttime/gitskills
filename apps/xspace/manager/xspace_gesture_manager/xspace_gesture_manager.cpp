#if defined(__XSPACE_GESTURE_MANAGER__)
#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_error.h"
#include "hal_touch.h"
#include "hal_pressure.h"
#include "dev_thread.h"
#include "xspace_interface.h"
#include "xspace_gesture_manager.h"

static xgm_event_func xgm_event_handle = NULL;
static xgm_work_status_e xgm_work_status = XGM_MODE_UNKNOWN;
static xgm_ui_need_execute_func xgm_ui_need_execute_handle = NULL;

static int32_t xspace_gesture_manage_revise_sensibility_if(uint8_t grade)
{
    int32_t ret = HAL_RUN_FAIL;

    XGM_TRACE(1, " revise sensibility grade=%d", grade);

#if defined(__GESTURE_MANAGER_USE_ACC__)
    ret = hal_accelerate_set_sensitivity(grade);
#endif

#if defined(__GESTURE_MANAGER_USE_TOUCH__)
    ret = hal_touch_set_sensibility_grade(grade);
#endif

    if(HAL_RUN_SUCCESS == ret ) {
        return 0;
    } else {
        XGM_TRACE(1, "ret=%d", ret);
        return -1;
    }
}

static void xspace_gesture_manage_enter_standby_mode_if(void)
{
    if (XGM_STANDBY_MODE == xgm_work_status) {
        XGM_TRACE(0, " Already enter standby mode!!!");
        return;
    }

    XGM_TRACE(0, " Enter standby mode");

    xgm_work_status = XGM_STANDBY_MODE;

#if defined(__GESTURE_MANAGER_USE_ACC__)
    hal_accelerate_enter_standby_mode();
#endif

#if defined(__GESTURE_MANAGER_USE_TOUCH__)
    hal_touch_enter_standby_mode();
#endif

#if defined(__GESTURE_MANAGER_USE_PRESSURE__)
	hal_pressure_enter_standby_mode();
#endif
}

static void xspace_gesture_manage_enter_detection_mode_if(void)
{
    if (XGM_DETECTION_MODE == xgm_work_status) {
        XGM_TRACE(0, " Already enter detection mode!!!!!!");
        return ;
    }

    XGM_TRACE(0, " Gesture enter detection mode");

    xgm_work_status = XGM_DETECTION_MODE;
#if defined (__GESTURE_MANAGER_USE_ACC__)
    hal_accelerate_enter_normal_mode();
#endif
#if defined(__GESTURE_MANAGER_USE_TOUCH__)
    hal_touch_enter_detection_mode();
#endif

#if defined(__GESTURE_MANAGER_USE_PRESSURE__)
	hal_pressure_enter_detection_mode();
#endif
}

static void xspace_gesture_manage_event_process(xgm_event_type_e event_type, uint8_t *data, uint16_t len)
{
    XGM_TRACE(1, " event_type:%d", event_type);

    if (NULL != xgm_ui_need_execute_handle) {
        if (!xgm_ui_need_execute_handle(event_type, data, len)) {
            return;
        }
    }

    if (NULL != xgm_event_handle) {
        xgm_event_handle(event_type, data, len);
    } else {
        XGM_TRACE(0, " UI callback is not exit!!!");
    }

    if(APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA != event_type)
    {
        if (XGM_STANDBY_MODE == xgm_work_status) {
            XGM_TRACE(0, " Current standby mode, return!!!");
            return;
        }
    }
}

#if defined (__GESTURE_MANAGER_USE_ACC__)
static void xspace_gesture_manage_aac_event_cb(hal_acc_event_type_enum event_type, uint8_t *data, uint16_t len)
{
    xgm_event_type_e app_gesture_event = XGM_EVENT_NONE;

    switch (event_type)
    {
        case HAL_ACC_EVENT_CLICK:
            XGM_TRACE(0, "HAL_ACC_EVENT_CLICK!!!");
            app_gesture_event = XGM_EVENT_CLICK;
            break;

        case HAL_ACC_EVENT_DOUBLE_CLICK:
            XGM_TRACE(0, "HAL_ACC_EVENT_DOUBLE_CLICK!!!");
            app_gesture_event = XGM_EVENT_DOUBLE_CLICK;
            break;

        case HAL_ACC_EVENT_THREE_CLICK:
            XGM_TRACE(0, "HAL_ACC_EVENT_THREE_CLICK!!!");
            app_gesture_event = XGM_EVENT_TRIPLE_CLICK;
            break;

        case HAL_ACC_EVENT_FOUR_CLICK:
            XGM_TRACE(0, "_ACC_EVENT_FOUR_CLICK!!!" );
            app_gesture_event = XGM_EVENT_ULTRA_CLICK;
            break;

        case HAL_ACC_EVENT_FIVE_CLICK:
            XGM_TRACE(0, "HAL_ACC_EVENT_FIVE_CLICK!!!");
            app_gesture_event = XGM_EVENT_RAMPAGE_CLICK;
            break;

        case HAL_ACC_EVENT_LONGPRESS:
            XGM_TRACE(0, "HAL_ACC_EVENT_LONGPRESS!!!");
            app_gesture_event = XGM_EVENT_LONGPRESS;
            break;

        case HAL_ACC_EVENT_FIFOFULL:
            XGM_TRACE(0, "HAL_ACC_EVENT_FIFOFULL!!!");
            app_gesture_event = APP_GESTURE_MANAGE_EVENT_SEND_ACC_DATA;
            break;

        default:

            XGM_TRACE(0, "Uknown event!!!");
            break;
    }

    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL,
        (uint32_t)xspace_gesture_manage_event_process, app_gesture_event, (uint32_t)data, (uint32_t)len);
}

static int32_t xspace_gesture_manage_acc_init(void)
{
    int32_t ret = 0;
    int32_t retry_count = 0;
    while(retry_count < 5) {
        ret = hal_accelerate_init();
        if (ret < 0) {
            osDelay(100);
            retry_count++;
            XGM_TRACE(1, "Init ACC failed,%d,%d.", ret, retry_count);
        } else {
            break;
        }
    }

    if(ret < 0) {
        XGM_TRACE(0, "Init ACC Retry Failed!!!");
        return -1;
    }

    hal_accelerate_set_event_cb(app_gesture_manage_aac_event_cb);

    return 0;
}
#endif

#if defined (__GESTURE_MANAGER_USE_TOUCH__)
static void xspace_gesture_manage_touch_event_cb(hal_touch_event_e event_type, uint8_t *data, uint16_t len)
{
    xgm_event_type_e xgm_event = XGM_EVENT_NONE;

/*
    if (APP_GESTURE_MANAGE_STANDBY == app_gesture_manage_work_status)
        return;
*/

    switch (event_type)
    {
        case HAL_TOUCH_EVENT_CLICK:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_CLICK!!!");
            xgm_event = XGM_EVENT_CLICK;
            break;

        case HAL_TOUCH_EVENT_DOUBLE_CLICK:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_DOUBLE_CLICK!!!");
            xgm_event = XGM_EVENT_DOUBLE_CLICK;
            break;

        case HAL_TOUCH_EVENT_TRIPLE_CLICK:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_THREE_CLICK!!!");
            xgm_event = XGM_EVENT_TRIPLE_CLICK;
            break;

        case HAL_TOUCH_EVENT_ULTRA_CLICK:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_FOUR_CLICK!!!" );
            xgm_event = XGM_EVENT_ULTRA_CLICK;
            break;

        case HAL_TOUCH_EVENT_RAMPAGE_CLICK:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_FIVE_CLICK!!!");
            xgm_event = XGM_EVENT_RAMPAGE_CLICK;
            break;

        case HAL_TOUCH_EVENT_LONGPRESS:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_LONGPRESS!!!");
            xgm_event = XGM_EVENT_LONGPRESS;
            break;
        case HAL_TOUCH_EVENT_LONGPRESS_3S:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_LONGPRESS_3S!!!");
            xgm_event = XGM_EVENT_LONGLONGPRESS;
            break;
        case HAL_TOUCH_EVENT_LONGPRESS_5S:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_LONGPRESS_5S!!!");
            xgm_event = XGM_EVENT_LONGLONGPRESS;
            break;

        case HAL_TOUCH_EVENT_UP_SLIDE:
            XGM_TRACE(0, "APP_GESTURE_MANAGE_EVENT_UP_SLIDE!!!");
            xgm_event = XGM_EVENT_UP_SLIDE;
        	break;

        case HAL_TOUCH_EVENT_DOWN_SLIDE:
            XGM_TRACE(0, "APP_GESTURE_MANAGE_EVENT_DOWN_SLIDE!!!");
            xgm_event = XGM_EVENT_DOWN_SLIDE;
            break;

        case HAL_TOUCH_EVENT_SEND_DATA:
            XGM_TRACE(0, "APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA!!!");
            xgm_event = APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA;
            break;
			
        case HAL_TOUCH_EVENT_PRESS:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_PRESS!!!");
            xgm_event = XGM_EVENT_DOWN;
            break;

        case HAL_TOUCH_EVENT_RELEASE:
            XGM_TRACE(0, "HAL_TOUCH_EVENT_RELEASE!!!");
            xgm_event = XGM_EVENT_UP;
            break;
        default:
            XGM_TRACE(0, "Uknown event!!!");
            break;
    }

    xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
        (uint32_t)xspace_gesture_manage_event_process, (uint32_t)xgm_event, (uint32_t)data, (uint32_t)len);
}

static int32_t xspace_gesture_manage_touch_init(void)
{
    int32_t ret = 0;
    int32_t retry_count = 0;

    XGM_TRACE(0, " >>>>> ");

    while(retry_count < 5) {
        ret = hal_touch_init();
        if (HAL_RUN_SUCCESS != ret) {
            osDelay(100);
            retry_count++;
            XGM_TRACE(1, " Touch Init failed,%d,%d.", ret, retry_count);
        } else {
            break;
        }
    }

    if(ret != HAL_RUN_SUCCESS) {
        XGM_TRACE(0, " Touch Init Retry Failed!!!");
        return -1;
    }

    hal_touch_register_gesture_generate_cb(xspace_gesture_manage_touch_event_cb);

    return 0;
}
#endif

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
int32_t xspace_gesture_manage_pressure_cali(uint8_t items,uint8_t *data)
{
    return hal_pressure_enter_pt_calibration(items, data);
}

static void xspace_gesture_manage_pressure_event_cb(hal_pressure_event_e event_type, uint8_t *data, uint16_t len)
{
    xgm_event_type_e xgm_event = XGM_EVENT_NONE;

/*
    if (APP_GESTURE_MANAGE_STANDBY == app_gesture_manage_work_status)
        return;
*/

    switch (event_type)
    {
        case HAL_PRESSURE_EVENT_CLICK:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_CLICK!!!");
            xgm_event = XGM_EVENT_CLICK;
            break;

        case HAL_PRESSURE_EVENT_DOUBLE_CLICK:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_DOUBLE_CLICK!!!");
            xgm_event = XGM_EVENT_DOUBLE_CLICK;
            break;

        case HAL_PRESSURE_EVENT_THREE_CLICK:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_THREE_CLICK!!!");
            xgm_event = XGM_EVENT_TRIPLE_CLICK;
            break;

        case HAL_PRESSURE_EVENT_LONGPRESS_1S:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_LONGPRESS_1S!!!");
            xgm_event = XGM_EVENT_LONGPRESS;
            break;

        case HAL_PRESSURE_EVENT_LONGPRESS_3S:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_LONGPRESS_3S!!!");
            xgm_event = XGM_EVENT_LONGLONGPRESS;
            break;

        case HAL_PRESSURE_EVENT_PRESS:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_PRESS!!!");
            xgm_event = XGM_EVENT_DOWN;
            break;

        case HAL_PRESSURE_EVENT_RELEASE:
            XGM_TRACE(0, "HAL_PRESSURE_EVENT_RELEASE!!!");
            xgm_event = XGM_EVENT_UP;
            break;

        default:
            XGM_TRACE(0, "Uknown event!!!");
            break;
    }

    xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
        (uint32_t)xspace_gesture_manage_event_process, (uint32_t)xgm_event, (uint32_t)data, (uint32_t)len);
}

static int32_t xspace_gesture_manage_pressure_init(void)
{
    int32_t ret = 0;
    int32_t retry_count = 0;

    XGM_TRACE(0, " >>>>> ");

    while(retry_count < 2) {
        ret = hal_pressure_init();
        if (HAL_RUN_SUCCESS != ret) {
            osDelay(100);
            retry_count++;
            XGM_TRACE(1, " Pressure Init failed,%d,%d.", ret, retry_count);
        } else {
            break;
        }
    }

    if(ret != HAL_RUN_SUCCESS) {
        XGM_TRACE(0, " Pressure Init Retry Failed!!!");
        return -1;
    }

    hal_pressure_register_gesture_generate_cb(xspace_gesture_manage_pressure_event_cb);

    return 0;
}
#endif

static int32_t xspace_gesture_manage_init_if(void)
{
    int32_t ret = -1;

#if defined (__GESTURE_MANAGER_USE_TOUCH__)
    uint32_t id = 0;
    if (!hal_touch_get_chip_id(&id)) {
        XGM_TRACE(1, "  touch id:0x%08x(0x0a08).", id);
#if defined (__XSPACE_UI__)
        if (0x00a1 == id) 
#else
        if (0x0a08 == id) 
#endif
        {
            ret = xspace_gesture_manage_touch_init();
            if(HAL_RUN_SUCCESS == ret)
                goto init_success;
        }
    }

#endif

#if defined (__GESTURE_MANAGE_USE_ACC__)
    ret = app_gesture_manage_acc_init();
	if(HAL_RUN_SUCCESS == ret)
		goto init_success;
#endif

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
	ret = xspace_gesture_manage_pressure_init();
	if(HAL_RUN_SUCCESS == ret)
	{
	#if defined (__GESTURE_MANAGER_USE_TOUCH__)
		#if defined (__TOUCH_GH61X__) || defined (__TOUCH_GH621X__)
			hal_touch_register_gesture_generate_cb(xspace_gesture_manage_touch_event_cb);
		#endif
	#endif
		goto init_success;
	}
#endif

    if (ret) {
        XGM_TRACE(0, " Init gesture Failed!!!");
        return ret;
    }
init_success:
    osDelay(30);
    xspace_gesture_manage_enter_detection_mode();
    //xspace_gesture_manage_enter_standby_mode();

    XGM_TRACE(0, " Init gesture Done!");

    return ret;
}

xgm_work_status_e xspace_gesture_manage_get_work_status(void)
{
    return xgm_work_status;
}

void xspace_gesture_manage_register_ui_cb(xgm_event_func cb)
{
    if (NULL == cb)
        return;

    xgm_event_handle = cb;
}

void xspace_gesture_manage_register_manage_need_execute_cb(xgm_ui_need_execute_func cb)
{
    if (NULL == cb)
        return;

    xgm_ui_need_execute_handle = cb;
}

void xspace_gesture_manage_enter_detection_mode(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_gesture_manage_enter_detection_mode_if, 0, 0, 0);
}

void xspace_gesture_manage_enter_standby_mode(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_gesture_manage_enter_standby_mode_if, 0, 0, 0);
}


int32_t xspace_gesture_manage_revise_sensibility(uint8_t grade)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_gesture_manage_revise_sensibility_if, grade, 0, 0);

    return 0;
}

int32_t xspace_gesture_manage_get_chip_id(uint32_t *chip_id)
{
#ifndef __XSPACE_UI__
	hal_pressure_get_chip_id(chip_id);
#endif
    return 0;
}

int32_t xspace_gesture_manage_init(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_gesture_manage_init_if, 0, 0, 0);
    return 0;
}

#endif  // (XSPACE_GESTURE_MANAGER)

