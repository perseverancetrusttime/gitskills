/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_drv.h
 *
 * @brief   gh621x driver functions
 *
 * @version ref gh621x_drv_version.h
 *
 * @author  Goodix Iot Team 
 *
 */


#ifndef _GH621X_DRV_H_
#define _GH621X_DRV_H_


/* type redefine */
typedef unsigned char       GU8;    /**< 8bit unsigned integer type */
typedef signed char         GS8;    /**< 8bit signed integer type */
typedef unsigned short      GU16;   /**< 16bit unsigned integer type */
typedef signed short        GS16;   /**< 16bit signed integer type */
typedef long int            GS32;   /**< 32bit signed integer type */
typedef unsigned long int   GU32;   /**< 32bit unsigned integer type */
typedef float               GF32;   /**< float type */
typedef char                GCHAR;  /**< char type */


/**
 * @brief universal protocol cmd enum
 */
typedef enum
{
    GH621X_UPROTOCOL_CMD_STOP = 0x00,                               /**< stop */
    GH621X_UPROTOCOL_CMD_MCU_MODE_START = 0x01,                     /**< mcu mode start */
    GH621X_UPROTOCOL_CMD_APP_MODE_START = 0x02,                     /**< app mode start */
    GH621X_UPROTOCOL_CMD_APP_MODE_WITHOUT_AUTOCC_START = 0x03,      /**< app mode without auto cancel start */
    GH621X_UPROTOCOL_CMD_HOST_MODE_START = 0x04,                    /**< host mode start */
    GH621X_UPROTOCOL_CMD_HOST_MODE_DEBUG_START = 0x05,              /**< host mode debug start */
    GH621X_UPROTOCOL_CMD_EVENT_REPORT_ACK = 0x30,                   /**< event report ack */
    GH621X_UPROTOCOL_CMD_IGNORE = 0xFF,                             /**< ignore cmd type */
} EMGh621xUprotocolParseCmdType;

/**
 * @brief universal protocol host ctrl cmd ctrl type enum
 */
enum EMGh621xUprotocolHostCtrlType
{
    GH621X_UPROTOCOL_HOST_CTRL_TYPE_OTA_BIN_CFG = 0x11,             /**< host ctrl ota bin cfg */
    GH621X_UPROTOCOL_HOST_CTRL_TYPE_RESET = 0x12,                   /**< host reset */
    GH621X_UPROTOCOL_HOST_CTRL_TYPE_CHIP_SELECT = 0x13,             /**< chip select */ 
    GH621X_UPROTOCOL_HOST_CTRL_TYPE_DMIC_ON = 0x20,                 /**< turn on DMIC/SPEAKER */ 
    GH621X_UPROTOCOL_HOST_CTRL_TYPE_DMIC_OFF = 0x21,                /**< turn off DMIC/SPEAKER */ 
};

/**
 * @brief gh621x mcu pd mode enum
 */
typedef enum
{
    GH621X_MCU_PD = 0,       /**< mcu powerdown */
    GH621X_MCU_DONT_PD,      /**< mcu don't powerdown */
} EMGh621xMcuPdMode;

/**
 * @brief gh621x pin level
 */
typedef enum
{
    GH621X_PIN_LEVEL_LOW,        /**< IO pin low level. */
    GH621X_PIN_LEVEL_HIGH,       /**< IO pin high level. */
} EMGh621xModulePinStatus;

/**
 * @brief start hook type enum, val define equal EMGh621xUprotocolParseCmdType, for pfnStartHookFunc
 */
enum EMGh621xStartHookType
{
    GH621X_START_HOOK_MCU_MODE_START = 0x01,                     /**< mcu mode start */
    GH621X_START_HOOK_APP_MODE_START = 0x02,                     /**< app mode start */
    GH621X_START_HOOK_APP_MODE_WITHOUT_AUTOCC_START = 0x03,      /**< app mode without auto cancel start */
    GH621X_START_HOOK_HOST_MODE_START = 0x04,                    /**< host mode start */
    GH621X_START_HOOK_HOST_MODE_DEBUG_START = 0x05,              /**< host mode debug start */
};

/**
 * @brief register struct
 */
typedef struct
{
    GU16 usRegAddr;     /**< register address */
    GU16 usRegData;     /**< register val */
} STGh621xReg;

/**
 * @brief init config struct type
 */
typedef struct
{
    STGh621xReg *pstRegConfigArr;    /**< pointer of register config array */
    GU16        usConfigArrLen;      /**< register config array length */
    GU8*        puchPatchAddr;       /**< patch arr addr */
} STGh621xInitConfig;


/* return code definitions list */
#define   GH621X_RET_RAW_DATA_PKG_OVER              (3)             /**< already read all of rawdata package data */
#define   GH621X_RET_RAW_DATA_PKG_CONTINUE          (2)             /**< there is rawdata package data left */
#define   GH621X_RET_START_SUCCESS                  (1)             /**< start success */
#define   GH621X_RET_OK                             (0)             /**< there is no error */
#define   GH621X_RET_GENERIC_ERROR                  (-1)            /**< a generic error happens */
#define   GH621X_RET_PARAMETER_ERROR                (-2)            /**< parameter error */
#define   GH621X_RET_COMM_NOT_REGISTERED_ERROR      (-3)            /**< i2c operation not registered error */
#define   GH621X_RET_COMM_ERROR                     (-4)            /**< i2c communication error */
#define   GH621X_RET_RESOURCE_ERROR                 (-5)            /**< resource full or not available error */
#define   GH621X_RET_NO_INITED_ERROR                (-6)            /**< no inited error */
#define   GH621X_RET_FUN_NOT_REGISTERED_ERROR       (-7)            /**< related function not registered error */
#define   GH621X_RET_FIRMWARE_LOAD_ERROR            (-8)            /**< gh621x firmware error */
#define   GH621X_RET_ABNORMAL_RESET_ERROR           (-9)            /**< gh621x abnormal reset error */
#define   GH621X_RET_INVALID_IN_MP_MODE_ERROR       (-10)           /**< function invalid in MP mode */
#define   GH621X_RET_FIRMWARE_HEAD_CRC_ERROR        (-11)           /**< gh621x Firmware Head CRC Error */
#define   GH621X_RET_FIRMWARE_HEAD_NO_TYPE_ERROR    (-12)           /**< gh621x Firmware Head do not have Chip Type Info */
#define   GH621X_RET_FIRMWARE_PATCH_MISMATCH_ERROR  (-13)           /**< gh621x Firmware Head Type misMatch with Chip Type Info */
#define   GH621X_RET_HOSTSOFTWAREVER_LENGTH_ERROR   (-14)           /**< host software version length check error */
#define   GH621X_RET_HOSTSOFTWAREVER_LENGTH_OVERRUN (-15)           /**< host software version length check error */

/* byte true & false */
#define   GH621X_BYTE_FLAG_TRUE                      (1)            /**< byte true flag val */
#define   GH621X_BYTE_FLAG_FALSE                     (0)            /**< byte false flag val */

/* irq status mask */
#define   GH621X_IRQ0_MSK_SAR_CHIP_RESET_BIT         (0x0001)       /**< sar chip reset */
#define   GH621X_IRQ0_MSK_SAR_AUTOCC_DONE_BIT        (0x0080)       /**< sar autocc done */
#define   GH621X_IRQ0_MSK_SAR_PROX_CLOSE_BIT         (0x0800)       /**< sar prox close */
#define   GH621X_IRQ0_MSK_SAR_PROX_FAR_BIT           (0x1000)       /**< sar prox far */
#define   GH621X_IRQ0_MSK_SAR_PROX_DONE_BIT          (0x4000)       /**< sar prox done */
#define   GH621X_IRQ0_MSK_SAR_DEFAULT_CFG_DONE_BIT   (0x8000)       /**< sar load default cfg done */
#define   GH621X_IRQ3_MSK_SAR_WAIT_CFG_TIMEOUT_BIT   (0x2000)       /**< sar wait cfg timeout */
#define   GH621X_IRQ3_MSK_SAR_AUTOCC_FAIL_BIT        (0x4000)       /**< sar autocc fail */
#define   GH621X_IRQ3_MSK_SAR_AUTOCC_NOT_FOUND_BIT   (0x8000)       /**< sar autocc not found */

#define   GH621X_IRQ0_MSK_SAMPLING_DONE_BIT          (0x0040)       /**< samping done */

#define   GH621X_IRQ2_MSK_CHIP_RESET_BIT             (0x0001)       /**< chip reset */
#define   GH621X_IRQ2_MSK_PDT_ON_BIT                 (0x0004)       /**< put on */
#define   GH621X_IRQ2_MSK_PDT_OFF_BIT                (0x0008)       /**< put off */
#define   GH621X_IRQ2_MSK_TK_SINGLE_BIT              (0x0010)       /**< touch key single */
#define   GH621X_IRQ2_MSK_TK_DOUBLE_BIT              (0x0020)       /**< touch key double */
#define   GH621X_IRQ2_MSK_TK_UP_BIT                  (0x0040)       /**< touch key up */
#define   GH621X_IRQ2_MSK_TK_DOWN_BIT                (0x0080)       /**< touch key down */
#define   GH621X_IRQ2_MSK_TK_LONG_BIT                (0x0100)       /**< touch key long */
#define   GH621X_IRQ2_MSK_TK_TRIPLE_BIT              (0x0200)       /**< touch key triple */
#define   GH621X_IRQ2_MSK_TK_RIGHT_BIT               (0x0400)       /**< touch key right */
#define   GH621X_IRQ2_MSK_TK_LEFT_BIT                (0x0800)       /**< touch key left */
#define   GH621X_IRQ2_MSK_TK_CLOCK_BIT               (0x1000)       /**< touch key clock */
#define   GH621X_IRQ2_MSK_TK_ANTICLOCK_BIT           (0x2000)       /**< touch key anticlock */
#define   GH621X_IRQ2_MSK_TK_RELEASE_BIT             (0x4000)       /**< touch key release */
#define   GH621X_IRQ2_MSK_FORCE_PRESS_BIT            (0x8000)       /**< force touch press */
#define   GH621X_IRQ3_MSK_FORCE_LEAVE_BIT            (0x0004)       /**< force touch leave */
#define   GH621X_IRQ3_MSK_RAWDATA_READY_BIT          (0x0400)       /**< rawdata is ready */
#define   GH621X_IRQ3_MSK_HEART_BEAT_BIT             (0x0100)       /**< heart beat event */

#define   GH621X_IRQ2_MSK_TK                         (GH621X_IRQ2_MSK_TK_SINGLE_BIT | GH621X_IRQ2_MSK_TK_DOUBLE_BIT | \
                                                    GH621X_IRQ2_MSK_TK_UP_BIT | GH621X_IRQ2_MSK_TK_DOWN_BIT | \
                                                    GH621X_IRQ2_MSK_TK_LONG_BIT | GH621X_IRQ2_MSK_TK_TRIPLE_BIT | \
                                                    GH621X_IRQ2_MSK_TK_RIGHT_BIT | GH621X_IRQ2_MSK_TK_LEFT_BIT | \
                                                    GH621X_IRQ2_MSK_TK_CLOCK_BIT | GH621X_IRQ2_MSK_TK_ANTICLOCK_BIT | \
                                                    GH621X_IRQ2_MSK_TK_RELEASE_BIT)      /**< touch key event mask */

/* event report mask */
#define   GH621X_SAR_EVT_MSK_CHIP_RESET_BIT          (0x00000001)       /**< sar chip reset event */
#define   GH621X_SAR_EVT_MSK_AUTOCC_FAIL_BIT         (0x00000010)       /**< sar autocc fail event*/
#define   GH621X_SAR_EVT_MSK_AUTOCC_NOT_FOUND_BIT    (0x00000020)       /**< sar autocc not found event */
#define   GH621X_SAR_EVT_MSK_PROX_CLOSE_BIT          (0x00010000)       /**< sar prox close event */
#define   GH621X_SAR_EVT_MSK_PROX_FAR_BIT            (0x00020000)       /**< sar prox far event */
#define   GH621X_SAR_EVT_MSK_ALL_BIT                 (0xFFFFFFFF)       /**< sar all event */

/* bit operation macro */
#define   GH621X_CHECK_BIT_SET(x, b)                 (((x) & (b)) == (b))  /**< macro of check bits set */
#define   GH621X_CHECK_BIT_NOT_SET(x, b)             (((x) & (b)) != (b))  /**< macro of check bits not set */
#define   GH621X_CHECK_BIT_SET_BITMASK(x, m, b)      (((x) & (m)) == (b))  /**< macro of check bits set with bitmask */
#define   GH621X_CHECK_BIT_NOT_SET_BITMASK(x, m, b)  (((x) & (m)) != (b))  /**< macro of check bits not set */

/// define i2c 7bits addr default list, only valid in function
#define   GH621X_I2C_7BITS_LIST_DEFAULT_VAR_DEF(var) \
                GU8  (var)[8]; \
                Gh621xI2c7BitsAddrListSetDefault((var), 0)

/* function ptr typedef */

typedef GU8 (*pfnI2cWrite)(GU8 uchDeviceId, const GU8 uchWriteBytesArr[], 
                                GU16 usWriteLen);                                   /**< i2c write type */
typedef GU8 (*pfnI2cRead)(GU8 uchDeviceId, const GU8 uchCmddBytesArr[], 
                        GU16 usCmddLen, GU8 uchReadBytesArr[], GU16 usMaxReadLen);  /**< i2c read type */
typedef void (*pfnDelayMs)(GU16 usMsec);                                            /**< delay ms type */
typedef void (*pfnNormalHookFunc)(void);                                            /**< normal hook type */
typedef void (*pfnStartHookFunc)(GU8 uchHookStartType);                             /**< start hook type */
typedef GU8 (*pfnReadPinLevel)(void);                                               /**< read pin level type */
typedef void (*pfnSetPinLevel)(void);                                               /**< set pin high/low level type */
typedef GCHAR* (*pfnGetVersion)(void);                                              /**< get version function type */
typedef GS8 (*pfnUprotocolHostCtrl)(GU8 uchCtrlType, GU8 *puchParam, 
                                    GU8 uchParamLen);                               /**< uprotocol host ctrl */

typedef GU8 (*pfnCalibrationListWrite)(GU8 *uchCalListArr, GU16 usWriteLen);        /**< calibration list write */
typedef GU8 (*pfnCalibrationListRead)(GU8 *uchReadBytesArr, GU16 usReadLen);        /**< calibration list read */

/**
 * @fn     void Gh621xRegisterHookFunc(pfnNormalHookFunc pInitHookFunc, 
 *                                      pfnStartHookFunc pStartHookFunc,
 *                                      pfnNormalHookFunc pStopHookFunc)
 *
 * @brief  Register hook function callback
 *
 * @attention   None
 *
 * @param[in]   pInitHookFunc           pointer to init hook function 
 * @param[in]   pStartHookFunc          pointer to start hook function
 * @param[in]   pStopHookFunc           pointer to stop hook function
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xRegisterHookFunc(pfnNormalHookFunc pInitHookFunc, 
                             pfnStartHookFunc pStartHookFunc,
                             pfnNormalHookFunc pStopHookFunc);

/**
 * @fn     void Gh621xRegisterDelayMsCallback(pfnDelayMs pPlatformDelayMsFunc)
 *
 * @brief  Register delay ms callback function
 *
 * @attention   None
 *
 * @param[in]   pPlatformDelayMsFunc      pointer to delay ms function
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xRegisterDelayMsCallback(pfnDelayMs pPlatformDelayMsFunc);

/**
 * @fn     void Gh621xDelayMs(GU16 usMsCnt)
 *
 * @brief  Delay N Ms for gh621x
 *
 * @attention   None
 *
 * @param[in]   usMsCnt     count need to delay(Ms)
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xDelayMs(GU16 usMsCnt);

/**
 * @fn     void Gh621xWriteReg(GU16 usRegAddr, GU16 usRegValue)
 *
 * @brief  Write register via i2c
 *
 * @attention   None
 *
 * @param[in]   usRegAddr       register addr
 * @param[in]   usRegValue      register data
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xWriteReg(GU16 usRegAddr, GU16 usRegValue);

/**
 * @fn     GU16 Gh621xReadReg(GU16 usRegAddr)
 *
 * @brief  Read register via i2c
 *
 * @attention   None
 *
 * @param[in]   usRegAddr      register addr
 * @param[out]  None
 *
 * @return  register data
 */
GU16 Gh621xReadReg(GU16 usRegAddr);

/**
 * @fn     void Gh621xReadRegs(GU16 usRegAddr, GU16 *pusRegValueBuffer, GU16 usLen)
 *
 * @brief  Read multi register via i2c
 *
 * @attention   usLen set 1~32767 valid
 *
 * @param[in]   usRegAddr           register addr
 * @param[in]   pusRegValueBuffer   pointer to register value buffer
 * @param[in]   usLen               registers len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xReadRegs(GU16 usRegAddr, GU16 *pusRegValueBuffer, GU16 usLen);

/**
 * @fn     void Gh621xWriteRegs(GU16 usRegAddr, GU16 *pusRegValueBuffer, GU16 usLen)
 *
 * @brief  Read multi register via i2c
 *
 * @attention   usLen set 1~32767 valid
 *
 * @param[in]   usRegAddr           register addr
 * @param[in]   pusRegValueBuffer   pointer to register value buffer
 * @param[in]   usLen               registers len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xWriteRegs(GU16 usRegAddr, GU16 *pusRegValueBuffer, GU16 usLen);

/**
 * @fn     void Gh621xWriteBigEndianBufferWithAddr(GU8 *puchRegAddrAndValueArr, GU16 usLen)
 *
 * @brief  Write big-endian data via i2c
 *
 * @attention   puchRegAddrAndValueArr data must have addr and value, and data is big-endian
 *
 * @param[in]   puchRegAddrAndValueArr   pointer to register addr and value buffer(big-endian)
 * @param[in]   usLen                    array len in byte
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xWriteBigEndianBufferWithAddr(GU8 *puchRegAddrAndValueArr, GU16 usLen);

/**
 * @fn     void Gh621xWriteRegBitField(GU16 usRegAddr, GU8 uchLsb, GU8 uchMsb, GU16 usValue)
 *
 * @brief  write register bit field via i2c
 *
 * @attention   None
 *
 * @param[in]   usRegAddr      register addr
 * @param[in]   uchLsb         lsb of bit field
 * @param[in]   uchMsb         msb of bit field
 * @param[in]   usRegValue     register data
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xWriteRegBitField(GU16 usRegAddr, GU8 uchLsb, GU8 uchMsb, GU16 usValue);

/**
 * @fn     GU16 Gh621xReadRegBitField(GU16 usRegAddr, GU8 uchLsb, GU8 uchMsb)
 *
 * @brief  Read register bit field via i2c
 *
 * @attention   None
 *
 * @param[in]   usRegAddr      register addr
 * @param[in]   uchLsb         lsb of bit field
 * @param[in]   uchMsb         msb of bit field
 * @param[out]  None
 *
 * @return  register bit field data
 */
GU16 Gh621xReadRegBitField(GU16 usRegAddr, GU8 uchLsb, GU8 uchMsb);

/**
 * @fn     GS8 Gh621xRegisterI2cOperationFunc(pfnI2cWrite pI2cWriteFunc, pfnI2cRead pI2cReadFunc)
 *
 * @brief  Register i2c operation function callback
 *
 * @attention   None
 *
 * @param[in]   pI2cWriteFunc               pointer to i2c write function
 * @param[in]   pI2cReadFunc                pointer to i2c read function
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_PARAMETER_ERROR  i2c operation function pointer parameter error
 */
GS8 Gh621xRegisterI2cOperationFunc(pfnI2cWrite pI2cWriteFunc, pfnI2cRead pI2cReadFunc);

/**
 * @fn     void Gh621xSetI2cDeviceId(GU8 uchDeviceId)
 *
 * @brief  set i2c device id in 7-bits
 *
 * @attention   None
 *
 * @param[in]   uchDeviceId             i2c device id 7-bits
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xSetI2cDeviceId(GU8 uchDeviceId);

/**
 * @fn     GU8 Gh621xI2c7BitsAddrListSetDefault(GU8 uchI2cAddrList[8], GU8 uchSetParam)
 *
 * @brief  set i2c 7bits addr list default 
 *
 * @attention   None
 *
 * @param[in]   uchSetParam         i2c addr set param
 * @param[out]  uchI2cAddrList      pointer to i2c addr list output
 *
 * @return  i2c addr list output len 
 */
GU8 Gh621xI2c7BitsAddrListSetDefault(GU8 uchI2cAddrList[8], GU8 uchSetParam);

/**
 * @fn     GS8 Gh621xCheckI2cAddrValid(GU8 *puchI2cAddrList, GU8 uchI2cAddrListLen, GU8 *puchValidAddr)
 *
 * @brief  check i2c addr valid
 *
 * @attention   should use after register i2c.
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_COMM_ERROR       return communicate error, no addr valid in list
 */
GS8 Gh621xCheckI2cAddrValid(GU8 *puchI2cAddrList, GU8 uchI2cAddrListLen, GU8 *puchValidAddr);

/**
 * @fn     void Gh621xSetMpModeFlag(GU8 uchMpModeFlag)
 *
 * @brief  set gh621x mp mode flag
 *
 * @attention   None
 *
 * @param[in]   uchMpModeFlag           mp mode flag, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xSetMpModeFlag(GU8 uchMpModeFlag);

/**
 * @fn     void Gh621xSetDbgModeFlag(GU8 uchDbgModeFlag)
 *
 * @brief  set gh621x dbg mode flag
 *
 * @attention   None
 *
 * @param[in]   uchDbgModeFlag           dbg mode flag, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xSetDbgModeFlag(GU8 uchDbgModeFlag);

/**
 * @fn     GU8 Gh621xIsSystemInDbgMode(void)
 *
 * @brief  get gh621x dbg mode flag
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  dbg mode flag, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 */
GU8 Gh621xIsSystemInDbgMode(void);

/**
 * @fn     void Gh621xResetStatusAndWorkMode(void)
 *
 * @brief  reset status and workmode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xResetStatusAndWorkMode(void);

/**
 * @fn     void Gh621xSetMcuModeRawdataEnable(GU8 uchEnable)
 *
 * @brief  set gh621x mcu mode rawdata enable flag
 *
 * @attention   None
 *
 * @param[in]   uchEnable           mcu mode rawdata enable flag, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xSetMcuModeRawdataEnable(GU8 uchEnable);

/**
 * @fn     GS8 Gh621xExitLowPowerMode(void)
 *
 * @brief  Exit lowpower mode, in this mode, can read&write reg with gh621x
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    exit low power mode error
 */
GS8 Gh621xExitLowPowerMode(void);

/**
 * @fn     GS8 Gh621xEnterLowPowerMode(void)
 *
 * @brief  Enter lowpower mode, in this mode, cann't read&write reg with gh621x
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    enter low power mode error
 */
GS8 Gh621xEnterLowPowerMode(void);

/**
 * @fn     GS8 Gh621xEnableNosleepMode(void)
 *
 * @brief  enable nosleep mode, gh621x will never into sleep
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xEnableNosleepMode(void);

/**
 * @fn     GS8 Gh621xDisableNosleepMode(void)
 *
 * @brief  disable nosleep mode, gh621x will auto sleep if mcu run
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xDisableNosleepMode(void);

/**
 * @fn     GS8 Gh621xActiveConfirm(void)
 *
 * @brief  check gh621x is active
 *
 * @attention   should use when irq detected
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    return generic error, that is inactive
 */
GS8 Gh621xActiveConfirm(void);

/**
 * @fn     GU8 Gh621xGetAbnormalResetStatus(void)
 *
 * @brief  get abnormal reset status
 *
 * @attention   use @ irq reset status detect
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  reset status
 * @retval  #GH621X_BYTE_FLAG_TRUE          abnormal reset
 * @retval  #GH621X_BYTE_FLAG_FALSE         isn't abnormal reset 
 */
GU8 Gh621xGetAbnormalResetStatus(void);

/**
 * @fn     GS8 Gh621xSetSarDataReadDone(GU8 uchReadDoneWaitEnable)
 *
 * @brief  set sar mode data has read done
 *
 * @attention   None
 *
 * @param[in]   uchReadDoneWaitEnable       wait mode enable, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xSetSarDataReadDone(GU8 uchReadDoneWaitEnable);

/**
 * @fn     void Gh621xRegisterResetPinControlFunc(pfnSetPinLevel pResetPinHighLevelFunc, 
 *                                                  pfnSetPinLevel pResetPinLowLevelFunc)
 *
 * @brief  Register reset pin level control function
 *
 * @attention   None
 *
 * @param[in]   pResetPinHighLevelFunc       pointer to set reset pin high level function
 * @param[in]   pResetPinLowLevelFunc        pointer to set reset pin low level function
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xRegisterResetPinControlFunc(pfnSetPinLevel pResetPinHighLevelFunc, pfnSetPinLevel pResetPinLowLevelFunc);

/**
 * @fn     GS8 Gh621xHardReset(void)
 *
 * @brief  Gh621x hard reset
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    reset pin control function not register
 */
GS8 Gh621xHardReset(void);

/**
 * @fn     GS8 Gh621xAbnormalReset(void)
 *
 * @brief  Gh621x simulating abnormal reset by pulling down reset pin
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK                          return successfully
 * @retval  #GH621X_RET_FUN_NOT_REGISTERED_ERROR    reset pin control function not register
 * @retval  #GH621X_RET_INVALID_IN_MP_MODE_ERROR    in mp mode, should ctrl after exit mp mode
 */
GS8 Gh621xAbnormalReset(void);

/**
 * @fn     GS8 Gh621xSoftReset(void)
 *
 * @brief  Gh621x softreset via i2c
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    exit low power mode error
 */
GS8 Gh621xSoftReset(void);

/**
 * @fn     GS8 Gh621xLoadNewRegConfigArr(STGh621xReg *pstRegConfigArr, GU16 usRegConfigLen)
 *
 * @brief  Load new gh621x reg config array
 *
 * @attention   If reg val don't need verify, should set reg addr bit 12;
 *              If reg is virtual reg, should set reg addr bit 13;
 *              e.g.      need config reg 0x0000: 0x0611
 *                        {0x0000, 0x0611} //verify write by read reg
 *                        {0x1000, 0x0611} //don't need verify write val
 *                        {0x2000, 0x0611} //set virtual reg
 *
 * @param[in]   pstRegConfigArr       pointer to the reg struct array
 * @param[in]   usRegConfigLen        reg struct array length
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_COMM_ERROR       gh621x communicate error
 */
GS8 Gh621xLoadNewRegConfigArr(STGh621xReg *pstRegConfigArr, GU16 usRegConfigLen);

/**
 * @fn     GS8 Gh621xDumpRegs(STGh621xReg *pstDumpRegsArr, GU16 usDumpRegsStartAddr, GU16 usDumpRegsLen)
 *
 * @brief  Dump gh621x regs
 *
 * @attention   usDumpRegsStartAddr only allow even address, if param set odd address val that val & 0xFFFE in api;
 *              If address greater than reg max addres 0xFFFE, it will return GH621X_RET_GENERIC_ERROR.
 *
 * @param[out]  pstDumpRegsArr           pointer to the dump reg struct output
 * @param[in]   usDumpRegsStartAddr     dump reg address
 * @param[in]   usDumpRegsLen           dump reg length
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    dump gh621x address out of bounds
 */
GS8 Gh621xDumpRegs(STGh621xReg *pstDumpRegsArr, GU16 usDumpRegsStartAddr, GU16 usDumpRegsLen);

/**
 * @fn     GS8 Gh621xConfigPatchBlockSize(GU16 usBlockSize)
 *
 * @brief  Config patch block size
 *
 * @attention   If parameter error, block size will use default val
 *
 * @param[in]   usBlockSize        block size(bytes)
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_PARAMETER_ERROR  parameter error
 */
GS8 Gh621xConfigPatchBlockSize(GU16 usBlockSize);

/**
 * @fn     GS8 Gh621xCommunicateConfirm(void)
 *
 * @brief  Communication operation interface confirm
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_COMM_ERROR       gh621x communicate error
 */
GS8 Gh621xCommunicateConfirm(void);

/**
 * @fn     GS8 Gh621xInit(STGh621xInitConfig *pstGh621xInitConfigParam)
 *
 * @brief  Init gh621x with configure parameters
 *
 * @attention   None
 *
 * @param[in]   pstGh621xInitConfigParam      pointer to gh621x init config param
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_COMM_ERROR       gh621x communicate error
 */
GS8 Gh621xInit(STGh621xInitConfig *pstGh621xInitConfigParam);

/**
 * @fn     GS8 Gh621xStartDetect(void)
 *
 * @brief  Start gh621x detect
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK                          return successfully
 * @retval  #GH621X_RET_NO_INITED_ERROR             gh621x has not inited
 * @retval  #GH621X_RET_GENERIC_ERROR               gh621x has started, need restart should call stop then start
 * @retval  #GH621X_RET_INVALID_IN_MP_MODE_ERROR    in mp mode, should ctrl after exit mp mode
 */
GS8 Gh621xStartDetect(void);

/**
 * @fn     GS8 Gh621xStopDetect(GU8 uchMcuPdMode)
 *
 * @brief  Stop gh621x detect
 *
 * @attention   None
 *
 * @param[in]   uchMcuPdMode              gh621x mcu pd mode, config val @ref EMGh621xMcuPdMode
 * @param[out]  None
 * 
 * @return  errcode
 * @retval  #GH621X_RET_OK                  return successfully
 * @retval  #GH621X_RET_NO_INITED_ERROR     gh621x has not inited
 * @retval  #GH621X_RET_INVALID_IN_MP_MODE_ERROR    in mp mode, should ctrl after exit mp mode
 */
GS8 Gh621xStopDetect(GU8 uchMcuPdMode);

/**
 * @fn     GS8 Gh621xForceStopDetectIntoDslp(void)
 *
 * @brief  Stop gh621x detect and into dslp(all function dslp) as no init status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 * 
 * @return  errcode
 * @retval  #GH621X_RET_OK                          return successfully
 * @retval  #GH621X_RET_INVALID_IN_MP_MODE_ERROR    in mp mode, should ctrl after exit mp mode
 */
GS8 Gh621xForceStopDetectIntoDslp(void);

/**
 * @fn     GS8 Gh621xGetIrqStatus(GU16 usIrqStatusArr[4])
 *
 * @brief  Get irq status reg val
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  usIrqStatusArr          irq status, 4 words
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    don't in wakeup status,may be an interference signal @ int pin
 */
GS8 Gh621xGetIrqStatus(GU16 usIrqStatusArr[4]);

/**
 * @fn     GU8 Gh621xGetIedSignalValue(void)
 *
 * @brief  get ied signal value
 *
 * @attention   suggest call this function in the same thread with other operation of gh621x.
 *              When use this function in other thread, don't forget to use mutex.
 *                
 *
 * @param[in]   None
 * @param[out]  psIedS0    signal value of IED channel 0
 * @param[out]  psIedS1    signal value of IED channel 1
 *
 * @return  None
 */
void Gh621xGetIedSignalValue(GS16* psIedS0, GS16* psIedS1);

/**
 * @fn     GCHAR *Gh621xGetDriverLibVersion(void)
 *
 * @brief  Get driver version
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  library version string
 */
GCHAR *Gh621xGetDriverLibVersion(void);

/**
 * @fn     GCHAR *Gh621xGetChipVersion(void)
 *
 * @brief  Get chip version
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  chip version string
 */
GCHAR *Gh621xGetChipVersion(void);

/**
 * @fn     GCHAR *Gh621xGetPatchCodeVersion(void)
 *
 * @brief  Get patch code version
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  patch code version string
 */
GCHAR *Gh621xGetPatchCodeVersion(void);

/**
 * @fn     GS8 Gh621xGetSamplingDoneDataBuffer(GU8 *puchDataBuffer, GU16 *pusRawdataLen)
 *
 * @brief  get sampling done data buffer
 *
 * @attention   None
 *
 * @param[out]  puchDataBuffer   pointer to data buffer
 * @param[out]  pusRawdataLen    pointer to rawdata length                   
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK                      return successfully
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           return generic error, not@dbg mode
 */
GS8 Gh621xGetSamplingDoneDataBuffer(GU8 *puchDataBuffer, GU16 *pusRawdataLen);

/**
 * @fn     GS8 Gh621xTriggerSarAutocc(void)
 *
 * @brief  trigger sar mode autocc
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xTriggerSarAutocc(void);

/**
 * @fn     GS8 Gh621xConfigSarMcuModeIrqEnable(void)
 *
 * @brief  config sar irq enable in mcu mode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xConfigSarMcuModeIrqEnable(void);

/**
 * @fn     GS8 Gh621xConfigSarAppModeIrqEnable(void)
 *
 * @brief  config sar irq enable in app mode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xConfigSarAppModeIrqEnable(void);

/**
 * @fn     GS8 Gh621xGetSarRawdata(GU8 *puchRawdataBuffer, GU16 *pusRawdataLen)
 *
 * @brief  get sar rawdata
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  puchRawdataBuffer       pointer to rawdata buffer
 * @param[out]  pusRawdataLen           pointer to rawdata length                     
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK                      return successfully
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           return generic error, not@app mode or get rawdata disabled@mcu mode
 */
GS8 Gh621xGetSarRawdata(GU8 *puchRawdataBuffer, GU16 *pusRawdataLen);

/**
 * @fn     GS8 Gh621xUprotocolReportSarRawdata(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
 *                                       GU8 *puchRawdataBuffer, GU16 usRawdataLen)
 *
 * @brief  report rawdata via protocol
 *
 * @attention   must call after Gh621xGetSarRawdata, rawdata format by Gh621xGetSarRawdata
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   puchRawdataBuffer       pointer to rawdata buffer
 * @param[in]   usRawdataLen            rawdata length                     
 *
 * @return  errcode
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           report rawdata is disabled, need enable it
 * @retval  #GH621X_RET_RAW_DATA_PKG_OVER       return already read all the package data 
 * @retval  #GH621X_RET_RAW_DATA_PKG_CONTINUE   return should read rawdata package data again
 */
GS8 Gh621xUprotocolReportSarRawdata(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
                                        GU8 *puchRawdataBuffer, GU16 usRawdataLen);

/**
 * @fn     GS8 Gh621xUprotocolReportSarEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, GU32 unEvent)
 *
 * @brief  report sar event
 *
 * @attention   none
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   unEvent                 event need report, @ref event report mask
 * 
 * @return  error code
 * @retval  #GH621X_RET_GENERIC_ERROR   couldn't report event, last event unfinished
 * @retval  #GH621X_RET_OK              could report event
 */
GS8 Gh621xUprotocolReportSarEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, GU32 unEvent);

/**
 * @fn     GS8 Gh621xConfigTkIedMcuModeIrqEnable(void)
 *
 * @brief  config tk ied irq enable in mcu mode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xConfigTkIedMcuModeIrqEnable(void);

/**
 * @fn     GS8 Gh621xConfigTkIedAppModeIrqEnable(GU8 uchNeedAutoCancelFlag)
 *
 * @brief  config tk ied irq enable in app mode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 */
GS8 Gh621xConfigTkIedAppModeIrqEnable(GU8 uchNeedAutoCancelFlag);

/**
 * @fn     GS8 Gh621xGetTkIedRawdata(GU8 *puchRawdataBuffer, GU16 *pusRawdataLen, GU8 *puchExData, GU8 uchExDataLen)
 *
 * @brief  get rawdata of afe1,refresh rate,and afe2,pack them to rawdata buf
 *
 * @attention   None
 *
 * @param[in]   puchExData              pointer to extra data buffer that need to upload
 * @param[in]   uchExDataLen            length of extra data buffer in byte
 * @param[out]  puchRawdataBuffer       pointer to rawdata buffer
 * @param[out]  pusRawdataLen           pointer to rawdata length                     
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK                      return successfully
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           return generic error, not@app mode or get rawdata disabled@mcu mode
 */
GS8 Gh621xGetTkIedRawdata(GU8 *puchRawdataBuffer, GU16 *pusRawdataLen, GU8 *puchExData, GU8 uchExDataLen);

/**
 * @fn     GS8 Gh621xUprotocolReportTkIedRawdata(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
 *                                                  GU8 *puchRawdataBuffer, GU16 usRawdataLen, GU8 uchExtraDataLen)
 *
 * @brief  report rawdata via protocol
 *
 * @attention   must call after Gh621xGetTkIedRawdata
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   puchRawdataBuffer       pointer to rawdata buffer
 * @param[in]   usRawdataLen            rawdata length           
 * @param[in]   uchExtraDataLen         extra data length   
 *
 * @return  errcode
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           report rawdata is disabled, need enable it
 * @retval  #GH621X_RET_RAW_DATA_PKG_OVER       return already read all the package data 
 * @retval  #GH621X_RET_RAW_DATA_PKG_CONTINUE   return should read rawdata package data again
 */
GS8 Gh621xUprotocolReportTkIedRawdata(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
                                        GU8 *puchRawdataBuffer, GU16 usRawdataLen, GU8 uchExtraDataLen);

/**
 * @fn     GS8 Gh621xUprotocolReportTkIedEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, GU32 unEvent)
 *
 * @brief  report tk ied event
 *
 * @attention   none
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   unEvent                 event need report, @ref event report mask
 * 
 * @return  error code
 * @retval  #GH621X_RET_GENERIC_ERROR   couldn't report event, last event unfinished
 * @retval  #GH621X_RET_OK              could report event
 */
GS8 Gh621xUprotocolReportTkIedEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, GU32 unEvent);

/**
 * @fn     GS8 Gh621xUprotocolReportSamplingData(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
                                        GU8 *pusDataBuffer, GU16 usDataRegLen)
 *
 * @brief  report sampling data via protocol
 *
 * @attention   must call after Gh621xGetSamplingDoneDataBuffer
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   pusDataBuffer           pointer to data buffer
 * @param[in]   usDataRegLen            data length                     
 *
 * @return  errcode
 * @retval  #GH621X_RET_PARAMETER_ERROR         return param error
 * @retval  #GH621X_RET_GENERIC_ERROR           report rawdata is disabled, need enable it
 * @retval  #GH621X_RET_RAW_DATA_PKG_OVER       return already read all the package data 
 * @retval  #GH621X_RET_RAW_DATA_PKG_CONTINUE   return should read rawdata package data again
 */
GS8 Gh621xUprotocolReportSamplingData(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
                                        GU8 *pusDataBuffer, GU16 usDataRegLen);

/**
 * @fn     void Gh621xRegisterReadPinStatusFunc(pfnReadPinLevel pReadIntPinLevelFunc,
 *                                    pfnReadPinLevel pReadResetPinLevelFunc,
 *                                     pfnReadPinLevel pReadSintPinLevelFunc)
 *
 * @brief  Register read pin status function
 *
 * @attention   Only use for debug, read gh621x some pin status can debug some hardware errors;
 *              Pin status return should define as 1 [high level] or 0 [low level];
 *
 * @param[in]   pReadIntPinLevelFunc       pointer to read int pin level function
 * @param[in]   pReadResetPinLevelFunc     pointer to read reset pin level function
 * @param[in]   pReadSintPinLevelFunc      pointer to read spcs pin level function
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xRegisterReadPinStatusFunc(pfnReadPinLevel pReadIntPinLevelFunc,
                                      pfnReadPinLevel pReadResetPinLevelFunc,
                                      pfnReadPinLevel pReadSintPinLevelFunc);

/**
 * @fn     GS8 Gh621xGetResetPinLevel(void)
 *
 * @brief  get reset pin level
 *
 * @attention   None;
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pin level or error code
 * @retval  #0                                      low level
 * @retval  #1                                      high level
 * @retval  #GH621X_RET_FUN_NOT_REGISTERED_ERROR    func not registered 
 */
GS8 Gh621xGetResetPinLevel(void);

/**
 * @fn     GS8 Gh621xGetIntPinLevel(void)
 *
 * @brief  get int pin level
 *
 * @attention   None;
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pin level or error code
 * @retval  #0                                      low level
 * @retval  #1                                      high level
 * @retval  #GH621X_RET_FUN_NOT_REGISTERED_ERROR    func not registered 
 */
GS8 Gh621xGetIntPinLevel(void);

/**
 * @fn     GS8 Gh621xGetSintPinLevel(void)
 *
 * @brief  get sint pin level
 *
 * @attention   None;
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pin level or error code
 * @retval  #0                                      low level
 * @retval  #1                                      high level
 * @retval  #GH621X_RET_FUN_NOT_REGISTERED_ERROR    func not registered 
 */
GS8 Gh621xGetSintPinLevel(void);

/**
 * @fn     void Gh621xRegisterGetFirmwareVersionFunc(pfnGetVersion pGetFirmwareVersionFunc)
 *
 * @brief  Register get firmware version function
 *
 * @attention   None
 *
 * @param[in]    pGetFirmwareVersionFunc    pointer to get firmware versionn function
 * @param[out]   None
 *
 * @return  None
 */
void Gh621xRegisterGetFirmwareVersionFunc(pfnGetVersion pGetFirmwareVersionFunc);

/**
 * @fn     void Gh621xRegisterGetHardwareVersionFunc(pfnGetVersion pGetHardwareVersionFunc)
 *
 * @brief  Register get hardware version function
 *
 * @attention   None
 *
 * @param[in]    pGetHardwareVersionFunc    pointer to get hardware versionn function
 * @param[out]   None
 *
 * @return  None
 */
void Gh621xRegisterGetHardwareVersionFunc(pfnGetVersion pGetHardwareVersionFunc);

/**
 * @fn     void Gh621xRegisterGetBluetoothVersionFunc(pfnGetVersion pGetBluetoothVersionFunc)
 *
 * @brief  Register get bluetooth version function
 *
 * @attention   None
 *
 * @param[in]    pGetBluetoothVersionFunc    pointer to get bluetooth versionn function
 * @param[out]   None
 *
 * @return  None
 */
void Gh621xRegisterGetBluetoothVersionFunc(pfnGetVersion pGetBluetoothVersionFunc);

/**
 * @fn     GS8 Gh621xUprotocolSetPatchUpdateCmdBufferPtr(GU8 *puchPatchBuffer, GU16 usPatchBufferLen)
 *
 * @brief  set patch update cmd buffer ptr
 *
 * @attention   None
 *
 * @param[in]    puchPatchBuffer    pointer to patch update buffer
 * @param[in]    usPatchBufferLen   patch update buffer len
 * @param[out]   None
 *
 * @return  None
 */
GS8 Gh621xUprotocolSetPatchUpdateCmdBufferPtr(GU8 *puchPatchBuffer, GU16 usPatchBufferLen);

/**
 * @fn     GS8 Gh621xUprotocolSetCaliParamRwCmdBufferPtr(GU8 *puchCaliParamBuffer, GU16 usCaliParamBufferLen)
 *
 * @brief  set cali param cmd buffer ptr
 *
 * @attention   None
 *
 * @param[in]    puchCaliParamBuffer    pointer to cali param buffer
 * @param[in]    usCaliParamBufferLen   cali param buffer len
 * @param[out]   None
 *
 * @return  None
 */
GS8 Gh621xUprotocolSetCaliParamRwCmdBufferPtr(GU8 *puchCaliParamBuffer, GU16 usCaliParamBufferLen);

/**
 * @fn     GS8 Gh621xUprotocolSetCfgLoadRegListInfoCmdBufferPtr(GU8 *puchSaveLoadRegListBuffer, GU16 usSaveLoadRegListBufferLen)
 *
 * @brief  set cfg load reg list cmd buffer ptr
 *
 * @attention   None
 *
 * @param[in]    puchSaveLoadRegListBuffer    pointer to cali param buffer
 * @param[in]    usSaveLoadRegListBufferLen   cali param buffer len
 * @param[out]   None
 *
 * @return  None
 */
GS8 Gh621xUprotocolSetCfgLoadRegListInfoCmdBufferPtr(GU8 *puchSaveLoadRegListBuffer, GU16 usSaveLoadRegListBufferLen);

/**
 * @fn     void Gh621xRegisterHostCtrlFunc(pfnUprotocolHostCtrl pHostCtrlFunc)
 *
 * @brief  Register host ctrl function
 *
 * @attention   pfnUprotocolHostCtrl uchCtrlType @ref EMGh621xUprotocolHostCtrlType
 *
 * @param[in]    pHostCtrlFunc    pointer to host ctrl function, @ref pfnUprotocolHostCtrl
 * @param[out]   None
 *
 * @return  None
 */
void Gh621xRegisterHostCtrlFunc(pfnUprotocolHostCtrl pHostCtrlFunc);

/**
 * @fn     EMGh621xUprotocolParseCmdType Gh621xUprotocolParseHandler(GU8 *puchRespondBuffer, GU16 *pusRespondLen,
 *                                                     GU8 *puchRecvDataBuffer, GU16 usRecvLen)
 *
 * @brief  universal protocol parse handler, parse protocol receive data
 *
 * @attention   None
 *
 * @param[out]   puchRespondBuffer    pointer to respond buffer
 * @param[out]   pusRespondLen        pointer to respond length
 * @param[in]    puchRecvDataBuffer   pointer to receive data buffer
 * @param[in]    usRecvLen            receive data len
 *
 * @return  parse cmd type @ref EMGh621xUprotocolParseCmdType
 */
EMGh621xUprotocolParseCmdType Gh621xUprotocolParseHandler(GU8 *puchRespondBuffer, GU16 *pusRespondLen,
                                                     GU8 *puchRecvDataBuffer, GU16 usRecvLen);

/**
 * @fn     GU8 Gh621xUprotocolPacketMaxLenConfig(GU8 uchPacketMaxLen)
 *
 * @brief  packet max len support, default len is 243
 *
 * @attention   uchPacketMaxLen val must at [20, 243]
 *
 * @param[out]   None
 * @param[in]    uchPacketMaxLen      packet max len
 *
 * @return  packet max len after config
 */
GU8 Gh621xUprotocolPacketMaxLenConfig(GU8 uchPacketMaxLen);

/**
 * @fn     GS8 Gh621xUprotocolReportEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
 *                               GU8 uchEventType, GU8 uchEventSize, GU32 unEvent)
 *
 * @brief  report event
 *
 * @attention   none
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * @param[in]   uchEventType            report event type
 * @param[in]   uchEventSize            report event size
 * @param[in]   unEvent                 event need report
 * 
 * @return  error code
 * @retval  #GH621X_RET_GENERIC_ERROR   couldn't report event, last event unfinished
 * @retval  #GH621X_RET_OK              could report event
 */
GS8 Gh621xUprotocolReportEvent(GU8 *puchRespondBuffer, GU16 *pusRespondLen, 
                                GU8 uchEventType, GU8 uchEventSize, GU32 unEvent);

/**
 * @fn     GS8 Gh621xUprotocolReportEventSched(GU8 *puchRespondBuffer, GU16 *pusRespondLen)
 *
 * @brief  report event sched
 *
 * @attention   none
 *
 * @param[out]  puchRespondBuffer       pointer to protocol pkg data.
 * @param[out]  pusRespondLen           pointer to protocol pkg data length
 * 
 * @return  error code
 * @retval  #GH621X_RET_GENERIC_ERROR   couldn't report event, last event unfinished
 * @retval  #GH621X_RET_OK              could report event
 */
GS8 Gh621xUprotocolReportEventSched(GU8 *puchRespondBuffer, GU16 *pusRespondLen);

/**
 * @fn     void Gh621xRegisterMainCalibrationListOperationFunc(pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc)
 *
 * @brief  Register main calibration list read/write operation function
 *
 * @attention   None
 *
 * @param[in]   pCaliListWriteFunc  main calibration list read operation function
 * @param[in]   pCaliListReadFunc   main calibration list write operation function
 * @param[out]  None
 *
 * @return  None
 */    
void Gh621xRegisterMainCalibrationListOperationFunc(pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc);

/**
 * @fn     void Gh621xRegisterBackupCalibrationListOperationFunc(pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc)
 *
 * @brief  Register main calibration list read/write operation function
 *
 * @attention   None
 *
 * @param[in]   pCaliListWriteFunc  backup calibration list read operation function
 * @param[in]   pCaliListReadFunc   backup calibration list write operation function
 * @param[out]  None
 *
 * @return  None
 */    
void Gh621xRegisterBackupCalibrationListOperationFunc(pfnCalibrationListWrite pCaliListWriteFunc, pfnCalibrationListRead pCaliListReadFunc);

/**
 * @fn     GU8 Gh621xIsSystemInMpMode(void)
 *
 * @brief  get gh621x mp mode flag
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  mp mode flag, @ref GH621X_BYTE_FLAG_TRUE & GH621X_BYTE_FLAG_FALSE
 */
GU8 Gh621xIsSystemInMpMode(void);

/**
 * @fn     GU8 Gh621xUprotocolReportEventClear(void)
 *
 * @brief  clear all event 
 *
 * @attention   none
 *
 * @param[out]  None
 * @param[in]   None
 * 
 * @return  errcode
 * @retval  #GH621X_RET_OK         return successfully
 */
GS8 Gh621xUprotocolReportEventClear(void);

/**
 * @fn     void Gh621xSetOffsetAbnormalThreshold(GU16 threshold[2])
 *
 * @brief  Set offset abnormal threshold
 *
 * @attention   None
 *
 * @param[in]   threshold    offset abnormal threshold12
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xSetOffsetAbnormalThreshold(GS32 threshold0, GS32 threshold1);

/**
 * @fn     void Gh621xSetSelfCalibError(GS32 uchErrorS0, GS32 uchErrorS1)
 *
 * @brief  set Self Calibration Error Thresh
 *
 * @attention   None
 *
 * @param[in]   uchErrorS0 uchErrorS1
 * @param[out]  void
 *
 * @return  None
 */
void Gh621xSetSelfCalibError(GS32 uchErrorS0, GS32 uchErrorS1);

/**
 * @fn     void Gh621xSetSelfCalibBoxDiff(GS32 uchBoxDiffS0, GS32 uchBoxDiffS1)
 *
 * @brief  Set Self Calibration Box Diff
 *
 * @attention   None
 *
 * @param[in]   GS32 uchBoxDiffS0, GS32 uchBoxDiffS1, box Diff in 2 IED Channel
 * @param[out]  void
 *
 * @return  None
 */
void Gh621xSetSelfCalibBoxDiff(GS32 uchBoxDiffS0, GS32 uchBoxDiffS1);

/**
 * @fn     void Gh621xSetOffsetLearnFactor(GU8 uchPosFactor, GU8 uchNegFactor)
 *
 * @brief  Set Offset Learn iir Filter Denominator
 *
 * @attention   None
 *
 * @param[in]   GU8 uchPosFactor, GU8 uchNegFactor,pos and neg iir filter
 * @param[out]  void
 *
 * @return  None
 */
void Gh621xSetOffsetLearnFactor(GU8 uchPosFactor, GU8 uchNegFactor);

/**
 * @fn     void Gh621xSetSelfCalibFullOffsetLearnCount(GU16 uchCount)
 *
 * @brief  Set Offset Full Self learn Count
 *
 * @attention   None
 *
 * @param[in]   Offset Full Self learn Count
 * @param[out]  void
 *
 * @return  None
 */
void Gh621xSetSelfCalibFullOffsetLearnCount(GU16 uchCount);

/**
 * @fn     void Gh621xSelfCaliInChargingBox(void)
 *
 * @brief  Offset Self Calibration In Charging Box
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  void
 *
 * @return  None
 */
void Gh621xSelfCaliInChargingBox(void);

#endif /* _GH621X_DRV_H_ */

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
