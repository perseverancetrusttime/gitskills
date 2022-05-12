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
//#include "mbed.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "apps.h"
#include "app_status_ind.h"
#include "app_utils.h"
#include "app_bt_stream.h"
#include "app_a2dp.h"
#include "app_hfp.h"
#include "nvrecord_dev.h"
#include "tgt_hardware.h"
#include "besbt_cfg.h"
#include "hfp_api.h"
#include "app_bt_func.h"
#include "bt_if.h"
#include "os_api.h"
#include "dip_api.h"
#include "sdp_api.h"
#include "intersyshci.h"

#ifdef BT_HID_DEVICE
#include "app_bt_hid.h"
#endif

#ifdef BTIF_DIP_DEVICE
#include "app_dip.h"
#endif

#ifdef TEST_OVER_THE_AIR_ENANBLED
#include "app_tota.h"
#endif

extern "C" {
#ifdef __IAG_BLE_INCLUDE__
#include "rwapp_config.h"
#include "besble.h"
#include "app.h"
#endif

#ifdef TX_RX_PCM_MASK
#include "hal_intersys.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#endif

#include "bt_drv_interface.h"
#include "app_btgatt.h"
#include "l2cap_api.h"

#ifdef __AI_VOICE__
#include "app_ai_if.h"
#endif

#ifdef GFPS_ENABLED
#include "app_fp_rfcomm.h"
#endif
} /// extern "C"

#include "besbt.h"
#include "cqueue.h"
#include "app_bt.h"
#include "btapp.h"

#if defined(BT_SOURCE)
#include "bt_source.h"
#endif

#if defined(BT_MAP_SUPPORT)
#include "app_btmap_sms.h"
#endif

#if defined(IBRT)
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "app_bt_cmd.h"
#include "app_ibrt_if.h"
#include "app_ibrt_custom_cmd.h"
#endif

#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif

#if defined(IBRT)
rssi_t raw_rssi[2];
#endif

struct besbt_cfg_t besbt_cfg = {
    .dip_vendor_id = 0,
    .dip_product_id = 0,
    .dip_product_version = 0,
#ifdef BT_HFP_DONT_SUPPORT_APPLE_HF_AT_COMMAND
    .apple_hf_at_support = false,
#else
    .apple_hf_at_support = true,
#endif
#ifdef BT_HFP_DONT_SUPPORT_CLI_FEATURE
    .hf_dont_support_cli_feature = true,
#else
    .hf_dont_support_cli_feature = false,
#endif
#ifdef BT_HFP_DONT_SUPPORT_ENHANCED_CALL_FEATURE
    .hf_dont_support_enhanced_call = true,
#else
    .hf_dont_support_enhanced_call = false,
#endif
#ifdef BT_HFP_SUPPORT_HF_INDICATORS_FEATURE
    .hf_support_hf_ind_feature = true,
#else
    .hf_support_hf_ind_feature = false,
#endif
#ifdef __BTIF_SNIFF__
    .sniff = true,
#else
    .sniff = false,
#endif
#if defined(BT_DONT_AUTO_REPORT_DELAY_REPORT)
    .dont_auto_report_delay_report = true,
#else
    .dont_auto_report_delay_report = false,
#endif
#if defined(SCO_ADD_CUSTOMER_CODEC)
    .vendor_codec_en = true,
#else
    .vendor_codec_en = false,
#endif
#if defined(SCO_FORCE_CVSD)
    .force_use_cvsd = true,
#else
    .force_use_cvsd = false,
#endif
#if defined(BT_OBEX_SUPPORT)
    .support_enre_mode = true,
#else
    .support_enre_mode = false,
#endif
#ifdef BT_HID_DEVICE
    .bt_hid_cod_enable = true,
#else
    .bt_hid_cod_enable = false,
#endif
#ifdef BT_WATCH_APP
    .bt_watch_enable = true,
#else
    .bt_watch_enable = false,
#endif
#ifdef __A2DP_AVDTP_CP__
    .avdtp_cp_enable = true,
#else
    .avdtp_cp_enable = false,
#endif
    .bt_source_enable = false,
    .bt_sink_enable = true,
#if defined (A2DP_LHDC_V3) || defined (A2DP_SOURCE_LHDC_ON)
    .lhdc_v3 = true,
#else
    .lhdc_v3 = false,
#endif
    .hfp_ag_pts_enable = false,
    .hfp_ag_pts_ecs_01 = false,
    .hfp_ag_pts_ecs_02 = false,
    .hfp_ag_pts_ecc = false,
    .acl_tx_flow_debug = false,
    .hci_tx_cmd_debug = false,
#ifdef BT_DONT_PLAY_MUTE_WHEN_A2DP_STUCK_PATCH
    .dont_play_mute_when_a2dp_stuck = true,
#else
    .dont_play_mute_when_a2dp_stuck = false,
#endif
#ifdef IBRT
    .start_ibrt_reserve_buff = true,
#else
    .start_ibrt_reserve_buff = false,
#endif
    .send_l2cap_echo_req = false,
    .a2dp_play_stop_media_first = true,
#ifdef BT_DISC_ACL_AFTER_AUTH_KEY_MISSING
    .disc_acl_after_auth_key_missing = true,
#else
    .disc_acl_after_auth_key_missing = false,
#endif
#ifdef USE_PAGE_SCAN_REPETITION_MODE_R1
    .use_page_scan_repetition_mode_r1 = true,
#else
    .use_page_scan_repetition_mode_r1 = false,
#endif
#ifdef __FUZZ_TEST_SUPPORT__
    .fuzz_test = true,
#else
    .fuzz_test = false,
#endif
};

osMessageQDef(evm_queue, 128, uint32_t);
osMessageQId  evm_queue_id;
#define BT_STATE_CHECKER_INTERVAL_MS 5000
#if defined(GET_PEER_RSSI_ENABLE)
#define GET_PEER_RSSI_CMD_INTERVAL_MS 3000
#endif

/* besbt thread */
#ifndef BESBT_STACK_SIZE
#define BESBT_STACK_SIZE (3326)
#endif

osThreadDef(BesbtThread, (osPriorityAboveNormal), 1, (BESBT_STACK_SIZE), "bes_bt_main");


static BESBT_HOOK_HANDLER bt_hook_handler[BESBT_HOOK_USER_QTY] = {0};

int Besbt_hook_handler_set(enum BESBT_HOOK_USER_T user, BESBT_HOOK_HANDLER handler)
{
    bt_hook_handler[user]= handler;
    return 0;
}

static void Besbt_hook_proc(void)
{
    uint8_t i;
    for (i=0; i<BESBT_HOOK_USER_QTY; i++){
        if (bt_hook_handler[i]){
            bt_hook_handler[i]();
        }
    }
}

unsigned char *bt_get_local_address(void)
{
    return bt_addr;
}

void bt_set_local_address(unsigned char* btaddr)
{
    if (btaddr != NULL) {
        memcpy(bt_addr, btaddr, BTIF_BD_ADDR_SIZE);
    }
}

void bt_set_ble_local_address(uint8_t* bleAddr)
{
    if (bleAddr)
    {
        memcpy(ble_addr, bleAddr, BTIF_BD_ADDR_SIZE);
    }
}

unsigned char *bt_get_ble_local_address(void)
{
    return ble_addr;
}

const char *bt_get_local_name(void)
{
    return BT_LOCAL_NAME;
}

void bt_set_local_name(const char* name)
{
    if (name != NULL) {
        BT_LOCAL_NAME = name;
    }
}

const char *bt_get_ble_local_name(void)
{
    return BLE_DEFAULT_NAME;
}

void bt_key_init(void);
void pair_handler_func(enum pair_event evt, const btif_event_t *event);
#ifdef BTIF_SECURITY
void auth_handler_func();
#endif

typedef void (*bt_hci_delete_con_send_complete_cmd_func)(uint16_t handle,uint8_t num);
extern "C" void register_hci_delete_con_send_complete_cmd_callback(bt_hci_delete_con_send_complete_cmd_func func);
extern "C" void HciSendCompletePacketCommandRightNow(uint16_t handle,uint8_t num);

void gen_bt_addr_for_debug(void)
{
    static const char host[] = TO_STRING(BUILD_HOSTNAME);
    static const char user[] = TO_STRING(BUILD_USERNAME);
    uint32_t hlen, ulen;
    uint32_t i, j;
    uint32_t addr_size = BTIF_BD_ADDR_SIZE;

    hlen = strlen(host);
    ulen = strlen(user);

    TRACE(0,"Configured BT addr is:");
    DUMP8("%02x ", bt_addr, BT_ADDR_OUTPUT_PRINT_NUM);

    j = 0;
    for (i = 0; i < hlen; i++) {
        bt_addr[j++] ^= host[i];
        if (j >= addr_size / 2) {
            j = 0;
        }
    }

    j = addr_size / 2;
    for (i = 0; i < ulen; i++) {
        bt_addr[j++] ^= user[i];
        if (j >= addr_size) {
            j = addr_size / 2;
        }
    }

    TRACE(0,"Modified debug BT addr is:");
    DUMP8("%02x ", bt_addr, BT_ADDR_OUTPUT_PRINT_NUM);
}

static void add_randomness(void)
{
    uint32_t generatedSeed = hal_sys_timer_get();

    //avoid bt address collision low probability
    for (uint8_t index = 0; index < sizeof(bt_addr); index++) {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
    }
    srand(generatedSeed);
}

static void __set_bt_sco_num(void)//sco连接允许主单元连接三个从单元，从单元可以连接两个主单元
{
    uint8_t sco_num = 1;
#if defined(__BT_ONE_BRING_TWO__)
    sco_num = 1;
#elif defined(IBRT_V2_MULTIPOINT)
    sco_num = 2;
#endif
    bt_set_max_sco_number(sco_num);
    TRACE(1,"sco_num=%d\n",sco_num);
}

void app_notify_stack_ready(uint8_t ready_flag);

static void stack_ready_callback(int status)
{
    dev_addr_name devinfo;
    uint32_t clock = 0;

    devinfo.btd_addr = bt_get_local_address();
    devinfo.ble_addr = bt_get_ble_local_address();
    devinfo.localname = bt_get_local_name();
    devinfo.ble_name = bt_get_ble_local_name();

#ifndef FPGA
    nvrec_dev_localname_addr_init(&devinfo);
#endif

    bt_set_local_dev_name((const unsigned char*)devinfo.localname, strlen(devinfo.localname) + 1);
    //Change bt local clock aim to avoid pscan using same freq on both side and cause connected by phone at same time.
    //pscan only use CLKN 16-12 to calculate frequency, so use rand() to set CLKN 16-12
    clock = (rand()&0x1F)<<12;
    bt_set_local_clock(clock);
    bt_stack_config((const unsigned char*)devinfo.localname, strlen(devinfo.localname) + 1);

#ifdef __IAG_BLE_INCLUDE__
    app_init_ble_name(devinfo.ble_name);
#endif

    app_notify_stack_ready(STACK_READY_BT);//经典蓝牙的协议栈准备完成
}

void l2cap_process_echo_req_func(uint8_t device_id, uint16_t conhdl, uint8_t id, uint16_t len, uint8_t *data)
{
    TRACE(0,"process_echo_req rxdata:\n");
    DUMP8("%x ",data,len);
    //uint8_t echo_data[10] = {0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01};

    //rewite echo data when receive echo req
    //memcpy(data,echo_data,10);

    btif_l2cap_process_echo_req_rewrite_rsp_data(device_id,conhdl,id,len,data);
}

void l2cap_fill_in_echo_req_data_func(uint8_t device_id, struct l2cap_conn *conn, uint8_t *data, uint16_t data_len)
{
    TRACE(0,"fill_in_echo_req_data data:\n");
    uint8_t echo_data[10] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};

    memcpy(data,&echo_data[0],10);
    DUMP8("%x ",data,10);
    data_len = 10;
    btif_l2cap_fill_in_echo_req_data(device_id,conn,data,data_len);
}

void l2cap_process_echo_res_func(uint8_t device_id, uint16_t conhdl, uint8_t* rxdata, uint16_t rxlen)
{
    TRACE(0,"process_echo_res rxdata: rxlen=%d\n",rxlen);
    DUMP8("%x ",rxdata,rxlen);

    //analyze echo data when receive echo res

    btif_l2cap_process_echo_res_analyze_data(device_id,conhdl,rxdata,rxlen);
}

int besmain(void)
{
    enum APP_SYSFREQ_FREQ_T sysfreq;
    uint32_t checkerSysTick = 0;
#if defined(GET_PEER_RSSI_ENABLE)
    uint32_t checkerRssiTick = 0;
#endif
#if !defined(BLE_ONLY_ENABLED)//未定义低功耗 
#ifdef A2DP_CP_ACCEL//未定义低功耗&定义了？？
    sysfreq = APP_SYSFREQ_26M;
#else//未定义低功耗和？？
    sysfreq = APP_SYSFREQ_52M;
#endif
#else//定义了低功耗
    sysfreq = APP_SYSFREQ_26M;
#endif

    BESHCI_Open();//主机控制器接口
#if defined( TX_RX_PCM_MASK)
    if(btdrv_is_pcm_mask_enable()==1)
        hal_intersys_mic_open(HAL_INTERSYS_ID_1,store_encode_frame2buff);
#endif
    __set_bt_sco_num();//设置连接从单元方式：一拖二/多点接入
    add_randomness();

#ifdef __IAG_BLE_INCLUDE__
    bes_ble_init();
#endif

    btif_set_btstack_chip_config(bt_drv_get_btstack_chip_config());//通过HCI接口去调用驱动来获取蓝牙协议栈的设置SCO路径的地址

#if defined(__GATT_OVER_BR_EDR__)
    btif_config_gatt_over_br_edr(true);//GATT设置设备的可发现、建立或断开连接、初始化安全管理、设备配置
#endif

    app_bt_manager_init();//A2DP、SCO连接配置

    /* bes stack init */
    bt_stack_initilize();//接口

    bt_stack_register_ready_callback(stack_ready_callback);//回调！！！传递函数名（即函数指针）
    btif_sdp_init();//初始化服务发现协议

    btif_cmgr_handler_init();

#if defined(BT_SOURCE)
    /* this init shall be in front of normal profile init */
    app_bt_source_init();
#endif

    /* normal profile init*/
    btif_avrcp_init();
    a2dp_init();
    app_hfp_init();

#ifdef __AI_VOICE__
    app_ai_init();
#endif

#ifdef GFPS_ENABLED
    app_fp_rfcomm_init();
#endif

#if defined(BT_MAP_SUPPORT)
    app_btmap_sms_init();
#endif

#if defined(__GATT_OVER_BR_EDR__)
    app_btgatt_init();
#endif

    /* pair callback init */
    bt_pairing_init(pair_handler_func);//回调函数，传递函数名（数指针 ）
#ifdef BT_HID_DEVICE
    app_bt_hid_init();
#endif

#ifdef BTIF_DIP_DEVICE
    app_dip_init();
#endif

#ifdef BT_PBAP_SUPPORT
    app_bt_pbap_init();
#endif

    btif_l2cap_echo_init(l2cap_process_echo_req_func, l2cap_process_echo_res_func,l2cap_fill_in_echo_req_data_func);//三个函数指针传递过去

#if defined(IBRT)
    app_ibrt_set_cmdhandle(TWS_CMD_IBRT, app_ibrt_cmd_table_get);
    app_ibrt_set_cmdhandle(TWS_CMD_CUSTOMER, app_ibrt_customif_cmd_table_get);
#if defined(BES_OTA) || defined(AI_OTA) || defined(__GMA_OTA_TWS__)
    app_ibrt_set_cmdhandle(TWS_CMD_IBRT_OTA, app_ibrt_ota_tws_cmd_table_get);
#endif
#ifdef __INTERACTION__
    app_ibrt_set_cmdhandle(TWS_CMD_OTA, app_ibrt_ota_cmd_table_get);
#endif

#ifdef AOB_UX_ENABLED
    app_ibrt_set_cmdhandle(TWS_CMD_AOB, app_ibrt_aob_cmd_table_get);
#endif

    tws_ctrl_thread_init();
#endif

    ///init bt key
    bt_key_init();//秘钥初始化，全1
#ifdef TEST_OVER_THE_AIR_ENANBLED
    app_tota_init();
#endif
    osapi_notify_evm();//不知道是？？？接口
    while(1) {
        app_sysfreq_req(APP_SYSFREQ_USER_BT_MAIN, APP_SYSFREQ_32K);
        osMessageGet(evm_queue_id, osWaitForever);
        app_sysfreq_req(APP_SYSFREQ_USER_BT_MAIN, sysfreq);//sysfreq=26K或52K
        //    BESHCI_LOCK_TX();
        #ifdef __LOCK_AUDIO_THREAD__
                bool stream_a2dp_sbc_isrun = app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC);//编解码方式SBC
                if (stream_a2dp_sbc_isrun) {
                    af_lock_thread();
                }
        #endif
                bt_process_stack_events();

        #ifdef __IAG_BLE_INCLUDE__
                bes_ble_schedule();
        #endif

                Besbt_hook_proc();//等级用户事件的执行

        #ifdef __LOCK_AUDIO_THREAD__
                if (stream_a2dp_sbc_isrun) {
                    af_unlock_thread();
                }
        #endif



        #if defined(IBRT)
                app_ibrt_data_send_handler();
                app_ibrt_data_receive_handler();
        #if defined(IBRT_UI_V1)
                app_ibrt_ui_controller_dbg_state_checker();
                app_ibrt_ui_stop_ibrt_condition_checker();
        #endif
        #endif


        app_check_pending_stop_sniff_op();//检查是否停止呼吸模式
        if(TICKS_TO_MS(hal_sys_timer_get()-checkerSysTick) > BT_STATE_CHECKER_INTERVAL_MS) {
            if (app_is_stack_ready())//蓝牙协议栈准备好了
            {
                app_bt_state_checker();
            }
            checkerSysTick = hal_sys_timer_get();
        }

        #if defined(GET_PEER_RSSI_ENABLE)
                if(TICKS_TO_MS(hal_sys_timer_get()-checkerRssiTick) > GET_PEER_RSSI_CMD_INTERVAL_MS) {
                    if (app_tws_is_connected())
                    {
                        app_bt_ibrt_rssi_status_checker();
                    }
                    checkerRssiTick = hal_sys_timer_get();
                }
#endif
    }

    return 0;
}

void BesbtThread(void const *argument)
{
    besmain();
}

static osThreadId besbt_tid;

bool app_bt_current_is_besbt_thread(void)
{
    return (osThreadGetId() == besbt_tid);
}

void BesbtInit(void)
{
    evm_queue_id = osMessageCreate(osMessageQ(evm_queue), NULL);
    /* bt */
    besbt_tid = osThreadCreate(osThread(BesbtThread), NULL);
    TRACE(1,"BesbtThread: %p\n", besbt_tid);
}
