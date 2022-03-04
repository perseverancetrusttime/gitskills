/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_example_common.h
 *
 * @brief   example code for gh621x
 * 
 * @version  @ref GH621X_EXAMPLE_VER_STRING
 *
 */


#ifndef _GH3011_EXAMPLE_COMMON_H_
#define _GH3011_EXAMPLE_COMMON_H_

/* includes */
#include "stdio.h"
#include "string.h"
#include "gh621x_drv.h"
#include "gh621x_example_config.h"


/* debug log control, if enable debug, Gh621xDebugLog must define in platforms code */
#if (__EXAMPLE_DEBUG_LOG_LVL__) // debug level > 0
    /// lv1 log string
    #define   EXAMPLE_DEBUG_LOG_L1(...)       do {\
                                                    char chExampleDbgLogStr[__EXAMPLE_LOG_DEBUG_SUP_LEN__] = {0};\
                                                    snprintf(chExampleDbgLogStr, __EXAMPLE_LOG_DEBUG_SUP_LEN__, \
                                                            "[GH621X] "__VA_ARGS__);\
                                                    Gh621xDebugLog(chExampleDbgLogStr);\
                                                } while (0)
    #if (__EXAMPLE_DEBUG_LOG_LVL__ > 1) // debug level > 1
        /// lv2 log string
        #define   EXAMPLE_DEBUG_LOG_L2(...)        { EXAMPLE_DEBUG_LOG_L1(__VA_ARGS__); }
    #else
        #define   EXAMPLE_DEBUG_LOG_L2(...)
    #endif
#else   // debug level <= 0
    #define   EXAMPLE_DEBUG_LOG_L1(...)  
    #define   EXAMPLE_DEBUG_LOG_L2(...) 
#endif

/// ok and err val for some api ret
#define GH621X_EXAMPLE_OK_VAL                (GH621X_BYTE_FLAG_TRUE)
#define GH621X_EXAMPLE_ERR_VAL               (GH621X_BYTE_FLAG_FALSE)

/// pkg respond buffer len, equal(255+5)
#define GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN   (260)

/// sar rawdata buffer max len
#define GH621X_EXAMPLE_SAR_RAWDATA_MAX_LEN   (256)

/// tk ied rawdata buffer max len
#define GH621X_EXAMPLE_TK_IED_RAWDATA_MAX_LEN (256)

/// get offset of 
#define GH621X_EXAMPLE_OFFSET_OF(type, member)          ((GU32) &(((type *) 0)->member))

/// tk ied rawdata channel num
#define GH621X_EXAMPLE_TK_IED_RAWDATA_CH_NUM (9)

/// default patch var
#if (__EXAMPLE_PATCH_LOAD_DEFAULT_ENABLE__)
#define GH621X_DEFAULT_PATCH_VAR             ((GU8 *)__EXAMPLE_PATCH_INCLUDE_ARRAY_VAR__) 
#else
#define GH621X_DEFAULT_PATCH_VAR             (NULL)
#endif


/// send data func ptr type
typedef GU8 (*Gh621xModuleSendDataFuncPtrType)(GU8 uchStr[], GU8 uchLen);  

/// sar rawdata struct type
typedef struct
{
    GS32 nProxUseful[9];         /**< useful */
    GS32 nProxAvg[5];            /**< avg */
    GS32 nProxDiff[5];           /**< diff */
    GU8 uchProxStat[5];          /**< status */
    GU8 uchProxBodyStat[5];      /**< body status */
    GU8 uchProxTableStat[5];     /**< table status */
    GU8 uchProxSteadyStat[9];    /**< steady status */
    GU8 uchProxSompStat[9];      /**< comp status */
    GU8 uchProxOrStat;           /**< or status */
    GU8 uchProxAndStat;          /**< and status */
    GU8 uchTempChangeFlag;       /**< temp change flag */
} STGh621xModuleSarRawdata;

/// motion event enum
typedef enum
{
    GH621X_MOTION_DEFAULT		= 0x00,             /**< default motion */
    GH621X_MOTION_ONE_TAP		= 0x01,             /**< one tap */
    GH621X_MOTION_DOUBLE_TAP	= 0x02,             /**< double tap */
    GH621X_MOTION_UP 			= 0x03,             /**< up slide */
    GH621X_MOTION_DOWN			= 0x04,             /**< down slide */
    GH621X_MOTION_LONG_PRESS	= 0x05,             /**< long press */
    GH621X_MOTION_TRIPLE_TAP	= 0x06,             /**< triple tap */
    GH621X_MOTION_LEFT			= 0x07,             /**< left slide */
    GH621X_MOTION_RIGHT			= 0x08,             /**< right slide */
    GH621X_MOTION_CLOCK			= 0x09,             /**< clock */
    GH621X_MOTION_ANTICLOCK		= 0x0A,             /**< anticlock */
    GH621X_MOTION_QUADRA_TAP	= 0x0B,             /**< quadra tap */
    GH621X_MOTION_PENTA_TAP		= 0x0C,             /**< penta tap */
    GH621X_MOTION_RELEASE		= 0x10,             /**< release/leave */
} EMGh621xModuleMotionEvent;

/// wear status enum
typedef enum
{
    GH621X_WEAR_OUT				= 0x00,             /**< wear out */
    GH621X_WEAR_IN				= 0x01,             /**< wear in */
    GH621X_WEAR_STATUS_DEFAULT	= 0xFF,             /**< default wear status */
} EMGh621xModuleWearStatus;

/// tk ied event info
typedef struct
{
    GU8 uchNewEventFlag;
    EMGh621xModuleWearStatus emWearInfo;
    EMGh621xModuleMotionEvent emTouchInfo;
    EMGh621xModuleMotionEvent emForceInfo;
    GU8 uchTempInfo;
} STGh621xModuleTkIedEventInfo;


/* var  */ 

/// gh621x default reg config array
extern STGh621xReg stGh621xModuleDefaultRegConfigArray[];

/// len of gh621x default reg config array
extern GU16 usGh621xModuleDefaultRegConfigArrayLen;

/// handle flag
extern GU8 usGh621xModuleHandlingLockFlag;

/// last send data func ptr 
extern Gh621xModuleSendDataFuncPtrType pfnGh621xModuleLastSendDataFuncPtr;


/* porting api */

/**
 * @fn     void Gh621xModuleHalI2cInit(void)
 *
 * @brief  i2c for gh621x init
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHalI2cInit(void);

/**
 * @fn     GU8 Gh621xModuleHalI2cWrite(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usLength)
 *
 * @brief  i2c for gh621x wrtie
 *
 * @attention   None
 *
 * @param[in]   uchDeviceId				i2c device 7bits addr 	
 * @param[in]   uchWriteBuffer			pointer to write data buffer
 * @param[in]   usLength				len of write data buffer
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error		
 */
GU8 Gh621xModuleHalI2cWrite(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usLength);

/**
 * @fn     GU8 Gh621xModuleHalI2cRead(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usWriteLength, GU8 uchReadBuffer[], GU16 usReadLength)
 *
 * @brief  i2c for gh621x read
 *
 * @attention   None
 *
 * @param[in]   uchDeviceId				i2c device 7bits addr 	
 * @param[in]   uchWriteBuffer			pointer to write data buffer
 * @param[in]   usWriteLength			len of write data buffer
 * @param[in]   usReadLength			len of read data buffer
 * @param[out]  uchReadBuffer			pointer to read data buffer
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error		
 */
GU8 Gh621xModuleHalI2cRead(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usWriteLength, GU8 uchReadBuffer[], GU16 usReadLength);

/**
 * @fn     void Gh621xModuleHalDelayMs(GU16 usMsCnt)
 *
 * @brief  delay ms
 *
 * @attention   None
 *
 * @param[in]   usMsCnt					delay ms count
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalDelayMs(GU16 usMsCnt);

/**
 * @fn     void Gh621xModuleHalIntInit(void)
 *
 * @brief  gh621x int pin init
 *
 * @attention   should config as falling edge trigger or low level trigger, gh621x default low pulse ouput
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntInit(void);

/**
 * @fn     GU8 Gh621xModuleHalGetIntPinStatus(void)
 *
 * @brief  get reset pin status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  reset pin status
 * @retval  #GH621X_PIN_LEVEL_LOW       reset pin low level
 * @retval  #GH621X_PIN_LEVEL_HIGH      reset pin high level
 */
GU8 Gh621xModuleHalGetIntPinStatus(void);

/**
 * @fn     void Gh621xModuleHalIntDetectDisable(void)
 *
 * @brief  gh621x int pin detect disable
 *
 * @attention   couldn't into handler by int trigger
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntDetectDisable(void);

/**
 * @fn     void Gh621xModuleHalIntDetectReenable(void)
 *
 * @brief  gh621x int pin detect reenable
 *
 * @attention   could into handler again by int trigger 
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntDetectReenable(void);

/**
 * @fn     void Gh621xModuleHalResetPinCtrlInit(void)
 *
 * @brief  gh621x reset pin control init, default level high
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinCtrlInit(void);

/**
 * @fn     void Gh621xModuleHalResetPinSetHighLevel(void)
 *
 * @brief  gh621x reset pin set high level
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinSetHighLevel(void);

/**
 * @fn     void Gh621xModuleHalResetPinSetLowLevel(void)
 *
 * @brief  gh621x reset pin set low level
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinSetLowLevel(void);

/**
 * @fn     GU8 Gh621xModuleHalGetResetPinStatus(void)
 *
 * @brief  get reset pin status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  reset pin status
 * @retval  #GH621X_PIN_LEVEL_LOW       reset pin low level
 * @retval  #GH621X_PIN_LEVEL_HIGH      reset pin high level
 */
GU8 Gh621xModuleHalGetResetPinStatus(void);

/**
 * @fn     GU8 Gh621xModuleSendDataViaBle(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via ble
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
GU8 Gh621xModuleSendDataViaBle(GU8 uchString[], GU8 uchLength); 

/**
 * @fn     GU8 Gh621xModuleSendDataViaUart(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via uart
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
GU8 Gh621xModuleSendDataViaUart(GU8 uchString[], GU8 uchLength);

/**
 * @fn     GU8 Gh621xModuleSendDataViaSpp(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via spp
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
GU8 Gh621xModuleSendDataViaSpp(GU8 uchString[], GU8 uchLength);

/**
 * @fn     void Gh621xModuleHandleGoodixCommCmd(EMGh621xUprotocolParseCmdType emCmdType)
 *
 * @brief  handle cmd with all ctrl cmd @ref EMGh621xUprotocolParseCmdType
 *
 * @attention   None
 *
 * @param[in]   emCmdType      cmd type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleGoodixCommCmd(EMGh621xUprotocolParseCmdType emCmdType);

/**
 * @fn     void Gh621xModuleHandleIedTkRawdata(uint8_t *puchIedTkRawdata, GU16 usDataLen)
 *
 * @brief  gh621x ied tk rawdata evt handler, @ref protocol docs
 *
 * @attention   None
 *
 * @param[in]   puchIedTkRawdata      pointer to rawdata buffer
 * @param[in]   usDataLen      		  rawdata data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleIedTkRawdata(GU8 *puchIedTkRawdata, GU16 usDataLen);

/**
 * @fn     void Gh621xModuleHandleTkForceEvent(EMGh621xModuleMotionEvent emEvent)
 *
 * @brief  tk/force event handle,@EMGh621xModuleMotionEvent
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleTkForceEvent(EMGh621xModuleMotionEvent emEvent);

/**
 * @fn     void Gh621xModuleHandlePressAndLeaveEvent(EMGh621xModuleMotionEvent emEvent)
 *
 * @brief  press and leave event handle,@EMGh621xModuleMotionEvent
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandlePressAndLeaveEvent(EMGh621xModuleMotionEvent emEvent);

/**
 * @fn     void Gh621xModuleHandleWearEvent(EMGh621xModuleWearStatus emEvent)
 *
 * @brief  wear event handle,@EMGh621xModuleWearStatus
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleWearEvent(EMGh621xModuleWearStatus emEvent);

/**
 * @fn     void Gh621xModuleHandleSamplingDoneData(GU8 *puchSamplingDoneData, GU16 usSamplingDoneDataLen)
 *
 * @brief  gh621x sampling data evt handler
 *
 * @attention   None
 *
 * @param[in]   puchSamplingDoneData      pointer to data buffer
 * @param[in]   usSamplingDoneDataLen     data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleSamplingDoneData(GU8 *puchSamplingDoneData, GU16 usSamplingDoneDataLen);

/**
 * @fn     void Gh621xModuleCommEventReportTimerStart(void)
 *
 * @brief  communicate event report timer start
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerStart(void);

/**
 * @fn     void Gh621xModuleCommEventReportTimerStop(void)
 *
 * @brief  communicate event report timer stop
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerStop(void);  

/**
 * @fn     void Gh621xModuleCommEventReportTimerInit(void)
 *
 * @brief  communicate event report timer init 
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerInit(void);

/**
 * @fn     void Gh621xDebugLog(char *pchLogString)
 *
 * @brief  print dbg log
 *
 * @attention   None
 *
 * @param[in]   pchLogString		pointer to debug log string
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xDebugLog(char *pchLogString);

/**
 * @fn     char *Gh621xModuleGetBluetoothVer(void)
 *
 * @brief  get bluetooth version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetBluetoothVer(void);

/**
 * @fn     char *Gh621xModuleGetHardwareVer(void)
 *
 * @brief  get hardware version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetHardwareVer(void);

/**
 * @fn     char *Gh621xModuleGetFirwarewareVer(void)
 *
 * @brief  get firmware version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetFirwarewareVer(void);

/**
 * @fn     void Gh621xModuleInitHook(void)
 *
 * @brief  init hook
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleInitHook(void);

/**
 * @fn     void Gh621xModuleStartHook(GU8 uchStartType)
 *
 * @brief  start hook
 *
 * @attention   None
 *
 * @param[in]   uchStartType		start type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStartHook(GU8 uchStartType);

/**
 * @fn     void Gh621xModuleStopHook(void)
 *
 * @brief  stop hook
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStopHook(void);

/**
 * @fn     GS8 Gh621xModuleHostCtrl(GU8 uchCtrlType,  GU8 *puchCtrlParam, GU8 uchCtrlParamLen)
 *
 * @brief  handle host ctrl cmd
 *
 * @attention   None
 *
 * @param[in]   uchCtrlType				ctrl type 
 * @param[in]   puchCtrlParam			pointer to ctrl param 
 * @param[in]   uchCtrlParamLen			ctrl param len
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    return error
 */
GS8 Gh621xModuleHostCtrl(GU8 uchCtrlType,  GU8 *puchCtrlParam, GU8 uchCtrlParamLen);

/**
 * @fn     void Gh621xModuleHandleRecvDataViaBle(GU8 *puchData,GU8 uchLength)
 *
 * @brief  handle recv data via ble
 *
 * @attention   call by recv goodix app/tools via ble
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaBle(GU8 *puchData,GU8 uchLength);

/**
 * @fn     void Gh621xModuleIntMsgHandler(void)
 *
 * @brief  gh621x int msg handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleIntMsgHandler(void);

/**
 * @fn     GS8 Gh621xModuleEventReportSchedule(void)
 *
 * @brief  event report schedule via communicate 
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 */
GS8 Gh621xModuleEventReportSchedule(void);

/**
 * @fn     void Gh621xModuleSarEventReport(GU32 unEvent)
 *
 * @brief  sar event report, @ref event report mask
 *
 * @attention   None
 *
 * @param[in]   unEvent             event mask
 * @param[out]  None
 *
 * @return  errcode
 */
void Gh621xModuleSarEventReport(GU32 unEvent);

/**
 * @fn     void Gh621xModuleTkIedEventReport(STGh621xModuleTkIedEventInfo* stEventInfo)
 *
 * @brief  ied tk event report, @ref event report mask
 *
 * @attention   None
 *
 * @param[in]   stEventInfo             event info
 * @param[out]  None
 *
 * @return  errcode
 */ 
void Gh621xModuleTkIedEventReport(STGh621xModuleTkIedEventInfo* stEventInfo);

/**
 * @fn     void Gh621xModuleCommParseHandler(Gh621xModuleSendDataFuncPtrType fnSendDataFunc, GU8 *puchBuffer, GU8 uchLen) 
 *
 * @brief  communicate parse handler
 *
 * @attention   None
 *
 * @param[in]   Gh621xModuleSendDataFuncPtrType     pointer to send data function
 * @param[in]   puchBuffer                          pointer to recv data buffer
 * @param[in]   uchLen                              recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommParseHandler(Gh621xModuleSendDataFuncPtrType fnSendDataFunc, GU8 *puchBuffer, GU8 uchLen);

/**
 * @fn     GU8 Gh621xModuleWriteMainCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  write main calibration list data to flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[in]   puchDataBuffer		pointer to data output 
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleWriteMainCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen);

/**
 * @fn     GU8 Gh621xModuleReadMainCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  read main calibration list data from flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[out]  puchDataBuffer		pointer to data output 
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleReadMainCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen);

/**
 * @fn     GU8 Gh621xModuleWriteBackupCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  write backup calibration list data to flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[in]   puchDataBuffer		pointer to data output 
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleWriteBackupCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen);

/**
 * @fn     GU8 Gh621xModuleReadBackupCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  read backup calibration list data from flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[out]  puchDataBuffer		pointer to data output 
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleReadBackupCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen);

#endif /* _GH3011_EXAMPLE_COMMON_H_ */

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
