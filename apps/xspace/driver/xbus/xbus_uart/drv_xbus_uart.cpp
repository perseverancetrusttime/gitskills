#if defined(__XBUS_UART_SUPPORT__)
#include "drv_xbus_uart.h"
#include "hal_xbus.h"
#include "app_thread.h"

#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "tgt_hardware.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_uart.h"
#include "app_utils.h"
#include "hal_sleep.h"
#include "hal_iomux_best1501.h"
#include "pmu_best1501.h"

#if defined(__XSPACE_PRODUCT_TEST__)
#include "xspace_pt_cmd_analyze.h"
#endif

#if defined(__XBUS_UART_DEBUG__)
#define DRV_XBUS_TRACE(num, str, ...) TRACE(num + 1, "[DRV_XBUS] line=%d," str, __LINE__, ##__VA_ARGS__)
#define DRV_XBUS_DUMP8(str, buf, cnt) DUMP8(str, buf, cnt)
#else
#define DRV_XBUS_TRACE(num, str, ...)
#define DRV_XBUS_DUMP8(str, buf, cnt)
#endif

#define XBUS_UART_TX_BAUD  125000
#define XBUS_UART_RX_BAUD  125000
#define XBUS_UART_OTA_BAUD 921600

#define XBUS_BUF_SIZE      (2 * 1024 + 8)
#define XBUS_FRAME_LEN_MIN (4)

typedef enum {
    FRAME_HEAD_RX_L = 0x24,
    FRAME_HEAD_RX_R = 0x25,
    FRAME_HEAD_TX_L = 0x42,
    FRAME_HEAD_TX_R = 0x52,
} xbus_frame_head_e;

typedef enum {
    XBUS_FRAME_HEAD,
    XBUS_FRAME_CMD,
    XBUS_FRAME_DATA_LEN,
    XBUS_FRAME_DATA,
    XBUS_FRAME_CRC16,
    XBUS_FRAME_END,

    XBUS_FRAME_QTY,
} xbus_frame_format_e;

typedef struct {
    uint8_t frame_head;
    uint8_t cmd;
    uint8_t data_len;
    uint8_t data[32];
    uint16_t crc16;
} xbus_uart_frame_s;

typedef enum { XBUS_UART_RX, XBUS_UART_TX, XBUS_UART_QTY } xbus_uart_dir_e;

typedef struct {
    bool inited;

    bool tx_dma_working;
    bool rx_dma_working;
    hal_xbus_mode_e xbus_mode;

    uint8_t buf[XBUS_BUF_SIZE];
    uint8_t buf_cache[XBUS_BUF_SIZE];
    uint32_t buf_size;
    uint32_t buf_data_len;

    uint32_t frame_len_min;
    xbus_uart_frame_s frame_rx;
    xbus_uart_frame_s frame_tx;

    enum HAL_UART_ID_T uart_port;
    uint32_t uart_tx_baud;
    uint32_t uart_rx_baud;
} xbus_comm_cfg_s;

static xbus_comm_cfg_s xbus_comm_cfg = {
    false,

    false,
    false,
    XBUS_MODE_CHARGING,

    {0},
    {0},
    0,
    XBUS_BUF_SIZE,
    XBUS_FRAME_LEN_MIN,
    {
        0,
        0,
        0,
        {0},
        0,
    },
    {
        0,
        0,
        0,
        {0},
        0,
    },

    HAL_UART_ID_1,
    XBUS_UART_TX_BAUD,
    XBUS_UART_RX_BAUD,
};

static struct HAL_UART_CFG_T xbus_uart_cfg = {HAL_UART_PARITY_NONE,
                                              HAL_UART_STOP_BITS_1,
                                              HAL_UART_DATA_BITS_8,
                                              HAL_UART_FLOW_CONTROL_NONE,
                                              HAL_UART_FIFO_LEVEL_1_2,
                                              HAL_UART_FIFO_LEVEL_1_8,
                                              XBUS_UART_RX_BAUD,
                                              false,
                                              false,
                                              false};

static int32_t drv_xbus_uart_mode_set(hal_xbus_mode_e mode);
static int32_t drv_xbus_uart_tx(uint8_t *buf, uint16_t length);
static void drv_xbus_uart_dir_set(xbus_uart_dir_e dir);

static hal_xbus_rx_cb drv_xbus_uart_rx_cb = NULL;

static osTimerId xbus_mode_check_timer = NULL;
static void drv_vbus_uart_mode_check_timeout_handler(void const *param);
osTimerDef(XBUS_MODE_CHECK_TIMER, drv_vbus_uart_mode_check_timeout_handler);

static void drv_vbus_uart_mode_check_timeout_handler(void const *param)
{
#if defined(__XSPACE_PRODUCT_TEST__)
    if (!xpt_get_mode_status()) {
        drv_xbus_uart_mode_set(XBUS_MODE_OUT_BOX);
    }
#else
    drv_xbus_uart_mode_set(XBUS_MODE_OUT_BOX);
#endif
}

static int drv_xbus_handle_process(APP_MESSAGE_BODY *msg_body)
{
    osTimerStop(xbus_mode_check_timer);
    osTimerStart(xbus_mode_check_timer, 60000);

    return 0;
}

static void drv_xbus_event_process(void)
{
    APP_MESSAGE_BLOCK msg = {0};
    msg.mod_id = APP_MODUAL_XBUS_UART;

    app_mailbox_put(&msg);

    return;
}

static void drv_xbus_uart1_rx_set(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart[] =
    {
        {HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_MCU_UART1_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_GPIO,         HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    };

    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_uart[0].pin, HAL_GPIO_DIR_IN, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_uart[1].pin, HAL_GPIO_DIR_IN, 1);

    hal_iomux_init(pinmux_uart, ARRAY_SIZE(pinmux_uart));

#ifndef UART_HALF_DUPLEX
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
#endif
}

static void drv_xbus_uart1_tx_set(void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart[] =
    {
        {HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_GPIO,         HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
        {HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_MCU_UART1_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
    };

    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_uart[0].pin, HAL_GPIO_DIR_IN, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_uart[1].pin, HAL_GPIO_DIR_IN, 1);

    hal_iomux_init(pinmux_uart, ARRAY_SIZE(pinmux_uart));
}

static void drv_xbus_uart_dir_set(xbus_uart_dir_e dir)
{
    if (XBUS_UART_QTY > dir) {
        switch (dir) {
            case XBUS_UART_RX:
                drv_xbus_uart1_rx_set();
                break;
            case XBUS_UART_TX:
                drv_xbus_uart1_tx_set();
                break;
            default:
                break;
        }
    } else {
        DRV_XBUS_TRACE(0, "arg err!");
    }

    return;
}

static void drv_xbus_uart_rx_dma_stop(void)
{
    union HAL_UART_IRQ_T mask;

    uint32_t lock = int_lock();
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
    mask.reg = 0;
    hal_uart_irq_set_mask(xbus_comm_cfg.uart_port, mask);
    hal_uart_stop_dma_recv(xbus_comm_cfg.uart_port);
    int_unlock(lock);

    return;
}

static void drv_xbus_uart_rx_dma_start(void)
{
    union HAL_UART_IRQ_T mask;

    DRV_XBUS_TRACE(0, "rx_dma_start");
    uint32_t lock = int_lock();
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
    mask.reg = 0;
    mask.RT = 1;
    mask.RX = 1;
    mask.TX = 1;
    mask.BE = 1;
    hal_uart_dma_recv_mask(xbus_comm_cfg.uart_port, (unsigned char *)&xbus_comm_cfg.buf[0], XBUS_BUF_SIZE, NULL, NULL, &mask);
    int_unlock(lock);

    return;
}

static void drv_xbus_uart_rx_dma_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    DRV_XBUS_TRACE(1, "dma rx size:%d", xfer_size);
    if (dma_error || status.FE || status.PE || status.BE || status.OE) {
        drv_xbus_uart_rx_dma_stop();
        DRV_XBUS_TRACE(5, "dma rx,status:dma:%d, FE:%d, PE:%d, BE:%d, OE:%d", dma_error, status.FE, status.PE, status.BE, status.OE);
        drv_xbus_uart_rx_dma_start();

        return;
    }

    if (xfer_size > xbus_comm_cfg.frame_len_min) {
        DRV_XBUS_TRACE(1, "[MODE:%s][XBUS RX CMD:]", XBUS_MODE_COMMUNICATON == xbus_comm_cfg.xbus_mode ? "IRQ" : "DMA");
        DRV_XBUS_DUMP8("0x%02x ", xbus_comm_cfg.buf_cache, xfer_size > 20 ? 20 : xfer_size);
        if (drv_xbus_uart_rx_cb != NULL) {
            memcpy(xbus_comm_cfg.buf_cache, xbus_comm_cfg.buf, XBUS_BUF_SIZE);
            drv_xbus_uart_rx_cb(xbus_comm_cfg.buf_cache, xfer_size);
        } else {
            DRV_XBUS_TRACE(0, "null pointer err!");
        }
    } else {
        drv_xbus_uart_rx_dma_stop();
        drv_xbus_uart_rx_dma_start();
        memset((void *)xbus_comm_cfg.buf, 0, sizeof(xbus_comm_cfg.buf) / sizeof(xbus_comm_cfg.buf[0]));
        DRV_XBUS_TRACE(1, "dma rx len < %d ", xbus_comm_cfg.frame_len_min);
    }

    return;
}

static void drv_xbus_uart_tx_dma_handler(uint32_t xfer_size, int dma_error)
{
    DRV_XBUS_TRACE(2, "xfer_size:%d, dma_error:%d", xfer_size, dma_error);

    return;
}

#if 0
static void drv_xbus_uart_dma_break_handler(void)
{
    union HAL_UART_IRQ_T mask;

    DRV_XBUS_TRACE(0, "UART-BREAK");
    mask.reg = 0;
    hal_uart_irq_set_mask(xbus_comm_cfg.uart_port, mask);
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
}
#endif

static void drv_xbus_uart_irq_rx_start(void)
{
    union HAL_UART_IRQ_T mask;

    uint32_t lock = int_lock();
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
    mask.reg = 0;
    mask.BE = 1;
    mask.RT = 1;
    mask.RX = 1;
    hal_uart_irq_set_mask(xbus_comm_cfg.uart_port, mask);
    int_unlock(lock);
}

static void drv_xbus_uart_irq_rx_stop(void)
{
    union HAL_UART_IRQ_T mask;

    uint32_t lock = int_lock();
    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
    mask.reg = 0;
    hal_uart_irq_set_mask(xbus_comm_cfg.uart_port, mask);
    int_unlock(lock);
}

#if 0
static void drv_xbus_uart_gpio_irq_handler(enum HAL_GPIO_PIN_T pin)
{
	DRV_XBUS_TRACE(0, "xbus rx data coming!");

    drv_xbus_uart_dir_set(XBUS_UART_RX);

    if(XBUS_MODE_OTA != xbus_comm_cfg.xbus_mode){
        drv_xbus_uart_mode_set(XBUS_MODE_COMMUNICATON);
        //drv_xbus_uart_mode_set(XBUS_MODE_OTA);
    }

    return;
}

static void drv_xbus_uart_gpio_irq_disable(void)
{
	// disable gpioint
	struct HAL_GPIO_IRQ_CFG_T gpiocfg;
	gpiocfg.irq_enable = false;
	hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)xbus_uart_tx_cfg.pin, &gpiocfg);
}

static void drv_xbus_uart_gpio_irq_init(void){
{
	//enable gpioint
	struct HAL_GPIO_IRQ_CFG_T gpiocfg;

	gpiocfg.irq_enable = true;
	gpiocfg.irq_debounce = true;
	gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
	gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
	gpiocfg.irq_handler = drv_xbus_uart_gpio_irq_handler;
	hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&xbus_uart_tx_cfg, 1);
	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)xbus_uart_tx_cfg.pin, HAL_GPIO_DIR_IN, 0);
	hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)xbus_uart_tx_cfg.pin, &gpiocfg);
}
#endif

static void drv_xbus_uart_rx_irq_handler(enum HAL_UART_ID_T id, union HAL_UART_IRQ_T status)
{
    static uint32_t pre_time = 0;
    uint32_t cur_time = 0;
    uint8_t rev_char = 0;
    uint32_t head_index = 0;

    DRV_XBUS_TRACE(0, "rx_irq_handler enter!");
    DRV_XBUS_TRACE(7, "xbus rx,status RT:%d, RX:%d, TX:%d, FE:%d, PE:%d, BE:%d, OE:%d", status.RT, status.RX, status.TX, status.FE, status.PE, status.BE, status.OE);


    if (status.RX || status.RT) {
        while (hal_uart_readable(xbus_comm_cfg.uart_port)) {
            cur_time = hal_sys_timer_get();
            if (TICKS_TO_MS(cur_time - pre_time) > 20) {
                memset(xbus_comm_cfg.buf, 0, xbus_comm_cfg.buf_size);
                xbus_comm_cfg.buf_data_len = 0;
                DRV_XBUS_TRACE(1, "time slot:%d ms", TICKS_TO_MS(cur_time - pre_time));
            }
            pre_time = cur_time;
            rev_char = hal_uart_getc(xbus_comm_cfg.uart_port);

            /*delay for fifo write ready*/
            hal_sys_timer_delay_us(50);

            xbus_comm_cfg.buf[xbus_comm_cfg.buf_data_len++] = rev_char;
        }

/* debug perpose*/
#if 0
        for(uint32_t i = 0; i < xbus_comm_cfg.buf_data_len; i++) {
            DRV_XBUS_TRACE(3, "recv char:0x%02X, len:%d", xbus_comm_cfg.buf[i], i);
        }
#endif
        DRV_XBUS_TRACE(1, "[MODE:%s][XBUS RX CMD]:", XBUS_MODE_COMMUNICATON == xbus_comm_cfg.xbus_mode ? "IRQ" : "DMA");
        DRV_XBUS_DUMP8("0x%02X.", xbus_comm_cfg.buf, xbus_comm_cfg.buf_data_len);

        for (head_index = 0; head_index < xbus_comm_cfg.buf_data_len; head_index++) {
            if (xbus_comm_cfg.buf[head_index] == xbus_comm_cfg.frame_rx.frame_head) {
                xbus_comm_cfg.buf_data_len = xbus_comm_cfg.buf[head_index + XBUS_FRAME_DATA_LEN] + 5u;
                memcpy(xbus_comm_cfg.buf_cache, &xbus_comm_cfg.buf[head_index], xbus_comm_cfg.buf_data_len);
                if (NULL != drv_xbus_uart_rx_cb) {
                    DRV_XBUS_TRACE(1, "line: %d", __LINE__);
                    drv_xbus_uart_rx_cb(xbus_comm_cfg.buf_cache, xbus_comm_cfg.buf_data_len);
                }
                break;
            }
        }

        memset(xbus_comm_cfg.buf, 0, xbus_comm_cfg.buf_size);
        xbus_comm_cfg.buf_data_len = 0;
        hal_uart_flush(xbus_comm_cfg.uart_port, 4);
    } else {
        DRV_XBUS_TRACE(6, "xbus rx,status RX:%d, TX:%d, FE:%d, PE:%d, BE:%d, OE:%d", status.RX, status.TX, status.FE, status.PE, status.BE, status.OE);
    }
}

static void drv_xbus_uart_baud_set(uint32_t baudrate)
{
    xbus_uart_cfg.baud = baudrate;
    hal_uart_close(xbus_comm_cfg.uart_port);
    hal_uart_open(xbus_comm_cfg.uart_port, &xbus_uart_cfg);

    return;
}

static int32_t drv_xbus_uart_mode_set(hal_xbus_mode_e mode)
{

    if (XBUS_MODE_QTY > mode) {
        DRV_XBUS_TRACE(1, "xbus mode set:%d(0:unkwn, 1:chgr ,2:commun,3:ota,4:ota_exit,5:out_box)", mode);
        if (mode == xbus_comm_cfg.xbus_mode) {
            return 0;
        }

        if (XBUS_MODE_OTA == xbus_comm_cfg.xbus_mode) {
            if (XBUS_MODE_OTA_EXIT != mode) {
                return 0;
            }
        }

        xbus_comm_cfg.xbus_mode = mode;
        switch (xbus_comm_cfg.xbus_mode) {
            case XBUS_MODE_COMMUNICATON:
                app_sysfreq_req(APP_SYSFREQ_USER_XBUS, APP_SYSFREQ_26M);
                drv_xbus_uart_dir_set(XBUS_UART_RX);
                drv_xbus_uart_rx_dma_stop();
                //drv_xbus_uart_gpio_irq_disable();

                xbus_uart_cfg.dma_rx = false;
                xbus_uart_cfg.dma_tx = false;
                drv_xbus_uart_baud_set(XBUS_UART_RX_BAUD);

                hal_uart_irq_set_handler(xbus_comm_cfg.uart_port, drv_xbus_uart_rx_irq_handler);
                drv_xbus_uart_irq_rx_start();
                drv_xbus_event_process();
                break;

            case XBUS_MODE_OTA:
                app_sysfreq_req(APP_SYSFREQ_USER_XBUS, APP_SYSFREQ_104M);

                //drv_xbus_uart_gpio_irq_disable();
                drv_xbus_uart_irq_rx_stop();
                osTimerStop(xbus_mode_check_timer);

                xbus_uart_cfg.dma_rx = true;
                xbus_uart_cfg.dma_tx = true;
                drv_xbus_uart_baud_set(XBUS_UART_OTA_BAUD);
                hal_uart_irq_set_dma_handler(xbus_comm_cfg.uart_port, drv_xbus_uart_rx_dma_handler, drv_xbus_uart_tx_dma_handler);
                drv_xbus_uart_rx_dma_start();
                break;

            case XBUS_MODE_OTA_EXIT:
            case XBUS_MODE_OUT_BOX:
            case XBUS_MODE_CHARGING:
                osTimerStop(xbus_mode_check_timer);

                hal_uart_irq_set_dma_handler(xbus_comm_cfg.uart_port, NULL, NULL);
                hal_uart_irq_set_handler(xbus_comm_cfg.uart_port, NULL);
                drv_xbus_uart_irq_rx_stop();
                drv_xbus_uart_rx_dma_stop();

                //drv_xbus_uart_gpio_irq_init();
                app_sysfreq_req(APP_SYSFREQ_USER_XBUS, APP_SYSFREQ_32K);
                break;

            default:
                break;
        }
    } else {
        DRV_XBUS_TRACE(1, "set xbus mode err mode:%d!", mode);

        return -1;
    }

    return 0;
}

static int32_t drv_xbus_uart_mode_get(hal_xbus_mode_e *mode)
{
    if (NULL != mode) {
        *mode = xbus_comm_cfg.xbus_mode;
        DRV_XBUS_TRACE(1, "xbus mode set:%d(0:unkwn, 1:chgr ,2:commun,3:ota,4:ota_exit,5:out_box)", *mode);
    } else {
        DRV_XBUS_TRACE(0, "arg err!");

        return -1;
    }

    return 0;
}

static int32_t drv_xbus_uart_tx(uint8_t *buf, uint16_t length)
{
    hal_xbus_mode_e xbus_mode = XBUS_MODE_QTY;
    drv_xbus_uart_mode_get(&xbus_mode);
    if ((XBUS_MODE_COMMUNICATON != xbus_mode) && (XBUS_MODE_OTA != xbus_mode)) {
        DRV_XBUS_TRACE(0, "xbus mode err!");
        return -1;
    }

    if (0 == length || NULL == buf) {
        DRV_XBUS_TRACE(0, "arg err!");
        return -1;
    }
    DRV_XBUS_TRACE(1, "xbus tx len:%d", length);
    drv_xbus_uart_dir_set(XBUS_UART_TX);
    memset((void *)xbus_comm_cfg.buf, 0, XBUS_BUF_SIZE);
    xbus_comm_cfg.buf_data_len = length;
    memcpy((void *)xbus_comm_cfg.buf, (const void *)buf, xbus_comm_cfg.buf_data_len);

    DRV_XBUS_TRACE(1, "[MODE:%s][XBUS TX CMD]:", XBUS_MODE_COMMUNICATON == xbus_comm_cfg.xbus_mode ? "IRQ" : "DMA");
    drv_xbus_uart_irq_rx_stop();
    DRV_XBUS_DUMP8("0x%02X.", (uint8_t *)&xbus_comm_cfg.buf[XBUS_FRAME_HEAD], xbus_comm_cfg.buf_data_len);

    if (XBUS_MODE_OTA == xbus_comm_cfg.xbus_mode) {
        drv_xbus_uart_rx_dma_stop();
        drv_xbus_uart_baud_set(XBUS_UART_OTA_BAUD);

        hal_uart_dma_send(xbus_comm_cfg.uart_port, xbus_comm_cfg.buf, xbus_comm_cfg.buf_data_len, NULL, NULL);
        osDelay(3);

        drv_xbus_uart_baud_set(XBUS_UART_OTA_BAUD);
        drv_xbus_uart_rx_dma_start();
    } else {        /* cumminication mode */
        osDelay(5); /*wariting for box ready,then tx.*/
        drv_xbus_uart_baud_set(xbus_comm_cfg.uart_tx_baud);

        for (uint32_t i = 0; i < xbus_comm_cfg.buf_data_len; i++) {
            hal_uart_blocked_putc(xbus_comm_cfg.uart_port, xbus_comm_cfg.buf[i]);
            //DRV_XBUS_TRACE(2, "send char:0x%02X, len:%d", xbus_comm_cfg.buf[i], i);
        }
        osDelay(3);

        drv_xbus_uart_baud_set(xbus_comm_cfg.uart_rx_baud);
        drv_xbus_uart_irq_rx_start();
    }

    hal_uart_flush(xbus_comm_cfg.uart_port, 0);
    drv_xbus_uart_dir_set(XBUS_UART_RX);
    memset((void *)xbus_comm_cfg.buf, 0, XBUS_BUF_SIZE);

    return 0;
}

static int32_t drv_xbus_uart_rx_cb_set(hal_xbus_rx_cb cb)
{
    if (NULL != cb) {
        drv_xbus_uart_rx_cb = cb;
    } else {
        DRV_XBUS_TRACE(0, "null pointer err!");

        return -1;
    }

    return 0;
}

static int32_t drv_xbus_uart_init(void)
{
    DRV_XBUS_TRACE(0, "init");

    if (xbus_mode_check_timer == NULL) {
        xbus_mode_check_timer = osTimerCreate(osTimer(XBUS_MODE_CHECK_TIMER), osTimerOnce, NULL);
    }

    app_set_threadhandle(APP_MODUAL_XBUS_UART, drv_xbus_handle_process);

    /* xbus communication cfg init */
    xbus_comm_cfg.inited = true;
    if (LEFT_SIDE == app_tws_get_earside()) {
        xbus_comm_cfg.frame_rx.frame_head = FRAME_HEAD_RX_L;
        xbus_comm_cfg.frame_tx.frame_head = FRAME_HEAD_TX_L;

    } else {
        xbus_comm_cfg.frame_rx.frame_head = FRAME_HEAD_RX_R;
        xbus_comm_cfg.frame_tx.frame_head = FRAME_HEAD_TX_R;
    }

    //drv_xbus_uart_gpio_irq_init();
    drv_xbus_uart_mode_set(XBUS_MODE_COMMUNICATON);

    return 0;
}

const hal_xbus_ctrl_s drv_xbus_uart_ctrl = {
    drv_xbus_uart_init, 
    drv_xbus_uart_rx_cb_set, 
    drv_xbus_uart_tx, 
    drv_xbus_uart_mode_set, 
    drv_xbus_uart_mode_get,
};
#endif   //__VBUS_UART_SUPPORT__
