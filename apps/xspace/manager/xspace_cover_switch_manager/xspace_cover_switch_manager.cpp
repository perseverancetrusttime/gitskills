#if defined(__XSPACE_COVER_SWITCH_MANAGER__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "dev_thread.h"
#include "tgt_hardware.h"
#include "xspace_interface.h"
#include "xspace_cover_switch_manager.h"

#define COVER_STATUS_INT_DEBOUNCE_MS                  (200)
#define COVER_STATUS_OPEN_TIMER_DELAY_MS              (50)
#define COVER_STATUS_CLOSE_TIMER_DELAY_MS             (500)

xcsm_cb_func xscm_ui_cb = NULL;
static osTimerId xcsm_debounce_timer = NULL;
xcsm_ui_need_execute_func xcsm_ui_need_execute_handle = NULL;
static xcsm_status_e xcsm_cover_status = XCSM_BOX_STATUS_UNKNOWN;
static struct HAL_GPIO_IRQ_CFG_T xcsm_cover_status_gpiocfg;

#if defined(__XSPACE_AUTO_TEST__)
static uint8_t simu_hall_gpio_val = 0;
#endif


static void xspace_cover_switch_manager_debounce_timeout_handler(void const *param);
osTimerDef (XCSM_DEBOUNCE_TIMER, xspace_cover_switch_manager_debounce_timeout_handler);

void xspace_cover_switch_manager_set_int_trig_mode(void)
{
    static uint8_t pin_last_status = 255;
    uint8_t pin_status = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin);

    if(pin_last_status == pin_status) {
        return;
    }
    pin_last_status = pin_status;

    if (pin_status) {
        xcsm_cover_status_gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    } else {
        xcsm_cover_status_gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
    }
	xcsm_cover_status_gpiocfg.irq_enable = true;
    hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin, &xcsm_cover_status_gpiocfg);
}

static void xspace_cover_switch_manager_status_changed(void)
{
#if defined (__XSPACE_AUTO_TEST__)
    uint8_t pin_status = simu_hall_gpio_val;
#else
    uint8_t pin_status = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin);
#endif
    xcsm_status_e cover_status = (pin_status == 1) ? (XCSM_BOX_STATUS_OPENED) : (XCSM_BOX_STATUS_CLOSED);

#if !defined (__XSPACE_AUTO_TEST__)
    xspace_cover_switch_manager_set_int_trig_mode();
#endif
    if(cover_status == xcsm_cover_status) {
        XCSM_TRACE(0, " Invaild Event!!!");
        return;
    }

    xcsm_cover_status = cover_status;
    XCSM_TRACE(1, " Box %s!!!", (xcsm_cover_status == XCSM_BOX_STATUS_OPENED)?"Opened":"Closed");
    if (NULL != xcsm_ui_need_execute_handle) {
        if (!xcsm_ui_need_execute_handle(xcsm_cover_status)) {
            return;
        }
    }

    if(xscm_ui_cb != NULL) {
        xscm_ui_cb(xcsm_cover_status);
    }
}

static void xspace_cover_switch_manager_debounce_timeout_handler(void const *param)
{
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_cover_switch_manager_status_changed, 0, 0, 0);
}

static void xspace_cover_switch_manager_start_deounce_timer(uint16_t duration)
{
    osTimerStop(xcsm_debounce_timer);
    osTimerStart(xcsm_debounce_timer, duration);
}

static void xspace_cover_switch_manager_int_handler(enum HAL_GPIO_PIN_T pin)
{
    static uint32_t prev_ms = 0;
    uint32_t curr_ms = GET_CURRENT_MS();

    xcsm_cover_status_gpiocfg.irq_enable = false;
    hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin, &xcsm_cover_status_gpiocfg);

#if defined (__XSPACE_AUTO_TEST__)
    uint8_t pin_status = simu_hall_gpio_val;
#else
    uint8_t pin_status = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin);
#endif
    xcsm_status_e cover_status = (pin_status == 1) ? (XCSM_BOX_STATUS_OPENED) : (XCSM_BOX_STATUS_CLOSED);

    XCSM_TRACE(2, " cover status=%d diff_tick=%d\n", cover_status, (curr_ms-prev_ms));
    if ((prev_ms == 0) ||(curr_ms-prev_ms > COVER_STATUS_INT_DEBOUNCE_MS)) {
        prev_ms = curr_ms;

        if (cover_status == XCSM_BOX_STATUS_OPENED) {
            xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_cover_switch_manager_start_deounce_timer,
                                        COVER_STATUS_OPEN_TIMER_DELAY_MS, 0, 0);
        } else if(cover_status == XCSM_BOX_STATUS_CLOSED) {
            xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xspace_cover_switch_manager_start_deounce_timer,
                                        COVER_STATUS_CLOSE_TIMER_DELAY_MS, 0, 0);
        }
    }

#if !defined (__XSPACE_AUTO_TEST__)
    xspace_cover_switch_manager_set_int_trig_mode();
#endif
}

#if defined(__XSPACE_AUTO_TEST__)
void config_simu_hall_gpio_val(uint8_t val)
{
    XCSM_TRACE(1, "val:%d(0:close,1:open).", val);

    if(simu_hall_gpio_val == val)
    {
        return;
    }
    simu_hall_gpio_val = val;

    xspace_cover_switch_manager_int_handler((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin);
}
#endif

static void xspace_cover_switch_manager_init_if(void)
{
    xcsm_debounce_timer = osTimerCreate (osTimer(XCSM_DEBOUNCE_TIMER), osTimerOnce, NULL);

#if defined (__XSPACE_AUTO_TEST__)
    config_simu_hall_gpio_val(1);
#else
    if (cover_status_int_cfg.pin < HAL_IOMUX_PIN_NUM) {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cover_status_int_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin, HAL_GPIO_DIR_IN, 0);

        xcsm_cover_status_gpiocfg.irq_enable = false;
        xcsm_cover_status_gpiocfg.irq_debounce = false;
        xcsm_cover_status_gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;
        xcsm_cover_status_gpiocfg.irq_handler = xspace_cover_switch_manager_int_handler;
        //xspace_cover_switch_manager_set_int_trig_mode();
        xspace_cover_switch_manager_int_handler((enum HAL_GPIO_PIN_T)cover_status_int_cfg.pin);
    }
#endif

    XCSM_TRACE(0, " Init success");
}

xcsm_status_e xspace_cover_switch_manage_get_status(void)
{
    return xcsm_cover_status;
}

void xspace_cover_switch_manage_set_status(xcsm_status_e status)
{
    xcsm_cover_status = status;
}

void xspace_cover_switch_manage_register_ui_cb(xcsm_cb_func cb)
{
    if (NULL != cb) {
        xscm_ui_cb = cb;
    }
}

void xspace_cover_switch_manage_register_manage_need_execute_cb(xcsm_ui_need_execute_func cb)
{
    if (NULL == cb) {
        return;
    }

    xcsm_ui_need_execute_handle = cb;
}

void xspace_cover_switch_manager_init(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_cover_switch_manager_init_if, 0, 0, 0);
}
#endif  // (__XSPACE_COVER_SWITCH_MANAGER__)

