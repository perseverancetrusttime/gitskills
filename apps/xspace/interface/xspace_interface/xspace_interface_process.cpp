#include "cmsis.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "app_thread.h"
#include "xspace_interface_process.h"

#define XIF_TRACE(num, str, ...)    TRACE(num + 1, "[XIF]%s," str, __func__, ##__VA_ARGS__)
#define XIF_ASSERT                  ASSERT

static osTimerId xif_delay_timer = NULL;
static uint32_t xif_delay_timer_status_max = 0;
static void xspace_interface_process_delay_timeout_handler(void const *param);
osTimerDef(XIF_DELAY_TIMEOUT_TIMER, xspace_interface_process_delay_timeout_handler);

#define XIF_DELAY_TIMER_EVENT_MAX 32

static uint8_t xif_diff_timeout_flag = 0;
static uint32_t xif_delay_timer_status = 0;
static xif_delay_timer_s xif_delay_timer_array[XIF_DELAY_TIMER_EVENT_MAX];
static xif_delay_timer_s *xif_next_timer = xif_delay_timer_array;
//static uint32_t xif_delay_timer_status_max = 0;

static uint32_t sdk_interface_get_current_ms(void){
    return GET_CURRENT_MS();
}


static int xspace_interface_process_event_handle(APP_MESSAGE_BODY *msg_body)
{
    switch (msg_body->message_id) {
        case XIF_EVENT_FUNC_CALL:
            if (msg_body->message_ptr) {
                ((xif_process_func_cb)(msg_body->message_ptr))(msg_body->message_Param0, msg_body->message_Param1, msg_body->message_Param2);
            }
            break;

        default:
            break;
    }

    return 0;
}

static void xspace_interface_process_delay_timeout_handler(void const *param)
{
    xif_delay_timer_s *timer = xif_delay_timer_array;
    uint32_t current_tick = sdk_interface_get_current_ms();
    uint32_t diff_tick = 0;

    XIF_TRACE(4, " 0x%x,%d,%d,%d.0x%08x", xif_delay_timer_status, current_tick, xif_diff_timeout_flag, xif_next_timer->en, xif_next_timer->ptr);

    if (xif_diff_timeout_flag == 0) {
        if (xif_next_timer->en) {
            xif_delay_timer_status &= ~(1 << xif_next_timer->timer_id);
            xspace_interface_process_event_process(XIF_EVENT_FUNC_CALL, xif_next_timer->ptr, xif_next_timer->id, xif_next_timer->param0,
                                                   xif_next_timer->param1);
            memset(xif_next_timer, 0x00, sizeof(xif_delay_timer_s));
        }
    }

    if (xif_delay_timer_status != 0) {
        timer = xif_delay_timer_array;
        for (uint8_t i = 0; i < sizeof(xif_delay_timer_array) / sizeof(xif_delay_timer_array[0]); i++) {
            if (timer->en) {
                if ((xif_next_timer->en == 0) || (xif_next_timer->end_tick > timer->end_tick)) {
                    xif_next_timer = timer;
                }

                if ((xif_next_timer != timer) && (xif_next_timer->end_tick == timer->end_tick)) {
                    timer->end_tick += 20;
                }
            }

            timer++;
        }
    }

    if (xif_next_timer->en) {
        current_tick = sdk_interface_get_current_ms();
        if (xif_next_timer->end_tick <= current_tick) {
            if (xif_next_timer->end_tick + 100 < current_tick) {
                XIF_TRACE(5, "Warning: xif_next_timer->id = %d,ptr:0x%x,start_tick:%d,end_tick:%d,current_tick:%d.", xif_next_timer->timer_id,
                          xif_next_timer->ptr, xif_next_timer->start_tick, xif_next_timer->end_tick, current_tick);
            }
            xif_next_timer->end_tick = current_tick + 100;
        }

        diff_tick = xif_next_timer->end_tick - current_tick;
        if (diff_tick > XIF_DIFF_TICK_MAX) {
            xif_diff_timeout_flag = 1;
            diff_tick = XIF_DIFF_TICK_MAX;
        } else {
            xif_diff_timeout_flag = 0;
        }

        osStatus status = osTimerStop(xif_delay_timer);
        if (status != osOK && status != osErrorResource) {
            XIF_TRACE(1, " error:osTimerStop failed,0x%x.", status);
        }
        status = osTimerStart(xif_delay_timer, diff_tick);
        if (status != osOK) {
            XIF_TRACE(1, " error:osTimerStart failed,0x%x.", status);
        }
    }
}

int xspace_interface_process_delay_timer_start(uint32_t time, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1)
{
    xif_delay_timer_s *timer = xif_delay_timer_array;
    uint32_t current_tick = sdk_interface_get_current_ms();
    uint8_t register_flag = 0;
    uint32_t diff_tick = 0;

    XIF_TRACE(2, " time:%d,ptr:0x%x.", time, ptr);

    if (xif_delay_timer_status == xif_delay_timer_status_max) {
        XIF_ASSERT(0, "too many timer event,xif_delay_timer_status = %d.", xif_delay_timer_status);
        return -1;
    }

    if (xif_delay_timer == NULL) {
        xif_delay_timer = osTimerCreate(osTimer(XIF_DELAY_TIMEOUT_TIMER), osTimerOnce, NULL);
    }

    timer = xif_delay_timer_array;
    for (uint8_t i = 0; i < sizeof(xif_delay_timer_array) / sizeof(xif_delay_timer_array[0]); i++) {
        if ((timer->en == 0) && (register_flag == 0)) {
            register_flag = 1;

            timer->en = 1;
            timer->timer_id = i;
            timer->start_tick = current_tick;
            timer->end_tick = time + current_tick;
            timer->ptr = ptr;
            timer->id = id;
            timer->param0 = param0;
            timer->param1 = param1;
            xif_delay_timer_status |= (1 << i);
        }

        if (timer->en) {
            if ((xif_next_timer->en == 0) || (xif_next_timer->end_tick > timer->end_tick)) {
                xif_next_timer = timer;
            }

            if ((xif_next_timer != timer) && (xif_next_timer->end_tick == timer->end_tick)) {
                timer->end_tick += 20;
            }
        }

        timer++;
    }

    if (xif_next_timer->en == 1) {
        current_tick = sdk_interface_get_current_ms();
        if (xif_next_timer->end_tick <= current_tick) {
            if (xif_next_timer->end_tick + 100 < current_tick) {
                XIF_TRACE(5, "Alarm:xif_next_timer->id:%d,ptr:0x%x,start_tick:%d,end_tick:%d,current_tick:%d.", xif_next_timer->timer_id, xif_next_timer->ptr,
                          xif_next_timer->start_tick, xif_next_timer->end_tick, current_tick);
            }
            xif_next_timer->end_tick = current_tick + 100;
        }

        diff_tick = xif_next_timer->end_tick - current_tick;
        if (diff_tick > XIF_DIFF_TICK_MAX) {
            xif_diff_timeout_flag = 1;
            diff_tick = XIF_DIFF_TICK_MAX;
        } else {
            xif_diff_timeout_flag = 0;
        }

        osStatus status = osTimerStop(xif_delay_timer);
        if (status != osOK && status != osErrorResource) {
            XIF_TRACE(1, " error:osTimerStop failed,0x%x.", status);
        }
        status = osTimerStart(xif_delay_timer, diff_tick);
        if (status != osOK) {
            XIF_TRACE(1, " error:osTimerStart failed,0x%x.", status);
        }
    }

    XIF_TRACE(6, " 0x%x,0x%x,%d,%d,%d,%d.", xif_delay_timer_status, ptr, current_tick, xif_diff_timeout_flag, time, diff_tick);

    return 0;
}

int xspace_interface_process_delay_timer_stop(uint32_t ptr)
{
    xif_delay_timer_s *timer = xif_delay_timer_array;
    uint32_t current_tick = sdk_interface_get_current_ms();
    uint8_t restart_flag = 0;
    uint32_t diff_tick = 0;

    XIF_TRACE(1, " ptr:0x%x.", ptr);

    if (xif_delay_timer == NULL) {
        xif_delay_timer = osTimerCreate(osTimer(XIF_DELAY_TIMEOUT_TIMER), osTimerOnce, NULL);
    }

    timer = xif_delay_timer_array;
    for (uint8_t i = 0; i < sizeof(xif_delay_timer_array) / sizeof(xif_delay_timer_array[0]); i++) {
        if (timer->ptr == ptr) {
            if ((timer->en == 1) && (timer->ptr == xif_next_timer->ptr) && (timer->id == xif_next_timer->id) && (timer->param0 == xif_next_timer->param0)
                && (timer->param1 == xif_next_timer->param1)) {
                restart_flag = 1;
            }

            xif_delay_timer_status &= ~(1 << timer->timer_id);
            memset(timer, 0x00, sizeof(xif_delay_timer_s));
        }
        timer++;
    }

    if ((xif_delay_timer_status != 0) && (restart_flag == 1)) {
        timer = xif_delay_timer_array;
        for (uint8_t i = 0; i < sizeof(xif_delay_timer_array) / sizeof(xif_delay_timer_array[0]); i++) {
            if (timer->en) {
                if ((xif_next_timer->en == 0) || (xif_next_timer->end_tick > timer->end_tick)) {
                    xif_next_timer = timer;
                }

                if ((xif_next_timer != timer) && (xif_next_timer->end_tick == timer->end_tick)) {
                    timer->end_tick += 20;
                }
            }
            timer++;
        }

        if (xif_next_timer->en == 1) {
            current_tick = sdk_interface_get_current_ms();
            if (xif_next_timer->end_tick <= current_tick) {
                if (xif_next_timer->end_tick + 100 < current_tick) {
                    XIF_TRACE(5, " Warning:xif_next_timer->id:%d,ptr:0x%x,start_tick:%d,end_tick:%d,current_tick:%d.", xif_next_timer->timer_id,
                              xif_next_timer->ptr, xif_next_timer->start_tick, xif_next_timer->end_tick, current_tick);
                }
                xif_next_timer->end_tick = current_tick + 100;
            }

            diff_tick = xif_next_timer->end_tick - current_tick;
            if (diff_tick > XIF_DIFF_TICK_MAX) {
                xif_diff_timeout_flag = 1;
                diff_tick = XIF_DIFF_TICK_MAX;
            } else {
                xif_diff_timeout_flag = 0;
            }

            osStatus status = osTimerStop(xif_delay_timer);
            if (status != osOK && status != osErrorResource) {
                XIF_TRACE(1, " error:osTimerStop failed,0x%x.", status);
            }
            status = osTimerStart(xif_delay_timer, diff_tick);
            if (status != osOK) {
                XIF_TRACE(1, " error:osTimerStart failed,0x%x.", status);
            }
        }
    }

    XIF_TRACE(3, " 0x%x,0x%x,%d.", xif_delay_timer_status, ptr, xif_diff_timeout_flag);

    return 0;
}

void xspace_interface_process_event_process(xif_event_e msg_id, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1)
{
    APP_MESSAGE_BLOCK msg;

    if (app_is_module_registered(APP_MODUAL_XSPACE_IF) == false) {
        XIF_TRACE(0, "APP_MODUAL_XSPACE_IF not register!!!");
        return;
    }

    msg.mod_id = APP_MODUAL_XSPACE_IF;
    msg.msg_body.message_id = (uint32_t)msg_id;
    msg.msg_body.message_ptr = (uint32_t)ptr;
    msg.msg_body.message_Param0 = (uint32_t)id;
    msg.msg_body.message_Param1 = (uint32_t)param0;
    msg.msg_body.message_Param2 = (uint32_t)param1;
    app_mailbox_put(&msg);
}

void xspace_interface_process_init(void)
{
    XIF_TRACE(0, " enter");

    xif_delay_timer_status_max = (uint32_t)pow(2, XIF_DELAY_TIMER_EVENT_MAX) - 1;

    app_set_threadhandle(APP_MODUAL_XSPACE_IF, xspace_interface_process_event_handle);
}
