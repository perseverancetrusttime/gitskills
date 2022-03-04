#if defined(__XSPACE_INOUT_BOX_MANAGER__)

#include "stdio.h"
#include "cmsis.h"
#include "hal_pmu.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "dev_thread.h"
#include "tgt_hardware.h"
#include "xspace_interface.h"
#include "xspace_inout_box_manager.h"
#if defined(__CHARGER_SUPPORT__)
#include "hal_charger.h"
#endif

#define XIOB_PMU_INT_DEBOUNCE_MS                (50)
#define XIOB_INBOX_DEBOUNCE_TIMER_DELAY_MS      (100)
#define XIOB_OUTBOX_DEBOUNCE_TIMER_DELAY_MS     (150)

static osTimerId xiob_debounce_timer = NULL;
static xiob_status_e xiob_curr_status = XIOB_UNKNOWN;
static xiob_status_change_cb_func xiob_status_change_cb = NULL;

#if defined(__XSPACE_AUTO_TEST__)
static hal_pmu_status_e simu_inout_box_gpio_val = HAL_PMU_UNKNOWN;
#endif

#if defined(__XSPACE_XBUS_MANAGER__)
extern "C" bool xbus_manage_is_communication_mode(void);
#endif

static void xspace_inout_box_manager_debounce_timeout_handler(void const *param);
osTimerDef (XIOB_DEBOUNCE_TIMER, xspace_inout_box_manager_debounce_timeout_handler);

static void xspace_inout_box_manager_debounce_timeout_handler(void const *param)
{
    xiob_status_e inout_box_status = XIOB_UNKNOWN;

#if defined(__XSPACE_AUTO_TEST__)
    hal_pmu_status_e charger_status = (hal_pmu_status_e)simu_inout_box_gpio_val;
#else
#if defined(__XSPACE_XBUS_MANAGER__)
    static char restart_timers = 0;
    XIOB_TRACE(2, "xbus status:%d,restart %d.", xbus_manage_is_communication_mode(), restart_timers);

    if(xbus_manage_is_communication_mode() && restart_timers++ < 5) {
        osTimerStart(xiob_debounce_timer, 50);
        return;
    }

    //XIOB_ASSERT(restart_timers < 5, " error:use xbus time over 1000 ms.");

    restart_timers = 0;
#endif

    hal_pmu_status_e charger_status = hal_pmu_force_get_charger_status();
#endif
    if(HAL_PMU_PLUGIN == charger_status) {
        inout_box_status = XIOB_IN_BOX;
    } else if (HAL_PMU_PLUGOUT == charger_status) {
        inout_box_status = XIOB_OUT_BOX;
    } else {
        XIOB_TRACE(0, " Invaild event!!!");
    }

    if(xiob_curr_status == inout_box_status) {
        XIOB_TRACE(0, " Repeat event!!!");
        return;
    }

    xiob_curr_status = inout_box_status;
	
#if defined(__CHARGER_SUPPORT__)
	hal_charger_polling_status(XIOB_IN_BOX == xiob_curr_status ? true : false);
#endif

    XIOB_TRACE(1, " Earphone %s Box!!!", (xiob_curr_status == XIOB_IN_BOX)?"In":"Out");
    if(xiob_status_change_cb) {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xiob_status_change_cb, xiob_curr_status, 0, 0);
    }
}

static void xspace_inout_box_manager_charger_status_changed_handle(void)
{
    XIOB_TRACE(1, "Inout box status changed %d!", xiob_curr_status);
    osTimerStop(xiob_debounce_timer);
    osTimerStart(xiob_debounce_timer,
                (XIOB_IN_BOX == xiob_curr_status) ? (XIOB_OUTBOX_DEBOUNCE_TIMER_DELAY_MS):(XIOB_INBOX_DEBOUNCE_TIMER_DELAY_MS));
}

static void xspace_inout_box_manager_status_changed_cb(hal_pmu_status_e status)
{
    static uint32_t prev_ticks_vbus = 0;
    uint32_t curr_ticks = hal_sys_timer_get();
    uint32_t diff_ticks_vbus = hal_timer_get_passed_ticks(curr_ticks, prev_ticks_vbus);

    XIOB_TRACE(2, "Inout box status changed:%d(%d)!", diff_ticks_vbus, MS_TO_TICKS(XIOB_PMU_INT_DEBOUNCE_MS));

    if(prev_ticks_vbus == 0 || diff_ticks_vbus > MS_TO_TICKS(XIOB_PMU_INT_DEBOUNCE_MS)) {
        prev_ticks_vbus = curr_ticks;
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
            (uint32_t)xspace_inout_box_manager_charger_status_changed_handle, 0, 0, 0);
    }
}

#if defined(__XSPACE_AUTO_TEST__)
void config_simu_inout_box_gpio_val(uint8_t val)
{
    XIOB_TRACE(1, "val:%d(1:in,2:out).", val);

    if(simu_inout_box_gpio_val == val)
    {
        return;
    }
    simu_inout_box_gpio_val = (hal_pmu_status_e)val;

    xspace_inout_box_manager_status_changed_cb(simu_inout_box_gpio_val);
}
#else
static void xspace_inout_box_manager_config_pmu_irq(bool status)
{
    if (status) {
        XIOB_TRACE(0, " Enable PMU IRQ!!!");
        hal_pmu_add_charger_status_change_cb(xspace_inout_box_manager_status_changed_cb);
    } else {
        XIOB_TRACE(0, " Disable PMU IRQ!!!");
        hal_pmu_remove_charger_status_change_cb(xspace_inout_box_manager_status_changed_cb);
    }
}
#endif

static void xpsace_inout_box_manage_init_if(void)
{
    hal_pmu_status_e status = HAL_PMU_UNKNOWN;

    if (xiob_debounce_timer == NULL) {
        xiob_debounce_timer = osTimerCreate (osTimer(XIOB_DEBOUNCE_TIMER), osTimerOnce, NULL);
    }
#if defined(__XSPACE_AUTO_TEST__)
    config_simu_inout_box_gpio_val(2);
#else
    xspace_inout_box_manager_config_pmu_irq(true);
    status = hal_pmu_force_get_charger_status();
#endif
    if(HAL_PMU_PLUGIN == status) {
        XIOB_TRACE(0, " Init Earphone In Box !!!");
        xiob_curr_status = XIOB_IN_BOX;
    } else if (HAL_PMU_PLUGOUT == status){
        XIOB_TRACE(0, " Init Earphone Out Box !!!");
        xiob_curr_status = XIOB_OUT_BOX;
    }
	
#if defined(__CHARGER_SUPPORT__)
	hal_charger_polling_status(XIOB_IN_BOX == xiob_curr_status ? true : false);
#endif

    if(xiob_status_change_cb)
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xiob_status_change_cb, xiob_curr_status, 0, 0);

    XIOB_TRACE(0, " Init Success !!!");
}

xiob_status_e xspace_inout_box_manager_get_curr_status(void)
{
    return xiob_curr_status;
}

void xspace_inout_box_manager_set_curr_status(xiob_status_e status)
{
    xiob_curr_status = status;
}

void xspace_inout_box_manager_register_ui_cb(xiob_status_change_cb_func cb)
{
    if(NULL == cb) {
        return;
    }

    xiob_status_change_cb = cb;
}

int32_t xspace_inout_box_manage_init(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xpsace_inout_box_manage_init_if, 0, 0, 0);

    return 0;
}
#endif  // (XSPACE_INOUT_BOX_MANAGER)

