#ifndef __SW_I2C_MASTER_H__
#define __SW_I2C_MASTER_H__

#if defined (__USE_SW_I2C__)

#include "stdint.h"
#include "stdbool.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "tgt_hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_READ_BIT                (0x01)        //!< If this bit is set in the address field, transfer direction is from slave to master.

#define I2C_ISSUE_STOP              ((bool)true)  //!< Parameter for @ref twi_master_transfer
#define I2C_DONT_ISSUE_STOP         ((bool)false) //!< Parameter for @ref twi_master_transfer

#define I2C_SW_SDA_HIGH()           do { hal_gpio_pin_set((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SDA_PIN); } while(0)    /*!< Pulls SDA line high */
#define I2C_SW_SDA_LOW()            do { hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SDA_PIN); } while(0)    /*!< Pulls SDA line low  */
#define I2C_SW_SDA_INPUT()          do { hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SDA_PIN, HAL_GPIO_DIR_IN, 1);  } while(0)   /*!< Configures SDA pin as input  */
#define I2C_SW_SDA_OUTPUT()         do { hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SDA_PIN, HAL_GPIO_DIR_OUT,1);  } while(0)   /*!< Configures SDA pin as output */
#define I2C_SW_SCL_OUTPUT()         do { hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SCL_PIN, HAL_GPIO_DIR_OUT,1);  } while(0)   /*!< Configures SCL pin as output */
#define I2C_SW_SCL_OUTPUTL()        do { hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SCL_PIN, HAL_GPIO_DIR_OUT,0);  } while(0)   /*!< Configures SCL pin as output */
#define I2C_SW_SCL_INPUT()          do { hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SCL_PIN, HAL_GPIO_DIR_IN,1);  } while(0)   /*!< Configures SCL pin as output */
#define I2C_SW_SCL_HIGH()           I2C_SW_SCL_INPUT() 
#define I2C_SW_SCL_LOW()            I2C_SW_SCL_OUTPUTL()


#define I2C_SW_SDA_READ()           (hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SDA_PIN))               /*!< Reads current state of SDA */
#define I2C_SW_SCL_READ()           (hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)SW_I2C_MASTER_SCL_PIN))               /*!< Reads current state of SCL */

#define I2C_RAM_52M_DELAYN()        hal_sys_timer_delay_ns(100)
#define I2C_RAM_SHORT_DELAYN()      hal_sys_timer_delay_ns(200)
#define I2C_RAM_DELAYN()         	hal_sys_timer_delay_ns(200)

#define I2C_SW_DELAYN_SHORT()       I2C_RAM_SHORT_DELAYN()
#define I2C_SW_DELAYN()             I2C_RAM_DELAYN()

bool sw_i2c_master_init(void);
bool sw_i2c_master_transfer(uint8_t address, uint8_t *data, uint16_t data_length, bool issue_stop_condition);

#ifdef __cplusplus
}
#endif

#endif

#endif  // __SW_I2C_MASTER_H__

