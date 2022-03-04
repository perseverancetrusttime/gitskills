#ifndef _AMA_GATT_SERVER_H_
#define _AMA_GATT_SERVER_H_

#ifdef __AI_VOICE_BLE_ENABLE__

/**
 ****************************************************************************************
 * @addtogroup AMA Datapath Profile Server
 * @ingroup AMA
 * @brief ama Profile Server
 *
 * Datapath Profile Server provides functionalities to upper layer module
 * application. The device using this profile takes the role of Datapath Server.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Send data to peer device via notifications  (from APP)
 *  - Receive data from peer device via write no response (from APP)
 *
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */



#include "ke_task.h"

#define DMA_MAX_LEN                 (244)


///Attributes State Machine
enum
{
    DMA_IDX_SVC,

    DMA_IDX_CMD_TX_CHAR,
    DMA_IDX_CMD_TX_VAL,
    DMA_IDX_CMD_TX_NTF_CFG,

    DMA_IDX_CMD_RX_CHAR,
    DMA_IDX_CMD_RX_VAL,

    DMA_IDX_DATA_TX_CHAR,
    DMA_IDX_DATA_TX_VAL,
    DMA_IDX_DATA_TX_NTF_CFG,

    DMA_IDX_DATA_RX_CHAR,
    DMA_IDX_DATA_RX_VAL,

    DMA_IDX_NB,
};


/// Messages for Smart Voice Profile 
enum dma_msg_id
{
	DMA_CMD_CCC_CHANGED = TASK_FIRST_MSG((TASK_ID_DMA)),

	DMA_TX_DATA_SENT,
	
	DMA_CMD_RECEIVED,

	DMA_SEND_CMD_VIA_NOTIFICATION,

	DMA_SEND_CMD_VIA_INDICATION,

	DMA_DATA_CCC_CHANGED,

	DMA_DATA_RECEIVED,

	DMA_SEND_DATA_VIA_NOTIFICATION,

	DMA_SEND_DATA_VIA_INDICATION,
};



/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void dma_task_init(struct ke_task_desc *task_desc);

void app_ai_ble_add_dma(void);

#endif //__AI_VOICE_BLE_ENABLE__
#endif //_AMA_GATT_SERVER_H_

