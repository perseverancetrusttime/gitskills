#ifndef _CSA37F71_ADAPTER_H_
#define _CSA37F71_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __CSA37F71_DEBUG__
#if defined(__CSA37F71_DEBUG__)
#define CSA37F71_TRACE(num, str, ...)  TRACE(num, "[CSA37F71] "str, ##__VA_ARGS__)
#else
#define CSA37F71_TRACE(num, str, ...)
#endif

//#define CSA37F71_DATA_PRINTF_DEBUG

#define KEY_DBLCLICK_THRESH_MS       500
#define KEY_LLPRESS_THRESH_MS        3*1000
#define KEY_LPRESS_THRESH_MS         1*1000


#define CSA37F71_PRESS_LEVEL_LOW	100
#define CSA37F71_PRESS_LEVEL_MID	150
#define CSA37F71_PRESS_LEVEL_HIG	300

typedef enum
{
    KEY_LONG_EVENT_NONE = 0,
    KEY_LONG_EVENT_LONGPRESS = 1,
    KEY_LONG_EVENT_LLONGPRESS = 2,
} key_long_event_e;

struct CSA37F71_KEY_STATUS_T {
    key_long_event_e long_press;
    uint8_t key_status;
    uint32_t time_updown;
    uint32_t time_click;
    uint8_t cnt_click;
    uint32_t press_time_ticks;
};

typedef enum {
	CS_CALI_DEVICE_WAKEUP_ITEM = 1,
	CS_CALI_EN_ITEM,
	CS_CALI_CHECK_ITEM,
	CS_CALI_DISEN_ITEM,
	CS_CALI_READ_RAWDATA_ITEM, //5
	CS_CALI_READ_PROCESSED_DATA_ITEM,
	CS_CALI_READ_NOISE_ITEM,
	CS_CALI_READ_OFFSET_ITEM,
	CS_CALI_READ_PRESSURE_ITEM,
	CS_CALI_READ_PRESS_LEVEL_ITEM,	//10
	CS_CALI_WRITE_PRESS_LEVEL_ITEM,
	CS_CALI_READ_CALIBRATION_FACTOR_ITEM,
	CS_CALI_WRITE_CALIBRATION_FACTOR_ITEM,
	
	CS_CALI_TEST_ITEM_TOTAL,
} csa37f71_cali_items;

#ifdef __cplusplus
}
#endif
#endif /*_CSA37F71_ADAPTER_H_*/
