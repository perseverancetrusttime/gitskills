#if defined(__XSPACE_XBUS_MANAGER__)
#include "hal_xbus.h"
#include "hal_error.h"
#include "drv_xbus_uart.h"

static hal_xbus_ctrl_s *p_xbus_ctrl = NULL;

int32_t hal_xbus_init(void)
{

   if(NULL != p_xbus_ctrl)
        return HAL_RUN_SUCCESS;

#if defined(__XBUS_UART_SUPPORT__)
    p_xbus_ctrl = (hal_xbus_ctrl_s *)&drv_xbus_uart_ctrl;


    if(NULL != p_xbus_ctrl && NULL != p_xbus_ctrl->init){
        if(!p_xbus_ctrl->init()){
            return HAL_RUN_SUCCESS;
        }else{
            return HAL_RUN_FAIL;
        }
    }
#endif

    return HAL_INIT_ERROR;
}

int32_t hal_xbus_rx_cb_set(hal_xbus_rx_cb cb)
{

    if (NULL != p_xbus_ctrl && NULL != p_xbus_ctrl->xbus_rx_cb_set) {
        if (!p_xbus_ctrl->xbus_rx_cb_set(cb)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_xbus_tx(uint8_t *data, uint16_t len)
{

    if (NULL != p_xbus_ctrl && NULL != p_xbus_ctrl->xbus_tx) {
        if(!p_xbus_ctrl->xbus_tx(data, len)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_xbus_mode_set(hal_xbus_mode_e mode)
{

    if (NULL != p_xbus_ctrl && NULL != p_xbus_ctrl->xbus_mode_set) {
        if(!p_xbus_ctrl->xbus_mode_set(mode)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}

int32_t hal_xbus_mode_get(hal_xbus_mode_e* mode)
{

    if (NULL != p_xbus_ctrl && NULL != p_xbus_ctrl->xbus_mode_get) {
        if (!p_xbus_ctrl->xbus_mode_get(mode)) {
            return HAL_RUN_SUCCESS;
        } else {
            return HAL_RUN_FAIL;
        }
    }

    return HAL_INVALID_PARAMETER;
}
#endif /* __XSPACE_XBUS_MANAGER__ */
