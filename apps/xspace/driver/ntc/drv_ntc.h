#ifndef __DRV_NTC_H__
#define __DRV_NTC_H__

#if defined(__NTC_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NTC_REF_R1                      (430)           //ohm
#define NTC_REF_R2                      (1100)          //ohm
#define NTC_REF_VOLTAGE                 (1700)          //mV
#define NTC_CALIBRATION_VOLTAGE         (40)            //mV
#define NTC_CAPTURE_STABLE_COUNT        (5)
#define NTC_CAPTURE_TEMPERATURE_STEP    (4)
#define NTC_CAPTURE_TEMPERATURE_REF     (15)
#define NTC_CAPTURE_VOLTAGE_REF         (1100)

#define NTC_GPADC_CHAN		HAL_GPADC_CHAN_2

typedef uint16_t ntc_voltage_mv;

typedef struct{
    ntc_voltage_mv currvolt;
    ntc_voltage_mv voltage[NTC_CAPTURE_STABLE_COUNT];
    uint16_t index;
}ntc_capture_measure_s;

typedef struct
{
    int16_t temperature;
    uint16_t resistance;    //resistance
} ntc_rt_s;

#ifdef __cplusplus
}
#endif

#endif /* __NTC_SUPPORT__ */
#endif /* __DRV_NTC_H__ */
