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
#include "cmsis_os.h"
#include <string.h>
#include "hal_trace.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "nvrecord.h"
#include "export_fn_rom.h"
#include "app_ibrt_if.h"
#include "app_ibrt_if_internal.h"
#include "a2dp_decoder.h"
#include "app_ibrt_nvrecord.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "btapp.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#include "app_ibrt_a2dp.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "app_ibrt_hf.h"
#include "app_bt_stream.h"
#include "app_tws_ctrl_thread.h"
#include "app_bt_media_manager.h"
#include "app_bt_func.h"
#include "app_bt.h"
#include "app_a2dp.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_ibrt_keyboard.h"
#include "factory_section.h"
#include "apps.h"
#include "nvrecord_env.h"
#include "hal_bootmode.h"
#include "audio_policy.h"
#include "ddbif.h"
#include "bt_sys_cfg.h"
#include "audio_policy.h"
#include "factory_section.h"

#if defined(BT_HID_DEVICE)
#include "app_bt_hid.h"
#endif

#ifdef __GMA_VOICE__
#include "gma_crypto.h"
#endif
#include "app_battery.h"
#include "app_hfp.h"
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
#include "app_ai_if.h"
#include "app_ai_tws.h"
#endif
#ifdef IBRT_UI_V2
#include "ibrt_mgr_evt.h"
#include "ibrt_mgr.h"
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_include.h"
#endif

#ifdef BES_OTA
#include "ota_control.h"
extern void ota_control_send_start_role_switch(void);
#endif

#ifdef BISTO_ENABLED
#include "app_ai_tws.h"
#endif

#if defined(IBRT)

#include "app_tws_besaud.h"
#include "app_custom_api.h"

typedef struct
{
    ibrt_config_t ibrt_config;
    nvrec_btdevicerecord rec_mobile;
    nvrec_btdevicerecord rec_peer;
    uint8_t reserved __attribute__((aligned(4)));
    uint32_t crc;
} ibrt_config_ram_bak_t;

ibrt_config_ram_bak_t REBOOT_CUSTOM_PARAM_LOC ibrt_config_ram_bak;
void app_ibrt_ui_start_perform_a2dp_switching(void);

#ifdef IBRT_UI_V2
void app_ibrt_if_register_custom_ui_callback(APP_IBRT_IF_LINK_STATUS_CHANGED_CALLBACK* cbs)
{
    ibrt_mgr_register_custom_ui_callback(cbs);
}

void app_ibrt_if_register_pairing_mode_callback(APP_IBRT_IF_PAIRING_MODE_CHANGED_CALLBACK* cbs)
{
    ibrt_mgr_register_pairing_mode_changed_callback(cbs);
}

void app_ibrt_if_register_case_event_complete_callback(APP_IBRT_IF_CASE_EVENT_COMPLETE_CALLBACK* cbs)
{
    ibrt_mgr_register_case_evt_complete_callback(cbs);
}

void app_ibrt_if_register_vender_handler_ind(APP_IBRT_IF_VENDER_EVENT_HANDLER_IND handler)
{
    ibrt_mgr_register_vender_event_update_ind(handler);
}

void app_ibrt_if_set_disc_dev_if_3rd_dev_request(const bt_bdaddr_t* remote_addr)
{
    ibrt_mgr_set_disc_dev_if_3rd_dev_request(remote_addr);
}

uint8_t app_ibrt_if_get_connected_remote_dev_count()
{
    return ibrt_mgr_get_connected_remote_dev_count();
}

void app_ibrt_if_event_entry(ibrt_mgr_evt_t event)
{
    ibrt_mgr_event_entry(event);
}

void app_ibrt_if_disconnet_moblie_device(const bt_bdaddr_t* addr)
{
    //uint8_t max_support_link_count= app_ibrt_conn_support_max_mobile_dev();
    //for(uint8_t i=0;i< max_support_link_count;i++)��
    ibrt_mgr_send_mobile_disconnect_event(addr);
}


void app_ibrt_if_reconnect_moblie_device(const bt_bdaddr_t* addr)
{
    ibrt_mgr_send_mobile_reconnect_event_by_addr(addr);
}


#endif

static void app_ibrt_pre_pairing_disconnecting_check_timer_handler(void const *param);
osTimerDef(app_ibrt_pre_pairing_disconnecting_check_timer,
    app_ibrt_pre_pairing_disconnecting_check_timer_handler);
static osTimerId app_ibrt_pre_pairing_disconnecting_check_timer_id = NULL;
static uint32_t app_ibrt_pre_pairing_disconnecting_check_counter = 0;

#define APP_IBRT_PRE_PAIRING_DISCONNECTING_STATUS_CHECK_PERIOD_MS   100
#define APP_IBRT_PRE_PAIRING_DISCONNECTING_TIMEOUT_MS               10000
#define APP_IBRT_PRE_PAIRING_DISCONNECTING_TIMEMOUT_CHECK_COUNT    \
    ((APP_IBRT_PRE_PAIRING_DISCONNECTING_TIMEOUT_MS)/(APP_IBRT_PRE_PAIRING_DISCONNECTING_STATUS_CHECK_PERIOD_MS))

static void app_ibrt_pre_pairing_disconnecting_check_timer_handler(void const *param)
{
    if (app_ibrt_pre_pairing_disconnecting_check_counter >=
        APP_IBRT_PRE_PAIRING_DISCONNECTING_TIMEMOUT_CHECK_COUNT)
    {
        osTimerStop(app_ibrt_pre_pairing_disconnecting_check_timer_id);
        TRACE(0, "Pre pairing disconnecting timeout!");
        // TODO: put the registered callback API here
        return;
    }

    if (app_bt_is_any_connection())
    {
        app_ibrt_pre_pairing_disconnecting_check_counter++;
    }
    else
    {
        osTimerStop(app_ibrt_pre_pairing_disconnecting_check_timer_id);
        #ifdef IBRT_UI_V2
        app_ibrt_if_event_entry(IBRT_MGR_EV_TWS_PAIRING);
        #endif
    }
}

static void app_ibrt_if_start_pre_pairing_disconnecting(void)
{
    if (NULL == app_ibrt_pre_pairing_disconnecting_check_timer_id)
    {
        app_ibrt_pre_pairing_disconnecting_check_timer_id =
            osTimerCreate(osTimer(app_ibrt_pre_pairing_disconnecting_check_timer),
                osTimerPeriodic, NULL);
    }

    app_ibrt_pre_pairing_disconnecting_check_counter = 0;

    osTimerStart(app_ibrt_pre_pairing_disconnecting_check_timer_id,
        APP_IBRT_PRE_PAIRING_DISCONNECTING_STATUS_CHECK_PERIOD_MS);

    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)app_disconnect_all_bt_connections);
}

static void app_ibrt_if_tws_free_pairing_entry(ibrt_role_e role, uint8_t* pMasterAddr, uint8_t *pSlaveAddr)
{
#ifdef IBRT_RIGHT_MASTER
    app_tws_ibrt_reconfig_role(role, pMasterAddr, pSlaveAddr, true);
#else
    app_tws_ibrt_reconfig_role(role, pMasterAddr, pSlaveAddr, false);
#endif

    app_tws_ibrt_use_the_same_bd_addr();
    if (app_tws_ibrt_is_connected_with_wrong_peer())
    {
        app_ibrt_if_start_pre_pairing_disconnecting();
    }
    else
    {
        #ifdef IBRT_UI_V2
        app_ibrt_if_event_entry(IBRT_MGR_EV_TWS_PAIRING);
        #endif
    }
}

void app_ibrt_if_start_tws_pairing(ibrt_role_e role, uint8_t* peerAddr)
{
    TRACE(0, "start tws pairing as role %d, peer addr:", role);
    DUMP8("%02x ", peerAddr, BT_ADDR_OUTPUT_PRINT_NUM);

    bt_bdaddr_t local_addr;
    factory_section_original_btaddr_get(local_addr.address);
    if (IBRT_MASTER == role)
    {
        app_ibrt_if_tws_free_pairing_entry(role, local_addr.address,
            peerAddr);
    }
    else if (IBRT_SLAVE == role)
    {
        app_ibrt_if_tws_free_pairing_entry(role, peerAddr,
            local_addr.address);
    }
    else
    {
        TRACE(0, "API %s doesn't accept unknown tws role", __FUNCTION__);
    }
}

void app_ibrt_if_update_tws_pairing_info(ibrt_role_e role, uint8_t* peerAddr)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    memset((uint8_t *)&(nvrecord_env->ibrt_mode), 0xFF, sizeof(nvrecord_env->ibrt_mode));
    nv_record_env_set(nvrecord_env);

    bt_bdaddr_t local_addr;
    factory_section_original_btaddr_get(local_addr.address);
    bool isRightMasterSidePolicy = true;
#ifdef IBRT_RIGHT_MASTER
    isRightMasterSidePolicy = true;
#else
    isRightMasterSidePolicy = false;
#endif

    if (IBRT_MASTER == role)
    {
        app_tws_ibrt_reconfig_role(role, local_addr.address,
            peerAddr, isRightMasterSidePolicy);
    }
    else if (IBRT_SLAVE == role)
    {
        app_tws_ibrt_reconfig_role(role, peerAddr,
            local_addr.address, isRightMasterSidePolicy);
    }
    else
    {
        TRACE(0, "API %s doesn't accept unknown tws role", __FUNCTION__);
    }

    nv_record_flash_flush();

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_CUSTOM_OP2_AFTER_REBOOT);
}
#ifdef IBRT_UI_V2
void app_ibrt_if_config(ibrt_mgr_config_t *ui_config)
{

    ibrt_mgr_reconfig_env(ui_config);

}
#endif
void app_ibrt_if_nvrecord_config_load(void *config)
{
    app_ibrt_nvrecord_config_load(config);
}

void app_ibrt_if_nvrecord_update_ibrt_mode_tws(bool status)
{
    app_ibrt_nvrecord_update_ibrt_mode_tws(status);
}

int app_ibrt_if_nvrecord_get_latest_mobiles_addr(bt_bdaddr_t *mobile_addr1, bt_bdaddr_t* mobile_addr2)
{
    return app_ibrt_nvrecord_get_latest_mobiles_addr(mobile_addr1,mobile_addr2);
}

int app_ibrt_if_config_keeper_clear(void)
{
    memset(&ibrt_config_ram_bak, 0, sizeof(ibrt_config_ram_bak));
    return 0;
}

int app_ibrt_if_config_keeper_flush(void)
{
    if(__export_fn_rom.crc32 != NULL)
    {
        ibrt_config_ram_bak.crc = __export_fn_rom.crc32(0,(uint8_t *)(&ibrt_config_ram_bak),(sizeof(ibrt_config_ram_bak_t)-sizeof(uint32_t)));
        TRACE(2,"%s crc:%08x", __func__, ibrt_config_ram_bak.crc);
    }
    return 0;
}

int app_ibrt_if_volume_ptr_update_v2(bt_bdaddr_t *addr)
{
    if (addr)
    {
        app_bt_stream_volume_ptr_update((uint8_t *)addr);
    }

    return 0;
}

int app_ibrt_if_config_keeper_mobile_update(bt_bdaddr_t *addr)
{
    nvrec_btdevicerecord *nv_record = NULL;
    nvrec_btdevicerecord *rambak_record = NULL;

    rambak_record = &ibrt_config_ram_bak.rec_mobile;

    if (!nv_record_btdevicerecord_find(addr,&nv_record))
    {
        TRACE(1,"%s success", __func__);
        DUMP8("%02x ", nv_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        DUMP8("%02x ", nv_record->record.linkKey, sizeof(nv_record->record.linkKey));
        ibrt_config_ram_bak.ibrt_config.mobile_addr = *addr;
        *rambak_record = *nv_record;
        app_ibrt_if_config_keeper_flush();
        app_ibrt_if_volume_ptr_update_v2(addr);
    }
    else
    {
        TRACE(1,"%s failure", __func__);
    }
#ifdef __GMA_VOICE__
    if(app_tws_ibrt_compare_btaddr())
    {
        gma_secret_key_send();
    }
#endif
    return 0;
}

#ifdef __GMA_VOICE__
void app_ibrt_gma_exchange_ble_key()
{
    if(app_tws_ibrt_compare_btaddr())
    {
        gma_secret_key_send();
    }
}
#endif

int app_ibrt_if_config_keeper_tws_update(bt_bdaddr_t *addr)
{
    nvrec_btdevicerecord *nv_record = NULL;
    nvrec_btdevicerecord *rambak_record = NULL;

    rambak_record = &ibrt_config_ram_bak.rec_peer;

    if (!nv_record_btdevicerecord_find(addr,&nv_record))
    {
        TRACE(1,"%s success", __func__);
        DUMP8("%02x ", nv_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        DUMP8("%02x ", nv_record->record.linkKey, sizeof(nv_record->record.linkKey));
        ibrt_config_ram_bak.ibrt_config.peer_addr = *addr;
        *rambak_record = *nv_record;
        app_ibrt_if_config_keeper_flush();
        app_ibrt_if_volume_ptr_update_v2(addr);
    }
    else
    {
        TRACE(1,"%s failure", __func__);
    }

    return 0;
}

/*
 *tws switch stable timer, beacuse tws switch complete event is async
 *so need wait a stable time to let both device syncable
*/
static osTimerId  ibrt_ui_tws_switch_prepare_timer_id = NULL;
static void app_ibrt_ui_tws_switch_prepare_timer_cb(void const *n)
{
    app_ibrt_conn_notify_prepare_complete();
}

osTimerDef (IBRT_UI_TWS_SWITCH_PREPARE_TIMER, app_ibrt_ui_tws_switch_prepare_timer_cb);

static void app_ibrt_ui_start_tws_switch_prepare_supervising(uint32_t timeoutMs)
{
    if (NULL == ibrt_ui_tws_switch_prepare_timer_id)
    {
        ibrt_ui_tws_switch_prepare_timer_id =
            osTimerCreate(osTimer(IBRT_UI_TWS_SWITCH_PREPARE_TIMER), \
                          osTimerOnce, NULL);
    }

    osTimerStart(ibrt_ui_tws_switch_prepare_timer_id, timeoutMs);
}

bool app_ibrt_if_get_bes_ota_state(void)
{
#if BES_OTA
    return app_get_bes_ota_state();
#else
    return false;
#endif
}

void app_ibrt_if_ble_role_switch_start(void)
{
#ifdef __IAG_BLE_INCLUDE__
    ble_callback_event_t event;
    event.evt_type = BLE_CALLBACK_RS_START;
    app_ble_core_global_callback_event(&event, NULL);
#endif
}

void app_ibrt_if_ai_role_switch_prepare(uint32_t *wait_ms)
{
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    p_ibrt_ctrl->ibrt_ai_role_switch_handle = app_ai_tws_role_switch_prepare(wait_ms);
    if (p_ibrt_ctrl->ibrt_ai_role_switch_handle)
    {
        p_ibrt_ctrl->ibrt_role_switch_handle_user |= IBRT_ROLE_SWITCH_USER_AI;
    }
#endif
}

/*
* tws preparation before tws switch if needed
*/
bool app_ibrt_if_tws_switch_prepare_needed(uint32_t *wait_ms)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->ibrt_role_switch_handle_user)
    {
        // this means another mobile link sm had reached here and
        // the tws role switch preparation is already on-going.
        // so we can just let the very mobile link wait for role switch
        // preparation complete event to be informed to both mobile links
        *wait_ms = 800;
        return true;
    }

    bool ret = false;

    app_ibrt_if_ble_role_switch_start();
    app_ibrt_if_ai_role_switch_prepare(wait_ms);

    if (app_ibrt_if_get_bes_ota_state())
    {
        p_ibrt_ctrl->ibrt_role_switch_handle_user |= IBRT_ROLE_SWITCH_USER_OTA;
        *wait_ms = 800;
    }

    if (p_ibrt_ctrl->ibrt_role_switch_handle_user)
    {
        ret = true;
    }

    app_ibrt_middleware_role_switch_started_handler();

    TRACE(2,"tws_switch_prepare_needed %d wait_ms %d handle 0x%x 0x%x", ret, *wait_ms,
                                                p_ibrt_ctrl->ibrt_ai_role_switch_handle,
                                                p_ibrt_ctrl->ibrt_role_switch_handle_user);
    return ret;
}

void app_ibrt_if_ai_role_switch_handle(void)
{
#if defined(BISTO_ENABLED)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (p_ibrt_ctrl->ibrt_ai_role_switch_handle & (1 << AI_SPEC_GSOUND))
    {
        app_ai_tws_role_switch();
    }
#endif
}

void app_ibrt_if_ota_role_switch_handle(void)
{
#ifdef BES_OTA
    ibrt_ctrl_t *ota_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if ((app_ibrt_if_get_bes_ota_state()) &&\
        (ota_ibrt_ctrl->ibrt_role_switch_handle_user & IBRT_ROLE_SWITCH_USER_OTA))
    {
        app_set_ota_role_switch_initiator(true);
        bes_ota_send_role_switch_req();
    }
#endif
}

/*
* tws preparation before tws switch
*/
void app_ibrt_if_tws_swtich_prepare(uint32_t timeoutMs)
{
    app_ibrt_ui_start_tws_switch_prepare_supervising(timeoutMs);
    app_ibrt_if_ai_role_switch_handle();
    app_ibrt_if_ota_role_switch_handle();
}

/*
* notify UI SM tws preparation done
*/
static void app_ibrt_if_tws_switch_prepare_done(IBRT_ROLE_SWITCH_USER_E user, uint32_t role)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE(4,"tws_switch_prepare_done switch_handle 0x%x", \
            p_ibrt_ctrl->ibrt_role_switch_handle_user);

    if (p_ibrt_ctrl->ibrt_role_switch_handle_user)
    {
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
        TRACE(4,"ai_handle 0x%x role %d%s", \
              p_ibrt_ctrl->ibrt_ai_role_switch_handle, \
              role, \
              ai_spec_type2str((AI_SPEC_TYPE_E)role));

        if (user == IBRT_ROLE_SWITCH_USER_AI)
        {
            if (role >= AI_SPEC_COUNT)
            {
                TRACE(1,"%s role error", __func__);
                return;
            }
            if (!p_ibrt_ctrl->ibrt_ai_role_switch_handle)
            {
                TRACE(1,"%s ai_handle is 0", __func__);
                return;
            }

            p_ibrt_ctrl->ibrt_ai_role_switch_handle &= ~(1 << role);
            if (!p_ibrt_ctrl->ibrt_ai_role_switch_handle)
            {
                p_ibrt_ctrl->ibrt_role_switch_handle_user &= ~IBRT_ROLE_SWITCH_USER_AI;
            }
        }
        else
#endif
        {
            p_ibrt_ctrl->ibrt_role_switch_handle_user &= ~user;
        }

        if (!p_ibrt_ctrl->ibrt_role_switch_handle_user)
        {
            osTimerStop(ibrt_ui_tws_switch_prepare_timer_id);
            app_ibrt_conn_notify_prepare_complete();
        }
    }
}

void app_ibrt_if_tws_switch_prepare_done_in_bt_thread(IBRT_ROLE_SWITCH_USER_E user, uint32_t role)
{
    app_bt_start_custom_function_in_bt_thread(user,
            role,
            (uint32_t)app_ibrt_if_tws_switch_prepare_done);
}

int app_ibrt_if_config_keeper_resume(ibrt_config_t *config)
{
    uint32_t crc;
    nvrec_btdevicerecord *nv_record = NULL;
    nvrec_btdevicerecord *rambak_record = NULL;
    bool mobile_check_ok = false;
    bool peer_check_ok = false;
    bool flash_need_flush = false;
    bt_bdaddr_t zeroAddr = {0,0,0,0,0,0};

    crc = __export_fn_rom.crc32(0,(uint8_t *)(&ibrt_config_ram_bak),(sizeof(ibrt_config_ram_bak_t)-sizeof(uint32_t)));

    TRACE(3,"%s start crc:%08x/%08x", __func__, ibrt_config_ram_bak.crc, crc);
    if (crc == ibrt_config_ram_bak.crc)
    {
        TRACE_IMM(1,"%s success", __func__);
        TRACE(1,"%s config loc", __func__);
        DUMP8("%02x ", config->local_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        TRACE(1,"%s config mobile", __func__);
        DUMP8("%02x ", config->mobile_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        TRACE(1,"%s config peer", __func__);
        DUMP8("%02x ", config->peer_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);

        rambak_record = &ibrt_config_ram_bak.rec_mobile;
        if (!nv_record_btdevicerecord_find(&config->mobile_addr,&nv_record))
        {
            TRACE(1,"%s  find mobile", __func__);
            DUMP8("%02x ", nv_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
            DUMP8("%02x ", nv_record->record.linkKey, sizeof(nv_record->record.linkKey));
            if (!memcmp(rambak_record->record.linkKey, nv_record->record.linkKey, sizeof(nv_record->record.linkKey)))
            {
                TRACE(1,"%s  check mobile success", __func__);
                mobile_check_ok = true;
            }
        }
        if (!mobile_check_ok)
        {
            TRACE(1,"%s  check mobile failure", __func__);
            DUMP8("%02x ", rambak_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
            DUMP8("%02x ", rambak_record->record.linkKey, sizeof(rambak_record->record.linkKey));
            if (memcmp(rambak_record->record.bdAddr.address, zeroAddr.address, sizeof(zeroAddr)))
            {
                nv_record_add(section_usrdata_ddbrecord, rambak_record);
                config->mobile_addr = rambak_record->record.bdAddr;
                flash_need_flush = true;
                TRACE(1,"%s resume mobile", __func__);
            }
        }

        rambak_record = &ibrt_config_ram_bak.rec_peer;
        if (!nv_record_btdevicerecord_find(&config->peer_addr,&nv_record))
        {
            TRACE(1,"%s  find tws peer", __func__);
            DUMP8("%02x ", nv_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
            DUMP8("%02x ", nv_record->record.linkKey, sizeof(nv_record->record.linkKey));
            if (!memcmp(rambak_record->record.linkKey, nv_record->record.linkKey, sizeof(nv_record->record.linkKey)))
            {
                TRACE(1,"%s  check tws peer success", __func__);
                peer_check_ok = true;
            }
        }
        if (!peer_check_ok)
        {
            TRACE(1,"%s  check tws peer failure", __func__);
            DUMP8("%02x ", rambak_record->record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
            DUMP8("%02x ", rambak_record->record.linkKey, sizeof(rambak_record->record.linkKey));
            if (memcmp(rambak_record->record.bdAddr.address, zeroAddr.address, sizeof(zeroAddr)))
            {
                nv_record_add(section_usrdata_ddbrecord, rambak_record);
                config->peer_addr = rambak_record->record.bdAddr;
                flash_need_flush = true;
                TRACE(1,"%s resume tws peer", __func__);
            }
        }

    }

    ibrt_config_ram_bak.ibrt_config = *config;
    rambak_record = &ibrt_config_ram_bak.rec_mobile;
    if (!nv_record_btdevicerecord_find(&config->mobile_addr,&nv_record))
    {
        *rambak_record = *nv_record;
    }
    else
    {
        memset(rambak_record, 0, sizeof(nvrec_btdevicerecord));
    }
    rambak_record = &ibrt_config_ram_bak.rec_peer;
    if (!nv_record_btdevicerecord_find(&config->peer_addr,&nv_record))
    {
        *rambak_record = *nv_record;
    }
    else
    {
        memset(rambak_record, 0, sizeof(nvrec_btdevicerecord));
    }
    app_ibrt_if_config_keeper_flush();
    if (flash_need_flush)
    {
        nv_record_flash_flush();
    }
    TRACE_IMM(2,"%s end crc:%08x", __func__, ibrt_config_ram_bak.crc);

    TRACE(1,"%s mobile", __func__);
    DUMP8("%02x ", ibrt_config_ram_bak.rec_mobile.record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
    DUMP8("%02x ", ibrt_config_ram_bak.rec_mobile.record.linkKey, sizeof(ibrt_config_ram_bak.rec_mobile.record.linkKey));
    TRACE(1,"%s peer", __func__);
    DUMP8("%02x ", ibrt_config_ram_bak.rec_peer.record.bdAddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
    DUMP8("%02x ", ibrt_config_ram_bak.rec_peer.record.linkKey, sizeof(ibrt_config_ram_bak.rec_peer.record.linkKey));
    return 0;
}

void app_ibrt_if_set_access_mode(ibrt_if_access_mode_enum mode)
{
    app_bt_ME_SetAccessibleMode((btif_accessible_mode_t)mode, NULL);
}

static bool g_ibrt_if_bluetooth_is_enabling = false;

void app_ibrt_if_stack_is_ready(void)
{
    TRACE(0, "%s", __func__);
    if (g_ibrt_if_bluetooth_is_enabling)
    {
        app_custom_ui_notify_bluetooth_enabled();
        g_ibrt_if_bluetooth_is_enabling = false;
    }
}

void app_ibrt_if_enable_bluetooth(void)
{
    g_ibrt_if_bluetooth_is_enabling = false;

    if (app_is_stack_ready())
    {
        app_custom_ui_notify_bluetooth_enabled();
    }
    else
    {
        TRACE(0, "%s waiting", __func__);
        g_ibrt_if_bluetooth_is_enabling = true;
    }
}

static bool g_ibrt_if_bluetooth_is_disabling = false;

void app_ibrt_if_case_is_closed_complete(void)
{
    /* case is closed */
}

void app_ibrt_if_link_disconnected(void)
{
    if (g_ibrt_if_bluetooth_is_disabling)
    {
        if (!app_bt_manager.tws_conn.acl_is_connected && !app_bt_count_connected_device())
        {
            app_custom_ui_notify_bluetooth_disabled();
            g_ibrt_if_bluetooth_is_disabling = false;
        }
    }
}

void app_ibrt_if_disable_bluetooth(void)
{
    g_ibrt_if_bluetooth_is_disabling = false;

    if (app_bt_count_connected_device() || app_tws_ibrt_tws_link_connected())
    {
        app_bt_start_custom_function_in_bt_thread(
            (uint32_t)NULL, (uint32_t)NULL, (uint32_t)(uintptr_t)app_disconnect_all_bt_connections);
        g_ibrt_if_bluetooth_is_disabling = true;
    }
    else
    {
        app_custom_ui_notify_bluetooth_disabled();
    }
}

void app_ibrt_if_set_extended_inquiry_response(ibrt_if_extended_inquiry_response_t *data)
{
    static ibrt_if_extended_inquiry_response_t tmpEIR;
    tmpEIR = *data;
    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)tmpEIR.eir, sizeof(tmpEIR.eir), (uint32_t)(uintptr_t)btif_set_extended_inquiry_response);
}

POSSIBLY_UNUSED ibrt_if_pnp_info* app_ibrt_if_get_pnp_info(bt_bdaddr_t *remote)
{
    return (ibrt_if_pnp_info*)btif_dip_get_device_info(remote);
}

/*
* only used for factory.
* this function is only for freeman pairing,no tws link or ibrt link should be connected
* when mobile link or tws link exist, this function will disconnect mobile link and tws link
*/
void app_ibrt_if_enter_freeman_pairing(void)
{
    TRACE(1,"%s",__func__);
#ifdef IBRT_UI_V2
#if !defined(__XSPACE_UI__)
    uint8_t *currentBtAddr = factory_section_get_bt_address();

    //send exit pair mode and close box to disable accessible mode before update bt addr.
    ibrt_mgr_update_scan_type_policy(IBRT_EXIT_PAIR_MODE_EVT);
    ibrt_mgr_update_scan_type_policy(IBRT_CLOSE_BOX_EVT);
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    memcpy(p_ibrt_ctrl->local_addr.address, currentBtAddr, BD_ADDR_LEN);

    app_tws_update_local_bt_addr(currentBtAddr);
#endif

    app_ibrt_if_init_open_box_state_for_evb();
    app_ibrt_if_event_entry(IBRT_MGR_EV_FREE_MAN_MODE);

#endif
}

app_ibrt_if_ctrl_t *app_ibrt_if_get_bt_ctrl_ctx(void)
{
    return (app_ibrt_if_ctrl_t *)app_tws_ibrt_get_bt_ctrl_ctx();
}


void app_ibrt_if_write_bt_local_address(uint8_t* btAddr)
{
    uint8_t* currentBtAddr = factory_section_get_bt_address();
    if (memcmp(currentBtAddr, btAddr, BD_ADDR_LEN))
    {
        nv_record_rebuild();
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        memcpy(p_ibrt_ctrl->local_addr.address, btAddr, BD_ADDR_LEN);
#if defined(__XSPACE_UI__)
        factory_section_set_bt_addr(btAddr);
#else
        factory_section_set_bt_address(btAddr);
#endif

        app_tws_update_local_bt_addr(btAddr);

        nv_record_flash_flush();
    }
}

uint8_t *app_ibrt_if_get_bt_local_address(void)
{
    uint8_t* localBtAddr = factory_section_get_bt_address();
    return localBtAddr;
}

uint8_t *app_ibrt_if_get_bt_peer_address(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    return nvrecord_env->ibrt_mode.record.bdAddr.address;
}

/*
* this function is only for tws ibrt pairing mode
* when tws earphone is in the nearby, tws link will be connected firstly
* when mobile link exist, this function will disconnect mobile link
*/
void app_ibrt_if_enter_pairing_after_tws_connected(void)
{
    #ifdef IBRT_UI_V2
    app_ibrt_if_event_entry(IBRT_MGR_EV_TWS_PAIRING);
    #endif
}

extern void app_ibrt_mgr_pairing_mode_test(void);
void app_ibrt_if_enter_pairing_after_power_on(void)
{
    app_ibrt_mgr_pairing_mode_test();
}

void app_ibrt_if_ctx_checker(void)
{
    #ifdef IBRT_UI_V2
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
    ibrt_box_state  box_state = ibrt_mgr_get_latest_box_state();

    TRACE(3,"checker: nv_role:%d current_role:%s access_mode:%d",
          p_ibrt_ctrl->nv_role,
          app_bt_get_device_current_roles(),
          p_ibrt_ctrl->access_mode);

    DUMP8("%02x ", p_ibrt_ctrl->local_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);
    DUMP8("%02x ", p_ibrt_ctrl->peer_addr.address, BT_ADDR_OUTPUT_PRINT_NUM);

    TRACE(1,"checker: box_state:%s",ibrt_mgr_box_state_to_string(box_state));

    if (p_ibrt_ctrl->tws_mode == IBRT_ACTIVE_MODE)
    {
        bt_drv_reg_op_bt_info_checker();
    }
    #endif
}

void app_ibrt_if_init_open_box_state_for_evb(void)
{
#ifdef IBRT_UI_V2
    ibrt_mgr_init_open_box_state_for_evb();
#endif
}

int app_ibrt_if_force_audio_retrigger(uint8_t retriggerType)
{
#if defined(__AUDIO_RETRIGGER_REPORT__)
    app_ibrt_if_report_audio_retrigger((uint8_t)retriggerType);
#endif

    app_tws_ibrt_audio_retrigger();
    return 0;
}

void app_ibrt_if_audio_mute(void)
{

}

void app_ibrt_if_audio_recover(void)
{

}

bool app_ibrt_if_ota_is_in_progress(void)
{
#ifdef BES_OTA
    return ota_is_in_progress();
#else
    return false;
#endif
}

bool app_ibrt_if_start_ibrt_onporcess(const bt_bdaddr_t *addr)
{
    return !app_ibrt_conn_is_ibrt_idle(addr);
}

void app_ibrt_if_get_tws_conn_state_test(void)
{
    if(app_tws_ibrt_tws_link_connected())
    {
        TRACE(0,"ibrt_ui_log:TWS CONNECTED");
    }
    else
    {
        TRACE(0,"ibrt_ui_log:TWS DISCONNECTED");
    }
}

bt_status_t app_tws_if_ibrt_write_link_policy(const bt_bdaddr_t *p_addr, btif_link_policy_t policy)
{
    return app_tws_ibrt_write_link_policy(p_addr, policy);
}

struct app_ibrt_profile_req
{
    bool error_status;
    bt_bdaddr_t remote;
    app_ibrt_profile_id_enum profile_id;
    uint32_t extra_data;
};

static void app_ibrt_if_connect_profile_handler(bool is_ibrt_slave_receive_request, uint8_t *p_buff, uint16_t length)
{
    struct app_ibrt_profile_req *profile_req = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T *curr_device = NULL;

    profile_req = (struct app_ibrt_profile_req *)p_buff;
    device_id = app_bt_get_device_id_byaddr(&profile_req->remote);

    TRACE(4, "%s d%x profile %x status %d", __func__, device_id, profile_req->profile_id, profile_req->error_status);

    curr_device = app_bt_get_device(device_id);

    if (curr_device == NULL || !curr_device->acl_is_connected)
    {
        profile_req->error_status = true;
        return;
    }

    if (app_bt_manager.config.keep_only_one_stream_close_connected_a2dp && profile_req->profile_id == APP_IBRT_A2DP_PROFILE_ID)
    {
        if (is_ibrt_slave_receive_request)
        {
            uint8_t another_streaming_device = app_bt_audio_select_another_streaming_a2dp(device_id);
            if (another_streaming_device != BT_DEVICE_INVALID_ID)
            {
                struct BT_DEVICE_T *another_device = app_bt_get_device(another_streaming_device);
                if (BTIF_AVRCP_MEDIA_PLAYING == another_device->avrcp_palyback_status && app_ibrt_conn_is_profile_exchanged(&another_device->remote))
                {
                    TRACE(3, "%s d%x skip due to another device d%x streaming", __func__, device_id, another_streaming_device);
                    profile_req->error_status = true;
                    curr_device->this_is_closed_bg_a2dp = true;
                    curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
                    return;
                }
            }
        }
        else
        {
            if (profile_req->error_status)
            {
                TRACE(2, "%s d%x skip a2dp connect", __func__, device_id);
                curr_device->this_is_closed_bg_a2dp = true;
                curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
                return;
            }
        }
    }

    if (profile_req->error_status)
    {
        TRACE(2, "%s d%x skip due to remote error_status", __func__, device_id);
        return;
    }

    btif_me_reset_l2cap_sigid(&curr_device->remote);

    switch (profile_req->profile_id)
    {
        case APP_IBRT_HFP_PROFILE_ID:
            app_bt_reconnect_hfp_profile(&curr_device->remote);
            break;
        case APP_IBRT_A2DP_PROFILE_ID:
            app_bt_reconnect_a2dp_profile(&curr_device->remote);
            curr_device->a2dp_disc_on_process = 0;
            curr_device->this_is_closed_bg_a2dp = false;
            break;
        case APP_IBRT_AVRCP_PROFILE_ID:
            app_bt_reconnect_avrcp_profile(&curr_device->remote);
            break;
        case APP_IBRT_HID_PROFILE_ID:
#if defined(BT_HID_DEVICE)
            app_bt_hid_profile_connect(device_id, profile_req->extra_data ? true : false);
#endif
            break;
        default:
            break;
    }
}

static void app_ibrt_if_disconnect_profile_handler(uint8_t *p_buff, uint16_t length)
{
    struct app_ibrt_profile_req *profile_req = NULL;
    uint8_t device_id = BT_DEVICE_INVALID_ID;
    struct BT_DEVICE_T *curr_device = NULL;
    profile_req = (struct app_ibrt_profile_req *)p_buff;
    device_id = app_bt_get_device_id_byaddr(&profile_req->remote);
    TRACE(3, "%s d%x profile %x", __func__, device_id, profile_req->profile_id);
    if (device_id == BT_DEVICE_INVALID_ID)
    {
        return;
    }
    curr_device = app_bt_get_device(device_id);
    switch (profile_req->profile_id)
    {
        case APP_IBRT_HFP_PROFILE_ID:
            app_bt_disconnect_hfp_profile(curr_device->hf_channel);
            break;
        case APP_IBRT_A2DP_PROFILE_ID:
            app_bt_disconnect_a2dp_profile(curr_device->a2dp_connected_stream);
            curr_device->ibrt_disc_a2dp_profile_only = true;
            curr_device->a2dp_disc_on_process = 1;
            if (!curr_device->this_is_closed_bg_a2dp)
            {
                curr_device->this_is_closed_bg_a2dp = true;
                curr_device->close_a2dp_resume_prio = app_bt_audio_create_new_prio();
            }
            break;
        case APP_IBRT_AVRCP_PROFILE_ID:
            app_bt_disconnect_avrcp_profile(curr_device->avrcp_channel);
            break;
        case APP_IBRT_HID_PROFILE_ID:
#if defined(BT_HID_DEVICE)
            app_bt_hid_profile_disconnect(curr_device->hid_channel);
#endif
            break;
        default:
            break;
    }
}

struct app_ibrt_rfcomm_req
{
    uint8_t reason;
    uint32_t sppdev_ptr;
};

static void app_ibrt_if_disconnect_rfcomm_handler(uint8_t *p_buff)
{
    struct app_ibrt_rfcomm_req *rfcomm_req = NULL;
    rfcomm_req = (struct app_ibrt_rfcomm_req *)p_buff;
    app_bt_call_func_in_bt_thread((uint32_t)rfcomm_req->sppdev_ptr, (uint32_t)rfcomm_req->reason, 0, 0,\
                                                  (uint32_t) btif_spp_disconnect);
}

void app_ibrt_if_profile_connect(uint8_t device_id, int profile_id, uint32_t extra_data)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct app_ibrt_profile_req profile_req = {false, {0}};

    if (curr_device == NULL || !curr_device->acl_is_connected)
    {
        TRACE(3, "%s d%x profile %d curr_device NULL", __func__, device_id, profile_id);
        return;
    }

    if (profile_id == APP_IBRT_A2DP_PROFILE_ID)
    {
        curr_device->ibrt_disc_a2dp_profile_only = false;
        curr_device->a2dp_disc_on_process = 0;
        curr_device->this_is_closed_bg_a2dp = false;
    }

    profile_req.remote = curr_device->remote;
    profile_req.profile_id = (app_ibrt_profile_id_enum)profile_id;
    profile_req.extra_data = extra_data;

    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", device_id, __func__, ibrt_role);

        if (IBRT_MASTER == ibrt_role)
        {
            if (app_ibrt_conn_is_profile_exchanged(&curr_device->remote))
            {
                tws_ctrl_send_cmd(APP_TWS_CMD_CONN_PROFILE_REQ, (uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
            }
            else
            {
                app_ibrt_if_connect_profile_handler(false, (uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
            }
        }
        else if (IBRT_SLAVE == ibrt_role)
        {
            app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_TELL_MASTER_CONN_PROFILE, profile_req.profile_id, profile_req.extra_data);
        }
    }
    else
    {
        app_ibrt_if_connect_profile_handler(false, (uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
    }
}

void app_ibrt_if_profile_disconnect(uint8_t device_id, int profile_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    struct app_ibrt_profile_req profile_req = {false, {0}};

    if (curr_device == NULL)
    {
        TRACE(3, "%s d%x profile %d curr_device NULL", __func__, device_id, profile_id);
        return;
    }

    profile_req.remote = curr_device->remote;
    profile_req.profile_id = (app_ibrt_profile_id_enum)profile_id;

    if (profile_id == APP_IBRT_A2DP_PROFILE_ID)
    {
        curr_device->ibrt_disc_a2dp_profile_only = true;
    }

    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", device_id, __func__, ibrt_role);

        if (IBRT_MASTER == ibrt_role)
        {
            if (app_ibrt_conn_is_profile_exchanged(&curr_device->remote))
            {
                tws_ctrl_send_cmd(APP_TWS_CMD_DISC_PROFILE_REQ, (uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
            }
            else
            {
                app_ibrt_if_disconnect_profile_handler((uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
            }
        }
        else if (IBRT_SLAVE == ibrt_role)
        {
            app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_TELL_MASTER_DISC_PROFILE, profile_req.profile_id, 0);
        }
    }
    else
    {
        app_ibrt_if_disconnect_profile_handler((uint8_t*)&profile_req, sizeof(struct app_ibrt_profile_req));
    }
}

void app_ibrt_if_master_connect_hfp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_connect(device_id, APP_IBRT_HFP_PROFILE_ID, 0);
}

void app_ibrt_if_master_connect_a2dp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_connect(device_id, APP_IBRT_A2DP_PROFILE_ID, 0);
}

void app_ibrt_if_master_connect_avrcp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_connect(device_id, APP_IBRT_AVRCP_PROFILE_ID, 0);
}

void app_ibrt_if_master_disconnect_hfp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_HFP_PROFILE_ID);
}

void app_ibrt_if_master_disconnect_a2dp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_A2DP_PROFILE_ID);
}

void app_ibrt_if_master_disconnect_avrcp_profile(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (tws_besaud_is_connected() && app_tws_get_ibrt_role(&curr_device->remote) != IBRT_MASTER)
    {
        TRACE(2, "(d%x) %s not master", device_id, __func__);
        return;
    }
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_AVRCP_PROFILE_ID);
}

void app_ibrt_if_connect_hfp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_connect(device_id, APP_IBRT_HFP_PROFILE_ID, 0);
}

void app_ibrt_if_connect_a2dp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_connect(device_id, APP_IBRT_A2DP_PROFILE_ID, 0);
}

void app_ibrt_if_connect_avrcp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_connect(device_id, APP_IBRT_AVRCP_PROFILE_ID, 0);
}

void app_ibrt_if_disconnect_hfp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_HFP_PROFILE_ID);
}

void app_ibrt_if_disconnect_a2dp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_A2DP_PROFILE_ID);
}

void app_ibrt_if_disconnect_avrcp_profile(uint8_t device_id)
{
    app_ibrt_if_profile_disconnect(device_id, APP_IBRT_AVRCP_PROFILE_ID);
}

void app_ibrt_if_register_ibrt_cbs()
{
    static app_ibrt_if_cbs_t app_ibrt_if_cbs  =
    {
        .keyboard_request_handler = app_ibrt_keyboard_request_handler_v2,
        .ui_perform_user_action = app_ibrt_ui_perform_user_action_v2,
        .conn_profile_handler = app_ibrt_if_connect_profile_handler,
        .disc_profile_handler = app_ibrt_if_disconnect_profile_handler,
        .disc_rfcomm_handler = app_ibrt_if_disconnect_rfcomm_handler,
        .ibrt_if_sniff_prevent_need = app_ibrt_if_customer_prevent_sniff,
        .tws_switch_prepare_needed = app_ibrt_if_tws_switch_prepare_needed,
        .tws_swtich_prepare = app_ibrt_if_tws_swtich_prepare,
    };

    app_ibrt_conn_reg_ibrt_if_cb(&app_ibrt_if_cbs);
}

bool app_ibrt_if_is_any_mobile_connected(void)
{
    return app_ibrt_conn_any_mobile_connected();
}

bool app_ibrt_if_is_maximum_mobile_dev_connected(void)
{
    return app_ibrt_conn_support_max_mobile_dev();
}

#ifdef IBRT_UI_V2
void app_ibrt_if_dump_ibrt_mgr_info()
{
    ibrt_mgr_dump_info();
}
#endif

btif_connection_role_t app_ibrt_if_get_tws_current_bt_role(void)
{
    btif_connection_role_t tws_bt_role = BTIF_BCR_UNKNOWN;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if(p_ibrt_ctrl->p_tws_remote_dev != NULL)
    {
        tws_bt_role = btif_me_get_current_role(p_ibrt_ctrl->p_tws_remote_dev);
    }
    return tws_bt_role;
}


/* API for Customer*/
AppIbrtStatus app_ibrt_if_get_a2dp_state(bt_bdaddr_t *addr, AppIbrtA2dpState *a2dp_state)
{
    uint8_t device_id = btif_me_get_device_id_from_addr(addr);
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
    if(0xff != device_id){
        if(a2dp_state == NULL)
        {
            return APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS;
        }
        if(app_tws_ibrt_mobile_link_connected(addr) || app_tws_ibrt_slave_ibrt_link_connected(addr))
        {
            if(btif_a2dp_get_stream_state(curr_device->btif_a2dp_stream->a2dp_stream) > BTIF_AVDTP_STRM_STATE_IDLE)
            {
                *a2dp_state = (AppIbrtA2dpState)btif_a2dp_get_stream_state(curr_device->btif_a2dp_stream->a2dp_stream);
            }
            else
            {
                *a2dp_state = APP_IBRT_IF_A2DP_IDLE;
            }
        }
        else
        {
            *a2dp_state = APP_IBRT_IF_A2DP_IDLE;
        }
    }
    else{
        *a2dp_state = APP_IBRT_IF_A2DP_IDLE;
    }
    return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_avrcp_state(bt_bdaddr_t *addr,AppIbrtAvrcpState *avrcp_state)
{
    if(NULL == avrcp_state)
    {
        return APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS;
    }
    uint8_t device_id = btif_me_get_device_id_from_addr(addr);
    if(0xff != device_id)
    {
        struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
        if(BTIF_AVRCP_STATE_DISCONNECTED != curr_device->avrcp_conn_flag)
        {
            if(avrcp_state != NULL)
            {
                if(curr_device->avrcp_palyback_status == BTIF_AVRCP_MEDIA_PLAYING)
                {
                    *avrcp_state = APP_IBRT_IF_AVRCP_PLAYING;
                }
                else if(curr_device->avrcp_palyback_status == BTIF_AVRCP_MEDIA_PAUSED)
                {
                    *avrcp_state = APP_IBRT_IF_AVRCP_PAUSED;
                }
                else
                {
                    *avrcp_state = APP_IBRT_IF_AVRCP_CONNECTED;
                }
            }
        }
        else
        {
            *avrcp_state = APP_IBRT_IF_AVRCP_DISCONNECTED;
        }
    }
    else
    {
        *avrcp_state = APP_IBRT_IF_AVRCP_DISCONNECTED;
    }
    return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_hfp_state(bt_bdaddr_t *addr, AppIbrtHfpState *hfp_state)
{
    uint8_t device_id = btif_me_get_device_id_from_addr(addr);
    if(NULL== hfp_state)
    {
        return APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS;
    }
    if(0xff != device_id)
    {
        struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
        if(btif_get_hf_chan_state(curr_device->hf_channel) == BTIF_HF_STATE_OPEN)
        {
            *hfp_state = APP_IBRT_IF_HFP_SLC_OPEN;
        }
        else
        {
            *hfp_state = APP_IBRT_IF_HFP_SLC_DISCONNECTED;
        }
    }
    else
    {
        *hfp_state = APP_IBRT_IF_HFP_SLC_DISCONNECTED;
    }
    return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_hfp_call_status(bt_bdaddr_t *addr, AppIbrtCallStatus *call_status)
{
    if(call_status == NULL)
    {
        return APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS;
    }

    uint8_t device_id = btif_me_get_device_id_from_addr(addr);
    if(0xff != device_id)
    {
        struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);
        if (curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN)
        {
            *call_status = APP_IBRT_IF_SETUP_INCOMMING;
        }
        else if(curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_OUT)
        {
            *call_status = APP_IBRT_IF_SETUP_OUTGOING;
        }
        else if(curr_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_ALERT)
        {
            *call_status = APP_IBRT_IF_SETUP_ALERT;
        }
        else if((curr_device->hfchan_call == BTIF_HF_CALL_ACTIVE) \
                && (curr_device->hf_audio_state == BTIF_HF_AUDIO_CON))
        {
            *call_status = APP_IBRT_IF_CALL_ACTIVE;
        }
        else if(curr_device->hf_callheld == BTIF_HF_CALL_HELD_ACTIVE)
        {
            *call_status = APP_IBRT_IF_HOLD;
        }
        else
        {
            *call_status = APP_IBRT_IF_NO_CALL;
        }
    }
    else
    {
        *call_status = APP_IBRT_IF_NO_CALL;
    }

    return APP_IBRT_IF_STATUS_SUCCESS;
}

uint8_t app_ibrt_if_get_mobile_connected_dev_list(bt_bdaddr_t *addr_list)
{
    return app_ibrt_conn_get_connected_mobile_list(addr_list);
}

bool app_ibrt_if_is_tws_role_switch_on(void)
{
    return app_ibrt_conn_any_role_switch_running();
}

void app_ibrt_if_tws_role_switch_request(void)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)0, (uint32_t)0,
        (uint32_t)app_ibrt_conn_all_dev_start_tws_role_switch);
}

bool app_ibrt_if_nvrecord_get_mobile_addr(bt_bdaddr_t mobile_addr_list[],uint8_t *count)
{
    bt_status_t result = BT_STS_SUCCESS;

    result = app_ibrt_nvrecord_get_mobile_addr(mobile_addr_list,count);
    if(result == BT_STS_FAILED)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void app_ibrt_if_nvrecord_delete_all_mobile_record(void)
{
    app_ibrt_nvrecord_delete_all_mobile_record();
}

bool app_ibrt_if_nvrecord_get_mobile_paired_dev_list(nvrec_btdevicerecord *nv_record,uint8_t *count)
{
    bt_status_t result = BT_STS_SUCCESS;

    result = app_ibrt_nvrecord_get_mobile_paired_dev_list(nv_record,count);
    if(result == BT_STS_FAILED)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool app_ibrt_if_is_tws_link_connected(void)
{
    return app_ibrt_conn_is_tws_connected();
}
#ifdef IBRT_UI_V2
uint8_t app_ibrt_if_get_connected_mobile_count(void)
{
    return app_ibrt_conn_get_local_connected_mobile_count();
}
#endif
uint8_t app_ibrt_if_is_in_freeman_mode(void)
{
    return app_ibrt_conn_is_freeman_mode();
}

ibrt_status_t app_ibrt_if_tws_disconnect_request(void)
{
    return app_ibrt_conn_tws_disconnect();
}

ibrt_status_t app_ibrt_if_mobile_disconnect_request(const bt_bdaddr_t *addr,ibrt_post_func post_func)
{
    return app_ibrt_conn_remote_dev_disconnect_request(addr, post_func);
}

bool app_ibrt_if_is_tws_in_pairing_state(void)
{
    return app_ibrt_conn_is_tws_in_pairing_state();
}

uint8_t app_ibrt_if_get_support_max_remote_link()
{
    return app_ibrt_conn_support_max_mobile_dev();
}

void app_ibrt_if_a2dp_send_play(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_PLAY, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_pause(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_PAUSE, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_forward(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_FORWARD, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_backward(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_BACKWARD, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_volume_up(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_AVRCP_VOLUP, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_volume_down(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_AVRCP_VOLDN, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_a2dp_send_set_abs_volume(uint8_t device_id, uint8_t volume)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_AVRCP_ABS_VOL, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_hf_create_audio_link(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_HFSCO_CREATE, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_hf_disc_audio_link(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_HFSCO_DISC, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_hf_call_redial(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_REDIAL, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_hf_call_answer(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_ANSWER, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_hf_call_hangup(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t) device_id, (uint32_t) IBRT_ACTION_HANGUP, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_set_local_volume_up(void)
{
    app_bt_call_func_in_bt_thread((uint32_t) BT_DEVICE_ID_1, (uint32_t) IBRT_ACTION_LOCAL_VOLUP, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

void app_ibrt_if_set_local_volume_down(void)
{
    app_bt_call_func_in_bt_thread((uint32_t) BT_DEVICE_ID_1, (uint32_t) IBRT_ACTION_LOCAL_VOLDN, 0, 0,\
        (uint32_t) app_ibrt_if_start_user_action_v2);
}

static void app_ibrt_start_switching_sco(uint32_t playing_sco)
{
    app_ibrt_if_start_user_action_v2(playing_sco, IBRT_ACTION_SWITCH_SCO, 0, 0);
}

void app_ibrt_if_switch_streaming_sco(void)
{
    uint8_t playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t sco_count = app_bt_audio_count_connected_sco();
    uint8_t another_call_active_device = BT_DEVICE_INVALID_ID;

    if (playing_sco == BT_DEVICE_INVALID_ID)
    {
        playing_sco = app_bt_audio_select_call_active_hfp();
    }

    if (playing_sco != BT_DEVICE_INVALID_ID)
    {
        another_call_active_device = app_bt_audio_select_another_device_to_create_sco(playing_sco);
    }

    TRACE(4, "%s playing sco d%x sco count %d active call d%x", __func__, playing_sco, sco_count, another_call_active_device);

    if (playing_sco != BT_DEVICE_INVALID_ID && (sco_count > 1 || another_call_active_device != BT_DEVICE_INVALID_ID))
    {
        app_bt_start_custom_function_in_bt_thread(playing_sco, (uint32_t)NULL, (uint32_t)(uintptr_t)app_ibrt_start_switching_sco);
    }
}

void app_ibrt_if_switch_streaming_a2dp(void)
{
    uint8_t playing_sco = app_bt_audio_get_curr_playing_sco();
    uint8_t sco_count = app_bt_audio_count_connected_sco();
    uint8_t a2dp_count = app_bt_audio_count_streaming_a2dp();

    TRACE(4, "%s %d %d %d", __func__, playing_sco, sco_count, a2dp_count);

    if (playing_sco == BT_DEVICE_INVALID_ID && sco_count == 0 && a2dp_count > 1)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)NULL, (uint32_t)NULL, (uint32_t)(uintptr_t)app_ibrt_ui_start_perform_a2dp_switching);
    }
}

#ifdef TOTA_v2
char *tota_get_strbuf(void);
void app_ibrt_if_tota_printf(const char * format, ...)
{
    char *strbuf = tota_get_strbuf();
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strbuf, format, vlist);
    va_end(vlist);
    app_ibrt_if_start_user_action_v2(BT_DEVICE_ID_1, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)strbuf, strlen(strbuf));
}
void app_ibrt_if_tota_printf_by_device(uint8_t device_id, const char * format, ...)
{
    char *strbuf = tota_get_strbuf();
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strbuf, format, vlist);
    va_end(vlist);
    app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_SEND_TOTA_DATA, (uint32_t)(uintptr_t)strbuf, strlen(strbuf));
}
#endif

#ifdef IBRT_UI_V2
const char* app_ibrt_if_ui_event_to_string(ibrt_mgr_evt_t type)
{
    return ibrt_mgr_event_to_string(type);
}

bool app_ibrt_if_event_has_been_queued(const bt_bdaddr_t* remote_addr,ibrt_mgr_evt_t event)
{
    return ibrt_mgr_event_has_been_queued(remote_addr,event);
}

ibrt_mgr_evt_t app_ibrt_if_get_remote_dev_active_event(const bt_bdaddr_t* remote_addr)
{
    return ibrt_mgr_get_active_event(remote_addr);
}

void app_ibrt_if_exit_pairing_mode()
{
    ibrt_mgr_start_exit_pairing_mode();
}
#endif

#ifdef PRODUCTION_LINE_PROJECT_ENABLED
void app_ibrt_if_test_open_box(void)
{
    ibrt_mgr_get_config()->reconnect_tws_max_times = 0xFFFF;
    ibrt_mgr_get_config()->open_reconnect_tws_max_times = 0xFFFF;

    app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
}

void app_ibrt_if_test_close_box(void)
{
    ibrt_mgr_get_config()->reconnect_tws_max_times = 0;
    ibrt_mgr_get_config()->open_reconnect_tws_max_times = 0;
    app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
}
#endif

bool app_ibrt_if_post_custom_reboot_handler(void)
{
    if (hal_sw_bootmode_get()&HAL_SW_BOOTMODE_CUSTOM_OP1_AFTER_REBOOT)
    {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CUSTOM_OP1_AFTER_REBOOT);
        app_ibrt_if_enter_freeman_pairing();
        return false;
    }
    else if (hal_sw_bootmode_get()&HAL_SW_BOOTMODE_CUSTOM_OP2_AFTER_REBOOT)
    {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CUSTOM_OP2_AFTER_REBOOT);
        #ifdef IBRT_UI_V2
        app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
        #endif
        return false;
    }
    else if(hal_sw_bootmode_get()&HAL_SW_BOOTMODE_TEST_NORMAL_MODE)
    {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_NORMAL_MODE);
        app_ibrt_reconect_mobile_after_factorty_test();
        return false;
    }
    else
    {
        return true;
    }
}

ibrt_status_t app_ibrt_if_get_local_name(uint8_t* name_buf, uint8_t max_size)
{
   return app_ibrt_conn_get_local_name(name_buf, max_size);
}

bool app_ibrt_if_get_all_paired_bt_link_key(ibrt_if_paired_bt_link_key_info *linkKey)
{
    if (linkKey == NULL) {
        TRACE(1, "%s null pointer,pls check", __func__);
        return false;
    }

    bt_status_t retStatus = BT_STS_SUCCESS;
    btif_device_record_t record;
    int32_t index= 0;

    memset(linkKey, 0x00, sizeof(ibrt_if_paired_bt_link_key_info));
    linkKey->pairedDevNum = nv_record_get_paired_dev_count();

    for (index = 0; index < linkKey->pairedDevNum; index++)
    {
        retStatus = nv_record_enum_dev_records(index, &record);
        if (BT_STS_SUCCESS == retStatus)
        {
            memcpy(&linkKey->linkKey[index].btAddr, record.bdAddr.address, BTIF_BD_ADDR_SIZE);
            memcpy(&linkKey->linkKey[index].linkKey, record.linkKey , 16);
        }
    }
    return true;
}

bool app_ibrt_if_is_audio_active(uint8_t device_id)
{
    struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    if (device_id > BT_DEVICE_NUM) {
        return false;
    }

    if(!curr_device) {
        return false;
    }

    return (curr_device->a2dp_streamming || \
            btapp_hfp_is_dev_sco_connected(device_id));
}

bool app_ibrt_if_get_active_device(bt_bdaddr_t* device)
{
    bool addr_valid = false;
    struct BT_DEVICE_T* curr_device = NULL;

    if(!device)
    {
        return false;
    }

    for(uint8_t index = 0;index < BT_DEVICE_NUM;index++)
    {
        if(app_ibrt_if_is_audio_active(index))
        {
            curr_device = app_bt_get_device(index);
            if(!curr_device)
            {
                return false;
            }

            memcpy((uint8_t*)&device->address[0],(uint8_t*)&curr_device->remote.address[0],BD_ADDR_LEN);
            TRACE(1,"custom_ui:d%x is service active ",index);
            addr_valid = true;
        }
    }

    if (addr_valid == false)
    {
        uint8_t device_id = app_bt_audio_select_connected_device();
        if (device_id != BT_DEVICE_INVALID_ID)
        {
            TRACE(1,"custom_ui:select d%x as service active ",device_id);
            *device = app_bt_get_device(device_id)->remote;
            addr_valid = true;
        }
    }

    return addr_valid;
}

void app_ibrt_if_get_mobile_bt_link_key(uint8_t *linkKey1, uint8_t *linkKey2)
{
    btif_device_record_t record;
    struct BT_DEVICE_T *curr_device = NULL;
    bt_bdaddr_t *mobile_addr = NULL;

    memset(linkKey1, 0x00, 16);
    memset(linkKey2, 0x00, 16);

    curr_device = app_bt_get_device(BT_DEVICE_ID_1);
    mobile_addr = &curr_device->remote;

    if ((mobile_addr != NULL) && ddbif_find_record(mobile_addr, &record) == BT_STS_SUCCESS) {
        memcpy(linkKey1, record.linkKey, 16);
    }

    curr_device = app_bt_get_device(BT_DEVICE_ID_2);
    mobile_addr = &curr_device->remote;

    if ((mobile_addr != NULL) && ddbif_find_record(mobile_addr, &record) == BT_STS_SUCCESS) {
        memcpy(linkKey2, record.linkKey, 16);
    }
}

void app_ibrt_if_get_tws_bt_link_key(uint8_t *linkKey)
{
    btif_device_record_t record;
    bt_bdaddr_t *tws_addr = NULL;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    memset(linkKey, 0x00, 16);

    tws_addr = btif_me_get_remote_device_bdaddr(p_ibrt_ctrl->p_tws_remote_dev);

    if ((tws_addr != NULL) && ddbif_find_record(tws_addr, &record) == BT_STS_SUCCESS) {
        memcpy(linkKey, record.linkKey, 16);
    }
}

static void app_ibrt_if_update_mobile_link_qos_handler(uint8_t device_id, uint8_t tpoll_slot)
{
    struct BT_DEVICE_T* pBtdev = app_bt_get_device(device_id);
    bt_status_t ret = BT_STS_SUCCESS;

    if (pBtdev && (pBtdev->acl_is_connected) && (app_ibrt_conn_get_ui_role() != TWS_UI_SLAVE))
    {
        ret = btif_me_qos_setup_with_tpoll_generic(pBtdev->acl_conn_hdl, tpoll_slot, QOS_SETUP_SERVICE_TYPE_BEST_EFFORT);
        TRACE(2, "btif_me_qos_setup_with_tpoll_generic:handle:%04x %d", pBtdev->acl_conn_hdl, ret);
    }
}

void app_ibrt_if_update_mobile_link_qos(uint8_t device_id, uint8_t tpoll_slot)
{
    app_bt_call_func_in_bt_thread((uint32_t)device_id, (uint32_t)tpoll_slot,
        0, 0, (uint32_t) app_ibrt_if_update_mobile_link_qos_handler);
}

bool app_ibrt_if_is_tws_addr(const uint8_t* pBdAddr)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (!memcmp(pBdAddr, p_ibrt_ctrl->local_addr.address, BTIF_BD_ADDR_SIZE) ||
        !memcmp(pBdAddr, p_ibrt_ctrl->peer_addr.address, BTIF_BD_ADDR_SIZE))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static osMutexId setConnectivityLogMutexId = NULL;
osMutexDef(setConnectivityLog);
static reportConnectivityLogCallback_t ibrt_if_report_connectivity_log_callback = NULL;
static reportDisconnectReasonCallback_t ibrt_if_report_disconnect_reason_callback = NULL;
static connectivity_log_t Connectivity_log;
static disconnect_reason_t disconnect_reason;
static uint32_t bt_clkoffset = 0;
connectivity_log_report_intersys_api ibrt_if_report_intersys_callback = NULL;

void app_ibrt_if_report_connectivity_log_init(void)
{
    if (!setConnectivityLogMutexId){
        setConnectivityLogMutexId = osMutexCreate((osMutex(setConnectivityLog)));
    }

    ibrt_if_report_intersys_callback = app_ibrt_if_acl_data_packet_check_handler;

    memset(&Connectivity_log, 0, sizeof(connectivity_log_t));
    Connectivity_log.retriggerType = RETRIGGER_MAX;
    TRACE(2,"%s, %d", __func__, sizeof(connectivity_log_t));
}

void app_ibrt_if_register_report_connectivity_log_callback(reportConnectivityLogCallback_t callback)
{
    ibrt_if_report_connectivity_log_callback = callback;
}

void app_ibrt_if_register_report_disonnect_reason_callback(reportDisconnectReasonCallback_t callback)
{
    ibrt_if_report_disconnect_reason_callback = callback;
}

void app_ibrt_if_save_bt_clkoffset(uint32_t clkoffset, uint8_t device_id)
{
    if (BT_DEVICE_NUM > device_id)
    {
        bt_clkoffset = clkoffset;
    }
}

void app_ibrt_if_disconnect_event(btif_remote_device_t *rem_dev, bt_bdaddr_t *addr, uint8_t disconnectReason, uint8_t activeConnection)
{
    uint8_t device_id = btif_me_get_device_id_from_rdev(rem_dev);
    uint16_t conhandle = INVALID_HANDLE;

    conhandle = btif_me_get_remote_device_hci_handle(rem_dev);
    bt_drv_reg_op_get_pkt_ts_rssi(conhandle, (CLKN_SER_NUM_T *)disconnect_reason.rxClknRssi);
    disconnect_reason.lcState = bt_drv_reg_op_force_get_lc_state(conhandle);

    if (BT_DEVICE_NUM > device_id)
    {
        for (uint8_t i = 0; i < RECORD_RX_NUM; i++)
        {
            disconnect_reason.rxClknRssi[i].clkn = (disconnect_reason.rxClknRssi[i].clkn + bt_clkoffset) & 0xFFFFFFFF;
        }
        bt_clkoffset = 0;
    }

    disconnect_reason.disconnectObject = device_id;
    disconnect_reason.disconnectReson = disconnectReason;
    disconnect_reason.activeConnection = activeConnection;
    memcpy(disconnect_reason.addr, addr, 6);

    if (ibrt_if_report_disconnect_reason_callback)
    {
        ibrt_if_report_disconnect_reason_callback(&disconnect_reason);
    }

    TRACE(3, "%s, Device_id[d%x] Disconnect_reson : 0x%02x", __func__, disconnect_reason.disconnectObject, disconnect_reason.disconnectReson);
    TRACE(1, "REMOTE DEVICE ADDR IS: ");
    DUMP8("%02x ", addr, BT_ADDR_OUTPUT_PRINT_NUM);
}

void app_ibrt_if_save_curr_mode_to_disconnect_info(uint8_t currMode, uint32_t interval, bt_bdaddr_t *addr)
{
    uint8_t device_id = btif_me_get_device_id_from_addr(addr);

    if (BT_DEVICE_NUM > device_id)
    {
        disconnect_reason.mobileCurrMode[device_id] = currMode;
        disconnect_reason.mobileSniffInterval[device_id] = interval;
    }
    else if (BT_DEVICE_INVALID_ID == device_id)
    {
        disconnect_reason.twsCurrMode = currMode;
        disconnect_reason.twsSniffInterval = interval;
    }
}

static void app_ibrt_if_get_newest_rssi_and_chlMap(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (NULL == curr_device)
    {
        return;
    }

    ibrt_mobile_info_t *p_mobile_info = NULL;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    rx_agc_t link_agc_info = {0};
    //uint8_t chlMap[10] = {0};
    //char strChlMap[32];
    //int pos = 0;

    p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(&curr_device->remote);
    osMutexWait(setConnectivityLogMutexId, osWaitForever);

    if (p_mobile_info)
    {
        if (p_mobile_info->p_mobile_remote_dev)
        {
            if (bt_drv_reg_op_read_rssi_in_dbm(btif_me_get_remote_device_hci_handle(p_mobile_info->p_mobile_remote_dev),&link_agc_info))
            {
                Connectivity_log.mobile_rssi[device_id] = link_agc_info.rssi;
                TRACE(3, "[MOBILE] [d%x] RSSI = %d, RX gain = %d",
                  device_id,
                  link_agc_info.rssi,
                  link_agc_info.rxgain);
                  TRACE(1, "MOBILE ADDR IS: ");
                  DUMP8("%02x ", curr_device->remote.address, BT_ADDR_OUTPUT_PRINT_NUM);
            }

#ifdef __UPDATE_CHNMAP__
            if (0 == bt_drv_reg_op_acl_chnmap(btif_me_get_remote_device_hci_handle(p_mobile_info->p_mobile_remote_dev), chlMap, sizeof(chlMap)))
            {
                memcpy(&Connectivity_log.mobileChlMap[device_id], chlMap, 10);
                for (uint8_t i = 0; i < 10; i++)
                {
                    pos += snprintf(strChlMap + pos, sizeof(strChlMap) - pos, "%02x ", Connectivity_log.mobileChlMap[device_id][i]);
                }
                TRACE(2, "%s chlMap %s", "[MOBILE] chlMap", strChlMap);
            }
#endif
        }
    }

    if (p_ibrt_ctrl->p_tws_remote_dev)
    {
        if (bt_drv_reg_op_read_rssi_in_dbm(btif_me_get_remote_device_hci_handle(p_ibrt_ctrl->p_tws_remote_dev),&link_agc_info))
        {
            Connectivity_log.tws_rssi = link_agc_info.rssi;
            bt_bdaddr_t *remote = btif_me_get_remote_device_bdaddr(p_ibrt_ctrl->p_tws_remote_dev);
            if (remote)
            {
                TRACE(2, "[TWS] RSSI = %d, RX gain = %d",
                      Connectivity_log.tws_rssi,
                      link_agc_info.rxgain);
                TRACE(1, "PEER TWS ADDR IS: ");
                DUMP8("%02x ",remote->address, BT_ADDR_OUTPUT_PRINT_NUM);
            }
        }
    }

    osMutexRelease(setConnectivityLogMutexId);
}

void app_ibrt_if_update_rssi_info(const char* tag, rx_agc_t link_agc_info, uint8_t device_id)
{
    osMutexWait(setConnectivityLogMutexId, osWaitForever);

    if (!memcmp(tag, "PEER TWS", sizeof(*tag)))
    {
        Connectivity_log.tws_rssi = link_agc_info.rssi;
    }
    else if(!memcmp(tag, "MASTER MOBILE", sizeof(*tag)) || !memcmp(tag, "SNOOP MOBILE", sizeof(*tag)))
    {
        Connectivity_log.mobile_rssi[device_id] = link_agc_info.rssi;
    }

    osMutexRelease(setConnectivityLogMutexId);
}

void app_ibrt_if_update_chlMap_info(const char* tag, uint8_t *chlMap, uint8_t device_id)
{
#ifdef __UPDATED_CHNMAP__
    if(!memcmp(tag, "MASTER MOBILE", sizeof(*tag)) || !memcmp(tag, "SNOOP MOBILE", sizeof(*tag)))
    {
        osMutexWait(setConnectivityLogMutexId, osWaitForever);
        memcpy(&Connectivity_log.mobileChlMap[device_id], chlMap, 10);
        osMutexRelease(setConnectivityLogMutexId);
    }
#endif
}

static const char* app_ibrt_if_retrigger_type_to_str(uint8_t retrigger_type)
{
    switch(retrigger_type)
    {
        case RETRIGGER_BY_ROLE_SWITCH:
            return "RETRIGGER_BY_ROLE_SWITCH";
        case RETRIGGER_BY_DECODE_ERROR:
            return "RETRIGGER_BY_DECODE_ERROR";
        case RETRIGGER_BY_DECODE_STATUS_ERROR:
            return "RETRIGGER_BY_DECODE_STATUS_ERROR";
        case RETRIGGER_BY_ROLE_MISMATCH:
            return "RETRIGGER_BY_ROLE_MISMATCH";
        case RETRIGGER_BY_TRIGGER_FAIL:
            return "RETRIGGER_BY_TRIGGER_FAIL";
        case RETRIGGER_BY_L_R_SYNC_DETECT:
            return "RETRIGGER_BY_L_R_SYNC_DETECT";
        case RETRIGGER_BY_SYNCHRONIZE_CNT_LIMIT:
            return "RETRIGGER_BY_SYNCHRONIZE_CNT_LIMIT";
        case RETRIGGER_BY_LOW_BUFFER:
            return "RETRIGGER_BY_LOW_BUFFER";
        case RETRIGGER_BY_SEQ_MISMATCH:
            return "RETRIGGER_BY_SEQ_MISMATCH";
        case RETRIGGER_BY_AUTO_SYNC_NO_SUPPORT:
            return "RETRIGGER_BY_AUTO_SYNC_NO_SUPPORT";
        case RETRIGGER_BY_PLAYER_NOT_ACTIVE:
            return "RETRIGGER_BY_PLAYER_NOT_ACTIVE";
        case RETRIGGER_BY_STATUS_ERROR:
            return "RETRIGGER_BY_STATUS_ERROR";
        case RETRIGGER_BY_STREAM_RESTART:
            return "RETRIGGER_BY_STREAM_RESTART";
        case RETRIGGER_BY_SYNC_MISMATCH:
            return "RETRIGGER_BY_SYNC_MISMATCH";
        case RETRIGGER_BY_SYNC_FAIL:
            return "RETRIGGER_BY_SYNC_FAIL";
        case RETRIGGER_BY_SYS_BUSY:
            return "RETRIGGER_BY_SYS_BUSY";
        case RETRIGGER_BY_SYNC_TARGET_CNT_ERROR:
            return "RETRIGGER_BY_SYNC_TARGET_CNT_ERROR";
        case RETRIGGER_BY_AI_STREAM:
            return "RETRIGGER_BY_AI_STREAM";
        case RETRIGGER_BY_UNKNOW:
            return "RETRIGGER_BY_UNKNOW";
        case RETRIGGER_MAX:
            return "RETRIGGER_MAX";
        default:
            ASSERT(0,"UNKNOWN retrigger_type = 0x%x",retrigger_type);
            break;
    }
}

uint32_t app_ibrt_if_get_curr_ticks(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (NULL == curr_device)
    {
        return 0;
    }

    uint32_t curr_ticks = 0;
    uint16_t conhandle = INVALID_HANDLE;

    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        conhandle = APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote);
        curr_ticks = bt_syn_get_curr_ticks(conhandle);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        conhandle = APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote);
        curr_ticks = bt_syn_get_curr_ticks(conhandle);
    }

    return curr_ticks;
}

void app_ibrt_if_report_audio_retrigger(uint8_t retriggerType)
{
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (NULL == curr_device)
    {
        return;
    }

    uint32_t curr_ticks = app_ibrt_if_get_curr_ticks(device_id);
    app_ibrt_if_get_newest_rssi_and_chlMap(device_id);

    TRACE(2, "[%s] RETRIGGER TYPE[%s]",__func__,app_ibrt_if_retrigger_type_to_str(retriggerType));
    osMutexWait(setConnectivityLogMutexId, osWaitForever);
    if (btif_me_is_in_sniff_mode(&curr_device->remote))
    {
        Connectivity_log.retriggerType = RETRIGGER_BY_IN_SNIFF;
    }
    else
    {
        Connectivity_log.retriggerType = retriggerType;
    }
    Connectivity_log.clock = curr_ticks;
    TRACE(1, "Retrigger Curr ticke = 0x%08x", Connectivity_log.clock);

    osMutexRelease(setConnectivityLogMutexId);
}

void app_ibrt_if_update_link_monitor_info(uint8_t *ptr)
{
    uint32_t *p = (uint32_t *)ptr + HEC_ERR;
    uint8_t tmp[sizeof(Connectivity_log.ll_monitor_info)/sizeof(uint8_t) - 14];
    //uint8_t device_id = app_bt_audio_get_curr_a2dp_device();

    for (uint8_t i = 0; i < (sizeof(Connectivity_log.ll_monitor_info)/sizeof(uint8_t) - 14); i++)
    {
        tmp[i] = ((uint8_t *)p)[0];
        p += 1;
    }

    osMutexWait(setConnectivityLogMutexId, osWaitForever);

    Connectivity_log.ll_monitor_info.rev_fa_cnt = 0;
    memcpy(&Connectivity_log.ll_monitor_info, tmp, sizeof(tmp));

    p += 6;
    Connectivity_log.ll_monitor_info.rx_seq_err_cnt = ((uint8_t *)p)[0];

    p = (uint32_t *)ptr + 12;
    for (uint8_t i = 12; i <= 23; i++)
    {
        Connectivity_log.ll_monitor_info.rev_fa_cnt += ((uint8_t *)p)[0];
        if (i == RX_DM1)
        {
            Connectivity_log.ll_monitor_info.rx_dm1 = ((uint8_t *)p)[0];
        }
        else if (i == RX_2DH1)
        {
            Connectivity_log.ll_monitor_info.rx_2dh1 = ((uint8_t *)p)[0];
        }
        else if (i == RX_2DH3)
        {
            Connectivity_log.ll_monitor_info.rx_2dh3 = ((uint8_t *)p)[0];
        }
        else if (i == RX_2DH5)
        {
            Connectivity_log.ll_monitor_info.rx_2dh5 = ((uint8_t *)p)[0];
        }
        p++;
    }
    Connectivity_log.ll_monitor_info.last_ticks = Connectivity_log.ll_monitor_info.curr_ticks;
    Connectivity_log.ll_monitor_info.curr_ticks = app_ibrt_if_get_curr_ticks(app_bt_audio_get_curr_a2dp_device());

    if (RETRIGGER_MAX != Connectivity_log.retriggerType)
    {
        if (ibrt_if_report_connectivity_log_callback)
        {
            ibrt_if_report_connectivity_log_callback(&Connectivity_log);
        }
        Connectivity_log.retriggerType = RETRIGGER_MAX;
    }

    osMutexRelease(setConnectivityLogMutexId);

    //TRACE(1, "%s curr tick 0x%x", __func__, Connectivity_log.ll_monitor_info.curr_ticks);
    //DUMP8("0x%02x ", &Connectivity_log.ll_monitor_info, sizeof(Connectivity_log.ll_monitor_info)/sizeof(uint8_t));
}

static uint32_t lastAclPacketTimeMs = 0;
static uint32_t lasttwsAclPacketTimeMs = 0;

void app_ibrt_if_reset_acl_data_packet_check(void)
{
    osMutexWait(setConnectivityLogMutexId, osWaitForever);

    memset(&Connectivity_log.acl_packet_interval, 0, sizeof(Connectivity_log.acl_packet_interval));
    lastAclPacketTimeMs = 0;
    osMutexRelease(setConnectivityLogMutexId);
}

static void app_ibrt_if_fill_acl_data_packet_interval(uint32_t intervalMs, int8_t currRssi, uint8_t lastRssi)
{
    char strAclPacketInterval[50];
    int len = 0;
    int8_t index = 0;
    uint32_t currBtclk = app_ibrt_if_get_curr_ticks(app_bt_audio_get_curr_a2dp_device());

    osMutexWait(setConnectivityLogMutexId, osWaitForever);

    for (index = TOP_ACL_PACKET_INTERVAL_CNT - 1; index > 0; index--)
    {
        memmove((uint8_t *)&Connectivity_log.acl_packet_interval[index],
                (uint8_t *)&Connectivity_log.acl_packet_interval[index-1],
                sizeof(acl_packet_interval_t));
    }
    index = 0;
    Connectivity_log.acl_packet_interval[index].AclPacketInterval = intervalMs;
    Connectivity_log.acl_packet_interval[index].currentReceivedRssi = currRssi;
    Connectivity_log.acl_packet_interval[index].lastReceivedRssi = lastRssi;
    Connectivity_log.acl_packet_interval[index].AclPacketBtclock = currBtclk;

    osMutexRelease(setConnectivityLogMutexId);

    for (index = 0;index < TOP_ACL_PACKET_INTERVAL_CNT;index++)
    {
        len += snprintf(strAclPacketInterval + len, sizeof(strAclPacketInterval) - len, "%08x ",
                        Connectivity_log.acl_packet_interval[index].AclPacketInterval);
    }

    TRACE(1, "ACL DATA INTERVAL %s, curr ticks is %d curr btclk is 0x%x", strAclPacketInterval, lastAclPacketTimeMs, currBtclk);
}

void app_ibrt_if_check_acl_data_packet_during_a2dp_streaming(void)
{
    uint32_t passedTimerMs = 0;
    uint32_t currentTimerMs = GET_CURRENT_MS();

    if (0 == lastAclPacketTimeMs)
    {
        lastAclPacketTimeMs = currentTimerMs;
    }
    else
    {
        if (currentTimerMs > lastAclPacketTimeMs)
        {
            passedTimerMs = currentTimerMs - lastAclPacketTimeMs;
        }
        else
        {
            passedTimerMs = 0xFFFFFFFF - lastAclPacketTimeMs + currentTimerMs + 1;
        }

        lastAclPacketTimeMs = currentTimerMs;
    }

    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (NULL == curr_device)
    {
        return;
    }

    ibrt_mobile_info_t *p_mobile_info = NULL;
    rx_agc_t link_agc_info = {0};
    static int8_t currRssi = 0;
    static int8_t lastRssi = 0;
    p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(&curr_device->remote);

    if (p_mobile_info)
    {
        if (p_mobile_info->p_mobile_remote_dev)
        {
            if (bt_drv_reg_op_read_rssi_in_dbm(btif_me_get_remote_device_hci_handle(p_mobile_info->p_mobile_remote_dev),&link_agc_info))
            {
                lastRssi = currRssi;
                currRssi = link_agc_info.rssi;
            }
        }
    }

    if (passedTimerMs >= ACL_PACKET_INTERVAL_THRESHOLD_MS)
    {
        TRACE(1, "%s passedTimerMs = %d ", __func__, passedTimerMs);
        app_ibrt_if_fill_acl_data_packet_interval(passedTimerMs, currRssi, lastRssi);
    }
}

void app_ibrt_if_reset_tws_acl_data_packet_check(void)
{
    //When needed, use it
}

static void app_ibrt_if_fill_tws_acl_data_packet_interval(uint32_t intervalMs)
{
    //When needed, use it
}

void app_ibrt_if_check_tws_acl_data_packet(void)
{
    uint32_t passedTimerMs = 0;
    uint32_t currentTimerMs = GET_CURRENT_MS();

    if (0 == lasttwsAclPacketTimeMs)
    {
        lasttwsAclPacketTimeMs = currentTimerMs;
    }
    else
    {
        if (currentTimerMs > lasttwsAclPacketTimeMs)
        {
            passedTimerMs = currentTimerMs - lasttwsAclPacketTimeMs;
        }
        else
        {
            passedTimerMs = 0xFFFFFFFF - lasttwsAclPacketTimeMs + currentTimerMs + 1;
        }

        lasttwsAclPacketTimeMs = currentTimerMs;
    }

    if (passedTimerMs >= ACL_PACKET_INTERVAL_THRESHOLD_MS)
    {
        TRACE(1, "%s passedTimerMs = %d", __func__, passedTimerMs);
        app_ibrt_if_fill_tws_acl_data_packet_interval(passedTimerMs);
    }
}

bool app_ibrt_if_is_mobile_connhandle(uint16_t connhandle)
{
    struct BT_DEVICE_T *curr_device = NULL;
    ibrt_mobile_info_t *p_mobile_info = NULL;
    bt_bdaddr_t *mobile_addr = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);

        if (curr_device)
        {
            mobile_addr = &curr_device->remote;
            p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(mobile_addr);
            if (p_mobile_info)
            {
                if (p_mobile_info->mobile_conhandle == connhandle || p_mobile_info->ibrt_conhandle == connhandle)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
    }
    return false;
}

bool app_ibrt_if_is_tws_connhandle(uint16_t connhandle)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->tws_conhandle == connhandle)
    {
        return true;
    }

    return false;
}

void app_ibrt_if_acl_data_packet_check_handler(uint8_t *data)
{
    if ((0x02 == data[0]) && (0x80&data[1]))
    {
        if (a2dp_is_music_ongoing() && app_ibrt_if_is_mobile_connhandle(data[1]))
        {
            app_ibrt_if_check_acl_data_packet_during_a2dp_streaming();
        }
        else if(app_ibrt_if_is_tws_connhandle((uint16_t)data[1]))
        {
            app_ibrt_if_check_tws_acl_data_packet();
        }
    }
}

static uint8_t LatestNewlyPairedMobileBtAddr[BTIF_BD_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};

static void app_ibrt_if_new_mobile_paired_callback(const uint8_t* btAddr)
{
    if (app_ibrt_if_is_tws_addr(btAddr))
    {
        TRACE(0, "New device paired: skip, local address!");
    }
    else
    {
        TRACE(0, "New mobile paired:");
        DUMP8("%02x ", btAddr, BT_ADDR_OUTPUT_PRINT_NUM);
        memcpy(LatestNewlyPairedMobileBtAddr, btAddr, sizeof(LatestNewlyPairedMobileBtAddr));
    }
}

uint8_t* app_ibrt_if_get_latest_paired_mobile_bt_addr(void)
{
    return LatestNewlyPairedMobileBtAddr;
}

void app_ibrt_if_clear_newly_paired_mobile(void)
{
    memset(LatestNewlyPairedMobileBtAddr, 0, sizeof(LatestNewlyPairedMobileBtAddr));
}

void app_ibrt_if_init_newly_paired_mobile_callback(void)
{
    nv_record_bt_device_register_newly_paired_device_callback(
        app_ibrt_if_new_mobile_paired_callback);
}

uint32_t app_ibrt_if_get_tws_mtu_size(void)
{
    return tws_ctrl_get_mtu_size();
}

static BSIR_event_callback_t ibrt_if_bsir_command_event_callback = NULL;
void app_ibrt_if_register_BSIR_command_event_callback(BSIR_event_callback_t callback)
{
    ibrt_if_bsir_command_event_callback = callback;
}

void app_ibrt_if_BSIR_command_event(uint8_t is_in_band_ring)
{
    if (ibrt_if_bsir_command_event_callback)
    {
        ibrt_if_bsir_command_event_callback(is_in_band_ring);
    }
}

static sco_disconnect_event_callback_t ibrt_if_sco_disconnect_event_callback = NULL;
void app_ibrt_if_register_sco_disconnect_event_callback(sco_disconnect_event_callback_t callback)
{
    ibrt_if_sco_disconnect_event_callback = callback;
}

void app_ibrt_if_sco_disconnect(uint8_t *addr, uint8_t disconnect_rseason)
{
    if (ibrt_if_sco_disconnect_event_callback)
    {
        ibrt_if_sco_disconnect_event_callback(addr, disconnect_rseason);
    }
}

void app_ibrt_if_a2dp_set_delay(a2dp_stream_t *Stream, uint16_t delayMs)
{
    app_bt_call_func_in_bt_thread((uint32_t) Stream, (uint32_t) delayMs, 0, 0,\
            (uint32_t) btif_a2dp_set_sink_delay);
}

#ifdef IBRT_UI_V2
bool app_ibrt_if_is_earbud_in_pairing_mode(void)
{
    return ibrt_mgr_is_earbud_in_pairing_mode();
}
#endif

static void app_ibrt_if_disconnect_mobile_links_handler(void)
{
    struct BT_DEVICE_T *curr_device = NULL;
    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (curr_device && curr_device->acl_is_connected)
        {
            app_tws_ibrt_disconnect_connection(btif_me_get_remote_device_by_handle(curr_device->acl_conn_hdl));
        }
    }
}

void app_ibrt_if_disconnect_mobile_links(void)
{
    app_bt_call_func_in_bt_thread(0, 0, 0, 0,\
            (uint32_t) app_ibrt_if_disconnect_mobile_links_handler);
}

static void app_ibrt_if_disconnect_specific_mobile_handler(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = NULL;

    curr_device = app_bt_get_device(device_id);
    if (curr_device && curr_device->acl_is_connected)
    {
        app_tws_ibrt_disconnect_connection(btif_me_get_remote_device_by_handle(curr_device->acl_conn_hdl));
    }
}

void app_ibrt_if_disconnect_specific_mobile(uint8_t device_id)
{
    app_bt_call_func_in_bt_thread((uint32_t)device_id, 0, 0, 0,\
            (uint32_t) app_ibrt_if_disconnect_specific_mobile_handler);
}

void app_ibrt_if_enable_hfp_voice_assistant(bool isEnable)
{
    app_bt_call_func_in_bt_thread((uint32_t)isEnable, 0, 0, 0,\
            (uint32_t) app_hfp_siri_voice);
}

void app_ibrt_if_disonnect_rfcomm(struct spp_device *dev,uint8_t reason)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(dev->device_id);
    struct app_ibrt_rfcomm_req rfcomm_req = {0, 0};

    if (curr_device == NULL)
    {
        TRACE(3, "%s d%x curr_device NULL", __func__, dev->device_id);
        return;
    }

    rfcomm_req.reason = reason;
    rfcomm_req.sppdev_ptr = (uint32_t)dev;

    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", dev->device_id, __func__, ibrt_role);

        if (IBRT_MASTER == ibrt_role)
        {
            if (app_ibrt_conn_is_profile_exchanged(&curr_device->remote))
            {
                tws_ctrl_send_cmd(APP_TWS_CMD_DISC_RFCOMM_REQ, (uint8_t*)&rfcomm_req, sizeof(struct app_ibrt_rfcomm_req));
            }
            else
            {
                app_ibrt_if_disconnect_rfcomm_handler((uint8_t*)&rfcomm_req);
            }
        }
        else if (IBRT_SLAVE == ibrt_role)
        {
            app_ibrt_if_start_user_action_v2(dev->device_id, IBRT_ACTION_TELL_MASTER_DISC_RFCOMM, (uint32_t)dev, reason);
        }
    }
    else
    {
        app_ibrt_if_disconnect_rfcomm_handler((uint8_t*)&rfcomm_req);
    }
}


void app_ibrt_if_enter_non_signalingtest_mode(void)
{
    app_bt_call_func_in_bt_thread(0, 0, 0, 0, (uint32_t)app_enter_non_signalingtest_mode);
}

void app_ibrt_if_bt_stop_inqury(void)
{
    app_bt_call_func_in_bt_thread(0, 0, 0, 0, (uint32_t)app_bt_stop_inquiry);
}

void app_ibrt_if_bt_start_inqury(void)
{
    app_bt_call_func_in_bt_thread(0, 0, 0, 0, (uint32_t)app_bt_start_inquiry);
}


void app_ibrt_if_bt_set_local_dev_name(const uint8_t *dev_name, unsigned char len)
{
    static uint8_t local_bt_name[BTM_NAME_MAX_LEN];
    memcpy(local_bt_name, dev_name, len);
    app_bt_call_func_in_bt_thread((uint32_t)local_bt_name, (uint32_t)len, 0, 0, (uint32_t)bt_set_local_dev_name);
}

void app_ibrt_if_disconnect_all_bt_connections(void)
{
    app_bt_call_func_in_bt_thread(0, 0, 0, 0, (uint32_t)app_disconnect_all_bt_connections);
}

static void app_ibrt_if_spp_write_handler(struct spp_device *dev, char *buff, uint16_t length)
{
    uint16_t len = length;
    btif_spp_write(dev, buff, &len);
}

void app_ibrt_if_spp_write(struct spp_device *dev, char *buff, uint16_t length)
{
    app_bt_call_func_in_bt_thread((uint32_t)dev, (uint32_t)buff, (uint32_t)length, 0,
            (uint32_t)app_ibrt_if_spp_write_handler);
}

void app_ibrt_if_spp_open(struct spp_device *dev,btif_remote_device_t *rem_dev,spp_callback_t callback)
{
    app_bt_call_func_in_bt_thread((uint32_t)dev, (uint32_t)rem_dev, (uint32_t)callback, 0, (uint32_t)btif_spp_open);
}

void app_ibrt_if_write_page_timeout(uint16_t timeout)
{
    app_bt_call_func_in_bt_thread((uint16_t)timeout, 0, 0, 0, (uint32_t)btif_me_write_page_timeout);
}

void app_ibrt_if_conn_tws_connect_request(bool isInPairingMode, uint32_t timeout)
{
    app_bt_call_func_in_bt_thread((uint16_t)isInPairingMode, (uint32_t)timeout, 0, 0, (uint32_t)app_ibrt_conn_tws_connect_request);
}

void app_ibrt_if_conn_remote_dev_connect_request(const bt_bdaddr_t *addr,connection_direction_t direction,bool request_connect, uint32_t timeout)
{
    static bt_bdaddr_t remote_bt_addr;
    memcpy(remote_bt_addr.address, addr->address, sizeof(bt_bdaddr_t));
    app_bt_call_func_in_bt_thread((uint32_t)&remote_bt_addr, (uint32_t)direction, (uint32_t)request_connect, (uint32_t)timeout, (uint32_t)app_ibrt_conn_remote_dev_connect_request);
}

bool app_ibrt_if_is_peer_mobile_link_exist_but_local_not_on_tws_connected(void)
{
    return app_tws_ibrt_get_bt_ctrl_ctx()->isPeerMobileLinkExistButLocalNotOnTwsConnected;
}

void app_ibrt_if_clear_flag_peer_mobile_link_exist_but_local_not_on_tws_connected(void)
{
    app_tws_ibrt_get_bt_ctrl_ctx()->isPeerMobileLinkExistButLocalNotOnTwsConnected = false;
}

void app_ibrt_if_register_is_reject_new_mobile_connection_query_callback(reject_new_connection_callback_t callback)
{
    app_tws_ibrt_register_is_reject_new_mobile_connection_callback(callback);
}

void app_ibrt_if_write_page_scan_setting(uint16_t window_slots, uint16_t interval_slots)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)window_slots, (uint32_t)interval_slots,
        (uint32_t)app_ibrt_conn_pscan_setting);
}

void app_ibrt_if_register_is_reject_tws_connection_callback(reject_new_connection_callback_t callback)
{
    app_tws_ibrt_register_is_reject_tws_connection_callback(callback);
}

#ifdef CUSTOM_BITRATE
#include "app_ibrt_customif_cmd.h"
//#include "product_config.h"
ibrt_custome_codec_t a2dp_custome_config;
static bool user_a2dp_info_set=false;
extern void a2dp_avdtpcodec_aac_user_configure_set(uint32_t bitrate,uint8_t user_configure);
extern void a2dp_avdtpcodec_sbc_user_configure_set(uint32_t bitpool,uint8_t user_configure);
extern void app_audio_dynamic_update_dest_packet_mtu_set(uint8_t codec_index, uint8_t packet_mtu, uint8_t user_configure);

#define RECONNECT_MOBILE_INTERVAL      (5000)
static void app_ibrt_reconnect_mobile_timehandler(void const *param);
osTimerDef (IBRT_RECONNECT_MOBILE, app_ibrt_reconnect_mobile_timehandler);
static osTimerId  app_ibrt_mobile_timer = NULL;
typedef struct{
    bool used;
    bt_bdaddr_t mobile_addr;
}manual_reconnect_device_info_t;
static manual_reconnect_device_info_t mobile_info[BT_DEVICE_NUM];
static void app_ibrt_reconnect_mobile_timehandler(void const *param)
{
    for (int i = 0; i < app_ibrt_conn_support_max_mobile_dev(); ++i){
        if(mobile_info[i].used == true){
            mobile_info[i].used = false;
            app_ibrt_if_reconnect_moblie_device((const bt_bdaddr_t*)(&mobile_info[i].mobile_addr));
        }else{
            continue;
        }
    }
}

static void app_ibrt_if_mannual_disconnect_mobile_then_reconnect(void)
{
    for (int i = 0; i < app_ibrt_conn_support_max_mobile_dev(); ++i)
    {
        struct BT_DEVICE_T *curr_device = app_bt_get_device(i);
        if (curr_device->acl_is_connected)
        {
            app_ibrt_if_disconnet_moblie_device(&curr_device->remote);
            if(mobile_info[i].used == false){
                mobile_info[i].used = true;
                memcpy((uint8_t*)&(mobile_info[i].mobile_addr),(uint8_t*)(&curr_device->remote),sizeof(bt_bdaddr_t));
            }
            if (app_ibrt_mobile_timer == NULL)
            {
                app_ibrt_mobile_timer = osTimerCreate (osTimer(IBRT_RECONNECT_MOBILE), osTimerOnce,NULL);
            }
            if(app_ibrt_mobile_timer != NULL){
                osTimerStart(app_ibrt_mobile_timer,RECONNECT_MOBILE_INTERVAL);
            }
        }
        else{
            TRACE(0,"acl is not connect");
        }
    }

}

void app_ibrt_if_manual_disconnect_mobile_then_reconnect_action_reset(void)
{
    if(app_ibrt_mobile_timer != NULL){
        osTimerStop(app_ibrt_mobile_timer);
    }
    for (int i = 0; i < app_ibrt_conn_support_max_mobile_dev(); ++i){
        mobile_info[i].used = false;
        memset((uint8_t*)&(mobile_info[i].mobile_addr),0,sizeof(bt_bdaddr_t));
    }
}

void app_ibrt_if_set_codec_param(uint32_t aac_bitrate,uint32_t sbc_boitpool,uint32_t audio_latency)
{
    TRACE(4,"%s aac=%d sbc=%d latency=%d",__func__,aac_bitrate,sbc_boitpool,audio_latency);
    if(app_bt_ibrt_has_mobile_link_connected()&&(app_tws_ibrt_tws_link_connected())){
        a2dp_custome_config.aac_bitrate = aac_bitrate;
        a2dp_custome_config.sbc_bitpool = sbc_boitpool;
        a2dp_custome_config.audio_latentcy = audio_latency;
        uint32_t lock = nv_record_pre_write_operation();
        nv_record_get_extension_entry_ptr()->codec_user_info.aac_bitrate = a2dp_custome_config.aac_bitrate;
        nv_record_get_extension_entry_ptr()->codec_user_info.sbc_bitpool = a2dp_custome_config.sbc_bitpool;
        nv_record_get_extension_entry_ptr()->codec_user_info.audio_latentcy = a2dp_custome_config.audio_latentcy;
        nv_record_post_write_operation(lock);
        nv_record_flash_flush();
        user_a2dp_info_set=true;
        app_ibrt_user_a2dp_info_sync_tws_share_cmd_send((uint8_t *)&a2dp_custome_config, sizeof(ibrt_custome_codec_t));
        app_ibrt_if_mannual_disconnect_mobile_then_reconnect();
    }
}

void app_ibrt_user_a2dp_codec_info_action(void)
{
    TRACE(1,"%s %d",__func__,user_a2dp_info_set);
    if(user_a2dp_info_set){
        user_a2dp_info_set=false;
    }
    else{
        return;
    }
    if(app_ibrt_if_get_ui_role()==IBRT_MASTER)
    {
        a2dp_avdtpcodec_sbc_user_configure_set(a2dp_custome_config.sbc_bitpool, true);
        a2dp_avdtpcodec_aac_user_configure_set(a2dp_custome_config.aac_bitrate, true);
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_SBC, (a2dp_custome_config.audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/3, true);//sbc
        app_audio_dynamic_update_dest_packet_mtu_set(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC, (a2dp_custome_config.audio_latentcy-USER_CONFIG_AUDIO_LATENCY_ERROR)/23, true);//aac
    }        
}
#endif


#endif /*IBRT*/

