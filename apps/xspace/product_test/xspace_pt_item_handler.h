#ifndef __XSPACE_PT_ITEM_HANDLER_H__
#define __XSPACE_PT_ITEM_HANDLER_H__

#if defined(__XSPACE_PRODUCT_TEST__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "xspace_pt_cmd_analyze.h"

int32_t xpt_cmd_no_support_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_start_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_enter_dut_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_reboot_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
extern uint8_t xpt_mic_test_get_channel(void);
int32_t xpt_cmd_talk_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_enc_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_mac_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_ir_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_hall_switch_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_touch_key_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_write_bt_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_write_ble_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_write_rf_calib_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_led_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_device_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_stop_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_battery_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_ntc_temp_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_version_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_ble_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_talk_mic_loopback_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_enc_mic_loopback_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_gsensor_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_anc_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_hall_ctrl_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_ear_side_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_fb_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_touch_calib_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_op_mode_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_bt_disconnect_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_audio_status_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_answer_call_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_hangup_call_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_music_play_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_music_pause_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_music_forward_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_music_backward_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_volume_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_switch_role_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_role_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_check_factory_reset_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_disconnect_hfp_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_disconnect_a2dp_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_reset_fatctory_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_ear_color_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_poweroff_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_reset_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_start_pairing_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_long_standby_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_dev_mac_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_volume_up_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_volume_down_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_write_ear_color_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_bt_name_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_pressure_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_enc_arithmetic_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_bt_chipid_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_hardware_version_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_check_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_write_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_read_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_presure_chip_id_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
void spp_cmd_start_test_handler(void);
void spp_cmd_reset_test_handler(void);
void spp_cmd_talk_mic_test_handler(void);
void spp_cmd_fb_mic_test_handler(void);
void spp_cmd_ff_mic_test_handler(void);
int32_t xpt_cmd_anc_mode_state_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_ear_india_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
int32_t xpt_cmd_get_is_burn_anc_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len);
#ifdef __cplusplus
}
#endif

#endif   // (__XSPACE_PRODUCT_TEST__)

#endif   // (__XSPACE_PT_ITEM_HANDLER_H__)
