#if defined(__PRESSURE_SUPPORT__)
#include "hal_pressure.h"
#include "hal_error.h"

/*********    Variable declaration area START   *********/
#ifdef __PRESSURE_AW8680X__
extern const hal_pressure_ctx_s c_aw8680x_pressure_ctx;
#endif
#ifdef __PRESSURE_CSA37F71__
extern const hal_pressure_ctx_s c_csa37f71_pressure_ctx;
#endif
#ifdef __PRESSURE_NEXTINPUT__
extern const hal_pressure_ctx_s c_nextinput_pressure_ctx;
#endif

hal_pressure_ctx_s *p_pressure_ctx = NULL;
/*********    Variable declaration area  END    *********/


/*************       Function  area START   *************/
int32_t hal_pressure_init(void)
{
#if defined(__PRESSURE_NEXTINPUT__)
	p_pressure_ctx = (hal_pressure_ctx_s *)&c_nextinput_pressure_ctx;
	if(!p_pressure_ctx->pressure_init()) {
		return HAL_RUN_SUCCESS;
	}
#endif

#if defined(__PRESSURE_CSA37F71__)
	p_pressure_ctx = (hal_pressure_ctx_s *)&c_csa37f71_pressure_ctx;
	if(!p_pressure_ctx->pressure_init()) {
		return HAL_RUN_SUCCESS;
	}
#endif

#if defined(__PRESSURE_AW8680X__)
    p_pressure_ctx = (hal_pressure_ctx_s *)&c_aw8680x_pressure_ctx;
    if(!p_pressure_ctx->pressure_init()) {
        return HAL_RUN_SUCCESS;
    }
#endif

    p_pressure_ctx = NULL;

    return HAL_INIT_ERROR;
}

int32_t hal_pressure_reset(void)
{
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_reset != NULL) {
        if(!p_pressure_ctx->pressure_reset())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_pressure_enter_standby_mode(void)
{
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_enter_standby_mode != NULL) {
        if(!p_pressure_ctx->pressure_enter_standby_mode())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_pressure_enter_detection_mode(void)
{
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_enter_detection_mode != NULL) {
        if(!p_pressure_ctx->pressure_enter_detection_mode())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_pressure_get_chip_id(uint32_t *chip_id)
{
    if (NULL == chip_id) {
        return HAL_INVALID_PARAMETER;
    }
    
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_get_chip_id != NULL) {
        if(!p_pressure_ctx->pressure_get_chip_id(chip_id)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_pressure_register_gesture_generate_cb(hal_pressure_gesture_event_cb cb)
{
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_set_gesture_events_cb != NULL) {
        if(!p_pressure_ctx->pressure_set_gesture_events_cb(cb))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_pressure_enter_pt_calibration(uint8_t items,uint8_t *data)
{
    if (p_pressure_ctx != NULL && p_pressure_ctx->pressure_enter_pt_calibration != NULL) {
        if(!p_pressure_ctx->pressure_enter_pt_calibration(items,data))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }

    return HAL_INVALID_PARAMETER;
}

#endif

