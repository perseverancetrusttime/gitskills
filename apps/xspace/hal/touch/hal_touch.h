#ifndef __HAL_TOUCH_H__
#define __HAL_TOUCH_H__

#if defined(__TOUCH_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HAL_TOUCH_DETACH_EAR = 0,
    HAL_TOUCH_IN_EAR
} hal_touch_inear_status_e;

typedef enum
{
    HAL_TOUCH_EVENT_NONE = 0,
    HAL_TOUCH_EVENT_CLICK = 1,
    HAL_TOUCH_EVENT_LONGPRESS = 2,
    HAL_TOUCH_EVENT_LONGPRESS_1S = 3,
    HAL_TOUCH_EVENT_LONGPRESS_3S = 4,
    HAL_TOUCH_EVENT_LONGPRESS_5S = 5,
    HAL_TOUCH_EVENT_DOUBLE_CLICK = 6,
    HAL_TOUCH_EVENT_TRIPLE_CLICK = 7,
    HAL_TOUCH_EVENT_ULTRA_CLICK = 8,
    HAL_TOUCH_EVENT_RAMPAGE_CLICK = 9,
    HAL_TOUCH_EVENT_UP_SLIDE = 10,
    HAL_TOUCH_EVENT_DOWN_SLIDE = 11,
    HAL_TOUCH_EVENT_LEFT_SLIDE = 12,
    HAL_TOUCH_EVENT_RIGHT_SLIDE = 13,  
    HAL_TOUCH_EVENT_SEND_DATA = 14,
	HAL_TOUCH_EVENT_PRESS = 21,
	HAL_TOUCH_EVENT_RELEASE = 22,
} hal_touch_event_e;


typedef void (*hal_touch_get_rawdata_cb)(uint8_t data);
typedef void (*hal_touch_inear_status_cb)(hal_touch_inear_status_e status);
typedef void (*hal_touch_gesture_event_cb)(hal_touch_event_e event, uint8_t *data, uint16_t len);

typedef struct
{      
    int32_t (* touch_init)(void);
    int32_t (* touch_reset)(void);
    int32_t (* touch_enter_standby_mode)(void);
    int32_t (* touch_enter_detection_mode)(void);
    int32_t (* touch_stop_inear_detection)(void);
    int32_t (* touch_start_inear_detection)(void);
    int32_t (* touch_get_chip_id)(uint32_t *chip_id);
    int32_t (* touch_set_sensibility_grade)(uint8_t grade);
    int32_t (* touch_set_inear_status_cb)(hal_touch_inear_status_cb cb);
    int32_t (* touch_set_gesture_events_cb)(hal_touch_gesture_event_cb cb);
    int32_t (* touch_off_set_self_cali)(void);
    int32_t (* touch_spp_rx_data_handler)(uint8_t *buffer, uint8_t length);
} hal_touch_ctx_s;


int32_t hal_touch_init(void);
int32_t hal_touch_reset(void);
int32_t hal_touch_enter_standby_mode(void);
int32_t hal_touch_enter_detection_mode(void);
int32_t hal_touch_stop_inear_detection(void);
int32_t hal_touch_start_inear_detection(void);
int32_t hal_touch_get_chip_id(uint32_t *chip_id);
int32_t hal_touch_set_sensibility_grade(uint8_t grade);
int32_t hal_touch_register_gesture_generate_cb(hal_touch_gesture_event_cb cb);
int32_t hal_touch_register_inear_status_changed_cb(hal_touch_inear_status_cb cb);
int32_t hal_touch_off_set_self_cali(void);
int32_t hal_touch_spp_rx_data_handler(uint8_t *buffer, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif //__TOUCH_SUPPORT__

#endif //__HAL_TOUCH_H__

