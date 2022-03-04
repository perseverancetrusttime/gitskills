#ifndef __XSPACE_PT_TRACE_UART_H__
#define __XSPACE_PT_TRACE_UART_H__

#if 1//defined(__XSPACE_PRODUCT_TEST__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define XPT_UART_FRAME_HEAD_LEN      (1+1+1)     //head+cmd+data_len
#define XPT_UART_FRAME_HEAD_IDX      (0)
#define XPT_UART_FRAME_CMD_IDX      (1)
#define XPT_UART_FRAME_DLEN_IDX      (2)
#define XPT_UART_FRAME_DATA_IDX      (3)

#define XPT_UART_RECV_BUFF_SIZE         (256)
#define XPT_UART_SEND_BUFF_SIZE         (64)

typedef enum {
    XPT_UART_FRAME_HEAD_RX_L = 0x24,
    XPT_UART_FRAME_HEAD_RX_R = 0x25,
    XPT_UART_FRAME_HEAD_TX_L = 0x42,
    XPT_UART_FRAME_HEAD_TX_R = 0x52,
    XPT_UART_FRAME_HEAD_RX_L_R = 0x31,
    XPT_UART_FRAME_HEAD_TX_L_R = 0x13,
} xpt_uart_frame_head_e;


typedef struct {
    uint8_t frame_head;
    uint8_t cmd;
//    uint16_t data_len;
    uint8_t data_len;    
    uint8_t *p_data;
    uint16_t crc16;
} xpt_uart_frame_s;

void xpt_trace_uart_send_data(uint8_t cmd, uint8_t *cmd_data, uint16_t data_len);
int xspace_pt_trace_uart_rx_process_cpp(uint8_t *rx_data, uint16_t rx_data_len);
int xspace_pt_trace_uart_rx_process(uint8_t *rx_data, uint16_t rx_data_len);


#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_PRODUCT_TEST__)

#endif  // (__XSPACE_PT_TRACE_UART_H__)

