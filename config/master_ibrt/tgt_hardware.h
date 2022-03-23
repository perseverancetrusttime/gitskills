/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#ifndef __TGT_HARDWARE__
#define __TGT_HARDWARE__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_key.h"
#include "hal_aud.h"

//config hwardware codec iir.
#define EQ_HW_DAC_IIR_LIST_NUM              3
#define EQ_HW_ADC_IIR_LIST_NUM              1
#define EQ_HW_IIR_LIST_NUM                  1
#define EQ_SW_IIR_LIST_NUM                  1
#define EQ_HW_FIR_LIST_NUM                  3

//pwl

#ifdef __BT_DEBUG_TPORTS__
#define CFG_HW_PWL_NUM (0)
#else
#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
#define CFG_HW_PWL_NUM (0)
#else
#define CFG_HW_PWL_NUM (3)
#endif
#endif

#ifdef IS_MULTI_AI_ENABLED
#define AI_MIC_CHANEL_NUM   AUD_CHANNEL_MAP_CH0
#endif

extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PWL_NUM];
#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_ibrt_indication_pinmux_pwl[3];
#endif

#ifdef __KNOWLES
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_pinmux_uart[2];
#endif

//adckey define
#define CFG_HW_ADCKEY_NUMBER 0
#define CFG_HW_ADCKEY_BASE 0
#define CFG_HW_ADCKEY_ADC_MAXVOLT 1000
#define CFG_HW_ADCKEY_ADC_MINVOLT 0
#define CFG_HW_ADCKEY_ADC_KEYVOLT_BASE 130
extern const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER];

#define BTA_AV_CO_SBC_MAX_BITPOOL  52
#define MAX_AAC_BITRATE (264630)

#if defined(__SIMPLE_PARING__)
#define CFG_HW_GPIOKEY_NUM (1)
#else
#define CFG_HW_GPIOKEY_NUM (0)
#endif

extern const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM];

// ANC function key
#define ANC_FUNCTION_KEY                    HAL_KEY_CODE_PWR

// ANC coefficient curve number
#define ANC_COEF_NUM                        (4)

#define PSAP_COEF_LIST_NUM                  (1)

#define HWLIMITER_PARA_LIST_NUM             (1)
//#define ANC_TALK_THROUGH

#ifdef ANC_TALK_THROUGH
#define ANC_COEF_LIST_NUM                   (ANC_COEF_NUM + 1)
#else
#define ANC_COEF_LIST_NUM                   (ANC_COEF_NUM)
#endif

#define ANC_FF_MIC_CH_L                     AUD_CHANNEL_MAP_CH1
#define ANC_FF_MIC_CH_R                     0
#define ANC_FB_MIC_CH_L                     AUD_CHANNEL_MAP_CH0
#define ANC_FB_MIC_CH_R                     0

#define ANC_TT_MIC_CH_L                     0 // AUD_CHANNEL_MAP_CH1, bobby modify
#define ANC_TT_MIC_CH_R                     0

#define ANC_TALK_MIC_CH                     AUD_CHANNEL_MAP_CH4
#define ANC_REF_MIC_CH                      AUD_CHANNEL_MAP_ECMIC_CH0

#define ANC_VMIC_CFG                        (AUD_VMIC_MAP_VMIC3) //bobby modify

// audio codec
#define CFG_HW_AUD_INPUT_PATH_NUM           4  
extern const struct AUD_IO_PATH_CFG_T cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM];

#define CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV  (AUD_CHANNEL_MAP_CH0)

#define CFG_HW_AUD_SIDETONE_MIC_DEV         (AUD_CHANNEL_MAP_CH0)
#define CFG_HW_AUD_SIDETONE_GAIN_DBVAL      (-20)

//bt config
extern const char *BT_LOCAL_NAME;
extern const char *BLE_DEFAULT_NAME;
extern uint8_t ble_addr[6];
extern uint8_t bt_addr[6];

#define CODEC_SADC_VOL (12)

extern const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY];
extern const struct CODEC_DAC_VOL_T codec_dac_a2dp_vol[TGT_VOLUME_LEVEL_QTY];//bobby add
extern const struct CODEC_DAC_VOL_T codec_dac_hfp_vol[TGT_VOLUME_LEVEL_QTY];
#define CFG_AUD_EQ_IIR_NUM_BANDS (4)

//battery info
#define APP_BATTERY_MIN_MV (3200)
#define APP_BATTERY_PD_MV   (3100)

#define APP_BATTERY_MAX_MV (4200)

#if defined (__USE_SW_I2C__)
#define SW_I2C_MASTER_SCL_PIN   	            (HAL_IOMUX_PIN_P0_6)
#define SW_I2C_MASTER_SDA_PIN   	            (HAL_IOMUX_PIN_P0_7)
#endif

extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_enable_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg;

#if defined(__TOUCH_SUPPORT__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP touch_int_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP touch_reset_cfg;
#endif

extern const struct HAL_IOMUX_PIN_FUNCTION_MAP ldo_en_cfg;

#if defined(__PRESSURE_SUPPORT__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_int_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_reset_cfg;

#if defined(__HARDWARE_VERSION_V1_1_20201214__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_intx_cfg;
#endif

#endif

#if defined(__CHARGER_SUPPORT__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_charger_int_cfg;
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cover_status_int_cfg;
#endif

#if defined (__XSPACE_IMU_MANAGER__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP imu_int_status_cfg;
#endif

#if defined(__XBUS_UART_SUPPORT__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_tx_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_rx_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_en_cfg;
#endif

#if defined(__BAT_ADC_INFO_GET__)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP bat_adc_enable_cfg;
#endif
//extern const struct HAL_IOMUX_PIN_FUNCTION_MAP dummy_load_enable_cfg;

//NTC QN0402X474F3950FB
#define NTC_R_TO_VOLT(RES)		(((RES) * 1703) / ((RES) + 470))
#define NTC_CUSTOM_TBL	\
    { -20,  NTC_R_TO_VOLT(4480) }, \
    { -19,  NTC_R_TO_VOLT(4232) }, \
    { -18,  NTC_R_TO_VOLT(3999) }, \
    { -17,  NTC_R_TO_VOLT(3780) }, \
    { -16,  NTC_R_TO_VOLT(3574) }, \
    { -15,  NTC_R_TO_VOLT(3381) }, \
    { -14,  NTC_R_TO_VOLT(3199) }, \
    { -13,  NTC_R_TO_VOLT(3028) }, \
    { -12,  NTC_R_TO_VOLT(2867) }, \
    { -11,  NTC_R_TO_VOLT(2716) }, \
    { -10,  NTC_R_TO_VOLT(2573) }, \
    {  -9,  NTC_R_TO_VOLT(2439) }, \
    {  -8,  NTC_R_TO_VOLT(2312) }, \
    {  -7,  NTC_R_TO_VOLT(2193) }, \
    {  -6,  NTC_R_TO_VOLT(2081) }, \
    {  -5,  NTC_R_TO_VOLT(1974) }, \
    {  -4,  NTC_R_TO_VOLT(1874) }, \
    {  -3,  NTC_R_TO_VOLT(1780) }, \
    {  -2,  NTC_R_TO_VOLT(1691) }, \
    {  -1,  NTC_R_TO_VOLT(1607) }, \
    {   0,  NTC_R_TO_VOLT(1527) }, \
    {   1,  NTC_R_TO_VOLT(1452) }, \
    {   2,  NTC_R_TO_VOLT(1381) }, \
    {   3,  NTC_R_TO_VOLT(1314) }, \
    {   4,  NTC_R_TO_VOLT(1250) }, \
    {   5,  NTC_R_TO_VOLT(1190) }, \
    {   6,  NTC_R_TO_VOLT(1133) }, \
    {   7,  NTC_R_TO_VOLT(1079) }, \
    {   8,  NTC_R_TO_VOLT(1028) }, \
    {   9,  NTC_R_TO_VOLT(980) }, \
    {  10,  NTC_R_TO_VOLT(934) }, \
    {  11,  NTC_R_TO_VOLT(890) }, \
    {  12,  NTC_R_TO_VOLT(849) }, \
    {  13,  NTC_R_TO_VOLT(810) }, \
    {  14,  NTC_R_TO_VOLT(773) }, \
    {  15,  NTC_R_TO_VOLT(738) }, \
    {  16,  NTC_R_TO_VOLT(705) }, \
    {  17,  NTC_R_TO_VOLT(673) }, \
    {  18,  NTC_R_TO_VOLT(643) }, \
    {  19,  NTC_R_TO_VOLT(614) }, \
    {  20,  NTC_R_TO_VOLT(587) }, \
    {  21,  NTC_R_TO_VOLT(561) }, \
    {  22,  NTC_R_TO_VOLT(537) }, \
    {  23,  NTC_R_TO_VOLT(513) }, \
    {  24,  NTC_R_TO_VOLT(491) }, \
    {  25,  NTC_R_TO_VOLT(470) }, \
    {  26,  NTC_R_TO_VOLT(450) }, \
    {  27,  NTC_R_TO_VOLT(431) }, \
    {  28,  NTC_R_TO_VOLT(413) }, \
    {  29,  NTC_R_TO_VOLT(395) }, \
    {  30,  NTC_R_TO_VOLT(379) }, \
    {  31,  NTC_R_TO_VOLT(363) }, \
    {  32,  NTC_R_TO_VOLT(348) }, \
    {  33,  NTC_R_TO_VOLT(333) }, \
    {  34,  NTC_R_TO_VOLT(320) }, \
    {  35,  NTC_R_TO_VOLT(307) }, \
    {  36,  NTC_R_TO_VOLT(294) }, \
    {  37,  NTC_R_TO_VOLT(282) }, \
    {  38,  NTC_R_TO_VOLT(271) }, \
    {  39,  NTC_R_TO_VOLT(260) }, \
    {  40,  NTC_R_TO_VOLT(250) }, \
    {  41,  NTC_R_TO_VOLT(240) }, \
    {  42,  NTC_R_TO_VOLT(231) }, \
    {  43,  NTC_R_TO_VOLT(222) }, \
    {  44,  NTC_R_TO_VOLT(213) }, \
    {  45,  NTC_R_TO_VOLT(205) }, \
    {  46,  NTC_R_TO_VOLT(197) }, \
    {  47,  NTC_R_TO_VOLT(189) }, \
    {  48,  NTC_R_TO_VOLT(182) }, \
    {  49,  NTC_R_TO_VOLT(175) }, \
    {  50,  NTC_R_TO_VOLT(169) }, \
    {  51,  NTC_R_TO_VOLT(162) }, \
    {  52,  NTC_R_TO_VOLT(156) }, \
    {  53,  NTC_R_TO_VOLT(150) }, \
    {  54,  NTC_R_TO_VOLT(145) }, \
    {  55,  NTC_R_TO_VOLT(140) }, \
    {  56,  NTC_R_TO_VOLT(134) }, \
    {  57,  NTC_R_TO_VOLT(130) }, \
    {  58,  NTC_R_TO_VOLT(125) }, \
    {  59,  NTC_R_TO_VOLT(120) }, \
    {  60,  NTC_R_TO_VOLT(116) }

typedef enum
{
    LEFT_SIDE = 0,
    RIGHT_SIDE,
    UNKNOWN_SIDE
} ear_side_e;


#if defined(__FORCE_OTA_BIN_UPDATE__)
#define BOOT_UPDATE_SECTION_LOC __attribute__((section(".boot_update_section")))
extern BOOT_UPDATE_SECTION_LOC const uint8_t boot_bin[];
uint32_t get_ota_bin_size(void);
#endif

ear_side_e app_tws_get_earside(void);
int tgt_hardware_setup(void);
uint8_t app_get_ear_hw_version(void);
void app_tws_entry_shipmode(void);

#ifdef __cplusplus
}
#endif

#endif
