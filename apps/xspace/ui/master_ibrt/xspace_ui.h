#ifndef __XSPACE_UI_H__
#define __XSPACE_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "hal_trace.h"
#include "tgt_hardware.h"

#if defined(__XSPACE_UI_DEBUG__)
#define XUI_TRACE_ENTER(...)     TRACE(1, "[XUI]%s <<<", __func__)
#define XUI_TRACE(num, str, ...) TRACE(num + 1, "[XUI]%s," str, __func__, ##__VA_ARGS__)
#define XUI_TRACE_EXIT(...)      TRACE(1, "[XUI]%s >>>", __func__)
#else
#define XUI_TRACE_ENTER(...)
#define XUI_TRACE(...)
#define XUI_TRACE_EXIT(...)
#endif

#define XUI_BAT_PD_PER_THRESHOLD   (0)
#define XUI_BAT_HIGH_PER_THRESHOLD (100)
#define XUI_BAT_LOW_PER_THRESHOLD  (15)

#define XUI_TEMPERATER_LOWEST   (2)
#define XUI_TEMPERATER_MEDIUM   (17)
#define XUI_TEMPERATURE_HIGH    (43)
#define XUI_TEMPERATURE_HIGHEST (50)

#define XUI_COVER_OPEN_DEBOUNCE_TIME  (10)
#define XUI_COVER_CLOSE_DEBOUNCE_TIME (2000)

#define XUI_TWS_BAT_DATA_VALID_FLAG (0xB5)
#define XUI_TWS_VER_DATA_VALID_FLAG (0xB5)
#define XUI_BOX_LOW_BATTERY         (1)
#define XUI_BOX_NO_BATTERY          (0)

#define XUI_TWS_SYNC_INFO_BUFF_LENGTH (100)

typedef enum {
    XUI_CHECK_ID_INOUT_BOX = 0,
    XUI_CHECK_ID_COVER_STATUS,
    XUI_CHECK_ID_TOTAL_NUM,
} xui_sys_init_status_check_e;

typedef enum {
    XUI_SYS_INVALID = 0,
    XUI_SYS_INIT,
    XUI_SYS_FORCE_TWS_PARING,
    XUI_SYS_FORCE_FREEMAN_PARING,
    XUI_SYS_PRODUCT_TEST,
    XUI_SYS_NORMAL,
} xui_sys_status_e;

typedef enum {
    XUI_INEAR_UNKNOWN,
    XUI_INEAR_OFF,
    XUI_INEAR_ON,
} xui_inear_status_e;

typedef enum {
    XUI_BOX_UNKNOWN,
    XUI_BOX_OUT_BOX,
    XUI_BOX_IN_BOX,
} xui_inout_box_status_e;

typedef enum {
    XUI_COVER_UNKNOWN,
    XUI_COVER_CLOSE,
    XUI_COVER_OPEN,
} xui_cover_status_e;

typedef enum {
    XUI_ID_NULL,
    XUI_ID_WEAR_STATUS,
    XUI_ID_COVER_STATUS,
    XUI_ID_BOX_STATUS,
    XUI_ID_SYS_STATUS,
} xui_id_status_e;

typedef enum {
    XUI_MUSIC_UNKNOWN,
    XUI_MUSIC_PLAYING,
    XUI_MUSIC_PAUSE,
    XUI_MUSIC_STOP,
    XUI_MUSIC_WEAR_PLAYING,
	XUI_MUSIC_WEAR_PAUSED,
	XUI_MUSIC_WEAR_STOPED,
	XUI_MUSIC_GESTURE_PLAYING,
	XUI_MUSIC_GESTURE_PAUSED,
	XUI_MUSIC_GESTURE_STOPED,
    XUI_MUSIC_STATUS_NUM,
} xui_music_status_e;

typedef enum {
    XUI_BT_UNKNOWN,
    XUI_BT_CONNECTED,
    XUI_BT_DISCONNECTECD_TIMETOUT,
    XUI_BT_DISCONNECTECD_HOST,
    XUI_BT_DISCONNECTECD_LOCAL,
    XUI_BT_STATUS_NUM,
} xui_bt_status_e;

typedef struct {
    uint8_t data_valid;
    uint8_t hw_version[2];
    uint8_t sw_version[4];
    uint8_t ota_version[4];
} xui_ver_sync_info_s;

typedef struct {
    xui_ver_sync_info_s ep_ver;
    xui_ver_sync_info_s box_ver;
} xui_box_and_ep_ver_sync_s;

typedef enum {
    XUI_PLAY_PAUSE_PLAY_MUSIC = 1,
    XUI_AUTO_PLAY_MUSIC,
    XUI_VOICE_ASSISTANT,
    XUI_PREVIOUS_MUSIC,
    XUI_NEXT_MUSIC,
    XUI_MODIFY_VOL,
    XUI_ANC_MODE_CHANGE,

} xui_gesture_func_e;

typedef struct {
    xui_music_status_e music_status;
    xui_bt_status_e bt_status;
} xui_misic_control_status_s;

typedef struct {
    xui_inear_status_e inear_status;
    xui_inout_box_status_e inout_box_status;
    xui_cover_status_e cover_status;
    xui_misic_control_status_s music_control_status;
    xui_ver_sync_info_s ver_info;
} xui_sys_info_s;

typedef struct {
    uint8_t data_valid;
    uint8_t bat_level;
    uint8_t bat_per;
    bool isCharging;
} xui_bat_sync_info_s;

typedef struct {
    uint8_t addr[6];
    uint8_t name_len;
    uint8_t name[50];
} xui_sync_remote_device_info_s;

typedef enum {
    XUI_SYS_INFO_LOCAL_BAT_EVENT = 1,
    XUI_SYS_INFO_INEAR_STATUS_EVENT = 2,
    XUI_SYS_INFO_INOUT_BOX_EVENT = 3,
    XUI_SYS_INFO_COVER_STATUS_EVENT = 4,
    XUI_SYS_INFO_VERSION_INFO_EVENT = 5,
    XUI_SYS_INFO_BOX_BAT_INFO_EVENT = 6,
    XUI_SYS_INFO_GESTURE_INFO_EVENT = 7,
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_STATUS_EVENT = 8,
    XUI_SYS_INFO_SMART_WEAR_MUSIC_REPLAY_FUNC_SWITCH_CONFIG_EVENT = 9,
#endif
#if defined(USER_REBOOT_PLAY_MUSIC_AUTO)
    XUI_SYS_INFO_AUTO_REBOOT_PLAY_MUSIC_STATUS_EVENT = 10,
#endif
    XUI_SYS_INFO_CLICK_FUNC_CONFIG_EVENT = 11,
    XUI_SYS_INFO_LOW_LANTENCY_EVENT = 14,//低延迟
#if defined(__TWS_DISTANCE_SLAVE_REBOOT__)
    XUI_SYS_INFO_SLAVE_REBOOT = 15,
#endif

    XUI_SYS_INFO_RECONNECT_SECOND_MOBILE = 16,
    XUI_SYS_INFO_SYNC_SHUTDOWN = 17,
    XUI_SYS_INFO_SYNC_REMOTE_DEVICE_INFO = 18,

#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    XUI_SYS_INFO_SYNC_WEAR_AUTO_PLAY_FUNC_SWITCH = 19,
#endif

#if defined(__XSPACE_CUSTOMIZE_ANC__)
    XUI_SYS_INFO_SYNC_ANC_INFO = 20,
#endif

#if defined(__XSPACE_RSSI_TWS_SWITCH__)
    XUI_SYS_INFO_SYNC_RSSI_INFO = 23,
#endif

    XUI_SYS_INFO_PEER_RUN_FUNC = 25,

#if defined(__XSPACE_SNR_WNR_TWS_SWITCH__)
    XUI_SYS_INFO_SYNC_SNR_WNR_INFO = 26,
#endif

    XUI_SYS_INFO_SYNC_PAIRING_STATUS = 27,
    XUI_SYS_INFO_SYNC_VOLUME_INFO = 28,
    XUI_SYS_INFO_SYNC_HFP_VOLUME_INFO = 29,
#if defined(__XSPACE_UI__)
    XUI_SYS_INFO_SYNC_LED_INFO = 30,
#endif
    XUI_SYS_INFO_TOTAL_INFO_EVENT = 0X80,
} xui_sys_info_sync_event_e;

typedef struct {
    xui_sys_info_sync_event_e event;
    uint8_t need_rsp;
    uint8_t data_len;
    uint8_t data[XUI_TWS_SYNC_INFO_BUFF_LENGTH];
} xui_tws_sync_info_s;

//Note(Mike.Cheng):
typedef enum {
    XUI_MOBILE_CONNECT_VOICE_PROMPT_STATUS_CHECK = 1,
    XUI_ANC_MODE_SWITCH,
    XUI_HANDLE_A2DP_KEY,
    XUI_SET_CONECT_VOICE_PROMPT_STATUS,
    XUI_SET_SINGLE_WEAR_ANC_MODE_SWITCH_INFO,
    XUI_UPDATE_ANC_MODE_STATUS,
    XUI_APP_ANC_STATUS_SYNC
} xui_let_peer_run_cmd_e;

typedef struct {
    uint32_t delay_time;
    uint32_t cmd_id;
    uint32_t id;
    uint32_t param0;
    uint32_t param1;
} xui_let_peer_run_cmd_config_s;

typedef struct {
    uint8_t sys_init_check_value;
    uint8_t sys_curr_init_value;
    xui_bat_sync_info_s local_bat_info;
    xui_bat_sync_info_s peer_bat_info;
    xui_bat_sync_info_s box_bat_info;
    xui_sys_info_s local_sys_info;
    xui_sys_info_s peer_sys_info;
    xui_ver_sync_info_s box_ver_info;
#if defined(__SMART_INEAR_MUSIC_REPLAY__)
    uint8_t wear_auto_play;
    bool wear_auto_play_func_switch_flag;
#endif
#if defined(__XSPACE_CUSTOMIZE_ANC__)
    uint8_t anc_mode_index;
#endif
} xui_tws_ctx_t;

#if defined(__XSPACE_XBUS_OTA__)
typedef enum {
    XBUS_OTA_STATUS_ENTER,
    XBUS_OTA_STATUS_EXIT,

    XBUS_OTA_STATUS_QTY,
} xbus_ota_status_e;

void xspace_ui_init(void);
void xspace_ui_env_get(xui_tws_ctx_t **p_ui_env);
void xspace_ui_tws_send_info_sync_cmd(xui_sys_info_sync_event_e id, uint8_t *data, uint16_t len);
void xspace_ui_tws_info_sync_recived_handle(uint16_t rsp_seq, uint8_t *data, uint16_t len);
void xspace_ui_set_system_status(xui_sys_status_e status);
xui_sys_status_e xspace_ui_get_system_status(void);
bool xspace_ui_init_finish_over(void);
void xspace_ui_check_a2dp_data(uint8_t *buf, uint32_t len);
bool xspace_ui_check_sco_data(uint8_t *buf, uint32_t len);
bool xspace_ui_check_mic_data(uint8_t *buf, uint32_t len);
void xspace_ui_update_info_when_shutdown_or_reboot(void);
void xspace_ui_let_peer_run_func(uint32_t time, uint32_t func_id, uint32_t id, uint32_t param0, uint32_t param1);

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)
bool xspace_ui_is_inear(void);
bool xspace_ui_is_outear(void);
#if defined(__SMART_INEAR_MIC_SWITCH__)
void xspace_ui_inear_hfp_mic_switch(bool start_flag);
#endif
#endif

#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
bool xspace_ui_is_box_closed(void);
#endif

#if defined(__XSPACE_INOUT_BOX_MANAGER__)
bool xspace_ui_is_inbox(void);
#endif

#if defined(__XSPACE_BATTERY_MANAGER__)
void xspace_ui_battery_init(void);
uint8_t xspace_ui_get_local_bat_level(void);
uint8_t xspace_ui_get_local_bat_percentage(void);
void xspace_ui_battery_report_remote_device(void);
#endif

void xspace_ui_mobile_connect_voice_check_handle(uint8_t bt_connect_tick_flag);
void xspace_ui_mobile_disconnect_voice_check_handle(uint8_t device_id);
bool xspace_ui_tws_switch_according_rssi_needed(void);
void xspace_ui_tws_sync_remote_device_name(uint8_t *addr);
void xspace_ui_get_mobile_record(uint8_t *record_num, uint8_t *p_data, uint16_t *len);

int xspace_ui_msg_buf_get(uint8_t **buff, uint32_t size);
int xspace_ui_msg_buf_free(uint8_t *buff);
uint8_t xspace_ui_tws_ctx_set(xui_tws_ctx_t ctx);
uint8_t xspace_ui_tws_ctx_get(xui_tws_ctx_t *ctx);
void xspace_ui_boost_freq_when_busy(void);  
void xspace_ui_mobile_connect_status_change_handle(uint8_t device_id, bool status, uint8_t reason);
void xspace_ui_tws_connect_status_change_handle(bool status, uint8_t reason);
extern void anc_howling_check_enable(int32_t flag);
extern bool f_factory_mode; 
#ifdef __cplusplus
}
#endif

#endif /* __XSPACE_UI__ */

#endif /* __XSPACE_UI_H__ */
