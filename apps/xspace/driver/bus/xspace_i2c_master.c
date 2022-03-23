#if defined(__USE_SW_I2C__) || defined(__USE_HW_I2C__)
#include "stdio.h"
#include "string.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "tgt_hardware.h"
#include "xspace_i2c_master.h"

#if defined (__USE_HW_I2C__)
#include "hw_i2c_master.h"
#endif
#if defined(__USE_SW_I2C__)
#include "sw_i2c_master.h"
#endif

#define RTOS_HAV 1

#ifdef RTOS_HAV
osMutexId xspace_i2c_communication_mutex_id = NULL;
osMutexDef(xspace_i2c_communication_mutex);
#endif

static bool init_status = false;

bool xspace_get_i2c_init_status() {

    return init_status;
}

void xspace_i2c_init(void)
{
#if defined (__USE_HW_I2C__)
    hw_i2c_master_init();
#endif
#if defined (__USE_SW_I2C__)
    sw_i2c_master_init();
#endif

#ifdef RTOS_HAV
    if (xspace_i2c_communication_mutex_id == NULL)
    {
        xspace_i2c_communication_mutex_id = osMutexCreate((osMutex(xspace_i2c_communication_mutex)));
    }
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    init_status = true;

}

bool xspace_i2c_write(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t data)
{
    uint8_t I2c_Write_Buffer[2];
    bool bret = false;

    I2c_Write_Buffer[0] = reg_addr;
    I2c_Write_Buffer[1] = data;
    
#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif

    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_write(address, I2c_Write_Buffer, 1, 1);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, I2c_Write_Buffer, 2, I2C_ISSUE_STOP);
#endif
    }
#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_write_NULL(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr)
{
    uint8_t i2c_Write_Buffer;
    bool bret = true;

    i2c_Write_Buffer = reg_addr;

#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif

    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_write(address, &i2c_Write_Buffer, 1, 0);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, &i2c_Write_Buffer, 1, I2C_ISSUE_STOP);
#endif
    }
#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_read(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t *data)
{
    bool bret = true;

#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif

    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_read(address, &reg_addr,1, data, 1);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, &reg_addr, 1, I2C_ISSUE_STOP);
        if (bret) {
            bret = sw_i2c_master_transfer((address << 1) + 1, data, 1, I2C_ISSUE_STOP);
        }
#endif
    }
    
#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_burst_write(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, const uint8_t *data_buffer, uint8_t len)
{
    uint8_t I2c_Write_Buffer[256];
    bool bret = true;

    I2c_Write_Buffer[0] = reg_addr;
    memcpy(I2c_Write_Buffer + 1, data_buffer, len);
    
#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif
    
    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_write(address, I2c_Write_Buffer, 1, len);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, I2c_Write_Buffer, 1 + len, I2C_ISSUE_STOP);
#endif
    }

#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_burst_read(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t reg_addr, uint8_t *data, uint16_t rx_size)
{
    bool bret = true;

#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif
   
    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_read(address, &reg_addr,1, data, rx_size);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, &reg_addr, 1, I2C_ISSUE_STOP);

        if (bret) {
            bret = sw_i2c_master_transfer((address << 1) + 1, data, rx_size, I2C_ISSUE_STOP);
        }
#endif
    }

#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_burst_write_01(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *write_data, uint8_t write_len)
{
    bool bret = true;

#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif
    
    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_write(address, write_data, 0, write_len);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, write_data, write_len, I2C_ISSUE_STOP);
#endif
    }
      
#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif  

    return bret;
}

bool xspace_i2c_burst_read_01(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *cmd_data, uint16_t cmd_len, uint8_t *read_data, uint16_t rx_size)
{
    bool bret = true;
    
#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif

    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_read(address, cmd_data, cmd_len,read_data, rx_size);
#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, cmd_data, cmd_len, I2C_ISSUE_STOP);
        if (bret)
        {
            bret = sw_i2c_master_transfer((address << 1) + 1, read_data, rx_size, I2C_ISSUE_STOP);
        }
#endif
    }

#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_burst_write_s16(xspace_i2c_type_e i2c_type, uint8_t address, uint16_t reg_addr, uint8_t *data_buffer, uint8_t len)
{
    uint8_t I2c_Write_Buffer[256];
    bool bret = true;
	
    I2c_Write_Buffer[0] = (reg_addr>>8)&0xff;
    I2c_Write_Buffer[1] = reg_addr&0xff;
    memcpy(I2c_Write_Buffer + 2, data_buffer, len);
    
#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif
    
    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_write(address, I2c_Write_Buffer, 2, len);

#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, I2c_Write_Buffer, 2 + len, I2C_ISSUE_STOP);
#endif
    }

#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

bool xspace_i2c_burst_read_s16(xspace_i2c_type_e i2c_type, uint8_t address, uint8_t *cmd_data, uint16_t cmd_len, uint8_t *read_data, uint16_t rx_size)
{
    bool bret = true;
#ifdef RTOS_HAV
    osMutexWait(xspace_i2c_communication_mutex_id, osWaitForever);
#endif

    if (XSPACE_HW_I2C == i2c_type) {
#if defined (__USE_HW_I2C__)    
        bret = hw_i2c_master_read(address, cmd_data, cmd_len, read_data, rx_size);

#endif
    } else {
#if defined (__USE_SW_I2C__)
        bret = sw_i2c_master_transfer(address << 1, cmd_data, cmd_len, I2C_ISSUE_STOP);
        if (bret)
        {
            bret = sw_i2c_master_transfer((address << 1) + 1, read_data, rx_size, I2C_ISSUE_STOP);
        }
#endif
    }

#ifdef RTOS_HAV
    osMutexRelease(xspace_i2c_communication_mutex_id);
#endif

    return bret;
}

#endif

