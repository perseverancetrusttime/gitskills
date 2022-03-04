#if defined(__XSPACE_PRODUCT_TEST__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "tgt_hardware.h"

#include "xspace_pt_cmd_analyze.h"
#include "xspace_pt_trace_uart.h"
#include "xspace_xbus_manager.h"
#include "xspace_interface_process.h"
#include "Xspace_interface.h"

static uint8_t xpt_uart_rx_data[XPT_UART_RECV_BUFF_SIZE];
static uint16_t xpt_uart_rx_len = 0;

extern void xbus_manage_recv_data(uint8_t *data, uint16_t len);


static uint16_t xpt_trace_uart_crc16(const uint8_t *buffer, uint32_t size, uint16_t crc)
{
    if (NULL != buffer && size > 0) {
        while (size--) {
            crc = (crc >> 8) | (crc << 8);
            crc ^= *buffer++;
            crc ^= ((unsigned char) crc) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xFF) << 5;
        }
    }

    return crc;
}


void xpt_trace_uart_send_data(uint8_t cmd, uint8_t *cmd_data, uint16_t data_len)
{
    uint8_t frame_buff[XPT_UART_SEND_BUFF_SIZE] = {0};
    uint16_t frame_len = 0;
    uint8_t *p_rxdata = xpt_uart_rx_data;

    if(data_len > XPT_UART_SEND_BUFF_SIZE - XPT_UART_FRAME_HEAD_LEN - 2) {
        XPT_TRACE(2, "No enough buff store data, data_len(%d) > blank(%d)",
                data_len, XPT_UART_SEND_BUFF_SIZE - XPT_UART_FRAME_HEAD_LEN - 2);
        return;
    }

    xpt_uart_frame_head_e tx_head_flag = XPT_UART_FRAME_HEAD_TX_L;
    if(app_tws_get_earside() == RIGHT_SIDE) {
        tx_head_flag = XPT_UART_FRAME_HEAD_TX_R;
    }

    if(p_rxdata[XPT_UART_FRAME_HEAD_IDX] == XPT_UART_FRAME_HEAD_RX_L_R)
    {
        tx_head_flag = XPT_UART_FRAME_HEAD_TX_L_R;
    }

    frame_buff[frame_len++] = tx_head_flag;
    frame_buff[frame_len++] = cmd;
//    frame_buff[frame_len++] = (uint8_t)((data_len>>8)&0xff);
    frame_buff[frame_len++] = (uint8_t)(data_len&0xff);
    for(int i = 0; i < data_len; i++) {
        frame_buff[frame_len++] = cmd_data[i];
    }

    uint16_t calc_crc = xpt_trace_uart_crc16(frame_buff, frame_len, 0xffff);
    frame_buff[frame_len++] = (uint8_t)((calc_crc>>8)&0xff);
    frame_buff[frame_len++] = (uint8_t)(calc_crc&0xff);

    XPT_TRACE(1, "frame_len=%d", frame_len);
    DUMP8("0x%02X.", frame_buff, frame_len);
    hal_trace_output(frame_buff, frame_len);

    return;
}

static void xpt_trace_uart_analyze_recv_data(void)
{
    uint8_t *p_data = xpt_uart_rx_data;
    uint16_t data_len = xpt_uart_rx_len;

    if(p_data == NULL) {
        XPT_TRACE(0, "data == NULL");
        return;
    }

    if(data_len == 0) {
        XPT_TRACE(0, "data_len == 0");
        return;
    }

    XPT_TRACE(0, "data_len=%d", data_len);

    xpt_uart_frame_s frame_data;
    frame_data.frame_head = p_data[XPT_UART_FRAME_HEAD_IDX];
    frame_data.cmd = p_data[XPT_UART_FRAME_CMD_IDX];
    frame_data.data_len = (p_data[XPT_UART_FRAME_DLEN_IDX]&0xff);
    frame_data.p_data = &p_data[XPT_UART_FRAME_DATA_IDX];
    //frame_data.data_len = ((data[XPT_UART_FRAME_DLEN_IDX]&0xff)<<8) | (data[XPT_UART_FRAME_DLEN_IDX+1]&0xff);

    xpt_uart_frame_head_e rx_head_flag = XPT_UART_FRAME_HEAD_RX_L;
    if(app_tws_get_earside() == RIGHT_SIDE) {
        rx_head_flag = XPT_UART_FRAME_HEAD_RX_R;
    }

    if((frame_data.frame_head != rx_head_flag)&&(frame_data.frame_head != XPT_UART_FRAME_HEAD_RX_L_R)) {
        XPT_TRACE(3, "frame_head(%02X) != rx_head_flag(%02X)",
                frame_data.frame_head, rx_head_flag);
        return;
    }

    if((frame_data.data_len + XPT_UART_FRAME_HEAD_LEN + 2) != data_len) {
        XPT_TRACE(3, "data_len(%d)+head_len(%d)+crc(2) != recv_len(%d)",
                frame_data.data_len, XPT_UART_FRAME_HEAD_LEN, data_len);
        return;
    }

    //calc and compare crc16
    uint16_t calc_crc16 = xpt_trace_uart_crc16(p_data, XPT_UART_FRAME_HEAD_LEN + frame_data.data_len, 0xffff);
    uint16_t recv_crc16 =  ((p_data[data_len - 2]&0xff)<<8) | (p_data[data_len - 1]&0xff);
    if(calc_crc16 != recv_crc16) {
        XPT_TRACE(2, "calc_crc16(%04X) != recv_crc16(%04X)", calc_crc16, recv_crc16);
        return;
    }

	XPT_TRACE(0, "Debug(%d bytes): ", data_len);
    DUMP8("%02X,", p_data, data_len);

    //Product Test Command
    xpt_cmd_recv_cmd_from_trace_uart(frame_data.cmd, frame_data.p_data, frame_data.data_len);
}

extern "C" int xspace_pt_trace_uart_rx_process_cpp(uint8_t *rx_data, uint16_t rx_data_len)
{
    XPT_TRACE(1, "rx_data_len=%d", rx_data_len);

    if(rx_data == NULL) {
        XPT_TRACE(0, "rx_data == NULL");
        return 1;
    }

    if(rx_data_len == 0) {
        XPT_TRACE(0, "rx_data_len == 0");
        return 2;
    }

    if(rx_data_len > XPT_UART_RECV_BUFF_SIZE) {
        XPT_TRACE(2, "rx_data_len(%d) > XPT_UART_RECV_BUFF_SIZE(%d)", rx_data_len, XPT_UART_RECV_BUFF_SIZE);
        return 3;
    }

    memset(xpt_uart_rx_data, 0, XPT_UART_RECV_BUFF_SIZE);
    memcpy(xpt_uart_rx_data, rx_data, rx_data_len);
    xpt_uart_rx_len = rx_data_len;
    xspace_interface_event_process(XIF_EVENT_FUNC_CALL, (uint32_t)xpt_trace_uart_analyze_recv_data, 0, 0, 0);
    return 0;
}


extern "C" int xspace_pt_trace_uart_rx_process(uint8_t *rx_data, uint16_t rx_data_len)
{
    xspace_pt_trace_uart_rx_process_cpp(rx_data, rx_data_len);
    return 0;
}


#endif  // (__XSPACE_PRODUCT_TEST__)

