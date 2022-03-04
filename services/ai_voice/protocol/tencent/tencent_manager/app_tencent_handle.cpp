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
#include "app_a2dp.h"
#include "app_key.h"
#include "hal_location.h"


#include "aes.h"
#include "tgt_hardware.h"

#include "nvrecord.h"
#include "nvrecord_env.h"

#include "voice_compression.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_tencent_handle.h"
#include "app_tencent_sv_cmd_code.h"
#include "app_tencent_sv_cmd_handler.h"
#include "app_tencent_sv_data_handler.h"
#include "app_tencent_ble.h"
#include "app_tencent_bt.h"
#include "app_tencent_voicepath.h"
#include "app_tencent_ota.h"
#include "app_key.h"

#ifdef BLE_V2
#include "tencent_gatt_server.h"
#include "app_ibrt_if.h"
#endif




extern uint8_t bt_addr[6];

#define APP_TENCENT_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS 5000

#if IS_TENCENT_SMARTVOICE_DEBUG_ON
#define TENCENT_SMARTVOICE_TRACE    TRACE
#else
#define TENCENT_SMARTVOICE_TRACE(str, ...)
#endif


typedef enum {
    CLOUD_PLATFORM_0,
    CLOUD_PLATFORM_1,
    CLOUD_PLATFORM_2,
} CLOUD_PLATFORM_E;

typedef struct {
    volatile uint8_t                     isSvStreamRunning   : 1;
    volatile uint8_t                     isDataXferOnGoing   : 1;
    volatile uint8_t                     isPlaybackStreamRunning : 1;
    volatile uint8_t                     isThroughputTestOn  : 1;
    uint8_t                     reserve             : 4;
    uint8_t                     conidx;
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
    uint8_t           cloudPlatform;
} CLOUD_PLATFORM_CHOOSING_REQ_T;




static smartvoice_context_t   tencent_env;

extern void app_hfp_enable_audio_link(bool isEnable);
void app_tencent_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len);

extern bool bt_media_is_media_active_by_sbc(void);
extern void app_a2dp_set_skip_frame(uint8_t frames);

/*
 * x11 - Length
 * x07 - Complete list of 16-bit UUIDs available
 * .... the 128 bit UUIDs
 */
//#define APP_TENCENT_SMARTVOICE_ADV_DATA_UUID        "\x1a\xff\x4c\x00\x02\x15\x7b\x93\x11\x04\x39\x23\x4c\xbc\x94\xda\x87\x5c\x80\x67\xf8\x45\x00\x01\x00\x02\xc5"
//#define APP_TENCENT_SMARTVOICE_ADV_DATA_UUID_LEN    (27)

//500

//pid is 2805, 0x0af5
#define APP_TENCENT_SMARTVOICE_ADV_DATA_UUID         "\x15\xFF\x58\x00\xf5\x0a\x00\x00"
#define APP_TENCENT_SMARTVOICE_ADV_DATA_UUID_LEN     (8)


//#define APP_SCNRSP_DATA         "\x03\x03\x09\x07\x02\x0A\xC5\x0D\xFF\xb0\x02\xCF\x75\x2B\x7D"
//#define APP_SCNRSP_DATA_LEN     (15)

/**
 * Default Scan response data
 * --------------------------------------------------------------------------------------
 * x08                             - Length
 * xFF                             - Vendor specific advertising type
 * \xB0\x02\x45\x51\x5F\x54\x45\x53\x54 - "EQ_TEST"
 * --------------------------------------------------------------------------------------
 */
#define APP_SCNRSP_DATA         "\x03\x03\x09\x07\x02\x0A\xC5"
#define APP_SCNRSP_DATA_LEN     (7)

static uint8_t tencent_flag[8];
void tencent_flag_send(void)
{
    TRACE(0,"tencent_flag_send");
    DUMP8("%02x ", tencent_flag,8);
    ai_cmd_instance_t tencent_flag_cmd;
    tencent_flag_cmd.cmdcode = AI_CMD_TENCENT_SEND_FLAG;
    tencent_flag_cmd.param_length = 8;
    tencent_flag_cmd.param = tencent_flag;
    app_ai_send_cmd_to_peer(&tencent_flag_cmd);
}
void app_tencent_set_flag(uint8_t *flag, uint8_t flag_len)
{
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    if (flag)
    {
        TRACE(0,"app_tencent_set_flag");
        DUMP8("%02x ", flag, flag_len);
        memcpy(nvrecord_env->flag_value, flag, flag_len);
    }
    nv_record_env_set(nvrecord_env);
}

static uint32_t app_tencent_start_advertising(void *param1, uint32_t param2)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T *)param1;

#ifdef BLE_V2
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
    uint8_t *bt_local_addr = p_ibrt_ctrl->local_addr.address;
    memcpy(bt_addr, bt_local_addr, 6);
#endif

    if (app_ai_dont_have_mobile_connect(AI_SPEC_TENCENT))
    {
        TRACE(1,"%s no mobile connect", __func__);
        return 0;
    }

    memcpy(&cmd->advData[cmd->advDataLen],
           APP_TENCENT_SMARTVOICE_ADV_DATA_UUID, APP_TENCENT_SMARTVOICE_ADV_DATA_UUID_LEN);
    cmd->advDataLen += APP_TENCENT_SMARTVOICE_ADV_DATA_UUID_LEN;

    memcpy(&cmd->advData[cmd->advDataLen],
           bt_addr, 6);
    cmd->advDataLen += 6;

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    memcpy(&cmd->advData[cmd->advDataLen],nvrecord_env->flag_value, 8);

        // Update Advertising Data Length
    cmd->advDataLen +=8;

    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
    cmd->scanRspDataLen += APP_SCNRSP_DATA_LEN;

    return 1;
}

uint32_t app_tencent_voice_connected(void *param1, uint32_t param2,uint8_t param3)
{
    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1;
    uint8_t device_id = (uint8_t)param3;
    TRACE(3,"%s,param2 is %d,param3 is %d", __func__,param2,param3);
    uint8_t ai_connect_index = app_ai_connect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_TENCENT, device_id, btaddr->address);
    if(AI_TRANSPORT_SPP == param2){
        ai_data_transport_init(app_ai_spp_send, AI_SPEC_TENCENT, AI_TRANSPORT_SPP);
        ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_TENCENT, AI_TRANSPORT_SPP);
        app_ai_set_mtu(TX_SPP_MTU_SIZE,AI_SPEC_TENCENT);
        app_ai_set_data_trans_size(160,AI_SPEC_TENCENT);
    }
#ifdef __AI_VOICE_BLE_ENABLE__
    else if(AI_TRANSPORT_BLE == param2){
        ai_data_transport_init(app_tencent_send_cmd_via_notification, AI_SPEC_TENCENT, AI_TRANSPORT_BLE);
        ai_cmd_transport_init(app_tencent_send_cmd_via_notification, AI_SPEC_TENCENT, AI_TRANSPORT_BLE);
        //if ble connected, close spp;
        app_ai_spp_disconnlink(AI_SPEC_TENCENT,ai_connect_index);
        app_ai_set_data_trans_size(160,AI_SPEC_TENCENT);
    }
#endif
    AIV_HANDLER_BUNDLE_T handlerBundle = {
        .upstreamDataHandler    = app_ai_voice_default_upstream_data_handler,
         };
    app_ai_voice_register_ai_handler(AIV_USER_PENGUIN,
                                     &handlerBundle);

    app_ai_setup_complete_handle(AI_SPEC_TENCENT);

    return 0;
}

void app_tencent_manual_disconnect(uint8_t deviceID)
{
    app_ai_spp_disconnlink(AI_SPEC_TENCENT, deviceID);
}

uint32_t app_tencent_voice_disconnected(void *param1, uint32_t param2,uint8_t parma3)
{
    TRACE(1,"%s", __func__);

    app_ai_disconnect_handle((AI_TRANS_TYPE_E)param2,AI_SPEC_TENCENT);

    //app_tencent_ota_disconnection_handler();

    return 0;
}

void app_tencent_smartvoice_start_xfer(void)
{
    TRACE(1,"%s", __func__);
    
#if IS_ENABLE_START_DATA_XFER_STEP
    APP_TENCENT_SV_START_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasCrcCheck = false;
    app_tencent_sv_start_data_xfer(&req);
#else
    app_tencent_sv_kickoff_dataxfer();
#endif
}

void app_tencent_smartvoice_stop_xfer(void)
{
#if 0
    app_tencent_sv_update_conn_parameter(SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS,
                                 SV_LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS,
                                 SV_LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS, 0);
#endif

#if IS_ENABLE_START_DATA_XFER_STEP
    TRACE(0,"IS_ENABLE_START_DATA_XFER_STEP is 1");
    APP_TENCENT_SV_STAPP_TENCENT_DATA_XFER_T req;
    memset((uint8_t *)&req, 0, sizeof(req));
    req.isHasWholeCrcCheck = false;
    app_tencent_sv_stAPP_TENCENT_DATA_XFER(&req);
#else
   TRACE(1,"%s", __func__);
    app_tencent_sv_stop_dataxfer();
#endif
}

void app_tencent_sv_data_xfer_started(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus)
{
    TRACE(1,"SV data xfer is started with ret %d", retStatus);
}

void app_tencent_sv_data_xfer_stopped(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus)
{
    app_hfp_enable_audio_link(false);
    TRACE(1,"SV data xfer is stopped with ret %d", retStatus);
}

int  app_tencent_sv_start_voice_stream_via_ble(uint8_t device_id)
{
    TRACE(1,"%s", __func__);

    if (AI_TRANSPORT_BLE == app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx())) {
        if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = TRANSPORT__BLUETOOTH_BLE;
            app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_WAKEUP_APP, (uint8_t *)&req, sizeof(req),device_id);
            return 1;
        } else {   
            TRACE(1,"%s ai_stream has open", __func__);
            return 0;
        }
    } else {   
        TRACE(1,"%s transport_type is error", __func__);
        return 0;
    }
}

int app_tencent_sv_start_voice_stream_via_spp(uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    
    if (AI_TRANSPORT_SPP == app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx())) {
        if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
            START_VOICE_STREAM_REQ_T req;
            req.voiceStreamPathToUse = TRANSPORT__BLUETOOTH_SPP;
            app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_WAKEUP_APP, (uint8_t *)&req, sizeof(req),device_id);
            return 1;
        } else {   
            TRACE(1,"%s ai_stream has open", __func__);
            return 0;
        }
    } else {   
        TRACE(1,"%s transport_type is error", __func__);
        return 0;
    }
}

void app_tencent_sv_start_voice_stream_via_hfp(uint8_t device_id)
{
    if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx())) {
        TRACE(1,"%s", __func__);
        app_hfp_enable_audio_link(true);
        START_VOICE_STREAM_REQ_T req;
        req.voiceStreamPathToUse = APP_TENCENT_PATH_TYPE_HFP;
        app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_WAKEUP_APP, (uint8_t *)&req, sizeof(req),device_id);
    }
}

uint32_t app_tencent_config_mtu(void *param1, uint32_t param2,uint8_t param3)  
{
    TRACE(2,"%s is %d", __func__, param2);
    app_ai_set_mtu(param2,AI_SPEC_TENCENT);

    return 0;
}

uint32_t app_tencent_handle_init(void *param1, uint32_t param2)
{
    TRACE(1,"%s", __func__);

    memset(&tencent_env, 0x00, sizeof(tencent_env));

    app_tencent_sv_cmd_handler_init();
    app_tencent_sv_data_reset_env();
    app_tencent_sv_throughput_test_init();
    //app_tencent_ota_handler_init();
    
    return 0;
}

void app_tencent_start_voice_stream(void *voice_stream_req, uint32_t req_len, uint8_t dest_id)
{
    TRACE(2,"%s ai_stream_opened %d", __func__, app_ai_is_stream_opened(AI_SPEC_TENCENT));
    
    if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
        //app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, BT_DEVICE_ID_1);
        app_ai_set_stream_opened(true,AI_SPEC_TENCENT);
        if (!app_ai_is_use_thirdparty(AI_SPEC_TENCENT))
        {
            app_ai_voice_upstream_control(true, AIV_USER_PENGUIN, UPSTREAM_INVALID_READ_OFFSET);
            /// update the read offset for AI voice module
            app_ai_voice_update_handle_frame_len(TENCENT_VOICE_ONESHOT_PROCESS_LEN);
     
        }
        app_ai_voice_stream_init(AIV_USER_PENGUIN);
        app_ai_spp_deinit_tx_buf();
    }
}

uint32_t app_tencent_stop_voice_stream(void *param1, uint32_t param2,uint8_t param3)
{
    if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx())) {
        if (app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
            app_ai_voice_upstream_control(false, AIV_USER_PENGUIN, 0);
            app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_STOP_VOICE_STREAM, NULL, 0,param3);
        }
    }
    return 0;
}


//wakeup APP，send 101 command to APP
uint32_t app_tencent_wake_up(void *param1, uint32_t param2, uint8_t param3)
{
    int ret =0;
    
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_TENCENT, ai_manager_get_foreground_ai_conidx());
    TRACE(2,"%s,transport_type is %d", __func__,transport_type);

    if (!app_ai_is_setup_complete(AI_SPEC_TENCENT) ||
       (transport_type == AI_TRANSPORT_IDLE)){
       TRACE(1,"%s ai haven't setup", __func__);
       return 1;
    }

    if (app_ai_is_sco_mode()){
        TRACE(1,"%s is in sco mode", __func__);
        return 2;
    }

    //if ota upgrade, can not wakeup xiaowei;
    if(0/*app_tencent_get_ota_status()*/)
    {
       TRACE(0,"now it is otaing..., please wait...");
        return 3;
    }

    if(!is_ai_can_wake_up(AI_SPEC_TENCENT))
    {
        TRACE(1,"%s off", __func__);
        app_tencent_stop_voice_stream(NULL, 0,param3);
        return 4;
    }
   else
    {
        app_ai_spp_deinit_tx_buf();

        if(AI_TRANSPORT_BLE == transport_type)     //use ble
        {
          ret =app_tencent_sv_start_voice_stream_via_ble(param3);    //send start cmd;
        }
        else if(AI_TRANSPORT_SPP == transport_type)    //use spp
        { 
           ret = app_tencent_sv_start_voice_stream_via_spp(param3);  //send start cmd;
        }else
        {
           TRACE(0,"it is error, not ble or spp connected");
        }
    
    }

    if(ret ==1)
    {
        ai_mailbox_put(SIGN_AI_WAKEUP, 0,AI_SPEC_TENCENT,param3);
        ai_set_can_wake_up(false,AI_SPEC_TENCENT);
        app_tencent_start_voice_stream(param1, param2, param3);    //open stream
    }
    else
    {
        TRACE(1,"%s error", __func__);
    }

    return 0;
}

uint32_t app_tencent_enable_handler(void *param1, uint32_t param2)
{
    TRACE(2,"%s %d", __func__, param2);
    return 0;
}

uint32_t app_tencent_key_event_handle(void *param1, uint32_t param2,uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

    TRACE(4,"%s code 0x%x event %d,param3 is %d", __func__, status->code, status->event,param3);

    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return 1;
    }

    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_TENCENT))
    {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event)
        {
            app_tencent_wake_up(status, 0, param3);
        }
        else if (APP_KEY_EVENT_UP == status->event)
        {
            tencent_loose_button_to_request_for_stop_record_voice_stream(status, 0,param3);
        }
        else if(APP_KEY_EVENT_DOUBLECLICK == status->event)
        {
            //TRACE(1,"%s stop replying", __func__);
            //ai_function_handle(CALLBACK_STOP_REPLYING, status, 0);
        }
    }
    else
    {
        if (APP_KEY_EVENT_CLICK == status->event)
        {
            app_tencent_wake_up(status, 0, param3);
        }
    }

    return 0;
}

#define TENCENT_TRANSFER_DATA_MAX_LEN 400
uint32_t app_tencent_stream_init(void *param1, uint32_t param2)
{
    return 0;
}

uint32_t app_tencent_stream_deinit(void *param1, uint32_t param2)
{
    return 0;
}

void tencent_double_click_to_stop_app_playing(uint8_t device_id)                      //double click, to stop playing
{
   TRACE(0,"tencent_double_click_to_stop_app_playing");
   if(AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx()))
    {
           app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_STOP_APP_PLAYING,NULL,0,device_id);
    }
    else
    { 
       TRACE(0,"not spp or ble connect");
    }
}

uint32_t tencent_loose_button_to_request_for_stop_record_voice_stream(void *param1, uint32_t param2,uint8_t param3)           //loose the button to send stop record to xiaoweiAPP        
{
    TRACE(0,"loose the button");
    if(AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx()))
    {
        app_ai_voice_upstream_control(false, AIV_USER_PENGUIN, 0);
        app_ai_voice_stream_control(false, AIV_USER_PENGUIN);
        app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_REQUEST_FOR_STOP,NULL,0,param3);
    }
    else
    {
        TRACE(0,"not spp or ble connect");
    }

    return 0;
}

static void app_tencent_sv_tencent_smartvoice_wakeup_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    // if APP send 101 command，do this;
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;

    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx());
    TRACE(2,"%s Voice Path type %d", __func__, voicePathType);
    START_VOICE_STREAM_RSP_T rsp;
    rsp.voiceStreamPathToUse = voicePathType;

    if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
        if ((AI_TRANSPORT_BLE == transport_type) ||
            (AI_TRANSPORT_SPP == transport_type)) {
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, (uint8_t *)&rsp, sizeof(rsp),device_id);
            app_tencent_start_voice_stream(ptrParam, paramLen, device_id);
            // start the data xfer
            app_tencent_smartvoice_start_xfer();
        } else if ((APP_TENCENT_PATH_TYPE_HFP == voicePathType) &&
                   !app_ai_is_sco_mode()) {
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_COMMAND_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp),device_id);
        } else {
            app_ai_set_stream_opened(true,AI_SPEC_TENCENT);
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, (uint8_t *)&rsp, sizeof(rsp),device_id);
        }
    } else {
        app_tencent_sv_send_response_to_command(funcCode, TENCENT_DATA_TRANSFER_HAS_ALREADY_BEEN_STARTED, (uint8_t *)&rsp, sizeof(rsp),device_id);
    }

}

static void app_tencent_sv_tencent_smartvoice_wakeup_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //if earphone touch to wake up, do this
 
   TRACE(2,"%s retStatus %d", __func__, retStatus);
 
    if (TENCENT_NO_ERROR == retStatus) {
        START_VOICE_STREAM_RSP_T* rsp = (START_VOICE_STREAM_RSP_T *)ptrParam;
        TRACE(2,"%s PathToUse %d", __func__, rsp->voiceStreamPathToUse);

        if (AI_TRANSPORT_IDLE != app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx())) {
            if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
                app_tencent_start_voice_stream(ptrParam, paramLen, device_id);
                // start the data xfer
                app_tencent_smartvoice_start_xfer();
            } else {
                //app_tencent_voice_report(APP_TENCENT_STATUS_INDICATION_ALEXA_START,0);//play media
                TRACE(0,"mic is open");
                app_tencent_smartvoice_start_xfer();
            }
            ai_audio_stream_allowed_to_send_set(true,AI_SPEC_TENCENT);
        }
        } else {
        // workaround, should be changed to false
        TRACE(0,"app_hfp_enable_audio_link");
        app_hfp_enable_audio_link(false);
    }
}

void app_tencent_sv_start_voice_stream_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //APP notify the device to record；
    uint8_t voicePathType = ((START_VOICE_STREAM_REQ_T *)ptrParam)->voiceStreamPathToUse;
    
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx());
    TRACE(4,"%s ai_stream_opened %d funcCode %d voicePathType %d", 
        __func__, app_ai_is_stream_opened(AI_SPEC_TENCENT), funcCode, voicePathType);
    
    START_VOICE_STREAM_RSP_T rsp;
    //rsp.voiceStreamPathToUse = voicePathType;
    rsp.voiceStreamPathToUse = voicePathType;

    if (!app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
        if ((AI_TRANSPORT_BLE == transport_type) ||
            (AI_TRANSPORT_SPP == transport_type)) {
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, (uint8_t *)&rsp, sizeof(rsp),device_id);
            app_tencent_start_voice_stream(ptrParam, paramLen, device_id);
            // start the data xfer
            app_tencent_smartvoice_start_xfer();
        } else if ((APP_TENCENT_PATH_TYPE_HFP == voicePathType) &&
                   !app_ai_is_sco_mode()) {
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_COMMAND_HANDLING_FAILED, (uint8_t *)&rsp, sizeof(rsp),device_id);
        } else {
            //ai_struct.ai_stream.ai_stream_opened = true;
            app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, (uint8_t *)&rsp, sizeof(rsp),device_id);
        }
    } else {
        app_tencent_sv_send_response_to_command(funcCode, TENCENT_DATA_TRANSFER_HAS_ALREADY_BEEN_STARTED, (uint8_t *)&rsp, sizeof(rsp),device_id);
    }
}

#if 0
static void app_tencent_sv_start_voice_stream_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    //not do it;
    TRACE(1,"Received the response %d to the start voice stream request.", retStatus);
    if (TENCENT_NO_ERROR == retStatus) {
        START_VOICE_STREAM_RSP_T* rsp = (START_VOICE_STREAM_RSP_T *)ptrParam;
        if ((AI_TRANSPORT_BLE == ai_struct.transport_type) ||
            (AI_TRANSPORT_SPP == ai_struct.transport_type)) {
            if (!ai_struct.ai_stream.ai_stream_opened) {
                app_tencent_start_voice_stream();
                // start the data xfer
                app_tencent_smartvoice_start_xfer();
            } else {
                //app_tencent_voice_report(APP_TENCENT_STATUS_INDICATION_ALEXA_START,0);//play media
                app_tencent_smartvoice_start_xfer();
            }
        }
    } else {
        // workaround, should be changed to false
        app_hfp_enable_audio_link(false);
    }

    ai_struct.ai_stream.ai_stream_opened = true;
}
#endif

static void app_tencent_sv_stop_voice_stream_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    TRACE(1,"%s", __func__);   //xiaowei APP send stop record command;
    if (app_ai_is_stream_opened(AI_SPEC_TENCENT)) {
        app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);

        app_ai_voice_upstream_control(false, AIV_USER_PENGUIN, 0);
        // Stop the data xfer
        app_tencent_smartvoice_stop_xfer();
    } else {
        TRACE(0,"error in app_tencent_sv_stop_voice_stream_handler");
        app_tencent_sv_send_response_to_command(funcCode, TENCENT_DATA_TRANSFER_HAS_NOT_BEEN_STARTED, NULL, 0,device_id);
    }
}


static void app_tencent_sv_stop_voice_stream_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //the device stop to record；
    TRACE(2,"%s %d from the remote dev.", __func__, retStatus);
    if ((TENCENT_NO_ERROR == retStatus) || (TENCENT_WAITING_RESPONSE_TIMEOUT == retStatus)) {
        app_tencent_smartvoice_stop_xfer();
    }

    app_ai_voice_upstream_control(false, AIV_USER_PENGUIN, 0);
}

static void app_tencent_sv_stop_app_playing_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //app send stop playing
    
    TRACE(0,"receive xiaowei stop record command");
    /*if(ai_struct.ai_stream.ai_stream_opened)
    {
       app_tencent_sv_send_response_to_command(funcCode, NO_ERROR, NULL, 0);

        app_ai_voice_stop_mic_stream();
        ai_audio_stream_allowed_to_send_set(false);
        // Stop the data xfer
        app_tencent_smartvoice_stop_xfer();
        
    }
    else
    {
        app_tencent_sv_send_response_to_command(funcCode, TENCENT_DATA_TRANSFER_HAS_NOT_BEEN_STARTED, NULL, 0);
    }*/
}
static void app_tencent_sv_stop_app_playing_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{   
    //the device double click to stop playing
     TRACE(0,"stop record rsp");
     if(TENCENT_NO_ERROR == retStatus)
     {
         TRACE(0,"stop palying success");
     }
    /*if ((TENCENT_NO_ERROR == retStatus) || (TENCENT_WAITING_RESPONSE_TIMEOUT == retStatus)) {
        app_ai_voice_stop_mic_stream();
        ai_audio_stream_allowed_to_send_set(false);
        app_tencent_smartvoice_stop_xfer();
    }
    ai_struct.ai_stream.ai_stream_opened = false;
    */
}

static void app_tencent_sv_request_for_stop_hanlder(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{

    TRACE(0,"it is 108 command, so still record");
    if(app_ai_is_stream_opened(AI_SPEC_TENCENT))
    {
       app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);
    }
}

static void app_tencent_sv_request_for_stop_rsp_hanlder(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //when loose the button，the device send request_for_stop command;
        TRACE(0,"stop record rsp");
     if(TENCENT_NO_ERROR == retStatus)
     {
         TRACE(0,"108 rsp, do nothing");
     }
}


static void app_tencent_sv_phoneid_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{   
   //app send phoneid to device
 
  if(ptrParam == NULL) TRACE(0,"phone id, it is null");
  else
    { 
       TRACE(0,"phone id:");
       DUMP8("0x%02x", ptrParam,paramLen);
    }
    uint8_t buf[255];  //the buffer store MAC and phone id；
    uint8_t len=0;
    uint8_t pad_value=0;
    uint8_t buf_str[255];    //the buffer stoer string mac

#if defined(TENCENT_SMARTVOICE_650)
    char key[] = "CxzKnfzUnYUUTR2w";
    char iv[] = "2100000436000000";

#elif defined(TENCENT_SMARTVOICE_400)
    char key[] = "USOlifUggfuhpoBu";
    char iv[] = "2100000432000000";

#else
    char key[] = "YdndTRb9VhatS0rY";
    char iv[] = "21000004340000000";
#endif

    memset(buf,0,255); 
    memset(buf_str,0,255);
    memcpy(buf + len, bt_addr, 6);    
    len += 6;

    for(int i=0;i<len;i++)
    {
        sprintf((char*)buf_str+2*i, "%02x",buf[i]);    
    }

    int len_str=strlen((char *)buf_str);
    TRACE(1,"len_str is %d", len_str);
    
    memcpy(buf_str+len_str,ptrParam, paramLen);  //phoneid store in buf_str
    len_str +=paramLen;              //len_str is the lens of phoneid +mac
    //TRACE(0,"bt address + phone id:");
    //DUMP8("0x%02x", buf_str,len_str);      //buf_str store the string of mac address +phone id；

    int len_last=0;
    if(len_str%16 ==0)   len_last =(1+len_str/16) *16;
    else 
        len_last =(1+len_str/16) *16;
        
       pad_value = len_last-len_str;
       memset(buf_str+len_str, pad_value, pad_value); //use PKCS7 to fill buff
       TRACE(1,"len_last is %d",len_last);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (const uint8_t*)key, (const uint8_t*)iv);   //init ASE
    AES_CBC_encrypt_buffer(&ctx, buf_str, len_last);    //encrypt the buf_str
    //DUMP8("0x%02x", buf_str,len_last);
        
    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR,buf_str,len_last,device_id);
}

static void app_tencent_sv_tencent_get_pid_mac_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    //xiaowei get pid+bt mac address,pid isproduct id，len is4
    TRACE(0,"get pid mac handler");

    if(ptrParam == NULL)   
    {
        TRACE(0,"ptrParam is NULL");
    }
    else
    {
        TRACE(0,"ptrParam is not NULL");
    }

	uint16_t len = 0;
	uint8_t buff[19];
	memset(buff,0,19);
  
    memcpy(buff + len, APP_TENCENT_PID_DATA, APP_TENCENT_PID_DATA_LEN);
    len += APP_TENCENT_PID_DATA_LEN;
    memcpy(buff + len, bt_addr, 6);
    len += 6;
	struct nvrecord_env_t *nvrecord_env;
	nv_record_env_get(&nvrecord_env);

	memcpy(buff + len, nvrecord_env->flag_value, 8);
	len += 8;

	buff[len] = 1;//is_connected
   
    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, buff, len,device_id);
}

static uint8_t ble_flag_value[8];
static void app_tencent_sv_inform_algorithm_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
   // xiaowei send command to get config information
    
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx());
    tencent_config_t *config = (tencent_config_t*)ptrParam;
    memcpy(ble_flag_value,config->flag_value,8);
    TRACE(1,"transport_type is %d, ptrParam is:",transport_type);
    DUMP8("0x%02x",ptrParam,paramLen);

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    memcpy(nvrecord_env->flag_value,config->flag_value,8);
    nv_record_env_set(nvrecord_env);
   //nv_record_flash_flush();

    TRACE(0,"flag value is");
    DUMP8("0x%02x ", nvrecord_env->flag_value, 8);
    memcpy(tencent_flag,config->flag_value,8);   //set tencent flag;

    tencent_flag_send();         //send tencent flag

    EARPHONE_CONFIG_INFO_T tInfo;
    int len=0;
    
    char *p =(char *) & (tInfo.firmware_version);
          p[0] = 0;
          p[1] = BT_FIRMWARE_VERSION[0]-'0';
          p[2] = BT_FIRMWARE_VERSION[2]-'0';
          p[3] = BT_FIRMWARE_VERSION[4]-'0';
    
    if(AI_TRANSPORT_BLE == transport_type)
    {
       tInfo.transport_mode = TRANSPORT__BLUETOOTH_BLE;
    }
    else if(AI_TRANSPORT_SPP == transport_type)
    {
        tInfo.transport_mode = TRANSPORT__BLUETOOTH_SPP;
    }
    else
    {
        tInfo.transport_mode = TRANSPORT__BLUETOOTH_IAP;
    }
    
    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_TENCENT)) {
        tInfo.record_method = SPEECH_INITIATOR__TYPE__PRESS_AND_HOLD;
    } else {
        tInfo.record_method = SPEECH_INITIATOR__TYPE__TAP;
    }

    if(app_ai_get_encode_type(AI_SPEC_TENCENT) == VOC_ENCODE_OPUS) {
        tInfo.encode_format = AUDIO_FORMAT__OPUS;
    }
#ifdef VOC_ENCODE_SBC
    else if(app_ai_get_encode_type(AI_SPEC_TENCENT) == VOC_ENCODE_SBC) {
        tInfo.encode_format = AUDIO_FORMAT__SBC;
    }
#endif

    tInfo.compression_ration = Audio_Comparess_Ration_16;
    tInfo.sample_rate = Audio_Sample_Rate_16_K;
    tInfo.mic_channel_count = Audio_Mic_Channel_Count_1;
    tInfo.bit_number = Audio_Bit_Number_16;
    
    strncpy((char*)tInfo.param, TENCENT_DEVICE_NAME, TENCENT_DEVICE_NAME_NUM+1);
    tInfo.param_length = TENCENT_DEVICE_NAME_NUM;

    TRACE(1,"name length is %d",tInfo.param_length);

    len=8*sizeof(uint8_t)+sizeof(uint32_t) +tInfo.param_length;
	tInfo.protocol_version = 0x02;
	len =  len+2;

	tInfo.extra_data.key = xw_headphone_extra_config_key_keyFunction;
	tInfo.extra_data.value = xw_headphone_extra_config_value_keyFunction_default;
	tInfo.extra_data_length = sizeof(xw_headphone_extra_config_item);
	len =  len+tInfo.extra_data_length+sizeof(uint8_t);
    TRACE(1,"len is %d", len);
    TRACE(0,"tInfo is:");
    DUMP8("0x%02x ", (uint8_t *)&tInfo, len);

    TRACE(0,"Inform the information of the used algorithm.");  
    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, (uint8_t *)&tInfo,len,device_id);
    
}

void app_tencent_sv_clean_flag_value_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    
    TRACE(0,"clean up the flag value");
    uint32_t lk = nv_record_pre_write_operation();
    memset(nvrecord_env->flag_value,0,8);
    nv_record_post_write_operation(lk);
    nv_record_env_set(nvrecord_env);

    TRACE(0,"flag value is：");
    DUMP8("0x%02x ", nvrecord_env->flag_value, 8);
        

    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);
}

#define TENCENT_TTS_END (1<<0)
#define TENCENT_AVRCP_PAUSE (1<<1)
#define TENCENT_AVRCP_RESUME ((1<<2)|(1<<0))
uint8_t switch_media_status = FALSE;
extern "C" uint8_t  avrcp_get_media_status(void);

void app_tencent_sv_play_control_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{

    if ((*ptrParam)&TENCENT_AVRCP_PAUSE)
    {
        //pause
        TRACE(0,"command  COMMAND_A2DP_PAUSE");
        if ((BTIF_AVRCP_MEDIA_PLAYING == avrcp_get_media_status())
            && (FALSE == switch_media_status))
        {
            switch_media_status = TRUE;
            a2dp_handleKey(AVRCP_KEY_PAUSE);
        }

    }
    else if((*ptrParam)&TENCENT_AVRCP_RESUME)
    {
        if((BTIF_AVRCP_MEDIA_PLAYING != avrcp_get_media_status())
            && (TRUE == switch_media_status))
        {
            //resume
            switch_media_status = FALSE;
            a2dp_handleKey(AVRCP_KEY_PLAY);
        }
        TRACE(0,"command  COMMAND_A2DP_PLAY");
    }
    else
    {
       TRACE(0,"value is error,do nothing");
    }

    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);
}

void app_tencent_sv_heartbeat_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);
}

void app_tencent_sv_general_config_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    TRACE(1,"%s",__func__);
    DUMP8("x%02x",ptrParam, paramLen);
    xw_headphone_general_config_item    *config_info= (xw_headphone_general_config_item *)ptrParam;

    uint8_t key_group = (config_info->key & 0xFF00) >> 8;
    uint8_t key_index = config_info->key & 0xFF;

    TRACE(2,"key_group is %d,key_index is %d",key_group, key_index);
    switch (key_group)
    {
        case xw_headphone_general_config_key_nc_group:
            if (key_index == xw_headphone_general_config_key_nc_report)
            {
                TRACE(0,"nc report");
            }
            if(key_index == xw_headphone_general_config_key_nc_increase)
            {
                TRACE(0,"nc increase");
            }
            if(key_index == xw_headphone_general_config_key_nc_decrease)
            {
                TRACE(0,"nc decrease");
            }
            if(key_index == xw_headphone_general_config_key_nc_set)
            {
                //int8_t data=config_info->payload[0];
                TRACE(0,"nc set");
            }
            if(key_index == xw_headphone_general_config_key_nc_on)
            {
                TRACE(0,"nc on");
            }
            if(key_index == xw_headphone_general_config_key_nc_off)
            {
                TRACE(0,"nc off");
            }
            if(key_index == xw_headphone_general_config_key_nc_max)
            {
                TRACE(0,"nc max");
            }
            if(key_index == xw_headphone_general_config_key_nc_min)
            {
                TRACE(0,"nc min");
            }
            if(key_index == xw_headphone_general_config_key_conversation_mode_on)
            {
                TRACE(0,"mode on");
            }
            if(key_index == xw_headphone_general_config_key_conversation_mode_off)
            {
                TRACE(0,"mode off");
            }
   
            break;
     case xw_headphone_general_config_key_group_eq:
         if (key_index == xw_headphone_general_config_key_eq_inc_treble)
         {
             TRACE(0,"inc treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_inc_mid)
         {
             TRACE(0,"inc mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_inc_bass)
         {
             TRACE(0,"inc bass");
         }
         if(key_index == xw_headphone_general_config_key_eq_dec_treble)
         {
             
             TRACE(0,"dec treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_dec_mid)
         {
             TRACE(0,"dec mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_dec_bass)
         {
             TRACE(0,"dec bass");
         }
         if(key_index == xw_headphone_general_config_key_eq_max_treble)
         {
             TRACE(0,"max treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_max_mid)
         {
             TRACE(0,"max mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_max_bass)
         {
             TRACE(0,"max bass");
         }
         if(key_index == xw_headphone_general_config_key_eq_min_treble)
         {
             TRACE(0,"min treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_min_mid)
         {
             TRACE(0,"min mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_min_bass)
         {
             TRACE(0,"min bass");
         }
         if(key_index == xw_headphone_general_config_key_eq_reset_treble)
         {
             TRACE(0,"reset treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_reset_mid)
         {
             TRACE(0,"reset mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_reset_bass)
         {
             TRACE(0,"mode on");
         }
         if(key_index == xw_headphone_general_config_key_eq_set_treble)
         {
             TRACE(0,"set treble");
         }
         if(key_index == xw_headphone_general_config_key_eq_set_mid)
         {
             TRACE(0,"set mid");
         }
         if(key_index == xw_headphone_general_config_key_eq_set_bass)
         {
             TRACE(0,"set bass");
         }
         if(key_index == xw_headphone_general_config_key_eq_set_eq)
         {
             TRACE(0,"set eq");
           //  uint8_t len = config_info->length;
             
         }
         break;
     default :
         break;
     }

    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0,device_id);
 
}

void app_tencent_sv_custom_skills_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    
}

void app_tencent_sv_req_disconnect_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    app_tencent_sv_send_response_to_command(funcCode, TENCENT_NO_ERROR, NULL, 0, device_id);
}


CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_WAKEUP_APP, app_tencent_sv_tencent_smartvoice_wakeup_handler, TRUE,
                      APP_TENCENT_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS,  app_tencent_sv_tencent_smartvoice_wakeup_rsp_handler );


CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_STOP_VOICE_STREAM, app_tencent_sv_stop_voice_stream_handler, TRUE,
                    APP_TENCENT_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS, app_tencent_sv_stop_voice_stream_rsp_handler);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_GET_PID_MAC, app_tencent_sv_tencent_get_pid_mac_handler, FALSE,
                     0,NULL );

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_START_VOICE_STREAM, app_tencent_sv_start_voice_stream_handler,TRUE,
                  0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_INFORM_ALGORITHM_TYPE, app_tencent_sv_inform_algorithm_handler, FALSE,
                       0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_PHONEID, app_tencent_sv_phoneid_handler, FALSE,
                      0, NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_STOP_APP_PLAYING,app_tencent_sv_stop_app_playing_handler, TRUE,
                     APP_TENCENT_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS, app_tencent_sv_stop_app_playing_rsp_handler);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_REQUEST_FOR_STOP, app_tencent_sv_request_for_stop_hanlder, TRUE,
                     APP_TENCENT_SMARTVOICE_WAITING_REMOTE_DEV_RESPONSE_TIMEOUT_IN_MS, app_tencent_sv_request_for_stop_rsp_hanlder);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_CLEAN_FLAG_VALUE,app_tencent_sv_clean_flag_value_handler,false,0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_PLAY_CONTROL,app_tencent_sv_play_control_handler,false,0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_HEARTBEAT,app_tencent_sv_heartbeat_handler,false,0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_GENERAL_CONFIG,app_tencent_sv_general_config_handler,false,0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_CUSTOM_SKILLS,app_tencent_sv_custom_skills_handler,false,0,NULL);

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_SMARTVOICE_REQUEST_DISCONNECT,app_tencent_sv_req_disconnect_handler,false,0,NULL);

typedef enum {
    PATTERN_RANDOM = 0,
    PATTERN_11110000,
    PATTERN_10101010,
    PATTERN_ALL_1,
    PATTERN_ALL_0,
    PATTERN_00001111,
    PATTERN_0101,
} THROUGHPUT_TEST_DATA_PATTER_E;

typedef enum {
    UP_STREAM = 0,
    DOWN_STREAM
} THROUGHPUT_TEST_DATA_DIRECTION_E;

typedef enum {
    WITHOUT_RSP = 0,
    WITH_RSP
} THROUGHPUT_TEST_RESPONSE_MODEL_E;


typedef struct {
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

#define APP_TENCENT_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS    2000
static void app_tencent_sv_throughput_pre_config_pending_timer_cb(void const *n);
osTimerDef (APP_TENCENT_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER, app_tencent_sv_throughput_pre_config_pending_timer_cb);
osTimerId   app_tencent_sv_throughput_pre_config_pending_timer_id = NULL;

static void app_tencent_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n);
osTimerDef (APP_TENCENT_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER, app_tencent_sv_throughput_test_data_xfer_lasting_timer_cb);
osTimerId   app_tencent_sv_throughput_test_data_xfer_lasting_timer_id = NULL;


static uint8_t app_tencent_sv_throughput_datapattern[MAXIMUM_DATA_PACKET_LEN];

static void app_tencent_sv_throughput_test_data_xfer_lasting_timer_cb(void const *n)
{
    app_tencent_sv_send_command(APP_TENCENT_THROUGHPUT_TEST_DONE, NULL, 0,0);

    app_tencent_sv_stop_throughput_test();
}

static void app_tencent_sv_throughput_pre_config_pending_timer_cb(void const *n)
{
    // inform the configuration
    app_tencent_sv_send_command(APP_TENCENT_INFORM_THROUGHPUT_TEST_CONFIG,
                        (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig),0);

    if (UP_STREAM == throughputTestConfig.direction) {
        app_tencent_sv_throughput_test_transmission_handler(NULL, 0,0);
        osTimerStart(app_tencent_sv_throughput_test_data_xfer_lasting_timer_id,
                     throughputTestConfig.lastTimeInSecond * 1000);
    }
}

void app_tencent_sv_throughput_test_init(void)
{
    app_tencent_sv_throughput_pre_config_pending_timer_id =
        osTimerCreate(osTimer(APP_TENCENT_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIMER),
                      osTimerOnce, NULL);

    app_tencent_sv_throughput_test_data_xfer_lasting_timer_id =
        osTimerCreate(osTimer(APP_TENCENT_SV_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER),
                      osTimerOnce, NULL);
}

void app_tencent_sv_set_throughput_test_conn_parameter(uint8_t conidx, uint16_t minConnIntervalInMs, uint16_t maxConnIntervalInMs)
{
    throughputTestConfig.minConnIntervalInMs = minConnIntervalInMs;
    throughputTestConfig.maxConnIntervalInMs = maxConnIntervalInMs;
}

uint32_t app_tencent_sv_throughput_test_transmission_handler(void *param1, uint32_t param2,uint8_t device_id)
{
    if (UP_STREAM == throughputTestConfig.direction) {
        app_tencent_sv_send_command(APP_TENCENT_THROUGHPUT_TEST_DATA,
                            app_tencent_sv_throughput_datapattern, throughputTestConfig.dataPacketSize - 4,device_id);
    }

    return 0;
}

static void app_tencent_sv_throughput_test_data_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    
    if ((WITH_RSP == throughputTestConfig.responseModel) &&
        (AI_TRANSPORT_SPP == app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx()))) {
        app_tencent_sv_send_command(APP_TENCENT_THROUGHPUT_TEST_DATA_ACK,
                            NULL, 0,device_id);
    }
}

static void app_tencent_sv_throughput_test_data_ack_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    if (tencent_env.isThroughputTestOn &&
        (WITH_RSP == throughputTestConfig.responseModel)) {
        app_tencent_sv_throughput_test_transmission_handler(NULL, 0,device_id);
    }
}

static void app_tencent_sv_throughput_test_done_signal_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    app_tencent_sv_stop_throughput_test();
}

void app_tencent_sv_stop_throughput_test(void)
{
    tencent_env.isThroughputTestOn = false;
    osTimerStop(app_tencent_sv_throughput_pre_config_pending_timer_id);
    osTimerStop(app_tencent_sv_throughput_test_data_xfer_lasting_timer_id);
}

static void app_tencent_sv_throughput_test_config_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen,uint8_t device_id)
{
    throughputTestConfig = *(THROUGHPUT_TEST_CONFIG_T *)ptrParam;

    TRACE(1,"data patter is %d", throughputTestConfig.dataPattern);

    // generate the data pattern
    switch (throughputTestConfig.dataPattern) {
        case PATTERN_RANDOM: {
            for (uint32_t index = 0; index < MAXIMUM_DATA_PACKET_LEN; index++) {
                app_tencent_sv_throughput_datapattern[index] = (uint8_t)rand();
            }
            break;
        }
        case PATTERN_11110000: {
            memset(app_tencent_sv_throughput_datapattern, 0xF0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_10101010: {
            memset(app_tencent_sv_throughput_datapattern, 0xAA, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_1: {
            memset(app_tencent_sv_throughput_datapattern, 0xFF, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_ALL_0: {
            memset(app_tencent_sv_throughput_datapattern, 0, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_00001111: {
            memset(app_tencent_sv_throughput_datapattern, 0x0F, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        case PATTERN_0101: {
            memset(app_tencent_sv_throughput_datapattern, 0x55, MAXIMUM_DATA_PACKET_LEN);
            break;
        }
        default:
            throughputTestConfig.dataPattern = 0;
            break;
    }

    tencent_env.isThroughputTestOn = true;

    // check whether need to use the new conn parameter
   
    if ((AI_TRANSPORT_BLE == app_ai_get_transport_type(AI_SPEC_TENCENT,ai_manager_get_foreground_ai_conidx()))
        && throughputTestConfig.isToUseSpecificConnParameter) {
        osTimerStart(app_tencent_sv_throughput_pre_config_pending_timer_id,
                     APP_TENCENT_SV_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS);
    } else {
        app_tencent_sv_send_command(APP_TENCENT_INFORM_THROUGHPUT_TEST_CONFIG,
                            (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig),device_id);

        if (UP_STREAM == throughputTestConfig.direction) {
            app_tencent_sv_throughput_test_transmission_handler(NULL, 0,device_id);
            osTimerStart(app_tencent_sv_throughput_test_data_xfer_lasting_timer_id,
                         throughputTestConfig.lastTimeInSecond * 1000);
        }
    }
}

void app_tencent_sv_force_disconnect_tencent_app(void)
{
	START_VOICE_STREAM_REQ_T req;
    uint8_t device_id = 0;

	req.voiceStreamPathToUse = 0;
	app_tencent_sv_send_command(APP_TENCENT_SMARTVOICE_REQUEST_DISCONNECT, (uint8_t *)&req, sizeof(req), device_id);

	if(AI_TRANSPORT_SPP == app_ai_get_transport_type(AI_SPEC_TENCENT, device_id))
	{
		app_ai_spp_disconnlink(AI_SPEC_TENCENT, device_id);
	}
}

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_INFORM_THROUGHPUT_TEST_CONFIG, app_tencent_sv_throughput_test_config_handler, FALSE,
                      0,    NULL );

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_THROUGHPUT_TEST_DATA, app_tencent_sv_throughput_test_data_handler, FALSE,
                      0,    NULL );

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_THROUGHPUT_TEST_DATA_ACK, app_tencent_sv_throughput_test_data_ack_handler, FALSE,
                      0,    NULL );

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_THROUGHPUT_TEST_DONE, app_tencent_sv_throughput_test_done_signal_handler, FALSE,
                      0,    NULL );

uint32_t app_tencent_voice_send_done(void *param1, uint32_t param2, uint8_t param3)
{
    //TRACE(2,"%s credit %d", __func__, ai_struct.tx_credit);
    if(ai_struct[AI_SPEC_TENCENT].tx_credit < MAX_TX_CREDIT) {
        ai_struct[AI_SPEC_TENCENT].tx_credit++;
    }

    if ((app_ai_get_data_trans_size(AI_SPEC_TENCENT) <= voice_compression_get_encode_buf_size()) ||
        ai_transport_has_cmd_to_send())
    {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_TENCENT),AI_SPEC_TENCENT,param3);
    }

    if (tencent_env.isThroughputTestOn &&
    (WITHOUT_RSP == throughputTestConfig.responseModel)) {
        app_tencent_sv_throughput_test_transmission_handler(NULL, 0,param3);
    }
    return 0;
}


/// TENCENT handlers function definition
const struct ai_func_handler tencent_handler_function_list[] =
{
    {API_HANDLE_INIT,           (ai_handler_func_t)app_tencent_handle_init},
    {API_START_ADV,             (ai_handler_func_t)app_tencent_start_advertising},
    {API_DATA_HANDLE,           (ai_handler_func_t)app_tencent_audio_stream_handle},
    {API_DATA_SEND,             (ai_handler_func_t)app_tencent_send_voice_stream},
    {API_DATA_INIT,             (ai_handler_func_t)app_tencent_stream_init},
    {API_DATA_DEINIT,           (ai_handler_func_t)app_tencent_stream_deinit},
    {API_AI_EXC_FLAG,           (ai_handler_func_t)app_tencent_set_flag},
    {CALLBACK_CMD_RECEIVE,      (ai_handler_func_t)app_tencent_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,     (ai_handler_func_t)app_tencent_rcv_data},
    {CALLBACK_DATA_PARSE,       (ai_handler_func_t)app_tencent_parse_cmd},
    {CALLBACK_AI_CONNECT,       (ai_handler_func_t)app_tencent_voice_connected},
    {CALLBACK_AI_DISCONNECT,    (ai_handler_func_t)app_tencent_voice_disconnected},
    {CALLBACK_UPDATE_MTU,       (ai_handler_func_t)app_tencent_config_mtu},
    {CALLBACK_WAKE_UP,          (ai_handler_func_t)app_tencent_wake_up},
    {CALLBACK_AI_ENABLE,        (ai_handler_func_t)app_tencent_enable_handler},
    {CALLBACK_START_SPEECH,     (ai_handler_func_t)NULL},
    {CALLBACK_ENDPOINT_SPEECH,  (ai_handler_func_t)tencent_loose_button_to_request_for_stop_record_voice_stream},
    {CALLBACK_STOP_SPEECH,      (ai_handler_func_t)app_tencent_stop_voice_stream},
    {CALLBACK_DATA_SEND_DONE,   (ai_handler_func_t)app_tencent_voice_send_done},
    {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)app_tencent_key_event_handle},
    {CALLBACK_DISCONNECT_APP,	(ai_handler_func_t)app_tencent_sv_force_disconnect_tencent_app},
};

const struct ai_handler_func_table tencent_handler_function_table =
  {&tencent_handler_function_list[0], (sizeof(tencent_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_TENCENT, &tencent_handler_function_table)

