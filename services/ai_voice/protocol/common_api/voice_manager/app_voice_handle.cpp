#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include <stdlib.h>
#include "app_audio.h"
#include "app_ble_mode_switch.h"
#include "app_bt_stream.h"
#include "app_battery.h"
#include "gapm_task.h"

#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_key.h"
#include "hal_location.h"
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif


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
#include "app_voice_handle.h"
#include "app_voice_ble.h"
#include "app_voice_bt.h"
#include "voice_compression.h"
#if defined(IBRT)
#include "app_tws_ctrl_thread.h"
#include "crc32_c.h"
#endif

#include "factory_section.h"

#define VOICE_TIME_TICK_MS    10
osTimerId voice_time_tick_handling_timer_id = NULL;
static int voice_time_tick_handling_handler(void const *param);
osTimerDef (VOICE_TIME_TICK_HANDLING_TIMER, (void (*)(void const *))voice_time_tick_handling_handler);

static voice_ble_adv_para_t voice_gatt_adv_data;

static int voice_time_tick_handling_handler(void const *param)
{
    static uint8_t tickTime = 0;
    if(tickTime == 5)
    {
        voice_time_tick_cb(TICK_TYPE_50MS);
        tickTime = 0;
        osTimerStart(voice_time_tick_handling_timer_id, VOICE_TIME_TICK_MS);
    }
    else
    {
        voice_time_tick_cb(TICK_TYPE_10MS);
        tickTime++;
        osTimerStart(voice_time_tick_handling_timer_id, VOICE_TIME_TICK_MS);
    }

    return 0;
}

void voice_init(void)
{
    TRACE(1, "%s", __func__);

    if (NULL == voice_time_tick_handling_timer_id)
    {
        voice_time_tick_handling_timer_id = osTimerCreate(osTimer(VOICE_TIME_TICK_HANDLING_TIMER), osTimerOnce, NULL);
    }

    //osTimerStart(gws_time_tick_handling_timer_id, GWS_TIME_TICK_MS);
    //uint8_t davData[] = {0x11, 0xff, 0xa8, 0x01, 0xa5, 0x0d, 0xd0, 0x04, 0x00, 0x00, 0x44, 0x5d, 0x03, 0xe6, 0x2f, 0xd8, 0x06, 0x00, 0x03, 0x16, 0xb3, 0xfe};
    //sal_gma_ble_adv_data_set(gma_davData, 17);
}


void voice_recv_proc_cb(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s", __func__);
}

void voice_ota_recv_proc_cb(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s", __func__);
}

void voice_mic_recv_data_cb(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s", __func__);
}

void voice_time_tick_cb(tick_type_e tick_type)
{
    TRACE(1, "%s", __func__);
}

void voice_bt_connect_cb(bt_connect_e bt_state)
{
    TRACE(1, "%s", __func__);
}

void voice_ble_adv_data_set(void *para, uint8_t len)
{
    TRACE(1, "%s", __func__);

    ASSERT(len <= BLE_DATA_LEN, " adv data exceed ");


    memcpy((uint8_t*)voice_gatt_adv_data.adv_data, (uint8_t*)para, len);
    voice_gatt_adv_data.adv_data_len = len;

    TRACE(0,"voice_ble_adv_data_set");
    DUMP8("%02x ", &voice_gatt_adv_data, len);

}

void voice_ble_adv_start(void)
{
    TRACE(1, "%s", __func__);

    app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

void voice_ble_adv_stop(void)
{
    TRACE(1, "%s", __func__);

    app_ble_stop_activities();
}

void voice_ble_send_data_notify(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s Data: ", __func__);
    DUMP8("0x%02x ",buf,len);

    app_voice_send_data_via_notification(buf, len);

}

void voice_ble_send_data_indicate(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s Data: ", __func__);
    DUMP8("0x%02x ",buf,len);

    app_voice_send_data_via_indication(buf, len);

}

void voice_ble_send_cmd_notify(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s Data: ", __func__);
    DUMP8("0x%02x ",buf,len);

    app_voice_send_cmd_via_notification(buf, len);

}

void voice_ble_send_cmd_indicate(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s Data: ", __func__);
    DUMP8("0x%02x ",buf,len);

    app_voice_send_cmd_via_indication(buf, len);

}

void voice_spp_send_data(uint8_t *buf, uint32_t len)
{
    TRACE(1, "%s Data: ", __func__);
    DUMP8("0x%02x ",buf,len);

    app_ai_spp_send(buf, len);
}

uint32_t voice_start_advertising(void *param1, uint32_t param2)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T *)param1;

    memcpy(&cmd->advData[cmd->advDataLen], (uint8_t*)&voice_gatt_adv_data, voice_gatt_adv_data.adv_data_len);
    cmd->advDataLen += voice_gatt_adv_data.adv_data_len ;

    TRACE(1,"%d", voice_gatt_adv_data.adv_data_len );

    TRACE(0,"app_voice_start_advertising");
    DUMP8("%02x ", cmd->advData, cmd->advDataLen);
    return 0;
}

uint32_t voice_connected(void *param1, uint32_t param2)
{
    TRACE(1, "%s", __func__);
    
    if(param2 == AI_TRANSPORT_SPP)
    {
        voice_bt_connect_cb(SPP_CONNECT);
    }
    else if(param2 == AI_TRANSPORT_BLE)
    {
        voice_bt_connect_cb(BLE_CONNECT);
    }

    return 0;
}

uint32_t voice_disconnected(void *param1, uint32_t param2)
{
    TRACE(1, "%s", __func__);
    
    if(param2 == AI_TRANSPORT_SPP)
    {
        voice_bt_connect_cb(SPP_DISCONNECT);
    }
    else if(param2 == AI_TRANSPORT_BLE)
    {
        voice_bt_connect_cb(BLE_DISCONNECT);
    }

    return 0;
}

extern "C" bool app_bt_a2dp_service_is_connected (uint8_t device_id);
bool a2dp_status_get(void)
{
    TRACE(1, "%s", __func__);

    bool a2dp_state = app_bt_a2dp_service_is_connected(BT_DEVICE_ID_1);

    return a2dp_state;
}

void a2dp_play_start(void)
{
    TRACE(1, "%s", __func__);

    a2dp_handleKey(AVRCP_KEY_PLAY);
}

void a2dp_play_pause(void)
{
    TRACE(1, "%s", __func__);

    a2dp_handleKey(AVRCP_KEY_PAUSE);
}

void a2dp_play_stop(void)
{
    TRACE(1, "%s", __func__);

    a2dp_handleKey(AVRCP_KEY_STOP);
}

void a2dp_play_next(void)
{
    TRACE(1, "%s", __func__);

    a2dp_handleKey(AVRCP_KEY_FORWARD);
}

void a2dp_play_last(void)
{
    TRACE(1, "%s", __func__);

    a2dp_handleKey(AVRCP_KEY_BACKWARD);
}

void voice_volume_set(uint16_t type, uint8_t vol_num)
{
    TRACE(1, "%s", __func__);

    app_bt_set_volume(type, vol_num);
}

extern "C" bool app_is_hfp_service_connected (uint8_t device_id);
bool hfp_status_get(void)
{
    TRACE(1, "%s", __func__);

    bool hfp_state = app_is_hfp_service_connected(BT_DEVICE_ID_1);

    return hfp_state;
}
void dial_phone_num(uint8_t *buf, uint16_t len)
{
    TRACE(1, "%s", __func__);

    btif_hf_channel_t* hf_chan = app_bt_get_device(BT_DEVICE_ID_1)->hf_channel;
    
    if (app_is_hfp_service_connected(BT_DEVICE_ID_1))
    {
        btif_hf_dial_number(hf_chan, buf, (uint16_t)len);
    }
}

void telephone_call(void)
{
    TRACE(1, "%s", __func__);

    hfp_handle_key(HFP_KEY_ANSWER_CALL);
}

void telephone_refuse(void)
{
    TRACE(1, "%s", __func__);

    hfp_handle_key(HFP_KEY_HANGUP_CALL);
}

void voice_mic_set(mic_state_e state)
{
    TRACE(1, "%s", __func__);
    app_ai_voice_stream_control(onOff);
}

void power_value_get(voice_power_value_t *gws_power)
{
    TRACE(1, "%s", __func__);

    app_battery_get_info((APP_BATTERY_MV_T *)&gws_power->value, NULL, (APP_BATTERY_STATUS_T *)&gws_power->status);
}

#if 0
void sal_random_value_get(uint8_t *gws_random, uint8_t size)
{
    TRACE(1, "%s", __func__);

    uint8_t tempVal = 0;
    
    while (size--) {
        tempVal = rand()%(52);
        if(tempVal > 25)
        {
            *gws_random++ = tempVal -25 -1 + RAND_CHAR_LOW;
        }
        else
        {
            *gws_random++ = tempVal + RAND_CHAR_CAP;
        }
    }
}
#endif

/// COMMON handlers function definition
const struct ai_func_handler commonVoice_handler_function_list[] =
{
    {API_HANDLE_INIT,         (ai_handler_func_t)voice_init},
    {API_START_ADV,           (ai_handler_func_t)voice_start_advertising},
    {API_DATA_SEND,           (ai_handler_func_t)voice_mic_recv_data_cb},
    {CALLBACK_DATA_RECEIVE,   (ai_handler_func_t)voice_recv_proc_cb},
    {CALLBACK_AI_CONNECT,     (ai_handler_func_t)voice_connected},
    {CALLBACK_AI_DISCONNECT,  (ai_handler_func_t)voice_disconnected},
    {CALLBACK_UPDATE_MTU,     (ai_handler_func_t)NULL},
    {CALLBACK_WAKE_UP,        (ai_handler_func_t)NULL},
    {CALLBACK_AI_ENABLE,      (ai_handler_func_t)NULL},
    {CALLBACK_START_SPEECH,   (ai_handler_func_t)NULL},
    {CALLBACK_ENDPOINT_SPEECH,(ai_handler_func_t)NULL},
    {CALLBACK_STOP_SPEECH,    (ai_handler_func_t)NULL},
    {CALLBACK_DATA_SEND_DONE, (ai_handler_func_t)NULL},
    {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)NULL},
};

const struct ai_handler_func_table commonVoice_handler_function_table =
    {&commonVoice_handler_function_list[0], (sizeof(commonVoice_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_COMMON, &commonVoice_handler_function_table)

