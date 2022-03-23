#ifndef __XSPACE_IMU_MANAGER_H__
#define __XSPACE_IMU_MANAGER_H__

#if defined(__XSPACE_IMU_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#if defined(__XSPACE_IMU_DEBUG__)
#define IMU_TRACE(num, str, ...)  TRACE(num + 1, "[IMU]%s," str, __func__, ##__VA_ARGS__)
#else
#define IMU_TRACE(num, str, ...)
#endif

/**
 ****************************************************************************************
 * @brief   initialize imu manager 
 * @note    None
 * @return  return 1/0, 1: success, 0：failed
 ****************************************************************************************
 */
int32_t xspace_imu_manager_init(void);

/**
 ****************************************************************************************
 * @brief   start imu feature, let imu driver chip into workable mode
 * @note    None
 * @return  None
 ****************************************************************************************
 */
void xspace_imu_manager_start(void);

/**
 ***************************************************************************************
 * @brief   stop imu feature, let imu driver into disable mode.
 * @note    None
 * @return  None
 ****************************************************************************************/
void xspace_inear_detect_manager_stop(void);
#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_IMU_MANAGER__)

#endif  // (__XSPACE_IMU_MANAGER_H__)

