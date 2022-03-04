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
#include "gatt.h"
#include "gatt_msg.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"
#include "ai_manager.h"
#include "ai_gatt_server.h"
#include "prf_dbg.h"
#include "recording_gatt_server.h"
#include "bt_if.h"
#include "app_btgatt.h"

#define PRF_TAG         "[VR]"

/*
 *VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define service_uuid_128_content                                                                       \
    {                                                                                                  \
        0x00, 0x00, 0x82, 0x6f, 0x63, 0x2e, 0x74, 0x6e, 0x69, 0x6f, 0x70, 0x6c, 0x65, 0x63, 0x78, 0x65 \
    }
#define cmd_tx_char_val_uuid_128_content                                                               \
    {                                                                                                  \
        0x01, 0x00, 0x82, 0x6f, 0x63, 0x2e, 0x74, 0x6e, 0x69, 0x6f, 0x70, 0x6c, 0x65, 0x63, 0x78, 0x65 \
    }
#define cmd_rx_char_val_uuid_128_content                                                               \
    {                                                                                                  \
        0x02, 0x00, 0x82, 0x6f, 0x63, 0x2e, 0x74, 0x6e, 0x69, 0x6f, 0x70, 0x6c, 0x65, 0x63, 0x78, 0x65 \
    }
#define data_tx_char_val_uuid_128_content                                                              \
    {                                                                                                  \
        0x03, 0x00, 0x82, 0x6f, 0x63, 0x2e, 0x74, 0x6e, 0x69, 0x6f, 0x70, 0x6c, 0x65, 0x63, 0x78, 0x65 \
    }
#define data_rx_char_val_uuid_128_content                                                              \
    {                                                                                                  \
        0x04, 0x00, 0x82, 0x6f, 0x63, 0x2e, 0x74, 0x6e, 0x69, 0x6f, 0x70, 0x6c, 0x65, 0x63, 0x78, 0x65 \
    }

/**
 * @brief This function is called when GATT server user has initiated event send to peer
 *        device or if an error occurs.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param status        Status of the procedure (@see enum hl_err)
 */
static void _cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

/**
 * @brief This function is called when peer want to read local attribute database value.
 *
 *        @see gatt_srv_att_read_get_cfm shall be called to provide attribute value
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param hdl           Attribute handle
 * @param offset        Value offset
 * @param max_length    Maximum value length to return
 */
static void _cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                             uint16_t max_length);

/**
 * @brief This function is called when GATT server user has initiated event send procedure,
 *
 *        @see gatt_srv_att_event_get_cfm shall be called to provide attribute value
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param dummy         Dummy parameter provided by upper layer for command execution.
 * @param hdl           Attribute handle
 * @param max_length    Maximum value length to return
 */
static void _cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy, uint16_t hdl,
                              uint16_t max_length);

/**
 * @brief This function is called during a write procedure to get information about a
 *        specific attribute handle.
 *
 *        @see gatt_srv_att_info_get_cfm shall be called to provide attribute information
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param hdl           Attribute handle
 */
static void _cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl);

/**
 * @brief This function is called during a write procedure to modify attribute handle.
 *
 *        @see gatt_srv_att_val_set_cfm shall be called to accept or reject attribute
 *        update.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param hdl           Attribute handle
 * @param offset        Value
 * @param p_data        Pointer to buffer that contains data to write starting from offset
 */
static void _cb_att_val_set(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                            co_buf_t* p_data);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
static const uint8_t VOICE_SERVICE_UUID_128[GATT_UUID_128_LEN] = service_uuid_128_content;

/// Full SMARTVOICE SERVER Database Description - Used to add attributes into the database
const struct gatt_att_desc record_att_db[VOICE_IDX_NB] = {
    // Service Declaration
    [VOICE_IDX_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_PRIMARY_SERVICE),
        PROP(RD),
        0,
    },

    // Command TX Characteristic Declaration
    [VOICE_IDX_CMD_TX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },

    // Command TX Characteristic Value
    [VOICE_IDX_CMD_TX_VAL] = {
        cmd_tx_char_val_uuid_128_content,
        PROP(N) | PROP(I) | PROP(RD) | SEC_LVL(RP, AUTH) | ATT_UUID(128),
        AI_MAX_LEN,
    },

    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [VOICE_IDX_CMD_TX_NTF_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        PROP(RD) | PROP(WR) | SEC_LVL(NIP, AUTH),
        0,
    },

    // Command RX Characteristic Declaration
    [VOICE_IDX_CMD_RX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },

    // Command RX Characteristic Value
    [VOICE_IDX_CMD_RX_VAL] = {
        cmd_rx_char_val_uuid_128_content,
        PROP(WR) | PROP(WC) | SEC_LVL(WP, AUTH) | ATT_UUID(128),
        AI_MAX_LEN,
    },

    // Data TX Characteristic Declaration
    [VOICE_IDX_DATA_TX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },

    // Data TX Characteristic Value
    [VOICE_IDX_DATA_TX_VAL] = {
        data_tx_char_val_uuid_128_content,
        PROP(N) | PROP(I) | PROP(RD) | SEC_LVL(RP, AUTH) | ATT_UUID(128),
        AI_MAX_LEN,
    },

    // Data TX Characteristic - Client Characteristic Configuration Descriptor
    [VOICE_IDX_DATA_TX_NTF_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        PROP(RD) | PROP(WR) | SEC_LVL(NIP, AUTH),
        0,
    },

    // Data RX Characteristic Declaration
    [VOICE_IDX_DATA_RX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },

    // Data RX Characteristic Value
    [VOICE_IDX_DATA_RX_VAL] = {
        data_rx_char_val_uuid_128_content,
        PROP(WR) | PROP(WC) | SEC_LVL(WP, AUTH) | ATT_UUID(128),
        AI_MAX_LEN,
    },
};

/// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t _gatt_srv_cb = {
    .cb_event_sent      = _cb_event_sent,
    .cb_att_read_get    = _cb_att_read_get,
    .cb_att_event_get   = _cb_att_event_get,
    .cb_att_info_get    = _cb_att_info_get,
    .cb_att_val_set     = _cb_att_val_set,
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
static uint16_t recording_init(prf_data_t *env, uint16_t *p_start_hdl, uint8_t sec_lvl,
                              uint8_t user_prio, const void *params, const void *p_cb)
{
    LOG_I("%s start_hdl:%d", __func__, *p_start_hdl);
    uint16_t status = GAP_ERR_NO_ERROR;

    /// allocate&init profile environment data
    PRF_ENV_T(ai) *sv_env = (PRF_ENV_T(ai) *)ke_malloc(sizeof(PRF_ENV_T(ai)), KE_MEM_ATT_DB);
    memset((uint8_t *)sv_env, 0, sizeof(PRF_ENV_T(ai)));

    do
    {
        /// registor GATT server user
        status = gatt_user_srv_register(PREFERRED_BLE_MTU, user_prio,
                                        &_gatt_srv_cb, &(sv_env->srv_user_lid));

        /// check the GATT server user registation excution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        if(btif_is_gatt_over_br_edr_enabled()) {
            app_btgatt_addsdp(ATT_SERVICE_UUID, *start_hdl, *start_hdl+VOICE_IDX_NB-1);
        }

        status = gatt_db_svc_add(sv_env->srv_user_lid, sec_lvl,
                                 VOICE_SERVICE_UUID_128,
                                 VOICE_IDX_NB, NULL, record_att_db, VOICE_IDX_NB,
                                 &sv_env->shdl);

        /// check the GATT database add execution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        /// initialize environment variable
        env->p_env = (prf_hdr_t *)sv_env;

        /// init descriptor
        recording_task_init(&(env->desc));

        /// init the task state
        // ke_state_set(KE_BUILD_ID(p_env->prf_task, 0), 0);
    } while (0);

    return status;
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
static uint16_t recording_destroy(prf_data_t *p_env, uint8_t reason)
{
    PRF_ENV_T(ai)* voice_env = (PRF_ENV_T(ai)*) p_env->p_env;

    LOG_I("%s env %p", __func__, p_env);
    // free profile environment variables
    p_env->p_env = NULL;
    ke_free(voice_env);

    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void recording_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s env %p conidx %d", __func__, p_env, conidx);
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
static void recording_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason)
{
    LOG_I("%s env %p, conidx %d reason %d", __func__, p_env, conidx, reason);
    /* Nothing to do */
}

static void recording_con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s", __func__);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// VOICE Task interface required by profile manager
const ai_ble_prf_task_cbs_t voice_itf =
{
    .cb_init = recording_init,
    .cb_destroy = recording_destroy,
    .cb_con_create = recording_create,
    .cb_con_cleanup = recording_cleanup,
    .cb_con_upd = recording_con_upd,
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
    voice_env->ntfIndEnableFlag[conidx] = false;
    return KE_MSG_CONSUMED;
}

static void _cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_D("%s", __func__);

    // notification or write request has been sent out
    struct ble_ai_tx_sent_ind_t *ind =
        KE_MSG_ALLOC(VOICE_TX_DATA_SENT,
                     KE_BUILD_ID(PRF_DST_TASK(AI), conidx),
                     KE_BUILD_ID(PRF_SRC_TASK(AI), conidx),
                     ble_ai_tx_sent_ind_t);

    ind->status = status;

    ke_msg_send(ind);
}

static void _cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                             uint16_t hdl, uint16_t offset, uint16_t max_length)
{
    LOG_I("%s hdl:%d", __func__, hdl);
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);
    uint16_t status = GAP_ERR_NO_ERROR;
    uint16_t notify_ccc = 0;
    uint16_t dataLen = sizeof(notify_ccc);
    co_buf_t *p_data = NULL;

    if (VOICE_IDX_CMD_TX_NTF_CFG == hdl - voice_env->shdl)
    {
        notify_ccc = voice_env->ntfIndEnableFlag[conidx]
                         ? 1
                         : 0;

        prf_buf_alloc(&p_data, dataLen);
        memcpy(co_buf_data(p_data), (uint8_t *)&notify_ccc, dataLen);
    }
    else if (VOICE_IDX_DATA_TX_NTF_CFG == hdl - voice_env->shdl)
    {
        notify_ccc = voice_env->ntfIndEnableFlag[conidx]
                         ? 1
                         : 0;

        prf_buf_alloc(&p_data, dataLen);
        memcpy(co_buf_data(p_data), (uint8_t *)&notify_ccc, dataLen);
    }
    else
    {
        dataLen = 0;
        status = ATT_ERR_REQUEST_NOT_SUPPORTED;
    }

    gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, dataLen, p_data);

    // Release the buffer
    co_buf_release(p_data);
}

static void _cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                              uint16_t dummy, uint16_t hdl, uint16_t max_length)
{
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);
    LOG_I("%s hdl:%d", __func__, (hdl - voice_env->shdl));
}

static void _cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    LOG_I("%s hdl:%d", __func__, hdl);
    uint16_t status = GAP_ERR_NO_ERROR;
    uint16_t length = 0;
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((VOICE_IDX_CMD_TX_NTF_CFG == (hdl - voice_env->shdl)) ||
        (VOICE_IDX_DATA_TX_NTF_CFG == (hdl - voice_env->shdl)))
    {
        length = 2;
    }
    else if ((VOICE_IDX_CMD_RX_VAL == (hdl - voice_env->shdl)) ||
             (VOICE_IDX_DATA_RX_VAL == (hdl - voice_env->shdl)))
    {
        length = 0;
    }
    else
    {
        length = 0;
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    /// Send the confirmation
    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, length);
}

static void _cb_att_val_set(uint8_t conidx, uint8_t user_lid, uint16_t token,
                            uint16_t hdl, uint16_t offset, co_buf_t *p_buf)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    uint8_t *data = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;
    LOG_I("%s Write req, hdl:%d, conidx:%d", __func__, hdl, conidx);

    /// Get the address of the environment
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);
    uint16_t handle = hdl - voice_env->shdl;

    if (voice_env != NULL)
    {
        if (VOICE_IDX_CMD_TX_NTF_CFG == handle)
        {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, data, sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                voice_env->ntfIndEnableFlag[conidx] = false;
            }
            else if (value == PRF_CLI_START_NTF)
            {
                voice_env->ntfIndEnableFlag[conidx] = true;
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
                struct ble_ai_tx_notif_config_t *ind =
                    KE_MSG_ALLOC(VOICE_CMD_CCC_CHANGED,
                                 KE_BUILD_ID(PRF_DST_TASK(AI), conidx),
                                 KE_BUILD_ID(PRF_SRC_TASK(AI), conidx),
                                 ble_ai_tx_notif_config_t);

                ind->isNotificationEnabled = voice_env->ntfIndEnableFlag[conidx];

                ke_msg_send(ind);
            }
        }
        else if (VOICE_IDX_DATA_TX_NTF_CFG == handle)
        {
            uint16_t value = 0x0000;

            /// Extract value before check
            memcpy(&value, data, sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                voice_env->ntfIndEnableFlag[conidx] = false;
            }
            else if (value == PRF_CLI_START_NTF)
            {
                voice_env->ntfIndEnableFlag[conidx] = true;
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
                struct ble_ai_tx_notif_config_t *ind =
                    KE_MSG_ALLOC(VOICE_DATA_CCC_CHANGED,
                                 KE_BUILD_ID(PRF_DST_TASK(AI), conidx),
                                 KE_BUILD_ID(PRF_SRC_TASK(AI), conidx),
                                 ble_ai_tx_notif_config_t);

                ind->isNotificationEnabled = voice_env->ntfIndEnableFlag[conidx];

                ke_msg_send(ind);
            }
        }
        else if (VOICE_IDX_CMD_RX_VAL == handle)
        {
            /// inform APP of data
            struct ble_ai_rx_data_ind_t *ind =
                KE_MSG_ALLOC_DYN(VOICE_CMD_RECEIVED,
                                 KE_BUILD_ID(PRF_DST_TASK(AI), conidx),
                                 KE_BUILD_ID(PRF_SRC_TASK(AI), conidx),
                                 ble_ai_rx_data_ind_t,
                                 sizeof(struct ble_ai_rx_data_ind_t) + dataLen);

            ind->length = dataLen;
            memcpy((uint8_t *)(ind->data), data, dataLen);

            ke_msg_send(ind);
        }
        else if (VOICE_IDX_DATA_RX_VAL == handle)
        {
            //inform APP of data
            struct ble_ai_rx_data_ind_t *ind =
                KE_MSG_ALLOC_DYN(VOICE_DATA_RECEIVED,
                                 KE_BUILD_ID(PRF_DST_TASK(AI), conidx),
                                 KE_BUILD_ID(PRF_SRC_TASK(AI), conidx),
                                 ble_ai_rx_data_ind_t,
                                 sizeof(struct ble_ai_rx_data_ind_t) + dataLen);

            ind->length = dataLen;
            memcpy((uint8_t *)(ind->data), data, dataLen);

            ke_msg_send(ind);
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    /// Inform GATT about handling
    gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
}

static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t *ptrData, uint32_t length)
{
    uint16_t handle_offset = 0xFFFF;
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (voice_env->ntfIndEnableFlag[conidx]))
    {
        handle_offset = VOICE_IDX_CMD_TX_VAL;
    }
    else if ((AI_CONTENT_TYPE_DATA == contentType) &&
             (voice_env->ntfIndEnableFlag[conidx]))
    {
        handle_offset = VOICE_IDX_DATA_TX_VAL;
    }

    if (0xFFFF != handle_offset)
    {
        co_buf_t *p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t *p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        /// Dummy parameter provided to GATT
        uint16_t dummy = 0;

        gatt_srv_event_send(conidx, voice_env->srv_user_lid, dummy, GATT_NOTIFY,
                            voice_env->shdl + handle_offset, p_buf);

        /// Release the buffer
        co_buf_release(p_buf);
    }
}

static void send_indication(uint8_t conidx,
                            uint8_t contentType,
                            const uint8_t *ptrData,
                            uint32_t length)
{
    uint16_t handle_offset = 0xFFFF;
    PRF_ENV_T(ai) *voice_env = PRF_ENV_GET(AI, ai);

    if ((AI_CONTENT_TYPE_COMMAND == contentType) &&
        (voice_env->ntfIndEnableFlag[conidx]))
    {
        handle_offset = VOICE_IDX_CMD_TX_VAL;
    }
    else if ((AI_CONTENT_TYPE_DATA == contentType) &&
             (voice_env->ntfIndEnableFlag))
    {
        handle_offset = VOICE_IDX_DATA_TX_VAL;
    }

    if (0xFFFF != handle_offset)
    {
        co_buf_t *p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t *p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        /// Dummy parameter provided to GATT
        uint16_t dummy = 0;

        gatt_srv_event_send(conidx, voice_env->srv_user_lid, dummy, GATT_INDICATE,
                            voice_env->shdl + handle_offset, p_buf);

        /// Release the buffer
        co_buf_release(p_buf);
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

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(recording)
{
    {GAPC_DISCONNECT_IND,               (ke_msg_func_t) gapc_disconnect_ind_handler},
    {VOICE_SEND_CMD_VIA_NOTIFICATION,   (ke_msg_func_t) send_cmd_via_notification_handler},
    {VOICE_SEND_CMD_VIA_INDICATION,     (ke_msg_func_t) send_cmd_via_indication_handler},
    {VOICE_SEND_DATA_VIA_NOTIFICATION,  (ke_msg_func_t) send_data_via_notification_handler},
    {VOICE_SEND_DATA_VIA_INDICATION,    (ke_msg_func_t) send_data_via_indication_handler},
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
