#if defined(__XSPACE_BATTERY_MANAGER__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_pmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_battery.h"
#include "hal_charger.h"
#include "dev_thread.h"
#include "xspace_interface.h"
#include "xspace_battery_manager.h"
#if defined(__NTC_SUPPORT__)
#include "hal_ntc.h"
#endif

static xbm_ctx_s xbm_ctx;
static osTimerId xbm_charger_debounce_timer = NULL;
static xbm_bat_info_ui_cb_func       xbm_bat_info_ui_cb = NULL;
static xbm_charger_ctrl_ui_cb_func   xbm_chagrer_ctrl_ui_cb = NULL;
static bool charger_debounce_timer_flag = false;
static void xspace_battery_manager_charger_debounce_timeout_handler(void const *param);
osTimerDef (XBM_CHARGER_DEBOUNCE_TIMER, xspace_battery_manager_charger_debounce_timeout_handler);

bool xspace_get_charger_debounce_timer_flag(void)
{
    return charger_debounce_timer_flag;
}

static void xspace_battery_manager_report_ui_layout(void)
{
    xbm_ui_status_s xbm_report_ui;

    XBM_TRACE(1, " xbm_bat_info_ui_cb: %08x", (unsigned int)xbm_bat_info_ui_cb);
    if (NULL != xbm_bat_info_ui_cb) {
        xbm_report_ui.curr_level = xbm_ctx.curr_level;
        xbm_report_ui.curr_per = xbm_ctx.curr_per;
        xbm_report_ui.status = xbm_ctx.status;
        xbm_report_ui.temperature = xbm_ctx.temperature;
        xbm_bat_info_ui_cb(xbm_report_ui);
    }
}

xbm_status_s xspace_get_battery_status(void)
{
    return xbm_ctx.status;
}

void xspace_update_battery_status(void)
{
    hal_pmu_status_e charger_status = hal_pmu_force_get_charger_status();

    if(HAL_PMU_PLUGIN == charger_status) {
        XBM_TRACE(0, " Charger Plugin !!!");
        xbm_ctx.status = XBM_STATUS_CHARGING;
        hal_bat_info_set_charging_status(HAL_BAT_STATUS_CHARGING);
    } else {
        XBM_TRACE(0, " Charger plugout !!!");
        xbm_ctx.status = XBM_STATUS_NORMAL;
        hal_bat_info_set_charging_status(HAL_BAT_STATUS_DISCHARGING);
    }
}

static void xspace_battery_manager_bat_info_cb(hal_bat_info_s bat_info)
{
    XBM_TRACE(3, " volt:%dV, level:%d, percentage:%d", bat_info.bat_volt, bat_info.bat_level, bat_info.bat_per);

    xbm_ctx.curr_per = bat_info.bat_per;
    xbm_ctx.curr_level = bat_info.bat_level;
    xbm_ctx.curr_volt = bat_info.bat_volt;

    if (!xspace_battery_manager_bat_is_charging()) {
        if(bat_info.bat_per <= xbm_ctx.ui_config.pd_per_threshold) {
            // shutdown
            xbm_ctx.status = XBM_STATUS_PDVOLT;
        } else if(bat_info.bat_per <= xbm_ctx.ui_config.low_per_threshold) {
            // low battery, please charging
            xbm_ctx.status = XBM_STATUS_UNDERVOLT;
        } else if(bat_info.bat_per >= xbm_ctx.ui_config.higt_per_threshold) {
            // over voltage while charging
            xbm_ctx.status = XBM_STATUS_OVERVOLT;
        } else {
            xbm_ctx.status = XBM_STATUS_NORMAL;
        }
    }
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_battery_manager_report_ui_layout, 0, 0, 0);

#if !defined(__NTC_SUPPORT__)
    if(xspace_battery_manager_bat_is_charging()) {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_battery_manager_charging_ctrl, 0, 0, 0);
    }
#endif

    //TODO: UI callback
}

static int32_t xspace_battery_manager_bat_info_init(void)
{
    int32_t ret = hal_bat_info_init();
    if (ret) {
        XBM_TRACE(0, "Battery IC Init Failed!!!");
        return ret;
    }

    hal_bat_info_set_query_bat_info_cb(xspace_battery_manager_bat_info_cb);
    xbm_ctx.init_complete_status |= XBM_BAT_INFO_INIT_FLAG;
   
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)hal_bat_info_query_bat_info, 0, 0, 0);
    
#if !defined(__BAT_ADC_INFO_GET__)
    hal_bat_info_query_bat_info();
#endif
    return ret;
}

static void xspace_battery_manager_charger_debounce_timeout_handler(void const *param)
{
    xspace_update_battery_status();
    charger_debounce_timer_flag = false;
}

static void xspace_battery_manager_charger_status_changed_handle(hal_pmu_status_e status, uint32_t *data, uint32_t len)
{
    charger_debounce_timer_flag = true;
    osTimerStop(xbm_charger_debounce_timer);
    osTimerStart(xbm_charger_debounce_timer,
                (xspace_battery_manager_bat_is_charging() == true) ? (XBM_CHARGER_PLUGOUT_TIMER_DELAY_MS):(XBM_CHARGER_PLUGIN_TIMER_DELAY_MS));
}

static void xspace_battery_manager_charger_status_changed_cb(hal_pmu_status_e status)
{
    static uint32_t prev_ticks_vbus = 0;
    uint32_t curr_ticks = hal_sys_timer_get();
    uint32_t diff_ticks_xbus = hal_timer_get_passed_ticks(curr_ticks, prev_ticks_vbus);

    //XBM_TRACE(2, " Charger status changed %d(%d)!!!", diff_ticks_xbus, MS_TO_TICKS(XBM_CHARGER_INT_DEBOUNCE_MS);

    if(prev_ticks_vbus == 0 || diff_ticks_xbus > MS_TO_TICKS(XBM_CHARGER_INT_DEBOUNCE_MS)) {
        prev_ticks_vbus = curr_ticks;
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
            (uint32_t)xspace_battery_manager_charger_status_changed_handle, 0, 0, 0);
    }
}

static void xspace_battery_manager_config_pmu_irq(bool status)
{
    if (status) {
        XBM_TRACE(0, " Enable PMU IRQ!!!");
        hal_pmu_add_charger_status_change_cb(xspace_battery_manager_charger_status_changed_cb);
    } else {
        XBM_TRACE(0, " Disable PMU IRQ!!!");
        hal_pmu_remove_charger_status_change_cb(xspace_battery_manager_charger_status_changed_cb);
    }
}

static void xspace_battery_manager_pmu_init(void)
{
    xspace_battery_manager_config_pmu_irq(true);
    hal_pmu_status_e status = hal_pmu_force_get_charger_status();
    XBM_TRACE(1, " Init Status %d(1:Plugin 2:PlugOut) !!!", status);

    if (HAL_PMU_PLUGIN == status) {
        xbm_ctx.status = XBM_STATUS_CHARGING;
    } else if (HAL_PMU_PLUGOUT == status) {
        xbm_ctx.status = XBM_STATUS_NORMAL;
    }

    xbm_ctx.init_complete_status |= XBM_PMU_INIT_FLAG;
    XBM_TRACE(1, " Charger Status %d !!!", xbm_ctx.status);
}

static void xspace_battery_manager_charging_ctrl(void)
{
    uint16_t set_charging_voltage = 0;
    uint16_t curr_charging_voltage = 0;
    hal_charging_ctrl_e curr_charging_status = HAL_CHARGING_CTRL_DISABLE;
    hal_charging_current_e curr_charging_current = HAL_CHARGING_CTRL_UNCHARGING;
    xbm_charging_ctrl_e set_charging_status = XBM_CHARGING_CTRL_DISABLE;
    xbm_charging_current_e set_charging_current = XBM_CHARGING_CTRL_UNCHARGING;
    POSSIBLY_UNUSED XBM_VOLTAGE_T curr_volt = xspace_battery_manager_bat_get_voltage();

    XBM_TRACE(2, "charging=%d,current_voltage:%d \n", xspace_battery_manager_bat_is_charging(), curr_volt);

    if (!xspace_battery_manager_bat_is_charging()) {
        return;
    }

    if (NULL == xbm_chagrer_ctrl_ui_cb) {
        return;
    }

    hal_charger_get_charging_current(&curr_charging_current);
    hal_charger_get_charging_status(&curr_charging_status);
    hal_charger_get_charging_voltage(&curr_charging_voltage);

    set_charging_current = (xbm_charging_current_e)curr_charging_current;
    set_charging_status =(xbm_charging_ctrl_e)curr_charging_status;
    //set_charging_voltage = curr_charging_voltage;

    xbm_chagrer_ctrl_ui_cb(&set_charging_current, &set_charging_voltage, &set_charging_status);

    if (XBM_STATUS_OVERVOLT == xbm_ctx.status) {
        XBM_TRACE(0, " current voltage is so highest, so stop charging mode");
        set_charging_status  = XBM_CHARGING_CTRL_DISABLE;
        set_charging_current = XBM_CHARGING_CTRL_UNCHARGING;
    }

    XBM_TRACE(3, "curr_current:%d,curr_status:%d,curr_voltage:%d",
        curr_charging_current, curr_charging_status, curr_charging_voltage);
    XBM_TRACE(3, "set_current =%d,set_status:%d,set_voltage:%d",
        set_charging_current, set_charging_status, set_charging_voltage);

    if(curr_charging_current != (hal_charging_current_e)set_charging_current) {
        hal_charger_set_charging_current((hal_charging_current_e)set_charging_current);
        XBM_TRACE(1, " set_charging_current =%d", set_charging_current);
    }

    if(curr_charging_status != (hal_charging_ctrl_e)set_charging_status) {
        hal_charger_set_charging_status((hal_charging_ctrl_e)set_charging_status);
        XBM_TRACE(1, " set_charging_status =%d", set_charging_status);
    }

    if (curr_charging_voltage != set_charging_voltage) {
        hal_charger_set_charging_voltage(set_charging_voltage);
        XBM_TRACE(1, " set_charging_voltage =%d", set_charging_voltage);
    }
}

#if defined(__NTC_SUPPORT__)
static void xspace_battery_manager_ntc_report_cb(int16_t temp)
{
    XBM_TRACE(1, " >>> temperature: %d!!!", temp);

    xspace_battery_manager_set_curr_temperature(temp);

    if(xspace_battery_manager_bat_is_charging()) {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_battery_manager_charging_ctrl, 0, 0, 0);
    }
}

static int32_t xspace_battery_manager_ntc_init(void)
{
    int32_t ret = -1;

    ret = hal_ntc_init();
    if (ret) {
        XBM_TRACE(0, " NTC Init Faild!!!");
        return ret;
    }

    hal_ntc_register_auto_report_temperature_cb(xspace_battery_manager_ntc_report_cb);
    xbm_ctx.init_complete_status |= XBM_NTC_INIT_FLAG;

    return ret;
}
#endif	/*	End of #if defined(__NTC_SUPPORT__) */

static void xpsace_battery_manager_charger_status_cb(hal_charging_status_e status)
{
    XBM_TRACE(1, "status=%d", status);

    //TODO: Do somthing when charging status is changed.

    //TODO: Neet to notify the UI layout
}

static int32_t xspace_battery_managre_charger_ctrl_init(void)
{
    int32_t ret = -1;

    ret = hal_charger_init();
    if (ret) {
        XBM_TRACE(0, " Charger Ctrl IC Init Faild!!!");
        return ret;
    }

    hal_charger_register_charging_status_report_cb(xpsace_battery_manager_charger_status_cb);
    xbm_ctx.init_complete_status |= XBM_CHARGER_CTRL_INIT_FLAG;

    return ret;
}

bool xspace_battery_manager_bat_is_charging(void)
{
    return (XBM_STATUS_CHARGING == xbm_ctx.status);
}

uint16_t xspace_battery_manager_bat_get_voltage(void)
{
    return xbm_ctx.curr_volt;
}

uint8_t xspace_battery_manager_bat_get_level(void)
{
    return xbm_ctx.curr_level;
}

uint8_t xspace_battery_manager_bat_get_percentage(void)
{
    XBM_TRACE(1, " Current bat per:%d", xbm_ctx.curr_per);
    return xbm_ctx.curr_per;
}

void xspace_battery_manager_set_curr_temperature(int16_t temp)
{
    XBM_TRACE(1, " Set current temperature:%d", temp);
    xbm_ctx.temperature = temp;
}

int16_t xspace_battery_manager_get_curr_temperature(void)
{
    XBM_TRACE(1, " Get current temperature:%d", xbm_ctx.temperature);
    return xbm_ctx.temperature;
}

#if defined(__NTC_SUPPORT__)   
static void xspace_battery_manager_high_temperature_shutdown(void)//add by chenjunjie 0310
{
	//XBM_TRACE(0," Enter");
	hal_ntc_start_oneshot_measure();
	int16_t curr_temp = 0;
	hal_ntc_get_temperature(&curr_temp);
	XBM_TRACE(1," the curr_temp: %d",curr_temp);
	if (curr_temp < XBM_HIGHEST_TEMP_C)
		return ;
	XBM_TRACE(0, "the curr_temp too high,shutdown!");
	xspace_interface_shutdown();
}
#endif

void xspace_battery_manager_timing_todo(void)
{
    XBM_TRACE(0, " query bat info");

    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)hal_bat_info_query_bat_info, 0, 0, 0);

#if defined(__NTC_SUPPORT__)
	xspace_battery_manager_high_temperature_shutdown();//added by chenjunjie 0310
#endif
}

void xspace_battery_manager_enter_shutdown_mode(void)
{
    hal_charger_enter_shutdown_mode();
}

void xspace_battery_manage_register_bat_info_ui_cb(xbm_bat_info_ui_cb_func cb)
{
    if (NULL == cb)
        return;

    xbm_bat_info_ui_cb = cb;
}

void xspace_battery_manager_register_charger_ctrl_ui_cb(xbm_charger_ctrl_ui_cb_func cb)
{
    if (NULL == cb)
        return;

    xbm_chagrer_ctrl_ui_cb = cb;
}

int32_t xspace_battery_manager_init(xbm_bat_ui_config_s bat_ui_cofig)
{
    int32_t ret = -1;

    memset((void *)&xbm_ctx, 0x00, sizeof(xbm_ctx));

    xbm_ctx.ui_config.higt_per_threshold = bat_ui_cofig.higt_per_threshold;
    xbm_ctx.ui_config.low_per_threshold = bat_ui_cofig.low_per_threshold;
    xbm_ctx.ui_config.pd_per_threshold = bat_ui_cofig.pd_per_threshold;

    xbm_ctx.temperature = 20;

    if (xbm_charger_debounce_timer == NULL) {
        xbm_charger_debounce_timer = osTimerCreate (osTimer(XBM_CHARGER_DEBOUNCE_TIMER), osTimerOnce, NULL);
    }

    xspace_battery_manager_pmu_init();

    ret = xspace_battery_managre_charger_ctrl_init();
    if (ret) {
        XBM_TRACE(0, " Changer Init Fail!!!");
    }

    ret = xspace_battery_manager_bat_info_init();
    if (ret) {
        XBM_TRACE(0, " BAT Capture Info Init Fail!!!");
    }

#if defined(__NTC_SUPPORT__)
    ret = xspace_battery_manager_ntc_init();
    if (ret) {
        XBM_TRACE(0, " NTC Init Fail!!!");
    }
#endif

    /* First Step */
    if (XBM_STATUS_CHARGING == xbm_ctx.status) {
        hal_bat_info_set_charging_status(HAL_BAT_STATUS_CHARGING);
    } else {
        hal_bat_info_set_charging_status(HAL_BAT_STATUS_DISCHARGING);
    }

#if defined(__NTC_SUPPORT__)
    /* Second Step */
    hal_ntc_start_oneshot_measure();
#endif

    /* Third Step */
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_battery_manager_charging_ctrl, 0, 0, 0);

    XBM_TRACE(0, " Init Success!!!");

    return 0;
}
#endif  // (__XSPACE_BATTERY_MANAGER__)
