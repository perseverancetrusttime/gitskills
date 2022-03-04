#ifndef __XSPACE_XBUS_OTA_H__
#define __XSPACE_XBUS_OTA_H__
#ifdef __XSPACE_XBUS_OTA__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XBUS_OTA_FRAME_NEXT,
    XBUS_OTA_FRAME_RETRANS,
    XBUS_OTA_FINISHED,
    XBUS_OTA_FAILED,

    XBUS_OTA_QTY,
} xbus_ota_err_e;

typedef enum {
    XBUS_OTA_FRAME_HEAD = 0,
    XBUS_OTA_FRAME_CMD = 1,
    XBUS_OTA_FRAME_DATA_LEN = 2,
    XBUS_OTA_FRAME_DATA = 3,
    XBUS_OTA_FRAME_CRC16,

    XBUS_FRAME_QTY,
} xbus_ota_cmd_frame_e;

/**
 ****************************************************************************************
 * @brief set xbus ota file size
 *
 * @note fw_size was setted by xbus cmd (detail useing at
 *:xbm_cmd_start_xbus_fwu_handler())
 *
 * @param[in] fw_size        ota_bin size
 *
 *
 * @return  default return 0
 ****************************************************************************************
 */
int32_t xbus_ota_fw_size_set(uint32_t fw_size);

/**
 ****************************************************************************************
 * @brief get xbus ota file size
 *
 * @note fw_size was setted by xbus cmd (detail useing at
 *:xbm_cmd_start_xbus_fwu_handler()) ,get the ota file size whenever you want
 *
 *
 * @param[in] inputparam addr for fw_size
 *
 *
 * @return  default return 0
 ****************************************************************************************
 */
int32_t xbus_ota_fw_size_get(uint32_t fw_size);

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
/**
 ****************************************************************************************
 * @brief check APP_BIN integeality
 *
 * @note  after XBUS_OTA_FINISHED, if open __APP_BIN_INTEGRALITY_CHECK__
 *         we need use this func to check app_bin
 *

 * @param[in] None
 *
 *
 * @return  success:0; fail:return -1
 ****************************************************************************************
 */
int32_t xbus_ota_sha256_check(void);
#endif

/**
 ****************************************************************************************
 * @brief receive ota frame from PC tool which send ota frame packge to earbuds
 *
 * @note  more detail handles at "xbm_cmd_fwu_data_process_handler()"
 *
 * @param[in] frame unsigned char* pointer which point ota frame raw data
 * @param[in] len frame packge lengths
 *
 *
 * @return  return type "xbus_ota_err_e"
 ****************************************************************************************
 */
xbus_ota_err_e xbus_ota_frame_receive(uint8_t *frame, uint32_t len);

/**
 ****************************************************************************************
 * @brief receive ota frame from PC tool which send ota frame packge to earbuds
 *
 * @note  more detail handles at "xbm_cmd_fwu_data_process_handler()"
 *
 * @param[in] frame unsigned char* pointer which point ota frame raw data
 * @param[in] len ota frame packge lengths
 *
 *
 * @return  return type "xbus_ota_err_e"
 ****************************************************************************************
 */
#ifdef __cplusplus
}
#endif

#endif /* __XSPACE_XBUS_OTA__ */
#endif /* __XSPACE_XBUS_OTA_H__*/
