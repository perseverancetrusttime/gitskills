#if defined(__XSPACE_UI__) && defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)
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
#include "xspace_low_battery_tws_switch.h"

static bool lb_force_tws_swtich = false;
static bool lb_need_tws_swtich = false;

void xspace_low_battery_tws_switch(bool force)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);

    lb_force_tws_swtich = force;
    lb_need_tws_swtich = force;

    TRACE(3, "%d,local_wear:%d,peer_wear:%d(1:off,2:on).", force, get_device_env_info->local_sys_info.inear_status,
          get_device_env_info->peer_sys_info.inear_status);
    TRACE(3, "ibrt_connected:%d,tws_link:%d", xspace_interface_tws_ibrt_link_connected(), xspace_interface_tws_link_connected());
    TRACE(3, "besaud:%d,mobile_link:%d,mode:%d.", xspace_interface_tws_link_connected(), xspace_interface_is_any_mobile_connected(), p_ibrt_ctrl->tws_mode);

    if (xspace_interface_tws_is_master_mode() && xspace_interface_tws_link_connected() && xspace_interface_tws_link_connected()
        && xspace_interface_is_any_mobile_connected()) {
        if (force == false) {
            if (get_device_env_info->local_sys_info.inout_box_status == XUI_BOX_OUT_BOX
                && get_device_env_info->peer_sys_info.inout_box_status == XUI_BOX_OUT_BOX
                && get_device_env_info->local_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
                && get_device_env_info->peer_bat_info.data_valid == XUI_TWS_BAT_DATA_VALID_FLAG
                && get_device_env_info->local_sys_info.inear_status == get_device_env_info->peer_sys_info.inear_status
                && get_device_env_info->local_bat_info.bat_per + XSPACE_LB_TWS_SWITCH_LIMIT <= get_device_env_info->peer_bat_info.bat_per) {
                lb_need_tws_swtich = 0x01;
            }
        }
    }

    TRACE(1, "force:%d need:%d.", lb_force_tws_swtich, lb_need_tws_swtich);
}

bool xspace_low_battery_tws_swtich_need(void)
{
    return lb_need_tws_swtich;
}

bool xspace_low_battery_tws_swtich_force(void)
{
    return lb_force_tws_swtich;
}

void xspace_low_battery_tws_switch_init(void)
{
}

#endif
