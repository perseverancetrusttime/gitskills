#ifndef __XSPACE_TWS_SWITCH_PROCESS_H__
#define __XSPACE_TWS_SWITCH_PROCESS_H__

#if defined(__XSPACE_UI__) && defined(__XSPACE_TWS_SWITCH__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "tgt_hardware.h"

#define __XSPACE_TWS_SWTICH_DEBUG__

#if 0//defined(__XSPACE_TWS_SWTICH_DEBUG__)
#define XTS_TRACE_ENTER(...)        TRACE(1, "[XST]%s <<<", __func__)
#define XTS_TRACE(num, str, ...)    TRACE(num + 1, "[XST]%s," str, __func__, ##__VA_ARGS__)
#define XTS_TRACE_EXIT(...)         TRACE(1, "[XST]%s >>>", __func__)
#else
#define XTS_TRACE_ENTER(...)
#define XTS_TRACE(...)
#define XTS_TRACE_EXIT(...)
#endif

#define XTS_MONITOR_SHROT_TIMEOUT  (5000)
#define XTS_MONITOR_LONG_TIMEOUT   (15000)

typedef enum {
    XTS_DETECT_RSSI,
    XTS_DETECT_SNR,
    XTX_DETECT_LOW_BATTERY,
} xts_detect_type_e;

void xspace_tws_switch_process_init(void);
void xspace_tws_switch_detect_start(void);
void xspace_tws_switch_detect_stop(void);
void xspace_tws_switch_call_active_process(void);
void xspace_tws_swtich_operat_detect_type(uint8_t type, bool opt);

#ifdef __cplusplus
}
#endif

#endif /* defined(__XSPACE_UI__) && defined(__XSPACE_TWS_SWITCH__) */

#endif /* __XSPACE_TWS_SWITCH_PROCESS_H__ */

