#ifndef __HAL_PRESSURE_H__
#define __HAL_PRESSURE_H__

#if defined(__PRESSURE_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HAL_PRESSURE_EVENT_NONE = 0,
    HAL_PRESSURE_EVENT_CLICK = 1,
    HAL_PRESSURE_EVENT_DOUBLE_CLICK = 2,
    HAL_PRESSURE_EVENT_THREE_CLICK = 3,
    HAL_PRESSURE_EVENT_MAX_CLICK = HAL_PRESSURE_EVENT_THREE_CLICK,
    HAL_PRESSURE_EVENT_FOUR_CLICK = 4,
    HAL_PRESSURE_EVENT_FIVE_CLICK = 5,
    HAL_PRESSURE_EVENT_SIX_CLICK = 6,
    HAL_PRESSURE_EVENT_SEVEN_CLICK = 7,
    HAL_PRESSURE_EVENT_LONGPRESS = 11,
    HAL_PRESSURE_EVENT_LONGPRESS_1S = 12,
    HAL_PRESSURE_EVENT_LONGPRESS_3S = 13,
    HAL_PRESSURE_EVENT_LONGPRESS_5S = 14,
	HAL_PRESSURE_EVENT_PRESS = 21,
	HAL_PRESSURE_EVENT_RELEASE = 22,
} hal_pressure_event_e;


typedef void (*hal_pressure_gesture_event_cb)(hal_pressure_event_e event, uint8_t *data, uint16_t len);
typedef struct
{      
    int32_t (* pressure_init)(void);
    int32_t (* pressure_reset)(void);
    int32_t (* pressure_get_chip_id)(uint32_t *chip_id);
    int32_t (* pressure_enter_standby_mode)(void);
    int32_t (* pressure_enter_detection_mode)(void);
    int32_t (* pressure_set_mode)(uint8_t mode);
    int32_t (* pressure_set_sensitivity)(uint8_t leve);
    int32_t (* pressure_set_gesture_events_cb)(hal_pressure_gesture_event_cb cb);
    int32_t (* pressure_enter_pt_calibration)(uint8_t items,uint8_t *data);
} hal_pressure_ctx_s;

int32_t hal_pressure_init(void);
int32_t hal_pressure_reset(void);
int32_t hal_pressure_get_chip_id(uint32_t *chip_id);
int32_t hal_pressure_enter_standby_mode(void);
int32_t hal_pressure_enter_detection_mode(void);
int32_t hal_pressure_register_gesture_generate_cb(hal_pressure_gesture_event_cb cb);
int32_t hal_pressure_enter_pt_calibration(uint8_t items,uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif //__PRESSURE_SUPPORT__

#endif //__HAL_PRESSURE_H__

