/**
 ****************************************************************************************
 *
 * @file app_sv_data_handler.c
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
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "app_smartvoice_handle.h"
#include "rwapp_config.h"
#include "crc32_c.h"
#include "app_smartvoice_ble.h"
#include "app_smartvoice_bt.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "voice_compression.h"


#define APP_SV_DATA_CMD_TIME_OUT_IN_MS  5000

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
} APP_SV_DATA_HANDLER_ENV_T;

static APP_SV_DATA_HANDLER_ENV_T app_sv_data_rec_handler_env;
static APP_SV_DATA_HANDLER_ENV_T app_sv_data_trans_handler_env;

void app_sv_data_reset_env(void)
{ 
    app_sv_data_rec_handler_env.isDataXferInProgress = false;
    app_sv_data_rec_handler_env.isCrcCheckEnabled = false;
    app_sv_data_rec_handler_env.wholeDataXferLen = 0;
    app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = 0;
    app_sv_data_rec_handler_env.currentWholeCrc32 = 0;
    app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = 0;
    app_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;

    app_sv_data_trans_handler_env.isDataXferInProgress = false;
    app_sv_data_trans_handler_env.isCrcCheckEnabled = false;
    app_sv_data_trans_handler_env.wholeDataXferLen = 0;
    app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification = 0;
    app_sv_data_trans_handler_env.currentWholeCrc32 = 0;
    app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg = 0;
    app_sv_data_trans_handler_env.crc32OfCurrentSeg = 0;
}

bool app_sv_data_is_data_transmission_started(void)
{
    return app_sv_data_trans_handler_env.isDataXferInProgress;
}

APP_SV_CMD_RET_STATUS_E app_sv_data_xfer_stop_handler(APP_SV_STOP_DATA_XFER_T* pStopXfer)
{
    APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;
    
    if (pStopXfer->isHasWholeCrcCheck) {
        if (pStopXfer->wholeDataLenToCheck != app_sv_data_rec_handler_env.wholeDataXferLen) {
            retStatus = DATA_XFER_LEN_NOT_MATCHED;
        }

        if (pStopXfer->crc32OfWholeData != app_sv_data_rec_handler_env.currentWholeCrc32) {
            retStatus = WHOLE_DATA_CRC_CHECK_FAILED;
        }
    }

    app_sv_data_reset_env();
    app_sv_data_rec_handler_env.isDataXferInProgress = false;

    return retStatus;
}

void app_sv_data_xfer_control_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;

    if (OP_START_DATA_XFER == funcCode) {
        if (app_sv_data_rec_handler_env.isDataXferInProgress) {
            retStatus = DATA_XFER_ALREADY_STARTED;
        } else {
            if (paramLen < sizeof(APP_SV_START_DATA_XFER_T)) {
                retStatus = PARAMETER_LENGTH_TOO_SHORT;
            } else if (paramLen > sizeof(APP_SV_START_DATA_XFER_T)) {
                retStatus = PARAM_LEN_OUT_OF_RANGE;
            } else {
                app_sv_data_reset_env();
                APP_SV_START_DATA_XFER_T* pStartXfer = (APP_SV_START_DATA_XFER_T *)ptrParam;
                app_sv_data_rec_handler_env.isDataXferInProgress = true;
                app_sv_data_rec_handler_env.isCrcCheckEnabled = pStartXfer->isHasCrcCheck;
            }
        }
        APP_SV_START_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp), device_id);
    } else if (OP_STOP_DATA_XFER == funcCode) {
        if (!app_sv_data_rec_handler_env.isDataXferInProgress) {
            retStatus = DATA_XFER_NOT_STARTED_YET;
        } else {
            if (paramLen < sizeof(APP_SV_STOP_DATA_XFER_T)) {
                retStatus = PARAMETER_LENGTH_TOO_SHORT;
            } else if (paramLen > sizeof(APP_SV_STOP_DATA_XFER_T)) {
                retStatus = PARAM_LEN_OUT_OF_RANGE;
            } else {
                retStatus = app_sv_data_xfer_stop_handler((APP_SV_STOP_DATA_XFER_T *)ptrParam);
            }
        }
        APP_SV_STOP_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp), device_id);
    } else {
        retStatus = INVALID_CMD;
        app_sv_send_response_to_command(funcCode, retStatus, NULL, 0, device_id);
    }
}

__attribute__((weak)) void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_sv_kickoff_dataxfer(void)
{
    app_sv_data_reset_env();
    app_sv_data_trans_handler_env.isDataXferInProgress = true;    
}

void app_sv_start_data_xfer_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    if (NO_ERROR == retStatus)
    {
        app_sv_kickoff_dataxfer();
    }
    app_sv_data_xfer_started(retStatus);
}

__attribute__((weak)) void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_sv_stop_dataxfer(void)
{
    app_sv_data_reset_env();
    app_sv_data_trans_handler_env.isDataXferInProgress = false;    
}

void app_sv_stop_data_xfer_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
    {
        app_sv_stop_dataxfer();
    }
    app_sv_data_xfer_stopped(retStatus);
}

void app_sv_data_segment_verifying_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;
    if (paramLen < sizeof(APP_SV_VERIFY_DATA_SEGMENT_T)) {
        retStatus = PARAMETER_LENGTH_TOO_SHORT;
    } else if (paramLen > sizeof(APP_SV_VERIFY_DATA_SEGMENT_T)) {
        retStatus = PARAM_LEN_OUT_OF_RANGE;
    } else {
        APP_SV_VERIFY_DATA_SEGMENT_T* pVerifyData = (APP_SV_VERIFY_DATA_SEGMENT_T *)ptrParam;
        if (pVerifyData->segmentDataLen != 
            (app_sv_data_rec_handler_env.wholeDataXferLen - 
            app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification)) {
            retStatus = DATA_XFER_LEN_NOT_MATCHED;
        } else if (pVerifyData->crc32OfSegment != app_sv_data_rec_handler_env.crc32OfCurrentSeg) {  
            app_sv_data_rec_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
            app_sv_data_rec_handler_env.currentWholeCrc32 = app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg;
            retStatus = DATA_SEGMENT_CRC_CHECK_FAILED;
        } else {
            app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = app_sv_data_rec_handler_env.wholeDataXferLen;
            app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = app_sv_data_rec_handler_env.currentWholeCrc32;     
        }

        app_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;
    }

    APP_SV_VERIFY_DATA_SEGMENT_RSP_T rsp = {0};
    app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp), device_id);
}

__attribute__((weak)) void app_sv_data_segment_verification_result_report(APP_SV_CMD_RET_STATUS_E retStatus)
{
    // should re-send this segment
}

void app_sv_verify_data_segment_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    if (NO_ERROR != retStatus) {    
        app_sv_data_trans_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
        app_sv_data_trans_handler_env.currentWholeCrc32 = app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg;
    } else {
        app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification = app_sv_data_trans_handler_env.wholeDataXferLen;
        app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg = app_sv_data_trans_handler_env.currentWholeCrc32;     
    }

    app_sv_data_trans_handler_env.crc32OfCurrentSeg = 0;

    app_sv_data_segment_verification_result_report(retStatus);
}

__attribute__((weak)) void app_sv_data_received_callback(uint8_t* ptrData, uint32_t dataLength)
{

}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData       Pointer of the received data
 * @param dataLength    Length of the received data
 * 
 * @return APP_SV_CMD_RET_STATUS_E
 */
uint32_t app_sv_rcv_data(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s", __func__);
    
    if ((OP_DATA_XFER != (APP_SV_CMD_CODE_E)(((uint16_t *)param1)[0])) || 
        (param2 < SMARTVOICE_CMD_CODE_SIZE)) {
        TRACE(2,"%s error len %d", __func__, param2);
        return 1;
    }

    uint8_t* rawDataPtr = (uint8_t*)param1 + SMARTVOICE_CMD_CODE_SIZE;
    uint32_t rawDataLen = param2 - SMARTVOICE_CMD_CODE_SIZE;
    app_sv_data_received_callback(rawDataPtr, rawDataLen);

    app_sv_data_rec_handler_env.wholeDataXferLen += rawDataLen;

    if (app_sv_data_rec_handler_env.isCrcCheckEnabled) {
        // calculate the CRC
        app_sv_data_rec_handler_env.currentWholeCrc32 = 
        crc32_c(app_sv_data_rec_handler_env.currentWholeCrc32, rawDataPtr, rawDataLen);
        app_sv_data_rec_handler_env.crc32OfCurrentSeg = 
            crc32_c(app_sv_data_rec_handler_env.crc32OfCurrentSeg, rawDataPtr, rawDataLen);
    }
    return 0;
}

#ifdef SPEECH_CAPTURE_TWO_CHANNEL
uint32_t app_sv_send_voice_stream(void *param1, uint32_t param2)
{
    uint8_t app_sv_tmpDataXferBuf[640];
    uint16_t fact_send_len = 0;
    uint32_t dataBytesToSend = 0;
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    TRACE(3,"%s length %d max %d", __func__, encodedDataLength, app_ai_get_mtu());
        
    if (encodedDataLength < (VOB_VOICE_XFER_CHUNK_SIZE*2)) {
        TRACE(2,"%s data len too small %d", __func__, encodedDataLength);
        return 1;
    }

    dataBytesToSend = VOB_VOICE_XFER_CHUNK_SIZE;
    fact_send_len = dataBytesToSend + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE;

    //ai_struct.ai_stream.seqNumWithInFrame = 0;
    ((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
    app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE] = 0;
    //memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);

    ai_transport_data_put(app_sv_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);

    //ai_struct.ai_stream.seqNumWithInFrame++;
    ((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
    app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE] = 1;
    //memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + fact_send_len, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);

#if 0
    app_sv_data_trans_handler_env.wholeDataXferLen += (dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    if (app_sv_data_trans_handler_env.isCrcCheckEnabled) {
        app_sv_data_trans_handler_env.currentWholeCrc32 = 
            crc32_c(app_sv_data_trans_handler_env.currentWholeCrc32, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
                dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
        app_sv_data_trans_handler_env.crc32OfCurrentSeg = 
            crc32_c(app_sv_data_trans_handler_env.crc32OfCurrentSeg, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
                dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    }
#endif

    //fact_send_len *= 2;
    TRACE(2,"%s fact_send_len %d", __func__, fact_send_len);
    ai_transport_data_put(app_sv_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);

    return 0;
}
#else
uint32_t app_sv_send_voice_stream(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    if(!is_ai_audio_stream_allowed_to_send(AI_SPEC_BES)){

        TRACE(2,"%s sv don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }

    if (encodedDataLength < app_ai_get_data_trans_size(AI_SPEC_BES)) {
        return 1;
    }

    TRACE(3,"%s encodedDataLength %d credit %d", __func__, encodedDataLength, ai_struct[AI_SPEC_BES].tx_credit);

    if (ai_struct[AI_SPEC_BES].tx_credit) {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_BES), AI_SPEC_BES, param3);
    }

    return 0;
}
#endif

uint32_t app_sv_audio_stream_handle(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t send_len = 0;
    uint8_t app_sv_data_buf[MAX_TX_BUFF_SIZE];

    TRACE(3,"%s len %d max_size %d", __func__, param2, app_ai_get_mtu(AI_SPEC_BES));

    if (NULL == param1)
    {
        TRACE(1,"%s the buff is NULL", __func__);
        return false;
    }

    if ((ai_struct[AI_SPEC_BES].ai_stream.sentDataSizeWithInFrame + param2) >= (VOB_VOICE_XFER_CHUNK_SIZE)) {
        param2 = (uint32_t)(VOB_VOICE_XFER_CHUNK_SIZE) - ai_struct[AI_SPEC_BES].ai_stream.sentDataSizeWithInFrame;
    }

    ai_struct[AI_SPEC_BES].ai_stream.sentDataSizeWithInFrame += param2;
    send_len = param2 + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE;

    ((uint16_t *)app_sv_data_buf)[0] = OP_DATA_XFER;
    memcpy(app_sv_data_buf + SMARTVOICE_CMD_CODE_SIZE, &(ai_struct[AI_SPEC_BES].ai_stream.seqNumWithInFrame), 
        SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    memcpy(app_sv_data_buf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE, \
                param1, param2);

    if (ai_struct[AI_SPEC_BES].ai_stream.sentDataSizeWithInFrame < (VOB_VOICE_XFER_CHUNK_SIZE)) {
        ai_struct[AI_SPEC_BES].ai_stream.seqNumWithInFrame++;
    } else {
        ai_struct[AI_SPEC_BES].ai_stream.seqNumWithInFrame = 0;
        ai_struct[AI_SPEC_BES].ai_stream.sentDataSizeWithInFrame = 0;
    }

    app_sv_data_trans_handler_env.wholeDataXferLen += (param2+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    if (app_sv_data_trans_handler_env.isCrcCheckEnabled) {
        app_sv_data_trans_handler_env.currentWholeCrc32 = 
            crc32_c(app_sv_data_trans_handler_env.currentWholeCrc32, &app_sv_data_buf[SMARTVOICE_CMD_CODE_SIZE],
                param2+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
        app_sv_data_trans_handler_env.crc32OfCurrentSeg = 
            crc32_c(app_sv_data_trans_handler_env.crc32OfCurrentSeg, &app_sv_data_buf[SMARTVOICE_CMD_CODE_SIZE],
                param2+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    }

    memcpy(param1, app_sv_data_buf, send_len);
    return send_len;
}

void app_sv_start_data_xfer(APP_SV_START_DATA_XFER_T* req, uint8_t device_id)
{
    app_sv_data_trans_handler_env.isCrcCheckEnabled = req->isHasCrcCheck;
    app_sv_send_command(OP_START_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_START_DATA_XFER_T), device_id);  
}

void app_sv_stop_data_xfer(APP_SV_STOP_DATA_XFER_T* req, uint8_t device_id)
{
    if (req->isHasWholeCrcCheck) {
        req->crc32OfWholeData = app_sv_data_trans_handler_env.currentWholeCrc32;
        req->wholeDataLenToCheck = app_sv_data_trans_handler_env.wholeDataXferLen;
    }

    app_sv_send_command(OP_STOP_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_STOP_DATA_XFER_T), device_id);    
}

void app_sv_verify_data_segment(uint8_t device_id)
{
    APP_SV_VERIFY_DATA_SEGMENT_T req;
    req.crc32OfSegment = app_sv_data_trans_handler_env.crc32OfCurrentSeg;
    req.segmentDataLen = (app_sv_data_trans_handler_env.wholeDataXferLen - 
            app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification);

    app_sv_send_command(OP_VERIFY_DATA_SEGMENT, (uint8_t *)&req, sizeof(APP_SV_VERIFY_DATA_SEGMENT_T), device_id); 
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_START_DATA_XFER,   app_sv_data_xfer_control_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,    app_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_STOP_DATA_XFER,    app_sv_data_xfer_control_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,    app_sv_stop_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_VERIFY_DATA_SEGMENT, app_sv_data_segment_verifying_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,     app_sv_verify_data_segment_rsp_handler );
#else
CUSTOM_COMMAND_TO_ADD(OP_START_DATA_XFER,   app_sv_data_xfer_control_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,   app_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_STOP_DATA_XFER,    app_sv_data_xfer_control_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,   app_sv_stop_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_VERIFY_DATA_SEGMENT, app_sv_data_segment_verifying_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,    app_sv_verify_data_segment_rsp_handler );
#endif


