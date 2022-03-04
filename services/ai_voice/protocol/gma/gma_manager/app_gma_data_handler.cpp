/**
 ****************************************************************************************
 *
 * @file app_gma_data_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the gma voice data handler
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
#include "app_gma_cmd_code.h"
#include "app_gma_cmd_handler.h"
#include "app_gma_data_handler.h"
#include "app_gma_handle.h"
#include "rwapp_config.h"
#include "crc32_c.h"
#include "app_gma_ble.h"
#include "app_gma_bt.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "voice_compression.h"
#include "factory_section.h"
#include "gma_crypto.h"
#include "nvrecord_env.h"
#include "tgt_hardware.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif

#ifdef __GMA_VOICE__

#define APP_GMA_DATA_CMD_TIME_OUT_IN_MS 5000
#define VOB_VOICE_XFER_CHUNK_SIZE   (VOB_VOICE_ENCODED_DATA_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)
/**
 * @brief gma voice data handling environment
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
} APP_GMA_DATA_HANDLER_ENV_T;

static APP_GMA_DATA_HANDLER_ENV_T app_gma_data_rec_handler_env;
static APP_GMA_DATA_HANDLER_ENV_T app_gma_data_trans_handler_env;
static APP_GMA_DEVICE_INFO_T app_gma_device_information;
static APP_GMA_PHONE_INFO_T app_gma_phone_information;

void gma_bt_addr_order_revers(uint8_t* bt_addr, uint16_t len)
{
    uint8_t temp[256];
    uint8_t i = 0;
    for(i=0; i<len; i++)
    {
        temp[len-1-i] = bt_addr[i];
    }
    for(i=0; i<len; i++)
    {
        bt_addr[i] = temp[i];
    }
}

void app_gma_device_infor_reset(void)
{
    uint8_t master_bt_addr[6];
    uint8_t slave_bt_addr[6];

    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    memcpy((void *)master_bt_addr, (void *)p_ibrt_ctrl->local_addr.address, 6);
    memcpy((void *)slave_bt_addr, (void *)p_ibrt_ctrl->peer_addr.address, 6);

    uint8_t gma_ver[2] = GMA_CAPACITY_GMA_VERSION;
    memset(&app_gma_phone_information, 0, sizeof(APP_GMA_PHONE_INFO_T));

    app_gma_device_information.capacity[0] = GMA_CAPACITY_B1;
    app_gma_device_information.capacity[1] = 0;
    app_gma_device_information.audioType = GMA_CAPACITY_AUDIO_TYPE;
    memcpy(app_gma_device_information.gmaVer, (uint8_t *)&gma_ver, sizeof(gma_ver));
    memcpy(app_gma_device_information.btAddr, master_bt_addr, 6);
    memcpy(app_gma_device_information.btAddr_slave, slave_bt_addr, 6);
}

void app_gma_data_reset_env(void)
{
    app_gma_data_rec_handler_env.isDataXferInProgress                = false;
    app_gma_data_rec_handler_env.isCrcCheckEnabled                   = false;
    app_gma_data_rec_handler_env.wholeDataXferLen                    = 0;
    app_gma_data_rec_handler_env.dataXferLenUntilLastSegVerification = 0;
    app_gma_data_rec_handler_env.currentWholeCrc32                   = 0;
    app_gma_data_rec_handler_env.wholeCrc32UntilLastSeg              = 0;
    app_gma_data_rec_handler_env.crc32OfCurrentSeg                   = 0;

    app_gma_data_trans_handler_env.isDataXferInProgress                = false;
    app_gma_data_trans_handler_env.isCrcCheckEnabled                   = false;
    app_gma_data_trans_handler_env.wholeDataXferLen                    = 0;
    app_gma_data_trans_handler_env.dataXferLenUntilLastSegVerification = 0;
    app_gma_data_trans_handler_env.currentWholeCrc32                   = 0;
    app_gma_data_trans_handler_env.wholeCrc32UntilLastSeg              = 0;
    app_gma_data_trans_handler_env.crc32OfCurrentSeg                   = 0;
}

bool app_gma_data_is_data_transmission_started(void)
{
    return app_gma_data_trans_handler_env.isDataXferInProgress;
}

APP_GMA_CMD_RET_STATUS_E app_gma_data_xfer_stop_handler(void)
{
    APP_GMA_CMD_RET_STATUS_E retStatus = NO_ERROR;

    app_gma_data_reset_env();
    app_gma_data_rec_handler_env.isDataXferInProgress = false;

    return retStatus;
}

void app_gma_dev_info_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(4,"%s line:%d funcCode:0x%02x paramLen:%d", __func__, __LINE__, funcCode, paramLen);

    if ((ptrParam == NULL) || (paramLen == 0))
    {
        TRACE(1,"%s invalid param!", __func__);
        return;
    }
    memcpy(&app_gma_phone_information, (APP_GMA_PHONE_INFO_T*)ptrParam, sizeof(APP_GMA_PHONE_INFO_T));
    if (app_gma_phone_information.system == 0x00)
    {
        TRACE(1,"%s the remote is IOS device", __func__);
    }
    else if(app_gma_phone_information.system == 0x01)
    {
        TRACE(1,"%s the remote is Android device", __func__);
    }

    APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_DEV_INFO_RSP;
    app_gma_send_command(rspCmdcode, (uint8_t*)&app_gma_device_information, sizeof(APP_GMA_DEVICE_INFO_T), false, device_id);
}

void app_gma_dev_activate_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"%s funcCode:0x%02x", __func__, funcCode);
    
    APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_DEV_ACTIV_RSP;
    APP_GMA_DEV_ACTIV_RSP_T rsp;
    uint8_t sha256[32];
    gma_rand_generator(rsp.random, sizeof(rsp.random));
    app_gma_cal_sha256(rsp.random, sha256);
    memcpy(rsp.digest, sha256, GMA_SHA256_RST_LEN);
    app_gma_send_command(rspCmdcode, (uint8_t*)&rsp, sizeof(APP_GMA_DEV_ACTIV_RSP_T), false, device_id);
}

__attribute__((weak)) void app_gma_data_xfer_started(APP_GMA_CMD_RET_STATUS_E retStatus)
{

}

void app_gma_kickoff_dataxfer(void)
{
    app_gma_data_reset_env();
    app_gma_data_trans_handler_env.isDataXferInProgress = true;    
}

void app_gma_start_data_xfer_control_rsp_handler(APP_GMA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if (NO_ERROR == retStatus)
    {
        app_gma_kickoff_dataxfer();
    }
    app_gma_data_xfer_started(retStatus);
}

__attribute__((weak)) void app_gma_data_xfer_stopped(APP_GMA_CMD_RET_STATUS_E retStatus)
{

}

void app_gma_stop_dataxfer(void)
{
    app_gma_data_reset_env();
    app_gma_data_trans_handler_env.isDataXferInProgress = false;    
}

void app_gma_stop_data_xfer_control_rsp_handler(APP_GMA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
    {
        app_gma_stop_dataxfer();
    }
    app_gma_data_xfer_stopped(retStatus);
}

__attribute__((weak)) void app_gma_data_received_callback(uint8_t* ptrData, uint32_t dataLength)
{
    
}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData       Pointer of the received data
 * @param dataLength    Length of the received data
 * 
 * @return APP_GMA_CMD_RET_STATUS_E
 */
uint32_t app_gma_rcv_data(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s", __func__);
    /* 
    if ((GMA_OP_DEV_SEND_STREAM != (APP_GMA_CMD_CODE_E)(((uint8_t *)param1)[0])) || 
        (param2 < GMA_CMD_CODE_SIZE)) {
        TR_DEBUG(2,"%s error len %d", __func__, param2);
        return 1;
    }
    */
    uint8_t* rawDataPtr = (uint8_t*)param1 + GMA_CMD_CODE_SIZE;
    uint32_t rawDataLen = param2 - GMA_CMD_CODE_SIZE;
    app_gma_data_received_callback(rawDataPtr, rawDataLen);

    app_gma_data_rec_handler_env.wholeDataXferLen += rawDataLen;

    if (app_gma_data_rec_handler_env.isCrcCheckEnabled) {
        // calculate the CRC
        app_gma_data_rec_handler_env.currentWholeCrc32 = 
        crc32_c(app_gma_data_rec_handler_env.currentWholeCrc32, rawDataPtr, rawDataLen);
        app_gma_data_rec_handler_env.crc32OfCurrentSeg = 
            crc32_c(app_gma_data_rec_handler_env.crc32OfCurrentSeg, rawDataPtr, rawDataLen);
    }
    return 0;
}

#ifdef SPEECH_CAPTURE_TWO_CHANNEL
uint32_t app_gma_send_voice_stream(void *param1, uint32_t param2,uint8_t param3)
{
    uint8_t app_gma_tmpDataXferBuf[640];
    uint16_t fact_send_len = 0;
    uint32_t dataBytesToSend = 0;
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    TRACE(3,"%s length %d max %d", __func__, encodedDataLength, app_ai_get_mtu());
        
    if (encodedDataLength < (VOB_VOICE_XFER_CHUNK_SIZE*2)) {
        TRACE(2,"%s data len too small %d", __func__, encodedDataLength);
        return 1;
    }

    dataBytesToSend = VOB_VOICE_XFER_CHUNK_SIZE;
    fact_send_len = dataBytesToSend + GMA_CMD_CODE_SIZE + GMA_ENCODED_DATA_SEQN_SIZE;

    app_gma_state_env_msg_id_incre();
    app_gma_tmpDataXferBuf[0] = app_gma_state_env_msg_id_get();
    app_gma_tmpDataXferBuf[1] = GMA_OP_DEV_SEND_STREAM;
    app_gma_tmpDataXferBuf[2] = 0;
    app_gma_tmpDataXferBuf[3] = dataBytesToSend + GMA_DATA_HEADER_SIZE;

    app_gma_tmpDataXferBuf[4] = 0x01;
    ((uint16_t *)app_gma_tmpDataXferBuf)[5] = dataBytesToSend;
    ((uint16_t *)app_gma_tmpDataXferBuf)[7] = 0;
    //memcpy(app_gma_tmpDataXferBuf + GMA_CMD_CODE_SIZE, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //GMA_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_gma_tmpDataXferBuf + GMA_CMD_CODE_SIZE + GMA_DATA_HEADER_SIZE + GMA_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);
    
    ai_transport_data_put(app_gma_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);
    
    //ai_struct.ai_stream.seqNumWithInFrame++;
    ((uint8_t *)app_gma_tmpDataXferBuf)[0] = GMA_OP_DEV_SEND_STREAM;
    app_gma_tmpDataXferBuf[GMA_CMD_CODE_SIZE] = 1;
    //memcpy(app_gma_tmpDataXferBuf + GMA_CMD_CODE_SIZE + fact_send_len, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //GMA_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_gma_tmpDataXferBuf + GMA_CMD_CODE_SIZE + GMA_DATA_HEADER_SIZE + GMA_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);

#if 0
    app_gma_data_trans_handler_env.wholeDataXferLen += (dataBytesToSend+GMA_ENCODED_DATA_SEQN_SIZE);
    if (app_gma_data_trans_handler_env.isCrcCheckEnabled) {
        app_gma_data_trans_handler_env.currentWholeCrc32 = 
            crc32_c(app_gma_data_trans_handler_env.currentWholeCrc32, &app_gma_tmpDataXferBuf[GMA_CMD_CODE_SIZE],
                dataBytesToSend+GMA_ENCODED_DATA_SEQN_SIZE);
        app_gma_data_trans_handler_env.crc32OfCurrentSeg = 
            crc32_c(app_gma_data_trans_handler_env.crc32OfCurrentSeg, &app_gma_tmpDataXferBuf[GMA_CMD_CODE_SIZE],
                dataBytesToSend+GMA_ENCODED_DATA_SEQN_SIZE);
    }
#endif

    //fact_send_len *= 2;
    TRACE(2,"%s fact_send_len %d", __func__, fact_send_len);
    ai_transport_data_put(app_gma_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);
    
    return 0;
}
#else
uint32_t app_gma_send_voice_stream(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();
    TRACE(2,"%s %d",__func__, is_ai_audio_stream_allowed_to_send(AI_SPEC_ALI));
    if(!is_ai_audio_stream_allowed_to_send(AI_SPEC_ALI)) {
        //voice_compression_get_encoded_data(NULL, encodedDataLength);
        TRACE(2,"%s gma don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }

    if (encodedDataLength < app_ai_get_data_trans_size(AI_SPEC_ALI)) {
        return 1;
    }

    TRACE(3,"%s encodedDataLength %d credit %d", __func__, encodedDataLength, ai_struct[AI_SPEC_ALI].tx_credit);
    if(ai_struct[AI_SPEC_ALI].tx_credit) {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_ALI), AI_SPEC_ALI, param3);
    }

    return 0;
}
#endif

uint32_t app_gma_audio_stream_handle(void *param1, uint32_t param2,uint8_t param3)
{
    uint32_t send_len = 0;
    APP_GMA_STREAM_CMD_T  stream_data;

    TRACE(3,"%s len %d mtu %d", __func__, param2, app_ai_get_mtu(AI_SPEC_ALI));

    if (NULL == param1)
    {
        TRACE(1,"%s the buff is NULL", __func__);
        return false;
    }

    send_len = param2 + GMA_CMD_CODE_SIZE + GMA_DATA_HEADER_SIZE;

    app_gma_state_env_msg_id_incre();


    stream_data.header0 = app_gma_state_env_msg_id_get();
    stream_data.cmdCode = GMA_OP_DEV_SEND_STREAM;
    stream_data.sequence=0;
    stream_data.length = param2 + GMA_DATA_HEADER_SIZE;

    stream_data.RefandMic= 0x01;
    stream_data.micLength= param2;
    stream_data.refLength = 0x00;
    memcpy((void *)stream_data.payLoad, param1, param2);
    memset(param1, 0, param2);
    memcpy(param1, (uint8_t *)&stream_data, send_len);
    return send_len;
}


void app_gma_dev_data_xfer(APP_GMA_DEV_STATUS_E isStart)
{
    APP_GMA_STAT_CHANGE_CMD_T rsp = {isStart, sizeof(APP_GMA_STAT_CHANGE_CMD_T) - GMA_TLV_HEADER_SIZE};
    app_gma_send_command(GMA_OP_DEV_CHANGE_CMD, (uint8_t*)&rsp, sizeof(APP_GMA_STAT_CHANGE_CMD_T), 1,0);  
}


CUSTOM_COMMAND_TO_ADD(GMA_OP_DEV_INFO_CMD,  app_gma_dev_info_handler, FALSE, 0, NULL );
CUSTOM_COMMAND_TO_ADD(GMA_OP_DEV_ACTIV_CMD,     app_gma_dev_activate_handler, FALSE, 0, NULL );

#endif //__GMA_VOICE__

