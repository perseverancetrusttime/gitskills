/**
 ****************************************************************************************
 *
 * @file app_gma_cmd_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the gma voice command handler
 *
 * Copyright (C) 2017
 *
 *
 ****************************************************************************************
 */
#include "string.h"
#include "cmsis_os.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "apps.h"
#include "stdbool.h"
#include "app_gma_ble.h"
#include "app_gma_cmd_handler.h"
#include "app_gma_cmd_code.h"
#include "rwapp_config.h"
#include "app_gma_bt.h"
#include "ai_transport.h"
#include "ai_thread.h"
#include "app_gma_state_env.h"
#include "gma_crypto.h"

#ifdef __GMA_VOICE__

#define APP_GMA_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT    8
/**
 * @brief waiting response timeout supervision data structure
 *
 */
typedef struct
{
    uint16_t        entryIndex;         /**< The command waiting for the response */
    uint16_t        msTillTimeout;  /**< run-time timeout left milliseconds */
} APP_GMA_CMD_WAITING_RSP_SUPERVISOR_T;

/**
 * @brief gma voice command handling environment
 *
 */
typedef struct
{
    APP_GMA_CMD_WAITING_RSP_SUPERVISOR_T
        waitingRspTimeoutInstance[APP_GMA_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT];
    uint32_t    lastSysTicks;
    uint8_t     timeoutSupervisorCount;
    osTimerId   supervisor_timer_id;
    osMutexId   mutex;

} APP_GMA_CMD_HANDLER_ENV_T;

static APP_GMA_CMD_HANDLER_ENV_T gma_cmd_handler_env;

osMutexDef(app_gma_cmd_handler_mutex);

extern bool g_is_ota_packet;

static void app_gma_cmd_handler_rsp_supervision_timer_cb(void const *n);
osTimerDef (APP_GMA_CMD_HANDLER_RSP_SUPERVISION_TIMER, app_gma_cmd_handler_rsp_supervision_timer_cb);

static void app_gma_cmd_handler_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex);

/**
 * @brief Callback function of the waiting response supervisor timer.
 *
 */
static void app_gma_cmd_handler_rsp_supervision_timer_cb(void const *n)
{
    uint32_t entryIndex = gma_cmd_handler_env.waitingRspTimeoutInstance[0].entryIndex;

    app_gma_cmd_handler_remove_waiting_rsp_timeout_supervision(entryIndex);

    // it means time-out happens before the response is received from the peer device,
    // trigger the response handler
    CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex)->cmdRspHandler(WAITING_RSP_TIMEOUT, NULL, 0,0);
}

APP_GMA_CMD_INSTANCE_T* app_gma_cmd_handler_get_entry_pointer_from_cmd_code(APP_GMA_CMD_CODE_E cmdCode)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(APP_GMA_CMD_INSTANCE_T);index++) {
        if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode) {
            return CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    return NULL;
}

uint16_t app_gma_cmd_handler_get_entry_index_from_cmd_code(APP_GMA_CMD_CODE_E cmdCode)
{

    for (uint32_t index = 0;
        index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(APP_GMA_CMD_INSTANCE_T);index++) {
        if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode) {
            return index;
        }
    }

    return INVALID_CUSTOM_ENTRY_INDEX;
}

/**
 * @brief Refresh the waiting response supervisor list
 *
 */
static void app_gma_cmd_refresh_supervisor_env(void)
{
    // do nothing if no supervisor was added
    if (gma_cmd_handler_env.timeoutSupervisorCount > 0) {
        uint32_t currentTicks = GET_CURRENT_TICKS();
        uint32_t passedTicks;
        if (currentTicks >= gma_cmd_handler_env.lastSysTicks) {
            passedTicks = (currentTicks - gma_cmd_handler_env.lastSysTicks);
        } else {
            passedTicks = (hal_sys_timer_get_max() - gma_cmd_handler_env.lastSysTicks + 1) + currentTicks;
        }

        uint32_t deltaMs = TICKS_TO_MS(passedTicks);

        APP_GMA_CMD_WAITING_RSP_SUPERVISOR_T* pRspSupervisor = &(gma_cmd_handler_env.waitingRspTimeoutInstance[0]);
        for (uint32_t index = 0;index < gma_cmd_handler_env.timeoutSupervisorCount;index++) {
            if (pRspSupervisor[index].msTillTimeout > deltaMs) {
                pRspSupervisor[index].msTillTimeout -= deltaMs;
            } else {
                pRspSupervisor[index].msTillTimeout = 0;
            }
        }
    }

    gma_cmd_handler_env.lastSysTicks = GET_CURRENT_TICKS();
}

/**
 * @brief Remove the time-out supervision of waiting response
 *
 * @param entryIndex    Entry index of the command table
 *
 */
static void app_gma_cmd_handler_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
    ASSERT(gma_cmd_handler_env.timeoutSupervisorCount > 0,
        "%s The BLE custom command time-out supervisor is already empty!!!", __FUNCTION__);

    osMutexWait(gma_cmd_handler_env.mutex, osWaitForever);

    uint32_t index;
    for (index = 0;index < gma_cmd_handler_env.timeoutSupervisorCount;index++) {
        if (gma_cmd_handler_env.waitingRspTimeoutInstance[index].entryIndex == entryIndex) {
            memcpy(&(gma_cmd_handler_env.waitingRspTimeoutInstance[index]),
                &(gma_cmd_handler_env.waitingRspTimeoutInstance[index + 1]),
                (gma_cmd_handler_env.timeoutSupervisorCount - index - 1)*sizeof(APP_GMA_CMD_WAITING_RSP_SUPERVISOR_T));
            break;
        }
    }

    // cannot find it, directly return
    if (index == gma_cmd_handler_env.timeoutSupervisorCount) {
        goto exit;
    }

    gma_cmd_handler_env.timeoutSupervisorCount--;

    //TR_DEBUG(1,"decrease the supervisor timer to %d", gma_cmd_handler_env.timeoutSupervisorCount);

    if (gma_cmd_handler_env.timeoutSupervisorCount > 0) {
        // refresh supervisor environment firstly
        app_gma_cmd_refresh_supervisor_env();

        // start timer, the first entry is the most close one
        osTimerStart(gma_cmd_handler_env.supervisor_timer_id,
            gma_cmd_handler_env.waitingRspTimeoutInstance[0].msTillTimeout);
    } else {
        // no supervisor, directly stop the timer
        osTimerStop(gma_cmd_handler_env.supervisor_timer_id);
    }

exit:
    osMutexRelease(gma_cmd_handler_env.mutex);
}

/**
 * @brief Add the time-out supervision of waiting response
 *
 * @param entryIndex    Index of the command entry
 *
 */

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData       Pointer of the received data
 * @param dataLength    Length of the received data
 *
 * @return APP_GMA_CMD_RET_STATUS_E
 */
uint32_t app_gma_rcv_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t i_lock = 0;
    TRACE(3,"%s line:%d len:%d",__func__, __LINE__, param2);
    //DUMP8("%02x ",pBuf,length);
    i_lock = int_lock();
    if (EnCQueue(&ai_rev_buf_queue, (CQItemType *)param1, param2) == CQ_ERR)
    {
        ASSERT(false, "%s avail of queue %d cmd len %d", __func__, AvailableOfCQueue(&ai_rev_buf_queue), param2);
    }
    int_unlock(i_lock);
    ai_mailbox_put(SIGN_AI_CMD_COME, param2,AI_SPEC_ALI, param3); // -> app_gma_parse_cmd

    return 0;
}

uint32_t app_gma_parse_cmd(void *param1, uint32_t param2, uint8_t param3)
{
    uint32_t i_lock = 0;
    uint8_t srcDataBuf[sizeof(APP_GMA_CMD_PAYLOAD_T)];
    uint8_t dstDataBuf[sizeof(APP_GMA_CMD_PAYLOAD_T)];
    uint8_t* ptrData;
    i_lock = int_lock();
    if (DeCQueue(&ai_rev_buf_queue, srcDataBuf, param2))
    {
        int_unlock(i_lock);
        TRACE(3,"%s line:%d DeCQueue error len %d", __func__, __LINE__, param2);
        return 1;
    }
    int_unlock(i_lock);
    TRACE(3,"%s line:%d len:%d", __func__, __LINE__, param2);
    DUMP8("%02x ", srcDataBuf, param2);
    uint8_t  if_secuityed = srcDataBuf[0]&0x10;

    if (if_secuityed)
    {
        memcpy(dstDataBuf, srcDataBuf, GMA_CMD_CODE_SIZE);
        app_gma_decrypt(srcDataBuf + GMA_CMD_CODE_SIZE, dstDataBuf + GMA_CMD_CODE_SIZE, param2);
        ptrData = dstDataBuf;
    }
    else
    {
        ptrData = srcDataBuf;
    }

    APP_GMA_CMD_PAYLOAD_T* pPayload = (APP_GMA_CMD_PAYLOAD_T*)ptrData;
    TRACE(6,"%s data line:%d len %d,gma_cmd 0x%02x %d %d", __func__, __LINE__, param2, pPayload->cmdCode,
                pPayload->sequence,
                pPayload->length);
    DUMP8("%02x ", pPayload, param2);

    app_gma_state_env_msg_id_receive(GMA_GET_HEAD_MSG_ID(pPayload->header0));
    app_gma_state_env_encry_flag_receive(GMA_GET_HEAD_ENCRY_FLAG(pPayload->header0));
    app_gma_state_env_version_num_receive(GMA_GET_HEAD_VERSION_NUM(pPayload->header0));

    APP_GMA_CMD_INSTANCE_T* pInstance =
        app_gma_cmd_handler_get_entry_pointer_from_cmd_code((APP_GMA_CMD_CODE_E)(pPayload->cmdCode));

    if(NULL != pInstance)
    {
        pInstance->cmdHandler((APP_GMA_CMD_CODE_E)(pPayload->cmdCode), (pPayload->length?pPayload->param:NULL), pPayload->length, param3);
    }
    else
    {
        TRACE(3,"%s line:%d Wrong gma_cmd 0x%02x", __func__, __LINE__, pPayload->cmdCode);
        return 1;
    }

    return 0;
}

static bool app_gma_cmd_send(APP_GMA_CMD_PAYLOAD_T* ptPayLoad, uint32_t wholeLen, uint8_t device_id)
{
    uint8_t outputBuf[128];
    APP_GMA_CMD_PAYLOAD_T* ptrData = ptPayLoad;

    if ((app_gma_is_security_enabled()) && (false == g_is_ota_packet))
    {
        APP_GMA_CMD_PAYLOAD_T tmpData;
        uint8_t buf_to_encypt[APP_GMA_CMD_MAXIMUM_PAYLOAD_SIZE - 4*sizeof(uint8_t)] = {0};
        uint8_t pad_remain = (ptPayLoad->length)%16;
        uint8_t padding = pad_remain?(16 - pad_remain):16;

        memcpy(buf_to_encypt, (uint8_t *)ptPayLoad->param, ptPayLoad->length);
        if(padding)
        {
            memset(&buf_to_encypt[ptPayLoad->length], padding, padding);
        }

        tmpData.header0 = ptPayLoad->header0;
        tmpData.cmdCode = ptPayLoad->cmdCode;
        tmpData.sequence = ptPayLoad->sequence;
        tmpData.length = ptPayLoad->length + padding;

        wholeLen += padding;

        TRACE(4,"%s line:%d [before encrypt] len:%d, pad:%d", __func__, __LINE__, tmpData.length, padding);
        DUMP8("%02x ", buf_to_encypt, tmpData.length);

        app_gma_encrypt(buf_to_encypt, (uint8_t *)&tmpData.param, tmpData.length);
#if 0
        // decrypt to verify whether encryption is OK
        uint8_t tmp_decypt_out[100] = {0};
        app_gma_decrypt((uint8_t *)&tmpData.param, tmp_decypt_out, tmpData.length);
        TR_DEBUG(3,"%s line:%d [test encryption] this is decrypted data, len:%d", __func__, __LINE__, tmpData.length);
        DUMP8("%02x ", tmp_decypt_out, tmpData.length);
#endif
        ptrData = &tmpData;
    }
    TRACE(4,"%s line:%d len:%d %sencrypted", __func__, __LINE__, wholeLen, app_gma_is_security_enabled()? "":"not");
    DUMP8("%02x ", ptrData, wholeLen);
    *(uint16_t *)outputBuf = (uint16_t)wholeLen;
    memcpy((void *)&outputBuf[AI_CMD_CODE_SIZE], (void *)ptrData, wholeLen);
    ai_transport_cmd_put(outputBuf, (wholeLen+AI_CMD_CODE_SIZE));
    ai_mailbox_put(SIGN_AI_TRANSPORT, wholeLen, AI_SPEC_ALI, device_id);

    return true;
}

/**
 * @brief Send the gma voice command to the peer device
 *
 * @param cmdCode   Command code
 * @param ptrParam  Pointer of the output parameter
 * @param paramLen  Length of the output parameter
 * @param path      Path of the data transmission
 *
 * @return APP_GMA_CMD_RET_STATUS_E
 */
APP_GMA_CMD_RET_STATUS_E app_gma_send_command(APP_GMA_CMD_CODE_E cmdCode,
    uint8_t* ptrParam, uint32_t paramLen, bool isCmd, uint8_t device_id)
{
    // check cmdCode's validity
    if (cmdCode >= OP_COMMAND_COUNT)
    {
        return INVALID_CMD;
    }

    APP_GMA_CMD_PAYLOAD_T payload;

    // check parameter length
    if (paramLen > sizeof(payload.param))
    {
        return PARAM_LEN_OUT_OF_RANGE;
    }
    if(isCmd)
    {
        app_gma_state_env_msg_id_incre();
    }
    payload.header0 = app_gma_state_env_msg_id_get() | (app_gma_is_security_enabled()<<4);
    payload.cmdCode = cmdCode;
    payload.sequence = 0;
    payload.length = paramLen;
    memcpy(payload.param, ptrParam, paramLen);

    TRACE(4,"%s line:%d cmdCode:0x%02x len:%d", __func__, __LINE__, cmdCode, paramLen + GMA_CMD_CODE_SIZE);
    DUMP8("%02x ", &payload, paramLen + GMA_CMD_CODE_SIZE);
    app_gma_cmd_send(&payload, paramLen + GMA_CMD_CODE_SIZE, device_id);

    return NO_ERROR;
}

/**
 * @brief Initialize the gma voice command handler framework
 *
 */
void app_gma_cmd_handler_init(void)
{
    memset((uint8_t *)&gma_cmd_handler_env, 0, sizeof(gma_cmd_handler_env));

    gma_cmd_handler_env.supervisor_timer_id =
        osTimerCreate(osTimer(APP_GMA_CMD_HANDLER_RSP_SUPERVISION_TIMER), osTimerOnce, NULL);

    gma_cmd_handler_env.mutex = osMutexCreate((osMutex(app_gma_cmd_handler_mutex)));
}


#endif  // #if __GMA_VOICE__

