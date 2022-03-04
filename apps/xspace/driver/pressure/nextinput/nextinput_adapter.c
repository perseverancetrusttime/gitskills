#include "cmsis.h"
#include "cmsis_os.h"
#include "nextinput_adapter.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_pressure.h"
#include "xspace_i2c_master.h"
#include "ni.h"

static hal_pressure_gesture_event_cb nextinput_gesture_event_cb = NULL;
static struct NEXTINPUT_KEY_STATUS_T nextinput_key_status;
static struct HAL_GPIO_IRQ_CFG_T nextinput_int_gpiocfg;
static osTimerId app_nextinput_timer = NULL;
#define APP_NEXTINPUT_TIMER_CHECK_MS 40

static void nextinput_event_process(void);
static bool nextinput_get_int_gpio_val(void);
static void nextinput_check_timehandler(void const *param);
osTimerDef(APP_NEXTINPUT_TIMER, nextinput_check_timehandler);
static int32_t nextinput_adapter_report_gesture_event(hal_pressure_event_e gesture_type);

/***************************NEXTINPUT I2C STAR***************************/
static int8_t nextinput_i2c_read(uint8_t PointReg,uint8_t *pData)
{
	bool bRet = true;
	bRet = xspace_i2c_read(NEXTINPUT_I2C_TYPE, NEXTINPUT_I2C_ADDR, PointReg, pData);
	return bRet;
}

static int8_t nextinput_i2c_write(uint8_t PointReg,uint8_t pData)
{
	bool bRet = true;
	bRet = xspace_i2c_write(NEXTINPUT_I2C_TYPE, NEXTINPUT_I2C_ADDR, PointReg, pData);
	return bRet;
}
/***************************NEXTINPUT I2C END***************************/

static void nextinput_check_timehandler(void const *param)
{
	uint8_t need_timer = false;

	nextinput_key_status.time_click++;
	nextinput_key_status.time_updown = nextinput_key_status.time_click*APP_NEXTINPUT_TIMER_CHECK_MS;
	if(nextinput_key_status.key_status)
	{
		if(nextinput_key_status.cnt_click == HAL_PRESSURE_EVENT_CLICK)
		{
			if(nextinput_key_status.time_updown >= KEY_LPRESS_THRESH_MS && \
				nextinput_key_status.long_press == KEY_LONG_EVENT_NONE )
			{
				nextinput_key_status.long_press = KEY_LONG_EVENT_LONGPRESS;
				NEXTINPUT_TRACE(0,"KEY_LPRESS");  //send long key
				nextinput_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_1S);
			}
			else if(nextinput_key_status.time_updown >= KEY_LLPRESS_THRESH_MS &&\
					nextinput_key_status.long_press == KEY_LONG_EVENT_LONGPRESS)
			{
				nextinput_key_status.long_press = KEY_LONG_EVENT_LLONGPRESS;
				NEXTINPUT_TRACE(0,"KEY_LLPRESS");	//send long long key
				nextinput_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_3S);
			}
			
		}
	}
	else
	{

		if(nextinput_key_status.long_press != KEY_LONG_EVENT_NONE)
		{
			nextinput_key_status.cnt_click = 0;
			nextinput_key_status.press_time_ticks = 0;
		}
		
		if(TICKS_TO_MS(hal_sys_timer_get()) - nextinput_key_status.press_time_ticks > KEY_DBLCLICK_THRESH_MS \
			&& nextinput_key_status.cnt_click != 0)
		{
			NEXTINPUT_TRACE(1,"cnt_click:%d",nextinput_key_status.cnt_click);	//send click key
			if(nextinput_key_status.cnt_click >HAL_PRESSURE_EVENT_MAX_CLICK)
				nextinput_key_status.cnt_click = HAL_PRESSURE_EVENT_MAX_CLICK;
			nextinput_adapter_report_gesture_event((hal_pressure_event_e)nextinput_key_status.cnt_click);
			nextinput_key_status.cnt_click = 0;
			nextinput_key_status.press_time_ticks = 0;
		}
		nextinput_key_status.time_click = 0;
		nextinput_key_status.long_press = KEY_LONG_EVENT_NONE;
	
	}
	if(nextinput_key_status.cnt_click != 0)
		need_timer = true;
	if(need_timer)
	{
		osTimerStop(app_nextinput_timer);
		osTimerStart(app_nextinput_timer, APP_NEXTINPUT_TIMER_CHECK_MS);
	}
}

static void nextinput_event_process(void)
{
    dev_message_block_s msg;
    msg.mod_id = DEV_MODUAL_PRESSURE;

    dev_thread_mailbox_put(&msg);
}
static int nextinput_handle_process(dev_message_body_s *msg_body)
{
	uint32_t curr_ticks_ms;
	if(!nextinput_get_int_gpio_val())
	{
		nextinput_key_status.key_status = true;
		NEXTINPUT_TRACE(0,"KEY_PRESS");	//send PRESS key
		nextinput_adapter_report_gesture_event(HAL_PRESSURE_EVENT_PRESS);
		osTimerStop(app_nextinput_timer);
		osTimerStart(app_nextinput_timer, APP_NEXTINPUT_TIMER_CHECK_MS);
		
		curr_ticks_ms = TICKS_TO_MS(hal_sys_timer_get());
		if(!nextinput_key_status.press_time_ticks)
		{
			nextinput_key_status.cnt_click++;
			nextinput_key_status.press_time_ticks = curr_ticks_ms;
		}
		else
		{
			if(curr_ticks_ms - nextinput_key_status.press_time_ticks <= KEY_DBLCLICK_THRESH_MS)
			{
				nextinput_key_status.cnt_click++;
				nextinput_key_status.press_time_ticks = curr_ticks_ms;
			}
		}
	}
	else
	{
		nextinput_key_status.key_status = false;
		NEXTINPUT_TRACE(0,"KEY_RELEASE");	//send RELEASE key
		nextinput_adapter_report_gesture_event(HAL_PRESSURE_EVENT_RELEASE);
	}

	return 0;
}

static void nextinput_int_irq_polarity_change(void)
{
	nextinput_int_gpiocfg.irq_polarity = nextinput_get_int_gpio_val()?\
									   	 HAL_GPIO_IRQ_POLARITY_LOW_FALLING:\
									   	 HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
#if defined(__HARDWARE_VERSION_V1_1_20201214__)
	hal_gpio_setup_irq(pressure_intx_cfg.pin, &nextinput_int_gpiocfg);
#else
	hal_gpio_setup_irq(pressure_int_cfg.pin, &nextinput_int_gpiocfg);
#endif
}

static bool nextinput_get_int_gpio_val(void)
{
#if defined(__HARDWARE_VERSION_V1_1_20201214__)
	return hal_gpio_pin_get_val(pressure_intx_cfg.pin);
#else
	return hal_gpio_pin_get_val(pressure_int_cfg.pin);
#endif
}

static void nextinput_int_handler(enum HAL_GPIO_PIN_T pin)
{
	NEXTINPUT_TRACE(2,"%s %d",__func__,nextinput_get_int_gpio_val());
	nextinput_int_irq_polarity_change();
	nextinput_event_process();
}

static void nextinput_int_init(void)
{
	nextinput_int_gpiocfg.irq_enable = true;
	nextinput_int_gpiocfg.irq_debounce = true;
	nextinput_int_gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

	nextinput_int_gpiocfg.irq_handler = nextinput_int_handler;
	nextinput_int_gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
#if defined(__HARDWARE_VERSION_V1_1_20201214__)
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_intx_cfg, 1);
    hal_gpio_pin_set_dir(pressure_intx_cfg.pin, HAL_GPIO_DIR_IN, 1);
	hal_gpio_setup_irq(pressure_intx_cfg.pin, &nextinput_int_gpiocfg);
#else
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_int_cfg, 1);
    hal_gpio_pin_set_dir(pressure_int_cfg.pin, HAL_GPIO_DIR_IN, 1);
	hal_gpio_setup_irq(pressure_int_cfg.pin, &nextinput_int_gpiocfg);
#endif
	dev_thread_handle_set(DEV_MODUAL_PRESSURE, nextinput_handle_process);
}

//static void nextinput_int_irq_enable(bool en)
//{
//	nextinput_int_gpiocfg.irq_enable = en;
//	hal_gpio_setup_irq(pressure_int_cfg.pin, &nextinput_int_gpiocfg);
//}

#if defined(NEXTINPUT_DATA_PRINTF_DEBUG)
static osTimerId dev_nextinput_timer_debug = NULL;

static void nextinput_check_timehandler_debug(void const *param);
osTimerDef(DEV_NEXTINPUT_TIMER_DEBUG, nextinput_check_timehandler_debug);
static void nextinput_check_timehandler_debug(void const *param)
{

}
#endif

#if 0
static int32_t nextinput_drv_init(void)
{
	uint8_t read_id = 0;
	nextinput_i2c_read(0x80,&read_id);	//read DeviceID REG:0x80，Read  value shifted 3 bits right->0x04
	NEXTINPUT_TRACE(2,"%s read_id:0x%02x" , __FUNCTION__, read_id);
	if(0x04 != (read_id >> 3))
		return 1;
	nextinput_i2c_write(0x00,0xB5);	//AFE enable, ADC mode
	nextinput_i2c_write(0x01,0x50);	//0x30 31x 0x40 61x 0x50 115xINAGAIN, Precharge 50us
	nextinput_i2c_write(0x06,0x85);	//Enable autopreldadj, FALLTHRSEL=0.75 x INTRTHR
	nextinput_i2c_write(0x07,0x06);
	nextinput_i2c_write(0x08,0x00);	//AUTOCAL threshold=32 lsb
	nextinput_i2c_write(0x09,0xB3);
	nextinput_i2c_write(0x0A,0x50);
	nextinput_i2c_write(0x0B,0x05);
	nextinput_i2c_write(0x0C,0x05);
	nextinput_i2c_write(0x0D,0x00);	//INTRTHR =80 lsb;
	nextinput_i2c_write(0x0E,0x13);	//BTN mode and 3 Interrupt samples
	nextinput_i2c_write(0x0F,0x01);	//Force baseline
	return 0;
}
#endif

static int32_t nextinput_adapter_init(void)
{
	int ret = 0;
	NEXTINPUT_TRACE(1,"%s enter" ,__FUNCTION__);
	
//    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_reset_cfg, 1);
//    hal_gpio_pin_set_dir(pressure_reset_cfg.pin, HAL_GPIO_DIR_OUT, 1);
	ret = ni_init();//nextinput_drv_init();
	if(ret == 4)
	{
		NEXTINPUT_TRACE(2,"%s ret:%d" ,__FUNCTION__,ret);
		ret = 0;
	}
	if(ret)
	{
		NEXTINPUT_TRACE(2,"%s fail ret:%d" ,__FUNCTION__,ret);
		return ret;
	}
		
	if (app_nextinput_timer == NULL)
	{
	   app_nextinput_timer = osTimerCreate (osTimer(APP_NEXTINPUT_TIMER), osTimerOnce, NULL);
	   //osTimerStart(app_nextinput_timer, 500);
	}
#if defined(NEXTINPUT_DATA_PRINTF_DEBUG)
	if (dev_nextinput_timer_debug == NULL)
	{
	   dev_nextinput_timer_debug = osTimerCreate (osTimer(DEV_NEXTINPUT_TIMER_DEBUG), osTimerPeriodic, NULL);
	   osTimerStart(dev_nextinput_timer_debug, 500);
	}
#endif
	nextinput_int_init();
	NEXTINPUT_TRACE(1,"%s exit" ,__FUNCTION__);
	return ret;
}

static int32_t nextinput_adapter_get_id(uint32_t *chip_id)
{
	uint8_t read_id = 0;

	if (NULL == chip_id)
		return -1;
	nextinput_i2c_read(0x80,&read_id);	//read DeviceID REG:0x80，Read  value shifted 3 bits right->0x04
	*chip_id = read_id >> 3;	//DeviceID:0x04
	
	NEXTINPUT_TRACE(2,"%s:0x%02x" ,__FUNCTION__,*chip_id);
	return 0;
}

static int32_t nextinput_adapter_enter_standby_mode(void)
{
	uint8_t reg_val = 0;
	nextinput_i2c_read(0x00,&reg_val);
	reg_val &= 0xfe;
	nextinput_i2c_write(0x00,reg_val);
	NEXTINPUT_TRACE(1,"%s exit" ,__FUNCTION__);
    return 0;
}

static int32_t nextinput_adapter_enter_detection_mode(void)
{
	uint8_t reg_val = 0;
	nextinput_i2c_read(0x00,&reg_val);
	reg_val |= 0x01;
	nextinput_i2c_write(0x00,reg_val);
	NEXTINPUT_TRACE(1,"%s exit" ,__FUNCTION__);
    return 0;
}

static int32_t nextinput_adapter_report_gesture_event(hal_pressure_event_e gesture_type)
{
    if(NULL == nextinput_gesture_event_cb) {
        return -1;
    }
    
    nextinput_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

static int32_t nextinput_adapter_set_mode(uint8_t mode)
{
    return 0;
}

static int32_t nextinput_adapter_set_sensitivity(uint8_t leve)
{
    return 0;
}

static int32_t nextinput_adapter_set_gesture_event_cb(hal_pressure_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    nextinput_gesture_event_cb = cb;

    return 0;
}

static int32_t nextinput_adapter_enter_pt_calibration(uint8_t items,uint8_t *data)
{
	int32_t ret = -1;
	#define NI_SENSOR_NUM 1
	static int16_t sensor_initial_adc[NI_SENSOR_NUM]={0};
	static int16_t sensor_load_adc[NI_SENSOR_NUM]={0};
	switch(items)
	{
		case NI_CALI_DEVICE_WAKEUP_ITEM:
			ret = ni_calibration_init(sensor_initial_adc);
			NEXTINPUT_TRACE(0,"ni_calibration_init");
			break;
		case NI_CALI_EN_ITEM:
			ret = ni_calibration_start(sensor_load_adc);
			NEXTINPUT_TRACE(0,"ni_calibration_start");
			break;
		case NI_CALI_DISEN_ITEM:
			ret = ni_calibration_done(sensor_load_adc, sensor_initial_adc);
			NEXTINPUT_TRACE(0,"ni_calibration_done");
			break;
		default:
			break;
	}
	NEXTINPUT_TRACE(3,"%s items:%d ret:%d", __FUNCTION__, items, ret);
    return ret;
}
const hal_pressure_ctx_s c_nextinput_pressure_ctx =
{
	nextinput_adapter_init,
	NULL,									//reset
	nextinput_adapter_get_id,				//get chip id
	nextinput_adapter_enter_standby_mode,	//enter standby
	nextinput_adapter_enter_detection_mode,	//enter detection
	nextinput_adapter_set_mode,
	nextinput_adapter_set_sensitivity,
	nextinput_adapter_set_gesture_event_cb,
	nextinput_adapter_enter_pt_calibration,
};

