/*
 *-----------------------------------------------------------------------------
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from  CHIPSEA.
 *
 *            (C) COPYRIGHT 2020 SHENZHEN CHIPSEA TECHNOLOGIES CO.,LTD.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from CHIPSEA.
 *
 *      Release Information : CSA37F71 chip forcetouch fw driver source file 
 *      version : v0.1
 *-----------------------------------------------------------------------------
 */
#include "cmsis.h"
#include "cmsis_os.h"
#include "cs_press_f71_driver.h"
#include "csa37f71_adapter.h"
#include "xspace_i2c_master.h"
#include "tgt_hardware.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "hal_timer.h"


#define RSTPIN_RESET_ENABLE             1

#define RETRY_NUM                       2       // n+1
#define DEBUG_MODE_DELAY_TIME           20

#define CS_MANUFACTURER_ID_LENGTH       2
#define CS_MODULE_ID_LENGTH             2
#define CS_FW_VERSION_LENGTH            2

#define AFE_MAX_CH                      8

#define DEBUG_MODE_REG                  0x60
#define DEBUG_READY_REG                 0x61
#define DEBUG_DATA_REG                  0x62

#define FW_ADDR_CODE_LENGTH             0x0e
#define FW_ADDR_VERSION                 0x08
#define FW_ADDR_CODE_START				0x100

#define FW_ONE_BLOCK_LENGTH_W           128
#define FW_ONE_BLOCK_LENGTH_R           256

#define AP_DEVICE_ID_REG                0x02
#define AP_MANUFACTURER_ID_REG          0x03
#define AP_MODULE_ID_REG                0x04
#define AP_VERSION_REG                  0x05
#define AP_WAKEUP_REG                   0x06
#define AP_SLEEP_REG                    0x07

#define AP_CALIBRATION_REG              0x1c
#define AP_WATCH_MODE_REG               0x1d
#define AP_RW_TEST_REG                  0x1f
#define AP_FORCEDATA_REG                0x20

#define AP_W_CAL_FACTOR_DEBUG_MODE      0x30
#define AP_R_CAL_FACTOR_DEBUG_MODE      0x31

#define AP_W_PRESS_LEVEL_DEBUG_MODE     0x32
#define AP_R_PRESS_LEVEL_DEBUG_MODE     0x33

#define AP_CALIBRATION_DEBUG_MODE     	0x34

#define AP_R_RAWDATA_DEBUG_MODE         0x01
#define AP_R_OFFSET_DEBUG_MODE		    0x04
#define AP_R_NOISE_DEBUG_MODE           0x11
#define AP_R_PROCESSED_DEBUG_MODE       0x14

#define BOOT_CMD_REG                    0x0000
#define BOOT_CMD_LENGTH                 4

#define DAC_RANGE_MAX                   540
#define AFE_VS_VOLTAGE                  2800	// mV
#define AFE_PGA                         512
#define ADCtoMV                         0.00267	// 2800/512/2^11

#define CALIBRATION_SUCCESS_FALG        0xf0
#define CALIBRATION_FAIL_FALG           0xf2
#define CALIBRATION_OVERTIME_FALG       0xff

unsigned char boot_fw_write_cmd[BOOT_CMD_LENGTH] = {0xaa,0x55,0xa5,0x5a};
// unsigned char boot_fw_read_cmd[BOOT_CMD_LENGTH] = {0x55,0xa5,0xa5};
// unsigned char boot_fw_jump_cmd[BOOT_CMD_LENGTH] = {0x55,0xa5,0xaa};
// unsigned char boot_fw_reset_cmd[BOOT_CMD_LENGTH] = {0x55,0xa5,0x55};
unsigned char boot_fw_wflag_cmd[BOOT_CMD_LENGTH] = {0x50,0x41,0x53,0x73};

unsigned char boot_status_reg_dat[4] ={0x10,0x01,0x18,0x60}; 
unsigned char boot_starus_reg_dat_vaildbit[4] = {0xf0,0xff,0xff,0xf0};

unsigned char ap_status_reg_dat[4] ={0x80,0x81,0x18,0x60}; 
unsigned char ap_starus_reg_dat_vaildbit[4] = {0xff,0xff,0xff,0xf0};

typedef struct{
	char pga;
	char ch_num;
	short dac[AFE_USE_CH];
    short adc[AFE_USE_CH];
}CS_AFE_INFO_Def;

/**
  * @brief  delay function
  * @param  time_ms: delay time, unit:ms
  * @retval None
  */
static void cs_press_delay_ms(unsigned int time_ms)
{
    // user program 
	osDelay(time_ms);
    //hal_sys_timer_delay(MS_TO_TICKS(time_ms));
}
#if !defined(RSTPIN_RESET_ENABLE)
/**
  * @brief  ic power on function
  * @param  None
  * @retval None
  */
static void cs_press_power_up(void)
{
    // user program 
}

/**
  * @brief  ic power down function
  * @param  None
  * @retval None
  */
static void cs_press_power_down(void)
{
    // user program 
}

/**
  * @brief  ic rst pin set high
  * @param  None
  * @retval None
  */
#else
static void cs_press_rstpin_high(void)
{
    // user program 
	hal_gpio_pin_set((enum HAL_GPIO_PIN_T)pressure_reset_cfg.pin);
}

/**
  * @brief  ic rst pin set low
  * @param  None
  * @retval None
  */
static void cs_press_rstpin_low(void)
{
    // user program 
	hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)pressure_reset_cfg.pin);
}
#endif
/**
  * @brief  iic write funciton
  * @param  regAddress: reg data, *dat: point to data to write, length: write data length
  * @retval 0:success, -1: fail
  */
static char cs_press_iic_write(unsigned char regAddress ,const unsigned char *dat, unsigned int length)
{
    unsigned char ret;
    unsigned char data_buf[256]={0};
	if(length>256)
		memcpy(data_buf, dat, 256);
	else
		memcpy(data_buf, dat, length);
    
    // user program 
	ret = !xspace_i2c_burst_write(CSA37F71_I2C_TYPE, CSA37F71_I2C_ADDR, regAddress, data_buf, length);
//	CSA37F71_TRACE(1, "cs_press_iic_write reg:0x%x",regAddress);
//	DUMP8("0x%x, ",data_buf,length);
    return ret;
}


/**
  * @brief  iic read funciton
  * @param  regAddress: reg data, *dat: read data buffer, length: read data length
  * @retval 0:success, -1: fail
  */
static char cs_press_iic_read(unsigned char regAddress ,unsigned char *dat, unsigned int length)
{
    unsigned char ret;
    
    // user program 
	ret = !xspace_i2c_burst_read(CSA37F71_I2C_TYPE, CSA37F71_I2C_ADDR, regAddress, dat, length);
//	CSA37F71_TRACE(1, "cs_press_iic_read reg:0x%x",regAddress);
//	DUMP8("0x%x, ",dat,length);
    return ret;
}

/**
  * @brief  iic write funciton
  * @param  regAddress: reg data, *dat: point to data to write, length: write data length
  * @retval 0:success, -1: fail
  */
static char cs_press_iic_write_double_reg(unsigned short regAddress ,const unsigned char *dat, unsigned int length)
{
    unsigned char ret;
    unsigned char data_buf[256]={0};
	
	if(length>256)
		memcpy(data_buf, dat, 256);
	else
		memcpy(data_buf, dat, length);
	
    // user program 
	ret = !xspace_i2c_burst_write_s16(CSA37F71_I2C_TYPE, CSA37F71_I2C_ADDR, regAddress, data_buf, length);
//	CSA37F71_TRACE(1, "write_double_reg reg:0x%x",regAddress);
//	DUMP8("0x%x, ",data_buf,length);
    return ret;
}


/**
  * @brief  iic read funciton
  * @param  regAddress: reg data, *dat: read data buffer, length: read data length
  * @retval 0:success, -1: fail
  */
static char cs_press_iic_read_double_reg(unsigned short regAddress ,unsigned char *dat, unsigned int length)
{
    unsigned char ret;
	
    // user program 
    unsigned char reg_buf[2] = {0};
	reg_buf[0] = (regAddress >> 8) & 0xff;
	reg_buf[1] = regAddress & 0xff;
    ret = !xspace_i2c_burst_read_s16(CSA37F71_I2C_TYPE, CSA37F71_I2C_ADDR, reg_buf, 2, dat, length);
//	CSA37F71_TRACE(1, "read_double_reg reg:0x%x",regAddress);
//	DUMP8("0x%x, ",dat,length);
    return ret;
}

/**
  * @brief  wakeup iic
  * @param  None
  * @retval 0:success, -1: fail
  */
static char cs_press_wakeup_iic(void)
{
    char ret;
//    unsigned char read_data;
	
    ret = cs_press_iic_rw_test(0x67);
	
    return ret;
}

/**
  * @brief  clean debug mode reg, debug ready reg
  * @param  None
  * @retval 0:success, -1: fail
  */
static char cs_press_clean_debugmode(void)
{
    char ret = 0;
    unsigned char temp_data = 0;
    
    ret |= cs_press_iic_write(DEBUG_MODE_REG, &temp_data, 1);
    ret |= cs_press_iic_write(DEBUG_READY_REG, &temp_data, 1);
    
    return ret;
}

/**
  * @brief  set debug mode reg
  * @param  mode_num: debug mode num data
  * @retval 0:success, -1: fail
  */
static char cs_press_set_debugmode(unsigned char mode_num)
{
    char ret;

    ret = cs_press_iic_write(DEBUG_MODE_REG, &mode_num, 1);
    
    return ret;
}

/**
  * @brief  set debug ready reg
  * @param  ready_num: debug ready num data
  * @retval 0:success, -1: fail
  */
static char cs_press_set_debugready(unsigned char ready_num)
{
    char ret;

    ret = cs_press_iic_write(DEBUG_READY_REG, &ready_num, 1);
    
    return ret;
}

/**
  * @brief  get debug ready reg data
  * @param  None
  * @retval ready reg data
  */
static unsigned char cs_press_get_debugready(void)
{
    char ret;
    unsigned char ready_num=0;
    
    ret = cs_press_iic_read(DEBUG_READY_REG, &ready_num, 1);
    
    if(ret!=0)
    {
        ready_num = 0;
    }
    
    return ready_num;
}

/**
  * @brief  write debug data
  * @param  *debugdata: point to data buffer, length: write data length
  * @retval 0:success, -1: fail
  */
static char cs_press_write_debugdata(unsigned char *debugdata, unsigned char length)
{
    char ret;

    ret = cs_press_iic_write(DEBUG_DATA_REG, debugdata, length);
    
    return ret;
}

/**
  * @brief  read debug data
  * @param  *debugdata: point to data buffer, length: write data length
  * @retval 0:success, -1: fail
  */
static char cs_press_read_debugdata(unsigned char *debugdata, unsigned char length)
{
    char ret;
    
    ret = cs_press_iic_read(DEBUG_DATA_REG, debugdata, length);
    
    return ret;
}


/**
  * @brief  reset ic
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_reset_ic(void)
{
    char ret = 0;

#if RSTPIN_RESET_ENABLE 		// rst pin resto
    
	cs_press_rstpin_high();
    cs_press_delay_ms(100);
    cs_press_rstpin_low();	
    cs_press_delay_ms(300);		// skip boot jump time
#else    						// hw reset ic
    cs_press_power_down();
    cs_press_delay_ms(100);
    cs_press_power_up();
#endif
    
    return ret;
}

/**
  * @brief  write and read iic test reg
  * @param  test_data: test data
  * @retval 0:success, -1: fail
  */
char cs_press_iic_rw_test(unsigned char test_data)
{
    char ret;
    unsigned char retry = RETRY_NUM;
    unsigned char read_data = 0;
    
    do
    {
        cs_press_iic_write(AP_RW_TEST_REG, &test_data, 1);
        cs_press_iic_read(AP_RW_TEST_REG, &read_data, 1);
        
        ret = 0;
        
        if(read_data != test_data)
        {
            ret = (char)-1;
            cs_press_delay_ms(1);
        }

    }while((ret!=0) && (retry--));
    
    return ret;
}

/**
  * @brief  wake up device
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_set_device_wakeup(void)
{
    char ret = 0;
    unsigned char retry = RETRY_NUM;
    unsigned char temp_data = 1;
    
    do
    {
        if(ret!=0)
        {
            cs_press_delay_ms(1);
        }
        ret = cs_press_iic_write(AP_WAKEUP_REG, &temp_data, 1);
        
    }while((ret!=0)&&(retry--));
    
    return ret;
}

/**
  * @brief  put the device to sleep
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_set_device_sleep(void)
{
    char ret = 0;
    unsigned char retry = RETRY_NUM;
    unsigned char temp_data = 1;
    
    do
    {
        if(ret!=0)
        {
            cs_press_delay_ms(1);
        }
        ret = cs_press_iic_write(AP_SLEEP_REG, &temp_data, 1);
        
    }while((ret!=0)&&(retry--));
    
    return ret;    
}

/**
  * @brief  init read rawdata 
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_read_rawdata_init(void)
{
    char ret;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    
    ret = cs_press_set_debugmode(AP_R_RAWDATA_DEBUG_MODE);
    
    return ret;
}

/**
  * @brief  read rawdata 
  * @param  *rawdata: point to sensor data strcut
  * @retval 0:none, -1: fail, >0:vaild data num
  */
char cs_press_read_rawdata(CS_RAWDATA_Def *rawdata)
{
    char ret;
    unsigned char i;
    unsigned char data_temp[AFE_MAX_CH*2];
    unsigned char byte_num;

    ret = (char)-1;
    
    byte_num = cs_press_get_debugready();
    
    if(byte_num <= (AFE_MAX_CH*2))
    {
        if((byte_num%2) == 0)
        {
            ret = cs_press_read_debugdata(data_temp, byte_num);    
            if(ret == 0)
            {
                for(i=0;i<(byte_num/2);i++)
                {
                    rawdata->rawdata[i] = ((((unsigned short)data_temp[i*2+1]<<8)&0xff00)|data_temp[i*2]);
                }
                ret = byte_num/2;
            }
        }
        else
        {
            ret = (char)-1;
        }
    }
    cs_press_set_debugready(0);
    
    return ret;
}

/**
  * @brief  init read processed data 
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_read_processed_data_init(void)
{
    char ret;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    
    ret = cs_press_set_debugmode(AP_R_PROCESSED_DEBUG_MODE);
    
    return ret;
}

/**
  * @brief  read processeddata 
  * @param  *proce_data: point to processed data strcut
  * @retval 0:success, -1: fail
  */
char cs_press_read_processed_data(CS_PROCESSED_DATA_Def *proce_data)
{
    char ret;
    unsigned char i;
    unsigned char data_temp[(AFE_USE_CH*4*2)+2]; // 1 ch have 4 types of data
	short data_temp_s16[(AFE_USE_CH*4)+1];
    unsigned char byte_num;
	short checksum;
	
    ret = (char)-1;
    
    byte_num = cs_press_get_debugready();
    
    if(byte_num == ((AFE_USE_CH*4*2)+2))
    {			
		ret = cs_press_read_debugdata(data_temp, byte_num);    
		
		if(ret == 0)
		{
			for(i=0;i<(AFE_USE_CH*4)+1;i++)
			{
				data_temp_s16[i] = ((((unsigned short)data_temp[i*2+1]<<8)&0xff00)|data_temp[i*2]);
			}
			
			checksum = 0;
			
			for(i=0;i<(AFE_USE_CH*4);i++)
			{
				checksum += data_temp_s16[i];
			}
			
			if(checksum == data_temp_s16[(AFE_USE_CH*4)])	// check right
			{
				for(i=0;i<AFE_USE_CH;i++)
				{
					proce_data->arith_rawdata[i] = data_temp_s16[i];
					proce_data->baseline[i] = data_temp_s16[AFE_USE_CH+i];
                    proce_data->diffdata[i] = data_temp_s16[AFE_USE_CH*2+i];
                    proce_data->energy_data[i] = data_temp_s16[AFE_USE_CH*3+i];
				}

				ret = 0;
			}
			
		}
    }
    cs_press_set_debugready(0);
    
    return ret;
}

/**
  * @brief  read press level
  * @param  *press_level: point to press level buffer
  * @retval 0:success, -1: fail
  */
char cs_press_read_press_level(unsigned short *press_level)
{
    char ret = (char)-1;
    unsigned char data_temp[4]={0,0,0,0};
    unsigned char num;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    cs_press_set_debugmode(AP_R_PRESS_LEVEL_DEBUG_MODE);
    cs_press_write_debugdata(data_temp, 2);
	cs_press_set_debugready(2);
    
    cs_press_delay_ms(DEBUG_MODE_DELAY_TIME);
    
    num = cs_press_get_debugready();

    if(num == 4)
    {
        ret = cs_press_read_debugdata(data_temp, num);
        
        if(ret == 0)
        {
            *press_level = ((((unsigned short)data_temp[1]<<8)&0xff00)|data_temp[0]);
        }
    }
    
    return ret;
}

/**
  * @brief  write press level
  * @param  press_level: press level data
  * @retval 0:success, -1: fail
  */
char cs_press_write_press_level(unsigned short press_level)
{
    char ret;
    unsigned char data_temp[6]={0};
    unsigned short read_press_level = 0;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    cs_press_set_debugmode(AP_W_PRESS_LEVEL_DEBUG_MODE);
    
    data_temp[0] = 0;
	data_temp[1] = 0;
    data_temp[2] = press_level;
    data_temp[3] = press_level>>8;
    data_temp[4] = 0;
	data_temp[5] = 0;
    
    cs_press_write_debugdata(data_temp, 6);
    
    cs_press_set_debugready(6);

    cs_press_delay_ms(DEBUG_MODE_DELAY_TIME*3);
    
    cs_press_read_press_level(&read_press_level);
    
    ret = 0;    
    if(read_press_level != press_level)
    {
        ret = (char)-1;
    }

    return ret;
}

/**
  * @brief  read calibration factor
  * @param  *cal_factor: point to calibration factor
  * @retval 0:success, -1: fail
  */
char cs_press_read_calibration_factor(unsigned char ch, unsigned short *cal_factor)
{
    char ret = (char)-1;
    unsigned char data_temp[4]={0,0,0,0};
    unsigned char num;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    cs_press_set_debugmode(AP_R_CAL_FACTOR_DEBUG_MODE);
	
	data_temp[0] = ch;
	cs_press_write_debugdata(data_temp, 2);
    
	cs_press_set_debugready(2);
    
    cs_press_delay_ms(DEBUG_MODE_DELAY_TIME);
    
    num = cs_press_get_debugready();

    if(num == 4)
    {
        ret = cs_press_read_debugdata(data_temp, 4);
        
        if(ret == 0)
        {
            *cal_factor = ((((unsigned short)data_temp[1]<<8)&0xff00)|data_temp[0]);
        }
    }
    
    return ret;
}

/**
  * @brief  write calibration factor
  * @param  cal_factor: calibration factor data
  * @retval 0:success, -1: fail
  */
char cs_press_write_calibration_factor(unsigned char ch, unsigned short cal_factor)
{
    char ret;
    unsigned char data_temp[7];
    unsigned short read_cal_factor;
    
    cs_press_wakeup_iic();
    
    cs_press_clean_debugmode();
    cs_press_set_debugmode(AP_W_CAL_FACTOR_DEBUG_MODE);
    
    data_temp[0] = ch;
    data_temp[1] = 0;
    data_temp[2] = cal_factor;
    data_temp[3] = cal_factor>>8;
    data_temp[4] = 0;
    data_temp[5] = 0;
    
    cs_press_write_debugdata(data_temp, 6);
    
    cs_press_set_debugready(6);

    cs_press_delay_ms(DEBUG_MODE_DELAY_TIME*3);
    
    cs_press_read_calibration_factor(ch, &read_cal_factor);
        
    ret = 0;
    if(read_cal_factor != cal_factor)
    {
        ret = (char)-1;
    }
    
    return ret;
}

/**
  * @brief  enable calibration function
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_calibration_enable(void)
{
    char ret = 0;
    unsigned char temp_data = 1 ;
    
    cs_press_wakeup_iic();
    
    cs_press_delay_ms(1);

    ret = cs_press_iic_write(AP_CALIBRATION_REG, &temp_data, 1);
	
    return ret;
}

/**
  * @brief  disable calibration function
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_calibration_disable(void)
{
    char ret = 0;
    unsigned char temp_data = 0;
    
    cs_press_wakeup_iic();
    
    cs_press_delay_ms(1);

    ret = cs_press_iic_write(AP_CALIBRATION_REG, &temp_data, 1);
    
    return ret;
	
}

/**
  * @brief  check calibration result
  * @param  None
  * @retval 0:running, -1: error, 1: success, 2: fail, 3: overtime
  */
char cs_press_calibration_check(CS_CALIBRATION_RESULT_Def *calibration_result)
{
    char ret;
    unsigned char num;
    unsigned char data_temp[10];
//	unsigned char retry = RETRY_NUM;
	
	ret = cs_press_set_debugmode(AP_CALIBRATION_DEBUG_MODE);

	num = cs_press_get_debugready();
	
	if(num == 10)
	{
		ret = cs_press_read_debugdata(data_temp, 10);    
		
		if(ret == 0)
		{
            calibration_result->calibration_channel = data_temp[0];
			calibration_result->calibration_progress = data_temp[1];
			calibration_result->calibration_factor = ((((unsigned short)data_temp[3]<<8)&0xff00)|data_temp[2]);
			calibration_result->press_adc_1st = ((((short)data_temp[5]<<8)&0xff00)|data_temp[4]);
			calibration_result->press_adc_2nd = ((((short)data_temp[7]<<8)&0xff00)|data_temp[6]);
			calibration_result->press_adc_3rd = ((((short)data_temp[9]<<8)&0xff00)|data_temp[8]);
		
			if(calibration_result->calibration_progress >= CALIBRATION_SUCCESS_FALG)
			{
				ret = 1;
			}
            if(calibration_result->calibration_progress == CALIBRATION_FAIL_FALG)
            {
                ret = 2;
            }
            if(calibration_result->calibration_progress == CALIBRATION_OVERTIME_FALG)
            {
                ret = 3;
            }
		}	
	}		
	
	return ret;
}

/**
  * @brief  init read noise data 
  * @param  period_num: period num of calculation the noise
  * @retval 0:success, -1: fail
  */
char cs_press_read_noise_init(unsigned short period_num)
{
    char ret = 0;
    unsigned char data_temp[2];
    
    cs_press_wakeup_iic();
    
    ret |= cs_press_clean_debugmode();
    
    ret |= cs_press_set_debugmode(AP_R_NOISE_DEBUG_MODE);
    
    data_temp[0] = period_num;
    data_temp[1] = period_num>>8;
    ret |= cs_press_write_debugdata(data_temp, 2);
    
    ret |= cs_press_set_debugready(2);
    
    return ret;    
}

/**
  * @brief  read noise data 
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_read_noise(CS_NOISE_DATA_Def *noise_data)
{
    char ret;
	unsigned char i;
    unsigned char num;
    unsigned char data_temp[AFE_USE_CH*6];
//    unsigned char checksum;
    unsigned int dat_temp;
    
    ret = (char)-1;
    
    cs_press_wakeup_iic();
    
    num = cs_press_get_debugready();
   
    if(num == AFE_USE_CH*6)
    {
        cs_press_read_debugdata(data_temp, num); 
        
        dat_temp = 0;
		for(i=0;i<AFE_USE_CH;i++)
		{
			noise_data->noise_peak[i] = ((((short)data_temp[1+i*6]<<8)&0xff00)|data_temp[0+i*6]);
			
            dat_temp = data_temp[5+i*6];
            dat_temp <<= 8;
            dat_temp |= data_temp[4+i*6];
            dat_temp <<= 8;
            dat_temp |= data_temp[3+i*6];
            dat_temp <<= 8;
            dat_temp |= data_temp[2+i*6];          
            
            noise_data->noise_std[i] = dat_temp;
		}
		
		ret = 0;
    }

    return ret;
}

/**
  * @brief  init read offset data 
  * @param  None
  * @retval 0:success, -1: fail
  */
static char cs_press_read_offset_init(void)
{
    char ret = 0;
    
    cs_press_wakeup_iic();
    
    ret |= cs_press_clean_debugmode();
    
    ret |= cs_press_set_debugmode(AP_R_OFFSET_DEBUG_MODE);

    ret |= cs_press_set_debugready(0);
    
    return ret;    
}

/**
  * @brief  read noise data 
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_read_offset(CS_OFFSET_DATA_Def *offset_data)
{
    char ret;
	unsigned char i;
    unsigned char num;
    unsigned char data_temp[AFE_USE_CH*4+2]={0};
//    unsigned char checksum;
	CS_AFE_INFO_Def afe_info;
	
    ret = (char)-1;
    
    cs_press_read_offset_init();
    
	cs_press_delay_ms(15);					// waitting for data ready
	
    num = cs_press_get_debugready();
   
    if(num == (AFE_USE_CH*4+2))
    {
        cs_press_read_debugdata(data_temp, num); 
        
		afe_info.pga = data_temp[0];
		afe_info.ch_num = data_temp[1];
		
		for(i=0;i<AFE_USE_CH;i++)
		{
			afe_info.dac[i] = ((((short)data_temp[3+i*4]<<8)&0xff00)|data_temp[2+i*4]);
			afe_info.adc[i] = ((((short)data_temp[5+i*4]<<8)&0xff00)|data_temp[4+i*4]);
		}
		
		// calculate offset
		for(i=0;i<AFE_USE_CH;i++)
		{
			offset_data->offset[i] = (short)((afe_info.dac[i]*DAC_RANGE_MAX/2048) + afe_info.adc[i]*ADCtoMV);
		}
		
		ret = 0;
    }

    return ret;
}

/**
  * @brief  read pressure data
  * @param  *pressure: point to pressure buffer
  * @retval 0: none, -1: fail, ¡Ý0: success£¬key num
  */
char cs_press_read_pressure(unsigned short *pressure)
{
    char ret;
    unsigned char read_temp[4]={0};
    unsigned char checksum;
    unsigned char key_num;
    
    ret = cs_press_iic_read(AP_FORCEDATA_REG, read_temp, 1); 
    
    if(ret == 0)
    {
        key_num = read_temp[0];
        
        if(key_num <= KEY_NUM)
        {
            ret = cs_press_iic_read(AP_FORCEDATA_REG+key_num, read_temp, 3); 
            
            if(ret == 0)
            {
                checksum = read_temp[0] + read_temp[1];
                
                if(checksum == read_temp[2])
                {
                    *pressure = ((((unsigned short)read_temp[2]<<8)&0xff00)|read_temp[1]);
                    ret = 0;                
                }        
            }
            
            ret = (char)-1;
        }
    }
    
    return ret;
}

/**
  * @brief  read fw info
  * @param  *fw_info: point to info struct
  * @retval 0:success, -1: fail
  */
char cs_press_read_fw_info(CS_FW_INFO_Def *fw_info)
{
    char ret = 0;
    unsigned char read_temp[FW_ONE_BLOCK_LENGTH_R];
    
    cs_press_wakeup_iic();
    
    // read fw info
    
    ret |= cs_press_iic_read(AP_MANUFACTURER_ID_REG, read_temp, CS_MANUFACTURER_ID_LENGTH);  // Manufacturer id
    if(ret==0)
    {
        fw_info->manufacturer_id = ((unsigned short)read_temp[1]<<8) | read_temp[0];
    }    
    
    ret |= cs_press_iic_read(AP_MODULE_ID_REG, read_temp, CS_MODULE_ID_LENGTH);  // Module id
    if(ret==0)
    {
        fw_info->module_id = ((unsigned short)read_temp[1]<<8) | read_temp[0];
    }     
   
    ret |= cs_press_iic_read(AP_VERSION_REG, read_temp, CS_FW_VERSION_LENGTH);  // FW Version
    if(ret==0)
    {
        fw_info->fw_version = ((unsigned short)read_temp[1]<<8) | read_temp[0];
    }  
    
    return ret;
}

/**
  * @brief  forced firmware update
  * @param  *fw_array: point to fw hex array
  * @retval 0:success, -1: fail
  */
char cs_press_fw_force_update(const unsigned char *fw_array)
{
    unsigned int i, j;
    char ret;
    unsigned int fw_code_length;
    unsigned int fw_block_num_w, fw_block_num_r;
    const unsigned char *fw_code_start;
    unsigned int fw_count;
    unsigned char fw_read_code[FW_ONE_BLOCK_LENGTH_R];
    unsigned short fw_default_version;
    unsigned short fw_read_version;
    
    // fw init
    fw_code_length = ((((unsigned short)fw_array[FW_ADDR_CODE_LENGTH+0]<<8)&0xff00)|fw_array[FW_ADDR_CODE_LENGTH+1]); 
    fw_code_start = &fw_array[FW_ADDR_CODE_START];
    fw_block_num_w = fw_code_length/FW_ONE_BLOCK_LENGTH_W;
	fw_block_num_r = fw_code_length/FW_ONE_BLOCK_LENGTH_R;
    fw_default_version = ((((unsigned short)fw_array[FW_ADDR_VERSION+0]<<8)&0xff00)|fw_array[FW_ADDR_VERSION+1]);
    
    
#if RSTPIN_RESET_ENABLE 		// rst pin resto
	
	cs_press_rstpin_high();
    cs_press_delay_ms(50);
    cs_press_rstpin_low();	
	cs_press_delay_ms(80);
	
#else    						// hw reset ic
    cs_press_power_down();
    cs_press_delay_ms(100);
    cs_press_power_up();
	cs_press_delay_ms(80);
#endif

    // send fw write cmd
    cs_press_iic_write_double_reg(BOOT_CMD_REG ,boot_fw_write_cmd, BOOT_CMD_LENGTH);
    cs_press_delay_ms(2000);	// waiting flash erase
    
    // send fw code 
    fw_count = 0;
    
    for(i=0;i<fw_block_num_w;i++)
    {
        ret = cs_press_iic_write_double_reg(i*FW_ONE_BLOCK_LENGTH_W, fw_code_start+fw_count, FW_ONE_BLOCK_LENGTH_W);
        
        fw_count += FW_ONE_BLOCK_LENGTH_W;
        
        if(ret != 0)    
        {
			CSA37F71_TRACE(2,"%s line:%d",__FUNCTION__,__LINE__);
            goto FLAG_FW_FAIL;
        }
        
        cs_press_delay_ms(20);
    }
    
    // read & check fw code
    fw_count = 0;
    
    for(i=0;i<fw_block_num_r;i++)
    {
        // read code data
        ret = cs_press_iic_read_double_reg(i*FW_ONE_BLOCK_LENGTH_R, fw_read_code, FW_ONE_BLOCK_LENGTH_R);
        
        if(ret != 0)    
        {
			CSA37F71_TRACE(2,"%s line:%d",__FUNCTION__,__LINE__);
            goto FLAG_FW_FAIL;
        }
        
        // check code data
        for(j=0;j<FW_ONE_BLOCK_LENGTH_R;j++)
        {
            if(fw_read_code[j] != fw_code_start[fw_count+j])
            {
				CSA37F71_TRACE(2,"%s line:%d",__FUNCTION__,__LINE__);
                goto FLAG_FW_FAIL;
            }
        }
        
        fw_count += FW_ONE_BLOCK_LENGTH_R;
        
        cs_press_delay_ms(20);
    }
    
    // send fw flag cmd
    cs_press_iic_write_double_reg(BOOT_CMD_REG ,boot_fw_wflag_cmd, BOOT_CMD_LENGTH);
    cs_press_delay_ms(50); 
    
    cs_press_reset_ic();
    
    // check fw version
    cs_press_delay_ms(300); // skip boot
    
    ret = cs_press_iic_read(AP_VERSION_REG, fw_read_code, CS_FW_VERSION_LENGTH);
    
    fw_read_version = 0;
    if(ret==0)
    {
        fw_read_version = ((((unsigned short)fw_read_code[1]<<8)&0xff00)|fw_read_code[0]);            
    }

    if(fw_read_version != fw_default_version)
    {
		CSA37F71_TRACE(2,"fw_read_version:0x%x fw_default_version:0x%x",fw_read_version,fw_default_version);
		CSA37F71_TRACE(2,"%s line:%d",__FUNCTION__,__LINE__);
        goto FLAG_FW_FAIL;
    }        
    
    return 0;
    
FLAG_FW_FAIL:
    
    return (char)-1;
}

/**
  * @brief  firmware high version update
  * @param  *fw_array: point to fw hex array
  * @retval 0:success, -1: fail, 1: no need update
  */
char cs_press_fw_high_version_update(const unsigned char *fw_array)
{
    char ret;
    unsigned char read_temp[FW_ONE_BLOCK_LENGTH_R];
    unsigned short read_version = 0;
    unsigned short default_version = 0;
    char flag_update = 0;   // 0: no need update fw, 1: need update fw
    unsigned char retry;
    
    //cs_press_delay_ms(300); // skip boot jump time
    
    // read ap version
    ret = cs_press_iic_read(AP_VERSION_REG, read_temp, CS_FW_VERSION_LENGTH);
	CSA37F71_TRACE(1,"version:0x%x ret:%d",(read_temp[0]|(read_temp[1]<<8)),ret);
    if(ret == 0)
    {
        // get driver ap version 
        default_version = ((((unsigned short)fw_array[FW_ADDR_VERSION+0]<<8)&0xff00)|fw_array[FW_ADDR_VERSION+1]);
    
        // get ic ap version
        read_version = ((((unsigned short)read_temp[1]<<8)&0xff00)|read_temp[0]);
        
        // compare
        if(read_version < default_version)
        {
            flag_update = 1;
        }
    }
    else
    {
        flag_update = 1;
    }
	CSA37F71_TRACE(2,"read_version:0x%x default_version:0x%d",read_version,default_version);
	CSA37F71_TRACE(1,"flag_update:0x%x",flag_update);
    if(flag_update == 0)
    {
        return 1;   // no need update
    }
    
    // update fw
    retry = RETRY_NUM;
    
    do
    {
        ret = cs_press_fw_force_update(fw_array);
    }while((ret!=0)&&(retry--));

    return ret;
}


/**
  * @brief  m6x init
  * @param  None
  * @retval 0:success, -1: fail
  */
char cs_press_init(void)
{
    char ret;
    
    // reset ic
    cs_press_reset_ic();

	ret = cs_press_iic_rw_test(0xaa);
	if(0 != ret)
	{
		CSA37F71_TRACE(2,"%s line:%d ret:%d start cs_press_fw_force_update",__FUNCTION__,__LINE__,ret);
		cs_press_fw_force_update(cs_default_fw_array);
//		return ret;
	}
	
    // check whether need to update ap fw
     ret = cs_press_fw_high_version_update(cs_default_fw_array);
	CSA37F71_TRACE(2,"%s ret:%d",__FUNCTION__,ret);
    if(ret > (char)-1)
    {
        return 0;
    }
    
    return ret;
}


