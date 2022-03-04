/**
 ****************************************************************************************
 * @addtogroup GFPS STASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GFPS_PROVIDER)
#include <string.h>

#include "co_utils.h"
#include "gap.h"
#include "gfps_provider.h"
//#include "crypto.h"
#include "gfps_provider_task.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
   

    TRACE(0,"send notification");
    PRF_ENV_T(gfpsp)* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    if(gfpsp_env->isNotificationEnabled[conidx])
    {
         co_buf_t* p_buf = NULL;
         prf_buf_alloc(&p_buf, length);

         uint8_t* p_data = co_buf_data(p_buf);
         memcpy(p_data, ptrData, length);

         uint16_t dummy = 0;

        // Inform the GATT that notification must be sent
        gatt_srv_event_send(conidx, gfpsp_env->user_lid, dummy, GATT_NOTIFY,
                            gfpsp_env->start_hdl + contentType, p_buf);

        // Release the buffer
        co_buf_release(p_buf); 
     }

}

__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
     TRACE(0,"send_data_via_notification_handler,GFPSP_IDX_KEY_BASED_PAIRING_VAL");
    send_notifiction(param->connecionIndex, GFPSP_IDX_KEY_BASED_PAIRING_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_passkey_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, GFPSP_IDX_PASSKEY_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_name_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, GFPSP_IDX_NAME_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of write request message.
 * The handler will analyse what has been set and decide alert level
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
uint8_t Encrypted_req[80]={
    0xb9,0x7b,0x2c,0x4b,0x84,0x59,0x3e,0x69,0xf9,0xc5,
    0x1c,0x6f,0xf9,0xba,0x7e,0xc0,0x27,0xa6,0x13,0x55,
    0x26,0x84,0xbc,0xa2,0xd8,0x95,0xd6,0xf8,0xdd,0x5e,
    0xb5,0x91,0xfe,0xf7,0x31,0x1c,0x19,0x3e,0x38,0x8e,
    0x5f,0x3a,0xe6,0x6b,0x68,0x46,0xc4,0x14,0x1c,0x03,
    0xcb,0xc3,0x18,0x06,0x6b,0x52,0xd9,0x5c,0xa0,0xa2,
    0xb5,0x80,0xd9,0x90,0x4b,0xed,0x46,0x23,0x22,0x9b,
    0x42,0xe7,0xc2,0xde,0x2e,0x2a,0xba,0x7c,0xac,0x2b
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
//
/// Default State handlers definition
KE_MSG_HANDLER_TAB(gfpsp)
{
    {GFPSP_KEY_BASED_PAIRING_WRITE_NOTIFY,      (ke_msg_func_t)send_data_via_notification_handler},
    {GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,           (ke_msg_func_t)send_passkey_data_via_notification_handler},
    {GFPSP_NAME_NOTIFY,             (ke_msg_func_t)send_name_via_notification_handler},
};

void gfpsp_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    PRF_ENV_T(gfpsp) *gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    task_desc->msg_handler_tab = gfpsp_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(gfpsp_msg_handler_tab);
    task_desc->state           = gfpsp_env->state;
    task_desc->idx_max         = GFPSP_IDX_MAX;
}

#endif //BLE_GFPS_SERVER

/// @} DISSTASK
