#ifndef __HAL_NTC_H__
#define __HAL_NTC_H__

#if defined(__NTC_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (* hal_ntc_report_temperature_cb)(int16_t temp);

typedef struct
{
    int32_t (* init)(void);
    int32_t (* get_chip_id)(uint32_t *chip_id);
    int32_t (* get_temperature)(int16_t *temp);
    int32_t (* start_oneshot_measure)(void);
    int32_t (* set_auto_report_temperature_cb)(hal_ntc_report_temperature_cb cb);
} hal_ntc_ctrl_s;

int32_t hal_ntc_init(void);
int32_t hal_ntc_get_chip_id(uint32_t *chip_id);
int32_t hal_ntc_get_temperature(int16_t *temp);
int32_t hal_ntc_start_oneshot_measure(void);
int32_t hal_ntc_register_auto_report_temperature_cb(hal_ntc_report_temperature_cb cb);

#ifdef __cplusplus
}
#endif

#endif /*  __NTC_SUPPORT__  */
#endif /*  __HAL_NTC_H__  */

