/**
 ****************************************************************************************
 *
 * @file app_tencent_sv_data_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the smart voice data handler
 *
 * Copyright (C) 2017
 *
 *
 ****************************************************************************************
 */
#include "string.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "apps.h"
#include "stdbool.h"
#include "bluetooth.h"
#include "app_tencent_sv_cmd_code.h"
#include "app_tencent_sv_cmd_handler.h"
#include "app_tencent_sv_data_handler.h"
#include "rwapp_config.h"
#include "crc32_c.h"
#include "app_tencent_ble.h"
#include "app_tencent_bt.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "voice_compression.h"

#define APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS  5000

/**
 * @brief smart voice data handling environment
 *
 */
typedef struct {
    uint8_t     isDataXferInProgress;
    uint8_t     isCrcCheckEnabled;
    uint32_t    wholeDataXferLen;
    uint32_t    dataXferLenUntilLastSegVerification;
    uint32_t    currentWholeCrc32;
    uint32_t    wholeCrc32UntilLastSeg;
    uint32_t    crc32OfCurrentSeg;
} APP_TENCENT_SV_DATA_HANDLER_ENV_T;

static APP_TENCENT_SV_DATA_HANDLER_ENV_T app_tencent_sv_data_rec_handler_env;
static APP_TENCENT_SV_DATA_HANDLER_ENV_T app_tencent_sv_data_trans_handler_env;

void app_tencent_sv_data_reset_env(void)
{
    app_tencent_sv_data_rec_handler_env.isDataXferInProgress = false;
    app_tencent_sv_data_rec_handler_env.isCrcCheckEnabled = false;
    app_tencent_sv_data_rec_handler_env.wholeDataXferLen = 0;
    app_tencent_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = 0;
    app_tencent_sv_data_rec_handler_env.currentWholeCrc32 = 0;
    app_tencent_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = 0;
    app_tencent_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;
}


APP_TENCENT_SV_CMD_RET_STATUS_E app_tencent_sv_data_xfer_stop_handler(APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T* pStopXfer)
{
    APP_TENCENT_SV_CMD_RET_STATUS_E retStatus = TENCENT_NO_ERROR;

    if (pStopXfer->isHasWholeCrcCheck) {
        if (pStopXfer->wholeDataLenToCheck != app_tencent_sv_data_rec_handler_env.wholeDataXferLen) {
            retStatus = TENCENT_DATA_TRANSFER_LENGTH_DOSENOT_MATCH;
        }

        if (pStopXfer->crc32OfWholeData != app_tencent_sv_data_rec_handler_env.currentWholeCrc32) {
            retStatus = TENCENT_WHOLE_DATA_CRC_CHECK_FAILED;
        }
    }

    app_tencent_sv_data_reset_env();
    app_tencent_sv_data_rec_handler_env.isDataXferInProgress = false;

    return retStatus;
}

void app_tencent_sv_data_xfer_control_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
   TRACE(0,"app_tencent_sv_data_xfer_control_handler");
    APP_TENCENT_SV_CMD_RET_STATUS_E retStatus = TENCENT_NO_ERROR;

    if (APP_TENCENT_START_DATA_XFER == funcCode) {
        if (app_tencent_sv_data_rec_handler_env.isDataXferInProgress) {
            retStatus = TENCENT_DATA_TRANSFER_HAS_ALREADY_BEEN_STARTED;
        } else {
            if (paramLen < sizeof(APP_TENCENT_SV_START_DATA_XFER_T)) {
                retStatus = TENCENT_PARAM_LEN_TOO_SHORT;
            } else if (paramLen > sizeof(APP_TENCENT_SV_START_DATA_XFER_T)) {
                retStatus = TENCENT_PARAM_LEN_OUT_OF_RANGE;
            } else {
                app_tencent_sv_data_reset_env();
                APP_TENCENT_SV_START_DATA_XFER_T* pStartXfer = (APP_TENCENT_SV_START_DATA_XFER_T *)ptrParam;
                app_tencent_sv_data_rec_handler_env.isDataXferInProgress = true;
                app_tencent_sv_data_rec_handler_env.isCrcCheckEnabled = pStartXfer->isHasCrcCheck;
            }
        }
        APP_TENCENT_SV_START_DATA_XFER_RSP_T rsp = {0};
        app_tencent_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp),device_id);
    } else if (APP_TENCENT_DATA_XFER == funcCode) {
        if (!app_tencent_sv_data_rec_handler_env.isDataXferInProgress) {
            retStatus = TENCENT_DATA_TRANSFER_HAS_NOT_BEEN_STARTED;
        } else {
            if (paramLen < sizeof(APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T)) {
                retStatus = TENCENT_PARAM_LEN_TOO_SHORT;
            } else if (paramLen > sizeof(APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T)) {
                retStatus = TENCENT_PARAM_LEN_OUT_OF_RANGE;
            } else {
                retStatus = app_tencent_sv_data_xfer_stop_handler((APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T *)ptrParam);
            }
        }
        APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_RSP_T rsp = {0};
        app_tencent_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp),device_id);
    } else {
        retStatus = TENCENT_INVALID_COMMAND_CODE;
        app_tencent_sv_send_response_to_command(funcCode, retStatus, NULL, 0,device_id);
    }
}

__attribute__((weak)) void app_tencent_sv_data_xfer_started(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_tencent_sv_kickoff_dataxfer(void)
{
    app_tencent_sv_data_reset_env();
    app_tencent_sv_data_trans_handler_env.isDataXferInProgress = true;
}

void app_tencent_sv_start_data_xfer_control_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    if (TENCENT_NO_ERROR == retStatus) {
        app_tencent_sv_kickoff_dataxfer();
    }
    app_tencent_sv_data_xfer_started(retStatus);
}

__attribute__((weak)) void app_tencent_sv_data_xfer_stopped(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_tencent_sv_stop_dataxfer(void)
{
    app_tencent_sv_data_reset_env();
    app_tencent_sv_data_trans_handler_env.isDataXferInProgress = false;
}

void app_tencent_sv_stAPP_TENCENT_DATA_XFER_control_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    if ((TENCENT_NO_ERROR == retStatus) || (TENCENT_WAITING_RESPONSE_TIMEOUT == retStatus)) {
        app_tencent_sv_stop_dataxfer();
    }
    app_tencent_sv_data_xfer_stopped(retStatus);
}

void app_tencent_sv_data_segment_verifying_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    APP_TENCENT_SV_CMD_RET_STATUS_E retStatus = TENCENT_NO_ERROR;
    if (paramLen < sizeof(APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T)) {
        retStatus = TENCENT_PARAM_LEN_TOO_SHORT;
    } else if (paramLen > sizeof(APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T)) {
        retStatus = TENCENT_PARAM_LEN_OUT_OF_RANGE;
    } else {
        APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T* pVerifyData = (APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T *)ptrParam;
        if (pVerifyData->segmentDataLen !=
            (app_tencent_sv_data_rec_handler_env.wholeDataXferLen -
             app_tencent_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification)) {
            retStatus = TENCENT_DATA_TRANSFER_LENGTH_DOSENOT_MATCH;
        } else if (pVerifyData->crc32OfSegment != app_tencent_sv_data_rec_handler_env.crc32OfCurrentSeg) {
            app_tencent_sv_data_rec_handler_env.wholeDataXferLen = app_tencent_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
            app_tencent_sv_data_rec_handler_env.currentWholeCrc32 = app_tencent_sv_data_rec_handler_env.wholeCrc32UntilLastSeg;
            retStatus = TENCENT_DATA_SEGMENT_CRC_CHECK_FAILED;
        } else {
            app_tencent_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = app_tencent_sv_data_rec_handler_env.wholeDataXferLen;
            app_tencent_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = app_tencent_sv_data_rec_handler_env.currentWholeCrc32;
        }

        app_tencent_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;
    }

    APP_TENCENT_SV_VERIFY_DATA_SEGMENT_RSP_T rsp = {0};
    app_tencent_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp),device_id);
}

__attribute__((weak)) void app_tencent_sv_data_segment_verification_result_report(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus)
{
    // should re-send this segment
}

void app_tencent_sv_verify_data_segment_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    if (TENCENT_NO_ERROR != retStatus) {
        app_tencent_sv_data_trans_handler_env.wholeDataXferLen = app_tencent_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
        app_tencent_sv_data_trans_handler_env.currentWholeCrc32 = app_tencent_sv_data_trans_handler_env.wholeCrc32UntilLastSeg;
    } else {
        app_tencent_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification = app_tencent_sv_data_trans_handler_env.wholeDataXferLen;
        app_tencent_sv_data_trans_handler_env.wholeCrc32UntilLastSeg = app_tencent_sv_data_trans_handler_env.currentWholeCrc32;
    }

    app_tencent_sv_data_trans_handler_env.crc32OfCurrentSeg = 0;

    app_tencent_sv_data_segment_verification_result_report(retStatus);
}

__attribute__((weak)) void app_tencent_sv_data_received_callback(uint8_t* ptrData, uint32_t dataLength)
{

}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData       Pointer of the received data
 * @param dataLength    Length of the received data
 *
 * @return APP_TENCENT_SV_CMD_RET_STATUS_E
 */
uint32_t app_tencent_rcv_data(void *param1, uint32_t param2)
{
    if ((APP_TENCENT_DATA_XFER != (APP_TENCENT_SV_CMD_CODE_E)(((uint8_t *)param1)[0])) ||
        (param2 < TENCENT_SMARTVOICE_CMD_CODE_SIZE)) {
        TRACE(2,"%s error len %d", __func__, param2);
		return 1;
    }

    uint8_t* rawDataPtr = (uint8_t*)param1 + TENCENT_SMARTVOICE_CMD_CODE_SIZE;
    uint32_t rawDataLen = param2 - TENCENT_SMARTVOICE_CMD_CODE_SIZE;
    app_tencent_sv_data_received_callback(rawDataPtr, rawDataLen);

    app_tencent_sv_data_rec_handler_env.wholeDataXferLen += rawDataLen;

    if (app_tencent_sv_data_rec_handler_env.isCrcCheckEnabled) {
        // calculate the CRC
        app_tencent_sv_data_rec_handler_env.currentWholeCrc32 =
            crc32_c(app_tencent_sv_data_rec_handler_env.currentWholeCrc32, rawDataPtr, rawDataLen);
        app_tencent_sv_data_rec_handler_env.crc32OfCurrentSeg =
            crc32_c(app_tencent_sv_data_rec_handler_env.crc32OfCurrentSeg, rawDataPtr, rawDataLen);
    }
    return 0;
}

uint32_t app_tencent_audio_stream_handle(void *param1, uint32_t param2)
{
    static uint8_t localSeqNum = 0;
    APP_TENCENT_SV_CMD_PAYLOAD_T payload;
    uint32_t send_len = (uint32_t)(&(((APP_TENCENT_SV_CMD_PAYLOAD_T *)0)->param)) + param2;

    if (NULL == param1)
    {
        TRACE(1,"%s the buff is NULL", __func__);
        return false;
    }

    payload.cmdCode = APP_TENCENT_DATA_XFER;
    payload.reqOrRsp = TENCENT_COMMAND_REQ;
    payload.seqNum = localSeqNum++;
    payload.paramLen = param2;
    memcpy(payload.param, param1, payload.paramLen);
    TRACE(2,"%s data length is %d", __func__, payload.paramLen);

    app_tencent_sv_data_trans_handler_env.wholeDataXferLen += payload.paramLen;

    memcpy(param1, (uint8_t *)(&payload), send_len);
    return send_len;
}

uint32_t app_tencent_send_voice_stream(void *param1, uint32_t param2, uint8_t param3)   //send data to APP
{
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    #if 0
    if(!is_ai_audio_stream_allowed_to_send()){
        voice_compression_get_encoded_data(NULL ,encodedDataLength);
        TRACE(2,"%s ama don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }
    #endif

    if (encodedDataLength < app_ai_get_data_trans_size(AI_SPEC_TENCENT)) {
        return 1;
    }

    TRACE(3,"%s len %d credit %d", __func__, encodedDataLength, ai_struct[AI_SPEC_TENCENT].tx_credit);

    if(0 == ai_struct[AI_SPEC_TENCENT].tx_credit) {
        return 1;
    }

    ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_TENCENT),AI_SPEC_TENCENT,param3);
    return 0;
}

void app_tencent_sv_start_data_xfer(APP_TENCENT_SV_START_DATA_XFER_T* req,uint8_t device_id)
{
    app_tencent_sv_data_trans_handler_env.isCrcCheckEnabled = req->isHasCrcCheck;
    app_tencent_sv_send_command(APP_TENCENT_START_DATA_XFER, (uint8_t *)req, sizeof(APP_TENCENT_SV_START_DATA_XFER_T),device_id);
}

void app_tencent_sv_stAPP_TENCENT_DATA_XFER(APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T* req,uint8_t device_id)
{
    if (req->isHasWholeCrcCheck) {
        req->crc32OfWholeData = app_tencent_sv_data_trans_handler_env.currentWholeCrc32;
        req->wholeDataLenToCheck = app_tencent_sv_data_trans_handler_env.wholeDataXferLen;
    }

    app_tencent_sv_send_command(APP_TENCENT_DATA_XFER, (uint8_t *)req, sizeof(APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T),device_id);
}

void app_tencent_sv_verify_data_segment(uint8_t device_id)
{
    APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T req;
    req.crc32OfSegment = app_tencent_sv_data_trans_handler_env.crc32OfCurrentSeg;
    req.segmentDataLen = (app_tencent_sv_data_trans_handler_env.wholeDataXferLen -
                          app_tencent_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification);

    app_tencent_sv_send_command(APP_TENCENT_VERIFY_DATA_SEGMENT, (uint8_t *)&req, sizeof(APP_TENCENT_SV_VERIFY_DATA_SEGMENT_T),device_id);
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_START_DATA_XFER,   app_tencent_sv_data_xfer_control_handler, TRUE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,    app_tencent_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_DATA_XFER,    app_tencent_sv_data_xfer_control_handler, TRUE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,    app_tencent_sv_stAPP_TENCENT_DATA_XFER_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_VERIFY_DATA_SEGMENT, app_tencent_sv_data_segment_verifying_handler, TRUE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,     app_tencent_sv_verify_data_segment_rsp_handler );
#else
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_START_DATA_XFER,   app_tencent_sv_data_xfer_control_handler, FALSE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,   app_tencent_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_DATA_XFER,    app_tencent_sv_data_xfer_control_handler, FALSE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,   app_tencent_sv_stAPP_TENCENT_DATA_XFER_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_VERIFY_DATA_SEGMENT, app_tencent_sv_data_segment_verifying_handler, FALSE,  APP_TENCENT_SV_DATA_CMD_TIME_OUT_IN_MS,    app_tencent_sv_verify_data_segment_rsp_handler );
#endif


