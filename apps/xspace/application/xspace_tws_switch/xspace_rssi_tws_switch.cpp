#if defined(__XSPACE_UI__) && defined(__XSPACE_RSSI_TWS_SWITCH__)
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
#include "xspace_rssi_tws_switch.h"

static xrssi_info_s xrssi_info;
bool xrssi_run_flag = false;
static osTimerId rssi_debounce_timer = NULL;
static void xspace_rssi_process_debounce_timeout_handler(void const *param);
osTimerDef(RSSI_DEBOUNCE_TIMER, xspace_rssi_process_debounce_timeout_handler);

static void xspace_rssi_process_reset(void)
{
    XRSSI_TRACE_ENTER();

    memset((void *)&xrssi_info, 0x00, sizeof(xrssi_info));
    xrssi_info.local_rssi.tws_rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    xrssi_info.local_rssi.mobile_rssi[0].rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    xrssi_info.local_rssi.mobile_rssi[1].rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    xrssi_info.peer_rssi.tws_rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    xrssi_info.peer_rssi.mobile_rssi[0].rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    xrssi_info.peer_rssi.mobile_rssi[1].rssi = XRSSI_PROCESS_MONITOR_INVAILD_DATA;
    XRSSI_TRACE_EXIT();
}

bool xspace_rssi_value_vaild(void)
{
    XRSSI_TRACE_ENTER();
    if (xrssi_info.local_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA || xrssi_info.peer_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA
        || xrssi_info.local_rssi.mobile_count != xrssi_info.peer_rssi.mobile_count
        || (xrssi_info.local_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA
            && xrssi_info.local_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
        || (xrssi_info.peer_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA
            && xrssi_info.peer_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)) {
        XRSSI_TRACE(0, "RSSI Value invaild!!!");
        return false;
    }

    XRSSI_TRACE(0, "RSSI Value vaild!!!");
    XRSSI_TRACE_EXIT();
    return true;
}

bool xspace_rssi_process_is_allowed_other_case_tws_switch(void)
{
    XRSSI_TRACE_ENTER();
    int8_t rssi_d0_value_diff = 0;
    int8_t rssi_d1_value_diff = 0;

    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);

    if ((xrssi_info.peer_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA) || (xrssi_info.local_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
        || (xrssi_info.peer_rssi.tws_rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI) || (xrssi_info.local_rssi.tws_rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
        XRSSI_TRACE(1, " Warning: TWS RSSI invaild:%d", __LINE__);
        return true;
    }

    if (xrssi_info.local_rssi.mobile_count == 1) {
        if ((xrssi_info.peer_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
            XRSSI_TRACE(1, " Warning: RSSI invaild:%d", __LINE__);
            return true;
        }

        if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
            XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
            return true;
        } else {
            rssi_d0_value_diff = xrssi_info.local_rssi.mobile_rssi[0].rssi - xrssi_info.peer_rssi.mobile_rssi[0].rssi;
            if (rssi_d0_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD) {
                XRSSI_TRACE(1, " RSSI diff:%d, line:%d", rssi_d0_value_diff, __LINE__);
                return false;
            }
        }
    } else {
        if ((xrssi_info.peer_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.peer_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[1].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[1].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
            XRSSI_TRACE(1, " Warning: RSSI invaild:%d", __LINE__);
            return true;
        }

        if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
            if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[1].addr, 6)) {
                XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                return true;
            } else {
                rssi_d0_value_diff = xrssi_info.peer_rssi.mobile_rssi[1].rssi - xrssi_info.local_rssi.mobile_rssi[0].rssi;
                if (memcmp(xrssi_info.local_rssi.mobile_rssi[1].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
                    XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                    return true;
                } else {
                    rssi_d1_value_diff = xrssi_info.local_rssi.mobile_rssi[0].rssi - xrssi_info.peer_rssi.mobile_rssi[1].rssi;
                }
            }
        } else {
            if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
                XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                return true;
            } else {
                rssi_d0_value_diff = xrssi_info.peer_rssi.mobile_rssi[0].rssi - xrssi_info.local_rssi.mobile_rssi[0].rssi;
                if (memcmp(xrssi_info.local_rssi.mobile_rssi[1].addr, xrssi_info.peer_rssi.mobile_rssi[1].addr, 6)) {
                    XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                    return true;
                } else {
                    rssi_d1_value_diff = xrssi_info.local_rssi.mobile_rssi[1].rssi - xrssi_info.peer_rssi.mobile_rssi[1].rssi;
                }
            }
        }

        XRSSI_TRACE(3, " RSSI diff:%d/%d, line:%d", rssi_d0_value_diff, rssi_d1_value_diff, __LINE__);

        if (rssi_d0_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD && rssi_d1_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD) {
            return false;
        }
    }

    XRSSI_TRACE_EXIT();
    return true;
}

bool xspace_rssi_process_tws_switch_according_rssi_needed(void)
{
    XRSSI_TRACE_ENTER();
    int8_t rssi_d0_value_diff = 0;
    int8_t rssi_d1_value_diff = 0;

    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);
    if (XUI_BOX_IN_BOX == get_device_env_info->peer_sys_info.inout_box_status) {
        XRSSI_TRACE(1, " Warning: peer_sys_info.inout_box_status:%d", get_device_env_info->peer_sys_info.inout_box_status);
        return false;
    }

    if ((xrssi_info.peer_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA) || (xrssi_info.local_rssi.tws_rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
        || (xrssi_info.peer_rssi.tws_rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI) || (xrssi_info.local_rssi.tws_rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
        XRSSI_TRACE(1, " Warning: TWS RSSI invaild:%d", __LINE__);
        return false;
    }

    /*
    int8_t rssi_d_value_diff = xrssi_info.peer_rssi.tws_rssi - xrssi_info.local_rssi.tws_rssi;
    if(rssi_d_value_diff < XRSSI_PROCESS_MONITOR_TWS_THREAD)
    {
        //local RSSI is stronger than peer and local role is SLAVE
        return false;
    }
*/
    if (xrssi_info.local_rssi.mobile_count == 1) {
        if ((xrssi_info.peer_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
            XRSSI_TRACE(1, " Warning: RSSI invaild:%d", __LINE__);
            return false;
        }

        if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
            XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
            xspace_rssi_process_reset();
            return false;
        } else {
            rssi_d0_value_diff = xrssi_info.peer_rssi.mobile_rssi[0].rssi - xrssi_info.local_rssi.mobile_rssi[0].rssi;
            if (rssi_d0_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD) {
                XRSSI_TRACE(1, " RSSI diff:%d, line:%d", rssi_d0_value_diff, __LINE__);
                return true;
            }
        }
    } else {
        if ((xrssi_info.peer_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.peer_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.local_rssi.mobile_rssi[1].rssi == XRSSI_PROCESS_MONITOR_INVAILD_DATA)
            || (xrssi_info.peer_rssi.mobile_rssi[1].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)
            || (xrssi_info.local_rssi.mobile_rssi[1].rssi < XRSSI_PROCESS_MONITOR_MIN_RSSI)) {
            XRSSI_TRACE(1, " Warning: RSSI invaild:%d", __LINE__);
            return false;
        }

        if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
            if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[1].addr, 6)) {
                XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                xspace_rssi_process_reset();
                return false;
            } else {
                rssi_d0_value_diff = xrssi_info.peer_rssi.mobile_rssi[1].rssi - xrssi_info.local_rssi.mobile_rssi[0].rssi;
                if (memcmp(xrssi_info.local_rssi.mobile_rssi[1].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
                    XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                    xspace_rssi_process_reset();
                    return false;
                } else {
                    rssi_d1_value_diff = xrssi_info.peer_rssi.mobile_rssi[0].rssi - xrssi_info.local_rssi.mobile_rssi[1].rssi;
                }
            }
        } else {
            if (memcmp(xrssi_info.local_rssi.mobile_rssi[0].addr, xrssi_info.peer_rssi.mobile_rssi[0].addr, 6)) {
                XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                xspace_rssi_process_reset();
                return false;
            } else {
                rssi_d0_value_diff = xrssi_info.peer_rssi.mobile_rssi[0].rssi - xrssi_info.local_rssi.mobile_rssi[0].rssi;
                if (memcmp(xrssi_info.local_rssi.mobile_rssi[1].addr, xrssi_info.peer_rssi.mobile_rssi[1].addr, 6)) {
                    XRSSI_TRACE(1, " Warning: both side mobile addr diffrent:%d", __LINE__);
                    xspace_rssi_process_reset();
                    return false;
                } else {
                    rssi_d1_value_diff = xrssi_info.peer_rssi.mobile_rssi[1].rssi - xrssi_info.local_rssi.mobile_rssi[1].rssi;
                }
            }
        }

        XRSSI_TRACE(3, " RSSI diff:%d/%d, line:%d", rssi_d0_value_diff, rssi_d1_value_diff, __LINE__);

        if (rssi_d0_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD && rssi_d1_value_diff >= XRSSI_PROCESS_MONITOR_MOBILE_THREAD) {
            return true;
        }
    }

    XRSSI_TRACE_EXIT();
    return false;
}

void xspace_rssi_process_send_sync_info(uint8_t *data, uint8_t *len)
{
    memcpy((uint8_t *)data, (uint8_t *)&xrssi_info.local_rssi, sizeof(xrssi_sys_info_s));
    *len = sizeof(xrssi_sys_info_s);
}
void xspace_rssi_process_sync_info(uint8_t *data, uint8_t len)
{
    int j = 0;

    XRSSI_TRACE_ENTER();
    if (len != sizeof(xrssi_sys_info_s)) {

        XRSSI_TRACE(0, " Warning: sync data platform failed");
        return;
    }

    memcpy((void *)&xrssi_info.peer_rssi, (void *)data, len);
    TRACE(0, "/***********************************************/");

    TRACE(8, "[XRSSI] LOCAL TWS_RSSI:%d", xrssi_info.local_rssi.tws_rssi);
    TRACE(8, "[XRSSI] PEER TWS_RSSI:%d", xrssi_info.peer_rssi.tws_rssi);
    for (j = 0; j < xrssi_info.local_rssi.mobile_count; j++) {
        TRACE(8, "[XRSSI] LOCAL MOBILE_RSSI[%d]:%d", j, xrssi_info.local_rssi.mobile_rssi[j].rssi);
        DUMP8("%02x ", (uint8_t *)xrssi_info.local_rssi.mobile_rssi[j].addr, sizeof(bt_bdaddr_t));
    }

    for (j = 0; j < xrssi_info.peer_rssi.mobile_count; j++) {
        TRACE(8, "[XRSSI] PEER MOBILE_RSSI[%d]:%d", j, xrssi_info.peer_rssi.mobile_rssi[j].rssi);
        DUMP8("%02x ", (uint8_t *)xrssi_info.peer_rssi.mobile_rssi[j].addr, sizeof(bt_bdaddr_t));
    }
    TRACE(0, "/***********************************************/");

    if (xrssi_info.peer_rssi.mobile_count != xrssi_info.local_rssi.mobile_count) {
        XRSSI_TRACE(0, " Warning: both side mobile rssi count diffrent");
        xspace_rssi_process_detect_start();
        xspace_rssi_process_reset();
    }
    XRSSI_TRACE_EXIT();
}

extern "C" uint8_t app_ibrt_conn_get_all_valid_mobile_info(ibrt_mobile_info_t *p_mobile_info_array[], uint8_t max_size);

static void xspace_rssi_process_get(void)
{
    XRSSI_TRACE_ENTER();
    int8_t j = 0;
    uint8_t count = 0;

    xrssi_run_flag = false;
    ibrt_mobile_info_t *p_mobile_info_array[BT_DEVICE_NUM + 1];
    ibrt_mobile_info_t *p_mobile_info = NULL;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (!xspace_interface_tws_link_connected() || !xspace_interface_tws_ibrt_link_connected()) {
        XRSSI_TRACE(2, " Warning: tws besaud:%d, ibrt link:%d", xspace_interface_tws_link_connected(), xspace_interface_tws_ibrt_link_connected());
        return;
    }

    xrssi_info.local_rssi.mobile_count = app_ibrt_conn_get_all_valid_mobile_info(p_mobile_info_array, BT_DEVICE_NUM + 1);
    for (j = 0; j < xrssi_info.local_rssi.mobile_count; j++) {
        p_mobile_info = p_mobile_info_array[0];
        if (p_mobile_info) {
            xrssi_info.local_rssi.mobile_rssi[count].rssi = p_mobile_info->raw_rssi.rssi0;
            memcpy(xrssi_info.local_rssi.mobile_rssi[count].addr, p_mobile_info->mobile_addr.address, 6);
            count++;
        }
    }

    xrssi_info.local_rssi.mobile_count = count;
    xrssi_info.local_rssi.tws_rssi = p_ibrt_ctrl->raw_rssi.rssi0;

    for (int j = 0; j < xrssi_info.local_rssi.mobile_count; j++) {
        TRACE(8, "[XRSSI] MOBILE_RSSI[%d]:%d", j, xrssi_info.local_rssi.mobile_rssi[j].rssi);
        DUMP8("%02x ", (uint8_t *)xrssi_info.local_rssi.mobile_rssi[j].addr, sizeof(bt_bdaddr_t));
    }

    if (xrssi_info.local_rssi.mobile_count != 0 && xspace_interface_tws_is_master_mode()
        && xrssi_info.local_rssi.mobile_rssi[0].rssi < XRSSI_PROCESS_MONITOR_NEED_SYNC_RSSI) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_SYNC_RSSI_INFO, (uint8_t *)&xrssi_info.local_rssi, sizeof(xrssi_sys_info_s));
    }

    osTimerStart(rssi_debounce_timer, XRSSI_PROCESS_MONITOR_TIMEOUT);
    xrssi_run_flag = true;
    XRSSI_TRACE_EXIT();
}
static void xspace_rssi_process_debounce_timeout_handler(void const *param)
{
    XRSSI_TRACE_ENTER();
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_rssi_process_get, 0, 0, 0);
    XRSSI_TRACE_EXIT();
}

void xspace_rssi_process_detect_start(void)
{
    XRSSI_TRACE_ENTER();
    TRACE(2, "[XRSSI] start timer, %d, %d", xrssi_run_flag, xspace_interface_is_sco_mode());
    if (xspace_interface_is_sco_mode())
        return;

    if (xrssi_run_flag) {
        return;
    }

    osTimerStart(rssi_debounce_timer, XRSSI_PROCESS_MONITOR_TIMEOUT);
    xrssi_run_flag = true;
    XRSSI_TRACE_EXIT();
}

void xspace_rssi_process_detect_stop(void)
{
    XRSSI_TRACE_ENTER();
    TRACE(0, "[XRSSI] stop timer");
    osTimerStop(rssi_debounce_timer);
    xspace_rssi_process_reset();
    xrssi_run_flag = false;
    XRSSI_TRACE_EXIT();
}

void xspace_rssi_process_init(void)
{
    XRSSI_TRACE_ENTER();
    if (NULL == rssi_debounce_timer) {
        rssi_debounce_timer = osTimerCreate(osTimer(RSSI_DEBOUNCE_TIMER), osTimerOnce, NULL);
    }

    xspace_rssi_process_reset();
    XRSSI_TRACE_EXIT();
}

#endif
