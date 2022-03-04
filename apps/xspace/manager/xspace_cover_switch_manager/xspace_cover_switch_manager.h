#ifndef __XSPACE_COVER_SWITCH_MANAGER_H__
#define __XSPACE_COVER_SWITCH_MANAGER_H__

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#if defined(__XSPACE_CSM_DEBUG__)
#define XCSM_TRACE(num, str, ...)  TRACE(num+2, "[XCSM] %s,line=%d," str, __func__, __LINE__, ##__VA_ARGS__)
#else
#define XCSM_TRACE(num, str, ...)
#endif

typedef enum
{
    XCSM_BOX_STATUS_UNKNOWN,
    XCSM_BOX_STATUS_CLOSED,
    XCSM_BOX_STATUS_OPENED,
} xcsm_status_e;

typedef void(*xcsm_cb_func)(xcsm_status_e cover_status);
typedef bool(*xcsm_ui_need_execute_func)(xcsm_status_e cover_status);

void xspace_cover_switch_manager_init(void);
void xspace_cover_switch_manage_set_status(xcsm_status_e status);
xcsm_status_e xspace_cover_switch_manage_get_status(void);
void xspace_cover_switch_manage_register_ui_cb(xcsm_cb_func cb);
void xspace_cover_switch_manage_register_manage_need_execute_cb(xcsm_ui_need_execute_func cb);
#if defined(__XSPACE_AUTO_TEST__)
void config_simu_hall_gpio_val(uint8_t val);
#endif
void xspace_cover_switch_manager_set_int_trig_mode(void);

#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_COVER_SWITCH_MANAGER__)

#endif  // (__XSPACE_COVER_SWITCH_MANAGER_H__)

