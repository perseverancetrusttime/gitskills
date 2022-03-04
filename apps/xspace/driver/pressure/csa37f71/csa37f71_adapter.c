#include "cmsis.h"
#include "cmsis_os.h"
#include "csa37f71_adapter.h"
#include "cs_press_f71_driver.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_pressure.h"

static hal_pressure_gesture_event_cb csa37f71_gesture_event_cb = NULL;
static struct CSA37F71_KEY_STATUS_T csa37f71_key_status;
static struct HAL_GPIO_IRQ_CFG_T csa37f71_int_gpiocfg;
static osTimerId app_csa37f71_timer = NULL;
#define APP_CSA37F71_TIMER_CHECK_MS 40

static void csa37f71_event_process(void);
static bool csa37f71_get_int_gpio_val(void);
static void csa37f71_check_timehandler(void const *param);
osTimerDef(APP_CSA37F71_TIMER, csa37f71_check_timehandler);
static int32_t csa37f71_adapter_report_gesture_event(hal_pressure_event_e gesture_type);

static const char *csa37f71_cali_items_string[] = {
	"NULL",
	"CS_CALI_DEVICE_WAKEUP_ITEM",
	"CS_CALI_EN_ITEM",
	"CS_CALI_CHECK_ITEM",
	"CS_CALI_DISEN_ITEM",
	"CS_CALI_READ_RAWDATA_ITEM", //5
	"CS_CALI_READ_PROCESSED_DATA_ITEM",
	"CS_CALI_READ_NOISE_ITEM",
	"CS_CALI_READ_OFFSET_ITEM",
	"CS_CALI_READ_PRESSURE_ITEM",
	"CS_CALI_READ_PRESS_LEVEL_ITEM", //10
	"CS_CALI_WRITE_PRESS_LEVEL_ITEM",
	"CS_CALI_READ_CALIBRATION_FACTOR_ITEM",
	"CS_CALI_WRITE_CALIBRATION_FACTOR_ITEM",
	"CS_CALI_TEST_ITEM_TOTAL",
};


static void csa37f71_check_timehandler(void const *param)
{
	uint8_t need_timer = false;

	csa37f71_key_status.time_click++;
	csa37f71_key_status.time_updown = csa37f71_key_status.time_click*APP_CSA37F71_TIMER_CHECK_MS;
	if(csa37f71_key_status.key_status)
	{
		if(csa37f71_key_status.cnt_click == HAL_PRESSURE_EVENT_CLICK)
		{
			if(csa37f71_key_status.time_updown >= KEY_LPRESS_THRESH_MS && \
				csa37f71_key_status.long_press == KEY_LONG_EVENT_NONE )
			{
				csa37f71_key_status.long_press = KEY_LONG_EVENT_LONGPRESS;
				CSA37F71_TRACE(0,"KEY_LPRESS");  //send long key
				csa37f71_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_1S);
			}
			else if(csa37f71_key_status.time_updown >= KEY_LLPRESS_THRESH_MS &&\
					csa37f71_key_status.long_press == KEY_LONG_EVENT_LONGPRESS)
			{
				csa37f71_key_status.long_press = KEY_LONG_EVENT_LLONGPRESS;
				CSA37F71_TRACE(0,"KEY_LLPRESS");	//send long long key
				csa37f71_adapter_report_gesture_event(HAL_PRESSURE_EVENT_LONGPRESS_3S);
			}
			
		}
	}
	else
	{

		if(csa37f71_key_status.long_press != KEY_LONG_EVENT_NONE)
		{
			csa37f71_key_status.cnt_click = 0;
			csa37f71_key_status.press_time_ticks = 0;
		}
		
		if(TICKS_TO_MS(hal_sys_timer_get()) - csa37f71_key_status.press_time_ticks > KEY_DBLCLICK_THRESH_MS \
			&& csa37f71_key_status.cnt_click != 0)
		{
			CSA37F71_TRACE(1,"cnt_click:%d",csa37f71_key_status.cnt_click);	//send click key
			if(csa37f71_key_status.cnt_click >HAL_PRESSURE_EVENT_MAX_CLICK)
				csa37f71_key_status.cnt_click = HAL_PRESSURE_EVENT_MAX_CLICK;
			csa37f71_adapter_report_gesture_event((hal_pressure_event_e)csa37f71_key_status.cnt_click);
			csa37f71_key_status.cnt_click = 0;
			csa37f71_key_status.press_time_ticks = 0;
		}
		csa37f71_key_status.time_click = 0;
		csa37f71_key_status.long_press = KEY_LONG_EVENT_NONE;
	
	}
	if(csa37f71_key_status.cnt_click != 0)
		need_timer = true;
	if(need_timer)
	{
		osTimerStop(app_csa37f71_timer);
		osTimerStart(app_csa37f71_timer, APP_CSA37F71_TIMER_CHECK_MS);
	}
}

static void csa37f71_event_process(void)
{
    dev_message_block_s msg;
    msg.mod_id = DEV_MODUAL_PRESSURE;

    dev_thread_mailbox_put(&msg);
}
static int csa37f71_handle_process(dev_message_body_s *msg_body)
{
	uint32_t curr_ticks_ms;
	if(!csa37f71_get_int_gpio_val())
	{
		csa37f71_key_status.key_status = true;
		CSA37F71_TRACE(0,"KEY_PRESS");	//send PRESS key
		csa37f71_adapter_report_gesture_event(HAL_PRESSURE_EVENT_PRESS);
		osTimerStop(app_csa37f71_timer);
		osTimerStart(app_csa37f71_timer, APP_CSA37F71_TIMER_CHECK_MS);
		
		curr_ticks_ms = TICKS_TO_MS(hal_sys_timer_get());
		if(!csa37f71_key_status.press_time_ticks)
		{
			csa37f71_key_status.cnt_click++;
			csa37f71_key_status.press_time_ticks = curr_ticks_ms;
		}
		else
		{
			if(curr_ticks_ms - csa37f71_key_status.press_time_ticks <= KEY_DBLCLICK_THRESH_MS)
			{
				csa37f71_key_status.cnt_click++;
				csa37f71_key_status.press_time_ticks = curr_ticks_ms;
			}
		}
	}
	else
	{
		csa37f71_key_status.key_status = false;
		CSA37F71_TRACE(0,"KEY_RELEASE");	//send RELEASE key
		csa37f71_adapter_report_gesture_event(HAL_PRESSURE_EVENT_RELEASE);
	}

	return 0;
}

static void csa37f71_int_irq_polarity_change(void)
{
	csa37f71_int_gpiocfg.irq_polarity = csa37f71_get_int_gpio_val()?\
									    HAL_GPIO_IRQ_POLARITY_LOW_FALLING:\
									    HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
	hal_gpio_setup_irq(pressure_int_cfg.pin, &csa37f71_int_gpiocfg);
}

static bool csa37f71_get_int_gpio_val(void)
{
	return hal_gpio_pin_get_val(pressure_int_cfg.pin);
}

static void csa37f71_int_handler(enum HAL_GPIO_PIN_T pin)
{
	CSA37F71_TRACE(2,"%s %d",__func__,csa37f71_get_int_gpio_val());
	csa37f71_int_irq_polarity_change();
	csa37f71_event_process();
}

static void csa37f71_int_init(void)
{

	csa37f71_int_gpiocfg.irq_enable = true;
	csa37f71_int_gpiocfg.irq_debounce = true;
	csa37f71_int_gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

	csa37f71_int_gpiocfg.irq_handler = csa37f71_int_handler;
	csa37f71_int_gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_int_cfg, 1);
    hal_gpio_pin_set_dir(pressure_int_cfg.pin, HAL_GPIO_DIR_IN, 1);
	hal_gpio_setup_irq(pressure_int_cfg.pin, &csa37f71_int_gpiocfg);
	dev_thread_handle_set(DEV_MODUAL_PRESSURE, csa37f71_handle_process);
}

//static void csa37f71_int_irq_enable(bool en)
//{
//	csa37f71_int_gpiocfg.irq_enable = en;
//	hal_gpio_setup_irq(pressure_int_cfg.pin, &csa37f71_int_gpiocfg);
//}

#if defined(CSA37F71_DATA_PRINTF_DEBUG)
static osTimerId dev_csa37f71_timer_debug = NULL;

static void csa37f71_check_timehandler_debug(void const *param);
osTimerDef(DEV_CSA37F71_TIMER_DEBUG, csa37f71_check_timehandler_debug);
static void csa37f71_check_timehandler_debug(void const *param)
{
//	static uint8_t flag = 1;
//	CS_RAWDATA_Def rawdata;
//	if(flag)
//	{
//		cs_press_read_rawdata_init();
//		flag = 0;
//	}
//	cs_press_read_rawdata(&rawdata);
//	CSA37F71_TRACE(1,"cs_press_read_rawdata:%d" ,rawdata.rawdata[0]);
}
#endif

static int32_t csa37f71_adapter_init(void)
{
	int ret = 0;
	uint16_t pressure;
	CSA37F71_TRACE(1,"%s enter" ,__FUNCTION__);
	
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pressure_reset_cfg, 1);
    hal_gpio_pin_set_dir(pressure_reset_cfg.pin, HAL_GPIO_DIR_OUT, 1);
	ret = cs_press_init();
	if(ret)
	{
		CSA37F71_TRACE(2,"%s fail ret:%d" ,__FUNCTION__,ret);
		return ret;
	}
	
	for(uint8_t i=0;i<3;i++)
	{
		uint8_t ret2 = cs_press_read_press_level(&pressure);
		if((250 != pressure)&&(0 == ret2))
		{
			pressure = 250;
			cs_press_write_press_level(pressure);
			break;
		}
	}

	for(uint8_t j=0;j<3;j++)
	{
		pressure = 0;
		uint8_t ret2 = cs_press_read_calibration_factor(0, &pressure);
		if(0 == ret2)
		{
			CSA37F71_TRACE(2,"%s calibration_factor:%d" ,__FUNCTION__,pressure);
			break;
		}
	}
	
	for(uint8_t k=0;k<3;k++)
	{
		pressure = 0;
		uint8_t ret2 = cs_press_read_press_level(&pressure);
		if(0 == ret2)
		{
			CSA37F71_TRACE(2,"%s press_level:%d" ,__FUNCTION__,pressure);
			break;
		}
	}
	cs_press_set_device_sleep();	//init Default sleep
	
	if (app_csa37f71_timer == NULL)
	{
	   app_csa37f71_timer = osTimerCreate (osTimer(APP_CSA37F71_TIMER), osTimerOnce, NULL);
	   //osTimerStart(app_csa37f71_timer, 500);
	}
#if defined(CSA37F71_DATA_PRINTF_DEBUG)
	if (dev_csa37f71_timer_debug == NULL)
	{
	   dev_csa37f71_timer_debug = osTimerCreate (osTimer(DEV_CSA37F71_TIMER_DEBUG), osTimerPeriodic, NULL);
	   osTimerStart(dev_csa37f71_timer_debug, 500);
	}
#endif
	csa37f71_int_init();
	CSA37F71_TRACE(1,"%s exit" ,__FUNCTION__);
	return ret;
}

static int32_t csa37f71_adapter_get_id(uint32_t *chip_id)
{
	CS_FW_INFO_Def fw_info;

	if (NULL == chip_id)
		return -1;
	cs_press_read_fw_info(&fw_info);
	CSA37F71_TRACE(4,"%s manufacturer_id:0x%04x,module_id:0x%04x,fw_version:0x%04x" ,__FUNCTION__,\
					fw_info.manufacturer_id,fw_info.module_id,fw_info.fw_version);
	*chip_id = fw_info.manufacturer_id;	//manufacturer_id£º0xc006
	
	CSA37F71_TRACE(2,"%s:0x%04x" ,__FUNCTION__,*chip_id);
	return 0;
}

static int32_t csa37f71_adapter_enter_standby_mode(void)
{
	uint8_t ret = 0;
	ret = cs_press_set_device_sleep();
	CSA37F71_TRACE(2,"%s ret:%d exit" ,__FUNCTION__,ret);
    return 0;
}

static int32_t csa37f71_adapter_enter_detection_mode(void)
{
	uint8_t ret = 0;
	ret = cs_press_set_device_wakeup();
	CSA37F71_TRACE(2,"%s ret:%d exit" ,__FUNCTION__,ret);
    return 0;
}

static int32_t csa37f71_adapter_report_gesture_event(hal_pressure_event_e gesture_type)
{
    if(NULL == csa37f71_gesture_event_cb) {
        return -1;
    }
    
    csa37f71_gesture_event_cb(gesture_type, 0, 0);

    return 0;
}

static int32_t csa37f71_adapter_set_mode(uint8_t mode)
{
    return 0;
}

static int32_t csa37f71_adapter_set_sensitivity(uint8_t leve)
{
	uint16_t press_level = 0;
	if(0 == leve)
	{
		press_level = CSA37F71_PRESS_LEVEL_LOW;
		cs_press_read_press_level(&press_level);
	}
	else if(1 == leve)
	{
		press_level = CSA37F71_PRESS_LEVEL_MID;
		cs_press_read_press_level(&press_level);
	}
	else if(2 == leve)
	{
		press_level = CSA37F71_PRESS_LEVEL_HIG;
		cs_press_read_press_level(&press_level);
	}
	
    return 0;
}

static int32_t csa37f71_adapter_set_gesture_event_cb(hal_pressure_gesture_event_cb cb)
{
    if (NULL == cb)
        return -1;
        
    csa37f71_gesture_event_cb = cb;

    return 0;
}

static int32_t csa37f71_adapter_enter_pt_calibration(uint8_t items,uint8_t *data)
{
	int32_t ret = -1;
	uint8_t len = 0;
	static uint8_t read_init_flag = 0;
	uint16_t pressure = 0;
	CS_CALIBRATION_RESULT_Def calibration_result;
	CS_RAWDATA_Def rawdata;
	CS_PROCESSED_DATA_Def proce_data;
	CS_NOISE_DATA_Def noise_data;
	CS_OFFSET_DATA_Def offset_data;
	
	switch(items)
	{
		case CS_CALI_DEVICE_WAKEUP_ITEM:
			if(false == data[0])
				ret = cs_press_set_device_sleep();
			else
				ret = cs_press_set_device_wakeup();
			CSA37F71_TRACE(2,"%s %s", csa37f71_cali_items_string[items],data[0]?"en wakeup":"dis sleep");
		  break;
		case CS_CALI_EN_ITEM:
			ret = cs_press_calibration_enable();
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_CHECK_ITEM:
			ret = cs_press_calibration_check(&calibration_result);
			len = sizeof(CS_CALIBRATION_RESULT_Def);
			data[0] = len;
			memcpy(&data[1],&calibration_result,len);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_DISEN_ITEM:
			ret = cs_press_calibration_disable();
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_RAWDATA_ITEM:
			if(read_init_flag != CS_CALI_READ_RAWDATA_ITEM)
			{
				cs_press_read_rawdata_init();
				osDelay(15);
				read_init_flag = CS_CALI_READ_RAWDATA_ITEM;
			}
			ret = cs_press_read_rawdata(&rawdata);
			len = sizeof(CS_RAWDATA_Def);
			data[0] = len;
			memcpy(&data[1],&rawdata,len);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_PROCESSED_DATA_ITEM:
			if(read_init_flag != CS_CALI_READ_PROCESSED_DATA_ITEM)
			{
				cs_press_read_processed_data_init();
				osDelay(15);
				read_init_flag = CS_CALI_READ_PROCESSED_DATA_ITEM;
			}
			ret = cs_press_read_processed_data(&proce_data);
			len = sizeof(CS_PROCESSED_DATA_Def);
			data[0] = len;
			memcpy(&data[1],&proce_data,len);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_NOISE_ITEM:
			if(read_init_flag != CS_CALI_READ_NOISE_ITEM)
			{
				cs_press_read_noise_init(100);
				read_init_flag = CS_CALI_READ_NOISE_ITEM;
				//osDelay(1000);
			}
			ret = cs_press_read_noise(&noise_data);
			len = sizeof(CS_NOISE_DATA_Def);
			data[0] = len;
			memcpy(&data[1],&noise_data,len);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_OFFSET_ITEM:
			ret = cs_press_read_offset(&offset_data);
			len = sizeof(CS_OFFSET_DATA_Def);
			data[0] = len;
			memcpy(&data[1],&offset_data,len);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_PRESSURE_ITEM:
			ret = cs_press_read_pressure(&pressure);
			if(NULL != data)
			{
				data[0] = 2;
				data[1] = pressure&0xff;
				data[2] = (pressure>>8)&0xff;
			}
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_PRESS_LEVEL_ITEM:
			ret = cs_press_read_press_level(&pressure);
			if(NULL != data)
			{
				data[0] = 2;
				data[1] = pressure&0xff;
				data[2] = (pressure>>8)&0xff;
			}
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_WRITE_PRESS_LEVEL_ITEM:
			pressure = data[0]|(data[1]<<8);
			ret = cs_press_write_press_level(pressure);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_READ_CALIBRATION_FACTOR_ITEM:
			ret = cs_press_read_calibration_factor(0, &pressure);
			if(NULL != data)
			{
				data[0] = 2;
				data[1] = pressure&0xff;
				data[2] = (pressure>>8)&0xff;
			}
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		case CS_CALI_WRITE_CALIBRATION_FACTOR_ITEM:
			pressure = data[0]|(data[1]<<8);
			ret = cs_press_write_calibration_factor(0, pressure);
			CSA37F71_TRACE(1,"%s", csa37f71_cali_items_string[items]);
		  break;
		default:
		  break;
	}
	CSA37F71_TRACE(3,"%s items:%d ret:%d", __FUNCTION__, items, ret);
    return ret;
}

const hal_pressure_ctx_s c_csa37f71_pressure_ctx =
{
	csa37f71_adapter_init,
	NULL,									//reset
	csa37f71_adapter_get_id,				//get chip id
	csa37f71_adapter_enter_standby_mode,	//enter standby
	csa37f71_adapter_enter_detection_mode,	//enter detection
	csa37f71_adapter_set_mode,
	csa37f71_adapter_set_sensitivity,
	csa37f71_adapter_set_gesture_event_cb,
	csa37f71_adapter_enter_pt_calibration,
};

