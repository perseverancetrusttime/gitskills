#ifndef _RECORDING_GATT_SERVER_H_
#define _RECORDING_GATT_SERVER_H_

/**
 ****************************************************************************************
 * @addtogroup SMARTVOICETASK Task
 * @ingroup SMARTVOICE
 * @brief Smart Voice Profile Task.
 *
 * The SMARTVOICETASK is responsible for handling the messages coming in and out of the
 * @ref SMARTVOICE collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */



#ifdef DUAL_MIC_RECORDING
#include "ke_task.h"



///Attributes State Machine
enum {
    VOICE_IDX_SVC,

    VOICE_IDX_CMD_TX_CHAR,
    VOICE_IDX_CMD_TX_VAL,
    VOICE_IDX_CMD_TX_NTF_CFG,

    VOICE_IDX_CMD_RX_CHAR,
    VOICE_IDX_CMD_RX_VAL,

    VOICE_IDX_DATA_TX_CHAR,
    VOICE_IDX_DATA_TX_VAL,
    VOICE_IDX_DATA_TX_NTF_CFG,

    VOICE_IDX_DATA_RX_CHAR,
    VOICE_IDX_DATA_RX_VAL,

    VOICE_IDX_NB,
};


/// Messages for Smart Voice Profile
enum sv_msg_id {
	VOICE_CMD_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_AI),

	VOICE_TX_DATA_SENT,

	VOICE_CMD_RECEIVED,

	VOICE_SEND_CMD_VIA_NOTIFICATION,

	VOICE_SEND_CMD_VIA_INDICATION,

	VOICE_DATA_CCC_CHANGED,

	VOICE_DATA_RECEIVED,

	VOICE_SEND_DATA_VIA_NOTIFICATION,

	VOICE_SEND_DATA_VIA_INDICATION,
};

/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void recording_task_init(struct ke_task_desc *task_desc);

#endif /* __SMART_VOICE__ */

#endif //_SMARTVOICE_GATT_SERVER_H_

