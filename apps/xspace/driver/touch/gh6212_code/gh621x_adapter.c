#if defined (__TOUCH_GH621X__)
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_touch.h"
#include "hal_trace.h"
#include "dev_thread.h"
#include "tgt_hardware.h"
#include "gh621x_example.h"
#include "gh621x_example_common.h"
#include "gh621x_adapter.h"


/*********    Variable declaration area START   *********/

static int8_t gh621x_dev_thread_status = -1;
static hal_touch_gesture_event_cb gh621x_gesture_event_cb = NULL;
static hal_touch_inear_status_cb gh621x_inear_status_event_cb = NULL;
uint16_t drv_gh621x_chipid = 0xffff;
uint8_t gh621xenablePowerOnOffset = true;

/*********    Variable declaration area  END    *********/



/*****    Extern function declaration area START   ******/

extern void app_interaction_touch_send_cmd_via_spp(uint8_t* ptrData, uint32_t length);
extern void app_start_custom_function_in_app_thread(uint32_t param0, uint32_t param1, uint32_t ptr);
//extern void gh621x_int_msg_handler(void);

/*****    Extern function declaration area STOP    ******/



/*************       Function  area  START   ************/

int32_t gh621x_adapter_report_gesture_event(hal_touch_event_e gesture_type)
{
    if(NULL == gh621x_gesture_event_cb) {
        return -1;
    }
    
    gh621x_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

int32_t gh621x_adapter_report_inear_status(bool is_wearing)
{
    if(NULL == gh621x_inear_status_event_cb) {
        return -1;
    }
    
    gh621x_inear_status_event_cb(is_wearing);

    return 0;
}

int32_t gh621x_adapter_send_data(hal_touch_event_e gesture_type, uint8_t* ptrData, uint32_t length)
{
    GH621X_TRACE(0, "  enter function ");
    
    if(NULL == gh621x_gesture_event_cb) {
        return -1;
    }
    
    GH621X_TRACE(0, " send data ready ");

    gh621x_gesture_event_cb(gesture_type, ptrData, length);

    return 0;
}

static int gh621x_adapter_mod_handler(dev_message_body_s *msg_body)
{
    switch(msg_body->message_id) {
        case GH621X_MSG_ID_INT:
            Gh621xModuleIntMsgHandler();
            break;
        case GH621X_MSG_ID_UART_RX_DATA:
            Gh621xModuleHandleRecvDataViaUart((uint8_t *)msg_body->message_ptr, (uint8_t)msg_body->message_Param0);
            break;
        default:
            GH621X_TRACE(0, "Unkonwn event");
            break;
    }

    return 0;
}

void gh621x_adapter_event_process(uint32_t msgid, uint32_t *ptr, uint32_t param0, uint32_t param1)
{
#if defined(__DEV_THREAD__)
    dev_message_block_s msg;
    static uint8_t error_time = 0;

    msg.mod_id = DEV_MODUAL_TOUCH;
    msg.msg_body.message_id = msgid;
    msg.msg_body.message_ptr = (uint32_t *)ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    int status = dev_thread_mailbox_put(&msg);
    if(status != osOK) {
        GH621X_TRACE(2, " mailbox send error status:0x%x(%d)", status, status);
        if (0xffffffff == status && error_time == 3) {
            ASSERT(0, " %s mailbox send error status:0x%x", __func__, status);
        } else if(0xffffffff == status) {
            error_time++;
            GH621X_TRACE(2, " %s mailbox send error:%d", __func__, error_time);
        } else {
            error_time = 0;
        }
    }
#endif
}

static int32_t gh621x_adapter_init(void)
{
    static bool init_flag = false;

    GH621X_TRACE(1, "enter,%d", init_flag);

    if(init_flag == true)
        return 0;

    //gh621x_dev_thread_status = dev_thread_handle_set(DEV_MODUAL_TOUCH, gh621x_adapter_mod_handler);

    if(Gh621xModuleInit() != 1) {
        GH621X_TRACE(0, "initialization failed ");
        return -1;
    } else {
		gh621x_dev_thread_status = dev_thread_handle_set(DEV_MODUAL_TOUCH, gh621x_adapter_mod_handler);
        GH621X_TRACE(0, "Initialization successful ");
        init_flag = true;
        return 0;
    }
}

static int32_t gh621x_adapter_get_chip_id(uint32_t *id)
{
	if (NULL == id)
		return -1;
		
    *id = drv_gh621x_chipid;

    return 0;
}

static int32_t  gh621x_adapter_reset(void)
{
    int32_t ret = 0;

//    ret = gh621x_Reset();
    GH621X_TRACE(2, "%s %d ", __func__, ret);
    return ret;
}

static int32_t gh621x_adapter_set_gesture_event_cb(hal_touch_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    gh621x_gesture_event_cb = cb;

    return 0;
}

static int32_t gh621x_adapter_set_wear_event_cb(hal_touch_inear_status_cb cb)
{
    if (NULL == cb)
        return -1;

    gh621x_inear_status_event_cb = cb;

    return 0;
}

static int32_t gh621x_adapter_enter_detect_mode(void)
{
	Gh621xModuleUsingPowerOnOffset(gh621xenablePowerOnOffset);
	Gh621xModuleStart(0);
	GH621X_TRACE(2, "%s offset:%d", __func__,gh621xenablePowerOnOffset);
	return 0;
}

static int32_t gh621x_adapter_enter_standy_mode(void)
{
    Gh621xModuleStop(1);
	GH621X_TRACE(1, "%s", __func__);
    return 0;
}

int8_t gh621x_adapter_thread_handle_create_status(void)
{
	return gh621x_dev_thread_status;
}

static int32_t gh621x_adapter_off_set_self_cali(void)
{
    Gh621xOffsetSelfCalibrationInChargingBox();
	GH621X_TRACE(1, "%s", __func__);
    return 0;
}

const hal_touch_ctx_s c_gh621x_touch_ctx =
{
    gh621x_adapter_init,
    gh621x_adapter_reset,
    NULL,
    NULL,
    gh621x_adapter_enter_standy_mode,
    gh621x_adapter_enter_detect_mode,
    gh621x_adapter_get_chip_id,
    NULL,
    gh621x_adapter_set_wear_event_cb,
    gh621x_adapter_set_gesture_event_cb,
    gh621x_adapter_off_set_self_cali,
    NULL,
};

#endif  /* __TOUCH_gh621x__ */


