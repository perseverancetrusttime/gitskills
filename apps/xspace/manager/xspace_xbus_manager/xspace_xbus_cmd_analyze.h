#ifndef __XSPACE_XBUS_CMD_ANALYZE_H__
#define __XSPACE_XBUS_CMD_ANALYZE_H__

#if defined(__XSPACE_XBUS_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

typedef enum {
    
    X2BM_CMD_TWS_PAIRING = 0x01,
    X2BM_CMD_QUERY_PAIRING_STATUS,
    X2BM_CMD_ENTER_OTA_MODE,
    X2BM_CMD_RESTORE_FACTORY_SETTING,
    X2BM_CMD_CTRL_EAR_POWEROFF = 0x05,
    X2BM_CMD_QUERY_EXCHANGE_INFO,
    X2BM_CMD_HW_REBOOT,
    X2BM_CMD_STOP_PARING,
    X2BM_CMD_SINGLE_PAIRING,
    X2BM_CMD_INBOX_START_OTA = 0X0A,
    //FWU CMD
    X2BM_CMD_START_XBUS_FWU = 0x10,
    X2BM_CMD_FWU_DATA_PROCESS = 0x11,
    X2BM_CMD_GET_VERSION_PROCESS = 0x12,
    X2BM_CMD_SAVE_FLASH_INFO = 0x13,
    X2BM_CMD_HW_SHIPMODE = 0x14,
    X2BM_CMD_GET_LINKKEY = 0x15,
#ifdef __XSPACE_TRACE_DISABLE__
	X2BM_CMD_TRACE_SWITCH = 0x16,
#endif
    X2BM_CMD_CNT
} x2bm_cmd_e;


typedef struct {
    x2bm_cmd_e cmd;
    const uint8_t* cmd_str;
    int32_t (* cmd_handler)(uint8_t *cmd_data, uint16_t data_len);
    
} x2bm_cmd_config_s;


extern int32_t xbus_manage_cmd_find_and_exec_handler(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len);

#if defined(__XSPACE_XBUS_OTA__)
extern void xbus_cmd_ota_register_cb(xbus_manage_ota_start_handle_cb cb);
#endif

#if defined (__XSPACE_AUTO_TEST__)
extern int32_t xbm_cmd_tws_pair_handler_for_auto_test(uint8_t *cmd_data, uint16_t data_len);
#endif

#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_XBUS_MANAGER__)

#endif  // (__XSPACE_XBUS_CMD_ANALYZE_H__)

