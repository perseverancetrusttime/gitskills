#ifndef __GH621X_ADAPTER_H__
#define __GH621X_ADAPTER_H__

#if defined (__TOUCH_GH621X__)

#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_touch.h"


#ifdef __cplusplus
extern "C" {
#endif

#define __GH621X_DEBUG__
#if defined(__GH621X_DEBUG__)
#define GH621X_TRACE(num, str, ...)  TRACE(num, "[GH621X] "str, ##__VA_ARGS__)
#else
#define GH621X_TRACE(num, str, ...)
#endif

extern uint16_t drv_gh621x_chipid;

typedef enum {
    GH621X_MSG_ID_INT,
    GH621X_MSG_ID_UART_RX_DATA,
    MSG_ID_CNT,
} gh61x_msg_id_e;

int8_t gh621x_adapter_thread_handle_create_status(void);
int32_t gh621x_adapter_report_inear_status(bool is_wearing);
int32_t gh621x_adapter_report_gesture_event(hal_touch_event_e key_type);
int32_t gh621x_adapter_send_data(hal_touch_event_e key_type,uint8_t* ptrData, uint32_t length);
void gh621x_adapter_event_process(uint32_t msgid, uint32_t *ptr, uint32_t param0, uint32_t param1);

#ifdef __cplusplus
}
#endif

#endif  /* __TOUCH_GH621X__ */

#endif

