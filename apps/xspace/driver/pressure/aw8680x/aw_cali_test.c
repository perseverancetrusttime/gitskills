#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "aw_cali_test.h"
#include "aw_log.h"
//#include "i2c.h"


//#define AW_TAG          "aw8680x"
//#define AW8680X_I2C_RETRIES     5

#define AW8680X_PRESS_ADC     34 //add czh 2020.11.27


/* Calibration weight(g) */
float cali_weight = 100.0;
/* Test weight(g) */
float test_weight = 200.0;

static struct aw_cali aw_cali_para;
/*****************************************************************************
*
* aw8680x i2c write/read
*
*****************************************************************************/
static int aw_i2c_write(unsigned char reg_addr, unsigned char reg_data)
{
  int ret = -1;
  unsigned char cnt = 0;
  unsigned char data[2] = {0};

  aw8680x_delay_ms(2);
  data[0] = reg_addr;
  data[1] = reg_data;

  while (cnt < AW8680X_I2C_RETRIES) {
    ret = aw8680x_i2c_write(data, 2);
    if (ret < 0) {
      AW_LOGE(3,AW_TAG,"%s: cnt=%d error=%d\n", __func__, cnt, ret);
    } else {
      break;
    }
    cnt++;
    aw8680x_delay_ms(1);
  }
  return ret;
}

static int aw_i2c_reads(unsigned char reg_addr, unsigned char *reg_data,
                        unsigned short read_len)
{
  int ret = -1;
  unsigned char cnt = 0;

  aw8680x_delay_ms(2);

  while (cnt < AW8680X_I2C_RETRIES) {
    ret = aw8680x_i2c_write(&reg_addr, 1);
    if (ret < 0) {
      AW_LOGE(2,AW_TAG,"aw8680x_i2c_write cnt=%d error=%d\n", cnt, ret);
    } else {
      break;
    }
    cnt++;
    aw8680x_delay_ms(1);
  }
  cnt = 0;
  while (cnt < AW8680X_I2C_RETRIES) {
    ret = aw8680x_i2c_read(reg_data, read_len);
    if (ret < 0) {
      AW_LOGE(2,AW_TAG,"aw8680x_i2c_read cnt=%d error=%d\n", cnt, ret);
    } else {
      break;
    }
    cnt++;
    aw8680x_delay_ms(1);
  }
  return ret;
}


static int aw_i2c_writes(unsigned char reg_addr, unsigned char *reg_data,
                         unsigned short write_len)
{
  int ret = -1;
  unsigned char cnt = 0;
  unsigned char *buf = NULL;

  aw8680x_delay_ms(2);

  buf = (unsigned char *)malloc(write_len + 1);
  if(buf == NULL) {
    AW_LOGE(1,AW_TAG,"write buf malloc fail, size is %d\n", write_len + 1);
    return ERR_FLAG;
  }

  buf[0] = reg_addr;
  memcpy(&buf[1], reg_data, write_len);

  while (cnt < AW8680X_I2C_RETRIES) {
    ret = aw8680x_i2c_write(buf, write_len + 1);
    if (ret < 0) {
      AW_LOGE(3,AW_TAG,"%s: cnt=%d error=%d\n", __func__, cnt, ret);
    } else {
      break;
    }
    cnt++;
    aw8680x_delay_ms(1);
  }
  free(buf);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////

static int dynamic_register(unsigned char reg, unsigned char *read_data,
                            unsigned char *len_data)
{
  unsigned char count = 5;

  aw_i2c_write(0xf6, 0x00);//Clear debug interface state
  aw_i2c_write(0xf7, 0x00);//Clear data length status
  aw_i2c_write(0xf6, reg);//Inform MCU of data content to be read

  while(count--) {
    aw_i2c_reads(0xf7, len_data, 1);
    if (len_data[0] != 0) {
      break;
    }
    aw8680x_delay_ms(10);
  }
  if (len_data[0] == 0) {
    AW_LOGE(0,AW_TAG, "read len_data fail\n");
    return ERR_DR;
  }

  aw_i2c_reads(0xf8, read_data, len_data[0]);

  aw_i2c_write(0xf7, 0x00);//Clear data length status
  aw8680x_delay_ms(10);
  return OK_FLAG;
}

/* Get firmware version */
static int aw_get_firmware_version(void)
{
  int ret = -1;
  unsigned char data[FW_VER_LEN] = {0};

  ret = aw_i2c_reads(FW_VER_ADDR, data, FW_VER_LEN);
  if(ret < 0) {
    AW_LOGE(2,AW_TAG,"%s error, ret is %d\n", __func__, ret);
    return ERR_FLAG;
  }
  aw_cali_para.firmware_version =
    (unsigned int)((data[10]<<0)|(data[11]<<8)|(data[12]<<16)|(data[13]<<24));
  aw_cali_para.bin_version =
    (unsigned int)((bin_data[60]<<0)|(bin_data[61]<<8) |
                   (bin_data[62] << 16)|(bin_data[63]<<24));
  AW_LOGI(2,AW_TAG, "soc version = 0x%x, bin version = 0x%x\n",
          aw_cali_para.firmware_version, aw_cali_para.bin_version);

  return OK_FLAG;
}

/*On off detection of sensor module.
 *return:From low level to high position,
 *it indicates the status of 1 ~ 16 channels,
 *1 indicates disconnection, and 0 indicates connection.
 */
static int aw_detect_sensor_status(void)
{
  int ret = -1;
  unsigned char len[1] = {0};
  unsigned char data[SEN_DET_LEN] = {0};

  aw_cali_para.sensor_status = 0xff;
  ret = dynamic_register(SEN_DET_ADDR, data, len);
  if (ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "%s fail\n", __func__);
    return ret;
  }
  aw_cali_para.sensor_status = data[0];
  AW_LOGI(2,AW_TAG, "len = %d, sensor_status = 0x%x\n",
          len[0], aw_cali_para.sensor_status);

  return OK_FLAG;
}

/* Read chip temperature / temperature compensation coefficient */
static int aw_get_temp(void)
{
  int i = 0;
  int ret = -1;
  unsigned char data[TEMP_READ_LEN] = {0};

  ret = aw_i2c_reads(TEMP_READ_ADDR, data, TEMP_READ_LEN);
  if(ret < 0) {
    AW_LOGE(2,AW_TAG,"%s error, ret is %d\n", __func__, ret);
    return ERR_FLAG;
  }

  /* Temperature value */
  aw_cali_para.temp = (float)(((data[0]) | (data[1] << 8)) / 10.0);
  AW_LOGI(1,AW_TAG, "temperature = %d/10\n", (uint16_t)aw_cali_para.temp*10);

  /* Temperature compensation coefficient */
  for(i = 0; i < PHY_CH_NUM; i++) {
    aw_cali_para.temp_coef[i] =
            (float)(((data[4 + 2*i]) | (data[4 + 2*i + 1] << 8)) / 1000.0);
    AW_LOGI(2,AW_TAG, "temp_coef[%d] = %f\n", i, (double)aw_cali_para.temp_coef[i]);
  }
  return OK_FLAG;
}

int aw_get_thr(unsigned short thr[])
{
  int i = 0;
  int ret = -1;
  unsigned char data[THR_LEN] = {0};

  ret = aw_i2c_reads(THR_ADDR, data, THR_LEN);
  if(ret < 0) {
    AW_LOGE(2,AW_TAG,"%s error, ret is %d\n", __func__, ret);
    return ERR_FLAG;
  }

  aw8680x_delay_ms(5);
  for (i = 0; i < PHY_CH_NUM; i++) {
    thr[i] = data[0] | data[1] << 8;
    AW_LOGI(2,AW_TAG,"read thr[%d] = %d\n", i, thr[i]);
  }

  return OK_FLAG;
}

int aw_set_thr(unsigned short thr[])
{
  int i = 0;
  int count = 5;
  unsigned char reg_data[THR_LEN] = {0};
  unsigned short read_thr[PHY_CH_NUM] = {0};

  for (i = 0; i < PHY_CH_NUM; i++) {
    reg_data[i*2] = thr[i];
    reg_data[i*2+1] = thr[i] >> 8;
  }

  aw_i2c_writes(THR_ADDR, reg_data, THR_LEN);
  aw8680x_delay_ms(5);

  reg_data[0] = 0x5b;
  reg_data[1] = 0xa5;
  aw_i2c_writes(0x01, reg_data, 2);

  aw8680x_delay_ms(20);
  while(count--)
  {
    aw_i2c_reads(0x01, reg_data, 2);
    if((reg_data[0] == 0) && (reg_data[1] == 0)) {
      break;
    }
    aw8680x_delay_ms(10);
  }
  if((reg_data[0] != 0) || (reg_data[1] != 0)) {
    AW_LOGE(2,AW_TAG,"data:0x%x data:0x%x\n", reg_data[0], reg_data[1]);
    AW_LOGE(1,AW_TAG,"%s fail\n", __func__);
    return ERR_SETTHR;
  }

  aw_get_thr(read_thr);
  for (i = 0; i < PHY_CH_NUM; i++) {
    if (read_thr[i] != thr[i]) {
      AW_LOGE(2,AW_TAG,"read_thr[%d] = %d\n", i, read_thr[i]);
      return ERR_SETTHR;
    }
  }
  AW_LOGI(1,AW_TAG,"%s successful\n", __func__);

  return OK_FLAG;
}

static int aw_raw_read(unsigned char data[])
{
  int ret = -1;

  ret = aw_i2c_reads(RAW_DATA_ADDR, data, RAW_DATA_LEN);
  if(ret < 0) {
    AW_LOGE(2,AW_TAG,"%s error, ret is %d\n", __func__, ret);
    return ERR_FLAG;
  }
  return OK_FLAG;
}

/* Read calibration factor */
unsigned char aw8680x_read_cali_factor(void)
{
  int i = 0;
  int ret = -1;
  unsigned char reg_data[CALI_READ_LEN] = {0};

  ret = aw_i2c_reads(CALI_READ_ADDR, reg_data, CALI_READ_LEN);
  if(ret < 0) {
    AW_LOGE(2,AW_TAG,"%s error, ret is %d\n", __func__, ret);
    return ERR_FLAG;
  }

  for (i = 0; i < PHY_CH_NUM; i++) {
    aw_cali_para.read_cali_factor[i] =
                (float)((reg_data[i*2]|(reg_data[i*2+1] <<8)) / 1000.0);
    AW_LOGI(2,AW_TAG, "read_cali_factor[%d] = %d/1000\n", i,
            (uint16_t)aw_cali_para.read_cali_factor[i]*1000);
  }
  return OK_FLAG;
}

static int aw_write_cali_factor(void)
{
  int i = 0;
  unsigned char len_count = 5;
  unsigned char data[1] = {0};
  unsigned char len_data[1] = {0};
  unsigned char reg_data[PHY_CH_NUM*4+2] = {0};
  unsigned int cali_factor = 0;
  int cali_factor_sum = 0;

  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  aw_i2c_write(0xfb, 0x00);//Clear debug interface state

  for (i = 0; i < PHY_CH_NUM; i++) {
    cali_factor = (unsigned int)(aw_cali_para.cali_factor[i]*1000);
    cali_factor_sum += cali_factor;
    reg_data[i*4+0] = (unsigned char)(cali_factor);
    reg_data[i*4+1] = (unsigned char)(cali_factor >> 8);
    reg_data[i*4+2] = (unsigned char)(cali_factor >> 16);
    reg_data[i*4+3] = (unsigned char)(cali_factor >> 24);
  }
  cali_factor_sum = -cali_factor_sum;
  AW_LOGI(2,AW_TAG,"cali_factor_sum:0x%x %d\n", cali_factor_sum,(short)cali_factor_sum);

  reg_data[PHY_CH_NUM*4] = (unsigned char)(cali_factor_sum);
  reg_data[PHY_CH_NUM*4+1] = (unsigned char)(cali_factor_sum >> 8);

  for (i = 0; i < PHY_CH_NUM*4+2; i++) {
    AW_LOGI(1,AW_TAG,"reg_data[%d] = 0x%x\n", i, reg_data[i]);
  }

  aw_i2c_writes(0xfd, reg_data, PHY_CH_NUM*4+2);

  //Inform MCU of calibration coefficient data length
  aw_i2c_write(0xfc, PHY_CH_NUM*4+2);
  aw8680x_delay_ms(20);
  aw_i2c_write(0xfb, 0X30);//Inform MCU to accept calibration factor

  aw8680x_delay_ms(20);
  while(len_count--) {
    aw8680x_delay_ms(20);
    aw_i2c_reads(0xfc, len_data, 1);
    if (len_data[0] == 1) {
      break;
    }
  }
  if (len_data[0] == 0) {
    AW_LOGE(0,AW_TAG, "read len_data fail\n");
    return ERR_WFCTOR;
  }
  aw_i2c_reads(0xfd, data, 1);
  if (data[0] != 1) {
    AW_LOGE(0,AW_TAG, "write_cali_factor fail\n");
    aw8680x_read_cali_factor();
    return ERR_WFCTOR;
  }
  AW_LOGI(0,AW_TAG, "write_cali_factor success\n");
  aw8680x_read_cali_factor();
  aw_i2c_write(0xfb, 0x00);//Clear data length status

  return OK_FLAG;
}

/* GET the average raw value of each channel before pressing */
static int aw_get_unpress_rawavg(void)
{
  int ret = -1;
  int i = 0, j = 0;
  unsigned char rawdata[RAW_DATA_LEN] = {0};//PHY_CH_NUM*2
  short raw_data[PHY_CH_NUM][128] = {0};

  /* Read 128 groups of raw data */
  for (i = 0; i < 128; i++) {
    ret = aw_raw_read(rawdata);
    if (ret != OK_FLAG) {
      AW_LOGE(1,AW_TAG, "aw_raw_read fail, i = %d\n", i);
      return ERR_FLAG;
    }
    /* Two 8-bit data are combined into one 16 bit data */
    for (j = 0; j < PHY_CH_NUM; j++) {
      raw_data[j][i] = (short)(rawdata[j*2]|(rawdata[j*2+1] <<8));
    }
    //AW_LOGI(AW_TAG,"%-5d  %-5d\r\n", raw_data[0][i], raw_data[1][i]);
    aw8680x_delay_ms(20);
  }

  for (j = 0; j < PHY_CH_NUM; j++) {
    /* Get the sum of raw data of each channel */
    aw_cali_para.unpress_rawsum[j] = 0;
    for (i = 0; i < 128; i++) {
      aw_cali_para.unpress_rawsum[j] += raw_data[j][i];
    }
    AW_LOGI(2,AW_TAG,"unpress_rawsum[%d] = %d\r\n",j, aw_cali_para.unpress_rawsum[j]);

    /* Get the average value of each channe */
    aw_cali_para.unpress_rawavg[j] = aw_cali_para.unpress_rawsum[j] / 128;
    AW_LOGI(2,AW_TAG,"unpress_rawavg[%d] = %d\r\n",j, aw_cali_para.unpress_rawavg[j]);
  }
  return OK_FLAG;
}

/* GET the average raw value of each channel after pressing */
static int aw_get_press_rawavg(void)
{
  int ret = -1;
  int i = 0, j = 0;
  unsigned char rawdata[RAW_DATA_LEN] = {0};//PHY_CH_NUM*2
  short raw_data[PHY_CH_NUM][128] = {0};

  /* Read 128 groups of raw data */
  for (i = 0; i < 128; i++) {
    ret = aw_raw_read(rawdata);
    if (ret != OK_FLAG) {
      AW_LOGE(1,AW_TAG, "aw_raw_read fail, i = %d\n", i);
      return ERR_FLAG;
    }
    /* Two 8-bit data are combined into one 16 bit data */
    for (j = 0; j < PHY_CH_NUM; j++) {
      raw_data[j][i] = (short)(rawdata[j*2]|(rawdata[j*2+1] <<8));
    }
    //AW_LOGI(AW_TAG,"%-5d  %-5d\r\n", raw_data[0][i], raw_data[1][i]);
    aw8680x_delay_ms(20);
  }

  for (j = 0; j < PHY_CH_NUM; j++) {
    /* Get the sum of raw data of each channel */
    aw_cali_para.press_rawsum[j] = 0;
    for (i = 0; i < 128; i++) {
      aw_cali_para.press_rawsum[j] += raw_data[j][i];
    }
    AW_LOGI(2,AW_TAG,"press_rawsum[%d] = %d\r\n",j, aw_cali_para.press_rawsum[j]);

    /* Get the average value of each channe */
    aw_cali_para.press_rawavg[j] = aw_cali_para.press_rawsum[j] / 128;
    AW_LOGI(2,AW_TAG,"press_rawavg[%d] = %d\r\n",j, aw_cali_para.press_rawavg[j]);
  }
  return OK_FLAG;
}

/* Calibration preparation phase,
 * return to OK_ Flag indicates that
 * the preparation is completed and the press is started. */
int aw_cali_ready(void)
{
  int ret = -1;

  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  /* Check firmware version */
  ret = aw_get_firmware_version();
  if(ret != OK_FLAG) {
    return ret;
  }
  if (aw_cali_para.firmware_version != aw_cali_para.bin_version) {
    AW_LOGE(0,AW_TAG,"firmware error\n");
    return ERR_FW;
  }
  AW_LOGI(0,AW_TAG,"firmware ok\n");

  /* Check sensor connection status */
  ret = aw_detect_sensor_status();
  if(ret != OK_FLAG) {
    return ret;
  }
  if (aw_cali_para.sensor_status != 0) {
    AW_LOGE(0,AW_TAG,"sensor connect error\n");
    return ERR_SENSOR;
  }
  AW_LOGI(0,AW_TAG,"sensor connect ok\n");

  /* GET temp and temp coef */
  ret = aw_get_temp();
  if(ret != OK_FLAG) {
    return ret;
  }
  /* GET calibration factor */
  ret = aw8680x_read_cali_factor();
  if(ret != OK_FLAG) {
    return ret;
  }

  /* GET the average raw value before pressing */
  ret = aw_get_unpress_rawavg();
  if(ret != OK_FLAG) {
    return ret;
  }
  AW_LOGI(0,AW_TAG,"ready to complete, start pressing\n");
  return OK_FLAG;
}

/* Press stage, wait for pressing,
 * and wait for press to complete,
 * return to OK_ Flag means press complete, start up */
int aw_cali_press(void)
{
  int i = 0;
  int ret = -1;
  short raw_data = 0;
  unsigned char rawdata[RAW_DATA_LEN] = {0};

  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  AW_LOGI(0,AW_TAG,"wait for press\n");
  /* Wait for press */
  while (raw_data < AW8680X_PRESS_ADC*PHY_CH_NUM) {
    aw_raw_read(rawdata);
    raw_data = 0;
    for (i = 0; i < PHY_CH_NUM; i++) {
      raw_data += (short)(rawdata[i*2]|(rawdata[i*2+1] <<8));
    }
    aw8680x_delay_ms(20);
  }
  aw8680x_delay_ms(1000);

  AW_LOGI(0,AW_TAG,"pressing...\n");
  /* GET the average raw value after pressing */
  ret = aw_get_press_rawavg();
  if(ret != OK_FLAG) {
    return ret;
  }

  AW_LOGI(0,AW_TAG,"press complete, wait up\n");
  return OK_FLAG;
}

/* Wait for the lifting stage,
 * calculate the calibration coefficient first,
 * then write the calibration coefficient,
 * and finally wait for lifting,
 * return to OK_ Flag indicates that the
 * calibration is completed and the test is started */
int aw_cali_wait_up(void)
{
  int i = 0;
  int ret = -1;
  short raw_data = 1000;
  unsigned char rawdata[RAW_DATA_LEN] = {0};
  unsigned int cali_factor = 0;

  for (i = 0; i < PHY_CH_NUM; i++) {
    aw_cali_para.cali_factor[i] =
      (float)(cali_weight/(aw_cali_para.press_rawavg[i] - aw_cali_para.unpress_rawavg[i]));
    cali_factor = (unsigned int)(aw_cali_para.cali_factor[i]*1000);
    aw_cali_para.cali_factor[i] = (float)(cali_factor/1000.0);
    AW_LOGI(2,AW_TAG,"cali_factor[%d] = %d/1000\n", i, (uint16_t)aw_cali_para.cali_factor[i]*1000);
  }

  ret = aw_write_cali_factor();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_write_cali_factor fail, ret = %d\n", ret);
    return ret;
  }
  AW_LOGI(0,AW_TAG,"wait up\n");
  while (raw_data > 50*PHY_CH_NUM) {
    aw_raw_read(rawdata);
    raw_data = 0;
    for (i = 0; i < PHY_CH_NUM; i++) {
      raw_data += (short)(rawdata[i*2]|(rawdata[i*2+1] <<8));
    }
    aw8680x_delay_ms(20);
  }
  aw8680x_delay_ms(1000);

  AW_LOGI(0,AW_TAG,"calibration complete, start testing\n");
  return OK_FLAG;
}

/* Calibration test preparation stage */
int aw_test_ready(void)
{
  int i = 0;
  int ret = -1;

  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  /* GET calibration factor */
  ret = aw8680x_read_cali_factor();
  if(ret != OK_FLAG) {
    return ret;
  }
  for (i = 0; i < PHY_CH_NUM; i++) {
    if (aw_cali_para.read_cali_factor[i] != aw_cali_para.cali_factor[i]) {
      AW_LOGE(1,AW_TAG,"calibration factor write error, i = %d\n", i);
      return ERR_CFCTOR;
    }
  }
  AW_LOGI(0,AW_TAG,"calibration factor check ok\n");

  /* GET the average raw value before pressing */
  ret = aw_get_unpress_rawavg();
  if(ret != OK_FLAG) {
    return ret;
  }
  AW_LOGI(0,AW_TAG,"ready to complete, start test pressing\n");
  return OK_FLAG;
}

/* Press stage of calibration test */
int aw_test_press(void)
{
  int i = 0;
  int ret = -1;
  short raw_data = 0;
  unsigned char rawdata[RAW_DATA_LEN] = {0};

  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  AW_LOGI(0,AW_TAG,"wait for test press\n");
  while (raw_data < AW8680X_PRESS_ADC*PHY_CH_NUM) {
    aw_raw_read(rawdata);
    raw_data = 0;
    for (i = 0; i < PHY_CH_NUM; i++) {
      raw_data += (short)(rawdata[i*2]|(rawdata[i*2+1] <<8));
    }
    aw8680x_delay_ms(20);
  }
  aw8680x_delay_ms(1000);

  AW_LOGI(0,AW_TAG,"test pressing...\n");
  /* GET the average raw value after pressing */
  ret = aw_get_press_rawavg();
  if(ret != OK_FLAG) {
    return ret;
  }

  for (i = 0; i < PHY_CH_NUM; i++) {
    aw_cali_para.deviation[i] =
      (float)((((aw_cali_para.press_rawavg[i] - aw_cali_para.unpress_rawavg[i])*
                aw_cali_para.cali_factor[i]) - test_weight) / test_weight);

    AW_LOGI(2,AW_TAG,"deviation[%d] = %d/1000\n",
            i, (uint16_t)(aw_cali_para.deviation[i]*1000));
  }

  for (i = 0; i < PHY_CH_NUM; i++) {
    if ((aw_cali_para.deviation[i]*100 < 20) && (aw_cali_para.deviation[i]*100 > -20)) {
      AW_LOGI(1,AW_TAG, "deviation[%d] is ok\n", i);
    } else {
      AW_LOGE(1,AW_TAG, "deviation[%d] is fail\n", i);
      return ERR_CALITEST;
    }
  }

  AW_LOGI(0,AW_TAG,"wait test up\n");
  return OK_FLAG;
}

/* Calibration test waiting for lift up phase */
int aw_test_wait_up(void)
{
  int i = 0;
  short raw_data = 1000;
  unsigned char rawdata[RAW_DATA_LEN] = {0};

  while (raw_data > 50*PHY_CH_NUM) {
    aw_raw_read(rawdata);
    raw_data = 0;
    for (i = 0; i < PHY_CH_NUM; i++) {
      raw_data += (short)(rawdata[i*2]|(rawdata[i*2+1] <<8));
    }
    aw8680x_delay_ms(20);
  }
  aw8680x_delay_ms(1000);

  AW_LOGI(0,AW_TAG,"calibration test complete\n");
  return OK_FLAG;
}

int aw_calibration(void)
{
  int ret = -1;

  //printf("\r\n");
  AW_LOGI(1,AW_TAG,"%s enter\n", __func__);

  ret = aw_cali_ready();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_cali_ready fail,ret = %d\n", ret);
    return ret;
  }

  ret = aw_cali_press();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_cali_press fail,ret = %d\n", ret);
    return ret;
  }

  ret = aw_cali_wait_up();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_cali_wait_up fail,ret = %d\n", ret);
    return ret;
  }

  ret = aw_test_ready();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_test_ready fail,ret = %d\n", ret);
    return ret;
  }

  ret = aw_test_press();
  if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_test_press fail,ret = %d\n", ret);
    return ret;
  }

  ret = aw_test_wait_up();
   if(ret != OK_FLAG) {
    AW_LOGE(1,AW_TAG, "aw_test_wait_up fail,ret = %d\n", ret);
    return ret;
  }

  return OK_FLAG;

}
