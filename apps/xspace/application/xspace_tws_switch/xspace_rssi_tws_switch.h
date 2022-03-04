#ifndef __XSPACE_RSSI_TWS_SWITCH_H__
#define __XSPACE_RSSI_TWS_SWITCH_H__

#if defined(__XSPACE_UI__) && defined(__XSPACE_RSSI_TWS_SWITCH__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "tgt_hardware.h"

#define __XSPACE_RSSI_DEBUG__

#if 0//defined(__XSPACE_RSSI_DEBUG__)
#define XRSSI_TRACE_ENTER(...)        TRACE(1, "[XRSSI]%s <<<", __func__)
#define XRSSI_TRACE(num, str, ...)    TRACE(num + 1, "[XRSSI]%s," str, __func__, ##__VA_ARGS__)
#define XRSSI_TRACE_EXIT(...)         TRACE(1, "[XRSSI]%s >>>", __func__)
#else
#define XRSSI_TRACE_ENTER(...)
#define XRSSI_TRACE(...)
#define XRSSI_TRACE_EXIT(...)
#endif

#define XRSSI_PROCESS_MONITOR_TIMEOUT           (5000)
#define XRSSI_PROCESS_MONITOR_TWS_THREAD        (20)
#define XRSSI_PROCESS_MONITOR_MOBILE_THREAD     (20)
#define XRSSI_PROCESS_MONITOR_INVAILD_DATA      (100)
#define XRSSI_PROCESS_MONITOR_MIN_RSSI          (-100)
#define XRSSI_PROCESS_MONITOR_NEED_SYNC_RSSI    (-60)

typedef struct{
    uint8_t addr[6];
    int8_t rssi;
} xrssi_mobile_info;

typedef struct{
    int8_t tws_rssi;
    uint8_t mobile_count;
    xrssi_mobile_info mobile_rssi[2];
} xrssi_sys_info_s;

typedef struct{
    xrssi_sys_info_s local_rssi;
    xrssi_sys_info_s peer_rssi;
} xrssi_info_s;


void xspace_rssi_process_init(void);
bool xspace_rssi_value_vaild(void);
void xspace_rssi_process_detect_stop(void);
void xspace_rssi_process_detect_start(void);
void xspace_rssi_process_send_sync_info(uint8_t *data, uint8_t *len);
void xspace_rssi_process_sync_info(uint8_t *data, uint8_t len);
bool xspace_rssi_process_tws_switch_according_rssi_needed(void);
bool xspace_rssi_process_is_allowed_other_case_tws_switch(void);

#ifdef __cplusplus
}
#endif

#endif /* defined(__XSPACE_UI__) && defined(__XSPACE_RSSI_TWS_SWITCH__) */

#endif /* __XSPACE_RSSI_TWS_SWITCH_H__ */

