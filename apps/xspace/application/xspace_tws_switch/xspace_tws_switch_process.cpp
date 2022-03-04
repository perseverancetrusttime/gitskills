#if defined(__XSPACE_UI__) && defined(__XSPACE_TWS_SWITCH__)
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
#include "xspace_ui.h"
#include "xspace_tws_switch_process.h"
#if defined(__XSPACE_RSSI_TWS_SWITCH__)
#include "xspace_rssi_tws_switch.h"
#endif
#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
#include "xspace_low_battery_tws_switch.h"
#endif
#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
#include "xspace_snr_wnr_tws_switch.h"
#endif

static uint16_t time_cycle = XTS_MONITOR_LONG_TIMEOUT;


static bool xspace_tws_swtich_ui_status_match(void)
{
    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);

    if (xspace_interface_tws_is_master_mode() && xspace_interface_tws_link_connected()) {
        if ((get_device_env_info->local_sys_info.cover_status == get_device_env_info->peer_sys_info.cover_status)
            && (get_device_env_info->local_sys_info.inout_box_status == get_device_env_info->peer_sys_info.inout_box_status)
            && (get_device_env_info->local_sys_info.inear_status == get_device_env_info->peer_sys_info.inear_status)) {
            return true;
        }
    }

    return false;
}

static void xspace_tws_switch_process(void)
{
    bool need_tws_switch = false;
    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);

    XTS_TRACE_ENTER();

    xspace_interface_delay_timer_start(time_cycle, (uint32_t)xspace_tws_switch_process, 0, 0, 0);

    if (!xspace_interface_tws_is_master_mode() || (!xspace_interface_tws_link_connected() || !xspace_interface_tws_ibrt_link_connected())) {
        return;
    }

    if (app_ibrt_customif_ui_is_tws_switching()) {
        XTS_TRACE(0, " \n\n\ntws switch is already goning!!\n\n\n");
        return;
    }

#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
    if (xspace_low_battery_tws_swtich_force()) {
        XTS_TRACE(0, " battery low pd need tws switch!!!");
        app_ibrt_customif_ui_tws_switch();
        return;
    }
#endif

    if (!xspace_tws_swtich_ui_status_match()) {
        XTS_TRACE(0, " ui status isn't allow tws switch!!!");
        return;
    }

    if (xspace_interface_is_sco_mode()) {
#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
        if (xspace_snr_wnr_tws_switch_is_need()) {
            need_tws_switch = true;
            XTS_TRACE(0, " <call> snr & wnr need tws switch");
        } else
#endif
#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
            if (xspace_low_battery_tws_swtich_need()
#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
                && xspace_snr_wnr_is_allow_other_case_tws_switch()
#endif
            ) {
            need_tws_switch = true;
            XTS_TRACE(0, " <call> low battery need tws switch");
        }
#endif
    } else {

#if defined(__XSPACE_RSSI_TWS_SWITCH__)
        if (xspace_rssi_process_tws_switch_according_rssi_needed()) {
            need_tws_switch = true;
            XTS_TRACE(0, " <not call> rssi need tws switch");
        } else
#endif
#if defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
            if (xspace_low_battery_tws_swtich_need()
#if defined(__XSPACE_RSSI_TWS_SWITCH__)
                && xspace_rssi_process_is_allowed_other_case_tws_switch()
#endif
            ) {
            XTS_TRACE(0, " <not call> low battery tws switch");
            need_tws_switch = true;
        }
#endif
    }

    XTS_TRACE(1, " need tws switch:%d", need_tws_switch);
    if (need_tws_switch) {
        app_ibrt_customif_ui_tws_switch();
    }
}

void xspace_tws_switch_detect_start(void)
{
    XTS_TRACE_ENTER();
    xspace_interface_delay_timer_stop((uint32_t)xspace_tws_switch_process);
    xspace_interface_delay_timer_start(time_cycle, (uint32_t)xspace_tws_switch_process, 0, 0, 0);
}

void xspace_tws_switch_detect_stop(void)
{
    XTS_TRACE_ENTER();
    xspace_interface_delay_timer_stop((uint32_t)xspace_tws_switch_process);
}

void xspace_tws_swtich_operat_detect_type(uint8_t type, bool opt)
{
    switch (type) {
        case XTS_DETECT_RSSI:
#if defined(__XSPACE_RSSI_TWS_SWITCH__)
            if (opt) {
                time_cycle = XTS_MONITOR_SHROT_TIMEOUT;
                xspace_rssi_process_detect_start();
            } else {
                time_cycle = XTS_MONITOR_LONG_TIMEOUT;
                xspace_rssi_process_detect_stop();
            }
#endif
            break;

        case XTS_DETECT_SNR:
#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
            if (opt) {
                time_cycle = XTS_MONITOR_SHROT_TIMEOUT;
                xspace_snr_wnr_tws_switch_detect_start();
            } else {
                time_cycle = XTS_MONITOR_LONG_TIMEOUT;
                xspace_snr_wnr_tws_switch_detect_stop();
            }
#endif
            break;

        default:
            break;
    }
}

void xspace_tws_switch_process_init(void)
{
#if defined(__XSPACE_RSSI_TWS_SWITCH__)
    xspace_rssi_process_init();
#endif

#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
    xspace_snr_wnr_tws_switch_init();
#endif

    //xspace_tws_switch_detect_start();
}

#endif
