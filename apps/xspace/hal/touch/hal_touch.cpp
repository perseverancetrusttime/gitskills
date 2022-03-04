#include "hal_touch.h"
#include "hal_error.h"


#if defined(__TOUCH_SUPPORT__)

/*********    Variable declaration area START   *********/

#ifdef __TOUCH_GH61X__
extern const hal_touch_ctx_s c_gh61x_touch_ctx;
#endif
#if defined (__TOUCH_GH621X__)
extern const hal_touch_ctx_s c_gh621x_touch_ctx;
#endif
#if defined (__TOUCH_SIMULATION__)
extern const hal_touch_ctx_s c_simu_touch_ctx;
#endif

hal_touch_ctx_s *p_touch_ctx = NULL;
/*********    Variable declaration area  END    *********/


/*************       Function  area START   *************/
int32_t hal_touch_init(void)
{
#if defined(__TOUCH_GH61X__)
    p_touch_ctx = (hal_touch_ctx_s *)&c_gh61x_touch_ctx;
    if(!p_touch_ctx->touch_init()) {
        return HAL_RUN_SUCCESS;
    }
#endif
#if defined (__TOUCH_GH621X__)
    p_touch_ctx = (hal_touch_ctx_s *)&c_gh621x_touch_ctx;
    if(!p_touch_ctx->touch_init()) {
        return HAL_RUN_SUCCESS;
    }
#endif
#if defined (__TOUCH_SIMULATION__)
    p_touch_ctx = (hal_touch_ctx_s *)&c_simu_touch_ctx;
    if(!p_touch_ctx->touch_init()) {
        return HAL_RUN_SUCCESS;
    }
#endif
    p_touch_ctx = NULL;
    return HAL_INIT_ERROR;
}

int32_t hal_touch_reset(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_reset != NULL) {
        if(!p_touch_ctx->touch_reset())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_enter_standby_mode(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_enter_standby_mode != NULL) {
        if(!p_touch_ctx->touch_enter_standby_mode())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_enter_detection_mode(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_enter_detection_mode != NULL) {
        if(!p_touch_ctx->touch_enter_detection_mode())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_stop_inear_detection(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_stop_inear_detection != NULL) {
        if(!p_touch_ctx->touch_stop_inear_detection())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_start_inear_detection(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_start_inear_detection != NULL) {
        if(!p_touch_ctx->touch_start_inear_detection())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}


int32_t hal_touch_get_chip_id(uint32_t *chip_id)
{
    if (NULL == chip_id) {
        return HAL_INVALID_PARAMETER;
    }
    
    if (p_touch_ctx != NULL && p_touch_ctx->touch_get_chip_id != NULL) {
        if(!p_touch_ctx->touch_get_chip_id(chip_id)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_set_sensibility_grade(uint8_t grade)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_set_sensibility_grade != NULL) {
        if(!p_touch_ctx->touch_set_sensibility_grade(grade))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_register_gesture_generate_cb(hal_touch_gesture_event_cb cb)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_set_gesture_events_cb != NULL) {
        if(!p_touch_ctx->touch_set_gesture_events_cb(cb))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_register_inear_status_changed_cb(hal_touch_inear_status_cb cb)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_set_inear_status_cb != NULL) {
        if(!p_touch_ctx->touch_set_inear_status_cb(cb))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

int32_t hal_touch_off_set_self_cali(void)
{
    if (p_touch_ctx != NULL && p_touch_ctx->touch_off_set_self_cali != NULL) {
        if(!p_touch_ctx->touch_off_set_self_cali())
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

#endif

