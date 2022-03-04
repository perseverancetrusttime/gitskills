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
 *
 ****************************************************************************/
#include "string.h"
#include "hal_trace.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_customif_cmd.h"
#include "app_ibrt_if.h"
#include "app_dip.h"
#include "xspace_ui.h"
#include "xspace_interface_process.h"
#include "xspace_interface.h"

#ifdef __GMA_VOICE__
#include "gma_crypto.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#endif

#ifdef DUAL_MIC_RECORDING
#include "app_ai_if.h"
#include "ai_control.h"
#include "app_ai_if.h"
#include "ai_thread.h"
#include "app_recording_handle.h"
#endif

#include "a2dp_decoder.h"
#include "app_bt.h"

#if defined(ANC_APP)
extern "C" int32_t app_anc_tws_sync_change(uint8_t *buf, uint32_t len);
#endif
#if defined(ANC_ASSIST_ENABLED)
extern "C" int32_t anc_assist_tws_sync_status_change(uint8_t *buf, uint32_t len);
#endif
extern "C" int32_t audio_process_tws_sync_change(uint8_t *buf, uint32_t len);
#if defined(IBRT)
/*
* custom cmd handler add here, this is just a example
*/

#define app_ibrt_custom_cmd_rsp_timeout_handler_null   (0)
#define app_ibrt_custom_cmd_rsp_handler_null           (0)
#define app_ibrt_custom_cmd_rx_handler_null            (0)

#ifdef __GMA_VOICE__
void app_ibrt_gma_secret_key_sync(uint8_t *p_buff, uint16_t length);
void app_ibrt_gma_secret_key_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

#ifdef BISTO_ENABLED
static void app_ibrt_bisto_dip_sync(uint8_t *p_buff, uint16_t length);
static void app_ibrt_bisto_dip_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

#ifdef DUAL_MIC_RECORDING
static void app_ibrt_customif_audio_send(uint8_t* p_buff, uint16_t length);
static void app_ibrt_customif_audio_send_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);
static void app_ibrt_customif_audio_send_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length);

static void app_ibrt_customif_update_bitrate(uint8_t* p_buff, uint16_t length);
static void app_ibrt_customif_update_bitrate_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length);
static void app_ibrt_customif_update_bitrate_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t* ptrParam, uint16_t paramLen);

static void app_ibrt_customif_report_buf_lvl(uint8_t* p_buff, uint16_t length);
static void app_ibrt_customif_report_buf_lvl_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length);
static void app_ibrt_customif_report_buf_lvl_done(uint16_t cmdcode, uint16_t rsp_seq,
                                                  uint8_t *ptrParam, uint16_t paramLen);
#endif

#if defined(ANC_APP)
static void app_ibrt_sync_anc_status(uint8_t *p_buff, uint16_t length);
static void app_ibrt_sync_anc_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif
#if defined(PSAP_APP)
static void app_ibrt_sync_psap_status(uint8_t *p_buff, uint16_t length);
static void app_ibrt_sync_psap_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif
#if defined(ANC_ASSIST_ENABLED)
static void app_ibrt_sync_anc_assist_status(uint8_t *p_buff, uint16_t length);
static void app_ibrt_sync_anc_assist_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif
static void app_ibrt_sync_audio_process(uint8_t *p_buff, uint16_t length);
static void app_ibrt_sync_audio_process_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

#ifdef CUSTOM_BITRATE
static void app_ibrt_codec_user_info_sync(uint8_t *p_buff, uint16_t length);
static void app_ibrt_codec_user_info_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

#ifdef __XSPACE_UI__
static void xspace_ui_info_sync_cmd_receive_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void xspace_ui_info_sync_cmd_send(uint8_t *p_buff, uint16_t length);
#endif

static const app_tws_cmd_instance_t g_ibrt_custom_cmd_handler_table[]=
{
#ifdef GFPS_ENABLED
    {
        APP_TWS_CMD_SHARE_FASTPAIR_INFO,                "SHARE_FASTPAIR_INFO",
        app_ibrt_share_fastpair_info,
        app_ibrt_shared_fastpair_info_received_handler,  0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,          app_ibrt_custom_cmd_rsp_handler_null
    },
#endif

#ifdef __GMA_VOICE__
    {
        APP_TWS_CMD_GMA_SECRET_KEY,                     "GMA_SECRET_KEY_SYNC",
        app_ibrt_gma_secret_key_sync,
        app_ibrt_gma_secret_key_sync_handler,           0,
        app_ibrt_cmd_rsp_timeout_handler_null,          app_ibrt_cmd_rsp_handler_null
    },
#endif

#ifdef BISTO_ENABLED
    {
        APP_TWS_CMD_BISTO_DIP_SYNC,                     "BISTO_DIP_SYNC",
        app_ibrt_bisto_dip_sync,
        app_ibrt_bisto_dip_sync_handler,                0,
        app_ibrt_cmd_rsp_timeout_handler_null,          app_ibrt_cmd_rsp_handler_null
    },
#endif

#ifdef DUAL_MIC_RECORDING
    {
        APP_IBRT_CUSTOM_CMD_DMA_AUDIO,                          "TWS_CMD_DMA_AUDIO",
        app_ibrt_customif_audio_send,
        app_ibrt_customif_audio_send_handler,               0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null,
        app_ibrt_customif_audio_send_done,
    },
    {
        APP_IBRT_CUSTOM_CMD_UPDATE_BITRATE,                     "TWS_CMD_UPDATE_BITRATE",
        app_ibrt_customif_update_bitrate,
        app_ibrt_customif_update_bitrate_handler,               0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null,
        app_ibrt_customif_update_bitrate_done,
    },
    {
        APP_IBRT_CUSTOM_CMD_REPORT_BUF_LVL,                     "TWS_CMD_REPORT_BUF_LVL",
        app_ibrt_customif_report_buf_lvl,
        app_ibrt_customif_report_buf_lvl_handler,               0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null,
        app_ibrt_customif_report_buf_lvl_done,
    },

#endif

#if defined(ANC_APP)
    {
        APP_TWS_CMD_SYNC_ANC_STATUS,                            "SYNC_ANC_STATUS",
        app_ibrt_sync_anc_status,
        app_ibrt_sync_anc_status_handler,                       0,
        app_ibrt_cmd_rsp_timeout_handler_null,                  app_ibrt_cmd_rsp_handler_null
    },
#endif
#if defined(PSAP_APP)
    {
        APP_TWS_CMD_SYNC_PSAP_STATUS,                           "SYNC_PSAP_STATUS",
        app_ibrt_sync_psap_status,
        app_ibrt_sync_psap_status_handler,                      0,
        app_ibrt_cmd_rsp_timeout_handler_null,                  app_ibrt_cmd_rsp_handler_null
    },
#endif
#if defined(ANC_ASSIST_ENABLED)
    {
        APP_TWS_CMD_SYNC_ANC_ASSIST_STATUS,                     "SYNC_ANC_ASSIST_STATUS",
        app_ibrt_sync_anc_assist_status,
        app_ibrt_sync_anc_assist_status_handler,                0,
        app_ibrt_cmd_rsp_timeout_handler_null,                  app_ibrt_cmd_rsp_handler_null
    },
#endif
    {
        APP_TWS_CMD_SYNC_AUDIO_PROCESS,                         "SYNC_AUDIO_PROCESS",
        app_ibrt_sync_audio_process,
        app_ibrt_sync_audio_process_handler,                    0,
        app_ibrt_cmd_rsp_timeout_handler_null,                  app_ibrt_cmd_rsp_handler_null
    },
    {
        APP_IBRT_CUSTOM_CMD_TEST1,                              "TWS_CMD_TEST1",
        app_ibrt_customif_test1_cmd_send,
        app_ibrt_customif_test1_cmd_send_handler,               0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null
    },
    {
        APP_IBRT_CUSTOM_CMD_TEST2,                              "TWS_CMD_TEST2",
        app_ibrt_customif_test2_cmd_send,
        app_ibrt_customif_test2_cmd_send_handler,               RSP_TIMEOUT_DEFAULT,
        app_ibrt_customif_test2_cmd_send_rsp_timeout_handler,   app_ibrt_customif_test2_cmd_send_rsp_handler
    },
#ifdef CUSTOM_BITRATE
    {
        APP_IBRT_CUSTOM_CMD_SYNC_CODEC_INFO,                    "CODEC_USER_INFO_SYNC",
        app_ibrt_codec_user_info_sync,
        app_ibrt_codec_user_info_sync_handler,                   0,
        app_ibrt_cmd_rsp_timeout_handler_null,                  app_ibrt_cmd_rsp_handler_null
    },
#endif

#if defined(__XSPACE_UI__)
    {
        XSPACE_UI_INFO_SYNC,                                    "XSPACE_UI_INFO_SYNC",
        xspace_ui_info_sync_cmd_send,
        xspace_ui_info_sync_cmd_receive_handler,                0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,           app_ibrt_custom_cmd_rsp_handler_null
    },
#endif
};

int app_ibrt_customif_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl = (void *)&g_ibrt_custom_cmd_handler_table;
    *cmd_size = ARRAY_SIZE(g_ibrt_custom_cmd_handler_table);
    return 0;
}

#ifdef __GMA_VOICE__
void app_ibrt_gma_secret_key_sync(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_GMA_SECRET_KEY, p_buff, length);
}
void app_ibrt_gma_secret_key_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
    gma_set_secret_key((char *)p_buff);
}

#endif

#ifdef BISTO_ENABLED
static void app_ibrt_bisto_dip_sync(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_BISTO_DIP_SYNC, p_buff, length);
}

static void app_ibrt_bisto_dip_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    gsound_mobile_type_get_callback(*(MOBILE_CONN_TYPE_E *)p_buff);
}
#endif

#ifdef DUAL_MIC_RECORDING
static void app_ibrt_customif_audio_send(uint8_t* p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);
    app_recording_send_data_to_master();
}

static void app_ibrt_customif_audio_send_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length)
{
    ai_function_handle(CALLBACK_STORE_SLAVE_DATA, (void*)p_buff, length, AI_SPEC_RECORDING, app_ai_get_device_id_from_index(AI_SPEC_RECORDING));
    //TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_audio_send_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t* ptrParam,
        uint16_t paramLen)
{
    TRACE(1, "%s", __func__);
    app_recording_audio_send_done();
}

static void app_ibrt_customif_update_bitrate(uint8_t* p_buff, uint16_t length)
{
    TRACE(1, "%s", __func__);
    app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_UPDATE_BITRATE, p_buff, length);
}

static void app_ibrt_customif_update_bitrate_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length)
{
#ifdef VOC_ENCODE_SCALABLE
    SCALABLE_CONFIG_BITRATE_T *info = (SCALABLE_CONFIG_BITRATE_T *)p_buff;
    TRACE(3, "%s bitrate:%d, cursor:%d", __func__, info->bitrate, info->cursor);

    app_recording_update_scalable_bitrate(info->bitrate, info->cursor);
#endif
}

static void app_ibrt_customif_update_bitrate_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t* ptrParam,
        uint16_t paramLen)
{
    TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_report_buf_lvl(uint8_t* p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_REPORT_BUF_LVL, p_buff, length);
}

static void app_ibrt_customif_report_buf_lvl_handler(uint16_t rsp_seq, uint8_t* p_buff, uint16_t length)
{
#ifdef VOC_ENCODE_SCALABLE
    app_recording_on_peer_buf_lvl_received(*p_buff);
#endif
}

static void app_ibrt_customif_report_buf_lvl_done(uint16_t cmdcode, uint16_t rsp_seq,
                                                  uint8_t *ptrParam, uint16_t paramLen)
{
    TRACE(1, "%s", __func__);
}
#endif

#if defined(ANC_APP)
static void app_ibrt_sync_anc_status(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SYNC_ANC_STATUS, p_buff, length);
}

static void app_ibrt_sync_anc_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    app_anc_tws_sync_change(p_buff, length);
}
#endif

#if defined(PSAP_APP)
static void app_ibrt_sync_psap_status(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SYNC_PSAP_STATUS, p_buff, length);
}

static void app_ibrt_sync_psap_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(0, "[%s] TODO ...", __func__);
}
#endif

#if defined(ANC_ASSIST_ENABLED)
static void app_ibrt_sync_anc_assist_status(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SYNC_ANC_ASSIST_STATUS, p_buff, length);
}

static void app_ibrt_sync_anc_assist_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    anc_assist_tws_sync_status_change(p_buff, length);
}
#endif

static void app_ibrt_sync_audio_process(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SYNC_AUDIO_PROCESS, p_buff, length);
}

static void app_ibrt_sync_audio_process_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    audio_process_tws_sync_change(p_buff, length);
}

void app_ibrt_customif_cmd_test(ibrt_custom_cmd_test_t *cmd_test)
{
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST1, (uint8_t*)cmd_test, sizeof(ibrt_custom_cmd_test_t));
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST2, (uint8_t*)cmd_test, sizeof(ibrt_custom_cmd_test_t));
}

static void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_TEST1, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    tws_ctrl_send_rsp(APP_IBRT_CUSTOM_CMD_TEST2, rsp_seq, NULL, 0);
    TRACE(1,"%s", __func__);

}

static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);

}

static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(1,"%s", __func__);
}

#ifdef CUSTOM_BITRATE
//#include "product_config.h"
#include "nvrecord_extension.h"
extern void a2dp_avdtpcodec_aac_user_configure_set(uint32_t bitrate,uint8_t user_configure);
extern void a2dp_avdtpcodec_sbc_user_configure_set(uint32_t bitpool,uint8_t user_configure);
extern "C" void app_audio_dynamic_update_dest_packet_mtu_set(uint8_t codec_index, uint8_t packet_mtu, uint8_t user_configure);

void app_ibrt_user_a2dp_info_sync_tws_share_cmd_send(uint8_t *p_buff, uint16_t length)
{
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_SYNC_CODEC_INFO, p_buff, length);
}

static void app_ibrt_codec_user_info_sync(uint8_t *p_buff, uint16_t length)
{
    if((app_bt_ibrt_has_mobile_link_connected())){
        app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_SYNC_CODEC_INFO, p_buff, length);
    }
}

static void app_ibrt_codec_user_info_sync_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ibrt_custome_codec_t *a2dp_user_config_ptr=(ibrt_custome_codec_t *)p_buff;
    TRACE(4,"%s %d %d %d",__func__,a2dp_user_config_ptr->aac_bitrate,a2dp_user_config_ptr->sbc_bitpool,a2dp_user_config_ptr->audio_latentcy);
    if((app_bt_ibrt_has_snoop_link_connected())){
        a2dp_avdtpcodec_sbc_user_configure_set(a2dp_user_config_ptr->sbc_bitpool, true);
        a2dp_avdtpcodec_aac_user_configure_set(a2dp_user_config_ptr->aac_bitrate, true); 
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_SBC, (a2dp_user_config_ptr->audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/3, true);//sbc
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC, (a2dp_user_config_ptr->audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/23, true);//aac
        uint32_t lock = nv_record_pre_write_operation();
        nv_record_get_extension_entry_ptr()->codec_user_info.aac_bitrate = a2dp_user_config_ptr->aac_bitrate;
        nv_record_get_extension_entry_ptr()->codec_user_info.sbc_bitpool = a2dp_user_config_ptr->sbc_bitpool;
        nv_record_get_extension_entry_ptr()->codec_user_info.audio_latentcy = a2dp_user_config_ptr->audio_latentcy;
        nv_record_post_write_operation(lock);
        nv_record_flash_flush();
    }
}
#endif

#if defined(__XSPACE_UI__)
void xspace_ui_send_info_sync_cmd_if(uint8_t *data, uint16_t len)
{
    tws_ctrl_send_cmd(XSPACE_UI_INFO_SYNC, (uint8_t*)data, len);
}

void xspace_ui_send_info_sync_rsp_if(uint16_t rsp_seq, uint8_t *data, uint16_t len)
{
    tws_ctrl_send_rsp(XSPACE_UI_INFO_SYNC, rsp_seq, (uint8_t*)data, len);
}

static void xspace_ui_info_sync_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(XSPACE_UI_INFO_SYNC, p_buff, length);
}

static void xspace_ui_info_sync_cmd_receive_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    uint8_t* mem_buff=NULL;
    if (NULL == p_buff || length == 0)
        return;
    //TRACE(1,"%s", __func__);
    //DUMP8("%02x ", (void *)p_buff, length);
    if (xspace_ui_msg_buf_get(&mem_buff,length) < length)
        return;
    memcpy(mem_buff, p_buff, length);
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
        (uint32_t)xspace_ui_tws_info_sync_recived_handle, (uint32_t)rsp_seq, (uint32_t)mem_buff, (uint32_t)length);
}
#endif

#endif
