/***************************************************************************************************
 *Name: aw8680x.h
 *Created on: 2020.9.15
 *Author: www.awinic.com.cn
 ***************************************************************************************************/
#ifndef _AW8680X_H_
#define _AW8680X_H_
#include <stdint.h>

#include "aw_soc_protocol.h"

#define AW_TAG          "aw8680x"
#define TIME_OUT        1000
#define AW8680X_I2C_RETRIES     5
#define AW8680X_I2C_ADDR        0x5c//(0x5c << 1)
#define AW8680X_DRIVER_VERSION  "v0.1.4"

extern unsigned char bin_data[22036];	//add czh 2020.11.27

int aw8680x_adc_read(unsigned char data[]);
int aw8680x_force_read(unsigned char data[]);
int aw8680x_key_read(unsigned char data[]);
int aw8680x_mode_set(unsigned char mode);
int aw8680x_chipid_read(unsigned char data[]);
int aw8680x_init(void);


int aw8680x_i2c_write(unsigned char *data, unsigned short len);
int aw8680x_i2c_read(unsigned char *data, unsigned short len);
void aw8680x_delay_ms(unsigned int nms);

//wake-up pin
#define WAKE_UP_Pin GPIO_PIN_13
#define WAKE_UP_GPIO_Port GPIOB

//reset pin
#define RESET_Pin GPIO_PIN_8
#define RESET_GPIO_Port GPIOB

#define CPU_IN_UBOOT            0x00000001
#define CPU_IN_FLASH_BOOT       0x00010002

#define FLASH_BASE_ADDR         0x01001000
#define FLASH_MAX_ADDR          0x01010000
#define ERASE_BYTE_MAX          512
#define WRITE_FLASH_MAX         64
#define READ_FLASH_MAX          64

#define SOC_ADDR                0x00000000
#define SOC_DATA_LEN            0x0000
#define SOC_READ_LEN            0x0000
#define BIN_DATA_OFFSET         72

#define SOC_APP_DATA_TYPE       0x21

/* register definition */
#define SCAN_MODE_SWITCH_ADDR   0x56
#define SCAN_MODE_SWITCH_LEN    1

#define ADC_DATA_ADDR           0x10
#define ADC_DATA_LEN            2

#define KEY_DATA_ADDR           0x12
#define KEY_DATA_LEN            1

#define FORCE_DATA_ADDR         0x20
#define FORCE_DATA_LEN          2

#define CHIP_ID_ADDR            0x30
#define CHIP_ID_DATA_LEN        4

enum scan_mode_switch_enum {
  DEEP_SLEEP = 1,
  HIGH_SPEED = 2,
  LOW_SPEED = 3,
  POWER_OFF = 4,
};

enum return_flag_enum {
  OK_FLAG = 0,
  ERR_FLAG = -1,
  NOT_NEED_UPDATE = -2,
};


struct aw8680x {

  unsigned char chip_type[8]; /* Frame header information-chip type */
  unsigned int bin_data_type; /* Frame header information-Data type */
  unsigned int valid_data_len; /* Effective data length of bin data */
  unsigned int valid_data_addr; /* Offset of valid data in bin data */
  unsigned int download_addr; /* Get the download address from bin data */
  unsigned int app_version; /* Software version number obtained from bin data */

  GUI_TO_SOC_S p_gui_data_s; /* GUI to SOC data structure */
  unsigned char module_id;
  unsigned char p_protocol_tx_data[PROTOCOL_TOTAL_LEN];
  unsigned char p_protocol_rx_data[PROTOCOL_TOTAL_LEN];

  unsigned int app_current_addr;
  unsigned int protocol_version;
  unsigned int software_version;
  unsigned int flash_app_len;
  unsigned int flash_app_addr;
  unsigned int erase_sector_num;
  int ack_flag;
};

#endif
