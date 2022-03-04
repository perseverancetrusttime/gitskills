#ifndef __XSPACE_SNR_WNR_TWS_SWITCH_H__
#define __XSPACE_SNR_WNR_TWS_SWITCH_H__

#if defined(__XSPACE_UI__) && defined(__XSPACE_SNR_WNR_TWS_SWITCH__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "tgt_hardware.h"

#define __XSPACE_SNR_TS_DEBUG__

#if 0//defined(__XSPACE_SNR_TS_DEBUG__)
#define XSNRTS_TRACE_ENTER(...)        TRACE(1, "[XSNRTS]%s <<<", __func__)
#define XSNRTS_TRACE(num, str, ...)    TRACE(num + 1, "[XSNRTS]%s," str, __func__, ##__VA_ARGS__)
#define XSNRTS_TRACE_EXIT(...)         TRACE(1, "[XSNRTS]%s >>>", __func__)
#else
#define XSNRTS_TRACE_ENTER(...)
#define XSNRTS_TRACE(...)
#define XSNRTS_TRACE_EXIT(...)
#endif

#define XSNRTS_STABLE_COUNT     (30)
#define XSNRTS_MONITOR_TIMEOUT  (3000)
#define XSNRTS_SNR_LIMIT        (800)
#define XSNRTS_WNR_LIMIT        (600)

typedef struct {
    uint32_t index;
    uint8_t count;
    int16_t data[XSNRTS_STABLE_COUNT];
} xsnrts_data_s;

typedef struct{
    uint8_t is_run;
    uint8_t wind_status;
    int16_t wind_noise;
    //int16_t snr;
} xsnrts_sys_info_s;

typedef struct{
    xsnrts_sys_info_s local_info;
    xsnrts_sys_info_s peer_info;
} xsnrts_info_s;

void xspace_snr_wnr_data_fill(void);
void xspace_snr_wnr_tws_switch_init(void);
void xspace_snr_wnr_tws_switch_detect_start(void);
void xspace_snr_wnr_tws_switch_detect_stop(void);
bool xspace_snr_wnr_tws_switch_is_need(void);
bool xspace_snr_wnr_is_allow_other_case_tws_switch(void);
void xspace_snr_wnr_tws_recv_sync_info(uint8_t *data, uint8_t len);
bool xspace_get_ear_wind_status(void);   //longz add 

#ifdef __cplusplus
}
#endif

#endif /* defined(__XSPACE_UI__) && defined(__XSPACE_SNR_WNR_TWS_SWITCH__) */

#endif /* __XSPACE_SNR_WNR_TWS_SWITCH_H__ */

