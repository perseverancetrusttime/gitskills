#ifndef __XSPACE_GESTURE_MANAGER_H__
#define __XSPACE_GESTURE_MANAGER_H__

#if defined(__XSPACE_GESTURE_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#if defined(__XSPACE_GM_DEBUG__)
#define XGM_TRACE(num, str, ...)  TRACE(num+2, "[XGM] %s,line=%d," str, __func__, __LINE__, ##__VA_ARGS__)
#else
#define XGM_TRACE(num, str, ...)
#endif

typedef enum
{
    XGM_EVENT_NONE = 0,
    XGM_EVENT_CLICK = 1,
    XGM_EVENT_DOUBLE_CLICK = 2,
    XGM_EVENT_TRIPLE_CLICK = 3,
    XGM_EVENT_ULTRA_CLICK = 4,
    XGM_EVENT_RAMPAGE_CLICK = 5,
    XGM_EVENT_LONGPRESS = 6,
    XGM_EVENT_LONGLONGPRESS = 7,
    XGM_EVENT_UP_SLIDE = 8,
    XGM_EVENT_DOWN_SLIDE = 9,
    XGM_EVENT_UP_SLIDE_LONGPRESS = 10,
    XGM_EVENT_DOWN_SLIDE_LONGPRESS = 11,
    XGM_EVENT_UP = 12,
    XGM_EVENT_DOWN = 13,
    APP_GESTURE_MANAGE_EVENT_DEBUG_SEND_DATA = 14,
    APP_GESTURE_MANAGE_EVENT_SEND_ACC_DATA = 15,
} xgm_event_type_e;

typedef enum
{
    XGM_MODE_UNKNOWN = 0,
    XGM_DETECTION_MODE,
    XGM_STANDBY_MODE,
} xgm_work_status_e;

typedef void (*xgm_event_func)(xgm_event_type_e event_type, uint8_t *data, uint16_t len);
typedef bool (*xgm_ui_need_execute_func)(xgm_event_type_e event_type, uint8_t *data, uint16_t len);


int32_t xspace_gesture_manage_init(void);
void xspace_gesture_manage_enter_standby_mode(void);
void xspace_gesture_manage_enter_detection_mode(void);
void xspace_gesture_manage_register_ui_cb(xgm_event_func cb);
xgm_work_status_e xspace_gesture_manage_get_work_status(void);
int32_t xspace_gesture_manage_revise_sensibility(uint8_t grade);
void xspace_gesture_manage_register_manage_need_execute_cb(xgm_ui_need_execute_func cb);
#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
int32_t xspace_gesture_manage_pressure_cali(uint8_t items,uint8_t *data);
#endif


#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_GESTURE_MANAGER__)

#endif  // (__XSPACE_GESTURE_MANAGER_H__)

