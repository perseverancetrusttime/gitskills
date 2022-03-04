/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_example.h
 *
 * @brief   example code for gh621x
 *
 */

#ifndef _GH621X_EXAMPLE_H_
#define _GH621X_EXAMPLE_H_


#include "gh621x_example_common.h" 


/// sar prox status enum
enum EMGh621xSarProxStatus
{
    GH621X_SAR_PROX_CLOSE = 0x00,                   /**< prox close */
    GH621X_SAR_PROX_FAR   = 0x01,                   /**< prox far */
};

/// autocc status enum
enum EMGh621xSarAutoccStatus
{
    GH621X_SAR_AUTOCC_DONE = 0x00,                  /**< autocc done */
    GH621X_SAR_AUTOCC_FAIL = 0x01,                  /**< autocc fail */
    GH621X_SAR_AUTOCC_NOT_FOUND = 0x02,             /**< autocc not found */
};

/// rawdata refresh rate enum
typedef enum
{
    GH621X_RAWDATA_REF_RATE_100HZ = 0x00,           /**< refresh rate 100Hz */
    GH621X_RAWDATA_REF_RATE_50HZ = 0x01,            /**< refresh rate 50Hz */
    GH621X_RAWDATA_REF_RATE_20HZ = 0x02,            /**< refresh rate 20Hz */
    GH621X_RAWDATA_REF_RATE_5HZ = 0x03,             /**< refresh rate 5Hz */
} EMGh621xRawdataRefreshRate;


/**
 * @fn     GS8 Gh621xModuleInit(void)
 *
 * @brief  gh621x module init
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      init error
 */
GS8 Gh621xModuleInit(void);

/**
 * @fn     void Gh621xModuleStart(GU8 uchGetRawdataEnable)
 *
 * @brief  gh621x module start
 *
 * @attention   None
 *
 * @param[in]   uchGetRawdataEnable         rawdata output enable flag
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStart(GU8 uchGetRawdataEnable);

/**
 * @fn     void Gh621xModuleStartWithRefreshRate(EMGh621xRawdataRefreshRate emRefreshRate)
 *
 * @brief  gh621x module start
 *
 * @attention   None
 *
 * @param[in]   emRefreshRate         refresh rate
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStartWithRefreshRate(EMGh621xRawdataRefreshRate emRefreshRate);

/**
 * @fn     void Gh621xModuleStop(GU8 uchMcuPdFlag)
 *
 * @brief  gh621x module stop
 *
 * @attention   if powerdown that next start request more time
 *
 * @param[in]   uchMcuPdFlag            mcu powerdown flag, @ref EMGh621xMcuPdMode
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStop(GU8 uchMcuPdFlag);

/**
 * @fn     void Gh621xModuleForcePowerDown(void)
 *
 * @brief  gh621x module force power down
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleForcePowerDown(void);

/**
 * @fn     void Gh621xModuleAutoccTrigger(void)
 *
 * @brief  gh621x module autocc trigger
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleAutoccTrigger(void);

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
 * @fn     void Gh621xModuleHandleRecvDataViaUart(GU8 *puchData, GU8 uchLength)
 *
 * @brief  handle recv data via uart
 *
 * @attention   call by recv goodix app/tools via uart
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaUart(GU8 *puchData, GU8 uchLength);

/**
 * @fn     void Gh621xModuleHandleRecvDataViaSpp(GU8 *puchData, GU8 uchLength)
 *
 * @brief  handle recv data via spp
 *
 * @attention   call by recv goodix app/tools via spp
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaSpp(GU8 *puchData, GU8 uchLength);

/**
 * @fn     void Gh621xOffsetSelfCalibrationInChargingBox(void)
 *
 * @brief  ied offset Self Calibration Function
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xOffsetSelfCalibrationInChargingBox(void);

/**
 * @fn     void Gh621xModuleUsingPowerOnOffset(GU8 enablePowerOnOffset)
 *
 * @brief  whether using power on offset for IED 
 *
 * @attention   call it before Gh621xModuleStart
 *
 * @param[in]   enablePowerOnOffset  0:using flash offset; 1:using power on offset
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleUsingPowerOnOffset(GU8 enablePowerOnOffset);

//get chipid
uint16_t gh621x_get_chipid(GU8 uchValidI2cAddr);
void Gh621xModuleFlashInit(void);

#endif /* _GH621X_EXAMPLE_H_ */

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
