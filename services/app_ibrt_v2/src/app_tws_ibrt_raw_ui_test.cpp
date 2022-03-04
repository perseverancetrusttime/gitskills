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
#if defined(IBRT_CORE_V2_ENABLE)

#include <string.h>
#include "hal_trace.h"
#include "apps.h"
#include "app_key.h"
#include "app_anc.h"
#include "app_bt_cmd.h"
#include "btapp.h"
#include "factory_section.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "app_tws_ibrt_conn_api.h"
#include "app_tws_ibrt_raw_ui_test.h"
#include "app_custom_api.h"
#include "app_tws_ibrt.h"
#include "app_ibrt_keyboard.h"
#include "app_ibrt_if.h"
#include "app_bt.h"
#include "btapp.h"
#include "app_ibrt_nvrecord.h"

#if defined(BISTO_ENABLED)
#include "gsound_custom_actions.h"
#endif

#include "app_media_player.h"
#include "a2dp_decoder.h"

#ifdef RECORDING_USE_SCALABLE
#include "voice_compression.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_if.h"
#include "app_ai_tws.h"
#endif
#include "app_dip.h"
//#include "ibrt_mgr_queues.h"
#include "app_ai_manager_api.h"
#include "app_anc.h"
#include "pmu.h"
#include "hal_bootmode.h"


typedef struct
{
    bt_bdaddr_t master_bdaddr;
    bt_bdaddr_t slave_bdaddr;
} ibrt_pairing_info_t;

const ibrt_pairing_info_t g_ibrt_pairing_info[] =
{
    {{0x51, 0x33, 0x33, 0x22, 0x11, 0x11},{0x50, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x53, 0x33, 0x33, 0x22, 0x11, 0x11},{0x52, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*LJH*/
    {{0x61, 0x33, 0x33, 0x22, 0x11, 0x11},{0x60, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x71, 0x33, 0x33, 0x22, 0x11, 0x11},{0x70, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x81, 0x33, 0x33, 0x22, 0x11, 0x11},{0x80, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x91, 0x33, 0x33, 0x22, 0x11, 0x11},{0x90, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Customer use*/
    {{0x05, 0x33, 0x33, 0x22, 0x11, 0x11},{0x04, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Rui*/
    {{0x07, 0x33, 0x33, 0x22, 0x11, 0x11},{0x06, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*zsl*/
    {{0x88, 0xaa, 0x33, 0x22, 0x11, 0x11},{0x87, 0xaa, 0x33, 0x22, 0x11, 0x11}}, /*Lufang*/
    {{0x77, 0x22, 0x66, 0x22, 0x11, 0x11},{0x77, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*xiao*/
    {{0xAA, 0x22, 0x66, 0x22, 0x11, 0x11},{0xBB, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*LUOBIN*/
    {{0x08, 0x33, 0x66, 0x22, 0x11, 0x11},{0x07, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin1*/
    {{0x0B, 0x33, 0x66, 0x22, 0x11, 0x11},{0x0A, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin2*/
    {{0x35, 0x33, 0x66, 0x22, 0x11, 0x11},{0x34, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Lulu*/
    {{0xF8, 0x33, 0x66, 0x22, 0x11, 0x11},{0xF7, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*jtx*/
    {{0xd3, 0x53, 0x86, 0x42, 0x71, 0x31},{0xd2, 0x53, 0x86, 0x42, 0x71, 0x31}}, /*shhx*/
    {{0xcc, 0xaa, 0x99, 0x88, 0x77, 0x66},{0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66}}, /*mql*/
    {{0x95, 0x33, 0x69, 0x22, 0x11, 0x11},{0x94, 0x33, 0x69, 0x22, 0x11, 0x11}}, /*wyl*/
    {{0x82, 0x35, 0x68, 0x24, 0x19, 0x17},{0x81, 0x35, 0x68, 0x24, 0x19, 0x17}}, /*rhy*/
    {{0x66, 0x66, 0x88, 0x66, 0x66, 0x88},{0x65, 0x66, 0x88, 0x66, 0x66, 0x88}}, /*xdl*/
    {{0x61, 0x66, 0x66, 0x66, 0x66, 0x81},{0x16, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test1*/
    {{0x62, 0x66, 0x66, 0x66, 0x66, 0x82},{0x26, 0x66, 0x66, 0x66, 0x66, 0x28}}, /*test2*/
    {{0x63, 0x66, 0x66, 0x66, 0x66, 0x83},{0x36, 0x66, 0x66, 0x66, 0x66, 0x38}}, /*test3*/
    {{0x64, 0x66, 0x66, 0x66, 0x66, 0x84},{0x46, 0x66, 0x66, 0x66, 0x66, 0x48}}, /*test4*/
    {{0x65, 0x66, 0x66, 0x66, 0x66, 0x85},{0x56, 0x66, 0x66, 0x66, 0x66, 0x58}}, /*test5*/
    {{0xaa, 0x66, 0x66, 0x66, 0x66, 0x86},{0xaa, 0x66, 0x66, 0x66, 0x66, 0x68}}, /*test6*/
    {{0x67, 0x66, 0x66, 0x66, 0x66, 0x87},{0x76, 0x66, 0x66, 0x66, 0x66, 0x78}}, /*test7*/
    {{0x68, 0x66, 0x66, 0x66, 0x66, 0xa8},{0x86, 0x66, 0x66, 0x66, 0x66, 0x8a}}, /*test8*/
    {{0x69, 0x66, 0x66, 0x66, 0x66, 0x89},{0x86, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test9*/
    {{0x67, 0x66, 0x66, 0x22, 0x11, 0x11},{0x66, 0x66, 0x66, 0x22, 0x11, 0x11}}, /*anonymous*/
    {{0x93, 0x33, 0x33, 0x33, 0x33, 0x33},{0x92, 0x33, 0x33, 0x33, 0x33, 0x33}}, /*gxl*/
    {{0x07, 0x13, 0x66, 0x22, 0x11, 0x11},{0x06, 0x13, 0x66, 0x22, 0x11, 0x11}}, /*yangguo*/
    {{0x02, 0x15, 0x66, 0x22, 0x11, 0x11},{0x01, 0x15, 0x66, 0x22, 0x11, 0x11}}, /*mql fpga*/
    {{0x31, 0x21, 0x68, 0x93, 0x52, 0x70},{0x30, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial0*/
    {{0x33, 0x21, 0x68, 0x93, 0x52, 0x70},{0x32, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial1*/
    {{0x35, 0x21, 0x68, 0x93, 0x52, 0x70},{0x34, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial2*/
    {{0x37, 0x21, 0x68, 0x93, 0x52, 0x70},{0x36, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial3*/
    {{0x39, 0x21, 0x68, 0x93, 0x52, 0x70},{0x38, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial4*/
    {{0x41, 0x21, 0x68, 0x93, 0x52, 0x70},{0x40, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial5*/
    {{0x43, 0x21, 0x68, 0x93, 0x52, 0x70},{0x42, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial6*/
    {{0x45, 0x21, 0x68, 0x93, 0x52, 0x70},{0x44, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial7*/
    {{0x47, 0x21, 0x68, 0x93, 0x52, 0x70},{0x46, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial8*/
    {{0x49, 0x21, 0x68, 0x93, 0x52, 0x70},{0x48, 0x21, 0x68, 0x93, 0x52, 0x70}}, /*xinyin serial9*/
    {{0x32, 0x15, 0x66, 0x22, 0x11, 0x11},{0x31, 0x15, 0x66, 0x22, 0x11, 0x11}}, /*dengcong*/
    {{0x33, 0x33, 0x33, 0x22, 0x11, 0x11},{0x32, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*zhanghao*/
    {{0x77, 0x77, 0x33, 0x22, 0x11, 0x11},{0x76, 0x77, 0x33, 0x22, 0x11, 0x11}}, /* jiaqizhu */
    {{0x10, 0x77, 0x56, 0x5b, 0x1a, 0x34},{0x11, 0x77, 0x56, 0x5b, 0x1a, 0x34}}, /* lining */
};

static bool trace_disabled;

#if !defined(FREE_TWS_PAIRING_ENABLED)
static void app_ibrt_raw_ui_test_load_from_bt_pair_list(void)
{
    const ibrt_pairing_info_t *ibrt_pairing_info_lst = g_ibrt_pairing_info;
    uint32_t lst_size = sizeof(g_ibrt_pairing_info)/sizeof(ibrt_pairing_info_t);
    struct nvrecord_env_t *nvrecord_env;
    uint8_t localAddr[BD_ADDR_LEN];
    uint8_t masterAddr[BD_ADDR_LEN];
    uint8_t slaveAddr[BD_ADDR_LEN];
    
    nv_record_env_get(&nvrecord_env);
    factory_section_original_btaddr_get(localAddr);

    bool isRightMasterSidePolicy = true;
#ifdef IBRT_RIGHT_MASTER
    isRightMasterSidePolicy = true;
#else
    isRightMasterSidePolicy = false;
#endif

    for(uint32_t i =0; i<lst_size; i++)
    {
        memcpy(masterAddr, ibrt_pairing_info_lst[i].master_bdaddr.address, BD_ADDR_LEN);
        memcpy(slaveAddr, ibrt_pairing_info_lst[i].slave_bdaddr.address, BD_ADDR_LEN);
        if (!memcmp(masterAddr, localAddr, BD_ADDR_LEN))
        {
            app_tws_ibrt_reconfig_role(IBRT_MASTER, masterAddr, slaveAddr, isRightMasterSidePolicy);
            return;
        }
        else if (!memcmp(slaveAddr, localAddr, BD_ADDR_LEN))
        {
            app_tws_ibrt_reconfig_role(IBRT_SLAVE, masterAddr, slaveAddr, isRightMasterSidePolicy);
            return;
        }
    }
}
#endif

int app_ibrt_raw_ui_test_config_load(void *config)
{
#if !defined(FREE_TWS_PAIRING_ENABLED)
    app_ibrt_raw_ui_test_load_from_bt_pair_list();
#endif

    ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    factory_section_original_btaddr_get(ibrt_config->local_addr.address);

#if !defined(FREE_TWS_PAIRING_ENABLED)
    // nv record content has been updated in app_ibrt_raw_ui_test_load_from_bt_pair_list
    ibrt_config->nv_role = nvrecord_env->ibrt_mode.mode;

#else
    /** customer can replace this with custom nv role configuration */
    /** by default bit 0 of the first byte decides the nv role:
        1: master and right bud
        0: slave and left bud
    */
    if (ibrt_config->local_addr.address[0]&1)
    {
        ibrt_config->nv_role = IBRT_MASTER;
    }
    else
    {
        ibrt_config->nv_role = IBRT_SLAVE;
    }

    nvrecord_env->ibrt_mode.mode = ibrt_config->nv_role;
#endif

#ifdef IBRT_RIGHT_MASTER
    if (IBRT_MASTER == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        app_ibrt_if_set_side(EAR_SIDE_RIGHT);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
    }
    else if (IBRT_SLAVE == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        app_ibrt_if_set_side(EAR_SIDE_LEFT);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address, BD_ADDR_LEN);
    }
    else
    {
        ibrt_config->nv_role = IBRT_UNKNOW;
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
    }
#else
    if (IBRT_SLAVE == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        app_ibrt_if_set_side(EAR_SIDE_RIGHT);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
    }
    else if (IBRT_MASTER == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        app_ibrt_if_set_side(EAR_SIDE_LEFT);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address, BD_ADDR_LEN);
    }
    else
    {
        ibrt_config->nv_role = IBRT_UNKNOW;
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
    }
#endif
    app_ibrt_conn_set_ui_role(nvrecord_env->ibrt_mode.mode);

    return 0;
}

#ifdef SEARCH_UI_COMPATIBLE_UI_V2
void app_ibrt_search_ui_load_config(void *config)
{
    ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    TRACE(1,"#nv env ibrt mode:%d",nvrecord_env->ibrt_mode.mode);
    factory_section_original_btaddr_get(ibrt_config->local_addr.address);
    ibrt_config->nv_role = nvrecord_env->ibrt_mode.mode;
    if (IBRT_MASTER == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        app_ibrt_if_set_side(EAR_SIDE_RIGHT);
        memcpy(ibrt_config->local_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
    }
    else if (IBRT_SLAVE == nvrecord_env->ibrt_mode.mode)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        app_ibrt_if_set_side(EAR_SIDE_LEFT);
        memcpy(ibrt_config->local_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address, BD_ADDR_LEN);
        }
    else
    {
        ibrt_config->nv_role = IBRT_UNKNOW;
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
    }

    app_ibrt_conn_set_ui_role(nvrecord_env->ibrt_mode.mode);

}
#endif

static void app_ibrt_soft_reset_test(void)
{
    TRACE(0,"soft_reset_test");
    app_reset();
}

void app_ibrt_ui_tws_swtich_test(void)
{
    TRACE(1, "%s",__func__);
}
#ifdef IBRT_UI_V2
void app_ibrt_mgr_pairing_mode_test(void)
{
#if !defined(FREE_TWS_PAIRING_ENABLED)
    app_ibrt_if_init_open_box_state_for_evb();
    app_ibrt_if_event_entry(IBRT_MGR_EV_TWS_PAIRING);
#else
    app_ibrt_if_init_open_box_state_for_evb();
    uint8_t btAddr[6];
    factory_section_original_btaddr_get(btAddr);
    btAddr[0] ^= 0x01;
    // the bit 0 of the bt address's first byte clarifies the tws addresses
    // master if it's 1, slave if it's 0
    app_ibrt_if_start_tws_pairing(app_tws_ibrt_get_bt_ctrl_ctx()->nv_role, 
        btAddr);
#endif
}

#define POWER_ON_TWS_PAIRING_DELAY_MS   2000

osTimerId   power_on_pairing_delay_timer_id = NULL;
static void power_on_pairing_delay_timer_handler(void const *para)
{
    app_ibrt_mgr_pairing_mode_test();
}

void app_ibrt_start_power_on_freeman_pairing(void)
{
    app_ibrt_if_enter_freeman_pairing();
}

osTimerDef (power_on_pairing_delay_timer, power_on_pairing_delay_timer_handler);
void app_ibrt_start_power_on_tws_pairing(void)
{
    if (NULL == power_on_pairing_delay_timer_id)
    {
        power_on_pairing_delay_timer_id = 
            osTimerCreate(osTimer(power_on_pairing_delay_timer), osTimerOnce, NULL);
    }

    osTimerStart(power_on_pairing_delay_timer_id, POWER_ON_TWS_PAIRING_DELAY_MS);
    TRACE(1, "%s",__func__);
}
#endif

void app_ibrt_tws_connect_test(void)
{
    TRACE(0, "tws_connect_test");
    app_ibrt_conn_tws_connect_request(false, 0);
}

void app_ibrt_tws_disconnect_test(void)
{
    TRACE(0, "tws_disconnect_test");
    app_ibrt_conn_tws_disconnect();
}

bt_bdaddr_t remote_addr_1 ={.address = {0x6C, 0x04, 0x5E, 0x6C, 0x66, 0x5C}};
bt_bdaddr_t remote_addr_2 ={.address = {0x3E, 0x44, 0x82, 0x73, 0x4D, 0x6C}};
bt_bdaddr_t remote_addr_3 ={.address = {0x26, 0x88, 0x2D, 0x43, 0xE1, 0xBC}};

void app_ibrt_mobile_connect_test_1()
{
    TRACE(0, "mobile_connect_test_1");

    app_ibrt_conn_remote_dev_connect_request(&remote_addr_1,OUTGOING_CONNECTION_REQ, true, 0);
}

void app_ibrt_mobile_connect_test_2()
{
    TRACE(0, "mobile_connect_test_2");

    app_ibrt_conn_remote_dev_connect_request(&remote_addr_2,OUTGOING_CONNECTION_REQ, true, 0);
}

void app_ibrt_mobile_connect_test_3()
{
    TRACE(0, "mobile_connect_test_3");

    app_ibrt_conn_remote_dev_connect_request(&remote_addr_3,OUTGOING_CONNECTION_REQ, true, 0);
}

void app_ibrt_mobile_disconnect_test_1(void)
{
    TRACE(0, "mobile_disconnect_test_1");
    app_ibrt_conn_remote_dev_disconnect_request(&remote_addr_1,NULL);
}

void app_ibrt_mobile_disconnect_test_2(void)
{
    TRACE(0, "mobile_disconnect_test_2");
    app_ibrt_conn_remote_dev_disconnect_request(&remote_addr_2,NULL);
}

void app_ibrt_mobile_disconnect_test_3(void)
{
    TRACE(0, "mobile_disconnect_test_3");
    app_ibrt_conn_remote_dev_disconnect_request(&remote_addr_3,NULL);
}

void app_ibrt_connect_profiles_test(void)
{
    TRACE(0, "connect_profiles_test");
    app_ibrt_conn_connect_profiles(NULL);
}

void app_ibrt_host_connect_cancel_test(void)
{
    TRACE(0, "host_connect_cancel_test");
    app_ibrt_conn_remote_dev_connect_request(NULL,OUTGOING_CONNECTION_REQ, true, 0);
    app_ibrt_conn_remote_dev_connect_cancel_request(NULL);
}

void app_ibrt_start_ibrt_test_1(void)
{
    TRACE(0, "start_ibrt_test_1");
    app_ibrt_conn_connect_ibrt(&remote_addr_1);
}

void app_ibrt_start_ibrt_test_2(void)
{
    TRACE(0, "start_ibrt_test_2");
    app_ibrt_conn_connect_ibrt(&remote_addr_2);
}

void app_ibrt_start_ibrt_test_3(void)
{
    TRACE(0, "start_ibrt_test_3");
    app_ibrt_conn_connect_ibrt(&remote_addr_3);
}

void app_ibrt_stop_ibrt_test_1(void)
{
    TRACE(0, "stop_ibrt_test_1");
    app_ibrt_conn_disconnect_ibrt(&remote_addr_1);
}

void app_ibrt_stop_ibrt_test_2(void)
{
    TRACE(0, "stop_ibrt_test_2");
    app_ibrt_conn_disconnect_ibrt(&remote_addr_2);
}

void app_ibrt_stop_ibrt_test_3(void)
{
    TRACE(0, "stop_ibrt_test_3");
    app_ibrt_conn_disconnect_ibrt(&remote_addr_3);
}


void app_ibrt_tws_role_get_request_test(void)
{
    tws_role_e role = app_ibrt_conn_tws_role_get_request(NULL);
    TRACE(2, "%s Role=%d",__func__,role);
}

void app_ibrt_tws_role_switch_test_1(void)
{
    ibrt_status_t status;

    status = app_ibrt_conn_tws_role_switch(&remote_addr_1);
    TRACE(2, "%s status=%d",__func__,status);
}

void app_ibrt_tws_role_switch_test_2(void)
{
    ibrt_status_t status;

    status = app_ibrt_conn_tws_role_switch(&remote_addr_2);
    TRACE(2, "%s status=%d",__func__,status);
}

void app_ibrt_tws_role_switch_test_3(void)
{
    ibrt_status_t status;

    status = app_ibrt_conn_tws_role_switch(&remote_addr_3);
    TRACE(2, "%s status=%d",__func__,status);
}

void app_ibrt_get_remote_device_count(void)
{
    TRACE(1,"remote device count = %d",app_ibrt_conn_get_local_connected_mobile_count());
}

void app_ibrt_tws_dump_info_test(void)
{
    TRACE(1, "%s",__func__);
    app_ibrt_conn_dump_ibrt_info();
}

#ifdef GFPS_ENABLED
extern "C" void app_enter_fastpairing_mode(void);
#endif

void app_ibrt_enable_access_mode_test(void)
{
#ifdef GFPS_ENABLED
    app_enter_fastpairing_mode();
#endif

    TRACE(1, "%s",__func__);
    app_ibrt_conn_set_discoverable_connectable(true,true);
}

void app_ibrt_disable_access_mode_test(void)
{
    TRACE(1, "%s",__func__);
    app_ibrt_conn_set_discoverable_connectable(false,false);
}

void app_ibrt_tws_get_master_mobile_rssi_test(void)
{
}

void app_ibrt_tws_get_master_slave_rssi_test(void)
{
}

void app_ibrt_tws_get_slave_mobile_rssi_test(void)
{
}

void app_ibrt_tws_get_peer_mobile_rssi_test(void)
{
    TRACE(1, "%s",__func__);
}

void app_ibrt_tws_freeman_pairing_test(void)
{
    TRACE(1, "%s",__func__);
}

void app_ibrt_tws_disconnect_all_connection_test(void)
{
    TRACE(1, "%s",__func__);
    app_ibrt_conn_disc_all_connnection(NULL);
}

void app_ibrt_service_fuction_test()
{
    bool result;

    TRACE(1, "%s",__func__);
    result  = app_ibrt_conn_is_tws_in_pairing_state();
    TRACE(1, "tws is in pairing mode = %d", result);

    result = app_ibrt_conn_is_ibrt_master(NULL);
    TRACE(1, "tws current role is master = %d", result);

    result = app_ibrt_conn_is_ibrt_slave(NULL);
    TRACE(1, "tws current role is slave = %d",result);

    result = app_ibrt_conn_is_freeman_mode();
    TRACE(1, "tws is freeman mode = %d",result);

    app_tws_buds_info_t buds_info;
    memset((char*)&buds_info,0,sizeof(buds_info));
    app_ibrt_conn_get_tws_buds_info(&buds_info);
    TRACE(0, "tws local addr:");
    DUMP8("%02x ", buds_info.local_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
    TRACE(1, "tws current role=%d",buds_info.current_ibrt_role);
    TRACE(0, "peer addr:");
    DUMP8("%02x ", buds_info.peer_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
    TRACE(1, "tws peer role=%d",buds_info.peer_ibrt_role);
}

void app_ibrt_tws_w4_mobile_connect_test(void)
{
    TRACE(1, "%s",__func__);

    if(app_ibrt_conn_is_nv_master())
    {
         btif_me_set_accessible_mode(BTIF_BAM_GENERAL_ACCESSIBLE,NULL);
    }
    else
    {
         btif_me_set_accessible_mode(BTIF_BAM_NOT_ACCESSIBLE,NULL);
    }
}

void app_ibrt_tws_mobile_connection_test()
{
    TRACE(1, "%s",__func__);
    TRACE(1, "mobile connection state = %d",app_ibrt_conn_get_mobile_conn_state(NULL));
}

void app_ibrt_get_a2dp_state_test(unsigned char *bt_addr,unsigned int lenth)
{
   const char* a2dp_state_strings[] = {
    "IDLE",
    "CODEC_CONFIGURED",
    "OPEN",
    "STREAMING",
    "CLOSED",
    "ABORTING"
   };
    if(parse_bt_addr(bt_addr, &remote_addr_1, lenth) == 1){
        AppIbrtA2dpState a2dp_state;
        AppIbrtStatus  status = app_ibrt_if_get_a2dp_state(&remote_addr_1, &a2dp_state);
        if(APP_IBRT_IF_STATUS_SUCCESS == status){
            TRACE(0,"ibrt_ui_log:a2dp_state=%s",a2dp_state_strings[a2dp_state]);
        }
        else{
            TRACE(0,"ibrt_ui_log:get a2dp state error");
        }
    }
}

void app_ibrt_get_avrcp_state_test(unsigned char *bt_addr, unsigned int lenth)
{
    const char* avrcp_state_strings[] = {
        "DISCONNECTED",
        "CONNECTED",
        "PLAYING",
        "PAUSED",
        "VOLUME_UPDATED"
    };
    if(parse_bt_addr(bt_addr, &remote_addr_1, lenth) == 1){
        AppIbrtAvrcpState avrcp_state;
        AppIbrtStatus status = app_ibrt_if_get_avrcp_state(&remote_addr_1, &avrcp_state);
        if(APP_IBRT_IF_STATUS_SUCCESS == status)
        {
            TRACE(0,"ibrt_ui_log:avrcp_state=%s",avrcp_state_strings[avrcp_state]);
        }
        else
        {
            TRACE(0,"ibrt_ui_log:get avrcp state error");
        }
    }
}

void app_ibrt_get_hfp_state_test(unsigned char *bt_addr, unsigned int lenth)
{
    const char* hfp_state_strings[] = {
        "SLC_DISCONNECTED",
        "CLOSED",
        "SCO_CLOSED",
        "PENDING",
        "SLC_OPEN",
        "NEGOTIATE",
        "CODEC_CONFIGURED",
        "SCO_OPEN",
        "INCOMING_CALL",
        "OUTGOING_CALL",
        "RING_INDICATION"
    };
        if(parse_bt_addr(bt_addr, &remote_addr_1, lenth) == 1){
            AppIbrtHfpState hfp_state;
            AppIbrtStatus status = app_ibrt_if_get_hfp_state(&remote_addr_1, &hfp_state);
            if(APP_IBRT_IF_STATUS_SUCCESS == status)
            {
                TRACE(0,"ibrt_ui_log:hfp_state=%s",hfp_state_strings[hfp_state]);
            }
            else
            {
                TRACE(0,"ibrt_ui_log:get hfp state error");
            }
        }

}

void app_ibrt_get_call_status_test(unsigned char *bt_addr, unsigned int lenth)
{
    const char* call_status_strings[] = {
        "NO_CALL",
        "CALL_ACTIVE",
        "HOLD",
        "INCOMMING",
        "OUTGOING",
        "ALERT"
    };
    if(parse_bt_addr(bt_addr, &remote_addr_1, lenth) == 1){
            AppIbrtCallStatus call_status;
            AppIbrtStatus status = app_ibrt_if_get_hfp_call_status(&remote_addr_1, &call_status);
            if(APP_IBRT_IF_STATUS_SUCCESS == status)
            {
                TRACE(0,"ibrt_ui_log:call_status=%s",call_status_strings[call_status]);
            }
            else
            {
                TRACE(0,"ibrt_ui_log:get call status error");
            }
        }
}

void app_ibrt_tws_role_get_request_test_with_parameter(unsigned char *bt_addr, unsigned int lenth)
{
    if(parse_bt_addr(bt_addr, &remote_addr_1, lenth) == 1){
        tws_role_e role = app_ibrt_conn_tws_role_get_request(&remote_addr_1);
        TRACE(1, "Role=%d",role);
    }
}

void app_ibrt_get_tws_state_test(void){
    app_ibrt_if_get_tws_conn_state_test();
    }

void app_ibrt_tws_avrcp_pause_test(void)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    if(device_id != BT_DEVICE_INVALID_ID){
        struct BT_DEVICE_T * curr_device = app_bt_get_device(device_id);
        if(app_tws_ibrt_tws_link_connected() && IBRT_SLAVE == app_tws_get_ibrt_role(&curr_device->remote)){
            app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_PAUSE, 0, 0);
        }
        else{
            a2dp_handleKey(AVRCP_KEY_PAUSE);
        }
    }
}

void app_ibrt_tws_avrcp_play_test(void)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    if(device_id != BT_DEVICE_INVALID_ID){
        struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
        if(app_tws_ibrt_tws_link_connected() && IBRT_SLAVE == app_tws_get_ibrt_role(&curr_device->remote)){
            app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_PLAY, 0, 0);
        }
        else{
            a2dp_handleKey(AVRCP_KEY_PLAY);
        }
    }
}

void app_ibrt_tws_avrcp_toggle_test(void)
{
}

void app_ibrt_tws_avrcp_vol_up_test(void)
{
}

void app_ibrt_tws_avrcp_vol_down_test(void)
{
}

void app_ibrt_tws_avrcp_next_track_test(void)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_FORWARD, 0, 0);
    }

}

void app_ibrt_tws_avrcp_prev_track_test(void)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_BACKWARD, 0, 0);
    }
}

void app_ibrt_ui_get_a2dp_active_phone(void)
{
    uint8_t a2dp_id = app_bt_audio_get_curr_a2dp_device();
    struct BT_DEVICE_T* curr_device = NULL;
    curr_device = app_bt_get_device(a2dp_id);
    if(!curr_device->a2dp_conn_flag)
    {
        TRACE(0,"a2dp active phone = None");
        return;
    }
    if(curr_device->a2dp_play_pause_flag == 0){
        if(app_bt_manager.a2dp_last_paused_device != BT_DEVICE_INVALID_ID){
             struct BT_DEVICE_T* last_paused_device = app_bt_get_device(app_bt_manager.a2dp_last_paused_device);
             if(app_tws_ibrt_tws_link_connected()){
                if (last_paused_device->a2dp_conn_flag && IBRT_MASTER == app_tws_get_ibrt_role(&last_paused_device->remote)){
                    TRACE(6, "a2dp active phone = %02x%02x%02x%02x%02x%02x", last_paused_device->remote.address[5],
                        last_paused_device->remote.address[4], last_paused_device->remote.address[3],last_paused_device->remote.address[2],
                        last_paused_device->remote.address[1], last_paused_device->remote.address[0]);
                    return;
                 }
                 else{
                        TRACE(0,"a2dp active phone = None");
                        return;
                 }
             }else{
                    TRACE(6, "a2dp active phone = %02x%02x%02x%02x%02x%02x", last_paused_device->remote.address[5],
                        last_paused_device->remote.address[4], last_paused_device->remote.address[3],last_paused_device->remote.address[2],
                        last_paused_device->remote.address[1], last_paused_device->remote.address[0]);
                    return; 
             }
           
        }
    }
    TRACE(6, "a2dp active phone = %02x%02x%02x%02x%02x%02x", curr_device->remote.address[5], curr_device->remote.address[4],
    curr_device->remote.address[3],curr_device->remote.address[2], curr_device->remote.address[1], curr_device->remote.address[0]);
}


void app_ibrt_tws_hfp_answer_test(void)
{
    uint8_t device_id = app_bt_audio_get_device_for_user_action();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_ANSWER, 0, 0);
    }
}

void app_ibrt_tws_hfp_reject_test(void)
{
    uint8_t device_id = app_bt_audio_get_device_for_user_action();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_HANGUP, 0, 0);
    }
}

void app_ibrt_tws_hfp_hangup_test(void)
{
    uint8_t device_id = app_bt_audio_get_device_for_user_action();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_HANGUP, 0, 0);
    }
}

void app_ibrt_tws_hfp_mute_test(void)
{
}

void app_ibrt_tws_hfp_unmute_test(void)
{
}

void app_ibrt_tws_hfp_assisant_test(void)
{
}

void app_ibrt_tws_hfp_vol_up_test(void)
{
}

void app_ibrt_tws_hfp_vol_down_test(void)
{
}

void app_ibrt_call_redial_test(void)
{
    uint8_t device_id = app_bt_audio_get_device_for_user_action();
    if(device_id != BT_DEVICE_INVALID_ID){
        app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_REDIAL, 0, 0);
    }
}


void app_ibrt_tws_set_audio_mute_test(void)
{
}

void app_ibrt_tws_clear_audio_mute_test(void)
{
}

void app_ibrt_tws_get_audio_mute_test(void)
{
}

void app_ibrt_tws_get_role_switch_done_test(void)
{
}

#if defined(IBRT_UI_V2)
void app_ibrt_mgr_open_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
}

void app_ibrt_mgr_fetch_out_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_UNDOCK);
}

void app_ibrt_mgr_put_in_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_DOCK);
}

void app_ibrt_mgr_close_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
}

void app_ibrt_mgr_wear_up_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_UP);
}

void app_ibrt_mgr_wear_down_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_DOWN);
}

void app_ibrt_mgr_free_man_test(void)
{
    app_ibrt_if_test_enter_freeman();
}

void app_ibrt_mgr_free_man_test_1(void)
{
    app_ibrt_if_event_entry(IBRT_MGR_EV_FREE_MAN_MODE);
}

void app_ibrt_mgr_dump_info_test(void)
{
    app_ibrt_if_dump_ibrt_mgr_info();
}

void app_ibrt_mgr_choice_mobile_connect_test()
{
    ibrt_mgr_choice_mobile_connect(&remote_addr_2);
}

#if 0
void app_ibrt_mgr_if_unit_test()
{
    ibrt_mgr_evt_t active_evt = IBRT_MGR_EV_NONE;

    if(app_ibrt_if_event_has_been_queued(&remote_addr_2,IBRT_MGR_EV_CASE_OPEN))
    {
        TRACE(0,"if_unit_test cached event");
    }
    else
    {
        TRACE(0,"if_unit_test haven't cached event");
    }

    active_evt = app_ibrt_if_get_remote_dev_active_event(&remote_addr_2);
    TRACE(1,"active evt = %s",app_ibrt_if_ui_event_to_string(active_evt));
}
#endif

#endif

static void app_prompt_PlayAudio(void)
{
    media_PlayAudio(AUD_ID_BT_CALL_INCOMING_CALL, 0);
}

static void app_prompt_locally_PlayAudio(void)
{
    media_PlayAudio_locally(AUD_ID_BT_CALL_INCOMING_CALL, 0);
}

static void app_prompt_remotely_PlayAudio(void)
{
    media_PlayAudio_remotely(AUD_ID_BT_CALL_INCOMING_CALL, 0);
}

static void app_prompt_standalone_PlayAudio(void)
{
    media_PlayAudio_standalone(AUD_ID_BT_CALL_INCOMING_CALL, 0);
}

static void app_prompt_test_stop_all(void)
{
    app_prompt_stop_all();
}

extern void bt_change_to_iic(APP_KEY_STATUS *status, void *param);
void app_ibrt_ui_iic_uart_switch_test(void)
{
    bt_change_to_iic(NULL,NULL);
}

int parse_bt_addr(unsigned char *addr_buf, bt_bdaddr_t *bt_addr, unsigned int lenth)
{
    //TRACE(3, "%s recv bt: %s, len: %d",__func__, addr_buf, strlen((char*)addr_buf));
    //TRACE(1, "len: %d",lenth);
    if(lenth<12){
        TRACE(0, "invalid recv bt addr");
        return 0;
    }
    char bt_addr_buff[3];
    /*for(int i=0;i<6; i++){
        memset(&bt_addr_buff, 0, sizeof(bt_addr_buff) - 1);
        strncpy(bt_addr_buff, (char *)(addr_buf+10 -2*i), 2);
        bt_addr->address[i] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
        }*/

    memset(&bt_addr_buff, 0, sizeof(bt_addr_buff) - 1);
    //unsigned char * temp_addr_buf;
    //temp_addr_buf = addr_buf;
    strncpy(bt_addr_buff, (char *)(addr_buf+10), 2);
    bt_addr->address[0] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    strncpy(bt_addr_buff, (char *)(addr_buf+8), 2);
    bt_addr->address[1] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    strncpy(bt_addr_buff, (char *)(addr_buf + 6), 2);
    bt_addr->address[2] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    strncpy(bt_addr_buff, (char *)(addr_buf + 4), 2);
    bt_addr->address[3] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    strncpy(bt_addr_buff, (char *)(addr_buf +2), 2);
    bt_addr->address[4] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    strncpy(bt_addr_buff, (char *)(addr_buf), 2);
    bt_addr->address[5] = (unsigned char)strtol(bt_addr_buff, NULL, 16);
    //TRACE(8,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
            //bt_addr->address[5], bt_addr->address[4], bt_addr->address[3],
            //bt_addr->address[2], bt_addr->address[1], bt_addr->address[0]);
    return 1;
}

static void app_trace_test_enable(void)
{
    if (trace_disabled) {
        trace_disabled = false;
        hal_trace_continue();
}
}
extern void bt_drv_reg_op_trigger_controller_assert(void);

static void app_trace_test_disable(void)
{
    if (!trace_disabled) {
        trace_disabled = true;
        hal_trace_pause();
}
}

static void app_trace_test_get_history_trace(void)
{
    const uint8_t* preBuf;
    const uint8_t* postBuf;
    uint32_t preBufLen, postBufLen;
    hal_trace_get_history_buffer(&preBuf, &preBufLen, &postBuf, &postBufLen);
    TRACE(0, "prelen %d postlen %d", preBufLen, postBufLen);
    uint8_t tmpTest[16];
    memset(tmpTest, 0, sizeof(tmpTest));
    memcpy(tmpTest, preBuf,
        (preBufLen > (sizeof(tmpTest)-1)?(sizeof(tmpTest)-1):preBufLen));
    TRACE(0, "preBuf:");
    TRACE(0, "%s", tmpTest);
    memset(tmpTest, 0, sizeof(tmpTest));
    memcpy(tmpTest, postBuf,
        (postBufLen > (sizeof(tmpTest)-1)?(sizeof(tmpTest)-1):postBufLen));
    TRACE(0, "postBuf:");
    TRACE(0, "%s", tmpTest);
}

#if defined( __BT_ANC_KEY__)&&defined(ANC_APP)
extern void app_anc_key(APP_KEY_STATUS *status, void *param);
static void app_test_anc_handler(void)
{
    app_anc_key(NULL, NULL);
}
#endif

#ifdef __AI_VOICE__
extern void app_test_toggle_hotword_supervising(void);
static void app_test_thirdparty_hotword(void)
{
    app_test_toggle_hotword_supervising();
}
#endif
#ifdef SENSOR_HUB
#include "mcu_sensor_hub_app.h"

static void app_test_mcu_sensorhub_demo_req_no_rsp(void)
{
    app_mcu_sensor_hub_send_demo_req_no_rsp();
}

static void app_test_mcu_sensorhub_demo_req_with_rsp(void)
{
    app_mcu_sensor_hub_send_demo_req_with_rsp();
}

static void app_test_mcu_sensorhub_demo_instant_req(void)
{
    app_mcu_sensor_hub_send_demo_instant_req();
}
#endif

// free tws test steps:
/*
Preparation:
1. Assure following macros are proper configured in target.mk
   POWER_ON_ENTER_TWS_PAIRING_ENABLED ?= 0
   FREE_TWS_PAIRING_ENABLED ?= 1
2. Program device A with bt addr testMasterBdAddr, select erase
   the whole flash when programming
3. Program device B with bt addr testSlaveBdAddr, select erase
   the whole flash when programming
Case 1: Do pairing info nv record update and TWS pairing
- Do preparation
- Call uart cmd "ibrt_test:test_master_tws_pairing" on device A
- Call uart cmd "ibrt_test:test_slave_tws_pairing" on device B
The device A and B will be successfully paired, after that,
the master bud A will enter discoverable mode for mobile to discover and connect

Case 2: Do pairing info nv record update only firslty, then trigger reboot and
        let TWS paired
- Do preparation
- Call uart cmd "ibrt_test:master_update_tws_pair_info_test_func" on device A
- Call uart cmd "ibrt_test:slave_update_tws_pair_info_test_func" on device B
- Call uart cmd "ibrt_test:soft_reset" on device A and B
- After A and B reboot
- Call uart cmd "ibrt_test:open_box_event_test" on device A and B
The device A and B will be successfully paired
- Call uart cmd "ibrt_test:enable_access_mode_test" on device A,
the master bud A will enter discoverable mode for mobile to discover and connect

*/

static uint8_t testMasterBdAddr[6] =
{0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

static uint8_t testSlaveBdAddr[6] =
{0x22, 0x22, 0x22, 0x22, 0x22, 0x22};

static void test_master_tws_pairing(void)
{
    app_ibrt_if_init_open_box_state_for_evb();
    app_ibrt_if_start_tws_pairing(IBRT_MASTER, testSlaveBdAddr);
}

static void test_slave_tws_pairing(void)
{
    app_ibrt_if_init_open_box_state_for_evb();
    app_ibrt_if_start_tws_pairing(IBRT_SLAVE, testMasterBdAddr);
}

static void master_update_tws_pair_info_test_func(void)
{
    app_ibrt_if_update_tws_pairing_info(IBRT_MASTER, testSlaveBdAddr);
}

static void slave_update_tws_pair_info_test_func(void)
{
    app_ibrt_if_update_tws_pairing_info(IBRT_SLAVE, testMasterBdAddr);
}

static void m_write_bt_local_addr_test(void)
{
    app_ibrt_if_write_bt_local_address(testMasterBdAddr);
}

static void s_write_bt_local_addr_test(void)
{
    app_ibrt_if_write_bt_local_address(testSlaveBdAddr);
}


static void turn_on_jlink_test(void)
{
    hal_iomux_set_jtag();
    hal_cmu_jtag_clock_enable();
}

void increase_dst_mtu(void);
void decrease_dst_mtu(void);

void dip_test(void)
{
    if(app_dip_function_enable() == true){
        ibrt_if_pnp_info* pnp_info = NULL;
        struct BT_DEVICE_T *curr_device = NULL;

        curr_device = app_bt_get_device(BT_DEVICE_ID_1);

        if (curr_device)
        {
            pnp_info = app_ibrt_if_get_pnp_info(&curr_device->remote);
        }

        if (pnp_info)
        {
            TRACE(4, "%s vendor_id %04x product_id %04x product_version %04x",
                    __func__, pnp_info->vend_id, pnp_info->prod_id, pnp_info->prod_ver);
        }
        else
        {
            TRACE(1, "%s N/A", __func__);
        }
    }
}

#ifdef SLT_TEST_ON
extern int app_reset(void);
void app_poweroff_test(void)
{
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    pmu_shutdown();
}

void app_pmu_reboot_test()
{
    app_reset();
}

#ifdef ANC_APP
void enable_mode1(void)
{
    TRACE(0, "enable_mode1");
    app_anc_switch(APP_ANC_MODE1);
}
void enable_mode2(void)
{
    TRACE(0, "enable_mode2");
    app_anc_switch(APP_ANC_MODE2);
}

void disable_mode(void)
{
    TRACE(0, "disable_mode");
    app_anc_switch(APP_ANC_MODE_OFF);
}
#endif
#endif


/*
* Connection step: tws_slave_pairing_test->tws_master_pairing_test->
* w4_host_connect -> start_ibrt_test
*/
const app_bt_cmd_handle_t app_ibrt_v2_uart_test_handle[]=
{
    {"soft_reset", app_ibrt_soft_reset_test},

#if defined(IBRT_UI_V2)
    {"open_box_event_test",     app_ibrt_mgr_open_box_event_test},
    {"fetch_out_box_event_test",app_ibrt_mgr_fetch_out_box_event_test},
    {"put_in_box_event_test",   app_ibrt_mgr_put_in_box_event_test},
    {"close_box_event_test",    app_ibrt_mgr_close_box_event_test},
    {"wear_up_event_test",      app_ibrt_mgr_wear_up_event_test},
    {"wear_down_event_test",    app_ibrt_mgr_wear_down_event_test},
    {"pairing_mode_test",       app_ibrt_mgr_pairing_mode_test},
    {"free_man_test",           app_ibrt_mgr_free_man_test_1},
    {"dump_mgr_info_test",      app_ibrt_mgr_dump_info_test},
    {"choice_connect_test",     app_ibrt_mgr_choice_mobile_connect_test},
    //{"ibrt_if_unittest",        app_ibrt_mgr_if_unit_test},
#endif
    {"iic_switch", app_ibrt_ui_iic_uart_switch_test},
    {"enable_access_mode_test",app_ibrt_enable_access_mode_test},
    {"disable_access_mode_test", app_ibrt_disable_access_mode_test},
    {"enter_freeman_test",app_ibrt_tws_freeman_pairing_test},
    {"tws_connect_test",app_ibrt_tws_connect_test},
    {"tws_disconnect_test",app_ibrt_tws_disconnect_test},
    {"mobile1_connect_test",app_ibrt_mobile_connect_test_1},
    {"mobile2_connect_test",app_ibrt_mobile_connect_test_2},
    {"mobile3_connect_test",app_ibrt_mobile_connect_test_3},
    {"mobile1_disconnect_test",app_ibrt_mobile_disconnect_test_1},
    {"mobile2_disconnect_test",app_ibrt_mobile_disconnect_test_2},
    {"mobile3_disconnect_test",app_ibrt_mobile_disconnect_test_3},
    {"connect_profiles_test",app_ibrt_connect_profiles_test},
    {"host_cancel_connect_test",app_ibrt_host_connect_cancel_test},
    {"start_ibrt1_test",app_ibrt_start_ibrt_test_1},
    {"start_ibrt2_test",app_ibrt_start_ibrt_test_2},
    {"start_ibrt3_test",app_ibrt_start_ibrt_test_3},
    {"stop_ibrt1_test",app_ibrt_stop_ibrt_test_1},
    {"stop_ibrt2_test",app_ibrt_stop_ibrt_test_2},
    {"stop_ibrt3_test",app_ibrt_stop_ibrt_test_3},
    {"tws_get_role_test",app_ibrt_tws_role_get_request_test},
    {"tws_role_swap_test_1",app_ibrt_tws_role_switch_test_1},
    {"tws_role_swap_test_2",app_ibrt_tws_role_switch_test_2},
    {"tws_role_swap_test_3",app_ibrt_tws_role_switch_test_3},
    {"disconect_all_test",app_ibrt_tws_disconnect_all_connection_test},
    {"w4_mobile_connect_test",app_ibrt_tws_w4_mobile_connect_test},
    {"get_curr_link_count_test",app_ibrt_get_remote_device_count},
    {"dump_ibrt_conn_info",app_ibrt_tws_dump_info_test},

    {"get_master_mobile_rssi_test",app_ibrt_tws_get_master_mobile_rssi_test},
    {"get_master_slave_rssi_test",app_ibrt_tws_get_master_slave_rssi_test},
    {"get_slave_mobile_rssi_test",app_ibrt_tws_get_slave_mobile_rssi_test},
    {"get_peer_mobile_rssi_test",app_ibrt_tws_get_peer_mobile_rssi_test},
    {"ibrt_function_test",app_ibrt_service_fuction_test},
    {"get_mobile_conn_status_test",app_ibrt_tws_mobile_connection_test},
    //{"get_a2dp_state_test",app_ibrt_get_a2dp_state_test},
    //{"get_avrcp_state_test",app_ibrt_get_avrcp_state_test},
    //{"get_hfp_state_test",app_ibrt_get_hfp_state_test},
    //{"get_call_status",app_ibrt_get_call_status_test},
    //{"get_ibrt_role",app_ibrt_get_ibrt_role_test},
    {"get_tws_state",app_ibrt_get_tws_state_test},
    {"avrcp_pause_test",app_ibrt_tws_avrcp_pause_test},
    {"avrcp_play_test",app_ibrt_tws_avrcp_play_test},
    {"avrcp_toggle_test",app_ibrt_tws_avrcp_toggle_test},
    {"avrcp_vol_up_test",app_ibrt_tws_avrcp_vol_up_test},
    {"avrcp_vol_down_test",app_ibrt_tws_avrcp_vol_down_test},
    {"avrcp_next_track_test",app_ibrt_tws_avrcp_next_track_test},
    {"avrcp_prev_track_test",app_ibrt_tws_avrcp_prev_track_test},
    {"a2dp_active_phone",app_ibrt_ui_get_a2dp_active_phone},
    {"hfp_answer_test",app_ibrt_tws_hfp_answer_test},
    {"hfp_reject_test",app_ibrt_tws_hfp_reject_test},
    {"hfp_hangup_test",app_ibrt_tws_hfp_hangup_test},
    {"hfp_mute_test",app_ibrt_tws_hfp_mute_test},
    {"hfp_unmute_test",app_ibrt_tws_hfp_unmute_test},
    {"hfp_assisant_test",app_ibrt_tws_hfp_assisant_test},
    {"hfp_vol_up_test",app_ibrt_tws_hfp_vol_up_test},
    {"hfp_vol_down_test",app_ibrt_tws_hfp_vol_down_test},
    {"call_redial",app_ibrt_call_redial_test},
    {"set_audio_mute_test",app_ibrt_tws_set_audio_mute_test},
    {"clear_audio_mute_test",app_ibrt_tws_clear_audio_mute_test},
    {"get_audio_mute_test",app_ibrt_tws_get_audio_mute_test},
    {"get_role_switch_done_test",app_ibrt_tws_get_role_switch_done_test},
    {"app_prompt_PlayAudio",app_prompt_PlayAudio},
    {"app_prompt_locally_PlayAudio",app_prompt_locally_PlayAudio},
    {"app_prompt_remotely_PlayAudio",app_prompt_remotely_PlayAudio},
    {"app_prompt_standalone_PlayAudio",app_prompt_standalone_PlayAudio},
    {"app_prompt_test_stop_all",app_prompt_test_stop_all},
    {"trace_test_enable",app_trace_test_enable},
    {"trace_test_disable",app_trace_test_disable},
    {"test_get_history_trace",app_trace_test_get_history_trace},
#if defined( __BT_ANC_KEY__)&&defined(ANC_APP)
    {"toggle_anc_test",app_test_anc_handler},
#endif
#ifdef __AI_VOICE__
    {"toggle_hw_thirdparty",app_test_thirdparty_hotword},
#endif
#ifdef SENSOR_HUB
    {"no_rsp_sensorhub_mcu_demo_cmd",app_test_mcu_sensorhub_demo_req_no_rsp},
    {"with_rsp_sensorhub_mcu_demo_cmd",app_test_mcu_sensorhub_demo_req_with_rsp},
    {"instant_sensorhub_mcu_demo_cmd",app_test_mcu_sensorhub_demo_instant_req},
#endif

    {"test_master_tws_pairing",test_master_tws_pairing},
    {"test_slave_tws_pairing",test_slave_tws_pairing},
    {"master_update_tws_pair_info_test_func",master_update_tws_pair_info_test_func},
    {"slave_update_tws_pair_info_test_func",slave_update_tws_pair_info_test_func},

    {"trigger_controller_assert",bt_drv_reg_op_trigger_controller_assert},
    {"m_write_bt_local_addr_test",m_write_bt_local_addr_test},
    {"s_write_bt_local_addr_test",s_write_bt_local_addr_test},
    {"turn_on_jlink_test",turn_on_jlink_test},
    {"increase_dst_mtu",increase_dst_mtu},
    {"decrease_dst_mtu",decrease_dst_mtu},
    {"dip_test",dip_test},
#ifdef SLT_TEST_ON
    {"power_off",app_poweroff_test},
    {"pmu_reset",app_pmu_reboot_test},
#ifdef ANC_APP
    {"enable_mode1", enable_mode1},
    {"enable_mode2", enable_mode2},
    {"disable_mode", disable_mode},
#endif
#endif

};


const app_uart_handle_t_p app_ibrt_uart_test_handle_p[]=
{
    {"get_a2dp_state_test",app_ibrt_get_a2dp_state_test},
    {"get_avrcp_state_test",app_ibrt_get_avrcp_state_test},
    {"get_hfp_state_test",app_ibrt_get_hfp_state_test},
    {"get_call_status",app_ibrt_get_call_status_test},
    {"tws_get_role_test",app_ibrt_tws_role_get_request_test_with_parameter}
};

void app_ibrt_ui_v2_add_test_cmd_table(void)
{
    app_bt_cmd_add_test_table(app_ibrt_v2_uart_test_handle, ARRAY_SIZE(app_ibrt_v2_uart_test_handle));
}

app_uart_test_function_handle_with_param app_ibrt_ui_find_uart_handle_with_param(unsigned char* buf)
{
    app_uart_test_function_handle_with_param p = NULL;
    for(uint8_t i = 0; i<sizeof(app_ibrt_uart_test_handle_p)/sizeof(app_uart_handle_t); i++)
    {
        if(strncmp((char*)buf, app_ibrt_uart_test_handle_p[i].string, strlen(app_ibrt_uart_test_handle_p[i].string))==0 ||
           strstr(app_ibrt_uart_test_handle_p[i].string, (char*)buf))
        {
            p = app_ibrt_uart_test_handle_p[i].function;
            break;
        }
    }
    return p;
}

int app_ibrt_raw_ui_test_cmd_handler_with_param(unsigned char *buf, unsigned char *param, unsigned int length)
{
    //TRACE(2,"%s execute command: %s\n",__func__, buf);
    //TRACE(1,"parameter: %s, len: %d \n", param, strlen((char *)param));
    //TRACE(1,"len: %d \n", length);
    int ret = 0;
    if (buf[length-2] == 0x0d ||
        buf[length-2] == 0x0a)
    {
        buf[length-2] = 0;
    }
    app_uart_test_function_handle_with_param handl_function = app_ibrt_ui_find_uart_handle_with_param(buf);
    if(handl_function)
    {
        handl_function(param, length);
    }
    else
    {
        ret = -1;
        TRACE(0, "can not find handle function");
    }
    return ret;
}

void app_tws_ibrt_test_key_io_event(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"app_tws_ibrt_test_key_io_event");
    TRACE(3, "%s 000%d,%d",__func__, status->code, status->event);
#if defined(IBRT_UI_V2)
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_UNDOCK);
            }
            else
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_UP);
            }
            break;

        case APP_KEY_EVENT_DOUBLECLICK:
            if (status->code== APP_KEY_CODE_FN1)
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
            }
            else if (status->code== APP_KEY_CODE_FN2)
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_DOCK);
            }
            else
            {
                app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_DOWN);
            }
            break;

        case APP_KEY_EVENT_LONGPRESS:
            if (status->code== APP_KEY_CODE_FN1)
            {

            }
            else if (status->code== APP_KEY_CODE_FN2)
            {

            }
            else
            {
            }
            break;

        case APP_KEY_EVENT_TRIPLECLICK:
            break;

        case HAL_KEY_EVENT_LONGLONGPRESS:
            break;

        case APP_KEY_EVENT_ULTRACLICK:
            break;

        case APP_KEY_EVENT_RAMPAGECLICK:
            break;
    }
#endif
}

void app_ibrt_raw_ui_test_key(APP_KEY_STATUS *status, void *param)
{
    uint8_t shutdown_key = HAL_KEY_EVENT_LONGLONGPRESS;
    uint8_t device_id = app_bt_audio_get_device_for_user_action();
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    TRACE(4, "%s %d,%d curr_device %x", __func__, status->code, status->event, device_id);

    if (IBRT_SLAVE == app_tws_get_ibrt_role(&curr_device->remote) && status->event != shutdown_key)
    {
        app_ibrt_if_keyboard_notify_v2(&curr_device->remote, status, param);
    }
    else
    {
#ifdef IBRT_SEARCH_UI
        app_ibrt_search_ui_handle_key_v2(&curr_device->remote, status, param);
#else
        app_ibrt_normal_ui_handle_key_v2(&curr_device->remote, status, param);
#endif
    }
}

const APP_KEY_HANDLE  app_ibrt_ui_v2_test_key_cfg[] =
{
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN}, "google assistant key", app_ai_manager_key_event_handle, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP}, "google assistant key", app_ai_manager_key_event_handle, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS}, "google assistant key", app_ai_manager_key_event_handle, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK}, "google assistant key", app_ai_manager_key_event_handle, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK}, "google assistant key", app_ai_manager_key_event_handle, NULL},
#endif
#if defined(__BT_ANC_KEY__)&&defined(ANC_APP)
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt anc key",app_anc_key, NULL},
#else
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
#endif
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"app_ibrt_ui_test_key", app_ibrt_raw_ui_test_key, NULL},

    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_CLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_DOUBLECLICK},"app_ibrt_service_test_key", app_tws_ibrt_test_key_io_event, NULL},
};

#define IS_ENABLE_RSSI_LOG_PRINTx

#ifdef IS_ENABLE_RSSI_LOG_PRINT

osTimerId  app_ibrt_rssi_print_timer_id;
static void app_ibrt_rssi_print_timer_handler(void const *current_evt);

osTimerDef (IBRT_UI_RSSI_PRINT_TIMER, app_ibrt_rssi_print_timer_handler);

static void app_ibrt_rssi_print_timer_handler(void const *current_evt)
{
#if 0
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (app_tws_get_ibrt_role(mobile_addr) == IBRT_MASTER)
    {
        app_ibrt_tws_get_master_mobile_rssi_test();
        app_ibrt_tws_get_master_slave_rssi_test();
    }
    else if (app_tws_get_ibrt_role(mobile_addr) == IBRT_SLAVE)
    {
        app_ibrt_tws_get_slave_mobile_rssi_test();
        app_ibrt_tws_get_master_slave_rssi_test();
    }
#endif
}
#endif

void app_tws_ibrt_raw_ui_test_key_init(void)
{
    TRACE(0,"app_tws_ibrt_raw_ui_test_key_init");
    app_key_handle_clear();
    for (uint8_t i=0; i<ARRAY_SIZE(app_ibrt_ui_v2_test_key_cfg); i++)
    {
        app_key_handle_registration(&app_ibrt_ui_v2_test_key_cfg[i]);
    }

#ifdef IS_ENABLE_RSSI_LOG_PRINT
    app_ibrt_rssi_print_timer_id = osTimerCreate(osTimer(IBRT_UI_RSSI_PRINT_TIMER), \
                                     osTimerPeriodic, NULL);

    osTimerStart(app_ibrt_rssi_print_timer_id, 1000);
#endif
}

#endif
