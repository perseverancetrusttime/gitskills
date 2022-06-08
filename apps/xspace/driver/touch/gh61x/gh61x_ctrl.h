/**
  ******************************************************************************
  * @file    gh61x_ctrl.h
  * @author  William Zhou
  * @version V0.1.0
  * @brief   GH61X sensor control library
  * @modification history
  *  Data                Name                  Description
  *  ================    ==================    =================================
  *   6-Nov.  -2018      William Zhou          Initial draft
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 Goodix Inc. All rights reserved.
  *
  ******************************************************************************
  */
  
#ifndef __GH61X_CTRL_H__
#define __GH61X_CTRL_H__

#if defined (__TOUCH_GH61X__)

#include <stdint.h>
#include <string.h>


#ifndef __GH61x_WD_STATUS_TYPE__
#define __GH61x_WD_STATUS_TYPE__
/**
 * @brief Structure for wearing detection status.
 */
typedef enum
{
    WD_STATUS_UNKNOWN = 0,
    WD_STATUS_WEARING = 1,
    WD_STATUS_UNWEARING = 2,
} EM_WD_STATUS_TYPE;
#endif


#ifndef __GH61x_TK_STATUS_TYPE__
#define __GH61x_TK_STATUS_TYPE__
/**
 * @brief Structure for touchkey detection status.
 */
typedef enum
{
    TK_NO_OPERA = 0,
    TK_SINGLE_CLICK = 1,
    TK_DOUBLE_CLICK = 2,
    TK_UP_SLIDE = 3,
    TK_DOWN_SLIDE = 4,
    TK_LONG_TOUCH = 5,

    TK_TRIPLE_CLICK = 6,
    TK_QUADRA_CLICK = 7,
    TK_PENTA_CLICK = 8,
    TK_TOUCH_DOWN = 9,
    TK_TOUCH_UP = 10,
    TK_LONG_LONG_TOUCH = 11,

    TK_SLIDE_LEFT = 12,
    TK_SLIDE_RIGHT = 13,
    TK_SLIDE_CLOCK = 14,//顺时针
    TK_SLIDE_ANTICLOCK = 15,	//逆时针
} EM_TK_STATUS_TYPE;
#endif

#ifndef __GH61x_SAMPLATE_RATE_TYPE__
#define __GH61x_SAMPLATE_RATE_TYPE__
/**
 * @brief Structure for sample rate status.
 */
typedef enum
{
    GH61X_SAMPLE_100HZ = 0,
    GH61X_SAMPLE_50HZ = 1,
    GH61X_SAMPLE_20HZ = 2,
    GH61X_SAMPLE_10HZ = 3,
    GH61X_SAMPLE_5HZ = 4,    
} EM_GH61x_SAMPLE_RATE_TYPE;
#endif

#ifndef __GH61x_FUNCTION_SWITCH_TYPE__
#define __GH61x_FUNCTION_SWITCH_TYPE__
/**
 * @brief Structure for sample rate status.
 */
typedef enum
{
    GH61X_FUNCTION_SWITCH_DISABLE = 0,
    GH61X_FUNCTION_SWITCH_ENABLE = 1,  
} EM_GH61x_FUNCTION_SWITCH_TYPE;
#endif

#ifndef __GH61x_SELFCALI_RESULT_TYPE__
#define __GH61x_SELFCALI_RESULT_TYPE__
/**
 * @brief Structure for offset self-calibration inbox result.
 */
typedef enum
{
    GH61X_SELFCALI_OK = 0,
    GH61X_SELFCALI_MINOFFSET_INVALID = 1,
    GH61X_SELFCALI_FLASH_INVALID = 2,
    GH61X_SELFCALI_FC_OFFSET_INVALID = 3,
    GH61X_SELFCALI_SC_OFFSET_INVALID = 4,
    GH61X_SELFCALI_MIN_EXCESS_INBOX = 5,    
	GH61X_SELFCALI_MIN_EQUAL_REAL = 6,    
	GH61X_SELFCALI_MIN_ABNORMAL = 7,    
} EM_GH61x_SELFCALI_RESULT_TYPE;
#endif

/**@brief Macro for GH61X function return code. */
#define GH61X_RET_START_SUCCESS				      (1)                /**< There is no error */
#define GH61X_RET_OK                              (0)                /**< There is no error */
#define GH61X_RET_GENERIC_ERROR                   (-1)               /**< A generic error happens */
#define GH61X_RET_PARAMETER_ERROR                 (-2)               /**< Parameter error */
#define GH61X_RET_COMM_NOT_REGISTERED_ERROR       (-3)               /**< I2C communication interface not registered error */
#define GH61X_RET_COMM_ERROR                      (-4)               /**< I2C Communication error */
#define GH61X_RET_RESOURCE_ERROR                  (-5)               /**< Resource full or not available error */
#define GH61X_RET_NO_INITED_ERROR                 (-6)               /**< No inited error */
#define GH61X_RET_FUN_NOT_REGISTERED_ERROR        (-7)               /**< Related function not registered error */
#define GH61X_RET_FIRMWARE_LOAD_ERROR             (-8)               /**< GH61X firmware error */
#define GH61X_RET_ABNORMAL_RESET_ERROR            (-9)               /**< GH61X abnormal reset error */
#define GH61X_RET_INVALID_IN_MP_MODE_ERROR        (-10)              /**< Function invalid in MP mode */
#define GH61X_RET_FIRMWARE_HEAD_CRC_ERROR	      (-11)				 /**< GH61X Firmware Head CRC Error */
#define GH61X_RET_FIRMWARE_HEAD_NO_TYPE_ERROR     (-12)				 /**< GH61X Firmware Head do not have Chip Type Info */
#define GH61X_RET_FIRMWARE_PATCH_MISMATCH_ERROR   (-13)			 /**< GH61X Firmware Head Type misMatch with Chip Type Info */
#define GH61X_RET_HOSTSOFTWAREVER_LENGTH_ERROR    (-14)            /**< Host software version length check error */
#define GH61X_RET_HOSTSOFTWAREVER_LENGTH_OVERRUN  (-15)            /**< Host software version length check error */

/**
 * @brief N-microsecond delay function prototype.
 */
#ifndef __GH61X_DELAY_US_FUNC_PROTOTYPE__
#define __GH61X_DELAY_US_FUNC_PROTOTYPE__
typedef void (*pfnDelayUs)(uint16_t usUsec);
#endif

/**
 * @brief N-millisecond delay function prototype.
 */
#ifndef __GH61X_DELAY_MS_FUNC_PROTOTYPE__
#define __GH61X_DELAY_MS_FUNC_PROTOTYPE__
typedef void (*pfnDelayMs)(uint16_t usMsec);
#endif

/**
 * @brief GH61X reset pin operation function prototype.
 */
#ifndef __GH61X_RESET_PIN_LEVEL_FUNC_PROTOTYPE__
#define __GH61X_RESET_PIN_LEVEL_FUNC_PROTOTYPE__
typedef void (*pfnSetResetPinLevel)(void);
#endif

/**
 * @brief Get GH61X reset pin level function prototype.
 * @param  None.
 * @retval 0: low-level; 1: high-level.
 */
#ifndef __GH61X_GET_PIN_LEVEL_FUNC_PROTOTYPE__
#define __GH61X_GET_PIN_LEVEL_FUNC_PROTOTYPE__
typedef uint8_t (*pfnGetPinLevel)(void);
#endif

/**
 * @brief GH61X I2C writting operation function prototype.
 */
#ifndef __GH61X_I2C_WRITE_FUNC_PROTOTYPE__
#define __GH61X_I2C_WRITE_FUNC_PROTOTYPE__
typedef uint8_t (*pfnI2cWrite)(uint8_t uchDeviceId, const uint8_t uchWriteBytesArr[], uint16_t usWriteLen); // i2c write function typde
#endif

/**
 * @brief GH61X I2C reading operation function prototype.
 */
#ifndef __GH61X_I2C_READ_FUNC_PROTOTYPE__
#define __GH61X_I2C_READ_FUNC_PROTOTYPE__
typedef uint8_t (*pfnI2cRead)(uint8_t uchDeviceId, const uint8_t uchCmddBytesArr[], uint16_t usCmddLen, uint8_t uchReadBytesArr[], uint16_t usMaxReadLen); // i2c read function typde
#endif

/**
 * @brief Sending data function prototype.
 */
#ifndef __GH61X_SEND_DATA_FUNC_PROTOTYPE__
#define __GH61X_SEND_DATA_FUNC_PROTOTYPE__
typedef void (*pfnSendData)(uint8_t uchDataBuff[], uint8_t uchLen); // send data function typde
#endif

/**
 * @brief GH61X calibration list writting operation function prototype.
 */
#ifndef __GH61X_CALI_LIST_WRITE_FUNC_PROTOTYPE__
#define __GH61X_CALI_LIST_WRITE_FUNC_PROTOTYPE__
typedef uint8_t (*pfnCalibrationListWrite)(const uint8_t uchCalListArr[], uint16_t usWriteLen);
#endif

/**
 * @brief GH61X calibration list reading operation function prototype.
 */
#ifndef __GH61X_CALI_LIST_READ_FUNC_PROTOTYPE__
#define __GH61X_CALI_LIST_READ_FUNC_PROTOTYPE__
typedef uint8_t (*pfnCalibrationListRead)(uint8_t uchReadBytesArr[], uint16_t usReadLen);
#endif


/**
  * @brief  Get GH61X Ctrl Library Version.
  * @note   User buffer length is 15 Bytes.
  * @param  pszGH61XCtrlVer: Pointer of user buffer for version string;
  *         len: should assign to 15 to get version completely
  * @retval None
  */
void GH61X_GetVersion(char *pszGH61XCtrlVer, uint8_t len);


/**
  * @brief  Get GH61X Firmware Version.
  * @param  None.
  * @retval GH61X Firmware Version in Word.
  */
uint16_t GH61X_GetFirmwareVersion(void);


/**
  * @brief  Register N-microsecond delay function. Deprecated !!!
  * @note   N-microsecond delay function should be implemented by User.
  * @param  pfnDelayUs  Pointer of N-microsecond delay function.
  * @retval None
  */
void GH61X_SetDelayUsCallback (pfnDelayUs pDelayUsFunc);


/**
  * @brief  Register N-millisecond delay function.
  * @note   N-millisecond delay function should be implemented by User.
  * @param  pfnDelayMs  Pointer of N-millisecond delay function.
  * @retval None
  */
void GH61X_SetDelayMsCallback (pfnDelayMs pDelayMsFunc);


/**
  * @brief  Register function for setting GH61X reset pin level high and function for setting GH61X reset pin level low.
  * @note   Functions should be implemented by User.
  * @param  pSetResetPinHigh  Pointer of function for setting GH61X reset pin level high.
            pSetResetPinLow   Pointer of function for setting GH61X reset pin level low.
  * @retval None
  */
void GH61X_SetResetPinHandler(pfnSetResetPinLevel pSetResetPinHigh, pfnSetResetPinLevel pSetResetPinLow);


/**
  * @brief  Register function for reading GH61X reset pin level.
  * @note   Functions should be implemented by User.
  * @param  pGetResetPinLevel  Pointer of function for reading GH61X reset pin level.
  * @retval None
  */
void GH61X_SetReadResetPinLevel(pfnGetPinLevel pGetResetPinLevel);


/**
  * @brief  Register function for reading GH61X INT pin level.
  * @note   Functions should be implemented by User.
  * @param  pGetIntPinLevel  Pointer of function for reading GH61X INT pin level.
  * @retval None
  */
void GH61X_SetReadIntPinLevel(pfnGetPinLevel pGetIntPinLevel);


/**
  * @brief  Register function for I2C writting operation and function for I2C reading operation.
  * @note   Functions should be implemented by User.
  * @param  pSetResetPinHigh  Pointer of function for I2C writting operation.
            pSetResetPinLow   Pointer of function for I2C reading operation.
  * @retval None
  */
void GH61X_SetI2cRW (pfnI2cWrite pI2cWriteFunc, pfnI2cRead pI2cReadFunc);


/**
  * @brief  Register function for calibration list writting operation and function for calibration list reading operation.
  * @note   Functions should be implemented by User.
  * @param  pCaliListWriteFunc  Pointer of function for calibration list writting operation.
            pCaliListReadFunc   Pointer of function for calibration list reading operation.
  * @retval None
  */
void GH61X_SetCalibrationListRW (pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc);

/**
  * @brief  Register function for backup calibration list writting operation and function for calibration list reading operation.
  * @note   Functions should be implemented by User.
  * @param  pCaliListWriteFunc  Pointer of function for calibration list writting operation.
            pCaliListReadFunc   Pointer of function for calibration list reading operation.
  * @retval None
  */
void GH61X_SetBackupCalibrationListRW (pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc);


/**
  * @brief  Register address of GH61X inner MCU firmware.
  * @param  ptr  GH61X inner MCU firmware address.
  * @retval None
  */
void GH61X_SetFirmwareAddress(const uint8_t *ptr);


/**
  * @brief  Function for loading GH61X register configuration.
  * @param  uchConfigArr                            Array for register configuration.
                                                    {
                                                        0x47, 0x52, 0x41, 0x5A, //Magic number
                                                        length,
                                                        addr_h, addr_l, value_h, value_l,
                                                        ...
                                                    }
  * @retval GH61X_RET_COMM_NOT_REGISTERED_ERROR     I2C opearation functions not registered.
            GH61X_RET_GENERIC_ERROR                 
            GH61X_RET_OK                            GH61X initialization succeed.                          
  */
int8_t GH61X_LoadConfig(uint8_t uchConfigArr[]);


/**
  * @brief  Function for initializing GH61X chip.
  * @param  None.
  * @retval GH61X_RET_COMM_NOT_REGISTERED_ERROR     I2C opearation functions not registered.
            GH61X_RET_COMM_ERROR                    GH61X I2C communication error happened.
            GH61X_RET_OK                            GH61X initialization succeed.                          
  */
int8_t GH61X_Init(void);


/**
  * @brief  Function for startting GH61X chip.
  * @param  None.
  * @retval GH61X_RET_NO_INITED_ERROR     GH61X not initialized.
            GH61X_RET_GENERIC_ERROR       GH61X MCU firmware CRC error happened.
            GH61X_RET_OK                  GH61X starting succeed.                          
  */
int8_t GH61X_Start(void);

/**
  * @brief  Function for startting GH61X chip in app mode.
  * @param  needAutoCancel				  0: No auto-cancel after started; 1: Auto-cancel after started.
  * @retval GH61X_RET_NO_INITED_ERROR     GH61X not initialized.
            GH61X_RET_OK                  GH61X starting succeed.                          
  */
int8_t GH61X_AppMode_Start(uint8_t needAutoCancel);

/**
  * @brief  Function for startting GH61X chip and Roundkey algorithm.
  * @param  None.
  * @retval GH61X_RET_NO_INITED_ERROR     GH61X not initialized.
            GH61X_RET_GENERIC_ERROR       GH61X MCU firmware CRC error happened.
            GH61X_RET_OK                  GH61X starting succeed.                          
  */
int8_t GH61X_HostMode_Start(void);


/**
  * @brief  Function for stoping GH61X chip.
  * @param  None.
  * @retval GH61X_RET_NO_INITED_ERROR     GH61X not initialized.
            GH61X_RET_OK                  GH61X stopping succeed.                          
  */
int8_t GH61X_Stop(void);


/**
  * @brief  Function for reseting GH61X chip by pulling down GH61x reset pin.
  * @param  None.
  * @retval GH61X_RET_COMM_NOT_REGISTERED_ERROR     GH61X reset pin operation functions not registered.
            GH61X_RET_OK                            GH61X reseting succeed.                          
  */
int8_t GH61X_Reset(void);


/**
  * @brief  Function for reseting GH61X chip by sending reset command.
  * @param  None.
  * @retval GH61X_RET_COMM_NOT_REGISTERED_ERROR     GH61X I2C writting function not registered.
            GH61X_RET_OK                            GH61X reseting succeed.                          
  */
int8_t GH61X_SW_Reset(void);


/**
  * @brief  Function for simulating abnormal reseting GH61X chip by pulling down GH61x reset pin.
  * @param  None.
  * @retval GH61X_RET_COMM_NOT_REGISTERED_ERROR     GH61X reset pin operation functions not registered.
            GH61X_RET_OK                            GH61X reseting succeed.                          
  */
int8_t GH61X_Abnormal_Reset(void);


/**
  * @brief  Register function for sending data to controller(MobilePhone or workstation).
  * @param  pfnSendData                 Pointer of function for sending data to controller.
  * @retval retval > 0                  communication interface id.
            GH61X_RET_RESOURCE_ERROR    have no resource,                          
  */
int8_t GH61X_SetSendDataFunc(pfnSendData pSendDataFunc);


/**
  * @brief  Function for parsing received data from controller(MobilePhone or workstation).
  * @param  uchCommInterFaceId      ID from GH61X_SetSendDataFunc return val.
            uchDataBuffArr          Pointer of received data from controller.
            uchLen                  Length of received data from controller.
  * @retval 0x10: MCU_MODE_START
  *         0x11: STOP
  *         0x12: APP_MODE_START
  *         0x13: APP_MODE_START_WITHOUT_CANCEL
  *         0x14: HOST_MODE_START
  *         0x15: HOST_MODE_START
  *         others: reserved
  */
uint8_t GH61X_CommParseHandler(uint8_t uchCommInterFaceId, uint8_t uchDataBuffArr[], uint8_t uchLen);


/**
  * @brief  Function for packing rawdata and sending to controller(MobilePhone or workstation).
  * @param  None
            None
  * @retval                           
  */
void GH61X_SendPackage(void);


/**
  * @brief  Function for handling GH61X interrupt.
  * @param  None
  * @retval None                          
  */
int8_t GH61X_IRQHandler(void);


/**
  * @brief  Function for getting GH61X wearing deteciton status.
  * @param  None
  * @retval See EM_WD_STATUS_TYPE definition.
  */
EM_WD_STATUS_TYPE GH61X_GetWearingStatus(void);


/**
  * @brief  Function for getting GH61X touchkey deteciton status.
  * @param  None
  * @retval See EM_TK_STATUS_TYPE definition.
  */
EM_TK_STATUS_TYPE GH61X_GetTouchkeyStatus(void);

/**
  * @brief  Function for checking GH61X wearing status flag and touchkey status flag validation.
  * @param  None
  * @retval 0: GH61X_GetWearingStatus() and GH61X_GetTouchkeyStatus() is valid for getting status; 
  *         1: GH61X_GetWearingStatus() and GH61X_GetTouchkeyStatus() is invalid for getting status;
  */
uint8_t GH61X_HasWearingOrTkEventDetected(void);


/**
  * @brief  Function for updating configuration from calibration lists.
  * @param  None
  * @retval See EM_TK_STATUS_TYPE definition.
  */
int8_t GH61X_Calibration(void);


/**
  * @brief  Function for disable in-ear detection, only touchkey detection available.
  * @param  None
  * @retval None.
  */
void GH61X_DisableInEarDetection(void);

/**
  * @brief  Function for enable in-ear detection, both in-ear detection and touchkey detection available.
  * @param  None
  * @retval None.
  */
void GH61X_EnableInEarDetection(void);


/**
 * @brief  Get Gsensor data function prototype.
 * @param  axisData[0]:x-axis data, axisData[1]:y-axis data, axisData[2]:z-axis data
 * @retval None.
 */
typedef void (*pfnGetGsensorData)(uint16_t axisData[3]);

/**
  * @brief  Register function for getting gsensor data.
  * @note   Functions should be implemented by User.
  * @param  pGetGsensorDataFunc  Pointer of function for getting gsensor data.
  * @retval None
  */
void GH61X_SetGsensorReadFunc(pfnGetGsensorData pGetGsensorDataFunc);


/**
  * @brief  Set GH61X I2C device ID.
  * @note   I2C device ID in 7-bits.
  * @param  device_id  GH61X I2C device ID: GH610(0x08); GH611(0x18); GH612(0x3c); GH613(0x44).
  * @retval None
  */
void GH61X_SetI2cDeviceId(uint8_t device_id);


/**
  * @brief  GH61x self calibration in charging box.
  * @note   Only called when earphone in charging box.
  * @param  None.
  * @retval None.
  */
void GH61X_SelfCaliInChargingBox(void);


/**
  * @brief  Function for checking whether system is in MP mode.
  * @param  None.
  * @retval 0: system not in MP mode; 1: system in MP mode.
  */
uint8_t GH61X_IsSystemInMpMode(void);

/**
  * @brief  Function for configuring whether disable touchkey all drive when IED working.
  * @param  0: driving touchkey by all drive configuration when IED working;
            others: disable touchkey all drive when IED working
  * @retval None.
  */
void GH61X_DisableTkWhenIEDSampling(uint8_t enable);

/**
  * @brief  Function for configuring the offset Abnormal threshold in flash.
  *         Called before GH61X_Init called from the power up.		
  * @param  thresholdS0,thresholdS1: if (self cali offset - factory cali offset) > threshold
  *						the host will reset the self cali offset in flash(default:200)           
  * @retval None.
  */
void GH61X_SetOffsetAbnormalThreshold(uint16_t thresholdS0,uint16_t thresholdS1);


/**
  * @brief  Function for Recording Offset in Flash before the Host Shutdown.
  * 		Only Called if needed to Powerup in Ear later
  * @param  None
  * @retval None.
  */
void GH61X_RecordOffsetBeforeHostShutdown(void);

/**
  * @brief  Function for Loading Offset Before GH61x Start.
  *			Only Called when GH61X power up in Ear
  * @param  None
  * @retval None.
  */
void GH61X_LoadOffsetBeforeStart(void);

/**
  * @brief  Function for geting data in app mode.
  * @param  data[N]: channel-N rawdata;
  *         channeltotal: the channel total, no more than 7;
  * @retval GH61X_RET_OK:			 Data is valid;
  			GH61X_RET_GENERIC_ERROR: Data is invalid because system is not in app mode or connected to Goodix APP/MP-Tool.
  */
int8_t GH61X_GetRawData(int16_t data[], uint8_t channeltotal);

/**
  * @brief  Function for geting IRQ status in mcu mode.
  * @param  None
  * @retval the IRQ status
  */
uint16_t GH61X_ReturnIRQStatus(void);

/**
  * @brief  Function for setting Host software version.
  * @note   User buffer max length is 16
            Host software version Transmit in ASCII
            Host software version end with '\0'
  * @param  puchHostSoftwareVer Pointer of user buffer for Host software version string
            usVerLength is the length of software version include the string '\0'
  * @retval GH61X_RET_OK
           GH61X_RET_HOSTSOFTWAREVER_LENGTH_ERROR
           GH61X_RET_HOSTSOFTWAREVER_LENGTH_OVERRUN
  */
int8_t GH61X_SetHostSoftwareVer(char *puchHostSoftwareVer, uint8_t usVerLength);

/**
  * @brief  Function for setting bluetooth mac information.
  * @note   User buffer length is 6
            Blutooth MAC information Transmit in Bid Endian
  * @param  puchBtMacInfo Pointer of user buffer for bluetooth mac information
  * @retval  
  */
int8_t GH61X_SetBtMacInfo(uint8_t *puchBtMacInfo);

/**
  * @brief  Function for setting second magnification power if needed.
  * @note   scope:[0.5, 1.5]
	        eg:     uint8_t dram_second_configuration_array[] = 
					{
						usFactorDeNominator, usFactorNumerator,//KEY0
						usFactorDeNominator, usFactorNumerator,//KEY1
						usFactorDeNominator, usFactorNumerator,//KEY2
						usFactorDeNominator, usFactorNumerator,//S1P
					    usFactorDeNominator, usFactorNumerator,//S0N
						usFactorDeNominator, usFactorNumerator,//S1N						
					};
		    call this interface after start gh61x;
  * @param   usFactorNumerator: the numerator of magnification power
             usFactorDeNominator: the denominatoor pf magnification power
  * @retval  GH61X_RET_OK
             GH61X_RET_PARAMETER_ERROR
  */
int8_t GH61X_Set2ndMgnPower(uint8_t *pusFactorArr);

/**
  * @brief  Function for setting samplate frequency.
  * @note   valid in app mode,;
            call this interface before start in app mode;
  * @param   enumSaplateRate must be the enum value in typedef  EM_GH61x_SAMPLATE_RATE_TYPE
             otherwise return error GH61X_RET_PARAMETER_ERROR
  * @retval  GH61X_RET_OK
             GH61X_RET_PARAMETER_ERROR
  */
int8_t GH61X_SetSampleRateType(EM_GH61x_SAMPLE_RATE_TYPE enumSampleRate);

/**
  * @brief  Function for setting offset self learning factors in charging box.
  * @note   valid range:[1, 64]
  * @param   usFactorNeg: self calibration factor negatively
             usFactorPos: self calibration factor positively
  * @retval  GH61X_RET_OK
             GH61X_RET_PARAMETER_ERROR
  */
int8_t GH61X_SetOffsetSelfLearnFactorInChargingBox(uint8_t usFactorNeg, uint8_t usFactorPos);

/**
  * @brief  Function for setting offset self learning overall count in charging box.
  * @note   valid range:[0, 255]
  * @param   usCount: self calibration overall count; In the first usCount self learning,
  *                   offset will directly update to the last min offset;
  *                   default is 0(disable)
  * @retval  void             
  */
void GH61X_SetOffsetSelfLearnOverAllCountInChargingBox(uint8_t usCount);

/**
  * @brief  Function for setting the information to normalize when there is no int_num information in CALI-LIST.
  * @param emEnable: GH61X_FUNCTION_SWITCH_ENABLE enable the function
  *         usOldNorS0IntNum: the target int_num to nnormalize for S0;
  *         usOldNorS1IntNum: the target int_num to nnormalize for S1;
  * @retval none.
  */
void GH61X_SetOldNormalizationInfo(EM_GH61x_FUNCTION_SWITCH_TYPE emEnable, uint8_t usOldNorS0IntNum, uint8_t usOldNorS1IntNum);

/**
  * @brief  Entering super mass production mode.
  * @param  None.
  * @retval GH61X_RET_OK:			 Entering super-mp-mode successful;
  			GH61X_RET_GENERIC_ERROR: GH61x is not in work mode.
  */
int8_t GH61X_EnterSuperMpMode(void);


/**
  * @brief  Exiting super mass production mode.
  * @param  None.
  * @retval GH61X_RET_OK:			 Exiting super-mp-mode successful;
  			GH61X_RET_GENERIC_ERROR: GH61x is not in super-mp-mode.
  */
int8_t GH61X_ExitSuperMpMode(void);


/**
  * @brief  Function for checking whether system is in super-mp-mode.
  * @param  None.
  * @retval 0: system not in super-mp-mode; 1: system in super-mp-mode.
  */
uint8_t GH61X_IsSystemInSuperMpMode(void);


/**
  * @brief  Function for getting data in super-mp-mode.
  * @param  data[0]: base;
  			data[1]: rawdata;
  			data[2]: diff;
  * @retval GH61X_RET_OK:			 Data is valid;
  			GH61X_RET_GENERIC_ERROR: Data is invalid because system is not in super-mp-mode.
  */
int8_t GH61X_GetDataInSuperMpMode(int16_t data[]);

/**
  * @brief  Function for getting result of offset-self-calibration in box.
  * @param  usResultSelfCali[0]: the result of S0 channel in offset-self-calibration;
  			usResultSelfCali[1]: the result of S1 channel in offset-self-calibration;
  * @retval none
  */
void GH61X_GetSelfCalibrationResult(uint8_t usResultSelfCali[2]);

/**
  * @brief  Function for getting debug information of offset-self-calibration in box.
  * @param  OffsetArray[0-1]: real-min-offset;
  			OffsetArray[2-3]: real-Offset in box;
  			OffsetArray[4-5]: real-SC-Offset;
            OffsetArray[6-7]: real-FC-Offset;
            OffsetArray[8-9]: real-box-diff;
  * @retval none
  */
void GH61X_GetOffsetInfoInSelfCali(int16_t OffsetArray[10]);

/**
  * @brief  Function for configure delay time in mp mode to read flash.
  * @param  usXms: delay Xms when reading flash per frame in mp mode;
  * @retval none
  */
void GH61X_SetCFGDelayMsReadFlashInMP(uint16_t usXms);

int GH61X_Ctrl_init(void);

void gh61x_int_handler_deal(void);

void uart_rx_data_handler(uint8_t *buffer, uint8_t length);

void user_gh61x_work_state_checker(void);

int8_t GH61X_Application_Start(void);

int8_t GH61X_Application_Stop(void);

void gh61x_spp_rx_data_handler(uint8_t *buffer, uint8_t length);
#endif

#endif /* __GH61X_CTRL_H__ */

/************* Copyright (c) 2018 Goodix Inc. All rights reserved. ************/
