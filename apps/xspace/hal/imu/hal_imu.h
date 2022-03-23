#ifndef __HAL_IMU_H__
#define __HAL_IMU_H__

#if defined(__IMU_SUPPORT__)
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
//#include "hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_imu_get_rawdata_cb)(uint8_t data);

typedef struct
{      
    int32_t (* imu_init)(void);
    int32_t (* imu_get_chip_id)(uint32_t *chip_id);
    //TODO(Mike):add more hal api here
} hal_imu_ctx_s;

/**
 ****************************************************************************************
 * @brief   initialize imu hal interface, create temp imu object
 * @note    None
 * @return  return 1/0, 1: success, otherwise：failed
 ****************************************************************************************
 */
int32_t hal_imu_init(void);

/**
 ****************************************************************************************
 * @brief   get imu i2c chip id
 * @note    None
 *  
 * @param[in] chip_id : pointer which will return addr of chip_id. 
 * @return  return 1/0, 1: success, otherwise：failed
 ****************************************************************************************
 */
int32_t hal_imu_get_chip_id(uint32_t *chip_id);
#ifdef __cplusplus
}
#endif

#endif //__IMU_SUPPORT__

#endif //__HAL_IMU_H__

