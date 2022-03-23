/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2016-2016 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively �Software�) is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */
#ifndef _EXAMPLE_RAW_FIFO_H_
#define _EXAMPLE_RAW_FIFO_H_

#include <stdint.h>
#include "inv_imu_transport.h"
#include "inv_imu_defs.h"
#include "inv_imu_driver.h"

/*** Example configuration ***/

/* 
 * Select communication link between SmartMotion and IMU 
 */
//#define SERIF_TYPE           UI_SPI4
#define SERIF_TYPE UI_I2C

/*
 * Set power mode flag
 * Set this flag to run example in low-noise mode.
 * Reset this flag to run example in low-power mode.
 * Note: low-noise mode is not available with sensor data frequencies less than 12.5Hz.
 */
#define USE_LOW_NOISE_MODE 0

/*
 * Select Fifo resolution Mode (default is low resolution mode)
 * Low resolution mode: 16 bits data format
 * High resolution mode: 20 bits data format
 * Warning: Enabling High Res mode will force FSR to 16g and 2000dps
 */
#define USE_HIGH_RES_MODE 0

/*
 * Select to use FIFO or to read data from registers
 */
#define USE_FIFO 1

/**
 * \brief This function is in charge of reseting and initializing IMU device. It should
 * be successfully executed before any access to IMU device.
 *
 * \return 0 on success, negative value on error.
 */
int setup_imu_device(struct inv_imu_serif *icm_serif);

/**
 * \brief This function configures the device in order to output gyro and accelerometer.
 * \return 0 on success, negative value on error.
 */
int configure_imu_device();

/**
 * \brief This function extracts data from the IMU FIFO.
 * \return 0 on success, negative value on error.
 */
int get_imu_data(void);

/**
 * \brief This function is the custom handling packet function.
 * \param[in] event structure containing sensor data from one packet
 */
void imu_callback(inv_imu_sensor_event_t *event);

#endif /* !_EXAMPLE_RAW_FIFO_H_ */
