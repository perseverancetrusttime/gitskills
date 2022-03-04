#if defined(__CHARGER_CW6305__)
#include "cw6305_adapter.h"
#include "cellwise_charger_cw6305.h"
#include "hal_charger.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "xspace_ram_access.h"

#define CW6305_TRACE(num, str, ...)  TRACE(num, "[CW6035]" str, ##__VA_ARGS__)


static int32_t cw6305_set_charging_voltage(uint16_t voltage);
static int32_t cw6305_set_charging_current(hal_charging_current_e current);
static int32_t cw6305_set_charging_status(hal_charging_ctrl_e status);
static int32_t cw6305_set_box_dummy_load_switch(uint8_t status);

//#######################################################################
static uint16_t u16ChargingCurrent = 0;
static uint16_t u16ChargingVoltage = 0;
extern uint8_t charge_ic_full_charge_flag;

static void cw6305_event_process(void)
{
    dev_message_block_s msg;
    msg.mod_id = DEV_MODUAL_CHARGER;

    dev_thread_mailbox_put(&msg);
}

static int cw6305_handle_process(dev_message_body_s *msg_body)
{
    uint8_t u8IntSrc = 0;
	u8IntSrc = cwget_Interrupt_Source();

	CW6305_TRACE(2, "%s INT SRC:0X%x\n", __func__, u8IntSrc);

	if(INT_SRC_CHG_DET & u8IntSrc)
	{
		CW6305_TRACE(0, "###check charging###\n");		
	}
	if(INT_SRC_CHG_FINISH & u8IntSrc)
	{
		cw6305_set_box_dummy_load_switch(0x01);
		CW6305_TRACE(0, "### charge finish###\n");
	}
	if(INT_SRC_CHG_REMOVE & u8IntSrc)
	{
		CW6305_TRACE(0, "## charge remove###\n");
	}
	return 0;

}
  

void cw6305_int_handler(enum HAL_GPIO_PIN_T pin)
{
	//CW6305_TRACE("%s",__func__);
	cw6305_event_process();

}

void cw6305_int_init(void)
{
	struct HAL_GPIO_IRQ_CFG_T gpiocfg;

	gpiocfg.irq_enable = true;
	gpiocfg.irq_debounce = true;
	gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;

	gpiocfg.irq_handler = cw6305_int_handler;
	gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
	hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_charger_int_cfg, 1);
	hal_gpio_pin_set_dir((HAL_GPIO_PIN_T)app_charger_int_cfg.pin, HAL_GPIO_DIR_IN, 0);
	hal_gpio_setup_irq((HAL_GPIO_PIN_T)app_charger_int_cfg.pin, &gpiocfg);
	dev_thread_handle_set(DEV_MODUAL_CHARGER, cw6305_handle_process);

}
//####################################################################
#ifdef POLLING_STATUS_REPORT
static void cw6305_timehandler(void const *param);
osTimerDef(APP_CW6305_TIMER, cw6305_timehandler);
static osTimerId app_cw6305_timer = NULL;

static void cw6305_timehandler(void const *param)
{
	uint8_t u8Status = 0;
	uint8_t charger_flag = 0;
	
	u8Status = cwget_Condition_II();
	CW6305_TRACE(1, "CII(0X%02x)\n", u8Status);
	if(BIT5 & u8Status)
	{
		charger_flag = 1;
		CW6305_TRACE(0, "The Dev in charging state!\n");
	}
	else
	{
		charger_flag = 0;
		charge_ic_full_charge_flag = 2;
		//CW6305_TRACE(0, "The Dev no charging!\n");
	}
	
	if(BIT6 & u8Status)
		CW6305_TRACE(0, "The VBUS is over vol!\n");
	
	if(1 == charger_flag)
	{
		u8Status = 0;
		u8Status = cwget_Condition();
		CW6305_TRACE(2, "%s u8Status(0X%02x)\n", __func__, u8Status);
		if(BIT3 & u8Status)
		{
			cw6305_set_box_dummy_load_switch(0x21);
			CW6305_TRACE(0,"chariging finished\n");
		}
		else
		{
			cw6305_set_box_dummy_load_switch(0x00);
		}
		
		if(BIT4 & u8Status)
			CW6305_TRACE(0, "CV charing state\n");
		if(BIT5 & u8Status)
			CW6305_TRACE(0, "CC charging state\n");
	}
}
#endif
//#####################################################################

static int32_t cw6305_init(void)
{
	uint8_t u8DevId = 0,read_times = 20;

    while((CW6305_DEV_ID_VALUE != u8DevId)&&((read_times--) > 0))
	{
	    u8DevId = (uint8_t)cwget_version();
		CW6305_TRACE(2, "%s ID = 0X%02x",__func__, u8DevId);
    }
	
	if(CW6305_DEV_ID_VALUE != u8DevId)
	{
		CW6305_TRACE(2, "%s ID = 0X%02xFailed\n", __func__, u8DevId);
		return -1;
	}
	cwset_clean_interrupt_source(8); //Clear interrupt source data
	cw6305_set_charging_voltage(4350); // Charge finish voltage
	cwset_Vbus_input_vol_limit(4750);
	cwset_Vsys_regualation_vol(4700);
	cwset_Vbus_input_cur_limit(200);
	cwset_prechg_to_FCCchg_vol(1);      //disable force charger
	cwset_IC_internal_max_thermal(100);//teper = 100degree
	cwset_prechg_cur(100);  // pre cur(<3.0V) = 10%*cc
	cwset_TCC_cur(100);   //stop cur = 10%
	cwset_int_btn_func_enable(0);
	cwset_chg_watchdog_enable(1);		/*0x08 Bit7*/
    cwset_dischg_watchdog_enable(0);
    cwset_watchdog_timer(180);//180s
	cwset_UVLO_vol(3000);     //shut down vol
	cwset_2X_safety_timer_PPM(0);
	cwset_vsys_over_cur_protection_cur(200);

	cwset_rechg_vol_step(RECHG_VOL_200MV);// Default value
	cw6305_set_charging_current(HAL_CHARGING_CTRL_2C);
	cw6305_set_charging_status(HAL_CHARGING_CTRL_ENABLE);
	
	CW6305_TRACE(1, "%s vol:4200,cur:2c\n", __func__);

#ifdef 	POLLING_STATUS_REPORT
	if (app_cw6305_timer == NULL)
	{
	   app_cw6305_timer = osTimerCreate (osTimer(APP_CW6305_TIMER), osTimerPeriodic, NULL);
	}
	osTimerStart(app_cw6305_timer, 10*1000);
#endif

	cw6305_int_init();
	cwset_chg_interrupt(INT_SRC_CHG_FINISH);
	cwset_chg_enable(CHG_ENABLE); // charging enable

	CW6305_TRACE(2, "%s ok, ID(0X%02x)!!!", __func__, u8DevId);


	return 0;
}

static int32_t cw6305_get_chip_id(uint32_t *chip_id)
{
	*chip_id = cwget_version();
	
	return 0;
}

static int32_t cw6305_get_charging_current(hal_charging_current_e *status)
{
	if(u16ChargingCurrent == 15)
	{
		*status = HAL_CHARGING_CTRL_HALF;
		return 0;
	}
	else if(u16ChargingCurrent == 30)
	{
		*status = HAL_CHARGING_CTRL_1C;
		return 0;
	}
	else if(u16ChargingCurrent == 70)
	{
		*status = HAL_CHARGING_CTRL_2C;
		return 0;
	}

	return -1;
}
/*
Range: 10mA-470mA
*/
static int32_t cw6305_set_charging_current(hal_charging_current_e current)
{
	uint32_t u32FastCurrent;
	uint32_t u32TccCurrent;
	
	int retVal;

	CW6305_TRACE(2, "%s current(%d)\n",__func__, current);
	if(current >= HAL_CHARGING_CTRL_TOTAL_NUM)
	{
		CW6305_TRACE(0, "Invalid current set\n");
		return -1;
	}
	if(HAL_CHARGING_CTRL_HALF == current)
	{
		u32FastCurrent = 15; //0.45c
		u32TccCurrent = 200;
	}
	else if(HAL_CHARGING_CTRL_1C == current)
	{
		u32FastCurrent = 30;
		u32TccCurrent = 100;
	}
	else if(HAL_CHARGING_CTRL_2C == current)
	{
		u32FastCurrent = 70; //2c
		u32TccCurrent = 50;
	}
	else
	{
		CW6305_TRACE(1, "%s Invalid\n", __func__);
		return -1;
	}
	retVal = cwset_FCCchg_cur(u32FastCurrent);
	if(0 != retVal)
	{
		CW6305_TRACE(2, "%s err(%d)\n", __func__,retVal);
		return -1;
	}

	u16ChargingCurrent = u32FastCurrent;
	
	//set stop cur
	retVal = cwset_TCC_cur(u32TccCurrent);
	if(0 != retVal)
	{
		CW6305_TRACE(2, "%s cwset_TCC_cur err(%d)\n", __func__,retVal);
		return -1;
	}
	
	return 0;

}

static int32_t cw6305_get_charging_status(hal_charging_ctrl_e *status)
{
	uint8_t u8IcStatus;
	u8IcStatus = cwget_Condition_II();
	if(STATUS_IC_CHARGING & u8IcStatus)
	{
		*status = HAL_CHARGING_CTRL_ENABLE;
		return 0;
	}
	else if(STATUS_IC_CHARGING & u8IcStatus)
	{
		*status = HAL_CHARGING_CTRL_DISABLE;
		return 0;
	}
	return -1;
}

static int32_t cw6305_set_charging_status(hal_charging_ctrl_e status)
{
	CW6305_TRACE(2, "%s status(%d)\n", __func__, status);
	if((HAL_CHARGING_CTRL_ENABLE != status)&(HAL_CHARGING_CTRL_DISABLE != status))
		return -1;
	
	if(HAL_CHARGING_CTRL_ENABLE == status)
	{
	//  cwset_HiZ_control();
		cwset_chg_enable(CHG_ENABLE);
	}
	else
	{
		cwset_chg_enable(CHG_DISABLE);
	}
	
	return 0;
}

static int32_t cw6305_get_charging_voltage(uint16_t *voltage)
{
	*voltage = u16ChargingVoltage;
	return 0;
}
/*
Charging Finish Voltage
Range: 3.85V-4.6V
*/
static int32_t cw6305_set_charging_voltage(uint16_t voltage)
{
	int32_t retVal = 0;


	CW6305_TRACE(2, "%s voltage(%d)\n",__func__, voltage);
	retVal = cwset_CV_vol(voltage);
	if(/*EINVAL*/0 != retVal)
	{
		CW6305_TRACE(2, "%s err(%d)\n",__func__, retVal);
		return -1;
	}
	u16ChargingVoltage = voltage;
	
	return 0;
}

static int32_t cw6305_set_chg_watchdog(bool status)
{
    int ret;
    CW6305_TRACE(2, "%s status(%d)\n", __func__, status);

    if(HIZ_CONTROL_ENABLE == status)
    {
        ret = cwset_chg_watchdog_enable(1);       /*0x08 Bit7*/
    }
    else
    {
        ret = cwset_chg_watchdog_enable(0);       /*0x08 Bit7*/
    }
    
    return ret;
}

int32_t cw6305_cutOff_Vsys()
{
	int status;
	cwset_bat_off_delay_time(4);//after 2s cut off vcc
    status=cwset_force_bat_off(1);     //cut off vcc
    
	cw6305_set_chg_watchdog(0);//Enter shipping mode to turn off the timer dog
    
    CW6305_TRACE(2, "%s status(%d)\n", __func__, status);
    return 0;
}

static int32_t cw6305_set_HiZ_status(hal_charging_ctrl_e status)
{
    CW6305_TRACE(2, "%s status(%d)\n", __func__, status);

    if(HIZ_CONTROL_ENABLE == status)
    {
      cwset_HiZ_control(HIZ_CONTROL_ENABLE);
    }
    else
    {
      cwset_HiZ_control(HIZ_CONTROL_DISABLE);
    }
    
    return 0;
}

//parameter status: 
//bit0:dummy load switch 1 open 0 close
//bit4:out box set 
//bit5:charge full set
//bit6:box open set 
static int32_t cw6305_set_box_dummy_load_switch(uint8_t status)
{
	
	static uint8_t inout_box_dummy_load_switch_flag = 0;
	static uint8_t box_open_dummy_load_switch_flag = 0;
	static uint32_t cw6305_full_ticks = 0; 
	static uint32_t cw6305_diff_ticks = 0;
	enum PMU_CHARGER_STATUS_T charger_status;
    uint32_t bat_per = 0;
    uint32_t bat_energy = 0;
	//static uint8_t current_load_status = 0xff;
	
    //CW6305_TRACE(2, "%s status(0x%02x)\n", __func__, status);
	
	charger_status = pmu_charger_get_status();
	
	if(0x01 & status)
	{
		if(0x10 & status)
		{
			inout_box_dummy_load_switch_flag = 1;
		}
		else if(0x20 & status)
		{
			if(1 != charge_ic_full_charge_flag)
			{
				cw6305_full_ticks = hal_sys_timer_get();
				charge_ic_full_charge_flag = 1;
				CW6305_TRACE(1,"charge_ic_full_charge_flag = %d\n",charge_ic_full_charge_flag);
			}
		}
		else if(0x40 & status)
		{
			box_open_dummy_load_switch_flag = 1;
		}
		
		if(0 == hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)dummy_load_enable_cfg.pin))
		{
			hal_gpio_pin_set((enum HAL_GPIO_PIN_T)dummy_load_enable_cfg.pin);
			CW6305_TRACE(0,"open dummy load\n");
		}
		
	}
	else
	{
		cw6305_diff_ticks = hal_timer_get_passed_ticks(hal_sys_timer_get(),cw6305_full_ticks);
		if(((cw6305_diff_ticks > MS_TO_TICKS(300*1000))||(cw6305_full_ticks == 0))
			&& (0 != charge_ic_full_charge_flag))
		{
			charge_ic_full_charge_flag = 0;
			CW6305_TRACE(1,"charge_ic_full_charge_flag = %d\n",charge_ic_full_charge_flag);
		}
			
		if(0x40 & status)
		{
			box_open_dummy_load_switch_flag = 0;
		}
		
		if(1 == hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)dummy_load_enable_cfg.pin))
		{
			xra_get_earphone_battery_data_from_ram(&bat_per,&bat_energy);
		
			if((charger_status == PMU_CHARGER_PLUGIN) && (bat_per <= 98)
				&& ( 1 != charge_ic_full_charge_flag)
				&& (!inout_box_dummy_load_switch_flag) && (!box_open_dummy_load_switch_flag))
			{
				hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)dummy_load_enable_cfg.pin);
				CW6305_TRACE(0,"close dummy load\n");
			}
		
			if(inout_box_dummy_load_switch_flag)
				inout_box_dummy_load_switch_flag = 0;
		}

	}
    return 0;
}

//parameter status: 
//bit1:1 box usb plug in. 0 box usb plug out.
//bit7:1 box otg mode.  0 box boost mode.
static int32_t cw6305_set_vbus_vsys_limit(uint8_t status)
{
    uint32_t bat_per = 0;
    uint32_t bat_energy = 0;
	static uint8_t current_vbs_status = 1;
	
    CW6305_TRACE(2, "%s status(%d)\n", __func__, status);

	if((0x01 & status) && (0x80 & status))	//box usb plug in and box otg mode
	{
		xra_get_earphone_battery_data_from_ram(&bat_per,&bat_energy);
		if(bat_per < 60)
		{
			if(current_vbs_status ==  1)
			{
				cw6305_set_charging_voltage(4150); // Charge finish voltage
				cwset_Vbus_input_vol_limit(4300);
				cwset_Vsys_regualation_vol(4250);
				current_vbs_status = 0;
			}
		}
		else
		{
			if(current_vbs_status ==  0)
			{
				cw6305_set_charging_voltage(4350); // Charge finish voltage
				cwset_Vbus_input_vol_limit(4750);
				cwset_Vsys_regualation_vol(4700);
				current_vbs_status = 1;
			}
		}
	}
	else	//box usb plug out or box boost mode
	{
		if(current_vbs_status ==  0)
		{
			cw6305_set_charging_voltage(4350); // Charge finish voltage
			cwset_Vbus_input_vol_limit(4750);
			cwset_Vsys_regualation_vol(4700);
			current_vbs_status = 1;
		}
	}

    return 0;
}

const hal_charger_ctrl_s cw6305_ctrl =
{
    cw6305_init,
    cw6305_get_chip_id,
    cw6305_get_charging_current,
    cw6305_set_charging_current,
    cw6305_get_charging_status,
    cw6305_set_charging_status,
    NULL,
    cw6305_get_charging_voltage,
    cw6305_set_charging_voltage,
    cw6305_cutOff_Vsys,
    cw6305_set_HiZ_status,
    cw6305_set_chg_watchdog,
    cw6305_set_box_dummy_load_switch,
    cw6305_set_vbus_vsys_limit,
};

#endif /* __CHARGER_CW6305__ */
