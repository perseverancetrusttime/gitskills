#ifndef _AW8680X_ADAPTER_H_
#define _AW8680X_ADAPTER_H_
#include "aw8680x.h"
#include "aw_log.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define AW8680X_DATA_PRINTF_DEBUG

#define KEY_DBLCLICK_THRESH_MS       500
#define KEY_LLPRESS_THRESH_MS        3*1000
#define KEY_LPRESS_THRESH_MS         1*1000

typedef enum
{
    KEY_LONG_EVENT_NONE = 0,
    KEY_LONG_EVENT_LONGPRESS = 1,
    KEY_LONG_EVENT_LLONGPRESS = 2,
} key_long_event_e;

struct AW8680X_KEY_STATUS_T {
    key_long_event_e long_press;
    uint8_t key_status;
    uint32_t time_updown;
    uint32_t time_click;
    uint8_t cnt_click;
    uint32_t press_time_ticks;
};

typedef enum {
	AW_CALI_READY_ITEM = 1,
	AW_CALI_PRESS_ITEM,
	AW_CALI_WRITE_ITEM,
	AW_TEST_READY_ITEM,
	AW_TEST_PRESS_ITEM,
	AW_TEST_FINISH_ITEM,
	AW_TEST_RAWDATA_ITEM,
	AW_CALI_TEST_ITEM_TOTAL,
} aw8680_cali_items;

#ifdef __cplusplus
}
#endif
#endif /*_AW8680X_ADAPTER_H_*/
