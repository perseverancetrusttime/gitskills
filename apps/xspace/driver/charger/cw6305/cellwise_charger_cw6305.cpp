/*-----------------------------------------------------------------------------
 * @file     | cellwise_charger_cw6305.c
 *-----------+-----------------------------------------------------------------
 * @function | cw6305 interface function
 *-----------+-----------------------------------------------------------------
 * @version  | Cellwise-0.1.
 * @author   | CELLWISE FAE. Chaman.
 * @note     | 2019/06.
 *---------------------------------------------------------------------------*/
#include "xspace_i2c_master.h"
#include "hal_trace.h"
#include "cellwise_charger_cw6305.h"
#include <stdio.h>
#include <stdarg.h>

typedef unsigned char bit;
#define CW6305_DEV_ADDR   (0X0B) // 7-bit address



static bit cw_read(unsigned char PointReg,unsigned char *pData)
{
	bool bRet = true;

	bRet = xspace_i2c_read(XSPACE_SW_I2C, CW6305_DEV_ADDR, PointReg, pData);
	return bRet;
}
// ret: 1-true;0-false
static bit cw_write(unsigned char PointReg,unsigned char *pData)
{
	bool bRet;

	bRet = xspace_i2c_write(XSPACE_SW_I2C, CW6305_DEV_ADDR, PointReg, *pData);
	return bRet;
}


/*static function, PointReg: Register location, bit_start: The start bit you want set, bit_num : How many bit do you want set, val : value */
static int set_reg_bit(unsigned char PointReg, unsigned char bit_start, unsigned char bit_num, unsigned char val)
{
    int ret = 0;
    unsigned char reg_val = 0;
    unsigned char clear_zero = 0;
    unsigned char read_reg_val = 0;	
    ret = cw_read(PointReg, &reg_val);
    if(false == ret){
        return -2;//ret;
    }
	
	val = val<<(bit_start - (bit_num - 1));
    for(; bit_num > 0; bit_num--){
        clear_zero = clear_zero | (1 << bit_start);
        bit_start--;
    }
    clear_zero = ~clear_zero;
    reg_val = reg_val & clear_zero;
    reg_val = reg_val | val;
    ret = cw_write(PointReg, &reg_val);
    if(false == ret){
        return -1;//ret;
    }
	ret = cw_read(PointReg, &read_reg_val);
    if((false == ret) || (reg_val != read_reg_val)){
        return -1;//ret;
    }    
    return 0;
}

static int get_reg_value(unsigned char PointReg)
{
    unsigned char reg_val = 0;
    int ret = 0;
    ret = cw_read(PointReg, &reg_val);
    if(false == ret){
        return -1;//ret;
    }
    return reg_val;
}

int cwset_HiZ_control(int enable)
{
    if(enable != HIZ_CONTROL_DISABLE && enable != HIZ_CONTROL_ENABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_VBUS_VOLT, BIT(7), 1, enable);
}

int cwset_chg_enable(int enable)
{
    if(enable != CHG_ENABLE && enable != CHG_DISABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_VBUS_VOLT, BIT(6), 1, enable);
}

int cwset_Vbus_input_vol_limit(int vol)
{
    int step = 0;
    int ret = 0;
    if(vol < VBUS_INPUT_VOL_LIMIT_OFFSET || vol > VBUS_INPUT_VOL_LIMIT_MAX){
        return EINVAL;
    }
    step = (vol - VBUS_INPUT_VOL_LIMIT_OFFSET) / VBUS_INPUT_VOL_LIMIT_STEP;
    ret = set_reg_bit(REG_VBUS_VOLT, BIT(3), 4, step);
    return ret;
}

int cwset_Vsys_regualation_vol(int vol)
{
    int step = 0;
    int ret = 0;
    if(vol < VBUS_REGUALATION_VOL_OFFSET || vol > VBUS_REGUALATION_VOL_MAX){
        return EINVAL;
    }
    step = (vol - VBUS_REGUALATION_VOL_OFFSET) / VBUS_REGUALATION_VOL_STEP;
    ret = set_reg_bit(REG_VBUS_CUR, BIT(7), 4, step);
    return ret;
    
}

int cwset_Vbus_input_cur_limit(int cur)
{
    int step = 0;
    int ret = 0;
    if(cur < VBUS_INPUT_CUR_LIMIT_OFFSET || cur > VBUS_INPUT_CUR_LIMIT_MAX){
        return EINVAL;
    }
    step = (cur - VBUS_INPUT_CUR_LIMIT_OFFSET) / VBUS_INPUT_CUR_LIMIT_STEP;
    ret = set_reg_bit(REG_VBUS_CUR, BIT(3), 4, step);
    return ret;
    
}
// return : 0 - ok
//          other - err
int cwset_CV_vol(int vol)
{
    int step = 0;
    int ret = 0;
    vol = vol * 10;
    if(vol < CV_OFFSET || vol > CV_MAX){
        return EINVAL;
    }
    step = (vol - CV_OFFSET) / CV_STEP;
    ret = set_reg_bit(REG_CHG_VOLT, BIT(7), 6, step);
    return ret;
}

int cwset_rechg_vol_step(int vol_step)
{
    if(vol_step != RECHG_VOL_200MV && vol_step != RECHG_VOL_100MV){
        return EINVAL;
    }
    return set_reg_bit(REG_CHG_VOLT, BIT(1), 1, vol_step);
}

int cwset_prechg_to_FCCchg_vol(int vol_step)
{
    if(vol_step != PRECHG_TO_CCCHG_VOL_3000 && vol_step != PRECHG_TO_CCCHG_VOL_2800){
        return EINVAL;
    }
    return set_reg_bit(REG_CHG_VOLT, BIT(0), 1, vol_step);
}

int cwset_IC_internal_max_thermal(int thermal)
{
    int step = 0;
    int ret = 0;
    if(thermal < IC_INTERNAL_MAX_THERMAL_OFFSET || thermal > IC_INTERNAL_MAX_THERMAL_MAX){
        return EINVAL;
    }
    step = (thermal - IC_INTERNAL_MAX_THERMAL_OFFSET) / IC_INTERNAL_MAX_THERMAL_STEP;
    ret = set_reg_bit(REG_CHG_CUR1, BIT(7), 2, step);
    return ret;    
}

int cwset_FCCchg_cur(unsigned int cur)
{
    int step = 0;
    int ret = 0;
    cur = cur * 100;
    
    if(cur < FCCCHG_CUR_OFFSET || cur > FCCCHG_CUR_MAX){
        return EINVAL;
    }
    if(cur <= FCCCHG_CUR_OFFSET * 2){  /*10 ~ 20*/
        step = (cur - FCCCHG_CUR_OFFSET) / FCCCHG_CUR_STEP1;
    }else if(cur <= FCCCHG_CUR_OFFSET * 2 * 2){  /*20 ~ 40*/
        step = (FCCCHG_CUR_OFFSET * 2 - FCCCHG_CUR_OFFSET) / FCCCHG_CUR_STEP1 
                + (cur - FCCCHG_CUR_OFFSET * 2) / (FCCCHG_CUR_STEP1 * 2);
    }else if(cur <= FCCCHG_CUR_OFFSET * 2 * 2 * 2){ /*40 ~ 80*/
        step = (FCCCHG_CUR_OFFSET * 2 - FCCCHG_CUR_OFFSET) / FCCCHG_CUR_STEP1 
                + (FCCCHG_CUR_OFFSET * 2 * 2 - FCCCHG_CUR_OFFSET * 2) / (FCCCHG_CUR_STEP1 * 2)
                + (cur - FCCCHG_CUR_OFFSET * 2 * 2) / (FCCCHG_CUR_STEP1 * 2 * 2);
    }else{  /*80 ~ 470*/
        step = (FCCCHG_CUR_OFFSET * 2 - FCCCHG_CUR_OFFSET) / FCCCHG_CUR_STEP1 
                + (FCCCHG_CUR_OFFSET * 2 * 2 - FCCCHG_CUR_OFFSET * 2) / (FCCCHG_CUR_STEP1 * 2)
                + (FCCCHG_CUR_OFFSET * 2 * 2 * 2 - FCCCHG_CUR_OFFSET * 2 * 2) / (FCCCHG_CUR_STEP1 * 2 * 2)    
                + (cur - FCCCHG_CUR_OFFSET * 2 * 2 * 2) / (FCCCHG_CUR_STEP1 * 2 * 2 * 2);
    }
    ret = set_reg_bit(REG_CHG_CUR1, BIT(5), 6, step);
    return ret;    
}

int cwset_force_prechg(int enable)
{
    if(enable != FORCE_PRECHG_ENABLE && enable != FORCE_PRECHG_DISABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_CHG_CUR2, BIT(7), 1, enable);    
}

int cwset_prechg_cur(int curX10)
{
    int step = 0;
    int ret = 0;
    
    if(curX10 < PRECHG_AND_TCC_CUR_OFFSET || curX10 > PRECHG_AND_TCC_CUR_MAX){
        return EINVAL;
    }
    step = (curX10 - PRECHG_AND_TCC_CUR_OFFSET) / PRECHG_AND_TCC_CUR_STEP;
    ret = set_reg_bit(REG_CHG_CUR2, BIT(6), 3, step);
    return ret;    
}

int cwset_TCC_cur(int curX10)
{
    int step = 0;
    int ret = 0;
    
    if(curX10 < PRECHG_AND_TCC_CUR_OFFSET || curX10 > PRECHG_AND_TCC_CUR_MAX){
        return EINVAL;
    }
    step = (curX10 - PRECHG_AND_TCC_CUR_OFFSET) / PRECHG_AND_TCC_CUR_STEP;
    ret = set_reg_bit(REG_CHG_CUR2, BIT(2), 3, step);
    return ret;        
}

int cwset_reset_all_register(int enable)
{
    if(enable != REG06_BIT_SET && enable != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(7), 1, enable);    
}

int cwset_chg_time_out_timer(int hours)
{
    int step = 0;
    int ret = 0;
    
    if(hours < CHG_TIME_OUT_TIMER_OFFSET || hours > CHG_TIME_OUT_TIMER_MAX){
        return EINVAL;
    }
    step = (hours - CHG_TIME_OUT_TIMER_OFFSET) / CHG_TIME_OUT_TIMER_STEP;
    ret = set_reg_bit(REG_SAFETY, BIT(6), 2, step);
    return ret;    
}

int cwset_chg_auto_termination(int enable)
{
    if(enable != REG06_BIT_SET && enable != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(4), 1, enable);    
}

int cwset_prechg_time_out_enable(int enable)
{
    if(enable != REG06_BIT_SET && enable != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(3), 1, enable);    
}

int cwset_ntc_enable(int enable)
{
    if(enable != REG06_BIT_SET && enable != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(2), 1, enable);    
}

int cwset_ntc_select_R(int r)
{
    if(r != REG06_BIT_SET && r != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(1), 1, r);    
}

int cwset_ntc_select_pull_up_net(int net)
{
    if(net != REG06_BIT_SET && net != REG06_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY, BIT(0), 1, net);    
}

int cwset_force_bat_off(int enable)
{
    if(enable != REG07_BIT_SET && enable != REG07_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_CONFIG, BIT(7), 1, enable);    
}

int cwset_bat_off_delay_time(int sec)           /*0x07 Bit6~Bit5*/
{
    int step = 0;
    int ret = 0;
    switch(sec){
        case 1:
            step = 0;
            break;
        case 2:
            step = 1;
            break;
        case 4:
            step = 2;
            break;
        case 8:
            step = 3;
            break;
        default:    
            /*Did not change*/
            return EINVAL;
    }
    ret = set_reg_bit(REG_CONFIG, BIT(6), 2, step);
    return ret;        
}

int cwset_int_btn_func_enable(int enable)       /*0x07 Bit4*/
{
    if(enable != REG07_BIT_SET && enable != REG07_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_CONFIG, BIT(4), 1, enable);    
}

int cwset_btn_func_select(int vsys_reset)       /*0x07 Bit3*/  /*REG07_BIT_SET : vsys reset */  /*REG07_BIT_CLEAR : BATFET off*/
{
    if(vsys_reset != REG07_BIT_SET && vsys_reset != REG07_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_CONFIG, BIT(3), 1, vsys_reset);        
}

int cwset_vsys_reset_delay_time(int sec)        /*0x07 Bit2*/  /*vsys reset time*/ /*REG07_BIT_SET : 4s, REG07_BIT_CLEAR : 2s*/
{
    if(sec != REG07_BIT_SET && sec != REG07_BIT_CLEAR){
        return EINVAL;
    }
    return set_reg_bit(REG_CONFIG, BIT(2), 1, sec);        
}

int cwset_int_btn_hold_time(int sec)            /*0x07 Bit1~Bit0*/
{
    int step = 0;
    int ret = 0;
    
    if(sec < INT_BTN_HOLD_TIME_OFFSET || sec > INT_BTN_HOLD_TIME_MAX){
        return EINVAL;
    }
    step = (sec - INT_BTN_HOLD_TIME_OFFSET) / INT_BTN_HOLD_TIME_STEP;
    ret = set_reg_bit(REG_CONFIG, BIT(1), 2, step);
    return ret;        
}

int cwset_chg_watchdog_enable(int enable)       /*0x08 Bit7*/
{
    if(enable != WATCHDOG_ENABLE && enable != WATCHDOG_DISABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY_CFG, BIT(7), 1, enable);            
}

int cwset_dischg_watchdog_enable(int enable)     /*0x08 Bit6*/
{
    if(enable != WATCHDOG_ENABLE && enable != WATCHDOG_DISABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY_CFG, BIT(6), 1, enable);        
}

int cwset_watchdog_timer(int sec)               /*0x08 Bit5~Bit4*/         //Mark
{
    int step = 0;
    int ret = 0;
    
    if(sec < WATCHDOG_TIMER_OFFSET || sec > WATCHDOG_TIMER_MAX){
        return EINVAL;
    }
    step = (sec - WATCHDOG_TIMER_OFFSET) / WATCHDOG_TIMER_STEP;
    ret = set_reg_bit(REG_SAFETY_CFG, BIT(5), 2, step);
    return ret;        
}

int cwset_UVLO_vol(int vol)                     /*0x08 Bit2~Bit0*/         //Mark
{
    int step = 0;
    int ret = 0;
    
    if(vol < UVLO_VOL_OFFSET || vol > UVLO_VOL_MAX){
        return EINVAL;
    }
    step = (vol - UVLO_VOL_OFFSET) / UVLO_VOL_STEP;
    ret = set_reg_bit(REG_SAFETY_CFG, BIT(2), 3, step);
    return ret;        
}

int cwset_2X_safety_timer_PPM(int enable)       /*0x09 Bit7*/
{
    if(enable != X2_SAFETY_TIMER_ENABLE && enable != X2_SAFETY_TIMER_DISABLE){
        return EINVAL;
    }
    return set_reg_bit(REG_SAFETY_CFG2, BIT(7), 1, enable);        
}

int cwset_vsys_over_cur_protection_cur(int cur) /*0x09 Bit3~Bit0*/
{
    int step = 0;
    int ret = 0;
    
    if(cur < VSYS_OVER_CUR_PROTECTION_CUR_OFFSET || cur > VSYS_OVER_CUR_PROTECTION_CUR_MAX){
        return EINVAL;
    }
    step = (cur - VSYS_OVER_CUR_PROTECTION_CUR_OFFSET) / VSYS_OVER_CUR_PROTECTION_CUR_STEP;
    ret = set_reg_bit(REG_SAFETY_CFG2, BIT(3), 4, step);
    return ret;            
}

int cwset_chg_interrupt(unsigned char interrupt_val) /*0x0A Bit7~Bit0*/
{
    return set_reg_bit(REG_INT_SET, BIT(7), 8, interrupt_val);
}

int cwset_clean_interrupt_source(int count, ...)     /*0x0B Bit7~Bit0*/
{
    int i;
    int res = 0;
    va_list v1;
    unsigned char clean_value = 0xFF;
    unsigned char reg_value = 0;
    if(count < 1 || count > 8){
        return EINVAL;
    }
    if(count == 8){
        return set_reg_bit(REG_INT_SRC, BIT(7), 8, 0x00);
    }
    va_start(v1, count);
    for(i = 0; i < count; i++)
    {
        res = va_arg(v1, int); 
        if(res < 0 || res > 7){
            return EINVAL;
        }
        //printf("res = %d\n",res);
        clean_value &= ~(1 << res);
    } 
    //printf("clean_value = %d\n", (int)clean_value);
    va_end(v1); 
    reg_value = get_reg_value(REG_INT_SRC);
    reg_value = reg_value & clean_value;
    //printf("reg_value = %d\n", (int)reg_value);
    return set_reg_bit(REG_INT_SRC, BIT(7), 8, reg_value);
}

int cwget_version(void)                              /*0x00*/
{
    return get_reg_value(REG_VERSION);
}

int cwget_Interrupt_Source(void)                     /*0x0B*/
{
    return get_reg_value(REG_INT_SRC);
}

int cwget_Condition(void)                            /*0x0D*/
{
    return get_reg_value(REG_IC_STATUS);
}

int cwget_Condition_II(void)                         /*0x0E*/
{
    return get_reg_value(REG_IC_STATUS2);
}


