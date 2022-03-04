#ifndef __XSPACE_FLASH_ACCESS_H__
#define __XSPACE_FLASH_ACCESS_H__

#if defined(__XSPACE_FLASH_ACCESS__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define XSPACE_FLASH_ACCESS_DEBUG (1)

#if (XSPACE_FLASH_ACCESS_DEBUG)
#define XFA_TRACE(num, str, ...) TRACE(num + 2, "[XFA] %s,line=%d, " str, __func__, __LINE__, ##__VA_ARGS__)
#else
#define XFA_TRACE(num, str, ...)
#endif

#define ALIGN4                       __attribute__((aligned(4)))
#define nvrec_xspace_dev_magic       (0xfeba)
#define nvrec_xspace_current_version (0x01)
#define XSPACE_SECTOR_SIZE           (4096)

#define TOUCH_DATA_LENG 256

typedef struct {
    unsigned short magic;
    unsigned short version;
    unsigned int crc;
    unsigned int reserved0;
    unsigned int reserved1;
} xspace_section_head_t;

typedef struct {
    uint8_t touch_data[TOUCH_DATA_LENG];
    uint32_t touch_key;
    uint8_t touch_back_data[TOUCH_DATA_LENG];
    uint32_t touch_back_key;

    uint32_t batterry_data[3];
    uint32_t box_batterry_data[3];

    uint8_t box_ver_data[10];
    uint32_t box_ver_key;

    uint8_t peer_ver_data[10];
    uint32_t peer_ver_key;
    uint8_t hw_rst_use_bat_data_flag;
#if defined(__GOODIX_DEBUG__)
	uint16_t gh621x_debug_buff[25];
#endif
} xspace_section_data_t;

typedef struct {
    xspace_section_head_t head;
    xspace_section_data_t data;
} xspace_section_t;

#ifdef __cplusplus
extern "C" {
#endif

void xspace_section_init(void);
int xspace_section_open(void);

int touch_data_write_flash(uint8_t *buf, uint16_t len);
int touch_data_read_flash(uint8_t *buf, uint16_t len);
int touch_back_data_write_flash(uint8_t *buf, uint16_t len);
int touch_back_data_read_flash(uint8_t *buf, uint16_t len);

int batterry_data_write_flash(uint32_t cuur_percent, uint32_t dump_energy);
int batterry_data_read_flash(uint32_t *cuur_percent, uint32_t *dump_energy);
int box_batterry_data_write_flash(uint32_t bat, uint32_t time);
int box_batterry_data_read_flash(uint32_t *bat, uint32_t *time);
int box_version_data_write_flash(uint8_t *ver, uint32_t len);
int box_version_data_read_flash(uint8_t *ver, uint32_t len);
int peer_version_data_clear_flash(void);
int peer_version_data_write_flash(uint8_t *ver, uint32_t len);
int peer_version_data_read_flash(uint8_t *ver, uint32_t len);

int xspace_backup_touch_back_data_write_flash(uint8_t *buf, uint16_t len);
int xspace_backup_touch_back_data_read_flash(uint8_t *buf, uint16_t len);

int hw_rst_use_bat_data_flag_write_flash(uint8_t flag);
int hw_rst_use_bat_data_flag_read_flash(uint8_t *flag);

#if defined(__GOODIX_DEBUG__)
void app_gh621x_debug_info_write_flash(void);
void app_gh621x_debug_info_read_flash(uint16_t* param, uint8_t len);
#endif
#ifdef __cplusplus
}
#endif
#endif /* __XSPACE_FLASH_ACCESS__ */

#endif /* __XSPACE_FLASH_ACCESS_H__ */
