#include <stdio.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "string.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_audio.h"
#include "app_status_ind.h"
#include "apps.h"
#include "app_key.h"
#include "med_memory.h"
#include "nvrecord.h"
#include "btapp.h"
#ifndef BLE_V2
#include "gapm_task.h"
#endif
#include "kfifo.h"
#include "app_ble_mode_switch.h"
#include "tgt_hardware.h"

#include "voice_compression.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_dma_bt.h"
#include "app_dma_ble.h"
#include "app_dma_handle.h"
#include "dueros_dma.h"
#include "app_ai_tws.h"


#define DMA_VOICE_XFER_CHUNK_SIZE               (VOB_VOICE_ENCODED_DATA_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)


static char bt_address[18]={0,};
char hex_to_char(int num)
{
    char char_num;
  switch(num)
  {
    case 0:
        char_num = '0';
        break;
    case 1:
        char_num = '1';
        break;
    case 2:
        char_num = '2';
        break;
    case 3:
        char_num = '3';
        break;
    case 4:
        char_num = '4';
        break;
    case 5:
        char_num = '5';
        break;

    case 6:
        char_num = '6';
        break;

    case 7:
        char_num = '7';
        break;

    case 8:
        char_num = '8';
        break;
    case 9:
        char_num = '9';
        break;
    case 0xa:
        char_num = 'a';
        break;
    case 0xb:
        char_num = 'b';
        break;
    case 0xc:
        char_num = 'c';
        break;
    case 0xd:
        char_num = 'd';
        break;

    case 0xe:
        char_num ='e';
        break;
    case 0xf:
        char_num ='f';
        break;
    default:
        char_num = '\0';
  }

    return char_num;
}

void  bt_address_convert_to_char(uint8_t *bt_addr)
{
    int i ,j=0;
    for(i=5 ; i>=0;i--)
    {
        bt_address[ j++] = hex_to_char((bt_addr[i]>>4)&0xf);
        bt_address[ j++] = hex_to_char(bt_addr[i]&0xf);
        bt_address[ j++] = ':';
    }

    bt_address[17] ='\0';
}

void  bt_address_convert_to_char_2(uint8_t *_bt_addr, char *out_str)
{
    int i ,j=0;
    for(i=5 ; i>=0;i--)
    {
        out_str[ j++] = hex_to_char((_bt_addr[i]>>4)&0xf);
        out_str[ j++] = hex_to_char(_bt_addr[i]&0xf);
    }
}


const char* dma_get_key(void)
{
    return DMA_KEY;
}

int dma_get_dev_sn(char *sn)
{
    int ret = 0, valid = 1;
    const char *_pad = "xxxx";
    unsigned char _bt_addr[6+1];
    char _bt_addr_str[12+1];
    unsigned char invalid_sn[BAIDU_DATA_SN_LEN];

    if (sn == NULL)
        return -1;

    ret = nvrec_dev_get_sn(sn);

    // if got data from nvrec, we should check if all 0 or 0xff
    if (ret == 0) {
        memset(invalid_sn, 0xFF, BAIDU_DATA_SN_LEN);
        if (memcmp(sn, invalid_sn, BAIDU_DATA_SN_LEN) == 0) {
            valid = 0;
        }

        if (valid) {
            memset(invalid_sn, 0, BAIDU_DATA_SN_LEN);
            if (memcmp(sn, invalid_sn, BAIDU_DATA_SN_LEN) == 0) {
                valid = 0;
            }
        }
    }

    TRACE(2,"%s : valid %d", __func__, valid);
    // no sn saved or all 0xff to get mac address
    if ((ret != 0) || (valid == 0)) {
        memset(_bt_addr_str, '\0', sizeof(_bt_addr_str));
        if (nvrec_dev_get_btaddr((char *)_bt_addr)) {
            TRACE(0,"nvrec get bt addr ok!");
            bt_address_convert_to_char_2(_bt_addr, _bt_addr_str);
        } else {
            // use default bt addr var as default bt address
            bt_address_convert_to_char_2(bt_addr, _bt_addr_str);
        }

        memcpy(sn, _bt_addr_str, strlen(_bt_addr_str));
        memcpy(sn+12, _pad, 4);
    }

    return 0;
}




Transport dma_transport[]={TRANSPORT__BLUETOOTH_LOW_ENERGY,TRANSPORT__BLUETOOTH_RFCOMM};
AudioFormat dma_audioformat[] = {AUDIO_FORMAT__PCM_L16_16KHZ_MONO,AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS};

char _sn[BAIDU_DATA_SN_LEN+1];

void dma_get_dev_info(DeviceInformation * device_info)
{
    if(device_info == NULL)
    {
        return ;
    }
    memset(_sn, '\0', sizeof(_sn));
    dma_get_dev_sn(_sn);
    TRACE(0,"sn:");
    DUMP8("%02x ", _sn, BAIDU_DATA_SN_LEN);

    device_info->serial_number = _sn;
    device_info->name= DMA_DEVICE_NAME;
    device_info->device_type  = DMA_DEVICE_TYPE;
    device_info->manufacturer =DMA_MANUFACTURER;

#ifdef SOFTWARE_VERSION
    device_info->firmware_version = SOFTWARE_VERSION;
    device_info->software_version = SOFTWARE_VERSION;
#else
    device_info->firmware_version = DMA_FIRMWARE_VERSION;
    device_info->software_version = DMA_SOFTWARE_VERSION;
#endif

#ifdef DMA_PUSH_TO_TALK
    device_info->initiator_type  = INITIATOR_TYPE__TAP;
#else
    device_info->initiator_type  = INITIATOR_TYPE__PHONE_WAKEUP;
#endif
    device_info->product_id    = DMA_PRODUCT_ID;
    device_info->n_supported_transports =( sizeof(dma_transport)/sizeof(Transport));
    device_info->supported_transports = dma_transport;
    device_info->n_supported_audio_formats = (sizeof(dma_audioformat)/sizeof(dma_audioformat));
    device_info->supported_audio_formats = dma_audioformat;

    bt_address_convert_to_char(bt_addr);
    device_info->classic_bluetooth_mac = bt_address;
    TRACE(1,"BT_ADDRES:%s",bt_address);
}


void dma_get_dev_config(DeviceConfiguration *device_config)
{
    if(device_config == NULL)
    {
        return ;
    }

    device_config->needs_assistant_override  = FALSE;
    device_config->needs_setup  = FALSE;
}

/*add by zhangjiajie 2018-8-15*/
int gloable_ota_set_flag = 0;

static void ota_disconnect_handler(void const *param);
osTimerDef(OTA_DISCONNECT_HAND, ota_disconnect_handler);
static osTimerId ota_disconnect_timer = NULL;
static void ota_disconnect_handler(void const *param)
{
    TRACE(0,"=====timer enter ota mode========");
    //app_otaMode_enter(NULL, NULL);
}

void ota_enter_check() {
    if(1 == gloable_ota_set_flag) {
        TRACE(0,"=====disconnect enter ota mode========");
        //app_otaMode_enter(NULL, NULL);
    }
}

void dma_set_state(State *p_state)
{
    if(p_state == NULL)
    {
        return;
    }
    switch(p_state->feature)
    {
        case 0xf002:
        {
            gloable_ota_set_flag = 1;
            if (NULL == ota_disconnect_timer)
            {
                ota_disconnect_timer = osTimerCreate (osTimer(OTA_DISCONNECT_HAND), osTimerOnce, NULL);
            }
            osTimerStart(ota_disconnect_timer, 3000);

            break;
        }

        case 0xf003:
        {
            #if 0
            TRACE(3,"=====enter set freq======== %d, min %d, max %d", p_state->integer, FM_MIN_FREQ, FM_MAX_FREQ);
            if (p_state->integer >= FM_MIN_FREQ && p_state->integer <= FM_MAX_FREQ) {
                qn8027_set_freq(p_state->integer);
                nvrec_set_fm_freq(p_state->integer);
            }
            else {
                // TODO, refuse this freq should tell phone
                TRACE(1,"ignore this freq %d", p_state->integer);
            }
            #endif
            break;
        }

    }
}


enum  TEST_COMMAND{
    COMMAND_A2DP_PLAY= 0,
    COMMAND_A2DP_PAUSE,
    COMMAND_VOLUME_UP,
    COMMAND_VOLUME_DOWN,
    COMMAND_FORWARD,
    COMMAND_BACKWARD,
    COMMAND_HFP_ANSWER,
    COMMAND_HFP_REJECT,
    COMMAND_HFP_CANCEL,
    COMMAND_HFP_SWITCH,
    COMMAND_HFP_REDIAL,
    COMMAND_RECONN_AVDTP,
    COMMAND_HFP_DIAl,
};

#define  A2DP_PLAY   "play"
#define  A2DP_PAUSE   "pause"
#define  VOLUME_UP   "up"
#define  VOLUME_DOWN  "down"
#define  FORWARD      "forward"
#define  BACKWARD     "backward"

#define  HFP_ANSWER  "answer"
#define  HFP_REJECT  "reject"
#define  HFP_CANCEL  "cancel"
#define  HFP_SWITCH   "switch"
#define  HFP_REDIAL  "redial"

#define  REC_AVDTP  "re_avdtp"
#define  HFP_DIAL  "dial"

int parse_test_command(char *command)
{
    TRACE(1,"==test command====%s=",command);
    if(command == NULL)
    {
        return -1;
    }


    if(strcmp(command,A2DP_PLAY) ==0 )
    {
        return  COMMAND_A2DP_PLAY;

    }
    else if(strcmp(command,A2DP_PAUSE) == 0)
    {
        return COMMAND_A2DP_PAUSE;


    }
    else if(strcmp(command,VOLUME_UP) == 0)
    {
        return  COMMAND_VOLUME_UP;


    }
    else if(strcmp(command,VOLUME_DOWN) == 0)
    {
        return  COMMAND_VOLUME_DOWN;


    }
    else if(strcmp(command,FORWARD) == 0)
    {
        return  COMMAND_FORWARD;


    }
    else if(strcmp(command,BACKWARD) == 0)
    {
        return  COMMAND_BACKWARD;


    }
    else if(strcmp(command,HFP_ANSWER) == 0)
    {
        return  COMMAND_HFP_ANSWER;


    }
    else if(strcmp(command,HFP_REJECT) == 0)
    {
        return  COMMAND_HFP_REJECT;


    }
    else if(strcmp(command,HFP_CANCEL) == 0)
    {
        return  COMMAND_HFP_CANCEL;


    }
    else if(strcmp(command,HFP_SWITCH) == 0)
    {
        return  COMMAND_HFP_SWITCH;

    }else if(strcmp(command,HFP_REDIAL) == 0)
    {
        return  COMMAND_HFP_REDIAL;

    }
    else if(strcmp(command,REC_AVDTP) == 0)
    {
        return  COMMAND_RECONN_AVDTP;

    }
    else if(strcmp(command,HFP_DIAL) == 0)
    {
        return  COMMAND_HFP_DIAl;

    }
    else
    {
        return -1;
    }

    //todo
}

#if defined(BQB_TEST)
void A2dp_pts_Create_Avdtp_Signal_Channel(APP_KEY_STATUS *status, void *param);
#endif

void bt_key_handle_up_key(enum APP_KEY_EVENT_T event);
void bt_key_handle_down_key(enum APP_KEY_EVENT_T event);
void bqb_test_command(char* command)
{
    static char bt_call = 1;
    int int_command ;
    int_command = parse_test_command(command);
    switch(int_command)
    {
        case COMMAND_A2DP_PLAY:
        {
            a2dp_handleKey(AVRCP_KEY_PLAY);
            TRACE(0,"command  COMMAND_A2DP_PLAY");
            break;
        }
        case COMMAND_A2DP_PAUSE:
        {
            TRACE(0,"command  COMMAND_A2DP_PAUSE");
            a2dp_handleKey(AVRCP_KEY_PAUSE);
            break;
        }

        case COMMAND_VOLUME_UP:
        {
            TRACE(0,"command  COMMAND_VOLUME_UP");
            //a2dp_handleKey(AVRCP_KEY_VOLUME_UP);
            bt_key_handle_up_key(APP_KEY_EVENT_UP);
            break;
        }

        case COMMAND_VOLUME_DOWN:
        {
            TRACE(0,"command  COMMAND_VOLUME_DOWN");
            //a2dp_handleKey(AVRCP_KEY_VOLUME_DOWN);
            bt_key_handle_down_key(APP_KEY_EVENT_UP);
            break;
        }

        case COMMAND_FORWARD:
        {
            TRACE(0,"command  COMMAND_FORWARD");
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        }

        case COMMAND_BACKWARD:
        {
            TRACE(0,"command  COMMAND_BACKWORD");
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        }
        case COMMAND_HFP_ANSWER:
        {
            TRACE(0,"command  COMMAND_HFP_ANSWER");
            hfp_handle_key(HFP_KEY_ANSWER_CALL);
            break;
        }
        case COMMAND_HFP_REJECT:
        {
            TRACE(0,"command  COMMAND_HFP_REJECT");
            hfp_handle_key(HFP_KEY_HANGUP_CALL);
            break;
        }
        case COMMAND_HFP_CANCEL:
        {
            TRACE(0,"command  COMMAND_HFP_CANCEL");
            hfp_handle_key(HFP_KEY_HANGUP_CALL);
            break;
        }
        case COMMAND_HFP_REDIAL:
        {
            TRACE(0,"command  COMMAND_HFP_REDIAL");
            hfp_handle_key(HFP_KEY_REDIAL_LAST_CALL);
            break;
        }
        case COMMAND_RECONN_AVDTP:
        {
            TRACE(0,"command  COMMAND_RECONN_AVDTP");
#if defined(BQB_TEST)
            A2dp_pts_Create_Avdtp_Signal_Channel(0, 0);
#else
            TRACE(0,"command  COMMAND_RECONN_AVDTP, please open BQB_TEST macro!!!!!");
#endif
            break;
        }
        case COMMAND_HFP_DIAl:
        {
            TRACE(0,"command  COMMAND_DIAL 654321");
            //app_hfp_dial_number("654321", 6);
            break;
        }

        case COMMAND_HFP_SWITCH:
        {
            TRACE(0,"command  COMMAND_HFP_SWITCH");
            if (bt_call == 1) {
                hfp_handle_key(HFP_KEY_CHANGE_TO_PHONE);
                bt_call = 0;
            }
            else if (bt_call == 0) {
                hfp_handle_key(HFP_KEY_ADD_TO_EARPHONE);
                bt_call = 1;
            }
            break;
        }
        default:
        {
            TRACE(0,"no support command");
            break;
        }
    }

    //todo
}

extern void a2dp_handleKey(uint8_t a2dp_key);
void dma_media_control(MediaControl control, uint32_t volume)
{
    TRACE(2,"%s control %d", __func__, control);

    switch(control)
    {
        case MEDIA_CONTROL__PLAY:
        {
            a2dp_handleKey(AVRCP_KEY_PLAY);
            TRACE(0,"command  MEDIA_CONTROL_PLAY");
            break;
        }
        case MEDIA_CONTROL__PAUSE:
        {
            TRACE(0,"command  MEDIA_CONTROL_PAUSE");
            a2dp_handleKey(AVRCP_KEY_PAUSE);
            break;
        }

        case MEDIA_CONTROL__PREVIOUS:
        {
            TRACE(0,"command  MEDIA_CONTROL_PREVIOUS");
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        }

        case MEDIA_CONTROL__NEXT:
        {
            TRACE(0,"command  MEDIA_CONTROL_NEXT");
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        }

        case MEDIA_CONTROL__ABSOLUTE_VOLUME:
        {
            TRACE(0,"command  MEDIA_CONTROL_ABSOLUTE_VOLUME");
            //set the absolute volume
            break;
        }

        default:
        {
            TRACE(0,"no support command");
            break;
        }
    }
}

void dma_get_setting(SpeechSettings *setting)
{
    if(setting == NULL)
    {
        return ;
    }

    setting->audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
    setting->audio_source = AUDIO_SOURCE__STREAM;
    setting->audio_profile = AUDIO_PROFILE__FAR_FIELD;
}

void dma_notify_speech_state(SpeechState speech_state, uint8_t ai_index)
{
    switch(speech_state)
    {
        case SPEECH_STATE__IDLE:
        {
            if(dueros_dma_get_speech_state() == SPEECH_STATE__LISTENING) {
                /*here  ASR end*/
            }
            break;
        }
        case SPEECH_STATE__LISTENING:
        {
            /*here ASR start*/
            dueros_dma_set_speech_state(SPEECH_STATE__LISTENING);
            break;
        }
        case SPEECH_STATE__PROCESSING:
        {

            if(dueros_dma_get_speech_state() == SPEECH_STATE__LISTENING) {
                /*here  ASR end*/
                dueros_dma_set_speech_state(SPEECH_STATE__PROCESSING);
                app_ai_voice_upstream_control(false, AIV_USER_DMA, 0);
            }
            break;
        }
        case SPEECH_STATE__SPEAKING:
        {
            /*not care*/
            dueros_dma_set_speech_state(SPEECH_STATE__SPEAKING);
            break;
        }
        default:
        {
            TRACE(0,"no support command");
            break;
        }
    }
}

static  volatile bool a2dp_connected = false;
volatile char dma_is_voice_call_on = 0;

void dma_a2dp_connected()
{
    TRACE(0,"-----A2DP IS CONNECTEC");
    a2dp_connected = true;
    dueros_dma_send_sync_state(0x132, 0);
}


void dma_a2dp_disconnected()
{
    TRACE(0,"-----A2DP IS DISCONNECTEC");
    a2dp_connected = false;
    dueros_dma_send_sync_state(0x132, 0);
}




void dma_sco_audio_connected(void)
{
    TRACE(1,"%s", __func__);
    dma_is_voice_call_on = 1;
#ifdef CLOSE_BLE_ADV_WHEN_VOICE_CALL
    if (app_ai_get_transport_type(AI_SPEC_BAIDU, ai_manager_get_foreground_ai_conidx()) != AI_TRANSPORT_SPP) {
        //app_ble_stop_activities();
    }
#endif
}

void dma_sco_audio_disconnected(void)
{
    TRACE(1,"%s", __func__);
    dma_is_voice_call_on = 0;
#ifdef CLOSE_BLE_ADV_WHEN_VOICE_CALL
    app_ai_if_ble_set_adv(AI_BLE_ADVERTISING_INTERVAL);
#endif
}

int dma_api_is_voicecall_on(void)
{
    return dma_is_voice_call_on;
}


bool get_a2dp_state()
{
    return  a2dp_connected;
}

void dma_pair_state(enum dma_pair_state state)
{
    // bt to pair state
    if (state == DMA_PAIR_PAIR) {
    }
    // bt to unpair state
    else if (state == DMA_PAIR_UNPAIR) {
    }
}

void dma_get_state(uint32_t profile ,State *p_state)
{
    switch(profile)
    {
        case 0x1:
        {
            p_state->feature = 1;
            p_state->value_case =STATE__VALUE_BOOLEAN;
            p_state->boolean =  true;
            break;

        }
        case 0xf001:
        {
            p_state->feature = 0xf001;
            p_state->value_case =STATE__VALUE_BOOLEAN;
            //p_state->boolean =    !get_phone_exist_pin_value();
            break;
        }

        case 0xf003:
        {
            p_state->feature = 0xf003;
            p_state->value_case =STATE__VALUE_INTEGER;
            //p_state->integer  =  qn8027_get_freq();
            TRACE(1,"------------------------------%d",p_state->integer);
            break;
        }

        case 0x132 :
        {
            p_state->feature = 0x132;
            p_state->value_case =STATE__VALUE_BOOLEAN;
            //p_state->boolean  = get_a2dp_state();
            p_state->boolean  = true;//if a2dp not connected,ble also can connect
            TRACE(1,"00000000000000000000000000p_state->boolean = %d",p_state->boolean);
            break;
        }
        // always
        case 0x2 :
        {
            p_state->feature = 0x2;
            p_state->value_case =STATE__VALUE_BOOLEAN;
            p_state->boolean  = true;
            TRACE(1,"00000000000000000000000000p_state->boolean = %d",p_state->boolean);
            break;
        }
        default:
        {
            TRACE(2,"%s:unimplemented profile 0x%x", __func__, profile);
            break;
        }
    }

}

#include "hal_timer.h"

#define LOCAL_TRACE(str, ...)\
    TRACE(1,"%d ms", TICKS_TO_MS(hal_sys_timer_get())); \
    TRACE(str, ##__VA_ARGS__)

#undef LOCAL_TRACE
#define LOCAL_TRACE(str,...)





#define USE_TX_BUFF 1
#define ASYNC_CALL_BLE_FUNC 1

#ifdef USE_TX_BUFF
#define TXBUFF_BLE_DEF_SAFE_MTU (20)
#define TXBUFF_BLE_DEF_MTU (128)
#define TXBUFF_SPP_DEF_MTU (404)


volatile char dma_txbuff_send_max_size_neged = 0;
#endif


#ifdef BAIDU_RFCOMM_DIRECT_CONN
#define APP_BAIDU_VEND_ADV_DATA_HEAD "\x0b\xFF\xB0\x02"
#define APP_BAIDU_VEND_ADV_DATA_HEAD_LEN (4)

#define APP_BAIDU_VEND_ADV_DATA_TAIL "\xFE\x04"
#define APP_BAIDU_VEND_ADV_DATA_TAIL_LEN (2)

#define APP_SCNRSP_DATA           "\x03\x03\x04\xFE"
#define APP_SCNRSP_DATA_LEN     (4)

static uint32_t app_dma_start_advertising(void *param1, uint32_t param2)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T *)param1;
    char _bt_addr[6] = {0};
    unsigned char temp = 0;
    unsigned char null_addr[6] = {0};
    uint8_t *_local_bt_addr = NULL;

    if (app_ai_dont_have_mobile_connect(AI_SPEC_BAIDU))
    {
        TRACE(1,"%s no mobile connect", __func__);
        return 0;
    }

    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
    cmd->scanRspDataLen += APP_SCNRSP_DATA_LEN;

    if (app_ai_is_in_tws_mode(0))
    {
        _local_bt_addr = app_ai_tws_local_address();
        if (memcmp(_local_bt_addr, null_addr, 6))
        {
            memcpy((void *)_bt_addr, (void *)_local_bt_addr, 6);
        }
    }

    if (!memcmp(_bt_addr, null_addr, 6))
    {
        TRACE(1, "%s get nvrec addr", __func__);
        if (nvrec_dev_get_btaddr(_bt_addr) == false) {
            // if get bt addr from nv false, we use default bt addr var as addr
            memcpy((void *)_bt_addr, (void *)bt_addr, 6);
        }
    }

    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           APP_BAIDU_VEND_ADV_DATA_HEAD, APP_BAIDU_VEND_ADV_DATA_HEAD_LEN);
    cmd->scanRspDataLen += APP_BAIDU_VEND_ADV_DATA_HEAD_LEN;
    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           APP_BAIDU_VEND_ADV_DATA_TAIL, APP_BAIDU_VEND_ADV_DATA_TAIL_LEN);
    cmd->scanRspDataLen += APP_BAIDU_VEND_ADV_DATA_TAIL_LEN;
    temp = _bt_addr[0];
    _bt_addr[0] = _bt_addr[5];
    _bt_addr[5] = temp;

    temp = _bt_addr[1];
    _bt_addr[1] = _bt_addr[4];
    _bt_addr[4] = temp;

    temp = _bt_addr[2];
    _bt_addr[2] = _bt_addr[3];
    _bt_addr[3] = temp;
    memcpy(&cmd->scanRspData[cmd->scanRspDataLen],
           _bt_addr, 6);
    cmd->scanRspDataLen += 6;

    return 1;
}
#endif

uint32_t app_dma_config_mtu(void *param1, uint32_t param2)
{
#ifdef FLOW_CONTROL_ON_UPLEVEL
    if (param2 < DUEROS_DMA_HEAD_LEN) {
        ASSERT(0, "%s : mtu_size %d < DUEROS_DMA_HEAD_LEN %d", __func__, param2, DUEROS_DMA_HEAD_LEN);
    }
    TRACE(2, "%s %d", __func__, param2);
    app_ai_set_mtu(param2, AI_SPEC_BAIDU);
#endif
    return 0;
}

uint32_t app_dma_voice_connected(void *param1, uint32_t param2, uint8_t param3);
uint32_t app_dma_voice_disconnected(void *param1, uint32_t param2, uint8_t param3);
void dma_ble_ccc_changed(unsigned char value)
{
    LOCAL_TRACE(2,"%s:%d\n", __func__, __LINE__);

    if (value) {
        app_dma_voice_connected(NULL, AI_TRANSPORT_BLE, AI_SPEC_BAIDU);
        app_dma_config_mtu(NULL, TXBUFF_SPP_DEF_MTU);
    }
    else {
        app_dma_voice_disconnected(NULL, AI_TRANSPORT_BLE, AI_SPEC_BAIDU);
    }

    return;
}

void dma_ble_mtu_changed(unsigned int mtu_size)
{
    TRACE(2,"%s mtu_size %d\n", __func__, mtu_size);

    app_dma_config_mtu(NULL, mtu_size);

}

uint32_t app_dma_rcv_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s len=%d",__func__, param2);
    //DUMP8("%02x ",pBuf,length);

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return -1;
        }
    }

    if (param2 <= (DUEROS_DMA_HEADER_SIZE + 1))
    {
        TRACE(1,"%s data_len not enough", __func__);
        return 2;
    }
    else
    {
        if (EnCQueue(&ai_rev_buf_queue, (CQItemType *)param1, param2) == CQ_ERR)
        {
            ASSERT(false, "%s avail of queue %d cmd len %d", __func__, AvailableOfCQueue(&ai_rev_buf_queue), param2);
        }
        ai_mailbox_put(SIGN_AI_CMD_COME, 0, AI_SPEC_BAIDU, param3);
    }
    return 0;
}

uint32_t app_dma_voice_connected(void *param1, uint32_t param2, uint8_t param3)
{
    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1;
    uint8_t dest_id = (uint8_t)param3;
    uint8_t ai_connect_index = app_ai_connect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_BAIDU, dest_id, btaddr->address);

    TRACE(2,"%s ai_connect_index = %d", __func__, ai_connect_index);

    if(param2 == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send, AI_SPEC_BAIDU, AI_TRANSPORT_SPP);
        ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_BAIDU, AI_TRANSPORT_SPP);
        app_ai_set_mtu(TX_SPP_MTU_SIZE, AI_SPEC_BAIDU);
        app_ai_set_data_trans_size(320, AI_SPEC_BAIDU);
    }
#ifdef __AI_VOICE_BLE_ENABLE__
    else if(param2 == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_dma_send_data_via_notification, AI_SPEC_BAIDU, AI_TRANSPORT_BLE);
        ai_cmd_transport_init(app_dma_send_data_via_notification, AI_SPEC_BAIDU, AI_TRANSPORT_BLE);
        app_ai_set_data_trans_size(160, AI_SPEC_BAIDU);
    }
#endif

    dueros_dma_connected();
    dueros_dma_sent_ver(param3);
    AIV_HANDLER_BUNDLE_T handlerBundle = {
        .upstreamDataHandler    = app_ai_voice_default_upstream_data_handler,
         };
    app_ai_voice_register_ai_handler(AIV_USER_DMA,&handlerBundle);
    app_ai_setup_complete_handle(AI_SPEC_BAIDU);

    TRACE(2,"%s txCredit=%d", __func__, ai_struct[AI_SPEC_BAIDU].tx_credit);
    return 0;
}

uint32_t app_dma_voice_disconnected(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s", __func__);

    app_ai_disconnect_handle((AI_TRANS_TYPE_E)param2, AI_SPEC_BAIDU);

    if(!app_ai_spec_have_other_connection(AI_SPEC_BAIDU))
    {
        (void)dueros_dma_disconnected();
    }
    //app_ir_close();
    ota_enter_check();
    return 0;
}


uint32_t app_dma_start_speech(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s stream open %d", __func__, app_ai_is_stream_opened(AI_SPEC_BAIDU));

    if(app_ai_is_stream_opened(AI_SPEC_BAIDU)){
        return 1;
    }else{
        app_ai_set_stream_opened(true, AI_SPEC_BAIDU);
        app_ai_voice_upstream_control(true, AIV_USER_DMA, UPSTREAM_INVALID_READ_OFFSET);
        /// update the read offset for AI voice module
        app_ai_voice_update_handle_frame_len(DMA_VOICE_ONESHOT_PROCESS_LEN);
        app_ai_voice_stream_init(AIV_USER_DMA);
        app_ai_spp_deinit_tx_buf();
        dueros_dma_start_speech(param3);
    }

    return 0;
}

uint32_t app_dma_stop_speech(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s",__func__);

    app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
    app_ai_voice_upstream_control(false, AIV_USER_DMA, 0);
    app_ai_voice_stream_control(false,AIV_USER_DMA);
    dueros_dma_stop_speech(param3);

    return 0;
}

#define DMA_TRANSFER_DATA_MAX_LEN 400
uint32_t app_dma_stream_init(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}

uint32_t app_dma_stream_deinit(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}

uint32_t app_dma_voice_send_done(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s credit %d", __func__, ai_struct[AI_SPEC_BAIDU].tx_credit);
    if(ai_struct[AI_SPEC_BAIDU].tx_credit < MAX_TX_CREDIT) {
        ai_struct[AI_SPEC_BAIDU].tx_credit++;
    }

    if ((app_ai_get_data_trans_size(AI_SPEC_BAIDU) <= voice_compression_get_encode_buf_size()) ||
        ai_transport_has_cmd_to_send())
    {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_BAIDU), AI_SPEC_BAIDU, param3);
    }
    return 0;
}


uint32_t app_dma_send_voice_stream(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_BAIDU, ai_manager_get_foreground_ai_conidx());

    if(!is_ai_audio_stream_allowed_to_send(AI_SPEC_BAIDU)){
        //voice_compression_get_encoded_data(NULL ,encodedDataLength);
        TRACE(2,"%s dma don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }

    //if (encodedDataLength < DMA_VOICE_XFER_CHUNK_SIZE){
        //TR_DEBUG(2,"%s encodedDataLength too short %d", __func__, encodedDataLength);
        //return 2;
    //}

    if(transport_type == AI_TRANSPORT_SPP || transport_type == AI_TRANSPORT_BLE) {
        if (encodedDataLength < app_ai_get_data_trans_size(AI_SPEC_BAIDU)) {
            return 3;
        }
    } else {
        TRACE(1,"%s dma transport_type is error", __func__);
        ai_audio_stream_allowed_to_send_set(false, AI_SPEC_BAIDU);
        return 4;
    }

    TRACE(3,"%s encodedDataLength %d credit %d", __func__, encodedDataLength, ai_struct[AI_SPEC_BAIDU].tx_credit);

    if(0 == ai_struct[AI_SPEC_BAIDU].tx_credit) {
        TRACE(1,"%s no txCredit", __func__);
        return 5;
    }

    ai_mailbox_put(SIGN_AI_TRANSPORT, encodedDataLength, AI_SPEC_BAIDU, param3);

    return 0;
}

uint32_t app_dma_enable_handler(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s %d", __func__, param2);
    return 0;
}

uint32_t app_dma_init(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s", __func__);

    dma_is_voice_call_on = 0;

    PROTOCOL_VER ver = \
    {               \
     .magic = DUEROS_DMA_MAGIC,\
     .hVersion = 1,\
     .lVersion = 1, \
     .reserve = {0,0,0,0,}, \
     };

    memset(dueros_dma.rand, '\0', sizeof(dueros_dma.rand));

    nvrec_get_rand(dueros_dma.rand);

    dueros_dma_init(&ver, SIGN_METHOD__SHA256);

    return 0;
}


int app_dueros_dma_deinit(void)
{
    return DMA_NO_ERROR;
}


uint32_t app_dma_wake_up(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s ai_setup_complete %d",__func__, app_ai_is_setup_complete(AI_SPEC_BAIDU));

    if (app_ai_is_sco_mode())
    {
        TRACE(1,"%s is_sco_mode", __func__);
        return 1;
    }

    if(!app_ai_is_setup_complete(AI_SPEC_BAIDU) ||
        (app_ai_get_transport_type(AI_SPEC_BAIDU, ai_manager_get_foreground_ai_conidx()) == AI_TRANSPORT_IDLE))
    {
        ai_set_can_wake_up(true, AI_SPEC_BAIDU);
        TRACE(1,"%s NOT CONNECTED!!!", __func__);
        return 1;
    }

    if (is_ai_can_wake_up(AI_SPEC_BAIDU)) {
        TRACE(0,"DMA START SPEECH!!!");
        app_dma_start_speech(NULL, 0, param3);
        ai_audio_stream_allowed_to_send_set(true, AI_SPEC_BAIDU);
        ai_mailbox_put(SIGN_AI_WAKEUP, 0, AI_SPEC_BAIDU, param3);
        ai_set_can_wake_up(false, AI_SPEC_BAIDU);
    }
    else {
        app_dma_stop_speech(NULL, 0, param3);
    }
    return 0;
}

uint32_t app_dma_key_event_handle(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

    TRACE(3,"%s code 0x%x event %d", __func__, status->code, status->event);

    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return 1;
    }

    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_BAIDU))
    {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event)
        {
            app_dma_wake_up(status, 0, param3);
        }
        else if (APP_KEY_EVENT_UP == status->event)
        {
            app_dma_stop_speech(status, 0, param3);
        }
    }
    else
    {
        if (APP_KEY_EVENT_CLICK == status->event)
        {
            app_dma_wake_up(status, 0, param3);
        }
    }

    return 0;
}

uint32_t app_dma_audio_stream_handle(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t send_len = 0;
    send_len = dma_audio_stream_send(param1, param2);
    return send_len;
}

/// DMA handlers function definition
const struct ai_func_handler dma_handler_function_list[] =
{
    {API_HANDLE_INIT,           (ai_handler_func_t)app_dma_init},
    {API_START_ADV,             (ai_handler_func_t)app_dma_start_advertising},
    {API_DATA_SEND,             (ai_handler_func_t)app_dma_send_voice_stream},
    {API_DATA_INIT,             (ai_handler_func_t)app_dma_stream_init},
    {API_DATA_DEINIT,           (ai_handler_func_t)app_dma_stream_deinit},
    {CALLBACK_CMD_RECEIVE,      (ai_handler_func_t)app_dma_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,     (ai_handler_func_t)app_dma_rcv_cmd},
    {CALLBACK_DATA_PARSE,       (ai_handler_func_t)app_dma_parse_cmd},
    {CALLBACK_AI_CONNECT,       (ai_handler_func_t)app_dma_voice_connected},
    {CALLBACK_AI_DISCONNECT,    (ai_handler_func_t)app_dma_voice_disconnected},
    {CALLBACK_UPDATE_MTU,       (ai_handler_func_t)app_dma_config_mtu},
    {CALLBACK_WAKE_UP,          (ai_handler_func_t)app_dma_wake_up},
    {CALLBACK_AI_ENABLE,        (ai_handler_func_t)app_dma_enable_handler},
    {CALLBACK_START_SPEECH,     (ai_handler_func_t)app_dma_start_speech},
    {CALLBACK_ENDPOINT_SPEECH,  (ai_handler_func_t)app_dma_stop_speech},
    {CALLBACK_STOP_SPEECH,      (ai_handler_func_t)app_dma_stop_speech},
    {CALLBACK_DATA_SEND_DONE,   (ai_handler_func_t)app_dma_voice_send_done},
    {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)app_dma_key_event_handle},
    {API_DATA_HANDLE,           (ai_handler_func_t)app_dma_audio_stream_handle},
};

const struct ai_handler_func_table dma_handler_function_table =
    {&dma_handler_function_list[0], (sizeof(dma_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_BAIDU, &dma_handler_function_table)


