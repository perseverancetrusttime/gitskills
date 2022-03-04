#if defined(__CHARGER_SY5500__)
#include "sy5500_adapter.h"
#include "hal_charger.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "xspace_i2c_master.h"

#define SY5500_TRACE(num, str, ...)  //TRACE(num, "[SY5500]" str, ##__VA_ARGS__)

static uint16_t u16ChargingCurrent = 0;
static uint16_t u16ChargingVoltage = 0;
extern uint8_t charge_ic_full_charge_flag;

static int32_t sy5500_set_charging_voltage(uint16_t voltage);
static int32_t sy5500_set_charging_current(hal_charging_current_e current);
static int32_t sy5500_set_charging_status(hal_charging_ctrl_e status);

/***************************SY5500 I2C REG OPERATING STAR***************************/
static int8_t sy5500_i2c_read(uint8_t PointReg,uint8_t *pData)
{
	bool bRet = true;
	bRet = xspace_i2c_read(XSPACE_SW_I2C, SY5500_DEV_ADDR, PointReg, pData);
	return bRet;
}

static int8_t sy5500_i2c_write(uint8_t PointReg,uint8_t *pData)
{
	bool bRet = true;
	bRet = xspace_i2c_write(XSPACE_SW_I2C, SY5500_DEV_ADDR, PointReg, *pData);
	return bRet;
}

/*static function, PointReg: Register location, bit_start: The start bit you want set, bit_num : How many bit do you want set, val : value */
static int8_t sy5500_set_reg_bit(uint8_t PointReg, uint8_t bit_start, uint8_t bit_num, uint8_t val)
{
    int8_t ret = 0;
    uint8_t reg_val = 0;
    uint8_t clear_zero = 0;
    uint8_t read_reg_val = 0;
    ret = sy5500_i2c_read(PointReg, &reg_val);
    if(false == ret){
        return -1;
    }
	
	val = val<<(bit_start - (bit_num - 1));
    for(; bit_num > 0; bit_num--){
        clear_zero = clear_zero | (1 << bit_start);
        bit_start--;
    }
    clear_zero = ~clear_zero;
    reg_val = reg_val & clear_zero;
    reg_val = reg_val | val;
    ret = sy5500_i2c_write(PointReg, &reg_val);
    if(false == ret){
        return -2;
    }
	ret = sy5500_i2c_read(PointReg, &read_reg_val);
    if((false == ret) || (reg_val != read_reg_val)){
        return -3;
    }    
    return 0;
}

static int8_t sy5500_set_reg_value(uint8_t PointReg, uint8_t value)
{
    uint8_t reg_val = value;
    int8_t ret = 0;
    ret = sy5500_i2c_write(PointReg, &reg_val);
    if(false == ret){
        return -1;
    }
    return ret;
}

static int8_t sy5500_get_reg_value(int8_t PointReg)
{
    uint8_t reg_val = 0;
    int8_t ret = 0;
    ret = sy5500_i2c_read(PointReg, &reg_val);
    if(false == ret){
        return -1;
    }
    return reg_val;
}

static uint8_t sy5500_get_ic_status(void)
{
    uint8_t reg_val = 0;
	reg_val = sy5500_get_reg_value(REG_STATE0);
	return (reg_val & 0x07);
}

static int8_t sy5500_set_ship_mode_delay_cutsys(uint8_t sec)
{
	int8_t ret = 0;
	if(1 == sec)
		ret = sy5500_set_reg_bit(REG_RST_DELAY_SET,0,1,SY5500_ENABLE);
	else if(0 == sec)
		ret = sy5500_set_reg_bit(REG_RST_DELAY_SET,0,1,SY5500_DISABLE);
	return ret;
}

static int8_t sy5500_set_charging_en(bool en)
{
	int8_t ret = 0;
	if(SY5500_ENABLE == en)
	{
		//ret = sy5500_set_reg_bit(REG_I2C_CMD,3,4,ENABLE_CG);
		ret = sy5500_set_reg_value(REG_I2C_CMD,ENABLE_CG);
	}
	else if(SY5500_DISABLE == en)
	{
		//ret = sy5500_set_reg_bit(REG_I2C_CMD,3,4,DISABLE_CG);
		ret = sy5500_set_reg_value(REG_I2C_CMD,DISABLE_CG);
	}
	return ret;
}


static int8_t sy5500_set_turnon_shipmode(bool en)
{
	int8_t ret = 0;
	if(SY5500_ENABLE == en)
	{
		sy5500_set_reg_bit(REG_I2C_CMD,3,4,EXIT_TRX_MODE);
		ret = sy5500_set_reg_bit(REG_I2C_CMD,3,4,TURNON_SHIPMODE);
	}
	return ret;
}

static int8_t sy5500_set_trickle_to_cc_vol(uint16_t volmv)
{
	int8_t ret = 0;
	if(2900 == volmv)
		ret = sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,0,1,SY5500_ENABLE);
	else if(2700 == volmv)
		ret = sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,0,1,SY5500_DISABLE);
	return ret;
}

static int8_t sy5500_set_charging_termination_curr(uint8_t percent)
{
	int8_t ret = 0;
	if(10 == percent)	//%10 cc cur
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,5,1,SY5500_DISABLE);
	else if(5 == percent)	//%5 cc cur
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,5,1,SY5500_ENABLE);
	return ret;
}

static int8_t sy5500_set_over_discharge_threshold_vol(uint16_t volmv)
{
	int8_t ret = 0;
	if(2800 == volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,4,1,SY5500_ENABLE);
	else if(2500== volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,4,1,SY5500_DISABLE);
	return ret;
}

static int8_t sy5500_set_charging_bat_vol(uint16_t volmv)
{
	int8_t ret = 0;
	if(4200 == volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,3,2,0);
	else if(4350 == volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,3,2,1);
	else if(4400 == volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,3,2,2);
	else if(4450 == volmv)
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,3,2,3);
	return ret;
}

static int8_t sy5500_set_charging_ntc_en(uint16_t fun)
{
	int8_t ret = 0;
	if(0 == fun)//jeita 标准,只针对充电
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,1,2,0);
	else if(1 == fun)//jeita 标准,充电和放电
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,1,2,1);
	else if(2 == fun)//输出温度信息，I2C 控制保护
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,1,2,2);
	else if(3 == fun)//屏蔽 NTC 保护，不输出温度信息
		ret = sy5500_set_reg_bit(REG_NTC_VIBATF_OD_SET,1,2,3);
	return ret;
}

static int8_t sy5500_set_constant_current_charging_curr(uint16_t percent)
{
	int8_t ret = 0;
	if(10 == percent)		//0.1c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,0);
	else if(20 == percent)	//0.2c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,1);
	else if(30 == percent)	//0.3c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,2);
	else if(50 == percent)	//0.5c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,3);
	else if(100 == percent)	//1c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,4);
	else if(150 == percent)	//1.5c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,5);
	else if(200 == percent)	//2c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,6);
	else if(300 == percent)	//3c charge
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,7,3,7);
	return ret;
}

static int8_t sy5500_set_recharge_voltage(uint16_t volmv)
{
	int8_t ret = 0;
	if(4100 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,0);
	else if(4000 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,1);
	else if(3900 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,2);
	else if(3800 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,3);
	else if(3700 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,4);
	else if(3550 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,5);
	else if(4300 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,6);
	else if(4200 == volmv)
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,3,3,7);
	return ret;
}

static int8_t sy5500_set_trickle_charge_current(uint8_t percent)
{
	int8_t ret = 0;
	if(10 == percent)	//%10 cc cur
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,0,1,0);
	else if(5 == percent)	//%5 cc cur
		ret = sy5500_set_reg_bit(REG_ITK_VRECG_ICC_SET,0,1,1);

	return ret;
}

static int8_t sy5500_set_constant_current_charging_timeout(uint16_t hour)
{
	int8_t ret = 0;
	if(3 == hour)
		ret = sy5500_set_reg_bit(REG_CHGTO_ODVER_PTVER,1,1,0);
	else if(6 == hour)
		ret = sy5500_set_reg_bit(REG_CHGTO_ODVER_PTVER,1,1,1);

	return ret;
}

static int8_t sy5500_set_charge_overtime_protection_en(bool en)
{
	int8_t ret = 0;
	if(SY5500_ENABLE == en)
		ret = sy5500_set_reg_bit(REG_CHGTO_ODVER_PTVER,0,1,1);
	else if(SY5500_DISABLE == en)
		ret = sy5500_set_reg_bit(REG_CHGTO_ODVER_PTVER,0,1,0);

	return ret;
}

#if defined(SY5500_PRINTF_REG_VALUE_DEBUG)
static void sy5500_printf_reg_value(void)
{
	SY5500_TRACE(2," STATE0-0x%02x:0x%02x", REG_STATE0, sy5500_get_reg_value(REG_STATE0));
	SY5500_TRACE(2," STATE1-0x%02x:0x%02x", REG_STATE1, sy5500_get_reg_value(REG_STATE1));
	SY5500_TRACE(2," STATE2-0x%02x:0x%02x", REG_STATE2, sy5500_get_reg_value(REG_STATE2));
	SY5500_TRACE(2," IO_CASECHK_SET-0x%02x:0x%02x", REG_IO_CASECHK_SET, sy5500_get_reg_value(REG_IO_CASECHK_SET));
	SY5500_TRACE(2," RST_DELAY_SET-0x%02x:0x%02x", REG_RST_DELAY_SET, sy5500_get_reg_value(REG_RST_DELAY_SET));
	SY5500_TRACE(2," PWK_SET-0x%02x:0x%02x", REG_PWK_SET, sy5500_get_reg_value(REG_PWK_SET));
	SY5500_TRACE(2," VINPULLDOWN_SET-0x%02x:0x%02x", REG_VINPULLDOWN_SET, sy5500_get_reg_value(REG_VINPULLDOWN_SET));
	SY5500_TRACE(2," VTK_IBATOC_TRX_SET-0x%02x:0x%02x", REG_VTK_IBATOC_TRX_SET, sy5500_get_reg_value(REG_VTK_IBATOC_TRX_SET));
	SY5500_TRACE(2," NTC_VIBATF_OD_SET-0x%02x:0x%02x", REG_NTC_VIBATF_OD_SET, sy5500_get_reg_value(REG_NTC_VIBATF_OD_SET));
	SY5500_TRACE(2," ITK_VRECG_ICC_SET-0x%02x:0x%02x", REG_ITK_VRECG_ICC_SET, sy5500_get_reg_value(REG_ITK_VRECG_ICC_SET));
	SY5500_TRACE(2," CHGTO_ODVER_PTVER-0x%02x:0x%02x", REG_CHGTO_ODVER_PTVER, sy5500_get_reg_value(REG_CHGTO_ODVER_PTVER));
}
#endif
/***************************SY5500 I2C REG OPERATING END***************************/

static void sy5500_event_process(void)
{
    dev_message_block_s msg;
    msg.mod_id = DEV_MODUAL_CHARGER;

    dev_thread_mailbox_put(&msg);
}

static int sy5500_handle_process(dev_message_body_s *msg_body)
{
	SY5500_TRACE(3,"%s reg0x%02x-b0~b2:0x%02x",__func__,REG_STATE0,sy5500_get_ic_status());

	return 0;
}

static void sy5500_int_handler(enum HAL_GPIO_PIN_T pin)
{
	SY5500_TRACE(1, "%s",__func__);
	sy5500_event_process();

}

static void sy5500_int_init(void)
{
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;

#ifdef SY5500_IRQ_STATUS_SUPPPORT
    gpiocfg.irq_enable = true;
#else
    gpiocfg.irq_enable = false;
#endif
    gpiocfg.irq_debounce = true;
    gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;

    gpiocfg.irq_handler = sy5500_int_handler;
    gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;

    if (app_charger_int_cfg.pin != HAL_IOMUX_PIN_NUM) {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_charger_int_cfg, 1);
        hal_gpio_pin_set_dir(app_charger_int_cfg.pin, HAL_GPIO_DIR_IN, 1);
        hal_gpio_setup_irq(app_charger_int_cfg.pin, &gpiocfg);
    }
}

//####################################################################
#ifdef POLLING_STATUS_REPORT
static void sy5500_timehandler(void const *param);
osTimerDef(APP_SY5500_TIMER, sy5500_timehandler);
static osTimerId app_sy5500_timer = NULL;

static void sy5500_timehandler(void const *param)
{
	uint8_t st_chip = sy5500_get_ic_status();
	if(ST_CHIP_CHARGEEND_MODE == st_chip)
	{
		if(1 != charge_ic_full_charge_flag)
		{
			charge_ic_full_charge_flag = 1;
			SY5500_TRACE(2,"%s charge_ic_full_charge_flag:%d",__func__,charge_ic_full_charge_flag);
		}
	}
	else if(ST_CHIP_STANDBY_MODE == st_chip)
	{
		if(2 != charge_ic_full_charge_flag)
		{
			charge_ic_full_charge_flag = 2;
			SY5500_TRACE(2,"%s charge_ic_full_charge_flag:%d",__func__,charge_ic_full_charge_flag);
		}
	}
	else
	{
		if(2 == charge_ic_full_charge_flag)
		{
			charge_ic_full_charge_flag = 0;
			SY5500_TRACE(2,"%s charge_ic_full_charge_flag:%d",__func__,charge_ic_full_charge_flag);
		}
	}
	SY5500_TRACE(3,"%s reg0x%02x-b0~b2:0x%02x",__func__,REG_STATE0,st_chip);
	//SY5500_TRACE(3,"%s reg0x%02x:0x%02x",__func__,REG_NTC_VIBATF_OD_SET,sy5500_get_reg_value(REG_NTC_VIBATF_OD_SET));
}
#endif
//#####################################################################

int32_t sy5500_init(void)
{
	SY5500_TRACE(1,"%s in" ,__FUNCTION__);
	
	dev_thread_handle_set(DEV_MODUAL_CHARGER, sy5500_handle_process);
	sy5500_set_reg_value(REG_WPEN_72,0x97);	//密码位寄存器
	sy5500_set_reg_value(REG_WPEN_73,0x39);	//密码位寄存器 当写入 0x39 时类型 3 的寄存器可写，写入其他值时类型 3 的寄存器只读
	sy5500_set_reg_bit(REG_IO_CASECHK_SET,4,1,SY5500_ENABLE); //ENABLE IO 端口（PWK/RST/TRX）功能
	sy5500_set_reg_bit(REG_IO_CASECHK_SET,3,1,SY5500_ENABLE); //RST 作用电平：1：高电平。PMOS 开漏输出，PMOS 上拉到 VBAT。
	sy5500_set_reg_bit(REG_RST_DELAY_SET,5,2,3); //rst 时间：00b：50ms 01b：100ms 10b：200ms 11b：500ms
	sy5500_set_trickle_to_cc_vol(2900);	//2900mv
	sy5500_set_charging_termination_curr(10); //%10 cc cur
	sy5500_set_trickle_charge_current(10);	//%10 cc cur
	sy5500_set_recharge_voltage(4200);	//4200mv
	sy5500_set_over_discharge_threshold_vol(2800);	//2800mv
	sy5500_set_constant_current_charging_timeout(3);	//3 hour
	sy5500_set_charging_ntc_en(3);	//屏蔽 NTC 保护，不输出温度信息
	sy5500_set_charge_overtime_protection_en(SY5500_ENABLE);
	sy5500_set_charging_current(HAL_CHARGING_CTRL_3C);
#if defined(__XSPACE_UI__)
	sy5500_set_charging_voltage(4200); //bat full vol
#else
	sy5500_set_charging_voltage(4400); //bat full vol
#endif	
	sy5500_set_charging_status(SY5500_ENABLE);
#if 0//def 	POLLING_STATUS_REPORT
	if (app_sy5500_timer == NULL)
	{
	   app_sy5500_timer = osTimerCreate (osTimer(APP_SY5500_TIMER), osTimerPeriodic, NULL);
	}
	osTimerStart(app_sy5500_timer, 10*1000);
#endif

	sy5500_int_init();
#if defined(SY5500_PRINTF_REG_VALUE_DEBUG)
	sy5500_printf_reg_value();
#endif
    sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,7,1,1);
    sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,3,2,0);

	SY5500_TRACE(1,"%s out" ,__FUNCTION__);
	return 0;
}

static int32_t sy5500_get_chip_id(uint32_t *chip_id)
{
	//*chip_id = sy5500_get_version();
	*chip_id = 0x5500;
	
	return 0;
}

static int32_t sy5500_get_charging_current(hal_charging_current_e *status)
{
	*status = (hal_charging_current_e)u16ChargingCurrent;
	return 0;
}

static int32_t sy5500_set_charging_current(hal_charging_current_e current)
{
	int8_t ret = 0;
	SY5500_TRACE(2, "%s current(%d)\n",__func__, current);
	if(current >= HAL_CHARGING_CTRL_TOTAL_NUM)
	{
		SY5500_TRACE(0, "Invalid current set\n");
		return -1;
	}

	if(HAL_CHARGING_CTRL_ONE_FIFTH == current)
	{
		ret = sy5500_set_constant_current_charging_curr(20);
	}
	else if(HAL_CHARGING_CTRL_HALF == current)
	{
		ret = sy5500_set_constant_current_charging_curr(50);
	}
	else if(HAL_CHARGING_CTRL_1C == current)
	{
		ret = sy5500_set_constant_current_charging_curr(100);
	}
	else if(HAL_CHARGING_CTRL_2C == current)
	{
		ret = sy5500_set_constant_current_charging_curr(200);
	}
	else if(HAL_CHARGING_CTRL_3C == current)
	{
		ret = sy5500_set_constant_current_charging_curr(300);
	}
	else
	{
		SY5500_TRACE(1, "%s Invalid\n", __func__);
		return -1;
	}
	if(!ret)
		u16ChargingCurrent = current;
	return 0;

}

static int32_t sy5500_get_charging_status(hal_charging_ctrl_e *status)
{
	uint8_t u8IcStatus;
	u8IcStatus = sy5500_get_reg_value(REG_STATE0) & 0x07;
	if(u8IcStatus >= 0x04)
	{
		*status = HAL_CHARGING_CTRL_ENABLE;
		return 0;
	}
	else
	{
		*status = HAL_CHARGING_CTRL_DISABLE;
		return 0;
	}
	return -1;
}

static int32_t sy5500_set_charging_status(hal_charging_ctrl_e status)
{
	SY5500_TRACE(2, "%s status(%d)\n", __func__, status);
	if(status == HAL_CHARGING_CTRL_ENABLE)
	{
		sy5500_set_charging_en(SY5500_ENABLE);
	}
	else if(status == HAL_CHARGING_CTRL_DISABLE)
	{
		sy5500_set_charging_en(SY5500_DISABLE);
	}
	return 0;
}

static int32_t sy5500_get_charging_voltage(uint16_t *voltage)
{
	*voltage = u16ChargingVoltage;
	return 0;
}

static int32_t sy5500_set_charging_voltage(uint16_t voltage)
{
	int8_t ret = 0;
    SY5500_TRACE(2, "%s voltage(%d)\n", __func__, voltage);
	ret = sy5500_set_charging_bat_vol(voltage);
	if(!ret)
		u16ChargingVoltage = voltage;
	return 0;
}

static int32_t sy5500_set_chg_watchdog(bool status)
{
    SY5500_TRACE(2, "%s status(%d)\n", __func__, status);
    SY5500_TRACE(1,	"%s:watchdog not support\n",__func__);
    return 0;
}

static int32_t sy5500_trx_mode_set(hal_charger_trx_mode_e mode)
{
    SY5500_TRACE(2, " %s trx mode %d", __func__, mode);
    if (mode > HAL_CHARGER_TRX_MODE_NUM) {
        SY5500_TRACE(0, "mode err, return.");
        return -1;
    }
    switch(mode) {
        case HAL_CHARGER_TRX_MODE_OD:
            sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,7,1,1);
            break;
        case HAL_CHARGER_TRX_MODE_PP:
            sy5500_set_reg_bit(REG_VTK_IBATOC_TRX_SET,7,1,0);        
            break;
        default:
            break;
    }
    return 0;
}
static int32_t sy5500_cutOff_Vsys()
{
    SY5500_TRACE(1, "%s \n", __func__);
	osDelay(2*1000);
	sy5500_set_ship_mode_delay_cutsys(1);
	sy5500_set_turnon_shipmode(SY5500_ENABLE);
    return 0;
}

static int32_t sy5500_charger_polling_status(bool en)
{
    SY5500_TRACE(1, "%s en:%d\n", __func__,en);
	
#ifdef 	POLLING_STATUS_REPORT
	if(en)
	{
		if(app_sy5500_timer == NULL)
		{
		   app_sy5500_timer = osTimerCreate (osTimer(APP_SY5500_TIMER), osTimerPeriodic, NULL);
		}
		osTimerStop(app_sy5500_timer);
		osTimerStart(app_sy5500_timer, 10*1000);
	}
	else
	{
		osTimerStop(app_sy5500_timer);
		charge_ic_full_charge_flag = 2;
		SY5500_TRACE(2,"%s charge_ic_full_charge_flag:%d",__func__,charge_ic_full_charge_flag);
	}
#endif	
	return 0;
}

const hal_charger_ctrl_s sy5500_ctrl =
{
    sy5500_init,
    sy5500_get_chip_id,
    sy5500_get_charging_current,
    sy5500_set_charging_current,
    sy5500_get_charging_status,
    sy5500_set_charging_status,
    NULL,
    sy5500_get_charging_voltage,
    sy5500_set_charging_voltage,
    sy5500_cutOff_Vsys,
    NULL,
    sy5500_set_chg_watchdog,
    NULL,
    NULL,
    sy5500_trx_mode_set,
    sy5500_charger_polling_status,
};

#endif /* __CHARGER_SY5500__ */
