/**
 ****************************************************************************************
 * @addtogroup SMARTVOICETASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#ifdef __AI_VOICE_BLE_ENABLE__

#include "rwip_config.h"
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"
#include "ai_manager.h"
#include "ai_gatt_server.h"
#include "recording_gatt_server.h"
#include "app_btgatt.h"

/*
 *VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define service_uuid_128_content              {0x00,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define cmd_tx_char_val_uuid_128_content      {0x01,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define cmd_rx_char_val_uuid_128_content      {0x02,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define data_tx_char_val_uuid_128_content      {0x03,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define data_rx_char_val_uuid_128_content      {0x04,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}


static const uint8_t VOICE_SERVICE_UUID_128[ATT_UUID_128_LEN]    = service_uuid_128_content;

/// Full SMARTVOICE SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 att_db[VOICE_IDX_NB] = {
    // Service Declaration
    [VOICE_IDX_SVC]         =   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},

    // Command TX Characteristic Declaration
    [VOICE_IDX_CMD_TX_CHAR]     =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command TX Characteristic Value
    [VOICE_IDX_CMD_TX_VAL]     =   {cmd_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(IND, ENABLE) | PERM(RD, ENABLE) | PERM(RP, AUTH),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},
    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [VOICE_IDX_CMD_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(NP, AUTH), 0, 0},

    // Command RX Characteristic Declaration
    [VOICE_IDX_CMD_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Command RX Characteristic Value
    [VOICE_IDX_CMD_RX_VAL]     =   {cmd_rx_char_val_uuid_128_content,
        PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE) | PERM(WP, AUTH),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},

    // Data TX Characteristic Declaration
    [VOICE_IDX_DATA_TX_CHAR]     =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Data TX Characteristic Value
    [VOICE_IDX_DATA_TX_VAL]     =   {data_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(IND, ENABLE) | PERM(RD, ENABLE) | PERM(RP, AUTH),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},
    // Data TX Characteristic - Client Characteristic Configuration Descriptor
    [VOICE_IDX_DATA_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(NP, AUTH), 0, 0},

    // Data RX Characteristic Declaration
    [VOICE_IDX_DATA_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // Data RX Characteristic Value
    [VOICE_IDX_DATA_RX_VAL]     =   {data_rx_char_val_uuid_128_content,
        PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE) | PERM(WP, AUTH),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), AI_MAX_LEN},

};

/**
 ****************************************************************************************
 * @brief Initialization of the VOICE module.
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
static uint8_t recording_init(struct prf_task_env* env, uint16_t* start_hdl,
    uint16_t app_task, uint8_t sec_lvl, void* params)
{
    uint8_t status;

    //Add Service Into Database
    status = attm_svc_create_db_128(start_hdl, VOICE_SERVICE_UUID_128, NULL,
            VOICE_IDX_NB, NULL, env->task, &att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE)
            | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));


    BLE_GATT_DBG("attm_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

    if(btif_is_gatt_over_br_edr_enabled()) {
        app_btgatt_addsdp(ATT_SERVICE_UUID, *start_hdl, *start_hdl+VOICE_IDX_NB-1);
    }

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate SMARTVOICE required environment variable
        PRF_ENV_T(ai)* sv_env =
                (PRF_ENV_T(ai)* ) ke_malloc(sizeof(PRF_ENV_T(ai)), KE_MEM_ATT_DB);

        // Initialize VOICE environment
        env->env           = (prf_env_t*) sv_env;
        sv_env->shdl     = *start_hdl;

        sv_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        sv_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_AI;
        recording_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, AI_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the VOICE module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void recording_destroy(struct prf_task_env* env)
{
    PRF_ENV_T(ai)* voice_env = (PRF_ENV_T(ai)*) env->env;

    TRACE(2,"%s env %p", __func__, env);
    // free profile environment variables
    env->env = NULL;
    ke_free(voice_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void recording_create(struct prf_task_env* env, uint8_t conidx)
{
    TRACE(3,"%s env %p conidx %d", __func__, env, conidx);

#if 0
    PRF_ENV_T(ai)* voice_env = (PRF_ENV_T(ai)*) env->env;
    struct prf_svc voice_svc = {voice_env->shdl, voice_env->shdl + VOICE_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &voice_svc);
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
static void recording_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    TRACE(4,"%s env %p, conidx %d reason %d", __func__, env, conidx, reason);
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// VOICE Task interface required by profile manager
const ai_ble_prf_task_cbs_t voice_itf =
{
    (prf_init_fnct) recording_init,
    recording_destroy,
    recording_create,
    recording_cleanup,
};

AI_GATT_SERVER_TO_ADD(AI_SPEC_RECORDING, &voice_itf)

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
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);
    voice_env->isCmdNotificationEnabled[conidx] = false;
    voice_env->isDataNotificationEnabled[conidx] = false;
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
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

    BLE_GATT_DBG("gattc_write_req_ind_handler sv_env 0x%x write handle %d shdl %d",
        voice_env, param->handle, voice_env->shdl);

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    if (voice_env != NULL) {
        if (param->handle == (voice_env->shdl + VOICE_IDX_CMD_TX_NTF_CFG)) {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND) {
                voice_env->isCmdNotificationEnabled[conidx] = false;
            } else if (value == PRF_CLI_START_NTF) {
                voice_env->isCmdNotificationEnabled[conidx] = true;
            } else {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR) {
                //Inform APP of TX ccc change
                struct ble_ai_tx_notif_config_t * ind = KE_MSG_ALLOC(VOICE_CMD_CCC_CHANGED,
                        prf_dst_task_get(&voice_env->prf_env, conidx),
                        prf_src_task_get(&voice_env->prf_env, conidx),
                        ble_ai_tx_notif_config_t);

                ind->isNotificationEnabled = voice_env->isCmdNotificationEnabled[conidx];

                ke_msg_send(ind);
            }
        } else if (param->handle == (voice_env->shdl + VOICE_IDX_DATA_TX_NTF_CFG)) {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND) {
                voice_env->isDataNotificationEnabled[conidx] = false;
            } else if (value == PRF_CLI_START_NTF) {
                voice_env->isDataNotificationEnabled[conidx] = true;
            } else {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR) {
                //Inform APP of TX ccc change
                struct ble_ai_tx_notif_config_t * ind = KE_MSG_ALLOC(VOICE_DATA_CCC_CHANGED,
                        prf_dst_task_get(&voice_env->prf_env, conidx),
                        prf_src_task_get(&voice_env->prf_env, conidx),
                        ble_ai_tx_notif_config_t);

                ind->isNotificationEnabled = voice_env->isDataNotificationEnabled[conidx];

                ke_msg_send(ind);
            }
        } else if (param->handle == (voice_env->shdl + VOICE_IDX_CMD_RX_VAL)) {
            //inform APP of data
            struct ble_ai_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(VOICE_CMD_RECEIVED,
                    prf_dst_task_get(&voice_env->prf_env, conidx),
                    prf_src_task_get(&voice_env->prf_env, conidx),
                    ble_ai_rx_data_ind_t,
                    sizeof(struct ble_ai_rx_data_ind_t) + param->length);

            ind->length = param->length;
            memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);
        } else if (param->handle == (voice_env->shdl + VOICE_IDX_DATA_RX_VAL)) {
            //inform APP of data
            struct ble_ai_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(VOICE_DATA_RECEIVED,
                    prf_dst_task_get(&voice_env->prf_env, conidx),
                    prf_src_task_get(&voice_env->prf_env, conidx),
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
    TRACE(1, "%s", __func__);
    // Get the address of the environment
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    uint8_t conidx = KE_IDX_GET(dest_id);

    // notification or write request has been sent out
    if ((GATTC_NOTIFY == param->operation) || (GATTC_INDICATE == param->operation)) {

        struct ble_ai_tx_sent_ind_t * ind = KE_MSG_ALLOC(VOICE_TX_DATA_SENT,
                prf_dst_task_get(&voice_env->prf_env, conidx),
                prf_src_task_get(&voice_env->prf_env, conidx),
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
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    struct gattc_read_cfm* cfm;
    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t conidx = KE_IDX_GET(src_id);

    BLE_GATT_DBG("gattc_read_req_ind_handler read handle %d shdl %d", param->handle, sv_env->shdl);

    if (param->handle == (voice_env->shdl + VOICE_IDX_CMD_TX_NTF_CFG)) {
        uint16_t notify_ccc;
        cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));

        if (voice_env->isCmdNotificationEnabled[conidx]) {
            notify_ccc = 1;
        } else {
            notify_ccc = 0;
        }
        cfm->length = sizeof(notify_ccc);
        memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
    }
    if (param->handle == (voice_env->shdl + VOICE_IDX_DATA_TX_NTF_CFG)) {
        uint16_t notify_ccc;
        cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));

        if (voice_env->isDataNotificationEnabled[conidx]) {
            notify_ccc = 1;
        } else {
            notify_ccc = 0;
        }
        cfm->length = sizeof(notify_ccc);
        memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
    }
    else {
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
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (voice_env->isCmdNotificationEnabled[conidx])) {
        handle_offset = VOICE_IDX_CMD_TX_VAL;
    } else if ((AI_CONTENT_TYPE_DATA == contentType) &&
        (voice_env->isDataNotificationEnabled[conidx])) {
        handle_offset = VOICE_IDX_DATA_TX_VAL;
    }

    if (0xFFFF != handle_offset) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&voice_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = voice_env->shdl + handle_offset;
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
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (voice_env->isCmdNotificationEnabled[conidx])) {
        handle_offset = VOICE_IDX_CMD_TX_VAL;
    } else if ((AI_CONTENT_TYPE_DATA == contentType) &&
        (voice_env->isDataNotificationEnabled)) {
        handle_offset = VOICE_IDX_DATA_TX_VAL;
    }

    if (0xFFFF != handle_offset) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&voice_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_INDICATE;
        report_ntf->handle    = voice_env->shdl + handle_offset;
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

    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((param->handle == (voice_env->shdl + VOICE_IDX_CMD_TX_NTF_CFG)) ||
        (param->handle == (voice_env->shdl + VOICE_IDX_DATA_TX_NTF_CFG))) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if ((param->handle == (voice_env->shdl + VOICE_IDX_CMD_RX_VAL)) ||
        (param->handle == (voice_env->shdl + VOICE_IDX_DATA_RX_VAL))) {
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
KE_MSG_HANDLER_TAB(recording)
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t) gapc_disconnect_ind_handler},
    {GATTC_WRITE_REQ_IND,        (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,              (ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,         (ke_msg_func_t) gattc_read_req_ind_handler},
    {VOICE_SEND_CMD_VIA_NOTIFICATION,       (ke_msg_func_t) send_cmd_via_notification_handler},
    {VOICE_SEND_CMD_VIA_INDICATION,           (ke_msg_func_t) send_cmd_via_indication_handler},
    {VOICE_SEND_DATA_VIA_NOTIFICATION,       (ke_msg_func_t) send_data_via_notification_handler},
    {VOICE_SEND_DATA_VIA_INDICATION,       (ke_msg_func_t) send_data_via_indication_handler},
    {GATTC_ATT_INFO_REQ_IND,                    (ke_msg_func_t) gattc_att_info_req_ind_handler },
};

void recording_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *sv_env = PRF_ENV_GET(AI, ai);

    task_desc->msg_handler_tab = recording_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(recording_msg_handler_tab);
    task_desc->state           = &(sv_env->state);
    task_desc->idx_max         = BLE_CONNECTION_MAX;
}


#endif /* __AI_VOICE_BLE_ENABLE__ */



