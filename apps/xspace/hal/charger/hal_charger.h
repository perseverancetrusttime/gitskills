#ifndef __HAL_CHARGER_H__
#define __HAL_CHARGER_H__

#if defined(__CHARGER_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_CHARGING_CTRL_UNCHARGING,
    HAL_CHARGING_CTRL_ONE_FIFTH,
    HAL_CHARGING_CTRL_HALF,
    HAL_CHARGING_CTRL_1C,
    HAL_CHARGING_CTRL_2C,
    HAL_CHARGING_CTRL_3C,
    HAL_CHARGING_CTRL_4C,
    HAL_CHARGING_CTRL_5C,
    HAL_CHARGING_CTRL_6C,
    HAL_CHARGING_CTRL_7C,
    HAL_CHARGING_CTRL_8C,
    HAL_CHARGING_CTRL_9C,
    HAL_CHARGING_CTRL_10C,

    HAL_CHARGING_CTRL_TOTAL_NUM,
} hal_charging_current_e;

typedef enum {
    HAL_CHARGING_CTRL_DISABLE,
    HAL_CHARGING_CTRL_ENABLE,
} hal_charging_ctrl_e;

typedef enum {
    HAL_CHARGING_STATUS_DISCHARGE,
    HAL_CHARGING_STATUS_CHARGING,
    HAL_CHARGING_STATUS_CHARGING_FULL,
    
} hal_charging_status_e;

typedef enum 
{
    HAL_CHARGER_TRX_MODE_UNKNOWN,
    HAL_CHARGER_TRX_MODE_OD, /*open-drain*/
    HAL_CHARGER_TRX_MODE_PP, /*push-pull*/
    HAL_CHARGER_TRX_MODE_NUM,
}hal_charger_trx_mode_e;
typedef void (* hal_charger_status_cb)(hal_charging_status_e status);

typedef struct
{

    /*
    * Select the charging IC driver and initialize the IC.
    * void:
    * return: 0: initialize success, -1: initialize fail.
    */     
    int32_t (* init)(void);

    /*
    * Get the chip ID of the charging IC.
    * chip_id: memory the chip ID
    * return: 0: get success, -1: get fail.
    */    
    int32_t (* get_chip_id)(uint32_t *chip_id);

    /*
    * Get the current charging current.
    * current: See the enum of hal_charging_current_e.
    * return: 0: get success, -1: get fail.
    */    
    int32_t (* get_charging_current)(hal_charging_current_e *current);

    /*
    * Set charging current.
    * current: See the enum of hal_charging_current_e.
    * return: 0: set success, -1: set fail.
    */    
    int32_t (* set_charging_current)(hal_charging_current_e current);

    /*
    * Get the current charging status.
    * status: HAL_CHARGING_CTRL_DISENABLE or HAL_CHARGING_CTRL_ENABLE
    * return: 0: get success, -1: get fail.
    */    
    int32_t (* get_charging_status)(hal_charging_ctrl_e *status);

    /*
    * Control the charge IC charge or not charge
    * status: HAL_CHARGING_CTRL_DISENABLE or HAL_CHARGING_CTRL_ENABLE
    * return: 0: set success, -1: set fail.
    */
    int32_t (* set_charging_status)(hal_charging_ctrl_e status);

    /*
    * Register the callback fucntion when the charging status changed.
    * cb: the function point will called when the charging status changed.
    * return: 0: register success, -1: rgister fail.
    */
    int32_t (* register_charging_status_report_cb)(hal_charger_status_cb cb);
    
    /*
    * Get the current charging voltage.
    * voltage: See the enum of hal_charging_voltage_enum.
    * return: 0: get success, -1: get fail.
    */    
    int32_t (* get_charging_voltage)(uint16_t *voltage);

    /*
    * Set charging voltage.
    * voltage: See the enum of hal_charging_voltage_enum.
    * return: 0: set success, -1: set fail.
    */    
    int32_t (* set_charging_voltage)(uint16_t voltage);

    /*
    * enter shutdown mode.
    * .
    * return: 0: set success, -1: set fail.
    */ 	
    int32_t (* enter_shutdown_mode)(void);

	/*
	* Control the charge IC HiZ on the VBUS to make the Vbus more stable.
	* status£ºHAL_CHARGING_CTRL_DISENABLE or HAL_CHARGING_CTRL_ENABLE
	* return£º0£ºset success,-1£ºset fail.
	*/
	int32_t (*set_HiZ_status)(hal_charging_ctrl_e status);

    /*
    * Set charging watchdog.Used as hard reset
    * status: enable/disable.
    * return: 0: set success, -1: set fail.
    */    
    int32_t (* set_charging_watchdog)(bool status);

    /*
    * Set charging box dummy load switch
    * status: enable/disable.
    * return: 0: set success, -1: set fail.
    */    
    int32_t (* set_charging_box_dummy_load_switch)(uint8_t status);


	int32_t (* set_xbus_vsys_limit)(uint8_t status);
	int32_t (* charger_trx_mode_set)(hal_charger_trx_mode_e mode);
	
    /*
    * Set polling charger status timer
    * en: enable/disable.
    * return: 0: set success, -1: set fail.
    */    
	int32_t (* set_charger_polling_status)(bool en);
} hal_charger_ctrl_s;

int32_t hal_charger_init(void);
int32_t hal_charger_enter_shutdown_mode(void);
int32_t hal_charger_get_chip_id(uint32_t *chip_id);
int32_t hal_charger_set_charging_watchdog(bool status);
int32_t hal_charger_set_charging_voltage(uint16_t voltage);
int32_t hal_charger_get_charging_voltage(uint16_t *voltage);
int32_t hal_charger_set_HiZ_status(hal_charging_ctrl_e status);
int32_t hal_charger_set_charging_status(hal_charging_ctrl_e status);
int32_t hal_charger_get_charging_status(hal_charging_ctrl_e *status);
int32_t hal_charger_set_charging_current(hal_charging_current_e current);
int32_t hal_charger_get_charging_current(hal_charging_current_e *current);
int32_t hal_charger_set_box_dummy_load_switch(uint8_t status);
int32_t hal_charger_set_xbus_vsys_limit(uint8_t status);
int32_t hal_charger_register_charging_status_report_cb(hal_charger_status_cb cb);
int32_t hal_charger_trx_mode_set(hal_charger_trx_mode_e mode);
int32_t hal_charger_polling_status(bool en);

#ifdef __cplusplus
}
#endif

#endif /* __CHARGER_SUPPORT__ */

#endif /* __HAL_CHARGING_CTRL_H__ */

