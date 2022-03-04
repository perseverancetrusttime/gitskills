#include "btapp.h"
#include "hfp_api.h"
#include "app_bt_stream.h"
#include "app_hfp.h"
#include "hal_bootmode.h"
#include "xspace_interface.h"
#include "xspace_interface_process.h"
#include "nvrecord_extension.h"
#include "hal_timer.h"
#include "xspace_ui.h"
#include "pmu.h"
#include "factory_section.h"
#include "a2dp_decoder.h"
#include "tgt_hardware.h"
#include "app_ibrt_if.h"
#include "app_tws_ibrt.h"
#include "app_bt.h"
#include "app_tws_ibrt_conn_api.h"
#include "os_api.h"
#include "nvrecord_bt.h"
#include "nvrecord_env.h"
#include "xspace_ui.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#if defined(__XSPACE_BATTERY_MANAGER__)
#include "xspace_battery_manager.h"
#endif

extern "C" uint8_t is_a2dp_mode(void);
extern "C" uint8_t is_sco_mode(void);
extern "C" uint8_t btapp_hfp_get_call_active(void);
extern int open_siri_flag;

extern "C" bool app_ibrt_conn_get_tws_use_same_addr_enable();
static shutdown_write_flash_manage_func shutdown_write_flash_manage_func_cb[BEFOR_SHUTDOWN_WRITE_FLASH_NUM] = {NULL};
static void xspace_interface_ibrt_nvrecord_config_set(ibrt_config_t *config);
static int xspace_interface_ibrt_if_reconfig(ibrt_config_t *config);

extern btif_accessible_mode_t g_bt_access_mode;

/**
 ****************************************************************************************
 * @brief static function prototype
 ****************************************************************************************
 */
static void xspace_interface_ibrt_nvrecord_config_set(ibrt_config_t *config)
{
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    nvrecord_env->ibrt_mode.mode = config->nv_role;
    nvrecord_env->ibrt_mode.record.bdAddr = config->peer_addr;

    XIF_TRACE(2, "nv role:%d, audio_chnl_sel:%d.", (uint32_t)config->nv_role, config->audio_chnl_sel);
    XIF_TRACE(0, "Local MAC");
    DUMP8("%02x ", config->local_addr.address, 6);
    XIF_TRACE(0, "Other MAC");
    DUMP8("%02x ", config->peer_addr.address, 6);

    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();
}

static int xspace_interface_ibrt_if_reconfig(ibrt_config_t *config)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    p_ibrt_ctrl->nv_role = config->nv_role;
    p_ibrt_ctrl->audio_chnl_sel = config->audio_chnl_sel;
    memcpy(p_ibrt_ctrl->peer_addr.address, config->peer_addr.address, 6);
    memcpy(p_ibrt_ctrl->local_addr.address, config->local_addr.address, 6);

    xspace_interface_ibrt_nvrecord_config_set(config);

    return 0;
}


/**
 ****************************************************************************************
 * @brief flash related
 ****************************************************************************************
 */
void xspace_interface_register_write_flash_befor_shutdown_cb(SHUTDOWN_WRITE_FLASH_MANAGE_E write_flash_enum, shutdown_write_flash_manage_func cb)
{
    if(cb == NULL)
        return;

    if(write_flash_enum < BEFOR_SHUTDOWN_WRITE_FLASH_NUM)
    {
        shutdown_write_flash_manage_func_cb[write_flash_enum] = cb;
    }
}

void xspace_interface_write_data_to_flash_befor_shutdown(void)
{
    for(uint8_t i = 0; i < BEFOR_SHUTDOWN_WRITE_FLASH_NUM; i++)
    {
        if(shutdown_write_flash_manage_func_cb[i] != NULL)
        {
            (*shutdown_write_flash_manage_func_cb[i])();
        }
    }
}

/**
 ****************************************************************************************
 * @brief time related
 ****************************************************************************************
 */

int xspace_interface_delay_timer_start(uint32_t time, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1){
    return xspace_interface_process_delay_timer_start(time, ptr, id, param0, param1);
}

int xspace_interface_delay_timer_stop(uint32_t ptr){
    return xspace_interface_process_delay_timer_stop(ptr);
}

uint32_t xspace_interface_get_current_ms(void){
    return TICKS_TO_MS(GET_CURRENT_TICKS());
}

uint32_t xspace_interface_get_current_sec(void){
    return TICKS_TO_MS(GET_CURRENT_TICKS())/1000;
}

/**
 ****************************************************************************************
 * @brief OTA related
 ****************************************************************************************
 */
bool xspace_interface_process_bat_allow_enter_ota_mode(void)
{
    XIF_TRACE(1, " %d.", xspace_ui_get_local_bat_percentage());
    if (xspace_ui_get_local_bat_percentage() < XIF_OTA_BATTERY_MIN_PERCENT) {
        return false;
    }
    
    return true;
}

void xspace_interface_enter_ota_mode(void)
{
    if (!xspace_interface_process_bat_allow_enter_ota_mode()) {
        XIF_TRACE(0, " battery low, Not allowed to enter ota mode!!!");
        return ;
    }

    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
    osDelay(100);
    TRACE_IMM(1, "%s,reboot!", __func__);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
    pmu_reboot();
}

/**
 ****************************************************************************************
 * @brief TWS related
 ****************************************************************************************
 */
void xspace_interface_freeman_config(void)
{
    uint8_t p_addr[6] ={0};
    ibrt_config_t local_ibrt_config = {0};
    factory_section_original_btaddr_get(p_addr);
    p_addr[1]++;
    memcpy((void *)local_ibrt_config.local_addr.address, p_addr, 6);
    memcpy((void *)local_ibrt_config.peer_addr.address, p_addr, 6);
    local_ibrt_config.nv_role = IBRT_UNKNOW;
    local_ibrt_config.audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
    xspace_interface_ibrt_if_reconfig(&local_ibrt_config);
}

bool xspace_interface_tws_pairing_config(uint8_t *addr)
{
    uint8_t bt_mac_addr[6] = {0};
    ibrt_config_t local_ibrt_config;

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
    if(xspace_ui_is_box_closed()) {
        XIF_TRACE(0, " box closed, can not enter tws pairng!!!");
        return false;
    }
#endif

    xspace_interface_force_disconnect_all_mobile_link();
    factory_section_original_btaddr_get(bt_mac_addr);
    bool use_same_addr = app_ibrt_conn_get_tws_use_same_addr_enable();
    XIF_TRACE(1, "use same address %d.", use_same_addr);
    if (use_same_addr) {
        if (LEFT_SIDE == app_tws_get_earside()) {
            local_ibrt_config.nv_role = IBRT_SLAVE;
            memcpy((void *)local_ibrt_config.local_addr.address, addr, 6);
            memcpy((void *)local_ibrt_config.peer_addr.address, addr, 6);
            local_ibrt_config.audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        } else {
            local_ibrt_config.nv_role = IBRT_MASTER;
            memcpy((void *)local_ibrt_config.local_addr.address, bt_mac_addr, 6);
            memcpy((void *)local_ibrt_config.peer_addr.address, bt_mac_addr, 6);
            local_ibrt_config.audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        }
        xspace_interface_ibrt_if_reconfig(&local_ibrt_config);
    } else {
        memset(bt_mac_addr, 0x00, sizeof(bt_mac_addr));
        if (memcmp(addr, (char *)bt_mac_addr, 6)) {
            memcpy((void *)local_ibrt_config.mobile_addr.address, bt_mac_addr, 6);
            factory_section_original_btaddr_get(bt_mac_addr);

            if (LEFT_SIDE == app_tws_get_earside()) {
                local_ibrt_config.nv_role = IBRT_SLAVE;
            } else {
                local_ibrt_config.nv_role = IBRT_MASTER;
            }

            if (IBRT_MASTER == local_ibrt_config.nv_role) {
                memcpy((void *)local_ibrt_config.local_addr.address, bt_mac_addr, 6);
                memcpy((void *)local_ibrt_config.peer_addr.address, addr, 6);
            } else if (IBRT_SLAVE == local_ibrt_config.nv_role) {
                memcpy((void *)local_ibrt_config.local_addr.address, bt_mac_addr, 6);
                memcpy((void *)local_ibrt_config.peer_addr.address, addr, 6);
            }

            xspace_interface_ibrt_if_reconfig(&local_ibrt_config);
        }
    }

    return true;
}

bool xspace_interface_tws_is_master_mode(void)
{
    return (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))?true:false;
}

bool xspace_interface_tws_is_slave_mode(void)
{
    return (IBRT_SLAVE== app_tws_ibrt_role_get_callback(NULL))?true:false;
}

bool xspace_interface_tws_is_freeman_mode(void)
{
    return app_ibrt_if_is_in_freeman_mode();
}

uint8_t xspace_interface_get_device_current_roles(void)
{
    return app_tws_ibrt_role_get_callback(NULL);
}

bool xspace_interface_device_is_in_paring_mode(void)
{
    if (g_bt_access_mode == BTIF_BAM_GENERAL_ACCESSIBLE) {
        return true;
    }
    return false;
}

void xspace_interface_tws_pairing_enter(void)
{
    xspace_interface_delay_timer_start(500, (uint32_t)app_ibrt_if_enter_pairing_after_tws_connected, 0, 0, 0);
}
/*
 ****************************************************************************************
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief BT profile link related
 ****************************************************************************************
 */
void xspace_interface_force_disconnect_all_mobile_link(void)
{
    XIF_TRACE(2, "%d,%d.", app_ibrt_if_is_any_mobile_connected(), xspace_interface_tws_is_slave_mode());

    app_set_disconnecting_all_bt_connections(true);
    if(osapi_lock_is_exist())
        osapi_lock_stack();

#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    struct BT_DEVICE_T *curr_device = NULL;
    if(p_ibrt_ctrl->init_done && xspace_interface_tws_is_master_mode() && app_ibrt_if_is_any_mobile_connected()) {
        for (int i = 0; i < BT_DEVICE_NUM; ++i) {
            curr_device = app_bt_get_device(i);
            if (curr_device->acl_is_connected) {
                app_tws_ibrt_disconnect_connection(btif_me_get_remote_device_by_handle(curr_device->acl_conn_hdl));
            }
        }
    }

    if(osapi_lock_is_exist())
        osapi_unlock_stack();

   osDelay(50);
#endif

}

int32_t xspace_interface_disconnect_hfp(void)
{
    return 0;
}

int32_t xspace_interface_disconnect_a2dp(void)
{
    return 0;
}

bool xspace_interface_is_a2dp_mode(void)
{
    for(int j =0;j<BT_DEVICE_NUM;j++){
        if(app_bt_is_a2dp_streaming(j))
            return true;
    }

    return false;
}

bool xspace_interface_is_sco_mode(void)
{
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_hf_channel_id);
    if (app_bt_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN){
        return true;
    }
    return false;
}

bool xspace_interface_tws_link_connected(void)
{
    return app_ibrt_if_is_tws_link_connected();
}

bool xspace_interface_is_any_mobile_connected(void)
{
    return app_ibrt_if_is_any_mobile_connected();
}

bool xspace_interface_tws_ibrt_link_connected(void)
{
    return (app_ibrt_if_is_tws_link_connected() && app_ibrt_if_is_any_mobile_connected());
}

bool xspace_interface_have_mobile_record(void)
{
    uint8_t paired_dev_count = nv_record_get_paired_dev_count();
    XIF_TRACE(1, "paired dev num %d.", paired_dev_count);
    if (paired_dev_count > 0)
        return true;

    return false;
}

uint8_t xspace_interface_get_avrcp_connect_status(void)
{
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_a2dp_stream_id);
    return app_bt_device->avrcp_palyback_status;
}

bool xspace_interface_a2dp_connected(uint8_t device_id)
{
    return app_bt_is_a2dp_connected(device_id);
}

bool xspace_interface_hfp_connected(uint8_t device_id)
{
    return app_bt_is_hfp_connected(device_id);
}

bool xspace_interface_avrcp_connected(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = NULL;
        curr_device = app_bt_get_device(device_id);
        if (curr_device->avrcp_conn_flag){
            return true;
        }
    return false;
}

bool xspace_interface_mobile_link_connected_by_id(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (curr_device->acl_is_connected)
        return true;

    return false;
}

/**
 ****************************************************************************************
 * @brief Music control related
 ****************************************************************************************
 */
void xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_E status)
{

    if (xspace_interface_get_avrcp_connect_status() == false) {
        XIF_TRACE(0, "avrcp don't connected.");
        return;
    }

    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_a2dp_stream_id);

    avrcp_media_status_t media_status = app_bt_device->avrcp_palyback_status;
    static xui_tws_ctx_t xui_tws_ctx = {0};

    XIF_TRACE(2, "status %d, media_status %d", status, media_status);

    switch (status) {
        case XIF_MUSIC_STATUS_PLAYING:
        case XIF_MUSIC_STATUS_PAUSE:
        case XIF_MUSIC_STATUS_STOP:
            XIF_TRACE(1, "media_status=%d", media_status);
            xspace_ui_tws_ctx_get(&xui_tws_ctx);
            if ((BTIF_AVRCP_MEDIA_PAUSED == media_status) || (BTIF_AVRCP_MEDIA_STOPPED == media_status)) {
                xui_tws_ctx.local_sys_info.music_control_status.music_status = XUI_MUSIC_GESTURE_PLAYING;
                a2dp_handleKey(AVRCP_KEY_PLAY);
            } else {
                xui_tws_ctx.local_sys_info.music_control_status.music_status = XUI_MUSIC_GESTURE_PAUSED;
                a2dp_handleKey(AVRCP_KEY_PAUSE);
            }
            xspace_ui_tws_ctx_set(xui_tws_ctx);
            break;
        case XIF_MUSIC_STATUS_FORWARD:
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        case XIF_MUSIC_STATUS_BACKWARD:
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        case XIF_MUSIC_STATUS_VOLUP:
            xspace_interface_bt_volumeup();
            //TODO(Mike.Cheng):send ibrt  sync event for volume controll
            //xspace_interface_ibrt_sync_volume_info();
            break;
        case XIF_MUSIC_STATUS_VOLDOWN:
            xspace_interface_bt_volumedown();
            //TODO(Mike.Cheng):send ibrt  sync event for volume controll
            //xspace_interface_ibrt_sync_volume_info();
            break;
        default:
            XIF_TRACE(0, "err music status");
            break;
    }
}

uint8_t xspace_interface_get_a2dp_volume(uint8_t id)
{
    return a2dp_volume_get((BT_DEVICE_ID_T)id);
}

bool xspace_interface_audio_status(void)
{
    //Note(Mike.Cheng): Get Bt HFP/A2dp Audio is triggered
    bool ret = false;
    if (is_a2dp_mode() || is_sco_mode()) {
        ret = true;
    }
    return ret;
}

void xspace_interface_ibrt_sync_volume_info(void)
{
#if 0
    xui_tws_sync_info_s tws_sync_info;
    if (!xspace_interface_tws_link_connected()) {
        XIF_TRACE(0, "-TWS info exchange channel is not connected!!!/n");
        return;
    }
    xspace_ui_tws_ctx_get(&xui_tws_ctx);
    memset((void *)&tws_sync_info, 0x00, sizeof(tws_sync_info));
    XIF_TRACE(0, " - Sync tws hfp volume info/n");
    XIF_TRACE(0, " - Tx: XUI_SYS_INFO_SYNC_HFP_VOLUME_INFO/n");
    tws_sync_info.event = XUI_SYS_INFO_SYNC_HFP_VOLUME_INFO;
    tws_sync_info.need_rsp = false;
    tws_sync_info.data_len = sizeof(xui_bat_sync_info_s);
#endif
    return;
}

/**
 ****************************************************************************************
 * @brief Calling control related
 ****************************************************************************************
 */
bool xspace_interface_call_is_active(void)
{
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_hf_channel_id);
    TRACE(3, " current hf channel:%d, callsetup:%d, audio_status:%d ", app_bt_manager.curr_hf_channel_id, app_bt_device->hfchan_callSetup,
          app_bt_device->hf_audio_state);
    if ((app_bt_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN) || (app_bt_device->hfchan_call == BTIF_HF_CALL_ACTIVE)
        || (app_bt_device->hf_callheld == BTIF_HF_CALL_HELD_ACTIVE)) {
        return true;
    } else {
        return false;
    }
}

void xspace_interface_call_ctrl(XIF_CALL_STATUS_E status)
{

    //Note(Mike.Cheng): hfp key ctrl
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_hf_channel_id);
    TRACE(3, " current hf channel:%d, callsetup:%d, audio_status:%d ", app_bt_manager.curr_hf_channel_id, app_bt_device->hfchan_callSetup,
          app_bt_device->hf_audio_state);

    if ((app_bt_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_NONE) && (app_bt_device->hfchan_call == BTIF_HF_CALL_NONE)
        && (app_bt_device->hf_callheld == BTIF_HF_CALL_HELD_NONE)) {
        return;
    }

    switch (status) {
        case XIF_CALL_STATUS_ANSWER:
            hfp_handle_key(HFP_KEY_ANSWER_CALL);
            break;
        case XIF_CALL_STATUS_HANGUP:
            hfp_handle_key(HFP_KEY_HANGUP_CALL);
            break;
        case XIF_CALL_STATUS_THREEWAY_HOLD_AND_ANSWER:
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);
            break;
        case XIF_CALL_STATUS_THREEWAY_HANGUP_AND_ANSWER:
            hfp_handle_key(HFP_KEY_THREEWAY_HANGUP_AND_ANSWER);
            break;
        case XIF_CALL_STATUS_THREEWAY_HOLD_REL_INCOMING:
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_REL_INCOMING);
            break;
        case XIF_CALL_STATUS_VOLUP:
            xspace_interface_bt_volumeup();
            //TODO(Mike.Cheng):send ibrt  sync event for volume controll
            //xspace_interface_ibrt_sync_volume_info();
            break;
        case XIF_CALL_STATUS_VOLDOWN:
            xspace_interface_bt_volumedown();
            //TODO(Mike.Cheng):send ibrt  sync event for volume controll
            //xspace_interface_ibrt_sync_volume_info();
            break;
        default:
            XIF_TRACE(0, "err call status.");
            break;
    }
}

bool xspace_interface_call_is_incoming(void)
{
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_hf_channel_id);
    TRACE(3, " current hf channel:%d, callsetup:%d, audio_status:%d ", app_bt_manager.curr_hf_channel_id, app_bt_device->hfchan_callSetup,
          app_bt_device->hf_audio_state);

    if ((app_bt_device->hfchan_callSetup == BTIF_HF_CALL_SETUP_IN) && (app_bt_device->hfchan_call == BTIF_HF_CALL_NONE)
        && (app_bt_device->hf_callheld == BTIF_HF_CALL_HELD_NONE)) {
        return true;
    } else {
        return false;
    }
}

bool xspace_interface_call_is_online(void)
{
    struct BT_DEVICE_T *app_bt_device = app_bt_get_device(app_bt_manager.curr_hf_channel_id);
    TRACE(3, " current hf channel:%d, callsetup:%d, audio_status:%d ", app_bt_manager.curr_hf_channel_id, app_bt_device->hfchan_callSetup,
          app_bt_device->hf_audio_state);

    if ((app_bt_device->hfchan_callSetup == BTIF_HF_CALL_ACTIVE) && (app_bt_device->hfchan_call == BTIF_HF_CALL_SETUP_NONE)
        && (app_bt_device->hf_callheld == BTIF_HF_CALL_HELD_NONE)) {
        return true;
    } else {
        return false;
    }
}

int32_t xspace_interface_call_answer_call(void)
{

    if (true == xspace_interface_call_is_incoming()) {
        hfp_handle_key(HFP_KEY_ANSWER_CALL);
        return 0;
    } else {
        return 1;
    }
}

int32_t xspace_interface_call_hangup_call(void)
{
    if (true == btapp_hfp_get_call_active() || xspace_interface_call_is_active() == true) {
        hfp_handle_key(HFP_KEY_HANGUP_CALL);
        return 0;
    } else {
        return 1;
    }
}

uint8_t xspace_interface_get_hfp_volume(uint8_t id)
{
    return hfp_volume_local_get((BT_DEVICE_ID_T)id);
}

void xspace_interface_wakeup_siri(void)
{
    //Note(Mike.Cheng):For siri/voice assistant, use bes flag
    XIF_TRACE(1, "cur siri staus = %d", open_siri_flag);

    if (open_siri_flag) {
        app_hfp_siri_voice(false);
    } else {
        app_hfp_siri_voice(true);
    }
}

void xspace_interface_bt_volumeup()
{
    //Note(Mike.Cheng):local dev
    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_UP, 0);
    return;
}


void xspace_interface_bt_volumedown()
{
    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_DOWN, 0);
}

/**
 ****************************************************************************************
 * @brief Other
 ****************************************************************************************
 */
void xspace_interface_enter_dut_mode(void)
{
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE && hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE) {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
        osDelay(100);
        XIF_TRACE(0, "reboot! exit dut mode");
        pmu_reboot();
    } else {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
        osDelay(100);
        XIF_TRACE(0, "reboot!");
        pmu_reboot();
    }
}

void xspace_interface_init(void)
{
    xspace_interface_process_init();
}

void xspace_interface_reset_factory_setting(void)
{
    xspace_interface_update_info_when_shutdown_or_reboot();
    xspace_interface_force_disconnect_all_mobile_link();
    nv_record_rebuild();
    nv_record_env_init();

    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();

#if defined(__REBOOT_FORCE_PAIRING__)
    osDelay(50);
    TRACE_IMM(1, "%s,reboot!", __func__);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_PAIRING);
#endif
}

void xspace_interface_reboot(void)
{
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
    osDelay(100);
    XIF_TRACE(0, "reboot!");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
    //hal_cmu_sys_reboot();
    pmu_reboot();
}

void xspace_interface_update_info_when_shutdown_or_reboot(void)
{
    //TODO?
}

void xspace_interface_shutdown(void)
{
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
    osDelay(100);
    TRACE_IMM(1, "%s,shutdown!", __func__);
    xspace_battery_manager_enter_shutdown_mode();
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    pmu_shutdown();
}

XIF_AUDIO_STATUS_E xspace_interface_get_audio_status(void)
{
    XIF_AUDIO_STATUS_E xif_bt_audio_status = XIF_AUDIO_IDLE;
    if (is_a2dp_mode()) {
        xif_bt_audio_status = XIF_AUDIO_SBC;
    } else if (is_sco_mode()) {
        xif_bt_audio_status = XIF_AUDIO_SCO;
    } else {
        xif_bt_audio_status = XIF_AUDIO_IDLE;
    }

    return xif_bt_audio_status;
}

void xspace_interface_event_process(xif_event_e msg_id, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1)
{
    xspace_interface_process_event_process(msg_id, ptr, id, param0, param1);
}

void xspace_interface_ibrt_nvrecord_config_load(void *config)
{
    ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);
    factory_section_original_btaddr_get(ibrt_config->local_addr.address);

    /** customer can replace this with custom nv role configuration */
    /** by default bit 0 of the first byte decides the nv role:
        1: master and right bud
        0: slave and left bud
    */
    if (app_tws_get_earside() == RIGHT_SIDE)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        ibrt_config->nv_role = IBRT_MASTER;
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address , BD_ADDR_LEN);
    }
    else if (app_tws_get_earside() == LEFT_SIDE)
    {
        ibrt_config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        ibrt_config->nv_role = IBRT_SLAVE;
        memcpy(ibrt_config->peer_addr.address, nvrecord_env->ibrt_mode.record.bdAddr.address, BD_ADDR_LEN);
    }

    nvrecord_env->ibrt_mode.mode = ibrt_config->nv_role;
    app_ibrt_conn_set_ui_role(nvrecord_env->ibrt_mode.mode);

    return;
}

#ifdef IBRT_SEARCH_UI
void xspace_interface_enter_search_ui_mode(void)
{

#if defined(__XSPACE_INTETFACE_TRACE__)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    XIF_TRACE(0, "app_start_tws_serching_direactly");
    XIF_TRACE(1, "p_ibrt_ctrl->current_role: %d", p_ibrt_ctrl->current_role);
#endif
    //Both ears are in limited mode, and then right goes to search
    app_ibrt_enter_limited_mode();
    if (app_tws_get_earside() == APP_EAR_SIDE_RIGHT) {
        app_start_tws_serching_direactly();
    } else if (app_tws_get_earside() == APP_EAR_SIDE_LEFT) {
#if defined(IBRT_SEARCH_UI)
        app_slave_tws_serching_start_by_slave();
#endif
    }
}
#endif

