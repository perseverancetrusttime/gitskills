#ifndef __XSPACE_INTERFACE_H__
#define __XSPACE_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "xspace_interface_process.h"

#if defined(__XSPACE_INTETFACE_TRACE__)
#define XIF_TRACE(num, str, ...) TRACE(num + 1, "[XIF]%s," str, __func__, ##__VA_ARGS__)
#define XIF_ASSERT               ASSERT
#else
#define XIF_TRACE(num, str, ...)
#define XIF_ASSERT(num, str, ...)
#endif

#define XIF_OTA_BATTERY_MIN_PERCENT (20)

typedef enum {
    XIF_MUSIC_STATUS_UNKONWN,
    XIF_MUSIC_STATUS_PLAYING,
    XIF_MUSIC_STATUS_PAUSE,
    XIF_MUSIC_STATUS_STOP,
    XIF_MUSIC_STATUS_FORWARD,
    XIF_MUSIC_STATUS_BACKWARD,
    XIF_MUSIC_STATUS_VOLUP,
    XIF_MUSIC_STATUS_VOLDOWN,
    XIF_MUSIC_STATUS_NUM
} XIF_MUSIC_STATUS_E;

typedef enum {
    XIF_CALL_STATUS_UNKNOWN,
    XIF_CALL_STATUS_ANSWER,
    XIF_CALL_STATUS_HANGUP,
    XIF_CALL_STATUS_THREEWAY_HOLD_AND_ANSWER,
    XIF_CALL_STATUS_THREEWAY_HANGUP_AND_ANSWER,
    XIF_CALL_STATUS_THREEWAY_HOLD_REL_INCOMING,
    XIF_CALL_STATUS_VOLUP,
    XIF_CALL_STATUS_VOLDOWN,
    XIF_CALL_STATUS_NUM
} XIF_CALL_STATUS_E;

typedef enum {
    WRITE_BATTRY_TO_FLASH,
    WRITE_TOUCH_TO_FLASH,
    WRITE_UI_INFO_TO_FLASH,
    BEFOR_SHUTDOWN_WRITE_FLASH_NUM,
} SHUTDOWN_WRITE_FLASH_MANAGE_E;

typedef enum {
    XIF_AUDIO_IDLE,
    XIF_AUDIO_SBC,
    XIF_AUDIO_SCO,
} XIF_AUDIO_STATUS_E;

typedef void (*shutdown_write_flash_manage_func)(void);

/**
 ****************************************************************************************
 * @brief flash related
 ****************************************************************************************
 */
void xspace_interface_register_write_flash_befor_shutdown_cb(SHUTDOWN_WRITE_FLASH_MANAGE_E write_flash_enum, shutdown_write_flash_manage_func cb);
void xspace_interface_write_data_to_flash_befor_shutdown(void);

/**
 ****************************************************************************************
 * @brief time related
 ****************************************************************************************
 */
int xspace_interface_delay_timer_stop(uint32_t ptr);
int xspace_interface_delay_timer_start(uint32_t time, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1);
uint32_t xspace_interface_get_current_ms(void);
uint32_t xspace_interface_get_current_sec(void);

/**
 ****************************************************************************************
 * @brief OTA related
 ****************************************************************************************
 */
bool xspace_interface_process_bat_allow_enter_ota_mode(void);
void xspace_interface_enter_ota_mode(void);

/**
 ****************************************************************************************
 * @brief TWS related
 ****************************************************************************************
 */
void xspace_interface_freeman_config(void);
bool xspace_interface_tws_pairing_config(uint8_t *addr);
bool xspace_interface_tws_is_master_mode(void);
bool xspace_interface_tws_is_slave_mode(void);
bool xspace_interface_tws_is_freeman_mode(void);
uint8_t xspace_interface_get_device_current_roles(void);
uint8_t xspace_interface_start_tws_switch(void);
bool xspace_interface_device_is_in_paring_mode(void);
void xspace_interface_tws_pairing_enter(void);
void xspace_interface_device_set_tws_pairing_flag(bool status);

/**
 ****************************************************************************************
 * @brief BT profile link related
 ****************************************************************************************
 */
void xspace_interface_force_disconnect_all_mobile_link(void);
int32_t xspace_interface_disconnect_hfp(void);
int32_t xspace_interface_disconnect_a2dp(void);
bool xspace_interface_is_a2dp_mode(void);
bool xspace_interface_is_sco_mode(void);
bool xspace_interface_tws_link_connected(void);
bool xspace_interface_is_any_mobile_connected(void);
bool xspace_interface_tws_ibrt_link_connected(void);
bool xspace_interface_have_mobile_record(void);
uint8_t xspace_interface_get_avrcp_connect_status(void);
bool xspace_interface_hfp_connected(uint8_t device_id);
bool xspace_interface_a2dp_connected(uint8_t device_id);
bool xspace_interface_avrcp_connected(uint8_t device_id);
bool xspace_interface_mobile_link_connected_by_id(uint8_t device_id);

/**
 ****************************************************************************************
 * @brief Music control related
 ****************************************************************************************
 */
void xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_E status);
uint8_t xspace_interface_get_a2dp_volume(uint8_t id);
bool xspace_interface_audio_status(void);
void xspace_interface_ibrt_sync_volume_info(void);
/**
 ****************************************************************************************
 * @brief Calling control related
 ****************************************************************************************
 */
bool xspace_interface_call_is_active(void);
void xspace_interface_call_ctrl(XIF_CALL_STATUS_E status);
bool xspace_interface_call_is_incoming(void);
bool xspace_interface_call_is_online(void);
int32_t xspace_interface_call_answer_call(void);
int32_t xspace_interface_call_hangup_call(void);
uint8_t xspace_interface_get_hfp_volume(uint8_t id);
void xspace_interface_wakeup_siri(void);
void xspace_interface_bt_volumeup();
void xspace_interface_bt_volumedown();
/**
 ****************************************************************************************
 * @brief Other
 ****************************************************************************************
 */
void xspace_interface_enter_dut_mode(void);
void xspace_interface_update_info_when_shutdown_or_reboot(void);
void xspace_interface_shutdown(void);
void xspace_interface_init(void);
void xspace_interface_reset_factory_setting(void);
void xspace_interface_reboot(void);
XIF_AUDIO_STATUS_E xspace_interface_get_audio_status(void);
void xspace_interface_event_process(xif_event_e msg_id, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1);
void xspace_interface_ibrt_nvrecord_config_load(void *config);

#ifdef IBRT_SEARCH_UI
void xspace_interface_enter_search_ui_mode(void);
#endif
#ifdef __cplusplus
}
#endif

#endif   //__XSPACE_INTERFACE_H__
