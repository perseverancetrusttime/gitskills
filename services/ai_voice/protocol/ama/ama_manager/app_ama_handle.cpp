#include <stdio.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "string.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "crc32_c.h"
#include "gap.h"
#include "voice_compression.h"
#include "app_audio.h"
#include "app_status_ind.h"
#include "apps.h"
#include "app_bt_func.h"
#include "app_ble_mode_switch.h"
#include "app_key.h"
#include "app_ai_if_thirdparty.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_ama_handle.h"
#include "app_ama_bt.h"
#include "app_ama_ble.h"
#include "ama_stream.h"
#include "app_ai_tws.h"
#include "app_bt_media_manager.h"

// mute the on-going a2dp streaming for some time when the ama start speech operation is triggered,
// if ama receives the media control pause request, stop the timer and clear the mute flag
// if ama is disconnected, stop the timer and clear the mute flag
// if ama stops the speech, stop the timer and clear the mute flag
// if timer expired, clear the mute flag

uint32_t app_ama_stop_speech(void *param1, uint32_t param2, uint8_t param3);

#define APP_AMA_UUID            "\x03\x16\x03\xFE"
#define APP_AMA_UUID_LEN        (4)

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
osTimerId ama_ind_rsp_timeout_timer = NULL;
static void ama_indication_rsp_timeout_handler(void const *param);
osTimerDef (AMA_IND_RSP_TIMEOUT_TIMER, ama_indication_rsp_timeout_handler);

static void ama_indication_rsp_timeout_handler(void const *param)
{
    TRACE(0,"ama_indication_rsp_timeout_handler");
    if (ai_manager_get_spec_update_flag())
    {
        app_bt_start_custom_function_in_bt_thread(0, 0, \
                (uint32_t)ai_manager_spec_update_start_reboot);
    }
}
#endif

static void app_ama_speech_state_timeout_timer_cb(void const *n)
{
    TRACE(1,"%s", __func__);
    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(NULL == ai_info) {
        return;
    }

    uint8_t dest_id = app_ai_get_dest_id(ai_info);

    if(app_ai_is_stream_opened(AI_SPEC_AMA))
    {
        app_ama_stop_speech(NULL, 0, dest_id);
    }
    ama_reset_connection(1, dest_id);
}

osTimerDef (APP_AMA_SPEECH_STATE_TIMEOUT, app_ama_speech_state_timeout_timer_cb); 
osTimerId   app_ama_speech_state_timeout_timer_id = NULL;

static void app_ama_endpoint_speech_timeout_timer_cb(void const *n)
{
    TRACE(2,"%s %d", __func__, is_ai_can_wake_up(AI_SPEC_AMA));
    if(!is_ai_can_wake_up(AI_SPEC_AMA))
    {
        AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
        if(NULL == ai_info) {
            return;
        }

        uint8_t dest_id = app_ai_get_dest_id(ai_info);
        ama_endpoint_speech_request(0, dest_id);
        app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
        app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);
    }
}

#define APP_AMA_ENDPOINT_SPEECH_TIMEOUT_INTERVEL    1000
osTimerDef (APP_AMA_ENDPOINT_SPEECH_TIMEOUT, app_ama_endpoint_speech_timeout_timer_cb); 
osTimerId   app_ama_endpoint_speech_timeout_timer_id = NULL;

static uint32_t app_ama_start_advertising(void *param1, uint32_t param2, uint8_t param3)
{
    BLE_ADV_PARAM_T *cmd = (BLE_ADV_PARAM_T*)param1;
    TRACE(1, "%s", __func__);

    if (app_ai_dont_have_mobile_connect(AI_SPEC_AMA))
    {
        TRACE(1, "%s no mobile connect", __func__);
        return 0;
    }
    else if (!app_ai_connect_mobile_has_ios(AI_SPEC_AMA))
    {
        TRACE(1, "%s no ios connect", __func__);
        return 0;
    }

    memcpy(&cmd->advData[cmd->advDataLen],
        APP_AMA_UUID, APP_AMA_UUID_LEN);
    cmd->advDataLen += APP_AMA_UUID_LEN;

    return 1;
}

void app_ama_voice_connected(void *param1, uint32_t param2, uint32_t param3)
{
    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1;
    AI_TRANS_TYPE_E ai_trans_type = (AI_TRANS_TYPE_E)param2;
    uint8_t dest_id = (uint8_t)param3;
    TRACE(2,"%s connect_type %d",__func__, ai_trans_type);

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            if(ai_trans_type == AI_TRANSPORT_SPP){
                app_ai_connect_handle(ai_trans_type, AI_SPEC_AMA, dest_id, btaddr->address);

                ai_data_transport_init(app_ai_spp_send, AI_SPEC_AMA, ai_trans_type);
                ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_AMA, ai_trans_type);
                app_ai_set_mtu(TX_SPP_MTU_SIZE, AI_SPEC_AMA);
                app_ai_set_data_trans_size(SPP_TRANS_SIZE, AI_SPEC_AMA);
            }
            return;
        }
    }

    if (ai_trans_type == AI_TRANSPORT_BLE && !app_ai_connect_mobile_has_ios(AI_SPEC_AMA))
    {
        TRACE(1,"%s no mobile connected or is ANDROID", __func__);
        return;
    }

    app_ai_connect_handle(ai_trans_type, AI_SPEC_AMA, dest_id, btaddr->address);

    if(ai_trans_type == AI_TRANSPORT_SPP){
        ai_data_transport_init(app_ai_spp_send, AI_SPEC_AMA, ai_trans_type);
        ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_AMA, ai_trans_type);
        app_ai_set_mtu(TX_SPP_MTU_SIZE, AI_SPEC_AMA);
        app_ai_set_data_trans_size(SPP_TRANS_SIZE, AI_SPEC_AMA);
    }
#ifdef __AI_VOICE_BLE_ENABLE__
    else if(ai_trans_type == AI_TRANSPORT_BLE){
        ai_data_transport_init(app_ama_send_via_notification, AI_SPEC_AMA, ai_trans_type);
        ai_cmd_transport_init(app_ama_send_via_notification, AI_SPEC_AMA, ai_trans_type);
        app_ai_set_data_trans_size(BLE_TRANS_SIZE, AI_SPEC_AMA);
    }
#endif

    //if(!app_ai_is_setup_complete(AI_SPEC_AMA))
    //{
        ama_control_send_transport_ver_exchange(param3);
    //}

    if (app_ama_speech_state_timeout_timer_id)
    {
        osTimerStop(app_ama_speech_state_timeout_timer_id);
    }

}

void app_ama_manual_disconnect(uint8_t deviceID)
{
    app_ai_spp_disconnlink(AI_SPEC_AMA, deviceID);
}

void app_ama_voice_disconnected(void *param1, uint32_t param2, uint8_t param3)
{
    AI_TRANS_TYPE_E ai_trans_type = (AI_TRANS_TYPE_E)param2;
    TRACE(1,"%s type %d", __func__, ai_trans_type);

    app_ai_disconnect_handle(ai_trans_type, AI_SPEC_AMA);
}

void app_ama_config_mtu(void *param1, uint32_t param2, uint8_t param3)  
{
    TRACE(3, "%s %d, %d", __func__, param2, param3);
    app_ai_set_mtu(param2, AI_SPEC_AMA);
}

uint32_t app_ama_handle_init(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"enter %s", __func__);

    app_ama_speech_state_timeout_timer_id = osTimerCreate(osTimer(APP_AMA_SPEECH_STATE_TIMEOUT), osTimerOnce, NULL);
    app_ama_endpoint_speech_timeout_timer_id = osTimerCreate(osTimer(APP_AMA_ENDPOINT_SPEECH_TIMEOUT), osTimerOnce, NULL);

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    ama_ind_rsp_timeout_timer = osTimerCreate(osTimer(AMA_IND_RSP_TIMEOUT_TIMER), osTimerOnce, NULL);
#endif

    TRACE(2,"%s %d ai_stream_opened set false",__func__,__LINE__);

    return 0;
}


void app_ama_parse_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t leftData = LengthOfCQueue(&ai_rev_buf_queue);
    uint8_t dataBuf[128];
    uint8_t tmp[sizeof(ama_stream_header_t)];
    static ama_stream_header_t header;
    uint16_t i = 0;
    uint16_t cmd_id = (AMA_VER << 12)|(AMA_CONTROL_STREAM_ID<<7)|(AMA_FLAG<<1);
    uint16_t cmd_head;
    uint16_t stream_header_size = sizeof(ama_stream_header_t)-1;
    uint16_t temp_peekbuf_size;
    int status = 0;
    bool find_cmd_head = false;

    while (leftData > 0) {
        if (leftData < stream_header_size) {
            // check whether the header is correct
            TRACE(2,"%s leftData is not enough %d", __func__, leftData);
            goto ama_parse_end;
        } else {
            temp_peekbuf_size = (leftData >= 128)?128:leftData;
            memset(dataBuf, 0, sizeof(dataBuf));
            if(PeekCQueueToBuf(&ai_rev_buf_queue, dataBuf, temp_peekbuf_size)) {
                TRACE(2,"%s PeekCQueueToBuf error %d", __func__, temp_peekbuf_size);
                status = 1;
                goto ama_parse_end;
            }

            //TRACE(1,"%s ai_rev_buf_queue is:", __func__);
            //DUMP8("%02x ", dataBuf, leftData);
            find_cmd_head = false;
            for (i = 0; i < (temp_peekbuf_size-1); i++) {
                cmd_head = ((uint16_t)dataBuf[i]<<8)|dataBuf[i+1];
                if ((cmd_head == cmd_id) || (cmd_head == (cmd_id+1)))
                {
                    find_cmd_head = true;
                    break;
                }
            }

            if (!find_cmd_head)
            {
                TRACE(2,"%s can't find_cmd_head i %d", __func__, i);
                status = 1;
                ResetCQueue(&ai_rev_buf_queue);
                goto ama_parse_end;
            }

            leftData = leftData - i;
            if(leftData < stream_header_size) {
                TRACE(2,"%s leftData too short %d", __func__, leftData);
                status = 1;
                goto ama_parse_end;
            }

            if(DeCQueue(&ai_rev_buf_queue, NULL, i)) {
                TRACE(2,"%s DeCQueue error i %d", __func__, i);
                status = 1;
                goto ama_parse_end;
            }

            if(PeekCQueueToBuf(&ai_rev_buf_queue, tmp, stream_header_size)) {
                TRACE(2,"%s PeekCQueueToBuf error head_s %d", __func__, stream_header_size);
                status = 1;
                goto ama_parse_end;
            }
            if(tmp[1]&0x01) {
                if(PeekCQueueToBuf(&ai_rev_buf_queue, tmp, stream_header_size+1)) {
                    TRACE(2,"%s PeekCQueueToBuf error head_s %d", __func__, stream_header_size);
                    status = 1;
                    goto ama_parse_end;
                }
            }

            if(false == ama_stream_header_parse(tmp, &header)) {
                TRACE(2,"%s ama_stream_header_parse error head_id 0x%x", __func__, header.streamId);
                status = 1;
                goto ama_parse_end;
            }
            
            if (header.length > (LengthOfCQueue(&ai_rev_buf_queue) - stream_header_size - header.use16BitLength)) {
                TRACE(2,"%s length error %d", __func__, header.length);
                status = 1;
                goto ama_parse_end;
            } else {
                memset(dataBuf, 0, sizeof(dataBuf));
                if(DeCQueue(&ai_rev_buf_queue, NULL, (stream_header_size + header.use16BitLength))) {
                    TRACE(1,"%s DeCQueue error", __func__);
                    status = 1;
                    goto ama_parse_end;
                }
                if(DeCQueue(&ai_rev_buf_queue, dataBuf, header.length)) {
                    TRACE(2,"%s DeCQueue error length %d", __func__, header.length);
                    status = 1;
                    goto ama_parse_end;
                }
                TRACE(1,"len %d Raw data is:", header.length);
                DUMP8("%02x ", dataBuf, header.length);
                TRACE(2,"Stream id is %d%s", header.streamId, ama_streamID2str((STREAM_ID_E)header.streamId));

                if (AMA_CONTROL_STREAM_ID == header.streamId) {
                    ama_control_stream_handler(dataBuf, header.length, param3);
                }
            }
        }

        leftData = LengthOfCQueue(&ai_rev_buf_queue);
    }

ama_parse_end:
    if(status) {
        osTimerStart(app_ai_rev_queue_reset_timeout_timer_id, APP_AI_REV_QUEUE_RESET_TIMEOUT_INTERVEL);
    }
    return;
}

uint32_t app_ama_rcv_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s len=%d",__func__, param2);
    //DUMP8("%02x ", param1, param2);

    osTimerStop(app_ai_rev_queue_reset_timeout_timer_id);
    if (EnCQueue(&ai_rev_buf_queue, (CQItemType *)param1, param2) == CQ_ERR)
    {
        ASSERT(false, "%s avail of queue %d cmd len %d", __func__, AvailableOfCQueue(&ai_rev_buf_queue), param2);
    }

    ai_mailbox_put(SIGN_AI_CMD_COME, 0, AI_SPEC_AMA, param3);

    return 0;
}

uint32_t app_ama_stream_init(void *param1, uint32_t param2, uint8_t param3)
{
    return 0;
}

void app_ama_stream_deinit(void *param1, uint32_t param2, uint8_t param3)
{
    osTimerStop(app_ama_endpoint_speech_timeout_timer_id);
}

void app_ama_reset(void *param1, uint32_t param2, uint8_t param3)
{
    if (app_is_ai_voice_connected(AI_SPEC_AMA))
    {
        ama_reset_connection(param2, param3);
    }
}

uint32_t app_ama_start_speech(void *param1, uint32_t param2, uint8_t param3)
{
    if (app_ai_is_stream_opened(AI_SPEC_AMA) && app_ai_voice_is_in_upstream_state())
    {
        TRACE(1, "%s already open", __func__);
        return 1;
    }
    else
    {
        app_ai_set_stream_opened(true, AI_SPEC_AMA);
        app_ai_voice_update_handle_frame_len(AMA_VOICE_ONESHOT_PROCESS_LEN);
        app_ai_voice_stream_init(AIV_USER_AMA);
        app_ai_voice_upstream_control(true, AIV_USER_AMA, param2);
        app_ai_spp_deinit_tx_buf();
        ama_start_speech_request(param3);
    }

    return 0;
}

uint32_t app_ama_stop_speech(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(1,"%s",__func__);

    if((!app_ai_is_setup_complete(AI_SPEC_AMA)) ||
        (AI_TRANSPORT_IDLE == app_ai_get_transport_type(AI_SPEC_AMA, ai_manager_get_foreground_ai_conidx())))
    {
        TRACE(1,"%s no ama connect", __func__);
        return 1;
    }

    if(!app_ai_is_stream_opened(AI_SPEC_AMA))
    {
        TRACE(1,"%s ama stream don't open", __func__);
        return 2;
    }

    app_ai_voice_update_handle_frame_len(AI_VOICE_ONESHOT_PROCESS_MAX_LEN);
    app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);
    ama_stop_speech_request(USER_CANCEL, param3);

    return 0;
}

uint32_t app_ama_endpoint_speech(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s %d", __func__, is_ai_can_wake_up(AI_SPEC_AMA));
    
    if(!is_ai_can_wake_up(AI_SPEC_AMA))
    {
        osTimerStop(app_ama_endpoint_speech_timeout_timer_id);
        osTimerStart(app_ama_endpoint_speech_timeout_timer_id, APP_AMA_ENDPOINT_SPEECH_TIMEOUT_INTERVEL);
    }

    return 0;
}

uint32_t app_ama_wake_up(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s can_wake_up %d", __func__, is_ai_can_wake_up(AI_SPEC_AMA));

    if (app_ai_is_role_switching(AI_SPEC_AMA))
    {
        TRACE(1,"%s ai need role switch", __func__);
        return 1;
    }

    if (!app_ai_is_setup_complete(AI_SPEC_AMA) || (AI_TRANSPORT_IDLE == app_ai_get_transport_type(AI_SPEC_AMA, ai_manager_get_foreground_ai_conidx())))
    {
        TRACE(1,"%s no ama connect", __func__);
        return 1;
    }

    if (app_ai_is_in_multi_ai_mode())
    {
        if (0 == ai_manager_get_spec_connected_status(AI_SPEC_AMA))
        {
            TRACE(1,"%s  APP not connected!", __func__);
            return 1;
        }
    }

    if (app_ai_is_sco_mode())
    {
        TRACE(1,"%s is_sco_mode", __func__);
        return 1;
    }

    if (is_ai_can_wake_up(AI_SPEC_AMA))
    {
        app_ama_start_speech(NULL, param2, param3);
        ai_mailbox_put(SIGN_AI_WAKEUP, 0, AI_SPEC_AMA, param3);
        ai_set_can_wake_up(false, AI_SPEC_AMA);
    }
    else
    {
        app_ama_stop_speech(NULL, 0, param3);
        return 1;
    }

    return 0;
}

uint32_t app_ama_send_voice_stream(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    if(!is_ai_audio_stream_allowed_to_send(AI_SPEC_AMA)) {
        TRACE(2,"%s ama don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
        return 1;
    }

    if (encodedDataLength < app_ai_get_data_trans_size(AI_SPEC_AMA)) {
        return 1;
    }

    TRACE(3,"%s len %d credit %d", __func__, encodedDataLength, ai_struct[AI_SPEC_AMA].tx_credit);

    if(0 == ai_struct[AI_SPEC_AMA].tx_credit) {
        return 1;
    }

    ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_AMA), AI_SPEC_AMA, param3);

    return 0;
}

uint32_t app_ama_voice_send_done(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s credit %d", __func__, ai_struct[AI_SPEC_AMA].tx_credit);
    if(ai_struct[AI_SPEC_AMA].tx_credit < MAX_TX_CREDIT) {
        ai_struct[AI_SPEC_AMA].tx_credit++;
    }

    if ((app_ai_get_data_trans_size(AI_SPEC_AMA) <= voice_compression_get_encode_buf_size()) ||
        ai_transport_has_cmd_to_send())
    {
        ai_mailbox_put(SIGN_AI_TRANSPORT, app_ai_get_data_trans_size(AI_SPEC_AMA), AI_SPEC_AMA, param3);
    }

    return 0;
}

uint32_t app_ama_enable_handle(void *param1, uint32_t param2, uint8_t param3)
{
    TRACE(2,"%s %d", __func__, param2);

    if (param2)
    {
        ama_notify_device_configuration(0, param3);
    }
    else
    {
        if (app_ai_is_use_thirdparty(AI_SPEC_AMA))
        {
            app_ai_voice_upstream_control(false, AIV_USER_AMA, 0);
            app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS, THIRDPARTY_AI_DISCONNECT, AI_SPEC_AMA);
        }

        ama_notify_device_configuration(1, param3);
        ama_reset_connection(0, param3);
    }
    app_ai_if_ble_set_adv(AI_BLE_ADVERTISING_INTERVAL);

    return 0;
}

uint32_t app_ama_key_event_handle(void *param1, uint32_t param2, uint8_t param3)
{
    APP_KEY_STATUS *status = (APP_KEY_STATUS *)param1;

    TRACE(4,"%s code 0x%x event %d wakeup_type %d", __func__, status->code, status->event,app_ai_get_wake_up_type(AI_SPEC_AMA));

    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return 1;
    }

    if (AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD == app_ai_get_wake_up_type(AI_SPEC_AMA))
    {
        if ((APP_KEY_EVENT_FIRST_DOWN == status->event) && \
            (true == is_ai_can_wake_up(AI_SPEC_AMA)))
        {
            return app_ama_wake_up(status, 0, param3);
        }
        else if ((APP_KEY_EVENT_UP == status->event) && \
                (false == is_ai_can_wake_up(AI_SPEC_AMA)))
        {
            app_ama_endpoint_speech(status, 0, param3);
            return 1;
        }
    }
    else
    {
        if ((APP_KEY_EVENT_CLICK == status->event) && \
            (true == is_ai_can_wake_up(AI_SPEC_AMA)))
        {
            return app_ama_wake_up(status, 0, param3);
        }
    }

    return 1;
}

/// AMA handlers function definition
const struct ai_func_handler ama_handler_function_list[] =
{
    {API_HANDLE_INIT,           (ai_handler_func_t)app_ama_handle_init},
    {API_START_ADV,             (ai_handler_func_t)app_ama_start_advertising},
    {API_DATA_HANDLE,           (ai_handler_func_t)app_ama_audio_stream_handle},
    {API_DATA_SEND,             (ai_handler_func_t)app_ama_send_voice_stream},
    {API_DATA_INIT,             (ai_handler_func_t)app_ama_stream_init},
    {API_DATA_DEINIT,           (ai_handler_func_t)app_ama_stream_deinit},
    {API_AI_RESET,              (ai_handler_func_t)app_ama_reset},
    {CALLBACK_CMD_RECEIVE,      (ai_handler_func_t)app_ama_rcv_cmd},
    {CALLBACK_DATA_RECEIVE,     (ai_handler_func_t)NULL},
    {CALLBACK_DATA_PARSE,       (ai_handler_func_t)app_ama_parse_cmd},
    {CALLBACK_AI_CONNECT,       (ai_handler_func_t)app_ama_voice_connected},
    {CALLBACK_AI_DISCONNECT,    (ai_handler_func_t)app_ama_voice_disconnected},
    {CALLBACK_UPDATE_MTU,       (ai_handler_func_t)app_ama_config_mtu},
    {CALLBACK_WAKE_UP,          (ai_handler_func_t)app_ama_wake_up},
    {CALLBACK_AI_ENABLE,        (ai_handler_func_t)app_ama_enable_handle},
    {CALLBACK_START_SPEECH,     (ai_handler_func_t)app_ama_start_speech},
    {CALLBACK_ENDPOINT_SPEECH,  (ai_handler_func_t)app_ama_endpoint_speech},
    {CALLBACK_STOP_SPEECH,      (ai_handler_func_t)app_ama_stop_speech},
    {CALLBACK_DATA_SEND_DONE,   (ai_handler_func_t)app_ama_voice_send_done},
    {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)app_ama_key_event_handle},
};

const struct ai_handler_func_table ama_handler_function_table =
    {&ama_handler_function_list[0], (sizeof(ama_handler_function_list)/sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_AMA, &ama_handler_function_table)


