#ifndef __HAL_PMU_H__
#define __HAL_PMU_H__

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_error.h"
#include "hal_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__HAL_PMU_DEBUG__)
#define HAL_PMU_TRACE(num, str, ...)  TRACE(num+1, "[HAL_PMU] %s," str, __func__, ##__VA_ARGS__)
#else
#define HAL_PMU_TRACE(num, str, ...)
#endif

typedef enum {
    HAL_PMU_UNKNOWN = 0,
    HAL_PMU_PLUGIN,
    HAL_PMU_PLUGOUT,
} hal_pmu_status_e;


typedef void (*hal_pmu_charger_status_register_cb)(hal_pmu_status_e status);


void hal_pmu_init(void);
hal_pmu_status_e hal_pmu_force_get_charger_status(void);
int32_t hal_pmu_add_charger_status_change_cb(hal_pmu_charger_status_register_cb cb);
int32_t hal_pmu_remove_charger_status_change_cb(hal_pmu_charger_status_register_cb cb);


#ifdef __cplusplus
}
#endif

#endif //__HAL_PMU_H__

