#if defined(__XSPACE_XBUS_MANAGER__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "xspace_xbus_manager.h"
#include "xspace_xbus_cmd_analyze.h"
#include "hal_xbus.h"
#include "hal_pmu.h"
#include "xspace_interface_process.h"
#include "Xspace_interface.h"
#include "xspace_crc16.h"
#include "hal_charger.h"

#if defined(__DEV_THREAD__)
#include "dev_thread.h"
#endif

#include "tgt_hardware.h"

#if defined(__XSPACE_PRODUCT_TEST__)
#include "Xspace_pt_cmd_analyze.h"
#endif

#if defined(__XSPACE_XBUS_OTA__)
#include "xspace_xbus_ota.h"
#endif

#define X2BM_FRAME_HEAD_LEN                 (1+1+1)     //head+cmd+data_len
#define X2BM_FRAME_HEAD_IDX                 (0)
#define X2BM_FRAME_CMD_IDX                  (1)
#define X2BM_FRAME_DLEN_IDX                 (2)
#define X2BM_FRAME_DATA_IDX                 (3)

#define X2BM_TX_FRAME_BUFF_SIZE             (64)

static crc16_model_e crc16_mode = CRC16_MODEL_QTY;

static uint16_t recv_data_len = 0;
static uint8_t *p_recv_data = NULL;
//static osTimerId x2bm_charger_status_debounce_timer = NULL;

xbus_manage_report_ui_event_func xbus_manage_report_ui_handle = NULL;
//static void xbus_manage_charger_status_debounce_timeout_handler(void const *param);
//osTimerDef (X2BM_CHARGER_STATUS_DEBOUNCE_TIMER, xbus_manage_charger_status_debounce_timeout_handler);

#if defined(__XSPACE_XBUS_OTA__)
void xbus_manage_ota_register_cb(xbus_manage_ota_start_handle_cb cb)
{
    if(NULL == cb) {
        X2BM_TRACE(0, "NULL == cb!");
        return;
    }
    xbus_cmd_ota_register_cb(cb);
}
#endif

static void xbus_manage_analyze_recv_data(void)
{
    uint8_t *p_data = p_recv_data;
    uint16_t data_len = recv_data_len;

    if(p_data == NULL) {
        X2BM_TRACE(0, "data == NULL");
        return;
    }

    if(data_len == 0) {
        X2BM_TRACE(0, "data_len == 0");
        return;
    }

    x2bm_frame_s frame_data;
    frame_data.frame_head = p_data[X2BM_FRAME_HEAD_IDX];
    frame_data.cmd = p_data[X2BM_FRAME_CMD_IDX];
    frame_data.data_len = (p_data[X2BM_FRAME_DLEN_IDX]&0xff);
    frame_data.p_data = &p_data[X2BM_FRAME_DATA_IDX];
    //frame_data.data_len = ((data[X2BM_FRAME_DLEN_IDX]&0xff)<<8) | (data[X2BM_FRAME_DLEN_IDX+1]&0xff);

#if defined(__XSPACE_XBUS_OTA__)
    if(X2BM_CMD_FWU_DATA_PROCESS == frame_data.cmd) {
        xbus_manage_cmd_find_and_exec_handler(frame_data.cmd, p_data, data_len);
        return;
    }
#endif

    X2BM_TRACE(0, "data_len=%d", data_len);
    DUMP8("%02X,", p_data, data_len);

    x2bm_frame_head_e rx_head_flag = X2BM_FRAME_HEAD_RX_L;
    if(app_tws_get_earside() == RIGHT_SIDE) {
        rx_head_flag = X2BM_FRAME_HEAD_RX_R;
    }

    if(frame_data.frame_head != rx_head_flag) {
        X2BM_TRACE(3, "frame_head(%02X) != rx_head_flag(%02X)",
                frame_data.frame_head, rx_head_flag);
        return;
    }

    if((frame_data.data_len + X2BM_FRAME_HEAD_LEN + 2) != data_len) {
        X2BM_TRACE(3, "data_len(%d)+head_len(%d)+crc(2) != recv_len(%d)",
                frame_data.data_len, X2BM_FRAME_HEAD_LEN, data_len);
        return;
    }

    uint16_t recv_crc16 =  ((p_data[data_len - 2]&0xff)<<8) | (p_data[data_len - 1]&0xff);
    uint16_t calc_crc16 = xspace_crc16_xmodem(p_data, X2BM_FRAME_HEAD_LEN + frame_data.data_len);
    if(calc_crc16 != recv_crc16) {
        //X2BM_TRACE(2, "calc_crc16(%04X) != recv_crc16(%04X)", calc_crc16, recv_crc16);
        calc_crc16 = xspace_crc16_ccitt_false(p_data, X2BM_FRAME_HEAD_LEN + frame_data.data_len, 0xffff);
        if(calc_crc16 != recv_crc16) {
            X2BM_TRACE(2, "calc_crc16(%04X) != recv_crc16(%04X)", calc_crc16, recv_crc16);
            return;
        } else {
            crc16_mode = CRC16_MODEL_CCITT_FALSE;
        }
    } else {
        crc16_mode = CRC16_MODEL_XMODEM;
    }

    //Function Command
    if(xbus_manage_cmd_find_and_exec_handler(frame_data.cmd, frame_data.p_data, frame_data.data_len) == 0)
        return;

    //Product Test Command
#if defined(__XSPACE_PRODUCT_TEST__)
    xpt_cmd_recv_cmd_from_xbus(frame_data.cmd, frame_data.p_data, frame_data.data_len);
#endif
}

static void xbus_manage_recv_data(uint8_t *data, uint16_t len)
{
    X2BM_TRACE(1, "len=%d", len);
    p_recv_data = data;
    recv_data_len = len;
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xbus_manage_analyze_recv_data, 0, 0, 0);
}

int32_t xbus_manage_send_data(uint8_t cmd, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t frame_buff[X2BM_TX_FRAME_BUFF_SIZE] = {0};
    uint16_t frame_len = 0;
    uint16_t calc_crc = 0;

    if(data_len > X2BM_TX_FRAME_BUFF_SIZE - X2BM_FRAME_HEAD_LEN - 2) {
        X2BM_TRACE(2, "No enough buff store data, data_len(%d) > blank(%d)",
                data_len, X2BM_TX_FRAME_BUFF_SIZE - X2BM_FRAME_HEAD_LEN - 2);
        return 1;
    }

    x2bm_frame_head_e tx_head_flag = X2BM_FRAME_HEAD_TX_L;
    if(app_tws_get_earside() == RIGHT_SIDE) {
        tx_head_flag = X2BM_FRAME_HEAD_TX_R;
    }

    frame_buff[frame_len++] = tx_head_flag;
    frame_buff[frame_len++] = cmd;
//    frame_buff[frame_len++] = (uint8_t)((data_len>>8)&0xff);
    frame_buff[frame_len++] = (uint8_t)(data_len&0xff);
#if defined(__XSPACE_XBUS_OTA__)
    if(X2BM_CMD_FWU_DATA_PROCESS == cmd) {
        frame_buff[2] = 0x01;
    }
#endif

    for(int i = 0; i < data_len; i++) {
        frame_buff[frame_len++] = cmd_data[i];
    }

    if(CRC16_MODEL_CCITT_FALSE == crc16_mode) {
        calc_crc = xspace_crc16_ccitt_false(frame_buff, frame_len, 0xffff);
    } else if(CRC16_MODEL_XMODEM == crc16_mode) {
        calc_crc = xspace_crc16_xmodem(frame_buff, frame_len);
    } else {
        X2BM_TRACE(1,"crc16 model err,model:%d\n", crc16_mode);
    }

    frame_buff[frame_len++] = (uint8_t)((calc_crc>>8)&0xff);
    frame_buff[frame_len++] = (uint8_t)(calc_crc&0xff);

    X2BM_TRACE(1, "frame_len=%d", frame_len);
    if((X2BM_CMD_START_XBUS_FWU != cmd) && (X2BM_CMD_FWU_DATA_PROCESS != cmd) && (X2BM_CMD_GET_VERSION_PROCESS != cmd))
    {
#if defined(__CHARGER_SUPPORT__)    
        hal_charger_trx_mode_set(HAL_CHARGER_TRX_MODE_PP);
#endif
        hal_xbus_tx(frame_buff, frame_len);
#if defined(__CHARGER_SUPPORT__)
        hal_charger_trx_mode_set(HAL_CHARGER_TRX_MODE_OD);
#endif
    }
    else
    {
        hal_xbus_tx(frame_buff, frame_len);
    }

    return 0;
}

/*
static void xbus_manage_charger_status_debounce_timeout_handler(void const *param)
{
    hal_xbus_mode_set(XBUS_MODE_OUT_BOX);
}
*/

static void xbus_manage_charger_status_changed_handle(void)
{
    hal_pmu_status_e pmu_charger_status = hal_pmu_force_get_charger_status();

    X2BM_TRACE(0, "Reset charger status debounce timer.");
    //osTimerStop(x2bm_charger_status_debounce_timer);
    if(HAL_PMU_PLUGIN == pmu_charger_status) {
        hal_xbus_mode_set(XBUS_MODE_CHARGING);
    } else if (HAL_PMU_PLUGOUT == pmu_charger_status) {
        hal_xbus_mode_set(XBUS_MODE_COMMUNICATON);
        //osTimerStart(x2bm_charger_status_debounce_timer, X2BM_CHARGER_PLUGIN_DEBOUNCE_TIMER_DELAY_MS);
    } else {
        X2BM_TRACE(0, " Invaild event!!!");
    }
}

static void xbus_manage_charger_status_changed_cb(hal_pmu_status_e status)
{
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL,
        (uint32_t)xbus_manage_charger_status_changed_handle, 0, 0, 0);
}

static void xbus_manage_config_pmu_irq(bool status)
{
    if (status) {
        X2BM_TRACE(0, " Enable PMU IRQ!!!");
        hal_pmu_add_charger_status_change_cb(xbus_manage_charger_status_changed_cb);
    } else {
        X2BM_TRACE(0, " Disable PMU IRQ!!!");
        hal_pmu_remove_charger_status_change_cb(xbus_manage_charger_status_changed_cb);
    }
}

bool xbus_manage_is_communication_mode(void)
{
    hal_xbus_mode_e mode;

    hal_xbus_mode_get(&mode);
    if (XBUS_MODE_COMMUNICATON != mode) {
        return false;
    }

    return true;
}

void xbus_manage_register_ui_event_cb(xbus_manage_report_ui_event_func cb)
{
    if (NULL == cb)
        return;

    xbus_manage_report_ui_handle = cb;
}

static int32_t xbus_manage_init_if(void)
{
    X2BM_TRACE(0, "ENTER");

/*
    if (x2bm_charger_status_debounce_timer == NULL) {
        x2bm_charger_status_debounce_timer = osTimerCreate (osTimer(X2BM_CHARGER_STATUS_DEBOUNCE_TIMER), osTimerOnce, NULL);
    }
*/
    xbus_manage_config_pmu_irq(true);
    hal_xbus_init();
    hal_xbus_rx_cb_set(xbus_manage_recv_data);

    return 0;
}

int32_t xspace_xbus_manage_init(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xbus_manage_init_if, 0, 0, 0);

    return 0;
}


#if 0   // For Test

uint8_t data[64] = {0};
void xbus_manage_modis_do(void)
{

    uint16_t len = 0;

    data[len++] = X2BM_FRAME_HEAD_RX_L;
    data[len++] = X2BM_CMD_TWS_PAIRING;
    data[len++] = 1;
    data[len++] = 0xA5;

    uint16_t calc_crc16 = xspace_crc16_xmodem(data, len, 0);
    data[len++] = (uint8_t)((calc_crc16>>8)&0xff);
    data[len++] = (uint8_t)(calc_crc16&0xff);

    DUMP8("%02X,", data, len);
    xbus_manage_recv_data(data, len);
}


void xbus_manage_modis(void)
{
    static uint16_t cnt = 0;

    cnt++;
    X2BM_TRACE(1, "cnt=%d", cnt);
    if(cnt == 2) {
        xbus_manage_modis_do();
    }

}

#endif


#endif  // (__XSPACE_XBUS_MANAGER__)

