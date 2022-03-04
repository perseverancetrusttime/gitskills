#ifndef _NEXTINPUT_ADAPTER_H_
#define _NEXTINPUT_ADAPTER_H_


#ifdef __cplusplus
extern "C" {
#endif

#define __NEXTINPUT_DEBUG__
#if 0//defined(__NEXTINPUT_DEBUG__)
#define NEXTINPUT_TRACE(num, str, ...)  TRACE(num, "[NEXTINPUT] "str, ##__VA_ARGS__)
#else
#define NEXTINPUT_TRACE(num, str, ...)
#endif

//#define NEXTINPUT_DATA_PRINTF_DEBUG

#define NEXTINPUT_I2C_TYPE					(XSPACE_HW_I2C)
#define NEXTINPUT_I2C_ADDR					0x4A

#define KEY_DBLCLICK_THRESH_MS       500
#define KEY_LLPRESS_THRESH_MS        3*1000
#define KEY_LPRESS_THRESH_MS         1*1000

typedef enum
{
    KEY_LONG_EVENT_NONE = 0,
    KEY_LONG_EVENT_LONGPRESS = 1,
    KEY_LONG_EVENT_LLONGPRESS = 2,
} key_long_event_e;

struct NEXTINPUT_KEY_STATUS_T {
    key_long_event_e long_press;
    uint8_t key_status;
    uint32_t time_updown;
    uint32_t time_click;
    uint8_t cnt_click;
    uint32_t press_time_ticks;
};

typedef enum {
	NI_CALI_DEVICE_WAKEUP_ITEM = 1,
	NI_CALI_EN_ITEM,
	NI_CALI_CHECK_ITEM,
	NI_CALI_DISEN_ITEM,
	NI_CALI_READ_RAWDATA_ITEM, //5
	NI_CALI_READ_PROCESSED_DATA_ITEM,
	NI_CALI_READ_NOISE_ITEM,
	NI_CALI_READ_OFFSET_ITEM,
	NI_CALI_READ_PRESSURE_ITEM,
	NI_CALI_READ_PRESS_LEVEL_ITEM,	//10
	NI_CALI_WRITE_PRESS_LEVEL_ITEM,
	NI_CALI_READ_CALIBRATION_FACTOR_ITEM,
	NI_CALI_WRITE_CALIBRATION_FACTOR_ITEM,
	
	NI_CALI_TEST_ITEM_TOTAL,
} nextinput_cali_items;

#ifdef __cplusplus
}
#endif
#endif /*_NEXTINPUT_ADAPTER_H_*/
