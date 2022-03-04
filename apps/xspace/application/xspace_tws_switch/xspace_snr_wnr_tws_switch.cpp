#if defined(__XSPACE_UI__) && defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
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
#include "xspace_snr_wnr_tws_switch.h"

static xsnrts_info_s xsnrts_info;
static xsnrts_data_s xsnrts_wnr_data;
//static xsnrts_data_s xsnrts_snr_data;

extern "C" int xspace_wind_state_get(void);
extern "C" float xspace_wind_value_get(void);
extern "C" float xspace_snr_value_get(void);

static void xspace_snr_wnr_tws_switch_data_reset(void)
{
    memset(&xsnrts_info, 0x00, sizeof(xsnrts_info_s));
    memset(&xsnrts_wnr_data, 0x00, sizeof(xsnrts_data_s));
    //    memset(&xsnrts_snr_data, 0x00, sizeof(xsnrts_data_s));
}

bool xspace_snr_wnr_is_allow_other_case_tws_switch(void)
{
    bool rlt = true;

    if (xsnrts_info.local_info.wind_status || xsnrts_info.peer_info.wind_status) {
        if (!xsnrts_info.local_info.wind_status && xsnrts_info.peer_info.wind_status) {
            rlt = false;
        } else if (xsnrts_info.local_info.wind_status && xsnrts_info.peer_info.wind_status) {
            if (xsnrts_info.peer_info.wind_noise - xsnrts_info.local_info.wind_noise > XSNRTS_WNR_LIMIT) {
                rlt = false;
            }
        }
    }

    return rlt;
}

bool xspace_snr_wnr_tws_switch_is_need(void)
{
    TRACE(0, "/***********************************************/");
    TRACE(2, "[XSNRTS] is_run:%d/%d", xsnrts_info.local_info.is_run, xsnrts_info.peer_info.is_run);
    TRACE(2, "[XSNRTS] wind status:%d/%d", xsnrts_info.local_info.wind_status, xsnrts_info.peer_info.wind_status);
    TRACE(2, "[XSNRTS] wind noise:%d/%d", xsnrts_info.local_info.wind_noise, xsnrts_info.peer_info.wind_noise);
    TRACE(1, "[XSNRTS] wind noise diff:%d", xsnrts_info.peer_info.wind_noise - xsnrts_info.local_info.wind_noise);
    TRACE(0, "/***********************************************/");

    xui_tws_ctx_t *get_device_env_info = NULL;
    xspace_ui_env_get(&get_device_env_info);

    if (!xspace_interface_tws_link_connected() || !xspace_interface_tws_ibrt_link_connected()) {
        XSNRTS_TRACE(2, " Warning: tws besaud:%d, ibrt link:%d", xspace_interface_tws_link_connected(), xspace_interface_tws_ibrt_link_connected());
        return false;
    }

    if (XUI_INEAR_ON != get_device_env_info->local_sys_info.inear_status || XUI_INEAR_ON != get_device_env_info->peer_sys_info.inear_status) {
        XSNRTS_TRACE(2, " Warning: wear:%d/%d", get_device_env_info->local_sys_info.inear_status, get_device_env_info->peer_sys_info.inear_status);
        return false;
    }

    if (!xspace_interface_tws_is_master_mode()) {
        XSNRTS_TRACE(0, " Warning: Is not master!!!");
        return false;
    }

    if (xsnrts_info.local_info.is_run && xsnrts_info.peer_info.is_run) {
        if (xsnrts_info.local_info.wind_status || xsnrts_info.peer_info.wind_status) {
            if (xsnrts_info.local_info.wind_status && !xsnrts_info.peer_info.wind_status) {
                return true;
            } else if (xsnrts_info.local_info.wind_status && xsnrts_info.peer_info.wind_status) {
                if (xsnrts_info.peer_info.wind_noise - xsnrts_info.local_info.wind_noise > XSNRTS_WNR_LIMIT) {
                    return true;
                }
            }
        }
    }

    return false;
}

static int16_t xspace_snr_wnr_data_filter(int16_t *value, unsigned int count)
{
    int16_t rlt = 0;
    int16_t sum = 0;
    int16_t min = 0x7FFF, max = 0x8000;
    unsigned int i;

    for (i = 0; i < count; i++) {
        sum += value[i];
        if (min > value[i])
            min = value[i];
        if (max < value[i])
            max = value[i];
    }

    if (count < XSNRTS_STABLE_COUNT)
        rlt = sum / ((int16_t)count);
    else
        rlt = (sum - max - min) / ((int16_t)(count - 2));

    return rlt;
}

void xspace_snr_wnr_data_fill(void)
{
    if (!xspace_interface_is_sco_mode()) {
        xsnrts_info.local_info.is_run = false;
        xspace_snr_wnr_tws_switch_data_reset();
        return;
    }

    //    TRACE(3, "[XSNRTS] wind noise:%d/%d/%d", xspace_wind_state_get(), (int)(xspace_wind_value_get()*100), (int)(xspace_snr_value_get()*100));

    xsnrts_info.local_info.is_run = true;
    xsnrts_info.local_info.wind_status = xspace_wind_state_get();

    xsnrts_wnr_data.data[xsnrts_wnr_data.index++ % XSNRTS_STABLE_COUNT] = (int16_t)(xspace_wind_value_get() * 100);
    xsnrts_wnr_data.count += (xsnrts_wnr_data.count < XSNRTS_STABLE_COUNT);
    xsnrts_info.local_info.wind_noise = xspace_snr_wnr_data_filter(xsnrts_wnr_data.data, xsnrts_wnr_data.count);
    /*
    xsnrts_snr_data.data[xsnrts_snr_data.index++%XSNRTS_STABLE_COUNT] = (int16_t)(xspace_snr_value_get()*100);
	xsnrts_snr_data.count +=(xsnrts_snr_data.count < XSNRTS_STABLE_COUNT);
	xsnrts_info.local_info.snr = xspace_snr_wnr_data_filter(xsnrts_snr_data.data, xsnrts_snr_data.count);
*/
}

void xspace_snr_wnr_tws_recv_sync_info(uint8_t *data, uint8_t len)
{
    if (len != sizeof(xsnrts_sys_info_s)) {
        XSNRTS_TRACE(0, " Warning: sync data platform failed");
        return;
    }

    memcpy((void *)&xsnrts_info.peer_info, data, len);
#if 0 
    TRACE(0, "/***********************************************/");
    TRACE(2, "[XSNRTS] is_run:%d/%d", xsnrts_info.local_info.is_run, xsnrts_info.peer_info.is_run);
    TRACE(2, "[XSNRTS] wind status:%d/%d", xsnrts_info.local_info.wind_status, xsnrts_info.peer_info.wind_status);
    TRACE(2, "[XSNRTS] wind noise:%d/%d", xsnrts_info.local_info.wind_noise, xsnrts_info.peer_info.wind_noise);
    //TRACE(2, "[XSNRTS] snr:%d/%d", xsnrts_info.local_info.snr, xsnrts_info.peer_info.snr);
    TRACE(0, "/***********************************************/");
#endif
}

/***********longz add***********************************************************/
bool xspace_get_ear_wind_status(void)
{
    if ((xsnrts_info.peer_info.wind_status && xsnrts_info.peer_info.is_run) || (xsnrts_info.local_info.wind_status && xsnrts_info.local_info.is_run)) {
        return true;
    }

    return false;
}
/***********longz add***********************************************************/
static void xspace_snr_wnr_tws_info_sync(void)
{
    if (xspace_interface_is_sco_mode() && xspace_interface_tws_is_slave_mode()) {
        xspace_ui_tws_send_info_sync_cmd(XUI_SYS_INFO_SYNC_SNR_WNR_INFO, (uint8_t *)&xsnrts_info.local_info, sizeof(xsnrts_sys_info_s));
    }
    xspace_interface_delay_timer_start(XSNRTS_MONITOR_TIMEOUT, (uint32_t)xspace_snr_wnr_tws_info_sync, 0, 0, 0);
}

void xspace_snr_wnr_tws_switch_detect_start(void)
{
    xspace_snr_wnr_tws_switch_data_reset();
    xspace_interface_delay_timer_stop((uint32_t)xspace_snr_wnr_tws_info_sync);
    xspace_interface_delay_timer_start(XSNRTS_MONITOR_TIMEOUT, (uint32_t)xspace_snr_wnr_tws_info_sync, 0, 0, 0);
}

void xspace_snr_wnr_tws_switch_detect_stop(void)
{
    xspace_snr_wnr_tws_switch_data_reset();
    xspace_interface_delay_timer_stop((uint32_t)xspace_snr_wnr_tws_info_sync);
}

void xspace_snr_wnr_tws_switch_init(void)
{
    xspace_snr_wnr_tws_switch_data_reset();
}

#endif
