#include "cmsis_os.h"
#include "hal_bootmode.h"
#include "hal_trace.h"
#include "nvrecord_env.h"
#include "spp_api.h"
#include "app_status_ind.h"
#include "apps.h"
#include "app_ai_ble.h"
#include "app_ai_tws.h"
#include "app_dip.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "app_ai_voice.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_manager_api.h"

#ifdef __IAG_BLE_INCLUDE__
#include "app_ai_ble.h"
#endif

extern "C" uint8_t is_sco_mode (void);

#define CASE_S(s) case s:return "["#s"]";
#define CASE_D()  default:return "[INVALID]";

const char *mobile_connect_type2str(MOBILE_CONN_TYPE_E connect_type)
{
    switch(connect_type) {
        CASE_S(MOBILE_CONNECT_IDLE)
        CASE_S(MOBILE_CONNECT_IOS)
        CASE_S(MOBILE_CONNECT_ANDROID)
        CASE_D()
    }
}

const char *ai_trans_type2str(AI_TRANS_TYPE_E trans_type)
{
    switch(trans_type) {
        CASE_S(AI_TRANSPORT_IDLE)
        CASE_S(AI_TRANSPORT_BLE)
        CASE_S(AI_TRANSPORT_SPP)
        CASE_D()
    }
}

const char *ai_speech_state2str(APP_AI_SPEECH_STATE_E speech_state)
{
    switch(speech_state) {
        CASE_S(AI_SPEECH_STATE__IDLE)
        CASE_S(AI_SPEECH_STATE__LISTENING)
        CASE_S(AI_SPEECH_STATE__PROCESSING)
        CASE_S(AI_SPEECH_STATE__SPEAKING)
        CASE_D()
    }
}

const char *ai_speech_type2str(AI_SPEECH_TYPE_E speech_state)
{
    switch(speech_state) {
        CASE_S(TYPE__NORMAL_SPEECH)
        CASE_S(TYPE__PROVIDE_SPEECH)
        CASE_D()
    }
}

static AI_UI_GLOBAL_HANDLER_IND ai_ui_global_handler_ind = NULL;
void app_ai_register_ui_global_handler_ind(AI_UI_GLOBAL_HANDLER_IND handler)
{
    ai_ui_global_handler_ind = handler;
}
void app_ai_ui_global_handler_ind(uint32_t code_id, void *param1, uint32_t param2, uint8_t ai_index, uint8_t dest_id)
{
    if (ai_ui_global_handler_ind)
    {
        ai_ui_global_handler_ind(code_id, param1, param2, ai_index, dest_id);
    }
}

static void app_ai_voice_mic_timeout_timer_cb(void const *n);
osTimerDef (APP_AI_VOICE_MIC_TIMEOUT, app_ai_voice_mic_timeout_timer_cb);
osTimerId   app_ai_voice_timeout_timer_id = NULL;

static void app_ai_voice_mic_timeout_timer_cb(void const *n)
{
    TRACE(1,"%s", __func__);
    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(NULL == ai_info) {
        return;
    }
    uint8_t dest_id = app_ai_get_dest_id(ai_info);
    ai_function_handle(CALLBACK_ENDPOINT_SPEECH, NULL, 0, ai_info->ai_spec, dest_id);
}

AI_struct ai_struct[MUTLI_AI_NUM] = {0};

APP_AI_TRANSPORT_HANDLER ai_trans_handler[MUTLI_AI_NUM];

AI_connect_info ai_connect_info[AI_CONNECT_NUM_MAX] = {0};

void app_ai_connect_info_init(void)
{
    uint8_t ai_connect_index = 0;

    TRACE(2, "%s", __func__);
    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        ai_connect_info[ai_connect_index].ai_connect_state = AI_IS_DISCONNECT;
        ai_connect_info[ai_connect_index].ai_connect_type = AI_TRANSPORT_IDLE;
        ai_connect_info[ai_connect_index].ai_spec = AI_SPEC_INIT;
        ai_connect_info[ai_connect_index].device_id = 0xFF;
        ai_connect_info[ai_connect_index].conidx = 0xFF;
        memset(ai_connect_info[ai_connect_index].addr, 0, 6);
    }
}

uint8_t app_ai_set_ble_connect_info(uint8_t type, uint8_t aiSpec, uint8_t conidx)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if(ai_connect_info[ai_connect_index].ai_connect_type == AI_TRANSPORT_IDLE)
        {
            ai_connect_info[ai_connect_index].ai_connect_state = AI_IS_CONNECTED;
            ai_connect_info[ai_connect_index].ai_connect_type = type;
            ai_connect_info[ai_connect_index].ai_spec = aiSpec;
            ai_connect_info[ai_connect_index].conidx = conidx;
            break;
        }
    }

    return ai_connect_index;
}

void app_ai_update_connect_state(uint8_t state, uint8_t ai_connect_index)
{
    TRACE(3, "%s conIdx:%d, state:%d", __func__, ai_connect_index, state);
    ai_connect_info[ai_connect_index].ai_connect_state = state;
}

uint8_t app_ai_get_connect_index_from_device_id(uint8_t device_id, uint8_t ai_spec)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if(ai_connect_info[ai_connect_index].device_id == device_id && ai_connect_info[ai_connect_index].ai_spec == ai_spec)
        {
            if(ai_connect_info[ai_connect_index].ai_connect_type == AI_TRANSPORT_SPP)
            {
                return ai_connect_index;
            }
        }
    }

    return AI_CONNECT_INVALID;
}

uint8_t app_ai_get_connect_index_from_ai_spec( uint8_t ai_spec)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if(ai_connect_info[ai_connect_index].ai_spec == ai_spec)
        {
            return ai_connect_index;
        }
    }

    return AI_CONNECT_INVALID;
}

uint8_t app_ai_get_connect_index_from_ble_conidx(uint8_t conidx, uint8_t ai_spec)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if(ai_connect_info[ai_connect_index].conidx == conidx && ai_connect_info[ai_connect_index].ai_spec == ai_spec)
        {
            if(ai_connect_info[ai_connect_index].ai_connect_type == AI_TRANSPORT_BLE)
            {
                TRACE(2, "%s %d", __func__, ai_connect_index);
                return ai_connect_index;
            }
        }
    }

    return 0;
}

uint8_t app_ai_get_connect_state_index(uint8_t state)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if(ai_connect_info[ai_connect_index].ai_connect_state == state)
        {
            return ai_connect_index;
        }
    }

    return 0;
}

uint8_t app_ai_set_spp_connect_info(uint8_t *_addr, uint8_t device_id, uint8_t type, uint8_t AiSpec)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if ((ai_connect_info[ai_connect_index].ai_spec) == AI_SPEC_INIT)
        {
            ai_connect_info[ai_connect_index].ai_connect_state = AI_IS_CONNECTED;
            ai_connect_info[ai_connect_index].ai_connect_type = type;
            ai_connect_info[ai_connect_index].ai_spec = AiSpec;
            ai_connect_info[ai_connect_index].device_id = device_id;
            TRACE(3, "%s connIdx:%d, ai:%d", __func__, ai_connect_index, AiSpec);
            if (_addr)
            {
                memcpy(ai_connect_info[ai_connect_index].addr, _addr, 6);
                DUMP8("%02x ", _addr, BT_ADDR_OUTPUT_PRINT_NUM);
            }
            return ai_connect_index;
        }
    }

    return 0;
}

bool app_ai_spec_have_other_connection(uint8_t ai_spec)
{
    for (uint8_t ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if((ai_connect_info[ai_connect_index].ai_spec) == ai_spec)
        {
            return true;
        }
    }

    return false;
}

AI_connect_info *app_ai_get_connect_info(uint8_t ai_connect_index)
{
    if(ai_connect_index < AI_CONNECT_NUM_MAX) {
        return &ai_connect_info[ai_connect_index];
    }else {
        return NULL;
    }
}

uint8_t app_ai_get_connected_mun(void)
{
    uint8_t connected_mun = 0;

    for (uint8_t ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if ((ai_connect_info[ai_connect_index].ai_connect_state) != AI_IS_DISCONNECT)
        {
            connected_mun +=1;
        }
    }

    return connected_mun;
}

uint8_t app_ai_get_dest_id(AI_connect_info* ai_info)
{
    uint8_t dest_id = 0;

    if (ai_info->ai_connect_type == AI_TRANSPORT_SPP)
    {
        dest_id = ai_info->device_id;
    }
    else if (ai_info->ai_connect_type == AI_TRANSPORT_BLE)
    {
        dest_id = SET_BLE_FLAG(ai_info->conidx);
    }

    return dest_id;
}

uint8_t app_ai_get_dest_id_by_ai_spec(uint8_t ai_spec)
{
    uint8_t dest_id = 0;
    AI_connect_info *foreground_ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(NULL == foreground_ai_info) {
        return 0;
    }
    if (foreground_ai_info->ai_spec == ai_spec)
    {
        dest_id = app_ai_get_dest_id(foreground_ai_info);
    }else {
        for(uint8_t i=0; i<AI_CONNECT_NUM_MAX; i++)
        {
            if ( (ai_connect_info[i].ai_spec == ai_spec) && \
                 (ai_connect_info[i].ai_connect_state == AI_IS_CONNECTED))
            {
                dest_id = ai_connect_info[i].device_id;
                break;
            }
        }
    }
    return dest_id;
}

void app_ai_clear_connect_info(uint8_t ai_connect_index)
{
    TRACE(2, "%s ai %d", __func__, ai_connect_index);
    if(ai_connect_index >= 0 && ai_connect_index < AI_CONNECT_NUM_MAX)
    {
        ai_connect_info[ai_connect_index].ai_connect_state = AI_IS_DISCONNECT;
        ai_connect_info[ai_connect_index].ai_connect_type = AI_TRANSPORT_IDLE;
        ai_connect_info[ai_connect_index].device_id = 0xFF;
        ai_connect_info[ai_connect_index].ai_spec = AI_SPEC_INIT;
        memset(ai_connect_info[ai_connect_index].addr, 0, 6);
    }
}

uint8_t app_ai_get_connect_index(uint8_t *_addr, uint8_t aiSpec)
{
    uint8_t ai_connect_index = 0;

    for (ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        if ((ai_connect_info[ai_connect_index].ai_spec == aiSpec) && 
        !memcmp(ai_connect_info[ai_connect_index].addr, _addr, 6))
        {
            break;
        }
    }

    return ai_connect_index;
}

uint8_t app_ai_get_device_id_from_index(uint8_t index)
{
    return ai_connect_info[index].device_id;
}

uint8_t app_ai_get_ai_index_from_connect_index(uint8_t connect_index)
{
    uint8_t ai_index = 0;

    for (uint8_t index = 0; index < MUTLI_AI_NUM; index++)
    {
        if (ai_connect_info[connect_index].ai_spec == index)
        {
            ai_index = index;
            break;
        }
    }
    TRACE(2, "%s ai_index = %d", __func__, ai_index);
    return ai_index;
}

void app_ai_set_ai_spec(uint8_t ai_index)
{
    //TRACE(3,"%s %d%s", __func__, ai_index, ai_spec_type2str((AI_SPEC_TYPE_E)ai_index));
    ai_struct[ai_index].currentAiSpec = ai_index;
}

uint8_t app_ai_get_ai_spec(uint8_t ai_index)
{
    //TRACE(4,"%s %d%s", __func__, ai_struct[ai_index].currentAiSpec, ai_spec_type2str((AI_SPEC_TYPE_E)ai_struct[ai_index].currentAiSpec));
    return ai_struct[ai_index].currentAiSpec;
}

void app_ai_set_transport_type(AI_TRANS_TYPE_E type, uint8_t ai_index, uint8_t ai_connect_index)
{
    TRACE(3,"%s ai %d index %d type %d%s", __func__, ai_index, ai_connect_index, type, ai_trans_type2str(type));
    ai_struct[ai_index].transport_type[ai_connect_index] = type;
}

AI_TRANS_TYPE_E app_ai_get_transport_type(uint8_t ai_index, uint8_t ai_connect_index)
{
    /*
    TRACE(3,"%s ai %d type %d%s", __func__,
                     ai_index,
                     ai_struct[ai_index].transport_type[ai_connect_index],
                     ai_trans_type2str(ai_struct[ai_index].transport_type[ai_connect_index]));
    */
    return ai_struct[ai_index].transport_type[ai_connect_index];
}

void app_ai_set_data_trans_size(uint32_t trans_size, uint8_t ai_index)
{
    TRACE(2,"%s %d ai %d", __func__, trans_size, ai_index);
    if (app_ai_get_mtu(ai_index) != AI_TRANS_DEINIT_MTU_SIZE &&
            trans_size > app_ai_get_mtu(ai_index))
    {
        TRACE(2,"%s error mtu %d", __func__, app_ai_get_mtu(ai_index));
        trans_size = app_ai_get_mtu(ai_index);
    }
    
    ai_struct[ai_index].data_trans_size_per_trans = trans_size;
}

uint32_t app_ai_get_data_trans_size(uint8_t ai_index)
{
    return ai_struct[ai_index].data_trans_size_per_trans;
}

void app_ai_set_header_size_per_frame(uint32_t header_size_per_frame, uint8_t ai_index)
{
    ai_struct[ai_index].header_size_per_xfer_frame = header_size_per_frame;
}

uint32_t app_ai_get_header_size_per_frame(uint8_t ai_index)
{
    return ai_struct[ai_index].header_size_per_xfer_frame;
}

void app_ai_set_mtu(uint32_t mtu, uint8_t ai_index)
{
    uint32_t trans_size = app_ai_get_data_trans_size(ai_index);
    TRACE(2,"%s %d t_size %d ai %d", __func__, mtu, trans_size, ai_index);
    if (trans_size && trans_size > mtu)
    {
        TRACE(2,"%s error trans size %d", __func__, trans_size);
        app_ai_set_data_trans_size(mtu, ai_index);
    }
    ai_struct[ai_index].mtu = mtu;
}

uint32_t app_ai_get_mtu(uint8_t ai_index)
{
    return ai_struct[ai_index].mtu;
}

bool app_is_ai_voice_connected(uint8_t ai_index)
{
    uint8_t ai_conidx = app_ai_get_connect_index_from_ai_spec(ai_index);
    return (AI_TRANSPORT_IDLE != ai_struct[ai_index].transport_type[ai_conidx]);
}

void app_ai_set_wake_up_type(uint8_t type, uint8_t ai_index)
{
    TRACE(3,"%s %d", __func__, type);
    ai_struct[ai_index].wake_up_type = type;
}

uint8_t app_ai_get_wake_up_type(uint8_t ai_index)
{
    return ai_struct[ai_index].wake_up_type;
}

void app_ai_set_speech_state(APP_AI_SPEECH_STATE_E speech_state, uint8_t ai_index)
{
    TRACE(3,"%s %d%s", __func__, speech_state, ai_speech_state2str(speech_state));
    ai_struct[ai_index].speechState = speech_state;
}

APP_AI_SPEECH_STATE_E app_ai_get_speech_state(uint8_t ai_index)
{
    TRACE(3,"%s %d%s", __func__,
                     ai_struct[ai_index].speechState,
                     ai_speech_state2str(ai_struct[ai_index].speechState));
    return ai_struct[ai_index].speechState;
}

static void app_ai_mobile_init(uint8_t ai_index)
{
    TRACE(2,"%s size %d", __func__, sizeof(ai_struct[ai_index].ai_mobile_info));
    memset(&ai_struct[ai_index].ai_mobile_info, 0, sizeof(ai_struct[ai_index].ai_mobile_info));
}

static uint8_t app_ai_get_mobile_index(uint8_t *_addr)
{
    uint8_t index=0;
    uint8_t ai_index =0;
    for (ai_index = 0; ai_index < MUTLI_AI_NUM; ai_index++)
    {
        for (index=0; index<BT_DEVICE_NUM; index++)
        {
            if (ai_struct[ai_index].ai_mobile_info[index].used == true)
            {
                if (!memcmp(ai_struct[ai_index].ai_mobile_info[index].addr, _addr, 6))
                {
                    TRACE(2, "%s %d", __func__, index);
                    break;
                }
            }
        }
    }

    return index;
}

static void app_ai_set_mobile_info(MOBILE_CONN_TYPE_E type, uint8_t *_addr)
{
    uint8_t index=BT_DEVICE_NUM;
    uint8_t ai_index = 0;

    if (app_ai_get_mobile_index(_addr) < BT_DEVICE_NUM)
    {
        TRACE(1,"%s already save the info", __func__);
    }
    else
    {
        for (ai_index = 0; ai_index < MUTLI_AI_NUM; ai_index++)
        {
            for (index=0; index<BT_DEVICE_NUM; index++)
            {
                if (ai_struct[ai_index].ai_mobile_info[index].used == false)
                {
                    ai_struct[ai_index].ai_mobile_info[index].used = true;
                    ai_struct[ai_index].ai_mobile_info[index].mobile_connect_type = type;
                    memcpy(ai_struct[ai_index].ai_mobile_info[index].addr, _addr, 6);
                    //TRACE(5,"%s type %d%s index %d ai_index %d addr:", __func__, type, mobile_connect_type2str(type), index, ai_index);
                    //DUMP8("%02x ", _addr->address, BT_ADDR_OUTPUT_PRINT_NUM);
                    break;
                }
            }
        }
    }

}

static uint8_t app_ai_clear_mobile_info(uint8_t *_addr)
{
    uint8_t index = app_ai_get_mobile_index(_addr);
    uint8_t ai_index = 0;

    TRACE(2,"%s index %d addr:", __func__, index);
    DUMP8("%02x ", _addr, BT_ADDR_OUTPUT_PRINT_NUM);

    if (index >= BT_DEVICE_NUM)
    {
        TRACE(2,"%s don't have this addr info, index=%d", __func__, index);
    }
    else
    {
        for (ai_index = 0; ai_index < MUTLI_AI_NUM; ai_index++)
        {
            ai_struct[ai_index].ai_mobile_info[index].used = false;
            ai_struct[ai_index].ai_mobile_info[index].mobile_connect_type = MOBILE_CONNECT_IDLE;
            memset(ai_struct[ai_index].ai_mobile_info[index].addr, 0, 6);
        }
    }

    return index;
}

void app_ai_disconnect_all_mobile_link(void)
{
    btif_remote_device_t *p_remote_dev;

    TRACE(1,"%s", __func__);
    for(uint8_t ai_index = 0; ai_index < MUTLI_AI_NUM; ai_index++)
    {
        for (uint8_t index=0; index < BT_DEVICE_NUM; index++)
        {
            if (ai_struct[ai_index].ai_mobile_info[index].used == true)
            {
                TRACE(1,"disconnect bt index %d addr:", index);
                DUMP8("%02x ", ai_struct[ai_index].ai_mobile_info[index].addr, BT_ADDR_OUTPUT_PRINT_NUM);
                p_remote_dev = btif_me_get_remote_device_by_bdaddr((bt_bdaddr_t *)ai_struct[ai_index].ai_mobile_info[index].addr);
                btif_me_force_disconnect_link_with_reason(NULL, p_remote_dev, BTIF_BEC_USER_TERMINATED, TRUE);
            }
        }
    }
}

bool app_ai_dont_have_mobile_connect(uint8_t ai_index)
{
    for (uint8_t index=0; index<BT_DEVICE_NUM; index++)
    {
        if (ai_struct[ai_index].ai_mobile_info[index].used == true)
        {
            return false;
        }
    }

    return true;
}

bool app_ai_connect_mobile_has_ios(uint8_t ai_index)
{
    for (uint8_t index=0; index<BT_DEVICE_NUM; index++)
    {
        if (ai_struct[ai_index].ai_mobile_info[index].used == true &&
            ai_struct[ai_index].ai_mobile_info[index].mobile_connect_type == MOBILE_CONNECT_IOS)
        {
            return true;
        }
    }

    return false;
}

void app_ai_set_speech_type(AI_SPEECH_TYPE_E type, uint8_t ai_index)
{
    TRACE(3,"%s %d%s", __func__, type, ai_speech_type2str(type));
    ai_struct[ai_index].ai_speech_type = type;
}

AI_SPEECH_TYPE_E app_ai_get_speech_type(uint8_t ai_index)
{
    TRACE(3,"%s %d%s", __func__,
                     ai_struct[ai_index].ai_speech_type,
                     ai_speech_type2str(ai_struct[ai_index].ai_speech_type));
    return ai_struct[ai_index].ai_speech_type;
}

void app_ai_set_encode_type(uint8_t encode_type, uint8_t ai_index)
{
    TRACE(2,"%s spec:%d, codec:%d", __func__, ai_index, encode_type);
    ai_struct[ai_index].encode_type = encode_type;
}

uint8_t app_ai_get_encode_type(uint8_t ai_index)
{
    //TRACE(2,"%s %d", __func__, ai_struct[ai_index].encode_type);
    return ai_struct[ai_index].encode_type;
}

void app_ai_set_role_switching(bool switching, uint8_t ai_index)
{
    ai_struct[ai_index].is_role_switching = switching;
}

bool app_ai_is_role_switching(uint8_t ai_index)
{
    return ai_struct[ai_index].is_role_switching;
}

void app_ai_set_can_role_switch(bool can_switch, uint8_t ai_index)
{
    ai_struct[ai_index].can_role_switch = can_switch;
}

bool app_ai_can_role_switch(uint8_t ai_index)
{
    return ai_struct[ai_index].can_role_switch;
}

void app_ai_set_initiative_switch(bool initiative_switch, uint8_t ai_index)
{
    ai_struct[ai_index].initiative_switch = initiative_switch;
}

bool app_ai_is_initiative_switch(uint8_t ai_index)
{
    return ai_struct[ai_index].initiative_switch;
}

void app_ai_set_role_switch_direct(bool role_switch_direct, uint8_t ai_index)
{
    ai_struct[ai_index].role_switch_direct = role_switch_direct;
}

bool app_ai_is_role_switch_direct(uint8_t ai_index)
{
    return ai_struct[ai_index].role_switch_direct;
}

void app_ai_set_need_ai_param(bool need, uint8_t ai_index)
{
    ai_struct[ai_index].role_switch_need_ai_param = need;
}

bool app_ai_is_need_ai_param(uint8_t ai_index)
{
    return ai_struct[ai_index].role_switch_need_ai_param;
}

void ai_set_can_wake_up(bool wake_up, uint8_t ai_index)
{
    if (ai_struct[ai_index].can_wake_up != wake_up)
    {
        TRACE(2, "AI(%d) can wakeup update:%d->%d", ai_index, ai_struct[ai_index].can_wake_up, wake_up);
        ai_struct[ai_index].can_wake_up = wake_up;
    }
}

bool is_ai_can_wake_up(uint8_t ai_index)
{
    return ai_struct[ai_index].can_wake_up;
}

void ai_audio_stream_allowed_to_send_set(bool isEnabled, uint8_t ai_index)
{
    if (ai_struct[ai_index].transfer_voice_start != isEnabled)
    {
        TRACE(2, "AI(%d) allow to send update:%d->%d",
              ai_index, ai_struct[ai_index].transfer_voice_start, isEnabled);

        ai_struct[ai_index].transfer_voice_start = isEnabled;
    }
}

bool is_ai_audio_stream_allowed_to_send(uint8_t ai_index)
{
    return ai_struct[ai_index].transfer_voice_start;
}

void app_ai_set_timer_need(bool need, uint8_t ai_index)
{
    ai_struct[ai_index].ai_voice_timeout_timer_need = need;
}

bool app_ai_is_timer_need(uint8_t ai_index)
{
    return ai_struct[ai_index].ai_voice_timeout_timer_need;
}

void app_ai_set_stream_opened(bool opened, uint8_t ai_index)
{
    ai_struct[ai_index].ai_stream.ai_stream_opened = opened;
}

bool app_ai_is_stream_opened(uint8_t ai_index)
{
    return ai_struct[ai_index].ai_stream.ai_stream_opened;
}

void app_ai_set_stream_init(bool init, uint8_t ai_index)
{
    TRACE(2, "AI(%d) stream init state update:%d->%d",
          ai_index, ai_struct[ai_index].ai_stream.ai_stream_init, init);
    ai_struct[ai_index].ai_stream.ai_stream_init = init;
}

bool app_ai_is_stream_init(uint8_t ai_index)
{
    return ai_struct[ai_index].ai_stream.ai_stream_init;
}

void app_ai_set_stream_running(bool running, uint8_t ai_index)
{
    TRACE(3, "Stream running state for spec[%d] update:%d->%d",
          ai_index, ai_struct[ai_index].ai_stream.ai_stream_running, running);
    ai_struct[ai_index].ai_stream.ai_stream_running = running;
}

bool app_ai_is_stream_running(uint8_t ai_index)
{
    return ai_struct[ai_index].ai_stream.ai_stream_running;
}

void app_ai_set_algorithm_engine_reset(bool reset, uint8_t ai_index)
{
    if (ai_struct[ai_index].ai_stream.audio_algorithm_engine_reset != reset)
    {
        TRACE(2, "Engine reset flag for AI(%d) update:%d->%d", 
              ai_index, ai_struct[ai_index].ai_stream.audio_algorithm_engine_reset, reset);
        ai_struct[ai_index].ai_stream.audio_algorithm_engine_reset = reset;
    }
}

bool app_ai_is_algorithm_engine_reset(uint8_t ai_index)
{
    return ai_struct[ai_index].ai_stream.audio_algorithm_engine_reset;
}

void app_ai_set_setup_complete(bool complete, uint8_t ai_index)
{
    TRACE(2,"%s setup %d %d", __func__, complete, ai_index);
    ai_struct[ai_index].ai_stream.ai_setup_complete = complete;
}

bool app_ai_is_setup_complete(uint8_t ai_index)
{
    //TRACE(2,"%s setup %d", __func__, ai_struct.ai_stream.ai_setup_complete);
    return ai_struct[ai_index].ai_stream.ai_setup_complete;
}

void app_ai_set_peer_setup_complete(bool complete, uint8_t ai_index)
{
    TRACE(2,"%s setup %d", __func__, complete);
    ai_struct[ai_index].peer_ai_set_complete = complete;
}

bool app_ai_is_peer_setup_complete(uint8_t ai_index)
{
    //TRACE(2,"%s setup %d", __func__, ai_struct.ai_stream.ai_setup_complete);
    return ai_struct[ai_index].peer_ai_set_complete;
}

void app_ai_set_use_ai_32kbps_voice(bool use_ai_32kbps_voice, uint8_t ai_index)
{
    TRACE(2,"%s %d", __func__, use_ai_32kbps_voice);
    ai_struct[ai_index].use_ai_32kbps_voice = use_ai_32kbps_voice;
}

bool app_ai_is_use_ai_32kbps_voice(uint8_t ai_index)
{
    return ai_struct[ai_index].use_ai_32kbps_voice;
}

void app_ai_set_local_start_tone(bool local_start_tone, uint8_t ai_index)
{
    TRACE(2,"%s %d", __func__, local_start_tone);
    ai_struct[ai_index].local_start_tone = local_start_tone;
}

bool app_ai_is_local_start_tone(uint8_t ai_index)
{
    return ai_struct[ai_index].local_start_tone;
}

void app_ai_set_in_tws_mode(bool is_in_tws_mode, uint8_t ai_index)
{
    TRACE(2,"%s %d index %d", __func__, is_in_tws_mode, ai_index);
    ai_struct[ai_index].is_in_tws_mode = is_in_tws_mode;
}

bool app_ai_is_in_tws_mode(uint8_t ai_index)
{
    return ai_struct[ai_index].is_in_tws_mode;
}

bool app_ai_is_in_multi_ai_mode(void)
{
    return app_ai_manager_is_in_multi_ai_mode();
}

void app_ai_set_use_thirdparty(bool is_use_thirdparty, uint8_t ai_index)
{
    TRACE(2,"%s ai:%d use_thirdparty:%d", __func__, ai_index, is_use_thirdparty);

    ai_struct[ai_index].want_to_use_thirdparty = is_use_thirdparty;
    if (app_ai_is_setup_complete(ai_index))
    {
        // make sure not in speeching now
        if (is_ai_can_wake_up(ai_index))
        {
            if (is_use_thirdparty == false && \
                ai_struct[ai_index].is_use_thirdparty ==true && \
                ai_struct[ai_index].ai_stream.ai_setup_complete == true)
            {
                app_ai_voice_stream_control(false, app_ai_voice_get_user_from_spec(ai_index));
                app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS, THIRDPARTY_AI_DISCONNECT, ai_index);
            }
            else if (is_use_thirdparty == true && \
                ai_struct[ai_index].is_use_thirdparty ==false && \
                ai_struct[ai_index].ai_stream.ai_setup_complete == true)
            {
                app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS, THIRDPARTY_AI_CONNECT, ai_index);
                app_ai_voice_stream_control(true, app_ai_voice_get_user_from_spec(ai_index));
            }
        }
        else
        {
            //don't set ai_struct.is_use_thirdparty
            return;
        }
    }
    ai_struct[ai_index].is_use_thirdparty = is_use_thirdparty;
}

bool app_ai_is_use_thirdparty(uint8_t ai_index)
{
    return ai_struct[ai_index].is_use_thirdparty;
}

void app_ai_check_if_need_set_thirdparty(uint8_t ai_index)
{
    TRACE(2,"%s %d %d", __func__, ai_struct[ai_index].want_to_use_thirdparty, ai_struct[ai_index].is_use_thirdparty);

    if (ai_struct[ai_index].want_to_use_thirdparty != ai_struct[ai_index].is_use_thirdparty)
    {
        app_ai_set_use_thirdparty(ai_struct[ai_index].want_to_use_thirdparty, ai_index);
    }
}

void app_ai_set_opus_in_overlay(bool opus_in_overlay, uint8_t ai_index)
{
    TRACE(2,"%s %d", __func__, opus_in_overlay);
    ai_struct[ai_index].opus_in_overlay = opus_in_overlay;
}

bool app_ai_is_opus_in_overlay(uint8_t ai_index)
{
    return ai_struct[ai_index].opus_in_overlay;
}

bool app_ai_is_sco_mode(void)
{
    return (is_sco_mode()==0)?false:true;
}

void app_ai_mobile_connect_handle(MOBILE_CONN_TYPE_E type, uint8_t *_addr)
{
    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return;
        }
    }

    app_ai_set_mobile_info(type, _addr);

#ifdef __IAG_BLE_INCLUDE__
    //if (type == MOBILE_CONNECT_IOS && app_ai_ble_is_connected() && AI_TRANSPORT_IDLE == app_ai_get_transport_type())
    //{
    //    TRACE(1, "%s set ai connect", __func__);
    //    ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE);
    //}
#endif

    if (app_ai_is_in_tws_mode(0))
    {
        app_ai_tws_sync_ai_info();
    }

    app_ai_ui_global_handler_ind(AI_CUSTOM_CODE_MOBILE_CONNECT, _addr, (uint32_t)type, 0, 0);
}

void app_ai_mobile_disconnect_handle(uint8_t *_addr)
{
    uint8_t index = app_ai_clear_mobile_info(_addr);
    if (index >= BT_DEVICE_NUM)
    {
        TRACE(1, "%s can't clear this mobile info", __func__);
        return;
    }

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return;
        }
    }

    //if (!memcmp(_addr->address, app_ai_spp_get_remdev_btaddr(), 6) && app_ai_spp_is_connected())
    //{
    //    app_ai_spp_disconnlink();
    //}

    if (app_ai_is_in_tws_mode(0))
    {
        app_ai_tws_sync_ai_info();
    }

    app_ai_ui_global_handler_ind(AI_CUSTOM_CODE_MOBILE_DISCONNECT, NULL, 0, 0, 0);
}

uint8_t app_ai_connect_handle(AI_TRANS_TYPE_E ai_trans_type, uint8_t ai_index, uint8_t dest_id, uint8_t *addr)
{
    uint8_t ai_connect_index = 0;

    if(ai_trans_type == AI_TRANSPORT_SPP)
    {
        ai_connect_index = app_ai_set_spp_connect_info(addr, dest_id, AI_TRANSPORT_SPP, ai_index);
    }
    else
    {
        ai_connect_index = app_ai_get_connect_index_from_ble_conidx(GET_BLE_CONIDX(dest_id), ai_index);
    }

    ai_manager_set_foreground_ai_conidx(ai_connect_index);

    app_ai_set_transport_type(ai_trans_type, ai_index, ai_connect_index);
    app_ai_set_speech_state(AI_SPEECH_STATE__IDLE, ai_index);
    app_ai_voice_deinit(ai_index, ai_connect_index);

#ifdef __IAG_BLE_INCLUDE__
    app_ai_if_ble_set_adv(AI_BLE_ADVERTISING_INTERVAL);
#endif

    return ai_connect_index;
}

void app_ai_disconnect_handle(AI_TRANS_TYPE_E ai_trans_type, uint8_t ai_index)
{
    TRACE(1,"%s", __func__);
    /// disable AI(stop data handling) when disconnected
    app_ai_voice_set_active_flag(app_ai_voice_get_user_from_spec(ai_index), false);

    app_ai_tws_role_switch_direct();

    uint8_t ai_tws_role = app_ai_tws_get_local_role();
    uint8_t ai_connect_index = app_ai_get_connect_state_index(AI_IS_DISCONNECTING);

    osTimerStop(app_ai_voice_timeout_timer_id);

    if (app_ai_ble_is_connected(ai_connect_index) && AI_TRANSPORT_BLE != ai_trans_type)
    {
        app_ai_disconnect_ble(ai_connect_index);
    }
    if (app_ai_spp_is_connected(ai_index, ai_connect_index) && AI_TRANSPORT_SPP != ai_trans_type)
    {
        app_ai_spp_disconnlink(ai_index, ai_connect_index);
    }

    app_ai_clear_connect_info(ai_connect_index);
    app_ai_set_transport_type(AI_TRANSPORT_IDLE, ai_index, ai_connect_index);

    if(!app_ai_spec_have_other_connection(ai_index))
    {
        app_ai_set_setup_complete(false, ai_index);
        app_ai_set_peer_setup_complete(false, ai_index);
        app_ai_set_speech_state(AI_SPEECH_STATE__IDLE, ai_index);

        app_ai_set_mtu(AI_TRANS_DEINIT_MTU_SIZE, ai_index);
        app_ai_set_data_trans_size(0, ai_index);
        app_ai_voice_stream_control(false, app_ai_voice_get_user_from_spec(ai_index));

        ai_set_can_wake_up(false, ai_index);

#ifdef __IAG_BLE_INCLUDE__
        app_ai_if_ble_set_adv(AI_BLE_ADVERTISING_INTERVAL);
#endif

        if (app_ai_is_in_multi_ai_mode())
        {
            ai_manager_set_spec_connected_status((AI_SPEC_TYPE_E)app_ai_get_ai_spec(ai_index), 0);
        }

        if (app_ai_is_use_thirdparty(ai_index))
        {
            app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS, THIRDPARTY_AI_DISCONNECT, ai_index);
        }
    }

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected())
        {
            if ((AI_TRANSPORT_BLE == ai_trans_type) || (ai_tws_role == APP_AI_TWS_MASTER))
            {
                app_ai_tws_sync_ai_info();
            }
            else
            {
                TRACE(2,"%s role %d isn't MASTER", __func__, ai_tws_role);
                return;
            }
        }
    }
}

void app_ai_setup_complete_handle(uint8_t ai_index)
{
    app_ai_set_setup_complete(true, ai_index);

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected())
        {
            if (app_ai_tws_get_local_role() == APP_AI_TWS_MASTER)
            {
                app_ai_tws_sync_ai_info();
            }
            else
            {
                TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            }
        }
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (app_ai_is_in_multi_ai_mode())
    {
        ai_voicekey_save_status(true);
        ai_manager_set_spec_connected_status((AI_SPEC_TYPE_E)ai_index, 1);
        app_ai_tws_sync_ai_manager_info();
        if (ai_index != ai_manager_get_current_spec())
        {
            TRACE(2,"%s spec %d", __func__, ai_index);
            return;
        }
    }
#endif

    ai_manager_set_spec_connected_status((AI_SPEC_TYPE_E)ai_index, 1);
    if (app_ai_is_use_thirdparty(ai_index))
    {
        app_ai_voice_stream_control(true, app_ai_voice_get_user_from_spec(ai_index));
    }
    else
    {
        TRACE(0, "AI(%d) not use thirdparty", ai_index);
    }

    app_ai_ui_global_handler_ind(AI_CUSTOM_CODE_SETUP_COMPLETE, NULL, 0, 0, 0);
}


static void ai_thread(void const *argument);
osThreadDef(ai_thread, osPriorityAboveNormal, 1, 4096, "ai_thread");

osMailQDef (ai_mailbox, AI_MAILBOX_MAX, AI_MESSAGE_BLOCK);
static osMailQId ai_mailbox = NULL;
static uint8_t ai_mailbox_cnt = 0;

static int ai_mailbox_init(void)
{
    ai_mailbox = osMailCreate(osMailQ(ai_mailbox), NULL);
    if (ai_mailbox == NULL)
    {
        TRACE(0,"Failed to Create ai_mailbox\n");
        return -1;
    }
    ai_mailbox_cnt = 0;
    return 0;
}

int ai_mailbox_put(AI_SIGNAL_E signal, uint32_t len, uint8_t ai_index, uint8_t dest_id)
{
    osStatus status = osOK;

    AI_MESSAGE_BLOCK *msg_p = NULL;

    if(ai_mailbox_cnt >= AI_MAILBOX_MAX -2)
    {
        TRACE(2,"%s warn ai_mailbox_cnt is %d", __func__, ai_mailbox_cnt);
        return osErrorValue;
    }

    if (signal == SIGN_AI_TRANSPORT)
    {
        if(++ai_struct[ai_index].send_num > MAX_TX_CREDIT)
        {
            ai_struct[ai_index].send_num = MAX_TX_CREDIT;
            return osErrorValue;
        }
    }

    msg_p = (AI_MESSAGE_BLOCK*)osMailAlloc(ai_mailbox, 0);
    ASSERT(msg_p, "osMailAlloc error");
    msg_p->signal = signal;
    msg_p->len = len;
    msg_p->ai_index = ai_index;
    msg_p->dest_id = dest_id;

    status = osMailPut(ai_mailbox, msg_p);
    if (osOK != status)
    {
        TRACE(2,"%s error status 0x%02x", __func__, status);
        return (int)status;
    }

    ai_mailbox_cnt++;

    return (int)status;
}

int ai_mailbox_free(AI_MESSAGE_BLOCK* msg_p)
{
    osStatus status;

    status = osMailFree(ai_mailbox, msg_p);
    if (osOK != status)
    {
        TRACE(2,"%s error status 0x%02x", __func__, status);
        return (int)status;
    }

    if (ai_mailbox_cnt)
    {
        ai_mailbox_cnt--;
    }

    return (int)status;
}

int ai_mailbox_get(AI_MESSAGE_BLOCK** msg_p)
{
    osEvent evt;

    evt = osMailGet(ai_mailbox, osWaitForever);
    if (evt.status == osEventMail)
    {
        *msg_p = (AI_MESSAGE_BLOCK *)evt.value.p;
        return 0;
    }
    else
    {
       TRACE(2,"%s evt.status 0x%02x", __func__, evt.status);
        return -1;
    }
}

static void ai_thread(void const *argument)
{
    //osEvent evt;
    AI_MESSAGE_BLOCK* msg_p = NULL;
    AI_SIGNAL_E signal = SIGN_AI_NONE;
    uint32_t len = 0;
    uint8_t ai_index = 0;
    uint8_t dest_id = 0;

    while(1)
    {
        if(!ai_mailbox_get(&msg_p))
        {
            //msg_p = (AI_MESSAGE_BLOCK *)evt.value.p;
            signal = msg_p->signal;
            len = msg_p->len;
            ai_index = msg_p->ai_index;
            dest_id = msg_p->dest_id;
            ai_mailbox_free(msg_p);

            switch(signal){
                case SIGN_AI_CONNECT:
                    //do nothing
                break;
                case SIGN_AI_WAKEUP:
                    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(ai_index) || app_ai_is_timer_need(ai_index)) {
                        app_ai_set_timer_need(false, ai_index);
                        osTimerStop(app_ai_voice_timeout_timer_id);
                        osTimerStart(app_ai_voice_timeout_timer_id, APP_AI_VOICE_MIC_TIMEOUT_INTERVEL);
                    }
                    if (app_ai_is_local_start_tone(ai_index))
                    {
#ifdef MEDIA_PLAYER_SUPPORT
                        if(0)
                        {

                        }
#if defined(__AMA_VOICE__)
                        else if(ai_index == AI_SPEC_AMA)
                            app_voice_report(APP_STATUS_INDICATION_ALEXA_START, 0);
#elif defined(__TENCENT_VOICE__)
                        else if(ai_index == AI_SPEC_TENCENT)
                            app_voice_report(APP_STATUS_INDICATION_TENCENT_START, 0);
#endif
                        else
                            app_voice_report(APP_STATUS_INDICATION_GSOUND_MIC_OPEN, 0);
#endif
                    }
                break;
                case SIGN_AI_CMD_COME:
                    ai_function_handle(CALLBACK_DATA_PARSE, NULL, len, ai_index, dest_id);
                break;
                case SIGN_AI_TRANSPORT:
                    ai_transport_handler(ai_index, dest_id);
                break;
                default:
                    TRACE(1,"%s error msg", __func__);
                break;
            }
        }
    }
}

AI_HANDLER_FUNCTION_TABLE_INSTANCE_T* ai_handler_function_get_entry_pointer_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__ai_handler_function_table_end-(uint32_t)__ai_handler_function_table_start)/sizeof(AI_HANDLER_FUNCTION_TABLE_INSTANCE_T);index++) {
        if (AI_HANDLER_FUNCTION_TABLE_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code) {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return AI_HANDLER_FUNCTION_TABLE_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return NULL;
}

uint32_t ai_handler_function_get_entry_index_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__ai_handler_function_table_end-(uint32_t)__ai_handler_function_table_start)/sizeof(AI_HANDLER_FUNCTION_TABLE_INSTANCE_T);index++) {
        if (AI_HANDLER_FUNCTION_TABLE_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code) {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return index;
        }
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return INVALID_AI_HANDLER_FUNCTION_ENTRY_INDEX;
}

static void _common_ai_init()
{
    static bool instanceflag = false;

    if (!instanceflag)
    {
        if (NULL == app_ai_voice_timeout_timer_id)
        {
            app_ai_voice_timeout_timer_id = osTimerCreate(osTimer(APP_AI_VOICE_MIC_TIMEOUT), osTimerOnce, NULL);
        }

        memset((uint8_t *)&ai_struct, 0, sizeof(ai_struct));
        /// init transport FIFO
        ai_transport_fifo_init();
        /// init connection info
        app_ai_connect_info_init();

#ifdef RTOS
        if (ai_mailbox_init())
        {
            TRACE(0, "ERROR: AI mailbox init failed");
            return;
        }

        osThreadId ai_thread_tid = osThreadCreate(osThread(ai_thread), NULL);
        if (ai_thread_tid == NULL)
        {
            TRACE(0, "ERROR: AI thread init failed");
            return;
        }
#endif

        instanceflag = true;
    }
    else
    {
        TRACE(0,"AI common already initiated!");
    }

    return;
}

void ai_open_specific_ai(uint32_t ai_spec)
{
    /// @see AI_SPEC_TYPE_E to get detailed AI spec
    TRACE(2, "%s spec:%d", __func__, ai_spec);
    /// init AI related common module
    _common_ai_init();

    uint8_t ai_index = (uint8_t)ai_spec;
    app_ai_set_ai_spec(ai_index);
    app_ai_set_transport_type(AI_TRANSPORT_IDLE, ai_index, 0);
    app_ai_set_transport_type(AI_TRANSPORT_IDLE, ai_index, 1);
    app_ai_mobile_init(ai_index);
    app_ai_set_can_role_switch(false, ai_index);
    ai_set_can_wake_up(false, ai_index);
    app_ai_set_role_switching(false, ai_index);
    app_ai_set_initiative_switch(false, ai_index);
    app_ai_set_role_switch_direct(false, ai_index);
    app_ai_set_need_ai_param(false, ai_index);
    app_ai_set_timer_need(false, ai_index);
    app_ai_set_stream_opened(false, ai_index);
    app_ai_set_setup_complete(false, ai_index);
    app_ai_set_peer_setup_complete(false, ai_index);
    ai_audio_stream_allowed_to_send_set(false, ai_index);
    app_ai_set_use_ai_32kbps_voice(false, ai_index);
    app_ai_set_local_start_tone(true, ai_index);
    app_ai_set_in_tws_mode(false, ai_index);
    app_ai_set_use_thirdparty(false, ai_index);
    app_ai_set_opus_in_overlay(true, ai_index);

    /// init codec type
    uint8_t encodeType = app_ai_get_codec_type(ai_index);
    app_ai_set_encode_type(encodeType, ai_index);
    /// init speech type
    app_ai_set_speech_type(TYPE__NORMAL_SPEECH, ai_index);
    ai_struct[ai_index].send_num              = 0;
    ai_struct[ai_index].tx_credit             = MAX_TX_CREDIT;
    ai_struct[ai_index].wake_word_start_index = 0;
    ai_struct[ai_index].wake_word_end_index   = 12000;

#ifdef __IAG_BLE_INCLUDE__
    app_ai_ble_init(ai_index);
    ai_ble_handler_init(ai_index);
#endif

    app_ai_spp_server_init(ai_index);

    AI_HANDLER_FUNCTION_TABLE_INSTANCE_T *pInstance = ai_handler_function_get_entry_pointer_from_ai_code(ai_index);
    if(pInstance)
    {
        const struct ai_handler_func_table* ai_handler_function_table = pInstance->ai_handler_function_table;
        ai_control_init(ai_handler_function_table, ai_index);
        TRACE(2, "Register func_table(%p) for AI(%d)", ai_handler_function_table, ai_index);
    }
    else
    {
        TRACE(2, "WARNING:no instance found for AI(%d)", ai_index);
    }
}

void ai_open_all_ai(uint32_t ai_spec)
{
    /// @see AI_SPEC_TYPE_E to get detailed AI spec
    TRACE(2, "%s spec:%d", __func__, ai_spec);
    /// init AI related common module
    _common_ai_init();

    AI_HANDLER_FUNCTION_TABLE_INSTANCE_T *pInstance = NULL;
    const struct ai_handler_func_table* ai_handler_function_table = NULL;

    for(uint8_t ai_index = AI_SPEC_AMA; ai_index < AI_SPEC_COUNT; ai_index++)
    {
        if(((ai_spec >> 8) == ai_index) || (ai_spec & 0xFF) == ai_index)
        {
            app_ai_set_ai_spec(ai_index);
            app_ai_set_transport_type(AI_TRANSPORT_IDLE, ai_index, 0);
            app_ai_set_transport_type(AI_TRANSPORT_IDLE, ai_index, 1);
            app_ai_mobile_init(ai_index);
            app_ai_set_can_role_switch(false, ai_index);
            ai_set_can_wake_up(false, ai_index);
            app_ai_set_role_switching(false, ai_index);
            app_ai_set_initiative_switch(false, ai_index);
            app_ai_set_role_switch_direct(false, ai_index);
            app_ai_set_need_ai_param(false, ai_index);
            app_ai_set_timer_need(false, ai_index);
            app_ai_set_stream_opened(false, ai_index);
            app_ai_set_setup_complete(false, ai_index);
            app_ai_set_peer_setup_complete(false, ai_index);
            ai_audio_stream_allowed_to_send_set(false, ai_index);
            app_ai_set_use_ai_32kbps_voice(false, ai_index);
            app_ai_set_local_start_tone(true, ai_index);
            app_ai_set_in_tws_mode(false, ai_index);
            app_ai_set_use_thirdparty(false, ai_index);
            app_ai_set_opus_in_overlay(true, ai_index);

            /// 0 means invalid encode type
            uint8_t encodeType = app_ai_get_codec_type(ai_index);
#ifdef VOC_ENCODE_ADPCM
            encodeType = VOC_ENCODE_ADPCM;
#endif
#ifdef VOC_ENCODE_SBC
            encodeType = VOC_ENCODE_SBC;
#endif
#ifdef VOC_ENCODE_OPUS
            encodeType = VOC_ENCODE_OPUS;
#endif
#ifdef VOC_ENCODE_SCALABLE
            encodeType = VOC_ENCODE_SCALABLE;
#endif

            app_ai_set_encode_type(encodeType, ai_index);
            app_ai_set_speech_type(TYPE__NORMAL_SPEECH, ai_index);
            ai_struct[ai_index].send_num              = 0;
            ai_struct[ai_index].tx_credit             = MAX_TX_CREDIT;
            ai_struct[ai_index].wake_word_start_index = 0;
            ai_struct[ai_index].wake_word_end_index   = 12000;

#ifdef __IAG_BLE_INCLUDE__
            app_ai_ble_init(ai_index);
            ai_ble_handler_init(ai_index);
#endif

            app_ai_spp_server_init(ai_index);

            pInstance = ai_handler_function_get_entry_pointer_from_ai_code(ai_index);
            if(pInstance)
            {
                ai_handler_function_table = pInstance->ai_handler_function_table;
                ai_control_init(ai_handler_function_table, ai_index);
                TRACE(2, "Register func_table(%p) for AI(%d)", ai_handler_function_table, ai_index);
            }
            else
            {
                TRACE(2, "WARNING:no instance found for AI(%d)", ai_index);
            }
        }
    }
}

uint32_t ai_save_ctx(uint8_t *buf, uint32_t buf_len)
{
    uint8_t ai_index =0;
    uint32_t offset = 0;
    uint32_t tmp_offset =0;
    uint32_t send_len = 0;
    uint32_t ai_spec = app_ai_if_get_ai_spec();

    for (uint8_t connect_index = 0; connect_index < AI_CONNECT_NUM_MAX; connect_index++)
    {
        ai_index = ((ai_spec >> (8 * connect_index)) & 0xFF);

        if (0 == ai_index)
        {
            continue;
        }

        memcpy((buf + tmp_offset), &ai_struct[ai_index].ai_mobile_info, sizeof(ai_struct[ai_index].ai_mobile_info));
        offset += sizeof(ai_struct[ai_index].ai_mobile_info);
        buf[offset++] = app_ai_get_transport_type(ai_index, connect_index);
        buf[offset++] = app_ai_is_setup_complete(ai_index);
        app_ai_set_peer_setup_complete(false, ai_index);
        *(uint32_t *)(buf + offset) = app_ai_get_mtu(ai_index);
        offset += 4;
        *(uint32_t *)(buf + offset) = app_ai_get_data_trans_size(ai_index);
        offset += 4;

        send_len = ai_function_handle(CALLBACK_AI_PARAM_SEND, &buf[offset], buf_len, ai_index, 0);
        if (send_len && ERROR_RETURN != send_len)
        {
            offset += send_len;
        }

        TRACE(3,"%s offset %d setup %d",
              __func__,
              offset,
              app_ai_is_setup_complete(ai_index));
        tmp_offset += offset;
        offset = 0;
    }
    return tmp_offset;
}



uint32_t ai_restore_ctx(uint8_t *buf, uint32_t buf_len)
{
    uint8_t ai_index =0;
    uint32_t offset = 0;
    uint32_t tmp_offset =0;
    uint32_t recieve_len = 0;
    uint32_t ai_spec = app_ai_if_get_ai_spec();

    for (uint8_t connect_index = 0; connect_index < AI_CONNECT_NUM_MAX; connect_index++)
    {
        ai_index = ((ai_spec >> (8 * connect_index)) & 0xFF);

        if (0 == ai_index)
        {
            continue;
        }

        memcpy(&ai_struct[ai_index].ai_mobile_info, (buf + tmp_offset), sizeof(ai_struct[ai_index].ai_mobile_info));
        offset += sizeof(ai_struct[ai_index].ai_mobile_info);
        app_ai_set_transport_type((AI_TRANS_TYPE_E)buf[offset], ai_index, connect_index);
        offset += 1;
        app_ai_set_peer_setup_complete(buf[offset], ai_index);
        if (AI_TRANSPORT_SPP == app_ai_get_transport_type(ai_index, connect_index))
        {
            app_ai_set_setup_complete(buf[offset], ai_index);
        }
        offset += 1;
        app_ai_set_mtu(*(uint32_t *)(buf + offset), ai_index);
        offset += 4;
        app_ai_set_data_trans_size(*(uint32_t *)(buf + offset), ai_index);
        offset += 4;

        TRACE(2,"%s peer setup %d", __func__, app_ai_is_peer_setup_complete(ai_index));

#ifdef __IAG_BLE_INCLUDE__
        app_ai_if_ble_set_adv(AI_BLE_ADVERTISING_INTERVAL);
#endif
        recieve_len = ai_function_handle(CALLBACK_AI_PARAM_RECEIVE, buf, buf_len, ai_index, 0);
        if (recieve_len && ERROR_RETURN != recieve_len)
        {
            offset += recieve_len;
        }
        tmp_offset += offset;
        offset = 0;
    }
    return tmp_offset;
}


