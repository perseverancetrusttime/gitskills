#if defined(__CHARGER_SIMULATION__)
#include "charger_simu_adapter.h"
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

#define CHARGER_SIMU_TRACE(num, str, ...)  TRACE(num, "[CHARGER_SIMU]" str, ##__VA_ARGS__)

static int32_t charger_simu_set_charging_voltage(uint16_t voltage);
static int32_t charger_simu_set_charging_current(hal_charging_current_e current);
static int32_t charger_simu_set_charging_status(hal_charging_ctrl_e status);
static int32_t charger_simu_set_box_dummy_load_switch(uint8_t status);

static uint16_t u16ChargingCurrent = 0;
static uint16_t u16ChargingVoltage = 0;
extern uint8_t charge_ic_full_charge_flag;
static hal_charging_ctrl_e charge_status = HAL_CHARGING_CTRL_DISABLE;

void charger_simu_int_init(void)
{
	CHARGER_SIMU_TRACE(1, "%s.", __func__);
}

static int32_t charger_simu_init(void)
{
	CHARGER_SIMU_TRACE(1, "%s.", __func__);

	return 0;
}

static int32_t charger_simu_get_chip_id(uint32_t *chip_id)
{

	return 0;
}

static int32_t charger_simu_get_charging_current(hal_charging_current_e *status)
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
static int32_t charger_simu_set_charging_current(hal_charging_current_e current)
{
	uint32_t u32FastCurrent;
	//uint32_t u32TccCurrent;
	//int retVal;

	CHARGER_SIMU_TRACE(2, "%s current(%d)\n",__func__, current);
	if(current >= HAL_CHARGING_CTRL_TOTAL_NUM)
	{
		CHARGER_SIMU_TRACE(0, "Invalid current set\n");
		return -1;
	}
	if(HAL_CHARGING_CTRL_HALF == current)
	{
		u32FastCurrent = 15; //0.45c
		//u32TccCurrent = 200;
	}
	else if(HAL_CHARGING_CTRL_1C == current)
	{
		u32FastCurrent = 30;
		//u32TccCurrent = 100;
	}
	else if(HAL_CHARGING_CTRL_2C == current)
	{
		u32FastCurrent = 70; //2c
		//u32TccCurrent = 50;
	}
	else
	{
		CHARGER_SIMU_TRACE(1, "%s Invalid\n", __func__);
		return -1;
	}

	u16ChargingCurrent = u32FastCurrent;

	return 0;
}

static int32_t charger_simu_get_charging_status(hal_charging_ctrl_e *status)
{
	*status = charge_status;

	return 0;
}

static int32_t charger_simu_set_charging_status(hal_charging_ctrl_e status)
{
	CHARGER_SIMU_TRACE(2, "%s status(%d)\n", __func__, status);

	if((HAL_CHARGING_CTRL_ENABLE != status)&(HAL_CHARGING_CTRL_DISABLE != status))
		return -1;

    charge_status = status;

    return 0;
}

static int32_t charger_simu_get_charging_voltage(uint16_t *voltage)
{
	*voltage = u16ChargingVoltage;
	return 0;
}
/*
Charging Finish Voltage
Range: 3.85V-4.6V
*/
static int32_t charger_simu_set_charging_voltage(uint16_t voltage)
{
	CHARGER_SIMU_TRACE(2, "%s voltage(%d)\n",__func__, voltage);
	u16ChargingVoltage = voltage;

	return 0;
}

static int32_t charger_simu_set_chg_watchdog(bool status)
{
    return 0;
}

int32_t charger_simu_cutOff_Vsys()
{
    CHARGER_SIMU_TRACE(1, "%s\n", __func__);
    return 0;
}

static int32_t charger_simu_set_HiZ_status(hal_charging_ctrl_e status)
{
    CHARGER_SIMU_TRACE(2, "%s status(%d)\n", __func__, status);

    return 0;
}

static int32_t charger_simu_set_box_dummy_load_switch(uint8_t status)
{
    return 0;
}

const hal_charger_ctrl_s charger_simu_ctrl =
{
    charger_simu_init,
    charger_simu_get_chip_id,
    charger_simu_get_charging_current,
    charger_simu_set_charging_current,
    charger_simu_get_charging_status,
    charger_simu_set_charging_status,
    NULL,
    charger_simu_get_charging_voltage,
    charger_simu_set_charging_voltage,
    charger_simu_cutOff_Vsys,
    charger_simu_set_HiZ_status,
    charger_simu_set_chg_watchdog,
    charger_simu_set_box_dummy_load_switch,
};

#endif /* __CHARGER_charger_simu__ */
