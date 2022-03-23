/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2017 InvenSense Inc. All rights reserved.
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
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_types.h"
#include "example_raw.h"

#include "inv_imu_extfunc.h"

#include "Message.h"
#include "RingBuffer.h"

/*
 * Print raw data or scaled data
 * 0 : print raw accel, gyro and temp data
 * 1 : print scaled accel, gyro and temp data in g, dps and degree Celsius
 */
#define SCALED_DATA_G_DPS 0

/* --------------------------------------------------------------------------------------
 *  Static and extern variables
 * -------------------------------------------------------------------------------------- */

/* Just a handy variable to handle the IMU object */
static struct inv_imu_device icm_driver;

#if !USE_FIFO
/* Buffer to keep track of the timestamp when IMU data ready interrupt fires. */
extern RINGBUFFER(timestamp_buffer, 64, uint64_t);
#endif

/*
 * ICM mounting matrix
 * Coefficients are coded as Q30 integer
 */
static int32_t icm_mounting_matrix[9] = {0, -(1 << 30), 0, (1 << 30), 0, 0, 0, 0, (1 << 30)};

/* --------------------------------------------------------------------------------------
 *  static function declaration
 * -------------------------------------------------------------------------------------- */
static void apply_mounting_matrix(const int32_t matrix[9], int32_t raw[3]);

/* --------------------------------------------------------------------------------------
 *  Functions definition
 * -------------------------------------------------------------------------------------- */

int setup_imu_device(struct inv_imu_serif *icm_serif)
{
    int rc = 0;
    uint8_t who_am_i;

    /* Init device */
    rc = inv_imu_init(&icm_driver, icm_serif, imu_callback);
    if (rc != INV_ERROR_SUCCESS) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Failed to initialize IMU!");
        return rc;
    }

    /* Check WHOAMI */
    rc = inv_imu_get_who_am_i(&icm_driver, &who_am_i);
    if (rc != INV_ERROR_SUCCESS) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Failed to read whoami!");
        return rc;
    }

    if (who_am_i != ICM_WHOAMI) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Bad WHOAMI value!");
        INV_MSG(INV_MSG_LEVEL_ERROR, "Read 0x%02x, expected 0x%02x", who_am_i, ICM_WHOAMI);
        return INV_ERROR;
    }

#if !USE_FIFO
    RINGBUFFER_CLEAR(&timestamp_buffer);
#endif

    return rc;
}

int configure_imu_device()
{
    int rc = 0;

    if (!USE_FIFO)
        rc |= inv_imu_configure_fifo(&icm_driver, INV_IMU_FIFO_DISABLED);

    if (USE_HIGH_RES_MODE) {
        rc |= inv_imu_enable_high_resolution_fifo(&icm_driver);
    } else {
        rc |= inv_imu_set_accel_fsr(&icm_driver, ACCEL_CONFIG0_FS_SEL_4g);
        rc |= inv_imu_set_gyro_fsr(&icm_driver, GYRO_CONFIG0_FS_SEL_2000dps);
    }

    if (USE_LOW_NOISE_MODE) {
        rc |= inv_imu_set_accel_frequency(&icm_driver, ACCEL_CONFIG0_ODR_800_HZ);
        rc |= inv_imu_set_gyro_frequency(&icm_driver, GYRO_CONFIG0_ODR_800_HZ);
        rc |= inv_imu_enable_accel_low_noise_mode(&icm_driver);
        //Note(Mark):enable gyro LN mode only in accel LN mode
        rc |= inv_imu_enable_gyro_low_noise_mode(&icm_driver);
    } else {
        rc |= inv_imu_set_accel_frequency(&icm_driver, ACCEL_CONFIG0_ODR_100_HZ);
        rc |= inv_imu_set_gyro_frequency(&icm_driver, GYRO_CONFIG0_ODR_100_HZ);
        rc |= inv_imu_enable_accel_low_power_mode(&icm_driver);
    }

    if (!USE_FIFO)
        inv_imu_sleep_us(GYR_STARTUP_TIME_US);

    return rc;
}

int get_imu_data(void)
{
#if USE_FIFO
    return inv_imu_get_data_from_fifo(&icm_driver);
#else
    return inv_imu_get_data_from_registers(&icm_driver);
#endif
}

#if SCALED_DATA_G_DPS
static void get_accel_and_gyr_fsr(uint16_t *accel_fsr_g, uint16_t *gyro_fsr_dps)
{
    ACCEL_CONFIG0_FS_SEL_t accel_fsr_bitfield;
    GYRO_CONFIG0_FS_SEL_t gyro_fsr_bitfield;

    inv_imu_get_accel_fsr(&icm_driver, &accel_fsr_bitfield);
    switch (accel_fsr_bitfield) {
        case ACCEL_CONFIG0_FS_SEL_2g:
            *accel_fsr_g = 2;
            break;
        case ACCEL_CONFIG0_FS_SEL_4g:
            *accel_fsr_g = 4;
            break;
        case ACCEL_CONFIG0_FS_SEL_8g:
            *accel_fsr_g = 8;
            break;
        case ACCEL_CONFIG0_FS_SEL_16g:
            *accel_fsr_g = 16;
            break;
        default:
            *accel_fsr_g = -1;
    }

    inv_imu_get_gyro_fsr(&icm_driver, &gyro_fsr_bitfield);
    switch (gyro_fsr_bitfield) {
        case GYRO_CONFIG0_FS_SEL_250dps:
            *gyro_fsr_dps = 250;
            break;
        case GYRO_CONFIG0_FS_SEL_500dps:
            *gyro_fsr_dps = 500;
            break;
        case GYRO_CONFIG0_FS_SEL_1000dps:
            *gyro_fsr_dps = 1000;
            break;
        case GYRO_CONFIG0_FS_SEL_2000dps:
            *gyro_fsr_dps = 2000;
            break;
        default:
            *gyro_fsr_dps = -1;
    }
}
#endif

void imu_callback(inv_imu_sensor_event_t *event)
{
    uint64_t timestamp;
    int32_t accel[3], gyro[3];
#if SCALED_DATA_G_DPS
    float accel_g[3];
    float gyro_dps[3];
    float temp_degc;
    uint16_t accel_fsr_g, gyro_fsr_dps;
#endif

#if USE_FIFO
    static uint64_t last_fifo_timestamp = 0;
    static uint32_t rollover_num = 0;

    // Handle rollover
    if (last_fifo_timestamp > event->timestamp_fsync)
        rollover_num++;
    last_fifo_timestamp = event->timestamp_fsync;

    // Compute timestamp in us
    timestamp = event->timestamp_fsync + rollover_num * UINT16_MAX;
    timestamp *= inv_imu_get_fifo_timestamp_resolution_us_q24(&icm_driver);
    timestamp /= (1UL << 24);

    if (icm_driver.fifo_highres_enabled) {
        accel[0] = (((int32_t)event->accel[0] << 4)) | event->accel_high_res[0];
        accel[1] = (((int32_t)event->accel[1] << 4)) | event->accel_high_res[1];
        accel[2] = (((int32_t)event->accel[2] << 4)) | event->accel_high_res[2];

        gyro[0] = (((int32_t)event->gyro[0] << 4)) | event->gyro_high_res[0];
        gyro[1] = (((int32_t)event->gyro[1] << 4)) | event->gyro_high_res[1];
        gyro[2] = (((int32_t)event->gyro[2] << 4)) | event->gyro_high_res[2];

    } else {
        accel[0] = event->accel[0];
        accel[1] = event->accel[1];
        accel[2] = event->accel[2];

        gyro[0] = event->gyro[0];
        gyro[1] = event->gyro[1];
        gyro[2] = event->gyro[2];
    }
#else
    //TODO(Mike):check if disable all irq,when operation RingBuffer
    //inv_disable_irq();
    uint32_t lock = int_lock();
    if (!RINGBUFFER_EMPTY(&timestamp_buffer))
        RINGBUFFER_POP(&timestamp_buffer, &timestamp);
    //inv_enable_irq();
    int_unlock(lock);

    accel[0] = event->accel[0];
    accel[1] = event->accel[1];
    accel[2] = event->accel[2];

    gyro[0] = event->gyro[0];
    gyro[1] = event->gyro[1];
    gyro[2] = event->gyro[2];

    // Force sensor_mask so it gets displayed below
    event->sensor_mask |= (1 << INV_SENSOR_TEMPERATURE);
    event->sensor_mask |= (1 << INV_SENSOR_ACCEL);
    event->sensor_mask |= (1 << INV_SENSOR_GYRO);
#endif

    apply_mounting_matrix(icm_mounting_matrix, accel);
    apply_mounting_matrix(icm_mounting_matrix, gyro);

#if SCALED_DATA_G_DPS
    /*
	 * Convert raw data into scaled data in g and dps
	*/
    get_accel_and_gyr_fsr(&accel_fsr_g, &gyro_fsr_dps);
    accel_g[0] = (float)(accel[0] * accel_fsr_g) / INT16_MAX;
    accel_g[1] = (float)(accel[1] * accel_fsr_g) / INT16_MAX;
    accel_g[2] = (float)(accel[2] * accel_fsr_g) / INT16_MAX;
    gyro_dps[0] = (float)(gyro[0] * gyro_fsr_dps) / INT16_MAX;
    gyro_dps[1] = (float)(gyro[1] * gyro_fsr_dps) / INT16_MAX;
    gyro_dps[2] = (float)(gyro[2] * gyro_fsr_dps) / INT16_MAX;
    if (USE_HIGH_RES_MODE || !USE_FIFO)
        temp_degc = 25 + ((float)event->temperature / 128);
    else
        temp_degc = 25 + ((float)event->temperature / 2);

    /*
	 * Output scaled data on UART link
	 */
    if (event->sensor_mask & (1 << INV_SENSOR_ACCEL) && event->sensor_mask & (1 << INV_SENSOR_GYRO))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", (uint32_t)timestamp,
                (double)accel_g[0], (double)accel_g[1], (double)accel_g[2], (double)temp_degc, (double)gyro_dps[0],
                (double)gyro_dps[1], (double)gyro_dps[2]);
    else if (event->sensor_mask & (1 << INV_SENSOR_GYRO))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: NA, NA, NA, %.3f, %.3f, %.3f, %.3f", (uint32_t)timestamp, (double)temp_degc,
                (double)gyro_dps[0], (double)gyro_dps[1], (double)gyro_dps[2]);
    else if (event->sensor_mask & (1 << INV_SENSOR_ACCEL))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: %.3f, %.3f, %.3f, %.3f, NA, NA, NA", (uint32_t)timestamp, (double)accel_g[0],
                (double)accel_g[1], (double)accel_g[2], (double)temp_degc);
#else

    /*
	 * Output raw data on UART link
	 */
    if (event->sensor_mask & (1 << INV_SENSOR_ACCEL) && event->sensor_mask & (1 << INV_SENSOR_GYRO))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: %d, %d, %d, %d, %d, %d, %d", (uint32_t)timestamp, accel[0], accel[1], accel[2],
                event->temperature, gyro[0], gyro[1], gyro[2]);
    else if (event->sensor_mask & (1 << INV_SENSOR_GYRO))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: NA, NA, NA, %d, %d, %d, %d", (uint32_t)timestamp, event->temperature, gyro[0],
                gyro[1], gyro[2]);
    else if (event->sensor_mask & (1 << INV_SENSOR_ACCEL))
        INV_MSG(INV_MSG_LEVEL_INFO, "%u: %d, %d, %d, %d, NA, NA, NA", (uint32_t)timestamp, accel[0], accel[1], accel[2],
                event->temperature);
#endif
}

/* --------------------------------------------------------------------------------------
 *  Static functions definition
 * -------------------------------------------------------------------------------------- */

static void apply_mounting_matrix(const int32_t matrix[9], int32_t raw[3])
{
    unsigned i;
    int64_t data_q30[3];

    for (i = 0; i < 3; i++) {
        data_q30[i] = ((int64_t)matrix[3 * i + 0] * raw[0]);
        data_q30[i] += ((int64_t)matrix[3 * i + 1] * raw[1]);
        data_q30[i] += ((int64_t)matrix[3 * i + 2] * raw[2]);
    }
    raw[0] = (int32_t)(data_q30[0] >> 30);
    raw[1] = (int32_t)(data_q30[1] >> 30);
    raw[2] = (int32_t)(data_q30[2] >> 30);
}
