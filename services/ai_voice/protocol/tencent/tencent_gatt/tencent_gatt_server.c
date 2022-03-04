#ifdef __AI_VOICE_BLE_ENABLE__
/**
 ****************************************************************************************
 * @addtogroup TENCENT_SMARTVOICETASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"
#include "ai_control.h"
#include "ai_gatt_server.h"
#include "ai_manager.h"
#include "tencent_gatt_server.h"

/*
 * TENCENT SMART VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define tencent_service_uuid_128_content       {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x09,0x07,0x00,0x00}
#define tencent_cmd_tx_char_val_uuid_128_content   {0xf9,0x47,0xdc,0xe3,0x8b,0xeb,0x48,0x82,0xf7,0x47,0x0c,0xe4,0x6e,0x41,0xd4,0xdf}
#define tencent_cmd_rx_char_val_uuid_128_content   {0xac,0x1f,0x85,0x1a,0xc1,0xb2,0xd3,0x94,0x24,0x4a,0xfe,0x61,0x12,0xd8,0x84,0x98}

static const uint8_t TENCENT_SERVICE_UUID_128[ATT_UUID_128_LEN]  = tencent_service_uuid_128_content;

/// Full TENCENT_SMARTVOICE SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 tencent_att_db[TENCENT_IDX_NB] = {
    // Service Declaration
    [TENCENT_IDX_SVC]        =   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},

    // Command TX Characteristic Declaration
    [TENCENT_IDX_TX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command TX Characteristic Value
    [TENCENT_IDX_TX_VAL]     =   {
        tencent_cmd_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(IND, ENABLE) | PERM(RD, ENABLE),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},
    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [TENCENT_IDX_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Command RX Characteristic Declaration
    [TENCENT_IDX_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command RX Characteristic Value
    [TENCENT_IDX_RX_VAL]     =   {
        tencent_cmd_rx_char_val_uuid_128_content,
        PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},

};

/**
 ****************************************************************************************
 * @brief Initialization of the TENCENT_SMARTVOICE module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @see enum attm_value_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t tencent_init(struct prf_task_env* env, uint16_t* start_hdl,
                       uint16_t app_task, uint8_t sec_lvl, void* params)
{
    uint8_t status;

    //Add Service Into Database
    status = attm_svc_create_db_128(start_hdl, TENCENT_SERVICE_UUID_128, NULL,
                                    TENCENT_IDX_NB, NULL, env->task, &tencent_att_db[0],
                                    (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE)
                                    | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));


    BLE_GATT_DBG("attm_tencent_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR) {
        // Allocate TENCENT_SMARTVOICE required environment variable
        PRF_ENV_T(ai)* tencent_env =
            (PRF_ENV_T(ai)* ) ke_malloc(sizeof(PRF_ENV_T(ai)), KE_MEM_ATT_DB);

        // Initialize TENCENT_SMARTVOICE environment
        env->env           = (prf_env_t*) tencent_env;
        tencent_env->shdl     = *start_hdl;

        tencent_env->prf_env.app_task   = app_task |
                    (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        tencent_env->prf_env.prf_task   = env->task |
                    (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));

        // initialize environment variable
        env->id                     = TASK_ID_AI;
        tencent_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, AI_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the TENCENT_SMARTVOICE module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void tencent_destroy(struct prf_task_env* env)
{
    PRF_ENV_T(ai)* tencent_env = (PRF_ENV_T(ai)*) env->env;

    TRACE(2,"%s env %p", __func__, env);
    // free profile environment variables
    env->env = NULL;
    ke_free(tencent_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void tencent_create(struct prf_task_env* env, uint8_t conidx)
{
        TRACE(3,"%s env %p conidx %d", __func__, env, conidx);
        
#if 0
    PRF_ENV_T(ai)* tencent_env = (PRF_ENV_T(ai)*) env->env;
    struct prf_svc tencent_svc = {tencent_env->shdl, tencent_env->shdl + TENCENT_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &tencent_svc);
#endif
}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void tencent_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    TRACE(4,"%s env %p, conidx %d reason %d", __func__, env, conidx, reason);
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// TENCENT_SMARTVOICE Task interface required by profile manager
const ai_ble_prf_task_cbs_t tencent_itf = {
    (prf_init_fnct) tencent_init,
    tencent_destroy,
    tencent_create,
    tencent_cleanup,
};

AI_GATT_SERVER_TO_ADD(AI_SPEC_TENCENT, &tencent_itf)

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                       struct gapc_disconnect_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
	uint8_t conidx = KE_IDX_GET(src_id);
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);
    tencent_env->isCmdNotificationEnabled[conidx] = false;
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_write_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

    BLE_GATT_DBG("gattc_write_req_ind_handler tencent_env 0x%x write handle %d shdl %d",
                 tencent_env, param->handle, tencent_env->shdl);

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                                      GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    if (tencent_env != NULL) {
        if (param->handle == (tencent_env->shdl + TENCENT_IDX_TX_NTF_CFG)) {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND) {
                tencent_env->isCmdNotificationEnabled[conidx] = false;
            } else if (value == PRF_CLI_START_NTF) {
                tencent_env->isCmdNotificationEnabled[conidx] = true;
            } else {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR) {
                //Inform APP of TX ccc change
                struct ble_ai_tx_notif_config_t * ind = KE_MSG_ALLOC(TENCENT_CCC_CHANGED,
                                                        prf_dst_task_get(&tencent_env->prf_env, conidx),
                                                        prf_src_task_get(&tencent_env->prf_env, conidx),
                                                        ble_ai_tx_notif_config_t);

                ind->isNotificationEnabled = tencent_env->isCmdNotificationEnabled[conidx];

                ke_msg_send(ind);
            }
        } else if (param->handle == (tencent_env->shdl + TENCENT_IDX_RX_VAL)) {
            //inform APP of data
            struct ble_ai_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(TENCENT_RECEIVED,
                                                prf_dst_task_get(&tencent_env->prf_env, conidx),
                                                prf_src_task_get(&tencent_env->prf_env, conidx),
                                                ble_ai_rx_data_ind_t,
                                                sizeof(struct ble_ai_rx_data_ind_t) + param->length);

            ind->length = param->length;
            memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);
        } else {
            status = PRF_APP_ERROR;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                   ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

    uint8_t conidx = KE_IDX_GET(dest_id);

    // notification or write request has been sent out
    if ((GATTC_NOTIFY == param->operation) || (GATTC_INDICATE == param->operation)) {

        struct ble_ai_tx_sent_ind_t * ind = KE_MSG_ALLOC(TENCENT_TX_DATA_SENT,
                                            prf_dst_task_get(&tencent_env->prf_env, conidx),
                                            prf_src_task_get(&tencent_env->prf_env, conidx),
                                            ble_ai_tx_sent_ind_t);

        ind->status = param->status;

        ke_msg_send(ind);

    }

	ke_state_set(dest_id, AI_IDLE);
	
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the read request from peer device
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gattc_read_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

    struct gattc_read_cfm* cfm;

    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t conidx = KE_IDX_GET(src_id);

    BLE_GATT_DBG("gattc_read_req_ind_handler read handle %d shdl %d,isCmdNotificationEnabled is %d", param->handle, tencent_env->shdl,tencent_env->isCmdNotificationEnabled);

    if (param->handle == (tencent_env->shdl + TENCENT_IDX_TX_NTF_CFG)) {
        uint16_t notify_ccc;
        cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));

        if (tencent_env->isCmdNotificationEnabled[conidx]) {
            notify_ccc = 1;
        } else {
            notify_ccc = 0;
        }
        cfm->length = sizeof(notify_ccc);
        memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
    } else {
        cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
        cfm->length = 0;
	
        status = ATT_ERR_REQUEST_NOT_SUPPORTED;
    }

    cfm->handle = param->handle;

    cfm->status = status;

    ke_msg_send(cfm);

	ke_state_set(dest_id, AI_IDLE);

    return (KE_MSG_CONSUMED);
}

static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
    uint16_t handle_offset = 0xFFFF;
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

	  BLE_GATT_DBG("send_notifiction,isCmdNotificationEnabled is %d",tencent_env->isCmdNotificationEnabled[conidx]);

	if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (tencent_env->isCmdNotificationEnabled[conidx])) {
        handle_offset = TENCENT_IDX_TX_VAL;
    } 

    if (0xFFFF != handle_offset) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                                                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&tencent_env->prf_env, conidx),
                                                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = tencent_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
    }
}

static void send_indication(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
    uint16_t handle_offset = 0xFFFF;
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

    if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (tencent_env->isCmdNotificationEnabled[conidx])) {
        handle_offset = TENCENT_IDX_TX_VAL;
    } 
    if (0xFFFF != handle_offset) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                                                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&tencent_env->prf_env, conidx),
                                                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_INDICATE;
        report_ntf->handle    = tencent_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
    }
}

__STATIC int send_cmd_via_notification_handler(ke_msg_id_t const msgid,
        struct ble_ai_send_data_req_t const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, AI_CONTENT_TYPE_COMMAND, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_cmd_via_indication_handler(ke_msg_id_t const msgid,
        struct ble_ai_send_data_req_t const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    send_indication(param->connecionIndex, AI_CONTENT_TYPE_COMMAND, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

#if 0
__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
        struct ble_ai_send_data_req_t const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, AI_CONTENT_TYPE_DATA, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_data_via_indication_handler(ke_msg_id_t const msgid,
        struct ble_ai_send_data_req_t const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    send_indication(param->connecionIndex, AI_CONTENT_TYPE_DATA, param->value, param->length);
    return (KE_MSG_CONSUMED);
}
#endif

/**
 ****************************************************************************************
 * @brief Handles reception of the attribute info request message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct gattc_att_info_cfm * cfm;

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

    if (param->handle == (tencent_env->shdl + TENCENT_IDX_TX_NTF_CFG)) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if (param->handle == tencent_env->shdl + TENCENT_IDX_RX_VAL) {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;
    } else {
        cfm->length = 0;
        cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(tencent)
{
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t) gapc_disconnect_ind_handler},
    {GATTC_WRITE_REQ_IND,       (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,             (ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,        (ke_msg_func_t) gattc_read_req_ind_handler},
    {TENCENT_SEND_VIA_NOTIFICATION,      (ke_msg_func_t) send_cmd_via_notification_handler},
    {TENCENT_SEND_VIA_INDICATION,        (ke_msg_func_t) send_cmd_via_indication_handler},
    {GATTC_ATT_INFO_REQ_IND,                    (ke_msg_func_t) gattc_att_info_req_ind_handler },
};

void tencent_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *tencent_env = PRF_ENV_GET(AI, ai);

    task_desc->msg_handler_tab = tencent_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(tencent_msg_handler_tab);
    task_desc->state           = &(tencent_env->state);
    task_desc->idx_max         = BLE_CONNECTION_MAX;
}

#endif //__AI_VOICE_BLE_ENABLE__

