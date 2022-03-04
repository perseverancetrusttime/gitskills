/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_example_port.c
 *
 * @brief   example code for gh621x
 *
 */

#include "gh621x_example_common.h"
#include "gh621x_example.h"
#include <stdio.h>
#include <string.h>
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_sleep.h"
#include "app_utils.h"
#include "hal_touch.h"
#include "tgt_hardware.h"
#include "xspace_i2c_master.h"
#include "gh621x_adapter.h"
#include "xspace_flash_access.h"
#include "xspace_ram_access.h"
#include "xspace_interface.h"
#include "apps.h"

#define BLE_VERSION_STR ("BLE_UNKNOWN")   //ble version
#define HW_VERSION_STR  ("HW_UNKNOWN")    //host hardware version
#define FW_VERSION_STR  ("FW_UNKNOWN")    //host firmware version

#define GH621X_I2C_TYPE (XSPACE_HW_I2C)
extern void factory_section_original_btaddr_get(uint8_t *btAddr);
extern int app_hfp_siri_voice(bool en);
static char BtMacInfoChar[12] = {0};
static uint8_t gh621x_long_press_cnt = 0;

static void gh621x_start_test_mode_timer_handler(void const *param);
uint8_t g_IsGH621xStartTestMode = 0;
static uint16_t g_IsGH621xStartTestModeCnt = 0;
osTimerDef(GH621X_START_TEST_MODE_TIMER, gh621x_start_test_mode_timer_handler);
static osTimerId gh621x_start_test_mode_timer = NULL;
static void gh621x_start_test_mode_timer_handler(void const *param)
{
    if (g_IsGH621xStartTestModeCnt) {
        g_IsGH621xStartTestModeCnt = 0;
        osTimerStop(gh621x_start_test_mode_timer);
        osTimerStart(gh621x_start_test_mode_timer, 30 * 1000);
    } else {
        hal_cpu_wake_unlock(HAL_CPU_WAKE_LOCK_USER_8);
        app_sysfreq_req(APP_SYSFREQ_USER_TOUCH_UTT, APP_SYSFREQ_32K);
        g_IsGH621xStartTestMode = 0;
        osTimerStop(gh621x_start_test_mode_timer);
    }
}

/* gh621x i2c interface */

/**
 * @fn     void Gh621xModuleHalI2cInit(void)
 *
 * @brief  i2c for gh621x init
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHalI2cInit(void)
{
    // code implement by user
}

/**
 * @fn     GU8 Gh621xModuleHalI2cWrite(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usLength)
 *
 * @brief  i2c for gh621x wrtie
 *
 * @attention   None
 *
 * @param[in]   uchDeviceId				i2c device 7bits addr 	
 * @param[in]   uchWriteBuffer			pointer to write data buffer
 * @param[in]   usLength				len of write data buffer
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error		
 */
GU8 Gh621xModuleHalI2cWrite(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usLength)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
    ret = xspace_i2c_burst_write_01(GH621X_I2C_TYPE, uchDeviceId, (uint8_t *)uchWriteBuffer, usLength);

    return ret;
}

/**
 * @fn     GU8 Gh621xModuleHalI2cRead(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usWriteLength, GU8 uchReadBuffer[], GU16 usReadLength)
 *
 * @brief  i2c for gh621x read
 *
 * @attention   None
 *
 * @param[in]   uchDeviceId				i2c device 7bits addr 	
 * @param[in]   uchWriteBuffer			pointer to write data buffer
 * @param[in]   usWriteLength			len of write data buffer
 * @param[in]   usReadLength			len of read data buffer
 * @param[out]  uchReadBuffer			pointer to read data buffer
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error		
 */
GU8 Gh621xModuleHalI2cRead(GU8 uchDeviceId, const GU8 uchWriteBuffer[], GU16 usWriteLength, GU8 uchReadBuffer[], GU16 usReadLength)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
    ret = xspace_i2c_burst_read_01(GH621X_I2C_TYPE, uchDeviceId, (uint8_t *)uchWriteBuffer, usWriteLength, uchReadBuffer, usReadLength);

    return ret;
}

/* delay */

/**
 * @fn     void Gh621xModuleHalDelayMs(GU16 usMsCnt)
 *
 * @brief  delay ms
 *
 * @attention   None
 *
 * @param[in]   usMsCnt					delay ms count
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalDelayMs(GU16 usMsCnt)
{
    // code implement by user
    //hal_sys_timer_delay(MS_TO_TICKS(usMsCnt));
    osDelay(usMsCnt);
}

/* int */

/**
 * @fn     void Gh621xModuleHalIntHandler(void)
 *
 * @brief  gh621x int handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntHandler(enum HAL_GPIO_PIN_T pin)
{
    //Gh621xModuleIntMsgHandler();
    Gh621xModuleHalIntDetectDisable();
    gh621x_adapter_event_process(GH621X_MSG_ID_INT, 0, 0, 0);
}

static struct HAL_GPIO_IRQ_CFG_T gh621x_irq_cfg;
static void hal_gh621x_int_irq_enable(bool enable)
{
    if (enable == true) {
        gh621x_irq_cfg.irq_enable = true;
    } else {
        gh621x_irq_cfg.irq_enable = false;
    }
    hal_gpio_setup_irq(touch_int_cfg.pin, &gh621x_irq_cfg);
}

/**
 * @fn     void Gh621xModuleHalIntInit(void)
 *
 * @brief  gh621x int pin init
 *
 * @attention   should config as falling edge trigger or low level trigger, gh621x default low pulse ouput
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntInit(void)
{
    // code implement by user
    // must register func Gh621xModuleHalIntHandler as callback
    gh621x_irq_cfg.irq_enable = true;
    gh621x_irq_cfg.irq_debounce = false;
    //	gh621x_irq_cfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
    gh621x_irq_cfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

    gh621x_irq_cfg.irq_handler = Gh621xModuleHalIntHandler;
    gh621x_irq_cfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    //gh621x_irq_cfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&touch_int_cfg, 1);
    //set gpio pullup
    hal_iomux_set_io_pull_select(touch_int_cfg.pin, HAL_IOMUX_PIN_PULLUP_ENABLE);
    hal_gpio_pin_set_dir(touch_int_cfg.pin, HAL_GPIO_DIR_IN, 0);
    hal_gpio_setup_irq(touch_int_cfg.pin, &gh621x_irq_cfg);
    GH621X_TRACE(0, "done");
}

/**
 * @fn     void Gh621xModuleHalIntDetectDisable(void)
 *
 * @brief  gh621x int pin detect disable
 *
 * @attention   couldn't into handler by int trigger
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntDetectDisable(void)
{
    // code implement by user
    hal_gh621x_int_irq_enable(false);
}

/**
 * @fn     void Gh621xModuleHalIntDetectReenable(void)
 *
 * @brief  gh621x int pin detect reenable
 *
 * @attention   could into handler again by int trigger 
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalIntDetectReenable(void)
{
    // code implement by user
    hal_gh621x_int_irq_enable(true);
}

/**
 * @fn     GU8 Gh621xModuleHalGetIntPinStatus(void)
 *
 * @brief  get int pin status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  int pin status
 * @retval  #GH621X_PIN_LEVEL_LOW       reset pin low level
 * @retval  #GH621X_PIN_LEVEL_HIGH      reset pin high level
 */
GU8 Gh621xModuleHalGetIntPinStatus(void)
{
    // code implement by user
    if (hal_gpio_pin_get_val(touch_int_cfg.pin))
        return GH621X_PIN_LEVEL_HIGH;
    else
        return GH621X_PIN_LEVEL_LOW;
}

/* reset pin */

/**
 * @fn     void Gh621xModuleHalResetPinCtrlInit(void)
 *
 * @brief  gh621x reset pin control init, default level high
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinCtrlInit(void)
{
    // code implement by user
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&touch_reset_cfg, 1);
    hal_gpio_pin_set_dir(touch_reset_cfg.pin, HAL_GPIO_DIR_OUT, 1);
}

/**
 * @fn     void Gh621xModuleHalResetPinSetHighLevel(void)
 *
 * @brief  gh621x reset pin set high level
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinSetHighLevel(void)
{
    // code implement by user
    hal_gpio_pin_set(touch_reset_cfg.pin);
}

/**
 * @fn     void Gh621xModuleHalResetPinSetLowLevel(void)
 *
 * @brief  gh621x reset pin set low level
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None	
 */
void Gh621xModuleHalResetPinSetLowLevel(void)
{
    // code implement by user
    hal_gpio_pin_clr(touch_reset_cfg.pin);
}

/**
 * @fn     GU8 Gh621xModuleHalGetResetPinStatus(void)
 *
 * @brief  get reset pin status
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  reset pin status
 * @retval  #GH621X_PIN_LEVEL_LOW       reset pin low level
 * @retval  #GH621X_PIN_LEVEL_HIGH      reset pin high level
 */
GU8 Gh621xModuleHalGetResetPinStatus(void)
{
    // code implement by user
    if (hal_gpio_pin_get_val(touch_reset_cfg.pin))
        return GH621X_PIN_LEVEL_HIGH;
    else
        return GH621X_PIN_LEVEL_LOW;
}

/// gh621x get chipid
uint16_t gh621x_get_chipid(GU8 uchValidI2cAddr)
{
    uint8_t read_reg[2] = {0x00, 0x04};
    uint8_t read_val[2] = {0};

    Gh621xExitLowPowerMode();
    Gh621xModuleHalI2cRead(uchValidI2cAddr, read_reg, 2, read_val, 2);
    drv_gh621x_chipid = read_val[1] << 8 | read_val[0];
    GH621X_TRACE(1, "chipid:0x%x", drv_gh621x_chipid);   //chipid 0x0a08
    return drv_gh621x_chipid;
}
/* ble */

/**
 * @fn     GU8 Gh621xModuleSendDataViaBle(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via ble
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
GU8 Gh621xModuleSendDataViaBle(GU8 uchString[], GU8 uchLength)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user

    EXAMPLE_DEBUG_LOG_L2("ble send length %d\r\n", uchLength);
    return ret;
}

/**
 * @fn     void Gh621xModuleHandleRecvDataViaBle(GU8 *puchData,GU8 uchLength)
 *
 * @brief  handle recv data via ble
 *
 * @attention   call by recv goodix app/tools via ble
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaBle(GU8 *puchData, GU8 uchLength)
{
    Gh621xModuleCommParseHandler(Gh621xModuleSendDataViaBle, puchData, uchLength);
}

/* uart */

/**
 * @fn     GU8 Gh621xModuleSendDataViaUart(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via uart
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
GU8 Gh621xModuleSendDataViaUart(GU8 uchString[], GU8 uchLength)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
    hal_trace_output(uchString, uchLength);
    return ret;
}
/**
 * @fn     void Gh621xModuleHandleRecvDataViaUart(GU8 *puchData, GU8 uchLength)
 *
 * @brief  handle recv data via uart
 *
 * @attention   call by recv goodix app/tools via uart
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaUart(GU8 *puchData, GU8 uchLength)
{

    if (NULL == gh621x_start_test_mode_timer)
        gh621x_start_test_mode_timer = osTimerCreate(osTimer(GH621X_START_TEST_MODE_TIMER), osTimerOnce, 0);

    if ((puchData[0] == 0xa5) && (puchData[1] == 0x25)) {
        if (!g_IsGH621xStartTestMode) {
            hal_cpu_wake_lock(HAL_CPU_WAKE_LOCK_USER_8);
            app_sysfreq_req(APP_SYSFREQ_USER_TOUCH_UTT, APP_SYSFREQ_52M);
            g_IsGH621xStartTestMode = 1;
            osTimerStop(gh621x_start_test_mode_timer);
            osTimerStart(gh621x_start_test_mode_timer, 30 * 1000);
        }
        g_IsGH621xStartTestModeCnt++;
    }
    Gh621xModuleCommParseHandler(Gh621xModuleSendDataViaUart, puchData, uchLength);
}

/* spp */

/**
 * @fn     GU8 Gh621xModuleSendDataViaSpp(GU8 uchString[], GU8 uchLength)
 *
 * @brief  send string via spp
 *
 * @attention   None
 *
 * @param[in]   uchString      pointer to send data buffer
 * @param[in]   uchLength      send data len
 * @param[out]  None
 *
 * @return  error code
 * @retval  #GH621X_EXAMPLE_OK_VAL       return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL      return error	
 */
static GU8 gh621x_spp_send_buf[0xff];
GU8 Gh621xModuleSendDataViaSpp(GU8 uchString[], GU8 uchLength)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
    //TRACE(3,"%s length %d", __func__, uchLength);
    //DUMP8 ( " %02x",uchString, uchLength );
    memset((void *)&gh621x_spp_send_buf, 0x00, sizeof(gh621x_spp_send_buf));
    memcpy((void *)&gh621x_spp_send_buf, (void *)uchString, uchLength);
    gh621x_adapter_send_data(HAL_TOUCH_EVENT_SEND_DATA, gh621x_spp_send_buf, (uint32_t)uchLength);
    return ret;
}
extern void xspace_ui_boost_freq_when_busy(void);

/**
 * @fn     void Gh621xModuleHandleRecvDataViaSpp(GU8 *puchData, GU8 uchLength)
 *
 * @brief  handle recv data via spp
 *
 * @attention   call by recv goodix app/tools via spp
 *
 * @param[in]   puchData       pointer to recv data buffer
 * @param[in]   uchLength      recv data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleRecvDataViaSpp(GU8 *puchData, GU8 uchLength)
{
    uint8_t get_bt_addr_cmd[4] = {0x04, 0xFF, 0x00, 0x05};
    if (memcmp(puchData, get_bt_addr_cmd, sizeof(get_bt_addr_cmd)) == 0)   //Production test spp get bt addr
    {
        uint8_t bt_mac_addr[6];
        uint8_t bt_mac_addr_swap[6];
        factory_section_original_btaddr_get(bt_mac_addr);
        for (uint8_t i = 0; i < 6; i++) {
            bt_mac_addr_swap[i] = bt_mac_addr[5 - i];
        }
        Gh621xModuleSendDataViaSpp(bt_mac_addr_swap, sizeof(bt_mac_addr_swap));
        return;
    }
    if ((puchData[0] == 0xa5) && (puchData[1] == 0x25)) {
#if 0
		if(!g_IsGH621xStartSppMode)
		{
			hal_cpu_wake_lock(HAL_CPU_WAKE_LOCK_USER_8);
			app_sysfreq_req(APP_SYSFREQ_USER_TOUCH_UTT, APP_SYSFREQ_52M);
			g_IsGH621xStartTestMode=1;
			osTimerStop(gh621x_start_test_mode_timer);
			osTimerStart(gh621x_start_test_mode_timer,30*1000);
		}
		g_IsGH621xStartTestModeCnt++;
#else
        xspace_ui_boost_freq_when_busy();
#endif
    }
    Gh621xModuleCommParseHandler(Gh621xModuleSendDataViaSpp, puchData, uchLength);
}

/**
 * @fn     void Gh621xModuleHandleGoodixCommCmd(EMGh621xUprotocolParseCmdType emCmdType)
 *
 * @brief  handle cmd with all ctrl cmd @ref EMGh621xUprotocolParseCmdType
 *
 * @attention   None
 *
 * @param[in]   emCmdType      cmd type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleGoodixCommCmd(EMGh621xUprotocolParseCmdType emCmdType)
{
    // code implement by user
}

/* handle evt */

/**
 * @fn     void Gh621xModuleHandleTkForceEvent(EMGh621xModuleMotionEvent emEvent)
 *
 * @brief  tk/force event handle,@EMGh621xModuleMotionEvent
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleTkForceEvent(EMGh621xModuleMotionEvent emEvent)
{
    switch (emEvent) {
        case GH621X_MOTION_ONE_TAP:
            // code implement by user
            gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_CLICK);
            break;
        case GH621X_MOTION_DOUBLE_TAP:
            // code implement by user
            gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_DOUBLE_CLICK);
            break;
        case GH621X_MOTION_TRIPLE_TAP:
            // code implement by user
            gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_TRIPLE_CLICK);
            //Note(Mark):for test
            //app_ibrt_ui_test_wakeup_ai_voice();
            break;
        case GH621X_MOTION_UP:
            // code implement by user
            break;
        case GH621X_MOTION_DOWN:
            // code implement by user
            break;
        case GH621X_MOTION_LONG_PRESS:
            // code implement by user
            gh621x_long_press_cnt++;
            GH621X_TRACE(2, "%s gh621x_long_press_cnt:%d", __FUNCTION__, gh621x_long_press_cnt);
            if (1 == gh621x_long_press_cnt)
                gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_LONGPRESS);
            else if (3 == gh621x_long_press_cnt)
                gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_LONGPRESS_3S);
            break;
        case GH621X_MOTION_RELEASE:
            // code implement by user
            //gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_RELEASE);
            break;
        default:
            break;
    }
}

/**
 * @fn     void Gh621xModuleHandlePressAndLeaveEvent(EMGh621xModuleMotionEvent emEvent)
 *
 * @brief  press and leave event handle,@EMGh621xModuleMotionEvent
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandlePressAndLeaveEvent(EMGh621xModuleMotionEvent emEvent)
{
    switch (emEvent) {
        case GH621X_MOTION_LONG_PRESS:
            // code implement by user
            gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_PRESS);
            break;
        case GH621X_MOTION_RELEASE:
            // code implement by user
            gh621x_long_press_cnt = 0;
            gh621x_adapter_report_gesture_event(HAL_TOUCH_EVENT_RELEASE);
            break;
        default:
            break;
    }
}

/**
 * @fn     void Gh621xModuleHandleWearEvent(EMGh621xModuleWearStatus emEvent)
 *
 * @brief  wear event handle,@EMGh621xModuleWearStatus
 *
 * @attention   None
 *
 * @param[in]   emEvent      event type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleWearEvent(EMGh621xModuleWearStatus emEvent)
{
    switch (emEvent) {
        case GH621X_WEAR_OUT:
            // code implement by user
            gh621x_adapter_report_inear_status(false);
            GH621X_TRACE(1, "%s GH621X_WEAR_OUT", __FUNCTION__);
            break;
        case GH621X_WEAR_IN:
            // code implement by user
            gh621x_adapter_report_inear_status(true);
            GH621X_TRACE(1, "%s GH621X_WEAR_IN", __FUNCTION__);
            break;
        default:
            break;
    }
}

/**
 * @fn     void Gh621xModuleHandleIedTkRawdata(uint8_t *puchIedTkRawdata, GU16 usDataLen)
 *
 * @brief  gh621x ied tk rawdata evt handler, @ref protocol docs
 *
 * @attention   None
 *
 * @param[in]   puchIedTkRawdata      pointer to rawdata buffer
 * @param[in]   usDataLen      		  rawdata data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleIedTkRawdata(GU8 *puchIedTkRawdata, GU16 usDataLen)
{
    // code implement by user
}

/**
 * @fn     void Gh621xModuleHandleSamplingDoneData(GU8 *puchSamplingDoneData, GU16 usSamplingDoneDataLen)
 *
 * @brief  gh621x sampling data evt handler
 *
 * @attention   None
 *
 * @param[in]   puchSamplingDoneData      pointer to data buffer
 * @param[in]   usSamplingDoneDataLen     data len
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleHandleSamplingDoneData(GU8 *puchSamplingDoneData, GU16 usSamplingDoneDataLen)
{
    // code implement by user
}

/* timer */

/**
 * @fn     void Gh621xModuleCommEventReportTimerHandler(void)
 *
 * @brief  communicate event report timer handler
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerHandler(void const *param)
{
    EXAMPLE_DEBUG_LOG_L2("Gh621xModuleCommEventReportTimerHandler\r\n");
    if (GH621X_RET_OK != Gh621xModuleEventReportSchedule()) {
        Gh621xModuleCommEventReportTimerStop();
    }
}

/***czh add 20210225***/
osTimerDef(GH621X_EVENT_REPORT_TIMER, Gh621xModuleCommEventReportTimerHandler);
static osTimerId gh621x_event_report_timer = NULL;
/***czh add 20210225***/

/**
 * @fn     void Gh621xModuleCommEventReportTimerStart(void)
 *
 * @brief  communicate event report timer start
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerStart(void)
{
    // code implement by user
    //	EXAMPLE_DEBUG_LOG_L2("Gh621xModuleCommEventReportTimerStart\r\n");
    EXAMPLE_DEBUG_LOG_L2("cancel timerstart!!!\r\n");
    /***czh add 20210225***/
    if (NULL == gh621x_event_report_timer)
        gh621x_event_report_timer = osTimerCreate(osTimer(GH621X_EVENT_REPORT_TIMER), osTimerPeriodic, 0);
    osTimerStop(gh621x_event_report_timer);
    osTimerStart(gh621x_event_report_timer, 250);
    /***czh add 20210225***/
}

/**
 * @fn     void Gh621xModuleCommEventReportTimerStop(void)
 *
 * @brief  communicate event report timer stop
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerStop(void)
{
    // code implement by user
    EXAMPLE_DEBUG_LOG_L2("cancel TimerStop!!!\r\n");
    //	EXAMPLE_DEBUG_LOG_L2("Gh621xModuleCommEventReportTimerStop\r\n");
    /***czh add 20210225***/
    osTimerStop(gh621x_event_report_timer);
    /***czh add 20210225***/
}

/**
 * @fn     void Gh621xModuleCommEventReportTimerInit(void)
 *
 * @brief  communicate event report timer init 
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleCommEventReportTimerInit(void)
{
    // code implement by user
    // must register func Gh621xModuleCommEventReportTimerHandler as callback
    /* should setup 5Hz-20Hz timer and ble connect interval should < 50ms
	*/
    /***czh add 20210225***/
    EXAMPLE_DEBUG_LOG_L2("cancel TimerInit!!!\r\n");
    if (NULL == gh621x_event_report_timer)
        gh621x_event_report_timer = osTimerCreate(osTimer(GH621X_EVENT_REPORT_TIMER), osTimerPeriodic, 0);
    /***czh add 20210225***/
}

/* log */

/**
 * @fn     void Gh621xDebugLog(char *pchLogString)
 *
 * @brief  print dbg log
 *
 * @attention   None
 *
 * @param[in]   pchLogString		pointer to debug log string
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xDebugLog(char *pchLogString)
{
    // code implement by user
    TRACE(0, "%s", pchLogString);
}

/* hook */

/**
 * @fn     void Gh621xModuleInitHook(void)
 *
 * @brief  init hook
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleInitHook(void)
{
    // code implement by user
    // e.g. sync msg to host app or add more config
}

/**
 * @fn     void Gh621xModuleStartHook(GU8 uchStartType)
 *
 * @brief  start hook
 *
 * @attention   None
 *
 * @param[in]   uchStartType		start type
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStartHook(GU8 uchStartType)
{
    // code implement by user, start_type @ref EMGh621xStartHookType
    // e.g. sync msg to host app or add more config
    if (uchStartType == GH621X_START_HOOK_MCU_MODE_START) {
        Gh621xConfigTkIedMcuModeIrqEnable();
    } else if (uchStartType == GH621X_START_HOOK_APP_MODE_START) {
        Gh621xConfigTkIedAppModeIrqEnable(1);
    } else if (uchStartType == GH621X_START_HOOK_APP_MODE_WITHOUT_AUTOCC_START) {
        Gh621xConfigTkIedAppModeIrqEnable(0);
    }
}

/**
 * @fn     void Gh621xModuleStopHook(void)
 *
 * @brief  stop hook
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh621xModuleStopHook(void)
{
    // code implement by user
    // e.g. sync msg to host app or add more config
}

/* uprotocol */

/**
 * @fn     char *Gh621xModuleGetBluetoothVer(void)
 *
 * @brief  get bluetooth version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetBluetoothVer(void)
{
    uint8_t bt_mac_addr[6] = {0};
    uint8_t temp_val = 0;
    factory_section_original_btaddr_get(bt_mac_addr);
    DUMP8("0x%02x,", bt_mac_addr, 6);
    for (uint8_t i = 0; i < sizeof(bt_mac_addr); i++) {
        temp_val = (bt_mac_addr[i] & 0xF0) >> 4;
        if (temp_val > 9) {
            BtMacInfoChar[11 - (2 * i + 1)] = (temp_val - 10) + 'A';
        } else {
            BtMacInfoChar[11 - (2 * i + 1)] = temp_val + '0';
        }

        temp_val = bt_mac_addr[i] & 0x0F;
        if (temp_val > 9) {
            BtMacInfoChar[11 - (2 * i)] = (temp_val - 10) + 'A';
        } else {
            BtMacInfoChar[11 - (2 * i)] = temp_val + '0';
        }
    }

    GH621X_TRACE(3, "%s BT ADDR:%s;%d", __FUNCTION__, BtMacInfoChar, sizeof(bt_mac_addr));
    return BtMacInfoChar;   //BLE_VERSION_STR;
}

/**
 * @fn     char *Gh621xModuleGetHardwareVer(void)
 *
 * @brief  get hardware version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetHardwareVer(void)
{
    return HW_VERSION_STR;
}

/**
 * @fn     char *Gh621xModuleGetFirwarewareVer(void)
 *
 * @brief  get firmware version string
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  pointer to version string
 */
char *Gh621xModuleGetFirwarewareVer(void)
{
    return FW_VERSION_STR;
}

/**
 * @fn     GS8 Gh621xModuleHostCtrl(GU8 uchCtrlType,  GU8 *puchCtrlParam, GU8 uchCtrlParamLen)
 *
 * @brief  handle host ctrl cmd
 *
 * @attention   None
 *
 * @param[in]   uchCtrlType				ctrl type 
 * @param[in]   puchCtrlParam			pointer to ctrl param 
 * @param[in]   uchCtrlParamLen			ctrl param len
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_RET_OK               return successfully
 * @retval  #GH621X_RET_GENERIC_ERROR    return error
 */
GS8 Gh621xModuleHostCtrl(GU8 uchCtrlType, GU8 *puchCtrlParam, GU8 uchCtrlParamLen)
{
    // code implement by user
    // uchCtrlType @ref EMGh621xUprotocolHostCtrlType
    GS8 ret = GH621X_RET_OK;
    switch (uchCtrlType) {
        case GH621X_UPROTOCOL_HOST_CTRL_TYPE_RESET:
            // code implement by user,reset host
            break;

        case GH621X_UPROTOCOL_HOST_CTRL_TYPE_DMIC_ON:
            // code implement by user,reset host
            break;
        case GH621X_UPROTOCOL_HOST_CTRL_TYPE_DMIC_OFF:
            // code implement by user,reset host
            break;

        default:
            ret = GH621X_RET_GENERIC_ERROR;
            break;
    }
    return ret;
}

#define GH621X_FLASH_DATA_LENG 256
/**
 * @fn     GU8 Gh621xModuleWriteMainCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  write main calibration list data to flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[in]   puchDataBuffer		pointer to data output 
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleWriteMainCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
#if 1
    if (0) {
        if (!touch_data_write_flash((uint8_t *)puchDataBuffer, usDataLen)) {
            xra_write_touch_data_to_ram((uint8_t *)puchDataBuffer, usDataLen);
            return 1;
        } else {
            return 0;
        }
    } else {
        if (!xra_write_touch_data_to_ram((uint8_t *)puchDataBuffer, usDataLen)) {
            return 1;
        } else {
            return 0;
        }
    }
#endif

    return ret;
}

/**
 * @fn     GU8 Gh621xModuleReadMainCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  read main calibration list data from flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[out]  puchDataBuffer		pointer to data output 
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleReadMainCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
#if 1
    if (0) {
        if (!touch_data_read_flash((uint8_t *)puchDataBuffer, usDataLen)) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (!xra_read_touch_data_from_ram((uint8_t *)puchDataBuffer, usDataLen)) {
            return 1;
        } else {
            return 0;
        }
    }
#endif

    return ret;
}

/**
 * @fn     GU8 Gh621xModuleWriteBackupCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  write backup calibration list data to flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[in]   puchDataBuffer		pointer to data output 
 * @param[out]  None
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleWriteBackupCalibrationListToFlash(GU8 *puchDataBuffer, GU16 usDataLen)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
#if 1
    if (0) {
        if ((!touch_back_data_write_flash((uint8_t *)puchDataBuffer, usDataLen))
            && (!xspace_backup_touch_back_data_write_flash((uint8_t *)puchDataBuffer, usDataLen))) {
            xra_write_touch_back_data_to_ram((uint8_t *)puchDataBuffer, usDataLen);
            return 1;
        } else {
            return 0;
        }
    } else {
        if (!xra_write_touch_back_data_to_ram((uint8_t *)puchDataBuffer, usDataLen)) {
            return 1;
        } else {
            return 0;
        }
    }
#endif
    return ret;
}
/**
 * @fn     GU8 Gh621xModuleReadBackupCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
 *
 * @brief  read backup calibration list data from flash
 *
 * @attention   None
 *
 * @param[in]   usDataLen			data len to read
 * @param[out]  puchDataBuffer		pointer to data output 
 *
 * @return  errcode
 * @retval  #GH621X_EXAMPLE_OK_VAL     return successfully
 * @retval  #GH621X_EXAMPLE_ERR_VAL    return error
 */
GU8 Gh621xModuleReadBackupCalibrationListFromFlash(GU8 *puchDataBuffer, GU16 usDataLen)
{
    GU8 ret = GH621X_EXAMPLE_OK_VAL;
    // code implement by user
#if 1
    uint8_t tepm_touch_data[GH621X_FLASH_DATA_LENG];
    uint8_t temp_touch_bake_data[GH621X_FLASH_DATA_LENG];

    if (0)

    {
        if ((usDataLen <= GH621X_FLASH_DATA_LENG) && (puchDataBuffer != NULL)) {
            if ((touch_back_data_read_flash((uint8_t *)tepm_touch_data, usDataLen) == 0)
                && (xspace_backup_touch_back_data_read_flash((uint8_t *)temp_touch_bake_data, usDataLen) == 0)) {
                if (memcmp(tepm_touch_data, temp_touch_bake_data, usDataLen) == 0) {
                    memcpy(puchDataBuffer, tepm_touch_data, usDataLen);
                } else {
                    memset(puchDataBuffer, 0x00, usDataLen);
                    return 0;
                }
                return 1;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        if (!xra_get_touch_back_data_from_ram((uint8_t *)puchDataBuffer, usDataLen)) {
            return 1;
        } else {
            return 0;
        }
    }
#endif
    return ret;
}

static void Gh621xModuleWriteFlashBeforShutdown(void)
{
    uint8_t temp_touch_data[GH621X_FLASH_DATA_LENG];
    uint8_t temp_touch_back_data[GH621X_FLASH_DATA_LENG];
    GH621X_TRACE(1, "%s enter", __FUNCTION__);

    if (0)

    {
        if (!xra_read_touch_data_from_ram(temp_touch_data, GH621X_FLASH_DATA_LENG)) {
            touch_data_write_flash(temp_touch_data, GH621X_FLASH_DATA_LENG);
        }

        if (!xra_get_touch_back_data_from_ram(temp_touch_back_data, GH621X_FLASH_DATA_LENG)) {
            touch_back_data_write_flash(temp_touch_back_data, GH621X_FLASH_DATA_LENG);
        }
    }
}

#if defined(__XSPACE_UI__)   //longz 20210128 add
extern uint8_t g_wear_calib_check_flag;    //0:fail   1:sucess
extern uint8_t g_press_calib_check_flag;   //0:fail   1:sucess
#endif

void Gh621xModuleFlashInit(void)
{
    uint8_t temp_touch_data[GH621X_FLASH_DATA_LENG];
    uint8_t temp_touch_back_data[GH621X_FLASH_DATA_LENG];

    if (!touch_data_read_flash(temp_touch_data, GH621X_FLASH_DATA_LENG)) {
        xra_write_touch_data_to_ram(temp_touch_data, GH621X_FLASH_DATA_LENG);
    }

    if (!touch_back_data_read_flash(temp_touch_back_data, GH621X_FLASH_DATA_LENG)) {
        xra_write_touch_back_data_to_ram(temp_touch_back_data, GH621X_FLASH_DATA_LENG);
    }
    xspace_interface_register_write_flash_befor_shutdown_cb(WRITE_TOUCH_TO_FLASH, Gh621xModuleWriteFlashBeforShutdown);
    GH621X_TRACE(0, "\n");
    GH621X_TRACE(3, "%s calibration_factor Pressure:%d,%d", __FUNCTION__, (temp_touch_data[2] & 0x80) >> 7, (temp_touch_back_data[2] & 0x80) >> 7);
    GH621X_TRACE(3, "%s calibration_factor Wear:%d,%d", __FUNCTION__, (temp_touch_data[2] & 0x10) >> 4, (temp_touch_back_data[2] & 0x10) >> 4);

#if defined(__XSPACE_UI__)   //longz 20210128 add
    if (((temp_touch_data[2] & 0x80) >> 7) && ((temp_touch_back_data[2] & 0x80) >> 7))   //press calib
    {
        g_press_calib_check_flag = 1;   //press calib OK
    } else {
        g_press_calib_check_flag = 0;   //press calib fail
    }

    if (((temp_touch_data[2] & 0x10) >> 4) && ((temp_touch_back_data[2] & 0x10) >> 4))   //Wear calib
    {
        g_wear_calib_check_flag = 1;   //wear calib OK
    } else {
        g_wear_calib_check_flag = 0;   //wear calib fail
    }

    GH621X_TRACE(1, "g_wear_calib_check_flag:%d", g_wear_calib_check_flag);
    GH621X_TRACE(1, "g_press_calib_check_flag:%d", g_press_calib_check_flag);
#endif

    for (uint16_t i = 0; i < usGh621xModuleDefaultRegConfigArrayLen; i++) {
        if (0x0924 == stGh621xModuleDefaultRegConfigArray[i].usRegAddr) {
            GH621X_TRACE(2, "%s leave thresh=%dg,press thresh=%dg", __FUNCTION__, stGh621xModuleDefaultRegConfigArray[i].usRegData >> 8,
                         stGh621xModuleDefaultRegConfigArray[i].usRegData & 0xff);
            break;
        }
    }
}
/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
