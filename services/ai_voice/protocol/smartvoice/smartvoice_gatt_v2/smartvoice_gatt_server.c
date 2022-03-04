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
#include "smartvoice_gatt_server.h"
#include "app_ai_ble.h"

#include "app_btgatt.h"
#include "bt_if.h"

/*
 * SMART VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define sv_service_uuid_128_content  			{0x00,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define sv_cmd_tx_char_val_uuid_128_content  	{0x01,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}	
#define sv_cmd_rx_char_val_uuid_128_content  	{0x02,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}
#define sv_data_tx_char_val_uuid_128_content  	{0x03,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}	
#define sv_data_rx_char_val_uuid_128_content  	{0x04,0x00,0x82,0x6f,0x63,0x2e,0x74,0x6e,0x69,0x6f,0x70,0x6c,0x65,0x63,0x78,0x65}


static const uint8_t SMARTVOICE_SERVICE_UUID_128[GATT_UUID_128_LEN]	= sv_service_uuid_128_content;  

/// Full SMARTVOICE SERVER Database Description - Used to add attributes into the database
const struct gatt_att_desc sv_att_db[SMARTVOICE_IDX_NB] = {
    // Service Declaration
    [SMARTVOICE_IDX_SVC] 		=   {ATT_DECL_PRIMARY_SERVICE_UUID, PROP(RD), 0},

    // Command TX Characteristic Declaration
    [SMARTVOICE_IDX_CMD_TX_CHAR] 	=   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Command TX Characteristic Value
    [SMARTVOICE_IDX_CMD_TX_VAL]     =   {sv_cmd_tx_char_val_uuid_128_content, 
        PROP(N) | PROP(I) | PROP(RD) | SEC_LVL(RP, AUTH) | ATT_UUID(128), AI_MAX_LEN},

    // Command TX Characteristic - Client Characteristic Configuration Descriptor
    [SMARTVOICE_IDX_CMD_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PROP(RD) | PROP(WR) | SEC_LVL(NIP, AUTH) | SEC_LVL(WP, AUTH), 0},

    // Command RX Characteristic Declaration
    [SMARTVOICE_IDX_CMD_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Command RX Characteristic Value
    [SMARTVOICE_IDX_CMD_RX_VAL]     =   {sv_cmd_rx_char_val_uuid_128_content, 
        PROP(WR) | PROP(WC) | SEC_LVL(WP, AUTH) | ATT_UUID(128), AI_MAX_LEN},

    // Data TX Characteristic Declaration
    [SMARTVOICE_IDX_DATA_TX_CHAR] 	=   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Data TX Characteristic Value
    [SMARTVOICE_IDX_DATA_TX_VAL]     =   {sv_data_tx_char_val_uuid_128_content,
        PROP(N) | PROP(I) | PROP(RD) | SEC_LVL(RP, AUTH) | ATT_UUID(128), AI_MAX_LEN},

    // Data TX Characteristic - Client Characteristic Configuration Descriptor
    [SMARTVOICE_IDX_DATA_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PROP(RD) | PROP(WR) | SEC_LVL(NIP, AUTH), 0},

    // Data RX Characteristic Declaration
    [SMARTVOICE_IDX_DATA_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},

    // Data RX Characteristic Value
    [SMARTVOICE_IDX_DATA_RX_VAL]     =   {sv_data_rx_char_val_uuid_128_content, 
        PROP(WR) | PROP(WC) | SEC_LVL(WP, AUTH) | ATT_UUID(128), AI_MAX_LEN},	

};

__STATIC void sv_gatt_cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    // notification or indication has been sent out
    TRACE(0, "%s conidx 0x%x", __func__, conidx);
    ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, AI_SPEC_BES, SET_BLE_FLAG(conidx));
}

__STATIC void sv_gatt_cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                             uint16_t max_length)
{
    co_buf_t* p_data = NULL;
    uint16_t dataLen = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    PRF_ENV_T(ai) *sv_env = PRF_ENV_GET(SMVOICE, ai);

    TRACE(0, "%s conidx 0x%x", __func__, conidx);
    TRACE(1, "read hdl 0x%x shdl 0x%x", hdl, sv_env->shdl);

    if (hdl == (sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG)|| hdl == (sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG))
    {
        uint16_t notify_ccc = sv_env->ntfIndEnableFlag[conidx];
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

__STATIC void sv_gatt_cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy, uint16_t hdl,
                              uint16_t max_length)
{
    TRACE(0, "%s conidx 0x%x", __func__, conidx);
}

__STATIC void sv_gatt_cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    uint16_t length = 0;
    uint16_t status = GAP_ERR_NO_ERROR;
    
    PRF_ENV_T(ai) *sv_env = PRF_ENV_GET(SMVOICE, ai);

    // check if it's a client configuration char
    if (hdl == sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG)
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG)
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == sv_env->shdl + SMARTVOICE_IDX_CMD_RX_VAL)
    {
        // force length to zero to reject any write starting from something != 0
        length = 0;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == sv_env->shdl + SMARTVOICE_IDX_DATA_RX_VAL)
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

__STATIC void sv_gatt_cb_att_set(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl,
                                          uint16_t offset, co_buf_t* p_buf)
{
    // Get the address of the environment
    PRF_ENV_T(ai) *sv_env = PRF_ENV_GET(SMVOICE, ai);

    uint8_t status = GAP_ERR_NO_ERROR;

    TRACE(4, "%s buds_env 0x%x write handle 0x%x shdl 0x%x conidx 0x%x", 
        __func__, (uint32_t)sv_env, hdl, sv_env->shdl, conidx);

    uint8_t* pData = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;
    uint16_t value = 0x0000;

    DUMP8("%02x ", pData, dataLen);

    if (sv_env != NULL)
    {
        // TX ccc
        if (hdl == (sv_env->shdl + SMARTVOICE_IDX_CMD_TX_NTF_CFG))
        {
            //Extract value before check
            memcpy(&value, pData, sizeof(uint16_t));
            TRACE(0, "SMARTVOICE_IDX_CMD_TX_NTF_CFG 0x%x value 0x%x", SMARTVOICE_IDX_CMD_TX_NTF_CFG, value);

            if (value <= (PRF_CLI_START_IND|PRF_CLI_START_NTF))
            {
                if(app_ai_get_connected_mun() == AI_CONNECT_NUM_MAX)
                {
                    TRACE(1,"%s, Already connected max num ai !!!", __func__);
                }
                else
                {
                    sv_env->ntfIndEnableFlag[conidx] = value;
                    uint8_t connect_index = app_ai_set_ble_connect_info(AI_TRANSPORT_BLE, AI_SPEC_BES, conidx);
                    if (!app_ai_ble_is_connected(connect_index) || (conidx == app_ai_env[connect_index].connectionIndex))
                    {
                        if (BLE_INVALID_CONNECTION_INDEX == app_ai_env[connect_index].connectionIndex)
                        {
                            app_ai_connected_evt_handler(conidx, connect_index);
                            if (app_ai_env[connect_index].mtu[conidx] > 0)
                            {
                                ai_function_handle(CALLBACK_UPDATE_MTU, NULL, app_ai_env[connect_index].mtu[conidx], AI_SPEC_BES, SET_BLE_FLAG(conidx));
                            }
                        }
                        ai_function_handle(CALLBACK_AI_CONNECT, NULL, AI_TRANSPORT_BLE, AI_SPEC_BES, SET_BLE_FLAG(conidx));
                    }
                }
            }
            else 
            {
                status = PRF_APP_ERROR;
            }
        }
        else if (hdl == (sv_env->shdl + SMARTVOICE_IDX_DATA_TX_NTF_CFG))
        {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, pData, sizeof(uint16_t));
            TRACE(0, "SMARTVOICE_IDX_DATA_TX_NTF_CFG 0x%x value 0x%x", SMARTVOICE_IDX_DATA_TX_NTF_CFG, value);

            if (value <= (PRF_CLI_START_IND|PRF_CLI_START_NTF))
            {
                sv_env->ntfIndEnableFlag[conidx] = value;
                //uint8_t connect_index = app_ai_set_connect_type(AI_TRANSPORT_BLE, AI_SPEC_BES);
                //app_ai_env[connect_index].isDataNotificationEnabled = value;
            }
            else 
            {
                status = PRF_APP_ERROR;
            }
        }
        // RX data
        else if (hdl == (sv_env->shdl + SMARTVOICE_IDX_CMD_RX_VAL))
        {
            TRACE(0, "SMARTVOICE_IDX_CMD_RX_VAL 0x%x dataLen %d", SMARTVOICE_IDX_CMD_RX_VAL, dataLen);
            DUMP8("0x%x ", pData, dataLen);
            // handle received data
            //if (buds_data_received_cb)
            //{
                //buds_data_received_cb(pData, dataLen);
            //}
            ai_function_handle(CALLBACK_CMD_RECEIVE, pData, dataLen, AI_SPEC_BES, SET_BLE_FLAG(conidx));
        }
        else if (hdl == (sv_env->shdl + SMARTVOICE_IDX_DATA_RX_VAL))
        {
            TRACE(0, "SMARTVOICE_IDX_DATA_RX_VAL 0x%x dataLen %d", SMARTVOICE_IDX_DATA_RX_VAL, dataLen);
            DUMP8("0x%x ", pData, dataLen);
            // handle received data
            //if (buds_data_received_cb)
            //{
                //buds_data_received_cb(pData, dataLen);
            //}
            ai_function_handle(CALLBACK_DATA_RECEIVE, pData, dataLen, AI_SPEC_BES, SET_BLE_FLAG(conidx));
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
__STATIC const gatt_srv_cb_t sv_gatt_srv_cb = {
    .cb_event_sent      = sv_gatt_cb_event_sent,
    .cb_att_read_get    = sv_gatt_cb_att_read_get,
    .cb_att_event_get   = sv_gatt_cb_att_event_get,
    .cb_att_info_get    = sv_gatt_cb_att_info_get,
    .cb_att_val_set     = sv_gatt_cb_att_set,
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
static uint16_t sv_init(prf_data_t* env, uint16_t* start_hdl, 
        uint8_t sec_lvl, uint8_t user_prio, const void* params, const void* p_cb)
{
    uint16_t status = GAP_ERR_NO_ERROR;

    // Allocate BUDS required environment variable
    PRF_ENV_T(ai)* sv_env =
            (PRF_ENV_T(ai)* ) ke_malloc(sizeof(PRF_ENV_T(ai)), KE_MEM_ATT_DB);

    memset((uint8_t *)sv_env, 0, sizeof(PRF_ENV_T(ai)));
    // Initialize BUDS environment
    env->p_env = (prf_hdr_t*) sv_env;

    // Register as GATT User Client
    status = gatt_user_srv_register(PREFERRED_BLE_MTU, 0, &sv_gatt_srv_cb,
                                    &(sv_env->srv_user_lid));

    //-------------------- allocate memory required for the profile  ---------------------
    if (GAP_ERR_NO_ERROR == status)
    {
        sv_env->shdl     = *start_hdl;
        status = gatt_db_svc_add(sv_env->srv_user_lid, SVC_SEC_LVL(NOT_ENC) | SVC_UUID(128), SMARTVOICE_SERVICE_UUID_128,
                                   SMARTVOICE_IDX_NB, NULL, sv_att_db, SMARTVOICE_IDX_NB,
                                   &sv_env->shdl);

       TRACE(1, "%s status %d nb_att %d shdl %d", __func__, status, SMARTVOICE_IDX_NB, sv_env->shdl);
    }

    *start_hdl = sv_env->shdl;
    TRACE(0, "sv_init returns %d start handle is %d sec_lvl 0x%x", status, *start_hdl, sec_lvl);

    if (btif_is_gatt_over_br_edr_enabled())
    {
        app_btgatt_addsdp(ATT_SERVICE_UUID, sv_env->shdl, sv_env->shdl+SMARTVOICE_IDX_NB-1);
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
static uint16_t sv_destroy(prf_data_t* p_env, uint8_t reason)
{
    PRF_ENV_T(ai)* sv_env = (PRF_ENV_T(ai)*) p_env->p_env;

    TRACE(0, "%s reason 0x%x", __func__, reason);
    // free profile environment variables
    p_env->p_env = NULL;
    ke_free(sv_env);

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
static void sv_create(prf_data_t* env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    TRACE(3,"%s env %p conidx %d", __func__, env, conidx);
    
#if 0
    PRF_ENV_T(ai)* sv_env = (PRF_ENV_T(ai)*) env->env;
    struct prf_svc sv_svc = {sv_env->shdl, sv_env->shdl + SMARTVOICE_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &sv_svc);
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
static void sv_cleanup(prf_data_t* env, uint8_t conidx, uint16_t reason)
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
static void sv_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    TRACE(0, "%s", __func__);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// AMA Task interface required by profile manager
const ai_ble_prf_task_cbs_t sv_itf =
{
    sv_init,
    sv_destroy,
    sv_create,
    sv_cleanup,
    sv_upd,
};

AI_GATT_SERVER_TO_ADD(AI_SPEC_BES, &sv_itf);

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
const struct prf_task_cbs* smartvoice_prf_itf_get(void)
{
    AI_GATT_SERVER_INSTANCE_T *pInstance = NULL;

    pInstance = ai_gatt_server_get_entry_pointer_from_ai_code(AI_SPEC_BES);

    TRACE(1, "%s pInstance %p", __func__, pInstance);

    if(pInstance)
    {
        return pInstance->ai_itf;
    }
    else
    {
        return NULL;
    }
}

void app_ai_ble_add_smartvoice(void)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    TRACE(0, "%s %d", __func__, TASK_ID_SMART);

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
    req->prf_api_id = TASK_ID_SMART;
    // Send the message
    ke_msg_send(req);

#endif // __AI_VOICE_BLE_ENABLE__
}

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static bool sv_send_ind_ntf_generic(bool isNotification, uint8_t conidx, const uint8_t* ptrData, uint32_t length, uint8_t contentType)
{
    enum gatt_evt_type evtType = isNotification?GATT_NOTIFY:GATT_INDICATE;
    uint16_t handle_offset = 0xFFFF;

    PRF_ENV_T(ai) *sv_env = PRF_ENV_GET(SMVOICE, ai);

    TRACE(1, "ntfIndEnableFlag %d, %d", sv_env->ntfIndEnableFlag[conidx],
        1 << (uint8_t)evtType);

    if ((AI_CONTENT_TYPE_COMMAND == contentType))
    {
        handle_offset = SMARTVOICE_IDX_CMD_TX_VAL;
    } else if ((AI_CONTENT_TYPE_DATA == contentType))
    {
        handle_offset = SMARTVOICE_IDX_DATA_TX_VAL;
    }

    if ((sv_env->ntfIndEnableFlag[conidx]) & (1 << (uint8_t)evtType))
    {
        co_buf_t* p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t* p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        // Dummy parameter provided to GATT
        uint16_t dummy = 0;

        // Inform the GATT that notification must be sent
        uint16_t ret = gatt_srv_event_send(conidx, sv_env->srv_user_lid, dummy, evtType,
                            sv_env->shdl + handle_offset, p_buf);

        // Release the buffer
        co_buf_release(p_buf);

        return (GAP_ERR_NO_ERROR == ret);
    }
    else
    {
        return false;
    }
}

bool app_sv_send_cmd_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    TRACE(2,"%s len %d", __func__, length);
    sv_send_ind_ntf_generic(true, app_ai_env[0].connectionIndex, ptrData, length, AI_CONTENT_TYPE_COMMAND);
    return true;
}

bool app_sv_send_data_via_notification(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    TRACE(2,"%s len %d", __func__, length);
    sv_send_ind_ntf_generic(true, app_ai_env[0].connectionIndex, ptrData, length, AI_CONTENT_TYPE_DATA);
    return true;
}

void app_sv_send_cmd_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    TRACE(2,"%s len= %d", __func__, length);
    sv_send_ind_ntf_generic(false, app_ai_env[0].connectionIndex, ptrData, length, AI_CONTENT_TYPE_COMMAND);
}

void app_sv_send_data_via_indication(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    TRACE(2,"%s len= %d", __func__, length);
    sv_send_ind_ntf_generic(false, app_ai_env[0].connectionIndex, ptrData, length, AI_CONTENT_TYPE_DATA);
}

#endif //__AI_VOICE_BLE_ENABLE__

