#include <stdio.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "string.h"
#include "hal_trace.h"
#include "nvrecord.h"
#include "accessories.pb-c.h"
#include <protobuf-c/protobuf-c.h>
#include <time.h>
#include "app_ama_handle.h"
#include "ama_stream.h"
#include "app_ai_voice.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_spp.h"
#include "app_ai_ble.h"
#include "app_ai_tws.h"
#include "app_ai_if_thirdparty.h"
#include "voice_compression.h"

extern void app_bt_suspend_current_a2dp_streaming(void);

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
#define AMA_INDICATION_RSP_TIMEOUT (3000)
extern osTimerId ama_ind_rsp_timeout_timer;
#endif

ProtobufCAllocator ama_stack;

typedef struct ama_stack_buffer {
    uint16_t poolSize; 
    uint8_t *pool; 
} ama_stack_buffer_t; 

void * ama_stack_alloc(void *allocator_data, size_t size) {
#if 0
    ama_stack_buffer_t * stack_buffer = (ama_stack_buffer_t *)allocator_data; 
    ASSERT(stack_buffer->poolSize >= size,"AMA MEMORY ALLOC ERROR!!");
    return stack_buffer->pool;
#endif
    ama_stack_buffer_t * stack_buffer = (ama_stack_buffer_t *)allocator_data; 
    TRACE(2,"buffer poolsize :%d , alloc size : %d", stack_buffer->poolSize, size);
    assert(stack_buffer->poolSize >= size);
    stack_buffer->poolSize -= size;
    uint8_t * ptr = stack_buffer->pool;
    stack_buffer->pool += size; 
    return ptr;

}

void  ama_stack_free(void *allocator_data, void *pointer) {
    //ama_stack_buffer_t * stack_buffer = (ama_stack_buffer_t *)allocator_data; 
    TRACE(3,"%s allocator_data %p %p", __func__, allocator_data, pointer);
    //TRACE(4,"%s len %d buf %p %p", __func__, stack_buffer->poolSize, stack_buffer->pool, pointer);
    //nothing to do 
}

void  ama_stack_allocator_init(){
    ama_stack.alloc = ama_stack_alloc;
    ama_stack.free = ama_stack_free;
    ama_stack.allocator_data = NULL;
}

#define AMA_STACK_ALLOC_INIT_POOL(a,b)   \
    memset(a, 0, b); \
    ama_stack_buffer_t  *allocator_data = (ama_stack_buffer_t  *)(a); \
    allocator_data->poolSize = ((b)-sizeof(ama_stack_buffer_t));  \
    allocator_data->pool = (((uint8_t *)(a))+ sizeof(ama_stack_buffer_t)); \
    ama_stack.alloc = ama_stack_alloc; \
    ama_stack.free = ama_stack_free;\
    ama_stack.allocator_data  = allocator_data;

#define CASE_S(s) case s:return "["#s"]";
#define CASE_D()  default:return "[INVALID]";
const char *ama_payload_case2str(ControlEnvelope__PayloadCase payload_case)
{
    switch(payload_case) {
        CASE_S(CONTROL_ENVELOPE__PAYLOAD__NOT_SET)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_RESPONSE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_RESET_CONNECTION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_SYNCHRONIZE_SETTINGS)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_KEEP_ALIVE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_REMOVE_DEVICE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_UPGRADE_TRANSPORT)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_SWITCH_TRANSPORT)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_START_SPEECH)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_PROVIDE_SPEECH)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_STOP_SPEECH)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_ENDPOINT_SPEECH)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_NOTIFY_SPEECH_STATE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_FORWARD_AT_COMMAND)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_INCOMING_CALL)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_GET_CENTRAL_INFORMATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_GET_DEVICE_INFORMATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_GET_DEVICE_CONFIGURATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_OVERRIDE_ASSISTANT)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_START_SETUP)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_COMPLETE_SETUP)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_CONFIGURATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_UPDATE_DEVICE_INFORMATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_INFORMATION)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_ISSUE_MEDIA_CONTROL)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_GET_STATE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_SET_STATE)
        CASE_S(CONTROL_ENVELOPE__PAYLOAD_SYNCHRONIZE_STATE)
        CASE_D()
    }
}

const char *ama_command2str(Command command)
{
    switch(command) {
        CASE_S(COMMAND__NONE)
        CASE_S(COMMAND__RESET_CONNECTION)
        CASE_S(COMMAND__SYNCHRONIZE_SETTINGS)
        CASE_S(COMMAND__KEEP_ALIVE)
        CASE_S(COMMAND__REMOVE_DEVICE)
        CASE_S(COMMAND__UPGRADE_TRANSPORT)
        CASE_S(COMMAND__SWITCH_TRANSPORT)
        CASE_S(COMMAND__START_SPEECH)
        CASE_S(COMMAND__PROVIDE_SPEECH)
        CASE_S(COMMAND__STOP_SPEECH)
        CASE_S(COMMAND__ENDPOINT_SPEECH)
        CASE_S(COMMAND__NOTIFY_SPEECH_STATE)
        CASE_S(COMMAND__FORWARD_AT_COMMAND)
        CASE_S(COMMAND__INCOMING_CALL)
        CASE_S(COMMAND__GET_CENTRAL_INFORMATION)
        CASE_S(COMMAND__GET_DEVICE_INFORMATION)
        CASE_S(COMMAND__GET_DEVICE_CONFIGURATION)
        CASE_S(COMMAND__OVERRIDE_ASSISTANT)
        CASE_S(COMMAND__START_SETUP)
        CASE_S(COMMAND__COMPLETE_SETUP)
        CASE_S(COMMAND__NOTIFY_DEVICE_CONFIGURATION)
        CASE_S(COMMAND__UPDATE_DEVICE_INFORMATION)
        CASE_S(COMMAND__NOTIFY_DEVICE_INFORMATION)
        CASE_S(COMMAND__ISSUE_MEDIA_CONTROL)
        CASE_S(COMMAND__GET_STATE)
        CASE_S(COMMAND__SET_STATE)
        CASE_S(COMMAND__SYNCHRONIZE_STATE)
        CASE_D()
    }
}

const char *ama_streamID2str(STREAM_ID_E stream_id)
{
    switch(stream_id) {
        CASE_S(AMA_CONTROL_STREAM_ID)
        CASE_S(AMA_VOICE_STREAM_ID)
        CASE_D()
    }
}

bool ama_stream_header_parse(uint8_t *buffer, ama_stream_header_t *header)
{
    *(uint16_t *)header = ((uint16_t)buffer[0]<<8) | (uint16_t)buffer[1];

    header->length = buffer[2];
    if(header->use16BitLength == 1) 
    {
        header->length = (header->length << 8) | buffer[3];
    }

    if(header->streamId != AMA_CONTROL_STREAM_ID && header->streamId != AMA_VOICE_STREAM_ID)
    {
        TRACE(2,"%s stream ID is error %d", __func__, header->streamId);
        return false;
    }

    //TRACE(6,"version=%d stream_id=%d%s flags=%d payload_length_flag=%d length=%d", 
                //header->version, 
                //header->streamId, 
                //ama_streamID2str((STREAM_ID_E)header->streamId),
                //header->flags, 
                //header->use16BitLength, 
                //header->length);
    return true;
}


#define AMA_RESTART_TIMEOUT_INTERVEL 5000

static void ama_restart_mic_timeout_timer_cb(void const *n);
osTimerDef (AMA_STATE_MONITOR_TIMEOUT, ama_restart_mic_timeout_timer_cb); 
osTimerId    ama_restart_timeout_timer_id = NULL;

SpeechState ama_gloable_state;

static void ama_restart_mic_timeout_timer_cb(void const *n)
{
    TRACE(0,"TIME OUT====================================================================================>");
    if(ama_gloable_state!=SPEECH_STATE__IDLE){
        TRACE(0,"timeout restart ama stream!!!!");
        app_ai_voice_upstream_control(true, AIV_USER_AMA, 0);
    }
}


volatile unsigned int ama_each_stream_len;
extern void a2dp_handleKey(uint8_t a2dp_key);

#define _BUF_LEN L2CAP_MTU
uint32_t app_ama_audio_stream_handle(void *param1, uint32_t param2, uint8_t param3)
{
    uint8_t audio_stream_buf[_BUF_LEN];
    uint8_t *audio_stream_buf_p = NULL;
    uint8_t count = 0;
    uint32_t send_len = 0;
    ama_stream_header_t audio_header = AMA_AUDIO_STREAM_HEADER__INIT;
    size_t offset=0;
    size_t frame_len = ama_each_stream_len-3;
    size_t size = 0;

    TRACE(3,"%s len %d mtu %d", __func__, param2, app_ai_get_mtu(AI_SPEC_AMA));

    if (NULL == param1)
    {
        TRACE(1,"%s the buff is NULL", __func__);
        return false;
    }

    for (count=0; count<(param2/frame_len); count++)
    {
        audio_stream_buf_p = &audio_stream_buf[send_len];
        size = frame_len;
        audio_stream_buf_p[0] = ((uint8_t *)&audio_header)[1];
        audio_stream_buf_p[1] = ((uint8_t *)&audio_header)[0];

        if (size < 256)
        {
            audio_stream_buf_p[2] = (uint8_t)frame_len;
            memcpy(audio_stream_buf_p+3, param1+offset, frame_len);
            size += 3;
        }
        else
        {
            ((ama_stream_header_t *)audio_stream_buf_p)->use16BitLength = 1;
            audio_stream_buf_p[2] = (uint8_t)((frame_len&0xff00)>>8);
            audio_stream_buf_p[3] = (uint8_t)frame_len;
            memcpy(audio_stream_buf_p+4, param1+offset, frame_len);
            size += 4;
        }

        offset+=frame_len;
        send_len += size;
    }

    memcpy(param1, audio_stream_buf, send_len);
    return send_len;
}

typedef struct
{
    uint16_t protocolId;
    uint8_t majorversion;
    uint8_t minorversion;
    uint16_t transport_packet_size;
    uint16_t max_trans_data_size;
    uint8_t reserve[12];
} TRANSPORT_VER_EXCHANGE_INFO_T;

void ama_control_send_transport_ver_exchange(uint8_t dest_id)
{
    TRACE(1,"%s", __func__);
#if 0
    TRANSPORT_VER_EXCHANGE_INFO_T exchangeInfo = 
    {
        0x03,
        0xFE, 
        1,    // major version must be 1
        0,  // minor version must be 0
        0,
    };
#endif
    uint8_t send_buf[22] = {0};
    uint16_t *output_size_p = NULL;
    uint8_t *output_buf_p = NULL;

    output_size_p = (uint16_t *)send_buf;
    output_buf_p = &send_buf[AI_CMD_CODE_SIZE];

    output_buf_p[0]=0xFE;
    output_buf_p[1]=0X03;
    output_buf_p[2]=0X01;
    output_buf_p[3]=0X00;

    *output_size_p = 20;
    ai_transport_cmd_put(send_buf, (20+AI_CMD_CODE_SIZE));
    ai_mailbox_put(SIGN_AI_TRANSPORT, 20, AI_SPEC_AMA, dest_id);
}

static bool ama_control_stream_send(const ControlEnvelope *message,uint32_t len, uint8_t dest_id)
{
    uint8_t out_put_buf[128];
    uint16_t *output_size_p = NULL;
    uint8_t *output_buf_p = NULL;
    uint32_t size = 0;
    ama_stream_header_t cmd_header = AMA_CTRL_STREAM_HEADER__INIT;

    output_size_p = (uint16_t *)out_put_buf;  //first two byte represent the cmd length
    output_buf_p = &out_put_buf[AI_CMD_CODE_SIZE];
    output_buf_p[0] = ((uint8_t *)&cmd_header)[1];
    output_buf_p[1] = ((uint8_t *)&cmd_header)[0];

    if(len < 256)
    {
        output_buf_p[2] = (uint8_t)len;
        size = control_envelope__pack(message, output_buf_p+3);
        size += 3;
    }
    else
    {
        ((ama_stream_header_t *)output_buf_p)->use16BitLength = 1;
        output_buf_p[2] = (uint8_t)((len&0xff00)>>8);
        output_buf_p[3] = (uint8_t)len;
        size = control_envelope__pack(message, output_buf_p+4);
        size += 4;
    }

    *output_size_p = (uint16_t)size;
    TRACE(3,"%s len %d size %d", __func__, len, size);
    ai_transport_cmd_put(out_put_buf, (size+AI_CMD_CODE_SIZE));
    ai_mailbox_put(SIGN_AI_TRANSPORT, size, AI_SPEC_AMA, dest_id);

    return true;
}

uint32_t dialog_id = 0;

void dialog_id_set(uint32_t id)
{
    TRACE(1,"set dialog id to %d",id);
    dialog_id = id;
}

uint32_t dialog_id_get(void)
{
    TRACE(1,"get dialog id: %d",dialog_id);
    return dialog_id;
}

uint32_t start_dialog_id_get(void){
    static uint32_t start_dialog_id = 0;
    ++start_dialog_id;
    if(start_dialog_id>0x7fffffff)
        start_dialog_id= 0;
    TRACE(1,"start dialog id :%d ",start_dialog_id);
    return start_dialog_id;
}

void dialog_id_reset()
{
    //start_dialog_id=0;
}



int implement_ama_media_control(IssueMediaControl *media_control)
{
    TRACE(1,"implement_ama_media_control : %d ",media_control->control);
    switch(media_control->control){
        case MEDIA_CONTROL__PLAY:
            a2dp_handleKey(2);
            break;
        case MEDIA_CONTROL__PAUSE:            
            //app_bt_suspend_current_a2dp_streaming();
            a2dp_handleKey(3);
            break;
        case MEDIA_CONTROL__NEXT:
            a2dp_handleKey(4);
            break;
        case MEDIA_CONTROL__PREVIOUS:
            a2dp_handleKey(5);
            break;
        default:
            break;
    }
    return 0;
}

extern uint8_t is_a2dp_mode(void);

extern osTimerId app_ama_speech_state_timeout_timer_id;
static void ama_cmd_handler(ControlEnvelope* msg, uint8_t* parameter, uint8_t dest_id)
{
    char dev_bt_addr[6];
    bool isToResponse = true;
    size_t size = 0;
    uint8_t index = 0;

    ControlEnvelope__PayloadCase payloadCase = msg->payload_case;
    Command command = msg->command;

    ControlEnvelope controlMessage;
    control_envelope__init(&controlMessage);

    Response responseMessage;
    response__init(&responseMessage);

    TRACE(2,"----->payload case %d%s",payloadCase, ama_payload_case2str(payloadCase));
    TRACE(2,"----->command %d%s", msg->command, ama_command2str(msg->command));

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());

            if(command == COMMAND__ENDPOINT_SPEECH)
            {
                TRACE(0, "tws slave resume a2dp right now.");
                app_ai_voice_resume_blocked_a2dp();
            }
            return;
        }
    }

    if(payloadCase==CONTROL_ENVELOPE__PAYLOAD_RESPONSE){
        TRACE(1,"rsp err code %d",msg->response->error_code); 
        isToResponse = false;
        switch (command)
        {    
            case COMMAND__START_SPEECH:
            {
                //app_voice_report(APP_STATUS_INDICATION_GSOUND_MIC_OPEN, 0);
                TRACE(2,"COMMAND__START_SPEECH dialog: %d error_code %d",
                    msg->response->dialog->id, msg->response->error_code);
                dialog_id_set(msg->response->dialog->id);
                if(msg->response->error_code == ERROR_CODE__SUCCESS) {
                    TRACE(0,"COMMAND__START_SPEECH success");
                } else {
                    app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);
                    TRACE(0,"XXXXXXX---COMMAND__START_SPEECH----xxxxxxFAIL");
                }
                break;
            }
            
            case COMMAND__STOP_SPEECH:
            {
                TRACE(0,"COMMAND__STOP_SPEECH ---response");      
                break;
            }
            
            case COMMAND__INCOMING_CALL:
            {
                TRACE(0,"COMMAND__INCOMING_CALL ---response");
                break; 
            }
            
            case COMMAND__SYNCHRONIZE_STATE:
            {
                TRACE(0,"COMMAND__SYNCHRONIZE_STATE ---response");
                break;
            }
            
            case COMMAND__ENDPOINT_SPEECH:
            {
                TRACE(0,"COMMAND__ENDPOINT_SPEECH ---response");
                break; 
            }
            case COMMAND__GET_CENTRAL_INFORMATION:
            {
                TRACE(0,"COMMAND__GET_CENTRAL_INFORMATION ---response");
                break;
            }
            case COMMAND__RESET_CONNECTION:
            {
                TRACE(0,"COMMAND__RESET_CONNECTION ---response");
                break;
            }
            case COMMAND__KEEP_ALIVE:
            {
                TRACE(0,"COMMAND__KEEP_ALIVE ---response");
                break;
            }
            case COMMAND__NOTIFY_DEVICE_CONFIGURATION:
            {
                TRACE(0,"COMMAND__NOTIFY_DEVICE_CONFIGURATION ---response");

                if (app_ai_is_in_multi_ai_mode())
                {
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
                    if (ai_manager_get_spec_update_flag())
                    {
                        osTimerStop(ama_ind_rsp_timeout_timer);
                        ai_manager_spec_update_start_reboot();
                    }
#endif
                }
                break;
            }
            default:
                break;
        }
    }else{
    
        switch (command)
        {
            case COMMAND__GET_DEVICE_INFORMATION:
            {
                Transport transport[3] = {TRANSPORT__BLUETOOTH_RFCOMM, TRANSPORT__BLUETOOTH_LOW_ENERGY, TRANSPORT__BLUETOOTH_IAP};
                DeviceInformation deviceInfo;
                AI_TRANS_TYPE_E transport_type = AI_TRANSPORT_IDLE;
                AI_BATTERY_INFO_T battery;
                DeviceBattery ama_battery;
                device_battery__init(&ama_battery);
                if(0 == (GET_DEST_FLAG(dest_id)))
                {
                    transport_type = AI_TRANSPORT_SPP;
                }
                else
                {
                    transport_type = AI_TRANSPORT_BLE;
                }

                controlMessage.command =  command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD_DEVICE_INFORMATION;
                responseMessage.device_information = &deviceInfo;

                device_information__init(&deviceInfo);
                deviceInfo.serial_number = AMA_SERIAL_NUM;
                deviceInfo.name =nvrec_dev_get_bt_name();

                deviceInfo.n_supported_transports  = 1;
                if(transport_type == AI_TRANSPORT_SPP) {
                    deviceInfo.supported_transports = &transport[0];
                } else if(transport_type == AI_TRANSPORT_BLE) {
                    deviceInfo.supported_transports = &transport[1];
                } else {
                    TRACE(1,"%s connect type error", __func__);
                }
                
                deviceInfo.device_type = AMA_DEVICE_TYPE;
                ai_manager_get_battery_info(&battery);
                ama_battery.status = battery.status;
                ama_battery.scale = battery.scale;
                ama_battery.level = battery.level;
                deviceInfo.battery = &ama_battery;
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__GET_DEVICE_CONFIGURATION:
            {
                DeviceConfiguration deviceConfiguration;

                controlMessage.command =  msg->command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS; 
                responseMessage.payload_case = RESPONSE__PAYLOAD_DEVICE_CONFIGURATION; 
                responseMessage.device_configuration = &deviceConfiguration;
                
                device_configuration__init(&deviceConfiguration);
                deviceConfiguration.needs_assistant_override = 0;
                deviceConfiguration.needs_setup = 0; 

                if (app_ai_is_in_multi_ai_mode())
                {
                    if ((ai_manager_get_ama_assistant_enable_status() == 0) || \
                                (ai_manager_spec_get_status_is_in_invalid() == true))
                    {
                        TRACE(0,"current ai status not in ama");
                        deviceConfiguration.needs_assistant_override = 1;
                    }
                }

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__UPGRADE_TRANSPORT:
            {
                ConnectionDetails connection_details;

                if (TRANSPORT__BLUETOOTH_RFCOMM == msg->upgrade_transport->transport)
                {
                    uint8_t tem_buf[6];
                    nvrec_dev_get_btaddr(dev_bt_addr);
                    memcpy(tem_buf,dev_bt_addr,6);
                    for(index=0; index<6; index++){
                        dev_bt_addr[index]=tem_buf[5-index];
                    }
                    DUMP8("%02x ",dev_bt_addr,BT_ADDR_OUTPUT_PRINT_NUM);

                    controlMessage.command =command;
                    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;

                    controlMessage.response = &responseMessage;
                    responseMessage.error_code = ERROR_CODE__SUCCESS;
                    responseMessage.payload_case = RESPONSE__PAYLOAD_CONNECTION_DETAILS;
                    responseMessage.connection_details=&connection_details;

                    connection_details__init(&connection_details);
                    connection_details.identifier.len = 6;
                    connection_details.identifier.data=(uint8_t    *)dev_bt_addr;
                    
                    size = control_envelope__get_packed_size(&controlMessage);
                }
                break;
            }

            case COMMAND__SWITCH_TRANSPORT:
            {
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__START_SETUP:
            {
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__COMPLETE_SETUP:
            {
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);

                if (app_ai_is_in_multi_ai_mode())
                {
                    ai_manager_set_current_spec(AI_SPEC_AMA);
                    app_ai_setup_complete_handle(AI_SPEC_AMA);
                    //if (ai_manager_get_ama_assistant_enable_status() == 0)
                    //{
                        //TRACE(0,"current ama assistant has been overided");
                        //break;
                    //}
                }

                break;
            }

            case COMMAND__NOTIFY_SPEECH_STATE:
            {
                /*
                IDLE=0;
                LISTENING=1;
                PROCESSING=2;
                SPEAKING=3;
                */
                app_ai_set_speech_state((APP_AI_SPEECH_STATE_E)msg->notify_speech_state->state, AI_SPEC_AMA);
                if(msg->notify_speech_state->state &&
                    (app_ai_get_speech_type(AI_SPEC_AMA) != TYPE__PROVIDE_SPEECH && msg->notify_speech_state->state != SPEECH_STATE__SPEAKING)) {
                    osTimerStop(app_ama_speech_state_timeout_timer_id);
                    osTimerStart(app_ama_speech_state_timeout_timer_id, APP_AMA_SPEECH_STATE_TIMEOUT_INTERVEL);
                } else {
                    osTimerStop(app_ama_speech_state_timeout_timer_id); 
                }
                
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__KEEP_ALIVE:
            {
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__PROVIDE_SPEECH:
            {
                SpeechProvider speech_provider = SPEECH_PROVIDER__INIT;
                SpeechSettings settings = SPEECH_SETTINGS__INIT; 
                Dialog dialog = DIALOG__INIT;

                settings.audio_profile = AUDIO_PROFILE__NEAR_FIELD;

                if (0)
                {
                }
#ifdef VOC_ENCODE_OPUS
                if (app_ai_get_encode_type(AI_SPEC_AMA) == VOC_ENCODE_OPUS)
                {
                    if (app_ai_is_use_ai_32kbps_voice(AI_SPEC_AMA))
                    {
                        // 16KHZ : sample rate     32KBPS: opus encode bitrate
                        settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS;
                    }
                    else
                    {
                        settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
                    }
                }
#endif
#ifdef VOC_ENCODE_SBC
                else if (app_ai_get_encode_type(AI_SPEC_AMA) == VOC_ENCODE_SBC)
                {
                    settings.audio_format = AUDIO_FORMAT__MSBC;
                }
#endif
                else
                {
                    settings.audio_format = AUDIO_FORMAT__PCM_L16_16KHZ_MONO;
                }
                settings.audio_source = AUDIO_SOURCE__STREAM;

                dialog_id_set(msg->provide_speech->dialog->id);
                dialog.id = msg->provide_speech->dialog->id;

                speech_provider.dialog = &dialog;
                speech_provider.speech_settings = &settings;

                controlMessage.command = command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.payload_case = RESPONSE__PAYLOAD_SPEECH_PROVIDER;
                responseMessage.speech_provider = &speech_provider;

                //fix the "set a reminder" reply is interrupted by a new conversation 
                {
                    static uint8_t delay_cnt = 0;
                    TRACE(1, "Wait for ama's reply to complete...");
                    while(is_a2dp_mode() && delay_cnt++ < 5){
                        osDelay(100);
                    }
                    TRACE(1, "Wait done..");
                    delay_cnt = 0;
                }

                if (app_ai_is_role_switching(AI_SPEC_AMA))
                {
                    responseMessage.error_code = ERROR_CODE__USER_CANCELLED;
                }
                else
                {
                    responseMessage.error_code = ERROR_CODE__SUCCESS;
                    app_ai_set_stream_opened(true, AI_SPEC_AMA);
                    ai_set_can_wake_up(false, AI_SPEC_AMA);
                    app_ai_set_speech_type(TYPE__PROVIDE_SPEECH, AI_SPEC_AMA);

                    app_ai_voice_upstream_control(true, AIV_USER_AMA, 0);//REOPEN MIC

                    app_ai_spp_deinit_tx_buf();
                    ai_audio_stream_allowed_to_send_set(true, AI_SPEC_AMA);
                    ai_mailbox_put(SIGN_AI_WAKEUP, 0, AI_SPEC_AMA, dest_id);
                }

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            /// conversation finish successfully
            case COMMAND__ENDPOINT_SPEECH:
            {
                app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);

                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            /// conversation finish because of receive speech timeout
            case COMMAND__STOP_SPEECH:
            {
                app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);

                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__OVERRIDE_ASSISTANT:
            {
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                ama_control_stream_send(&controlMessage, size, dest_id);

                //notify alexa app to revert back to ama again
                if (app_ai_is_in_multi_ai_mode())
                {
                    if (msg->override_assistant->error_code == ERROR_CODE__SUCCESS)
                    {
                        TRACE(0,"enable switch back to alexa");
                        ama_notify_device_configuration(0, dest_id);
                        ai_manager_set_current_spec(AI_SPEC_AMA);
                        app_ai_tws_sync_ai_manager_info();
                    }
                    else
                    {
                        TRACE(0,"cancel switch back to alexa");
                    }
                }

                isToResponse = false;
                break;
            }

            case COMMAND__SYNCHRONIZE_SETTINGS:
            {
                if (app_ai_is_in_multi_ai_mode())
                {
                    ai_manager_set_current_spec(AI_SPEC_AMA);
                    app_ai_setup_complete_handle(AI_SPEC_AMA);
                }

                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__GET_STATE:
            {
                State state_rsp;
                AI_DEVICE_ACTION_PARAM_T param;
                controlMessage.command =command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                TRACE(1, "COMMAND__GET_STATE feature 0x%0x", msg->get_state->feature);
                state__init(&state_rsp);
                responseMessage.state = &state_rsp;

                param.event = msg->get_state->feature;
                if(ai_manager_get_device_status(AI_SPEC_AMA, &param)) {
                    responseMessage.state->feature = param.event;
                    if(param.value_type == AI_VALUE_BOOL) {
                        responseMessage.state->value_case = STATE__VALUE_BOOLEAN;
                        responseMessage.state->boolean = param.p.booler;
                    }else {
                        responseMessage.state->value_case = STATE__VALUE_INTEGER;
                        responseMessage.state->boolean = param.p.inter;
                    }
                    responseMessage.error_code = ERROR_CODE__SUCCESS;
                }else {
                    responseMessage.error_code = ERROR_CODE__UNKNOWN;
                }
                responseMessage.payload_case = RESPONSE__PAYLOAD_STATE;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            case COMMAND__SET_STATE:
            {
                TRACE(1, "COMMAND__SET_STATE state 0x%0x", msg->set_state->state->feature);
                AI_DEVICE_ACTION_PARAM_T param;
                SetState *setstate = msg->set_state;

                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                param.event = setstate->state->feature;
                if(setstate->state->value_case == STATE__VALUE_BOOLEAN){
                    param.value_type = AI_VALUE_BOOL;
                    param.p.booler = setstate->state->boolean;
                }else {
                    param.value_type = AI_VALUE_INT;
                    param.p.booler = setstate->state->integer;
                }

                if(ai_manager_set_device_status(AI_SPEC_AMA, &param)) {
                    responseMessage.error_code = ERROR_CODE__SUCCESS;
                }else {
                    responseMessage.error_code = ERROR_CODE__UNSUPPORTED;
                }
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;
                size = control_envelope__get_packed_size(&controlMessage);
                break;

            }
            case COMMAND__GET_DEVICE_FEATURES:
            {
                TRACE(1, "COMMAND__GET_DEVICE_FEATURES");
                DeviceFeatures dev_feature;
                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                device_features__init(&dev_feature);
                dev_feature.features = ama_device_action;
                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD_DEVICE_FEATURES;
                responseMessage.device_features = &dev_feature;
                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }
            case COMMAND__ISSUE_MEDIA_CONTROL:
            {
                TRACE(1, "COMMAND__ISSUE_MEDIA_CONTROL control %d", msg->issue_media_control->control);

                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                implement_ama_media_control(msg->issue_media_control);
                osDelay(100);
                break;
            }

            case COMMAND__FORWARD_AT_COMMAND:
            {
                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }
            case COMMAND__SYNCHRONIZE_STATE:
            {
                /*
                STATE__VALUE__NOT_SET = 0,
                STATE__VALUE_BOOLEAN = 2,
                STATE__VALUE_INTEGER = 3
                */
                TRACE(1, "synchronize_state %d", msg->synchronize_state->state->value_case);
                //ama_control_send_transport_ver_exchange();
                controlMessage.command = command;
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                responseMessage.error_code = ERROR_CODE__SUCCESS;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                size = control_envelope__get_packed_size(&controlMessage);
                break;
            }

            default:
            {
                TRACE(0,"AMA NOT SUPPORT CMD!!!");
                responseMessage.error_code = ERROR_CODE__UNSUPPORTED;
                responseMessage.payload_case = RESPONSE__PAYLOAD__NOT_SET;

                controlMessage.command = command; 
                controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
                controlMessage.response = &responseMessage;

                size = control_envelope__get_packed_size(&controlMessage);
                isToResponse = true;
                break;
            }
        }
    }
    
    if (isToResponse) {    
        ama_control_stream_send(&controlMessage, size, dest_id);
    }
}


void ama_control_stream_handler(uint8_t* ptr, uint32_t len, uint8_t dest_id)
{
    static uint8_t out[128];

    AMA_STACK_ALLOC_INIT_POOL(out, sizeof(out));
    ControlEnvelope* msg = control_envelope__unpack(&ama_stack, 
        len, ptr);

    ama_cmd_handler(msg, NULL, dest_id);
}

//reset the connection, the earphone will disconnect with APP. APP 
//will reconnect earphone after timeout.
//timeout -- A timeout in seconds. A value of 0 indicates an indefinite timeout
void ama_reset_connection (uint32_t timeout, uint8_t dest_id)
{
    size_t size = 0;

    ResetConnection req = RESET_CONNECTION__INIT;
    req.timeout = timeout;
    req.force_disconnect = true;

    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__RESET_CONNECTION;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESET_CONNECTION;
    controlMessage.reset_connection = &req;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE(3, "%s <------> size = %d timeout = %d", __func__, size, timeout);
    ama_control_stream_send(&controlMessage, size, dest_id);
}

void ama_notify_device_configuration (uint8_t is_assistant_override, uint8_t dest_id)
{
    bool need_send_control_strem = true;

    if (app_ai_is_in_multi_ai_mode() && 
            ai_manager_get_spec_connected_status(AI_SPEC_AMA) != 1)
    {
        need_send_control_strem = false;
    }

    if (need_send_control_strem)
    {
        size_t size = 0;
        DeviceConfiguration devicecfg;
        device_configuration__init(&devicecfg);
        devicecfg.needs_assistant_override = is_assistant_override;
        devicecfg.needs_setup = 0;

        NotifyDeviceConfiguration req = NOTIFY_DEVICE_CONFIGURATION__INIT;
        req.device_configuration = &devicecfg;

        ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
        controlMessage.command = COMMAND__NOTIFY_DEVICE_CONFIGURATION;
        controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_CONFIGURATION;
        controlMessage.notify_device_configuration = &req;

        size = control_envelope__get_packed_size(&controlMessage);
        TRACE(3,"%s<------>size=%d override %d", __func__, size, is_assistant_override);
        ama_control_stream_send(&controlMessage, size, dest_id);
    }

    if (app_ai_is_in_multi_ai_mode())
    {
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
        if (is_assistant_override)
        {
            ai_manager_set_ama_assistant_enable_status(0);
            ai_manager_set_spec_update_flag(true);
            osTimerStart(ama_ind_rsp_timeout_timer, AMA_INDICATION_RSP_TIMEOUT);
        }
        else
        {
            ai_manager_set_ama_assistant_enable_status(1);
            ai_manager_set_spec_update_flag(false);
        }
#endif
    }
}

extern uint32_t kw_start_index;
extern uint32_t kw_end_index;
//devices request speech
void ama_start_speech_request(uint8_t dest_id) 
{
    size_t size = 0;
    uint8_t wake_up_type = app_ai_get_wake_up_type(AI_SPEC_AMA);

    TRACE(5,"%s wake_up_type %d start %d end %d", __func__, \
                                wake_up_type, \
                                ai_struct[AI_SPEC_AMA].wake_word_start_index, \
                                ai_struct[AI_SPEC_AMA].wake_word_end_index);
    app_ai_set_speech_type(TYPE__NORMAL_SPEECH, AI_SPEC_AMA);

    ControlEnvelope controlMessage;
    StartSpeech speech;
    SpeechSettings settings; 
    SpeechInitiator initiator; 
    Dialog  dialog;
    SpeechInitiator__WakeWord wake_Word;

    control_envelope__init(&controlMessage);
    controlMessage.command = COMMAND__START_SPEECH; 
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_START_SPEECH;
    
    start_speech__init(&speech);

    speech_settings__init(&settings);
    if(wake_up_type == AIV_WAKEUP_TRIGGER_KEYWORD) {
        settings.audio_profile = AUDIO_PROFILE__FAR_FIELD;
    } else if(wake_up_type == AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD) {
        settings.audio_profile = AUDIO_PROFILE__CLOSE_TALK;
    } else {
        settings.audio_profile = AUDIO_PROFILE__NEAR_FIELD;
    }

    if (0)
    {
    }
#ifdef VOC_ENCODE_OPUS
    if(app_ai_get_encode_type(AI_SPEC_AMA) == VOC_ENCODE_OPUS)
    {
        if (app_ai_is_use_ai_32kbps_voice(AI_SPEC_AMA))
        {
            settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS;
        }
        else
        {
            settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
        }
    }
#endif
#ifdef VOC_ENCODE_SBC
    else if(app_ai_get_encode_type(AI_SPEC_AMA) == VOC_ENCODE_SBC)
    {
        settings.audio_format = AUDIO_FORMAT__MSBC;
    }
#endif
    else
    {
        settings.audio_format = AUDIO_FORMAT__PCM_L16_16KHZ_MONO;
    }

    settings.audio_source = AUDIO_SOURCE__STREAM;

    if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS){
        ama_each_stream_len=43;
    }else if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS){
        ama_each_stream_len=83;
    }else if(settings.audio_format==AUDIO_FORMAT__MSBC){
        ama_each_stream_len=60;
    }else{
        ama_each_stream_len=171;
    }
    
    speech_initiator__wake_word__init(&wake_Word);
    if(wake_up_type == AIV_WAKEUP_TRIGGER_KEYWORD) {
        if(settings.audio_format==AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS) {
#ifdef __KNOWLES
            wake_Word.start_index_in_samples = kw_start_index;
            wake_Word.end_index_in_samples = kw_end_index; 
#else
            wake_Word.start_index_in_samples = ai_struct[AI_SPEC_AMA].wake_word_start_index;
            wake_Word.end_index_in_samples = ai_struct[AI_SPEC_AMA].wake_word_end_index; 
#endif
        } else if(settings.audio_format==AUDIO_FORMAT__MSBC) {
            wake_Word.start_index_in_samples = 0;
            wake_Word.end_index_in_samples = 12000;
        } else {
            wake_Word.start_index_in_samples = 200;
            wake_Word.end_index_in_samples = 24000;
        }
    } else {
        wake_Word.start_index_in_samples = 0;
        wake_Word.end_index_in_samples = 0;
    }

    speech_initiator__init(&initiator);
    if(wake_up_type == AIV_WAKEUP_TRIGGER_KEYWORD) {
        initiator.type = SPEECH_INITIATOR__TYPE__WAKEWORD;
    } else if(wake_up_type == AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD) {
        initiator.type = SPEECH_INITIATOR__TYPE__PRESS_AND_HOLD;
    } else if(wake_up_type == AIV_WAKEUP_TRIGGER_TAP) {
        initiator.type = SPEECH_INITIATOR__TYPE__TAP;
    }

    initiator.wake_word = &wake_Word;

    dialog__init(&dialog);
    dialog.id = start_dialog_id_get();

    speech.initiator = &initiator;
    speech.settings = &settings;
    speech.dialog = &dialog;
    speech.suppressendpointearcon = false;
    speech.suppressstartearcon = false;

    controlMessage.start_speech = &speech;

    size = control_envelope__get_packed_size(&controlMessage);

    TRACE(2,"<------>size = %d dialog = %d", size, dialog.id);

    ama_control_stream_send(&controlMessage, size, dest_id);

    ai_audio_stream_allowed_to_send_set(true, AI_SPEC_AMA);
}

void ama_stop_speech_request (ama_stop_reason_e stop_reason, uint8_t dest_id) 
{
    size_t size = 0;

    Dialog dialog = DIALOG__INIT;
    dialog.id = dialog_id_get();
    TRACE(2,"%s dialog: %d", __func__, dialog.id);
    
    StopSpeech stop_speech = STOP_SPEECH__INIT;
    stop_speech.dialog = &dialog;
    if(stop_reason==USER_CANCEL){
        stop_speech.error_code = ERROR_CODE__USER_CANCELLED;
    }else if(stop_reason==TIME_OUT){
        stop_speech.error_code = ERROR_CODE__INTERNAL;
    }else{
        stop_speech.error_code = ERROR_CODE__UNKNOWN;
    }
    
    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__STOP_SPEECH;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_STOP_SPEECH;
    controlMessage.stop_speech = &stop_speech;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE(1,"<------>size = %d", size);
    ama_control_stream_send(&controlMessage, size, dest_id);
}


void ama_endpoint_speech_request(uint32_t dialog_id, uint8_t dest_id)
{
    size_t size = 0;

    Dialog dialog = DIALOG__INIT;
    dialog.id = dialog_id_get();
    TRACE(2,"%s dialog: %d", __func__, dialog.id);
    
    EndpointSpeech endpoint_speech = ENDPOINT_SPEECH__INIT;
    endpoint_speech.dialog = &dialog;
    
    ControlEnvelope controlMessage = CONTROL_ENVELOPE__INIT;
    controlMessage.command = COMMAND__ENDPOINT_SPEECH;
    controlMessage.payload_case = CONTROL_ENVELOPE__PAYLOAD_ENDPOINT_SPEECH;
    controlMessage.endpoint_speech = &endpoint_speech;

    size = control_envelope__get_packed_size(&controlMessage);
    TRACE(1,"<------>size = %d", size);
    ama_control_stream_send(&controlMessage, size, dest_id);
}

