#ifndef __XSPACE_I2C_MASTER_H__
#define __XSPACE_I2C_MASTER_H__

#if defined(__USE_SW_I2C__) || defined(__USE_HW_I2C__)

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XSPACE_HW_I2C = 0,
    XSPACE_SW_I2C = 1,
} xspace_i2c_type_e;

bool xspace_get_i2c_init_status();
void xspace_i2c_init(void);
bool xspace_i2c_write(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t data);
bool xspace_i2c_read(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t *data);
bool xspace_i2c_burst_write(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, const uint8_t *data_buffer, uint8_t len);
bool xspace_i2c_burst_read(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t *data, uint16_t rx_size);
bool xspace_i2c_write_NULL(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr);
bool xspace_i2c_burst_write_01(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *write_data, uint8_t write_len);
bool xspace_i2c_burst_read_01(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *cmd_data, uint16_t cmd_len, uint8_t *read_data, uint16_t rx_size);
bool xspace_i2c_burst_write_s16(xspace_i2c_type_e i2c_type, uint8_t address, uint16_t reg_addr, uint8_t *data_buffer, uint8_t len);
bool xspace_i2c_burst_read_s16(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *cmd_data, uint16_t cmd_len, uint8_t *read_data, uint16_t rx_size);

#ifdef __cplusplus
}
#endif

#endif  // #if defined(__USE_SW_I2C__) || defined(__USE_HW_I2C__)

#endif  // __XSPACE_I2C_MASTER_H

