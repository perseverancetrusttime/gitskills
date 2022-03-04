/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include <stdio.h>
#include <stdlib.h>
#include "gsound_dbg.h"

extern "C"
{
#include "cmsis_os.h"
#include "bluetooth.h"
#include "app_bt.h"
#include "hal_aud.h"
#include "gsound_custom_bt.h"
#include "gsound_target.h"
#include "cqueue.h"
#include "me_api.h"
#include "sdp_api.h"
#include "spp_api.h"
#include "os_api.h"
#include "a2dp_api.h"
#include "avdtp_api.h"
#include "avctp_api.h"
#include "avrcp_api.h"
#include "hfp_api.h"
#include "bt_if.h"
#include "app_spp.h"
#include "app_ai_manager_api.h"
#include "app_bt_media_manager.h"
}

#include "btapp.h"
#include "gsound_custom_bt.h"
#include "gsound_custom_ota.h"
#include "gsound_custom.h"
#include "cmsis.h"
#include "app_rfcomm_mgr.h"
#include "gsound_custom_audio.h"

#ifdef IBRT
#include "app_ibrt_if.h"
#endif

#if defined(DUMP_CRASH_ENABLE)
#include "gsound_custom_reset.h"
#endif

#ifdef __AI_VOICE__
#include "ai_manager.h"
#endif

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/
static const GSoundBtInterface *bt_interface;

/****************************function defination****************************/

//static void gsound_read_thread(const void *arg);
//osThreadDef(gsound_read_thread, osPriorityAboveNormal, 1, 2048, "gsound_read");

#define RX_WAIT_SIGNAL_MASK (SPP_RX_SIGNAL_ID << 2)

typedef struct
{
    bool isSppConnected;
    GSoundChannelType channelType;
    bool gsoundConnectionAllowed;
    uint8_t serviceIndex;
    uint8_t gAddr[6];
    bool rx_completed;
} GsoundRFcommServiceEnv_t;

osMutexDef(gsound_data_spp_mutex);
osMutexDef(gsound_data_credit_mutex);
osMutexDef(gsound_voice_spp_mutex);
osMutexDef(gsound_voice_credit_mutex);

GsoundRFcommServiceEnv_t gsoundControl[BT_DEVICE_NUM] = {0};
GsoundRFcommServiceEnv_t gsoundAudio[BT_DEVICE_NUM] = {0};


#if SPP_SERVER == XA_ENABLED

/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/

/* 128 bit UUID in Big Endian f8d1fbe4-7966-4334-8024-ff96c9330e15 */
static const uint8_t GSOUND_DATA_UUID_128[16] = {
    0x15, 0x0e, 0x33, 0xc9, 0x96, 0xff, 0x24, 0x80, 0x34, 0x43, 0x66, 0x79, 0xe4, 0xfb, 0xd1, 0xf8};

/* 128 bit UUID in Big Endian 81c2e72a-0591-443e-a1ff-05f988593351 */
static const uint8_t GSOUND_VOICE_UUID_128[16] = {
    0x51, 0x33, 0x59, 0x88, 0xf9, 0x05, 0xff, 0xa1, 0x3e, 0x44, 0x91, 0x05, 0x2a, 0xe7, 0xc2, 0x81};

#endif

GsoundRFcommServiceEnv_t * gsound_bt_get_channel_service(uint8_t channel)
{
    if (GSOUND_CHANNEL_CONTROL == channel)
    {
        return &gsoundControl[0];
    }
    else if (GSOUND_CHANNEL_AUDIO == channel)
    {
        return &gsoundAudio[0];
    }else {
        GLOG_I("%s channel %d is invalid",__func__, channel);
        return NULL;
    }
}

void *gsound_bt_get_service(uint8_t channel)
{
    return channel == GSOUND_CHANNEL_CONTROL
               ? (void *)&gsoundControl
               : (void *)&gsoundAudio;
}

static GsoundRFcommServiceEnv_t *gsound_get_service_from_rfcomm_instance_index(uint8_t index,
                                                                                uint8_t *link_idx)
{
    for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
    {
        if (index == gsoundAudio[i].serviceIndex)
        {
            *link_idx = i;
            return &gsoundAudio[i];
        }

        if (index == gsoundControl[i].serviceIndex)
        {
           *link_idx = i;
            return &gsoundControl[i];
        }
    }

    return NULL;
}

int gsound_spp_handle_data_event_func(uint8_t device_id,
                                      void *pDev,
                                      uint8_t process,
                                      uint8_t *pData,
                                      uint16_t dataLen)
{
    ASSERT(pDev, "false device, %s, %d", __func__, __LINE__);

#if defined(IBRT)
    if (IBRT_SLAVE == app_ibrt_if_get_ui_role())
    {
        GLOG_I("slave is not allowed to handle data.");
        return 0;
    }
#endif

    uint8_t serviceIndex = app_rfcomm_get_instance_index(pDev);
    uint8_t conn_idx = 0xFF;
    GsoundRFcommServiceEnv_t *service =
        gsound_get_service_from_rfcomm_instance_index(serviceIndex,&conn_idx);

    GLOG_I("idx %d gsound channel:%s conn state:%d", conn_idx,
           gsound_chnl2str(service->channelType),
           gsound_get_rfcomm_connect_state((uint8_t)service->channelType));

    if(!gsound_get_rfcomm_connect_state((uint8_t)service->channelType))
    {
        GLOG_I("channel is not connected.");
        return 0;
    }
    GLOG_I("%s data %p len %d", __func__, pData, dataLen);

    bt_interface->gsound_bt_on_rx_ready(service->channelType, pData, dataLen);
    service->rx_completed = false;
    osSignalWait(RX_WAIT_SIGNAL_MASK, osWaitForever);

    return 0;
}

uint8_t gsound_custom_bt_on_channel_connected_ind(uint8_t *addr, uint8_t chnl)
{
    GSoundBTAddr gAddr = {0};
    gsound_convert_bdaddr_to_gsound(addr,
                                    &gAddr);

    GSoundStatus status = bt_interface->gsound_bt_on_connect_ind((GSoundChannelType)chnl, &gAddr);
    GLOG_I("%s %d", __func__, status);

    return (uint8_t)(status);
}

uint8_t gsound_custom_bt_on_channel_connected(uint8_t *addr, uint8_t chnl)
{
    GSoundBTAddr gAddr = {0};
    gsound_convert_bdaddr_to_gsound(addr,
                                    &gAddr);

    GSoundStatus status = bt_interface->gsound_bt_on_connected((GSoundChannelType)chnl, &gAddr);

    GLOG_I("%s %d", __func__, status);
    if (GSOUND_STATUS_OK == status)
    {
        gsound_set_rfcomm_connect_state(chnl, CHANNEL_CONNECTED);
    }

    return (uint8_t)status;
}

void gsound_custom_bt_on_channel_disconnected(uint8_t chnl)
{
    GLOG_I("%s channel:%s", __func__, gsound_chnl2str(chnl));

    gsound_set_rfcomm_connect_state(chnl, CHANNEL_DISCONNECTED);
    bt_interface->gsound_bt_on_channel_disconnect((GSoundChannelType)chnl);
}

void gsound_inform_available_rfcomm_tx_buf(void)
{
    bt_interface->gsound_bt_on_tx_available(GSOUND_CHANNEL_CONTROL);
    bt_interface->gsound_bt_on_tx_available(GSOUND_CHANNEL_AUDIO);
}

static bool gsound_spp_callback(uint8_t device_id,
                                RFCOMM_EVENT_E event,
                                uint8_t instanceIndex, uint16_t connHandle,
                                uint8_t *pBtAddr, uint8_t *pSentDataBuf,
                                uint16_t sentDataLen)
{
    uint8_t conn_idx = 0xFF;
    GsoundRFcommServiceEnv_t *pGsoundRFCommService =
        gsound_get_service_from_rfcomm_instance_index(instanceIndex, &conn_idx);

    if(pGsoundRFCommService == NULL)
    {
        return false;
    }
    GLOG_I("gsound event %d, hcihandle 0x%x service index %d ch type %d",
           event,
           connHandle,
           instanceIndex,
           pGsoundRFCommService->channelType);

    switch (event)
    {
    case RFCOMM_INCOMING_CONN_REQ:
        break;
    case RFCOMM_CONNECTED:
    {
        if(gsound_bt_is_channel_connected(pGsoundRFCommService->channelType))
        {
            GLOG_I("already connected one phone!");
            return false;
        }
        spp_device * spp_dev = (spp_device *)app_rfcomm_get_spp_dev(instanceIndex);
        if(spp_dev) {
            app_tws_ibrt_spp_bisto_save_ctx((bt_bdaddr_t *)pBtAddr, spp_dev);
        }
        GLOG_I("Connected SPP Bd addr:");
        DUMP8("%02x ", pBtAddr, BT_ADDR_OUTPUT_PRINT_NUM);

        gsound_set_connected_bd_addr(pBtAddr);
        gsound_set_bt_connection_state(true);
        pGsoundRFCommService->isSppConnected = true;
        memcpy(pGsoundRFCommService->gAddr, pBtAddr, 6);

        if(bt_interface) {
            GSoundBTAddr gsound_addr;
            gsound_convert_bdaddr_to_gsound((void const *)pBtAddr, &gsound_addr);
            bt_interface->gsound_bt_on_link_change(GSOUND_TARGET_BT_LINK_CONNECTED,
                                                   &gsound_addr);
        }

        if (GSOUND_STATUS_OK != gsound_custom_bt_on_channel_connected_ind(pBtAddr,
                                                                          (uint8_t)pGsoundRFCommService->channelType))
        {
            pGsoundRFCommService->gsoundConnectionAllowed = false;
        }
        else
        {
            pGsoundRFCommService->gsoundConnectionAllowed = true;
        }

        if (pGsoundRFCommService->gsoundConnectionAllowed &&
            GSOUND_STATUS_OK == gsound_custom_bt_on_channel_connected(pBtAddr,
                                                                      (uint8_t)pGsoundRFCommService->channelType))
        {
            if (app_ai_manager_is_in_multi_ai_mode())
            {
                if (pGsoundRFCommService->channelType == GSOUND_CHANNEL_AUDIO)
                {
#ifdef __AI_VOICE__
                    ai_manager_set_spec_connected_status(AI_SPEC_GSOUND, 1);
#endif
                }
            }

            if (pGsoundRFCommService->channelType == GSOUND_CHANNEL_AUDIO)
            {
                ai_manager_update_gsound_spp_info(pBtAddr, device_id, RFCOMM_CONNECTED);
            }

            gsound_rfcomm_reset_tx_buf(pGsoundRFCommService->channelType);
#ifdef IBRT
            if ((app_bt_is_a2dp_connected(device_id) == false) && (app_bt_is_hfp_connected(device_id) == false))
            {
                if(IBRT_MASTER == app_tws_get_ibrt_role((bt_bdaddr_t *)pBtAddr))
                {
                    TRACE(1, "manual reconnect mobile profiles");
                    app_bt_ibrt_reconnect_mobile_profile((bt_bdaddr_t *)pBtAddr);
                }
            }
#endif
        }
        else
        {
            GLOG_I("connect Disallowed");
            GLOG_I("voicepath bt connect state: %d", gsound_is_bt_connected());
            // if (gsound_is_bt_connected())
            // {
            //     GLOG_I("connected bdAddr: 0x");
            //     DUMP8("%02x ", gsound_get_connected_bd_addr(), BT_ADDR_OUTPUT_PRINT_NUM);
            // }

            // if (gsound_get_rfcomm_connect_state(pGsoundRFCommService->channelType))
            // {
            //     GLOG_I("already connected");
            //     return false;
            // }

            // app_rfcomm_close(pGsoundRFCommService->serviceIndex);
            // gsound_rfcomm_reset_tx_buf(pGsoundRFCommService->channelType);
        }
        break;
    }
    case RFCOMM_DISCONNECTED:
    {
        if (pGsoundRFCommService->gsoundConnectionAllowed)
        {
            gsound_custom_audio_close_mic();
            // Do not notify gsound that the channel disconnected if gsound was the
            // did not allow the connection in the first place (it aborts).
            gsound_custom_bt_on_channel_disconnected((uint8_t)(pGsoundRFCommService->channelType));

            if (app_ai_manager_is_in_multi_ai_mode())
            {
                if (pGsoundRFCommService->channelType == GSOUND_CHANNEL_AUDIO)
                {
#ifdef __AI_VOICE__
                    ai_manager_set_spec_connected_status(AI_SPEC_GSOUND, 0);
#endif
                }
            }

            if (GSOUND_CHANNEL_AUDIO == pGsoundRFCommService->channelType || GSOUND_CHANNEL_CONTROL == pGsoundRFCommService->channelType)
            {
                ai_manager_update_gsound_spp_info(pBtAddr, device_id, RFCOMM_DISCONNECTED);
            }
        }
        // TODO: freddie->unify the channel disconnected api
        gsound_bt_rx_complete_handler(pGsoundRFCommService->channelType, NULL, 0);
        pGsoundRFCommService->isSppConnected = false;
        memset(pGsoundRFCommService->gAddr, 0, 6);

        //gsound_custom_ota_exit(OTA_PATH_BT);
        break;
    }
    case RFCOMM_TX_DONE:
    {
        gsound_rfcomm_free_tx_chunk(pGsoundRFCommService->channelType, pSentDataBuf);
        gsound_inform_available_rfcomm_tx_buf();

        break;
    }
    default:
        break;
    }
    return true;
}

static bt_status_t gsound_spp_init(GsoundRFcommServiceEnv_t *service,
                                   int serviceId,
                                   int txPacketCount,
                                   osMutexId mid,
                                   osMutexId creditMutexId)
{
    RFCOMM_CONFIG_T tConfig;
    tConfig.callback = gsound_spp_callback;
    tConfig.spp_handle_data_event_func = gsound_spp_handle_data_event_func;
    tConfig.tx_pkt_cnt = txPacketCount;
    tConfig.mutexId = mid;
    tConfig.creditMutexId = creditMutexId;
    tConfig.rfcomm_ch = serviceId;

    if (RFCOMM_CHANNEL_GS_CONTROL == serviceId)
    {
        tConfig.app_id = app_spp_alloc_server_id();
        tConfig.rfcomm_128bit_uuid = GSOUND_DATA_UUID_128;
    }
    else if (RFCOMM_CHANNEL_GS_AUDIO == serviceId)
    {
        tConfig.app_id = app_spp_alloc_server_id();
        tConfig.rfcomm_128bit_uuid = GSOUND_VOICE_UUID_128;
    }
    else
    {
        ASSERT(0, "invalid service ID is received.");
    }

    service->isSppConnected = false;
    service->gsoundConnectionAllowed = false;

    int8_t index = app_rfcomm_open(&tConfig);

    if (-1 != index)
    {
        service->serviceIndex = index;
        TRACE(3, "%s index=%d.",__func__, index);
        return BT_STS_SUCCESS;
    }
    else
    {
        return BT_STS_FAILED;
    }
}

// return -1 means no bt connection exists
static int8_t _get_bd_id(bt_bdaddr_t *addr)
{
    uint8_t id;

    bool isSuccessful = a2dp_id_from_bdaddr(addr, &id);

    if (isSuccessful)
    {
        return id;
    }
    else
    {
        return -1;
    }
}

int gsound_custom_bt_stop_sniff()
{
    bt_bdaddr_t besAddr;
    memcpy(besAddr.address, gsound_get_connected_bd_addr(), 6);
    int8_t id = _get_bd_id(&besAddr);

#ifdef IBRT
    app_ibrt_if_prevent_sniff_set(gsound_get_connected_bd_addr(), OTA_ONGOING);
#else
    if (-1 != id)
    {
        app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, id);
    }
#endif

    return id;
}

int gsound_custom_bt_allow_sniff(void)
{
    bt_bdaddr_t besAddr;
    memcpy(besAddr.address, gsound_get_connected_bd_addr(), 6);
    int8_t id = _get_bd_id(&besAddr);

#ifdef IBRT
    app_ibrt_if_prevent_sniff_clear(gsound_get_connected_bd_addr(), OTA_ONGOING);
#else
    if (-1 != id)
    {
        app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, id);
    }
#endif

    return id;
}

void gsound_custom_bt_link_disconnected_handler(uint8_t *pBtAddr)
{
    GLOG_I("%s", __func__);
    DUMP8("%02x ", pBtAddr, BT_ADDR_OUTPUT_PRINT_NUM);

    if (memcmp(pBtAddr, gsound_get_connected_bd_addr(), 6))
    {
        GLOG_I("device is not connected:");
        DUMP8("0x%02x ", pBtAddr, BT_ADDR_OUTPUT_PRINT_NUM);
        return;
    }

    if (!gsound_is_bt_connected())
    {
        GLOG_I("BT already disconnected");
        return;
    }

    if(!memcmp(gsound_get_connected_bd_addr(),pBtAddr, 6)) {
        GSoundBTAddr gsound_addr;
        gsound_convert_bdaddr_to_gsound((void const *)pBtAddr, &gsound_addr);
    
        // inform glib that gsound channels are disconnected
        gsound_custom_bt_on_channel_disconnected(GSOUND_CHANNEL_AUDIO);
        gsound_custom_bt_on_channel_disconnected(GSOUND_CHANNEL_CONTROL);
        gsound_set_bt_connection_state(false);
        bt_interface->gsound_bt_on_link_change(GSOUND_TARGET_BT_LINK_DISCONNECTED,
                                               &gsound_addr);
    }
}

void gsound_custom_bt_link_connected_handler(uint8_t *pBtAddr)
{
    GLOG_I("%s", __func__);
    DUMP8("%02x ", pBtAddr, BT_ADDR_OUTPUT_PRINT_NUM);

    if (gsound_is_bt_connected() && (gsound_get_bt_connect_state() || gsound_get_ble_connect_state()))
    {
        GLOG_I("ready connected.");
        return;
    }

#if defined(IBRT)
    if (IBRT_SLAVE == app_ibrt_if_get_ui_role())
    {
        GLOG_I("slave not update the bt connected info");
        return;
    }
#endif

    gsound_set_connected_bd_addr(pBtAddr);
    gsound_set_bt_connection_state(true);

}

#ifdef __BT_ONE_BRING_TWO__
static uint8_t last_paused_device_id = 0;

static bool app_gsound_is_bt_in_bisto(void const *bt_addr, void const *bisto_addr)
{
    return (memcmp(bt_addr, bisto_addr, BTIF_BD_ADDR_SIZE) == 0);
}

static APP_A2DP_STREAMING_STATE_E a2dp_get_streaming_state(uint8_t deviceId)
{
    if (app_bt_is_music_player_working(deviceId))
    {
        return APP_A2DP_MEDIA_PLAYING;
    }
    else if (app_bt_is_a2dp_streaming(deviceId))
    {
        return APP_A2DP_STREAMING_ONLY;
    }
    else
    {
        return APP_A2DP_IDLE;
    }
}

static APP_A2DP_STREAMING_STATE_E app_gsound_get_stream_state_of_device_not_in_bisto(
    uint8_t *deviceIdNotInBisto, void const *bisto_addr)
{
    uint8_t deviceId;
    uint8_t bdAddr[BTIF_BD_ADDR_SIZE];
    APP_A2DP_STREAMING_STATE_E streamState = APP_A2DP_IDLE;
    for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++)
    {
        if (app_bt_get_device_bdaddr(deviceId, bdAddr))
        {
            GLOG_I("MP: Get bdaddr of dev %d:", deviceId);
            DUMP8("%02x ", bdAddr, BT_ADDR_OUTPUT_PRINT_NUM);
            if (app_gsound_is_bt_in_bisto(bdAddr, bisto_addr))
            {
                GLOG_I("MP: Device bisto, Ignore.");
                continue;
            }
            else
            {
                GLOG_I("MP: Device is not in bisto.");
                *deviceIdNotInBisto = deviceId;
                streamState = a2dp_get_streaming_state(deviceId);
                GLOG_I("MP: Device stream state is %d", streamState);
            }
        }
    }

    GLOG_I("MP: Inactive=Final State %d", streamState);
    return streamState;
}
#endif

/**
 * Pause and/or suspend any streaming non-Bisto device.
 *
 * If called multiple times, it will retain the paused
 * state of the first call until post_voice_query is called.
 */
bool app_gsound_a2dp_streaming_handler_pre_voice_query(void const *bisto_addr)
{
#ifdef __BT_ONE_BRING_TWO__
    uint8_t deviceIdNotInBisto = 0;
    APP_A2DP_STREAMING_STATE_E streamingState =
        app_gsound_get_stream_state_of_device_not_in_bisto(&deviceIdNotInBisto, bisto_addr);
    if (APP_A2DP_MEDIA_PLAYING == streamingState)
    {
        // pause the media player now, and resume it after the voice query
        GLOG_I("MP: Inactive=Media streaming");
        bool isPaused = app_bt_pause_music_player(deviceIdNotInBisto);
        if (isPaused)
        {
            last_paused_device_id = deviceIdNotInBisto;
            GLOG_I("MP: Inactive=Force-Paused %d", last_paused_device_id);
            return true;
        }
    }
    else if (APP_A2DP_STREAMING_ONLY == streamingState)
    {
        GLOG_I("MP: Inactive=Non-media streaming");
        // stop the a2dp streaming only if the media player is not on
        // Return false so we don't incorrectly resume any media play-back
        app_bt_suspend_a2dp_streaming(deviceIdNotInBisto);
    }
    else
    {
        GLOG_I("MP: Inactive=Idle");
    }

    return false;
#else
    return false;
#endif
}

void app_gsound_a2dp_streaming_handler_post_voice_query(void)
{
#ifdef __BT_ONE_BRING_TWO__
    if (app_bt_get_device(last_paused_device_id)->a2dp_need_resume_flag)
    {
        GLOG_I("MP: Inactive Resume! %d", last_paused_device_id);
        app_bt_resume_music_player(last_paused_device_id);
    }
    else
    {
        GLOG_I("MP: Inactive no-resume");
    }
#endif
}

uint8_t gsound_bt_init(void)
{
    GLOG_I("[%s]+++", __func__);

    bt_status_t status = BT_STS_SUCCESS;
    osMutexId data_mid, dataCredit, voice_mid, voiceCredit;

    if (gsound_is_bt_init_done() != GSOUND_INIT_DONE)
    {
        data_mid = osMutexCreate(osMutex(gsound_data_spp_mutex));
        dataCredit = osMutexCreate(osMutex(gsound_data_credit_mutex));
        voice_mid = osMutexCreate(osMutex(gsound_voice_spp_mutex));
        voiceCredit = osMutexCreate(osMutex(gsound_voice_credit_mutex));

        if (!data_mid || !voice_mid || !dataCredit || !voiceCredit)
        {
            ASSERT(0, "cannot create mutex");
        }

#if defined(DUMP_CRASH_ENABLE)
        gsound_crash_dump_init();
#endif

        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if (BT_STS_SUCCESS == status)
            {
                gsoundControl[i].channelType = GSOUND_CHANNEL_CONTROL;
                gsoundControl[i].serviceIndex = 0xFF;
                gsoundControl[i].rx_completed = true;
                status = gsound_spp_init(&(gsoundControl[i]),
                                         RFCOMM_CHANNEL_GS_CONTROL,
                                         SPP_DATA_MAX_PACKET_NUM,
                                         data_mid,
                                         dataCredit);
            }
    
            if (BT_STS_SUCCESS == status)
            {
                gsoundAudio[i].channelType = GSOUND_CHANNEL_AUDIO;
                gsoundAudio[i].serviceIndex = 0xFF;
                gsoundAudio[i].rx_completed = true;
                status = gsound_spp_init(&(gsoundAudio[i]),
                                         RFCOMM_CHANNEL_GS_AUDIO,
                                         SPP_VOICE_MAX_PACKET_NUM,
                                         voice_mid,
                                         voiceCredit);
            }
        }
        if (BT_STS_SUCCESS == status)
        {
            gsound_set_bt_init_state(true);
        }
    }
    else
    {
        GLOG_I("Already init.");
    }

    GLOG_I("[%s]---", __func__);
    return status;
}

void gsound_bt_update_channel_connection_state(uint8_t channel,uint8_t *connBdAddr,bool state)
{
    GLOG_I("BT_%s_%s", gsound_chnl2str(channel), state ? "CONNECTED" : "DISCONNECTED");
    GsoundRFcommServiceEnv_t *channelService = gsound_bt_get_channel_service(channel);

    if(channelService) {
        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if(!memcmp(channelService[i].gAddr, connBdAddr, 6)) {
                channelService[i].isSppConnected = state;
                break;
            }
        }
    }
}

bool gsound_bt_is_channel_connected(uint8_t channel)
{
    bool ret = false;
    GsoundRFcommServiceEnv_t *channelService = gsound_bt_get_channel_service(channel);
    if(channelService) {
        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if(channelService[i].isSppConnected) {
                ret = true;
                break;
            }
        }
    }

    return ret;
}

bool gsound_bt_is_all_connected(void)
{
    return (gsound_bt_is_channel_connected(GSOUND_CHANNEL_CONTROL) &&
            gsound_bt_is_channel_connected(GSOUND_CHANNEL_AUDIO));
}

bool gsound_bt_is_any_connected(void)
{
    return (gsound_bt_is_channel_connected(GSOUND_CHANNEL_CONTROL) ||
            gsound_bt_is_channel_connected(GSOUND_CHANNEL_AUDIO));
}

static void _disconnect_channel(uint8_t channel)
{
    GsoundRFcommServiceEnv_t *channelService = gsound_bt_get_channel_service(channel);

    if(channelService) {
        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if (channelService[i].isSppConnected)
            {
                app_rfcomm_close(channelService[i].serviceIndex);
            }
            else
            {
                GLOG_I("%s already disconnected", gsound_chnl2str(channel));
            }
        }
    }
}

void gsound_custom_bt_disconnect_all_channel(void)
{
    _disconnect_channel(GSOUND_CHANNEL_CONTROL);
    _disconnect_channel(GSOUND_CHANNEL_AUDIO);
}

bool app_gsound_bt_is_connected_and_enabled(void)
{
    return ((gsound_bt_is_all_connected() == true) && (gsound_is_service_enabled() == true));
}

bool app_gsound_is_btaddr_connected(const uint8_t* btAddr)
{
    TRACE(0,"Check whether following bt addr is connected with SPP gsound:");
    if(btAddr == NULL){
        return false;
    }
    DUMP8("%02x ", btAddr, 6);
    if(app_gsound_bt_is_connected_and_enabled() == true){
        if(memcmp(gsound_get_connected_bd_addr(), btAddr, 6)){
            TRACE(0,"gsound_voice.gsound_remote_device_addr");
            DUMP8("%02x ", gsound_get_connected_bd_addr(), 6);
            return false;
        }
        if(memcmp(gsound_get_connected_bd_addr(), btAddr, 6)){
            TRACE(0,"gsound_data.gsound_remote_device_addr");
            DUMP8("%02x ", gsound_get_connected_bd_addr(), 6);
            return false;
        }
        
        return true;
    }

    return false;
}
void gsound_bt_rx_complete_handler(GSoundChannelType type, const uint8_t *buffer, uint32_t len)
{
    GsoundRFcommServiceEnv_t *service = gsound_bt_get_channel_service(type);

    if(service) {
        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if(service[i].isSppConnected && (!service[i].rx_completed)) {
                osSignalSet(app_rfcomm_get_rx_thread_id(service[i].serviceIndex), RX_WAIT_SIGNAL_MASK);
                service[i].rx_completed = true;
                break;
            }
        }
    }
}

void gsound_bt_register_target_handle(const void *handler)
{
    bt_interface = (const GSoundBtInterface *)handler;
}

uint16_t gsound_bt_get_mtu(void)
{
    return ((L2CAP_MTU)-6 - 4);
}

uint8_t gsound_bt_transmit(uint8_t channel,
                           const uint8_t *ptrData,
                           uint32_t length,
                           uint32_t *bytes_consumed)
{
    *bytes_consumed = 0;
    uint16_t len = (uint16_t)length;
    uint8_t ret = 0;
    GsoundRFcommServiceEnv_t *service = gsound_bt_get_channel_service(channel);

    if(service) {
        for(uint8_t i = 0; i < BT_DEVICE_NUM; i++)
        {
            if(service[i].isSppConnected) {
                ret = app_rfcomm_write(service[i].serviceIndex, ptrData, len);
                break;
            }
        }
    }
    return ret;
}

bool gsound_custom_bt_is_hfp_active(void)
{
    bool ret = (bool)bt_media_is_media_active_by_type(BT_STREAM_VOICE);
    return ret;
}
