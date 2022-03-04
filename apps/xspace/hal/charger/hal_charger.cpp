#if defined(__CHARGER_SUPPORT__)
#include "hal_charger.h"
#include "hal_error.h"
#include "cw6305_adapter.h"

static hal_charger_ctrl_s *p_charger_ctrl;

#if defined(__CHARGER_CW6305__)
extern const hal_charger_ctrl_s cw6305_ctrl;
#endif

#if defined(__CHARGER_SY5500__)
extern const hal_charger_ctrl_s sy5500_ctrl;
#endif

int32_t hal_charger_init(void)
{
    if(p_charger_ctrl != NULL)
        return HAL_RUN_SUCCESS;

#if defined(__CHARGER_CW6305__)
    p_charger_ctrl = (hal_charger_ctrl_s *)&cw6305_ctrl;
    if(!p_charger_ctrl->init()) {
        return HAL_RUN_SUCCESS;
    }
#elif defined(__CHARGER_SY5500__)
    p_charger_ctrl = (hal_charger_ctrl_s *)&sy5500_ctrl;
    if(!p_charger_ctrl->init()) {
        return HAL_RUN_SUCCESS;
    }
#endif

    p_charger_ctrl = NULL;
    return HAL_INIT_ERROR;
}

int32_t hal_charger_get_chip_id(uint32_t *chip_id)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->get_chip_id != NULL)
    {
        if(!p_charger_ctrl->get_chip_id(chip_id))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_get_charging_status(hal_charging_ctrl_e *status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->get_charging_status != NULL)
    {
        if(!p_charger_ctrl->get_charging_status(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_charging_status(hal_charging_ctrl_e status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charging_status != NULL)
    {
        if(!p_charger_ctrl->set_charging_status(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_get_charging_current(hal_charging_current_e *current)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->get_charging_current != NULL)
    {
        if(!p_charger_ctrl->get_charging_current(current))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_charging_current(hal_charging_current_e current)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charging_current != NULL)
    {
        if(!p_charger_ctrl->set_charging_current(current))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_register_charging_status_report_cb(hal_charger_status_cb cb)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->register_charging_status_report_cb != NULL)
    {
        if(!p_charger_ctrl->register_charging_status_report_cb(cb))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_get_charging_voltage(uint16_t *voltage)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->get_charging_voltage != NULL)
    {
        if(!p_charger_ctrl->get_charging_voltage(voltage))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_charging_voltage(uint16_t voltage)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charging_voltage != NULL)
    {
        if(!p_charger_ctrl->set_charging_voltage(voltage))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_enter_shutdown_mode(void)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->enter_shutdown_mode != NULL)
    {
        if(!p_charger_ctrl->enter_shutdown_mode())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_HiZ_status(hal_charging_ctrl_e status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_HiZ_status != NULL)
    {
        if(!p_charger_ctrl->set_HiZ_status(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_charging_watchdog(bool status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charging_watchdog != NULL)
    {
        if(!p_charger_ctrl->set_charging_watchdog(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_box_dummy_load_switch(uint8_t status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charging_box_dummy_load_switch != NULL)
    {
        if(!p_charger_ctrl->set_charging_box_dummy_load_switch(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_set_xbus_vsys_limit(uint8_t status)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_xbus_vsys_limit != NULL)
    {
        if(!p_charger_ctrl->set_xbus_vsys_limit(status))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_trx_mode_set(hal_charger_trx_mode_e mode)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->charger_trx_mode_set != NULL)
    {
        if(!p_charger_ctrl->charger_trx_mode_set(mode))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }
    return HAL_INVALID_PARAMETER;
}

int32_t hal_charger_polling_status(bool en)
{
    if (p_charger_ctrl != NULL && p_charger_ctrl->set_charger_polling_status != NULL)
    {
        if(!p_charger_ctrl->set_charger_polling_status(en))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL; 
    }
    return HAL_INVALID_PARAMETER;
}
#endif /* __CHARGER_SUPPORT__ */

