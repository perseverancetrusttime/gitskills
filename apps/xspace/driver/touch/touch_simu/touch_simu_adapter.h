#ifndef __TOUCH_SIMU_ADAPTER_H__
#define __TOUCH_SIMU_ADAPTER_H__

#if defined (__TOUCH_SIMULATION__)
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIMU_TRACE(num, str, ...)  TRACE(num+1, "[simu_touch] %s," str, __func__, ##__VA_ARGS__)

int32_t simu_adapter_report_inear_status(bool is_wearing);
int32_t simu_adapter_report_gesture_event(uint8_t key_type);

#ifdef __cplusplus
}
#endif

#endif  /* __TOUCH_simu__ */

#endif

