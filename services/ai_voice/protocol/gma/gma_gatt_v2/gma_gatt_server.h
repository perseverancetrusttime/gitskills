#ifndef _TENCENT_GATT_SERVER_H_
#define _TENCENT_GATT_SERVER_H_

/**
 ****************************************************************************************
 * @addtogroup TENCENT_SMARTVOICETASK Task
 * @ingroup TENCENT_SMARTVOICE
 * @brief Smart Voice Profile Task.
 *
 * The TENCENT_SMARTVOICETASK is responsible for handling the messages coming in and out of the
 * @ref TENCENT_SMARTVOICE collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */



/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ke_task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMA_GATT_MAX_ATTR_LEN   255


///Attributes State Machine
enum {
    GMA_IDX_SVC,    //0

    GMA_IDX_READ_CHAR,   //1
    GMA_IDX_READ_VAL,    //2

    GMA_IDX_WRITE_CHAR,   //3
    GMA_IDX_WRITE_VAL,    //4

    GMA_IDX_INDICATE_CHAR,    //5
    GMA_IDX_INDICATE_VAL,   //6
    GMA_IDX_INDICATE_CFG,    //7

    GMA_IDX_WR_NO_RSP_CHAR,   //8
    GMA_IDX_WR_NO_RSP_VAL,    //9

    GMA_IDX_NTF_CHAR,      //10
    GMA_IDX_NTF_VAL,      //11
    GMA_IDX_NTF_CFG,    //12

    GMA_IDX_NB,

};

/// Messages for Smart Voice Profile
enum tencent_sv_msg_id {
    GMA_CMD_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_AI),

    GMA_TX_DATA_SENT,
    
    GMA_CMD_RECEIVED,

    GMA_SEND_CMD_VIA_NOTIFICATION,

    GMA_SEND_CMD_VIA_INDICATION,

    GMA_DATA_RECEIVED,

    GMA_SEND_DATA_VIA_NOTIFICATION,

    GMA_SEND_DATA_VIA_INDICATION,

};

/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void gma_task_init(struct ke_task_desc *task_desc);

bool app_gma_send_cmd_via_notification(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);
bool app_gma_send_data_via_notification(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);

bool app_gma_send_cmd_via_indication(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);
bool app_gma_send_data_via_indication(uint8_t* ptrData, uint32_t length,uint8_t ai_index, uint8_t device_id);

void app_ai_ble_add_gma(void);

#ifdef __cplusplus
}
#endif


/// @} 

#endif /* _TENCENT_SMARTVOICE_TASK_H_ */

