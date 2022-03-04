#ifndef __XSPACE_LOW_BATTERY_TWS_SWITCH_H__
#define __XSPACE_LOW_BATTERY_TWS_SWITCH_H__

#if defined(__XSPACE_UI__) && defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "tgt_hardware.h"

#define __XSPACE_LB_TWS_SWITCH_DEBUG__

#if defined(__XSPACE_LB_TWS_SWITCH_DEBUG__)
#define XLBTS_TRACE_ENTER(...)        TRACE(1, "[LBTS]%s <<<", __func__)
#define XLBTS_TRACE(num, str, ...)    TRACE(num + 1, "[LBTS]%s," str, __func__, ##__VA_ARGS__)
#define XLBTS_TRACE_EXIT(...)         TRACE(1, "[LBTS]%s >>>", __func__)
#else
#define XLBTS_TRACE_ENTER(...)
#define XLBTS_TRACE(...)
#define XLBTS_TRACE_EXIT(...)
#endif

#define XSPACE_LB_TWS_SWITCH_LIMIT  (10)

void xspace_low_battery_tws_switch(bool force);
bool xspace_low_battery_tws_swtich_force(void);
bool xspace_low_battery_tws_swtich_need(void);

#ifdef __cplusplus
}
#endif

#endif /* defined(__XSPACE_UI__) && defined(__XSPACE_LOW_BATTERY_TWS_SWITCH__) */

#endif /* __XSPACE_LOW_BATTERY_TWS_SWITCH_H__ */

