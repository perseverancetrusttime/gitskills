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
#include "btif_sys_config.h"

#include <stdio.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "dip_api.h"
#include "plat_types.h"
#include "app_bt.h"
#include "app_dip.h"
#include "app_bt_func.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#include "sdp_api.h"
#include "me_api.h"

#if defined(IBRT)
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "app_ibrt_if.h"
#if !defined(IBRT_UI_V1)
#include "app_tws_ibrt_conn_api.h"
#endif
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
#include "app_ai_if.h"
#endif

#if defined(__AI_VOICE__)
#include "ai_thread.h"
#endif
#include "a2dp_decoder.h"
#include "app_ibrt_customif_cmd.h"
#include "btapp.h"

void a2dp_avdtpcodec_sbc_user_configure_set(uint32_t bitpool,uint8_t user_configure);
void a2dp_avdtpcodec_aac_user_configure_set(uint32_t bitrate,uint8_t user_configure);
void app_audio_dynamic_update_dest_packet_mtu_set(uint8_t codec_index, uint8_t packet_mtu, uint8_t user_configure);

void app_dip_sync_dip_info(void);

static app_dip_info_queried_callback_func_t dip_info_queried_callback = NULL;

#ifdef BTIF_DIP_DEVICE
static const bool dip_function_enable = true;
#else
static const bool dip_function_enable = false;
#endif
static void app_dip_callback(bt_bdaddr_t *_addr, bool ios_flag)
{

#if defined(IBRT)
#if defined(IBRT_UI_V1)
    btif_remote_device_t *p_remote_dev = btif_me_get_remote_device_by_bdaddr(_addr);

    TRACE(2,"%s dev %p addr :", __func__, p_remote_dev);
    DUMP8("%x ", _addr->address, BT_ADDR_OUTPUT_PRINT_NUM);
    if (TWS_LINK == app_tws_ibrt_get_remote_link_type(p_remote_dev))
#else
    TRACE(2,"%s addr :", __func__);
    DUMP8("%x ", _addr->address, BT_ADDR_OUTPUT_PRINT_NUM);
    if (TWS_LINK == app_tws_ibrt_get_remote_link_type((const bt_bdaddr_t *)_addr))
#endif
    {
        TRACE(1,"%s connect type is TWS", __func__);
        return;
    }

    app_dip_sync_dip_info();
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    app_ai_if_mobile_connect_handle((void*)_addr);
#endif

    if (dip_info_queried_callback)
    {
        btif_dip_pnp_info* pnp_info = btif_dip_get_device_info(_addr);
        dip_info_queried_callback(_addr->address, (app_dip_pnp_info_t *)pnp_info);
    }
}

#if defined(IBRT)
static void app_dip_sync_info_prepare_handler(uint8_t *buf, uint16_t *length)
{
    uint32_t offset = 0;
    bt_dip_pnp_info_t pnp_info;
    bt_bdaddr_t *mobile_addr = NULL;

#if defined(IBRT_UI_V1)
    ibrt_ctrl_t *p_g_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    mobile_addr = &p_g_ibrt_ctrl->mobile_addr;
#else
    ibrt_mobile_info_t *mInfo = app_ibrt_conn_get_mobile_info_ext();
    mobile_addr = &(mInfo->mobile_addr);

    bt_bdaddr_t mobile_addr_list[BT_DEVICE_NUM];
    uint8_t connected_mobile_num = app_ibrt_conn_get_connected_mobile_list(mobile_addr_list);

    for(uint8_t i=0; i<connected_mobile_num; i++)
    {
        mobile_addr = &mobile_addr_list[i];
#endif

        nv_record_get_pnp_info(mobile_addr, &pnp_info);
        memcpy(buf+offset, mobile_addr->address, 6);
        offset += 6;
        memcpy(buf+offset, (uint8_t *)&pnp_info, sizeof(pnp_info));
        offset += sizeof(pnp_info);

        uint8_t sbc_bitpool = a2dp_avdtpcodec_sbc_user_bitpool_get();
        memcpy(buf+offset, (uint8_t*)&sbc_bitpool, 1);
        offset += 1;
        
        uint32_t aac_bitrate = a2dp_avdtpcodec_aac_user_bitrate_get();
        memcpy(buf+offset, (uint8_t*)&aac_bitrate, 4);
        offset += 4;
        
        uint8_t sbc_latency_mtu = app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_SBC);
        memcpy(buf+offset, (uint8_t*)&sbc_latency_mtu, 1);
        offset += 1;
        
        uint8_t aac_latency_mtu = app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC);
        memcpy(buf+offset, (uint8_t*)&aac_latency_mtu, 1);
        offset += 1;

#if !defined(IBRT_UI_V1)
    }
#endif

    *length = offset;
}

static void app_dip_sync_info_received_handler(uint8_t *buf, uint16_t length)
{
    bt_bdaddr_t mobile_addr;
    bt_dip_pnp_info_t pnp_info;
    pnp_info.vend_id = 0;
    uint32_t offset = 0;
    nvrec_btdevicerecord *record = NULL;

    while((length - offset) >= 10)
    {
        memcpy(mobile_addr.address, buf+offset, 6);
        offset += 6;
        memcpy((uint8_t *)&pnp_info, buf+offset, sizeof(pnp_info));
        offset += sizeof(pnp_info);
        TRACE(2, "%s vend_id 0x%x vend_id_source 0x%x addr:", __func__,
            pnp_info.vend_id, pnp_info.vend_id_source);
        DUMP8("0x%x ", mobile_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);

        if (pnp_info.vend_id && !nv_record_btdevicerecord_find(&mobile_addr, &record))
        {
            nv_record_btdevicerecord_set_pnp_info(record, &pnp_info);
        }

        uint8_t sbc_bitpool = 0;
        uint8_t sbc_latency_mtu = 0;
        uint8_t aac_latency_mtu = 0;
        uint32_t aac_bitrate = 0;
        
        memcpy((uint8_t *)&sbc_bitpool, buf+offset, 1);
        offset += 1;
        memcpy((uint8_t *)&aac_bitrate, buf+offset, 4);
        offset += 4;
        memcpy((uint8_t *)&sbc_latency_mtu, buf+offset, 1);
        offset += 1;
        memcpy((uint8_t *)&aac_latency_mtu, buf+offset, 1);
        offset += 1;
        
        TRACE(5, "%s %d %d %d %d", __func__, sbc_bitpool, aac_bitrate, sbc_latency_mtu, aac_latency_mtu);
        a2dp_avdtpcodec_sbc_user_configure_set(sbc_bitpool, true);
        a2dp_avdtpcodec_aac_user_configure_set(aac_bitrate, true);
        
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_SBC, sbc_latency_mtu, true);
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC, aac_latency_mtu, true);

    }
}

void app_dip_sync_init(void)
{
    TWS_SYNC_USER_T user_app_dip_t = {
        app_dip_sync_info_prepare_handler,
        app_dip_sync_info_received_handler,
        NULL,
        NULL,
        NULL,
    };

    app_ibrt_if_register_sync_user(TWS_SYNC_USER_DIP, &user_app_dip_t);
}

void app_dip_sync_dip_info(void)
{
    app_ibrt_if_prepare_sync_info();
    app_ibrt_if_sync_info(TWS_SYNC_USER_DIP);
    app_ibrt_if_flush_sync_info();
}
#endif

POSSIBLY_UNUSED static void app_dip_info_queried_default_callback(uint8_t *remote_addr, app_dip_pnp_info_t *pnp_info)
{
    if (pnp_info)
    {
        TRACE(4, "get dip info vendor_id %04x product_id %04x product_version %04x",
                pnp_info->vend_id, pnp_info->prod_id, pnp_info->prod_ver);
    }
    else
    {
        TRACE(1, "%s N/A", __func__);
    }
}

static app_dip_pnp_info_t dip_target_device_info[] =
{
/*
//IOS
    {
        .spec_id = 0x0102,
        .vend_id = 0x004c,
        .prod_id = 0x7308,
        .prod_ver = 0x0e00,
        .prim_rec = 0x01,
        .vend_id_source = 0x01,
    },
*/
/*
//Pixel 3XL
{
    .spec_id = 0x0103,
    .vend_id = 0x00e0,
    .prod_id = 0x1200,
    .prod_ver = 0x1436,
    .prim_rec = 0x01,
    .vend_id_source = 0x01,
},
*/
    {
        .spec_id = 0x0103,
        .vend_id = 0x000f,
        .prod_id = 0x1200,
        .prod_ver = 0x1436,
        .prim_rec = 0x01,
        .vend_id_source = 0x01,
    },
    {
        .spec_id = 0x0103,
        .vend_id = 0x038f,
        .prod_id = 0x1200,
        .prod_ver = 0x1436,
        .prim_rec = 0x01,
        .vend_id_source = 0x01,
    }
};

static bool app_dip_info_queried_check_target_info_matched(app_dip_pnp_info_t *pnp_info, uint8_t * index)
{
    uint8_t len = sizeof(dip_target_device_info)/sizeof(app_dip_pnp_info_t);
    app_dip_pnp_info_t *ptr_target_dip_info;

    for (uint8_t i=0 ; i<len ; i++) {
        ptr_target_dip_info = &dip_target_device_info[i];
        if(memcmp((uint8_t*)ptr_target_dip_info,(uint8_t*)pnp_info,sizeof(app_dip_pnp_info_t)) == 0){
            *index = i;
            return true;
        }
    }
    return false;
}

static void app_dip_info_queried_customer_callback(uint8_t *remote_addr, app_dip_pnp_info_t *pnp_info)
{
    if (pnp_info)
    {
        uint8_t index = 0;
        TRACE(1,"%s",__func__);

        TRACE(6, "spec_id %02x v_id %02x p_id %02x p_v %02x p_rec %02x v_id_src %02x",pnp_info->spec_id,
                pnp_info->vend_id, pnp_info->prod_id, pnp_info->prod_ver,pnp_info->prim_rec,pnp_info->vend_id_source);
        if(app_dip_info_queried_check_target_info_matched(pnp_info,&index)){
            TRACE(0,"target got !");
            a2dp_avdtpcodec_sbc_user_configure_set(41, true);
            a2dp_avdtpcodec_aac_user_configure_set(156*1024, true);
        }else{
#ifdef CUSTOM_BITRATE
            a2dp_avdtpcodec_sbc_user_configure_set(nv_record_get_extension_entry_ptr()->codec_user_info.sbc_bitpool, true);
            a2dp_avdtpcodec_aac_user_configure_set(nv_record_get_extension_entry_ptr()->codec_user_info.aac_bitrate, true);
#else
            a2dp_avdtpcodec_sbc_user_configure_set(USER_CONFIG_DEFAULT_SBC_BITPOOL, true);
            a2dp_avdtpcodec_aac_user_configure_set(USER_CONFIG_DEFAULT_AAC_BITRATE, true);
#endif
        }
    }
    else
    {
        TRACE(1, "%s N/A", __func__);
    }

}

void app_dip_init(void)
{
    btif_dip_init(app_dip_callback);

    app_dip_register_dip_info_queried_callback(app_dip_info_queried_customer_callback);
}

void app_dip_register_dip_info_queried_callback(app_dip_info_queried_callback_func_t func)
{
    dip_info_queried_callback = func;
}

void app_dip_get_remote_info(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (curr_device)
    {
#ifdef IBRT
        if (app_tws_is_connected()&&(app_tws_get_ibrt_role(&curr_device->remote) == IBRT_SLAVE))
        {
            return;
        }
#endif
        app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)curr_device->btm_conn, 0, (uint32_t)(uintptr_t)btif_dip_get_remote_info);
    }
}

bool app_dip_function_enable(void)
{
    return dip_function_enable;
}

