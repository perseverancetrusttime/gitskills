#ifndef __XSPACE_CRC16_H__
#define __XSPACE_CRC16_H__

#include "stdint.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRC16_MODEL_CCITT_FALSE,
    CRC16_MODEL_XMODEM,

    CRC16_MODEL_QTY,
} crc16_model_e;

#ifndef XSPACE_CRC_INIT
#define XSPACE_CRC_INIT (0xFFFF)
#endif

/**
 ****************************************************************************************
 * @brief Follow the principle:CRC-16/CCITT-FALSE
 *
 * @note  x16 + x12 + x5 + 1
 *        WIDTH:16;Policy:1021;INIT:crc;
 *        REFIN:false;REFOUT:false;XOROUT:0x0000
 *
 * @param[in] buffer: data which will be calculated
 * @param[in] size: data length
 * @param[in] crc crc init value
 *
 * 
 * @return  return crc16 calculate result
 ****************************************************************************************
 */
uint16_t xspace_crc16_ccitt_false(const uint8_t *buffer, uint32_t size, uint16_t crc);

/**
 ****************************************************************************************
 * @brief Follow the principle:CRC-16/CCITT-FALSE
 *
 * @note  x16 + x12 + x5 + 1
 *        WIDTH:16;Policy:1021;INIT:0x0000;
 *        REFIN:false;REFOUT:false;XOROUT:0x0000
 *
 *
 * @param[in] buffer: data which will be calculated
 * @param[in] size: data length
 *
 * 
 * @return  return crc16 calculate result
 ****************************************************************************************
 */
uint16_t xspace_crc16_xmodem(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif /* __XSPACE_CRC16_H__ */
