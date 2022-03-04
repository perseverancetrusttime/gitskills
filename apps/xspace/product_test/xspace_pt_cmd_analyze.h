#ifndef __XSPACE_PT_CMD_ANALYZE_H__
#define __XSPACE_PT_CMD_ANALYZE_H__

#if defined(__XSPACE_PRODUCT_TEST__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"


#if defined(__XSPACE_PT_DEBUG__)
#define XPT_TRACE(num, str, ...)  TRACE(num+2, "[XPT] %s,line=%d," str, __func__, __LINE__, ##__VA_ARGS__)
#else
#define XPT_TRACE(num, str, ...)
#endif


typedef enum {
    XPT_CMD_NONE,
    XPT_CMD_START_TEST = 0x80,
    XPT_CMD_ENTER_DUT = 0x81,
    XPT_CMD_REBOOT = 0x82,
    XPT_CMD_TALK_MIC_TEST = 0x83,
    XPT_CMD_ENC_MIC_TEST = 0x84,
    XPT_CMD_GET_MAC_ADDR = 0x85,
    XPT_CMD_IR_TEST = 0x86,
    XPT_CMD_HALL_SWITCH_TEST = 0x87,
    XPT_CMD_TOUCH_KEY_TEST = 0x88,
    XPT_CMD_WRITE_BT_ADDR = 0x89,
    XPT_CMD_WRITE_BLE_ADDR = 0x8A,
    XPT_CMD_WRITE_RF_CALIB = 0x8B,
    XPT_CMD_LED_TEST = 0x8C,
    XPT_CMD_GET_DEVICE_INFO = 0x8D,
    XPT_CMD_STOP_TEST = 0x8E,
    XPT_CMD_GET_BATTERY_INFO = 0x8F,
    XPT_CMD_GET_NTC_TMP = 0x90,
    XPT_CMD_GET_VERSION = 0x91,
    XPT_CMD_GET_BLE_ADDR = 0x92,
    XPT_CMD_TALK_MIC_LOOPBACK_TEST = 0x93,
    XPT_CMD_ENC_MIC_LOOPBACK_TEST = 0x94,
    XPT_CMD_GSENSOR_TEST = 0x95,
	XPT_CMD_GET_BT_CHIP_UID_TEST = 0x96,		
    XPT_CMD_HALL_CTRL = 0x97,    
	XPT_CMD_CHECK_LHDC_LINK_KEY_TEST = 0x98,
	XPT_CMD_WRITE_LHDC_LINK_KEY_TEST = 0x99,
	XPT_CMD_READ_LHDC_LINK_KEY_TEST = 0x9A,	
    XPT_CMD_TOUCH_CALIB_TEST = 0x9B,
    XPT_CMD_GET_OP_MODE_TEST = 0x9C,
    XPT_CMD_BT_DISCONNECT_TEST = 0x9D,
    XPT_CMD_GET_AUDIO_STATUS_TEST = 0x9E,
    XPT_CMD_ANSWER_CALL_TEST = 0x9F,
    XPT_CMD_HANGUP_CALL_TEST = 0xA0,
    XPT_CMD_MUSIC_PLAY_TEST = 0xA1,
    XPT_CMD_MUSIC_PAUSE_TEST = 0xA2,
    XPT_CMD_MUSIC_FORWARD_TEST = 0xA3,
    XPT_CMD_MUSIC_BACKWARD_TEST = 0xA4,
    XPT_CMD_GET_VOLUME_TEST = 0xA5,
    XPT_CMD_SWITCH_ROLE_TEST = 0xA6,
    XPT_CMD_GET_ROLE_TEST = 0xA7,
    XPT_CMD_CHECK_RESTORE_FACTORY = 0xA8,
    XPT_CMD_DISCONNECT_HFP_TEST = 0xA9,
    XPT_CMD_DISCONNECT_A2DP_TEST = 0xAA,
    XPT_CMD_RESET_FACTORY_TEST = 0xAB,
    XPT_CMD_GET_EAR_COLOR_TEST = 0xAC,
    XPT_CMD_BT_POWEROFF_TEST = 0xAD,
    XPT_CMD_BT_RESET_TEST = 0xAE,
    XPT_CMD_STAR_PAIR_TEST = 0xAF,    
    XPT_CMD_LONG_STANDBY_TEST = 0xB0,
    XPT_CMD_GET_DEV_MAC_TEST = 0xB1,
    XPT_CMD_VOLUME_UP_TEST = 0xB2,
    XPT_CMD_VOLUME_DOWN_TEST = 0xB3,
	XPT_CMD_GET_BT_NAME_TEST = 0xB5,
    XPT_CMD_PRESSURE_TEST = 0xB6,
    XPT_CMD_GET_ENC_ARITHMETIC_TEST = 0xB7,
	XPT_CMD_GET_BT_HW_VERSION_TEST = 0xB8,	
    XPT_CMD_ANC_TEST = 0xB9,
    XPT_CMD_FB_MIC_TEST = 0xBA, 
    XPT_CMD_GET_EAR_SIDE = 0xBB,	
    XPT_CMD_GET_PRESURE_CHIPID = 0xBC,	 
	XPT_CMD_ANC_MODE_STATE = 0xC0,	
	XPT_CMD_GET_EAR_INDIA_TEST = 0xC1,
	XPT_CMD_GET_IS_CHANYIN_TEST = 0xC2,
	XPT_CMD_GET_IS_BURN_ANC_TEST = 0xC3,
    XPT_CMD_CNT
} xpt_cmd_id_enum;

bool xpt_get_mode_status(void);

typedef enum {
    XPT_STATUS_NORMAL_MODE,
    XPT_STATUS_TEST_MODE,        
} xpt_status_e;

typedef enum {
    XPT_TEST_PATH_TRACE_UART,
    XPT_TEST_PATH_XBUS,
#if defined (__SKH_APP_DEMO_USING_SPP__)
    XPT_TEST_PATH_SPP,
#endif
} xpt_test_path_e;


typedef enum{
    XPT_RSP_SUCCESS = 0x01,
    XPT_RSP_NOT_TEST_MODE,
    XPT_RSP_FLASH_READ_FAIL,
    XPT_RSP_FACTORY_ZERO_INVALID,
    XPT_RSP_SENSOR_INIT_FAIL,
    XPT_RSP_SENSOR_NOT_INT,
    XPT_RSP_FLASH_WRITE_FAIL,
    XPT_RSP_DATA_INVALID,
    XPT_RSP_EARPHONE_TIMEOUT,
    XPT_RSP_NO_INCOMING_CALL,
    XPT_RSP_NOT_IN_CALLING,
    XPT_RSP_NOT_SUCCESS,
    XPT_RSP_A2DP_NOT_CONNECTED,
    XPT_RSP_HFP_NOT_CONNECTED,
    XPT_RSP_NO_SUPPORT_CMD
    
} xpt_response_error_e;

typedef struct {
    xpt_status_e status;
    xpt_test_path_e test_path;
    xpt_cmd_id_enum curr_cmd_id;
    
} xpt_context_s;


typedef struct {
    xpt_cmd_id_enum cmd_id;
    const uint8_t *cmd_name;
    bool need_start_cmd;    /* First need send XPT_CMD_START_TEST cmd. */
    int32_t (* cmd_handler)(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
    
} xpt_cmd_config_s;

typedef enum {
	XPT_PRESSURE_DEVICE_WAKEUP_ITEM = 1,	
	XPT_PRESSURE_EN_ITEM,
	XPT_PRESSURE_CHECK_ITEM,
	XPT_PRESSURE_FINSH_ITEM,
	
	XPT_PRESSURE_READ_RAWDATA_ITEM,//5
	XPT_PRESSURE_READ_PROCESSED_DATA_ITEM,	
	XPT_PRESSURE_READ_NOISE_ITEM,
	XPT_PRESSURE_READ_OFFSET_ITEM,
	XPT_PRESSURE_READ_PRESSURE_ITEM,
	
	XPT_PRESSURE_READ_PRESS_LEVEL_ITEM,//10
	XPT_PRESSURE_WRITE_PRESS_LEVEL_ITEM,
	XPT_PRESSURE_READ_CALIBRATION_FACTOR_ITEM,
	XPT_PRESSURE_WRITE_CALIBRATION_FACTOR_ITEM,
	
	XPT_PRESSURE_TEST_ITEM_TOTAL,
} xpt_cmd_CSA37F71_parameter;

typedef struct{
    unsigned char calibration_channel;
    unsigned char calibration_progress;
    unsigned short calibration_factor;
    short press_adc_1st;
    short press_adc_2nd;
	short press_adc_3rd;
}CS_CALIBRATION_RESULT_Def;

typedef enum {
    XPT_PRESSURE_CALI_READY = 1,
    XPT_PRESSURE_CALI_PRESS,
    XPT_PRESSURE_CALI_WRITE,
    XPT_PRESSURE_TEST_READY,
    XPT_PRESSURE_TEST_PRESS,
    XPT_PRESSURE_TEST_FINISH,
    XPT_PRESSURE_TEST_RAWDATA,    
} xpt_cmd_aw8680_parameter;

extern int32_t xpt_cmd_recv_cmd_from_xbus(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len);
extern int32_t xpt_cmd_recv_cmd_from_trace_uart(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len);
#if defined (__SKH_APP_DEMO_USING_SPP__)
extern int32_t xpt_cmd_recv_cmd_from_spp(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len);
#endif
extern void xpt_cmd_response(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len);
extern void xspace_pt_init(void);


#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_PRODUCT_TEST__)

#endif  // (__XSPACE_PT_CMD_ANALYZE_H__)

