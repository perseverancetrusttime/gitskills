#if defined(__BAT_ADC_INFO_GET__)
#include "cmsis.h"
#include "Cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_gpadc.h"
#include "tgt_hardware.h"
#include "nvrecord_env.h"
#include "dev_thread.h"
#include "drv_bat_adc.h"
#include "hal_battery.h"
#include "adc_bat_cap_coulombmeter.h"
#include "hal_charger.h"
#include "xspace_flash_access.h"
#include "xspace_ram_access.h"
#include "pmu.h"
#include "xspace_interface.h"
#include "xspace_interface_process.h"
#if defined(__XSPACE_BATTERY_MANAGER__)
#include "xspace_battery_manager.h"
#endif
#define BAT_ADC_SET_MESSAGE(appevt, status, volt) (appevt = (((uint32_t)status&0xffff)<<16)|(volt&0xffff))
#define BAT_ADC_GET_STATUS(appevt, status) (status = (appevt>>16)&0xffff)
#define BAT_ADC_GET_VOLT(appevt, volt) (volt = appevt&0xffff)

#define BAT_ADC_CC_TO_CV_VOLT				4000		//unit: mV
#define BAT_ADC_CV_TO_END_SECOND			(60*15.00f)	//unit: Second
#define BAT_ADC_CV_TO_END_INCRE_VOLT		200			//unit: mV
#define ADC_BAT_VOL_INFO_CALIB_PARA(num)		(num*10105/10000)

static bat_adc_measure_s bat_adc_measure;
static hal_bat_query_cb bat_adc_query_cb = NULL;

#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
static const bat_volt_level_s charging_curve[] = {
		{ 2750, (uint16_t)((25.0f - 25.00f) * 100 * 256 / 25.0f)},
		{ 3106, (uint16_t)((25.0f - 25.00f) * 100 * 256 / 25.0f)},
		{ 3350, (uint16_t)((25.0f - 25.00f) * 100 * 256 / 25.0f)},
		{ 3784, (uint16_t)((25.0f - 22.50f) * 100 * 256 / 25.0f)},
		{ 3821, (uint16_t)((25.0f - 21.25f) * 100 * 256 / 25.0f)},
		{ 3849, (uint16_t)((25.0f - 20.00f) * 100 * 256 / 25.0f)},
		{ 3856, (uint16_t)((25.0f - 18.75f) * 100 * 256 / 25.0f)},
		{ 3882, (uint16_t)((25.0f - 17.50f) * 100 * 256 / 25.0f)},
		{ 3893, (uint16_t)((25.0f - 16.25f) * 100 * 256 / 25.0f)},
		{ 3904, (uint16_t)((25.0f - 15.00f) * 100 * 256 / 25.0f)},
		{ 3918, (uint16_t)((25.0f - 13.75f) * 100 * 256 / 25.0f)},
		{ 3935, (uint16_t)((25.0f - 12.50f) * 100 * 256 / 25.0f)},
		{ 3954, (uint16_t)((25.0f - 11.25f) * 100 * 256 / 25.0f)},
		{ 3982, (uint16_t)((25.0f - 10.00f) * 100 * 256 / 25.0f)},
		{ 4015, (uint16_t)((25.0f - 8.75f) * 100 * 256 / 25.0f)},
		{ 4042, (uint16_t)((25.0f - 7.50f) * 100 * 256 / 25.0f)},
		{ 4071, (uint16_t)((25.0f - 6.25f) * 100 * 256 / 25.0f)},
		{ 4105, (uint16_t)((25.0f - 5.00f) * 100 * 256 / 25.0f)},
		{ 4143, (uint16_t)((25.0f - 3.75f) * 100 * 256 / 25.0f)},
		{ 4186, (uint16_t)((25.0f - 2.50f) * 100 * 256 / 25.0f)},
		{ 4200, (uint16_t)((25.0f - 0.00f) * 100 * 256 / 25.0f)},
		{ 4206, (uint16_t)((25.0f - 0.00f) * 100 * 256 / 25.0f)},
		{ 4230, (uint16_t)((25.0f - 0.00f) * 100 * 256 / 25.0f)},
};

static const bat_volt_level_s discharging_curve[] = {
		{ 2750, (uint16_t)((24.0f - 24.00f) * 100 * 256 / 24.0f)},
		{ 2799, (uint16_t)((24.0f - 24.00f) * 100 * 256 / 24.0f)},
		{ 3350, (uint16_t)((24.0f - 24.00f) * 100 * 256 / 24.0f)},
		{ 3577, (uint16_t)((24.0f - 22.80f) * 100 * 256 / 24.0f)},
		{ 3649, (uint16_t)((24.0f - 21.60f) * 100 * 256 / 24.0f)},
		{ 3677, (uint16_t)((24.0f - 20.40f) * 100 * 256 / 24.0f)},
		{ 3699, (uint16_t)((24.0f - 19.20f) * 100 * 256 / 24.0f)},
		{ 3716, (uint16_t)((24.0f - 18.00f) * 100 * 256 / 24.0f)},
		{ 3726, (uint16_t)((24.0f - 16.80f) * 100 * 256 / 24.0f)},
		{ 3736, (uint16_t)((24.0f - 15.60f) * 100 * 256 / 24.0f)},
		{ 3747, (uint16_t)((24.0f - 14.40f) * 100 * 256 / 24.0f)},
		{ 3760, (uint16_t)((24.0f - 13.20f) * 100 * 256 / 24.0f)},
		{ 3777, (uint16_t)((24.0f - 12.00f) * 100 * 256 / 24.0f)},
		{ 3796, (uint16_t)((24.0f - 10.80f) * 100 * 256 / 24.0f)},
		{ 3818, (uint16_t)((24.0f - 9.60f) * 100 * 256 / 24.0f)},
		{ 3846, (uint16_t)((24.0f - 8.40f) * 100 * 256 / 24.0f)},
		{ 3875, (uint16_t)((24.0f - 7.20f) * 100 * 256 / 24.0f)},
		{ 3906, (uint16_t)((24.0f - 6.00f) * 100 * 256 / 24.0f)},
		{ 3941, (uint16_t)((24.0f - 4.80f) * 100 * 256 / 24.0f)},
		{ 3979, (uint16_t)((24.0f - 3.60f) * 100 * 256 / 24.0f)},
		{ 4000, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4021, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4050, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4072, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4080, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4100, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4177, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4200, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
		{ 4230, (uint16_t)((24.0f - 0.00f) * 100 * 256 / 24.0f)},
};

static uint8_t volt_to_level(const bat_volt_level_s *curve, uint32_t curve_size, HAL_GPADC_MV_T volt)
{
    uint32_t l = 0, h = curve_size - 1, m = 0;
    while (l <= h) {
        m = (l + h) / 2;
        if (volt < curve[m].mv)
            h = m - 1;
        else if (volt > curve[m].mv)
            l = m + 1;
        else{
			return curve[m].level / 256;
		}

    }

    if (h < m) {
        if (m == 0){
            return curve[m].level / 256;
        }
        m--;
    } else {
        if (m == curve_size - 1){
            return curve[m].level / 256;
        }
    }

    return (curve[m].level + (volt - curve[m].mv) * (curve[m + 1].level - curve[m].level) / (curve[m + 1].mv - curve[m].mv)) / 256;
}

static HAL_GPADC_MV_T volt_mean_filter(const HAL_GPADC_MV_T *volts, unsigned int count)
{
    unsigned int sum = 0;
    HAL_GPADC_MV_T min = (HAL_GPADC_MV_T)~0U, max = 0;
    unsigned int i;

    for (i = 0; i < count; i++) {
        sum += volts[i];
        if (min > volts[i])
            min = volts[i];
        if (max < volts[i])
            max = volts[i];
    }

    if (count < BAT_ADC_STABLE_COUNT-1)
        return sum / count;
    else
        return (sum - max - min) / (count - 2);
}
#endif

#if 0
static HAL_GPADC_MV_T volt_charger_switch_filter(const HAL_GPADC_MV_T volt, bat_adc_status_e status)
{
	HAL_GPADC_MV_T vbat = 0;
	HAL_GPADC_MV_T vbat_correc = 0;
	HAL_GPADC_MV_T vbat_filter = 0;
	HAL_GPADC_MV_T cv_volt_incre = 0;
	static uint32_t cnt_10s = 0;

	vbat = volt*(10+16)/10;  //vbat = volt*(1M+1.6M)/1M
	vbat_correc = vbat + 0;		//GPADC2 deviation

	if(BAT_ADC_STATUS_CHARGING == bat_adc_measure.status){
		if(bat_adc_measure.curr_volt >= BAT_ADC_CC_TO_CV_VOLT){
			cnt_10s += 10;
			cnt_10s = (uint32_t)(cnt_10s>BAT_ADC_CV_TO_END_SECOND?BAT_ADC_CV_TO_END_SECOND:cnt_10s);
			cv_volt_incre = (HAL_GPADC_MV_T)(cnt_10s/BAT_ADC_CV_TO_END_SECOND*BAT_ADC_CV_TO_END_INCRE_VOLT);
			vbat_filter = BAT_ADC_CC_TO_CV_VOLT + cv_volt_incre;
			DRV_ADC_TRACE(4, "CV vbat_filter:%d, vbat_correc:%d,vbat:%d, cn_10s:%d\n", vbat_filter, vbat_correc,vbat, cnt_10s);
		}else{
			vbat_filter = vbat_correc;
			cnt_10s = 0;
			DRV_ADC_TRACE(3, "CC vbat_filter:%d, vbat_correc:%d, vbat:%d\n", vbat_filter, vbat_correc, vbat);
		}
	}else{
		vbat_filter = vbat_correc;
		cnt_10s = 0;
		DRV_ADC_TRACE(3, "dischgr  vbat_filter:%d, vbat_correc:%d, vbat:%d\n", vbat_filter,vbat_correc, vbat);
	}

	return vbat_filter;
}
#endif

void adc_bat_info_report(bat_adc_voltage volt, uint8_t level, uint8_t percentage)
{
    hal_bat_info_s bat_info;

	bat_adc_measure.curr_volt = volt;
	bat_adc_measure.curr_level = level;
	bat_adc_measure.curr_percentage = percentage;

	if (NULL != bat_adc_query_cb) {
		#if 0
		if(bat_adc_measure.curr_percentage > bat_adc_measure.report_percentage)
			!bat_adc_measure.report_percentage?bat_adc_measure.report_percentage = bat_adc_measure.curr_percentage:++bat_adc_measure.report_percentage;
		else if(bat_adc_measure.curr_percentage < bat_adc_measure.report_percentage)
			--bat_adc_measure.report_percentage;
		else
		#endif
			bat_adc_measure.report_percentage = bat_adc_measure.curr_percentage;

		bat_adc_measure.report_level = bat_adc_measure.report_percentage/10;
		bat_info.bat_volt = bat_adc_measure.curr_volt;
		bat_info.bat_level = (bat_adc_measure.report_level != 10)?bat_adc_measure.report_level:9;
		bat_info.bat_per = bat_adc_measure.report_percentage;
		DRV_ADC_TRACE(4, "status:%s, vol:%d, level:%d,percentage:%d%%!!!",
			(bat_adc_measure.status == BAT_ADC_STATUS_CHARGING)? "charging":"discharging",
			 bat_info.bat_volt,
			 bat_info.bat_level,
			 bat_info.bat_per);

		bat_adc_query_cb(bat_info);
	}
}

static int adc_bat_info_handle_process(dev_message_body_s *msg_body)
{
    uint8_t level = 0;
	uint8_t percentage =0;
    uint8_t status;
    bat_adc_voltage volt;

    BAT_ADC_GET_STATUS(msg_body->message_id, status);
    BAT_ADC_GET_VOLT(msg_body->message_id, volt);
	DRV_ADC_TRACE(2, "status:%d(0:dischr 1:charging 2:invalid) volt:%d \n", status, volt);
    switch (status)
    {
        case BAT_ADC_STATUS_DISCHARGING:
			#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
			percentage =volt_to_level(discharging_curve, ARRAY_SIZE(discharging_curve),volt);
			#else
			percentage = bat_capacity_conversion_interface(BAT_ADC_STATUS_DISCHARGING,volt);
			#endif

			DRV_ADC_TRACE(2, "Dischgr per:%d%%, curr per:%d%%\n", percentage,bat_adc_measure.curr_percentage);
			if(BAT_ADC_STATUS_DISCHARGING != bat_adc_measure.pre_status){
				bat_adc_measure.curr_percentage = bat_adc_measure.report_percentage;
			}
			bat_adc_measure.pre_status = BAT_ADC_STATUS_DISCHARGING;
			#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
			if(percentage > bat_adc_measure.curr_percentage && bat_adc_measure.curr_percentage){
				percentage = bat_adc_measure.curr_percentage;
			}
			#endif
			level = percentage/10;
			adc_bat_info_report(volt, level, percentage);
            break;

        case BAT_ADC_STATUS_CHARGING:

			#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
			percentage = volt_to_level(charging_curve, ARRAY_SIZE(charging_curve),volt);
			#else
			percentage = bat_capacity_conversion_interface(BAT_ADC_STATUS_CHARGING,volt);
			#endif
			if(BAT_ADC_STATUS_CHARGING != bat_adc_measure.pre_status){
				bat_adc_measure.curr_percentage = bat_adc_measure.report_percentage;
			}
			bat_adc_measure.pre_status = BAT_ADC_STATUS_CHARGING;
			DRV_ADC_TRACE(2, "Chgr per:%d%%, curr per:%d%%\n", percentage,bat_adc_measure.curr_percentage);
			#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
			if(percentage < bat_adc_measure.curr_percentage && bat_adc_measure.curr_percentage){
				percentage = bat_adc_measure.curr_percentage;
			}
			#endif
			level = percentage/10;
			adc_bat_info_report(volt, level, percentage);
            break;

        case BAT_ADC_STATUS_INVALID:
            DRV_ADC_TRACE(1, " BAT_ADC_STATUS_INVALID :%d", volt);
            break;

        default:
            break;
    }

    return 0;
}


static void adc_bat_info_adc_irqhandle(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    uint32_t meanBattVolt = 0;
    HAL_GPADC_MV_T vbat = 0;
    static uint8_t adc_irqhandler_init=0;
    uint32_t temp_currPercentage = 0;
    uint32_t temp_dump_energy = 0;
    hal_charging_ctrl_e charging_sta;

    DRV_ADC_TRACE(1, "-%d", volt);

    // TODO: change
	vbat = ADC_BAT_VOL_INFO_CALIB_PARA(volt)*4;//volt_charger_switch_filter(volt*3, bat_adc_measure.status);
	if(vbat >= bat_adc_measure.max_volt)
		vbat = bat_adc_measure.max_volt;
	if(vbat <= bat_adc_measure.min_volt)
		vbat = bat_adc_measure.min_volt;

    if (vbat == HAL_GPADC_BAD_VALUE) {
        bat_adc_measure.cb(BAT_ADC_STATUS_INVALID, vbat);
        return;
    }

    bat_adc_measure.voltage[bat_adc_measure.index++%BAT_ADC_STABLE_COUNT] = vbat;
	bat_adc_measure.count +=(bat_adc_measure.count < ARRAY_SIZE(bat_adc_measure.voltage));

	#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
	meanBattVolt = volt_mean_filter(bat_adc_measure.voltage, bat_adc_measure.count);
	#else
    if (bat_adc_measure.index > BAT_ADC_STABLE_COUNT) {
        for (uint8_t i=0; i<BAT_ADC_STABLE_COUNT; i++) {
            meanBattVolt += bat_adc_measure.voltage[i];
        }
        meanBattVolt /= BAT_ADC_STABLE_COUNT;
        if(adc_irqhandler_init == 0){
            hal_charger_set_charging_status(HAL_CHARGING_CTRL_ENABLE);
            if(meanBattVolt > 3700){
                //meanBattVolt -= 50;
            }
            adc_irqhandler_init = 1;
        }
    }
    else{
        if(adc_irqhandler_init == 0){
            if(!xra_get_earphone_battery_data_from_ram(&temp_currPercentage,&temp_dump_energy))
            {
                for(uint8_t i=0; i<BAT_ADC_STABLE_COUNT+1; i++){
                    bat_adc_measure.voltage[bat_adc_measure.index++%BAT_ADC_STABLE_COUNT] = vbat;
                }
				meanBattVolt = vbat;
            }
            else if(batterry_data_read_flash(&temp_currPercentage,&temp_dump_energy) == 0)
            {
                batterry_data_write_flash(temp_currPercentage,temp_dump_energy);
                for(uint8_t i=0; i<BAT_ADC_STABLE_COUNT+1; i++){
                    bat_adc_measure.voltage[bat_adc_measure.index++%BAT_ADC_STABLE_COUNT] = vbat;
                }
                meanBattVolt = vbat;
            }
            else
            {
                if(vbat > 3600){
                    if(!hal_charger_get_charging_status(&charging_sta)){
                        if(charging_sta == HAL_CHARGING_CTRL_ENABLE){
                            hal_charger_set_charging_status(HAL_CHARGING_CTRL_DISABLE);
                        }
                    }
                    for(uint8_t i=0; i<BAT_ADC_STABLE_COUNT; i++){
                        bat_adc_measure.voltage[bat_adc_measure.index++%BAT_ADC_STABLE_COUNT] = vbat;
                    }

					meanBattVolt = vbat;
                }
                else{
                    hal_charger_set_charging_status(HAL_CHARGING_CTRL_ENABLE);
                    for(uint8_t i=0; i<BAT_ADC_STABLE_COUNT+1; i++){
                        bat_adc_measure.voltage[bat_adc_measure.index++%BAT_ADC_STABLE_COUNT] = vbat;
                    }
                    meanBattVolt = vbat;

					DRV_ADC_TRACE(2, "%s vbat %d",__func__, vbat);
                }
            }
        }
    }
	#endif

    DRV_ADC_TRACE(4, "ADC bat curr_volt:%d,meanBattVolt :%d, vbat:%d, status:%d(0:dischgr,1:chgr)\n", bat_adc_measure.curr_volt, meanBattVolt,ADC_BAT_VOL_INFO_CALIB_PARA(volt)*4,bat_adc_measure.status);/*vbat = volt*(1M+1.6M)/1M*/
	#if !defined(__ADC_BAT_CAP_COULOMBMETER__)
	if(BAT_ADC_STATUS_CHARGING == bat_adc_measure.status){
		meanBattVolt = (meanBattVolt < bat_adc_measure.curr_volt)?bat_adc_measure.curr_volt:meanBattVolt;
	}else{
		meanBattVolt = (meanBattVolt > bat_adc_measure.curr_volt && bat_adc_measure.curr_volt)?bat_adc_measure.curr_volt:meanBattVolt;
	}
	#endif
	if(NULL != bat_adc_measure.cb){
		bat_adc_measure.cb(bat_adc_measure.status, meanBattVolt);
	}
}

static void adc_bat_info_adc_irqhandler(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)adc_bat_info_adc_irqhandle, (uint32_t)irq_val, (uint32_t)volt, 0);
}

static void adc_bat_info_event_process(bat_adc_status_e status, bat_adc_voltage volt)
{
    uint32_t app_battevt;
    dev_message_block_s msg;

    DRV_ADC_TRACE(2, "%d,%d", status, volt);
    msg.mod_id = DEV_MODULE_BATTERY;
    BAT_ADC_SET_MESSAGE(app_battevt, status, volt);
    msg.msg_body.message_id = app_battevt;
    msg.msg_body.message_ptr = (uint32_t)NULL;
	dev_thread_mailbox_put(&msg);
}

static void adc_bat_poweron_bat_low_check(bat_adc_status_e status)
{
    switch(status){
        case HAL_BAT_STATUS_DISCHARGING:
            if(bat_adc_measure.curr_volt <= (uint32_t )bat_algorithm_poweroff_volt_get(BAT_ADC_STATUS_DISCHARGING)) {
                DRV_ADC_TRACE(2, "Battery low,SHUT DOWN. bat status:%s, volt:%d", "discharging", bat_adc_measure.curr_volt);
                DRV_ADC_TRACE(0, "byebye~~~");
                osDelay(20);
                pmu_shutdown();
            }        
            break;
        case HAL_BAT_STATUS_CHARGING:            
        default:
            break;            
    }

    return;
}

static int32_t bat_adc_register_auto_report_cb(hal_bat_query_cb cb)
{
    if (cb == NULL)
        return -1;

    bat_adc_query_cb = cb;
    return 0;
}

static int32_t bat_adc_set_charging_status(hal_bat_charging_status_e status)
{
    DRV_ADC_TRACE(1, "status:%d\n", status);

    if(HAL_BAT_STATUS_DISCHARGING == status) {
        bat_adc_measure.status = BAT_ADC_STATUS_DISCHARGING;
    } else if (HAL_BAT_STATUS_CHARGING == status) {
        bat_adc_measure.status = BAT_ADC_STATUS_CHARGING;
    }

    return 0;
}

static int32_t bat_adc_start_oneshot_measure(void)
{
    if (ADC_BAT_START_INTERVAL_CAPTURE_STATUS == bat_adc_measure.capture_status) {
        DRV_ADC_TRACE(0, "Interval capture is not ending, please wait seconds!!!");
        return -1;
    }
#if defined(__GET_BAT_VOL_GPIO_EN__)
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)bat_adc_enable_cfg.pin);
    osDelay(1);
#endif
    hal_gpadc_open(HAL_GPADC_CHAN_BATTERY, HAL_GPADC_ATP_ONESHOT, adc_bat_info_adc_irqhandler);
#if defined(__GET_BAT_VOL_GPIO_EN__)
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)bat_adc_enable_cfg.pin);
#endif
    return 0;
}

static int32_t bat_adc_init(void)
{
    struct nvrecord_env_t *nvrecord_env;
#if defined(__XSPACE_BATTERY_MANAGER__)
	xbm_status_s status = XBM_STATUS_INVALID;
#endif
    dev_thread_handle_set(DEV_MODULE_BATTERY, adc_bat_info_handle_process);

    memset((void *)&bat_adc_measure, 0x00, sizeof(bat_adc_measure_s));
    bat_adc_measure.cb = adc_bat_info_event_process;
    bat_adc_measure.periodic = BAT_ADC_CIRCLE_FAST_MS;
    bat_adc_measure.capture_status = ADC_BAT_PASSIVE_CAPTURE;
    bat_adc_measure.max_volt = DRV_BAT_MAX_MV;
    bat_adc_measure.min_volt =DRV_BAT_MIN_MV;
    bat_adc_measure.pre_status = BAT_ADC_STATUS_INVALID;
#if defined(__GET_BAT_VOL_GPIO_EN__)
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&bat_adc_enable_cfg, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)bat_adc_enable_cfg.pin, HAL_GPIO_DIR_OUT, 0);
#endif
    nv_record_env_get(&nvrecord_env);

    DRV_ADC_TRACE(1, "env_local_bat:%d", nvrecord_env->local_bat);
    if(nvrecord_env->local_bat){
        bat_adc_measure.report_percentage = nvrecord_env->local_bat;
        bat_adc_measure.report_level = bat_adc_measure.report_percentage/10 != 10?
                                            bat_adc_measure.report_percentage/10:9;
    }

#if defined(__XSPACE_BATTERY_MANAGER__)
	status = xspace_get_battery_status();
    bat_adc_set_charging_status(XBM_STATUS_CHARGING == status ? HAL_BAT_STATUS_CHARGING:HAL_BAT_STATUS_DISCHARGING);
#endif

    bat_adc_start_oneshot_measure();

    do
    {
        osDelay(5);
    }
    while(!bat_adc_measure.curr_volt);

    DRV_ADC_TRACE(3, "int volt:%d,report per:%d%%,report level:%d\n",
        bat_adc_measure.curr_volt,bat_adc_measure.report_percentage, bat_adc_measure.report_level);

    adc_bat_poweron_bat_low_check(bat_adc_measure.status);

    return 0;
}

const hal_bat_ctrl_if_s bat_adc_ctrl =
{
    bat_adc_init,
    NULL,
    NULL,
    bat_adc_set_charging_status,
    bat_adc_register_auto_report_cb,
    bat_adc_start_oneshot_measure,
};

#endif   /*  __BAT_ADC_INFO_GET__  */

