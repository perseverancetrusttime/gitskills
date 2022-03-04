/*-----------------------------------------------------------------------------
 * @file     | cellwise_charger_cw6305.h
 *-----------+-----------------------------------------------------------------
 * @function | cellwise_charger_cw6305.c Function prototype include definition
 *-----------+-----------------------------------------------------------------
 * @version  | Cellwise-0.1.
 * @author   | CELLWISE FAE. Chaman.
 * @note     | 2019/06.
 *---------------------------------------------------------------------------*/
#ifndef _cw6305_charger_H_
#define _cw6305_charger_H_

/**************************** Abbreviation Table ***********************************************/
/*  REG reg->register,  vol->voltage, cur->current, chg->charger charge, prechg->pre_charge    */
/*  CV->Battery Charge Finish Voltage,  rechg->recharge,  FCCchg->fast charge, func->function  */
/*  TCC->Termination Charger Current, Bat->battery, UVLO->Battery Under Voltage Lockout        */
/*  btn->button, sec->second,                                                                  */

/*Register define*/
#define REG_VERSION     0x00   /*Read only*/
#define REG_VBUS_VOLT   0x01
#define REG_VBUS_CUR    0x02
#define REG_CHG_VOLT    0x03
#define REG_CHG_CUR1    0x04
#define REG_CHG_CUR2    0x05
#define REG_SAFETY      0x06
#define REG_CONFIG      0x07
#define REG_SAFETY_CFG  0x08
#define REG_SAFETY_CFG2 0x09
#define REG_INT_SET     0x0A
#define REG_INT_SRC     0x0B
#define REG_IC_STATUS   0x0D   /*Read only*/
#define REG_IC_STATUS2  0x0E   /*Read only*/

#define BIT(num) (int)num
#define EINVAL         -111    /*Invalid argument*/
#define EIO            -112    /*I/O communication error*/

/*Function*/
/*Config set Function*/
/**************************************************************************************************/
/*                     �����е�ע�ͷǳ���Ҫ���˴�������Ҫ������ϸ�Ķ�                             */
/*                     *��������������Ϲ����ͺ�����һ���Ķ�*                                   */
/*config set function�����ࣺ                                                                     */
/*��һ�ֻࣺ����һ��bit�ĺ���,Ӧ�ô�������ֵ��ֻ����1��0������ֵ�ᱻ��������ֵ������������ֵ�˼�*/
/*        ��Ӧbit�����ֲ��䣬��������EINVAL��Invalid argument������������ʱʹ�ö�Ӧ�ĺ����Ӻ����� */
/*        ���ԣ�����cwset_HiZ_control��������ʹ��HIZ_CONTROL_ENABLE��HIZ_CONTROL_DISABLE����Ϊ����*/                                
/*�ڶ��ࣺ���ö��bit���������ݲ������ã�OFFSET�Ǵ˺��������õ���Сֵ��Ҳ�����ò���������MAX�����*/
/*        ���������õ����ֵ��STEP�ǲ�������cwset_Vbus_input_vol_limitΪ������Сֵ������������4200*/
/*        ���ֵ4950�����ͻ����ó�����Χֵʱ��������Ч������EINVAL��-111�����ͻ������м���κ�һ��*/
/*        ����������ݲ�������ֵ����ӽ���ֵ�����紫ֵ4250��ʵ�ʾͻ����ó�4250�������ֵ4310����Ϊ*/
/*        ������50���������ò���4310���ֵ����Ҳ����ݲ������õ�4300                              */
/*�����ࣺcwset_bat_off_delay_time���˺���Ҳ�����ö��bit�ģ�����Ϊ������2��ָ����ʽ��ͨ�����벻��*/
/*        ʵ�֣�����ʹ����switch��ʽ���������ֻ�ܴ�4��ֵ��ȥ������ֵ�ᱻ��Ϊ����Ч����EINVAL���� */
/*        �Դ����4��ֵ��1��2��4��8                                                               */
/*�����ࣺcwset_chg_interrupt���˺����������ж�ʹ�ܵģ�û�ж�ÿ���ж�ʹ�ܵ������÷����������ʹ�� */
/*        ��رղ����жϣ������0x0A  interrupt map �Լ�������Ӧ��bitλ���ɡ�������Ҫ��BAT_OT   */
/*        BAT_UT������ֵ���ֲ��䣬����Ҫ���ó�1010 1100�������cwset_chg_interrupt(0xAC);����     */
/*�����ࣺcwset_clean_interrupt_source���˺������������жϱ�ǵģ�0B�Ĵ�����ʾ�����������жϣ�����*/
/*        �Զ����������BAT_OT���������жϣ������ϻָ������ˣ���Condition�Ĵ���������ֻ�ǵ�ǰ״̬ */
/*        ��0B�Ĵ������Կ������������������飬0B�Ĵ�����Ҫ�ֶ��������Ϊ����ʹ���߿���ֻ���������*/
/*        ĳЩ���������������Ĵ���������ʹ�õ��ǿɱ������������һ��������ʾҪ�����bit������ */
/*        ��������ֵ���Դ�1��8��������Χ�᷵����Ч������8�Ǹ�����ֵ����ʾҪ�������Ĵ���������Ҫ */
/*        ����д����Ĳ��������ֻ����������е�Bit7��Bit6�����һ����������2��Ҫ����ĸ��������� */
/*        ���������Ҫ�����λ��BIT(7),BIT(6),��Ϊ�ɱ���������Ա�α���������Ҫ��ÿ���������ѳ�*/
/*        ����int�ͻ����������͸��ݱ�������ͬҲ��ͬ�������벻Ҫֱ��ʹ�ó�����Ϊ��β�������ʹ���� */
/*        ����ĺ�BIT(num),���������ӣ�����Ҫ���Bit7,bit6,                                       */
/*        ���������Ӧ����cwset_clean_interrupt_source(2, BIT(7), BIT(6));                        */
/*        ǧ��Ҫд��cwset_clean_interrupt_source(2, 7, 6);                                      */
/*        �Ĵ���ȫ������cwset_clean_interrupt_source(8);                                          */
/**************************************************************************************************/
#define HIZ_CONTROL_ENABLE    1
#define HIZ_CONTROL_DISABLE   0
int cwset_HiZ_control(int enable);               /*0x01 Bit7*/
#define CHG_ENABLE            1
#define CHG_DISABLE           0
int cwset_chg_enable(int enable);                /*0x01 Bit6*/
#define VBUS_INPUT_VOL_LIMIT_OFFSET 4200
#define VBUS_INPUT_VOL_LIMIT_MAX    4950
#define VBUS_INPUT_VOL_LIMIT_STEP   50
int cwset_Vbus_input_vol_limit(int vol);         /*0x01 Bit3~Bit0*/

#define VBUS_REGUALATION_VOL_OFFSET 4200
#define VBUS_REGUALATION_VOL_MAX    4950
#define VBUS_REGUALATION_VOL_STEP   50
int cwset_Vsys_regualation_vol(int vol);         /*0x02 Bit7~Bit4*/
#define VBUS_INPUT_CUR_LIMIT_OFFSET 50
#define VBUS_INPUT_CUR_LIMIT_MAX    500
#define VBUS_INPUT_CUR_LIMIT_STEP   30
int cwset_Vbus_input_cur_limit(int cur);         /*0x02 Bit3~Bit0*/

#define CV_OFFSET  3850*10
#define CV_MAX     4600*10
#define CV_STEP    125
/*cwset_CV_vol function parameter min 3850, max 4600*/
int cwset_CV_vol(int vol);                       /*0x03 Bit7~Bit2*/
#define RECHG_VOL_200MV 1
#define RECHG_VOL_100MV 0
int cwset_rechg_vol_step(int vol_step);          /*0x03 Bit1*/
#define PRECHG_TO_CCCHG_VOL_3000 1
#define PRECHG_TO_CCCHG_VOL_2800 0
int cwset_prechg_to_FCCchg_vol(int vol_step);    /*0x03 Bit0*/

#define IC_INTERNAL_MAX_THERMAL_OFFSET 60
#define IC_INTERNAL_MAX_THERMAL_MAX    120
#define IC_INTERNAL_MAX_THERMAL_STEP   20
int cwset_IC_internal_max_thermal(int thermal);  /*0x04 Bit7~Bit6*/
#define FCCCHG_CUR_OFFSET 10*100
#define FCCCHG_CUR_MAX    470*100
#define FCCCHG_CUR_STEP1  125
/*cwset_FCCchg_cur function parameter min 10, max 470*/
int cwset_FCCchg_cur(unsigned int cur);          /*0x04 Bit5~Bit0*/

#define FORCE_PRECHG_ENABLE  1
#define FORCE_PRECHG_DISABLE 0
int cwset_force_prechg(int enable);              /*0x05 Bit7*/
#define PRECHG_AND_TCC_CUR_OFFSET 25 /*2.5% of fast charge current*/
#define PRECHG_AND_TCC_CUR_MAX    200
#define PRECHG_AND_TCC_CUR_STEP   25
int cwset_prechg_cur(int curX10);                /*0x05 Bit6~Bit4*/
int cwset_TCC_cur(int curX10);                      /*0x05 Bit2~Bit0*/

#define REG06_BIT_SET   1
#define REG06_BIT_CLEAR 0
int cwset_reset_all_register(int enable);        /*0x06 Bit7*/
#define CHG_TIME_OUT_TIMER_OFFSET 0
#define CHG_TIME_OUT_TIMER_MAX    12
#define CHG_TIME_OUT_TIMER_STEP   4
int cwset_chg_time_out_timer(int hours);         /*0x06 Bit6~Bit5*/
int cwset_chg_auto_termination(int enable);      /*0x06 Bit4*/
int cwset_prechg_time_out_enable(int enable);    /*0x06 Bit3*/
int cwset_ntc_enable(int enable);                /*0x06 Bit2*/
int cwset_ntc_select_R(int r);                   /*0x06 Bit1*/
int cwset_ntc_select_pull_up_net(int net);       /*0x06 Bit0*/

#define REG07_BIT_SET   1
#define REG07_BIT_CLEAR 0
int cwset_force_bat_off(int enable);             /*0x07 Bit7*/
#define BAT_OFF_DELAY_TIME_MIN    1
#define BAT_OFF_DELAY_TIME_MAX    8
int cwset_bat_off_delay_time(int sec);           /*0x07 Bit6~Bit5*/
int cwset_int_btn_func_enable(int enable);       /*0x07 Bit4*/
int cwset_btn_func_select(int vsys_reset);       /*0x07 Bit3*/
int cwset_vsys_reset_delay_time(int sec);        /*0x07 Bit2*/
#define INT_BTN_HOLD_TIME_OFFSET  8
#define INT_BTN_HOLD_TIME_MAX     20
#define INT_BTN_HOLD_TIME_STEP    4
int cwset_int_btn_hold_time(int sec);            /*0x07 Bit1~Bit0*/

#define WATCHDOG_ENABLE   1
#define WATCHDOG_DISABLE  0
int cwset_chg_watchdog_enable(int enable);       /*0x08 Bit7*/
int cwset_dischg_watchdog_enable(int enable);    /*0x08 Bit6*/
#define WATCHDOG_TIMER_OFFSET  60
#define WATCHDOG_TIMER_MAX     240
#define WATCHDOG_TIMER_STEP    60
int cwset_watchdog_timer(int sec);               /*0x08 Bit5~Bit4*/         //Mark
#define UVLO_VOL_OFFSET        2500
#define UVLO_VOL_MAX           3200
#define UVLO_VOL_STEP          100
int cwset_UVLO_vol(int vol);                     /*0x08 Bit2~Bit0*/         //Mark

#define X2_SAFETY_TIMER_ENABLE  1
#define X2_SAFETY_TIMER_DISABLE 0
int cwset_2X_safety_timer_PPM(int enable);       /*0x09 Bit7*/
#define VSYS_OVER_CUR_PROTECTION_CUR_OFFSET 100
#define VSYS_OVER_CUR_PROTECTION_CUR_MAX    1600
#define VSYS_OVER_CUR_PROTECTION_CUR_STEP   100
int cwset_vsys_over_cur_protection_cur(int cur); /*0x09 Bit3~Bit0*/

/**************************************************************************/
/*                  0x0A  interrupt map                                   */
/*0x0A INT_SET: Interrupt Enable 1: Enable interrupt, 0: Disable interrupt*/
/*           ����������������������������������������������������������������������������������������                 */
/*             |BIT   |      NAME    |  TYPE  |  DEFAULT  |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit7  | CHG_DET      |  R/W   |     1     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit6  | CHG_REMOVE   |  R/W   |     0     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit5  | CHG_FINISH   |  R/W   |     1     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit4  | CHG_OV       |  R/W   |     0     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit3  | BAT_OT       |  R/W   |     0     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit2  | BAT_UT       |  R/W   |     0     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit1  | SAFETY_TIMER |  R/W   |     0     |               */
/*             |������������|����������������������������|����������������|����������������������|               */
/*             |Bit0  | PRECHG_TIMER |  R/W   |     0     |               */
/*             ����������������������������������������������������������������������������������������               */
/**************************************************************************/
int cwset_chg_interrupt(unsigned char interrupt_val);/*0x0A Bit7~Bit0*/
#define INT_SRC_CHG_DET      (1<<7)
#define INT_SRC_CHG_REMOVE   (1<<6)
#define INT_SRC_CHG_FINISH   (1<<5)
#define INT_SRC_CHG_OV       (1<<4)
#define INT_SRC_CHG_OT       (1<<3)
#define INT_SRC_BAT_UT       (1<<2)
#define INT_SRC_SAFETY_TIMER (1<<1)
#define INT_SRC_PRECHG_TIMER (1<<0)
int cwset_clean_interrupt_source(int count, ...);    /*0x0B Bit7~Bit0*/

/*Function*/
/*register get Function*/
int cwget_version(void);                             /*0x00*/
int cwget_Interrupt_Source(void);                    /*0x0B*/
int cwget_Condition(void);                           /*0x0D*/
int cwget_Condition_II(void);                        /*0x0E*/
#define STATUS_VBUS_PG     (1<<7)
#define STATUS_VBUS_OV     (1<<6)
#define STATUS_IC_CHARGING (1<<5)
#define STATUS_IC_OT       (1<<4)
#define STATUS_BAT_OT      (1<<3)
#define STATUS_BAT_UT      (1<<2)
#define STATUS_SAFETY_TIMER_EXPIRATION  (1<<1)
#define STATUS_PRECHG_TIMER_EXPIRATION  (1<<0)


#endif
