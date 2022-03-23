#if defined(__USE_HW_I2C__)

#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_iomux.h"
#include "tgt_hardware.h"
#include "hw_i2c_master.h"

void hw_i2c_master_init(void)
{
    struct HAL_I2C_CONFIG_T hal_i2c_cfg;
    
	//hal_iomux_set_i2c0();

    hal_i2c_cfg.mode = HAL_I2C_API_MODE_TASK;
    hal_i2c_cfg.use_dma  = 0;
    hal_i2c_cfg.use_sync = 1;
    hal_i2c_cfg.speed = 400000;
    hal_i2c_cfg.as_master = 1;
    
    hal_i2c_open(HAL_I2C_ID_0, &hal_i2c_cfg);
}

bool hw_i2c_master_write(uint8_t address, uint8_t *data, uint16_t reg_len, uint32_t value_len)
{
    bool ret = true;

    ret = !hal_i2c_task_send(HAL_I2C_ID_0, address, data, reg_len + value_len, 0, NULL);

    return ret;
}

bool hw_i2c_master_read(uint8_t address, uint8_t *reg_addr,uint16_t reg_len, uint8_t *data, uint16_t rx_size)
{
    bool ret = true;

    ret = !hal_i2c_task_recv(HAL_I2C_ID_0, address, reg_addr, reg_len,(uint8_t *)data,rx_size, 0, NULL);
    return ret;
}

#endif  // __USE_HW_I2C__

