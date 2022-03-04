#ifndef __XSPACE_RAM_ACCESS_H__
#define __XSPACE_RAM_ACCESS_H__

#if defined(__XSPACE_RAM_ACCESS__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define XSPACE_RAM_ACCESS_DEBUG (1)

#define TOUCH_DATA_LENG 256

#if (XSPACE_RAM_ACCESS_DEBUG)
#define XRA_TRACE(num, str, ...) TRACE(num + 2, "[XRM]%s," str, __func__, ##__VA_ARGS__)
#else
#define XRA_TRACE(num, str, ...)
#endif

//add by quwenchao start
typedef struct {
    uint8_t touch_data[TOUCH_DATA_LENG];
    uint32_t touch_key;
    uint8_t touch_back_data[TOUCH_DATA_LENG];
    uint32_t touch_back_key;
    uint32_t batterry_data[3];
    uint32_t box_batterry_data[3];
    uint8_t box_ver_data[10];
    uint8_t peer_ver_data[10];
    uint32_t spp_mode;
    uint8_t key_func_data[4];

    uint8_t reserved __attribute__((aligned(4)));
    uint32_t crc;

} xspace_data_cache_t;

int xra_write_touch_data_to_ram(uint8_t *buf, uint16_t len);
int xra_read_touch_data_from_ram(uint8_t *buf, uint16_t len);
int xra_write_touch_back_data_to_ram(uint8_t *buf, uint16_t len);
int xra_get_touch_back_data_from_ram(uint8_t *buf, uint16_t len);

int xra_write_box_version_data_to_ram(uint8_t *ver, uint32_t len);
int xra_get_box_version_data_from_ram(uint8_t *ver, uint32_t len);
int xra_write_earphone_battery_data_to_ram(uint32_t cur_percent, uint32_t dump_energy);
int xra_get_earphone_battery_data_from_ram(uint32_t *cuur_percent, uint32_t *dump_energy);
int xra_write_box_battery_data_to_ram(uint32_t bat, uint32_t time);
int xra_get_box_battery_data_from_ram(uint32_t *bat, uint32_t *time);
int xra_clear_peer_version_data_in_ram(void);
int xra_write_peer_version_data_to_ram(uint8_t *ver, uint32_t len);
int xra_get_peer_version_data_from_ram(uint8_t *ver, uint32_t len);
int xra_clear_key_func_data_in_ram(void);
int xra_write_key_func_data_to_ram(uint8_t left, uint8_t right, uint8_t update);
int xra_get_key_func_data_form_ram(uint8_t *left, uint8_t *right, uint8_t *update);

int xra_spp_mode_write_noInit_data(uint32_t spp_mod);
uint32_t xra_spp_mode_read_noInit_data(void);

//add by quwenchao end

#ifdef __cplusplus
}
#endif

#endif /* __XSPACE_RAM_ACCESS__ */

#endif /* __XSPACE_RAM_ACCESS_H__ */
