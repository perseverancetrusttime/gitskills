#if defined(__XSPACE_PRODUCT_TEST__)
#include "pmu.h"
#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "xspace_pt_item_handler.h"
#include "xspace_pt_cmd_analyze.h"
#include "xspace_interface_process.h"
#include "xspace_interface.h"
#include "hal_bootmode.h"
#if defined(__XSPACE_COVER_SWITCH_MANAGER__)
#include "xspace_cover_switch_manager.h"
#endif
#include "Factory_section.h"
#if defined(__XSPACE_BATTERY_MANAGER__)
#include "Xspace_battery_manager.h"
#endif
#if defined(__XSPACE_INOUT_BOX_MANAGER__)
#include "Xspace_inout_box_manager.h"
#endif
#if defined(__XSPACE_GESTURE_MANAGER__)
#include "xspace_gesture_manager.h"
#endif
#include "Xspace_dev_info.h"
#include "Tgt_hardware.h"
#include "Hal_ntc.h"
//#include "app_ibrt_nvrecord.h"

#if defined(__XSPACE_RAM_ACCESS__)
#include "Xspace_ram_access.h"
#endif
#include "xspace_ui.h"
#include "app_bt.h"
#include "bt_if.h"
#include "app_utils.h"
#include "hal_norflash.h"
#include "norflash_drv.h"

extern const char xui_bt_init_name[];

void xspace_interface_loopback_test(bool on, uint8_t mic_channel)
{
    XPT_TRACE(1, "Neet to be implemented, on=%d, mic_channel=%d", on, mic_channel);
}

int32_t xpt_cmd_no_support_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_NO_SUPPORT_CMD;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_start_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    context->status = XPT_STATUS_TEST_MODE;
    xspace_ui_set_system_status(XUI_SYS_PRODUCT_TEST);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_enter_dut_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    osDelay(100);
    xspace_interface_enter_dut_mode();
    return 0;
}

int32_t xpt_cmd_reboot_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    osDelay(100);
    xspace_interface_reboot();

    return 0;
}

static uint8_t xpt_mic_channel = 0;   //0:normal, 1: talk mic, 2:enc mic, 3: fb mic;

uint8_t xpt_mic_test_get_channel(void)
{
    return xpt_mic_channel;
}

int32_t xpt_cmd_talk_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xpt_mic_channel = 1;
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_enc_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xpt_mic_channel = 2;
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_mac_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t bt_mac_addr[6];

    XPT_TRACE(1, "data_len=%d", data_len);

    factory_section_original_btaddr_get(bt_mac_addr);
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    memcpy((void *)&rsp_cmd_data[rsp_data_len], bt_mac_addr, 6);
    rsp_data_len += 6;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    osDelay(100);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_FREEMAN | HAL_SW_REBOOT_ENTER_PRODUCT_MODE);
    TRACE_IMM(1, "%s,reboot!", __func__);
    pmu_reboot();

    return 0;
}

int32_t xpt_cmd_ir_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    XPT_TRACE(1, "data_len=%d", data_len);
    return 0;
}

int32_t xpt_cmd_hall_switch_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    if (XCSM_BOX_STATUS_OPENED == xspace_cover_switch_manage_get_status()) {
        rsp_cmd_data[rsp_data_len++] = 2;
    } else if (XCSM_BOX_STATUS_CLOSED == xspace_cover_switch_manage_get_status()) {
        rsp_cmd_data[rsp_data_len++] = 1;
    } else {
        rsp_cmd_data[0] = XPT_RSP_SENSOR_NOT_INT;
        rsp_data_len = 1;
    }

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_touch_key_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

#if defined(__TOUCH_GH61X__) || defined(__TOUCH_GH621X__)
    xra_spp_mode_write_noInit_data(0x5aa5aa01);
#endif

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    osDelay(100);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_FREEMAN | HAL_SW_REBOOT_ENTER_PRODUCT_MODE);
    TRACE_IMM(1, "%s,Freeman!", __func__);
    pmu_reboot();

    return 0;
}

int32_t xpt_cmd_write_bt_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t read_addr[6];

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (data_len == 0) {
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        factory_section_original_btaddr_get(read_addr);

        memcpy(&rsp_cmd_data[rsp_data_len], read_addr, 6);
        rsp_data_len += 6;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    } else if (data_len == 6) {

        if (data_len != 6) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
            xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
            return 1;
        }

        int update_result = factory_section_set_bt_addr(cmd_data);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else if (update_result == -2) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FLASH_WRITE_FAIL;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }

        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
        return 0;
    }

    return 0;
}

int32_t xpt_cmd_write_ble_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t read_addr[6];

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (data_len == 0) {
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        factory_section_original_bleaddr_get(read_addr);

        memcpy(&rsp_cmd_data[rsp_data_len], read_addr, 6);
        rsp_data_len += 6;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    } else if (data_len == 6) {

        if (data_len != 6) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
            xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
            return 1;
        }

        int update_result = factory_section_ble_addr_set(cmd_data);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else if (update_result == -2) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FLASH_WRITE_FAIL;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }

        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
        return 0;
    }

    return 0;
}

int32_t xpt_cmd_write_rf_calib_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    int update_result;
    uint8_t write_value = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("0x%02X.", cmd_data, data_len);

    if (data_len == 0) {
        //read xtal fcap
        factory_section_xtal_fcap_get((unsigned int *)&rsp_cmd_data[0]);
        XPT_TRACE(1, "read xtal fcap=%d", rsp_cmd_data[0]);
        rsp_data_len = 1;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    } else if (data_len == 1) {
        //set xtal fcap
        write_value = *cmd_data;   //longz 20200518 modify error
        update_result = factory_section_xtal_fcap_set((unsigned int)write_value);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }
    } else {
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
    }

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_led_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_device_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[30] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t bt_mac_addr[6] = {0};
    uint8_t ble_mac_addr[6] = {0};
    uint8_t character_id = 0;
    unsigned int rf_calib;
    uint8_t box_open = 0;
    uint8_t out_box = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    if (context->test_path == XPT_TEST_PATH_TRACE_UART) {
        factory_section_original_btaddr_get(bt_mac_addr);
        factory_section_original_bleaddr_get(ble_mac_addr);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        memcpy(&rsp_cmd_data[rsp_data_len], bt_mac_addr, 6);
        rsp_data_len += 6;

        memcpy(&rsp_cmd_data[rsp_data_len], ble_mac_addr, 6);
        rsp_data_len += 6;

        rsp_cmd_data[rsp_data_len++] = xspace_battery_manager_bat_get_percentage();
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[0];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[1];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[2];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[3];

        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    } else if (context->test_path == XPT_TEST_PATH_XBUS) {
        if (0 == app_tws_get_earside()) {
            XPT_TRACE(0, "--- Device Character	  : Left\n");
            character_id = 0;
        } else {
            XPT_TRACE(0, "--- Device Character	  : Right\n");
            character_id = 1;
        }

        factory_section_original_btaddr_get(bt_mac_addr);
        factory_section_xtal_fcap_get(&rf_calib);

        if (XIOB_OUT_BOX == xspace_inout_box_manager_get_curr_status())
            out_box = 1;

        if (XCSM_BOX_STATUS_OPENED == xspace_cover_switch_manage_get_status())
            box_open = 1;

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        rsp_cmd_data[rsp_data_len++] = (out_box << 7) | (box_open << 6) | (0 << 2) | (character_id << 1) | xspace_battery_manager_bat_is_charging();

        rsp_cmd_data[rsp_data_len++] = 0;
        rsp_cmd_data[rsp_data_len++] = 0;
        rsp_cmd_data[rsp_data_len++] = 0;
        rsp_cmd_data[rsp_data_len++] = 0;
        rsp_cmd_data[rsp_data_len++] = rf_calib;
        rsp_cmd_data[rsp_data_len++] = xspace_battery_manager_bat_get_percentage();
        memcpy(&rsp_cmd_data[rsp_data_len], bt_mac_addr, 6);
        rsp_data_len += 6;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    }
#if defined(__SKH_APP_DEMO_USING_SPP__)
    else if (context->test_path == XPT_TEST_PATH_SPP) {
        factory_section_original_btaddr_get(bt_mac_addr);
        factory_section_original_bleaddr_get(ble_mac_addr);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        memcpy(&rsp_cmd_data[rsp_data_len], bt_mac_addr, 6);
        rsp_data_len += 6;

        memcpy(&rsp_cmd_data[rsp_data_len], ble_mac_addr, 6);
        rsp_data_len += 6;

        rsp_cmd_data[rsp_data_len++] = xspace_battery_manager_bat_get_percentage();
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[0];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[1];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[2];
        rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[3];

        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    }
#endif
    return 0;
}

int32_t xpt_cmd_stop_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    context->status = XPT_STATUS_NORMAL_MODE;
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    osDelay(20);
#if defined(__XSPACE_KY01__)
    app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
    xspace_interface_delay_timer_start(3000, (uint32_t)xspace_interface_shutdown, 0, 0, 0);
#else
    xspace_interface_shutdown();
#endif
    return 0;
}

int32_t xpt_cmd_get_battery_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint16_t bat_vol = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    bat_vol = xspace_battery_manager_bat_get_voltage();
    XPT_TRACE(1, "bat_vol=%d", bat_vol);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;

    if (cmd_data[0] == 0)   //read bat voltage
    {
        rsp_cmd_data[rsp_data_len++] = (bat_vol & 0xff00) >> 8;
        rsp_cmd_data[rsp_data_len++] = (bat_vol & 0x00ff);

    } else if (cmd_data[0] == 1)   //read bat percentage
    {
        rsp_cmd_data[rsp_data_len++] = xspace_battery_manager_bat_get_percentage();
    }

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_ntc_temp_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

#if defined(__NTC_SUPPORT__)
    int16_t ntc_temp_value = 0;
    hal_ntc_get_temperature(&ntc_temp_value);
#endif

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
#if defined(__NTC_SUPPORT__)
    rsp_cmd_data[rsp_data_len++] = (int8_t)ntc_temp_value;
#else
    rsp_cmd_data[rsp_data_len++] = 0;
#endif
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_version_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    rsp_cmd_data[rsp_data_len++] = 0x01;
    rsp_cmd_data[rsp_data_len++] = 0x02;
    rsp_cmd_data[rsp_data_len++] = 0x03;
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[0];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[1];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[2];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[3];
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_ble_addr_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t ble_mac_addr[6] = {0};

    XPT_TRACE(1, "data_len=%d", data_len);

    factory_section_original_bleaddr_get(ble_mac_addr);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    memcpy((void *)&rsp_cmd_data[rsp_data_len], ble_mac_addr, 6);
    rsp_data_len += 6;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_talk_mic_loopback_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    xspace_interface_loopback_test(false, 0);
    xspace_interface_loopback_test(true, 0);

    return 0;
}

int32_t xpt_cmd_enc_mic_loopback_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    xspace_interface_loopback_test(false, 1);
    xspace_interface_loopback_test(true, 1);

    return 0;
}

int32_t xpt_cmd_gsensor_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_anc_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_hall_ctrl_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_ear_side_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = (uint8_t)app_tws_get_earside();
    ;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_fb_mic_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[2] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xpt_mic_channel = 3;
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_ear_color_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    int update_result;
    uint8_t write_value = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (data_len == 0) {
        //read xtal fcap
        update_result = factory_section_ear_color_get((unsigned int *)&rsp_cmd_data[1]);
        XPT_TRACE(1, "read ear_color=%d", rsp_cmd_data[1]);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }
        rsp_data_len++;
    } else if (data_len == 1) {
        //set xtal fcap
        write_value = *cmd_data;
        update_result = factory_section_ear_color_set((unsigned int)write_value);
        XPT_TRACE(2, "write ear color %d, result %d", write_value, update_result);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }
    } else {
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
    }

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_get_ear_india_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    int update_result;
    uint8_t write_value = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (data_len == 0) {
        //read local value
        update_result = factory_section_ear_india_get((unsigned int *)&rsp_cmd_data[1]);

        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }

        rsp_data_len++;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    } else if (data_len == 1) {
        write_value = *cmd_data;   //0:other local   1:India local
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

        factory_section_ear_india_set((unsigned int)write_value);
    }

    return 0;
}

#if defined(__TOUCH_GH621X__)   //longz 20210128 add
extern uint8_t g_wear_calib_check_flag;    //0:fail   1:sucess
extern uint8_t g_press_calib_check_flag;   //0:fail   1:sucess
#endif

int32_t xpt_cmd_touch_calib_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    //rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
#if defined(__XSPACE_UI__)   //longz 20210128 add
    XPT_TRACE(1, "g_wear_calib_check_flag=%d", g_wear_calib_check_flag);
    XPT_TRACE(1, "g_press_calib_check_flag=%d", g_press_calib_check_flag);

    rsp_cmd_data[rsp_data_len++] = g_wear_calib_check_flag;    //0:fail   1:sucess
    rsp_cmd_data[rsp_data_len++] = g_press_calib_check_flag;   //0:fail   1:sucess
#endif

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

#if 0   //defined (__TOUCH_GH61X__)
    if(!GH61X_Sensor_thread_create_status())
    {
        XPT_TRACE(0, "Success\n");
        memcpy(gh61x_uart_rx,pt_rx_frame_data, len);
        gh61x_adapter_event_process(MSG_ID_DEAL_SPP_RX_DATA, (uint32_t *)gh61x_uart_rx, (uint32_t)len, 0);
    }
#endif

    return 0;
}

#if 0   //defined (__TOUCH_GH61X__)
void xpt_touch_send_data(u8 databuf[], uint8_t len)
{
    uint8_t rsp_cmd_data[100] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    if(!GH61X_Sensor_thread_create_status()) {
        memcpy(&rsp_cmd_data[0],databuf, len);
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    }
}
#endif

int32_t xpt_cmd_get_op_mode_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    rsp_cmd_data[rsp_data_len++] = (uint8_t)xspace_interface_get_audio_status();
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_bt_disconnect_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    //app_disconnect_all_bt_connections();

    return 0;
}

int32_t xpt_cmd_get_audio_status_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    rsp_cmd_data[rsp_data_len++] = xspace_interface_audio_status();
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_answer_call_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    if (xspace_interface_call_answer_call() == 0)
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    else
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_NO_INCOMING_CALL;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_hangup_call_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    if (xspace_interface_call_hangup_call() == 0)
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    else
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_NOT_IN_CALLING;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_music_play_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_PLAYING);
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_music_pause_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_PAUSE);
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_music_forward_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_FORWARD);
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_music_backward_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    xspace_interface_gesture_music_ctrl(XIF_MUSIC_STATUS_BACKWARD);
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_get_volume_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    rsp_cmd_data[rsp_data_len++] = xspace_interface_get_a2dp_volume(0);
    rsp_cmd_data[rsp_data_len++] = xspace_interface_get_hfp_volume(0);
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_switch_role_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
#if 0
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    if (xspace_interface_start_tws_switch())
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    else
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_NOT_SUCCESS;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
#endif
    return 0;
}

int32_t xpt_cmd_get_role_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    if (xspace_interface_tws_is_master_mode())
        rsp_cmd_data[rsp_data_len++] = 0;
    else if (xspace_interface_tws_is_slave_mode())
        rsp_cmd_data[rsp_data_len++] = 1;
    else
        rsp_cmd_data[rsp_data_len++] = 2;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_check_factory_reset_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    //bt_bdaddr_t mobile_addr;

    XPT_TRACE(1, "data_len=%d", data_len);

    if (1)   //if (BT_STS_SUCCESS != app_ibrt_nvrecord_get_mobile_addr(&mobile_addr))
        rsp_cmd_data[rsp_data_len++] = 1;
    else
        rsp_cmd_data[rsp_data_len++] = 0;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_disconnect_hfp_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    if (xspace_interface_disconnect_hfp() == 0)
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    else
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_HFP_NOT_CONNECTED;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_disconnect_a2dp_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    if (xspace_interface_disconnect_a2dp() == 0)
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    else
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_A2DP_NOT_CONNECTED;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_reset_fatctory_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    xspace_interface_reset_factory_setting();

    return 0;
}

int32_t xpt_cmd_poweroff_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    osDelay(20);
    xspace_interface_shutdown();

    return 0;
}

int32_t xpt_cmd_reset_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    osDelay(20);
    xspace_interface_reboot();

    return 0;
}

int32_t xpt_cmd_long_standby_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
    //app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

/*int32_t xpt_cmd_start_pairing_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
	//bt_bdaddr_t connect_bt_addr;

    XPT_TRACE(1, "data_len=%d", data_len);

	if(data_len == 6)
	{
		//btif_pts_hf_create_service_link(connect_bt_addr);
	}

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}*/

int32_t xpt_cmd_start_pairing_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    bt_bdaddr_t connect_bt_addr;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);
    for (uint8_t i = 0; i < 6; i++) {
        connect_bt_addr.address[i] = *(cmd_data + i);
    }
    btif_pts_hf_create_service_link(&connect_bt_addr);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_get_dev_mac_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[30] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t bt_mac_addr[6] = {0};
    uint8_t ble_mac_addr[6] = {0};

    XPT_TRACE(1, "data_len=%d", data_len);

    factory_section_original_btaddr_get(bt_mac_addr);
    factory_section_original_bleaddr_get(ble_mac_addr);

    memcpy(&rsp_cmd_data[rsp_data_len], bt_mac_addr, 6);
    rsp_data_len += 6;

    memcpy(&rsp_cmd_data[rsp_data_len], ble_mac_addr, 6);
    rsp_data_len += 6;

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

extern void app_bt_volumeup(void);
extern void app_bt_volumedown(void);

int32_t xpt_cmd_get_volume_up_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    app_bt_volumeup();

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_volume_down_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    app_bt_volumedown();

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_write_ear_color_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    int update_result;
    uint8_t write_value = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (data_len == 1) {
        //set xtal fcap
        write_value = *cmd_data;
        update_result = factory_section_ear_color_set((unsigned int)write_value);
        if (update_result == -1) {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_FACTORY_ZERO_INVALID;
        } else {
            rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        }
    } else {
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
    }

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_get_bt_name_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[30] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t bt_name_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    bt_name_len = strlen(xui_bt_init_name);
    if (bt_name_len > 15) {
        bt_name_len = 15;
    }
    for (uint8_t i = 0; i < bt_name_len; i++) {
        rsp_cmd_data[rsp_data_len++] = xui_bt_init_name[i];
    }

    XPT_TRACE(1, "bt_name: %s", xui_bt_init_name);
    XPT_TRACE(2, "bt_name_len: %d, rsp_data_len: %d", bt_name_len, rsp_data_len);

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_pressure_test_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
#ifndef __XSPACE_UI__
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    int32_t ret = 0;
    uint8_t rcv_rawdata[20] = {0};
    CS_CALIBRATION_RESULT_Def *calibration_result = (CS_CALIBRATION_RESULT_Def *)rcv_rawdata;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

#if defined(__PRESSURE_CSA37F71__)
    if (cmd_data[0] == XPT_PRESSURE_DEVICE_WAKEUP_ITEM)   //test start
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_DEVICE_WAKEUP_ITEM, &cmd_data[1]);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        XPT_TRACE(1, "pressure_cali_start ret%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_EN_ITEM)   //test start
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_EN_ITEM, NULL);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        XPT_TRACE(1, "pressure_cali_start ret%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_CHECK_ITEM)   //check test state
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_CHECK_ITEM, rcv_rawdata);
        XPT_TRACE(1, "pressure_cali_check ret:%d", ret);

        XPT_TRACE(5, " progress:0x%02x,factor:%d,adc:%d %d %d", calibration_result->calibration_progress, calibration_result->calibration_factor,
                  calibration_result->press_adc_1st, calibration_result->press_adc_2nd, calibration_result->press_adc_3rd);

        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];
    } else if (cmd_data[0] == XPT_PRESSURE_FINSH_ITEM)   //test finsh
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_FINSH_ITEM, NULL);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        XPT_TRACE(1, "pressure_cali_finsh ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_RAWDATA_ITEM)   //READ_RAWDATA
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_RAWDATA_ITEM, rcv_rawdata);

        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_RAWDATA ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_NOISE_ITEM)   //noise value
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_NOISE_ITEM, rcv_rawdata);

        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_PRESSURE ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_OFFSET_ITEM)   //offset value
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_OFFSET_ITEM, rcv_rawdata);

        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_PRESSURE ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_PRESSURE_ITEM)   //READ_PRESSURE value
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_PRESSURE_ITEM, rcv_rawdata);

        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_PRESSURE ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_PRESS_LEVEL_ITEM)   //PRESS_LEVEL READ
    {
        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_PRESS_LEVEL_ITEM, rcv_rawdata);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_PRESS_LEVEL ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_WRITE_PRESS_LEVEL_ITEM)   //PRESS_LEVEL WRITE
    {
        uint8_t rcv_press_level[2] = {0};
        rcv_press_level[0] = cmd_data[1];
        rcv_press_level[1] = cmd_data[2];

        //DUMP8("%02X,", rcv_rawdata, 2);
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_WRITE_PRESS_LEVEL_ITEM, rcv_press_level);
        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        XPT_TRACE(1, "WRITE_PRESS_LEVEL ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_READ_CALIBRATION_FACTOR_ITEM)   //CALIBRATION READ
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_READ_CALIBRATION_FACTOR_ITEM, rcv_rawdata);

        //DUMP8("%02X,", &rcv_rawdata[1], rcv_rawdata[0]);
        memcpy(&rsp_cmd_data[rsp_data_len], &rcv_rawdata[1], rcv_rawdata[0]);
        rsp_data_len += rcv_rawdata[0];

        XPT_TRACE(1, "READ_CALIBRATION ret:%d", ret);
    } else if (cmd_data[0] == XPT_PRESSURE_WRITE_CALIBRATION_FACTOR_ITEM)   //CALIBRATION WRITE
    {
        uint8_t rcv_press_calib[2] = {0};
        rcv_press_calib[0] = cmd_data[1];
        rcv_press_calib[1] = cmd_data[2];

        //DUMP8("%02X,", rcv_rawdata, rcv_rawdata[0]);
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_WRITE_CALIBRATION_FACTOR_ITEM, rcv_press_calib);

        rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
        XPT_TRACE(1, "WRITE_CALIBRATION ret:%d", ret);
    }
#endif
#if 0   //defined(__PRESSURE_AW8680X__)
    if(cmd_data[0] == XPT_PRESSURE_CALI_READY)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_CALI_READY,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_CALI_READY ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_CALI_PRESS)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_CALI_PRESS,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_CALI_PRESS ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_CALI_WRITE)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_CALI_WRITE,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_CALI_WRITE ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_TEST_READY)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_TEST_READY,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_TEST_READY ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_TEST_PRESS)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_TEST_PRESS,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_TEST_PRESS ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_TEST_FINISH)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_TEST_FINISH,NULL);
        XPT_TRACE(1, "XPT_PRESSURE_TEST_FINISH ret:%d",ret);
    }
    else if(cmd_data[0] == XPT_PRESSURE_TEST_RAWDATA)
    {
        ret = xspace_gesture_manage_pressure_cali((uint8_t)XPT_PRESSURE_TEST_RAWDATA,rcv_rawdata);
        XPT_TRACE(1, "XPT_PRESSURE_TEST_RAWDATA ret:%d",ret);

	    if(0 != *rcv_rawdata)
	    {
	        memcpy(&rsp_cmd_data[rsp_data_len],rcv_rawdata+1,*rcv_rawdata);
	        rsp_data_len += *rcv_rawdata;
	        XPT_TRACE(1, "rcv_rawdata len=%d", *rcv_rawdata);
	        DUMP8("%02X,", rcv_rawdata, *rcv_rawdata+1);
	    }
    }
#endif

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
#endif
    return 0;
}

int32_t xpt_cmd_get_enc_arithmetic_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    //bobby add interface

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_get_bt_chipid_info_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[25] = {0};
    uint16_t rsp_data_len = 0;
    uint8_t bt_chip_uid[16];

    XPT_TRACE(1, "data_len=%d", data_len);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_14, APP_SYSFREQ_104M);
    TRACE(1, "Main TestUID1: app_sysfreq_req %d", APP_SYSFREQ_104M);
    TRACE(1, "sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

    norflash_get_unique_id(HAL_FLASH_ID_0, bt_chip_uid, HAL_NORFLASH_UNIQUE_ID_LEN);
    TRACE_IMM(0, "Flash UID:");
    DUMP8("%02x,", bt_chip_uid, 16);
    TRACE(0, " Done!!!");

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;

    memcpy(&rsp_cmd_data[rsp_data_len], bt_chip_uid, 16);
    rsp_data_len += 16;

    DUMP8("%02x,", rsp_cmd_data, rsp_data_len);

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_get_hardware_version_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    rsp_cmd_data[rsp_data_len++] = app_get_ear_hw_version();

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);

    return 0;
}

int32_t xpt_cmd_check_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    //bobby add interface
    //rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_write_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    //bobby add interface

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

int32_t xpt_cmd_read_lhdc_link_key_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

    //bobby add interface

    rsp_cmd_data[rsp_data_len++] = XPT_RSP_SUCCESS;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}
void spp_cmd_start_test_handler(void)
{
    xspace_ui_set_system_status(XUI_SYS_PRODUCT_TEST);
}

void spp_cmd_reset_test_handler(void)
{
    osDelay(100);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_FREEMAN | HAL_SW_REBOOT_ENTER_PRODUCT_MODE);

    xspace_interface_reboot();
}

void spp_cmd_talk_mic_test_handler(void)
{
    xpt_mic_channel = 1;
}
void spp_cmd_fb_mic_test_handler(void)
{
    xpt_mic_channel = 2;
}
void spp_cmd_ff_mic_test_handler(void)
{
    xpt_mic_channel = 3;
}

extern int32_t xspace_gesture_manage_get_chip_id(uint32_t *chip_id);

int32_t xpt_cmd_get_presure_chip_id_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
#ifndef __XSPACE_UI__
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;
    uint32_t chip_id = 0x00;

    XPT_TRACE(1, "data_len=%d", data_len);
    xspace_gesture_manage_get_chip_id((uint32_t *)&chip_id);

    rsp_cmd_data[rsp_data_len++] = (uint8_t)((chip_id >> 8) & 0xff);
    rsp_cmd_data[rsp_data_len++] = (uint8_t)(chip_id & 0xff);

    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
#endif
    return 0;
}

int32_t xpt_cmd_anc_mode_state_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);
    DUMP8("0x%02X.", cmd_data, data_len);

    if (cmd_data[0] == 0)   //get anc mode
    {
#if defined(__XSPACE_CUSTOMIZE_ANC__)
    //TODO(Mike):if need,plz implement
        //rsp_cmd_data[rsp_data_len++] = xspace_ui_get_anc_mode();
#endif
        xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    } else {
#if defined(__XSPACE_CUSTOMIZE_ANC__)
        if (cmd_data[0] >= 1 && cmd_data[0] <= 4) {
            //TODO(Mike):if need,plz implement
            //xspace_ui_set_anc_mode(cmd_data[0], false);
            rsp_cmd_data[rsp_data_len++] = cmd_data[0];
            xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
        }
#endif
    }

    return 0;
}

int32_t xpt_cmd_get_is_burn_anc_handler(xpt_context_s *context, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    XPT_TRACE(1, "data_len=%d", data_len);

//TODO(Mike):if need ,plz implement it
    rsp_cmd_data[rsp_data_len++] = XPT_RSP_DATA_INVALID;
    xpt_cmd_response(context->curr_cmd_id, rsp_cmd_data, rsp_data_len);
    return 0;
}

#endif   // (__XSPACE_PRODUCT_TEST__)
