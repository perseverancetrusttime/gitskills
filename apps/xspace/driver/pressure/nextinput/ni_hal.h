/** @file ni_hal.h
 *  @brief Function prototypes for the NextInput driver.
 *
 *  The empty function definitions have to be filled by user in order to work
 *  on different embedded platform.  Same goes the platform dependent definitions
 *  and arrays.
 *
 *  @author Jerry Hung
 *  @bug You tell me
 */

#ifndef _NI_HAL_H_
#define _NI_HAL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "ni.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "xspace_i2c_master.h"
#include "nextinput_adapter.h"

/*Include platform specific I2C/TWI header files*/

//#define AB1562_PLATFORM  0//relative I2C and delay functions
//#define NRF51_PLATFORM   1

#if AB1562_PLATFORM

	#include "hal_platform.h"
	#include "hal_i2c_master.h"
	
#elif NRF51_PLATFORM

	#include "boards.h"
	#include "app_util_platform.h"
	#include "app_error.h"
	#include "nrf_drv_twi.h"
	#include "nrf_delay.h"

	/* TWI instance ID. */
	#define TWI_INSTANCE_ID     0

	/* Indicates if operation on TWI has ended. */
	static volatile bool m_xfer_done = false;

	/* TWI instance. */
	static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);


	/**
	 * @brief TWI events handler.
	 */
	void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
	{
		switch (p_event->type)
		{
			case NRF_DRV_TWI_EVT_DONE:
				m_xfer_done = true;
				break;
			default:
				break;
		}
	}

#endif

#define REG_INIT 12

#ifdef REG_INIT
static u8 reg_init[REG_INIT][2] = {
	{0x00,0xB5},	//AFE enable, ADC mode
	{0x01,0x50},	//0x30 31x 0x40 61x 0x50 115xINAGAIN, Precharge 50us
	{0x06,0x85},	//Enable autopreldadj, FALLTHRSEL=0.75 x INTRTHR
	{0x07,0x06},
	{0x08,0x00},	//AUTOCAL threshold=32 lsb
	{0x09,0xB3},
	{0x0A,0x50},
	{0x0B,0x05},
	{0x0C,0x05},
	{0x0D,0x00},	//INTRTHR =80 lsb;
	{0x0E,0x13},	//BTN mode and 3 Interrupt samples
	{0x0F,0x01},	//Force baseline
};
#endif

//Define the number of clients on the platform
#define NI_SENSOR_NUM 1

#if AB1562_PLATFORM
#define DF100_I2C_PORT HAL_I2C_MASTER_0
static hal_i2c_config_t df100_i2c_config;
#endif
static struct ni_sensor_t sensor_list[NI_SENSOR_NUM] = {
	{0,0x4A} //ADR pin GND
//	{0,0x49}//for EVK test
};

//Set REG_INIT to the number of register values need to be changed during
//initialization
u8 ni_hal_i2c_init();
u8 ni_hal_i2c_read( struct ni_sensor_t s, u8 reg, u8 length, u8 *data);
u8 ni_hal_i2c_write(struct ni_sensor_t s, u8 reg, u8 length, u8 *data);


u8 ni_hal_i2c_init()
{
#if AB1562_PLATFORM
	df100_i2c_config.frequency = HAL_I2C_FREQUENCY_400K;
	return hal_i2c_master_init(DF100_I2C_PORT, &df100_i2c_config);
#elif NRF51_PLATFORM
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_df100_config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_df100_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
    return 0;
#else
    return NI_OK;
#endif
}

u8 ni_hal_i2c_read( struct ni_sensor_t s, u8 reg, u8 length, u8 *data)
{
	u8 error = NI_OK;
    /*Insert platform specific I2C read method here*/

#if AB1562_PLATFORM

	hal_i2c_send_to_receive_config_t i2c_read;
	u8 senddata[1];

	senddata[0]=reg;
	i2c_read.send_data =senddata ;
	i2c_read.send_length=1;
	i2c_read.receive_buffer=data;
	i2c_read.receive_length=length;
	i2c_read.slave_address = s.addr;	
	error = hal_i2c_master_send_to_receive_polling(DF100_I2C_PORT,&i2c_read);
#elif NRF51_PLATFORM
    ret_code_t err_code;
    
    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, s.addr, &reg, 1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    m_xfer_done = false;
    /* Read 1 byte from the specified address - skip 3 bits dedicated for fractional part of temperature. */
    err_code = nrf_drv_twi_rx(&m_twi, s.addr,data, length);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
#else
	error = !xspace_i2c_burst_read_01(NEXTINPUT_I2C_TYPE, NEXTINPUT_I2C_ADDR, &reg, 1, data, length);
#endif

	return error;
}


u8 ni_hal_i2c_write(struct ni_sensor_t s, u8 reg, u8 length, u8 *data)
{
	u8 error = NI_OK;
    /*Insert platform specific I2C write method here*/
#if AB1562_PLATFORM
	u8 i=0;
	u8 buffer[length+1];
	buffer[0]=reg;
	for(i=0; i<length; i++) {
		buffer[i+1] = data[i];
	}
	error = hal_i2c_master_send_polling(DF100_I2C_PORT, s.addr, buffer, length+1);
#elif NRF51_PLATFORM	
    ret_code_t err_code;    
    u8 i=0;
    u8 buffer[length+1];
    for(i=0; i< length;i++) {
        buffer[i+1] = data[i];
    }
    buffer[0]= reg;
    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, s.addr, buffer, length+1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
#else
	u8 buffer[length+1];
	buffer[0]=reg;
	for(u8 i=0; i<length; i++) {
		buffer[i+1] = data[i];
	}
	error = !xspace_i2c_burst_write_01(NEXTINPUT_I2C_TYPE, NEXTINPUT_I2C_ADDR, buffer, length+1);
#endif
    return error;

}

void ni_hal_delay(u16 delay_ms)
{	
//this delay value must check with platform unit :ms
#if  AB1562_PLATFORM	
	hal_gpt_delay_ms(delay_ms);//this delay function is used in AB1562 platform
#elif NRF51_PLATFORM
    nrf_delay_ms(50);
#else
 	osDelay(delay_ms);
#endif
}
static u16 ni_calibration_data = 0x00;
static u16 ni_cal_flag = 0xff;

u8 ni_hal_write_calbiraiton_data(u8 num_sensor, u16 *calibration_data)
{
	u8 error = NI_OK;
	
	// write calibration data to storage
	ni_calibration_data = (u16)*calibration_data;
	// write cal_flag to storage
	ni_cal_flag = 0x00;
	return error;
}

u8 ni_hal_read_calbiraiton_data(u8 num_sensor, u16 *calibration_data)
{
	u8 error = NI_OK;
	u8 cal_flag = 0xff;
	
	// read calibration flag.
	ni_cal_flag = ni_cal_flag;
	if (cal_flag == 0) {
		
		//read out calibration data from storage
		calibration_data = &ni_calibration_data;
	}
	else if (cal_flag == 1) {
		error = NI_ERROR_CAL_FAILED;
	}
	else{
		
		error = NI_ERROR_NOTCAL;
	}
	
	return error;
}





#ifdef __cplusplus
}
#endif

#endif
