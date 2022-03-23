#if defined(__IMU_TDK42607P__)
#include "tdk42607p_adapter.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "dev_thread.h"
#include "hal_gpio.h"
#include "hal_imu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "tgt_hardware.h"
#include "xspace_i2c_master.h"

/* --------------------------------------------------------------------------------------
 * Note(Mike): add TDK42607 Drivers/utils Header file
 */
#include "example_raw.h"
#include "Message.h"
#include "RingBuffer.h"
#include "InvError.h"
#include "inv_imu_defs.h"
#include "ErrorHelper.h"
#include <stdio.h>

/* 
 * Define msg level 
 */
#define MSG_LEVEL INV_MSG_LEVEL_DEBUG

/* --------------------------------------------------------------------------------------
 *  Global variables 
 * -------------------------------------------------------------------------------------- */

#if !USE_FIFO
/* 
	 * Buffer to keep track of the timestamp when IMU data ready interrupt fires.
	 * The buffer can contain up to 64 items in order to store one timestamp for each packet in FIFO.
	 */
RINGBUFFER(timestamp_buffer, 64, uint64_t);
#endif

/* --------------------------------------------------------------------------------------
 *  Forward declaration
 * -------------------------------------------------------------------------------------- */
static int32_t tdk42607_adapter_init(void);
static int32_t tdk42607_adapter_get_chip_id(uint32_t *chip_id);
static void tdk42607_adapter_event_process(uint32_t msgid, uint32_t *ptr, uint32_t param0, uint32_t param1);
static int tdk42607_adapter_handle_process(dev_message_body_s *msg_body);
static void tdk42607_adapter_int_handler(enum HAL_GPIO_PIN_T pin);
static void tdk42607_adapter_int_init(void);
static int tdk42607_adapter_i2c_read(struct inv_imu_serif *serif, uint8_t reg, uint8_t *buf, uint32_t len);
static int tdk42607_adapter_i2c_write(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *buf, uint32_t len);
static int tdk42607_adapter_icm_serif_init(struct inv_imu_serif *serif);
static void tdk42607_adapter_check_rc(int rc, const char *msg_context);
static void tdk42607_adapter_set_int_irq_status(bool enable);
static struct HAL_GPIO_IRQ_CFG_T tdk42607_irq_cfg;
void msg_printer(int level, const char *str, va_list ap);

/* --------------------------------------------------------------------------------------
 * Object Func Map 
 * -------------------------------------------------------------------------------------- */
const hal_imu_ctx_s tdk42607p_ctrl = {
    tdk42607_adapter_init, tdk42607_adapter_get_chip_id,
    // TODO(Mike):add more hal api here
};

/* --------------------------------------------------------------------------------------
 * Func Define 
 * -------------------------------------------------------------------------------------- */
static int32_t tdk42607_adapter_init(void)
{
    TDK42607P_TRACE(0," -- IN");
    int rc = 0;
    dev_thread_handle_set(DEV_MODUAL_IMU, tdk42607_adapter_handle_process);
    struct inv_imu_serif icm_serif;

    /* Setup message facility to see internal traces from FW */
    INV_MSG_SETUP(MSG_LEVEL, msg_printer);
    INV_MSG(INV_MSG_LEVEL_INFO, "######### tdk42607_adapter_init in #############");

    /*
    * Configure input capture mode GPIO connected to pin EXT3-9 (pin PB03).
    * This pin is connected to Icm406xx INT1 output and thus will receive interrupts 
    * enabled on INT1 from the device.
    * A callback function is also passed that will be executed each time an interrupt
    * fires.
	*/
    tdk42607_adapter_int_init();

    /* 
    * Initialize serial interface between MCU and IMU 
    */
    icm_serif.context = 0; /* no need */
    icm_serif.read_reg = tdk42607_adapter_i2c_read;
    icm_serif.write_reg = tdk42607_adapter_i2c_write;
    icm_serif.max_read = 1024 * 32;  /* maximum number of bytes allowed per serial read */
    icm_serif.max_write = 1024 * 32; /* maximum number of bytes allowed per serial write */
    icm_serif.serif_type = SERIF_TYPE;

    /* 
    * Initialize serial interface between MCU and IMU 
    */
    rc |= tdk42607_adapter_icm_serif_init(&icm_serif);

    /* 
    * Initialize and configure tdk42607p device
    */
    rc |= setup_imu_device(&icm_serif);
    rc |= configure_imu_device();
    tdk42607_adapter_check_rc(rc, "error during initialization");
    INV_MSG(INV_MSG_LEVEL_INFO, "IMU device successfully initialized");
    //tdk42607_adapter_set_int_irq_status(true);
    TDK42607P_TRACE(0," -- OUT");
    return 0;
}

static int32_t tdk42607_adapter_get_chip_id(uint32_t *chip_id)
{
    //*chip_id = tdk42607_adapter_get_version();
    *chip_id = 0xFFFF;
    // TODO(Mike):

    return 0;
}

static int32_t tdk42607_adapter_icm_serif_init(struct inv_imu_serif *serif)
{
    switch (serif->serif_type) {
        case UI_SPI3:
        case UI_SPI4:
            //TODO(Mike): if use spi serial interface, plz adpter it
            return INV_ERROR_HW;
        case UI_I2C:
            if (!xspace_get_i2c_init_status()) {
                xspace_i2c_init();
            }
            return INV_ERROR_SUCCESS;
        default:
            return INV_ERROR_HW;
    }
    return INV_ERROR_NIMPL;
}

static void tdk42607_adapter_check_rc(int rc, const char *msg_context)
{
    if (rc < 0) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "%s: error %d (%s)\r\n", msg_context, rc, inv_error_str(rc));
        while (1);
    }
}

static int tdk42607_adapter_i2c_read(struct inv_imu_serif *serif, uint8_t reg, uint8_t *buf, uint32_t len)
{
    INV_MSG(INV_MSG_LEVEL_INFO, "tdk42607_adapter_i2c_read\r\n");
    switch (serif->serif_type) {
        case UI_SPI3:
        case UI_SPI4:
            //TODO(Mike): if use spi serial interface, plz adpter it
            return INV_ERROR_HW;
        case UI_I2C:
            if (xspace_i2c_burst_read(XSPACE_HW_I2C, ICM42607P_ADDRESS, reg, buf, len)) {           
                return INV_ERROR_SUCCESS;
            } else {
                return INV_ERROR_TRANSPORT;
            }

        default:
            return INV_ERROR_HW;
    }
    return INV_ERROR_NIMPL;
}

static int tdk42607_adapter_i2c_write(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *buf, uint32_t len)
{
    switch (serif->serif_type) {
        case UI_SPI3:
        case UI_SPI4:
            //TODO(Mike): if use spi serial interface, plz adpter it
            return INV_ERROR_HW;
        case UI_I2C:
            if (xspace_i2c_burst_write(XSPACE_HW_I2C, ICM42607P_ADDRESS, reg, buf, len)) {
                return INV_ERROR_SUCCESS;
            } else {
                return INV_ERROR_TRANSPORT;
            }

        default:
            return INV_ERROR_HW;
    }
    return INV_ERROR_NIMPL;
}

static void tdk42607_adapter_event_process(uint32_t msgid, uint32_t *ptr, uint32_t param0, uint32_t param1)
{
#if defined(__DEV_THREAD__)
    dev_message_block_s msg;
    static uint8_t error_time = 0;
    msg.mod_id = DEV_MODUAL_IMU;
    msg.msg_body.message_id = msgid;
    msg.msg_body.message_ptr = (uint32_t *)ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    int status = dev_thread_mailbox_put(&msg);
    if (status != osOK) {
        INV_MSG(INV_MSG_LEVEL_ERROR, " mailbox send error status:0x%x(%d)", status, status);
        if (0xffffffff == status && error_time == 3) {
            ASSERT(0, " %s mailbox send error status:0x%x", __func__, status);
        } else if (0xffffffff == status) {
            error_time++;
            INV_MSG(INV_MSG_LEVEL_ERROR, " %s mailbox send error:%d", __func__, error_time);
        } else {
            error_time = 0;
        }
    }
#endif
}

static int tdk42607_adapter_handle_process(dev_message_body_s *msg_body)
{
    INV_MSG(INV_MSG_LEVEL_DEBUG, " ######### tdk42607_adapter_handle_process -in #############");
    int rc = 0;
    switch (msg_body->message_id) {
        case TDK42607P_MSG_ID_INT:
            rc = get_imu_data();
            tdk42607_adapter_check_rc(rc, "error while getting data");
            break;
        default:
            INV_MSG(INV_MSG_LEVEL_ERROR, "Unkonwn event");
            break;
    }

    tdk42607_adapter_set_int_irq_status(true);
    return rc;
}

static void tdk42607_adapter_int_handler(enum HAL_GPIO_PIN_T pin)
{
    INV_MSG(INV_MSG_LEVEL_DEBUG, " ######### tdk42607_adapter_int_handler -in #############");
    tdk42607_adapter_set_int_irq_status(false);
#if !USE_FIFO
    /* 
	 * Read timestamp from the timer dedicated to timestamping 
	 */
    uint64_t timestamp = inv_imu_get_time_us();
    if (!RINGBUFFER_FULL(&timestamp_buffer)) {
        RINGBUFFER_PUSH(&timestamp_buffer, &timestamp);
    }

#endif
    tdk42607_adapter_event_process(TDK42607P_MSG_ID_INT, 0, 0, 0);
}

static void tdk42607_adapter_int_init(void)
{

    INV_MSG(INV_MSG_LEVEL_INFO, " ######### tdk42607_adapter_int_init -in #############");
    tdk42607_irq_cfg.irq_enable = true;
    tdk42607_irq_cfg.irq_debounce = false;
    tdk42607_irq_cfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;
    tdk42607_irq_cfg.irq_handler = tdk42607_adapter_int_handler;
    tdk42607_irq_cfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&imu_int_status_cfg, 1);
    //set gpio pullup
    hal_iomux_set_io_pull_select(imu_int_status_cfg.pin, HAL_IOMUX_PIN_PULLUP_ENABLE);
    hal_gpio_pin_set_dir(imu_int_status_cfg.pin, HAL_GPIO_DIR_IN, 0);
    hal_gpio_setup_irq(imu_int_status_cfg.pin, &tdk42607_irq_cfg);

    INV_MSG(INV_MSG_LEVEL_INFO, " ######### tdk42607_adapter_int_init -done #############");
}

static void tdk42607_adapter_set_int_irq_status(bool enable)
{
    tdk42607_irq_cfg.irq_enable = enable;
    hal_gpio_setup_irq(imu_int_status_cfg.pin, &tdk42607_irq_cfg);
}

/*
 * Printer function for message facility
 */
void msg_printer(int level, const char *str, va_list ap)
{
    static char out_str[256]; /* static to limit stack usage */
    unsigned idx = 0;
    const char *s[INV_MSG_LEVEL_MAX] = {
        "",                 // INV_MSG_LEVEL_OFF
        " [42607P]-[E] ",   // INV_MSG_LEVEL_ERROR
        " [42607P]-[W] ",   // INV_MSG_LEVEL_WARNING
        " [42607P]-[I] ",   // INV_MSG_LEVEL_INFO
        " [42607P]-[V] ",   // INV_MSG_LEVEL_VERBOSE
        " [42607P]-[D] ",   // INV_MSG_LEVEL_DEBUG
    };

    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "%s", s[level]);

    if (idx >= (sizeof(out_str))) {
        return;
    }
    idx += vsnprintf(&out_str[idx], sizeof(out_str) - idx, str, ap);

    if (idx >= (sizeof(out_str))) {
        return;
    }
    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "\r\n");

    if (idx >= (sizeof(out_str))) {
        return;
    }

    hal_trace_output((unsigned char *)out_str, idx);
}

/* --------------------------------------------------------------------------------------
 *  Extern functions definition
 * -------------------------------------------------------------------------------------- */

/* Sleep implementation */
void inv_imu_sleep_us(uint32_t us)
{
    hal_sys_timer_delay_us(us);
}

/* Get time implementation */
uint64_t inv_imu_get_time_us(void)
{
    return TICKS_TO_US(GET_CURRENT_TICKS());
}

#endif /* __CHARGER_TDK42607P__ */
