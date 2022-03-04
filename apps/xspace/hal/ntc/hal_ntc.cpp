#if defined(__NTC_SUPPORT__)
#include "hal_ntc.h"
#include "hal_error.h"

static hal_ntc_ctrl_s *p_ntc_ctrl = NULL;

extern hal_ntc_ctrl_s ntc_ctrl;

int32_t hal_ntc_init(void)
{
    p_ntc_ctrl = (hal_ntc_ctrl_s *)&ntc_ctrl;
    if(!p_ntc_ctrl->init()) {
        return HAL_RUN_SUCCESS;
    }

    return HAL_INIT_ERROR;
}

int32_t hal_ntc_get_chip_id(uint32_t *chip_id)
{
    if (p_ntc_ctrl != NULL && p_ntc_ctrl->get_chip_id != NULL) {
        return p_ntc_ctrl->get_chip_id(chip_id);
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_ntc_get_temperature(int16_t *temp)
{
    if (p_ntc_ctrl != NULL && p_ntc_ctrl->get_temperature != NULL) {
        return p_ntc_ctrl->get_temperature(temp);
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_ntc_start_oneshot_measure(void)
{
    if (p_ntc_ctrl != NULL && p_ntc_ctrl->start_oneshot_measure != NULL) {
        return p_ntc_ctrl->start_oneshot_measure();
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_ntc_register_auto_report_temperature_cb(hal_ntc_report_temperature_cb cb)
{
    if (p_ntc_ctrl != NULL && p_ntc_ctrl->set_auto_report_temperature_cb != NULL) {
        return p_ntc_ctrl->set_auto_report_temperature_cb(cb);
    }

    return HAL_INVALID_PARAMETER;
}

#endif /* __NTC_SUPPORT__ */

