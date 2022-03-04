/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/



/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TOTA)

#include "gap.h"
#include "tota_ble.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"
#include "prf_svc.h"
#include "app_btgatt.h"


/*
 * TOTA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define tota_service_uuid_128_content              {0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86 }
#define tota_val_char_val_uuid_128_content          {0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97 }    

#define GATT_DECL_PRIMARY_SERVICE        { 0x00, 0x28 }
#define GATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define GATT_DESC_CLIENT_CHAR_CFG_UUID        { 0x02, 0x29 }

static const uint8_t TOTA_SERVICE_UUID_128[GATT_UUID_128_LEN]    = tota_service_uuid_128_content;  

/// Full OTA SERVER Database Description - Used to add attributes into the database
const struct gatt_att_desc tota_att_db[TOTA_IDX_NB] =
{
    // TOTA Service Declaration
    [TOTA_IDX_SVC]     =   {GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0},
    // TOTA Characteristic Declaration
    [TOTA_IDX_CHAR]    =  {GATT_DECL_CHARACTERISTIC_UUID, PROP(RD), 0},
    // TOTA service
    [TOTA_IDX_VAL]     =  {tota_val_char_val_uuid_128_content, PROP(WR) | PROP(N) | ATT_UUID(128), TOTA_MAX_LEN},
    // TOTA Characteristic
    [TOTA_IDX_NTF_CFG]    =   {GATT_DESC_CLIENT_CHAR_CFG_UUID, PROP(RD) | PROP(WR), 0},

};

__STATIC void tota_gatt_cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    // notification or indication has been sent out
    TRACE(0, "%s conidx 0x%x", __func__, conidx);
    PRF_ENV_T(tota) *tota_env = PRF_ENV_GET(TOTA, tota);
        
    // notification or write request has been sent out
    struct ble_tota_tx_sent_ind_t * ind = KE_MSG_ALLOC(TOTA_TX_DATA_SENT,
            prf_dst_task_get(&tota_env->prf_env, conidx),
            prf_src_task_get(&tota_env->prf_env, conidx),
            ble_tota_tx_sent_ind_t);

    ind->status = status;

    ke_msg_send(ind);
}

__STATIC void tota_gatt_cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                             uint16_t max_length)
{
    co_buf_t* p_data = NULL;
    uint16_t dataLen = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    PRF_ENV_T(tota) *tota_env = PRF_ENV_GET(TOTA, tota);

    TRACE(0, "%s conidx 0x%x", __func__, conidx);
    TRACE(1, "read hdl 0x%x shdl 0x%x", hdl, tota_env->shdl);

    if (hdl == (tota_env->shdl + TOTA_IDX_NTF_CFG))
    {
        uint16_t notify_ccc = tota_env->ntfIndEnableFlag[conidx];
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

__STATIC void tota_gatt_cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy, uint16_t hdl,
                              uint16_t max_length)
{
    TRACE(0, "%s conidx 0x%x", __func__, conidx);
}

__STATIC void tota_gatt_cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    TRACE(1,"[%s]TOTA",__func__);
    uint16_t length = 0;
    uint16_t status = GAP_ERR_NO_ERROR;
    
    PRF_ENV_T(tota) *tota_env = PRF_ENV_GET(TOTA, tota);

    if (hdl == (tota_env->shdl + TOTA_IDX_NTF_CFG))
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == (tota_env->shdl + TOTA_IDX_VAL))
    {
        // force length to zero to reject any write starting from something != 0
        length = 0;
        status = GAP_ERR_NO_ERROR;            
    }
    else
    {
        length = 0;
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, length);
}

__STATIC void tota_gatt_cb_att_set(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl,
                                          uint16_t offset, co_buf_t* p_buf)
{
    // Get the address of the environment
    PRF_ENV_T(tota) *tota_env = PRF_ENV_GET(TOTA, tota);

    uint8_t status = GAP_ERR_NO_ERROR;

    TRACE(4, "%s buds_env 0x%x write handle %d shdl %d", 
        __FUNCTION__, (uint32_t)tota_env, hdl, tota_env->shdl);

    uint8_t* pData = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;

    if (tota_env != NULL)
    {
        // TX ccc
        if (hdl == (tota_env->shdl + TOTA_IDX_NTF_CFG))
        {
            uint16_t value = 0x0000;
            
            //Extract value before check
            memcpy(&value, pData, sizeof(uint16_t));
            
            if ((value == PRF_CLI_STOP_NTFIND)||(value == PRF_CLI_START_NTF))
            {

                tota_env->ntfIndEnableFlag[conidx] = value;
            }
            else
            {
                status = PRF_APP_ERROR;
            }
            
            // Inform GATT about handling
            gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
               //Inform APP of TX ccc change
                struct ble_tota_tx_notif_config_t * ind = KE_MSG_ALLOC(TOTA_CCC_CHANGED,
                        prf_dst_task_get(&tota_env->prf_env, conidx),
                        prf_src_task_get(&tota_env->prf_env, conidx),
                        ble_tota_tx_notif_config_t);

                if(PRF_CLI_STOP_NTFIND == tota_env->ntfIndEnableFlag[conidx])
                {
                    ind->isNotificationEnabled  = false;
                }
                else if(PRF_CLI_START_NTF == tota_env->ntfIndEnableFlag[conidx])
                {
                    ind->isNotificationEnabled  = true;
                }

                ke_msg_send(ind);
            }
        }
        else if (hdl == (tota_env->shdl + TOTA_IDX_VAL))
        {        
            tota_env->rx_token = token;
    
            //inform APP of data
            struct ble_tota_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(TOTA_DATA_RECEIVED,
                    prf_dst_task_get(&tota_env->prf_env, conidx),
                    prf_src_task_get(&tota_env->prf_env, conidx),
                    ble_tota_rx_data_ind_t,
                    sizeof(dataLen));

            ind->length = dataLen;
            memcpy((uint8_t *)(ind->data), pData, dataLen);

            ke_msg_send(ind);

            gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
        }
        else
        {
            // Inform GATT about handling
            gatt_srv_att_val_set_cfm(conidx, user_lid, token, PRF_APP_ERROR);
        }
    }
}

/// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t tota_gatt_srv_cb = {
    .cb_event_sent      = tota_gatt_cb_event_sent,
    .cb_att_read_get    = tota_gatt_cb_att_read_get,
    .cb_att_event_get   = tota_gatt_cb_att_event_get,
    .cb_att_info_get    = tota_gatt_cb_att_info_get,
    .cb_att_val_set     = tota_gatt_cb_att_set,
};


/**
 ****************************************************************************************
 * @brief Initialization of the TOTA module.
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
static uint16_t tota_init(prf_data_t* env, uint16_t* start_hdl, 
        uint8_t sec_lvl, uint8_t user_prio, const void* params, const void* p_cb)
{
    uint16_t status = GAP_ERR_NO_ERROR;

    TRACE(0, "tota_init returns %d start handle is %d sec_lvl 0x%x", status, *start_hdl, sec_lvl);

    // Allocate BUDS required environment variable
    PRF_ENV_T(tota)* tota_env =
            (PRF_ENV_T(tota)* ) ke_malloc(sizeof(PRF_ENV_T(tota)), KE_MEM_ATT_DB);

    memset((uint8_t *)tota_env, 0, sizeof(PRF_ENV_T(tota)));
    // Initialize BUDS environment
    env->p_env = (prf_hdr_t*) tota_env;

    // Register as GATT User Client
    status = gatt_user_srv_register(PREFERRED_BLE_MTU, 0, &tota_gatt_srv_cb,
                                    &(tota_env->srv_user_lid));

    //-------------------- allocate memory required for the profile  ---------------------
    if (GAP_ERR_NO_ERROR == status)
    {
        tota_env->shdl     = *start_hdl;
        status = gatt_db_svc_add(tota_env->srv_user_lid, SVC_SEC_LVL(NOT_ENC) | SVC_UUID(128), TOTA_SERVICE_UUID_128,
                                   TOTA_IDX_NB, NULL, tota_att_db, TOTA_IDX_NB,
                                   &tota_env->shdl);

       TRACE(1, "%s status %d nb_att %d shdl %d", __func__, status, TOTA_IDX_NB, tota_env->shdl);
       ASSERT(GAP_ERR_NO_ERROR == status, "%s init failed", __FILE__);
    }

    if (btif_is_gatt_over_br_edr_enabled())
    {
        app_btgatt_addsdp(ATT_SERVICE_UUID, tota_env->shdl, tota_env->shdl+TOTA_IDX_NB-1);
    }
    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the TOTA module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static uint16_t tota_destroy(prf_data_t* p_env, uint8_t reason)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(tota)* tota_env = (PRF_ENV_T(tota)*) p_env->p_env;

    if(reason != PRF_DESTROY_RESET)
    {
        status = gatt_user_unregister(tota_env->srv_user_lid);
    }

    if(status == GAP_ERR_NO_ERROR)
    {
        // free profile environment variables
        p_env->p_env = NULL;
        ke_free(tota_env);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void tota_create(prf_data_t* env, uint8_t conidx)
{
    TRACE(3,"%s env %p conidx %d", __func__, env, conidx);
#if 0
    PRF_ENV_T(tota)* tota_env = (PRF_ENV_T(tota)*) env->env;
    struct prf_svc tota_tota_svc = {tota_env->shdl, tota_env->shdl + TOTA_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &tota_tota_svc);
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
static void tota_cleanup(prf_data_t* p_env, uint8_t conidx, uint16_t reason)
{
    /* Nothing to do */
    PRF_ENV_T(tota) *p_tota_env = (PRF_ENV_T(tota) *) p_env->p_env;    
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);    
    // force notification config to zero when peer device is disconnected    
    p_tota_env->ntfIndEnableFlag[conidx] = 0;
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// TOTA Task interface required by profile manager
const struct prf_task_cbs tota_itf =
{
    (prf_init_fnct) tota_init,
    tota_destroy,
    tota_create,
    tota_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* tota_prf_itf_get(void)
{
   return &tota_itf;
}

bool tota_send_ind_ntf_generic(bool isNotification, uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
    enum gatt_evt_type evtType = isNotification?GATT_NOTIFY:GATT_INDICATE;

    PRF_ENV_T(tota) *tota_env = PRF_ENV_GET(TOTA, tota);

    if ((tota_env->ntfIndEnableFlag[conidx])&(1 << (uint8_t)evtType)) 
    {

        co_buf_t* p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t* p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        // Dummy parameter provided to GATT
        uint16_t dummy = 0;

        // Inform the GATT that notification must be sent
        uint16_t ret = gatt_srv_event_send(conidx, tota_env->srv_user_lid, dummy, evtType,
                            tota_env->shdl + TOTA_IDX_VAL, p_buf);

        // Release the buffer
        co_buf_release(p_buf);

        return (GAP_ERR_NO_ERROR == ret);
    }
    else
    {
        return false;
    }
}

#endif /* BLE_TOTA */
