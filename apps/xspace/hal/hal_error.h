#ifndef __HAL_ERROR_H__
#define __HAL_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HAL_RUN_SUCCESS = 0,
    HAL_RUN_FAIL = 1,
    HAL_INIT_ERROR = 2,
    HAL_INVALID_PARAMETER = 3,
    HAL_IR_CALIB_ERROR = 4,
} hal_error_e;

#ifdef __cplusplus
    }
#endif

#endif  /* __HAL_ERROR_H__ */
