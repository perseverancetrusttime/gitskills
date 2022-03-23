#include "hal_imu.h"
#include "hal_error.h"


#if defined(__IMU_SUPPORT__)

/*********    Variable declaration area START   *********/

#ifdef __IMU_TDK42607P__
extern const hal_imu_ctx_s tdk42607p_ctrl;
#endif

hal_imu_ctx_s *p_imu_ctx = NULL;
/*********    Variable declaration area  END    *********/


/*************       Function  area START   *************/
int32_t hal_imu_init(void)
{
#if defined(__IMU_TDK42607P__)
    p_imu_ctx = (hal_imu_ctx_s *)&tdk42607p_ctrl;
    if(!p_imu_ctx->imu_init()) {
        return HAL_RUN_SUCCESS;
    }
#endif

//TODO(Mike): add others imu chip series
    p_imu_ctx = NULL;
    return HAL_INIT_ERROR;
}

int32_t hal_imu_get_chip_id(uint32_t *chip_id)
{
    if (p_imu_ctx != NULL && p_imu_ctx->imu_get_chip_id != NULL) {
        if(!p_imu_ctx->imu_get_chip_id(chip_id))
            return HAL_RUN_SUCCESS;
        else
            return HAL_RUN_FAIL;
    }
    
    return HAL_INVALID_PARAMETER;
}

//TODO(Mike):add more feature func here
#endif

