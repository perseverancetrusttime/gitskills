#ifndef __HAL_XBUS_H__
#define __HAL_XBUS_H__

#if defined(__XSPACE_XBUS_MANAGER__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    XBUS_MODE_UNKNOWN,
    XBUS_MODE_CHARGING,
    XBUS_MODE_COMMUNICATON,
    XBUS_MODE_OTA,
    XBUS_MODE_OTA_EXIT,
    XBUS_MODE_OUT_BOX,
    
    XBUS_MODE_QTY,
}hal_xbus_mode_e;

typedef void (*hal_xbus_rx_cb)(uint8_t *data, uint16_t len);

typedef struct
{  
    int32_t (* init)(void);
    int32_t (* xbus_rx_cb_set)(hal_xbus_rx_cb cb); /* cb Called in an interrupt function, must be fast */
    int32_t (* xbus_tx)(uint8_t *data, uint16_t len);
    int32_t (* xbus_mode_set)(hal_xbus_mode_e mode);
    int32_t (* xbus_mode_get)(hal_xbus_mode_e* mode);
} hal_xbus_ctrl_s;

int32_t hal_xbus_init(void);
int32_t hal_xbus_rx_cb_set(hal_xbus_rx_cb cb);
int32_t hal_xbus_tx(uint8_t *data, uint16_t len);
int32_t hal_xbus_mode_set(hal_xbus_mode_e mode);
int32_t hal_xbus_mode_get(hal_xbus_mode_e *mode);

#ifdef __cplusplus
}
#endif

#endif /* __XSPACE_XBUS_MANAGER__ */
#endif /* __HAL_XBUS_H__ */
