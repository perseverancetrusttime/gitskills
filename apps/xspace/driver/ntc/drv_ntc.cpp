#if defined(__NTC_SUPPORT__)
#include "drv_ntc.h"
#include "hal_ntc.h"
#include "tgt_hardware.h"
#include "dev_thread.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"

static bool ntc_mod_init = false;
static int16_t ntc_temperature = 0xffff;
static ntc_capture_measure_s ntc_capture_measure;
static hal_ntc_report_temperature_cb ntc_report_temperature_cb = NULL;

static const ntc_rt_s c_ntc_rt[] =
{
#ifdef NTC_CUSTOM_TBL
	NTC_CUSTOM_TBL
#else
    {-10,   1482 },
    { -9,   1467 },
    { -8,   1452 },
    { -7,   1437 },
    { -6,   1422 },
    { -5,   1407 },
    { -4,   1392 },
    { -3,   1377 },
    { -2,   1362 },
    { -1,   1347 },
    {  0,   1332 },//0
    {  1,   1318 },
    {  2,   1303 },//2
    {  3,   1288 },
    {  4,   1273 },
    {  5,   1258 },
    {  6,   1240 },
    {  7,   1210 },
    {  8,   1180 },
    {  9,   1150 },
    { 10,   1130 },
    { 11,   1110 },
    { 12,   1090 },//12
    { 13,   1070 },
    { 14,   1050 },
    { 15,   1030 },
    { 16,   1010 },//16
    { 17,    994 },
    { 18,    977 },
    { 19,    962 },
    { 20,    946 },
    { 21,    931 },
    { 22,    915 },
    { 23,    900 },
    { 24,    884 },
    { 25,    869 },
    { 26,    853 },
    { 27,    838 },
    { 28,    822 },
    { 29,    807 },
    { 30,    791 },
    { 31,    776 },
    { 32,    760 },
    { 33,    745 },
    { 34,    729 },
    { 35,    714 },
    { 36,    698 },
    { 37,    683 },
    { 38,    667 },
    { 39,    652 },
    { 40,    637 },
    { 41,    621 },
    { 42,    606 },
    { 43,    590 },//43
    { 44,    580 },
    { 45,    565 },
    { 46,    550 },
    { 47,    535 },
    { 48,    520 },
    { 49,    505 },
    { 50,    490 },
    { 51,    475 },
    { 52,    460 },
    { 53,    445 },
    { 54,    430 },
    { 55,    415 },//55
    { 56,    400 },
#endif
};

static uint16_t ntc_calc_resistance(uint16_t voltage)
{
    uint16_t resistance;
    static uint16_t voltOld[4] = {0};
    static uint32_t cnt = 0;

    voltOld[cnt++ & 0x03] = voltage;
    if (cnt < 4) {
        for (int j = 0; j < 4; j++) {
            voltOld[(++cnt & 0x03)] = voltage;
        }
    }

    voltage = (voltOld[0] + voltOld[1] + voltOld[2] + voltOld[3]) / 4;
    //voltage += NTC_CALIBRATION_VOLTAGE;

    if (voltage > 2600)
        voltage = 2600;

    if (voltage < 200)
        voltage = 200;

    resistance = voltage;//((voltage*NTC_REF_R1*NTC_REF_R2) / (NTC_REF_R2 * NTC_REF_VOLTAGE - voltage*(NTC_REF_R1 + NTC_REF_R2)));

    return resistance;
}

static int16_t ntc_calc_temperature(uint16_t resistance)
{
    int16_t temperature = 60;
    static int16_t old_res[4];
    static int16_t cnt = 0;

    old_res[(++cnt) & 0x3] = resistance;
    if (cnt < 4) {
        for (int j = 0; j < 4; j++) {
            old_res[(++cnt & 0x03)] = resistance;
        }
    }

    resistance = (old_res[0] + old_res[1] + old_res[2] + old_res[3]) / 4;
    //resistance -= 20;
    TRACE(2, "%s voltage=%d", __func__, resistance);

    for (uint32_t i = 0; i < sizeof(c_ntc_rt) / sizeof(c_ntc_rt[0]); i++) {
        if (resistance >= c_ntc_rt[i].resistance) {
            temperature = c_ntc_rt[i].temperature;
            break;
        }
    }

#if 0
    static int16_t my_temprature = 0;
    temperature = my_temprature;
    my_temprature += 1;
#endif

#if 0
    if(temperature <= 45){
        temperature = 25; //Modify by Xiaqihui 20201119 For interim tests
    }
#endif

    return temperature;

}

int32_t ntc_get_temperature(int16_t *temp)
{
    if (NULL == temp)
        return -1;
    *temp = ntc_temperature;
    
    return 0; //Modify by Xiaqihui 20201119 
}

static int ntc_mod_handler(dev_message_body_s *body)
{
    uint16_t ntc_r = ntc_calc_resistance((uint16_t)body->message_Param0);

    if (0 != ntc_r)
        ntc_temperature = ntc_calc_temperature(ntc_r);

    TRACE(3, "%s,%d,%X.", __func__, ntc_temperature, (uint32_t)ntc_report_temperature_cb);
    if (NULL != ntc_report_temperature_cb) {
        ntc_report_temperature_cb(ntc_temperature);
    }

    return 0;
}

static void ntc_event_process(uint16_t voltage)
{
    dev_message_block_s msg;

    if (!ntc_mod_init)
    {
        return;
    }

    msg.mod_id = DEV_MODULE_NTC;
    msg.msg_body.message_id = 0;
    msg.msg_body.message_ptr = 0;
    msg.msg_body.message_Param0 = voltage;
    msg.msg_body.message_Param1 = 0;
    dev_thread_mailbox_put(&msg);
}

static void ntc_capture_irqhandler(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    uint32_t meanVolt = 0;
    TRACE(3, "%s %d irq:0x%04x",__func__, volt, irq_val);

    if (volt == HAL_GPADC_BAD_VALUE)
        return;

    ntc_capture_measure.voltage[ntc_capture_measure.index++%NTC_CAPTURE_STABLE_COUNT] = volt;

    if (ntc_capture_measure.index > NTC_CAPTURE_STABLE_COUNT) {
        for (uint8_t i=0; i<NTC_CAPTURE_STABLE_COUNT; i++) {
            meanVolt += ntc_capture_measure.voltage[i];
        }
        meanVolt /= NTC_CAPTURE_STABLE_COUNT;
        ntc_capture_measure.currvolt = meanVolt;
    } else if (!ntc_capture_measure.currvolt) {
        ntc_capture_measure.currvolt = volt;
    }
    
    TRACE(2, "%s voltage:%d",__func__, ntc_capture_measure.currvolt);
    ntc_event_process(ntc_capture_measure.currvolt);
}

static int ntc_capture_open(void)
{
    ntc_capture_measure.currvolt = 0;
    ntc_capture_measure.index = 0;

    hal_gpadc_open(NTC_GPADC_CHAN, HAL_GPADC_ATP_ONESHOT, ntc_capture_irqhandler);
    return 0;
}

int32_t ntc_register_temperature_report_cb(hal_ntc_report_temperature_cb cb)
{
    if (NULL != ntc_report_temperature_cb)
        return -1;

    ntc_report_temperature_cb = cb;
    return 0;
}

int32_t ntc_capture_start(void)
{
    hal_gpadc_open(NTC_GPADC_CHAN, HAL_GPADC_ATP_ONESHOT, ntc_capture_irqhandler);
    return 0;
}

int32_t ntc_init(void)
{
    TRACE(2, "%s,%d.", __func__, ntc_mod_init);

    if(ntc_mod_init == true) {
        return -1;
    }

    ntc_capture_open();
    dev_thread_handle_set(DEV_MODULE_NTC, ntc_mod_handler);
    ntc_mod_init = true;

    return 0;
}

hal_ntc_ctrl_s ntc_ctrl = {
    ntc_init,
    NULL,
    ntc_get_temperature,
    ntc_capture_start,
    ntc_register_temperature_report_cb,
};

#endif /* __NTC_SUPPORT__ */
