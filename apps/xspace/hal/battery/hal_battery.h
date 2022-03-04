#ifndef __XSPACE_HAL_BATTERY_H__
#define __XSPACE_HAL_BATTERY_H__

#if defined(__XSPACE_BATTERY_MANAGER__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

//battery info
#define HAL_BAT_MIN_MV	DRV_BAT_MIN_MV
#define HAL_BAT_PD_MV	DRV_BAT_PD_MV
#define HAL_BAT_LOW_MV	DRV_BAT_LOW_MV
#define HAL_BAT_MAX_MV	DRV_BAT_MAX_MV
#define HAL_BAT_LOW_PER	DRV_BAT_LOW_PER  /*battery low percentage*/

typedef enum {
    HAL_BAT_STATUS_DISCHARGING,
    HAL_BAT_STATUS_CHARGING,
    HAL_BAT_STATUS_UNKNOWN,
} hal_bat_charging_status_e;

typedef struct 
{
    uint32_t bat_volt;
    uint8_t bat_level;
    uint8_t bat_per;
}hal_bat_info_s;

typedef void (* hal_bat_query_cb)(hal_bat_info_s bat_info);

typedef struct
{
    int32_t (* init)(void);
    int32_t (* get_chip_id)(uint32_t *chip_id);
    int32_t (* auto_calib)(uint32_t value);
    int32_t (* set_charging_status)(hal_bat_charging_status_e status);
    int32_t (* set_bat_info_query_cb)(hal_bat_query_cb cb);
    int32_t (* query_bat_info)(void);
}hal_bat_ctrl_if_s;


int32_t hal_bat_info_init(void);
int32_t hal_bat_info_get_chip_id(uint32_t *chip_id);
int32_t hal_bat_info_auto_calib(uint32_t value);
int32_t hal_bat_info_set_charging_status(hal_bat_charging_status_e status);
int32_t hal_bat_info_set_query_bat_info_cb(hal_bat_query_cb cb);
int32_t hal_bat_info_query_bat_info(void);
#ifdef __cplusplus
}
#endif

#endif /*  __XSPACE_BATTERY_MANAGER__  */
#endif /*  __XSPACE_HAL_BATTERY_H__  */
