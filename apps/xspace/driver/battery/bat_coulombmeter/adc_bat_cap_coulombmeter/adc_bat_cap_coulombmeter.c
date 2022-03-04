/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
 
#if defined(__ADC_BAT_CAP_COULOMBMETER__)
#include "cmsis_os.h"
#include "cmsis_nvic.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "app_thread.h"
#include "app_utils.h"
#include "hal_charger.h"
#include "drv_bat_adc.h"
#include "adc_bat_cap_coulombmeter.h"
#include "xspace_flash_access.h"
#include "xspace_ram_access.h"
#include "xspace_interface.h"

#define ADC_BAT_CAP_COULOMBMETER_DEBUG         1

#if (ADC_BAT_CAP_COULOMBMETER_DEBUG)
#define ADC_BAT_CAP_COULOMBMETER_TRACE(num, str, ...)  TRACE(num, "[BAT_ALGO] "str, ##__VA_ARGS__)
#else
#define ADC_BAT_CAP_COULOMBMETER_TRACE(num, str, ...)
#endif

#define ADC_BAT_STABLE_COUNT                    (10)
#define VOLTAGE_FIRST_ORDER_COEFFICIENT         (0.9)
#define ADC_BAT_CHARGING_FULL_VOLA              4340

typedef struct
{
    uint8_t     InitOnFlag;
    uint32_t    constant_voltage_charge_time;//
    uint32_t    simulate_dump_energy;
    uint32_t    quitStandbyTimerOutCnt;
    
    uint8_t     isChargeFullFlag;
    uint8_t     guessThePowerStaus;
    uint8_t     guessPowerTimeCnt;
} calc_energy_changing_struct;

typedef enum {
    BATTERY_STATUS_INIT,        // init status
    BATTERY_STATUS_NORMAL,      // normal used status
    BATTERY_STATUS_CHARGING,    // charging statu
    BATTERY_STATUS_INVALID,     // invalid statu

    BATTERY_STATUS_QTY
}BATTERY_STATUS_ENUM;


typedef struct 
{
    BATTERY_STATUS_ENUM 	status;
    uint8_t					currlevel;
    uint8_t					currPercentage;
    uint16_t				currvolt;
    uint16_t 				index;
    uint16_t 				logic_index[ADC_BAT_STABLE_COUNT];
    uint16_t				voltage[ADC_BAT_STABLE_COUNT];
    uint32_t				curr_tim[ADC_BAT_STABLE_COUNT];
    float					first_order_vol[ADC_BAT_STABLE_COUNT];
    bat_adc_status_e		charge_status[ADC_BAT_STABLE_COUNT];
    hal_charging_current_e	charge_current[ADC_BAT_STABLE_COUNT];
}BATTERY_INFO_T;

const uint16_t bat_discharge_vol_tab[11] = 
{
	3300,3661,3717,3757,3800,3855,3933,4038,4144,4250,//1-100
	4360
};

const uint16_t bat_changing_vol_tab[11] =
{	
	3480,3736,3800,3836,3869,3920,4009,4105,4214,4338,//1-100
	4400
};

uint8_t charge_ic_full_charge_flag = 2;
static calc_energy_changing_struct calc_energy_changing_str = {0};
static BATTERY_INFO_T battery_measure = {0};
static uint32_t bat_calculation_per99_ticks = 0;

static uint8_t bat_algorithm_calculation_capacity_limit(bat_adc_status_e charge_status,uint16_t valt_val);
static uint8_t bat_algorithm_charge_calculation(float valt_val,float old_valt_val,hal_charging_current_e calc_cuur);

uint16_t bat_algorithm_poweroff_volt_get(bat_adc_status_e bat_status)
{
    if(BAT_ADC_STATUS_CHARGING == bat_status) {
        return bat_changing_vol_tab[0]; /*element 0 is power off voltage*/
    } else {
        return bat_discharge_vol_tab[0];
    }
}

void bat_algorithm_battery_write_flash(void) 
{
    ADC_BAT_CAP_COULOMBMETER_TRACE(2, "currPercentage=%d,simulate_dump_energy=%d",battery_measure.currPercentage, calc_energy_changing_str.simulate_dump_energy);
    batterry_data_write_flash((uint32_t)battery_measure.currPercentage, calc_energy_changing_str.simulate_dump_energy);
}

static void bat_algorithm_powerup_calculation_capacity(bat_adc_status_e bat_status)
{
    uint8_t percentage = (uint8_t)(bat_algorithm_calculation_capacity_limit(bat_status,battery_measure.currvolt));
    if(battery_measure.currvolt < 3300)
    {
		percentage = 0;
	}
	else if(battery_measure.currvolt < 3600){
        percentage = 1;
    }
    ADC_BAT_CAP_COULOMBMETER_TRACE(1, "bat_algorithm_powerup_calculation_capacity=%d",percentage);
    if(percentage > 98)
    {
        percentage = 98;
    }
    battery_measure.currPercentage = percentage;//
    calc_energy_changing_str.simulate_dump_energy = percentage*10000;
    calc_energy_changing_str.guessThePowerStaus = 0; 
}

static BATTERY_STATUS_ENUM bat_algorithm_get_charge_mode(void)
{
    if(battery_measure.charge_status[battery_measure.logic_index[ADC_BAT_STABLE_COUNT-1]] == BAT_ADC_STATUS_CHARGING)
    {
        return BATTERY_STATUS_CHARGING;
    }
    else
    {
        for(uint8_t i=0;i<ADC_BAT_STABLE_COUNT;i++)
        {
            if(battery_measure.charge_status[i] == BAT_ADC_STATUS_CHARGING)
            {
                return BATTERY_STATUS_CHARGING;
            }
        }
     
        return BATTERY_STATUS_NORMAL;
    }

    return BATTERY_STATUS_CHARGING;
}

static void bat_algorithm_get_battery_calculate_voltage(bat_adc_status_e charge_status,bat_adc_voltage battery_adc_data)
{
    uint8_t i;
    uint16_t temp_index;
    hal_charging_current_e temp_charge_current;
    static uint8_t input_parameter_init_flag=0;
    
    ADC_BAT_CAP_COULOMBMETER_TRACE(2, "input,charge_status=%d,bat_vol=%d;",charge_status,battery_adc_data);
    if(input_parameter_init_flag == 0)
    {
        input_parameter_init_flag = 1;
        memset(&battery_measure,0,sizeof(battery_measure));
        battery_measure.index = 0;
    }
    if(battery_adc_data > 0)
    {
        if (battery_measure.index < ADC_BAT_STABLE_COUNT)
        {
            for (i=0;i<ADC_BAT_STABLE_COUNT;i++)
            {
                battery_measure.charge_status[battery_measure.index%ADC_BAT_STABLE_COUNT] = charge_status;
                battery_measure.curr_tim[battery_measure.index%ADC_BAT_STABLE_COUNT] = hal_sys_timer_get();
                battery_measure.voltage[battery_measure.index%ADC_BAT_STABLE_COUNT] = battery_adc_data;
                battery_measure.first_order_vol[battery_measure.index%ADC_BAT_STABLE_COUNT] = battery_adc_data;
                if(hal_charger_get_charging_current(&temp_charge_current) == 0)
                {
                    battery_measure.charge_current[battery_measure.index%ADC_BAT_STABLE_COUNT] = temp_charge_current;
                }
                battery_measure.logic_index[i] = i;
                battery_measure.index++;
            }
            battery_measure.status = BATTERY_STATUS_INIT;
            
            xspace_interface_register_write_flash_befor_shutdown_cb(WRITE_BATTRY_TO_FLASH, bat_algorithm_battery_write_flash);
            ADC_BAT_CAP_COULOMBMETER_TRACE(0, "init;");
        }
        else
        {
            battery_measure.charge_status[battery_measure.index%ADC_BAT_STABLE_COUNT] = charge_status;
            battery_measure.curr_tim[battery_measure.index%ADC_BAT_STABLE_COUNT] = hal_sys_timer_get();
            battery_measure.voltage[battery_measure.index%ADC_BAT_STABLE_COUNT] = battery_adc_data;
            battery_measure.first_order_vol[battery_measure.index%ADC_BAT_STABLE_COUNT] = (battery_adc_data*(1-VOLTAGE_FIRST_ORDER_COEFFICIENT)
                                                                                              +(battery_measure.voltage[(battery_measure.index-1)%ADC_BAT_STABLE_COUNT]*VOLTAGE_FIRST_ORDER_COEFFICIENT));
            if(hal_charger_get_charging_current(&temp_charge_current) == 0)
            {
                battery_measure.charge_current[battery_measure.index%ADC_BAT_STABLE_COUNT] = temp_charge_current;
            }
            for (i=0;i<ADC_BAT_STABLE_COUNT;i++)
            {
                battery_measure.logic_index[ADC_BAT_STABLE_COUNT-1-i] = (battery_measure.index-i)%ADC_BAT_STABLE_COUNT;
            }
            battery_measure.index++;
        }
    }

    temp_index = (battery_measure.index-1)%ADC_BAT_STABLE_COUNT;
    battery_measure.currvolt = (bat_adc_voltage)battery_measure.first_order_vol[temp_index];
    ADC_BAT_CAP_COULOMBMETER_TRACE(3, "curr_tim=%d,first_order_vol=%d,charge_status=%d;", 
                        battery_measure.curr_tim[temp_index],
                        (uint32_t)battery_measure.first_order_vol[temp_index], 
                        battery_measure.charge_status[temp_index]);
}

static uint8_t bat_algorithm_calculation_capacity_limit(bat_adc_status_e charge_status,uint16_t valt_val)
{
    const uint16_t *p;
    uint16_t delta_vol;
    unsigned char tmp_start,tmp_stop,tmp_mid;
    unsigned char tmp_result = 0;
    uint8_t i;

    if(charge_status == BAT_ADC_STATUS_CHARGING)
    {
        p = bat_changing_vol_tab;
    }
    else
    {
        p = bat_discharge_vol_tab;
    }

    if(valt_val >= *(p + 10))
    {
        return 100;
    }
    else if(valt_val <= *p) 
    {
        return 0;
    }

    tmp_start = 0;
    tmp_stop = 11;

    for(i = 0; i < 4; i++)
    {
        tmp_mid = ((tmp_stop + tmp_start) / 2);
        if(valt_val >= *(p+tmp_mid))
        {
            tmp_start = tmp_mid;
        }
        else
        { 
            tmp_stop = tmp_mid;
        }

        if((tmp_stop - tmp_start) == 1)
        {
            tmp_result = tmp_start;
            break;
        }
    }
    delta_vol = (*(p+tmp_result+1) - *(p+tmp_result))/10;
    for(i = 0; i < 10; i++)
    {
        if((valt_val >= (*(p+tmp_result)+(delta_vol*i)))&&(valt_val < (*(p+tmp_result)+(delta_vol*(i+1)))))
        {
            tmp_start = tmp_mid;
            return (tmp_result*10 + i);
        }
    }
    
    return (tmp_result*10 + i);
}

uint8_t bat_algorithm_charge_calculation(float valt_val,float old_valt_val,hal_charging_current_e calc_cuur)
{
    float dump_sloop;
    float delt_vol;
    delt_vol = valt_val-old_valt_val;
    ADC_BAT_CAP_COULOMBMETER_TRACE(2, "in valt_val=%d,simulate_dump=%d;",(uint32_t)(valt_val),calc_energy_changing_str.simulate_dump_energy);
    ADC_BAT_CAP_COULOMBMETER_TRACE(2, "in calc_cuur=%d,delt_vol=%d;",calc_cuur,(uint32_t)(delt_vol*100));

    if(valt_val < ADC_BAT_CHARGING_FULL_VOLA) 
    {
        if((calc_energy_changing_str.simulate_dump_energy <= (bat_algorithm_calculation_capacity_limit(BAT_ADC_STATUS_CHARGING,(uint16_t)valt_val)*10000))
            &&((delt_vol > 0.5)||(calc_cuur == HAL_CHARGING_CTRL_HALF)))
        {
            switch(calc_cuur)
            {
                case HAL_CHARGING_CTRL_UNCHARGING:
                    break;
                
                case HAL_CHARGING_CTRL_HALF:
                    calc_energy_changing_str.simulate_dump_energy += (90*10000/(150*60/10));//
                    break;
                
                case HAL_CHARGING_CTRL_1C:
                    calc_energy_changing_str.simulate_dump_energy += (60*10000/(60*60/10));//
                    break;
                
                case HAL_CHARGING_CTRL_2C:
                    calc_energy_changing_str.simulate_dump_energy += (70*10000/(20*60/10));//
                    break;
				
                case HAL_CHARGING_CTRL_3C:
                    calc_energy_changing_str.simulate_dump_energy += (70*10000/(16*60/10));//
                    break;
                default:
                    break;
            }
        }

        ADC_BAT_CAP_COULOMBMETER_TRACE(0, "add");
    }
    else 
    {
        if((calc_cuur == HAL_CHARGING_CTRL_2C) && calc_energy_changing_str.simulate_dump_energy <= (70*10000))
    	{
            calc_energy_changing_str.simulate_dump_energy += (70*10000/(20*60/10));//
    	}
		else if((calc_cuur == HAL_CHARGING_CTRL_3C) && calc_energy_changing_str.simulate_dump_energy <= (70*10000))
    	{
            calc_energy_changing_str.simulate_dump_energy += (70*10000/(16*60/10));//
    	}
		else if((calc_cuur == HAL_CHARGING_CTRL_1C) && calc_energy_changing_str.simulate_dump_energy <= (60*10000))
		{
			calc_energy_changing_str.simulate_dump_energy += (60*10000/(60*60/10));//
		}
		else if((calc_cuur == HAL_CHARGING_CTRL_HALF) && calc_energy_changing_str.simulate_dump_energy <= (90*10000))
		{
			calc_energy_changing_str.simulate_dump_energy += (90*10000/(150*60/10));//
		}
        else
        {
            dump_sloop = (float)(1000000-calc_energy_changing_str.simulate_dump_energy)/200000;
            if(dump_sloop > 1.0) dump_sloop = 1.0;
            switch(calc_cuur)
            {
                case HAL_CHARGING_CTRL_UNCHARGING:
                    break;
                
                case HAL_CHARGING_CTRL_HALF:
                    calc_energy_changing_str.simulate_dump_energy += (10*10000/(30*60/10));//40%,80min,60s,10s
                    break;
                
                case HAL_CHARGING_CTRL_1C:
                    if(dump_sloop < 0.4) dump_sloop = 0.4;
                    calc_energy_changing_str.simulate_dump_energy += (uint32_t)((40*10000/(40*60/10))*dump_sloop);//
                    break;
                
                case HAL_CHARGING_CTRL_2C:
                    if(dump_sloop < 0.25) dump_sloop = 0.25;
                    calc_energy_changing_str.simulate_dump_energy += (uint32_t)((30*10000/(20*60/10))*dump_sloop);//30%,20min,60s,10s
                    break;
					
                case HAL_CHARGING_CTRL_3C:
                    if(dump_sloop < 0.20) dump_sloop = 0.20;
                    calc_energy_changing_str.simulate_dump_energy += (uint32_t)((30*10000/(25*60/10))*dump_sloop);//30%,20min,60s,10s
                    break;
                default:
                break;
            }
        }
    }

	if(calc_energy_changing_str.simulate_dump_energy >= 99*10000)
	{
		hal_charger_set_box_dummy_load_switch(0x01); //open load
		if(bat_calculation_per99_ticks == 0)
			bat_calculation_per99_ticks = hal_sys_timer_get();
	}
	
	
	if((1 == charge_ic_full_charge_flag) || (bat_calculation_per99_ticks && TICKS_TO_MS(hal_sys_timer_get() - bat_calculation_per99_ticks) >= 20*60*1000))
	{
		/***
		ADC_BAT_CAP_COULOMBMETER_TRACE(3, "bat_calculation_per99_ticks sys_ms:%d per99_ms:%d min:%d\r\n", 
		TICKS_TO_MS(bat_calculation_per99_ticks),TICKS_TO_MS(hal_sys_timer_get() - bat_calculation_per99_ticks),
		TICKS_TO_MS(hal_sys_timer_get() - bat_calculation_per99_ticks)/1000/60);
		***/
		//ADC_BAT_CAP_COULOMBMETER_TRACE(1, "charge_ic_full_charge_flag %d\r\n", charge_ic_full_charge_flag);
		if(calc_energy_changing_str.simulate_dump_energy < 100*10000)
		{
			calc_energy_changing_str.simulate_dump_energy += 5000;
		}
	}
	else if(0 == charge_ic_full_charge_flag)
	{
		//ADC_BAT_CAP_COULOMBMETER_TRACE(1, "charge_ic_full_charge_flag %d\r\n", charge_ic_full_charge_flag);
		if(calc_energy_changing_str.simulate_dump_energy >= 100*10000)
			calc_energy_changing_str.simulate_dump_energy = (100*10000) - 200;
	}

    ADC_BAT_CAP_COULOMBMETER_TRACE(1, "out %d\r\n", calc_energy_changing_str.simulate_dump_energy);
    if(calc_energy_changing_str.simulate_dump_energy>= 1000000)
    {
        calc_energy_changing_str.simulate_dump_energy = 1000000;
    }

    return (calc_energy_changing_str.simulate_dump_energy/10000);
}

uint8_t bat_capacity_conversion_interface(bat_adc_status_e bat_status,bat_adc_voltage adc_data)
{
    int8_t temp_percentage = 0;
    uint32_t temp_batt_percent,temp_batt_energy;
    static uint32_t old_bat_percent;
    
    bat_algorithm_get_battery_calculate_voltage(bat_status,adc_data);
    if(battery_measure.status == BATTERY_STATUS_INIT) 
    {
        bat_algorithm_powerup_calculation_capacity(bat_status);
        
        if(xra_get_earphone_battery_data_from_ram(&temp_batt_percent,&temp_batt_energy) == 0)
        {
    	   if(bat_status == BAT_ADC_STATUS_DISCHARGING)
		   {
			   if(battery_measure.currPercentage > temp_batt_percent)
			   {
				   if((battery_measure.currPercentage - temp_batt_percent)<60)
				   {
						battery_measure.currPercentage = temp_batt_percent;
						calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
				   }
			   }
			   else if(battery_measure.currPercentage < temp_batt_percent)
			   {
				   if((temp_batt_percent - battery_measure.currPercentage)<15)
				   {
						battery_measure.currPercentage = temp_batt_percent;
						calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
				   }
			   }
    	   	}
			else if(bat_status == BAT_ADC_STATUS_CHARGING)
			{
				battery_measure.currPercentage = temp_batt_percent;
				calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
			}
			 
            ADC_BAT_CAP_COULOMBMETER_TRACE(2, "init from ram currPer:%d,dump_energy:%d", battery_measure.currPercentage,calc_energy_changing_str.simulate_dump_energy);
        }
        else
        {
			 uint8_t hw_rst_flag = 0xff;
			 if(hw_rst_use_bat_data_flag_read_flash(&hw_rst_flag) != 0)
			 	hw_rst_flag = 0xff;
			 if(true == hw_rst_flag)
			 	hw_rst_use_bat_data_flag_write_flash(false);
             if(batterry_data_read_flash(&temp_batt_percent,&temp_batt_energy) == 0)
             {
                if(battery_measure.currPercentage > temp_batt_percent)
                {
                    if(((battery_measure.currPercentage - temp_batt_percent)<40) || (true == hw_rst_flag))
                    {
                         battery_measure.currPercentage = temp_batt_percent;
                         calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
                    }
                }
                else if(battery_measure.currPercentage < temp_batt_percent)
                {
                    if(((temp_batt_percent - battery_measure.currPercentage)<40) || (true == hw_rst_flag))
                    {
                         battery_measure.currPercentage = temp_batt_percent;
                         calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
                    }
                }
                else
                {
                     battery_measure.currPercentage = temp_batt_percent;
                     calc_energy_changing_str.simulate_dump_energy = temp_batt_energy;
                }
             }
        }
		
		xra_write_earphone_battery_data_to_ram((uint32_t)battery_measure.currPercentage,(uint32_t)calc_energy_changing_str.simulate_dump_energy);
        battery_measure.status = BATTERY_STATUS_CHARGING;
        return battery_measure.currPercentage;
    }
    
	battery_measure.status = bat_algorithm_get_charge_mode();
	
    if (battery_measure.status == BATTERY_STATUS_CHARGING)
    {
        temp_percentage = bat_algorithm_charge_calculation(battery_measure.first_order_vol[battery_measure.logic_index[ADC_BAT_STABLE_COUNT-1]],
                                                    battery_measure.first_order_vol[battery_measure.logic_index[ADC_BAT_STABLE_COUNT-2]],
                                                    battery_measure.charge_current[battery_measure.logic_index[ADC_BAT_STABLE_COUNT-1]]);    //Percentage

        if(battery_measure.currvolt < ADC_BAT_CHARGING_FULL_VOLA-30)
        {
            calc_energy_changing_str.isChargeFullFlag = 0;
        }

        if(battery_measure.currPercentage < temp_percentage)
        {
            battery_measure.currPercentage++;
        }
        else if(battery_measure.currPercentage == 0)
        {
            battery_measure.currPercentage = temp_percentage;
        }
	}
	else if (battery_measure.status == BATTERY_STATUS_NORMAL)
    {
        calc_energy_changing_str.isChargeFullFlag = 0;
		bat_calculation_per99_ticks = 0;
        temp_percentage = bat_algorithm_calculation_capacity_limit(BAT_ADC_STATUS_DISCHARGING,(uint16_t)battery_measure.first_order_vol[battery_measure.logic_index[ADC_BAT_STABLE_COUNT-1]]);    //Percentage
        if((temp_percentage < battery_measure.currPercentage)&&(battery_measure.currPercentage>0))
        {
            battery_measure.currPercentage--;
        }
        else if(battery_measure.currPercentage == 0)
        {
            battery_measure.currPercentage = temp_percentage;
        }
        calc_energy_changing_str.simulate_dump_energy = battery_measure.currPercentage*10000;
           
    }
    if(battery_measure.currPercentage >100)
    {
        battery_measure.currPercentage = 100;
    }
    
    if(old_bat_percent != battery_measure.currPercentage)
    {
		old_bat_percent = battery_measure.currPercentage;
    	xra_write_earphone_battery_data_to_ram((uint32_t)battery_measure.currPercentage,(uint32_t)calc_energy_changing_str.simulate_dump_energy);
    }
    ADC_BAT_CAP_COULOMBMETER_TRACE(2, "currPer:%d,dump_energy:%d", battery_measure.currPercentage,calc_energy_changing_str.simulate_dump_energy);

    return battery_measure.currPercentage;
}

#endif

