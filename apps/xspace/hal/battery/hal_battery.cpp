#if defined(__XSPACE_BATTERY_MANAGER__)
#include "hal_battery.h"
#include "hal_error.h"
#include "drv_bat_adc.h"

static hal_bat_ctrl_if_s *p_bat_ctrl_if;

#ifdef __COULOMB_COUNTER_SUPPORT__

#ifdef __COULOMB_COUNTER_MAX17050__
extern const hal_bat_ctrl_if_s max17050_coulomb_ctrl;
#elif defined (__COULOMB_COUNTER_CW2015__)
extern const hal_bat_ctrl_if_s cw2015_coulomb_ctrl;
#endif

#endif

int32_t hal_bat_info_init(void)
{
#if defined __COULOMB_COUNTER_MAX17050__
    p_bat_ctrl_if = (hal_bat_ctrl_if_s *)&max17050_coulomb_ctrl;
    if(!p_bat_ctrl_if->init()) {
        return HAL_RUN_SUCCESS;
    }
#endif
    
#if defined __COULOMB_COUNTER_CW2015__
    p_bat_ctrl_if = (hal_bat_ctrl_if_s *)&cw2015_coulomb_ctrl;
    if(!p_bat_ctrl_if->init()) {
        return HAL_RUN_SUCCESS;
    }
#endif

#if defined(__BAT_ADC_INFO_GET__)
    p_bat_ctrl_if = (hal_bat_ctrl_if_s *)&bat_adc_ctrl;
    if(!p_bat_ctrl_if->init()) {
        return HAL_RUN_SUCCESS;
    }
#endif

    return HAL_INIT_ERROR;
}

int32_t hal_bat_info_get_chip_id(uint32_t *chip_id)
{
    if (p_bat_ctrl_if != NULL && p_bat_ctrl_if->get_chip_id != NULL)
    {
        return p_bat_ctrl_if->get_chip_id(chip_id);
    }

    return HAL_INVALID_PARAMETER;
}


int32_t hal_bat_info_auto_calib(uint32_t value)
{
    if (p_bat_ctrl_if != NULL && p_bat_ctrl_if->auto_calib != NULL)
    {
        return p_bat_ctrl_if->auto_calib(value);
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_bat_info_set_charging_status(hal_bat_charging_status_e status)
{
    if (p_bat_ctrl_if != NULL && p_bat_ctrl_if->set_charging_status != NULL)
    {
        return p_bat_ctrl_if->set_charging_status(status);
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_bat_info_set_query_bat_info_cb(hal_bat_query_cb cb)
{
    if (p_bat_ctrl_if != NULL && p_bat_ctrl_if->set_bat_info_query_cb != NULL)
    {
        return p_bat_ctrl_if->set_bat_info_query_cb(cb);
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_bat_info_query_bat_info(void)
{
    if (p_bat_ctrl_if != NULL && p_bat_ctrl_if->query_bat_info != NULL)
    {
        return p_bat_ctrl_if->query_bat_info();
    }

    return HAL_INVALID_PARAMETER;
}

#endif /* __XSPACE_BATTERY_MANAGER__ */

