/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_example_process.c
 *
 * @brief   example code for gh621x
 *
 */

#include "gh621x_drv.h"
#include "gh621x_example_common.h"
#include "gh621x_example.h"
#if (__EXAMPLE_PATCH_LOAD_DEFAULT_ENABLE__)
#include __EXAMPLE_PATCH_INCLUDE_FILE_NAME__
#endif


/// start retry count
GU8 uchGh621xModuleStartRetryCnt = 0;

#if (__UPOROTOCOL_PATCH_UPDATE_BUFFER_SUP_LEN__ > 0)

/// patch buffer
GU8 uchGh621xModulePatchBuffer[__UPOROTOCOL_PATCH_UPDATE_BUFFER_SUP_LEN__] = {0};

#endif

#if (__UPOROTOCOL_CALI_PARAM_RW_BUFFER_SUP_LEN__ > 0)

/// cali param buffer
GU8 uchGh621xModuleCaliParamRwBuffer[__UPOROTOCOL_CALI_PARAM_RW_BUFFER_SUP_LEN__] = {0};

#endif

#if (__UPOROTOCOL_SAVE_LOAD_REG_LIST_BUFFER_SUP_LEN__ > 0)

/// save load reg list buffer
GU8 uchGh621xModuleSaveLoadRegListBuffer[__UPOROTOCOL_SAVE_LOAD_REG_LIST_BUFFER_SUP_LEN__] = {0};

#endif

/// last send data func ptr 
Gh621xModuleSendDataFuncPtrType pfnGh621xModuleLastSendDataFuncPtr = NULL;

/// handle flag
GU8 usGh621xModuleHandlingLockFlag = 0;

/// enable rawdata flag in reset exception 
GU8 uchGh621xModuleGetRawdataFlagInResetException = GH621X_BYTE_FLAG_FALSE;

/// gh621x chip i2c check addr list
const GU8 uchGh621xModuleChipI2cAddrCheckArr[] = {0x09, 0x08, 0x17, 0x16, 0x3D, 0x3C, 0x19, 0x18};

/// event info
STGh621xModuleTkIedEventInfo stGh621xModuleEventInfo = {0};

#if (__UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__)

/// send mutli pkg index
static GU8 uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;

/// send mutli pkg buffer
static GU8 uchGh621xModuleReportRawdataPkgMutliPkgBuffer[__UPOROTOCOL_COMMM_PKG_SIZE_MAX__ + 1];

#endif

extern GU8 g_uchGh621xStatus;
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
GS8 Gh621xModuleInit(void)
{ 
    static GU8 uchFirstInitFlag = GH621X_BYTE_FLAG_TRUE; // first init status flag
    GS8 chInitErrFlag = GH621X_RET_OK; // init error flag
    GU8 uchValidI2cAddr = 0;
    STGh621xInitConfig stInitConfigParam = {stGh621xModuleDefaultRegConfigArray, usGh621xModuleDefaultRegConfigArrayLen, GH621X_DEFAULT_PATCH_VAR}; // param

    /* log all version string */
    EXAMPLE_DEBUG_LOG_L1(GH621X_EXAMPLE_VER_STRING);
    EXAMPLE_DEBUG_LOG_L1("GH621X_DRV_%s\r\n", Gh621xGetDriverLibVersion());
	Gh621xModuleGetBluetoothVer();

    /* config protocol */
    Gh621xUprotocolPacketMaxLenConfig(__UPOROTOCOL_COMMM_PKG_SIZE_MAX__);
    Gh621xRegisterGetBluetoothVersionFunc(Gh621xModuleGetBluetoothVer);
    Gh621xRegisterGetFirmwareVersionFunc(Gh621xModuleGetFirwarewareVer);
    Gh621xRegisterGetHardwareVersionFunc(Gh621xModuleGetHardwareVer);
    Gh621xRegisterHostCtrlFunc(Gh621xModuleHostCtrl);
    #if (__UPOROTOCOL_PATCH_UPDATE_BUFFER_SUP_LEN__ > 0)
    Gh621xUprotocolSetPatchUpdateCmdBufferPtr(uchGh621xModulePatchBuffer, __UPOROTOCOL_PATCH_UPDATE_BUFFER_SUP_LEN__);
    #endif
    #if (__UPOROTOCOL_CALI_PARAM_RW_BUFFER_SUP_LEN__ > 0)
    Gh621xUprotocolSetCaliParamRwCmdBufferPtr(uchGh621xModuleCaliParamRwBuffer, 
                                                __UPOROTOCOL_CALI_PARAM_RW_BUFFER_SUP_LEN__);
    #endif
    #if (__UPOROTOCOL_SAVE_LOAD_REG_LIST_BUFFER_SUP_LEN__ > 0)
    Gh621xUprotocolSetCfgLoadRegListInfoCmdBufferPtr(uchGh621xModuleSaveLoadRegListBuffer, 
                                                __UPOROTOCOL_SAVE_LOAD_REG_LIST_BUFFER_SUP_LEN__);
    #endif

	Gh621xRegisterMainCalibrationListOperationFunc(Gh621xModuleWriteMainCalibrationListToFlash, Gh621xModuleReadMainCalibrationListFromFlash);
	Gh621xRegisterBackupCalibrationListOperationFunc(Gh621xModuleWriteBackupCalibrationListToFlash, Gh621xModuleReadBackupCalibrationListFromFlash);

    /* config hook */
    Gh621xRegisterHookFunc(Gh621xModuleInitHook, Gh621xModuleStartHook, Gh621xModuleStopHook);

    if (uchFirstInitFlag == GH621X_BYTE_FLAG_TRUE)
    {
        Gh621xModuleHalI2cInit(); // i2c init

        Gh621xRegisterI2cOperationFunc(Gh621xModuleHalI2cWrite, Gh621xModuleHalI2cRead); // register i2c RW func api

        #if (__PLATFORM_DELAY_FUNC_CONFIG__)
        Gh621xRegisterDelayMsCallback(Gh621xModuleHalDelayMs);
        #endif

        #if (__PLATFORM_RESET_PIN_FUNC_CONFIG__)
        Gh621xModuleHalResetPinCtrlInit();
        Gh621xRegisterResetPinControlFunc(Gh621xModuleHalResetPinSetHighLevel, Gh621xModuleHalResetPinSetLowLevel);
        #endif

        if (GH621X_RET_OK != Gh621xCheckI2cAddrValid((GU8 *)uchGh621xModuleChipI2cAddrCheckArr, 
                                    sizeof(uchGh621xModuleChipI2cAddrCheckArr), &uchValidI2cAddr))
        {
            EXAMPLE_DEBUG_LOG_L1("gh621x module i2c addr set error, need set valid addr\r\n");
            return GH621X_EXAMPLE_ERR_VAL;
        }
        EXAMPLE_DEBUG_LOG_L1("gh621x module i2c addr valid:0x%x\r\n", uchValidI2cAddr);
    }

    if (uchFirstInitFlag == GH621X_BYTE_FLAG_FALSE) // if not first init, disable int before gh621x init
    {
        Gh621xModuleHalIntDetectDisable();
    }
	//get chipid add czh 20201221
	 gh621x_get_chipid(uchValidI2cAddr);
	
    Gh621xSetOffsetAbnormalThreshold(50000, 50000); //if abs(min offset - factory offset) > 20000, stop self calibration
    Gh621xSetSelfCalibError(0, 0);              //if abs(min offset - current offset) < 0, stop self calibration
    Gh621xSetSelfCalibBoxDiff(0, 0);        //charging box diff / 2
    Gh621xSetOffsetLearnFactor(4, 8);           //if min offset > current offset,learn 1/4; if min offset < current offset,learn 1/8
    Gh621xSetSelfCalibFullOffsetLearnCount(20); //first 20 times self calibration 100% learn minoffset
	chInitErrFlag = Gh621xInit(&stInitConfigParam); // init gh621x
    if (uchFirstInitFlag == GH621X_BYTE_FLAG_FALSE) // if not first init, disable int before gh621x init
    {
        Gh621xModuleHalIntDetectReenable();
    }
    if (GH621X_RET_OK != chInitErrFlag)  
	{
        EXAMPLE_DEBUG_LOG_L1("gh621x module init error, err_code:%d\r\n", chInitErrFlag);
    	return GH621X_EXAMPLE_ERR_VAL;
	}
	
    if (uchFirstInitFlag == GH621X_BYTE_FLAG_TRUE)
    {
        Gh621xModuleHalIntInit(); // gh621x int pin init
        Gh621xModuleCommEventReportTimerInit(); // init communicate event report
    }
	/* register getting pin status func */
    Gh621xRegisterReadPinStatusFunc(Gh621xModuleHalGetIntPinStatus, Gh621xModuleHalGetResetPinStatus, NULL);
	//add czh 20210106
	Gh621xModuleFlashInit();

    EXAMPLE_DEBUG_LOG_L1("gh621x module init ok\r\n");
    uchFirstInitFlag = GH621X_BYTE_FLAG_FALSE;
    return GH621X_EXAMPLE_OK_VAL;  
}

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
void Gh621xModuleStart(GU8 uchGetRawdataEnable)
{
    GS8 chRet;
    Gh621xSetMcuModeRawdataEnable(uchGetRawdataEnable);
    chRet = Gh621xStartDetect();
    if (chRet == GH621X_RET_NO_INITED_ERROR)
    {
        chRet = Gh621xModuleInit(); // retry one times
        if (chRet == GH621X_RET_OK)
        {
            chRet = Gh621xStartDetect();
        }
    }
    EXAMPLE_DEBUG_LOG_L1("gh621x module start:%d, enable flag:%d\r\n", chRet, uchGetRawdataEnable);
}

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
void Gh621xModuleStartWithRefreshRate(EMGh621xRawdataRefreshRate emRefreshRate)
{
    GS8 chRet;
    Gh621xSetMcuModeRawdataEnable(1);

    Gh621xExitLowPowerMode();
    switch(emRefreshRate)
    {
        case GH621X_RAWDATA_REF_RATE_100HZ:
            Gh621xWriteReg(0x2ce, 0x5a01);
            break;
        
        case GH621X_RAWDATA_REF_RATE_50HZ:
            Gh621xWriteReg(0x2ce, 0x5a03);
            break;
        
        case GH621X_RAWDATA_REF_RATE_20HZ:
            Gh621xWriteReg(0x2ce, 0x5a09);
            break;

        case GH621X_RAWDATA_REF_RATE_5HZ:
            Gh621xWriteReg(0x2ce, 0x5a27);
            break;

        default:
            Gh621xWriteReg(0x2ce, 0x5a27);
            break;
    } 
    Gh621xReadReg(0x2ce);
    
    chRet = Gh621xStartDetect();
    if (chRet == GH621X_RET_NO_INITED_ERROR)
    {
        chRet = Gh621xModuleInit(); // retry one times
        if (chRet == GH621X_RET_OK)
        {
            Gh621xExitLowPowerMode();
            switch(emRefreshRate)
            {
                case GH621X_RAWDATA_REF_RATE_100HZ:
                    Gh621xWriteReg(0x2ce, 0x5a01);
                    break;
                
                case GH621X_RAWDATA_REF_RATE_50HZ:
                    Gh621xWriteReg(0x2ce, 0x5a03);
                    break;
                
                case GH621X_RAWDATA_REF_RATE_20HZ:
                    Gh621xWriteReg(0x2ce, 0x5a09);
                    break;

                case GH621X_RAWDATA_REF_RATE_5HZ:
                    Gh621xWriteReg(0x2ce, 0x5a27);
                    break;

                default:
                    Gh621xWriteReg(0x2ce, 0x5a27);
                    break;
            }   
            Gh621xReadReg(0x2ce);
            
            chRet = Gh621xStartDetect();
        }
    }
    EXAMPLE_DEBUG_LOG_L1("gh621x module start:%d, enable flag:%d\r\n", chRet, 1);
}


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
void Gh621xModuleStop(GU8 uchMcuPdFlag)
{
    GS8 chRet = Gh621xStopDetect(uchMcuPdFlag);
    if (chRet == GH621X_RET_INVALID_IN_MP_MODE_ERROR)
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x module in mp mode\r\n");
    }
    EXAMPLE_DEBUG_LOG_L1("gh621x module stop:%d\r\n", chRet);
}

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
void Gh621xModuleForcePowerDown(void)
{
    GS8 chRet = Gh621xForceStopDetectIntoDslp();
    if (chRet == GH621X_RET_INVALID_IN_MP_MODE_ERROR)
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x module in mp mode\r\n");
    }
    EXAMPLE_DEBUG_LOG_L1("gh621x module force stop:%d\r\n", chRet);
}

/**
 * @fn     void Gh621xModuleHandleResetToInit(void)
 *
 * @brief  reset to init status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleResetToInit(void)
{
    GU8 uchAbnormalResetRetryCnt = 0;

    Gh621xResetStatusAndWorkMode();
	Gh621xModuleSarEventReport(GH621X_SAR_EVT_MSK_CHIP_RESET_BIT);
    Gh621xModuleHalIntDetectDisable(); // disable int detect
    do 
    {
        EXAMPLE_DEBUG_LOG_L1("uchAbnormalResetRetryCnt:%d\r\n", uchAbnormalResetRetryCnt);
        if (Gh621xInit(NULL) == GH621X_RET_OK)
        {
            break;
        }
        else // if init error, retry reset
        {
            if (Gh621xHardReset() != GH621X_RET_OK)
            {
                Gh621xSoftReset();
            }
            Gh621xDelayMs(30);
        }
    } while ((uchAbnormalResetRetryCnt ++) < __ABNORMAL_RESET_RETRY_CNT_MAX__);
    Gh621xModuleHalIntDetectReenable(); // reenable int detect
}

/**
 * @fn     void Gh621xModuleHandleResetException(void)
 *
 * @brief  handle gh621x exception
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleResetException(void)
{
    Gh621xModuleHandleResetToInit();
    Gh621xModuleStart(uchGh621xModuleGetRawdataFlagInResetException);
	EXAMPLE_DEBUG_LOG_L1("gh621x starting in Exception Handler!\r\n");
}

/**
 * @fn     void Gh621xModuleSarResetEvtHandler(void)
 *
 * @brief  gh621x sar reset evt handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleSarResetEvtHandler(void)
{
    GU8 uchResetStatus = GH621X_BYTE_FLAG_TRUE;
    if (Gh621xIsSystemInDbgMode() == GH621X_BYTE_FLAG_FALSE)
    {
        uchResetStatus = Gh621xGetAbnormalResetStatus();
        EXAMPLE_DEBUG_LOG_L1("gh621x sar reset evt, reset state:%d\r\n", uchResetStatus);
        
        if (uchResetStatus == GH621X_BYTE_FLAG_TRUE)
        {
            uchGh621xModuleStartRetryCnt = 0;
            Gh621xModuleHandleResetException(); // handle reset by exception
        }
        else
        {
            if (Gh621xIsSystemInMpMode() == GH621X_BYTE_FLAG_FALSE)
            {
                Gh621xModuleHandleResetToInit(); // handle reset to init status
            }
            else
            {
                Gh621xResetStatusAndWorkMode();
                Gh621xModuleSarEventReport(GH621X_SAR_EVT_MSK_CHIP_RESET_BIT);
            }
        }
    }
    else
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x sar reset evt, in dbg mode!\r\n");
        Gh621xResetStatusAndWorkMode();
    }
}

/**
 * @fn     void Gh621xModuleTkIedRawdataEvtHandler(void)
 *
 * @brief  gh621x tk/ied rawdata evt handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleTkIedRawdataEvtHandler(STGh621xModuleTkIedEventInfo* pstEventInfo)
{
    GU8 uchRawdataBuffer[GH621X_EXAMPLE_TK_IED_RAWDATA_MAX_LEN] = {0};
    GU16 usRawdataLen = 0;
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    GS8 chReportPacketStatus = GH621X_RET_RAW_DATA_PKG_OVER;
    GU32 uiEventData = 0xFFFFFFFF;

    if (NULL != pstEventInfo)
    {
        uiEventData = ((pstEventInfo->uchTempInfo << 24) | (pstEventInfo->emForceInfo << 16) | \
                        (pstEventInfo->emTouchInfo << 8) | (pstEventInfo->emWearInfo));
    }

    if (GH621X_RET_OK == Gh621xGetTkIedRawdata(uchRawdataBuffer, &usRawdataLen, (GU8*)&uiEventData, sizeof(uiEventData)))
    {
        //EXAMPLE_DEBUG_LOG_L1("gh621x tk/ied rawdata evt, rawdata len:%d\r\n", usRawdataLen);
        do 
        {
            chReportPacketStatus = Gh621xUprotocolReportTkIedRawdata(uchRespondBuffer, &uchRespondLen, 
                                                                 uchRawdataBuffer, usRawdataLen, sizeof(uiEventData));
            #if (__UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__)
            if (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_CONTINUE)
            {
                if (pfnGh621xModuleLastSendDataFuncPtr != NULL)
                {
                    pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
                }
                uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
            }
            else if (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_OVER)
            {
                GU16 usNewPkgOfBufferIndex = ((GU16)uchGh621xModuleReportRawdataPkgMutliPkgIndex) * uchRespondLen;
                memcpy(&uchGh621xModuleReportRawdataPkgMutliPkgBuffer[usNewPkgOfBufferIndex], uchRespondBuffer,
                        uchRespondLen);
                usNewPkgOfBufferIndex = usNewPkgOfBufferIndex + uchRespondLen; // add two len, for compare
                uchGh621xModuleReportRawdataPkgMutliPkgIndex ++;
                if (((usNewPkgOfBufferIndex + uchRespondLen) > __UPOROTOCOL_COMMM_PKG_SIZE_MAX__) // if not enough
                  || (uchGh621xModuleReportRawdataPkgMutliPkgIndex > __UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__))
                {
                    if (pfnGh621xModuleLastSendDataFuncPtr != NULL)
                    {
                        pfnGh621xModuleLastSendDataFuncPtr(uchGh621xModuleReportRawdataPkgMutliPkgBuffer, usNewPkgOfBufferIndex);
                    }
                    uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
                }
            }
            else
            {
                uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
                EXAMPLE_DEBUG_LOG_L1("report sar rawdata error:%d\r\n", chReportPacketStatus);
            }
            #else
            if ((uchRespondLen != 0) && (pfnGh621xModuleLastSendDataFuncPtr != NULL))
            {
                pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
            }
            else
            {
                EXAMPLE_DEBUG_LOG_L1("report ied/tk rawdata error:%d\r\n", chReportPacketStatus);
            }
            #endif
        } while (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_CONTINUE);
        if (0/*to be done*/) //only when temp data chaged, report temp event
        {
            stGh621xModuleEventInfo.uchTempInfo = 0;
            stGh621xModuleEventInfo.uchNewEventFlag = 1;
        }        

        Gh621xModuleHandleIedTkRawdata(uchRawdataBuffer, usRawdataLen);
    }
    else
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x ied/tk rawdata evt,param error\r\n");
    }
}

/**
 * @fn     void Gh621xModuleSamplingDoneEvtHandler(void)
 *
 * @brief  gh621x sampling done evt handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleSamplingDoneEvtHandler(void)
{
    GU8 uchSamplingDoneDataBuffer[GH621X_EXAMPLE_SAR_RAWDATA_MAX_LEN] = {0};
    GU16 usSamplingDoneDataLen = 0;
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    GS8 chReportPacketStatus = GH621X_RET_RAW_DATA_PKG_OVER;

    if (GH621X_RET_OK == Gh621xGetSamplingDoneDataBuffer(uchSamplingDoneDataBuffer, &usSamplingDoneDataLen))
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x samping don data evt, data len:%d\r\n", usSamplingDoneDataLen);
        do 
        {
            chReportPacketStatus = Gh621xUprotocolReportSamplingData(uchRespondBuffer, &uchRespondLen, 
                                                    uchSamplingDoneDataBuffer, usSamplingDoneDataLen); // packet
            #if (__UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__)
            if (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_CONTINUE)
            {
                if (pfnGh621xModuleLastSendDataFuncPtr != NULL)
                {
                    pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
                }
                uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
            }
            else if (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_OVER)
            {
                GU16 usNewPkgOfBufferIndex = ((GU16)uchGh621xModuleReportRawdataPkgMutliPkgIndex) * uchRespondLen;
                memcpy(&uchGh621xModuleReportRawdataPkgMutliPkgBuffer[usNewPkgOfBufferIndex], uchRespondBuffer,
                        uchRespondLen);
                usNewPkgOfBufferIndex = usNewPkgOfBufferIndex + uchRespondLen; // add two len, for compare
                uchGh621xModuleReportRawdataPkgMutliPkgIndex ++;
                if (((usNewPkgOfBufferIndex + uchRespondLen) > __UPOROTOCOL_COMMM_PKG_SIZE_MAX__) // if not enough
                  || (uchGh621xModuleReportRawdataPkgMutliPkgIndex > __UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__))
                {
                    if (pfnGh621xModuleLastSendDataFuncPtr != NULL)
                    {
                        pfnGh621xModuleLastSendDataFuncPtr(uchGh621xModuleReportRawdataPkgMutliPkgBuffer, usNewPkgOfBufferIndex);
                    }
                    uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
                }
            }
            else
            {
                uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
                EXAMPLE_DEBUG_LOG_L1("report sar rawdata error:%d\r\n", chReportPacketStatus);
            }
            #else
            if ((uchRespondLen != 0) && (pfnGh621xModuleLastSendDataFuncPtr != NULL))
            {
                pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
            }
            else
            {
                EXAMPLE_DEBUG_LOG_L1("report samping don data error:%d\r\n", chReportPacketStatus);
            }
            #endif
        } while (chReportPacketStatus == GH621X_RET_RAW_DATA_PKG_CONTINUE);
        Gh621xModuleHandleSamplingDoneData(uchSamplingDoneDataBuffer, usSamplingDoneDataLen); // handle data by app layer
    }
    else
    {
        EXAMPLE_DEBUG_LOG_L1("gh621x samping don data evt,param error\r\n");
    }
}

/**
 * @fn     void Gh621xModuleTkEvtHandler(GU16 usIrqStatusArr[4])
 *
 * @brief  gh621x tk evt handler
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleTkEvtHandler(GU16 usIrqStatusArr[4])
{
    stGh621xModuleEventInfo.uchNewEventFlag = 1;
    switch (usIrqStatusArr[2] & GH621X_IRQ2_MSK_TK)
    {
        case GH621X_IRQ2_MSK_TK_SINGLE_BIT:
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_ONE_TAP;
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_SINGLE event\n");
            break;
        case GH621X_IRQ2_MSK_TK_DOUBLE_BIT:
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_DOUBLE_TAP;
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_DOUBLE event\n");
            break;
        case GH621X_IRQ2_MSK_TK_UP_BIT:
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_UP;
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_UP event\n");
            break;
        case GH621X_IRQ2_MSK_TK_DOWN_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_DOWN event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_DOWN;
            break;
        case GH621X_IRQ2_MSK_TK_LONG_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_LONG event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_LONG_PRESS;
            break;
        case GH621X_IRQ2_MSK_TK_TRIPLE_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_TRIPLE event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_TRIPLE_TAP;
            break;
        case GH621X_IRQ2_MSK_TK_RIGHT_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_RIGHT event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_RIGHT;
            break;
        case GH621X_IRQ2_MSK_TK_LEFT_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_LEFT event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_LEFT;
            break;
        case GH621X_IRQ2_MSK_TK_CLOCK_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_CLOCK event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_CLOCK;
            break;
        case GH621X_IRQ2_MSK_TK_ANTICLOCK_BIT:
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_ANTICLOCK event\n");
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_ANTICLOCK;
            break;
        case GH621X_IRQ2_MSK_TK_RELEASE_BIT:
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_RELEASE;
			EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_TK_RELEASE event\n");
            break;
        default:
            stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_DEFAULT;
            break;
    }
	Gh621xModuleHandleTkForceEvent(stGh621xModuleEventInfo.emTouchInfo);
}

/**
 * @fn     void Gh621xModuleForceEvtHandler(GU16 usIrqStatusArr[4])
 *
 * @brief  gh621x force evt handler
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleForceEvtHandler(GU16 usIrqStatusArr[4])
{
    stGh621xModuleEventInfo.uchNewEventFlag = 1;
    if (GH621X_CHECK_BIT_SET(usIrqStatusArr[2], GH621X_IRQ2_MSK_FORCE_PRESS_BIT))
    {
    	EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ2_MSK_FORCE_PRESS event\n");
        stGh621xModuleEventInfo.emForceInfo = GH621X_MOTION_LONG_PRESS;
    }
    else if (GH621X_CHECK_BIT_SET(usIrqStatusArr[3], GH621X_IRQ3_MSK_FORCE_LEAVE_BIT))
    {
    	EXAMPLE_DEBUG_LOG_L1("GH621X_IRQ3_MSK_FORCE_LEAVE event\n");
        stGh621xModuleEventInfo.emForceInfo = GH621X_MOTION_RELEASE;
    }
	Gh621xModuleHandlePressAndLeaveEvent(stGh621xModuleEventInfo.emForceInfo);
}

/**
 * @fn     void Gh621xModuleWearEvtHandler(GU16 usIrqStatusArr[4])
 *
 * @brief  gh621x wear evt handler
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleWearEvtHandler(GU16 usIrqStatusArr[4])
{
    stGh621xModuleEventInfo.uchNewEventFlag = 1;
    if (GH621X_CHECK_BIT_SET(usIrqStatusArr[2], GH621X_IRQ2_MSK_PDT_ON_BIT))
    {
        stGh621xModuleEventInfo.emWearInfo = GH621X_WEAR_IN;
    }
    else if (GH621X_CHECK_BIT_SET(usIrqStatusArr[2], GH621X_IRQ2_MSK_PDT_OFF_BIT))
    {
        stGh621xModuleEventInfo.emWearInfo = GH621X_WEAR_OUT;
    }
	Gh621xModuleHandleWearEvent(stGh621xModuleEventInfo.emWearInfo);
}

/**
 * @fn     void Gh621xModuleEventInfoInit(void)
 *
 * @brief  gh621x event info init
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleEventInfoInit(void)
{
    stGh621xModuleEventInfo.emWearInfo  = GH621X_WEAR_STATUS_DEFAULT;
    stGh621xModuleEventInfo.emTouchInfo = GH621X_MOTION_DEFAULT;
    stGh621xModuleEventInfo.emForceInfo = GH621X_MOTION_DEFAULT;
    stGh621xModuleEventInfo.uchTempInfo  = 0xFF;
}

/**
 * @fn     GU8 Gh621xModuleIsTkEvent(GU16 usIrqStatusArr[])
 *
 * @brief  gh621x check if it's tk event
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  flag
 */
GU8 Gh621xModuleIsTkEvent(GU16 usIrqStatusArr[])
{
    return (GH621X_CHECK_BIT_NOT_SET_BITMASK(usIrqStatusArr[2], GH621X_IRQ2_MSK_TK, 0));
}
 
/**
 * @fn     GU8 Gh621xModuleIsForceEvent(GU16 usIrqStatusArr[])
 *
 * @brief  gh621x check if it's force event
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  flag
 */
GU8 Gh621xModuleIsForceEvent(GU16 usIrqStatusArr[])
{
    return ((GH621X_CHECK_BIT_SET(usIrqStatusArr[2], GH621X_IRQ2_MSK_FORCE_PRESS_BIT)) || \
                (GH621X_CHECK_BIT_SET(usIrqStatusArr[3], GH621X_IRQ3_MSK_FORCE_LEAVE_BIT)));
}

/**
 * @fn     GU8 Gh621xModuleIsWearEvent(GU16 usIrqStatusArr[])
 *
 * @brief  gh621x check if it's wear event
 *
 * @attention   None
 *
 * @param[in]   usIrqStatusArr      irq status array
 * @param[out]  None
 *
 * @return  flag
 */
GU8 Gh621xModuleIsWearEvent(GU16 usIrqStatusArr[])
{
    return (GH621X_CHECK_BIT_NOT_SET_BITMASK(usIrqStatusArr[2], \
                                                (GH621X_IRQ2_MSK_PDT_ON_BIT | GH621X_IRQ2_MSK_PDT_OFF_BIT), 0));
}

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
void Gh621xModuleIntMsgHandler(void)
{
	if (usGh621xModuleHandlingLockFlag == 0)
	{
        GU16 usGh621xIrqStatus[4] = {0};
        if (Gh621xActiveConfirm() == GH621X_RET_OK) // if confirm is active
        {
            Gh621xEnableNosleepMode(); // enable no sleep
            if (Gh621xGetIrqStatus(usGh621xIrqStatus) == GH621X_RET_OK) // if get irq status ok
            {
                EXAMPLE_DEBUG_LOG_L1("gh621x got int msg!status:%.4x,%.4x,%.4x,%.4x\r\n", 
                                usGh621xIrqStatus[0], usGh621xIrqStatus[1], usGh621xIrqStatus[2], usGh621xIrqStatus[3]);

                if (GH621X_CHECK_BIT_NOT_SET(usGh621xIrqStatus[0], GH621X_IRQ2_MSK_CHIP_RESET_BIT))
                {
                    Gh621xModuleEventInfoInit();
                    if (Gh621xModuleIsTkEvent(usGh621xIrqStatus))
                    {
                        Gh621xModuleTkEvtHandler(usGh621xIrqStatus);
                    }
                    if (Gh621xModuleIsForceEvent(usGh621xIrqStatus))
                    {
                        Gh621xModuleForceEvtHandler(usGh621xIrqStatus);
                    }
                    if (Gh621xModuleIsWearEvent(usGh621xIrqStatus))
                    {
                        Gh621xModuleWearEvtHandler(usGh621xIrqStatus);
                    }

                    //rawdata bit
                    if (GH621X_CHECK_BIT_SET(usGh621xIrqStatus[3], GH621X_IRQ3_MSK_RAWDATA_READY_BIT))
                    {
                        Gh621xModuleTkIedRawdataEvtHandler(&stGh621xModuleEventInfo);
                    }
                    else if (GH621X_CHECK_BIT_SET(usGh621xIrqStatus[0], GH621X_IRQ0_MSK_SAMPLING_DONE_BIT)) // sampling done
                    {
                        Gh621xModuleSamplingDoneEvtHandler();
                    }

                    Gh621xModuleTkIedEventReport(&stGh621xModuleEventInfo);
                    #if (__UPOROTOCOL_REPORT_EVENT_RESEND_ENABLE__ == 0)
                    Gh621xUprotocolReportEventClear();
                    #endif
                }
                else
                {
                    Gh621xModuleSarResetEvtHandler(); // chip reset, handler with reset mode
                }
            }
            Gh621xDisableNosleepMode(); // disable no sleep
        }
        else
        {
            /* clear all irq status */
            Gh621xEnableNosleepMode(); // enable no sleep
            Gh621xGetIrqStatus(usGh621xIrqStatus); 
            Gh621xDisableNosleepMode(); // disable no sleep
        }
    }
	Gh621xModuleHalIntDetectReenable();
}

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
void Gh621xModuleCommParseHandler(Gh621xModuleSendDataFuncPtrType fnSendDataFunc, GU8 *puchBuffer, GU8 uchLen) 
{
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    Gh621xModuleHalIntDetectDisable();
    usGh621xModuleHandlingLockFlag = 1;
    EXAMPLE_DEBUG_LOG_L2("recv len:%d\r\n", uchLen);
    EMGh621xUprotocolParseCmdType emCmdType = Gh621xUprotocolParseHandler(uchRespondBuffer, &uchRespondLen, puchBuffer, uchLen);
    if ((uchRespondLen != 0) && (fnSendDataFunc != NULL))
    {
        pfnGh621xModuleLastSendDataFuncPtr = fnSendDataFunc; // save last send data ptr
        fnSendDataFunc(uchRespondBuffer, uchRespondLen);
        if ((emCmdType == GH621X_UPROTOCOL_CMD_APP_MODE_START) 
         || (emCmdType == GH621X_UPROTOCOL_CMD_APP_MODE_WITHOUT_AUTOCC_START)) 
        {
            uchGh621xModuleGetRawdataFlagInResetException = GH621X_BYTE_FLAG_TRUE;
        }
        else
        {
            uchGh621xModuleGetRawdataFlagInResetException = GH621X_BYTE_FLAG_FALSE;
        }
        #if (__UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__)
        uchGh621xModuleReportRawdataPkgMutliPkgIndex = 0;
        #endif
    }
    if (emCmdType != GH621X_UPROTOCOL_CMD_IGNORE)
    {
        Gh621xModuleHandleGoodixCommCmd(emCmdType);
    }
    usGh621xModuleHandlingLockFlag = 0;
    Gh621xModuleHalIntDetectReenable();
}

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
GS8 Gh621xModuleEventReportSchedule(void)
{
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    GS8 chRet = Gh621xUprotocolReportEventSched(uchRespondBuffer, &uchRespondLen);
    if ((chRet == GH621X_RET_OK) && (uchRespondLen != 0) && (pfnGh621xModuleLastSendDataFuncPtr != NULL))
    {
        EXAMPLE_DEBUG_LOG_L2("event respond len:%d\r\n", uchRespondLen);
        pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
    }
    return chRet;
}

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
void Gh621xModuleSarEventReport(GU32 unEvent)
{
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    GS8 chRet = GH621X_RET_OK;
    Gh621xModuleCommEventReportTimerStop();
    chRet = Gh621xUprotocolReportSarEvent(uchRespondBuffer, &uchRespondLen, unEvent);
    if ((chRet == GH621X_RET_OK) && (uchRespondLen != 0) && (pfnGh621xModuleLastSendDataFuncPtr != NULL))
    {
        EXAMPLE_DEBUG_LOG_L2("event respond len:%d\r\n", uchRespondLen);
        Gh621xModuleCommEventReportTimerStart();
        pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
    }
}

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
void Gh621xModuleTkIedEventReport(STGh621xModuleTkIedEventInfo* stEventInfo)
{
    GU8 uchRespondBuffer[GH621X_EXAMPLE_PKG_RESPOND_MAX_LEN] = {0};
    GU16 uchRespondLen = 0;
    GS8 chRet = GH621X_RET_OK;
    GU32 unEvent = 0;
    
    if ((NULL == stEventInfo) || (0 == stEventInfo->uchNewEventFlag))
    {
        return;
    }
    stEventInfo->uchNewEventFlag = 0;
    unEvent = ((stEventInfo->emWearInfo << 24) | (stEventInfo->emTouchInfo << 16) | (stEventInfo->emForceInfo << 8) | \
            (stEventInfo->uchTempInfo));
    Gh621xModuleCommEventReportTimerStop();
    chRet = Gh621xUprotocolReportTkIedEvent(uchRespondBuffer, &uchRespondLen, unEvent);
    if ((chRet == GH621X_RET_OK) && (uchRespondLen != 0) && (pfnGh621xModuleLastSendDataFuncPtr != NULL))
    {
        EXAMPLE_DEBUG_LOG_L2("event respond len:%d\r\n", uchRespondLen);
        Gh621xModuleCommEventReportTimerStart();
        pfnGh621xModuleLastSendDataFuncPtr(uchRespondBuffer, uchRespondLen);
    }    
}

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
void Gh621xOffsetSelfCalibrationInChargingBox(void)
{
    //some config
    //Gh621xSetOffsetAbnormalThreshold(20000, 20000); //if abs(min offset - factory offset) > 20000, stop self calibration
    //Gh621xSetSelfCalibError(0, 0);              //if abs(min offset - current offset) < 0, stop self calibration
    //Gh621xSetSelfCalibBoxDiff(200, 200);        //charging box diff / 2
    //Gh621xSetOffsetLearnFactor(4, 8);           //if min offset > current offset,learn 1/4; if min offset < current offset,learn 1/8
    //Gh621xSetSelfCalibFullOffsetLearnCount(20); //first 20 times self calibration 100% learn minoffset
	if(g_uchGh621xStatus == 3)//3:GH621X_STATUS_STOP
	{
	    //or disable host Int
	    Gh621xModuleHalIntDetectDisable();
	    usGh621xModuleHandlingLockFlag = 1;
	    Gh621xSelfCaliInChargingBox();
	    //or Enable host Int
	    usGh621xModuleHandlingLockFlag = 0;
		Gh621xModuleHalIntDetectReenable();
		Gh621xModuleIntMsgHandler();
		//gh621x_adapter_event_process(GH621X_MSG_ID_INT, 0, 0, 0);
	}
}

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
void Gh621xModuleUsingPowerOnOffset(GU8 enablePowerOnOffset)
{
    GU16 temp = 0;
    GU8 i;
    
    Gh621xExitLowPowerMode();
    Gh621xDelayMs(2);
    temp = Gh621xReadReg(0x8f8);
    EXAMPLE_DEBUG_LOG_L2("Read 0x08F8 = %d\r\n", temp);

    if (enablePowerOnOffset)
    {
        // Gh621xWriteReg(0x8f8, temp | 0x0001);
        temp = temp | 0x0001;
    }
    else
    {
        // Gh621xWriteReg(0x8f8, temp & 0xFF00);
        temp = temp & 0xFF00;
    }

    for (i = 0; i < 3; i++)
    {
        Gh621xWriteReg(0x8f8, temp);
        if(temp == Gh621xReadReg(0x8f8))
        {
            EXAMPLE_DEBUG_LOG_L2("Write 0x08F8 = %d Succeed!\r\n", temp);
            break;
        }
        else
        {
            EXAMPLE_DEBUG_LOG_L2("Write 0x08F8 = %d Failed!\r\n", temp);
        }
    }   
}

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
