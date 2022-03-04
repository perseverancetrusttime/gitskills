#if defined(__XSPACE_XBUS_MANAGER__)
#include "pmu.h"
#include "stdio.h"
#include "cmsis.h"
#include "app_bt.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_chipid.h"
#include "hal_bootmode.h"
#include "nvrecord_env.h"
#include "factory_section.h"
#include "xspace_interface.h"
#include "nvrecord_extension.h"
#include "xspace_xbus_manager.h"
#include "xspace_xbus_cmd_analyze.h"
#include "xspace_interface_process.h"
#include "xspace_ui.h"
#include "nvrecord_bt.h"
#include "app_status_ind.h"

#if defined(__XSPACE_BATTERY_MANAGER__)
#include "xspace_battery_manager.h"
#endif

#if defined(__DEV_THREAD__)
#include "dev_thread.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_ibrt_customif_ui.h"
#include "app_ibrt_voice_report.h"

#if defined(IBRT_UI_V1)
#include "app_ibrt_ui_test.h"
#endif

#if defined(IBRT_CONN_API_ENABLE)
#include "app_tws_ibrt_raw_ui_test.h"
#include "app_custom_api.h"
#endif

#endif

#include "hal_xbus.h"

#if defined(__XSPACE_XBUS_OTA__)
#include "xspace_xbus_ota.h"
#endif

#if defined(__XSPACE_BATTERY_MANAGER__)
#include "hal_charger.h"
#endif

extern xbus_manage_report_ui_event_func xbus_manage_report_ui_handle;

#if defined(__XSPACE_XBUS_OTA__)
extern const uint8_t xui_dev_sw_version[4];
static osTimerId xbus_ota_timeout_check_timer = NULL;
static void xbus_ota_timeout_check_timer_handler(void const *param);
osTimerDef(XBUS_OTA_TIMEROUT_CHECK_TIMER, xbus_ota_timeout_check_timer_handler);
extern bool xspace_interface_mobile_link_connected(const bt_bdaddr_t *mobile_addr);
extern "C" int app_bt_stream_closeall();
extern bool xspace_interface_slave_ibrt_link_connected(const bt_bdaddr_t *addr);
#if defined(__XSPACE_FLASH_ACCESS__)
extern "C" int hw_rst_use_bat_data_flag_write_flash(uint8_t flag);
#endif

static void xbus_ota_timeout_check_timer_handler(void const *param)
{
    hal_xbus_mode_set(XBUS_MODE_OTA_EXIT);
}

xbus_manage_ota_start_handle_cb xbus_cmd_ota_start_handle = NULL;
void xbus_cmd_ota_register_cb(xbus_manage_ota_start_handle_cb cb)
{
    xbus_cmd_ota_start_handle = cb;
}
#endif

static int32_t xbm_cmd_tws_pair_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t rlt = 0;
    uint8_t send_buf[6] = {0};

    TRACE(0, "\n--------------XBUS_CMD_TWS_PAIRING--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

#if 0   //defined (__XSPACE_UI__)
    if (xspace_interface_tws_link_connected()) {
        X2BM_TRACE(0, "TWS connected,return");
        return 0;
    }
#endif
    if (NULL == cmd_data || data_len < 6) {
        X2BM_TRACE(0, "invaild data");
    }

    factory_section_original_btaddr_get(send_buf);
    xbus_manage_send_data(X2BM_CMD_TWS_PAIRING, send_buf, 6);
    osDelay(20);

    rlt = xspace_interface_tws_pairing_config(cmd_data);
    if (rlt) {
        app_bt_stream_closeall();
        xspace_interface_write_data_to_flash_befor_shutdown();
        nv_record_flash_flush();
#if defined(__XSPACE_FLASH_ACCESS__)
        hw_rst_use_bat_data_flag_write_flash(true);
#endif
        TRACE_IMM(1, "%s,reboot!", __func__);
        osDelay(100);
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_PAIRING);
        pmu_reboot();
    }

    return 0;
}

static int32_t xbm_cmd_query_pairing_status_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------XBUS_CMD_PAIRING_STATUS--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (NULL != cmd_data && NULL != xbus_manage_report_ui_handle) {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xbus_manage_report_ui_handle, XBUS_MANAGE_SYNC_BOX_BAT_EVENT, (uint32_t)cmd_data, 1);
    }

    /**tws pair status*/
    X2BM_TRACE(2, " %d/%d", xspace_interface_tws_is_master_mode(), xspace_interface_tws_is_slave_mode());
    if (xspace_interface_tws_is_master_mode()) {
        if (xspace_interface_tws_link_connected()) {

            if (xspace_interface_is_any_mobile_connected() && xspace_interface_get_avrcp_connect_status() && !xspace_interface_device_is_in_paring_mode()) {
                send_data = 2;
            } else {
                send_data = 1;
            }
        }
    } else if (xspace_interface_tws_is_slave_mode()) {
        if (xspace_interface_is_any_mobile_connected() && !xspace_interface_device_is_in_paring_mode()) {
            send_data = 2;
        } else {
            send_data = 1;
        }
    }
    /**freeman pair status*/
    else if (xspace_interface_tws_is_freeman_mode()) {
        if (xspace_interface_is_any_mobile_connected() && (xspace_interface_get_avrcp_connect_status()) && (!xspace_interface_device_is_in_paring_mode())) {
            send_data = 3;
        }
    }

    X2BM_TRACE(4, "Pairing_status:%d,%d,%d,%d,%d.", send_data, xspace_interface_tws_is_master_mode(), xspace_interface_tws_link_connected(),
               xspace_interface_is_any_mobile_connected(), xspace_interface_device_is_in_paring_mode());

    xbus_manage_send_data(X2BM_CMD_QUERY_PAIRING_STATUS, &send_data, 1);

    return 0;
}

static int32_t xbm_cmd_enter_ota_mode_handler(uint8_t *cmd_data, uint16_t data_len)
{

    TRACE(0, "\n--------------XBUS_CMD_ENTER_OTA_MODE--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);
    /*
    uint8_t send_data = 0;
    if (xspace_interface_process_bat_allow_enter_ota_mode()) {
        send_data = 1;
    } else {
        send_data = 0;
    }

    xbus_manage_send_data(X2BM_CMD_ENTER_OTA_MODE, &send_data, 1);
    if (send_data) {
    #if defined(__CHARGER_CW6305__)
        extern int cwset_watchdog_timer(int sec);
        cwset_watchdog_timer(240);
    #endif
        osDelay(20);
        xspace_interface_enter_ota_mode();
    }
    */
    return 0;
}

static int32_t xbm_cmd_restore_factory_setting_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------XBUS_CMD_RESTORE_FACTORY_SETTING--------------\n");
#if defined(__XSPACE_UI__)
    app_status_indication_set(APP_STATUS_INDICATION_RESET_FACTORY);
#endif
    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    send_data = 1;
    xbus_manage_send_data(X2BM_CMD_RESTORE_FACTORY_SETTING, &send_data, 1);
    osDelay(20);
#if defined(__XSPACE_FLASH_ACCESS__)
    hw_rst_use_bat_data_flag_write_flash(true);
#endif

    xspace_interface_reset_factory_setting();
    return 0;
}

static int32_t xbm_cmd_ctrl_ear_poweroff_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------XBUS_CMD_POWEROFF--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    send_data = 1;
    xbus_manage_send_data(X2BM_CMD_CTRL_EAR_POWEROFF, &send_data, 1);

#if defined(__XSPACE_UI__)
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
#endif
#if defined(__XSPACE_FLASH_ACCESS__)
    hw_rst_use_bat_data_flag_write_flash(true);
#endif

    osDelay(20);
    xspace_interface_shutdown();

    return 0;
}

extern void app_tws_entry_shipmode(void);                                             //longz 20210219 add
static int32_t xbm_cmd_entry_shipmode_handler(uint8_t *cmd_data, uint16_t data_len)   //longz 20210219 add
{
    uint8_t send_data = 0;
    TRACE(0, "\n--------------X2BM_CMD_HW_SHIPMODE--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    send_data = 1;
    xbus_manage_send_data(X2BM_CMD_HW_SHIPMODE, &send_data, 1);
    osDelay(20);
#if defined(__XSPACE_UI__)
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
#endif
#if defined(__XSPACE_FLASH_ACCESS__)
    hw_rst_use_bat_data_flag_write_flash(true);
#endif
    osDelay(50);
    app_tws_entry_shipmode();

    return 0;
}

static int32_t xbm_cmd_query_exchange_info_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_buf[4] = {0};
    uint8_t box_chg_flag = 0;

    TRACE(0, "\n--------------XBUS_CMD_EXCHANGE_INFO--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    if (NULL == cmd_data) {
        return 0;
    }

    box_chg_flag = *cmd_data++;
    if (NULL != cmd_data && NULL != xbus_manage_report_ui_handle) {
        xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xbus_manage_report_ui_handle, XBUS_MANAGE_SYNC_BOX_BAT_EVENT, (uint32_t)cmd_data, 1);
        if (data_len == 6) {
            cmd_data++;
            xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xbus_manage_report_ui_handle, XBUS_MANAGE_SYNC_BOX_VER_EVENT, (uint32_t)cmd_data, 4);
        }
    }

    X2BM_TRACE(1, " box chg flag:%d", box_chg_flag);

#if defined(__XSPACE_BATTERY_MANAGER__)
    hal_charger_set_xbus_vsys_limit(box_chg_flag);
    send_buf[0] = (uint8_t)xspace_battery_manager_bat_get_percentage();
#else
    send_buf[0] = 100;
#endif
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();

    send_buf[2] = (uint8_t)xspace_interface_have_mobile_record();

    if (xspace_interface_tws_is_freeman_mode()) {
        send_buf[1] = 0;   // freeman mode
    } else {
        if (IBRT_SLAVE == p_ibrt_ctrl->nv_role) {
            send_buf[1] = 2;   // slave
        } else if (IBRT_MASTER == p_ibrt_ctrl->nv_role) {
            send_buf[1] = 1;   // master
        } else {
            send_buf[1] = 0xFF;   // unknown
        }
    }
    send_buf[3] = hal_get_chip_metal_id();
    X2BM_TRACE(3, "current percent:%d,mode:%d,bt_connect:%d.", send_buf[0], send_buf[1], send_buf[2]);

    xbus_manage_send_data(X2BM_CMD_QUERY_EXCHANGE_INFO, send_buf, 4);

    return 0;
}

static int32_t xbm_cmd_hw_reboot_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------XBUS_CMD_HW_REBOOT--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    xbus_manage_send_data(X2BM_CMD_HW_REBOOT, &send_data, 1);
    osDelay(20);
    xspace_interface_reboot();

    return 0;
}

static int32_t xbm_cmd_stop_pairing_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------XBUS_CMD_STOP_PAIRING--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    xbus_manage_send_data(X2BM_CMD_STOP_PARING, &send_data, 1);
    return 0;
}

static int32_t xbm_cmd_single_pairing_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;
    bool need_reboot = 0;

    TRACE(0, "\n--------------XBUS_CMD_SINGLE_PAIRING--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);
    if (!xspace_interface_tws_link_connected()) {
        send_data = 1;
    } else {
        send_data = 0;
    }

    xbus_manage_send_data(X2BM_CMD_SINGLE_PAIRING, &send_data, 1);
    if (send_data == 0) {
        X2BM_TRACE(1, "freeman pair failed,%d!", xspace_interface_tws_link_connected());
    } else {
#if defined(__REBOOT_FORCE_PAIRING__)
        need_reboot = true;
#else
        X2BM_TRACE(1, "freeman %d! need_reboot %d", xspace_interface_tws_is_freeman_mode(),need_reboot);
        if (xspace_interface_tws_is_freeman_mode()) {
            app_ibrt_if_enter_freeman_pairing();
        } else {
            need_reboot = true;
        }
#endif
        if (need_reboot) {
            app_bt_stream_closeall();
            xspace_interface_write_data_to_flash_befor_shutdown();
            nv_record_flash_flush();
#if defined(__XSPACE_FLASH_ACCESS__)
            hw_rst_use_bat_data_flag_write_flash(true);
#endif
            osDelay(100);
            TRACE_IMM(1, "%s,reboot!", __func__);
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
            hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_ENTER_FREEMAN);
            pmu_reboot();
        }
    }

    return 0;
}

static int32_t xbm_cmd_inbox_auto_enter_ota_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------X2BM_CMD_INBOX_START_OTA--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    xbus_manage_send_data(X2BM_CMD_INBOX_START_OTA, &send_data, 1);
    osDelay(20);

#if defined(__XSPACE_UI__)
    xspace_interface_write_data_to_flash_befor_shutdown();
    nv_record_flash_flush();
#endif
#if defined(__XSPACE_FLASH_ACCESS__)
    hw_rst_use_bat_data_flag_write_flash(true);
#endif

    return 0;
}

static int32_t xbm_cmd_start_xbus_fwu_handler(uint8_t *cmd_data, uint16_t data_len)
{
    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

#if defined(__XSPACE_XBUS_OTA__)
    uint8_t send_data = 1;
    uint32_t file_size = 0;
    if (!xspace_interface_process_bat_allow_enter_ota_mode()) {
        X2BM_TRACE(0, "bat low,xbus ota not allow.");
        return -1;
    }

    xbus_manage_send_data(X2BM_CMD_START_XBUS_FWU, &send_data, 1);

    if (NULL != xbus_cmd_ota_start_handle) {
        xbus_cmd_ota_start_handle();
    }

    file_size = cmd_data[0] << 24 | cmd_data[1] << 16 | cmd_data[2] << 8 | cmd_data[3];
    if (file_size > XSPACE_OTA_BACKUP_SECTION_SIZE) {
        X2BM_TRACE(2, "Error:new ota bin size:%d, more than %d.", file_size, XSPACE_OTA_BACKUP_SECTION_SIZE);
        return -2;
    }
    xbus_ota_fw_size_set(file_size);
    osDelay(20); /* waiting for xbus tx end,then switch xbus mode to ota*/
    hal_xbus_mode_set(XBUS_MODE_OTA);

    if (xbus_ota_timeout_check_timer == NULL) {
        xbus_ota_timeout_check_timer = osTimerCreate(osTimer(XBUS_OTA_TIMEROUT_CHECK_TIMER), osTimerOnce, NULL);
    }

    osTimerStop(xbus_ota_timeout_check_timer);
    osTimerStart(xbus_ota_timeout_check_timer, 2000);
#endif

    return 0;
}

static int32_t xbm_cmd_fwu_data_process_handler(uint8_t *cmd_data, uint16_t data_len)
{
    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);
#if defined(__XSPACE_XBUS_OTA__)
    uint8_t send_buf[3] = {1, 0, 0};
    xbus_ota_err_e ret = XBUS_OTA_QTY;
    uint32_t tick = 0;
    static uint32_t cur_time = 0, pre_time = 0;

    /* when ota recv timeout,xbus exit ota mode */
    tick = hal_sys_timer_get();
    cur_time = TICKS_TO_MS(tick);
    if (cur_time > (pre_time + 1000)) {
        osTimerStop(xbus_ota_timeout_check_timer);
        osTimerStart(xbus_ota_timeout_check_timer, 2000);
        pre_time = cur_time;
    }

    ret = xbus_ota_frame_receive(cmd_data, data_len);
    switch (ret) {
        case XBUS_OTA_FRAME_NEXT:
            send_buf[0] = 0x01;
            xbus_manage_send_data(X2BM_CMD_FWU_DATA_PROCESS, send_buf, 3);
            break;
        case XBUS_OTA_FRAME_RETRANS:
            send_buf[0] = 0x00;
            xbus_manage_send_data(X2BM_CMD_FWU_DATA_PROCESS, send_buf, 3);
            break;
        case XBUS_OTA_FINISHED:
            send_buf[0] = 0x01;
            xbus_manage_send_data(X2BM_CMD_FWU_DATA_PROCESS, send_buf, 3);
            hal_xbus_mode_set(XBUS_MODE_OTA_EXIT);
#if defined(__APP_BIN_INTEGRALITY_CHECK__)
            if (!xbus_ota_sha256_check()) {
                xspace_interface_enter_ota_mode();
            } else {
                X2BM_TRACE(0, "xbus ota failed.");
            }
#endif
            //hal_xbus_mode_set(XBUS_MODE_OTA_EXIT);
            break;
        case XBUS_OTA_FAILED:
            X2BM_TRACE(0, "xbus ota failed,no response,waiting for pc tool timeout");
            break;
        default:
            break;
    }
#endif

    return 0;
}

static int32_t xbm_cmd_fwu_get_version_process_handler(uint8_t *cmd_data, uint16_t data_len)
{
    X2BM_TRACE(1, "data_len=%d", data_len);
    //DUMP8("%02X,", cmd_data, data_len);
#if defined(__XSPACE_XBUS_OTA__)
    uint8_t rsp_cmd_data[10] = {0};
    uint16_t rsp_data_len = 0;

    rsp_cmd_data[rsp_data_len++] = 0x01;
    rsp_cmd_data[rsp_data_len++] = 0x01;
    rsp_cmd_data[rsp_data_len++] = 0x02;
    rsp_cmd_data[rsp_data_len++] = 0x03;
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[0];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[1];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[2];
    rsp_cmd_data[rsp_data_len++] = xui_dev_sw_version[3];
    xbus_manage_send_data(X2BM_CMD_GET_VERSION_PROCESS, rsp_cmd_data, rsp_data_len);

#endif

    return 0;
}

static int32_t xbm_cmd_save_flash_info_handler(uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t send_data = 0;

    TRACE(0, "\n--------------X2BM_CMD_SAVE_FLASH_INFO--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);
    send_data = 1;
    xbus_manage_send_data(X2BM_CMD_SAVE_FLASH_INFO, &send_data, 1);

    xspace_interface_write_data_to_flash_befor_shutdown();
#if defined(__XSPACE_FLASH_ACCESS__)
    hw_rst_use_bat_data_flag_write_flash(true);
#endif
    nv_record_flash_flush();
    osDelay(100);

    return 0;
}

//add by liyuan
static int32_t xbm_cmd_get_linkkey_handler(uint8_t *cmd_data, uint16_t data_len)
{
    X2BM_TRACE(1, "data_len=%d", data_len);
    //DUMP8("%02X,", cmd_data, data_len);

    btif_device_record_t record1;
    btif_device_record_t record2;

    uint8_t buf[44] = {0};
    uint8_t tx_len = 0;

    int flag;

    flag = nv_record_enum_latest_two_paired_dev(&record1, &record2);
    if (flag == -1 || flag == 0) {
        TRACE(0, "No BT paired dev.");
        tx_len = 1;
        buf[0] = 0xff;
    } else if (flag == 1) {
        memcpy(buf, record1.bdAddr.address, sizeof(record1.bdAddr.address));
        tx_len += sizeof(record1.bdAddr.address);
        memcpy(buf + tx_len, record1.linkKey, sizeof(record1.linkKey));
        tx_len += sizeof(record1.linkKey);
    } else if (flag == 2) {
        //device 1
        memcpy(buf, record1.bdAddr.address, sizeof(record1.bdAddr.address));
        tx_len += sizeof(record1.bdAddr.address);
        memcpy(buf + tx_len, record1.linkKey, sizeof(record1.linkKey));
        tx_len += sizeof(record1.linkKey);
        //device 2
        memcpy(buf + tx_len, record2.bdAddr.address, sizeof(record2.bdAddr.address));
        tx_len += sizeof(record2.bdAddr.address);
        memcpy(buf + tx_len, record2.linkKey, sizeof(record2.linkKey));
        tx_len += sizeof(record2.linkKey);
    } else {
        TRACE(0, "paired_dev >2.");
        tx_len = 2;
        buf[0] = 0x03;
        buf[1] = 0xff;
    }
    xbus_manage_send_data(X2BM_CMD_GET_LINKKEY, buf, tx_len);

    return 0;
}

#ifdef __XSPACE_TRACE_DISABLE__
static int32_t xbm_cmd_trace_switch_handler(uint8_t *cmd_data, uint16_t data_len)
{
    extern bool trace_enable;
    uint8_t send_data = 0;
    uint8_t flash_update = 0;
    TRACE(0, "\n--------------XBUS_CMD_TRACE_SWITCH--------------\n");

    X2BM_TRACE(1, "data_len=%d", data_len);
    DUMP8("%02X,", cmd_data, data_len);

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    if (data_len == 0) {
        if (nvrecord_env->trace_enable == 1)
            send_data = 1;
        else
            send_data = 0;
    } else if ((data_len == 1) && ((*cmd_data == 0) || (*cmd_data == 1))) {
        trace_enable = *cmd_data;

        nvrecord_env->trace_enable = trace_enable;
        flash_update = 1;

        send_data = 1;
    } else {
        send_data = 0xFF;
    }

    xbus_manage_send_data(X2BM_CMD_TRACE_SWITCH, &send_data, 1);
    osDelay(20);

    if (flash_update) {
        nv_record_env_set(nvrecord_env);
        nv_record_flash_flush();
    }
    return 0;
}

#endif

static const x2bm_cmd_config_s c_x2bm_cmd_cfg[] = {
    {X2BM_CMD_TWS_PAIRING, (uint8_t *)"TWS Pairing", xbm_cmd_tws_pair_handler},
    {X2BM_CMD_QUERY_PAIRING_STATUS, (uint8_t *)"Query Pairing Status", xbm_cmd_query_pairing_status_handler},
    {X2BM_CMD_ENTER_OTA_MODE, (uint8_t *)"Enter OTA Mode", xbm_cmd_enter_ota_mode_handler},
    {X2BM_CMD_RESTORE_FACTORY_SETTING, (uint8_t *)"Restore Factory Setting", xbm_cmd_restore_factory_setting_handler},
    {X2BM_CMD_CTRL_EAR_POWEROFF, (uint8_t *)"Ctrl Earphone Poweroff", xbm_cmd_ctrl_ear_poweroff_handler},
    {X2BM_CMD_QUERY_EXCHANGE_INFO, (uint8_t *)"Query Exchange Info", xbm_cmd_query_exchange_info_handler},
    {X2BM_CMD_HW_REBOOT, (uint8_t *)"Hardware Reboot", xbm_cmd_hw_reboot_handler},
    {X2BM_CMD_STOP_PARING, (uint8_t *)"Stop Pairing", xbm_cmd_stop_pairing_handler},
    {X2BM_CMD_SINGLE_PAIRING, (uint8_t *)"Single Pairing", xbm_cmd_single_pairing_handler},
    {X2BM_CMD_INBOX_START_OTA, (uint8_t *)"Inbox auto enter ota mode", xbm_cmd_inbox_auto_enter_ota_handler},
    {X2BM_CMD_START_XBUS_FWU, (uint8_t *)"Start XBus Firmware Update", xbm_cmd_start_xbus_fwu_handler},
    {X2BM_CMD_FWU_DATA_PROCESS, (uint8_t *)"XBus firmware data process", xbm_cmd_fwu_data_process_handler},
    {X2BM_CMD_GET_VERSION_PROCESS, (uint8_t *)"XBus get version process", xbm_cmd_fwu_get_version_process_handler},
    {X2BM_CMD_SAVE_FLASH_INFO, (uint8_t *)"Save Flash Info", xbm_cmd_save_flash_info_handler},
    {X2BM_CMD_HW_SHIPMODE, (uint8_t *)"entry shipmode", xbm_cmd_entry_shipmode_handler},
    {X2BM_CMD_GET_LINKKEY, (uint8_t *)"get linkkey", xbm_cmd_get_linkkey_handler},
#ifdef __XSPACE_TRACE_DISABLE__
    {X2BM_CMD_TRACE_SWITCH, (uint8_t *)"trace switch", xbm_cmd_trace_switch_handler},
#endif
};

int32_t xbus_manage_cmd_find_and_exec_handler(uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    x2bm_cmd_e xbm_cmd = (x2bm_cmd_e)cmd;

    for (uint16_t i = 0; i < sizeof(c_x2bm_cmd_cfg) / sizeof(c_x2bm_cmd_cfg[0]); i++) {
        if (xbm_cmd == c_x2bm_cmd_cfg[i].cmd) {
            if (c_x2bm_cmd_cfg[i].cmd_handler != NULL) {
                c_x2bm_cmd_cfg[i].cmd_handler(cmd_data, data_len);
                return 0;
            } else {
                X2BM_TRACE(1, "the handler of the cmd(%02X) is NULL.", xbm_cmd);
                return 1;
            }
        }
    }

    X2BM_TRACE(1, "Not found the cmd(%02X)", xbm_cmd);
    return 2;
}

#if defined(__XSPACE_AUTO_TEST__)
extern int32_t xbm_cmd_tws_pair_handler_for_auto_test(uint8_t *cmd_data, uint16_t data_len)
{
    return xbm_cmd_tws_pair_handler(cmd_data, data_len);
}
#endif

#endif   // (__XSPACE_XBUS_MANAGER__)
