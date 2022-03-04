#if defined (__TOUCH_SIMULATION__)
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_touch.h"
#include "hal_trace.h"
#include "dev_thread.h"
#include "tgt_hardware.h"
#include "touch_simu_adapter.h"

static int8_t simu_dev_thread_status = -1;
static hal_touch_gesture_event_cb simu_gesture_event_cb = NULL;
static hal_touch_inear_status_cb simu_inear_status_event_cb = NULL;
static bool simu_work_status = false;

int32_t simu_adapter_report_gesture_event(uint8_t gesture_type)
{
    SIMU_TRACE(1, "gesture_type:%d.", gesture_type);

    if(NULL == simu_gesture_event_cb) {
        return -1;
    }

    simu_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

int32_t simu_adapter_report_inear_status(bool is_wearing)
{
    SIMU_TRACE(1, "is_wearing:%d(1:).", is_wearing);

    if(NULL == simu_inear_status_event_cb) {
        return -1;
    }
    
    simu_inear_status_event_cb(is_wearing);

    return 0;
}

int32_t simu_adapter_send_data(hal_touch_event_e gesture_type, uint8_t* ptrData, uint32_t length)
{
    SIMU_TRACE(0, "  enter function ");
    
    if(NULL == simu_gesture_event_cb) {
        return -1;
    }

    SIMU_TRACE(0, " send data ready ");

    simu_gesture_event_cb(gesture_type, ptrData, length);

    return 0;
}

static int32_t simu_adapter_init(void)
{
    static bool init_flag = false;

    SIMU_TRACE(1, "enter,%d", init_flag);

    if(init_flag == true)
        return 0;

    return 0;
}

static int32_t simu_adapter_get_chip_id(uint32_t *id)
{
    if (NULL == id)
        return -1;

    *id = 0x61;

    return 0;
}

static int32_t  simu_adapter_reset(void)
{
    int32_t ret = 0;

    SIMU_TRACE(1, " %d ",ret);
    return ret;
}

static int32_t simu_adapter_set_gesture_event_cb(hal_touch_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    simu_gesture_event_cb = cb;

    return 0;
}

static int32_t simu_adapter_set_wear_event_cb(hal_touch_inear_status_cb cb)
{
    if (NULL == cb)
        return -1;

    simu_inear_status_event_cb = cb;

    return 0;
}

static int32_t simu_adapter_enter_detect_mode(void)
{
    int8_t ret = 0;
    simu_work_status = true;
    SIMU_TRACE(1, "%d", ret);
    return ret;
}

static int32_t simu_adapter_enter_standy_mode(void)
{
    int8_t ret = 0;
    SIMU_TRACE(1, "%d", ret);
    simu_work_status = false;

    return ret;
}

int8_t simu_adapter_thread_handle_create_status(void)
{
    return simu_dev_thread_status;
}

const hal_touch_ctx_s c_simu_touch_ctx =
{
    simu_adapter_init,
    simu_adapter_reset,
    NULL,
    NULL,
    simu_adapter_enter_standy_mode,
    simu_adapter_enter_detect_mode,
    simu_adapter_get_chip_id,
    NULL,
    simu_adapter_set_wear_event_cb,
    simu_adapter_set_gesture_event_cb,
};

#endif  /* __TOUCH_simu__ */


