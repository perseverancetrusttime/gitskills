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
#include "hal_location.h"
//#include "aes.h"
#include "tgt_hardware.h"
#include "app_key.h"
#include "factory_section.h"

#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_gma_handle.h"
#include "app_gma_cmd_code.h"
#include "app_gma_cmd_handler.h"
#include "app_gma_data_handler.h"
#include "app_gma_ble.h"
#include "app_gma_bt.h"
#include "voice_compression.h"
#include "app_gma_state_env.h"
#include "gma_crypto.h"
#include "app_gma_ota.h"
//#include "app_tws_ui.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif
#include "app_bt_media_manager.h"
#include "app_ai_tws.h"
#include "nvrecord_env.h"
#include "nvrecord.h"


#ifdef BLE_V2
#include "gma_gatt_server.h"
#endif


extern uint8_t bt_addr[6];

#ifdef BLE_V2
GMA_GATT_ADV_DATA_PAC gma_gatt_adv_data = {GMA_GATT_ADV_DATA_LENGTH, GMA_GATT_ADV_DATA_TYPE, GMA_GATT_ADV_DATA_CID, GMA_GATT_ADV_DATA_VID, GMA_GATT_ADV_DATA_FMSK, GMA_GATT_ADV_DATA_PID, };
#endif

extern "C" bool app_gma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);
extern "C" bool app_gma_send_data_via_notification(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);

extern "C" bool app_gma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);
extern "C" bool app_gma_send_data_via_indication(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);



#define APP_GMA_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS 5000
#define GMA_SYSTEM_FREQ       APP_SYSFREQ_208M   

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


} APP_GMA_AUDIO_STREAM_ENV_T;           

typedef enum
{
    CLOUD_PLATFORM_0,
    CLOUD_PLATFORM_1,
    CLOUD_PLATFORM_2,
} CLOUD_PLATFORM_E;

typedef enum
{
    WITHOUT_RSP = 0,
    WITH_RSP
} THROUGHPUT_TEST_RESPONSE_MODEL_E;

typedef struct
{
    uint8_t     dataPattern;
    uint16_t    lastTimeInSecond;
    uint16_t    dataPacketSize;
    uint8_t     direction;
    uint8_t     responseModel;
    uint8_t     isToUseSpecificConnParameter;
    uint16_t    minConnIntervalInMs;
    uint16_t    maxConnIntervalInMs;
    uint8_t     reserve[4];
} __attribute__((__packed__)) THROUGHPUT_TEST_CONFIG_T;

static THROUGHPUT_TEST_CONFIG_T throughputTestConfig;

typedef struct {
    volatile uint8_t                     isGmaStreamRunning   : 1;
    volatile uint8_t                     isDataXferOnGoing   : 1;
    volatile uint8_t                     isPlaybackStreamRunning : 1;
    volatile uint8_t                     isThroughputTestOn  : 1;
    uint8_t                     reserve             : 4;
    uint8_t                     connType;   
    uint8_t                     conidx;
    uint16_t                    gmaVoiceDataSectorSize;
    CLOUD_PLATFORM_E            currentPlatform;
    
} gma_context_t;

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
    uint8_t           cloudPlatform;
} CLOUD_PLATFORM_CHOOSING_REQ_T;

static gma_context_t                 gma_env;
#if (MIX_MIC_DURING_MUSIC)
static uint32_t more_mic_data(uint8_t* ptrBuf, uint32_t length);
#endif

extern void app_hfp_enable_audio_link(bool isEnable);
extern struct nvrecord_env_t *nvrecord_env_p;
void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len);

#define APP_SCNRSP_DATA         "\x11\x07\xa2\xaf\x34\xd6\xba\x17\x53\x8b\xd4\x4d\x3c\x8a\xdc\x0c\x3e\x26"
#define APP_SCNRSP_DATA_LEN     (18)

static uint32_t app_gma_start_advertising(void *param1, uint32_t param2)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T *)param1;
    char _bt_addr[6];
    uint8_t extend_data[2];
    extend_data[0]=0x06;
    extend_data[1]=0x00;

    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    memcpy((void *)_bt_addr, (void *)p_ibrt_ctrl->local_addr.address, 6);

    memcpy(gma_gatt_adv_data.gma_gatt_adv_mac, _bt_addr, 6);

    memcpy(gma_gatt_adv_data.gam_gatt_adv_extend,extend_data,2);

    memcpy(&cmd->advData[cmd->advDataLen], (uint8_t*)&gma_gatt_adv_data, GMA_GATT_ADV_LENGTH);
    cmd->advDataLen += GMA_GATT_ADV_LENGTH;

    cmd->advData[cmd->advDataLen++] = 0x03;
    cmd->advData[cmd->advDataLen++] = 0x16; /* Service Data */
    cmd->advData[cmd->advDataLen++] = (GMA_GATT_SERVICE_UUID >> 0) & 0xFF;
    cmd->advData[cmd->advDataLen++] = (GMA_GATT_SERVICE_UUID >> 8) & 0xFF;

    TRACE(0,"app_gma_start_advertising");
    DUMP8("%02x ", cmd->advData, cmd->advDataLen);
    return 1;
}

//all ota should use notification
//other use indication
#ifdef __AI_VOICE_BLE_ENABLE__
static bool app_gma_send_data(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    uint32_t cmd = ptrData[1];
    TRACE(1,"**send data:%x", cmd);
    DUMP8("0x%02x ",ptrData,length);
    if(cmd == GMA_OP_DEV_SEND_STREAM)
        app_gma_send_data_via_notification(ptrData, length, AI_SPEC_ALI, device_id);
    else if(cmd < GMA_OP_OTA_VER_GET_CMD || cmd >GMA_OP_OTA_SEND_PKG_CMD)
        app_gma_send_data_via_indication(ptrData, length, AI_SPEC_ALI, device_id);
    else
        app_gma_send_data_via_notification(ptrData, length, AI_SPEC_ALI,device_id);

    return true;
}

static bool app_gma_send_cmd(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    uint32_t cmd = ptrData[1];
    TRACE(2,"%s, **send cmd:%x", __func__, cmd);
    if(cmd < GMA_OP_OTA_VER_GET_CMD || cmd >GMA_OP_OTA_SEND_PKG_CMD)
        app_gma_send_cmd_via_indication(ptrData, length, AI_SPEC_ALI, device_id);
    else
        app_gma_send_cmd_via_notification(ptrData, length, AI_SPEC_ALI, device_id);

    return true;
}
#endif

uint32_t app_gma_voice_connected(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s connect_type %d",__func__, param2);

    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1;
    uint8_t device_id = (uint8_t)param3;

    uint8_t ai_connect_index = app_ai_connect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_ALI, device_id, btaddr->address);
    if(param2 == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send, AI_SPEC_ALI, AI_TRANSPORT_SPP);
        ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_ALI, AI_TRANSPORT_SPP);
        app_ai_set_mtu(TX_SPP_MTU_SIZE>255?255:TX_SPP_MTU_SIZE, AI_SPEC_ALI);
        app_ai_set_data_trans_size(120,AI_SPEC_ALI);
    }
#ifdef __AI_VOICE_BLE_ENABLE__
    else if(param2 == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_gma_send_data, AI_SPEC_ALI, AI_TRANSPORT_BLE);
        ai_cmd_transport_init(app_gma_send_cmd, AI_SPEC_ALI, AI_TRANSPORT_BLE);
        //if ble connected, close spp;
        app_ai_spp_disconnlink(AI_SPEC_ALI, ai_connect_index);
        app_ai_set_data_trans_size(120,AI_SPEC_ALI);
    }
#endif

    AIV_HANDLER_BUNDLE_T handlerBundle = {
        .upstreamDataHandler    = app_ai_voice_default_upstream_data_handler,
         };
    app_ai_voice_register_ai_handler(AIV_USER_GMA,&handlerBundle);
    app_gma_data_reset_env();
    app_ai_setup_complete_handle(AI_SPEC_ALI);

    return 0;
}

uint32_t app_gma_voice_disconnected(void *param1, uint32_t param2, uint8_t parma3)
{
     uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    TRACE(2,"%s transport_type %d",__func__, ai_struct[AI_SPEC_ALI].transport_type[foreground_ai]);

    // reset the security state
    app_gma_encrypt_reset();
    
    app_gma_data_reset_env();
    app_gma_stop_throughput_test();

//    ai_set_power_key_stop();
//    app_ai_spp_open();

    app_ai_disconnect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_ALI);
  //  app_gma_ota_finished_handler();
    return 0;
}

uint32_t app_gma_voice_send_done(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s credit %d", __func__, ai_struct[AI_SPEC_ALI].tx_credit);
    if(ai_struct[AI_SPEC_ALI].tx_credit < MAX_TX_CREDIT)
    {
        ai_struct[AI_SPEC_ALI].tx_credit++;
    }

    if ((app_ai_get_data_trans_size(AI_SPEC_ALI) <= voice_compression_get_encode_buf_size()) ||
        ai_transport_has_cmd_to_send())
    {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_ALI), AI_SPEC_ALI, param3);
    }

    if (gma_env.isThroughputTestOn &&(WITHOUT_RSP == throughputTestConfig.responseModel))
    {
        app_gma_throughput_test_transmission_handler(NULL, 0, param3);
    }

    return 0;
}

void app_gma_start_xfer(void)
{
    TRACE(1,"%s", __func__);
    app_gma_kickoff_dataxfer();
}

void app_gma_stop_xfer(void)
{
   TRACE(1,"%s", __func__);
    app_gma_stop_dataxfer();
}

void app_gma_data_xfer_started(APP_GMA_CMD_RET_STATUS_E retStatus)
{
    TRACE(2,"%s ret %d", __func__, retStatus);
}

void app_gma_data_xfer_stopped(APP_GMA_CMD_RET_STATUS_E retStatus)
{
    app_hfp_enable_audio_link(false);
    TRACE(2,"%s ret %d", __func__, retStatus);
}

void app_gma_start_voice_stream_cmd(uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    

    if (!ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
    {
        APP_GMA_STAT_CHANGE_CMD_T req = {0x00, 0x00};
        app_gma_send_command(GMA_OP_DEV_CHANGE_CMD, (uint8_t *)&req, sizeof(req), 1, device_id);
    }
    else
    {   
        TRACE(1,"%s ai_stream has open", __func__);
    }
}

uint32_t app_gma_config_mtu(void *param1, uint32_t param2, uint8_t param3)  
{
    TRACE(2,"%s is %d", __func__, param2);
    uint32_t mtu = param2>255?255:param2;
    app_ai_set_mtu(mtu,AI_SPEC_ALI);

    TRACE(1,"ai_struct.mtu is %d", mtu);

    return 0;
}

extern "C" void app_gma_ota_handler_init(void);

uint32_t app_gma_handle_init(void *param1, uint32_t param2)
{
    TRACE(1,"%s", __func__);

    memset(&gma_env,         0x00, sizeof(gma_env));

    app_gma_cmd_handler_init();
    app_gma_data_reset_env();

    app_gma_throughput_test_init();

   // app_gma_device_infor_reset();
    app_gma_state_env_reset();
    app_gma_ota_handler_init();

    app_gma_device_infor_reset();

    return 0;
}

void app_gma_start_voice_stream(uint8_t device_id)
{
    TRACE(2,"%s ai_stream_opened %d", __func__, ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened);

    if (!ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened) {
        app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
        TRACE(0,"Start gma voice stream.");
        ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened = true;
        if (!app_ai_is_use_thirdparty(AI_SPEC_ALI))
        {
            app_ai_voice_upstream_control(true, AIV_USER_GMA,UPSTREAM_INVALID_READ_OFFSET);
            /// update the read offset for AI voice module
            app_ai_voice_update_handle_frame_len(GMA_VOICE_ONESHOT_PROCESS_LEN);
            ai_mailbox_put(SIGN_AI_WAKEUP, 0,AI_SPEC_ALI,device_id);
        }
        app_ai_voice_stream_init(AIV_USER_GMA);
        app_ai_spp_deinit_tx_buf();

    }
}

void app_gma_stop_stream(void)
{
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    if (AI_TRANSPORT_IDLE != ai_struct[AI_SPEC_ALI].transport_type[foreground_ai]) {
        if (ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened) {
            app_ai_voice_upstream_control(false, AIV_USER_GMA, 0);
            app_ai_voice_stream_control(false, AIV_USER_GMA);
            app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
            app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, BT_DEVICE_ID_1);
        }
    }
}

uint32_t app_gma_stop_voice_stream(void *param1, uint32_t param2)
{
    TRACE(2,"%s ai_stream_opened %d", __func__, ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened);
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    if (AI_TRANSPORT_IDLE != ai_struct[AI_SPEC_ALI].transport_type[foreground_ai]) {
        if (ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened) {
            app_ai_voice_upstream_control(false, AIV_USER_GMA, 0);
            app_ai_voice_stream_control(false, AIV_USER_GMA);
            app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
            app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, BT_DEVICE_ID_1);
            app_gma_stop_xfer();
            //app_gma_dev_data_xfer(GMA_DEV_STA_REC_STOP);
        }
    }
    return 0;
}

uint32_t app_gma_wake_up(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
//    app_tws_ibrt_exit_sniff_with_mobile();
//    app_tws_ibrt_exit_sniff_with_tws();
    TRACE(4,"%s transport_type %d Wake_up_type %d event %d",__func__, ai_struct[AI_SPEC_ALI].transport_type[foreground_ai], ai_struct[AI_SPEC_ALI].wake_up_type, status->event);

    if (ai_struct[AI_SPEC_ALI].transport_type[foreground_ai] == AI_TRANSPORT_IDLE){
       TRACE(1,"%s not ai connect", __func__);
       return 1;
    }
       
    if (app_ai_is_sco_mode()){
        TRACE(1,"%s is in sco mode", __func__);
        return 2;
    }

    if(app_ai_tws_get_local_role() == APP_AI_TWS_SLAVE)
    {
        return 3;
    }
    TRACE(0,"gma_wakeup_app_key!");
    app_ai_spp_deinit_tx_buf();

    if(AI_TRANSPORT_BLE == ai_struct[AI_SPEC_ALI].transport_type[foreground_ai] || AI_TRANSPORT_SPP == ai_struct[AI_SPEC_ALI].transport_type[foreground_ai])
    {     //use ble
        app_gma_switch_stream_status(param3);    //send start cmd;
    }
    else
    {
        TRACE(0,"it is error, not ble or spp connected");
        return 4;
    }
            //mute the music;

//    ai_audio_stream_allowed_to_send_set(true);
    app_gma_start_voice_stream(param3);
    ai_audio_stream_allowed_to_send_set(true, AI_SPEC_ALI);
    TRACE(2,"%s %d", __func__, ai_struct[AI_SPEC_ALI].transfer_voice_start);
    app_gma_start_xfer(); 

    return 0;
}

uint32_t app_gma_enable_handler(void *param1, uint32_t param2)
{
    TRACE(2,"%s %d", __func__, param2);
    return 0;
}

uint32_t app_gma_key_event_handle(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

    TRACE(3,"%s code 0x%x event %d", __func__, status->code, status->event);

    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return 1;
    }

    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_ALI))
    {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event)
        {
            app_gma_wake_up(status, 0, param3);
        }
        else if (APP_KEY_EVENT_UP == status->event)
        {
            TRACE(1,"%s stop speech", __func__);
            app_gma_stop_voice_stream(status, 0);
        }
    }
    else
    {
        if (APP_KEY_EVENT_CLICK == status->event)
        {
            app_gma_wake_up(status, 0, param3);
        }
    }

    return 0;
}

uint32_t app_gma_stream_init(void *param1, uint32_t param2,uint8_t param3)
{
    return 0;
}

uint32_t app_gma_stream_deinit(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}

void app_gma_start_voice_stream_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;
    TRACE(1,"Received the wakeup req from the remote dev. Voice Path type %d", voicePathType);
    
    if (!ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
    {     
        if ((APP_GMA_PATH_TYPE_BLE == voicePathType) ||
            (APP_GMA_PATH_TYPE_SPP == voicePathType)) {
            if (app_ai_is_sco_mode()) {
                TRACE(1,"%s CMD_HANDLING_FAILED", __func__);
            } else {
                app_gma_dev_data_xfer(GMA_DEV_STA_REC_START);
                
                app_gma_start_voice_stream(device_id);
                // start the data xfer
                app_gma_start_xfer(); 
            }
        } else if ((APP_GMA_PATH_TYPE_HFP == voicePathType) &&
            !app_ai_is_sco_mode()) {

        } else {
            ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened = true;
            app_gma_dev_data_xfer(GMA_DEV_STA_REC_START);
        }
    } else {

    }
}

void app_gma_status_change_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(3,"%s line:%d funcCode:0x%02x", __func__, __LINE__, funcCode);

    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();

    if(app_ai_tws_get_local_role() == APP_AI_TWS_SLAVE)
    {
        return;
    }

    if (GMA_OP_PHO_CHANGE_CMD == funcCode)
    {
        APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_PHO_CHANGE_RSP;
        APP_GMA_STAT_CHANGE_CMD_T* cmd = (APP_GMA_STAT_CHANGE_CMD_T*)ptrParam;
        APP_GMA_STAT_CHANGE_RSP_T resp = {cmd->cmdType, sizeof(APP_GMA_STAT_CHANGE_RSP_T) - GMA_TLV_HEADER_SIZE, GMA_DEV_STA_CHANGE_SUCS};

        TRACE(3,"%s line:%d cmdType:0x%02x", __func__, __LINE__, cmd->cmdType);
        switch(cmd->cmdType)
        {
            case GMA_PHO_STA_CON_READY:
            {
                break;
            }
            case GMA_PHO_STA_LISTENING:
            {
                if (!ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
                {
                // start the data xfer
                    app_gma_start_voice_stream(device_id);
                    app_gma_start_xfer();
                    resp.value = GMA_DEV_STA_CHANGE_SUCS;
                }
                else
                {
                    //resp.value = GMA_DEV_STA_CHANGE_FAIL;
                }
                break;
            }
            case GMA_PHO_STA_THINKING:
            {
                if (AI_TRANSPORT_IDLE != ai_struct[AI_SPEC_ALI].transport_type[foreground_ai])
                {
                    if (ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
                    {
                        gma_env.isDataXferOnGoing = false;
                        app_gma_stop_stream();
                        app_gma_stop_xfer();
                        resp.value = GMA_DEV_STA_CHANGE_SUCS;
                    }
                    else
                    {
                        //resp.value = GMA_DEV_STA_CHANGE_FAIL;
                    }
                }
                else
                {
                    //resp.value = GMA_DEV_STA_CHANGE_FAIL;
                }
                
                break;
            }
            case GMA_PHO_STA_SPEAKING:
            {
                break;
            }
            case GMA_PHO_STA_IDLE:
            {
                break;
            }
            case GMA_PHO_STA_START_RECORD:
            {
                break;
            }
            case GMA_PHO_STA_STOP_RECORD:
            {
                break;
            }
        }
        app_gma_send_command(rspCmdcode, (uint8_t*)&resp, (uint32_t)sizeof(APP_GMA_STAT_CHANGE_RSP_T), false, device_id);
    }
}

void app_gma_switch_stream_status(uint8_t device_id)
{
    if (ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
    {
        app_gma_stop_voice_stream(NULL, 0);
    }
    else
    {
        app_gma_start_voice_stream_cmd(device_id);
    }
}

static void app_gma_cloud_music_playing_control_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"%s funcCode:0x%02x", __func__, funcCode);
    TRACE(0,"Received the cloud music playing control request from the remote dev.");
}

static void app_gma_cloud_music_playing_control_rsp_handler(APP_GMA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    TRACE(1,"Music control returns %d", retStatus);
}

void app_gma_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req, uint8_t device_id)
{
    if (gma_env.connType > 0)
    {
        if (ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened)
        {
            CLOUD_MUSIC_PLAYING_CONTROL_REQ_T request;
            request.cloudMusicPlayingControlReq = (uint8_t)req;
            app_gma_send_command(OP_CLOUD_MUSIC_PLAYING_CONTROL, (uint8_t *)&request, sizeof(request), 0, device_id);
        }
    }
}

static void app_gma_choose_cloud_platform(CLOUD_PLATFORM_E req, uint8_t device_id)
{
    CLOUD_PLATFORM_CHOOSING_REQ_T request;
    request.cloudPlatform = req;
    app_gma_send_command(OP_CHOOSE_CLOUD_PLATFORM, (uint8_t *)&request, sizeof(request), 0, device_id);
}

void app_gma_switch_cloud_platform(uint8_t device_id)
{
    if (CLOUD_PLATFORM_0 == gma_env.currentPlatform)
    {
        gma_env.currentPlatform = CLOUD_PLATFORM_1;
    }
    else
    {
        gma_env.currentPlatform = CLOUD_PLATFORM_0;
    }
    app_gma_choose_cloud_platform(gma_env.currentPlatform, device_id);
}

static void app_gma_spp_data_ack_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"%s", __func__);
     uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    if (AI_TRANSPORT_SPP == ai_struct[AI_SPEC_ALI].transport_type[foreground_ai])
    {
        gma_env.isDataXferOnGoing = false;
#if 0
        if (LengthOfCQueue(&(gma_audio_stream.encodedDataBufQueue)) > 0)
        {
            app_gma_send_encoded_audio_data();
        }
#endif
    }
}

static void app_gma_spp_data_ack_rsp_handler(APP_GMA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{

}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_gma_cloud_music_playing_control_handler, false,  
    APP_GMA_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_gma_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_gma_spp_data_ack_handler, FALSE,      
    APP_GMA_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_gma_spp_data_ack_rsp_handler);

#else
CUSTOM_COMMAND_TO_ADD(OP_CLOUD_MUSIC_PLAYING_CONTROL, app_gma_cloud_music_playing_control_handler, FALSE,  
    APP_GMA_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_gma_cloud_music_playing_control_rsp_handler );

CUSTOM_COMMAND_TO_ADD(OP_SPP_DATA_ACK, app_gma_spp_data_ack_handler, FALSE,      
    APP_GMA_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,    app_gma_spp_data_ack_rsp_handler);

#endif
CUSTOM_COMMAND_TO_ADD(GMA_OP_PHO_CHANGE_CMD,    app_gma_status_change_handler, FALSE, 0, NULL );

typedef enum
{
    PATTERN_RANDOM = 0,
    PATTERN_11110000,
    PATTERN_10101010,
    PATTERN_ALL_1,
    PATTERN_ALL_0,
    PATTERN_00001111,
    PATTERN_0101,
} THROUGHPUT_TEST_DATA_PATTER_E;

typedef enum
{
    UP_STREAM = 0,
    DOWN_STREAM
} THROUGHPUT_TEST_DATA_DIRECTION_E;

#define APP_GMA_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS    2000
static void app_gma_throughput_pre_config_pending_timer_cb(void const *n);
osTimerDef (APP_GMA_THROUGHPUT_PRE_CONFIG_PENDING_TIMER, app_gma_throughput_pre_config_pending_timer_cb); 
osTimerId   app_gma_throughput_pre_config_pending_timer_id = NULL;

static void app_gma_throughput_test_data_xfer_lasting_timer_cb(void const *n);
osTimerDef (APP_GMA_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER, app_gma_throughput_test_data_xfer_lasting_timer_cb); 
osTimerId   app_gma_throughput_test_data_xfer_lasting_timer_id = NULL;


static uint8_t app_gma_throughput_datapattern[MAXIMUM_DATA_PACKET_LEN];

static void app_gma_throughput_test_data_xfer_lasting_timer_cb(void const *n)
{
    app_gma_send_command(OP_THROUGHPUT_TEST_DONE, NULL, 0, 0, 0);

    app_gma_stop_throughput_test();
}

static void app_gma_throughput_pre_config_pending_timer_cb(void const *n)
{
    // inform the configuration
    app_gma_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
        (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig), 0,0);

    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_gma_throughput_test_transmission_handler(NULL, 0, 0);
        osTimerStart(app_gma_throughput_test_data_xfer_lasting_timer_id, 
            throughputTestConfig.lastTimeInSecond*1000);
    }
}

void app_gma_throughput_test_init(void)
{
    app_gma_throughput_pre_config_pending_timer_id = 
        osTimerCreate(osTimer(APP_GMA_THROUGHPUT_PRE_CONFIG_PENDING_TIMER), 
        osTimerOnce, NULL);

    app_gma_throughput_test_data_xfer_lasting_timer_id = 
        osTimerCreate(osTimer(APP_GMA_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER), 
        osTimerOnce, NULL);
}

uint32_t app_gma_throughput_test_transmission_handler(void *param1, uint32_t param2,uint8_t param3)
{
    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_gma_send_command(OP_THROUGHPUT_TEST_DATA, 
            app_gma_throughput_datapattern, throughputTestConfig.dataPacketSize - 4, 0, param3);  
    }
    return ((uint32_t)throughputTestConfig.direction);
}

static void app_gma_throughput_test_data_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();
    if ((WITH_RSP == throughputTestConfig.responseModel) && 
        (AI_TRANSPORT_SPP == ai_struct[AI_SPEC_ALI].transport_type[foreground_ai]))
    {
        app_gma_send_command(OP_THROUGHPUT_TEST_DATA_ACK, NULL, 0, 0, device_id); 
    }
}

static void app_gma_throughput_test_data_ack_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    if (gma_env.isThroughputTestOn &&
        (WITH_RSP == throughputTestConfig.responseModel))
    {
        app_gma_throughput_test_transmission_handler(NULL, 0, device_id);
    }
}

static void app_gma_throughput_test_done_signal_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    app_gma_stop_throughput_test();
}

void app_gma_stop_throughput_test(void)
{
    gma_env.isThroughputTestOn = false;
    osTimerStop(app_gma_throughput_pre_config_pending_timer_id);
    osTimerStop(app_gma_throughput_test_data_xfer_lasting_timer_id);
}

static void app_gma_throughput_test_config_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    throughputTestConfig = *(THROUGHPUT_TEST_CONFIG_T *)ptrParam;

    TRACE(1,"data patter is %d", throughputTestConfig.dataPattern);

    uint8_t foreground_ai = ai_manager_get_foreground_ai_conidx();

    // generate the data pattern
    switch (throughputTestConfig.dataPattern)
    {
        case PATTERN_RANDOM:
        {
            for (uint32_t index = 0;index < MAXIMUM_DATA_PACKET_LEN;index++)
            {
                app_gma_throughput_datapattern[index] = (uint8_t)rand();                
            }
            break;
        }
        case PATTERN_11110000:
        {
            memset(app_gma_throughput_datapattern, 0xF0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_10101010:
        {
            memset(app_gma_throughput_datapattern, 0xAA, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_1:
        {
            memset(app_gma_throughput_datapattern, 0xFF, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_0:
        {
            memset(app_gma_throughput_datapattern, 0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_00001111:
        {
            memset(app_gma_throughput_datapattern, 0x0F, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_0101:
        {
            memset(app_gma_throughput_datapattern, 0x55, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        default:
            throughputTestConfig.dataPattern = 0;
            break;        
    }

    gma_env.isThroughputTestOn = true;

    // check whether need to use the new conn parameter
    if (AI_TRANSPORT_BLE == ai_struct[AI_SPEC_ALI].transport_type[foreground_ai]
        && throughputTestConfig.isToUseSpecificConnParameter)
    {
       /* app_ai_update_conn_parameter(gma_env.conidx, throughputTestConfig.minConnIntervalInMs,
            throughputTestConfig.maxConnIntervalInMs, 
            AI_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,
            AI_BLE_CONNECTION_SLAVELATENCY);   */    
    
        osTimerStart(app_gma_throughput_pre_config_pending_timer_id, 
            APP_GMA_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS);
    }
    else
    {
        app_gma_send_command(OP_INFORM_THROUGHPUT_TEST_CONFIG, 
            (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig), 0,device_id);

        if (UP_STREAM == throughputTestConfig.direction)
        {
            app_gma_throughput_test_transmission_handler(NULL, 0, device_id);
            osTimerStart(app_gma_throughput_test_data_xfer_lasting_timer_id, 
                throughputTestConfig.lastTimeInSecond*1000);
        }
    }
}

CUSTOM_COMMAND_TO_ADD(OP_INFORM_THROUGHPUT_TEST_CONFIG, app_gma_throughput_test_config_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA, app_gma_throughput_test_data_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DATA_ACK, app_gma_throughput_test_data_ack_handler, FALSE,  
    0,    NULL );

CUSTOM_COMMAND_TO_ADD(OP_THROUGHPUT_TEST_DONE, app_gma_throughput_test_done_signal_handler, FALSE,  
    0,    NULL );

/// TENCENT handlers function definition
const struct ai_func_handler gma_handler_function_list[] =
{
    {API_HANDLE_INIT,         (ai_handler_func_t)app_gma_handle_init},
    {API_START_ADV,           (ai_handler_func_t)app_gma_start_advertising},
    {API_DATA_SEND,           (ai_handler_func_t)app_gma_send_voice_stream},
    {API_DATA_HANDLE,         (ai_handler_func_t)app_gma_audio_stream_handle},
    {API_DATA_INIT,           (ai_handler_func_t)app_gma_stream_init},
    {API_DATA_DEINIT,         (ai_handler_func_t)app_gma_stream_deinit},
    {CALLBACK_CMD_RECEIVE,    (ai_handler_func_t)app_gma_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,   (ai_handler_func_t)app_gma_rcv_data},
    {CALLBACK_DATA_PARSE,     (ai_handler_func_t)app_gma_parse_cmd},
    {CALLBACK_AI_CONNECT,     (ai_handler_func_t)app_gma_voice_connected},
    {CALLBACK_AI_DISCONNECT,  (ai_handler_func_t)app_gma_voice_disconnected},
    {CALLBACK_UPDATE_MTU,     (ai_handler_func_t)app_gma_config_mtu},
    {CALLBACK_WAKE_UP,        (ai_handler_func_t)app_gma_wake_up},
    {CALLBACK_AI_ENABLE,      (ai_handler_func_t)app_gma_enable_handler},
    {CALLBACK_START_SPEECH,   (ai_handler_func_t)NULL},
    {CALLBACK_ENDPOINT_SPEECH,(ai_handler_func_t)app_gma_stop_voice_stream},
    {CALLBACK_STOP_SPEECH,    (ai_handler_func_t)app_gma_stop_voice_stream},
    {CALLBACK_DATA_SEND_DONE, (ai_handler_func_t)app_gma_voice_send_done},
    {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)app_gma_key_event_handle},
};

const struct ai_handler_func_table gma_handler_function_table =
    {&gma_handler_function_list[0], (sizeof(gma_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_ALI, &gma_handler_function_table)

