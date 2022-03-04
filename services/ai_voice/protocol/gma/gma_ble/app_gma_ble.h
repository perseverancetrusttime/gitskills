/**
 ****************************************************************************************
 *
 * @file app_gma_ble.h
 *
 * @brief Gma Voice Application entry point
 *
 * Copyright (C) BES
 *
 *
 ****************************************************************************************
 */

#ifndef APP_GMA_BLE_H_
#define APP_GMA_BLE_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#ifdef __GMA_VOICE__
#ifndef BLE_V2

#include <stdint.h>          // Standard Integer Definition

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t     gma_gatt_adv_lenth;
    uint8_t     gma_gatt_adv_type;
    uint8_t     gma_gatt_adv_cid[2];
    uint8_t     gma_gatt_adv_vid;
    uint8_t     gma_gatt_adv_fmsk;
    uint8_t     gma_gatt_adv_pid[4];
    uint8_t     gma_gatt_adv_mac[6];
    uint8_t     gam_gatt_adv_extend[2];
}__attribute__ ((__packed__)) GMA_GATT_ADV_DATA_PAC;

extern GMA_GATT_ADV_DATA_PAC gma_gatt_adv_data;
/**
 ****************************************************************************************
 * @brief Add a DataPath Server instance in the DB
 ****************************************************************************************
 */
void app_gma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);

void app_gma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);

void app_gma_send_data_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);

void app_gma_send_data_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id);


#ifdef __cplusplus
    }
#endif

#endif
#endif //__GMA_VOICE__

/// @} APP

#endif // APP_GMA_BLE_H_
