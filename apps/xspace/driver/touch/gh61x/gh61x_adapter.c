#if defined (__TOUCH_GH61X__)
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_touch.h"
#include "hal_trace.h"
#include "gh61x_ctrl.h"
#include "dev_thread.h"
#include "tgt_hardware.h"
#include "gh61x_adapter.h"


/*********    Variable declaration area START   *********/

static int8_t gh61x_dev_thread_status = -1;
static hal_touch_gesture_event_cb gh61x_gesture_event_cb = NULL;
static hal_touch_inear_status_cb gh61x_inear_status_event_cb = NULL;
uint16_t drv_gh61x_chipid = 0xffff;

/*********    Variable declaration area  END    *********/



/*****    Extern function declaration area START   ******/

extern void app_interaction_touch_send_cmd_via_spp(uint8_t device_id, uint8_t* ptrData, uint32_t length);
extern void app_start_custom_function_in_app_thread(uint32_t param0, uint32_t param1, uint32_t ptr);

/*****    Extern function declaration area STOP    ******/



/*************       Function  area  START   ************/

int32_t gh61x_adapter_report_gesture_event(hal_touch_event_e gesture_type)
{
    if(NULL == gh61x_gesture_event_cb) {
        return -1;
    }
    
    gh61x_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

int32_t gh61x_adapter_report_inear_status(bool is_wearing)
{
    if(NULL == gh61x_inear_status_event_cb) {
        return -1;
    }
    
    gh61x_inear_status_event_cb(is_wearing);

    return 0;
}

int32_t gh61x_adapter_send_data(hal_touch_event_e gesture_type, uint8_t* ptrData, uint32_t length)
{
    GH61X_TRACE(0, "  enter function ");
    
    if(NULL == gh61x_gesture_event_cb) {
        return -1;
    }
    
    GH61X_TRACE(0, " send data ready ");

    gh61x_gesture_event_cb(gesture_type, ptrData, length);

    return 0;
}

static int gh61x_adapter_mod_handler(dev_message_body_s *msg_body)
{
    switch(msg_body->message_id) {
        case GH61X_MSG_ID_INT:
            gh61x_int_handler_deal();
            break;
        case GH61X_MSG_ID_SPP_RX_DATA:
            uart_rx_data_handler((uint8_t *)msg_body->message_ptr, (uint8_t)msg_body->message_Param0);
            break;
		case GH61X_MSG_ID_WORK_STATE:
			user_gh61x_work_state_checker();
			break;

        default:
            GH61X_TRACE(0, "Unkonwn event");
            break;
    }

    return 0;
}

void gh61x_adapter_event_process(uint32_t msgid, uint32_t *ptr, uint32_t param0, uint32_t param1)
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
        GH61X_TRACE(2, " mailbox send error status:0x%x(%d)", status, status);
        if (0xffffffff == status && error_time == 3) {
            ASSERT(0, " %s mailbox send error status:0x%x", __func__, status);
        } else if(0xffffffff == status) {
            error_time++;
            GH61X_TRACE(2, " %s mailbox send error:%d", __func__, error_time);
        } else {
            error_time = 0;
        }
    }
#endif
}

static int32_t gh61x_adapter_init(void)
{
    static bool init_flag = false;

    GH61X_TRACE(1, "enter,%d", init_flag);

    if(init_flag == true)
        return 0;

    //gh61x_dev_thread_status = dev_thread_handle_set(DEV_MODUAL_TOUCH, gh61x_adapter_mod_handler);

    if(GH61X_Ctrl_init() != 0) {
        GH61X_TRACE(0, "initialization failed ");
        return -1;
    } else {
		gh61x_dev_thread_status = dev_thread_handle_set(DEV_MODUAL_TOUCH, gh61x_adapter_mod_handler);
        GH61X_TRACE(0, "Initialization successful ");
        init_flag = true;
        return 0;
    }
}

static int32_t gh61x_adapter_get_chip_id(uint32_t *id)
{
	if (NULL == id)
		return -1;
		
    *id = drv_gh61x_chipid;

    return 0;
}

static int32_t  gh61x_adapter_reset(void)
{
    int32_t ret = 0;

    ret = GH61X_Reset();
    GH61X_TRACE(2, "%s %d ", __func__, ret);
    return ret;
}

static int32_t gh61x_adapter_set_gesture_event_cb(hal_touch_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    gh61x_gesture_event_cb = cb;

    return 0;
}

static int32_t xpt_gh61x_spp_rx_data_handler(uint8_t *buffer, uint8_t length){
    gh61x_spp_rx_data_handler(buffer,length);
    return 0;
}

static int32_t gh61x_adapter_set_wear_event_cb(hal_touch_inear_status_cb cb)
{
    if (NULL == cb)
        return -1;

    gh61x_inear_status_event_cb = cb;

    return 0;
}

static int32_t gh61x_adapter_enter_detect_mode(void)
{ 
	int8_t ret;
	ret = GH61X_Application_Start();
	GH61X_TRACE(2, "%s %d", __func__, ret);
	return ret;
}

static int32_t gh61x_adapter_enter_standy_mode(void)
{
    int8_t ret = GH61X_Application_Stop();
    GH61X_TRACE(2, "%s %d", __func__, ret);

    return ret;
}

int8_t gh61x_adapter_thread_handle_create_status(void)
{
	return gh61x_dev_thread_status;
}

const hal_touch_ctx_s c_gh61x_touch_ctx =
{
    gh61x_adapter_init,
    gh61x_adapter_reset,
    gh61x_adapter_enter_standy_mode,
    gh61x_adapter_enter_detect_mode,
    NULL,
    NULL,
    gh61x_adapter_get_chip_id,
    NULL,
    gh61x_adapter_set_wear_event_cb,
    gh61x_adapter_set_gesture_event_cb,
    NULL,
    xpt_gh61x_spp_rx_data_handler,
};

#endif  /* __TOUCH_GH61X__ */


