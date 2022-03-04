#ifndef __XSPACE_BATTERY_MANAGER_H__
#define __XSPACE_BATTERY_MANAGER_H__

#if defined(__XSPACE_BATTERY_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"

#if defined(__XSPACE_BM_DEBUG__)
#define XBM_TRACE(num, str, ...)  TRACE(num+1, "[XBM]%s," str, __func__, ##__VA_ARGS__)
#else
#define XBM_TRACE(num, str, ...)
#endif
#define XBM_HIGHEST_TEMP_C                          (60) 
#define XBM_CHARGER_INT_DEBOUNCE_MS                 (50)
#define XBM_CHARGER_PLUGIN_TIMER_DELAY_MS           (150)
#define XBM_CHARGER_PLUGOUT_TIMER_DELAY_MS          (1000)
#define XBM_INIT_PENDING_DELAY_MS                   (100)

#define XBM_BAT_INFO_INIT_FLAG          (0x01 << 0)
#define XBM_NTC_INIT_FLAG               (0x01 << 1)
#define XBM_CHARGER_CTRL_INIT_FLAG      (0x01 << 2)
#define XBM_PMU_INIT_FLAG               (0x01 << 3)

typedef uint16_t XBM_VOLTAGE_T;

typedef struct {
    uint8_t pd_per_threshold;   // power down percentage threshold
    uint8_t low_per_threshold;  // lowest percentage threshold
    uint8_t higt_per_threshold; // highest percentage threshold
} xbm_bat_ui_config_s;

typedef enum {
    XBM_STATUS_INVALID,     // Invaild Mode
    XBM_STATUS_NORMAL,      // Normal Mode
    XBM_STATUS_CHARGING,    // Charging Mode
    XBM_STATUS_OVERVOLT,    // Over voltage Mode
    XBM_STATUS_UNDERVOLT,   // lowest volgage Mode
    XBM_STATUS_PDVOLT,      // Power Down Mode

    XBM_STATUS_QTY
} xbm_status_s;

typedef struct {
    xbm_status_s status;
    uint8_t curr_level;
    uint8_t curr_per;
    XBM_VOLTAGE_T curr_volt;
    uint8_t init_complete_status;
    xbm_bat_ui_config_s ui_config;
    uint8_t quick_charging_current;
    uint8_t slow_charging_current;
    int16_t temperature;
} xbm_ctx_s;

typedef struct {
    xbm_status_s status;
    uint8_t curr_level;
    uint8_t curr_per;
    int16_t temperature;
} xbm_ui_status_s;

typedef enum {
    XBM_CHARGING_CTRL_UNCHARGING,
    XBM_CHARGING_CTRL_ONE_FIFTH,
    XBM_CHARGING_CTRL_HALF,
    XBM_CHARGING_CTRL_1C,
    XBM_CHARGING_CTRL_2C,
    XBM_CHARGING_CTRL_3C,
    XBM_CHARGING_CTRL_4C,
    XBM_CHARGING_CTRL_5C,
    XBM_CHARGING_CTRL_6C,
    XBM_CHARGING_CTRL_7C,
    XBM_CHARGING_CTRL_8C,
    XBM_CHARGING_CTRL_9C,
    XBM_CHARGING_CTRL_10C,

    XBM_CHARGING_CTRL_TOTAL_NUM,
} xbm_charging_current_e;

typedef enum {
    XBM_CHARGING_CTRL_DISABLE,
    XBM_CHARGING_CTRL_ENABLE,
} xbm_charging_ctrl_e;


typedef void (*xbm_bat_info_ui_cb_func)(xbm_ui_status_s status);
typedef void (*xbm_charger_ctrl_ui_cb_func)(xbm_charging_current_e *charging_curr, uint16_t *charging_vol, xbm_charging_ctrl_e *charging_ctrl);

bool xspace_get_charger_debounce_timer_flag(void);
xbm_status_s xspace_get_battery_status(void);
void xspace_update_battery_status(void);
void     xspace_battery_manager_timing_todo(void);
uint8_t  xspace_battery_manager_bat_get_level(void);
uint16_t xspace_battery_manager_bat_get_voltage(void);
bool     xspace_battery_manager_bat_is_charging(void);
uint8_t  xspace_battery_manager_bat_get_percentage(void);
void     xspace_battery_manager_enter_shutdown_mode(void);
int16_t  xspace_battery_manager_get_curr_temperature(void);
void     xspace_battery_manager_set_curr_temperature(int16_t temp);
int32_t  xspace_battery_manager_init(xbm_bat_ui_config_s bat_ui_cofig);
void     xspace_battery_manage_register_bat_info_ui_cb(xbm_bat_info_ui_cb_func cb);
void     xspace_battery_manager_register_charger_ctrl_ui_cb(xbm_charger_ctrl_ui_cb_func cb);

#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_BATTERY_MANAGER__)

#endif  // (__XSPACE_BATTERY_MANAGER_H__)

