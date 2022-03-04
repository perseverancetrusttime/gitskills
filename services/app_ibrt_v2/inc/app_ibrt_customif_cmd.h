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
#ifndef __APP_IBRT_IF_CUSTOM_CMD__
#define __APP_IBRT_IF_CUSTOM_CMD__

#include "app_ibrt_custom_cmd.h"

typedef enum
{
    APP_IBRT_CUSTOM_CMD_TEST1 = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x01,
    APP_IBRT_CUSTOM_CMD_TEST2 = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x02,

    APP_TWS_CMD_SHARE_FASTPAIR_INFO = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x03,
#ifdef __GMA_VOICE__
    APP_TWS_CMD_GMA_SECRET_KEY      = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x04,
#endif
#ifdef BISTO_ENABLED
    APP_TWS_CMD_BISTO_DIP_SYNC      = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x05,
#endif
#ifdef DUAL_MIC_RECORDING
    APP_IBRT_CUSTOM_CMD_DMA_AUDIO     = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x06,
    APP_IBRT_CUSTOM_CMD_UPDATE_BITRATE= APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x07,
    APP_IBRT_CUSTOM_CMD_REPORT_BUF_LVL= APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x08,
#endif
#if defined(ANC_APP)
    APP_TWS_CMD_SYNC_ANC_STATUS         = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x09,
#endif
#if defined(PSAP_APP)
    APP_TWS_CMD_SYNC_PSAP_STATUS        = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0A,
#endif
#if defined(ANC_ASSIST_ENABLED)
    APP_TWS_CMD_SYNC_ANC_ASSIST_STATUS  = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0B,
#endif    
    APP_TWS_CMD_SYNC_AUDIO_PROCESS      = APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0C,
    //new customer cmd add here
#ifdef APP_MIC_CAPTURE_LOOPBACK
    APP_IBRT_CUSTOM_CMD_TL_SEALINGCHECK_SYNC = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x0D,
#endif
#ifdef CUSTOM_BITRATE
    APP_IBRT_CUSTOM_CMD_SYNC_CODEC_INFO  = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x0E,
#endif
#if defined(__XSPACE_UI__)	
#if defined(__XSPACE_CUSTOMIZE_ASSIST_ANC__) //long add 0530
	APP_TWS_CMD_SYNC_ANC_STATUS_CHECK	= APP_IBRT_CMD_BASE | APP_IBRT_CUSTOM_CMD_PREFIX | 0x0D,
#endif    
    XSPACE_UI_INFO_SYNC     = APP_IBRT_CMD_BASE|APP_IBRT_CUSTOM_CMD_PREFIX|0x81,
#endif
} app_ibrt_custom_cmd_code_e;

typedef struct
{
    uint8_t rsv;
    uint8_t buff[6];
} __attribute__((packed))ibrt_custom_cmd_test_t;

typedef struct
{
    uint32_t aac_bitrate;
    uint32_t sbc_bitpool;
    uint32_t audio_latentcy;
} ibrt_custome_codec_t; 
void app_ibrt_user_a2dp_info_sync_tws_share_cmd_send(uint8_t *p_buff, uint16_t length);

#define USER_CONFIG_DEFAULT_AUDIO_LATENCY (280)
#define USER_CONFIG_AUDIO_LATENCY_ERROR (20)

#define USER_CONFIG_MIN_AAC_BITRATE (197*1024)
#define USER_CONFIG_MAX_AAC_BITRATE (256*1024)
#define USER_CONFIG_DEFAULT_AAC_BITRATE USER_CONFIG_MAX_AAC_BITRATE

#define USER_CONFIG_MIN_SBC_BITPOOL (41)
#define USER_CONFIG_MAX_SBC_BITPOOL (53)
#define USER_CONFIG_DEFAULT_SBC_BITPOOL USER_CONFIG_MAX_SBC_BITPOOL


void app_ibrt_customif_cmd_test(ibrt_custom_cmd_test_t *cmd_test);
#if defined(__XSPACE_UI__)
void xspace_ui_send_info_sync_cmd_if(uint8_t *data, uint16_t len);
void xspace_ui_send_info_sync_rsp_if(uint16_t rsp_seq, uint8_t *data, uint16_t len);
#endif


#endif
