/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *Leonardo
 ****************************************************************************/
/*
 * Note that this code is mostly lifted from BES ota_handler.cpp but rearranged
 * to connect with the GSound library.  Also removed connection and protocol
 * related items since GSound defines an independent protocol.
 */
 
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "app_gma_ota_tws.h"
#include "app_gma_ota.h"
#include "hal_trace.h"
#include "app_tws_ctrl_thread.h"
#include "app_ibrt_ota_cmd.h"
#include "ai_thread.h"

GMA_OTA_TWS_ENV_T gma_ota_tws_env;
static void          gma_ota_tws_thread(const void *arg);
osThreadDef(gma_ota_tws_thread, osPriorityNormal, 1, 2046, "gma_ota_tws");
osThreadId gma_ota_tws_env_thread_id = NULL;

osMutexId twsTxQueueMutexID = NULL;
osMutexDef(gmaTwsTxQueueMutex);

osMutexId twsRxQueueMutexID = NULL;
osMutexDef(gmaTwsRxQueueMutex);

uint8_t tempRxBuf[GMA_MAX_MTU];

osMutexId app_gma_ota_tws_get_tx_queue_mutex(void)
{
    return twsTxQueueMutexID;
}

void lock_gma_tws_tx_queue(void)
{
    osMutexWait(twsTxQueueMutexID, osWaitForever);
}

void unlock_gma_tws_tx_queue(void)
{
    osMutexRelease(twsTxQueueMutexID);
}

osMutexId app_gma_ota_tws_get_rx_queue_mutex(void)
{
    return twsRxQueueMutexID;
}

void lock_gma_tws_rx_queue(void)
{
    osMutexWait(twsRxQueueMutexID, osWaitForever);
}

void unlock_gma_tws_rx_queue(void)
{
    osMutexRelease(twsRxQueueMutexID);
}

static void app_gma_ota_tws_pop_data_from_cqueue(CQueue *ptrQueue, osMutexId queueMutex, uint8_t *buff, uint32_t len)
{
    uint8_t *    e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    osMutexWait(queueMutex, osWaitForever);
    PeekCQueue(ptrQueue, len, &e1, &len1, &e2, &len2);

    if (buff == NULL)
    {
        DeCQueue(ptrQueue, 0, len1 + len2);
    }

    if (len == (len1 + len2))
    {
        memcpy(buff, e1, len1);
        memcpy(buff + len1, e2, len2);
        DeCQueue(ptrQueue, 0, len);
    }
    else
    {
        memset(buff, 0x00, len);
    }
    osMutexRelease(queueMutex);
}

void app_gma_ota_tws_push_data_into_cqueue(CQueue *ptrQueue, osMutexId queueMutex, uint8_t *buff, uint32_t len)
{
    osMutexWait(queueMutex, osWaitForever);
    EnCQueue(ptrQueue, ( CQItemType * )buff, len);
    osMutexRelease(queueMutex);
}

void app_gma_ota_tws_transfer(uint8_t *pDataBuf, uint16_t dataLength)
{
    TRACE(1,"[%s]", __func__);
    //app_gma_ota_tws_transfer_data(pDataBuf, dataLength);
    tws_ctrl_send_cmd(IBRT_OTA_TWS_IMAGE_BUFF, pDataBuf, dataLength);
}

void app_gma_ota_update_peer_result(APP_GMA_CMD_RET_STATUS_E ota_result)
{
    gma_ota_tws_env.peerResult = ota_result;
}

APP_GMA_CMD_RET_STATUS_E app_gma_ota_get_peer_result(void)
{
    return gma_ota_tws_env.peerResult;
}

static void gma_ota_tws_thread(const void *arg)
{
    uint8_t txBuf[GMA_OTA_TWS_BUFSIZE];
    volatile uint16_t qLen = 0;

    while (1)
    {
        lock_gma_tws_tx_queue();
        qLen = LengthOfCQueue(&gma_ota_tws_env.txQueue);
        unlock_gma_tws_tx_queue();
        TRACE(1,"there is %d data in tx queue.", qLen);
        if (qLen)
        {
            ASSERT(qLen <= GMA_OTA_TWS_BUFSIZE, "tws data packet overflowed!!!");

            app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.txQueue, twsTxQueueMutexID, txBuf, qLen);
            TRACE(1,"%d data will be sent to peer.", qLen);
            app_gma_ota_tws_transfer(txBuf, qLen);
            memset(txBuf, 0, GMA_OTA_TWS_BUFSIZE);
            qLen = 0;
        }
        else
        {
            gma_ota_tws_env.txThreadId = osThreadGetId();
            osSignalWait((1 << GMA_THREAD_SIGNAL), osWaitForever);
        }
    }
}

void app_gma_ota_create_tws_thread(void)
{
    TRACE(1,"[%s]", __func__);
    gma_ota_tws_env.txThreadId = osThreadCreate(osThread(gma_ota_tws_thread), NULL);
}

void app_gma_ota_tws_init_tx_buf(uint8_t *tx_buf, unsigned int size)
{
    TRACE(2,"[%s],size=%d", __func__, size);
    lock_gma_tws_tx_queue();
    InitCQueue(&(gma_ota_tws_env.txQueue), size, ( CQItemType * )tx_buf);
    memset(tx_buf, 0, size);
    unlock_gma_tws_tx_queue();
}

void app_gma_ota_tws_init_rx_buf(uint8_t *rxBuf, unsigned int size)
{
    TRACE(2,"[%s],size=%d", __func__, size);
    lock_gma_tws_rx_queue();
    InitCQueue(&(gma_ota_tws_env.rxQueue), size, ( CQItemType * )rxBuf);
    memset(rxBuf, 0, size);
    unlock_gma_tws_rx_queue();
}

void app_gma_ota_tws_init(void)
{
    TRACE(1,"[%s]", __func__);
    app_gma_ota_tws_init_tx_buf(gma_ota_tws_env.txBuf, GMA_OTA_TWS_BUFSIZE);
    app_gma_ota_tws_init_rx_buf(gma_ota_tws_env.rxBuf, GMA_MAX_MTU);
    app_gma_ota_create_tws_thread();

    if (twsTxQueueMutexID == NULL)
        twsTxQueueMutexID = osMutexCreate((osMutex(gmaTwsTxQueueMutex)));

    if (twsRxQueueMutexID == NULL)
        twsRxQueueMutexID = osMutexCreate((osMutex(gmaTwsRxQueueMutex)));
}

void app_gma_ota_tws_rsp( APP_GMA_CMD_RET_STATUS_E ota_result)
{
    TRACE(1,"[%s]", __func__);

    GMA_OTA_TWS_DATA_T tRsp;
    tRsp.packetType            = GMA_OTA_RSP_RESULT;
    tRsp.magicCode             = gma_ota_tws_env.currentReceivedMagicCode;
    tRsp.length                = sizeof(GMA_OTA_TWS_RSP_T);
    GMA_OTA_TWS_RSP_T *temp = ( GMA_OTA_TWS_RSP_T * )&tRsp.data;
    temp->ota_result           = ota_result;

    uint16_t frameLen = sizeof(GMA_OTA_TWS_RSP_T) + GMA_OTA_TWS_HEAD_SIZE;

    TRACE(0,"tws response data:");
    DUMP8("0x%02x ", ( uint8_t * )&tRsp, frameLen);

    lock_gma_tws_tx_queue();
    EnCQueue(&gma_ota_tws_env.txQueue, ( CQItemType * )&tRsp, frameLen);
    unlock_gma_tws_tx_queue();

    osSignalSet(gma_ota_tws_env.txThreadId, (1 << GMA_THREAD_SIGNAL));
}

static bool app_gma_ota_tws_frame_validity_check(uint8_t *dataPtr, uint32_t length)
{
    bool isValid = false;

    GMA_OTA_TWS_DATA_T *otaTwsData = ( GMA_OTA_TWS_DATA_T * )dataPtr;

    // TODO: add more validity check conditions
    ASSERT(GMA_OTA_TWS_BUFSIZE >= length, "received overloaded data packet!!!");
    if (GMApacketIncompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode &&
        otaTwsData->packetType != gma_ota_tws_env.currentReceivedPacketType)
    {
        ASSERT(0, "bad frame is received!!!");
    }

    isValid = (length == otaTwsData->length  + GMA_OTA_TWS_HEAD_SIZE);

    if (GMApacketCompleteMagicCode != otaTwsData->magicCode &&
        GMApacketIncompleteMagicCode != otaTwsData->magicCode)
    {
        TRACE(1,"Warning: invalid magic code is 0x%08x", otaTwsData->magicCode);
        isValid = false;
    }

    return isValid;
}

void app_gma_ota_tws_peer_data_handler(uint8_t *ptrParam, uint32_t paramLen)
{
    TRACE(3,"[%s] paramLen = %d, packtype = 0x%x", __func__, paramLen, ptrParam[0]);

    APP_GMA_CMD_RET_STATUS_E           status;
    uint16_t               packetLen  = 0;
    GMA_OTA_TWS_DATA_T *otaTwsData = ( GMA_OTA_TWS_DATA_T * )ptrParam;

    if (app_gma_ota_tws_frame_validity_check(ptrParam, paramLen))
    {
        TRACE(0,"ota tws data frame is valid.");
        gma_ota_tws_env.currentReceivedMagicCode  = otaTwsData->magicCode;
        gma_ota_tws_env.currentReceivedPacketType = otaTwsData->packetType;

        switch (otaTwsData->packetType)
        {
        case GMA_OTA_RSP_RESULT:
            status = (( GMA_OTA_TWS_RSP_T * )otaTwsData->data)->ota_result;
            app_gma_ota_update_peer_result(status);
            osSignalSet(gma_ota_tws_env.rxThreadId, (1 << GMA_SIGNAL_NUM));
            break;

        case GMA_OTA_BEGIN:
            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.rxQueue,
                                                 twsRxQueueMutexID,
                                                 otaTwsData->data,
                                                 paramLen - GMA_OTA_TWS_HEAD_SIZE);

            if (GMApacketCompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode)
            {
                lock_gma_tws_rx_queue();
                packetLen = LengthOfCQueue(&gma_ota_tws_env.rxQueue);
                unlock_gma_tws_rx_queue();
                app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.rxQueue, twsRxQueueMutexID, tempRxBuf, packetLen);
                //GMA_OTA_CMD_BEGIN_T *begin = ( GMA_OTA_CMD_BEGIN_T * )tempRxBuf;

                app_gma_ota_start_handler(GMA_OP_OTA_UPDATE_CMD,tempRxBuf,packetLen,0);
            }
            else
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }

            break;

        case GMA_OTA_DATA:
            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.rxQueue,
                                                 twsRxQueueMutexID,
                                                 otaTwsData->data,
                                                 paramLen - GMA_OTA_TWS_HEAD_SIZE);

            if (GMApacketCompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode)
            {
                lock_gma_tws_rx_queue();
                packetLen = LengthOfCQueue(&gma_ota_tws_env.rxQueue);
                unlock_gma_tws_rx_queue();
                app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.rxQueue, twsRxQueueMutexID, tempRxBuf, packetLen);

                app_gma_ota_upgrade_data_handler(GMA_OP_OTA_SEND_PKG_CMD,tempRxBuf, packetLen,0);
            }
            else
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }

            break;

        case GMA_OTA_CRC:
            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.rxQueue,
                                                 twsRxQueueMutexID,
                                                 otaTwsData->data,
                                                 paramLen - GMA_OTA_TWS_HEAD_SIZE);

            if (GMApacketCompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode)
            {
                lock_gma_tws_rx_queue();
                packetLen = LengthOfCQueue(&gma_ota_tws_env.rxQueue);
                unlock_gma_tws_rx_queue();
                app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.rxQueue, twsRxQueueMutexID, tempRxBuf, packetLen);

                app_gma_ota_whole_crc_validate_handler(GMA_OP_OTA_COMP_NTF_CMD,tempRxBuf,packetLen,0);
            }
            else
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }

            break;
            
        case GMA_OTA_APPLY:
            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.rxQueue,
                                                 twsRxQueueMutexID,
                                                 otaTwsData->data,
                                                 paramLen - GMA_OTA_TWS_HEAD_SIZE);

            if (GMApacketCompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode)
            {
                lock_gma_tws_rx_queue();
                packetLen = LengthOfCQueue(&gma_ota_tws_env.rxQueue);
                unlock_gma_tws_rx_queue();
                app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.rxQueue, twsRxQueueMutexID, tempRxBuf, packetLen);

                app_gma_ota_finished_handler();
            }
            else
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }

            break;

        case GMA_OTA_ABORT:
            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.rxQueue,
                                                 twsRxQueueMutexID,
                                                 otaTwsData->data,
                                                 paramLen - GMA_OTA_TWS_HEAD_SIZE);

            if (GMApacketCompleteMagicCode == gma_ota_tws_env.currentReceivedMagicCode)
            {
                lock_gma_tws_rx_queue();
                packetLen = LengthOfCQueue(&gma_ota_tws_env.rxQueue);
                unlock_gma_tws_rx_queue();
                app_gma_ota_tws_pop_data_from_cqueue(&gma_ota_tws_env.rxQueue, twsRxQueueMutexID, tempRxBuf, packetLen);

                //GSoundTargetOtaAbort(abort->device_index);
            }
            else
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }

            break;

        default:
            status = INVALID_CMD;
            app_gma_ota_tws_rsp(status);
            break;
        }
    }
    else
    {
        status = INVALID_CMD;

        switch (otaTwsData->packetType)
        {
        case GMA_OTA_RSP_RESULT:
            app_gma_ota_update_peer_result(status);
            osSignalSet(gma_ota_tws_env.rxThreadId, (1 << GMA_SIGNAL_NUM));
            break;

        default:
            status = INVALID_CMD;
            app_gma_ota_tws_rsp(status);
            break;
        }
    }
}

void app_gma_ota_tws_deinit(void)
{
    app_gma_ota_tws_reset();
    if (gma_ota_tws_env.txThreadId)
    {
        osThreadTerminate(gma_ota_tws_env.txThreadId);
        gma_ota_tws_env.txThreadId = NULL;
    }

    if (twsTxQueueMutexID != NULL)
    {
        osMutexDelete(twsTxQueueMutexID);
        twsTxQueueMutexID = NULL;
    }

    if (twsRxQueueMutexID != NULL)
    {
        osMutexDelete(twsRxQueueMutexID);
        twsRxQueueMutexID = NULL;
    }
}

void app_gma_ota_tws_reset(void)
{
    ResetCQueue(&gma_ota_tws_env.txQueue);
    ResetCQueue(&gma_ota_tws_env.rxQueue);
    memset(gma_ota_tws_env.txBuf, 0, GMA_OTA_TWS_BUFSIZE);
    memset(gma_ota_tws_env.rxBuf, 0, GMA_MAX_MTU);
}
