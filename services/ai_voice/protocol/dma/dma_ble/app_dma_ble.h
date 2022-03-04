#ifndef APP_DMA_BLE_H_
#define APP_DMA_BLE_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Smart Voice Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>          // Standard Integer Definition

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief Add a DataPath Server instance in the DB
 ****************************************************************************************
 */
void app_ai_add_dma(void);

void app_dma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx);

void app_dma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx);

bool app_dma_send_data_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx);

void app_dma_send_data_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx);

#ifdef __cplusplus
}
#endif


/// @} APP

#endif // APP_DMA_VOICE_H_
