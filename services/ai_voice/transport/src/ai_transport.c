#include "kfifo.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "ai_transport.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "cqueue.h"
#include "voice_compression.h"
#include "app_ai_tws.h"
#include "app_ai_ble.h"
#include "ai_manager.h"

#ifdef IOS_IAP2_PROTOCOL
#include "bluetooth_iap2_link.h"
#endif

static struct kfifo ai_cmd_transport_fifo;
static unsigned char ai_cmd_transport_fifo_buff[AI_CMD_TRANSPORT_BUFF_FIFO_SIZE];

AI_TRANSPORT_DATA_T ai_send_cmd_t;
static osMutexId ai_transport_fifo_mutex_id = NULL;
osMutexDef(ai_transport_fifo_mutex);

static uint8_t ai_rev_buf[AI_REV_BUF_SIZE]; //ai recieve cmd buff
CQueue ai_rev_buf_queue;    //ai recieve buf queue

//bool (*ai_data_transport_func)(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);
//bool (*ai_cmd_transport_func)(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);


static void app_ai_rev_queue_reset_timeout_timer_cb(void const *n)
{
    TRACE(1,"%s", __func__);
    memset(ai_rev_buf, 0, sizeof(ai_rev_buf));
    InitCQueue(&ai_rev_buf_queue, sizeof(ai_rev_buf), (CQItemType *)(ai_rev_buf));
}

osTimerDef (APP_AI_REV_QUEUE_RESET_TIMEOUT, app_ai_rev_queue_reset_timeout_timer_cb); 
osTimerId   app_ai_rev_queue_reset_timeout_timer_id = NULL;

void LOCK_AI_TRANSPORT_FIFO(void)
{
    if(osMutexWait(ai_transport_fifo_mutex_id, osWaitForever) != osOK)
        TRACE(1,"%s Error", __func__);
}

void UNLOCK_AI_TRANSPORT_FIFO(void)
{
    if(osMutexRelease(ai_transport_fifo_mutex_id) != osOK)
        TRACE(1,"%s Error", __func__);
}

void ai_transport_fifo_init()
{
    if (ai_transport_fifo_mutex_id == NULL)
    {
        ai_transport_fifo_mutex_id = osMutexCreate((osMutex(ai_transport_fifo_mutex)));
    }
    if (app_ai_rev_queue_reset_timeout_timer_id == NULL)
    {
        app_ai_rev_queue_reset_timeout_timer_id = osTimerCreate(osTimer(APP_AI_REV_QUEUE_RESET_TIMEOUT), osTimerOnce, NULL);
    }

    memset(ai_rev_buf, 0, sizeof(ai_rev_buf));
    InitCQueue(&ai_rev_buf_queue, sizeof(ai_rev_buf), (CQItemType *)(ai_rev_buf));
    //ai_audio_stream_allowed_to_send_set(false, 0);
    LOCK_AI_TRANSPORT_FIFO();
    kfifo_init(&ai_cmd_transport_fifo, ai_cmd_transport_fifo_buff, sizeof(ai_cmd_transport_fifo_buff));
    UNLOCK_AI_TRANSPORT_FIFO();
}

void ai_transport_fifo_deinit(uint8_t ai_index)
{
    ai_struct[ai_index].send_num = 0;
    ai_struct[ai_index].tx_credit = MAX_TX_CREDIT;
}

void ai_data_transport_init(bool  (* cb)(uint8_t*,uint32_t, uint8_t, uint8_t), uint8_t ai_index, uint8_t transType)
{
    if(transType == AI_TRANSPORT_SPP)
    {
        ai_trans_handler[ai_index].ai_spp_data_transport_func=cb;
    }
    else
    {
        ai_trans_handler[ai_index].ai_ble_data_transport_func=cb;
    }
    //ai_data_transport_func=cb;
}

void ai_cmd_transport_init(bool  (* cb)(uint8_t*,uint32_t, uint8_t, uint8_t), uint8_t ai_index, uint8_t transType)
{
    if(transType == AI_TRANSPORT_SPP)
    {
        ai_trans_handler[ai_index].ai_spp_cmd_transport_func=cb;
    }
    else
    {
        ai_trans_handler[ai_index].ai_ble_cmd_transport_func=cb;
    }
    //ai_cmd_transport_func=cb;
}

bool ai_transport_cmd_value(void)
{
    uint8_t cmd_buf[AI_CMD_TRANSPORT_BUFF_FIFO_SIZE];
    uint16_t cmd_len = 0;
    uint16_t total_len = 0;
    uint8_t count_time = 0;

    LOCK_AI_TRANSPORT_FIFO();
    total_len = kfifo_len(&ai_cmd_transport_fifo);
    if (total_len <= AI_CMD_TRANSPORT_BUFF_FIFO_SIZE)
    {
        if (total_len == kfifo_peek_to_buf(&ai_cmd_transport_fifo, cmd_buf, total_len))
        {
            //DUMP8("%x ", cmd_buf, total_len);
            while (cmd_len < total_len)
            {
                cmd_len += (*(uint16_t *)&cmd_buf[cmd_len] + AI_CMD_CODE_SIZE);    //first two byte represent the cmd length
                if (count_time++ >= 50)
                {
                    break;
                }
            }
        }
    }
    UNLOCK_AI_TRANSPORT_FIFO();

    TRACE(4,"%s cmd len %d total %d count_time %d", __func__, cmd_len, total_len, count_time);
    if (cmd_len == total_len)
    {
        return true;
    }

    return false;
}

bool ai_transport_cmd_put(uint8_t *in_buf,uint16_t len)
{
    unsigned int ret = 0;

    TRACE(3,"%s len=%d buf=%p", __func__, len, in_buf);
#ifndef SLAVE_ADV_BLE
    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return false;
        }
    }
#endif
    if(in_buf==NULL)
    {
        TRACE(1,"%s the buf is null", __func__);
        return false;
    }

    LOCK_AI_TRANSPORT_FIFO();
    ret = kfifo_put(&ai_cmd_transport_fifo, in_buf, len);
    UNLOCK_AI_TRANSPORT_FIFO();
    if (ret != len)
    {
        ASSERT(false, "%s: AI transport fifo OVERFLOW len %d ret %d", __func__, len, ret);
    }

    return true;
}

bool ai_transport_has_cmd_to_send(void)
{
    bool ret = kfifo_len(&ai_cmd_transport_fifo)?true:false;
    return ret;
}

bool ai_transport_is_busy(uint8_t ai_index)
{
    bool ret = false;

#ifdef __IAG_BLE_INCLUDE__
    if (app_ai_get_transport_type(ai_index, ai_manager_get_foreground_ai_conidx()) == AI_TRANSPORT_BLE)
    {
        ret = app_ai_ble_notification_processing_is_busy(ai_index);
    }
#endif

    return ret;
}

static bool ai_transport_cmd(uint8_t ai_index, uint8_t dest_id)
{
    bool ret = false;
    uint16_t cmd_len=0;
    uint32_t send_len=0;

    TRACE(1,"%s send cmd", __func__);
    if (!ai_transport_cmd_value())
    {
        TRACE(0,"cmd fifo is error, need init");
        LOCK_AI_TRANSPORT_FIFO();
        kfifo_init(&ai_cmd_transport_fifo, ai_cmd_transport_fifo_buff, sizeof(ai_cmd_transport_fifo_buff));
        UNLOCK_AI_TRANSPORT_FIFO();
        goto cmd_trans_end;
    }
    
    LOCK_AI_TRANSPORT_FIFO();
    kfifo_peek_to_buf(&ai_cmd_transport_fifo, (uint8_t *)&cmd_len, AI_CMD_CODE_SIZE);
    if (cmd_len <= kfifo_len(&ai_cmd_transport_fifo) && cmd_len <= MAX_TX_BUFF_SIZE)
    {
        send_len = kfifo_peek_to_buf(&ai_cmd_transport_fifo, (uint8_t *)&ai_send_cmd_t, cmd_len+AI_CMD_CODE_SIZE);
        UNLOCK_AI_TRANSPORT_FIFO();
    }
    else
    {
        TRACE(1,"error cmd_len %d", cmd_len);
        cmd_len = kfifo_len(&ai_cmd_transport_fifo)>MAX_TX_BUFF_SIZE?MAX_TX_BUFF_SIZE:kfifo_len(&ai_cmd_transport_fifo);
        send_len = kfifo_peek_to_buf(&ai_cmd_transport_fifo, (uint8_t *)&ai_send_cmd_t, cmd_len+AI_CMD_CODE_SIZE);
        DUMP8("%x ", ai_send_cmd_t.data, send_len);
        kfifo_init(&ai_cmd_transport_fifo, ai_cmd_transport_fifo_buff, sizeof(ai_cmd_transport_fifo_buff));
        UNLOCK_AI_TRANSPORT_FIFO();
        goto cmd_trans_end;
    }

    if (cmd_len > app_ai_get_mtu(ai_index))
    {
        kfifo_get(&ai_cmd_transport_fifo, (uint8_t *)&ai_send_cmd_t, cmd_len+AI_CMD_CODE_SIZE);
        TRACE(2, "cmd_len %d too large, mtu is %d", cmd_len, app_ai_get_mtu(ai_index));
        goto cmd_trans_end;
    }

    if(0 == (GET_DEST_FLAG(dest_id)))
    {
         ret = ai_trans_handler[ai_index].ai_spp_cmd_transport_func(ai_send_cmd_t.data, cmd_len, ai_index, dest_id);
    }
    else
    {
         ret = ai_trans_handler[ai_index].ai_ble_cmd_transport_func(ai_send_cmd_t.data, cmd_len, ai_index, GET_BLE_CONIDX(dest_id));
    }

cmd_trans_end:
    if (ret == true)
    {
        kfifo_get(&ai_cmd_transport_fifo, (uint8_t *)&ai_send_cmd_t, cmd_len+AI_CMD_CODE_SIZE);
    }
    else
    {
        TRACE(1, "%s fail", __func__);
    }
    return ret;
}

static bool ai_transport_data(uint8_t ai_index, uint8_t dest_id)
{
    bool ret = false;
    uint32_t data_len=0;
    uint32_t send_len=0;

    if (!is_ai_audio_stream_allowed_to_send(ai_index))
    {
        TRACE(1,"WARN:%s don't allow", __func__);
        goto data_trans_end;
    }

    data_len = voice_compression_get_encode_buf_size();
    if (app_ai_get_data_trans_size(ai_index) > data_len)
    {
        TRACE(2,"%s data_len %d", __func__, data_len);
        goto data_trans_end;
    }

    data_len = voice_compression_peek_encoded_data(ai_send_cmd_t.data, \
                                                  app_ai_get_data_trans_size(ai_index));
    send_len = ai_function_handle(API_DATA_HANDLE, ai_send_cmd_t.data, data_len, ai_index, dest_id);

    if (send_len > app_ai_get_mtu(ai_index))
    {
        voice_compression_get_encoded_data(NULL, data_len);
        TRACE(2, "send_len %d too large, mtu is %d", send_len, app_ai_get_mtu(ai_index));
        goto data_trans_end;
    }

    TRACE(3,"%s data len %d send len %d", __func__, data_len, send_len);
#ifdef IOS_IAP2_PROTOCOL
    if (app_ai_connect_mobile_has_ios(ai_index))
    {
        ret = iap2_ea_send_data(ai_send_cmd_t.data, send_len, IAP2_AMA_IDENTIFIER);
    }
    else
#endif
    {
        if (0 == (GET_DEST_FLAG(dest_id)))
        {
            ret = ai_trans_handler[ai_index].ai_spp_data_transport_func(ai_send_cmd_t.data, send_len, ai_index, dest_id);
        }
        else
        {
            ret = ai_trans_handler[ai_index].ai_ble_data_transport_func(ai_send_cmd_t.data, send_len, ai_index, GET_BLE_CONIDX(dest_id));
        }
    }

data_trans_end:
    if (ret == true)
    {
        voice_compression_get_encoded_data(NULL, data_len);
    }
    else
    {
        TRACE(1, "ERR:%s fail", __func__);
    }

    return ret;
}

bool ai_transport_handler(uint8_t ai_index, uint8_t dest_id)
{
    bool ret = false;

#ifndef SLAVE_ADV_BLE
    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return false;
        }
    }
#endif

    if (ai_struct[ai_index].send_num) 
    {
        ai_struct[ai_index].send_num--;
    }

    if (0 == ai_struct[ai_index].tx_credit)
    {
        TRACE(2,"%s no txCredit ai_index=%d", __func__, ai_index);
        goto trans_end;
    }

    if (ai_transport_is_busy(ai_index))
    {
        TRACE(1,"%s is busy now", __func__);
        goto trans_end;
    }

    if (kfifo_len(&ai_cmd_transport_fifo))
    {
        ret = ai_transport_cmd(ai_index, dest_id);
    }
    else
    {
        ret = ai_transport_data(ai_index, dest_id);
    }

trans_end:
    if (true == ret)
    {
        ai_struct[ai_index].tx_credit--;
    }
    return ret;
}

