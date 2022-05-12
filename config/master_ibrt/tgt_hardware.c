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
#include "tgt_hardware.h"
#include "aud_section.h"
#include "iir_process.h"
#include "fir_process.h"
#include "drc.h"
#include "limiter.h"
#include "spectrum_fix.h"

#if defined(__HARDWARE_VERSION_V1_0_20201028__)

#define TXON_CONTROL_PIN                        (HAL_IOMUX_PIN_NUM)

#if defined(__TOUCH_SUPPORT__)
#define TOUCH_INT_PIN                           (HAL_IOMUX_PIN_P1_7)
#define TOUCH_RESET_PIN                         (HAL_IOMUX_PIN_P1_6)
#endif

#if defined(__PRESSURE_SUPPORT__)
#define PRESSURE_INT_PIN                        (HAL_IOMUX_PIN_P1_5)
#define PRESSURE_RESET_PIN                      (HAL_IOMUX_PIN_P1_2)
#endif

#define LDO_EN_PIN			            		(HAL_IOMUX_PIN_P2_1)

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
#define HALL_SWITCH_INT_PIN			            (HAL_IOMUX_PIN_P1_4)
#endif

#if defined(__GET_BAT_VOL_GPIO_EN__)
#define BAT_ADC_EN_PIN			            	(HAL_IOMUX_PIN_NUM)
#endif

#if defined(__CHARGER_SUPPORT__)
#define CHARGER_INT_PIN                         (HAL_IOMUX_PIN_P1_3)
#endif

#if defined(__XBUS_UART_SUPPORT__)
#define XBUS_UART_TX_PIN			            (HAL_IOMUX_PIN_P2_0)
#define XBUS_UART_RX_PIN                        (HAL_IOMUX_PIN_P2_0)
#define XBUS_UART_EN_PIN			            (HAL_IOMUX_PIN_NUM)
#endif

#elif defined(__HARDWARE_VERSION_V1_1_20201214__)

#define TXON_CONTROL_PIN                        (HAL_IOMUX_PIN_P1_7)


#if defined(__TOUCH_SUPPORT__)
#define TOUCH_INT_PIN                           (HAL_IOMUX_PIN_P1_0)
#define TOUCH_RESET_PIN                         (HAL_IOMUX_PIN_P1_6)
#endif

#if defined(__PRESSURE_SUPPORT__)
#define PRESSURE_INT_PIN                        (HAL_IOMUX_PIN_P0_0)
#define PRESSURE_RESET_PIN                      (HAL_IOMUX_PIN_P0_1)
#define PRESSURE_WAKE_UP_PIN                    (HAL_IOMUX_PIN_P0_2)
#define PRESSURE_INTX_PIN                       (HAL_IOMUX_PIN_P1_5)
#endif

#define LDO_EN_PIN			            		(HAL_IOMUX_PIN_P1_4)

#if defined (__XSPACE_IMU_MANAGER__)
//Note(Mike):Hardcode HALL Interrupt pin for IMU Interrupt pin
#define IMU_INT_PIN			                    (HAL_IOMUX_PIN_P1_2)
#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
#define HALL_SWITCH_INT_PIN			            (HAL_IOMUX_PIN_NUM)
#endif
#endif

#if defined(__GET_BAT_VOL_GPIO_EN__)
#define BAT_ADC_EN_PIN			            	(HAL_IOMUX_PIN_NUM)
#endif

#if defined(__CHARGER_SUPPORT__)
#define CHARGER_INT_PIN                         (HAL_IOMUX_PIN_P0_3)
#endif

#if defined(__XBUS_UART_SUPPORT__)
#define XBUS_UART_TX_PIN			            (HAL_IOMUX_PIN_P2_1)
#define XBUS_UART_RX_PIN                        (HAL_IOMUX_PIN_P2_0)
#define XBUS_UART_EN_PIN			            (HAL_IOMUX_PIN_NUM)
#endif
#endif

//#define DUMMY_LOAD_ENABLE			            (HAL_IOMUX_PIN_P1_5)

#define TWS_EAR_SIDE_PIN			            (HAL_IOMUX_PIN_P2_7)

#define EAR_HW_SHIPMODE_PIN			        	(HAL_IOMUX_PIN_P0_2)


#define EAR_HW_VER_BIT0_PIN			         	(HAL_IOMUX_PIN_P2_5)
#define EAR_HW_VER_BIT1_PIN			         	(HAL_IOMUX_PIN_P2_6)

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PWL_NUM] = {
#if (CFG_HW_PWL_NUM > 0)
#if defined (__XSPACE_UI__)
    {HAL_IOMUX_PIN_LED1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE},
#else
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
#endif
};

#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_ibrt_indication_pinmux_pwl[3] = {
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VBAT, HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VBAT, HAL_IOMUX_PIN_PULLUP_ENABLE},
};
#endif

//adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
#if (CFG_HW_ADCKEY_NUMBER > 0)
    HAL_KEY_CODE_FN9,HAL_KEY_CODE_FN8,HAL_KEY_CODE_FN7,
    HAL_KEY_CODE_FN6,HAL_KEY_CODE_FN5,HAL_KEY_CODE_FN4,
    HAL_KEY_CODE_FN3,HAL_KEY_CODE_FN2,HAL_KEY_CODE_FN1,
#endif
};

//gpiokey define
#define CFG_HW_GPIOKEY_DOWN_LEVEL          (0)
#define CFG_HW_GPIOKEY_UP_LEVEL            (1)
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {
#if defined(__SIMPLE_PARING__)
    {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},//left
#else //__SIMPLE_PARING__
#if (CFG_HW_GPIOKEY_NUM > 0)
#ifdef BES_AUDIO_DEV_Main_Board_9v0
    {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN3,{HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN4,{HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN5,{HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN6,{HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
#else
    {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},//left
    {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},//right
    {HAL_KEY_CODE_FN3,{HAL_IOMUX_PIN_P0_7, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},//middle
#endif
#endif

#endif // __SIMPLE_PARING__
};

//bt config
const char *BT_LOCAL_NAME = TO_STRING(BT_DEV_NAME) "\0";
#if defined (__XSPACE_AUTO_TEST__)
const char *BLE_DEFAULT_NAME = "BO001_AUTO_TESTTTTTTTT";
#else
const char *BLE_DEFAULT_NAME = "MASTER_BRANCHHHHHHHHHH";
#endif

uint8_t ble_addr[6] = {
#ifdef BLE_DEV_ADDR
	BLE_DEV_ADDR
#else
	0xBE,0x99,0x34,0x45,0x56,0x67
#endif
};
uint8_t bt_addr[6] = {
#ifdef BT_DEV_ADDR
	BT_DEV_ADDR
#else
	0x1e,0x57,0x34,0x45,0x56,0x67
#endif
};

#define TX_PA_GAIN                          CODEC_TX_PA_GAIN_DEFAULT

const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY] = {  //bobby : for onepuls outsize hpone
    {TX_PA_GAIN,0x03,-99},
    {TX_PA_GAIN,0x03,-45},
    {TX_PA_GAIN,0x03,-44},
    {TX_PA_GAIN,0x03,-43},
    {TX_PA_GAIN,0x03,-41},
    {TX_PA_GAIN,0x03,-39},
    {TX_PA_GAIN,0x03,-37},
    {TX_PA_GAIN,0x03,-35},
    {TX_PA_GAIN,0x03,-33},
    {TX_PA_GAIN,0x03,-31},
    {TX_PA_GAIN,0x03,-29},
    {TX_PA_GAIN,0x03,-27},
    {TX_PA_GAIN,0x03,-25},
    {TX_PA_GAIN,0x03,-23},
    {TX_PA_GAIN,0x03,-21},
    {TX_PA_GAIN,0x03,-19},
    {TX_PA_GAIN,0x03,-18},
    {TX_PA_GAIN,0x03,-17},
    {TX_PA_GAIN,0x03,-16},
    {TX_PA_GAIN,0x03,-15},
    {TX_PA_GAIN,0x03,-14},
    {TX_PA_GAIN,0x03,-13},
    {TX_PA_GAIN,0x03,-12},
    {TX_PA_GAIN,0x03,-11},
    {TX_PA_GAIN,0x03,-10},
    {TX_PA_GAIN,0x03,-9},
    {TX_PA_GAIN,0x03,-8},
    {TX_PA_GAIN,0x03,-6},
    {TX_PA_GAIN,0x03,-4},
    {TX_PA_GAIN,0x03,-2},
    {TX_PA_GAIN,0x03,-0},  //0dBm
};

const struct CODEC_DAC_VOL_T codec_dac_a2dp_vol[TGT_VOLUME_LEVEL_QTY] = {  //for other hpone
    {TX_PA_GAIN,0x03,-99},//
    {TX_PA_GAIN,0x03,-63},
    {TX_PA_GAIN,0x03,-60},//
    {TX_PA_GAIN,0x03,-58},
    {TX_PA_GAIN,0x03,-56},//
    {TX_PA_GAIN,0x03,-52},//
    {TX_PA_GAIN,0x03,-50},
    {TX_PA_GAIN,0x03,-48},//
    {TX_PA_GAIN,0x03,-46},
    {TX_PA_GAIN,0x03,-44},//
    {TX_PA_GAIN,0x03,-42},
    {TX_PA_GAIN,0x03,-40},//
    {TX_PA_GAIN,0x03,-38},
    {TX_PA_GAIN,0x03,-36},//
    {TX_PA_GAIN,0x03,-34},
    {TX_PA_GAIN,0x03,-32},//
    {TX_PA_GAIN,0x03,-30},
    {TX_PA_GAIN,0x03,-28},//
    {TX_PA_GAIN,0x03,-26},
    {TX_PA_GAIN,0x03,-24},//
    {TX_PA_GAIN,0x03,-22},
    {TX_PA_GAIN,0x03,-20},//
    {TX_PA_GAIN,0x03,-16},//
    {TX_PA_GAIN,0x03,-14},
    {TX_PA_GAIN,0x03,-12},//
    {TX_PA_GAIN,0x03,-10},
    {TX_PA_GAIN,0x03,-8},//
    {TX_PA_GAIN,0x03,-6},
    {TX_PA_GAIN,0x03,-4},//
    {TX_PA_GAIN,0x03,-2},
    {TX_PA_GAIN,0x03,-0},  //0dBm
};

#define HFP_VOL_ATT  0
const struct CODEC_DAC_VOL_T codec_dac_hfp_vol[TGT_VOLUME_LEVEL_QTY] = { //bobby modify
    {TX_PA_GAIN,0x03,-99},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-45},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-44},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-43},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-41},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-39},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-37},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-35},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-33},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-31},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-29},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-27},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-25},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-23},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-21},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-19},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-18},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-17},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-16},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-15},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-14},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-13},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-12},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-11},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-10},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-9},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-8},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-6},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-4},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-2},
    {TX_PA_GAIN,0x03,HFP_VOL_ATT-0},  //0dBm
};

// Dev mother board VMIC1 <---> CHIP VMIC2
// Dev mother board VMIC2 <---> CHIP VMIC1
#ifndef VMIC_MAP_CFG
#define VMIC_MAP_CFG                        AUD_VMIC_MAP_VMIC3|AUD_VMIC_MAP_VMIC5  //bobby odify
#endif


//#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV       (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 |AUD_CHANNEL_MAP_CH4|AUD_VMIC_MAP_VMIC3 | AUD_VMIC_MAP_VMIC5)
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV       (AUD_CHANNEL_MAP_CH0 | AUD_VMIC_MAP_VMIC3)

#define CFG_HW_AUD_INPUT_PATH_ANC_ASSIST_DEV    (ANC_FF_MIC_CH_L | ANC_FB_MIC_CH_L | ANC_TALK_MIC_CH | ANC_REF_MIC_CH | VMIC_MAP_CFG)
#define CFG_HW_AUD_INPUT_PATH_LINEIN_DEV        (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#ifdef VOICE_DETECTOR_SENS_EN

#define CFG_HW_AUD_INPUT_PATH_VADMIC_DEV        (AUD_CHANNEL_MAP_CH4 | VMIC_MAP_CFG)
#else
#define CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV        (AUD_CHANNEL_MAP_CH0 | VMIC_MAP_CFG)
#endif

#define CFG_HW_AUD_INPUT_PATH_ADAPT_ANC_DEV     (ANC_FF_MIC_CH_L | ANC_FB_MIC_CH_L | VMIC_MAP_CFG)

#define CFG_HW_AUD_INPUT_PATH_HEARING_DEV   (AUD_CHANNEL_MAP_CH0 | VMIC_MAP_CFG)
const struct AUD_IO_PATH_CFG_T cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM] = {
#if defined(__ELEVOC_CODEC_REF__)||defined(__SP_REF_CHA__)||defined(__WENWEN__CODEC_REF__)
    { AUD_INPUT_PATH_MAINMIC, CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV | AUD_CHANNEL_MAP_ECMIC_CH0, },
#else
    { AUD_INPUT_PATH_MAINMIC, CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV, },
#endif
    { AUD_INPUT_PATH_LINEIN,  CFG_HW_AUD_INPUT_PATH_LINEIN_DEV, },
#ifdef VOICE_DETECTOR_SENS_EN
    { AUD_INPUT_PATH_VADMIC,  CFG_HW_AUD_INPUT_PATH_VADMIC_DEV, },
#else
    { AUD_INPUT_PATH_ASRMIC,  CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV, },
#endif
#if defined(__ELEVOC_CODEC_REF__)||defined(__SP_REF_CHA__)||defined(__WENWEN__CODEC_REF__)
    { AUD_INPUT_PATH_HEARING,   CFG_HW_AUD_INPUT_PATH_HEARING_DEV | AUD_CHANNEL_MAP_ECMIC_CH0, },
#else
    { AUD_INPUT_PATH_HEARING,   CFG_HW_AUD_INPUT_PATH_HEARING_DEV, },
#endif
};
/* mimi modify and define mic config********************************************************************************/
const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_enable_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};

#if defined(__TOUCH_SUPPORT__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP touch_int_cfg = {
    TOUCH_INT_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP touch_reset_cfg = {
    TOUCH_RESET_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};
#endif

#if defined(__PRESSURE_SUPPORT__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_int_cfg = {
    PRESSURE_INT_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_reset_cfg = {
    PRESSURE_RESET_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLDOWN_ENABLE
};
	
#if defined(__HARDWARE_VERSION_V1_1_20201214__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP pressure_intx_cfg = {
    PRESSURE_INTX_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
};
#endif

#endif


const struct HAL_IOMUX_PIN_FUNCTION_MAP ldo_en_cfg = {
    LDO_EN_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};


#if defined (__XSPACE_IMU_MANAGER__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP imu_int_status_cfg = {
	IMU_INT_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL
};

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP cover_status_int_cfg = {
	HALL_SWITCH_INT_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL
};
#endif
#endif

#if defined(__CHARGER_SUPPORT__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP app_charger_int_cfg = {
	CHARGER_INT_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL
};
#endif

#if defined(__GET_BAT_VOL_GPIO_EN__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP bat_adc_enable_cfg = {
    BAT_ADC_EN_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};
#endif

#if defined(__XBUS_UART_SUPPORT__)
const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_tx_cfg = {
    XBUS_UART_TX_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_rx_cfg = {
    XBUS_UART_RX_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP xbus_uart_en_cfg = {
    XBUS_UART_EN_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};
#endif

//const struct HAL_IOMUX_PIN_FUNCTION_MAP dummy_load_enable_cfg = {
//    DUMMY_LOAD_ENABLE, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
//};

static const struct HAL_IOMUX_PIN_FUNCTION_MAP app_tws_ear_side_cfg = {
    TWS_EAR_SIDE_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};

static const struct HAL_IOMUX_PIN_FUNCTION_MAP app_ear_shipmode_cfg = {
    EAR_HW_SHIPMODE_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL,
};

static const struct HAL_IOMUX_PIN_FUNCTION_MAP ear_hw_version_cfg[2] = {
    {EAR_HW_VER_BIT0_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
    {EAR_HW_VER_BIT1_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
};

static const struct HAL_IOMUX_PIN_FUNCTION_MAP app_txon_ctrl_cfg = {
    TXON_CONTROL_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE,
};

#define IIR_COUNTER_FF_L (4)
#define IIR_COUNTER_FF_R (6)
#define IIR_COUNTER_FB_L (6)
#define IIR_COUNTER_FB_R (5)

#if (ANC_COEF_LIST_NUM >= 1)
static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_50p7k_mode0 = {
    .anc_cfg_ff_l = {
		.total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x0105ecf0,0xfe565a1e,0x00b006aa},
		.iir_coef[0].coef_a={0x08000000,0xf0fd1033,0x070de6fd},

		.iir_coef[1].coef_b={0x0801162a,0xf0089107,0x07f6853e},
		.iir_coef[1].coef_a={0x08000000,0xf0089107,0x07f79b68},

		.iir_coef[2].coef_b={0x07ed6eef,0xf03456a5,0x07e0fa1a},
		.iir_coef[2].coef_a={0x08000000,0xf03456a5,0x07ce6909},

		.iir_coef[3].coef_b={0x07f811ad,0xf019aa8e,0x07f2bf88},
		.iir_coef[3].coef_a={0x08000000,0xf019aa8e,0x07ead135},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x079d8bb9,0xf0d79873,0x078e8b2e},
		.iir_coef[0].coef_a={0x08000000,0xf0d79873,0x072c16e7},

		.iir_coef[1].coef_b={0x080a6815,0xf009a373,0x07ebf7c6},
		.iir_coef[1].coef_a={0x08000000,0xf009a373,0x07f65fdb},

		.iir_coef[2].coef_b={0x08071515,0xf00b7537,0x07ed7b94},
		.iir_coef[2].coef_a={0x08000000,0xf00b7537,0x07f490a8},

		.iir_coef[3].coef_b={0x0839e3cf,0xf0968f0d,0x073158ab},
		.iir_coef[3].coef_a={0x08000000,0xf0968f0d,0x076b3c7a},

		.iir_coef[4].coef_b={0x07d2392e,0xf146bf0a,0x06f44bd5},
		.iir_coef[4].coef_a={0x08000000,0xf146bf0a,0x06c68503},

		.iir_coef[5].coef_b={0x07717052,0xf1748ca2,0x0733c4fc},
		.iir_coef[5].coef_a={0x08000000,0xf1748ca2,0x06a5354e},

/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_48k_mode0 = {
    .anc_cfg_ff_l = {
		.total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x010526f3,0xfe557274,0x00b1265a},
		.iir_coef[0].coef_a={0x08000000,0xf0f738db,0x07133fcf},

		.iir_coef[1].coef_b={0x08010fa8,0xf0085cb7,0x07f6be01},
		.iir_coef[1].coef_a={0x08000000,0xf0085cb7,0x07f7cda9},

		.iir_coef[2].coef_b={0x07eddcf8,0xf0331059,0x07e1b1f5},
		.iir_coef[2].coef_a={0x08000000,0xf0331059,0x07cf8eed},

		.iir_coef[3].coef_b={0x07f840fd,0xf018f729,0x07f30e96},
		.iir_coef[3].coef_a={0x08000000,0xf018f729,0x07eb4f93},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x079fbc39,0xf0d2b770,0x07911117},
		.iir_coef[0].coef_a={0x08000000,0xf0d2b770,0x0730cd50},

		.iir_coef[1].coef_b={0x080a29c9,0xf00969ad,0x07ec6fb0},
		.iir_coef[1].coef_a={0x08000000,0xf00969ad,0x07f6997a},

		.iir_coef[2].coef_b={0x0806eab4,0xf00b3085,0x07edea60},
		.iir_coef[2].coef_a={0x08000000,0xf00b3085,0x07f4d514},
            
        .iir_coef[3].coef_b={0x083894e6,0xf0931d7e,0x07360439},
        .iir_coef[3].coef_a={0x08000000,0xf0931d7e,0x076e9920},
        
        .iir_coef[4].coef_b={0x07d336b3,0xf13f5f7e,0x06fa166e},
        .iir_coef[4].coef_a={0x08000000,0xf13f5f7e,0x06cd4d21},
        
        .iir_coef[5].coef_b={0x07747d57,0xf16bfc0d,0x073823dd},
        .iir_coef[5].coef_a={0x08000000,0xf16bfc0d,0x06aca134},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=5,

		.iir_coef[0].coef_b={0x00033c73,0x000678e7,0x00033c73},
		.iir_coef[0].coef_a={0x08000000,0xf0eed9b6,0x071e1818},
            
        .iir_coef[1].coef_b={0x07cf849e,0xf082057c,0x07aefdd3},
        .iir_coef[1].coef_a={0x08000000,0xf082057c,0x077e8271},
        
        .iir_coef[2].coef_b={0x07ffcc1e,0xf0028e6d,0x07fda5a2},
        .iir_coef[2].coef_a={0x08000000,0xf0028e6d,0x07fd71bf},
            
        .iir_coef[3].coef_b={0x07ffcfd9,0xf000ee2a,0x07ff4201},
        .iir_coef[3].coef_a={0x08000000,0xf000ee2f,0x07ff11df},
        
        .iir_coef[4].coef_b={0x07fc6317,0xf018c68c,0x07eadf16},
        .iir_coef[4].coef_a={0x08000000,0xf018c68c,0x07e7422d},
        
		.dac_gain_offset=0,
		.adc_gain_offset=(0)*4,
    },
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_44p1k_mode0 = {
    .anc_cfg_ff_l = {
		.total_gain =512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x010801ab,0xfe58ce64,0x00ad0a0f},
		.iir_coef[0].coef_a={0x08000000,0xf10cb9e3,0x06ff9e98},

		.iir_coef[1].coef_b={0x080127a1,0xf0091db0,0x07f5ece1},
		.iir_coef[1].coef_a={0x08000000,0xf0091db0,0x07f71481},

		.iir_coef[2].coef_b={0x07ec47d6,0xf037c58a,0x07df0d07},
		.iir_coef[2].coef_a={0x08000000,0xf037c58a,0x07cb54dd},

		.iir_coef[3].coef_b={0x07f792b9,0xf01b922c,0x07f1eb68},
		.iir_coef[3].coef_a={0x08000000,0xf01b922c,0x07e97e20},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x0797b1b5,0xf0e4a7b8,0x0787cce7},
		.iir_coef[0].coef_a={0x08000000,0xf0e4a7b8,0x071f7e9d},

		.iir_coef[1].coef_b={0x080b0f4d,0xf00a3e8c,0x07eab5e2},
		.iir_coef[1].coef_a={0x08000000,0xf00a3e8c,0x07f5c530},

		.iir_coef[2].coef_b={0x080786d5,0xf00c2da2,0x07ec522c},
		.iir_coef[2].coef_a={0x08000000,0xf00c2da2,0x07f3d900},
		
        .iir_coef[3].coef_b={0x083d6422,0xf09fc8dc,0x0724d900},
        .iir_coef[3].coef_a={0x08000000,0xf09fc8dc,0x07623d22},
        
        .iir_coef[4].coef_b={0x07cf9534,0xf15a7a60,0x06e4daaf},
        .iir_coef[4].coef_a={0x08000000,0xf15a7a60,0x06b46fe3},
        
        .iir_coef[5].coef_b={0x07694fdc,0xf18b8117,0x07282090},
        .iir_coef[5].coef_a={0x08000000,0xf18b8117,0x0691706c},

/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=5,
		
        .iir_coef[0].coef_b={0x0003d09b,0x0007a136,0x0003d09b},
        .iir_coef[0].coef_a={0x08000000,0xf103e70d,0x070b5b5f},
            
        .iir_coef[1].coef_b={0x07cb60c8,0xf08d2cda,0x07a812f2},
        .iir_coef[1].coef_a={0x08000000,0xf08d2cda,0x077373bb},
        
        .iir_coef[2].coef_b={0x07ffc788,0xf002c847,0x07fd7066},
        .iir_coef[2].coef_a={0x08000000,0xf002c847,0x07fd37ed},
            
        .iir_coef[3].coef_b={0x07ffcb97,0xf001033a,0x07ff3135},
        .iir_coef[3].coef_a={0x08000000,0xf001033f,0x07fefcd1},
        
        .iir_coef[4].coef_b={0x07fc11d5,0xf01af49a,0x07e903e4},
        .iir_coef[4].coef_a={0x08000000,0xf01af49a,0x07e515b9},

		.dac_gain_offset=0,
		.adc_gain_offset=(0)*4,
    },
};
#endif

/**add by lmzhang,20201221*/
#if (ANC_COEF_LIST_NUM >= 2)
static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_50p7k_mode1 = {
    .anc_cfg_ff_l = {
		.total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x0105ecf0,0xfe565a1e,0x00b006aa},
		.iir_coef[0].coef_a={0x08000000,0xf0fd1033,0x070de6fd},

		.iir_coef[1].coef_b={0x0801162a,0xf0089107,0x07f6853e},
		.iir_coef[1].coef_a={0x08000000,0xf0089107,0x07f79b68},

		.iir_coef[2].coef_b={0x07ed6eef,0xf03456a5,0x07e0fa1a},
		.iir_coef[2].coef_a={0x08000000,0xf03456a5,0x07ce6909},

		.iir_coef[3].coef_b={0x07f811ad,0xf019aa8e,0x07f2bf88},
		.iir_coef[3].coef_a={0x08000000,0xf019aa8e,0x07ead135},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x079d8bb9,0xf0d79873,0x078e8b2e},
		.iir_coef[0].coef_a={0x08000000,0xf0d79873,0x072c16e7},

		.iir_coef[1].coef_b={0x080a6815,0xf009a373,0x07ebf7c6},
		.iir_coef[1].coef_a={0x08000000,0xf009a373,0x07f65fdb},

		.iir_coef[2].coef_b={0x08071515,0xf00b7537,0x07ed7b94},
		.iir_coef[2].coef_a={0x08000000,0xf00b7537,0x07f490a8},

		.iir_coef[3].coef_b={0x0839e3cf,0xf0968f0d,0x073158ab},
		.iir_coef[3].coef_a={0x08000000,0xf0968f0d,0x076b3c7a},

		.iir_coef[4].coef_b={0x07d2392e,0xf146bf0a,0x06f44bd5},
		.iir_coef[4].coef_a={0x08000000,0xf146bf0a,0x06c68503},

		.iir_coef[5].coef_b={0x07717052,0xf1748ca2,0x0733c4fc},
		.iir_coef[5].coef_a={0x08000000,0xf1748ca2,0x06a5354e},

/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_48k_mode1 = {
    .anc_cfg_ff_l = {
		.total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x010526f3,0xfe557274,0x00b1265a},
		.iir_coef[0].coef_a={0x08000000,0xf0f738db,0x07133fcf},

		.iir_coef[1].coef_b={0x08010fa8,0xf0085cb7,0x07f6be01},
		.iir_coef[1].coef_a={0x08000000,0xf0085cb7,0x07f7cda9},

		.iir_coef[2].coef_b={0x07eddcf8,0xf0331059,0x07e1b1f5},
		.iir_coef[2].coef_a={0x08000000,0xf0331059,0x07cf8eed},

		.iir_coef[3].coef_b={0x07f840fd,0xf018f729,0x07f30e96},
		.iir_coef[3].coef_a={0x08000000,0xf018f729,0x07eb4f93},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x079fbc39,0xf0d2b770,0x07911117},
		.iir_coef[0].coef_a={0x08000000,0xf0d2b770,0x0730cd50},

		.iir_coef[1].coef_b={0x080a29c9,0xf00969ad,0x07ec6fb0},
		.iir_coef[1].coef_a={0x08000000,0xf00969ad,0x07f6997a},

		.iir_coef[2].coef_b={0x0806eab4,0xf00b3085,0x07edea60},
		.iir_coef[2].coef_a={0x08000000,0xf00b3085,0x07f4d514},
            
        .iir_coef[3].coef_b={0x083894e6,0xf0931d7e,0x07360439},
        .iir_coef[3].coef_a={0x08000000,0xf0931d7e,0x076e9920},
        
        .iir_coef[4].coef_b={0x07d336b3,0xf13f5f7e,0x06fa166e},
        .iir_coef[4].coef_a={0x08000000,0xf13f5f7e,0x06cd4d21},
        
        .iir_coef[5].coef_b={0x07747d57,0xf16bfc0d,0x073823dd},
        .iir_coef[5].coef_a={0x08000000,0xf16bfc0d,0x06aca134},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=5,

		.iir_coef[0].coef_b={0x00033c73,0x000678e7,0x00033c73},
		.iir_coef[0].coef_a={0x08000000,0xf0eed9b6,0x071e1818},
            
        .iir_coef[1].coef_b={0x07cf849e,0xf082057c,0x07aefdd3},
        .iir_coef[1].coef_a={0x08000000,0xf082057c,0x077e8271},
        
        .iir_coef[2].coef_b={0x07ffcc1e,0xf0028e6d,0x07fda5a2},
        .iir_coef[2].coef_a={0x08000000,0xf0028e6d,0x07fd71bf},
            
        .iir_coef[3].coef_b={0x07ffcfd9,0xf000ee2a,0x07ff4201},
        .iir_coef[3].coef_a={0x08000000,0xf000ee2f,0x07ff11df},
        
        .iir_coef[4].coef_b={0x07fc6317,0xf018c68c,0x07eadf16},
        .iir_coef[4].coef_a={0x08000000,0xf018c68c,0x07e7422d},
        
		.dac_gain_offset=0,
		.adc_gain_offset=(0)*4,
    },
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_44p1k_mode1 = {
    .anc_cfg_ff_l = {
		.total_gain =512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FF_L,

		.iir_coef[0].coef_b={0x010801ab,0xfe58ce64,0x00ad0a0f},
		.iir_coef[0].coef_a={0x08000000,0xf10cb9e3,0x06ff9e98},

		.iir_coef[1].coef_b={0x080127a1,0xf0091db0,0x07f5ece1},
		.iir_coef[1].coef_a={0x08000000,0xf0091db0,0x07f71481},

		.iir_coef[2].coef_b={0x07ec47d6,0xf037c58a,0x07df0d07},
		.iir_coef[2].coef_a={0x08000000,0xf037c58a,0x07cb54dd},

		.iir_coef[3].coef_b={0x07f792b9,0xf01b922c,0x07f1eb68},
		.iir_coef[3].coef_a={0x08000000,0xf01b922c,0x07e97e20},


/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=IIR_COUNTER_FB_L,

		.iir_coef[0].coef_b={0x0797b1b5,0xf0e4a7b8,0x0787cce7},
		.iir_coef[0].coef_a={0x08000000,0xf0e4a7b8,0x071f7e9d},

		.iir_coef[1].coef_b={0x080b0f4d,0xf00a3e8c,0x07eab5e2},
		.iir_coef[1].coef_a={0x08000000,0xf00a3e8c,0x07f5c530},

		.iir_coef[2].coef_b={0x080786d5,0xf00c2da2,0x07ec522c},
		.iir_coef[2].coef_a={0x08000000,0xf00c2da2,0x07f3d900},
		
        .iir_coef[3].coef_b={0x083d6422,0xf09fc8dc,0x0724d900},
        .iir_coef[3].coef_a={0x08000000,0xf09fc8dc,0x07623d22},
        
        .iir_coef[4].coef_b={0x07cf9534,0xf15a7a60,0x06e4daaf},
        .iir_coef[4].coef_a={0x08000000,0xf15a7a60,0x06b46fe3},
        
        .iir_coef[5].coef_b={0x07694fdc,0xf18b8117,0x07282090},
        .iir_coef[5].coef_a={0x08000000,0xf18b8117,0x0691706c},

/*		.fir_bypass_flag=1,
        .fir_len = AUD_COEF_LEN,
        .fir_coef =
        {
            32767,
        },
*/
		.dac_gain_offset=0,
		.adc_gain_offset=-24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,

		.iir_bypass_flag=0,
		.iir_counter=5,
		
        .iir_coef[0].coef_b={0x0003d09b,0x0007a136,0x0003d09b},
        .iir_coef[0].coef_a={0x08000000,0xf103e70d,0x070b5b5f},
            
        .iir_coef[1].coef_b={0x07cb60c8,0xf08d2cda,0x07a812f2},
        .iir_coef[1].coef_a={0x08000000,0xf08d2cda,0x077373bb},
        
        .iir_coef[2].coef_b={0x07ffc788,0xf002c847,0x07fd7066},
        .iir_coef[2].coef_a={0x08000000,0xf002c847,0x07fd37ed},
            
        .iir_coef[3].coef_b={0x07ffcb97,0xf001033a,0x07ff3135},
        .iir_coef[3].coef_a={0x08000000,0xf001033f,0x07fefcd1},
        
        .iir_coef[4].coef_b={0x07fc11d5,0xf01af49a,0x07e903e4},
        .iir_coef[4].coef_a={0x08000000,0xf01af49a,0x07e515b9},

		.dac_gain_offset=0,
		.adc_gain_offset=(0)*4,
    },
};
#endif

#if (ANC_COEF_LIST_NUM >= 3)
static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_50p7k_mode2 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0x01396643,0xfdf0bb7d,0x00e36e2c},
        .iir_coef[0].coef_a = {0x08000000,0xf0e957ee,0x07230680},
        .iir_coef[1].coef_b = {0x080017ca,0xf0008ea6,0x07ff599c},
        .iir_coef[1].coef_a = {0x08000000,0xf0008ea3,0x07ff7163},
        .iir_coef[2].coef_b = {0x0801162a,0xf0089107,0x07f6853e},
        .iir_coef[2].coef_a = {0x08000000,0xf0089107,0x07f79b68},
        .iir_coef[3].coef_b = {0x07e4514c,0xf04cab50,0x07d1bed5},
        .iir_coef[3].coef_a = {0x08000000,0xf04cab50,0x07b61021},
        .iir_coef[4].coef_b = {0x07f5a922,0xf01da15e,0x07f13025},
        .iir_coef[4].coef_a = {0x08000000,0xf01da15e,0x07e6d947},
        .iir_coef[5].coef_b = {0x08002600,0xf00128d3,0x07feb47d},
        .iir_coef[5].coef_a = {0x08000000,0xf00128d3,0x07feda7d},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x079d8bb9,0xf0d79873,0x078e8b2e},
        .iir_coef[0].coef_a = {0x08000000,0xf0d79873,0x072c16e7},
        .iir_coef[1].coef_b = {0x07fed501,0xf003e1e4,0x07fd4979},
        .iir_coef[1].coef_a = {0x08000000,0xf003e1e4,0x07fc1e7a},
        .iir_coef[2].coef_b = {0x08082972,0xf00acf54,0x07ed0a88},
        .iir_coef[2].coef_a = {0x08000000,0xf00acf54,0x07f533fa},
        .iir_coef[3].coef_b = {0x080606cd,0xf00c2228,0x07eddcea},
        .iir_coef[3].coef_a = {0x08000000,0xf00c2228,0x07f3e3b7},
        .iir_coef[4].coef_b = {0x0839e3cf,0xf0968f0d,0x073158ab},
        .iir_coef[4].coef_a = {0x08000000,0xf0968f0d,0x076b3c7a},
        .iir_coef[5].coef_b = {0x07d2392e,0xf146bf0a,0x06f44bd5},
        .iir_coef[5].coef_a = {0x08000000,0xf146bf0a,0x06c68503},
        .iir_coef[6].coef_b = {0x07717052,0xf1748ca2,0x0733c4fc},
        .iir_coef[6].coef_a = {0x08000000,0xf1748ca2,0x06a5354e},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf022991b,0x07ddf8db},
        .iir_coef[0].coef_a = {0x08000000,0xf022991b,0x07ddf8db},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x00036386,0x0006c70c,0x00036386},
        .iir_coef[0].coef_a = {0x08000000,0xf0f490a9,0x0718fd6f},
        .iir_coef[1].coef_b = {0x07ce6468,0xf0850dc3,0x07ad1c41},
        .iir_coef[1].coef_a = {0x08000000,0xf0850dc3,0x077b80a9},
        .iir_coef[2].coef_b = {0x07ffcadf,0xf0029e20,0x07fd972f},
        .iir_coef[2].coef_a = {0x08000000,0xf0029e20,0x07fd620e},
        .iir_coef[3].coef_b = {0x07ffceb2,0xf000f3e2,0x07ff3d72},
        .iir_coef[3].coef_a = {0x08000000,0xf000f3e6,0x07ff0c28},
        .iir_coef[4].coef_b = {0x07fc4d08,0xf0195e0a,0x07ea5e13},
        .iir_coef[4].coef_a = {0x08000000,0xf0195e0a,0x07e6ab1b},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_48k_mode2 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0x0138a664,0xfdefb5da,0x00e496d0},
        .iir_coef[0].coef_a = {0x08000000,0xf0e3e134,0x0727ee2a},
        .iir_coef[1].coef_b = {0x0800173b,0xf0008b4e,0x07ff5d82},
        .iir_coef[1].coef_a = {0x08000000,0xf0008b4c,0x07ff74bb},
        .iir_coef[2].coef_b = {0x08010fa8,0xf0085cb7,0x07f6be01},
        .iir_coef[2].coef_a = {0x08000000,0xf0085cb7,0x07f7cda9},
        .iir_coef[3].coef_b = {0x07e4f464,0xf04ad797,0x07d2cf58},
        .iir_coef[3].coef_a = {0x08000000,0xf04ad797,0x07b7c3bd},
        .iir_coef[4].coef_b = {0x07f5e6c0,0xf01cd685,0x07f1886c},
        .iir_coef[4].coef_a = {0x08000000,0xf01cd685,0x07e76f2c},
        .iir_coef[5].coef_b = {0x0800251c,0xf00121cb,0x07febc41},
        .iir_coef[5].coef_a = {0x08000000,0xf00121cb,0x07fee15d},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x079fbc39,0xf0d2b770,0x07911117},
        .iir_coef[0].coef_a = {0x08000000,0xf0d2b770,0x0730cd50},
        .iir_coef[1].coef_b = {0x07fedc01,0xf003ca9c,0x07fd59bc},
        .iir_coef[1].coef_a = {0x08000000,0xf003ca9c,0x07fc35bd},
        .iir_coef[2].coef_b = {0x0807f89a,0xf00a8e8f,0x07ed7bfe},
        .iir_coef[2].coef_a = {0x08000000,0xf00a8e8f,0x07f57497},
        .iir_coef[3].coef_b = {0x0805e2be,0xf00bd96e,0x07ee496c},
        .iir_coef[3].coef_a = {0x08000000,0xf00bd96e,0x07f42c2b},
        .iir_coef[4].coef_b = {0x083894e6,0xf0931d7e,0x07360439},
        .iir_coef[4].coef_a = {0x08000000,0xf0931d7e,0x076e9920},
        .iir_coef[5].coef_b = {0x07d336b3,0xf13f5f7e,0x06fa166e},
        .iir_coef[5].coef_a = {0x08000000,0xf13f5f7e,0x06cd4d21},
        .iir_coef[6].coef_b = {0x07747d57,0xf16bfc0d,0x073823dd},
        .iir_coef[6].coef_a = {0x08000000,0xf16bfc0d,0x06aca134},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf021c7e1,0x07dec359},
        .iir_coef[0].coef_a = {0x08000000,0xf021c7e1,0x07dec359},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x00033c73,0x000678e7,0x00033c73},
        .iir_coef[0].coef_a = {0x08000000,0xf0eed9b6,0x071e1818},
        .iir_coef[1].coef_b = {0x07cf849e,0xf082057c,0x07aefdd3},
        .iir_coef[1].coef_a = {0x08000000,0xf082057c,0x077e8271},
        .iir_coef[2].coef_b = {0x07ffcc1e,0xf0028e6d,0x07fda5a2},
        .iir_coef[2].coef_a = {0x08000000,0xf0028e6d,0x07fd71bf},
        .iir_coef[3].coef_b = {0x07ffcfd9,0xf000ee2a,0x07ff4201},
        .iir_coef[3].coef_a = {0x08000000,0xf000ee2f,0x07ff11df},
        .iir_coef[4].coef_b = {0x07fc6317,0xf018c68c,0x07eadf16},
        .iir_coef[4].coef_a = {0x08000000,0xf018c68c,0x07e7422d},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_44p1k_mode2 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0x013b6b21,0xfdf38151,0x00e0599c},
        .iir_coef[0].coef_a = {0x08000000,0xf0f802fc,0x0715eb07},
        .iir_coef[1].coef_b = {0x08001949,0xf00097a0,0x07ff4f24},
        .iir_coef[1].coef_a = {0x08000000,0xf000979d,0x07ff686a},
        .iir_coef[2].coef_b = {0x080127a1,0xf0091db0,0x07f5ece1},
        .iir_coef[2].coef_a = {0x08000000,0xf0091db0,0x07f71481},
        .iir_coef[3].coef_b = {0x07e29c21,0xf05194d6,0x07cee45e},
        .iir_coef[3].coef_a = {0x08000000,0xf05194d6,0x07b1807f},
        .iir_coef[4].coef_b = {0x07f503cc,0xf01fc7df,0x07f0434a},
        .iir_coef[4].coef_a = {0x08000000,0xf01fc7df,0x07e54716},
        .iir_coef[5].coef_b = {0x08002864,0xf0013bb8,0x07fe9fa2},
        .iir_coef[5].coef_a = {0x08000000,0xf0013bb8,0x07fec806},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x0797b1b5,0xf0e4a7b8,0x0787cce7},
        .iir_coef[0].coef_a = {0x08000000,0xf0e4a7b8,0x071f7e9d},
        .iir_coef[1].coef_b = {0x07fec235,0xf0042065,0x07fd1dd0},
        .iir_coef[1].coef_a = {0x08000000,0xf0042065,0x07fbe005},
        .iir_coef[2].coef_b = {0x0808ac8e,0xf00b7d32,0x07ebd9fb},
        .iir_coef[2].coef_a = {0x08000000,0xf00b7d32,0x07f4868a},
        .iir_coef[3].coef_b = {0x08066793,0xf00ce564,0x07ecb9ab},
        .iir_coef[3].coef_a = {0x08000000,0xf00ce564,0x07f3213e},
        .iir_coef[4].coef_b = {0x083d6422,0xf09fc8dc,0x0724d900},
        .iir_coef[4].coef_a = {0x08000000,0xf09fc8dc,0x07623d22},
        .iir_coef[5].coef_b = {0x07cf9534,0xf15a7a60,0x06e4daaf},
        .iir_coef[5].coef_a = {0x08000000,0xf15a7a60,0x06b46fe3},
        .iir_coef[6].coef_b = {0x07694fdc,0xf18b8117,0x07282090},
        .iir_coef[6].coef_a = {0x08000000,0xf18b8117,0x0691706c},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf024cb39,0x07dbd999},
        .iir_coef[0].coef_a = {0x08000000,0xf024cb39,0x07dbd999},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x0003d09b,0x0007a136,0x0003d09b},
        .iir_coef[0].coef_a = {0x08000000,0xf103e70d,0x070b5b5f},
        .iir_coef[1].coef_b = {0x07cb60c8,0xf08d2cda,0x07a812f2},
        .iir_coef[1].coef_a = {0x08000000,0xf08d2cda,0x077373bb},
        .iir_coef[2].coef_b = {0x07ffc788,0xf002c847,0x07fd7066},
        .iir_coef[2].coef_a = {0x08000000,0xf002c847,0x07fd37ed},
        .iir_coef[3].coef_b = {0x07ffcb97,0xf001033a,0x07ff3135},
        .iir_coef[3].coef_a = {0x08000000,0xf001033f,0x07fefcd1},
        .iir_coef[4].coef_b = {0x07fc11d5,0xf01af49a,0x07e903e4},
        .iir_coef[4].coef_a = {0x08000000,0xf01af49a,0x07e515b9},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};
#endif

#if (ANC_COEF_LIST_NUM >= 4)
static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_50p7k_mode3 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0xf8001c94,0x0fff1b7f,0xf800c7b4},
        .iir_coef[0].coef_a = {0x08000000,0xf000e498,0x07ff1bce},
        .iir_coef[1].coef_b = {0x0803f8ee,0xf00d9b31,0x07ee71bf},
        .iir_coef[1].coef_a = {0x08000000,0xf00d9b31,0x07f26aad},
        .iir_coef[2].coef_b = {0x08101b6c,0xf029a1cf,0x07c68053},
        .iir_coef[2].coef_a = {0x08000000,0xf029a1cf,0x07d69bc0},
        .iir_coef[3].coef_b = {0x082d26da,0xf079edd7,0x075ed18d},
        .iir_coef[3].coef_a = {0x08000000,0xf079edd7,0x078bf867},
        .iir_coef[4].coef_b = {0x0156f6aa,0xfd9451e0,0x0125ef1e},
        .iir_coef[4].coef_a = {0x08000000,0xf08c5fb5,0x0784d7f2},
        .iir_coef[5].coef_b = {0x0800985a,0xf002e2f9,0x07fc850b},
        .iir_coef[5].coef_a = {0x08000000,0xf002e2f9,0x07fd1d65},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x079d8bb9,0xf0d79873,0x078e8b2e},
        .iir_coef[0].coef_a = {0x08000000,0xf0d79873,0x072c16e7},
        .iir_coef[1].coef_b = {0x0801002f,0xf00292b3,0x07fc6d7c},
        .iir_coef[1].coef_a = {0x08000000,0xf00292b3,0x07fd6dab},
        .iir_coef[2].coef_b = {0x080606cd,0xf00c1f97,0x07eddce9},
        .iir_coef[2].coef_a = {0x08000000,0xf00c1f97,0x07f3e3b6},
        .iir_coef[3].coef_b = {0x0803f8ee,0xf00d9b31,0x07ee71bf},
        .iir_coef[3].coef_a = {0x08000000,0xf00d9b31,0x07f26aad},
        .iir_coef[4].coef_b = {0x0839e3cf,0xf0968f0d,0x073158ab},
        .iir_coef[4].coef_a = {0x08000000,0xf0968f0d,0x076b3c7a},
        .iir_coef[5].coef_b = {0x07d2392e,0xf146bf0a,0x06f44bd5},
        .iir_coef[5].coef_a = {0x08000000,0xf146bf0a,0x06c68503},
        .iir_coef[6].coef_b = {0x07717052,0xf1748ca2,0x0733c4fc},
        .iir_coef[6].coef_a = {0x08000000,0xf1748ca2,0x06a5354e},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf022991b,0x07ddf8db},
        .iir_coef[0].coef_a = {0x08000000,0xf022991b,0x07ddf8db},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x00036386,0x0006c70c,0x00036386},
        .iir_coef[0].coef_a = {0x08000000,0xf0f490a9,0x0718fd6f},
        .iir_coef[1].coef_b = {0x07ce6468,0xf0850dc3,0x07ad1c41},
        .iir_coef[1].coef_a = {0x08000000,0xf0850dc3,0x077b80a9},
        .iir_coef[2].coef_b = {0x07ffcadf,0xf0029e20,0x07fd972f},
        .iir_coef[2].coef_a = {0x08000000,0xf0029e20,0x07fd620e},
        .iir_coef[3].coef_b = {0x07ffceb2,0xf000f3e2,0x07ff3d72},
        .iir_coef[3].coef_a = {0x08000000,0xf000f3e6,0x07ff0c28},
        .iir_coef[4].coef_b = {0x07fc4d08,0xf0195e0a,0x07ea5e13},
        .iir_coef[4].coef_a = {0x08000000,0xf0195e0a,0x07e6ab1b},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_48k_mode3 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0xf8001be9,0x0fff20db,0xf800c306},
        .iir_coef[0].coef_a = {0x08000000,0xf000df3a,0x07ff2127},
        .iir_coef[1].coef_b = {0x0803e12d,0xf00d49af,0x07eedabd},
        .iir_coef[1].coef_a = {0x08000000,0xf00d49af,0x07f2bbe9},
        .iir_coef[2].coef_b = {0x080fbbbd,0xf028a915,0x07c7d5e6},
        .iir_coef[2].coef_a = {0x08000000,0xf028a915,0x07d791a3},
        .iir_coef[3].coef_b = {0x082c1fb1,0xf0770497,0x07627cf9},
        .iir_coef[3].coef_a = {0x08000000,0xf0770497,0x078e9ca9},
        .iir_coef[4].coef_b = {0x01567666,0xfd936b8e,0x01268c31},
        .iir_coef[4].coef_a = {0x08000000,0xf088c978,0x0787a4ad},
        .iir_coef[5].coef_b = {0x080094c8,0xf002d1a8,0x07fc99e9},
        .iir_coef[5].coef_a = {0x08000000,0xf002d1a8,0x07fd2eb2},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x079fbc39,0xf0d2b770,0x07911117},
        .iir_coef[0].coef_a = {0x08000000,0xf0d2b770,0x0730cd50},
        .iir_coef[1].coef_b = {0x0800fa2f,0xf0028343,0x07fc82e7},
        .iir_coef[1].coef_a = {0x08000000,0xf0028343,0x07fd7d16},
        .iir_coef[2].coef_b = {0x0805e2bf,0xf00bd6fc,0x07ee496b},
        .iir_coef[2].coef_a = {0x08000000,0xf00bd6fc,0x07f42c2a},
        .iir_coef[3].coef_b = {0x0803e12d,0xf00d49af,0x07eedabd},
        .iir_coef[3].coef_a = {0x08000000,0xf00d49af,0x07f2bbe9},
        .iir_coef[4].coef_b = {0x083894e6,0xf0931d7e,0x07360439},
        .iir_coef[4].coef_a = {0x08000000,0xf0931d7e,0x076e9920},
        .iir_coef[5].coef_b = {0x07d336b3,0xf13f5f7e,0x06fa166e},
        .iir_coef[5].coef_a = {0x08000000,0xf13f5f7e,0x06cd4d21},
        .iir_coef[6].coef_b = {0x07747d57,0xf16bfc0d,0x073823dd},
        .iir_coef[6].coef_a = {0x08000000,0xf16bfc0d,0x06aca134},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf021c7e1,0x07dec359},
        .iir_coef[0].coef_a = {0x08000000,0xf021c7e1,0x07dec359},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x00033c73,0x000678e7,0x00033c73},
        .iir_coef[0].coef_a = {0x08000000,0xf0eed9b6,0x071e1818},
        .iir_coef[1].coef_b = {0x07cf849e,0xf082057c,0x07aefdd3},
        .iir_coef[1].coef_a = {0x08000000,0xf082057c,0x077e8271},
        .iir_coef[2].coef_b = {0x07ffcc1e,0xf0028e6d,0x07fda5a2},
        .iir_coef[2].coef_a = {0x08000000,0xf0028e6d,0x07fd71bf},
        .iir_coef[3].coef_b = {0x07ffcfd9,0xf000ee2a,0x07ff4201},
        .iir_coef[3].coef_a = {0x08000000,0xf000ee2f,0x07ff11df},
        .iir_coef[4].coef_b = {0x07fc6317,0xf018c68c,0x07eadf16},
        .iir_coef[4].coef_a = {0x08000000,0xf018c68c,0x07e7422d},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};

static const struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_44p1k_mode3 = {
    .anc_cfg_ff_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 6,
        .iir_coef[0].coef_b = {0xf8001e61,0x0fff0d19,0xf800d445},
        .iir_coef[0].coef_a = {0x08000000,0xf000f300,0x07ff0d73},
        .iir_coef[1].coef_b = {0x080438b2,0xf00e75fe,0x07ed57f1},
        .iir_coef[1].coef_a = {0x08000000,0xf00e75fe,0x07f190a3},
        .iir_coef[2].coef_b = {0x08111c18,0xf02c3d57,0x07c2ec12},
        .iir_coef[2].coef_a = {0x08000000,0xf02c3d57,0x07d4082a},
        .iir_coef[3].coef_b = {0x082fe791,0xf081c1c3,0x0754fddf},
        .iir_coef[3].coef_a = {0x08000000,0xf081c1c3,0x0784e570},
        .iir_coef[4].coef_b = {0x0158532e,0xfd96c88e,0x01244f9f},
        .iir_coef[4].coef_a = {0x08000000,0xf0961286,0x077d58d5},
        .iir_coef[5].coef_b = {0x0800a1ee,0xf0031176,0x07fc4d06},
        .iir_coef[5].coef_a = {0x08000000,0xf0031176,0x07fceef4},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_fb_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 7,
        .iir_coef[0].coef_b = {0x0797b1b5,0xf0e4a7b8,0x0787cce7},
        .iir_coef[0].coef_a = {0x08000000,0xf0e4a7b8,0x071f7e9d},
        .iir_coef[1].coef_b = {0x0801104b,0xf002bc26,0x07fc33fa},
        .iir_coef[1].coef_a = {0x08000000,0xf002bc26,0x07fd4445},
        .iir_coef[2].coef_b = {0x08066793,0xf00ce27e,0x07ecb9aa},
        .iir_coef[2].coef_a = {0x08000000,0xf00ce27e,0x07f3213d},
        .iir_coef[3].coef_b = {0x080438b2,0xf00e75fe,0x07ed57f1},
        .iir_coef[3].coef_a = {0x08000000,0xf00e75fe,0x07f190a3},
        .iir_coef[4].coef_b = {0x083d6422,0xf09fc8dc,0x0724d900},
        .iir_coef[4].coef_a = {0x08000000,0xf09fc8dc,0x07623d22},
        .iir_coef[5].coef_b = {0x07cf9534,0xf15a7a60,0x06e4daaf},
        .iir_coef[5].coef_a = {0x08000000,0xf15a7a60,0x06b46fe3},
        .iir_coef[6].coef_b = {0x07694fdc,0xf18b8117,0x07282090},
        .iir_coef[6].coef_a = {0x08000000,0xf18b8117,0x0691706c},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_tt_l = {
        .total_gain = 0,
        .iir_bypass_flag = 0,
        .iir_counter = 1,
        .iir_coef[0].coef_b = {0x08000000,0xf024cb39,0x07dbd999},
        .iir_coef[0].coef_a = {0x08000000,0xf024cb39,0x07dbd999},
        .iir_coef[1].coef_b = {0,0,0},
        .iir_coef[1].coef_a = {0,0,0},
        .iir_coef[2].coef_b = {0,0,0},
        .iir_coef[2].coef_a = {0,0,0},
        .iir_coef[3].coef_b = {0,0,0},
        .iir_coef[3].coef_a = {0,0,0},
        .iir_coef[4].coef_b = {0,0,0},
        .iir_coef[4].coef_a = {0,0,0},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = -24,
    },
    .anc_cfg_mc_l = {
        .total_gain = 512,
        .iir_bypass_flag = 0,
        .iir_counter = 5,
        .iir_coef[0].coef_b = {0x0003d09b,0x0007a136,0x0003d09b},
        .iir_coef[0].coef_a = {0x08000000,0xf103e70d,0x070b5b5f},
        .iir_coef[1].coef_b = {0x07cb60c8,0xf08d2cda,0x07a812f2},
        .iir_coef[1].coef_a = {0x08000000,0xf08d2cda,0x077373bb},
        .iir_coef[2].coef_b = {0x07ffc788,0xf002c847,0x07fd7066},
        .iir_coef[2].coef_a = {0x08000000,0xf002c847,0x07fd37ed},
        .iir_coef[3].coef_b = {0x07ffcb97,0xf001033a,0x07ff3135},
        .iir_coef[3].coef_a = {0x08000000,0xf001033f,0x07fefcd1},
        .iir_coef[4].coef_b = {0x07fc11d5,0xf01af49a,0x07e903e4},
        .iir_coef[4].coef_a = {0x08000000,0xf01af49a,0x07e515b9},
        .iir_coef[5].coef_b = {0,0,0},
        .iir_coef[5].coef_a = {0,0,0},
        .iir_coef[6].coef_b = {0,0,0},
        .iir_coef[6].coef_a = {0,0,0},
        .iir_coef[7].coef_b = {0,0,0},
        .iir_coef[7].coef_a = {0,0,0},
        .dac_gain_offset = 0,
        .adc_gain_offset = 0,
    }
};
#endif
/**add by lmzhang,20201221*/

const struct_anc_cfg * anc_coef_list_50p7k[ANC_COEF_LIST_NUM] = {
    #if (ANC_COEF_LIST_NUM >= 1)
    &AncFirCoef_50p7k_mode0,
    #endif
    #if (ANC_COEF_LIST_NUM >= 2)
    &AncFirCoef_50p7k_mode1,
    #endif
    #if (ANC_COEF_LIST_NUM >= 3)
    &AncFirCoef_50p7k_mode2,
    #endif
    #if (ANC_COEF_LIST_NUM >= 4)
    &AncFirCoef_50p7k_mode3,
    #endif
};

const struct_anc_cfg * anc_coef_list_48k[ANC_COEF_LIST_NUM] = {
    #if (ANC_COEF_LIST_NUM >= 1)
    &AncFirCoef_48k_mode0,
    #endif
    #if (ANC_COEF_LIST_NUM >= 2)
    &AncFirCoef_48k_mode1,
    #endif
    #if (ANC_COEF_LIST_NUM >= 3)
    &AncFirCoef_48k_mode2,
    #endif
    #if (ANC_COEF_LIST_NUM >= 4)
    &AncFirCoef_48k_mode3,
    #endif
};

const struct_anc_cfg * anc_coef_list_44p1k[ANC_COEF_LIST_NUM] = {
    #if (ANC_COEF_LIST_NUM >= 1)
    &AncFirCoef_44p1k_mode0,
    #endif
    #if (ANC_COEF_LIST_NUM >= 2)
    &AncFirCoef_44p1k_mode1,
    #endif
    #if (ANC_COEF_LIST_NUM >= 3)
    &AncFirCoef_44p1k_mode2,
    #endif
    #if (ANC_COEF_LIST_NUM >= 4)
    &AncFirCoef_44p1k_mode3,
    #endif
};

static const struct_psap_cfg POSSIBLY_UNUSED hwlimiter_para_44p1k_mode0 = {
    .psap_cfg_l = {
        .psap_total_gain = 811,
        .psap_band_num = 2,
        .psap_band_gain={0x3f8bd79e, 0x08000000},
        .psap_iir_coef[0].iir0.coef_b={0x04fa1dfc, 0x0c6d6b7e, 0x08000000, 0x00000000},
        .psap_iir_coef[0].iir0.coef_a={0x08000000, 0x0c6d6b7e, 0x04fa1dfc, 0x00000000},
        .psap_iir_coef[0].iir1.coef_b={0x04f9f115, 0x11279bfc, 0x1404d0c8, 0x08000000},
        .psap_iir_coef[0].iir1.coef_a={0x08000000, 0x1404d0c8, 0x11279bfc, 0x04f9f115},
        .psap_cpd_cfg[0]={0x5740, 0x0000, 0x4e02, 0x0000, 0x3b85, 0x0000, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 0x7333, 0x0ccd, 0},
        .psap_limiter_cfg={524287, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 127},
        .psap_dehowling_cfg.dehowling_delay=0,
        .psap_dehowling_cfg.dehowling_l.total_gain=0,
        .psap_dehowling_cfg.dehowling_l.iir_bypass_flag=0,
        .psap_dehowling_cfg.dehowling_l.iir_counter=1,
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_b={0x08000000, 0xf096cb51, 0x07733cd7},
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_a={0x08000000, 0xf096cb51, 0x07733cd7},
        .psap_dehowling_cfg.dehowling_l.dac_gain_offset=0,
        .psap_dehowling_cfg.dehowling_l.adc_gain_offset=0,
        .psap_type = 3,
        .psap_dac_gain_offset=0,
        .psap_adc_gain_offset=0,
    }
};
static const struct_psap_cfg POSSIBLY_UNUSED hwlimiter_para_48k_mode0 = {
    .psap_cfg_l = {
        .psap_total_gain = 811,
        .psap_band_num = 2,
        .psap_band_gain={0x3f8bd79e, 0x08000000},
        .psap_iir_coef[0].iir0.coef_b={0x0364529c, 0x09dd9c0b, 0x08000000, 0x00000000},
        .psap_iir_coef[0].iir0.coef_a={0x08000000, 0x09dd9c0b, 0x0364529c, 0x00000000},
        .psap_iir_coef[0].iir1.coef_b={0x0361ed64, 0x0cc9bec2, 0x109eeca2, 0x08000000},
        .psap_iir_coef[0].iir1.coef_a={0x08000000, 0x109eeca2, 0x0cc9bec2, 0x0361ed64},
        .psap_cpd_cfg[0]={0x5740, 0x0000, 0x4e02, 0x0000, 0x3b85, 0x0000, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 0x7333, 0x0ccd, 0},
        .psap_limiter_cfg={524287, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 127},
        .psap_dehowling_cfg.dehowling_delay=0,
        .psap_dehowling_cfg.dehowling_l.total_gain=0,
        .psap_dehowling_cfg.dehowling_l.iir_bypass_flag=0,
        .psap_dehowling_cfg.dehowling_l.iir_counter=1,
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_b={0x08000000, 0xf08a323c, 0x077e4bc1},
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_a={0x08000000, 0xf08a323c, 0x077e4bc1},
        .psap_dehowling_cfg.dehowling_l.dac_gain_offset=0,
        .psap_dehowling_cfg.dehowling_l.adc_gain_offset=0,
        .psap_type = 3,
        .psap_dac_gain_offset=0,
        .psap_adc_gain_offset=0,
    }
};
static const struct_psap_cfg POSSIBLY_UNUSED hwlimiter_para_50p7k_mode0 = {
    .psap_cfg_l = {
        .psap_total_gain = 811,
        .psap_band_num = 2,
        .psap_band_gain={0x3f8bd79e, 0x08000000},
        .psap_iir_coef[0].iir0.coef_b={0x03c46400, 0x0a8a3bd5, 0x08000000, 0x00000000},
        .psap_iir_coef[0].iir0.coef_a={0x08000000, 0x0a8a3bd5, 0x03c46400, 0x00000000},
        .psap_iir_coef[0].iir1.coef_b={0x03c305c1, 0x0de12c8a, 0x118f7298, 0x08000000},
        .psap_iir_coef[0].iir1.coef_a={0x08000000, 0x118f7298, 0x0de12c8a, 0x03c305c1},
        .psap_cpd_cfg[0]={0x5740, 0x0000, 0x4e02, 0x0000, 0x3b85, 0x0000, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 0x7333, 0x0ccd, 0},
        .psap_limiter_cfg={524287, 0x7cf7, 0x0309, 0x7ff0, 0x0010, 127},
        .psap_dehowling_cfg.dehowling_delay=0,
        .psap_dehowling_cfg.dehowling_l.total_gain=0,
        .psap_dehowling_cfg.dehowling_l.iir_bypass_flag=0,
        .psap_dehowling_cfg.dehowling_l.iir_counter=1,
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_b={0x08000000, 0xf08d9c03, 0x077b49d0},
        .psap_dehowling_cfg.dehowling_l.iir_coef[0].coef_a={0x08000000, 0xf08d9c03, 0x077b49d0},
        .psap_dehowling_cfg.dehowling_l.dac_gain_offset=0,
        .psap_dehowling_cfg.dehowling_l.adc_gain_offset=0,
        .psap_type = 3,
        .psap_dac_gain_offset=0,
        .psap_adc_gain_offset=0,
    }
};

const struct_psap_cfg * hwlimiter_para_list_50p7k[HWLIMITER_PARA_LIST_NUM] = {
    &hwlimiter_para_50p7k_mode0,
};

const struct_psap_cfg * hwlimiter_para_list_48k[HWLIMITER_PARA_LIST_NUM] = {
    &hwlimiter_para_48k_mode0,
};

const struct_psap_cfg * hwlimiter_para_list_44p1k[HWLIMITER_PARA_LIST_NUM] = {
    &hwlimiter_para_44p1k_mode0,
};

static const struct_psap_cfg POSSIBLY_UNUSED PsapFirCoef_50p7k_mode0 = {
    .psap_cfg_l = {
        .psap_total_gain = 723,
        .psap_band_num = 9,
        .psap_band_gain={0, 476222470, 672682118, 534330399, 950188747, 1066129310, 1066129310, 424433723, 0},
        .psap_iir_coef[0].iir0.coef_b={131557370, -265754754, 134217728, 0},
        .psap_iir_coef[0].iir0.coef_a={134217728, -265754754, 131557370, 0},
        .psap_iir_coef[0].iir1.coef_b={-131557370, 397299677, -399959783, 134217728},
        .psap_iir_coef[0].iir1.coef_a={134217728, -399959783, 397299677, -131557370},
        .psap_iir_coef[1].iir0.coef_b={129986488, -264152445, 134217728, 0},
        .psap_iir_coef[1].iir0.coef_a={134217728, -264152445, 129986488, 0},
        .psap_iir_coef[1].iir1.coef_b={-129986488, 394107454, -398337669, 134217728},
        .psap_iir_coef[1].iir1.coef_a={134217728, -398337669, 394107454, -129986488},
        .psap_iir_coef[2].iir0.coef_b={125134130, -259108702, 134217728, 0},
        .psap_iir_coef[2].iir0.coef_a={134217728, -259108702, 125134130, 0},
        .psap_iir_coef[2].iir1.coef_b={-125134129, 384097929, -393171000, 134217728},
        .psap_iir_coef[2].iir1.coef_a={134217728, -393171000, 384097929, -125134129},
        .psap_iir_coef[3].iir0.coef_b={120220377, -253849171, 134217728, 0},
        .psap_iir_coef[3].iir0.coef_a={134217728, -253849171, 120220377, 0},
        .psap_iir_coef[3].iir1.coef_b={-120220366, 373726258, -387683554, 134217728},
        .psap_iir_coef[3].iir1.coef_a={134217728, -387683554, 373726258, -120220366},
        .psap_iir_coef[4].iir0.coef_b={109848832, -242200690, 134217728, 0},
        .psap_iir_coef[4].iir0.coef_a={134217728, -242200690, 109848832, 0},
        .psap_iir_coef[4].iir1.coef_b={-109848632, 351018377, -375156857, 134217728},
        .psap_iir_coef[4].iir1.coef_a={134217728, -375156857, 351018377, -109848632},
        .psap_iir_coef[5].iir0.coef_b={81171724, -205166742, 134217728, 0},
        .psap_iir_coef[5].iir0.coef_a={134217728, -205166742, 81171724, 0},
        .psap_iir_coef[5].iir1.coef_b={-81156770, 281801460, -331721088, 134217728},
        .psap_iir_coef[5].iir1.coef_a={134217728, -331721088, 281801460, -81156770},
        .psap_iir_coef[6].iir0.coef_b={66181540, -181899702, 134217728, 0},
        .psap_iir_coef[6].iir0.coef_a={134217728, -181899702, 66181540, 0},
        .psap_iir_coef[6].iir1.coef_b={-66113394, 241306967, -301500735, 134217728},
        .psap_iir_coef[6].iir1.coef_a={134217728, -301500735, 241306967, -66113394},
        .psap_iir_coef[7].iir0.coef_b={62863906, -176244865, 134217728, 0},
        .psap_iir_coef[7].iir0.coef_a={134217728, -176244865, 62863906, 0},
        .psap_iir_coef[7].iir1.coef_b={-62771443, 231892680, -293809571, 134217728},
        .psap_iir_coef[7].iir1.coef_a={134217728, -293809571, 231892680, -62771443},
        .psap_cpd_cfg[0]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[1]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[2]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[3]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[4]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[5]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[6]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_cpd_cfg[7]={27069, 0, 27069, 0, 10505, -29491, 32689, 79, 32766, 2, 29491, 3277, 0},
        .psap_limiter_cfg={524287, 31991, 777, 32752, 16, 0},
        .psap_dac_gain_offset=0,
        .psap_adc_gain_offset=-12,
    }

};

const struct_psap_cfg * psap_coef_list_50p7k[PSAP_COEF_LIST_NUM] = {
    &PsapFirCoef_50p7k_mode0,
};

const struct_psap_cfg * psap_coef_list_48k[PSAP_COEF_LIST_NUM] = {
    &PsapFirCoef_50p7k_mode0,
};

const struct_psap_cfg * psap_coef_list_44p1k[PSAP_COEF_LIST_NUM] = {
    &PsapFirCoef_50p7k_mode0,
};

const IIR_CFG_T audio_eq_sw_iir_cfg = {
    .gain0 = 0,
    .gain1 = 0,
    .num = 5,
    .param = {
        {IIR_TYPE_PEAK, .0,   200,   2},
        {IIR_TYPE_PEAK, .0,   600,  2},
        {IIR_TYPE_PEAK, .0,   2000.0, 2},
        {IIR_TYPE_PEAK, .0,  6000.0, 2},
        {IIR_TYPE_PEAK, .0,  12000.0, 2}
    }
};

const IIR_CFG_T * const audio_eq_sw_iir_cfg_list[EQ_SW_IIR_LIST_NUM]={
    &audio_eq_sw_iir_cfg,
};

const FIR_CFG_T audio_eq_hw_fir_cfg_44p1k = {
    .gain = 0.0f,
    .len = 384,
    .coef =
    {
        (1<<23)-1,
    }
};

const FIR_CFG_T audio_eq_hw_fir_cfg_48k = {
    .gain = 0.0f,
    .len = 384,
    .coef =
    {
        (1<<23)-1,
    }
};


const FIR_CFG_T audio_eq_hw_fir_cfg_96k = {
    .gain = 0.0f,
    .len = 384,
    .coef =
    {
        (1<<23)-1,
    }
};

const FIR_CFG_T * const audio_eq_hw_fir_cfg_list[EQ_HW_FIR_LIST_NUM]={
    &audio_eq_hw_fir_cfg_44p1k,
    &audio_eq_hw_fir_cfg_48k,
    &audio_eq_hw_fir_cfg_96k,
};

//hardware dac iir eq
//#define DEFALUT_EQ
//#define BO003_EQ_V1_02_22
#define BO003_EQ_V1_02_24

#if defined DEFALUT_EQ
//hardware dac iir eq
const IIR_CFG_T audio_eq_hw_dac_iir_cfg = {
#if defined(AUDIO_HEARING_COMPSATN)
    .gain0 = -22,
    .gain1 = -22,
#else
    .gain0 = -10.0f,
    .gain1 = -10.0f,
#endif
    .num = 8,
    .param = {
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
        {IIR_TYPE_PEAK, 0,   1000.0,   0.7},
    }
};
#elif defined (BO003_EQ_V1_02_22)
//hardware dac iir eq
const IIR_CFG_T audio_eq_hw_dac_iir_cfg = {
    .gain0 = -10.0f,
    .gain1 = -10.0f,
    .num = 7,
    .param = {
        {IIR_TYPE_HIGH_PASS,      -3,    20.0,  0.75},
        {IIR_TYPE_PEAK,      1,     30.0,     	0.5},
        {IIR_TYPE_PEAK,      1,     80.0,     	0.6},
        {IIR_TYPE_PEAK,     -4.5,   180.0,     	0.6},        
        {IIR_TYPE_PEAK,     3.5,    3200.0,     0.75},
        {IIR_TYPE_PEAK,     -3,    	6000.0,     5.0},
        {IIR_TYPE_PEAK,      9,     15000.0,    0.75},
    }
};
#elif defined (BO003_EQ_V1_02_24)
//hardware dac iir eq
const IIR_CFG_T audio_eq_hw_dac_iir_cfg = {
    .gain0 = -11.0f,
    .gain1 = -11.0f,
    .num = 9,
    .param = {
        {IIR_TYPE_PEAK,      2,     30.0,     	0.75},
        {IIR_TYPE_PEAK,      3,     80.0,     	1},
        {IIR_TYPE_PEAK,     -5,   	160.0,     	0.8},    
        {IIR_TYPE_PEAK,      1.5,   500.0,     	1.5},
        {IIR_TYPE_PEAK,      2,     1500.0,     0.7},        
        {IIR_TYPE_PEAK,      5,     3750.0,     0.7},
        {IIR_TYPE_PEAK,     -3,     9000.0,     5},
        {IIR_TYPE_PEAK,      6,    12000.0,    	1.5},
        {IIR_TYPE_PEAK,      9,    15000.0,    	1},
    }
};
	
const IIR_CFG_T audio_eq_hw_dac_iir_india_cfg = 	
{
    .gain0 = -7.0f,
    .gain1 = -7.0f,
    .num = 8,
    .param = {
        {IIR_TYPE_PEAK,      5,     30.0,     	0.75},
        {IIR_TYPE_PEAK,      6,     70.0,     	1},
        {IIR_TYPE_PEAK,     -5,   	160.0,     	0.8},    
        {IIR_TYPE_PEAK,      1.5,   500.0,     	1.5},
        {IIR_TYPE_PEAK,      2,     1500.0,     0.7},        
        {IIR_TYPE_PEAK,      5,     3750.0,     0.7},
        {IIR_TYPE_PEAK,      6,    12000.0,    	1.5},
        {IIR_TYPE_PEAK,      9,    15000.0,    	1},
    }
};	

const IIR_CFG_T audio_eq_hw_dac_iir_hear_cfg = {
    .gain0 = -1.0f,
    .gain1 = -1.0f,
    .num = 8,
    .param = {
        {IIR_TYPE_HIGH_PASS,      -3,    20.0,  0.75},
        {IIR_TYPE_PEAK,      1,     30.0,     	0.5},
        {IIR_TYPE_PEAK,      1,     80.0,     	0.6},
        {IIR_TYPE_PEAK,     -4.5,   160.0,     	0.8},    
        {IIR_TYPE_PEAK,      2,     1250.0,     0.9},        
        {IIR_TYPE_PEAK,      3,     3750.0,     0.8},
        {IIR_TYPE_PEAK,     -3,    	6000.0,     5.0},
        {IIR_TYPE_PEAK,      9,     15000.0,    0.75},
    }
};	

#endif

const IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_dac_iir_cfg_list[EQ_HW_DAC_IIR_LIST_NUM]={
	&audio_eq_hw_dac_iir_india_cfg,
    &audio_eq_hw_dac_iir_cfg,
	&audio_eq_hw_dac_iir_hear_cfg,
};

//hardware dac iir eq
const IIR_CFG_T audio_eq_hw_adc_iir_adc_cfg = {
    .gain0 = 0,
    .gain1 = 0,
    .num = 1,
    .param = {
        {IIR_TYPE_PEAK, 0.0,   1000.0,   0.7},
    }
};

const IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_adc_iir_cfg_list[EQ_HW_ADC_IIR_LIST_NUM]={
    &audio_eq_hw_adc_iir_adc_cfg,
};



//hardware iir eq
const IIR_CFG_T audio_eq_hw_iir_cfg = {
    .gain0 = 0,
    .gain1 = 0,
    .num = 8,
    .param = {
        {IIR_TYPE_PEAK, -10.1,   100.0,   7},
        {IIR_TYPE_PEAK, -10.1,   400.0,   7},
        {IIR_TYPE_PEAK, -10.1,   700.0,   7},
        {IIR_TYPE_PEAK, -10.1,   1000.0,   7},
        {IIR_TYPE_PEAK, -10.1,   3000.0,   7},
        {IIR_TYPE_PEAK, -10.1,   5000.0,   7},
        {IIR_TYPE_PEAK, -10.1,   7000.0,   7},
        {IIR_TYPE_PEAK, -10.1,   9000.0,   7},
            
    }
};

const IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_iir_cfg_list[EQ_HW_IIR_LIST_NUM]={
    &audio_eq_hw_iir_cfg,
};

const DrcConfig audio_drc_cfg = {
     .knee = 3,
     .filter_type = {14, -1},
     .band_num = 2,
     .look_ahead_time = 10,
     .band_settings = {
         {-20, 0, 2, 3, 3000, 1},
         {-20, 0, 2, 3, 3000, 1},
     }
 };

const LimiterConfig audio_limiter_cfg = {
    .knee = 2,
    .look_ahead_time = 10,
    .threshold = -20,
    .makeup_gain = 19,
    .ratio = 1000,
    .attack_time = 3,
    .release_time = 3000,
};

const SpectrumFixConfig audio_spectrum_cfg = {
    .freq_num = 9,
    .freq_list = {200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800},
};

/**0:left;1:right;2:unknown*/
ear_side_e app_tws_get_earside(void)
{
    static ear_side_e earside = UNKNOWN_SIDE;

    if(earside == UNKNOWN_SIDE)
        earside = (ear_side_e)!hal_gpio_pin_get_val(app_tws_ear_side_cfg.pin);

    return earside;
}

/**ear hw version*/
uint8_t app_get_ear_hw_version(void)
{
    uint8_t hw_ver = 0;
    hw_ver = hal_gpio_pin_get_val(ear_hw_version_cfg[1].pin) << 1;
    hw_ver |= hal_gpio_pin_get_val(ear_hw_version_cfg[0].pin);

    return hw_ver;
}

/**ear hw shipmode*/
void app_tws_entry_shipmode(void)
{
	//longz 20210219 add  gpio0_2 high, poweroff

	hal_gpio_pin_set((enum HAL_GPIO_PIN_T)app_ear_shipmode_cfg.pin);
}

#if defined(__FORCE_OTA_BIN_UPDATE__)
BOOT_UPDATE_SECTION_LOC const uint8_t boot_bin[] = {
#include "./update_ota_bin/new_ota_bin_hex.txt"
};

uint32_t get_ota_bin_size(void)
{
    return sizeof(boot_bin);
}
#endif

extern uint8_t xui_dev_hw_version[2];
int tgt_hardware_setup(void)
{
#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
    for (uint8_t i=0;i<3;i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_ibrt_indication_pinmux_pwl[i], 1);
        if(i==0)
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin, HAL_GPIO_DIR_OUT, 0);
        else
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin, HAL_GPIO_DIR_OUT, 1);
    }
#endif

    /**tws side role*/
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_tws_ear_side_cfg, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_tws_ear_side_cfg.pin, HAL_GPIO_DIR_IN, 1);

    /**shipmode pin*/
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_ear_shipmode_cfg, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_ear_shipmode_cfg.pin, HAL_GPIO_DIR_OUT, 0);
	hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)app_txon_ctrl_cfg.pin);

    /**ear hw version*/
    for (uint8_t i=0;i<2;i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&ear_hw_version_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)ear_hw_version_cfg[i].pin, HAL_GPIO_DIR_IN, 0);
    }

    uint8_t version = app_get_ear_hw_version();
    xui_dev_hw_version[0] = 0x01;
    xui_dev_hw_version[1] = version;

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl, sizeof(cfg_hw_pinmux_pwl)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_indicator_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin, HAL_GPIO_DIR_IN, 1);
    }

    if (app_txon_ctrl_cfg.pin != HAL_IOMUX_PIN_NUM){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_txon_ctrl_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_txon_ctrl_cfg.pin, HAL_GPIO_DIR_OUT, 1);
        hal_gpio_pin_set((enum HAL_GPIO_PIN_T)app_txon_ctrl_cfg.pin);
    }

    if (ldo_en_cfg.pin != HAL_IOMUX_PIN_NUM) { 
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&ldo_en_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)ldo_en_cfg.pin, HAL_GPIO_DIR_OUT, 1);
    }
    
    return 0;
}

