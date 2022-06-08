#include "stdio.h"
#include "cmsis.h"
#include "apps.h"
#include "app_bt.h"
#include "hal_pmu.h"
#include "app_hfp.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "xspace_ui.h"
#include "besaud_api.h"
#include "app_ibrt_if.h"
#include "tgt_hardware.h"
#include "app_tws_ibrt.h"
#include "nvrecord_env.h"
#include "xspace_dev_info.h"
#include "xspace_interface.h"
#include "app_ibrt_customif_cmd.h"
#include "app_ibrt_customif_ui.h"
#include "app_a2dp.h"
#include "hal_bootmode.h"
#include "resources.h"
#include "app_utils.h"
#include "app_bt_stream.h"
#include "app_audio.h"
#include "pmu.h"
#include "app_bt_media_manager.h"
#include "app_hfp.h"
#include <stdio.h>
#ifdef GFPS_ENABLED 
#include "app_gfps.h"
#endif

#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#if defined(__XSPACE_XBUS_MANAGER__)
#include "hal_xbus.h"
#include "xspace_xbus_manager.h"
#endif

#if defined(__XSPACE_PRODUCT_TEST__)
#include "xspace_pt_cmd_analyze.h"
#endif

#if defined(__XSPACE_INOUT_BOX_MANAGER__)
#include "xspace_inout_box_manager.h"
#endif

#if defined(__XSPACE_BATTERY_MANAGER__)
#include "hal_charger.h"
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
#include "xspace_cover_switch_manager.h"
#endif

#if defined(__XSPACE_GESTURE_MANAGER__)
#include "xspace_gesture_manager.h"
#endif

#if defined(__XSPACE_RSSI_TWS_SWITCH__)
#include "xspace_rssi_tws_switch.h"
#endif
#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
#include "xspace_low_battery_tws_switch.h"
#endif
#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
#include "xspace_snr_wnr_tws_switch.h"
#endif
#if defined(__XSPACE_TWS_SWITCH__)
#include "xspace_tws_switch_process.h"
#endif

#if defined (__XSPACE_IMU_MANAGER__)
#include "xspace_imu_manager.h"
#endif
#ifdef __XSPACE_CUSTOMIZE_ANC__
#include "app_anc.h"
#include "app_anc_assist.h"
#endif

/******************************* Static Variable/Func Delcaration *******************************/
static void xspace_ui_init_done(void);
#if defined(__XSPACE_BATTERY_MANAGER__)
static void xspace_ui_battery_low_voice_report(void);
static void xspace_ui_tws_bat_info_sync(void);
#endif

#if defined(__XSPACE_INOUT_BOX_MANAGER__)
static void xspace_ui_inout_box_status_change_handle(xiob_status_e status);
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
static void xspace_ui_cover_status_change_handle(xcsm_status_e status);
#endif

static void app_voice_report_connect_prompt_device0(void);
static void app_voice_report_connect_prompt_device1(void);

#if defined(__FORCE_UPDATE_A2DP_VOL__)
static void xspace_ui_a2dp_vol_force_changed(uint8_t device_id);
#endif

#if defined(__XSPACE_XBUS_OTA__)
static xbus_ota_status_e xbus_ota_status = XBUS_OTA_STATUS_QTY;
#endif

#if defined(__XSPACE_CUSTOMIZE_ANC__)
//Note(Mike):add for record anc mode after user changed the anc mode
static uint8_t cur_anc_mode_index = 0;
#endif

static xui_tws_ctx_t xui_tws_ctx;
static uint16_t a2dp_mute_flag = 0xffff;
static xui_sys_status_e xui_sys_status = XUI_SYS_INVALID;
static uint32_t wear_on_tick = 0;
/*Note(Mike.Cheng): mobiles connect voice play status :
                    bit0->device0, 1: voice promte had played
                    bit1->device1, 1: voice promte had played
*/
static uint8_t mobile_connect_voice_status = 0;
#define XSPACE_UI_MSG_BUF_CNT  8
#define XSPACE_UI_MSG_BUF_SIZE (XUI_TWS_SYNC_INFO_BUFF_LENGTH + 4)
static uint8_t xspace_ui_msg_buf[XSPACE_UI_MSG_BUF_CNT * XSPACE_UI_MSG_BUF_SIZE] = {0};

/***************************** Extern Variable/Func Declaration****************************/
extern "C" void nv_record_print_all(void);
extern void app_ibrt_if_event_entry(ibrt_mgr_evt_t event);
extern "C" uint8_t is_sco_mode(void);
extern "C" uint8_t is_a2dp_mode(void);

extern "C" bool get_sys_abnormal_reboot_flag(void);
extern bool app_get_ibrt_stack_init_result(void);
extern "C" bool get_anc_vmic_status(void);
extern "C" void anc_vmic_enable(bool enable);
extern "C" int app_bt_stream_closeall();
extern uint32_t get_bt_a2dp_mode_open_tick(void);
extern "C" bool get_sys_abnormal_reboot_flag(void);
extern "C" void clear_sys_abnormal_reboot_flag(void);
extern "C" AUD_ID_ENUM app_prompt_checker(void);
#ifdef GFPS_ENABLED
extern "C" void app_gfps_battery_hide_show(void);
extern "C" void app_fp_msg_send_battery_levels(void);
#endif
extern "C" int app_play_audio_press_dwon_up_onoff(bool onoff, uint16_t aud_id);
extern void app_bt_stream_force_retigger_by_xspace(void);
extern bool ibrt_stack_init_finish;
#ifdef USER_REBOOT_PLAY_MUSIC_AUTO
extern bool a2dp_need_to_play;
#endif

#if defined(__TOUCH_GH621X__)
/*Note(Mike.Cheng): check factory gh6212 calib result
                    0:fail   1:sucess
*/
uint8_t g_wear_calib_check_flag = 0;
uint8_t g_press_calib_check_flag = 0;
#endif

/******************************************************************************************/

int xspace_ui_msg_buf_get(uint8_t **buff, uint32_t size)
{
    int i;
    if (size >= XSPACE_UI_MSG_BUF_SIZE) {
        XUI_TRACE(2, "too much memory required,Req:%d MAX:%d", size, XSPACE_UI_MSG_BUF_SIZE - 1);
        return 0;
    }
    for (i = 0; i < XSPACE_UI_MSG_BUF_CNT; i++) {
        if (xspace_ui_msg_buf[i * XSPACE_UI_MSG_BUF_SIZE] != 0x55) {
            xspace_ui_msg_buf[i * XSPACE_UI_MSG_BUF_SIZE] = 0x55;
            *buff = &xspace_ui_msg_buf[i * XSPACE_UI_MSG_BUF_SIZE + 1];
            // XUI_TRACE(1," msg get addr=%d",(uint32_t)*buff);
            return size;
        }
    }
    XUI_TRACE(0, "warning! malloc fail");
    return 0;
}

int xspace_ui_msg_buf_free(uint8_t *buff)
{
    // XUI_TRACE(1, " msg free addr=%d", (uint32_t)buff);
    buff--;
    if ((buff >= xspace_ui_msg_buf)
        && (buff < (xspace_ui_msg_buf + XSPACE_UI_MSG_BUF_CNT * XSPACE_UI_MSG_BUF_SIZE))) {
        buff[0] = 0;
    }
    return 0;
}

uint8_t xspace_ui_tws_ctx_set(xui_tws_ctx_t ctx)
{
    memcpy(&xui_tws_ctx, &ctx, sizeof(xui_tws_ctx_t));
    return 0;
}

uint8_t xspace_ui_tws_ctx_get(xui_tws_ctx_t *ctx)
{
    memcpy(ctx, &xui_tws_ctx, sizeof(xui_tws_ctx_t));
    return 0;
}

void xspace_ui_set_system_status(xui_sys_status_e status)
{
    XUI_TRACE(1, " Set system status: %d", status);
    xui_sys_status = status;
}

xui_sys_status_e xspace_ui_get_system_status(void)
{
    if (!xspace_interface_audio_status() && !xspace_interface_call_is_active()) {
        XUI_TRACE(1, " Get system status: %d", xui_sys_status);
    }
    return xui_sys_status;
}

bool xspace_ui_init_finish_over(void)
{
    if (XUI_SYS_INIT == xui_sys_status || XUI_SYS_INVALID == xui_sys_status) {
        return false;
    }
    return true;
}

void xspace_ui_boost_freq_when_busy(void)
{
    app_sysfreq_req(APP_SYSFREQ_USER_BUSY, APP_SYSFREQ_104M);
    xspace_interface_delay_timer_stop((uint32_t)app_sysfreq_req);
    xspace_interface_delay_timer_start(6 * 1000, (uint32_t)app_sysfreq_req, APP_SYSFREQ_USER_BUSY,
                                       APP_SYSFREQ_32K, 0);
}

void xspace_ui_let_peer_run_func(uint32_t time, uint32_t func_id, uint32_t id, uint32_t param0,
                                 uint32_t param1)
{
    uint32_t peer_run_cmd_info[5] = {time, func_id, id, param0, param1};
    XUI_TRACE(1, "func_id:%d.", func_id);
    xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_PEER_RUN_FUNC, (uint8_t*)&peer_run_cmd_info, sizeof(peer_run_cmd_info));
}

static void app_voice_report_connect_prompt_device0(void)
{
    XUI_TRACE(3, "device0:sco_mode:%d,online:%d,a2dp_mode:%d.", is_sco_mode(),
          xspace_interface_call_is_online(), is_a2dp_mode());
    if (is_sco_mode() || xspace_interface_call_is_online() || is_a2dp_mode()) {
        XUI_TRACE(0, "device0:a2dp mode or sco mode,can not play connect prompt voice!");
        return;
    }
    // TODO(Mark)
}

static void app_voice_report_connect_prompt_device1(void)
{
    XUI_TRACE(3, "device1:sco_mode:%d,online:%d,a2dp_mode:%d.", is_sco_mode(),
          xspace_interface_call_is_online(), is_a2dp_mode());
    if (is_sco_mode() || xspace_interface_call_is_online() || is_a2dp_mode()) {
        XUI_TRACE(0, "device1:a2dp mode or sco mode,can not play connect prompt voice!");
        return;
    }
    // TODO(Mark)
}

#if defined(__FORCE_UPDATE_A2DP_VOL__)
static void xspace_ui_a2dp_vol_force_changed(uint8_t device_id)
{
	if (0 == device_id)
	{
        xspace_interface_delay_timer_stop((uint32_t)btapp_a2dp_vol_force_changed_d0);
        xspace_interface_delay_timer_start(2000, (uint32_t)btapp_a2dp_vol_force_changed_d0, device_id, 0, 0);		
	}
	else if (1 == device_id)
	{
        xspace_interface_delay_timer_stop((uint32_t)btapp_a2dp_vol_force_changed_d1);
        xspace_interface_delay_timer_start(2000, (uint32_t)btapp_a2dp_vol_force_changed_d1, device_id, 0, 0);			
	}
}

#endif	

static uint8_t get_mobile_connect_status(void)
{
    uint8_t mobile_connect_status = 0;

    for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
        XUI_TRACE(4, "device%d:%d,%d,%d.", i, xspace_interface_a2dp_connected(i),
              xspace_interface_hfp_connected(i), xspace_interface_avrcp_connected(i));

        uint8_t j = 0;
        if (xspace_interface_a2dp_connected(i)) {
            j++;
        }
        if (xspace_interface_hfp_connected(i)) {
            j++;
        }
        if (xspace_interface_avrcp_connected(i)) {
            j++;
        }

        if (j > 1) {
            mobile_connect_status |= (1 << i);
        }
    }

    return mobile_connect_status;
}

void set_mobile_connect_voice_status(uint8_t status, uint8_t test_flag)
{
    XUI_TRACE(1, "status:%d,>>>>test_flag:%d.", status, test_flag);
    mobile_connect_voice_status = status;
}

static void clear_mobile_connect_voice_status(uint8_t device_id)
{
    mobile_connect_voice_status &= ~(1 << device_id);
    XUI_TRACE(1, "mobile_connect_voice_status:%d.", mobile_connect_voice_status);
    xspace_ui_let_peer_run_func(0, XUI_SET_CONECT_VOICE_PROMPT_STATUS, mobile_connect_voice_status,
                                3, 0);
}

static void notice_master_call_mobile_connect_voice_check_handle(void)
{
    xspace_ui_let_peer_run_func(0, XUI_MOBILE_CONNECT_VOICE_PROMPT_STATUS_CHECK, 0, 0, 0);
}

void xspace_ui_mobile_connect_voice_check_handle(uint8_t bt_connect_tick_flag)
{
    uint8_t mobile_connect_status = get_mobile_connect_status();
    static uint32_t mobile_connect_tick = 0xffffffff - 2000;
    bool retry_flag = (mobile_connect_voice_status == mobile_connect_status) ? false : true;
    uint8_t device0_play_flag = 0;

    XUI_TRACE(2, "inear,local:%d,peer:%d(1:off,2:on).", xui_tws_ctx.local_sys_info.inear_status,
          xui_tws_ctx.peer_sys_info.inear_status);
    XUI_TRACE(3, "mobile_connect_voice_status:%d,%d,retry_flag:%d.", mobile_connect_voice_status,
          mobile_connect_status, retry_flag);

    if (bt_connect_tick_flag == 1) {
        mobile_connect_tick = xspace_interface_get_current_ms();
    }

    XUI_TRACE(2, "bt_connect_tick_flag:%d,mobile_connect_tick:%d", bt_connect_tick_flag,
          mobile_connect_tick);

    xspace_interface_delay_timer_stop((uint32_t)xspace_ui_mobile_connect_voice_check_handle);
    if (retry_flag == false) {
        return;
    }

    XUI_TRACE(1, "current_role:%d.", xspace_interface_get_device_current_roles());

    if (XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status
        && XUI_INEAR_ON != xui_tws_ctx.peer_sys_info.inear_status) {
        return;
    }

    xspace_ui_boost_freq_when_busy();

    if (xspace_interface_get_device_current_roles() == IBRT_SLAVE) {
        xspace_interface_delay_timer_stop(
            (uint32_t)notice_master_call_mobile_connect_voice_check_handle);
        xspace_interface_delay_timer_start(
            1000, (uint32_t)notice_master_call_mobile_connect_voice_check_handle, 0, 0, 0);
        xspace_interface_delay_timer_start(
            2000, (uint32_t)xspace_ui_mobile_connect_voice_check_handle, 0, 0, 0);
        return;
    }

    if ((mobile_connect_voice_status & 0x01) == 0 && (mobile_connect_status & 0x01) == 1) {
        mobile_connect_voice_status |= 0x01;
        xspace_interface_delay_timer_stop((uint32_t)app_voice_report_connect_prompt_device0);
        if (xspace_interface_get_current_ms() > 2000 + mobile_connect_tick) {
            device0_play_flag = 1;
            xspace_interface_delay_timer_start(
                1000, (uint32_t)app_voice_report_connect_prompt_device0, 0, 0, 0);
        } else {
            device0_play_flag = 2;
            xspace_interface_delay_timer_start(
                2500, (uint32_t)app_voice_report_connect_prompt_device0, 0, 0, 0);
        }
    }

    if ((mobile_connect_voice_status & 0x02) == 0 && (mobile_connect_status & 0x02) == 2) {
        mobile_connect_voice_status |= 0x02;
        xspace_interface_delay_timer_stop((uint32_t)app_voice_report_connect_prompt_device1);
        if (device0_play_flag == 0) {
            if (xspace_interface_get_current_ms() > 2000 + mobile_connect_tick) {
                xspace_interface_delay_timer_start(
                    1000, (uint32_t)app_voice_report_connect_prompt_device1, 0, 0, 0);
            } else {
                xspace_interface_delay_timer_start(
                    2500, (uint32_t)app_voice_report_connect_prompt_device1, 0, 0, 0);
            }
        } else if (device0_play_flag == 1) {
            xspace_interface_delay_timer_start(
                1500, (uint32_t)app_voice_report_connect_prompt_device1, 0, 0, 0);
        } else if (device0_play_flag == 2) {
            xspace_interface_delay_timer_start(
                4000, (uint32_t)app_voice_report_connect_prompt_device1, 0, 0, 0);
        }
    }

    xspace_ui_let_peer_run_func(0, XUI_SET_CONECT_VOICE_PROMPT_STATUS, mobile_connect_voice_status,
                                2, 0);
    xspace_interface_delay_timer_start(2000, (uint32_t)xspace_ui_mobile_connect_voice_check_handle,
                                       0, 0, 0);
}

void xspace_ui_mobile_disconnect_voice_check_handle(uint8_t device_id)
{
    XUI_TRACE(2, "current_role:%d,tws_aud:%d.", xspace_interface_get_device_current_roles(),
          xspace_interface_tws_link_connected());
    if (xspace_interface_get_device_current_roles() == IBRT_SLAVE) {
        return;
    }

    clear_mobile_connect_voice_status(device_id);
}

void xspace_ui_mobile_connect_status_change_handle(uint8_t device_id, bool status, uint8_t reason)
{
    uint8_t __attribute__((unused)) current_role = xspace_interface_get_device_current_roles();
    static uint8_t last_mobile_connect_status[2] = {0};

    XUI_TRACE(3, "status:%d,last_status[%d]:%d.", status, device_id,
               last_mobile_connect_status[device_id]);
    XUI_TRACE(3, "current_role:%d,tws_link:%d,tws_aud:%d.", current_role,
               xspace_interface_tws_link_connected(), xspace_interface_tws_link_connected());
    XUI_TRACE(3, "mobile_link[%d]:%d,reason:0x%x.", device_id,
               xspace_interface_mobile_link_connected_by_id(device_id), reason);

    if (device_id >= 2) {
        return;
    }
    if (status) {
        xspace_ui_mobile_connect_voice_check_handle(1);
        xspace_ui_let_peer_run_func(0, XUI_MOBILE_CONNECT_VOICE_PROMPT_STATUS_CHECK, 1, 0, 0);
#if defined(__FORCE_UPDATE_A2DP_VOL__)
		if(app_ibrt_if_event_has_been_queued(&app_bt_get_device(device_id)->remote,IBRT_MGR_EV_CASE_OPEN))
        {
			xspace_ui_a2dp_vol_force_changed(device_id);
        }
#endif
    } else {
        clear_mobile_connect_voice_status(device_id);
    }
    if (status == last_mobile_connect_status[device_id]) {
        XUI_TRACE(1, "bt connect status same,ignore,%d!!!", device_id);
        return;
    }
    last_mobile_connect_status[device_id] = status;

    /**bt connect status*/
    XUI_TRACE(1, "app_bt_count_connected_device: %d", app_bt_count_connected_device());
    if (status) {
        if (xui_sys_status == XUI_SYS_FORCE_TWS_PARING
            || xui_sys_status == XUI_SYS_FORCE_FREEMAN_PARING) {
            xui_sys_status = XUI_SYS_NORMAL;
        }
    }
}

void xspace_ui_tws_connect_status_change_handle(bool status, uint8_t reason)
{
    uint8_t current_role = xspace_interface_get_device_current_roles();
    XUI_TRACE(3, "status:%d,current role:%d,reason:0x%x.", status, current_role, reason);

    xspace_ui_boost_freq_when_busy();

    /**tws connect status*/
    if (status) {
#if defined(__XSPACE_TWS_SWITCH__)
        xspace_tws_switch_detect_start();
#endif

#if defined(__XSPACE_UI__)
        if (current_role == IBRT_MASTER || current_role == IBRT_SLAVE) {
            if (current_role == IBRT_MASTER /*&& !xspace_interface_have_mobile_record()*/) {
                // Note:customer req:enter pairing if TWS reconnected
                app_ibrt_if_event_entry(IBRT_MGR_EV_TWS_PAIRING);
            }
        }
#endif
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_TOTAL_INFO_EVENT, NULL, 0);

        if (mobile_connect_voice_status > 0) {
            xspace_ui_let_peer_run_func(0, XUI_SET_CONECT_VOICE_PROMPT_STATUS,
                                        mobile_connect_voice_status, 4, 0);
        }

#if defined(USER_REBOOT_PLAY_MUSIC_AUTO)
        if (a2dp_need_to_play == true && current_role == IBRT_SLAVE) {
            xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT, NULL,
                                             0);
        }
#endif
    } else {
#if defined(__XSPACE_TWS_SWITCH__)
        xspace_tws_switch_detect_stop();
#endif
        if (xspace_interface_is_any_mobile_connected() == false) {
            set_mobile_connect_voice_status(0, 5);
        }
        xui_tws_ctx.peer_sys_info.cover_status = XUI_COVER_UNKNOWN;
        xui_tws_ctx.peer_sys_info.inear_status = XUI_INEAR_UNKNOWN;
        xui_tws_ctx.peer_sys_info.inout_box_status = XUI_BOX_UNKNOWN;

        if (xui_tws_ctx.local_sys_info.cover_status == XUI_COVER_CLOSE) {
            mobile_connect_voice_status = 0;
        }
    }

#if defined(__XSPACE_BATTERY_MANAGER__)
    xspace_ui_tws_bat_info_sync();
#endif
}

#if defined(__XSPACE_XBUS_OTA__)
static void xspace_ui_xbus_ota_status_get(xbus_ota_status_e *status)
{
    *status = xbus_ota_status;
    return;
}

static void xspace_ui_xbus_ota_enter(void)
{

    xspace_ui_inout_box_status_change_handle(XIOB_IN_BOX);
    xspace_ui_cover_status_change_handle(XCSM_BOX_STATUS_CLOSED);

    xbus_ota_status = XBUS_OTA_STATUS_ENTER;
}
#endif

#if defined(__XSPACE_GESTURE_MANAGER__)
static void xspace_ui_gesture_freeman_left_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;//防抖
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
    //奇数次亮灯，偶数次灭灯
}

static void xspace_ui_gesture_freeman_right_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_left_double_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_right_double_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_left_three_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_right_three_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_left_longpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_right_longpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_left_longlongpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_freeman_right_longlongpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

extern uint8_t curr_goodix_device_id;

static void xspace_ui_gesture_freeman_debug_send_data(uint8_t *data, uint16_t len)
{

    if (NULL == data || len == 0)
        return;

    if (xspace_interface_is_any_mobile_connected()) {
    }
}

static void xspace_ui_gesture_tws_left_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am master,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)

    //jinyao_learning:左耳单击播放welcome提示音
    app_voice_report(APP_STATUS_INDICATION_PROMPT_WELCOME,0);
}

static void xspace_ui_gesture_tws_right_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am master,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)

    //jinyao_learning:右耳单击播放welcome提示音
    app_voice_report(APP_STATUS_INDICATION_PROMPT_WELCOME,0);
}

static void xspace_ui_gesture_tws_left_double_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am master,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_right_double_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am master,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_left_three_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();

    XUI_TRACE(2, "I am master,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_right_three_click_handler(void)
{
    static uint32_t last_enter_tick = 0;

    if (xspace_interface_get_current_ms() - last_enter_tick < 1000) {
        return;
    }
    last_enter_tick = xspace_interface_get_current_ms();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_left_longpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_right_longpress_handler(void)
{
    XUI_TRACE_ENTER();
    XUI_TRACE(2, "I am freeman,is_sco_mode:%d,%d.", xspace_interface_is_sco_mode(),
          btapp_hfp_get_call_active());
    // TODO(Mark)
}

static void xspace_ui_gesture_tws_left_longlongpress_handler(void)
{
    // TODO(Mike.Cheng)
}

static void xspace_ui_gesture_tws_right_longlongpress_handler(void)
{
    //TODO(Mike.Cheng)
}

static void xspace_gesture_single_earphone_handle(xgm_event_type_e event_type, uint8_t *data,
                                                  uint16_t len)
{

    XUI_TRACE(3, "ear_side=%d, event_type=%d, wear_status=%d.", app_tws_get_earside(), event_type,
          xui_tws_ctx.local_sys_info.inear_status);
    if (LEFT_SIDE == app_tws_get_earside()) {
        switch (event_type) {
            case XGM_EVENT_CLICK:
                xspace_ui_gesture_freeman_left_click_handler();
                break;

            case XGM_EVENT_DOUBLE_CLICK:
                xspace_ui_gesture_freeman_left_double_click_handler();
                break;

            case XGM_EVENT_TRIPLE_CLICK:
                xspace_ui_gesture_freeman_left_three_click_handler();
                break;

            case XGM_EVENT_LONGPRESS:
                xspace_ui_gesture_freeman_left_longpress_handler();
                break;

            case XGM_EVENT_LONGLONGPRESS:
                xspace_ui_gesture_freeman_left_longlongpress_handler();
                break;

            case APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA:
                xspace_ui_gesture_freeman_debug_send_data(data, len);
                break;

            default:
                XUI_TRACE(0, " Unknown event!!!");
                break;
        }
    } else if (RIGHT_SIDE == app_tws_get_earside()) {
        switch (event_type) {
            case XGM_EVENT_CLICK:
                xspace_ui_gesture_freeman_right_click_handler();
                break;

            case XGM_EVENT_DOUBLE_CLICK:
                xspace_ui_gesture_freeman_right_double_click_handler();
                break;

            case XGM_EVENT_TRIPLE_CLICK:
                xspace_ui_gesture_freeman_right_three_click_handler();
                break;

            case XGM_EVENT_LONGPRESS:
                xspace_ui_gesture_freeman_right_longpress_handler();
                break;

            case XGM_EVENT_LONGLONGPRESS:
                xspace_ui_gesture_freeman_right_longlongpress_handler();
                break;

            case APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA:
                xspace_ui_gesture_freeman_debug_send_data(data, len);
                break;

            default:
                XUI_TRACE(0, " Unknown event!!!");
                break;
        }
    }
}

static void xspace_gesture_tws_earphone_handle(uint8_t local_flag, xgm_event_type_e event_type,
                                               uint8_t *data, uint16_t len)
{

    XUI_TRACE(3, "ear_side=%d, event_type=%d, wear_status=%d.", app_tws_get_earside(), event_type,
          xui_tws_ctx.local_sys_info.inear_status);

    if (app_tws_get_earside() == ((local_flag == 1) ? LEFT_SIDE : RIGHT_SIDE)) {
        switch (event_type) {
            case XGM_EVENT_CLICK:
                xspace_ui_gesture_tws_left_click_handler();
                break;

            case XGM_EVENT_DOUBLE_CLICK:
                xspace_ui_gesture_tws_left_double_click_handler();
                break;

            case XGM_EVENT_TRIPLE_CLICK:
                xspace_ui_gesture_tws_left_three_click_handler();
                break;

            case XGM_EVENT_LONGPRESS:
                xspace_ui_gesture_tws_left_longpress_handler();
                break;

            case XGM_EVENT_LONGLONGPRESS:
                xspace_ui_gesture_tws_left_longlongpress_handler();
                break;

            default:
                XUI_TRACE(0, " Unknown event!!!");
                break;
        }
    } else if (app_tws_get_earside() == ((local_flag == 1) ? RIGHT_SIDE : LEFT_SIDE)) {
        switch (event_type) {
            case XGM_EVENT_CLICK:
                xspace_ui_gesture_tws_right_click_handler();
                break;

            case XGM_EVENT_DOUBLE_CLICK:
                xspace_ui_gesture_tws_right_double_click_handler();
                break;

            case XGM_EVENT_TRIPLE_CLICK:
                xspace_ui_gesture_tws_right_three_click_handler();
                break;

            case XGM_EVENT_LONGPRESS:
                xspace_ui_gesture_tws_right_longpress_handler();
                break;

            case XGM_EVENT_LONGLONGPRESS:
                xspace_ui_gesture_tws_right_longlongpress_handler();
                break;

            default:
                XUI_TRACE(0, " Unknown event!!!");
                break;
        }
    }
}

static void xspace_gesture_status_change_handle(xgm_event_type_e event_type, uint8_t *data,
                                                uint16_t len)
{
    uint8_t temp = event_type;

#if defined(__XSPACE_XBUS_OTA__)
    xbus_ota_status_e ota_status = XBUS_OTA_STATUS_QTY;
    xspace_ui_xbus_ota_status_get(&ota_status);
    if (XBUS_OTA_STATUS_ENTER == ota_status) {
        return;
    }
#endif

    XUI_TRACE(2, "event:%d,inear:%d.", event_type, xui_tws_ctx.local_sys_info.inear_status);

    if (xui_tws_ctx.local_sys_info.inear_status != XUI_INEAR_ON
        && xspace_ui_get_system_status() != XUI_SYS_PRODUCT_TEST) {
        return;
    }

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
    switch(event_type)
    {
        case XGM_EVENT_UP:
            app_play_audio_press_dwon_up_onoff(false, AUD_ID_BT_PRESSURE_UP);
            app_play_audio_press_dwon_up_onoff(true, AUD_ID_BT_PRESSURE_UP);
            return;
        case XGM_EVENT_DOWN:
            app_play_audio_press_dwon_up_onoff(false, AUD_ID_BT_PRESSURE_DOWN);
            app_play_audio_press_dwon_up_onoff(true, AUD_ID_BT_PRESSURE_DOWN);
            return;
        default:
            break;
    }
#endif

    if (!xspace_interface_tws_link_connected()) {
        xspace_gesture_single_earphone_handle(event_type, data, len);//单耳
    } else if (xspace_interface_tws_is_master_mode() && xspace_interface_tws_link_connected()) {
        xspace_gesture_tws_earphone_handle(true, event_type, data, len);//主耳触发
    } else if (xspace_interface_tws_is_slave_mode() && xspace_interface_tws_link_connected()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_GESTURE_INFO_EVENT, &temp, 1);//从耳触发
    }
}

static bool xspace_ui_gesture_manage_need_execute(xgm_event_type_e event_type, uint8_t *data,
                                                  uint16_t len)
{
    return true;
}
#endif

void xspace_ui_check_a2dp_data(uint8_t *buf, uint32_t len)
{
    uint32_t i = 0;
    static uint16_t mute_count = 0;
    static uint16_t entry_count = 0;

    entry_count++;

    for (i = 0; i < len; i++) {
        if (buf[i] != 0)
            break;
    }

    if (i == len) {
        mute_count++;
        a2dp_mute_flag = mute_count / 25;
    } else {
        mute_count = 0;
        a2dp_mute_flag = 0;   // other play
    }

    // XUI_TRACE(4, "%d,%d,%d,%d.", i, len, mute_count, a2dp_mute_flag);

    if (XUI_SYS_PRODUCT_TEST == xspace_ui_get_system_status()) {
        return;
    }

    if (XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status) {
        if (entry_count % 50 == 0) {
            XUI_TRACE(0, "I am inbox mute,a2dp!!!!!");
        }
        memset(buf, 0, len);
    }
}

bool xspace_ui_check_sco_data(uint8_t *buf, uint32_t len)
{
    if ((XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status
         && XUI_SYS_PRODUCT_TEST != xspace_ui_get_system_status())) {
        XUI_TRACE(1, "I am inbox mute,sco!!!!!");
        memset(buf, 0, len);
        return true;
    } else {
        return false;
    }
}

bool xspace_ui_check_mic_data(uint8_t *buf, uint32_t len)
{
    if (XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status
        && XUI_SYS_PRODUCT_TEST != xspace_ui_get_system_status()) {
        XUI_TRACE(0, "I am inbox mute,mic!!!!!");
        memset(buf, 0, len);
        return true;
    } else {
        return false;
    }
}

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)

#include "xspace_inear_detect_manager.h"
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
extern "C" uint8_t gh621xenablePowerOnOffset;
#endif

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
#include "app_bt_media_manager.h"
#define WEAR_AUTO_PLAY_CLEAR_TIME (3 * 60000)   // 3min
static uint8_t re_sync_wear_status_cnt = 0;
static uint32_t wear_auto_play_tick = 0;

static void freeman_later_re_sync_wear_status_timeout_handler(void)
{
    XUI_TRACE(4, "%d,%d,%d,%d.", xspace_interface_is_any_mobile_connected(), a2dp_mute_flag,
          xspace_interface_is_a2dp_mode(), re_sync_wear_status_cnt);

    if (!xspace_interface_is_any_mobile_connected()) {
        re_sync_wear_status_cnt = 0;
        xui_tws_ctx.wear_auto_play = 0;
        return;
    }

    if (xspace_interface_is_sco_mode() || xspace_interface_call_is_online()) {
        re_sync_wear_status_cnt = 0;
        return;
    }

    XUI_TRACE(2, "wear_auto_play diff time:%d,media_status:%d.",
          xspace_interface_get_current_ms() - wear_auto_play_tick,
          bt_media_is_media_active_by_type(BT_STREAM_MEDIA));

    if (xspace_interface_get_current_ms() - wear_auto_play_tick > WEAR_AUTO_PLAY_CLEAR_TIME) {
        XUI_TRACE(0, "over 3min time, freeman clear wear_auto_play!!!!!!!!!!!!!!!");
        xui_tws_ctx.wear_auto_play = 0;
    }

    if (xui_tws_ctx.wear_auto_play == 1
        && XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status) {
        if ((!xspace_interface_is_a2dp_mode()
             || (xspace_interface_is_a2dp_mode() && a2dp_mute_flag >= 2))
            && bt_media_is_media_active_by_type(BT_STREAM_MEDIA) == 0) {
            re_sync_wear_status_cnt = 0;
            xui_tws_ctx.wear_auto_play = 0;
            XUI_TRACE(0, "freeman wear on, play!!!!!!!!!!!!!!!");
            a2dp_handleKey(AVRCP_KEY_PLAY);
        } else {
            if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA) == 1) {
                XUI_TRACE(0, "media play!!!");
                re_sync_wear_status_cnt = 0;
            }

            re_sync_wear_status_cnt++;
            if (re_sync_wear_status_cnt < 8) {
                xspace_interface_process_delay_timer_start(
                    200, (uint32_t)freeman_later_re_sync_wear_status_timeout_handler, 0, 0, 0);
            } else {
                re_sync_wear_status_cnt = 0;
            }
        }
    } else {
        re_sync_wear_status_cnt = 0;
    }
}
#endif

#if defined (__SMART_INEAR_MIC_SWITCH__)
static uint8_t hfp_mic_switch_try_cnt = 0;
static bool user_switch_mic_in_phone_flag = false;

void xspace_ui_inear_hfp_mic_switch(bool start_flag)
{
    XUI_TRACE(4, "connect,mobile:%d,tws:%d,avrcp:%d,role_switch:%d.",
                 xspace_interface_is_any_mobile_connected(),
                 xspace_interface_tws_link_connected(),
                 xspace_interface_get_avrcp_connect_status(),
                 app_ibrt_customif_ui_is_tws_switching());

    XUI_TRACE(3, "local_wear:%d,peer_wear:%d(1:off,2,on),hfp:%d.",
                 xui_tws_ctx.local_sys_info.inear_status,
                 xui_tws_ctx.peer_sys_info.inear_status,
                 xspace_interface_call_is_online());

    if(start_flag)
    {
        hfp_mic_switch_try_cnt = 4;
    }

    if(!xspace_interface_get_avrcp_connect_status() || !xspace_interface_call_is_online())
    {
        hfp_mic_switch_try_cnt = 0;
        return;
    }

    if(app_ibrt_customif_ui_is_tws_switching())
    {
        XUI_TRACE(0, "role switching!!!");
        hfp_mic_switch_try_cnt = 0;
        xspace_interface_delay_timer_stop((uint32_t)xspace_ui_inear_hfp_mic_switch);
        xspace_interface_delay_timer_start(500, (uint32_t)xspace_ui_inear_hfp_mic_switch, 0, 0, 0);
        return;
    }

    /**tws connected*/
    if (xspace_interface_tws_link_connected())
    {
        if(xspace_interface_is_any_mobile_connected())
        {
            if((XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status ||
                XUI_INEAR_ON == xui_tws_ctx.peer_sys_info.inear_status) &&
                app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == BTIF_HF_AUDIO_DISCON &&
                user_switch_mic_in_phone_flag == false)
            {
                hfp_handle_key(HFP_KEY_ADD_TO_EARPHONE);
            }
            else if(XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status &&
                    XUI_INEAR_ON != xui_tws_ctx.peer_sys_info.inear_status &&
                    app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == BTIF_HF_AUDIO_CON)
            {
                hfp_handle_key(HFP_KEY_CHANGE_TO_PHONE);
            }
            else
            {
                XUI_TRACE(0, "tws,mic config success,%d!!!", user_switch_mic_in_phone_flag);
                hfp_mic_switch_try_cnt = 0;
                user_switch_mic_in_phone_flag = false;
            }
        }
    }
    else
    {
        if(xspace_interface_is_any_mobile_connected())
        {
            if(XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status &&
               app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == BTIF_HF_AUDIO_DISCON)
            {
                hfp_handle_key(HFP_KEY_ADD_TO_EARPHONE);
            }
            else if(XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status &&
                    app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == BTIF_HF_AUDIO_CON)
            {
                hfp_handle_key(HFP_KEY_CHANGE_TO_PHONE);
            }
            else
            {
                XUI_TRACE(0, "freeman,mic config success!!!");
                hfp_mic_switch_try_cnt = 0;
                user_switch_mic_in_phone_flag = false;
            }
        }
    }

    if(hfp_mic_switch_try_cnt > 0)
    {
        hfp_mic_switch_try_cnt--;
        XUI_TRACE(1, "retry,%d!!!", hfp_mic_switch_try_cnt);
        xspace_interface_delay_timer_stop((uint32_t)xspace_ui_inear_hfp_mic_switch);
        xspace_interface_delay_timer_start(500, (uint32_t)xspace_ui_inear_hfp_mic_switch, 0, 0, 0);
    }
}
#endif


static void xspace_ui_inear_status_changed_process(void)
{
    static uint8_t last_local_inear_status = 0xff;
    static uint8_t last_peer_inear_status = 0xff;
    static uint8_t wear_off_check_cnt = 0;
    static uint8_t play_status_order = 0;
    static uint8_t touch_status_order = 0;//是否可沿用沿用音频流的入耳检测？？？
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    static uint8_t wear_on_check_cnt = 0;
    static uint8_t last_wear_auto_play = 0xff;
#endif

    XUI_TRACE(3, "connect,mobile:%d,tws:%d,avrcp:%d.", xspace_interface_is_any_mobile_connected(),
          xspace_interface_tws_link_connected(), xspace_interface_get_avrcp_connect_status());

    XUI_TRACE(3, "local_wear:%d,peer_wear:%d(1:off,2,on),hfp:%d.",
          xui_tws_ctx.local_sys_info.inear_status, xui_tws_ctx.peer_sys_info.inear_status,
          btapp_hfp_get_call_active());

    XUI_TRACE(2, "inbox status,local:%d, peer:%d(1:out,2.in).",
          xui_tws_ctx.local_sys_info.inout_box_status, xui_tws_ctx.peer_sys_info.inout_box_status);

    XUI_TRACE(2, "box status,local:%d,peer:%d(1:close,2:open).",
          xui_tws_ctx.local_sys_info.cover_status, xui_tws_ctx.peer_sys_info.cover_status);

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    struct nvrecord_env_t *nvrecord_env;
    if (nv_record_env_get(&nvrecord_env) == 0) {
        if (nvrecord_env->wear_auto_play_func_switch == true) {
            xui_tws_ctx.wear_auto_play = 0;
            wear_off_check_cnt = 0;
            wear_on_check_cnt = 0;
            last_wear_auto_play = 0xff;
            last_local_inear_status = 0xff;
            last_peer_inear_status = 0xff;
            XUI_TRACE(1, "wear auto play function stop,%d!!!",
                  xui_tws_ctx.wear_auto_play_func_switch_flag);
            return;
        }
    }
#endif

    if (xspace_interface_is_sco_mode() || xspace_interface_call_is_online()) {
#if defined (__SMART_INEAR_MIC_SWITCH__)
        if (xspace_interface_call_is_online())
        {
            xspace_ui_inear_hfp_mic_switch(true);
        }
#endif
        wear_off_check_cnt = 0;
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        wear_on_check_cnt = 0;
        re_sync_wear_status_cnt = 0;
#endif
        return;
    }

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    XUI_TRACE(1, "wear_auto_play:%d.", xui_tws_ctx.wear_auto_play);
#endif

    /**tws connected*/
    if (xspace_interface_tws_link_connected()) {
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        if (XUI_COVER_CLOSE == xui_tws_ctx.local_sys_info.cover_status
            && XUI_COVER_CLOSE == xui_tws_ctx.peer_sys_info.cover_status) {
            xui_tws_ctx.wear_auto_play = 0;
            wear_off_check_cnt = 0;
            wear_on_check_cnt = 0;
        }
#endif

        if (last_local_inear_status == xui_tws_ctx.local_sys_info.inear_status
            && last_peer_inear_status == xui_tws_ctx.peer_sys_info.inear_status) {
            if (1 &&
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
                wear_on_check_cnt == 0 &&
#endif
                wear_off_check_cnt == 0) {
                return;
            }
        } else {
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
            wear_on_check_cnt = 0;
#endif
            wear_off_check_cnt = 0;
        }

        XUI_TRACE(0, "------------------------------------------");
        XUI_TRACE(2, "local_wear:%d,peer_wear:%d(1:off,2,on).", xui_tws_ctx.local_sys_info.inear_status,
              xui_tws_ctx.peer_sys_info.inear_status);

        XUI_TRACE(2, "last:local_wear:%d,peer_wear:%d(1:off,2,on).", last_local_inear_status,
              last_peer_inear_status);
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        XUI_TRACE(2, "wear_on_check_cnt:%d,wear_off_check_cnt:%d.", wear_on_check_cnt,
              wear_off_check_cnt);
#else
        XUI_TRACE(1, "wear_off_check_cnt:%d.", wear_off_check_cnt);
#endif
        XUI_TRACE(2, "a2dp_mode:%d,a2dp_mute_flag:%d.", xspace_interface_is_a2dp_mode(),
              a2dp_mute_flag);
        XUI_TRACE(0, "------------------------------------------");

        POSSIBLY_UNUSED uint8_t test_flag = 0;

        if (xui_tws_ctx.local_sys_info.inear_status != xui_tws_ctx.peer_sys_info.inear_status) {
            if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                && 0xff == last_local_inear_status
                && XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                && 0xff == last_peer_inear_status) {    //本耳入耳了，对耳未入耳
                if (xspace_interface_is_a2dp_mode()) {
                    test_flag = 1;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;    //这里应该是双耳模式
                }
                touch_status_order=BTIF_AVRCP_MEDIA_PLAYING;
            } else if (XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                       && 0xff == last_local_inear_status
                       && XUI_INEAR_ON == xui_tws_ctx.peer_sys_info.inear_status
                       && 0xff == last_peer_inear_status) {
                if (xspace_interface_is_a2dp_mode()) {
                    test_flag = 2;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;    //同上
                }
                touch_status_order=BTIF_AVRCP_MEDIA_PLAYING;
            } else if ((XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_ON != last_local_inear_status)
                       || (XUI_INEAR_ON == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_ON != last_peer_inear_status)) {
                test_flag = 3;
                play_status_order = BTIF_AVRCP_MEDIA_PLAYING;   //双耳均保持入耳，可继续播放
                touch_status_order=BTIF_AVRCP_MEDIA_PLAYING;
            } else if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                       && XUI_INEAR_ON == last_local_inear_status
                       && XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                       && XUI_INEAR_UNKNOWN == last_peer_inear_status) {//对耳是否入耳未知
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 4;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;
                } else {
                    test_flag = 5;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
                touch_status_order=BTIF_AVRCP_MEDIA_PLAYING;
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_ON == last_local_inear_status)
                       || (XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_ON == last_peer_inear_status)) {
                test_flag = 6;
                play_status_order = BTIF_AVRCP_MEDIA_PAUSED;//双耳均明确已摘出
            } else if ((XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_ON == last_local_inear_status)
                       && (XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_OFF == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 7;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//如果已经建立音频流，则本耳继续播放
                } else {
                    test_flag = 8;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
                touch_status_order=BTIF_AVRCP_MEDIA_PLAYING;
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_OFF == last_local_inear_status)
                       && (XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_UNKNOWN == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 9;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//如果有音频流，则保持原音频流的播放
                } else {
                    test_flag = 10;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_UNKNOWN == last_local_inear_status)
                       && (XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_UNKNOWN == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 11;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//同上
                } else {
                    test_flag = 12;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_UNKNOWN == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_UNKNOWN == last_local_inear_status)
                       && (XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_UNKNOWN == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 13;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//同上
                } else {
                    test_flag = 14;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_OFF == last_local_inear_status)
                       && (XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_OFF == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 15;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//同上
                } else {
                    test_flag = 16;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_UNKNOWN == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_OFF == last_local_inear_status)
                       && (XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_OFF == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 17;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//同上
                } else {
                    test_flag = 18;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                       && XUI_INEAR_ON == last_local_inear_status
                       && XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status
                       && XUI_INEAR_UNKNOWN == last_peer_inear_status) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 19;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;//若音频流存在则保持原音频流的播放
                } else {
                    test_flag = 20;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            }
        } else if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
                   && XUI_INEAR_ON == xui_tws_ctx.peer_sys_info.inear_status) {
            test_flag = 21;
            play_status_order = BTIF_AVRCP_MEDIA_PLAYING;
        } else if (XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status
                   && XUI_INEAR_ON != xui_tws_ctx.peer_sys_info.inear_status) {
            if ((XUI_INEAR_UNKNOWN == xui_tws_ctx.local_sys_info.inear_status)
                && (XUI_INEAR_UNKNOWN == xui_tws_ctx.peer_sys_info.inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 22;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;
                } else {
                    test_flag = 23;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_ON != last_local_inear_status)
                       && (XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_ON != last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 24;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;
                } else {
                    test_flag = 25;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else if ((XUI_INEAR_OFF == xui_tws_ctx.local_sys_info.inear_status
                        && XUI_INEAR_OFF == last_local_inear_status)
                       && (XUI_INEAR_OFF == xui_tws_ctx.peer_sys_info.inear_status
                           && XUI_INEAR_OFF == last_peer_inear_status)) {
                if (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0) {
                    test_flag = 26;
                    play_status_order = BTIF_AVRCP_MEDIA_PLAYING;
                } else {
                    test_flag = 27;
                    play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
                }
            } else {
                test_flag = 28;
                play_status_order = BTIF_AVRCP_MEDIA_PAUSED;
            }
        }

        last_local_inear_status = xui_tws_ctx.local_sys_info.inear_status;
        last_peer_inear_status = xui_tws_ctx.peer_sys_info.inear_status;

        XUI_TRACE(1, "play_status_order:%d(1:play;2:pause),test_flag:%d.", play_status_order,
              test_flag);
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        last_wear_auto_play = xui_tws_ctx.wear_auto_play;
#endif

        if (play_status_order == BTIF_AVRCP_MEDIA_PAUSED) {
            XUI_TRACE(1, ">>1,role_switch:%d.", app_ibrt_customif_ui_is_tws_switching());
            XUI_TRACE(2, ">>2,is_a2dp_mode:%d,a2dp_mute_flag:%d.", xspace_interface_is_a2dp_mode(),
                  a2dp_mute_flag);

            if (xspace_interface_get_avrcp_connect_status()) {
                if (xspace_interface_is_any_mobile_connected() && xspace_interface_is_a2dp_mode()
                    && (a2dp_mute_flag == 0
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
                        || xui_tws_ctx.wear_auto_play == 0)
#endif
                    && !app_ibrt_customif_ui_is_tws_switching()) {
                    XUI_TRACE(0, "ether wear off, pause!!!!!!!!!!!!!!!");
                    a2dp_handleKey(AVRCP_KEY_PAUSE);
                    wear_off_check_cnt = 0;
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
                    xui_tws_ctx.wear_auto_play = 1;
                    wear_auto_play_tick = xspace_interface_get_current_ms();
#endif
                } else {
                    if (app_ibrt_customif_ui_is_tws_switching()) {
                        XUI_TRACE(0, "role switching!!!");
                        wear_off_check_cnt = 1;
                    }

                    XUI_TRACE(1, "wear_off_check_cnt = %d.", wear_off_check_cnt);
                    if (wear_off_check_cnt < 8) {
                        wear_off_check_cnt++;
                        xspace_interface_delay_timer_stop(
                            (uint32_t)xspace_ui_inear_status_changed_process);
                        xspace_interface_delay_timer_start(
                            200, (uint32_t)xspace_ui_inear_status_changed_process, 0, 0, 0);
                    } else {
                        if (xspace_interface_is_any_mobile_connected()
                            && !app_ibrt_customif_ui_is_tws_switching()) {
                            wear_off_check_cnt = 0;
                        }
                    }
                }
            }
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
            wear_on_check_cnt = 0;
            XUI_TRACE(1, "wear_auto_play = %d.", xui_tws_ctx.wear_auto_play);
#endif
        }
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        else if (play_status_order == BTIF_AVRCP_MEDIA_PLAYING) {
            XUI_TRACE(2, ">>3,is_a2dp_mode:%d,a2dp_mute_flag:%d.", xspace_interface_is_a2dp_mode(),
                  a2dp_mute_flag);
            XUI_TRACE(2, ">>4,role_switch:%d,wear_auto_play:%d.",
                  app_ibrt_customif_ui_is_tws_switching(), xui_tws_ctx.wear_auto_play);
            XUI_TRACE(2, "wear_auto_play diff time:%d,media_status:%d.",
                  xspace_interface_get_current_ms() - wear_auto_play_tick,
                  bt_media_is_media_active_by_type(BT_STREAM_MEDIA));

            wear_off_check_cnt = 0;

            if (xui_tws_ctx.wear_auto_play != 0
                && xspace_interface_get_current_ms() - wear_auto_play_tick
                    > WEAR_AUTO_PLAY_CLEAR_TIME) {
                XUI_TRACE(0, "over 3min time, clear wear_auto_play!!!!!!!!!!!!!!!");
                xui_tws_ctx.wear_auto_play = 0;
            }

            if (xspace_interface_get_avrcp_connect_status() && xui_tws_ctx.wear_auto_play == 1) {
                if (xspace_interface_is_any_mobile_connected()
                    && (!xspace_interface_is_a2dp_mode()
                        || (xspace_interface_is_a2dp_mode() && a2dp_mute_flag >= 2))
                    && bt_media_is_media_active_by_type(BT_STREAM_MEDIA) == 0
                    && !app_ibrt_customif_ui_is_tws_switching()) {
                    wear_on_check_cnt = 0;
                    XUI_TRACE(0, "ether wear on, play!!!!!!!!!!!!!!!");
                    a2dp_handleKey(AVRCP_KEY_PLAY);
                    xui_tws_ctx.wear_auto_play = 0;
                } else {
                    if (app_ibrt_customif_ui_is_tws_switching()) {
                        XUI_TRACE(0, "role switching!!!");
                        wear_on_check_cnt = 1;
                    }

                    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA) == 1) {
                        XUI_TRACE(0, "media play!!!");
                        wear_on_check_cnt = 1;
                    }

                    XUI_TRACE(1, "wear_on_check_cnt = %d.", wear_on_check_cnt);
                    if (wear_on_check_cnt < 20) {
                        wear_on_check_cnt++;
                        xspace_interface_delay_timer_stop(
                            (uint32_t)xspace_ui_inear_status_changed_process);
                        xspace_interface_delay_timer_start(
                            200, (uint32_t)xspace_ui_inear_status_changed_process, 0, 0, 0);
                    } else {
                        xui_tws_ctx.wear_auto_play = 0;
                        wear_on_check_cnt = 0;
                    }
                }
            }
        }

        if (last_wear_auto_play != xui_tws_ctx.wear_auto_play) {
            last_wear_auto_play = xui_tws_ctx.wear_auto_play;
            if (xspace_interface_is_any_mobile_connected()) {
                xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT,
                                                 &last_wear_auto_play, 1);
            }
        }
#endif
    }
    /**tws disconnected*/
    else {
        if (!xspace_interface_is_any_mobile_connected()) {
            return;
        }

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        if (XUI_COVER_CLOSE == xui_tws_ctx.local_sys_info.cover_status) {
            xui_tws_ctx.wear_auto_play = 0;
            wear_off_check_cnt = 0;
            wear_on_check_cnt = 0;
        }
#endif
        uint8_t music_ctrl_flag = 0;
        if ((XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
             && XUI_INEAR_UNKNOWN == last_local_inear_status)) {
            music_ctrl_flag = 2;
        } else {
            music_ctrl_flag = 0;
        }
        // XUI_TRACE(4,"avrcp:%d a2dp:%d %d
        // %d",xspace_interface_get_avrcp_connect_status(),xspace_interface_is_a2dp_mode(),a2dp_mute_flag,music_ctrl_flag);
        if (XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status) {
            if (xspace_interface_get_avrcp_connect_status()
                && (xspace_interface_is_a2dp_mode() && a2dp_mute_flag == 0)
                && music_ctrl_flag == 0) {
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
                xui_tws_ctx.wear_auto_play = 1;
                wear_auto_play_tick = xspace_interface_get_current_ms();
#endif
                XUI_TRACE(0, "freeman earphone wear off, pause!!!!!!!!!!!!!!!");
                a2dp_handleKey(AVRCP_KEY_PAUSE);
            }
        } else {
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
            XUI_TRACE(3, "wear_auto_play = %d, a2dp_mute_flag = %d, is_a2dp_mode = %d.",
                  xui_tws_ctx.wear_auto_play, a2dp_mute_flag, xspace_interface_is_a2dp_mode());

            XUI_TRACE(2, "wear_auto_play diff time:%d,media_status:%d.",
                  xspace_interface_get_current_ms() - wear_auto_play_tick,
                  bt_media_is_media_active_by_type(BT_STREAM_MEDIA));

            if (xui_tws_ctx.wear_auto_play != 0
                && xspace_interface_get_current_ms() - wear_auto_play_tick
                    > WEAR_AUTO_PLAY_CLEAR_TIME) {
                XUI_TRACE(0, "over 3min time, freeman clear wear_auto_play!!!!!!!!!!!!!!!");
                xui_tws_ctx.wear_auto_play = 0;
            }

            if (xspace_interface_get_avrcp_connect_status() && xui_tws_ctx.wear_auto_play == 1) {
                if ((!xspace_interface_is_a2dp_mode()
                     || (xspace_interface_is_a2dp_mode() && a2dp_mute_flag >= 2))
                    && bt_media_is_media_active_by_type(BT_STREAM_MEDIA) == 0) {
                    xui_tws_ctx.wear_auto_play = 0;
                    XUI_TRACE(0, "freeman wear on, play!!!!!!!!!!!!!!!");
                    a2dp_handleKey(AVRCP_KEY_PLAY);
                } else {
                    re_sync_wear_status_cnt = 0;
                    xspace_interface_delay_timer_stop(
                        (uint32_t)freeman_later_re_sync_wear_status_timeout_handler);
                    xspace_interface_delay_timer_start(
                        200, (uint32_t)freeman_later_re_sync_wear_status_timeout_handler, 0, 0, 0);
                }
            }
#endif
        }
    }
}

void xspace_ui_inear_detect_manager_status_change(inear_detect_manager_status_e inear_status)
{
    ibrt_mgr_evt_t ibrt_event = IBRT_MGR_EV_NONE;
    static uint8_t first_wear = true;
#if defined(__XSPACE_XBUS_OTA__)
    xbus_ota_status_e ota_status = XBUS_OTA_STATUS_QTY;
    xspace_ui_xbus_ota_status_get(&ota_status);
    if (XBUS_OTA_STATUS_ENTER == ota_status) {
        return;
    }
#endif

    if ((first_wear == true) && (xspace_interface_get_current_ms() < 3500)
        && (INEAR_DETECT_MG_IN_EAR == inear_status)) {
        xspace_interface_delay_timer_stop((uint32_t)xspace_ui_inear_detect_manager_status_change);
        xspace_interface_delay_timer_start(
            2000, (uint32_t)xspace_ui_inear_detect_manager_status_change, inear_status, 0, 0);
        first_wear = false;
        return;
    }

    XUI_TRACE(2, " status:%d,current status:%d(1:off;2.on)!", inear_status,
          xui_tws_ctx.local_sys_info.inear_status);

    if (xui_tws_ctx.local_sys_info.inear_status == (xui_inear_status_e)inear_status) {
        XUI_TRACE(0, " repeat event!!!");
        return;
    }
    // add by chenjunjie 20210601 start
    if (xui_tws_ctx.local_sys_info.inout_box_status == XUI_BOX_IN_BOX) {
        XUI_TRACE(0, " in box, can not change inear status!!!");
        return;
    }
    // add by chenjunjie 20210601 end
    xspace_ui_boost_freq_when_busy();

    if (INEAR_DETECT_MG_DETACH_EAR == inear_status) {
        XUI_TRACE(0, " EAR_Detech");
#if defined(__XSPACE_GESTURE_MANAGER__)
        xspace_gesture_manage_enter_standby_mode();//待机、低功耗模式
#endif
        xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_OFF;
        ibrt_event = IBRT_MGR_EV_WEAR_DOWN;
    } else if (INEAR_DETECT_MG_IN_EAR == inear_status) {
        XUI_TRACE(0, " EAR_IN");
        xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_ON;
#if defined(__XSPACE_GESTURE_MANAGER__)
        xspace_gesture_manage_enter_detection_mode();
#endif
        ibrt_event = IBRT_MGR_EV_WEAR_UP;
        wear_on_tick = xspace_interface_get_current_ms();
        xspace_ui_mobile_connect_voice_check_handle(0);
    }

#if defined(__XSPACE_BATTERY_MANAGER__)
    xspace_ui_battery_low_voice_report();
#endif

    if (xspace_interface_tws_link_connected()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_INEAR_STATUS_EVENT, NULL, 0);
    }

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
    xspace_ui_inear_status_changed_process();
#endif

    app_ibrt_if_event_entry(ibrt_event);
}

bool xspace_ui_inear_detect_need_execute(inear_detect_manager_status_e wear_status)
{

    return true;
}

bool xspace_ui_is_inear(void)
{
    return (xui_tws_ctx.local_sys_info.inear_status == XUI_INEAR_ON) ? true : false;
}

/**add by longz,20210613*/
bool xspace_anc_assist_is_outear(void)
{
    if ((xui_tws_ctx.local_sys_info.inear_status == XUI_INEAR_ON)
        && (xui_tws_ctx.peer_sys_info.inear_status == XUI_INEAR_ON)) {
        return true;
    }
    return false;
}
/**add by longz,20210613*/

bool xspace_ui_is_outear(void)
{
    if ((xui_tws_ctx.local_sys_info.inear_status == XUI_INEAR_OFF)
        || (xui_tws_ctx.peer_sys_info.inear_status == XUI_INEAR_OFF)) {
        return true;
    }
    return false;
}

void xspace_ui_tws_sync_inear_status_handle(xui_inear_status_e status)
{
    xui_tws_ctx.peer_sys_info.inear_status = status;
}
void xspace_ui_spp_sync_inear_status_handle(void)
{
    // TODO
}
#endif

#if defined(__XSPACE_INOUT_BOX_MANAGER__)
static void xspace_ui_inout_box_status_change_handle(xiob_status_e status)
{
    xiob_status_e inout_box_status = status;

#if defined(__XSPACE_XBUS_OTA__)
    xbus_ota_status_e ota_status = XBUS_OTA_STATUS_QTY;
    xspace_ui_xbus_ota_status_get(&ota_status);
    if (XBUS_OTA_STATUS_ENTER == ota_status) {
        return;
    }
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
    if (XCSM_BOX_STATUS_CLOSED == xspace_cover_switch_manage_get_status()) {
        inout_box_status = XIOB_IN_BOX;
    }
#endif

    if (xui_tws_ctx.sys_init_check_value != xui_tws_ctx.sys_curr_init_value) {
        xui_tws_ctx.sys_curr_init_value |= (1 << XUI_CHECK_ID_INOUT_BOX);
        XUI_TRACE(2, ">>> check_value:0x%02x, current init value: 0x%02x!",
              xui_tws_ctx.sys_init_check_value, xui_tws_ctx.sys_curr_init_value);

        if (xui_tws_ctx.sys_init_check_value == xui_tws_ctx.sys_curr_init_value) {
            xspace_ui_init_done();
        }
        return;
    }

    xspace_interface_delay_timer_stop((uint32_t)xspace_ui_inout_box_status_change_handle);
    if (status == XIOB_IN_BOX && xui_tws_ctx.local_sys_info.inear_status == XUI_INEAR_ON) {
        xspace_ui_inear_detect_manager_status_change(INEAR_DETECT_MG_DETACH_EAR);
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
        xspace_inear_detect_manager_stop();
#endif
        xspace_interface_delay_timer_start(800, (uint32_t)xspace_ui_inout_box_status_change_handle,
                                           (uint32_t)xspace_inout_box_manager_get_curr_status(), 0,
                                           0);
        return;
    }

    XUI_TRACE(2, " status:%d, current status: %d!", inout_box_status,
          xui_tws_ctx.local_sys_info.inout_box_status);
    if (xui_tws_ctx.local_sys_info.inout_box_status == (xui_inout_box_status_e)inout_box_status) {
        if ((INEAR_DETECT_MG_IN_EAR == xspace_inear_detect_manager_inear_status()
             || (false == xspace_inear_detect_manager_is_workon()))
            && (XIOB_OUT_BOX == inout_box_status)) {
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
            gh621xenablePowerOnOffset = false;
#endif
            xspace_inear_detect_manager_start();
#endif
        }
        return;
    }

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
    if (XUI_COVER_CLOSE == xui_tws_ctx.local_sys_info.cover_status && XIOB_OUT_BOX == status) {
        XUI_TRACE(0, " Cover closed, inout box manage invalid trigger out box!!!");
        return;
    }
#endif

    xspace_ui_boost_freq_when_busy();

    if (XIOB_IN_BOX == inout_box_status) {
        XUI_TRACE(0, " Box_IN!");
        xui_tws_ctx.local_sys_info.inout_box_status = XUI_BOX_IN_BOX;
        if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status) {
            app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_DOWN);
        }
        xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_UNKNOWN;

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
        xspace_inear_detect_manager_stop();
#endif
        app_ibrt_if_event_entry(IBRT_MGR_EV_DOCK);
    } else if (XIOB_OUT_BOX == inout_box_status) {
        XUI_TRACE(0, " BOX_OUT!");
        /**out of charge box*/
        XUI_TRACE(0, "cover_open -> out_box");
        xui_tws_ctx.local_sys_info.inout_box_status = XUI_BOX_OUT_BOX;
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
        if (xui_tws_ctx.local_sys_info.inear_status != XUI_INEAR_ON)
#endif
            xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_OFF;
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
        gh621xenablePowerOnOffset = true;
#endif
        xspace_inear_detect_manager_start();
#endif
        app_ibrt_if_event_entry(IBRT_MGR_EV_UNDOCK);
#if defined(__XSPACE_BATTERY_MANAGER__)
        hal_charger_set_box_dummy_load_switch(0x11);
#endif
#if defined(__XSPACE_BATTERY_MANAGER__)
        xspace_ui_battery_low_voice_report();
#endif
    }

#if defined(__XSPACE_GESTURE_MANAGER__)
    xspace_gesture_manage_enter_standby_mode();
#endif

    if (btif_besaud_is_connected()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_INOUT_BOX_EVENT, NULL, 0);
    }

    /**Comment out because of the new ui:in or out of box,no need to change the
   * playing state of music*/
#if 0   // defined(__XSPACE_INEAR_DETECT_MANAGER__)
    xspace_ui_inear_status_changed_process();
#endif
}

bool xspace_ui_is_inbox(void)
{
    return (xui_tws_ctx.local_sys_info.inout_box_status == XUI_BOX_IN_BOX) ? true : false;
}

void xspace_ui_tws_sync_inout_box_handle(xui_inout_box_status_e status)
{
    xui_tws_ctx.peer_sys_info.inout_box_status = status;

    if (XUI_BOX_OUT_BOX == status) {
        xui_tws_ctx.peer_sys_info.inear_status = XUI_INEAR_OFF;
    } else {
        xui_tws_ctx.peer_sys_info.inear_status = XUI_INEAR_UNKNOWN;
    }
}

// rssi strong and batter not low as master,
bool xspace_ui_tws_switch_according_rssi_needed(void)
{
    if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status
        && XUI_INEAR_ON == xui_tws_ctx.peer_sys_info.inear_status) {
        if (xui_tws_ctx.local_sys_info.inout_box_status == XUI_BOX_OUT_BOX
            && xui_tws_ctx.peer_sys_info.inout_box_status == XUI_BOX_OUT_BOX
            && xui_tws_ctx.local_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
            && xui_tws_ctx.peer_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
            && xui_tws_ctx.peer_bat_info.bat_per + 10
                >= xui_tws_ctx.local_bat_info.bat_per)   // if peer bat > local bat,
        {
            XUI_TRACE(1, " %s switch according rssi", __func__);
            return true;
        }
    }
    return false;
}
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
static uint8_t cover_close_cb_sdk = 0;
static osTimerId xui_cover_debounce_timer = NULL;
static void xspace_ui_cover_status_change_debounce_timeout_handler(void const *param);
osTimerDef(XUI_COVER_DEBOUNCE_TIMER, xspace_ui_cover_status_change_debounce_timeout_handler);

static void xspace_ui_cover_status_change_debounce_timeout_handler(void const *param)
{

    XUI_TRACE(1, "cover_status:%d(1:close 2:open)", xui_tws_ctx.local_sys_info.cover_status);
    if (XUI_COVER_OPEN == xui_tws_ctx.local_sys_info.cover_status) {
        app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
#if defined(__XSPACE_INOUT_BOX_MANAGER__)
        if (XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status
            && XIOB_OUT_BOX == xspace_inout_box_manager_get_curr_status()) {
            XUI_TRACE(0, " xio manager:out box, but ui in box!");
            xspace_ui_inout_box_status_change_handle(XIOB_OUT_BOX);
        }
#endif
    } else if (XUI_COVER_CLOSE == xui_tws_ctx.local_sys_info.cover_status) {
        cover_close_cb_sdk = 0;
        app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
        app_ibrt_conn_clear_freeman_enable();
    }
}

static void xspace_ui_cover_status_change_handle(xcsm_status_e status)
{

#if defined(__XSPACE_XBUS_OTA__)
    xbus_ota_status_e ota_status = XBUS_OTA_STATUS_QTY;
    xspace_ui_xbus_ota_status_get(&ota_status);
    if (XBUS_OTA_STATUS_ENTER == ota_status) {
        return;
    }
#endif

    if (xui_tws_ctx.sys_init_check_value != xui_tws_ctx.sys_curr_init_value) {

        xui_tws_ctx.sys_curr_init_value |= (1 << XUI_CHECK_ID_COVER_STATUS);

        XUI_TRACE(2, ">>> check_value:0x%02x, current init value: 0x%02x!",
              xui_tws_ctx.sys_init_check_value, xui_tws_ctx.sys_curr_init_value);

        if (xui_tws_ctx.sys_init_check_value == xui_tws_ctx.sys_curr_init_value) {
            xspace_ui_init_done();
        }
        return;
    }

    xspace_ui_boost_freq_when_busy();

    osTimerStop(xui_cover_debounce_timer);
    if (XCSM_BOX_STATUS_OPENED == status) {
        XUI_TRACE(0, " Cover_Open!");
#if defined(__XSPACE_BATTERY_MANAGER__)
        hal_charger_set_box_dummy_load_switch(0x41);
#endif
        xui_tws_ctx.local_sys_info.cover_status = XUI_COVER_OPEN;
        if (cover_close_cb_sdk == 1) {
            cover_close_cb_sdk = 0;
            app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
        }

        osTimerStart(xui_cover_debounce_timer, XUI_COVER_OPEN_DEBOUNCE_TIME);
    } else if (XCSM_BOX_STATUS_CLOSED == status) {
        XUI_TRACE(0, " Cover_Close!");
#if defined(__XSPACE_BATTERY_MANAGER__)
        hal_charger_set_box_dummy_load_switch(0x40);
#endif
        xui_tws_ctx.local_sys_info.cover_status = XUI_COVER_CLOSE;
        if (!xspace_interface_is_any_mobile_connected()) {
            cover_close_cb_sdk = 1;
        }

#ifdef GFPS_ENABLED   // Stop gfps adv when cover close.
        app_gfps_set_battery_datatype(HIDE_UI_INDICATION);
#endif
        osTimerStart(xui_cover_debounce_timer, XUI_COVER_CLOSE_DEBOUNCE_TIME);

        if (XUI_INEAR_ON == xui_tws_ctx.local_sys_info.inear_status) {
            XUI_TRACE(0, " current inear stauts!!! warning!!!");
            xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_OFF;
            app_ibrt_if_event_entry(IBRT_MGR_EV_WEAR_DOWN);
        }

        if (XUI_BOX_OUT_BOX == xui_tws_ctx.local_sys_info.inout_box_status) {
            XUI_TRACE(0, " current out box stauts!!! warning!!!");
            xui_tws_ctx.local_sys_info.inout_box_status = XUI_BOX_IN_BOX;
            xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_UNKNOWN;
            app_ibrt_if_event_entry(IBRT_MGR_EV_DOCK);
        }
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
        xspace_inear_off_set_self_cali();
#endif
#if defined(__XSPACE_BATTERY_MANAGER__)
        xspace_ui_battery_low_voice_report();
#endif
    }

#if defined(__XSPACE_GESTURE_MANAGER__)
    xspace_gesture_manage_enter_standby_mode();
#endif

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
    xspace_inear_detect_manager_stop();
#endif

    if (btif_besaud_is_connected()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_COVER_STATUS_EVENT, NULL, 0);
    }

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
    xspace_ui_inear_status_changed_process();
#endif
}

static bool xspace_ui_cover_status_ui_need_execute(xcsm_status_e status)
{
    return true;
}

bool xspace_ui_is_box_closed(void)
{
    return (xui_tws_ctx.local_sys_info.cover_status == XUI_COVER_CLOSE) ? true : false;
}

void xspace_ui_tws_sync_cover_status_handle(xui_cover_status_e status)
{
    xui_tws_ctx.peer_sys_info.cover_status = status;
}
#endif

#if defined(__XSPACE_BATTERY_MANAGER__)
#include "xspace_battery_manager.h"

uint8_t xspace_ui_get_local_bat_level(void)
{
    return xui_tws_ctx.local_bat_info.bat_level;
}

uint8_t xspace_ui_get_local_bat_percentage(void)
{
    return xui_tws_ctx.local_bat_info.bat_per;
}

static void xspace_ui_in_chargebox_shutdown(void)
{
    static uint32_t start_tick = 0xffffffff - 300;

    if (xui_tws_ctx.local_bat_info.isCharging == false
        && xui_tws_ctx.local_sys_info.cover_status == XUI_COVER_CLOSE
        && xui_tws_ctx.local_sys_info.inout_box_status == XUI_BOX_IN_BOX) {
        if (xui_tws_ctx.box_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
            && xui_tws_ctx.box_bat_info.bat_per > 20
            && xui_tws_ctx.local_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
            && xui_tws_ctx.local_bat_info.bat_per >= 98 && start_tick == 0xffffffff - 300) {
            start_tick = xspace_interface_get_current_sec();
            XUI_TRACE(2, "in box charge full shutdown,start calc time,%u,%u!", start_tick,
                  start_tick + 300);
        }
    } else {
        start_tick = 0xffffffff - 300;
    }

    if (xspace_interface_get_current_sec() > start_tick + 300) {
        XUI_TRACE(2, "in box charge full shutdown,over 5 min, now shutdown,%u,%u!",
              xspace_interface_get_current_sec(), start_tick + 300);
        xspace_interface_shutdown();
    }
}

void xspace_ui_battery_report_remote_device(void)
{

    XUI_TRACE(4, "0x%x,0x%x.%d/%d", xui_tws_ctx.local_bat_info.data_valid,
          xui_tws_ctx.peer_bat_info.data_valid, xspace_interface_tws_link_connected(),
          xspace_interface_is_any_mobile_connected());

#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
#if defined(IBRT)
    POSSIBLY_UNUSED uint8_t left_level = 0xFF;
    POSSIBLY_UNUSED uint8_t right_level = 0xFF;
    POSSIBLY_UNUSED uint8_t box_level = 0xFF;

    if (LEFT_SIDE == app_tws_get_earside()) {
        left_level = xui_tws_ctx.local_bat_info.bat_level;
    } else {
        right_level = xui_tws_ctx.local_bat_info.bat_level;
    }

    if ((XUI_TWS_BAT_DATA_VALID_FLAG == xui_tws_ctx.box_bat_info.data_valid)
        && (xui_tws_ctx.box_bat_info.bat_per > XUI_BOX_LOW_BATTERY)
        && ((XUI_TWS_BAT_DATA_VALID_FLAG == xui_tws_ctx.peer_bat_info.data_valid
             && XUI_BOX_IN_BOX == xui_tws_ctx.peer_sys_info.inout_box_status)
            || (XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status))) {
        box_level = xui_tws_ctx.box_bat_info.bat_level;
    } else if (XUI_TWS_BAT_DATA_VALID_FLAG == xui_tws_ctx.box_bat_info.data_valid
               && xui_tws_ctx.box_bat_info.bat_per == XUI_BOX_LOW_BATTERY) {
        box_level = 0xfe;
    }

    if (xspace_interface_tws_link_connected()) {
        if (XUI_TWS_BAT_DATA_VALID_FLAG == xui_tws_ctx.peer_bat_info.data_valid) {
            if (LEFT_SIDE == app_tws_get_earside()) {
                right_level = xui_tws_ctx.peer_bat_info.bat_level;
            } else {
                left_level = xui_tws_ctx.peer_bat_info.bat_level;
            }
        }
    }
    XUI_TRACE(3, "bat remote send ->%d,%d,%d", left_level, right_level, box_level);
    if (xspace_interface_is_any_mobile_connected()) {

        if (XUI_TWS_BAT_DATA_VALID_FLAG == xui_tws_ctx.peer_bat_info.data_valid
            && xui_tws_ctx.peer_bat_info.bat_level < xui_tws_ctx.local_bat_info.bat_level) {
            app_hfp_set_battery_level(xui_tws_ctx.peer_bat_info.bat_level);
        } else {
            app_hfp_set_battery_level(xui_tws_ctx.local_bat_info.bat_level);
        }
#else
    app_hfp_set_battery_level(xui_tws_ctx.local_bat_info.bat_level);
#endif
#else
    XUI_TRACE(0, " Can not enable SUPPORT_BATTERY_REPORT");
#endif
    }
}

static void xspace_ui_tws_bat_info_sync(void)
{
    static uint8_t bat_per = 100;
    static bool is_charging = false;

    XUI_TRACE(6, " Sync bat info, %d,%d/%d, 0x%x, %d/%d", xspace_interface_tws_link_connected(),
          bat_per / 5, xui_tws_ctx.local_bat_info.bat_level, xui_tws_ctx.local_bat_info.data_valid,
          is_charging, xui_tws_ctx.local_bat_info.isCharging);

    if (xui_tws_ctx.local_bat_info.bat_level > 9) {
        XUI_TRACE(1, "Invaild level:%d ", xui_tws_ctx.local_bat_info.bat_level);
        return;
    }

    if (xspace_interface_tws_link_connected()) {
        if (bat_per / 5 != xui_tws_ctx.local_bat_info.bat_per / 5
            || is_charging != xui_tws_ctx.local_bat_info.isCharging
            || XUI_TWS_BAT_DATA_VALID_FLAG != xui_tws_ctx.local_bat_info.data_valid
            || XUI_TWS_BAT_DATA_VALID_FLAG != xui_tws_ctx.peer_bat_info.data_valid) {
            xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_LOCAL_BAT_EVENT, NULL, 0);
            bat_per = xui_tws_ctx.local_bat_info.bat_per;
            is_charging = xui_tws_ctx.local_bat_info.isCharging;
        }
    } else {
        bat_per = 100;
        xui_tws_ctx.peer_bat_info.data_valid = 0;
        xui_tws_ctx.peer_bat_info.bat_per = 0;
        xui_tws_ctx.peer_bat_info.bat_level = 0;
        xui_tws_ctx.peer_bat_info.isCharging = 0;
    }
}

static void xspace_ui_battery_low_voice_report(void)
{
    static uint8_t low_power_report_flag = 0;
    static bool wear_on_first_flag = false;

    XUI_TRACE(3, " battery status:%d,charge status:%d,low_power_report_flag:%d!",
          xspace_get_battery_status(), xui_tws_ctx.local_bat_info.isCharging,
          low_power_report_flag);

    if (XBM_STATUS_UNDERVOLT != xspace_get_battery_status()
        || xui_tws_ctx.local_bat_info.isCharging == true) {
        low_power_report_flag = 0;
        wear_on_first_flag = false;
        return;
    }

    if (XUI_INEAR_ON != xui_tws_ctx.local_sys_info.inear_status) {
        low_power_report_flag = 0;
        wear_on_first_flag = false;
        XUI_TRACE(0, " Battery low, wear off,Please charging!");
        return;
    }

    if (xspace_interface_get_current_ms() < wear_on_tick + 9000) {
        if (wear_on_first_flag == false) {
            wear_on_first_flag = true;
            xspace_interface_delay_timer_stop((uint32_t)xspace_ui_battery_low_voice_report);
            xspace_interface_delay_timer_start(10000, (uint32_t)xspace_ui_battery_low_voice_report,
                                               0, 0, 0);
        }
        return;
    }

    XUI_TRACE(1, " Battery low,wear on,Please charging,%d!", low_power_report_flag);

    if (low_power_report_flag == 0) {
        low_power_report_flag = 1;
        if (!is_sco_mode() && !xspace_interface_call_is_online()
            && !xspace_interface_call_is_incoming()) {
#ifdef MEDIA_PLAYER_SUPPORT
            app_voice_report(APP_STATUS_INDICATION_CHARGENEED, 0);
#endif
        }
    }
}

static void xspace_ui_battery_handle(xbm_ui_status_s bat_status)
{
    static uint32_t pdvotl_start_tick = 0;
    bool pdvotl_flag = false;

    XUI_TRACE(2, " status:%d, bat_level:%d", bat_status.status, bat_status.curr_level);

    xui_tws_ctx.local_bat_info.bat_per = bat_status.curr_per;
    xui_tws_ctx.local_bat_info.bat_level = bat_status.curr_level;
    xui_tws_ctx.local_bat_info.isCharging = xspace_battery_manager_bat_is_charging();

    if (xui_tws_ctx.local_bat_info.bat_per > 0) {
        xui_tws_ctx.local_bat_info.data_valid = XUI_TWS_BAT_DATA_VALID_FLAG;
    }

    xspace_ui_tws_bat_info_sync();
    xspace_ui_battery_report_remote_device();

    if (xui_tws_ctx.local_bat_info.isCharging == false) {
        if (XBM_STATUS_UNDERVOLT == bat_status.status) {
            if (xspace_interface_get_current_ms() > wear_on_tick + 15000) {
                xspace_ui_battery_low_voice_report();
            }
        } else if (XBM_STATUS_PDVOLT == bat_status.status) {
            XUI_TRACE(0, " battery low, need Power OFF!");
            if (xspace_interface_tws_link_connected()) {
                if (xspace_interface_tws_is_master_mode()
                    && (pdvotl_start_tick == 0
                        || xspace_interface_get_current_sec() - pdvotl_start_tick < 30)) {
                    pdvotl_flag = true;
                    if (pdvotl_start_tick == 0) {
                        pdvotl_start_tick = xspace_interface_get_current_sec();
                    }
                }
            }

            if (xspace_interface_tws_is_slave_mode() || !xspace_interface_tws_link_connected()
                || xspace_interface_get_current_sec() - pdvotl_start_tick > 100) {
                pdvotl_start_tick = 0;
                XUI_TRACE(0, " Power OFF!");
                app_shutdown();
            }
        }
    }

    XUI_TRACE(1, " pdvotl_flag:%d.", pdvotl_flag);
#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
    xspace_low_battery_tws_switch(pdvotl_flag);
#endif
    xspace_ui_in_chargebox_shutdown();

    XUI_TRACE(4, "[XUI] Local_Bat: Data vaild:0x%x level:%d percentage:%d is charging:%d",
          xui_tws_ctx.local_bat_info.data_valid, xui_tws_ctx.local_bat_info.bat_level,
          xui_tws_ctx.local_bat_info.bat_per, xui_tws_ctx.local_bat_info.isCharging);
    XUI_TRACE(4, "[XUI] Peer__Bat: Data vaild:0x%x level:%d percentage:%d is charging:%d",
          xui_tws_ctx.peer_bat_info.data_valid, xui_tws_ctx.peer_bat_info.bat_level,
          xui_tws_ctx.peer_bat_info.bat_per, xui_tws_ctx.peer_bat_info.isCharging);
    XUI_TRACE(4, "[XUI] Box_Bat: Data vaild:0x%x level:%d percentage:%d is charging:%d",
          xui_tws_ctx.box_bat_info.data_valid, xui_tws_ctx.box_bat_info.bat_level,
          xui_tws_ctx.box_bat_info.bat_per, xui_tws_ctx.box_bat_info.isCharging);

    XUI_TRACE(3,
          "[XUI] Local_sys: wear:%d(2:on 1:off) inout box:%d(2:in 1:out) box "
          "status:%d(2:open 1:close)",
          xui_tws_ctx.local_sys_info.inear_status, xui_tws_ctx.local_sys_info.inout_box_status,
          xui_tws_ctx.local_sys_info.cover_status);
    XUI_TRACE(3,
          "[XUI] Peer__sys: wear:%d(2:on 1:off) inout box:%d(2:in 1:out) box "
          "status:%d(2:open 1:close)",
          xui_tws_ctx.peer_sys_info.inear_status, xui_tws_ctx.peer_sys_info.inout_box_status,
          xui_tws_ctx.peer_sys_info.cover_status);

    XUI_TRACE(4, "[XUI] Local_Ver: V%d.%d.%d.%d", xui_tws_ctx.local_sys_info.ver_info.sw_version[0],
          xui_tws_ctx.local_sys_info.ver_info.sw_version[1],
          xui_tws_ctx.local_sys_info.ver_info.sw_version[2],
          xui_tws_ctx.local_sys_info.ver_info.sw_version[3]);
    XUI_TRACE(4, "[XUI] Peer__Ver: V%d.%d.%d.%d", xui_tws_ctx.peer_sys_info.ver_info.sw_version[0],
          xui_tws_ctx.peer_sys_info.ver_info.sw_version[1],
          xui_tws_ctx.peer_sys_info.ver_info.sw_version[2],
          xui_tws_ctx.peer_sys_info.ver_info.sw_version[3]);
    XUI_TRACE(4, "[XUI] BOX Ver: V%d.%d.%d.%d", xui_tws_ctx.box_ver_info.sw_version[0],
          xui_tws_ctx.box_ver_info.sw_version[1], xui_tws_ctx.box_ver_info.sw_version[2],
          xui_tws_ctx.box_ver_info.sw_version[3]);
}

static void xspace_ui_battery_charger_ctrl_handle(xbm_charging_current_e *charging_curr,
                                                  uint16_t *charging_vol,
                                                  xbm_charging_ctrl_e *charging_ctrl)
{
    int16_t curr_temp = xspace_battery_manager_get_curr_temperature();
    int16_t curr_volt = xspace_battery_manager_bat_get_voltage();
    int16_t curr_percent = xspace_battery_manager_bat_get_percentage();

    if (NULL == charging_curr || NULL == charging_vol || NULL == charging_ctrl) {
        return;
    }

#if defined(__XSPACE_UI__)
    if (curr_temp < XUI_TEMPERATER_LOWEST || curr_temp > XUI_TEMPERATURE_HIGH) {
        *charging_ctrl = XBM_CHARGING_CTRL_DISABLE;
        *charging_curr = XBM_CHARGING_CTRL_UNCHARGING;
        *charging_vol = 4200;
    } else if (curr_volt <= 2000) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_ONE_FIFTH;
        *charging_vol = 4200;
    } else if (curr_volt > 2000 && curr_volt <= 3200) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_1C;
        *charging_vol = 4200;
    } else if ((curr_temp >= XUI_TEMPERATER_LOWEST) && (curr_temp < XUI_TEMPERATER_MEDIUM)) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_HALF;
        *charging_vol = 4200;
    } else if ((curr_temp >= XUI_TEMPERATER_MEDIUM) && (curr_temp <= XUI_TEMPERATURE_HIGH)) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        if ((90 <= curr_percent) && (4150 <= curr_volt)) {
            *charging_curr = XBM_CHARGING_CTRL_HALF;
        } else if ((80 <= curr_percent) && (4100 <= curr_volt)) {
            *charging_curr = XBM_CHARGING_CTRL_1C;
        } else {
            *charging_curr = XBM_CHARGING_CTRL_2C;
        }
        *charging_vol = 4200;
    }

#else
    if (curr_temp < XUI_TEMPERATER_LOWEST || curr_temp > XUI_TEMPERATURE_HIGH) {
        *charging_ctrl = XBM_CHARGING_CTRL_DISABLE;
        *charging_curr = XBM_CHARGING_CTRL_UNCHARGING;
        *charging_vol = 4200;
    } else if (curr_volt <= 2000) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_ONE_FIFTH;
        *charging_vol = 4400;
    } else if (curr_volt > 2000 && curr_volt <= 3200) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_1C;
        *charging_vol = 4400;
    } else if ((curr_temp >= XUI_TEMPERATER_LOWEST) && (curr_temp < XUI_TEMPERATER_MEDIUM)) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        *charging_curr = XBM_CHARGING_CTRL_HALF;
        *charging_vol = 4400;
    } else if ((curr_temp >= XUI_TEMPERATER_MEDIUM) && (curr_temp <= XUI_TEMPERATURE_HIGH)) {
        *charging_ctrl = XBM_CHARGING_CTRL_ENABLE;
        if ((90 <= curr_percent) && (4350 <= curr_volt)) {
            *charging_curr = XBM_CHARGING_CTRL_HALF;
        } else if ((80 <= curr_percent) && (4330 <= curr_volt)) {
            *charging_curr = XBM_CHARGING_CTRL_1C;
        } else {
            *charging_curr = XBM_CHARGING_CTRL_3C;
        }
        *charging_vol = 4400;
    }
#endif
}

void xspace_ui_battery_init(void)
{
    xbm_bat_ui_config_s bat_ui_cofig = {
        XUI_BAT_PD_PER_THRESHOLD,
        XUI_BAT_LOW_PER_THRESHOLD,
        XUI_BAT_HIGH_PER_THRESHOLD,
    };

    XUI_TRACE(0, " Xspace Battery Init!!!");

    hal_pmu_init();
    xspace_battery_manage_register_bat_info_ui_cb(xspace_ui_battery_handle);
    xspace_battery_manager_register_charger_ctrl_ui_cb(xspace_ui_battery_charger_ctrl_handle);
    xspace_battery_manager_init(bat_ui_cofig);
}
#endif

#if defined(__XSPACE_XBUS_MANAGER__)
#include "xspace_xbus_manager.h"

static void xspace_ui_xbus_sync_info(xbus_manage_event_e event_type, uint8_t *data, uint16_t len)
{
    struct nvrecord_env_t *nvrecord_env;

    nv_record_env_get(&nvrecord_env);

    XUI_TRACE(1, " event type:%d", event_type);
    switch (event_type) {
        case XBUS_MANAGE_SYNC_BOX_BAT_EVENT:
            if (data == NULL || len == 0)
                return;

            XUI_TRACE(2, " box bat per:%d, is charging:%d", (*data & 0x7F), (*data >> 7));
            xui_tws_ctx.box_bat_info.data_valid = XUI_TWS_BAT_DATA_VALID_FLAG;
            xui_tws_ctx.box_bat_info.bat_per = *data & 0x7F;
            xui_tws_ctx.box_bat_info.bat_level = (*data & 0x7F) / 10;
            if (xui_tws_ctx.box_bat_info.bat_level == 10) {
                xui_tws_ctx.box_bat_info.bat_level = 9;
            }
            if ((xui_tws_ctx.box_bat_info.bat_per < 5) && (xui_tws_ctx.box_bat_info.bat_per > 0)) {
                xui_tws_ctx.box_bat_info.bat_per = 1;
            }
            xui_tws_ctx.box_bat_info.isCharging = (bool)(*data >> 7);

            if (nvrecord_env->box_bat != *data) {
                nvrecord_env->box_bat = *data;
                nv_record_env_set(nvrecord_env);

                if (xspace_interface_tws_link_connected()
                    && XUI_BOX_OUT_BOX == xui_tws_ctx.peer_sys_info.inout_box_status) {
                    xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_BOX_BAT_INFO_EVENT, NULL, 0);
                }

#if defined(GFPS_ENABLED)
                app_fp_msg_send_battery_levels();
#endif
            }
            break;

        case XBUS_MANAGE_SYNC_BOX_VER_EVENT:
            if (data != NULL && len == 4) {
                memcpy((void *)xui_tws_ctx.box_ver_info.sw_version, (void *)data, len);
                XUI_TRACE(4, "[XUI] BOX Ver: V%d.%d.%d.%d", xui_tws_ctx.box_ver_info.sw_version[0],
                      xui_tws_ctx.box_ver_info.sw_version[1],
                      xui_tws_ctx.box_ver_info.sw_version[2],
                      xui_tws_ctx.box_ver_info.sw_version[3]);
                if (memcmp((void *)nvrecord_env->box_ver, (void *)data, len)) {
                    memcpy((void *)nvrecord_env->box_ver, (void *)data, len);
                    nv_record_env_set(nvrecord_env);
                    XUI_TRACE(0, " Need update box version");
                }
            }
            break;

        default:
            break;
    }
}
#endif

#if defined(__TWS_DISTANCE_SLAVE_REBOOT__)
static void xspace_ui_tws_distance_reboot(void)
{
    xspace_interface_reboot();
}
#endif

void xspace_ui_tws_sync_volume_info(uint8_t *addr, uint8_t vol)
{
    uint8_t sync_info[7] = {0};
    if (addr) {
        memcpy(sync_info, addr, 6);
        sync_info[6] = vol;
        XUI_TRACE(1, " SYNC VOL:%d", vol);
        DUMP8("%02x ", addr, 6);
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_SYNC_VOLUME_INFO, (uint8_t *)sync_info, 7);
    }
}

void xspace_ui_tws_recieve_volume_info(uint8_t *info, uint8_t len)
{
    bt_bdaddr_t bdaddr;
    uint8_t device_id = 0xFF;

    if (info && len == 7) {
        memcpy(bdaddr.address, info, 6);
        device_id = app_bt_get_device_id_byaddr(&bdaddr);
        XUI_TRACE(1, " SYNC VOL:%d, id:%d", info[6], device_id);
        DUMP8("%02x ", bdaddr.address, 6);
        if (device_id != 0xff) {
            a2dp_volume_local_set((enum BT_DEVICE_ID_T)device_id, info[6]);
            if (app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)device_id)) {
                app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, info[6]);
            }
        }
    }
}

void xspace_ui_tws_info_sync_recived_handle(uint16_t rsp_seq, uint8_t *data, uint16_t len)//从耳触发
{
    xui_tws_sync_info_s tws_sync_info_tx;
    xui_tws_sync_info_s tws_sync_info_rx;
    xui_tws_ctx_t xui_peer_tws_ctx;

    if (NULL == data || len < 2) {
        XUI_TRACE(1, " %s invaild parameter!!!", __func__);
        return;
    }

    memset((void *)&tws_sync_info_tx, 0x00, sizeof(tws_sync_info_tx));
    memset((void *)&tws_sync_info_rx, 0x00, sizeof(tws_sync_info_rx));
    memcpy((void *)&tws_sync_info_rx, (void *)data, len);
    xspace_ui_msg_buf_free(data);

    DUMP8("%02x ", (void *)&tws_sync_info_rx, tws_sync_info_rx.data_len + 3);

    switch (tws_sync_info_rx.event) {
        case XUI_SYS_INFO_LOCAL_BAT_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_LOCAL_BAT_EVENT");
            if (tws_sync_info_rx.data_len != sizeof(xui_bat_sync_info_s)) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_LOCAL_BAT_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            memcpy((void *)&xui_tws_ctx.peer_bat_info, (void *)tws_sync_info_rx.data,
                   sizeof(xui_bat_sync_info_s));
            if (tws_sync_info_rx.need_rsp) {
                tws_sync_info_tx.event = XUI_SYS_INFO_LOCAL_BAT_EVENT;
                tws_sync_info_tx.need_rsp = false;
                tws_sync_info_tx.data_len = sizeof(xui_bat_sync_info_s);
                memcpy((void *)tws_sync_info_tx.data, (void *)&xui_tws_ctx.local_bat_info,
                       sizeof(xui_bat_sync_info_s));
            }

            XUI_TRACE(4,
                  "[XUI] **Peer Bat: Data vaild:0x%x level:%d percentage:%d is "
                  "charging:%d",
                  xui_tws_ctx.peer_bat_info.data_valid, xui_tws_ctx.peer_bat_info.bat_level,
                  xui_tws_ctx.peer_bat_info.bat_per, xui_tws_ctx.peer_bat_info.isCharging);

#if defined(GFPS_ENABLED)
            app_fp_msg_send_battery_levels();
#endif
            break;

        case XUI_SYS_INFO_INEAR_STATUS_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_INEAR_STATUS_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_INEAR_STATUS_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            if (tws_sync_info_rx.need_rsp) {
                tws_sync_info_tx.event = XUI_SYS_INFO_INEAR_STATUS_EVENT;
                tws_sync_info_tx.need_rsp = false;
                tws_sync_info_tx.data_len = 1;
                tws_sync_info_tx.data[0] = xui_tws_ctx.local_sys_info.inear_status;
            }
            xspace_ui_mobile_connect_voice_check_handle(0);
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
            xspace_ui_tws_sync_inear_status_handle((xui_inear_status_e)tws_sync_info_rx.data[0]);
#endif
            break;

        case XUI_SYS_INFO_INOUT_BOX_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_INOUT_BOX_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_INOUT_BOX_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
#if defined(__XSPACE_INOUT_BOX_MANAGER__)
            xspace_ui_tws_sync_inout_box_handle((xui_inout_box_status_e)tws_sync_info_rx.data[0]);
#endif
            break;

        case XUI_SYS_INFO_COVER_STATUS_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_COVER_STATUS_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_COVER_STATUS_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
#if defined(__XSPACE_INOUT_BOX_MANAGER__)
            xspace_ui_tws_sync_cover_status_handle((xui_cover_status_e)tws_sync_info_rx.data[0]);
#endif
            break;

        case XUI_SYS_INFO_VERSION_INFO_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_VERSION_INFO_EVENT");
            XUI_TRACE(4, "[XUI] *Peer Ver: V%d.%d.%d.%d",
                  xui_tws_ctx.peer_sys_info.ver_info.sw_version[0],
                  xui_tws_ctx.peer_sys_info.ver_info.sw_version[1],
                  xui_tws_ctx.peer_sys_info.ver_info.sw_version[2],
                  xui_tws_ctx.peer_sys_info.ver_info.sw_version[3]);
            break;

        case XUI_SYS_INFO_BOX_BAT_INFO_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_BOX_BAT_INFO_EVENT");
            if (tws_sync_info_rx.data_len != sizeof(xui_bat_sync_info_s)) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_BOX_BAT_INFO_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            memcpy((void *)&xui_tws_ctx.box_bat_info, (void *)tws_sync_info_rx.data,
                   sizeof(xui_bat_sync_info_s));
            if (tws_sync_info_rx.need_rsp) {
                tws_sync_info_tx.event = XUI_SYS_INFO_BOX_BAT_INFO_EVENT;
                tws_sync_info_tx.need_rsp = false;
                tws_sync_info_tx.data_len = sizeof(xui_bat_sync_info_s);
                memcpy((void *)tws_sync_info_tx.data, (void *)&xui_tws_ctx.box_bat_info,
                       sizeof(xui_bat_sync_info_s));
            }

            XUI_TRACE(4,
                  "[XUI] **Box  Bat: Data vaild:0x%x level:%d percentage:%d is "
                  "charging:%d",
                  xui_tws_ctx.box_bat_info.data_valid, xui_tws_ctx.box_bat_info.bat_level,
                  xui_tws_ctx.box_bat_info.bat_per, xui_tws_ctx.box_bat_info.isCharging);

            break;

        case XUI_SYS_INFO_GESTURE_INFO_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_GESTURE_INFO_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_GESTURE_INFO_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);

#if defined(__XSPACE_GESTURE_MANAGER__)
            if (xspace_interface_tws_is_slave_mode()) {
                xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_GESTURE_INFO_EVENT,
                                                 &tws_sync_info_rx.data[0], 1);
            } else if (xspace_interface_tws_is_master_mode()) {
                xspace_ui_boost_freq_when_busy();
                xspace_gesture_tws_earphone_handle(
                    false, (xgm_event_type_e)tws_sync_info_rx.data[0], NULL, 0);
            }
#endif
            break;

#if defined(__XSPACE_UI__)
        case XUI_SYS_INFO_SYNC_LED_INFO:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_LED_INFO_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_GESTURE_INFO_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            app_status_indication_set((APP_STATUS_INDICATION_T)tws_sync_info_rx.data[0]);
            break;
#endif

        case XUI_SYS_INFO_LOW_LANTENCY_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_LOW_LANTENCY_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_LOW_LANTENCY_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            //Note(Mike):if need,plz implement
            //xspace_ui_set_low_latency_mode(tws_sync_info_rx.data[0]);
            break;

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        case XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0,
                      " Rx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT, "
                      "Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            xui_tws_ctx.wear_auto_play = tws_sync_info_rx.data[0];
            if (xui_tws_ctx.wear_auto_play == 1) {
                wear_auto_play_tick = xspace_interface_get_current_ms();
            }
            break;

        case XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0,
                      " Rx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT, "
                      "Invaild parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            {
                struct nvrecord_env_t *nvrecord_env;
                int ret = nv_record_env_get(&nvrecord_env);
                if (ret == 0) {
                    nvrecord_env->wear_auto_play_func_switch = (bool)tws_sync_info_rx.data[0];
                    nv_record_env_set(nvrecord_env);
                    nv_record_touch_cause_flush();
                }
            }
            break;
#endif

#if defined(USER_REBOOT_PLAY_MUSIC_AUTO)
        case XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT");
            if (tws_sync_info_rx.data_len != 2) {
                XUI_TRACE(0,
                      " Rx: XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT, Invaild "
                      "parameter");
                return;
            }
            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            a2dp_need_to_play = true;
            a2dp_handleKey(AVRCP_KEY_PLAY);
            break;
#endif

        case XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT");
            // TODO(Mike.Cheng):if using app config key_map
            break;

#if defined(__TWS_DISTANCE_SLAVE_REBOOT__)
        case XUI_SYS_INFO_SLAVE_REBOOT: {
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SLAVE_REBOOT");
            if (xspace_interface_tws_is_slave_mode()) {
                uint8_t stop_ibrt_buff[] = {IBRT_UI_STOP_IBRT, IBRT_UI_NO_ERROR};
                tws_ctrl_send_cmd(APP_TWS_CMD_STOP_IBRT, stop_ibrt_buff, sizeof(stop_ibrt_buff));

                xspace_interface_delay_timer_stop((uint32_t)xspace_ui_tws_distance_reboot);
                xspace_interface_delay_timer_start(500, (uint32_t)xspace_ui_tws_distance_reboot, 0,
                                                   0, 0);
            }
        } break;
#endif

        case XUI_SYS_INFO_RECONNECT_SECOND_MOBILE:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_RECONNECT_SECOND_MOBILE");
            break;

        case XUI_SYS_INFO_SYNC_SHUTDOWN:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_SHUTDOWN");
            if (xspace_interface_tws_is_slave_mode()) {
                XUI_TRACE(0, " Power OFF!");
                app_shutdown();
            }
            break;

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        case XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0,
                      " Rx: XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH, Invaild "
                      "parameter");
                return;
            }
            {
                struct nvrecord_env_t *nvrecord_env;
                if (nv_record_env_get(&nvrecord_env) == 0) {
                    nvrecord_env->wear_auto_play_func_switch = (bool)tws_sync_info_rx.data[0];
                    xui_tws_ctx.wear_auto_play_func_switch_flag =
                        nvrecord_env->wear_auto_play_func_switch;
                    nv_record_env_set(nvrecord_env);
                }
            }
            break;
#endif

#if defined(__XSPACE_CUSTOMIZE_ANC__)
        case XUI_SYS_INFO_SYNC_ANC_INFO:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_ANC_INFO");
            if (tws_sync_info_rx.data_len != 1) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_ANC_INFO, Invaild parameter");
                return;
            }

            xspace_ui_boost_freq_when_busy();
            //TODO(Mike):if need,plz implement it
            xui_tws_ctx.anc_mode_index = tws_sync_info_rx.data[0];
#endif

#if defined(__XSPACE_RSSI_TWS_SWITCH__)
        case XUI_SYS_INFO_SYNC_RSSI_INFO:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_RSSI_INFO");
            xspace_rssi_process_sync_info(tws_sync_info_rx.data, tws_sync_info_rx.data_len);
            if (tws_sync_info_rx.need_rsp) {
                tws_sync_info_tx.need_rsp = false;
                tws_sync_info_tx.event = XUI_SYS_INFO_SYNC_RSSI_INFO;
                xspace_rssi_process_send_sync_info(tws_sync_info_tx.data,
                                                   &tws_sync_info_tx.data_len);
                XUI_TRACE(1, " Rx: XUI_SYS_INFO_SYNC_RSSI_INFO, %d", tws_sync_info_tx.data_len);
                DUMP8("%02x ", tws_sync_info_tx.data, tws_sync_info_tx.data_len);
            }

            break;
#endif

#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
        case XUI_SYS_INFO_SYNC_SNR_WNR_INFO:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_SNR_WNR_INFO");
            xspace_snr_wnr_tws_recv_sync_info(tws_sync_info_rx.data, tws_sync_info_rx.data_len);
            break;
#endif

        case XUI_SYS_INFO_SYNC_PAIRING_STATUS:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_PAIRING_STATUS");
            if (xspace_interface_tws_is_slave_mode()) {
                // xspace_interface_device_set_tws_pairing_flag((bool)tws_sync_info_rx.data[0]);
            }
            break;

        case XUI_SYS_INFO_SYNC_VOLUME_INFO:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_SYNC_VOLUME_INFO");
            xspace_ui_tws_recieve_volume_info(tws_sync_info_rx.data, tws_sync_info_rx.data_len);
            break;

        case XUI_SYS_INFO_PEER_RUN_FUNC: {
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_PEER_RUN_FUNC");
            if (tws_sync_info_rx.data_len != 20) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_PEER_RUN_FUNC, Invaild parameter");
                return;
            }

            xui_let_peer_run_cmd_config_s peer_run_cmd_info = {0};
            memcpy(&peer_run_cmd_info, tws_sync_info_rx.data, 20);
            uint32_t *ptr = NULL;
            XUI_TRACE(1, " Rx: func_id:%d", peer_run_cmd_info.cmd_id);
            switch (peer_run_cmd_info.cmd_id) {
                case XUI_MOBILE_CONNECT_VOICE_PROMPT_STATUS_CHECK:
                    ptr = (uint32_t *)xspace_ui_mobile_connect_voice_check_handle;
                    break;
                case XUI_ANC_MODE_SWITCH:
#if defined(__XSPACE_CUSTOMIZE_ANC__)
                    //Note(Mike.Cheng):TODO
                    //ptr = (uint32_t *)xspace_ui_set_anc_mode;
#endif
                    break;
                case XUI_HANDLE_A2DP_KEY:
                    ptr = (uint32_t *)a2dp_handleKey;
                    break;
                case XUI_SET_CONECT_VOICE_PROMPT_STATUS:
                    ptr = (uint32_t *)set_mobile_connect_voice_status;
                    break;
                case XUI_SET_SINGLE_WEAR_ANC_MODE_SWITCH_INFO:
                    break;
#if defined(__XSPACE_CUSTOMIZE_ANC__)
//Note(Mike.Cheng):TODO
                case XUI_UPDATE_ANC_MODE_STATUS:
                    //ptr = (uint32_t *)update_anc_mode 
                    break;
                case XUI_APP_ANC_STATUS_SYNC:
                    //ptr = (uint32_t *)app_anc_sync_status;
                    break;
#endif
                default:
                    XUI_TRACE(1, " unknow Rx: c_id:%d", peer_run_cmd_info.cmd_id);
                    break;
            }

            if ((void *)ptr != NULL) {
                if (peer_run_cmd_info.delay_time == 0) {
                    ((xif_process_func_cb)(ptr))(peer_run_cmd_info.id, peer_run_cmd_info.param0, peer_run_cmd_info.param1);
                } else {
                    xspace_interface_delay_timer_stop((uint32_t)ptr);
                    xspace_interface_delay_timer_start(peer_run_cmd_info.delay_time, (uint32_t)ptr, peer_run_cmd_info.id, peer_run_cmd_info.param0, peer_run_cmd_info.param1);
                }
            }
        } break;

        case XUI_SYS_INFO_TOTAL_INFO_EVENT:
            XUI_TRACE(0, " Rx: XUI_SYS_INFO_TOTAL_INFO_EVENT");
            if (tws_sync_info_rx.data_len != sizeof(xui_tws_ctx_t)) {
                XUI_TRACE(0, " Rx: XUI_SYS_INFO_LOCAL_BAT_EVENT, Invaild parameter");
                return;
            }
            XUI_TRACE(0, "xspace_interface_is_any_mobile_connected:%d",
                  xspace_interface_is_any_mobile_connected());

            memcpy((void *)&xui_peer_tws_ctx, (void *)tws_sync_info_rx.data, sizeof(xui_tws_ctx_t));
            memcpy((void *)&xui_tws_ctx.peer_bat_info, (void *)&xui_peer_tws_ctx.local_bat_info,
                   sizeof(xui_bat_sync_info_s));
            memcpy((void *)&xui_tws_ctx.peer_sys_info, (void *)&xui_peer_tws_ctx.local_sys_info,
                   sizeof(xui_sys_info_s));

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
            if (xspace_interface_tws_is_slave_mode()) {
#if defined(__XSPACE_CUSTOMIZE_ANC__)
                xui_tws_ctx.anc_mode_index = xui_peer_tws_ctx.anc_mode_index;
                //TODO(Mike):if need plz implement it
#endif
                xui_tws_ctx.wear_auto_play_func_switch_flag = xui_peer_tws_ctx.wear_auto_play_func_switch_flag;
                struct nvrecord_env_t *nvrecord_env;
                if (nv_record_env_get(&nvrecord_env) == 0) {
                    XUI_TRACE(2,
                          "local wear_auto_play_func_switch:%d, peer "
                          "wear_auto_play_func_switch:%d",
                          nvrecord_env->wear_auto_play_func_switch,
                          xui_tws_ctx.wear_auto_play_func_switch_flag);
                    nvrecord_env->wear_auto_play_func_switch =
                        xui_tws_ctx.wear_auto_play_func_switch_flag;
                    nv_record_env_set(nvrecord_env);
                }
            }
#endif

            XUI_TRACE(1, " Rx: rsp:%d", tws_sync_info_rx.need_rsp);
            if (tws_sync_info_rx.need_rsp) {
                tws_sync_info_tx.event = XUI_SYS_INFO_TOTAL_INFO_EVENT;
                tws_sync_info_tx.need_rsp = false;
                tws_sync_info_tx.data_len = sizeof(xui_tws_ctx_t);
                memcpy((void *)tws_sync_info_tx.data, (void *)&xui_tws_ctx, sizeof(xui_tws_ctx_t));
            }

#if defined(GFPS_ENABLED)
            app_fp_msg_send_battery_levels();
#endif
            break;

        default:
            XUI_TRACE(1, " Rx: Invaild id:%d!!!", tws_sync_info_rx.event);
            break;
    }

    if (tws_sync_info_rx.need_rsp) {
        xspace_ui_send_info_sync_cmd_if((uint8_t *)&tws_sync_info_tx,
                                        tws_sync_info_tx.data_len + 3);
    }

    if (XUI_SYS_INFO_INEAR_STATUS_EVENT == tws_sync_info_rx.event
        || XUI_SYS_INFO_INOUT_BOX_EVENT == tws_sync_info_rx.event
        || XUI_SYS_INFO_COVER_STATUS_EVENT == tws_sync_info_rx.event
        || XUI_SYS_INFO_TOTAL_INFO_EVENT == tws_sync_info_rx.event) {
        XUI_TRACE(3,
              "[XUI]Peer_sys:wear:%d(2:on 1:off) inout box:%d(2:in 1:out) box "
              "status:%d(2:open 1:close)",
              xui_tws_ctx.peer_sys_info.inear_status, xui_tws_ctx.peer_sys_info.inout_box_status,
              xui_tws_ctx.peer_sys_info.inout_box_status);

        xspace_ui_boost_freq_when_busy();

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
        /**modified because of the new ui:in or out of box,no need to change the
     * playing state of music*/
        if (XUI_SYS_INFO_INOUT_BOX_EVENT != tws_sync_info_rx.event) {
            xspace_ui_inear_status_changed_process();
        }
#endif
    }
}

void xspace_ui_tws_send_info_sync_cmd(xui_sys_info_sync_event_e id, uint8_t *data, uint16_t len)
{
    xui_tws_sync_info_s tws_sync_info;

    if (!xspace_interface_tws_link_connected()) {
        XUI_TRACE(0, " TWS info exchange channel is not connected!!!");
        return;
    }

    memset((void *)&tws_sync_info, 0x00, sizeof(tws_sync_info));
    XUI_TRACE(1, " Sync tws ui status,id:%d", id);

    switch (id) {
        case XUI_SYS_INFO_LOCAL_BAT_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_LOCAL_BAT_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_LOCAL_BAT_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = sizeof(xui_bat_sync_info_s);
            memcpy((void *)tws_sync_info.data, (void *)&xui_tws_ctx.local_bat_info,
                   sizeof(xui_bat_sync_info_s));
            break;

        case XUI_SYS_INFO_INEAR_STATUS_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_INEAR_STATUS_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_INEAR_STATUS_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 1;
            tws_sync_info.data[0] = xui_tws_ctx.local_sys_info.inear_status;
            break;

        case XUI_SYS_INFO_INOUT_BOX_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_INOUT_BOX_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_INOUT_BOX_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 1;
            tws_sync_info.data[0] = xui_tws_ctx.local_sys_info.inout_box_status;
            break;

        case XUI_SYS_INFO_COVER_STATUS_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_COVER_STATUS_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_COVER_STATUS_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 1;
            tws_sync_info.data[0] = xui_tws_ctx.local_sys_info.cover_status;
            break;

        case XUI_SYS_INFO_VERSION_INFO_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_VERSION_INFO_EVENT");
            break;

        case XUI_SYS_INFO_BOX_BAT_INFO_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_BOX_BAT_INFO_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_BOX_BAT_INFO_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = sizeof(xui_bat_sync_info_s);
            memcpy((void *)tws_sync_info.data, (void *)&xui_tws_ctx.box_bat_info,
                   sizeof(xui_bat_sync_info_s));
            break;

        case XUI_SYS_INFO_GESTURE_INFO_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_GESTURE_INFO_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_GESTURE_INFO_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#if defined(__XSPACE_UI__)
        case XUI_SYS_INFO_SYNC_LED_INFO:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_LED_INFO_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_LED_INFO;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

        case XUI_SYS_INFO_LOW_LANTENCY_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_LOW_LANTENCY_EVENT");
            // Note(Mike.Cheng):if need todo
            break;

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        case XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;

        case XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

#if defined(USER_REBOOT_PLAY_MUSIC_AUTO)
        case XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 0;
            break;
#endif

        case XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT");
            tws_sync_info.event = XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;

#if defined(__TWS_DISTANCE_SLAVE_REBOOT__)
        case XUI_SYS_INFO_SLAVE_REBOOT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SLAVE_REBOOT");
            tws_sync_info.event = XUI_SYS_INFO_SLAVE_REBOOT;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 0;
            break;
#endif

        case XUI_SYS_INFO_RECONNECT_SECOND_MOBILE:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_RECONNECT_SECOND_MOBILE");
            tws_sync_info.event = XUI_SYS_INFO_RECONNECT_SECOND_MOBILE;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 0;
            break;

        case XUI_SYS_INFO_SYNC_SHUTDOWN:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_SHUTDOWN");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_SHUTDOWN;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = 0;
            break;

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
        case XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

#if defined(__XSPACE_CUSTOMIZE_ANC__)
        case XUI_SYS_INFO_SYNC_ANC_INFO:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_ANC_INFO");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_ANC_INFO;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

#if defined(__XSPACE_RSSI_TWS_SWITCH__)
        case XUI_SYS_INFO_SYNC_RSSI_INFO:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_RSSI_INFO");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_RSSI_INFO;
            tws_sync_info.need_rsp = true;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
        case XUI_SYS_INFO_SYNC_SNR_WNR_INFO:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_SNR_WNR_INFO");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_SNR_WNR_INFO;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;
#endif

        case XUI_SYS_INFO_SYNC_PAIRING_STATUS:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_PAIRING_STATUS");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_PAIRING_STATUS;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;

        case XUI_SYS_INFO_SYNC_VOLUME_INFO:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_SYNC_VOLUME_INFO");
            tws_sync_info.event = XUI_SYS_INFO_SYNC_VOLUME_INFO;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;

        case XUI_SYS_INFO_PEER_RUN_FUNC:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_PEER_RUN_FUNC");
            tws_sync_info.event = XUI_SYS_INFO_PEER_RUN_FUNC;
            tws_sync_info.need_rsp = false;
            tws_sync_info.data_len = len;
            memcpy((void *)tws_sync_info.data, (void *)data, len);
            break;

        case XUI_SYS_INFO_TOTAL_INFO_EVENT:
            XUI_TRACE(0, " Tx: XUI_SYS_INFO_TOTAL_INFO_EVENT");
            XUI_TRACE(0, "xspace_interface_is_any_mobile_connected:%d",
                  xspace_interface_is_any_mobile_connected());
            tws_sync_info.event = XUI_SYS_INFO_TOTAL_INFO_EVENT;
            tws_sync_info.need_rsp = true;
            tws_sync_info.data_len = sizeof(xui_tws_ctx_t);
#if defined(__XSPACE_CUSTOMIZE_ANC__)
            xui_tws_ctx.anc_mode_index = cur_anc_mode_index;
#endif
            memcpy((void *)tws_sync_info.data, (void *)&xui_tws_ctx, sizeof(xui_tws_ctx_t));
            break;

        default:
            XUI_TRACE(1, " Invaild id:%d!!!", id);
            break;
    }

    xspace_ui_send_info_sync_cmd_if((uint8_t *)&tws_sync_info, tws_sync_info.data_len + 3);
}

static void xspace_ui_wait_ibrt_stack_init_finish(void)
{
    XUI_TRACE(1, " current ibrt stack init result:%d", app_get_ibrt_stack_init_result());
    if (app_get_ibrt_stack_init_result() && (xui_sys_status != XUI_SYS_FORCE_TWS_PARING)
        && (xui_sys_status != XUI_SYS_FORCE_FREEMAN_PARING)) {
        if (XUI_COVER_OPEN == xui_tws_ctx.local_sys_info.cover_status) {
            app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
            if (XUI_BOX_OUT_BOX == xui_tws_ctx.local_sys_info.inout_box_status) {
                app_ibrt_if_event_entry(IBRT_MGR_EV_UNDOCK);
            }
        }
    } else {
        xspace_interface_delay_timer_start(500, (uint32_t)xspace_ui_wait_ibrt_stack_init_finish, 0,
                                           0, 0);
    }
}
static void xspace_ui_init_done(void)
{
    XUI_TRACE(0, ">>> UI Init done");
    XUI_TRACE(1, " xui_sys_status:%d!", xui_sys_status);
    if (XCSM_BOX_STATUS_CLOSED == xspace_cover_switch_manage_get_status()) {
        /**in charge box*/
        XUI_TRACE(0, " In_Box!");
        xui_tws_ctx.local_sys_info.inout_box_status = XUI_BOX_IN_BOX;
        xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_UNKNOWN;

        /**box close*/
        XUI_TRACE(0, " Cover_Close!");
        xui_tws_ctx.local_sys_info.cover_status = XUI_COVER_CLOSE;

#if defined(__XSPACE_BATTERY_MANAGER__)
        hal_charger_set_box_dummy_load_switch(0x40);
#endif

#if defined(__XSPACE_GESTURE_MANAGER__)
        xspace_gesture_manage_enter_standby_mode();
#endif

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
        xspace_inear_detect_manager_stop();
#endif

        XUI_TRACE(1, " xui_sys_status:%d!", xui_sys_status);
        if (xui_sys_status == XUI_SYS_FORCE_TWS_PARING
            || xui_sys_status == XUI_SYS_FORCE_FREEMAN_PARING) {
            app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_CLOSE);
        }
    } else {
        XUI_TRACE(0, " Cover_Open!");
        xui_tws_ctx.local_sys_info.cover_status = XUI_COVER_OPEN;
#if defined(__XSPACE_BATTERY_MANAGER__)
        hal_charger_set_box_dummy_load_switch(0x41);
#endif

        if (app_get_ibrt_stack_init_result() && xui_sys_status != XUI_SYS_FORCE_TWS_PARING
            && xui_sys_status != XUI_SYS_FORCE_FREEMAN_PARING) {
            app_ibrt_if_event_entry(IBRT_MGR_EV_CASE_OPEN);
        } else {
            XUI_TRACE(0, " ibrt stack isn't init finish!!!");
            xspace_interface_delay_timer_start(500, (uint32_t)xspace_ui_wait_ibrt_stack_init_finish,
                                               0, 0, 0);
        }

        xui_tws_ctx.local_sys_info.inout_box_status =
            (xui_inout_box_status_e)xspace_inout_box_manager_get_curr_status();

        if (XUI_BOX_IN_BOX == xui_tws_ctx.local_sys_info.inout_box_status) {
            /**in charge box*/
            XUI_TRACE(0, " Box_IN!");
            xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_UNKNOWN;
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
            xspace_inear_detect_manager_stop();
#endif
        } else {
            XUI_TRACE(0, " BOX_OUT!");
            /**out of charge box*/
            xui_tws_ctx.local_sys_info.inout_box_status = XUI_BOX_OUT_BOX;
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
            if (xui_tws_ctx.local_sys_info.inear_status != XUI_INEAR_ON)
#endif
                xui_tws_ctx.local_sys_info.inear_status = XUI_INEAR_OFF;
            if (app_get_ibrt_stack_init_result() && xui_sys_status != XUI_SYS_FORCE_TWS_PARING
                && xui_sys_status != XUI_SYS_FORCE_FREEMAN_PARING) {
                app_ibrt_if_event_entry(IBRT_MGR_EV_UNDOCK);
            } else {
                XUI_TRACE(0, " ibrt stack isn't init finish!!!");
                xspace_interface_delay_timer_start(
                    500, (uint32_t)xspace_ui_wait_ibrt_stack_init_finish, 0, 0, 0);
            }
#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
            xspace_inear_detect_manager_start();
#endif
        }

        if (xui_sys_status == XUI_SYS_FORCE_TWS_PARING) {
            xspace_interface_tws_pairing_enter();
        } else if (xui_sys_status == XUI_SYS_FORCE_FREEMAN_PARING
                   && app_get_ibrt_stack_init_result()) {
            app_ibrt_if_enter_freeman_pairing();
        }
    }

    if (xspace_interface_tws_link_connected()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_TOTAL_INFO_EVENT, NULL, 0);
    }

#if defined(__XSPACE_PRODUCT_TEST__)
    if (hal_sw_bootmode_get() & HAL_SW_REBOOT_ENTER_PRODUCT_MODE) {
        if (xui_tws_ctx.local_sys_info.cover_status == XUI_COVER_CLOSE
            && xui_sys_status == XUI_SYS_FORCE_FREEMAN_PARING) {
            if (app_get_ibrt_stack_init_result()) {
                app_ibrt_if_enter_freeman_pairing();
            } else {
                xspace_interface_delay_timer_start(
                    1000, (uint32_t)app_ibrt_if_enter_freeman_pairing, 0, 0, 0);
            }
        }
        hal_sw_bootmode_clear(HAL_SW_REBOOT_ENTER_PRODUCT_MODE);
        xui_sys_status = XUI_SYS_PRODUCT_TEST;
        xspace_interface_delay_timer_stop((uint32_t)xspace_interface_shutdown);
        xspace_interface_delay_timer_start(60000 * 20, (uint32_t)xspace_interface_shutdown, 0, 0,
                                           0);
    } else
#endif
    {
        if (xui_sys_status != XUI_SYS_FORCE_TWS_PARING
            && xui_sys_status != XUI_SYS_FORCE_FREEMAN_PARING) {
            xui_sys_status = XUI_SYS_NORMAL;
        }
    }
}
static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2c[] = {
        {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
};
void xspace_ui_env_get(xui_tws_ctx_t **p_ui_env)
{
    *p_ui_env = &xui_tws_ctx;
}

void xspace_ui_init(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    XUI_TRACE(0, " Init xspace ui ");
    nv_record_print_all();

    if (xui_sys_status != XUI_SYS_FORCE_TWS_PARING
        && xui_sys_status != XUI_SYS_FORCE_FREEMAN_PARING) {
        xui_sys_status = XUI_SYS_INIT;
    }

    xspace_interface_register_write_flash_befor_shutdown_cb(
        WRITE_UI_INFO_TO_FLASH, xspace_interface_update_info_when_shutdown_or_reboot);

#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
    xui_inear_status_e inear_status_temp = XUI_INEAR_UNKNOWN;
    if (xui_tws_ctx.local_sys_info.inear_status == XUI_INEAR_ON)
        inear_status_temp = xui_tws_ctx.local_sys_info.inear_status;
#endif
    memset((void *)&xui_tws_ctx, 0x00, sizeof(xui_tws_ctx_t));
#if defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
    if (inear_status_temp == XUI_INEAR_ON)
        xui_tws_ctx.local_sys_info.inear_status = inear_status_temp;
#endif
    for (uint8_t i = 0; i < XUI_CHECK_ID_TOTAL_NUM; i++) {
        xui_tws_ctx.sys_init_check_value |= (1 << i);
    }
    XUI_TRACE(1, " system init check value: 0x%02x!", xui_tws_ctx.sys_init_check_value);

    xui_tws_ctx.local_sys_info.ver_info.data_valid = XUI_TWS_VER_DATA_VALID_FLAG;
    memcpy((void *)xui_tws_ctx.local_sys_info.ver_info.sw_version, (void *)xui_dev_sw_version, 4);
    memcpy((void *)xui_tws_ctx.local_sys_info.ver_info.hw_version, (void *)xui_dev_hw_version, 2);
    memcpy((void *)xui_tws_ctx.local_sys_info.ver_info.ota_version, (void *)xui_dev_sw_version, 4);
    nv_record_all_ddbrec_print();

    if ((nvrecord_env->local_bat & 0x7F) <= 100) {
        xui_tws_ctx.local_bat_info.data_valid = XUI_TWS_BAT_DATA_VALID_FLAG;
        xui_tws_ctx.local_bat_info.bat_per = nvrecord_env->local_bat;
        xui_tws_ctx.local_bat_info.bat_level = nvrecord_env->local_bat / 10;
        if (xui_tws_ctx.local_bat_info.bat_level > 9) {
            xui_tws_ctx.local_bat_info.bat_level = 9;
        }
    }

    if ((nvrecord_env->peer_bat & 0x7F) <= 100) {
        // xui_tws_ctx.peer_bat_info.data_valid = XUI_TWS_BAT_DATA_VALID_FLAG;
        xui_tws_ctx.peer_bat_info.bat_per = nvrecord_env->peer_bat;
        xui_tws_ctx.peer_bat_info.bat_level = nvrecord_env->peer_bat / 10;
        if (xui_tws_ctx.peer_bat_info.bat_level > 9) {
            xui_tws_ctx.peer_bat_info.bat_level = 9;
        }
    }

    if ((nvrecord_env->box_bat & 0x7F) <= 100 && (nvrecord_env->box_bat & 0x7F) != 0) {
        xui_tws_ctx.box_bat_info.data_valid = XUI_TWS_BAT_DATA_VALID_FLAG;
        xui_tws_ctx.box_bat_info.bat_per = nvrecord_env->box_bat & 0x7F;
        xui_tws_ctx.box_bat_info.bat_level = (nvrecord_env->box_bat & 0x7F) / 10;
        if (xui_tws_ctx.box_bat_info.bat_level == 10) {
            xui_tws_ctx.box_bat_info.bat_level = 9;
        }
        xui_tws_ctx.box_bat_info.isCharging = (bool)(nvrecord_env->box_bat >> 7);
    }

    XUI_TRACE(2, " nv restore bat info: %d(local)/%d(peer)/%d(box)!", nvrecord_env->local_bat,
          nvrecord_env->peer_bat, nvrecord_env->box_bat);


#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    xui_tws_ctx.wear_auto_play_func_switch_flag = nvrecord_env->wear_auto_play_func_switch;
#endif

#if defined(__XSPACE_BATTERY_MANAGER__)
    xspace_battery_manager_timing_todo();
#endif

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
#if !defined(PO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX)
    xspace_inear_detect_manager_register_ui_cb(xspace_ui_inear_detect_manager_status_change);
    xspace_inear_detect_manager_register_ui_need_execute_cb(xspace_ui_inear_detect_need_execute);
    xspace_inear_detect_manager_init();
#endif
#endif

#if defined(__XSPACE_GESTURE_MANAGER__)
    xspace_gesture_manage_register_ui_cb(xspace_gesture_status_change_handle);
    xspace_gesture_manage_register_manage_need_execute_cb(xspace_ui_gesture_manage_need_execute);
    xspace_gesture_manage_init();
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
    xui_cover_debounce_timer = osTimerCreate(osTimer(XUI_COVER_DEBOUNCE_TIMER), osTimerOnce, NULL);
    xspace_cover_switch_manage_register_ui_cb(xspace_ui_cover_status_change_handle);
    xspace_cover_switch_manage_register_manage_need_execute_cb(xspace_ui_cover_status_ui_need_execute);
    xspace_cover_switch_manager_init();
#endif

#if defined(__XSPACE_INOUT_BOX_MANAGER__)
    xspace_inout_box_manager_register_ui_cb(xspace_ui_inout_box_status_change_handle);
    xspace_inout_box_manage_init();
#endif

#if defined(__XSPACE_XBUS_MANAGER__)
    xspace_xbus_manage_init();
    xbus_manage_register_ui_event_cb(xspace_ui_xbus_sync_info);
#endif

#if defined(__XSPACE_XBUS_OTA__)
    xbus_manage_ota_register_cb(xspace_ui_xbus_ota_enter);
#endif

#if defined(__XSPACE_PRODUCT_TEST__)
    xspace_pt_init();
#endif

#if defined(__XSPACE_TWS_SWITCH__)
    xspace_tws_switch_process_init();
#endif

#if defined (__XSPACE_IMU_MANAGER__)
    xspace_imu_manager_init();
#endif
}