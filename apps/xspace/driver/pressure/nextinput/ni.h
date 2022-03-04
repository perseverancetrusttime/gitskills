/** @file ni.h
 *  @brief Function prototypes for the NextInput driver.
 *
 *  This contains the prototypes for the NextInput driver and eventually
 *  any macros, constants, or global variables you will need.
 *
 *  @author Jerry Hung
 *  @bug You tell me
 */

#ifndef _NI_H_
#define _NI_H_

#ifdef __cplusplus
extern "C" {
#endif



// Registers
#define TOEN_REG                    0
#define TOEN_POS                    7
#define TOEN_MSK                    0x80

#define WAIT_REG                    0
#define WAIT_POS                    4
#define WAIT_MSK                    0x70

#define ADCRAW_REG                  0
#define ADCRAW_POS                  3
#define ADCRAW_MSK                  0x08

// #define TEMPWAIT_REG                0
// #define TEMPWAIT_POS                1
// #define TEMPWAIT_MSK                0x06

#define EN_REG                      0
#define EN_POS                      0
#define EN_MSK                      0x01

#define INAGAIN_REG                 1
#define INAGAIN_POS                 4
#define INAGAIN_MSK                 0x70

#define PRECHARGE_REG               1
#define PRECHARGE_POS               0
#define PRECHARGE_MSK               0x07

#define OTPBUSY_REG                 2
#define OTPBUSY_POS                 7
#define OTPBUSY_MSK                 0x80

#define OVRINTRTH_REG               2
#define OVRINTRTH_POS               3
#define OVRINTRTH_MSK               0x08

#define OVRACALTH_REG               2
#define OVRACALTH_POS               2
#define OVRACALTH_MSK               0x04

// #define SNSERR_REG                  2
// #define SNSERR_POS                  1
// #define SNSERR_MSK                  0x02

#define INTR_REG                    2
#define INTR_POS                    0
#define INTR_MSK                    0x01

#define ADCOUT_REG                  3
#define ADCOUT_SHIFT                4

#define SCOUNT_REG                  4
#define SCOUNT_POS                  0
#define SCOUNT_MSK                  0x0f

#define TEMP_REG                    5
#define TEMP_SHIFT                  4

#define AUTOPRELDADJ_REG            6
#define AUTOPRELDADJ_POS            7
#define AUTOPRELDADJ_MSK            0x80

#define FALLTHRSEL_REG              6
#define FALLTHRSEL_POS              0
#define FALLTHRSEL_MSK              0x07

#define AUTOCAL_THR_REG             7
#define AUTOCAL_THR_SHIFT           4

// #define ENNEGZONE_REG               8
// #define ENNEGZONE_POS               0
// #define ENNEGZONE_MSK               0x01

#define ENCALMODE_REG               9
#define ENCALMODE_POS               7
#define ENCALMODE_MSK               0x80

#define CALRESET_REG                9
#define CALRESET_POS                4
#define CALRESET_MSK                0x70

#define INTRPOL_REG                 9
#define INTRPOL_POS                 3
#define INTRPOL_MSK                 0x08

#define CALPERIOD_REG               9
#define CALPERIOD_POS               0
#define CALPERIOD_MSK               0x07

#define RISEBLWGT_REG               10
#define RISEBLWGT_POS               4
#define RISEBLWGT_MSK               0x70

#define LIFTDELAY_REG               10
#define LIFTDELAY_POS               0
#define LIFTDELAY_MSK               0x07

#define PRELDADJ_REG                11
#define PRELDADJ_POS                3
#define PRELDADJ_MSK                0xf8

#define FALLBLWGT_REG               11
#define FALLBLWGT_POS               0
#define FALLBLWGT_MSK               0x07

#define INTRTHRSLD_REG              12
#define INTRTHRSLD_SHIFT            4

#define INTREN_REG                  14
#define INTREN_POS                  7
#define INTREN_MSK                  0x80

#define INTRMODE_REG                14
#define INTRMODE_POS                6
#define INTRMODE_MSK                0x40

#define INTRPERSIST_REG             14
#define INTRPERSIST_POS             5
#define INTRPERSIST_MSK             0x20

#define BTNMODE_REG                 14
#define BTNMODE_POS                 4
#define BTNMODE_MSK                 0x10

#define INTRSAMPLES_REG             14
#define INTRSAMPLES_POS             0
#define INTRSAMPLES_MSK             0x07

// #define PWRMODE_REG                 15
// #define PWRMODE_POS                 4
// #define PWRMODE_MSK                 0xf0

#define FORCEBL_REG                 15
#define FORCEBL_POS                 0
#define FORCEBL_MSK                 0x01

// #define ADCMAX_REG                  16
// #define ADCMAX_SHIFT                4

#define BASELINE_REG                18
#define BASELINE_SHIFT              4

// #define ADCLIFO_REG                 20
// #define ADCLIFO_SHIFT               4

// #define LCOUNT_REG                  21
// #define LCOUNT_POS                  0
// #define LCOUNT_MSK                  0x0f

#define FALLTHR_REG                 22
#define FALLTHR_SHIFT               4

#define DEVID_REG                   0x80
#define DEVID_POS                   3
#define DEVID_MSK                   0xf8

#define REV_REG                     0x80
#define REV_POS                     0
#define REV_MSK                     0x07

// #define TEMPSNSBL_REG               0xB3
// #define TEMPSNSBL_POS               0
// #define TEMPSNSBL_MSK               0xff

#define FIRST_REG                   TOEN_REG
#define LAST_REG                    (FALLTHR_REG + 1)

// Wait
#define WAIT_0MS                    0
#define WAIT_1MS                    1
#define WAIT_4MS                    2
#define WAIT_8MS                    3
#define WAIT_16MS                   4
#define WAIT_32MS                   5
#define WAIT_64MS                   6
#define WAIT_256MS                  7

// Adcraw
#define ADCRAW_BL                   0
#define ADCRAW_RAW                  1

// Temperature
#define TEMPWAIT_DISABLED           0
#define TEMPWAIT_32                 1
#define TEMPWAIT_64                 2
#define TEMPWAIT_128                3

#define TEMP_READ_TIME_MS           1

// Inagain
#define INAGAIN_1X                  0
#define INAGAIN_8X                  1
#define INAGAIN_16X                 2
#define INAGAIN_31X                 3
#define INAGAIN_61X                 4
#define INAGAIN_115X                5
#define INAGAIN_208X                6
#define INAGAIN_416X                7

// Precharge
#define PRECHARGE_50US              0
#define PRECHARGE_100US             1
#define PRECHARGE_200US             2
#define PRECHARGE_400US             3
#define PRECHARGE_800US             4
#define PRECHARGE_1600US            5
#define PRECHARGE_3200US            6
#define PRECHARGE_6400US            7

// Calreset
#define CALRESET_500MS              0
#define CALRESET_1000MS             1
#define CALRESET_2000MS             2
#define CALRESET_4000MS             3
#define CALRESET_8000MS             4
#define CALRESET_16000MS            5
#define CALRESET_32000MS            6
#define CALRESET_DISABLED           7

#define CALRESET_TIMER_STOPPED      0
#define CALRESET_TIMER_STARTED      1
#define CALRESET_TIMER_FINISHED     2

// Calperiod
#define CALPERIOD_25MS              0
#define CALPERIOD_50MS              1
#define CALPERIOD_100MS             2
#define CALPERIOD_200MS             3
#define CALPERIOD_400MS             4
#define CALPERIOD_800MS             5
#define CALPERIOD_1600MS            6
#define CALPERIOD_DISABLED          7

#define CALPERIOD_TIMER_STOPPED     0
#define CALPERIOD_TIMER_STARTED     1
#define CALPERIOD_TIMER_FINISHED    2

// Riseblwgt
#define RISEBLWGT_0X                0
#define RISEBLWGT_1X                1
#define RISEBLWGT_3X                2
#define RISEBLWGT_7X                3
#define RISEBLWGT_15X               4
#define RISEBLWGT_31X               5
#define RISEBLWGT_63X               6
#define RISEBLWGT_127X              7

// Liftdelay
#define LIFTDELAY_DISABLED          0
#define LIFTDELAY_20MS              1
#define LIFTDELAY_40MS              2
#define LIFTDELAY_80MS              3
#define LIFTDELAY_160MS             4
#define LIFTDELAY_320MS             5
#define LIFTDELAY_640MS             6
#define LIFTDELAY_1280MS            7

#define LIFTDELAY_TIMER_DISABLED    0
#define LIFTDELAY_TIMER_ENABLED     1
#define LIFTDELAY_TIMER_RUNNING     2

// Fallblwgt
#define FALLBLWGT_0X                0
#define FALLBLWGT_1X                1
#define FALLBLWGT_3X                2
#define FALLBLWGT_7X                3
#define FALLBLWGT_15X               4
#define FALLBLWGT_31X               5
#define FALLBLWGT_63X               6
#define FALLBLWGT_127X              7

// Calmode
#define CALMODE_COUPLED             0
#define CALMODE_DECOUPLED           1

// Intrmode
#define INTRMODE_THRESH             0
#define INTRMODE_DRDY               1

// Intrpersist
#define INTRPERSIST_PULSE           0
#define INTRPERSIST_INF             1

#define INT_DURATION                100

// Intrsamples
#define INTRSAMPLES_1               1
#define INTRSAMPLES_2               2
#define INTRSAMPLES_3               3
#define INTRSAMPLES_4               4
#define INTRSAMPLES_5               5
#define INTRSAMPLES_6               6
#define INTRSAMPLES_7               7



// Error status code
enum NI_ERROR
{
    NI_OK			    	=  0,
    NI_ERROR_COM	    		,  // Communication error
    NI_ERROR_NON	    		,  // Target client doesn't exist (sensor_id > NUM_SENSOR)
    NI_ERROR_OVR	    		,  // Too many retry
    NI_ERROR_NOTCAL	    		,  // have not calibrate
    NI_ERROR_CAL_FAILED			,  // calibration failed 
    NI_ERROR_CAL_DATA_MIN		,  // calibration data too small
    NI_ERROR_LOAD_DATA			,  // load calibration data failed
    NI_ERROR_INIT_LOAD			,  // calibration init status is with load
    NI_ERROR_INIT_ADC			,  // calibration init adc sample error
    NI_ERROR_CAL_DATA_R 		,  // calibration data read error
    NI_ERROR_CAL_DATA_W			,  // calibration data write error
    NI_ERROR_STORER	    		   // calibration data storer error
};

#define MAX_RETRIES 3

#ifndef u8
typedef unsigned char u8;
#endif

#ifndef u16
typedef unsigned short u16;
#endif

#ifndef s16
typedef signed short s16;
#endif

#define NI_INTR_MIN_THRESHOLD 20 // unit:lsb
#define NI_CAL_START_THRESLD 20 //unit lsb, calibration threshold to trigger start calibration

#define average_sample_num 16


struct ni_sensor_t {
	void *i2c_bus; //Only has value when clients are in different buses,
				   //otherwise set to 0
	u8	addr;
};


/** @brief Initialize the AFE
 *
 *
 *  Initialize all AFE by reading register 0 of all AFE listed in sensor_list.
 *  If there is any initial register value defined in reg_init then write those
 *  values to the AFE.  Finally, set the ENABLE bit of all clients to 1
 *
 *
 *  @return Error status
 */
u8 ni_init(void);


/** @brief Read ADCOUT value of designated AFE
 *
 *
 *  Read the 12-bit ADC from the designated client(s).  The ADC value will be
 *  shifted into  *  proper 12-bit format for easy process.  For example, to
 *  read the ADC from client #0:
 *
 *  ni_read_ADC( 0, *temp );
 *
 *  Also, to read the ADC from all clients, set sensor_id to a number >= NUM_SENSOR:
 *
 *  If NUM_SENSOR = 4, following function call reads ADC from all 4 clients
 *  ni_read_ADC( 4, *temp );
 *
 *
 *  @param sensor_id Clients ID to read ADC from.  When sensor_id >= NUM_SENSOR then read from all clients
 *  @param ni_adc Data buffer that stores ADC value read from client(s)
 *  @return Error status
 */
u8 ni_read_ADC( u8 sensor_id, s16* ni_adc );


/** @brief Function wrapper for I2C read
 *
 *
 *  The read function wrapper is designed to work on various embedded platform.
 *  User should fill the platform specific I2C read operation in the function
 *  definition in ni_hal.h
 *
 *
 *  @param s Target client to read from
 *  @param reg Register address
 *  @param length Length of read operation
 *  @param data Data buffer that stores the read value
 *  @return Error status
 */
u8 ni_hal_i2c_read( struct ni_sensor_t s, u8 reg, u8 length, u8 *data );


/** @brief Function wrapper for I2C write
 *
 *
 *  The write function wrapper is designed to work on various embedded platform.
 *  User should fill the platform specific I2C write operation in the function
 *  definition in ni_hal.h
 *
 *
 *  @param s Target client to write to
 *  @param reg Register address
 *  @param length Length of write operation
 *  @param data Data buffer that stores the write value
 *  @return Error status
 */
u8 ni_hal_i2c_write( struct ni_sensor_t s, u8 reg, u8 length, u8 *data );


/** @brief Modify specific bits within a register
 *
 *
 *  Read NUM_SENSORS-length register array, and for each register spaced 'offset'
 *  bytes apart within this array, clear bits specified by 'mask', set any bits
 *  specified by 'data', then write back to register
 *
 *
 *  @param s Target client to write to
 *  @param reg Register address
 *  @param data Bit(s) to be modified
 *  @param data Bit mask
 *  @return Error status
 */
u8 ni_i2c_modify_array( struct ni_sensor_t s, u8 reg, u8 data, u8 mask );


/** @brief Function wrapper for I2C read
 *
 *
 *  The read function wrapper is designed to work on various embedded platform.
 *  User should fill the platform specific I2C read operation in the function
 *  definition in ni_hal.h
 *
 *
 *  @param sensor_id Target client to read from
 *  @param reg Register address
 *  @param length Length of read operation
 *  @param data Data buffer that stores the read value
 *  @return Error status
 */
u8 ni_i2c_read( u8 sensor_id, u8 reg, u8 length, u8 *data );


/** @brief Function wrapper for I2C write
 *
 *
 *  The write function wrapper is designed to work on various embedded platform.
 *  User should fill the platform specific I2C write operation in the function
 *  definition in ni_hal.h
 *
 *
 *  @param sensor_id Target client to write to
 *  @param reg Register address
 *  @param length Length of write operation
 *  @param data Data buffer that stores the write value
 *  @return Error status
 */
u8 ni_i2c_write( u8 sensor_id, u8 reg, u8 length, u8 *data );


/** @brief Force reset baseline
 *
 *  Manually reset the baseline of designated AFE
 *
 *  @param sensor_id Target client to reset baseline
 */
u8 ni_force_baseline( u8 sensor_id );


/** @brief Read Baseline value of designated AFE
 *
 *
 *  Read the 12-bit baseline from the designated client(s).  The baseline value will be
 *  shifted into  *  proper 12-bit format for easy process.  For example, to
 *  read the baseline from client #0:
 *
 *  ni_read_baseline( 0, *temp );
 *
 *  Also, to read the baseline from all clients, set sensor_id to a number >= NUM_SENSOR:
 *
 *  If NUM_SENSOR = 4, following function call reads baseline from all 4 clients
 *  ni_read_baseline( 4, *temp );
 *
 *
 *  @param sensor_id Clients ID to read baseline from.  When sensor_id >= NUM_SENSOR then read from all clients
 *  @param ni_baseline Data buffer that stores baseline value read from client(s)
 *  @return Error status
 */
u8 ni_read_baseline( u8 sensor_id, s16* ni_baseline );


/** @brief Read temperature value of designated AFE
 *
 *
 *  Read the 12-bit temperature from the designated client(s).  The temperature value will be
 *  shifted into  *  proper 12-bit format for easy process.  For example, to
 *  read the temperature from client #0:
 *
 *  ni_read_temperature( 0, *temp );
 *
 *  Also, to read the temperature from all clients, set sensor_id to a number >= NUM_SENSOR:
 *
 *  If NUM_SENSOR = 4, following function call reads temperature from all 4 clients
 *  ni_read_temperature( 4, *temp );
 *
 *
 *  @param sensor_id Clients ID to read temperature from.  When sensor_id >= NUM_SENSOR then read from all clients
 *  @param ni_temperature Data buffer that stores temperature value read from client(s)
 *  @return Error status
 */
/*u8 ni_read_temperature( u8 sensor_id, u16* ni_temperature );*/


/** @brief Read interrupt threshold value of designated AFE
 *
 *
 *  Read the 12-bit interrupt threshold from the designated client(s).  The interrupt threshold
 *  value will be shifted into  *  proper 12-bit format for easy process.  For example, to
 *  read the interrupt threshold from client #0:
 *
 *  ni_read_interrupt_threshold( 0, *temp );
 *
 *  Also, to read the interrupt threshold from all clients, set sensor_id to a number >= NUM_SENSOR:
 *
 *  If NUM_SENSOR = 4, following function call reads interrupt threshold from all 4 clients
 *  ni_read_interrupt_threshold( 4, *temp );
 *
 *
 *  @param sensor_id Clients ID to read interrupt threshold from.  When sensor_id >= NUM_SENSOR then read from all clients
 *  @param ni_interrupt Data buffer that stores interrupt threshold value read from client(s)
 *  @return Error status
 */
u8 ni_read_interrupt_threshold( u8 sensor_id, u16* ni_interrupt_threshold );


/** @brief Read autocal threshold value of designated AFE
 *
 *
 *  Read the 12-bit autocal threshold from the designated client(s).  The autocal threshold
 *  value will be shifted into  *  proper 12-bit format for easy process.  For example, to
 *  read the autocal threshold from client #0:
 *
 *  ni_read_autocal_thr( 0, *temp );
 *
 *  Also, to read the autocal threshold from all clients, set sensor_id to a number >= NUM_SENSOR:
 *
 *  If NUM_SENSOR = 4, following function call reads autocal threshold from all 4 clients
 *  ni_read_autocal_thr( 4, *temp );
 *
 *
 *  @param sensor_id Clients ID to read autocal threshold from.  When sensor_id >= NUM_SENSOR then read from all clients
 *  @param ni_autocal Data buffer that stores autocal threshold value read from client(s)
 *  @return Error status
 */
u8 ni_read_autocal_thr( u8 sensor_id, u16* ni_autocal_thr );


/** @brief Set interrupt threshold value of designated AFE
 *
 *  Set the 12-bit interrupt threshold to the designated client(s).
 *
 *  @param sensor_id Clients ID to read interrupt threshold from.
 *  @param ni_interrupt Data buffer that stores interrupt threshold value to be sent to client
 *  @return Error status
 */
u8 ni_set_interrupt_threshold( u8 sensor_id, u16* ni_interrupt_threshold );


/** @brief Set autocal threshold value of designated AFE
 *
 *  Set the 12-bit autocal threshold to the designated client(s).
 *
 *  @param sensor_id Clients ID to read autocal threshold from.
 *  @param ni_autocal Data buffer that stores autocal threshold value to be sent to client
 *  @return Error status
 */
u8 ni_set_autocal_thr( u8 sensor_id, u16* ni_autocal );


u8 ni_write_reg(u8 sensor_id, u8 data, u8 reg, u8 shift, u8 mask ) ;

u8 ni_set_fallthr( u8 sensor_id, u16* fallthr );

u8 ni_set_interrupt_enable(u8 sensor_id);
u8 ni_set_interrupt_disable(u8 sensor_id);
u8 ni_set_adcraw_bl(u8 sensor_id);
u8 ni_set_adcraw_raw(u8 sensor_id);


u8 ni_calibration_init(s16 *sensor_init_offset) ;
u8 ni_calibration_start(s16 *sensor_load_adc) ;
u8 ni_load_calibration_data(float desired_force);
u8 ni_calibration_process(void);
u8 ni_calc_sensor_cal_data(u8 sensor_num, u16 *sensor_cal_data, s16 * sensor_load_adc, s16 * sensor_initial_adc);
u8 ni_calibration_done(s16 * sensor_load_adc, s16 * sensor_initial_adc);
u8 ni_calibration_process(void);


void ni_hal_delay(u16 delay_ms);
u8 ni_hal_write_calbiraiton_data(u8 num_sensor, u16 *calibration_data);
u8 ni_hal_read_calbiraiton_data(u8 num_sensor, u16 *calibration_data);


#ifdef __cplusplus
}
#endif

#endif
