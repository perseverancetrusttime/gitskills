#ifndef __XSPACE_XBUS_MANAGER_H__
#define __XSPACE_XBUS_MANAGER_H__

#if defined(__XSPACE_XBUS_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#if defined(__XSPACE_XBM_DEBUG__)
#define X2BM_TRACE(num, str, ...)  TRACE(num+1, "[X2BM] %s," str, __func__, ##__VA_ARGS__)
#else
#define X2BM_TRACE(num, str, ...)
#endif

#define X2BM_PMU_INT_DEBOUNCE_MS                         (5)
#define X2BM_CHARGER_PLUGIN_DEBOUNCE_TIMER_DELAY_MS      (600)

typedef enum {
    X2BM_CHARGER_UNKNOWN,
    X2BM_CHARGER_PLUGIN,
    X2BM_CHARGER_PLUGOUT,
    
} x2bm_charger_status_e;


typedef enum {
    X2BM_FRAME_HEAD_RX_L = 0x24,
    X2BM_FRAME_HEAD_RX_R = 0x25,
    X2BM_FRAME_HEAD_TX_L = 0x42,
    X2BM_FRAME_HEAD_TX_R = 0x52,
} x2bm_frame_head_e;

typedef enum {
    X2BM_FRAME_HEAD,
    X2BM_FRAME_CMD,
    X2BM_FRAME_DATA_LEN,
    X2BM_FRAME_DATA,
    X2BM_FRAME_CRC16,
    
    X2BM_FRAME_QTY,
} x2bm_frame_format_e;

typedef struct {
    uint8_t frame_head;
    uint8_t cmd;
//    uint16_t data_len;
    uint8_t data_len;    
    uint8_t *p_data;
    uint16_t crc16;
} x2bm_frame_s;

typedef enum
{
    XBUS_MANAGE_INVAILD_EVENT = 0,
    XBUS_MANAGE_SYNC_BOX_BAT_EVENT,
    XBUS_MANAGE_SYNC_BOX_VER_EVENT,
} xbus_manage_event_e;

typedef void (*xbus_manage_report_ui_event_func)(xbus_manage_event_e event_type, uint8_t *data, uint16_t len);

#if defined(__XSPACE_XBUS_OTA__)
typedef void (*xbus_manage_ota_start_handle_cb)(void);
void xbus_manage_ota_register_cb(xbus_manage_ota_start_handle_cb cb);
#endif

extern int32_t xbus_manage_send_data(uint8_t cmd, uint8_t *cmd_data, uint16_t data_len);
extern int32_t xspace_xbus_manage_init(void);

bool xbus_manage_is_communication_mode(void);
void xbus_manage_register_ui_event_cb(xbus_manage_report_ui_event_func cb);


#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_XBUS_MANAGER__)

#endif  // (__XSPACE_XBUS_MANAGER_H__)

