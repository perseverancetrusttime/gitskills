#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <time.h>

#include "dma.pb-c.h"
#include "platform.h"
#include "cqueue.h"
#include "app_hfp.h"

#include "nvrecord.h"

#include "ai_transport.h"
#include "ai_thread.h"
#include "ai_spp.h"
#include "app_ai_voice.h"
#include "sha256.h"
#include "app_bt.h"
#include "dueros_dma.h"
#include "app_dma_handle.h"
#include "app_ai_if.h"
#include "ai_manager.h"

//#undef TRACE
//#define TRACE

#define USE_RECV_DATA_QUEUE 1

DUEROS_DMA dueros_dma = {0,};

#if defined(USE_RECV_DATA_QUEUE)
#define RECV_DATA_QUEUE_SIZE (1024)
unsigned char recv_data_temp_buff[RECV_DATA_QUEUE_SIZE];
#endif

#ifdef ASSAM_PKT_ON_UPLEVEL
#define ASSAM_PKT_QUEUE_SIZE (1024)
CQueue assam_pkt_queue;
unsigned char assam_pkt_queue_buff[ASSAM_PKT_QUEUE_SIZE];
unsigned char assam_pkt_data_temp_buff[ASSAM_PKT_QUEUE_SIZE];
#endif

int dueros_dma_init(PROTOCOL_VER* dma_version, SignMethod signMode)
{
    if(dma_version == NULL)
    {
        return -1;
    }

    srand( time(NULL) );

    dueros_dma.protocol_version = *dma_version;

    /*init queque*/
    dueros_dma.signMode  =  signMode;

#ifdef ASSAM_PKT_ON_UPLEVEL
    InitCQueue(&assam_pkt_queue, sizeof(assam_pkt_queue_buff), assam_pkt_queue_buff);
#endif

    dueros_dma.inited = DUEROS_DMA_INITED;

    TRACE(0,"[DMA]DMA INIT ok... \n");
    return 0;
}

int dueros_dma_connected()
{
#ifdef ASSAM_PKT_ON_UPLEVEL
    InitCQueue(&assam_pkt_queue, sizeof(assam_pkt_queue_buff), assam_pkt_queue_buff);
#endif

    dueros_dma.linked = DUEROS_DMA_LINKED;
    return 0;
}

int dueros_dma_disconnected()
{
    //dueros_clear_all_queue();
    dueros_dma.linked= 0;
    return 0;
}


/*get current version */
static WORD32 get_cur_version(void)
{
    if(dueros_dma.inited != DUEROS_DMA_INITED)
    {
        return -1;
    }

    return dueros_dma.protocol_version.hVersion;
}

#define _BUF_LEN L2CAP_MTU
uint32_t dma_audio_stream_send(void *param1, uint32_t param2)
{
    DUEROS_DMA_HEADER audio_header = {0};
    WORD8 audio_buf_p[_BUF_LEN];
    int data_len = 0;
    uint8_t *data_len_p = NULL;
    uint32_t send_len = 0;

    TRACE(2,"%s len %d", __func__, param2);

    if(!is_ai_audio_stream_allowed_to_send(AI_SPEC_BAIDU)){
        TRACE(1,"%s AI audio stream don't allow to send", __func__);
        return 0;
    }
    if(param1 == NULL){
        TRACE(1,"%s the buff is NULL", __func__);
        return 0;
    }

    memset(audio_buf_p, 0, _BUF_LEN);
    data_len_p = audio_buf_p + DUEROS_DMA_HEADER_SIZE;
    if(param2 < 0xff) {
        data_len = 1;
        *(unsigned char *)data_len_p = param2;
    } else if(param2 < 0xffff) {
        *(unsigned char*)data_len_p =(param2&0xff00) >>8;
        *((unsigned char*)data_len_p+1) = param2 &0xff;
        data_len = 2;
    } else {
        TRACE(2,"%s error len %d", __func__, param2);
        return 0;
    }

    audio_header.length    = data_len -1;
    audio_header.reserve   = 0;
    audio_header.streamID  = DUEROS_DMA_AUDIO_STREAM_ID;
    audio_header.version   = get_cur_version();

    audio_buf_p[0] = ((uint8_t *)&audio_header)[1];
    audio_buf_p[1] = ((uint8_t *)&audio_header)[0];

    memcpy(audio_buf_p + DUEROS_DMA_HEADER_SIZE + data_len, param1, param2);
    send_len = DUEROS_DMA_HEADER_SIZE + data_len + param2;

    memcpy(param1, audio_buf_p, send_len);
    TRACE(2,"%s len %d", __func__, send_len);
    return send_len;
}

bool dma_control_stream_send(const ControlEnvelope *message, uint32_t length, uint8_t dest_id)
{
    DUEROS_DMA_HEADER cmd_header = {0};
    uint16_t *output_size_p = NULL;
    uint8_t *output_buf_p = NULL;
    uint8_t *data_len_p = NULL;
    WORD8 _buf[256];
    int data_len = 0;
    uint32_t send_len = 0;

    TRACE(2,"%s len %d", __func__, length);
#if 0
    if(dueros_dma.linked == 0)
    {
        TRACE(0,"[dueros] data path  is not linked ");
        return false;
    }
#endif

    memset(_buf, 0, 256);
    output_size_p = (uint16_t *)_buf;  //first two byte represent the cmd length
    data_len_p = _buf + DUEROS_DMA_HEADER_SIZE + AI_CMD_CODE_SIZE;

    if (length < 0xff) {
        data_len = 1;
        *(unsigned char *)data_len_p = length;
    } else if (length < 0xffff) {
        *(unsigned char*)data_len_p = (length & 0xff00) >> 8;
        *((unsigned char*)data_len_p+1) = length & 0xff;
        data_len = 2;
    } else {
        TRACE(2,"%s error len %d", __func__, length);
        return false;
    }


    cmd_header.length    = data_len -1;
    cmd_header.reserve   = 0;
    cmd_header.streamID  = DUEROS_DMA_CMD_STREAM_ID;
    cmd_header.version   = get_cur_version();

    output_buf_p = &_buf[AI_CMD_CODE_SIZE];
    output_buf_p[0] = ((uint8_t *)&cmd_header)[1];
    output_buf_p[1] = ((uint8_t *)&cmd_header)[0];

    dma_control_envelope__pack(message, (_buf + DUEROS_DMA_HEADER_SIZE + data_len + AI_CMD_CODE_SIZE));
    send_len = length + DUEROS_DMA_HEADER_SIZE + data_len;

    *output_size_p = (uint16_t)send_len;

    TRACE(2,"%s send_len %d", __func__, send_len);
    ai_transport_cmd_put(_buf, (send_len+AI_CMD_CODE_SIZE));


    ai_mailbox_put(SIGN_AI_TRANSPORT, send_len, AI_SPEC_BAIDU, dest_id);
    return true;
}

extern void ota_enter_check() ;
/*Fristly know how to get the version info ,now write directly*/
int dueros_dma_sent_ver(uint8_t dest_id)
{
    WORD32 offset = 0;
    WORD32 len = 0;
    WORD8 pBuf[128];
    uint16_t *data_len_p = (uint16_t *)pBuf;

    if(dueros_dma.inited != DUEROS_DMA_INITED)
    {
        TRACE(1,"%s DUEROS_DMA NOT INITED", __func__);
        return 0;
    }

    memcpy(pBuf + AI_CMD_CODE_SIZE, (WORD8*)&dueros_dma.protocol_version, sizeof(PROTOCOL_VER));

    offset = 0 + AI_CMD_CODE_SIZE;
    DUEROS_StoreBE16(pBuf+offset,dueros_dma.protocol_version.magic);
    offset += sizeof(dueros_dma.protocol_version.magic);
    DUEROS_StoreBE8(pBuf+offset,dueros_dma.protocol_version.hVersion);
    offset += sizeof(dueros_dma.protocol_version.hVersion);
    DUEROS_StoreBE8(pBuf+offset,dueros_dma.protocol_version.lVersion);
    offset += sizeof(dueros_dma.protocol_version.lVersion);
    (void)memcpy(pBuf+offset, &dueros_dma.protocol_version.reserve, sizeof(dueros_dma.protocol_version.reserve));

    len  = sizeof(PROTOCOL_VER);

    *data_len_p = (uint16_t)len;

    TRACE(1,"%s", __func__);
    DUMP8("%02x ", pBuf, len+AI_CMD_CODE_SIZE);
    TRACE(0,"end");

    ai_transport_cmd_put(pBuf, len+AI_CMD_CODE_SIZE);
    ai_mailbox_put(SIGN_AI_TRANSPORT, len, AI_SPEC_BAIDU, dest_id);
    return 0;

}


int dueros_dma_send_prov_speech_ack(uint32_t dialog_id,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    SpeechProvider provider ;
    SpeechSettings setting ;
    Dialog  dialog ;
    WORD32 size  = 0;

    dma_dialog__init(&dialog);
    dma_speech_settings__init(&setting);
    dma_speech_provider__init(&provider);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    dialog.id = dialog_id;
    dma_get_setting(&setting);
    dueros_dma.dialog= dialog_id;


    provider.dialog = &dialog;
    provider.settings = &setting;

    response.error_code = ERROR_CODE__SUCCESS;
    response.payload_case = RESPONSE__PAYLOAD_SPEECH_PROVIDER ;
    response.speechprovider = &provider;

    envelope.command = COMMAND__PROVIDE_SPEECH_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);
    return 0;
}

int dueros_dma_stop_speech_ack(uint32_t dialog_id,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_control_envelope__init(&envelope);
    dma_response__init(&response);

    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code = ERROR_CODE__SUCCESS;

    envelope.command = COMMAND__STOP_SPEECH_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;  //todo how to fill request

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}


int dueros_dma_send_notify_speech_state_ack(char * request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_control_envelope__init(&envelope);
    dma_response__init(&response);

    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode

    envelope.command = COMMAND__NOTIFY_SPEECH_STATE_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}



InitiatorType dueros_dma_send_get_dev_info_ack(char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    DeviceInformation deviceinformation;
    WORD32 size  = 0;

    dma_device_information__init(&deviceinformation);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    TRACE(0,"-1--dueros_dma_send_get_dev_info_ack --- \n");
    dma_get_dev_info(&deviceinformation);

    response.payload_case = RESPONSE__PAYLOAD_DEVICE_INFORMATION ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    response.deviceinformation = &deviceinformation;

    envelope.command = COMMAND__GET_DEVICE_INFORMATION_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return deviceinformation.initiator_type;

}

int  dueros_dma_sent_get_dev_conf_ack(char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    DeviceConfiguration deviceinformation ;
    WORD32 size  = 0;

    dma_device_configuration__init(&deviceinformation);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    dma_get_dev_config(&deviceinformation);

    response.payload_case = RESPONSE__PAYLOAD_DEVICE_CONFIGURATION ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    response.deviceconfiguration = &deviceinformation;

    envelope.command = COMMAND__GET_DEVICE_CONFIGURATION_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;
    TRACE(1,"---dueros_dma_sent_get_dev_conf_ack request_id = %s ",envelope.request_id);

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}

int dueros_dma_send_get_state_ack(uint32_t feature,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    State   state ;
    WORD32 size  = 0;

    dma_state__init(&state);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    dma_get_state(feature, &state);

    response.payload_case = RESPONSE__PAYLOAD_STATE ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    response.state = &state;

    envelope.command = COMMAND__GET_STATE_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}

/*?????????????notify state*/
int  dueros_dma_send_notify_dev_conf(uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    NotifyDeviceConfiguration notify_dev_conf;
    unsigned int request_id_nu = 0;
    char    char_request_id[10]={0,};
    WORD32 size  = 0;

    dma_notify_device_configuration__init(&notify_dev_conf);
    dma_control_envelope__init(&envelope);

//  dueros_dma.dma_oper.get_dev_config(&deviceinformation);

    envelope.command = COMMAND__NOTIFY_DEVICE_CONFIGURATION;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_CONFIGURATION;
    envelope.notifydeviceconfiguration = &notify_dev_conf;

    request_id_nu = rand();
    sprintf(char_request_id, "%x",request_id_nu);
    envelope.request_id = char_request_id;

    TRACE(1,"---dueros_dma_send_notify_dev_conf request_id = %s ",envelope.request_id);

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
    //return  dueros_dma_sent_get_dev_conf_ack(INITIAL_REQUEST_ID);
}


int dueros_dma_send_set_state_ack(SetState *set_state,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    State   state ;
    WORD32 size  = 0;

    dma_state__init(&state);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    //???????????????sstate.base????????
    state.feature= set_state->state->feature;

    state.value_case = set_state->state->value_case;
    state.integer = set_state->state->integer;

    dma_set_state(set_state->state);

    response.payload_case = RESPONSE__PAYLOAD_STATE ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    response.state = &state;

    envelope.command = COMMAND__SET_STATE_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}


//?????????????
void dueros_dma_send_sync_state(uint32_t feature, uint8_t dest_id)
{
    //TRACE(1,"***** IN dueros_dma_send_sync_state feature=%x",feature);
    ControlEnvelope  envelope ;
    SynchronizeState   synchronize_state ;
    State   state ;
    unsigned int request_id_nu = 0;
    char char_request_id[10]={0,};
    WORD32 size  = 0;

    dma_state__init(&state);
    dma_synchronize_state__init(&synchronize_state);
    dma_control_envelope__init(&envelope);

    dma_get_state(feature, &state);

    synchronize_state.state =  &state;

    envelope.command = COMMAND__SYNCHRONIZE_STATE;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_SYNCHRONIZE_STATE;
    envelope.synchronizestate = &synchronize_state;

    request_id_nu = rand();
    sprintf(char_request_id, "%x",request_id_nu);
    envelope.request_id = char_request_id;



    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return ;
}



int dueros_dma_start_speech(uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    StartSpeech  start_speech;
    SpeechSettings speech_setting;
    SpeechInitiator speech_initiator;
    Dialog  dialog;
    SpeechInitiator__WakeWord speech_wake_word;
//  DeviceInformation  dev_info;

    unsigned int request_id_nu = 0;
    char char_request_id[10]={0,};
    app_ai_set_speech_type(TYPE__NORMAL_SPEECH, AI_SPEC_BAIDU);

    WORD32 len  = 0;
    dma_speech_initiator__wake_word__init(&speech_wake_word);
    dma_speech_settings__init(&speech_setting);
    dma_speech_initiator__init(&speech_initiator);
    dma_dialog__init(&dialog);
    dma_start_speech__init(&start_speech);
    dma_control_envelope__init(&envelope);

    /*unknow*/
    speech_wake_word.start_index_in_samples =0 ;
    speech_wake_word.end_index_in_samples  = 10;

    speech_initiator.wake_word = &speech_wake_word;
    speech_initiator.type = SPEECH_INITIATOR__TYPE__TAP;
    speech_initiator.play_prompt_tone = false;

    dma_get_setting(&speech_setting);

    dialog.id = (rand()%100 +1);
    dueros_dma.dialog = dialog.id;

    TRACE(2,"%s dialog %d", __func__, dueros_dma.dialog);
    start_speech.initiator = &speech_initiator;
    start_speech.settings = &speech_setting;
    start_speech.dialog = &dialog;

    envelope.command = COMMAND__START_SPEECH;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_START_SPEECH;
    envelope.startspeech = &start_speech;

    request_id_nu = rand();
    sprintf(char_request_id, "%x",request_id_nu);
    envelope.request_id = char_request_id;

    len = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, len, dest_id);

    return 0;
}

int dueros_dma_stop_speech(uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    StopSpeech  stop_speech;
    Dialog  dialog;
    unsigned int request_id_nu = 0;
    char char_request_id[10]={0,};

    WORD32 len  = 0;
    TRACE(1,"%s", __func__);

    dma_dialog__init(&dialog);
    dma_stop_speech__init(&stop_speech);
    dma_control_envelope__init(&envelope);

    dialog.id = dueros_dma.dialog ;

    stop_speech.dialog = &dialog;

    envelope.command = COMMAND__STOP_SPEECH;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_STOP_SPEECH;
    envelope.stopspeech = &stop_speech;

    request_id_nu = rand();
    sprintf(char_request_id, "%x",request_id_nu);
    envelope.request_id = char_request_id;

    len = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, len, dest_id);

    return 0;

}


int dueros_dma_send_endpoint_speech_ack(char *requeset_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_response__init(&response);
    dma_control_envelope__init(&envelope);


    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode


    envelope.command = COMMAND__END_POINT_SPEECH_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;


    envelope.request_id = requeset_id;


    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}

int dueros_dma_send_at_command_ack(char *request_id,char *cmd, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_response__init(&response);
    dma_control_envelope__init(&envelope);


    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode

    envelope.command = COMMAND__FORWARD_AT_COMMAND_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    app_hfp_send_at_command(cmd);
    return 0;
}

int dueros_dma_send_synchronize_state_ack(char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    State      state;

    WORD32 size  = 0;

    dma_state__init(&state);
    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    state.feature = 0x1;
    state.value_case  = STATE__VALUE_BOOLEAN;
    state.boolean    = true;

    response.payload_case = RESPONSE__PAYLOAD_STATE ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    response.state   = &state;

    envelope.command = COMMAND__SYNCHRONIZE_STATE_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;
}

extern const ProtobufCEnumDescriptor atcommand__descriptor;


int dueros_dma_find_at_cmd_enum_by_name(char *cmd)
{
    int i = 0;
    int size = atcommand__descriptor.n_value_names;
    for(i = 0;i <size ;i++)
    {
        if (strcmp(atcommand__descriptor.values_by_name[i].name, cmd) == 0)
        {
            return atcommand__descriptor.values_by_name[i].index;
        }


    }

    return -1;

}

static void dueros_get_random_string(char *rand_string, int len)
{
    unsigned int i;
    unsigned int flag = 0;

    if(rand_string == NULL)
    {
        TRACE(1,"[%s] rand_string is NULL", __FUNCTION__);
        return;
    }

    for (i = 0; i < len - 1; i++)
    {
        flag = rand() % 3;
        switch (flag)
        {
            case 0:
                rand_string[i] = 'A' + rand() % 26;
                break;
            case 1:
                rand_string[i] = 'a' + rand() % 26;
                break;
            case 2:
                rand_string[i] = '0' + rand() % 10;
                break;
            default:
                rand_string[i] = 'x';
                break;
        }
    }

    rand_string[len-1] = '\0';
    TRACE(2,"[%s] %s", __func__, rand_string);
}

static uint32_t merge_5strings(char *new_string, char *string1, char *string2, const char *string3, const char*string4, char *string5)
{
    unsigned int char_num;
    char_num = 1 + strlen(string1) +  strlen(string2) + strlen(string3) + strlen(string4) + strlen(string5);

    (void)sprintf(new_string, "%s%s%s%s%s", string1, string2, string3, string4, string5);
    TRACE(2,"--merge 5 num %d -- %s", char_num, new_string);
    return char_num;
}


static uint32_t merge_4strings(char *new_string, char *string1, const char*string2, const char*string3, char*string4)
{
    unsigned int char_num;
    char_num = 1 + strlen(string1) +  strlen(string2) + strlen(string3) + strlen(string4) ;
    (void)sprintf(new_string,"%s%s%s%s",string1,string2,string3,string4);
    TRACE(1,"--merge 4-%s",new_string);

    return char_num;
}


void dump_byte(unsigned char *byte,unsigned int);
void convert_byte_to_char(void* byte,char *sha256_char,unsigned int len)
{
    int count = 0;
    while(len--) {
        snprintf(sha256_char +count*2,3,"%02x",*(unsigned char*)byte++);
        count++;
    }
}

int dueros_dma_paired_ack(char *request_id, uint8_t dest_id)
{

    char sha256_char[65] =  {0,};
    unsigned char sha256_result[33] =  {0,};
    unsigned int size = 0;
    char new_string[256];
    const char *key = NULL;

    DeviceInformation devInfo;

    ControlEnvelope  envelope ;
    PairInformation pairInfo;
    Response   response ;

    dma_control_envelope__init(&envelope);
    dma_response__init(&response);
    pair_information__init(&pairInfo);


#if 0
    if(dueros_dma.paired == 1)
    {
        TRACE(1,"%s:paired!", __func__);
    }else
#endif

    {
        dueros_get_random_string(dueros_dma.rand, BAIDU_DATA_RAND_LEN+1);
#ifdef NVREC_BAIDU_DATA_SECTION
        nvrec_set_rand(dueros_dma.rand);
#endif
        if(dueros_dma.rand == NULL)
        {
            TRACE(1,"[%s] generate rand error",__func__);
            return  -1;
        }

        dma_get_dev_info(&devInfo);
        key = dma_get_key();

        merge_4strings(new_string, dueros_dma.rand, key ,devInfo.product_id, devInfo.serial_number);

        SHA256_hash(new_string, strlen(new_string), sha256_result);
        TRACE(1,"sha256_result %d", strlen((const char*)sha256_result));
        DUMP8("%02x ", (const char*)sha256_result, strlen((const char*)sha256_result));
        TRACE(0," ");
        DUMP8("%c ", (const char*)sha256_result, strlen((const char*)sha256_result));
        TRACE(0," ");

        sha256_result[32] = '\0';
        //dump_byte(sha256_result,32);
        convert_byte_to_char(sha256_result, sha256_char, 32);

        pairInfo.rand = dueros_dma.rand;
        pairInfo.sign =  sha256_char;
        TRACE(1,"-qwer---%s-",sha256_char);
        pairInfo.signmethod = dueros_dma.signMode ;

        response.error_code = ERROR_CODE__SUCCESS;
        response.pairinformation = &pairInfo;
        response.payload_case = RESPONSE__PAYLOAD_PAIR_INFORMATION;


        envelope.command = COMMAND__PAIR_ACK;
        envelope.request_id = request_id;
        envelope.payload_case    = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
        envelope.response = &response;

        size = dma_control_envelope__get_packed_size(&envelope);
        if(size >= 128)
        {
            TRACE(1,"[%s] control_envelope__get_packed_size too large",__func__);
            return -1;
        }

        dma_control_stream_send(&envelope, size, dest_id);

        return 0;
    }
}
void dump_byte(unsigned char *byte,unsigned int count)
{
    if (byte == NULL) {
        return ;
    }
    TRACE(0,"dump-----");

    while(count--) {
        TRACE(1,"%02x",*byte);
        byte++;
    }
}

int dueros_dma_check_summary(ControlEnvelope * envelop)
{

    char sha256_char[65] =  {0,};
    unsigned char sha256_result[33] =  {0,};

    char input_string[256];
    const char *key = dma_get_key();
    DeviceInformation devInfo;
    dma_get_dev_info(&devInfo);


    merge_5strings(input_string,envelop->rand2,dueros_dma.rand ,key,devInfo.product_id,devInfo.serial_number);
    TRACE(3,"%s rand2 %s sign2 %s", __func__, envelop->rand2, envelop->sign2);

    SHA256_hash(input_string, strlen(input_string), sha256_result);

    sha256_result[32]='\0';
    //dump_byte(sha256_result, 32);
    convert_byte_to_char(sha256_result, sha256_char, 32);
    if(strcmp(envelop->sign2, sha256_char) == 0) {
        TRACE(0,"[dueros_dma_check_summary]OK");
        return  0 ;
    } else {
        TRACE(0,"[dueros_dma_check_summary] not OK");
        return -1;
    }
}

int dueros_dma_send_check_sum_fail(Command cmd,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_control_envelope__init(&envelope);
    dma_response__init(&response);

    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code = ERROR_CODE__SIGN_VERIFY_FAIL;

    envelope.command = cmd;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;  //todo how to fill request

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;

}

int dueros_dma_send_get_state_check_sum_fail(Command cmd,char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;
    State      state;

    dma_state__init(&state);
    dma_control_envelope__init(&envelope);
    dma_response__init(&response);

    state.feature = 0x2;
    state.value_case  = STATE__VALUE_BOOLEAN;
    state.boolean    = false;

    response.payload_case = RESPONSE__PAYLOAD_STATE;
    response.error_code = ERROR_CODE__SIGN_VERIFY_FAIL;
    response.state = &state;

    envelope.command = cmd;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;  //todo how to fill request

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);

    return 0;

}

void forward_test_command_ack(char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_response__init(&response);
    dma_control_envelope__init(&envelope);


    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode


    envelope.command = COMMAND__FORWARD_TEST_COMMAND_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;


    envelope.request_id = request_id;


    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);
}

void dueros_dma_forward_test(char *request_id, char *command, uint8_t dest_id)
{
    forward_test_command_ack(request_id, dest_id);
}

void dueros_dma_media_control_ack(char *request_id, uint8_t dest_id)
{
    ControlEnvelope  envelope ;
    Response   response ;
    WORD32 size  = 0;

    dma_response__init(&response);
    dma_control_envelope__init(&envelope);

    response.payload_case = RESPONSE__PAYLOAD__NOT_SET ;
    response.error_code =  ERROR_CODE__SUCCESS;//todo errorcode
    envelope.command = COMMAND__ISSUE_MEDIA_CONTROL_ACK;
    envelope.payload_case  = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
    envelope.response = &response;
    envelope.request_id = request_id;

    size = dma_control_envelope__get_packed_size(&envelope);
    dma_control_stream_send(&envelope, size, dest_id);
}

void dueros_dma_media_control(char *request_id, MediaControl control, uint32_t volume, uint8_t dest_id)
{
    dma_media_control(control, volume);
    dueros_dma_media_control_ack(request_id, dest_id);
}


void    dueros_dma_set_speech_state(SpeechState speech_state)
{
       dueros_dma.speech_state  = speech_state;
}

SpeechState  dueros_dma_get_speech_state()
{
    return dueros_dma.speech_state;
}

uint32_t app_dma_parse_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    ControlEnvelope * envelope = NULL ;
    InitiatorType initiatorTye ;
    DUEROS_DMA_HEADER cmd_header = {0};
    WORD8 *buf = NULL;
    int avail = 0;
    uint8_t *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    WORD8 * payload_buf = NULL;
    WORD32 payload_len = 0, process_len = 0;

process_again:
    avail = LengthOfCQueue(&ai_rev_buf_queue);
    if (avail > 4) {
        PeekCQueue(&ai_rev_buf_queue, avail, &e1, &len1, &e2, &len2);
        if (avail == (len1+len2)) {
            memcpy(recv_data_temp_buff, e1, len1);
            memcpy(recv_data_temp_buff+len1, e2, len2);

            buf = recv_data_temp_buff;
        }
        else {
            ResetCQueue(&ai_rev_buf_queue);
            TRACE(1,"%s: PeekCQueue fail!", __func__);
            return 0;
        }
    }
    else {
        ResetCQueue(&ai_rev_buf_queue);
        TRACE(1,"%s: queue too short, return!", __func__);
        return 0;
    }

    TRACE(1,"%s", __func__);

    if(dueros_dma.inited != DUEROS_DMA_INITED) {
        TRACE(0,"dueros_dma.inited != DUEROS_DMA_INITED\n");
        return -1;
    }

    //  dump_byte(buf,len );
    ((uint8_t *)&cmd_header)[0] = buf[1];
    ((uint8_t *)&cmd_header)[1] = buf[0];
    //DUMP8("0x%x ", buf, 2);
    //TRACE(4,"version %d streamID %d reserve %d length %d",
            //cmd_header.version,
            //cmd_header.streamID,
            //cmd_header.reserve,
            //cmd_header.length);

    if(cmd_header.length) {
        payload_buf = buf + 4;
        payload_len = buf[2]<<8|payload_buf[3];
        process_len = payload_len + 4;
    } else {
        payload_buf = buf + 3;
        payload_len = buf[2];
        process_len = payload_len + 3;
    }

    envelope = dma_control_envelope__unpack(NULL, payload_len, payload_buf);

    switch(envelope->command)
    {
        case COMMAND__PAIR:
        {
            TRACE(1,"COMMAND__PAIR id= %s\n",envelope->request_id);
            //ai_struct.ai_stream.ai_setup_complete = true;
            dueros_dma_paired_ack(envelope->request_id, param3);
            break;
        }

        case COMMAND__PROVIDE_SPEECH :
        {
            uint8_t ai_connect_index = app_ai_get_connect_index_from_ai_spec(AI_SPEC_BAIDU);
            if(ai_connect_index == ai_manager_get_foreground_ai_conidx())
            {
                TRACE(1,"COMMAND__PROVIDE_SPEECH id = %s\n",envelope->request_id);

                app_ai_spp_deinit_tx_buf();
                if( dueros_dma_check_summary(envelope) != 0) {
                    TRACE(1,"%s provide check sum fail", __func__);
                    dueros_dma_send_check_sum_fail(COMMAND__PROVIDE_SPEECH_ACK, envelope->request_id, param3);
                    break;
                }

                // need BES to configurate open mic .andthen
                dueros_dma_send_prov_speech_ack(envelope->providespeech->dialog->id, envelope->request_id, param3);
                app_ai_voice_upstream_control(true, AIV_USER_DMA, UPSTREAM_INVALID_READ_OFFSET);
                app_ai_voice_update_handle_frame_len(DMA_VOICE_ONESHOT_PROCESS_LEN);
                ai_audio_stream_allowed_to_send_set(true, AI_SPEC_BAIDU);
                app_ai_set_stream_opened(true, AI_SPEC_BAIDU);
                app_ai_set_speech_type(TYPE__PROVIDE_SPEECH, AI_SPEC_BAIDU);
                app_ai_spp_deinit_tx_buf();
                ai_mailbox_put(SIGN_AI_WAKEUP, 0, AI_SPEC_BAIDU, param3);
                TRACE(1,"envelope->providespeech->dialog =%x",envelope->providespeech->dialog->id);
            }
            else
            {
                TRACE(1, "%s This is not foreground ai", __func__);
            }

            break;
        }

        case COMMAND__START_SPEECH_ACK:
        {
            TRACE(1,"COMMAND__START_SPEECH_ACK  %s\n",envelope->request_id);
            ai_audio_stream_allowed_to_send_set(true, AI_SPEC_BAIDU);
            //do nothing
            break;;
        }

        case COMMAND__STOP_SPEECH:
        {
            TRACE(1,"COMMAND__STOP_SPEECH id=%s\n",envelope->request_id);
            app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
            app_ai_voice_upstream_control(false, AIV_USER_DMA, 0);
            dueros_dma_stop_speech_ack(dueros_dma.dialog,envelope->request_id, param3);

            break;
        }
        case COMMAND__STOP_SPEECH_ACK:
        {
            TRACE(1,"COMMAND__STOP_SPEECH_ACK id=%s\n\n",envelope->request_id);

            break;
        }
        case COMMAND__NOTIFY_SPEECH_STATE:
        {
            TRACE(1,"COMMAND__NOTIFY_SPEECH_STATE state=%d\n",envelope->notifyspeechstate->state);
            dma_notify_speech_state(envelope->notifyspeechstate->state, AI_SPEC_BAIDU);
            dueros_dma_send_notify_speech_state_ack(envelope->request_id, param3);

            break;
        }
        case COMMAND__GET_DEVICE_INFORMATION:
        {
            TRACE(1,"COMMAND__GET_DEVICE_INFORMATION id=%s\n",envelope->request_id);
            initiatorTye = dueros_dma_send_get_dev_info_ack(envelope->request_id, param3);
            TRACE(1,"initiatorTye=%x\n",initiatorTye);

            /*?????????????????????????????start*/
            if(initiatorTye == INITIATOR_TYPE__PHONE_WAKEUP || initiatorTye == INITIATOR_TYPE__TAP) {
                break;
            }

            /*?????????????????*/
            ai_function_handle(CALLBACK_START_SPEECH, NULL, 0, AI_SPEC_BAIDU, param3);
            break;
        }
        case COMMAND__GET_DEVICE_CONFIGURATION:
        {
            TRACE(1,"COMMAND__GET_DEVICE_CONFIGURATION %s \n",envelope->request_id);
            dueros_dma_sent_get_dev_conf_ack(envelope->request_id, param3);
            break;
        }

        case COMMAND__NOTIFY_DEVICE_CONFIGURATION_ACK:
        {
            TRACE(1,"COMMAND__NOTIFY_DEVICE_CONFIGURATION_ACK id=%s\n",envelope->request_id);

            //??????????????
            //do nothing
            break;
        }
        case COMMAND__GET_STATE:
        {
            TRACE(2,"COMMAND__GET_STATE id=%s, f %x\n",envelope->request_id, envelope->getstate->feature);
            if(envelope->getstate->feature == 0x02)
            {
                TRACE(1,"dueros_dma.rand %p", dueros_dma.rand);
                if (dueros_dma.rand != NULL) {
                    if (dueros_dma_check_summary(envelope) != 0) {
                        TRACE(1,"%s get_state_check_sum_fail", __func__);
                        dueros_dma_send_get_state_check_sum_fail(COMMAND__GET_STATE_ACK,envelope->request_id, param3);
                        goto _go_exit;
                    }
                } else {
                    dueros_dma_send_get_state_check_sum_fail(COMMAND__GET_STATE_ACK,envelope->request_id, param3);
                    goto _go_exit;
                }
            }

            dueros_dma_send_get_state_ack(envelope->getstate->feature,envelope->request_id, param3);
            //dueros_dma_send_notify_dev_conf();

            break;
        }
        case COMMAND__SET_STATE:
        {
            TRACE(1,"COMMAND__SET_STATE id=%s\n",envelope->request_id);
            if(envelope->setstate->state->feature == 0xf002) {
                if (dueros_dma_check_summary(envelope) != 0) {
                    TRACE(1,"%s set_state_check_sum_fail", __func__);
                    dueros_dma_send_check_sum_fail(COMMAND__SET_STATE_ACK,envelope->request_id, param3);
                    goto _go_exit;
                }
            }

            dueros_dma_send_set_state_ack(envelope->setstate,envelope->request_id, param3);
            break;
        }
        case COMMAND__SYNCHRONIZE_STATE_ACK:
        {   //????????,?????????????
            //do nothing
    //      dueros_dma_send_synchronize_state_ack(envelope->request_id);
            TRACE(1,"COMMAND__SYNCHRONIZE_STATE_ACK id=%s\n",envelope->request_id);
            break;
        }
        case COMMAND__FORWARD_AT_COMMAND:
        {
            TRACE(1,"COMMAND__FORWARD_AT_COMMAND id=%s\n",envelope->request_id);
            dueros_dma_send_at_command_ack(envelope->request_id,envelope->forwardatcommand->command, param3);
            break;
        }
        case COMMAND__END_POINT_SPEECH:
        {
            TRACE(1,"COMMAND__END_POINT_SPEECH id=%s\n",envelope->request_id);
            app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
            app_ai_voice_upstream_control(false, AIV_USER_DMA, 0);
        #ifndef DMA_PUSH_TO_TALK
            dueros_dma_stop_speech(param3);
        #endif
            dueros_dma_send_endpoint_speech_ack(envelope->request_id, param3);
            break;
        }
        case COMMAND__FORWARD_TEST_COMMAND:
        {
            TRACE(1,"COMMAND__FORWARD_TEST_COMMAND id=%s\n",envelope->request_id);
            dueros_dma_forward_test(envelope->request_id,envelope->forwardatcommand->command, param3);
            break;
        }
        case COMMAND__ISSUE_MEDIA_CONTROL:
        {
            TRACE(0,"command  COMMAND__ISSUE_MEDIA_CONTROL");
            dueros_dma_media_control(envelope->request_id,envelope->issue_media_control->control,envelope->issue_media_control->volume, param3);
            break;
        }

        default:
        {

            TRACE(0,"COMMAND__default\n");
            break;
        }
    }

    TRACE(1,"-end_ev_command=%d-\n",envelope->command);

_go_exit:

    dma_control_envelope__free_unpacked(envelope,NULL);

#if defined(USE_RECV_DATA_QUEUE)
    {
        int st = 0;
        st = DeCQueue(&ai_rev_buf_queue, NULL, process_len);
        if (st != CQ_OK) {
            TRACE(3,"%s: DeCQueue fail, queue avail %d, want to dequeue %d", __func__, LengthOfCQueue(&ai_rev_buf_queue), payload_len+2);
            ASSERT(0, "cmd dequeue fail!");
        }
    }

    if (LengthOfCQueue(&ai_rev_buf_queue)) {
        goto process_again;
    }
#endif
    return 0;
}

