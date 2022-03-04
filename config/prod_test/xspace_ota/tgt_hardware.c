#include "tgt_hardware.h"
#include "hal_location.h"

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PWL_NUM] = {
    //{HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
    //{HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE},
};

//adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
    //HAL_KEY_CODE_FN9,HAL_KEY_CODE_FN8,HAL_KEY_CODE_FN7,
    //HAL_KEY_CODE_FN6,HAL_KEY_CODE_FN5,HAL_KEY_CODE_FN4,
    //HAL_KEY_CODE_FN3,HAL_KEY_CODE_FN2,HAL_KEY_CODE_FN1
};

//gpiokey define
#define CFG_HW_GPIOKEY_DOWN_LEVEL (0)
#define CFG_HW_GPIOKEY_UP_LEVEL   (1)
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {
    //{HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    //{HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
};

//bt config
const char *BT_LOCAL_NAME = TO_STRING(BT_DEV_NAME) "\0";
const char *BLE_DEFAULT_NAME = "OTA_BLE";
uint8_t ble_addr[6] = {
#ifdef BLE_DEV_ADDR
    BLE_DEV_ADDR
#else
    0xBE, 0x99, 0x34, 0x45,
    0x56, 0x67
#endif
};
uint8_t bt_addr[7] = {
#ifdef BT_DEV_ADDR
    BT_DEV_ADDR
#else
    0x1e, 0x57, 0x34, 0x45,
    0x56, 0x67
#endif
};

#define TX_PA_GAIN CODEC_TX_PA_GAIN_DEFAULT

const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY] = {
    {TX_PA_GAIN, 0x03, -99}, {TX_PA_GAIN, 0x03, -45}, {TX_PA_GAIN, 0x03, -42}, {TX_PA_GAIN, 0x03, -39}, {TX_PA_GAIN, 0x03, -36}, {TX_PA_GAIN, 0x03, -33},
    {TX_PA_GAIN, 0x03, -30}, {TX_PA_GAIN, 0x03, -27}, {TX_PA_GAIN, 0x03, -24}, {TX_PA_GAIN, 0x03, -21}, {TX_PA_GAIN, 0x03, -18}, {TX_PA_GAIN, 0x03, -15},
    {TX_PA_GAIN, 0x03, -12}, {TX_PA_GAIN, 0x03, -9},  {TX_PA_GAIN, 0x03, -6},  {TX_PA_GAIN, 0x03, -3},  {TX_PA_GAIN, 0x03, 0},   //0dBm
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
                                                                                HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg = {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
                                                                                 HAL_IOMUX_PIN_PULLUP_ENABLE};

/*
    callback to configure the peripheral on ota_bootloader:
    .key_code , not HAL_KEY_CODE_NONE, the element configure would be valid.  the valud can be anything except HAL_KEY_CODE_NONE if wnats to valid.
    .key_config.pin , not HAL_IOMUX_PIN_NUM , the element configure would be valid.and the val of pin is to be configured.
    .key_config.? , the configure like general gpio configure.
    .key_down, wants the pin to output high or low.

    only can be used to output the dedicated pin to high or low.
*/
const struct HAL_KEY_GPIOKEY_CFG_T ota_bootloader_pin_cfg[OTA_BOOTLOADER_PIN_CFG_NUM] = {
    {HAL_KEY_CODE_FN2, {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}, HAL_KEY_GPIOKEY_VAL_LOW},
};

void programmer_set_ext_gpio(void)
{
    //TODO(Mike.Cheng):For some IO PIN wants to output high or low in ota_bootloader
}

#define PROGRAMMER_OP_WDT_TIMEOUT_MS (60)

const uint32_t programmer_op_wdt_timeout_ms = PROGRAMMER_OP_WDT_TIMEOUT_MS;
BOOT_TEXT_FLASH_LOC bool is_ota_remap_enabled(void)
{
#ifdef OTA_REBOOT_FLASH_REMAP
    return true;
#else
    return false;
#endif
}
