#if defined(__XSPACE_PRODUCT_TEST__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "xspace_pt_item_handler.h"
#include "xspace_xbus_manager.h"
#include "xspace_pt_trace_uart.h"
#include "xspace_ui.h"
#include "xspace_interface.h"
#if defined(__SKH_APP_DEMO_USING_SPP__)
#include "app_spp_tota.h"
#endif

static xpt_context_s xpt_context;

static const xpt_cmd_config_s c_xpt_cmd_cfg[] = {
    {XPT_CMD_START_TEST, (uint8_t *)"XPT_CMD_START_TEST", false, xpt_cmd_start_test_handler},
    {XPT_CMD_ENTER_DUT, (uint8_t *)"XPT_CMD_ENTER_DUT", false, xpt_cmd_enter_dut_handler},
    {XPT_CMD_REBOOT, (uint8_t *)"XPT_CMD_REBOOT", false, xpt_cmd_reboot_handler},
    {XPT_CMD_TALK_MIC_TEST, (uint8_t *)"XPT_CMD_TALK_MIC_TEST", true, xpt_cmd_talk_mic_test_handler},
    {XPT_CMD_ENC_MIC_TEST, (uint8_t *)"XPT_CMD_ENC_MIC_TEST", true, xpt_cmd_enc_mic_test_handler},
    {XPT_CMD_GET_MAC_ADDR, (uint8_t *)"XPT_CMD_GET_MAC_ADDR", true, xpt_cmd_get_mac_handler},
    {XPT_CMD_IR_TEST, (uint8_t *)"XPT_CMD_IR_TEST", true, xpt_cmd_ir_test_handler},
    {XPT_CMD_HALL_SWITCH_TEST, (uint8_t *)"XPT_CMD_HALL_SWITCH_TEST", true, xpt_cmd_hall_switch_test_handler},
    {XPT_CMD_TOUCH_KEY_TEST, (uint8_t *)"XPT_CMD_TOUCH_KEY_TEST", true, xpt_cmd_touch_key_test_handler},
    {XPT_CMD_WRITE_BT_ADDR, (uint8_t *)"XPT_CMD_WRITE_BT_ADDR", true, xpt_cmd_write_bt_addr_handler},
    {XPT_CMD_WRITE_BLE_ADDR, (uint8_t *)"XPT_CMD_WRITE_BLE_ADDR", true, xpt_cmd_write_ble_addr_handler},
    {XPT_CMD_WRITE_RF_CALIB, (uint8_t *)"XPT_CMD_WRITE_RF_CALIB", true, xpt_cmd_write_rf_calib_handler},
    {XPT_CMD_LED_TEST, (uint8_t *)"XPT_CMD_LED_TEST", true, xpt_cmd_led_test_handler},
    {XPT_CMD_GET_DEVICE_INFO, (uint8_t *)"XPT_CMD_GET_DEVICE_INFO", true, xpt_cmd_get_device_info_handler},
    {XPT_CMD_STOP_TEST, (uint8_t *)"XPT_CMD_STOP_TEST", true, xpt_cmd_stop_test_handler},
    {XPT_CMD_GET_BATTERY_INFO, (uint8_t *)"XPT_CMD_GET_BATTERY_INFO", true, xpt_cmd_get_battery_info_handler},
    {XPT_CMD_GET_NTC_TMP, (uint8_t *)"XPT_CMD_GET_NTC_TMP", true, xpt_cmd_get_ntc_temp_handler},
    {XPT_CMD_GET_VERSION, (uint8_t *)"XPT_CMD_GET_VERSION", true, xpt_cmd_get_version_handler},
    {XPT_CMD_GET_BLE_ADDR, (uint8_t *)"XPT_CMD_GET_BLE_ADDR", true, xpt_cmd_get_ble_addr_handler},
    {XPT_CMD_TALK_MIC_LOOPBACK_TEST, (uint8_t *)"XPT_CMD_TALK_MIC_LOOPBACK_TEST", true, xpt_cmd_talk_mic_loopback_test_handler},
    {XPT_CMD_ENC_MIC_LOOPBACK_TEST, (uint8_t *)"XPT_CMD_ENC_MIC_LOOPBACK_TEST", true, xpt_cmd_enc_mic_loopback_test_handler},
    {XPT_CMD_GSENSOR_TEST, (uint8_t *)"XPT_CMD_GSENSOR_TEST", true, xpt_cmd_gsensor_test_handler},
    {XPT_CMD_ANC_TEST, (uint8_t *)"XPT_CMD_ANC_TEST", true, xpt_cmd_anc_test_handler},
    {XPT_CMD_HALL_CTRL, (uint8_t *)"XPT_CMD_HALL_CTRL", true, xpt_cmd_hall_ctrl_handler},
    {XPT_CMD_GET_EAR_SIDE, (uint8_t *)"XPT_CMD_GET_EAR_SIDE", true, xpt_cmd_get_ear_side_handler},
    {XPT_CMD_CHECK_LHDC_LINK_KEY_TEST, (uint8_t *)"XPT_CMD_CHECK_LHDC_LINK_KEY_TEST", true, xpt_cmd_check_lhdc_link_key_handler},
    {XPT_CMD_WRITE_LHDC_LINK_KEY_TEST, (uint8_t *)"XPT_CMD_WRITE_LHDC_LINK_KEY_TEST", true, xpt_cmd_write_lhdc_link_key_handler},
    {XPT_CMD_READ_LHDC_LINK_KEY_TEST, (uint8_t *)"XPT_CMD_READ_LHDC_LINK_KEY_TEST", true, xpt_cmd_read_lhdc_link_key_handler},
    {XPT_CMD_FB_MIC_TEST, (uint8_t *)"XPT_CMD_FB_MIC_TEST", true, xpt_cmd_fb_mic_test_handler},
    {XPT_CMD_TOUCH_CALIB_TEST, (uint8_t *)"XPT_CMD_TOUCH_CALIB_TEST", true, xpt_cmd_touch_calib_test_handler},
    {XPT_CMD_GET_OP_MODE_TEST, (uint8_t *)"XPT_CMD_GET_OP_MODE_TEST", true, xpt_cmd_get_op_mode_handler},
    {XPT_CMD_BT_DISCONNECT_TEST, (uint8_t *)"XPT_CMD_BT_DISCONNECT_TEST", true, xpt_cmd_bt_disconnect_test_handler},
    {XPT_CMD_GET_AUDIO_STATUS_TEST, (uint8_t *)"XPT_CMD_GET_AUDIO_STATUS_TEST", true, xpt_cmd_get_audio_status_test_handler},
    {XPT_CMD_ANSWER_CALL_TEST, (uint8_t *)"XPT_CMD_ANSWER_CALL_TEST", true, xpt_cmd_answer_call_test_handler},
    {XPT_CMD_HANGUP_CALL_TEST, (uint8_t *)"XPT_CMD_HANGUP_CALL_TEST", true, xpt_cmd_hangup_call_test_handler},
    {XPT_CMD_MUSIC_PLAY_TEST, (uint8_t *)"XPT_CMD_MUSIC_PLAY_TEST", true, xpt_cmd_music_play_test_handler},
    {XPT_CMD_MUSIC_PAUSE_TEST, (uint8_t *)"XPT_CMD_MUSIC_PAUSE_TEST", true, xpt_cmd_music_pause_test_handler},
    {XPT_CMD_MUSIC_FORWARD_TEST, (uint8_t *)"XPT_CMD_MUSIC_FORWARD_TEST", true, xpt_cmd_music_forward_test_handler},
    {XPT_CMD_MUSIC_BACKWARD_TEST, (uint8_t *)"XPT_CMD_MUSIC_BACKWARD_TEST", true, xpt_cmd_music_backward_test_handler},
    {XPT_CMD_GET_VOLUME_TEST, (uint8_t *)"XPT_CMD_GET_VOLUME_TEST", true, xpt_cmd_get_volume_test_handler},
    {XPT_CMD_SWITCH_ROLE_TEST, (uint8_t *)"XPT_CMD_SWITCH_ROLE_TEST", true, xpt_cmd_switch_role_test_handler},
    {XPT_CMD_GET_ROLE_TEST, (uint8_t *)"XPT_CMD_GET_ROLE_TEST", true, xpt_cmd_get_role_test_handler},
    {XPT_CMD_CHECK_RESTORE_FACTORY, (uint8_t *)"XPT_CMD_CHECK_RESTORE_FACTORY", true, xpt_cmd_check_factory_reset_handler},
    {XPT_CMD_DISCONNECT_HFP_TEST, (uint8_t *)"XPT_CMD_DISCONNECT_HFP_TEST", true, xpt_cmd_disconnect_hfp_test_handler},
    {XPT_CMD_DISCONNECT_A2DP_TEST, (uint8_t *)"XPT_CMD_DISCONNECT_A2DP_TEST", true, xpt_cmd_disconnect_a2dp_test_handler},
    {XPT_CMD_RESET_FACTORY_TEST, (uint8_t *)"XPT_CMD_RESET_FACTORY_TEST", true, xpt_cmd_reset_fatctory_test_handler},
    {XPT_CMD_GET_EAR_COLOR_TEST, (uint8_t *)"XPT_CMD_GET_EAR_COLOR", true, xpt_cmd_get_ear_color_handler},
    {XPT_CMD_BT_POWEROFF_TEST, (uint8_t *)"XPT_CMD_BT_POWEROFF_TEST", true, xpt_cmd_poweroff_handler},
    {XPT_CMD_BT_RESET_TEST, (uint8_t *)"XPT_CMD_BT_RESET_TEST", true, xpt_cmd_reset_handler},
    {XPT_CMD_STAR_PAIR_TEST, (uint8_t *)"XPT_CMD_STAR_PAIR_TEST", true, xpt_cmd_start_pairing_test_handler},
    {XPT_CMD_LONG_STANDBY_TEST, (uint8_t *)"XPT_CMD_LONG_STANDBY_TEST", true, xpt_cmd_long_standby_test_handler},
    {XPT_CMD_GET_DEV_MAC_TEST, (uint8_t *)"XPT_CMD_GET_BT_INFO_TEST", true, xpt_cmd_get_dev_mac_handler},
    {XPT_CMD_VOLUME_UP_TEST, (uint8_t *)"XPT_CMD_VOLUME_UP_TEST", true, xpt_cmd_get_volume_up_handler},
    {XPT_CMD_VOLUME_DOWN_TEST, (uint8_t *)"XPT_CMD_VOLUME_DOWN_TEST", true, xpt_cmd_get_volume_down_handler},
    {XPT_CMD_GET_BT_NAME_TEST, (uint8_t *)"XPT_CMD_GET_BT_NAME_TEST", true, xpt_cmd_get_bt_name_handler},
    {XPT_CMD_PRESSURE_TEST, (uint8_t *)"XPT_CMD_PRESSURE_TEST", true, xpt_cmd_pressure_test_handler},
    {XPT_CMD_GET_ENC_ARITHMETIC_TEST, (uint8_t *)"XPT_CMD_GET_ENC_ARITHMETIC_TEST", true, xpt_cmd_get_enc_arithmetic_info_handler},
    {XPT_CMD_GET_BT_CHIP_UID_TEST, (uint8_t *)"XPT_CMD_GET_BT_CHIP_UID_TEST", true, xpt_cmd_get_bt_chipid_info_handler},
    {XPT_CMD_GET_BT_HW_VERSION_TEST, (uint8_t *)"XPT_CMD_GET_BT_HW_VERSION_TEST", true, xpt_cmd_get_hardware_version_handler},
    {XPT_CMD_GET_PRESURE_CHIPID, (uint8_t *)"XPT_CMD_GET_PRESURE_CHIPID", true, xpt_cmd_get_presure_chip_id_handler},
    {XPT_CMD_ANC_MODE_STATE, (uint8_t *)"XPT_CMD_ANC_MODE_STATE", true, xpt_cmd_anc_mode_state_handler},
    {XPT_CMD_GET_EAR_INDIA_TEST, (uint8_t *)"XPT_CMD_GET_EAR_INDIA_TEST", true, xpt_cmd_get_ear_india_handler},
    {XPT_CMD_GET_IS_BURN_ANC_TEST, (uint8_t *)"XPT_CMD_GET_IS_BURN_ANC_TEST", true, xpt_cmd_get_is_burn_anc_handler},
};

bool xpt_get_mode_status(void)
{
    if (xpt_context.status == XPT_STATUS_TEST_MODE) {
        return true;
    } else {
        return false;
    }
}

static bool xpt_check_cmd(xpt_context_s *context, xpt_cmd_config_s *cmd_config)
{
    if (cmd_config->need_start_cmd) {
#if defined(__XSPACE_XBUS_OTA__)
        if (XPT_CMD_GET_VERSION == cmd_config->cmd_id) {
            return true;
        }
#endif
        if (XPT_STATUS_TEST_MODE == context->status) {
            return true;
        } else {
            uint8_t cmd_data[10] = {0};
            cmd_data[0] = XPT_RSP_NOT_TEST_MODE;
            xpt_cmd_response(cmd_config->cmd_id, cmd_data, 1);
            context->curr_cmd_id = XPT_CMD_NONE;

            XPT_TRACE(1, "Not in test mode, access denied.");
            return false;
        }
    }

    return true;
}

static int32_t xpt_cmd_find_and_exec_handler(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    xpt_cmd_id_enum cmd_id = (xpt_cmd_id_enum)cmd;

    for (uint16_t i = 0; i < sizeof(c_xpt_cmd_cfg) / sizeof(c_xpt_cmd_cfg[0]); i++) {
        if (cmd_id == c_xpt_cmd_cfg[i].cmd_id) {
            if (xpt_check_cmd(&xpt_context, (xpt_cmd_config_s *)&c_xpt_cmd_cfg[i])) {
                if (c_xpt_cmd_cfg[i].cmd_handler != NULL) {
                    XPT_TRACE(1, "Excute %s command.", c_xpt_cmd_cfg[i].cmd_name);

                    xpt_context.curr_cmd_id = cmd_id;
                    c_xpt_cmd_cfg[i].cmd_handler(&xpt_context, cmd_data, data_len);
                    if (xspace_ui_get_system_status() == XUI_SYS_PRODUCT_TEST) {
                        xspace_interface_delay_timer_stop((uint32_t)xspace_interface_shutdown);
                        xspace_interface_delay_timer_start(60000 * 20, (uint32_t)xspace_interface_shutdown, 0, 0, 0);
                    }
                    return 0;
                } else {
                    XPT_TRACE(1, "the handler of the cmd(%02X) is NULL.", cmd_id);
                    return 3;
                }
            } else {
                return 2;
            }
        }
    }

    XPT_TRACE(1, "Not found the cmd(%02X)", cmd_id);
    return 1;
}

int32_t xpt_cmd_recv_cmd_from_xbus(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    xpt_context.test_path = XPT_TEST_PATH_XBUS;
    return xpt_cmd_find_and_exec_handler(cmd, cmd_data, data_len);
}

int32_t xpt_cmd_recv_cmd_from_trace_uart(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    xpt_context.test_path = XPT_TEST_PATH_TRACE_UART;
    return xpt_cmd_find_and_exec_handler(cmd, cmd_data, data_len);
}
#if defined(__SKH_APP_DEMO_USING_SPP__)
int32_t xpt_cmd_recv_cmd_from_spp(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    xpt_context.test_path = XPT_TEST_PATH_SPP;
    return xpt_cmd_find_and_exec_handler(cmd, cmd_data, data_len);
}
#endif
void xpt_cmd_response(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    XPT_TRACE(2, "cmd=%02X, data_len=%d", cmd, data_len);

    if (xpt_context.test_path == XPT_TEST_PATH_TRACE_UART)
        xpt_trace_uart_send_data(cmd, cmd_data, data_len);
#if defined(__XSPACE_XBUS_MANAGER__)
    else if (xpt_context.test_path == XPT_TEST_PATH_XBUS)
        xbus_manage_send_data(cmd, cmd_data, data_len);
#endif
#if defined(__SKH_APP_DEMO_USING_SPP__)
    else if (xpt_context.test_path == XPT_TEST_PATH_SPP)
        xpt_spp_send_data(cmd, cmd_data, data_len);
#endif
}

void xspace_pt_init(void)
{
    xpt_context.status = XPT_STATUS_NORMAL_MODE;
    xpt_context.curr_cmd_id = XPT_CMD_NONE;
    xpt_context.test_path = XPT_TEST_PATH_TRACE_UART;
}

#endif   // (__XSPACE_PRODUCT_TEST__)
