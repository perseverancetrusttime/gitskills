#ifndef __XSPACE_INOUT_BOX_MANAGER_H__
#define __XSPACE_INOUT_BOX_MANAGER_H__

#if defined(__XSPACE_INOUT_BOX_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "hal_pmu.h"

#if (__XSPACE_IOBOX_DEBUG__)
#define XIOB_TRACE(num, str, ...)   TRACE(num+1, "[XIOB]%s," str, __func__, ##__VA_ARGS__)
#define XIOB_ASSERT                 ASSERT
#else
#define XIOB_TRACE(num, str, ...)
#define XIOB_ASSERT
#endif

typedef enum
{
    XIOB_UNKNOWN,
    XIOB_OUT_BOX,
    XIOB_IN_BOX,
} xiob_status_e;

typedef void (*xiob_status_change_cb_func)(xiob_status_e status);

int32_t xspace_inout_box_manage_init(void);
xiob_status_e xspace_inout_box_manager_get_curr_status(void);
void xspace_inout_box_manager_set_curr_status(xiob_status_e status);
void xspace_inout_box_manager_register_ui_cb(xiob_status_change_cb_func cb);
#if defined(__XSPACE_AUTO_TEST__)
void config_simu_inout_box_gpio_val(uint8_t val);
#endif

#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_INOUT_BOX_MANAGER__)

#endif  // (__XSPACE_INOUT_BOX_MANAGER_H__)

