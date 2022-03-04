#ifndef __DRV_BAT_ADC_H__
#define __DRV_BAT_ADC_H__

#if defined(__BAT_ADC_INFO_GET__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_gpadc.h"
#include "hal_battery.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__DRV_ADC_DEBUG__)
#define DRV_ADC_TRACE(n, str, ...)  TRACE(n+1, "[DRV_ADC] %s," str, __func__, ##__VA_ARGS__)
#else
#define DRV_ADC_TRACE(n, str, ...)
#endif

typedef uint32_t bat_adc_voltage;

//battery info
#define DRV_BAT_MIN_MV			(2750) //unit: mV
#define DRV_BAT_PD_MV   		(3350) //unit: mV
#define DRV_BAT_LOW_MV 			(3649) //unit: mV
#define DRV_BAT_MAX_MV 			(4400) //unit: mV
#define DRV_BAT_LOW_PER			(10) // %

#define BAT_ADC_STABLE_COUNT            (10)
#define BAT_ADC_CIRCLE_FAST_MS          (200)
#define BAT_ADC_CIRCLE_DISCHARGING_MS    (10000)
#define BAT_ADC_CIRCLE_CHARGING_MS      (2000)

typedef enum {
    BAT_ADC_STATUS_DISCHARGING,
    BAT_ADC_STATUS_CHARGING,
    BAT_ADC_STATUS_INVALID,

    BAT_ADC_STATUS_QTY,    
} bat_adc_status_e;

typedef enum {
    ADC_BAT_INVAILD_STATUS,
    ADC_BAT_START_INTERVAL_CAPTURE_STATUS,
    ADC_BAT_PASSIVE_CAPTURE,
}bat_adc_capture_status_e;

typedef struct{
    uint16_t mv;
    uint16_t level;
}bat_volt_level_s;

typedef void (*bat_adc_event_cb)(bat_adc_status_e status, bat_adc_voltage volt);

typedef struct 
{
    uint8_t curr_level;
    uint8_t curr_percentage;
	uint8_t report_level;
	uint8_t report_percentage;
    bat_adc_status_e status;
    bat_adc_status_e pre_status;	
    bat_adc_capture_status_e capture_status;
    bat_adc_voltage curr_volt;
    bat_adc_voltage max_volt;
    bat_adc_voltage min_volt;	
    uint32_t periodic;
    uint16_t index;
    HAL_GPADC_MV_T voltage[BAT_ADC_STABLE_COUNT];
	uint16_t count;
    bat_adc_event_cb cb;
}bat_adc_measure_s;

extern const hal_bat_ctrl_if_s bat_adc_ctrl;

#ifdef __cplusplus
}
#endif

#endif   /*  __BAT_ADC_INFO_GET__  */
#endif   /*  __DRV_BAT_ADC_H__  */
