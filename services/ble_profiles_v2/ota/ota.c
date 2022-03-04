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

#if (BLE_OTA)
#include "app.h"
#include "gap.h"
#include "ota.h"
#include "prf_utils.h"
#include "gatt_msg.h"
#include "ke_mem.h"
#include "co_utils.h"
#include "gatt.h"
#include "prf.h"
#include "prf_dbg.h"
#include "ota_basic.h"
#include "ota_bes.h"
#include "app_btgatt.h"
#include "app_ota.h"
#include "bt_if.h"
#include "btgatt_api.h"
/*
 * MACRO DEFINITION
 ****************************************************************************************
 */
#define PRF_TAG         "[OTA]"

#define RD_PERM         PROP(RD)
#define VAL_PERM        PROP(WR) | PROP(N) | ATT_UUID(128) | PROP(WC)
#define NTF_CFG_PERM    PROP(RD) | PROP(WR)

/*
 * OTA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
static const uint8_t OTA_SERVICE_UUID_128[GATT_UUID_128_LEN] = BES_OTA_UUID_128;
static const gatt_srv_cb_t ota_gatt_cb;


/// Full OTA SERVER Database Description - Used to add attributes into the database
const struct gatt_att_desc ota_att_db[OTA_IDX_NB] = {
    // OTA Service Declaration
    [OTA_IDX_SVC] = {UUID_16_TO_ARRAY(GATT_DECL_PRIMARY_SERVICE), RD_PERM, 0},
    // OTA Characteristic Declaration
    [OTA_IDX_CHAR] = {UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC), RD_PERM, 0},
    // OTA service
    [OTA_IDX_VAL] = {ota_val_char_val_uuid_128_content, VAL_PERM, OTA_MAX_LEN},
    // OTA Characteristic
    [OTA_IDX_NTF_CFG] = {UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG), NTF_CFG_PERM, 0},
};

/**
 ****************************************************************************************
 * @brief Initialization of the OTA module.
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
static uint16_t _init(prf_data_t *p_env, uint16_t *p_start_hdl, uint8_t sec_lvl, uint8_t user_prio,
                      const void *params, const void *p_cb)
{
    uint16_t status = GAP_ERR_NO_ERROR;

    // Initialize OTA environment

    // Allocate OTA required environment variable
    PRF_ENV_T(ota) *p_ota_env_tag =
        (PRF_ENV_T(ota) *)ke_malloc(sizeof(PRF_ENV_T(ota)), KE_MEM_ATT_DB);

    memset((uint8_t *)p_ota_env_tag, 0, sizeof(PRF_ENV_T(ota)));
    p_env->p_env = (prf_hdr_t *)p_ota_env_tag;

    do
    {
        // register PROXR user
        status = gatt_user_srv_register(PREFERRED_BLE_MTU, user_prio, &ota_gatt_cb,
                                        &(p_ota_env_tag->user_lid));
        if (status != GAP_ERR_NO_ERROR)
        {
            break;
        }

        // Add Link loss service
        status = gatt_db_svc_add(p_ota_env_tag->user_lid, sec_lvl,
                                 OTA_SERVICE_UUID_128,
                                 OTA_IDX_NB, NULL, ota_att_db, OTA_IDX_NB,
                                 &p_ota_env_tag->shdl);

        if (status != GAP_ERR_NO_ERROR) 
        {
            break;
        }

    } while (0);

    ASSERT(GAP_ERR_NO_ERROR == status, "%s init failed", __FILE__);

    LOG_I("uid %d shdl %d", p_ota_env_tag->user_lid, p_ota_env_tag->shdl);

    // TODO: enable gatt over br/edr for new ble stack
    if (btif_is_gatt_over_br_edr_enabled())
    {
        app_btgatt_addsdp(ATT_SERVICE_UUID, p_ota_env_tag->shdl, p_ota_env_tag->shdl+OTA_IDX_NB-1);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the OTA module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static uint16_t _destroy(prf_data_t *p_env, uint8_t reason)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(ota) *p_ota_env = (PRF_ENV_T(ota) *)p_env->p_env;

    if (reason != PRF_DESTROY_RESET)
    {
        status = gatt_user_unregister(p_ota_env->user_lid);
    }

    if (status == GAP_ERR_NO_ERROR)
    {
        // free profile environment variables
        p_env->p_env = NULL;
        ke_free(p_ota_env);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env          Collector or Service allocated environment data.
 * @param[in]        conidx       Connection index
 * @param[in]        p_con_param  Pointer to connection parameters information
 ****************************************************************************************
 */
static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    PRF_ENV_T(ota) *p_ota_env = (PRF_ENV_T(ota) *)p_env->p_env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    p_ota_env->ntfIndEnableFlag[conidx] = 0;
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
static void _con_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason)
{
    PRF_ENV_T(ota) *p_ota_env = (PRF_ENV_T(ota) *)p_env->p_env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    p_ota_env->ntfIndEnableFlag[conidx] = 0;

    ota_basic_disable_datapath_status(OTA_BASIC_BLE_DATAPATH_ENABLED,
                                      app_ble_get_peer_addr(conidx));
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
static void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s", __func__);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// OTA Task interface required by profile manager
const prf_task_cbs_t ota_itf = {
    .cb_init        = _init,
    .cb_destroy     = _destroy,
    .cb_con_create  = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd     = _con_upd,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
const prf_task_cbs_t *ota_prf_itf_get(void)
{
    return &ota_itf;
}

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

__STATIC void ota_gatt_cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    // notification or write request has been sent out
    struct ble_ota_tx_sent_ind_t *ind = KE_MSG_ALLOC(OTA_TX_DATA_SENT,
                                                     PRF_DST_TASK(OTA),
                                                     PRF_SRC_TASK(OTA),
                                                     ble_ota_tx_sent_ind_t);

    ind->status = status;

    ke_msg_send(ind);
}

__STATIC void ota_gatt_cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                                       uint16_t max_length)
{
    // Get the address of the environment
    PRF_ENV_T(ota) *ota_env = PRF_ENV_GET(OTA, ota);

    co_buf_t *p_data = NULL;
    uint16_t dataLen = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    if (hdl == (ota_env->shdl + OTA_IDX_NTF_CFG))
    {
        uint16_t notify_ccc = ota_env->ntfIndEnableFlag[conidx];
        dataLen = sizeof(notify_ccc);
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

__STATIC void ota_gatt_cb_att_set(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl,
                                  uint16_t offset, co_buf_t *p_buf)
{
    // Get the address of the environment
    PRF_ENV_T(ota) *ota_env = PRF_ENV_GET(OTA, ota);

    uint8_t status = GAP_ERR_NO_ERROR;

    LOG_I("%s ota_env 0x%x write handle %d shdl %d, token:%d", 
          __func__, (uint32_t)ota_env, hdl, ota_env->shdl, token);

    uint8_t *pData = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;

    if (ota_env != NULL)
    {
        // TX ccc
        if (hdl == (ota_env->shdl + OTA_IDX_NTF_CFG))
        {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, pData, sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                ota_basic_disable_datapath_status(OTA_BASIC_BLE_DATAPATH_ENABLED,
                                                  app_ble_get_peer_addr(conidx));
                ota_env->ntfIndEnableFlag[conidx] = value;

                if (btif_is_gatt_over_br_edr_enabled() && btif_btgatt_is_connected_by_conidx(conidx))
                {
                    app_ota_disconnected_evt_handler(conidx);
                }
            }
            else if (value == PRF_CLI_START_NTF)
            {
                bool isAccept = ota_basic_enable_datapath_status(OTA_BASIC_BLE_DATAPATH_ENABLED,
                                                                 app_ble_get_peer_addr(conidx));
                status = isAccept ? GAP_ERR_NO_ERROR : PRF_APP_ERROR;

                ota_env->ntfIndEnableFlag[conidx] = value;
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            // Inform GATT about handling
            gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);

            if (status == GAP_ERR_NO_ERROR)
            {
                app_ota_update_conidx(conidx);
                //Inform APP of TX ccc change
                struct ble_ota_tx_notif_config_t *ind = KE_MSG_ALLOC(OTA_CCC_CHANGED,
                                                                     PRF_DST_TASK(OTA),
                                                                     PRF_SRC_TASK(OTA),
                                                                     ble_ota_tx_notif_config_t);

                ind->ntfIndEnableFlag = ota_env->ntfIndEnableFlag[conidx];

                ke_msg_send(ind);
            }
        }
        else if (hdl == (ota_env->shdl + OTA_IDX_VAL))
        {
            ota_env->rx_token = token;
            ota_env->user_lid = user_lid;

            //inform APP of data
            struct ble_ota_rx_data_ind_t *ind = KE_MSG_ALLOC_DYN(OTA_DATA_RECEIVED,
                                                                 PRF_DST_TASK(OTA),
                                                                 PRF_SRC_TASK(OTA),
                                                                 ble_ota_rx_data_ind_t,
                                                                 dataLen);

            ind->length = dataLen;
            memcpy((uint8_t *)(ind->data), pData, dataLen);

            ke_msg_send(ind);
        }
        else
        {
            // Inform GATT about handling
            gatt_srv_att_val_set_cfm(conidx, user_lid, token, PRF_APP_ERROR);
        }
    }
}

void ota_send_rx_cfm(uint8_t conidx)
{
    // TODO: should get status from up layer
    uint16_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(ota) *ota_env = PRF_ENV_GET(OTA, ota);

    if (GAP_ERR_NO_ERROR != status)
    {
        LOG_E("%s status:%d, conidx:%d, user_lid:%d, token:%d",
              __func__, status, conidx, ota_env->user_lid, ota_env->rx_token);
    }

    gatt_srv_att_val_set_cfm_t *cfm = KE_MSG_ALLOC(GATT_CFM,
                                                   TASK_GATT,
                                                   PRF_SRC_TASK(OTA),
                                                   gatt_srv_att_val_set_cfm);

    cfm->req_ind_code = GATT_SRV_ATT_VAL_SET;
    cfm->token = ota_env->rx_token;
    cfm->user_lid = ota_env->user_lid;
    cfm->conidx = conidx;
    cfm->status = status;

    ke_msg_send(cfm);
}

bool ota_send_ind_ntf_generic(bool isNotification, uint8_t conidx, const uint8_t *ptrData, uint32_t length)
{
    enum gatt_evt_type evtType = isNotification ? GATT_NOTIFY : GATT_INDICATE;

    PRF_ENV_T(ota) *ota_env = PRF_ENV_GET(OTA, ota);

    if ((ota_env->ntfIndEnableFlag[conidx]) & (1 << (uint8_t)evtType))
    {

        co_buf_t *p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t *p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        // Dummy parameter provided to GATT
        uint16_t dummy = 0;

        // Inform the GATT that notification must be sent
        uint16_t ret = gatt_srv_event_send(conidx, ota_env->user_lid, dummy, evtType,
                                           ota_env->shdl + OTA_IDX_VAL, p_buf);

        // Release the buffer
        co_buf_release(p_buf);

        return (GAP_ERR_NO_ERROR == ret);
    }
    else
    {
        return false;
    }
}

__STATIC void ota_gatt_cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    uint16_t length = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    PRF_ENV_T(ota) *ota_env = PRF_ENV_GET(OTA, ota);

    // check if it's a client configuration char
    if (hdl == ota_env->shdl + OTA_IDX_NTF_CFG)
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if (hdl == ota_env->shdl + OTA_IDX_VAL)
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

    // Send the confirmation
    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, length);
}

__STATIC const gatt_srv_cb_t ota_gatt_cb = {
    .cb_event_sent      = ota_gatt_cb_event_sent,
    .cb_att_read_get    = ota_gatt_cb_att_read_get,
    .cb_att_val_set     = ota_gatt_cb_att_set,
    .cb_att_info_get    = ota_gatt_cb_att_info_get,
};

#endif /* BLE_OTA */

/// @} OTA
