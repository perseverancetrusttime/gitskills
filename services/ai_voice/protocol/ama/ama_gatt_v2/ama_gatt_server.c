#ifdef __AI_VOICE_BLE_ENABLE__
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "gatt.h"
#include "gatt_msg.h"
#include "prf_types.h"
#include "prf.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"

#include "ai_thread.h"
#include "ai_control.h"
#include "ai_manager.h"
#include "ai_gatt_server.h"
#include "ama_gatt_server.h"
#include "app_ai_ble.h"

/*
 * AMA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define ama_service_uuid_128_content        {0xFB,0x34,0x9B,0x5F,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x03,0xFE,0x00,0x00}
#define ama_rx_char_val_uuid_128_content    {0x76,0x30,0xF8,0xDD,0x90,0xA3,0x61,0xAC,0xA7,0x43,0x05,0x30,0x77,0xB1,0x4E,0xF0}
#define ama_tx_char_val_uuid_128_content    {0x0B,0x42,0x82,0x1F,0x64,0x72,0x2F,0x8A,0xB4,0x4B,0x79,0x18,0x5B,0xA0,0xEE,0x2B}   

static const uint8_t AMA_SERVICE_UUID_128[GATT_UUID_128_LEN] = ama_service_uuid_128_content;

/// Full AMA SERVER Database Description - Used to add attributes into the database
const struct gatt_att_desc ama_att_db[AMA_IDX_NB] =
{
    // Service Declaration
    [AMA_IDX_SVC]       =   {ATT_DECL_PRIMARY_SERVICE_UUID, PROP(RD), 0},

    // Command RX Characteristic Declaration
    [AMA_IDX_RX_CHAR]   =   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Command RX Characteristic Value
    [AMA_IDX_RX_VAL]    =   {ama_rx_char_val_uuid_128_content, 
        PROP(WR) | SEC_LVL(WP, AUTH) | ATT_UUID(128), AI_MAX_LEN},

    // Command TX Characteristic Declaration
    [AMA_IDX_TX_CHAR]   =   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Command TX Characteristic Value
    [AMA_IDX_TX_VAL]    =   {ama_tx_char_val_uuid_128_content, 
        PROP(N) | PROP(RD) | SEC_LVL(RP, AUTH) | ATT_UUID(128), AI_MAX_LEN},

    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [AMA_IDX_TX_NTF_CFG] =  {ATT_DESC_CLIENT_CHAR_CFG_UUID, PROP(RD) | PROP(WR) | SEC_LVL(NIP, AUTH), 0},
};

__STATIC void ama_gatt_cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    // notification or indication has been sent out
    TRACE(0, "%s conidx 0x%x", __func__, conidx);

    ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, AI_SPEC_AMA, SET_BLE_FLAG(conidx));
}

__STATIC void ama_gatt_cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                             uint16_t max_length)
{
    co_buf_t* p_data = NULL;
    uint16_t dataLen = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    PRF_ENV_T(ai) *ama_env = PRF_ENV_GET(AMA, ai);

    TRACE(0, "%s conidx 0x%x", __func__, conidx);
    TRACE(1, "read hdl 0x%x shdl 0x%x", hdl, ama_env->shdl);

    if (hdl == (ama_env->shdl + AMA_IDX_TX_NTF_CFG))
    {
        uint16_t notify_ccc = ama_env->ntfIndEnableFlag[conidx];
        dataLen = sizeof(notify_ccc);
        prf_buf_alloc(&p_data, dataLen);
        memcpy(co_buf_data(p_data), (uint8_t *)&notify_ccc, dataLen);
    }
    else {
        dataLen = 0;
        status = ATT_ERR_REQUEST_NOT_SUPPORTED;
    }    

    gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, dataLen, p_data);

    // Release the buffer
    co_buf_release(p_data);
}

__STATIC void ama_gatt_cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy, uint16_t hdl,
                              uint16_t max_length)
{
    TRACE(0, "%s conidx 0x%x", __func__, conidx);
}

__STATIC void ama_gatt_cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    uint16_t length = 0;
    uint16_t status = GAP_ERR_NO_ERROR;
    
    PRF_ENV_T(ai) *ama_env = PRF_ENV_GET(AMA, ai);

    // check if it's a client configuration char
    if (hdl == ama_env->shdl + AMA_IDX_TX_NTF_CFG)
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == ama_env->shdl + AMA_IDX_RX_VAL)
    {
        // force length to zero to reject any write starting from something != 0
        length = 0;
        status = GAP_ERR_NO_ERROR;
    }
    // not expected request
    else
    {
        length = 0;
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    TRACE(1, "%s status 0x%x hdl %d", __func__, status, hdl);

    // Send the confirmation
    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, length);
}

__STATIC void ama_gatt_cb_att_set(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl,
                                          uint16_t offset, co_buf_t* p_buf)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *ama_env = PRF_ENV_GET(AMA, ai);

    uint8_t status = GAP_ERR_NO_ERROR;

    TRACE(4, "%s buds_env 0x%x write handle 0x%x shdl 0x%x conidx 0x%x", 
        __func__, (uint32_t)ama_env, hdl, ama_env->shdl, conidx);

    uint8_t* pData = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;

    DUMP8("%02x ", pData, dataLen);

    if (ama_env != NULL)
    {
        // TX ccc
        if (hdl == (ama_env->shdl + AMA_IDX_TX_NTF_CFG))
        {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, pData, sizeof(uint16_t));
            TRACE(0, "AMA_IDX_TX_NTF_CFG 0x%x value 0x%x", AMA_IDX_TX_NTF_CFG, value);

            if (value <= (PRF_CLI_START_IND|PRF_CLI_START_NTF))
            {
                if(value == 0)
                {
                    app_ai_disconnected_evt_handler(conidx);
                }
                else
                {
                    if(app_ai_get_connected_mun() == AI_CONNECT_NUM_MAX)
                    {
                        TRACE(1,"%s, Already connected max num ai !!!", __func__);
                    }
                    else
                    {
                        ama_env->ntfIndEnableFlag[conidx] = value;
                        uint8_t connect_index = app_ai_set_ble_connect_info(AI_TRANSPORT_BLE, AI_SPEC_AMA, conidx);
                        if (!app_ai_ble_is_connected(connect_index) || (conidx == app_ai_env[connect_index].connectionIndex))
                        {
                            if (BLE_INVALID_CONNECTION_INDEX == app_ai_env[connect_index].connectionIndex)
                            {
                                app_ai_connected_evt_handler(conidx, connect_index);
                                if (app_ai_env[connect_index].mtu[conidx] > 0)
                                {
                                    ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env[connect_index].mtu[conidx], AI_SPEC_AMA, SET_BLE_FLAG(conidx));
                                }
                            }
                            ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE, AI_SPEC_AMA, SET_BLE_FLAG(conidx));
                        }
                    }
                }
            }
            else 
            {
                status = PRF_APP_ERROR;
            }
        }
        // RX data
        else if (hdl == (ama_env->shdl + AMA_IDX_RX_VAL))
        {
            TRACE(0, "AMA_IDX_RX_VAL 0x%x dataLen %d", AMA_IDX_RX_VAL, dataLen);
            DUMP8("0x%x ", pData, dataLen);
            // handle received data
            //if (buds_data_received_cb)
            //{
                //buds_data_received_cb(pData, dataLen);
            //}
            ai_function_handle(CALLBACK_CMD_RECEIVE, pData, dataLen, AI_SPEC_AMA, SET_BLE_FLAG(conidx));
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    // Inform GATT about handling
    gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
}

/// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t ama_gatt_srv_cb = {
    .cb_event_sent      = ama_gatt_cb_event_sent,
    .cb_att_read_get    = ama_gatt_cb_att_read_get,
    .cb_att_event_get   = ama_gatt_cb_att_event_get,
    .cb_att_info_get    = ama_gatt_cb_att_info_get,
    .cb_att_val_set     = ama_gatt_cb_att_set,
};

/**
 ****************************************************************************************
 * @brief Initialization of the SMARTVOICE module.
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
static uint16_t ama_init(prf_data_t* env, uint16_t* start_hdl, 
        uint8_t sec_lvl, uint8_t user_prio, const void* params, const void* p_cb)
{
    uint16_t status = GAP_ERR_NO_ERROR;

    TRACE(0, "ama_init returns %d start handle is %d sec_lvl 0x%x", status, *start_hdl, sec_lvl);

    // Allocate BUDS required environment variable
    PRF_ENV_T(ai)* ama_env =
            (PRF_ENV_T(ai)* ) ke_malloc(sizeof(PRF_ENV_T(ai)), KE_MEM_ATT_DB);

    memset((uint8_t *)ama_env, 0, sizeof(PRF_ENV_T(ai)));
    // Initialize BUDS environment
    env->p_env = (prf_hdr_t*) ama_env;

    // Register as GATT User Client
    status = gatt_user_srv_register(PREFERRED_BLE_MTU, user_prio, &ama_gatt_srv_cb,
                                    &(ama_env->srv_user_lid));

    //-------------------- allocate memory required for the profile  ---------------------
    if (GAP_ERR_NO_ERROR == status)
    {
        ama_env->shdl     = *start_hdl;
        status = gatt_db_svc_add(ama_env->srv_user_lid, SVC_SEC_LVL(NOT_ENC) | SVC_UUID(128), AMA_SERVICE_UUID_128,
                                   AMA_IDX_NB, NULL, ama_att_db, AMA_IDX_NB,
                                   &ama_env->shdl);

       TRACE(1, "%s status %d nb_att %d shdl %d", __func__, status, AMA_IDX_NB, ama_env->shdl);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the SMARTVOICE module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static uint16_t ama_destroy(prf_data_t* p_env, uint8_t reason)
{
    PRF_ENV_T(ai)* ama_env = (PRF_ENV_T(ai)*) p_env->p_env;

    TRACE(0, "%s reason 0x%x", __func__, reason);
    // free profile environment variables
    p_env->p_env = NULL;
    ke_free(ama_env);

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
static void ama_create(prf_data_t* env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    TRACE(3,"%s env %p conidx %d", __func__, env, conidx);
    
#if 0
    PRF_ENV_T(ai)* ama_env = (PRF_ENV_T(ai)*) env->env;
    struct prf_svc ama_svc = {ama_env->shdl, ama_env->shdl + AMA_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &ama_svc);
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
static void ama_cleanup(prf_data_t* env, uint8_t conidx, uint16_t reason)
{
    TRACE(4,"%s env %p, conidx %d reason %d", __func__, env, conidx, reason);
    /* Nothing to do */
}

/**
 ****************************************************************************************
 * @brief Indicates update of connection parameters
 *
 * @param[in|out]    env          Collector or Service allocated environment data.
 * @param[in]        conidx       Connection index
 * @param[in]        p_con_param  Pointer to new connection parameters information
 ****************************************************************************************
 */
static void ama_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    TRACE(0, "%s", __func__);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// AMA Task interface required by profile manager
const ai_ble_prf_task_cbs_t ama_itf =
{
    ama_init,
    ama_destroy,
    ama_create,
    ama_cleanup,
    ama_upd,
};

AI_GATT_SERVER_TO_ADD(AI_SPEC_AMA, &ama_itf);

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
const struct prf_task_cbs* ama_prf_itf_get(void)
{
    AI_GATT_SERVER_INSTANCE_T *pInstance = NULL;

    pInstance = ai_gatt_server_get_entry_pointer_from_ai_code(AI_SPEC_AMA);

    TRACE(1, "%s", __func__);

    if(pInstance)
    {
        return pInstance->ai_itf;
    }
    else
    {
        return NULL;
    }
}

 
void app_ai_ble_add_ama(void)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    TRACE(0, "%s %d", __func__, TASK_ID_AMA);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_AMA;
    // Send the message
    ke_msg_send(req);

#endif // __AI_VOICE_BLE_ENABLE__
}


static bool ama_send_ind_ntf_generic(bool isNotification, uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
    enum gatt_evt_type evtType = isNotification?GATT_NOTIFY:GATT_INDICATE;

    PRF_ENV_T(ai) *ama_env = PRF_ENV_GET(AMA, ai);

    TRACE(1, "ntfIndEnableFlag %d, %d", ama_env->ntfIndEnableFlag[conidx],
        1 << (uint8_t)evtType);

    if ((ama_env->ntfIndEnableFlag[conidx]) & (1 << (uint8_t)evtType))
    {
        co_buf_t* p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t* p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        // Dummy parameter provided to GATT
        uint16_t dummy = 0;

        // Inform the GATT that notification must be sent
        uint16_t ret = gatt_srv_event_send(conidx, ama_env->srv_user_lid, dummy, evtType,
                            ama_env->shdl + AMA_IDX_TX_VAL, p_buf);

        // Release the buffer
        co_buf_release(p_buf);

        return (GAP_ERR_NO_ERROR == ret);
    }
    else
    {
        return false;
    }
}

bool app_ama_send_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    TRACE(2,"%s len %d", __func__, length);
    uint8_t connect_index = app_ai_get_connect_index_from_ble_conidx(conidx, AI_SPEC_AMA);
    ama_send_ind_ntf_generic(true, app_ai_env[connect_index].connectionIndex, ptrData, length);
    return true;
}

void app_ama_send_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t conidx)
{
    TRACE(2,"%s len= %d", __func__, length);
    uint8_t connect_index = app_ai_get_connect_index_from_ble_conidx(conidx, AI_SPEC_AMA);
    ama_send_ind_ntf_generic(false, app_ai_env[connect_index].connectionIndex, ptrData, length);
}

#endif //__AI_VOICE_BLE_ENABLE__

