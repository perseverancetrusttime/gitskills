#ifndef __HW_I2C_MASTER_H__
#define __HW_I2C_MASTER_H__

#if defined(__USE_HW_I2C__)
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

void hw_i2c_master_init(void);
bool hw_i2c_master_write(uint8_t address, uint8_t *data, uint16_t reg_len, uint32_t value_len);
bool hw_i2c_master_read(uint8_t address, uint8_t *reg_addr,uint16_t reg_len, uint8_t *data, uint16_t rx_size);


#ifdef __cplusplus
}
#endif

#endif  // __USE_HW_I2C__

#endif  // __HW_I2C_MASTER_H__

