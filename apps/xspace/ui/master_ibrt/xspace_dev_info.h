#ifndef __XAPCE_DEV_INFO_H__
#define __XAPCE_DEV_INFO_H__

#if defined(__XSPACE_UI__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

extern const char xui_bt_init_name[];
extern const uint8_t xui_dev_sw_version[4];
extern uint8_t xui_dev_hw_version[2];

#define LEN_OF_IMAGE_TAIL_TO_FIND_STRING (512)

#define OTA_BOOT_INFO_FLASH_OFFSET (0x1000)
#define OTA_BOOT_VER_FLASH_OFFSET  (0x2000)

#ifndef __APP_IMAGE_FLASH_OFFSET__
#define __APP_IMAGE_FLASH_OFFSET__ (0x28000)
#endif

#ifndef NEW_IMAGE_FLASH_OFFSET
#define NEW_IMAGE_FLASH_OFFSET (0x1E8000)
#endif

typedef struct {
    uint8_t softwareRevByte0;
    uint8_t softwareRevByte1;
    uint8_t softwareRevByte2;
    uint8_t softwareRevByte3;
} OTA_SOFTWARE_REV_INFO_T;

#ifdef __cplusplus
}
#endif

#endif /* __XSPACE_UI__ */

#endif /* __XAPCE_DEV_INFO_H__ */
