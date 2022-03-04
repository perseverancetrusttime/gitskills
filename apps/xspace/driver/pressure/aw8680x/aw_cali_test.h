#ifndef __AW_CALI_TEST_H_
#define __AW_CALI_TEST_H_

#include "aw8680x.h"

int aw_cali_ready(void);
int aw_cali_press(void);
int aw_cali_wait_up(void);
int aw_test_ready(void);
int aw_test_press(void);
int aw_test_wait_up(void);
int aw_calibration(void);

int aw_set_thr(unsigned short thr[]);
int aw_get_thr(unsigned short thr[]);

/* Number of channels */
#define PHY_CH_NUM      1


#define FW_VER_ADDR     0x85
#define FW_VER_LEN      14

#define SEN_DET_ADDR    0xA4
#define SEN_DET_LEN     8

#define TEMP_READ_ADDR  0x25
#define TEMP_READ_LEN   PHY_CH_NUM*2+4

#define THR_ADDR        0x5B
#define THR_LEN         PHY_CH_NUM*2

#define RAW_DATA_ADDR   0x10
#define RAW_DATA_LEN    PHY_CH_NUM*2

#define CALI_READ_ADDR  0x24
#define CALI_READ_LEN   PHY_CH_NUM*2


#define ERR_DR          1
#define ERR_SETTHR      2
#define ERR_WFCTOR      3
#define ERR_FW          4
#define ERR_SENSOR      5
#define ERR_CFCTOR      6
#define ERR_CALITEST    7


struct aw_cali {

  unsigned int firmware_version;
  unsigned int bin_version;

  unsigned char sensor_status;

  float temp;
  float temp_coef[PHY_CH_NUM];

  int unpress_rawsum[PHY_CH_NUM];
  short unpress_rawavg[PHY_CH_NUM];

  int press_rawsum[PHY_CH_NUM];
  short press_rawavg[PHY_CH_NUM];

  float cali_factor[PHY_CH_NUM];
  float read_cali_factor[PHY_CH_NUM];

  float deviation[PHY_CH_NUM];

};

#endif



