#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include <stdlib.h>
#include "app_audio.h"
#include "app_ble_mode_switch.h"
#ifndef BLE_V2
#include "gapm_task.h"
#endif
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_key.h"
#include "hal_location.h"


//#include "aes.h"
#include "tgt_hardware.h"

#include "nvrecord.h"
#include "nvrecord_env.h"

#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_ai_tws.h"
#include "app_smartvoice_handle.h"
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "app_smartvoice_ble.h"
#include "app_smartvoice_bt.h"
#include "voice_compression.h"
#include "crc32_c.h"

#include "factory_section.h"
#include "app_ai_if_thirdparty.h"

extern uint8_t bt_addr[6];

#define APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS 5000
#define SMARTVOICE_SYSTEM_FREQ       APP_SYSFREQ_208M


typedef struct
{
    CQueue      encodedDataBufQueue;
    uint8_t*    encodedDataBuf;
    uint8_t     isAudioAlgorithmEngineReseted;
    uint8_t*    tmpBufForEncoding;
    uint8_t*    tmpBufForXfer;

    uint32_t    sentDataSizeWithInFrame;
    uint8_t     seqNumWithInFrame;
    CQueue      pcmDataBufQueue;
    uint8_t*    pcmDataBuf;
    uint8_t*    tmpPcmDataBuf;


} APP_SMARTVOICE_AUDIO_STREAM_ENV_T;

typedef enum _SVSpeechState {
    SV_SPEECH_STATE__IDLE = 0,
    SV_SPEECH_STATE__LISTENING = 1,
    SV_SPEECH_STATE__PROCESSING = 2,
    SV_SPEECH_STATE__SPEAKING = 3
} APP_SV_SPEECH_STATE_E;

typedef struct {
    volatile uint8_t            isPlaybackStreamRunning : 1;
    volatile uint8_t            isThroughputTestOn  : 1;
    volatile uint8_t            reserve             : 6;
    uint8_t                     connType;
    uint8_t                     conidx;
    uint16_t                    smartVoiceDataSectorSize;
    CLOUD_PLATFORM_E            currentPlatform;
} smartvoice_context_t;

typedef struct {
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_REQ_T;

typedef struct {
    uint8_t           voiceStreamPathToUse;
} START_VOICE_STREAM_RSP_T;

typedef struct {
    uint8_t           cloudMusicPlayingControlReq;
} CLOUD_MUSIC_PLAYING_CONTROL_REQ_T;

typedef struct {
    CLOUD_PLATFORM_E        cloudPlatform;
} CLOUD_PLATFORM_CHOOSING_REQ_T;

typedef struct {
    APP_SV_SPEECH_STATE_E   speechState;
} SPEECH_STATE_RSP_T;

typedef struct {
    bool   canSwitchNow;
} ROLE_SWITCH_RSP_T;

typedef struct {
    uint8_t     aiParam[255];
    uint8_t     param_len;
} ROLE_SWITCH_AI_PARAM_T;

static smartvoice_context_t smartvoice_env;
#if (MIX_MIC_DURING_MUSIC)
static uint32_t more_mic_data(uint8_t* ptrBuf, uint32_t length);
#endif

extern void app_hfp_enable_audio_link(bool isEnable);
void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len);

#define APP_SCNRSP_DATA         "\x11\x07\xa2\xaf\x34\xd6\xba\x17\x53\x8b\xd4\x4d\x3c\x8a\xdc\x0c\x3e\x26"
#define APP_SCNRSP_DATA_LEN     (18)

static uint32_t app_sv_start_advertising(void *param1, uint32_t param2)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T *)param1;

    if (app_ai_dont_have_mobile_connect(AI_SPEC_BES))
    {
        TRACE(1,"%s no mobile connect", __func__);
        return 0;
    }

    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
    cmd->scanRspDataLen += APP_SCNRSP_DATA_LEN;

    if (app_ai_is_in_tws_mode(0))
    {
        uint8_t *_local_bt_addr = app_ai_tws_local_address();
        uint8_t dev_bt_addr[6];
        uint32_t bt_addr_crc32 = 0;
        if (_local_bt_addr == NULL)
        {
            TRACE(1, "%s bt addr is NULL", __func__);
            return -1;
        }
        memcpy(dev_bt_addr, _local_bt_addr, 6);
        DUMP8("%x ", dev_bt_addr, BT_ADDR_OUTPUT_PRINT_NUM);
        bt_addr_crc32 = crc32_c(bt_addr_crc32, dev_bt_addr, 6);

#ifdef SLAVE_ADV_BLE
        cmd->scanRspData[cmd->scanRspDataLen++] = 12;
#else
        cmd->scanRspData[cmd->scanRspDataLen++] = 5;
#endif
        cmd->scanRspData[cmd->scanRspDataLen++] = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
        memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
               &bt_addr_crc32, 4);
        cmd->scanRspDataLen += 4;
#ifdef SLAVE_ADV_BLE
        memcpy(&cmd->scanRspData[cmd->scanRspDataLen],dev_bt_addr, 6);
        cmd->scanRspDataLen += 6;

        if(app_ai_tws_get_local_role() != APP_AI_TWS_SLAVE)
        {
            cmd->scanRspData[cmd->scanRspDataLen++] = 1;
        }
        else
        {
            cmd->scanRspData[cmd->scanRspDataLen++] = 0;
        }
#endif
    }
    return 1;
}

uint32_t app_sv_stream_init(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}

uint32_t app_sv_stream_deinit(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}
static int _on_stream_started(void)
{
    return 0;
}

static void _on_stream_stopped(void)
{
#ifdef __BIXBY
    app_ai_voice_deinit_encoder(AI_SPEC_BES);
#endif
}

uint32_t app_sv_voice_connected(void *param1, uint32_t param2, uint8_t param3)
{
    AI_TRANS_TYPE_E ai_trans_type = (AI_TRANS_TYPE_E)param2;
    uint8_t dest_id = (uint8_t)param3;
    TRACE(2,"%s connect_type %d",__func__, ai_trans_type);

    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1; 
    uint8_t ai_connect_index = app_ai_connect_handle(ai_trans_type, AI_SPEC_BES, dest_id, btaddr->address);
    if(ai_trans_type == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send, AI_SPEC_BES, ai_trans_type);
        ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_BES, ai_trans_type);
        app_ai_set_mtu(TX_SPP_MTU_SIZE, AI_SPEC_BES);
        app_ai_set_data_trans_size(VOB_VOICE_XFER_CHUNK_SIZE, AI_SPEC_BES);
    }
#ifdef __AI_VOICE_BLE_ENABLE__
    else if(ai_trans_type == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_sv_send_data_via_notification, AI_SPEC_BES, ai_trans_type);
        ai_cmd_transport_init(app_sv_send_cmd_via_notification, AI_SPEC_BES, ai_trans_type);
        //if ble connected, close spp;
        //app_ai_spp_disconnlink(AI_SPEC_BES, ai_connect_index);
        app_ai_set_data_trans_size(VOB_VOICE_XFER_CHUNK_SIZE, AI_SPEC_BES);
    }
#endif

    app_sv_data_reset_env();
    /// register AI voice related handlers
    AIV_HANDLER_BUNDLE_T handlerBundle = {
        .streamStarted          = _on_stream_started,
        .streamStopped          = _on_stream_stopped,
        .upstreamDataHandler    = app_ai_voice_default_upstream_data_handler,
        .hwDetectHandler        = app_ai_voice_default_hotword_detect_handler,
    };
    app_ai_voice_register_ai_handler(AIV_USER_BES,
                                     &handlerBundle);

    uint32_t lock = int_lock();
    /// NOTE: Add this part because Samsung BIXBY parasitize on SMART VOICE
#ifdef __BIXBY
    app_ai_setup_complete_handle(AI_SPEC_BIXBY);
#endif
    app_ai_setup_complete_handle(AI_SPEC_BES);
    int_unlock(lock);

    app_ai_update_connect_state(AI_IS_CONNECTED, ai_connect_index);

    return 0;
}

uint32_t app_sv_voice_disconnected(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s ", __func__);

    app_sv_data_reset_env();

#ifdef __BIXBY
    app_ai_disconnect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_BIXBY);
#endif
    app_ai_disconnect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_BES);
    return 0;
}

void app_smartvoice_start_xfer(uint8_t device_id)
{
    TRACE(1,"%s", __func__);

#if IS_ENABLE_START_DATA_XFER_STEP
    APP_TENCENT_SV_START_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasCrcCheck = false;
    app_sv_start_data_xfer(&req, device_id);
#else
    app_sv_kickoff_dataxfer();
#endif
}

void app_smartvoice_stop_xfer(uint8_t device_id)
{
#if 0
    app_sv_update_conn_parameter(SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS,
        SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS,
        SV_LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,0);
#endif

#if IS_ENABLE_START_DATA_XFER_STEP
    APP_SV_STOP_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasWholeCrcCheck = false;
    app_sv_stop_data_xfer(&req, device_id);
#else
   TRACE(1,"%s", __func__);
    app_sv_stop_dataxfer();
#endif
}

void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus)
{
    TRACE(2,"%s ret %d", __func__, retStatus);
}

void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus)
{
    app_hfp_enable_audio_link(false);
    TRACE(2,"%s ret %d", __func__, retStatus);
}

void app_sv_start_voice_stream_via_ble(uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_BES);

    if (AI_TRANSPORT_BLE == app_ai_get_transport_type(AI_SPEC_BES, connect_index)) {
        if (!app_ai_is_stream_opened(AI_SPEC_BES)) {
            ai_struct[AI_SPEC_BES].tx_credit = MAX_TX_CREDIT;
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_BLE;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), device_id);
        } else {
            TRACE(1,"%s ai_stream has open", __func__);
        }
    } else {
        TRACE(1,"%s transport_type is error", __func__);
    }
}

void app_sv_start_voice_stream_via_spp(uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_BES);

    if (AI_TRANSPORT_SPP == app_ai_get_transport_type(AI_SPEC_BES, connect_index)) {
        if (!app_ai_is_stream_opened(AI_SPEC_BES)) {
            ai_struct[AI_SPEC_BES].tx_credit = MAX_TX_CREDIT;
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_SPP;
            app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), device_id);
        } else {
            TRACE(1,"%s ai_stream has open", __func__);
        }
    } else {
        TRACE(1,"%s transport_type is error", __func__);
    }
}

void app_sv_start_voice_stream_via_hfp(uint8_t device_id)
{
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_BES);
    if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_BES, connect_index)) {
        TRACE(1,"%s", __func__);
        ai_struct[AI_SPEC_BES].tx_credit = MAX_TX_CREDIT;
        app_hfp_enable_audio_link(true);
        START_VOICE_STREAM_REQ_T req;
        req.voiceStreamPathToUse = APP_SMARTVOICE_PATH_TYPE_HFP;
        app_sv_send_command(OP_START_SMART_VOICE_STREAM, (uint8_t *)&req, sizeof(req), device_id);
    }
}

uint32_t app_sv_config_mtu(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s is %d", __func__, param2);
    app_ai_set_mtu(param2, AI_SPEC_BES);

    return 0;
}


uint32_t app_sv_handle_init(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s", __func__);

    memset(&smartvoice_env, 0x00, sizeof(smartvoice_env));

    app_sv_cmd_handler_init();
    app_sv_data_reset_env();

    app_ai_set_need_ai_param(false, AI_SPEC_BES);
    smartvoice_env.currentPlatform = CLOUD_PLATFORM_IDLE;

    return 0;
}

void app_sv_start_voice_stream(uint8_t device_id, uint32_t readOffset)
{
    TRACE(2,"%s ai_stream_opened %d", __func__, app_ai_is_stream_opened(AI_SPEC_BES));

    if (!app_ai_is_stream_opened(AI_SPEC_BES)) {
        app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, device_id);

        TRACE(0,"Start smart voice stream.");
        app_ai_set_stream_opened(true, AI_SPEC_BES);
        app_ai_voice_stream_init(AIV_USER_BES);
        ai_audio_stream_allowed_to_send_set(true, AI_SPEC_BES);
        ai_mailbox_put(SIGN_AI_WAKEUP, 0, AI_SPEC_BES, device_id);
        app_ai_voice_upstream_control(true, AIV_USER_BES, readOffset);
    }
}

void app_sv_stop_stream(uint8_t device_id)
{
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_BES);

    if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_BES, connect_index)) 
    {
        if (app_ai_is_stream_opened(AI_SPEC_BES))
        {
            app_ai_voice_upstream_control(false, AIV_USER_BES, UPSTREAM_INVALID_READ_OFFSET);
            app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, device_id);
        }
    }
}

uint32_t app_sv_stop_voice_stream(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s ai_stream_opened %d", __func__, app_ai_is_stream_opened(AI_SPEC_BES));
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(param3, AI_SPEC_BES);

    if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_BES, connect_index)) {
        if (app_ai_is_stream_opened(AI_SPEC_BES)) {
            app_ai_voice_upstream_control(false, AIV_USER_BES, UPSTREAM_INVALID_READ_OFFSET);
            app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, param3);
            app_sv_send_command(OP_STOP_SMART_VOICE_STREAM, NULL, 0, param3);
        }
    }
    return 0;
}

uint32_t app_sv_wake_up(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(param3, AI_SPEC_BES);
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_BES, connect_index);

    TRACE(2,"%s event %d", __func__, status->event);

    if (app_ai_is_role_switching(AI_SPEC_BES))
    {
        TRACE(1,"%s ai need role switch", __func__);
        return 1;
    }

    if (transport_type == AI_TRANSPORT_IDLE){
       TRACE(1,"%s not ai connect", __func__);
       return 1;
    }

    if (app_ai_is_sco_mode()){
        TRACE(1,"%s is in sco mode", __func__);
        return 2;
    }

    switch(status->event)
    {
        case APP_KEY_EVENT_FIRST_DOWN:
            TRACE(0,"smartvoice_wakeup_app_key!");
            app_ai_spp_deinit_tx_buf();

            if(AI_TRANSPORT_BLE == transport_type) {     //use ble
                app_smartvoice_switch_ble_stream_status(param3);    //send start cmd;
            }
            else if(AI_TRANSPORT_SPP == transport_type) {    //use spp
                app_smartvoice_switch_spp_stream_status(param3);  //send start cmd;
            } else {
                TRACE(0,"it is error, not ble or spp connected");
                return 3;
            }

            app_sv_start_voice_stream(param3, UPSTREAM_INVALID_READ_OFFSET);
            break;
        case APP_KEY_EVENT_UP:
            TRACE(0,"smartvoice_stop_app_key");
            app_sv_stop_voice_stream(NULL, 0, param3);
            break;
    }

    return 0;
}

uint32_t app_sv_enable_handler(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s %d", __func__, param2);
    return 0;
}

void app_sv_start_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;
    TRACE(2,"%s Path %d", __func__, voicePathType);
    START_VOICE_STREAM_RSP_T rsp;
    rsp.voiceStreamPathToUse = voicePathType;

    if (!app_ai_is_stream_opened(AI_SPEC_BES))
    {
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == voicePathType) ||
            (APP_SMARTVOICE_PATH_TYPE_SPP == voicePathType)) {
            if (app_ai_is_sco_mode()) {
                TRACE(1,"%s CMD_HANDLING_FAILED", __func__);
                app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp), device_id);
            } else {
                app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp), device_id);

                app_ai_set_timer_need(true, AI_SPEC_BES);
                app_sv_start_voice_stream(device_id, UPSTREAM_INVALID_READ_OFFSET);
                // start the data xfer
                app_smartvoice_start_xfer(device_id);
            }
        } else if ((APP_SMARTVOICE_PATH_TYPE_HFP == voicePathType) &&
            !app_ai_is_sco_mode()) { //need check -- zsl
            app_sv_send_response_to_command(funcCode, CMD_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp), device_id);
        } else {
            ai_struct[AI_SPEC_BES].tx_credit = MAX_TX_CREDIT;
            app_ai_voice_stream_init(AIV_USER_BES);
            app_ai_set_stream_opened(true, AI_SPEC_BES);
            app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp), device_id);
        }
    } else {
        app_sv_send_response_to_command(funcCode, DATA_XFER_ALREADY_STARTED, (uint8_t *)&rsp, sizeof(rsp), device_id);
    }
}

static void app_sv_start_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(3,"%s ret %d stream %d", __func__, retStatus, app_ai_is_stream_opened(AI_SPEC_BES));
    if (NO_ERROR == retStatus) {
        START_VOICE_STREAM_RSP_T* rsp = (START_VOICE_STREAM_RSP_T *)ptrParam;
        TRACE(2,"%s path %d", __func__, rsp->voiceStreamPathToUse);
        if ((APP_SMARTVOICE_PATH_TYPE_BLE == rsp->voiceStreamPathToUse) ||
            (APP_SMARTVOICE_PATH_TYPE_SPP == rsp->voiceStreamPathToUse)) {
            if (!app_ai_is_stream_opened(AI_SPEC_BES)) {
                app_sv_start_voice_stream(device_id, UPSTREAM_INVALID_READ_OFFSET);
                // start the data xfer
                app_smartvoice_start_xfer(device_id);
            } else {
                app_smartvoice_start_xfer(device_id);
            }
        }
    } else {
        // workaround, should be changed to false
        app_hfp_enable_audio_link(false);
    }
}

static void app_sv_stop_voice_stream_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"%s stream %d", __func__, app_ai_is_stream_opened(AI_SPEC_BES));
    if (app_ai_is_stream_opened(AI_SPEC_BES))
    {
        app_sv_send_response_to_command(funcCode, NO_ERROR, NULL, 0, device_id);
        app_sv_stop_stream(device_id);
        // Stop the data xfer
        app_smartvoice_stop_xfer(device_id);
    }
    else
    {
        app_sv_send_response_to_command(funcCode, DATA_XFER_NOT_STARTED_YET, NULL, 0, device_id);
    }
}

static void app_sv_stop_voice_stream_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"%s status %d", __func__, retStatus);
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
    {
        app_sv_stop_stream(device_id);
        app_smartvoice_stop_xfer(device_id);
    }

    app_ai_set_stream_opened(false, AI_SPEC_BES);
}

uint32_t app_voice_catpure_interval_in_ms(void)
{
    return 0;//VOB_VOICE_CAPTURE_INTERVAL_IN_MS;
}

void app_smartvoice_switch_hfp_stream_status(uint8_t device_id)
{
    if (app_ai_is_stream_opened(AI_SPEC_BES))
    {
        app_sv_stop_voice_stream(NULL, 0, device_id);
    }
    else
    {
        app_sv_start_voice_stream_via_hfp(device_id);
    }
}

void app_smartvoice_switch_ble_stream_status(uint8_t device_id)
{
    if (app_ai_is_stream_opened(AI_SPEC_BES))
    {
        app_sv_stop_voice_stream(NULL, 0, device_id);
    }
    else
    {
        app_sv_start_voice_stream_via_ble(device_id);
    }
}

void app_smartvoice_switch_spp_stream_status(uint8_t device_id)
{
    if (app_ai_is_stream_opened(AI_SPEC_BES))
    {
        app_sv_stop_voice_stream(NULL, 0, device_id);
    }
    else
    {
        app_sv_start_voice_stream_via_spp(device_id);
    }
}

static void app_sv_cloud_music_playing_control_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(0,"Received the cloud music playing control request from the remote dev.");
}

static void app_sv_cloud_music_playing_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"Music control returns %d", retStatus);
}

void app_sv_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req, uint8_t device_id)
{
    if (smartvoice_env.connType > 0)
    {
        if (app_ai_is_stream_opened(AI_SPEC_BES))
        {
            CLOUD_MUSIC_PLAYING_CONTROL_REQ_T request;
            request.cloudMusicPlayingControlReq = (uint8_t)req;
            app_sv_send_command(OP_CLOUD_MUSIC_PLAYING_CONTROL, (uint8_t *)&request, sizeof(request), device_id);
        }
    }
}

static void app_sv_choose_cloud_platform(CLOUD_PLATFORM_E req, uint8_t device_id)
{
    CLOUD_PLATFORM_CHOOSING_REQ_T request;
    request.cloudPlatform = req;
    app_sv_send_command(OP_CHOOSE_CLOUD_PLATFORM, (uint8_t *)&request, sizeof(request), device_id);
}

void app_sv_switch_cloud_platform(CLOUD_PLATFORM_E platform, uint8_t device_id)
{
    if (platform == smartvoice_env.currentPlatform)
    {
        TRACE(2,"%s %d no need", __func__, platform);
        return;
    }

    smartvoice_env.currentPlatform = platform;
    app_sv_choose_cloud_platform(smartvoice_env.currentPlatform, device_id);
}

static void app_sv_choose_cloud_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{

}

static void app_sv_spp_data_ack_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    uint8_t connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_BES);
    if (AI_TRANSPORT_SPP == app_ai_get_transport_type(AI_SPEC_BES, connect_index))
    {
#if 0
        if (LengthOfCQueue(&(smartvoice_audio_stream.encodedDataBufQueue)) > 0)
        {
            app_smartvoice_send_encoded_audio_data();
        }
#endif
    }
}

static void app_sv_spp_data_ack_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{

}

static void app_sv_inform_algorithm_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
#ifdef SV_ENCODE_USE_SBC
    VOICE_SBC_CONFIG_INFO_T tInfo =
    {
        VOC_ENCODE_SBC,
        VOB_VOICE_CAPTURE_FRAME_CNT,
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOICE_SBC_PCM_DATA_SIZE_PER_FRAME,
        VOICE_SBC_BLOCK_SIZE        ,
        VOICE_SBC_CHANNEL_COUNT     ,
        VOICE_SBC_CHANNEL_MODE      ,
        VOICE_SBC_BIT_POOL          ,
        VOICE_SBC_SIZE_PER_SAMPLE   ,
        VOICE_SBC_SAMPLE_RATE       ,
        VOICE_SBC_NUM_BLOCKS        ,
        VOICE_SBC_NUM_SUB_BANDS     ,
        VOICE_SBC_MSBC_FLAG         ,
        VOICE_SBC_ALLOC_METHOD      ,
    };
#else
    VOICE_OPUS_CONFIG_INFO_T tInfo =
    {
        VOC_ENCODE_OPUS,
        VOB_VOICE_ENCODED_DATA_FRAME_SIZE,
        VOICE_OPUS_PCM_DATA_SIZE_PER_FRAME,
        VOB_VOICE_CAPTURE_FRAME_CNT,

        VOICE_OPUS_CHANNEL_COUNT,
        VOICE_OPUS_COMPLEXITY,
        VOICE_OPUS_PACKET_LOSS_PERC,
        VOICE_SIZE_PER_SAMPLE,
        VOICE_OPUS_APP,
        VOICE_OPUS_BANDWIDTH,
        VOICE_OPUS_BITRATE,
        VOICE_OPUS_SAMPLE_RATE,
        VOICE_SIGNAL_TYPE,
        VOICE_FRAME_PERIOD,

        VOICE_OPUS_USE_VBR,
        VOICE_OPUS_CONSTRAINT_USE_VBR,
        VOICE_OPUS_USE_INBANDFEC,
        VOICE_OPUS_USE_DTX,
    };
#endif

    uint32_t sentBytes = 0;
    uint8_t tmpBuf[sizeof(tInfo)+1];
    // the first byte will tell the offset of the info data, in case that
    // payloadPerPacket is smaller than the info size and multiple packets have
    // to be sent
    uint32_t payloadPerPacket = smartvoice_env.smartVoiceDataSectorSize - 1;

    TRACE(4,"%s SectorSize %d Info_size %d payloadPerPacket 0x%x", __func__, \
                                smartvoice_env.smartVoiceDataSectorSize, sizeof(tInfo), payloadPerPacket);
    while ((sizeof(tInfo) - sentBytes) >= payloadPerPacket)
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, payloadPerPacket);
        app_sv_send_response_to_command(funcCode, NO_ERROR, tmpBuf, payloadPerPacket + 1, device_id);

        sentBytes += payloadPerPacket;
    }

    if (sentBytes < sizeof(tInfo))
    {
        tmpBuf[0] = sentBytes;
        memcpy(&tmpBuf[1], ((uint8_t *)&tInfo) + sentBytes, sizeof(tInfo)-sentBytes);
        app_sv_send_response_to_command(funcCode, NO_ERROR, tmpBuf, sizeof(tInfo) - sentBytes + 1, device_id);
    }
}

void app_sv_notify_speech_state_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    SPEECH_STATE_RSP_T rsp;
    rsp.speechState = ((SPEECH_STATE_RSP_T *)ptrParam)->speechState;

    TRACE(2,"%s speechState %d", __func__, rsp.speechState);
    app_ai_set_speech_state((APP_AI_SPEECH_STATE_E)rsp.speechState, AI_SPEC_BES);
    app_sv_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&rsp, sizeof(rsp), device_id);
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, TRUE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, TRUE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, TRUE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#else
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_sv_cloud_music_playing_control_handler, FALSE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_START_SMART_VOICE_STREAM, app_sv_start_voice_stream_handler, FALSE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_start_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_STOP_SMART_VOICE_STREAM, app_sv_stop_voice_stream_handler, FALSE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_stop_voice_stream_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_sv_spp_data_ack_handler, FALSE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_spp_data_ack_rsp_handler);

#endif

CUSTOM_COMMAND_TO_ADD(OP_INFORM_ALGORITHM_TYPE, app_sv_inform_algorithm_handler, FALSE,
    0,    NULL);


CUSTOM_COMMAND_TO_ADD(OP_CHOOSE_CLOUD_PLATFORM, app_sv_choose_cloud_handler, FALSE,
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_NOTIFY_SPEECH_STATE, app_sv_notify_speech_state_handler, FALSE,
    0,    NULL);


static void app_sv_role_switch_rsp_timer_cb(void const *n);
osTimerDef (APP_SV_ROLE_SWITCH_RSP_TIMER, app_sv_role_switch_rsp_timer_cb);
osTimerId   app_sv_role_switch_rsp_timer_id = NULL;
#define APP_SV_ROLE_SWITCH_RSP_TIME_IN_MS 3000
#define APP_SV_ROLE_SWITCH_RSP_TIMES      5

static void app_sv_role_switch_rsp_timer_cb(void const *n)
{
    app_sv_send_command(OP_ROLE_SWITCH_REQUEST, NULL, 0, 0);
}

uint32_t app_sv_role_switch(void *param1, uint32_t param2, uint8_t param3)
{
    uint8_t *bt_addr = NULL;
    uint8_t dev_bt_addr[6];
    uint32_t bt_addr_crc32 = 0;

    if (app_ai_is_in_tws_mode(0))
    {
        bt_addr = app_ai_tws_local_address();
        if (bt_addr)
        {
            TRACE(1, "%s get bt_addr_crc32", __func__);
            memcpy(dev_bt_addr, bt_addr, 6);
            bt_addr_crc32 = crc32_c(bt_addr_crc32, dev_bt_addr, 6);
        }
        app_sv_send_command(OP_ROLE_SWITCH_REQUEST, (uint8_t *)&bt_addr_crc32, 4, param3);
    }
    else
    {
        app_sv_send_command(OP_ROLE_SWITCH_REQUEST, NULL, 0, param3);
    }
    return 0;
}

static void app_sv_role_switch_request_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"%s", __func__);
}

static void app_sv_role_switch_response_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    static uint8_t role_switch_rsp_times = 0;
    ROLE_SWITCH_RSP_T rsp;

    TRACE(3,"%s len %d status %d", __func__, paramLen, retStatus);

    if (WAITING_RSP_TIMEOUT == retStatus)
    {
        TRACE(1,"%s WAITING_RSP_TIMEOUT", __func__);
        //disconnect with AI, role switch
        goto role_switch_rsp_error;
    }

    if (NO_ERROR == retStatus)
    {
        rsp.canSwitchNow = ((ROLE_SWITCH_RSP_T *)ptrParam)->canSwitchNow;
        TRACE(2,"%s switch_now %d", __func__, rsp.canSwitchNow);
        if (false == rsp.canSwitchNow)
        {
            TRACE(1,"%s can't switch now", __func__);
            /* set a timer*/
            if (NULL == app_sv_role_switch_rsp_timer_id)
            {
                app_sv_role_switch_rsp_timer_id =
                    osTimerCreate(osTimer(APP_SV_ROLE_SWITCH_RSP_TIMER),
                    osTimerOnce, NULL);
            }

            if (role_switch_rsp_times < APP_SV_ROLE_SWITCH_RSP_TIMES)
            {
                role_switch_rsp_times++;
                osTimerStart(app_sv_role_switch_rsp_timer_id,
                    APP_SV_ROLE_SWITCH_RSP_TIME_IN_MS);
                TRACE(1,"%s start role switch rsp timer", __func__);
                return;
            }

            //disconnected with AI, role switch
            goto role_switch_rsp_error;
        }
    }
    else
    {
        TRACE(1,"%s role switch command error", __func__);
        if (role_switch_rsp_times < APP_SV_ROLE_SWITCH_RSP_TIMES)
        {
            role_switch_rsp_times++;
            app_sv_send_command(OP_ROLE_SWITCH_REQUEST, NULL, 0, device_id);
            return;
        }

        //disconnected with AI, role switch
        goto role_switch_rsp_error;
    }

    if (0 != role_switch_rsp_times)
    {
        role_switch_rsp_times = 0;
        if (NULL != app_sv_role_switch_rsp_timer_id)
        {
            osTimerStop(app_sv_role_switch_rsp_timer_id);
        }
    }

    app_ai_set_can_role_switch(true, AI_SPEC_BES);
    if (true == app_ai_is_need_ai_param(AI_SPEC_BES))
    {
        TRACE(1,"%s need ai param", __func__);
        return;
    }

role_switch_rsp_error:
    app_ai_tws_role_switch_dis_ble();
    return;
}

ROLE_SWITCH_AI_PARAM_T ai_param_t = {0};
static void app_sv_role_switch_ai_param_request_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{

    TRACE(2,"%s len %d", __func__, paramLen);

    if (app_ai_is_role_switching(AI_SPEC_BES) && app_ai_can_role_switch(AI_SPEC_BES))
    {
        app_sv_send_response_to_command(funcCode, NO_ERROR, NULL, 0, device_id);
        memcpy(ai_param_t.aiParam, ptrParam, paramLen);
        ai_param_t.param_len = paramLen;
        DUMP8("%x ", ptrParam, paramLen);

        app_ai_tws_role_switch_dis_ble();
    }
}

uint32_t app_sv_ai_param_send(void *param1, uint32_t param2, uint8_t param3)
{
    uint8_t *buf = (uint8_t *)param1;

    TRACE(2,"%s length %d", __func__, ai_param_t.param_len);

    buf[0] = ai_param_t.param_len;
    memcpy(buf+1, ai_param_t.aiParam, ai_param_t.param_len);
    return (ai_param_t.param_len+1);
}
uint32_t app_sv_ai_param_recieve(void *param1, uint32_t param2, uint8_t param3)
{
    uint8_t *buf = (uint8_t *)param1;
    uint8_t data_len = buf[0];

    TRACE(2,"%s length %d", __func__, data_len);
    memcpy(ai_param_t.aiParam, (buf+1), data_len);
    ai_param_t.param_len = data_len;
    DUMP8("%x ", ai_param_t.aiParam, data_len);
    return (data_len+1);
}

CUSTOM_COMMAND_TO_ADD(OP_ROLE_SWITCH_REQUEST, app_sv_role_switch_request_handler, TRUE,
    APP_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_sv_role_switch_response_handler );
CUSTOM_COMMAND_TO_ADD(OP_ROLE_SWITCH_AI_PARAM, app_sv_role_switch_ai_param_request_handler, false,
    0,    NULL );

uint32_t app_sv_voice_send_done(void *param1, uint32_t param2, uint8_t param3)
{
    //TRACE(2,"%s credit %d", __func__, ai_struct.tx_credit);
    if(ai_struct[AI_SPEC_BES].tx_credit < MAX_TX_CREDIT) {
        ai_struct[AI_SPEC_BES].tx_credit++;
    }

    if ((app_ai_get_data_trans_size(AI_SPEC_BES) <= voice_compression_get_encode_buf_size()) ||
        ai_transport_has_cmd_to_send())
    {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_BES), AI_SPEC_BES, param3);
    }

    return 0;
}

uint32_t app_sv_key_event_handle(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

    TRACE(3,"%s code 0x%x event %d", __func__, status->code, status->event);

    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return 1;
    }

    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_BES))
    {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event)
        {
            app_sv_wake_up(status, 0, param3);
        }
        else if (APP_KEY_EVENT_UP == status->event)
        {
            app_sv_stop_voice_stream(status, 0, param3);
        }
    }
    else
    {
        if (APP_KEY_EVENT_CLICK == status->event)
        {
            app_sv_wake_up(status, 0, param3);
        }
    }

    return 0;
}

/// TENCENT handlers function definition
const struct ai_func_handler smartvoice_handler_function_list[] =
{
    {API_HANDLE_INIT,               (ai_handler_func_t)app_sv_handle_init},
    {API_START_ADV,                 (ai_handler_func_t)app_sv_start_advertising},
    {API_DATA_HANDLE,               (ai_handler_func_t)app_sv_audio_stream_handle},
    {API_DATA_SEND,                 (ai_handler_func_t)app_sv_send_voice_stream},
    {API_DATA_INIT,                 (ai_handler_func_t)app_sv_stream_init},
    {API_DATA_DEINIT,               (ai_handler_func_t)app_sv_stream_deinit},
    {API_AI_ROLE_SWITCH,            (ai_handler_func_t)app_sv_role_switch},
    {CALLBACK_CMD_RECEIVE,          (ai_handler_func_t)app_sv_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,         (ai_handler_func_t)app_sv_rcv_data},
    {CALLBACK_DATA_PARSE,           (ai_handler_func_t)app_sv_parse_cmd},
    {CALLBACK_AI_CONNECT,           (ai_handler_func_t)app_sv_voice_connected},
    {CALLBACK_AI_DISCONNECT,        (ai_handler_func_t)app_sv_voice_disconnected},
    {CALLBACK_UPDATE_MTU,           (ai_handler_func_t)app_sv_config_mtu},
    {CALLBACK_WAKE_UP,              (ai_handler_func_t)app_sv_wake_up},
    {CALLBACK_AI_ENABLE,            (ai_handler_func_t)app_sv_enable_handler},
    {CALLBACK_START_SPEECH,         (ai_handler_func_t)NULL},
    {CALLBACK_ENDPOINT_SPEECH,      (ai_handler_func_t)app_sv_stop_voice_stream},
    {CALLBACK_STOP_SPEECH,          (ai_handler_func_t)app_sv_stop_voice_stream},
    {CALLBACK_DATA_SEND_DONE,       (ai_handler_func_t)app_sv_voice_send_done},
    {CALLBACK_AI_PARAM_SEND,        (ai_handler_func_t)app_sv_ai_param_send},
    {CALLBACK_AI_PARAM_RECEIVE,     (ai_handler_func_t)app_sv_ai_param_recieve},
    {CALLBACK_KEY_EVENT_HANDLE,     (ai_handler_func_t)app_sv_key_event_handle},
};

const struct ai_handler_func_table smartvoice_handler_function_table =
    {&smartvoice_handler_function_list[0], (sizeof(smartvoice_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_BES, &smartvoice_handler_function_table)

