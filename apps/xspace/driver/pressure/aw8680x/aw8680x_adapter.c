#include "cmsis.h"
#include "cmsis_os.h"
#include "aw8680x_adapter.h"
#include "aw_cali_test.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_pressure.h"

static hal_pressure_gesture_event_cb aw8680x_gesture_event_cb = NULL;
static struct AW8680X_KEY_STATUS_T aw8680x_key_status;
static struct HAL_GPIO_IRQ_CFG_T aw8080x_int_gpiocfg;
static osTimerId app_aw8680x_timer = NULL;
#define APP_AW8680X_TIMER_CHECK_MS 40

static void aw8680x_event_process(void);
static bool aw8680x_get_int_gpio_val(void);
static void aw8680x_check_timehandler(void const *param);
osTimerDef(APP_AW8680X_TIMER, aw8680x_check_timehandler);
static int32_t aw8680x_adapter_report_gesture_event(hal_pressure_event_e gesture_type);

static void aw8680x_check_timehandler(void const *param)
{
	uint8_t need_timer = false;

	aw8680x_key_status.time_click++;
	aw8680x_key_status.time_updown = aw8680x_key_status.time_click*APP_AW8680X_TIMER_CHECK_MS;
	if(aw8680x_key_status.key_status)
	{
		if(aw8680x_key_status.cnt_click == HAL_PRESSURE_EVENT_CLICK)
		{
			if(aw8680x_key_status.time_updown >= KEY_LPRESS_THRESH_MS && \
				aw8680x_key_status.long_press == KEY_LONG_EVENT_NONE )
			{
				aw8680x_key_status.long_press = KEY_LONG_EVENT_LONGPRESS;
				AW_LOGI(0,AW_TAG,"KEY_LPRESS");  //send long key
				aw8680x_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_1S);
			}
			else if(aw8680x_key_status.time_updown >= KEY_LLPRESS_THRESH_MS &&\
					aw8680x_key_status.long_press == KEY_LONG_EVENT_LONGPRESS)
			{
				aw8680x_key_status.long_press = KEY_LONG_EVENT_LLONGPRESS;
				AW_LOGI(0,AW_TAG,"KEY_LLPRESS");	//send long long key
				aw8680x_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_3S);
			}
			
		}
	}
	else
	{

		if(aw8680x_key_status.long_press != KEY_LONG_EVENT_NONE)
		{
			aw8680x_key_status.cnt_click = 0;
			aw8680x_key_status.press_time_ticks = 0;
		}
		
		if(TICKS_TO_MS(hal_sys_timer_get()) - aw8680x_key_status.press_time_ticks > KEY_DBLCLICK_THRESH_MS \
			&& aw8680x_key_status.cnt_click != 0)
		{
			AW_LOGI(1,AW_TAG,"cnt_click:%d",aw8680x_key_status.cnt_click);	//send click key
			if(aw8680x_key_status.cnt_click >HAL_PRESSURE_EVENT_MAX_CLICK)
				aw8680x_key_status.cnt_click = HAL_PRESSURE_EVENT_MAX_CLICK;
			aw8680x_adapter_report_gesture_event((hal_pressure_event_e)aw8680x_key_status.cnt_click);
			aw8680x_key_status.cnt_click = 0;
			aw8680x_key_status.press_time_ticks = 0;
		}
		aw8680x_key_status.time_click = 0;
		aw8680x_key_status.long_press = KEY_LONG_EVENT_NONE;
	
	}
	if(aw8680x_key_status.cnt_click != 0)
		need_timer = true;
	if(need_timer)
	{
		osTimerStop(app_aw8680x_timer);
		osTimerStart(app_aw8680x_timer, APP_AW8680X_TIMER_CHECK_MS);
	}
}

static void aw8680x_event_process(void)
{
    dev_message_block_s msg;
    msg.mod_id = DEV_MODUAL_PRESSURE;

    dev_thread_mailbox_put(&msg);
}
static int aw8680x_handle_process(dev_message_body_s *msg_body)
{
	uint32_t curr_ticks_ms;
	if(!aw8680x_get_int_gpio_val())
	{
		aw8680x_key_status.key_status = true;
		AW_LOGI(0,AW_TAG,"KEY_PRESS");	//send PRESS key
		aw8680x_adapter_report_gesture_event(HAL_PRESSURE_EVENT_PRESS);
		osTimerStop(app_aw8680x_timer);
		osTimerStart(app_aw8680x_timer, APP_AW8680X_TIMER_CHECK_MS);
		
		curr_ticks_ms = TICKS_TO_MS(hal_sys_timer_get());
		if(!aw8680x_key_status.press_time_ticks)
		{
			aw8680x_key_status.cnt_click++;
			aw8680x_key_status.press_time_ticks = curr_ticks_ms;
		}
		else
		{
			if(curr_ticks_ms - aw8680x_key_status.press_time_ticks <= KEY_DBLCLICK_THRESH_MS)
			{
				aw8680x_key_status.cnt_click++;
				aw8680x_key_status.press_time_ticks = curr_ticks_ms;
			}
		}
	}
	else
	{
		aw8680x_key_status.key_status = false;
		AW_LOGI(0,AW_TAG,"KEY_RELEASE");	//send RELEASE key
		aw8680x_adapter_report_gesture_event(HAL_PRESSURE_EVENT_RELEASE);
	}

	return 0;
}

static void aw8680x_int_irq_polarity_change(void)
{
	aw8080x_int_gpiocfg.irq_polarity = aw8680x_get_int_gpio_val()?\
									   HAL_GPIO_IRQ_POLARITY_LOW_FALLING:\
									   HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
	hal_gpio_setup_irq(pressure_int_cfg.pin, &aw8080x_int_gpiocfg);
}

static bool aw8680x_get_int_gpio_val(void)
{
	return hal_gpio_pin_get_val(pressure_int_cfg.pin);
}

static void aw8680x_int_handler(enum HAL_GPIO_PIN_T pin)
{
	AW_LOGI(2,AW_TAG,"%s %d",__func__,aw8680x_get_int_gpio_val());
	aw8680x_int_irq_polarity_change();
	aw8680x_event_process();
}

static void aw8680x_int_init(void)
{

	aw8080x_int_gpiocfg.irq_enable = true;
	aw8080x_int_gpiocfg.irq_debounce = true;
	aw8080x_int_gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

	aw8080x_int_gpiocfg.irq_handler = aw8680x_int_handler;
	aw8080x_int_gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_int_cfg, 1);
    hal_gpio_pin_set_dir(pressure_int_cfg.pin, HAL_GPIO_DIR_IN, 1);
	hal_gpio_setup_irq(pressure_int_cfg.pin, &aw8080x_int_gpiocfg);
	dev_thread_handle_set(DEV_MODUAL_PRESSURE, aw8680x_handle_process);
}

static void aw8680x_int_irq_enable(bool en)
{
//	aw8080x_int_gpiocfg.irq_enable = en;
//	hal_gpio_setup_irq(pressure_int_cfg.pin, &aw8080x_int_gpiocfg);
}

#if defined(AW8680X_DATA_PRINTF_DEBUG)
static osTimerId dev_aw8680x_timer_debug = NULL;

static void aw8680x_check_timehandler_debug(void const *param);
osTimerDef(DEV_AW8680X_TIMER_DEBUG, aw8680x_check_timehandler_debug);
static void aw8680x_check_timehandler_debug(void const *param)
{
	uint8_t data[2];
	aw8680x_adc_read(data);
	AW_LOGI(3,AW_TAG,"aw8680x_adc_read:%d [0x%02x 0x%02x]",(int16_t)(data[0]|(data[1]<<8)),data[0],data[1]);
		
	aw8680x_force_read(data);
	AW_LOGI(3,AW_TAG,"aw8680x_force_read:%d [0x%02x 0x%02x]",(int16_t)(data[0]|(data[1]<<8)),data[0],data[1]);
		
	aw8680x_key_read(data);
	AW_LOGI(1,AW_TAG,"aw8680x_key_read:0x%02x",data[0]);
}
#endif

static int32_t aw8680x_adapter_init(void)
{
	int ret = 0;
	AW_LOGI(1,AW_TAG,"%s enter" ,__FUNCTION__);
	
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_reset_cfg, 1);
    hal_gpio_pin_set_dir(pressure_reset_cfg.pin, HAL_GPIO_DIR_OUT, 1);
	ret = aw8680x_init();
	if(ret)
	{
		AW_LOGE(2,AW_TAG,"%s fail ret:%d" ,__FUNCTION__,ret);
		return ret;
	}
		
	//aw8680x_mode_set(HIGH_SPEED);
	if (app_aw8680x_timer == NULL)
	{
	   app_aw8680x_timer = osTimerCreate (osTimer(APP_AW8680X_TIMER), osTimerOnce, NULL);
	   //osTimerStart(app_aw8680x_timer, 500);
	}
#if defined(AW8680X_DATA_PRINTF_DEBUG)
	if (dev_aw8680x_timer_debug == NULL)
	{
	   dev_aw8680x_timer_debug = osTimerCreate (osTimer(DEV_AW8680X_TIMER_DEBUG), osTimerPeriodic, NULL);
	   osTimerStart(dev_aw8680x_timer_debug, 500);
	}
#endif
	aw8680x_int_init();
	AW_LOGI(1,AW_TAG,"%s exit" ,__FUNCTION__);
	return ret;
}

static int32_t aw8680x_adapter_get_id(uint32_t *chip_id)
{
	uint8_t read_id_buf[4] = {0};

	if (NULL == chip_id)
		return -1;
	aw8680x_chipid_read(read_id_buf);
	*chip_id = read_id_buf[0]|(read_id_buf[1]<<8)|(read_id_buf[2]<<16);
	
	AW_LOGI(2,AW_TAG,"%s:0x%06x" ,__FUNCTION__,*chip_id);
	return 0;
}

static int32_t aw8680x_adapter_enter_standby_mode(void)
{
	AW_LOGI(1,AW_TAG,"%s enter" ,__FUNCTION__);
	aw8680x_int_irq_enable(false);
    return 0;
}

static int32_t aw8680x_adapter_enter_detection_mode(void)
{
	AW_LOGI(1,AW_TAG,"%s enter" ,__FUNCTION__);
	aw8680x_int_irq_enable(true);
    return 0;
}

static int32_t aw8680x_adapter_report_gesture_event(hal_pressure_event_e gesture_type)
{
    if(NULL == aw8680x_gesture_event_cb) {
        return -1;
    }
    
    aw8680x_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

static int32_t aw8680x_adapter_set_mode(uint8_t mode)
{
    aw8680x_mode_set(mode);
    return 0;
}

static int32_t aw8680x_adapter_set_sensitivity(uint8_t leve)
{
    return 0;
}

static int32_t aw8680x_adapter_set_gesture_event_cb(hal_pressure_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    aw8680x_gesture_event_cb = cb;

    return 0;
}

static int32_t aw8680x_adapter_enter_pt_calibration(uint8_t items,uint8_t *data)
{
	int32_t ret = -1;
	switch(items)
	{
		case AW_CALI_READY_ITEM:
			ret = aw_cali_ready();
		  break;
		case AW_CALI_PRESS_ITEM:
			ret = aw_cali_press();
		  break;
		case AW_CALI_WRITE_ITEM:
			ret = aw_cali_wait_up();
		  break;
		case AW_TEST_READY_ITEM:
			ret = aw_test_ready();
		  break;
		case AW_TEST_PRESS_ITEM:
			ret = aw_test_press();
		  break;
		case AW_TEST_FINISH_ITEM:
			ret = aw_test_wait_up();
		  break;
		case AW_TEST_RAWDATA_ITEM:
		  if(NULL != data)
		  {
			  data[0] = 0x05;
			  aw8680x_adc_read(&data[1]);
			  AW_LOGI(3,AW_TAG,"aw8680x_adc_read:%d [0x%02x 0x%02x]",(int16_t)(data[1]|(data[2]<<8)),data[1],data[2]);
				  
			  aw8680x_force_read(&data[3]);
			  AW_LOGI(3,AW_TAG,"aw8680x_force_read:%d [0x%02x 0x%02x]",(int16_t)(data[3]|(data[4]<<8)),data[3],data[4]);
				  
			  aw8680x_key_read(&data[5]);
			  AW_LOGI(1,AW_TAG,"aw8680x_key_read:0x%02x",data[5]);
			  ret = 0;
		  }
		  break;
		default:
		  break;
	}
	AW_LOGI(3,AW_TAG,"%s items:%d ret:%d", __FUNCTION__, items, ret);
    return ret;
}

const hal_pressure_ctx_s c_aw8680x_pressure_ctx =
{
	aw8680x_adapter_init,
	NULL,									//reset
	aw8680x_adapter_get_id,					//get chip id
	aw8680x_adapter_enter_standby_mode,		//enter standby
	aw8680x_adapter_enter_detection_mode,	//enter detection
	aw8680x_adapter_set_mode,
	aw8680x_adapter_set_sensitivity,
	aw8680x_adapter_set_gesture_event_cb,
	aw8680x_adapter_enter_pt_calibration,
};

