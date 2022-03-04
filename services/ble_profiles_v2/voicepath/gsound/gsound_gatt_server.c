/**
 * @file gsound_gatt_server.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-17
 * 
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "ke_mem.h"
#include "gsound_gatt_server.h"
#include "prf_dbg.h"
#include "app.h"
#include "bt_if.h"
#include "btgatt_api.h"

#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
/*********************external function declearation************************/

/************************private macro defination***************************/
#define BISTO_MAX_LEN   244
#define PRF_TAG         "[GS]"
#define CCC_DATA_LEN    2

#define RD_PERM         PROP(RD) | SEC_LVL(RP, AUTH)
#define WR_REQ_PERM     PROP(WR) | SEC_LVL(WP, AUTH)
#define NTF_PERM        PROP(N)  | SEC_LVL(NIP, AUTH)

/*
 * Bisto App Communications Service:                     fe59bfa8-7fe3-4a05-9d94-99fadc69faff
 * Control Channel TX Characteristic(outgoing protobuf): 69745240-ec29-4899-a2a8-cf78fd214303
 * Control Channel RX Characteristic(incoming protobuf): 104c022e-48d6-4dd2-8737-f8ac5489c5d4
 * Audio Channel TX Characteristic(audio data):          70efdf00-4375-4a9e-912d-63522566d947
 * Audio Channel RX Characteristic(audio sidechannel):   094f376d-9bbd-4690-8c38-3b3ebd9c9ead  Doesn't exist yet.
 */

/*
 * TODO(mfk): All except BISTO_AUD_RX_UUID_128 (Audio Sidechannel) are also on 8670.
 * 8670 also has these two:
 * 0xEEA2E8A0-89F0-4985-A1E2-D91DC4A52632 (Bonded Status - readable unencrypted)
 * 0XA79E2BD1-D6E4-4D1E-8B4F-141D69011CBB (MAC Address Verification - writeable unencrypted)
 *
 * TODO(mfk): investigate what "sidechannel" is actually getting used for here.   Add bonded state and MAC address verification.
 */
#define BISTO_SERVICE_UUID_128                                                                         \
    {                                                                                                  \
        0xFF, 0xFA, 0x69, 0xDC, 0xFA, 0x99, 0x94, 0x9D, 0x05, 0x4A, 0xE3, 0x7F, 0xA8, 0xBF, 0x59, 0xFE \
    }
#define BISTO_CTRL_RX_UUID_128                                                                         \
    {                                                                                                  \
        0xD4, 0xC5, 0x89, 0x54, 0xAC, 0xF8, 0x37, 0x87, 0xD2, 0x4D, 0xD6, 0x48, 0x2E, 0x02, 0x4C, 0x10 \
    }
#define BISTO_CTRL_TX_UUID_128                                                                         \
    {                                                                                                  \
        0x03, 0x43, 0x21, 0xFD, 0x78, 0xCF, 0xA8, 0xA2, 0x99, 0x48, 0x29, 0xEC, 0x40, 0x52, 0x74, 0x69 \
    }
#define BISTO_AUD_RX_UUID_128                                                                          \
    {                                                                                                  \
        0xAD, 0x9E, 0x9C, 0xBD, 0x3E, 0x3B, 0x38, 0x8C, 0x90, 0x46, 0xBD, 0x9B, 0x6D, 0x37, 0x4F, 0x09 \
    }
#define BISTO_AUD_TX_UUID_128                                                                          \
    {                                                                                                  \
        0x47, 0xD9, 0x66, 0x25, 0x52, 0x63, 0x2D, 0x91, 0x9E, 0x4A, 0x75, 0x43, 0x00, 0xDF, 0xEF, 0x70 \
    }

/************************private type defination****************************/
typedef enum
{
    BISTO_IDX_SVC,

    BISTO_IDX_CONTROL_TX_CHAR,
    BISTO_IDX_CONTROL_TX_VAL,
    BISTO_IDX_CONTROL_TX_NTF_CFG,

    BISTO_IDX_CONTROL_RX_CHAR,
    BISTO_IDX_CONTROL_RX_VAL,

    BISTO_IDX_AUDIO_TX_CHAR,
    BISTO_IDX_AUDIO_TX_VAL,
    BISTO_IDX_AUDIO_TX_NTF_CFG,

    BISTO_IDX_AUDIO_RX_CHAR,
    BISTO_IDX_AUDIO_RX_VAL,

    BISTO_IDX_NUM,
} bisto_gatt_handles;

/************************extern function declearation***********************/

/**********************private function declearation************************/
/**
 * @brief Initialization of the GSound module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 * 
 * @param p_env         Environment variable of profile layer
 * @param p_start_hdl   Pointer of service's start handle in ATT database
 * @param sec_lvl       Security level
 * @param user_prio     User priority
 * @param params        Extended parameters
 * @param p_cb          Callback function pointer
 * @return uint8_t      Execution result of initialization
 */
static uint16_t _init(prf_data_t *p_env,
                      uint16_t *p_start_hdl,
                      uint8_t sec_lvl,
                      uint8_t user_prio,
                      const void *params,
                      const void *p_cb);

/**
 * @brief Destruction of the GSound module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 * 
 * @param p_env         Pointer of environment variable
 * @param reason        Reason to destory service
 */
static uint16_t _destroy(prf_data_t *p_env, uint8_t reason);

/**
 * @brief Handles connection creation for GSound module
 * 
 * @param p_env         Pointer of environment variable
 * @param conidx        Connection index
 * @param p_con_param   Pointer to connection parameters information
 */
static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param);

/**
 * @brief Handles disconnection for GSound module
 * 
 * @param p_env         Environment variable pointer of profile layer
 * @param conidx        Connection index
 * @param reason        Reason of disconnection
 */
static void _con_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason);

/**
 * @brief Indicates update of connection parameters
 * 
 * @param p_env         Collector or Service allocated environment data.
 * @param conidx        Connection index
 * @param p_con_param   Pointer to new connection parameters information
 */
static void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param);

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

static int _chnl_conn_update_cfm_handler(ke_msg_id_t const msgid,
                                         gsound_chnl_conn_ind_t *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id);

/************************private variable defination************************/
/*******************************************************************************
 *******************************************************************************
 * Full Bisto service Database Description
 *  - Used to add attributes into the database
 *
 *
 *******************************************************************************
 *******************************************************************************/
const struct gatt_att_desc gsound_att_db[] = {
    /**************************************************/
    // Service Declaration
    [BISTO_IDX_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_PRIMARY_SERVICE),
        PROP(RD),
        0,
    },

    /**************************************************/
    // Control TX Characteristic Declaration
    [BISTO_IDX_CONTROL_TX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,                       /// Attribute Permissions (@see enum attm_perm_mask) - **Contians Characteristic Property, but ALSO seems to include Attribute Permission if NON-VALUE Declaration
        0                              /// Attribute Max Size (note: for characteristic declaration contains handle offse) (note: for included service, contains target service handl)
    },

    // Control TX Characteristic Value
    [BISTO_IDX_CONTROL_TX_VAL] = {
        BISTO_CTRL_TX_UUID_128,   // <-- ATT UUID
        NTF_PERM | ATT_UUID(128), // <-- notify access
        BISTO_MAX_LEN,            // <-- Attribute value max size
    },

    // Control TX Characteristic - Client Characteristic Configuration Descriptor
    [BISTO_IDX_CONTROL_TX_NTF_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WR_REQ_PERM, // <-- Allow read + write request
        0,
    },

    /**************************************************/
    // Control RX Characteristic Declaration
    [BISTO_IDX_CONTROL_RX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },

    // Control RX Characteristic Value
    [BISTO_IDX_CONTROL_RX_VAL] = {
        BISTO_CTRL_RX_UUID_128,
        WR_REQ_PERM | ATT_UUID(128),
        BISTO_MAX_LEN,
    },

    /**************************************************/
    // Audio TX Characteristic Declaration
    [BISTO_IDX_AUDIO_TX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },

    // Audio TX Characteristic Value
    [BISTO_IDX_AUDIO_TX_VAL] = {
        BISTO_AUD_TX_UUID_128,
        NTF_PERM | ATT_UUID(128),
        BISTO_MAX_LEN,
    },
    // Audio TX Characteristic - Client Characteristic Configuration Descriptor
    [BISTO_IDX_AUDIO_TX_NTF_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WR_REQ_PERM,
        0,
    },

    /**************************************************/
    // Audio RX Characteristic Declaration
    [BISTO_IDX_AUDIO_RX_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },

    // Audio RX Characteristic Value
    [BISTO_IDX_AUDIO_RX_VAL] = {
        BISTO_AUD_RX_UUID_128,
        WR_REQ_PERM | PROP(WC) | ATT_UUID(128),
        BISTO_MAX_LEN,
    },
};

static const uint8_t bisto_service_uuid[] = BISTO_SERVICE_UUID_128;

const struct prf_task_cbs gsound_itf = {
    .cb_init        = _init,
    .cb_destroy     = _destroy,
    .cb_con_create  = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd     = _con_upd,
};

/// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t _gatt_srv_cb = {
    .cb_event_sent      = _cb_event_sent,
    .cb_att_read_get    = _cb_att_read_get,
    .cb_att_event_get   = _cb_att_event_get,
    .cb_att_info_get    = _cb_att_info_get,
    .cb_att_val_set     = _cb_att_val_set,
};

KE_MSG_HANDLER_TAB(gsound){
    {GSOUND_SEND_CONNECTION_UPDATE_CFM, (ke_msg_func_t)_chnl_conn_update_cfm_handler},
};

/**
 * @brief Get the connection status according to incoming op_type and op_ret
 * 
 * +--------------------+---------------+---------------+
 * |                    |   OP_ACCETP   |   OP_REJECT   |
 * +--------------------+---------------+---------------+
 * | OP_CONNECTION      |   true        |   false       |
 * +--------------------+---------------+---------------+
 * | OP_DISCONNECTION   |   false       |   false       |
 * +--------------------+---------------+---------------+
 * 
 */
static const bool __conn_status_map[GSOUND_OP_TYPE_CNT][GSOUND_OP_RET_CNT] = {
    {true, false},
    {false, false},
};

/****************************function defination****************************/
static uint16_t _init(prf_data_t *p_env,
                      uint16_t *p_start_hdl,
                      uint8_t sec_lvl,
                      uint8_t user_prio,
                      const void *params,
                      const void *p_cb)
{
    LOG_I("%s start_hdl:%d", __func__, *p_start_hdl);
    uint16_t status = GAP_ERR_NO_ERROR;

    /// allocate&init profile environment data
    PRF_ENV_T(gsound) *gsound_env = ke_malloc(sizeof(PRF_ENV_T(gsound)), KE_MEM_ATT_DB);
    memset((uint8_t *)gsound_env, 0, sizeof(PRF_ENV_T(gsound)));

    do
    {
        /// registor GATT server user
        status = gatt_user_srv_register(PREFERRED_BLE_MTU, user_prio,
                                        &_gatt_srv_cb, &(gsound_env->user_lid));

        /// check the GATT server user registation excution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        status = gatt_db_svc_add(gsound_env->user_lid, sec_lvl,
                                 bisto_service_uuid,
                                 BISTO_IDX_NUM, NULL, gsound_att_db, BISTO_IDX_NUM,
                                 &gsound_env->shdl);

        /// check the GATT database add execution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        /// initialize environment variable
        p_env->p_env = (prf_hdr_t *)gsound_env;

        /// init descriptor
        p_env->desc.msg_handler_tab = gsound_msg_handler_tab;
        p_env->desc.msg_cnt = ARRAY_SIZE(gsound_msg_handler_tab);
        p_env->desc.state = &(gsound_env->state);
        p_env->desc.idx_max = BLE_CONNECTION_MAX;

        /// init the task state
        // ke_state_set(KE_BUILD_ID(p_env->prf_task, 0), 0);
    } while (0);

    if(btif_is_gatt_over_br_edr_enabled()) {
        btif_btgatt_addsdp(ATT_SERVICE_UUID, gsound_env->shdl, gsound_env->shdl+BISTO_IDX_NUM-1);
    }
    return status;
}

static uint16_t _destroy(prf_data_t *p_env, uint8_t reason)
{
    LOG_I("%s reason:%d", __func__, reason);

    PRF_ENV_T(gsound) *gsound_env = (PRF_ENV_T(gsound) *)p_env->p_env;
    p_env->p_env = NULL;

    ke_free(gsound_env);

    return GAP_ERR_NO_ERROR;
}

static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s conidx:%d", __func__, conidx);
}

static void _con_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason)
{
    /// reset the saved connection status
    PRF_ENV_T(gsound) *gsound_env = (PRF_ENV_T(gsound) *)p_env->p_env;
    gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_CONTROL] = 0;
    gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_AUDIO] = 0;
}

static void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s", __func__);
}

static void _cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s", __func__);
    bool success = (GAP_ERR_NO_ERROR == status);
    // PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);

    if (!success)
    {
        LOG_W("GSound GATT Notification Failed: status=%d", status);
    }

    struct gsound_gatt_tx_complete_ind_t *ind =
        KE_MSG_ALLOC(GSOUND_SEND_COMPLETE_IND,
                     KE_BUILD_ID(PRF_DST_TASK(VOICEPATH), conidx),
                     KE_BUILD_ID(PRF_SRC_TASK(VOICEPATH), conidx),
                     gsound_gatt_tx_complete_ind_t);

    ind->success = success;
    ke_msg_send(ind);
}

static void _cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                                  uint16_t hdl, uint16_t offset, uint16_t max_length)
{
    LOG_I("%s hdl:%d", __func__, hdl);
    uint16_t status = GAP_ERR_NO_ERROR;
    uint16_t notify_ccc = 0;
    uint16_t dataLen = sizeof(notify_ccc);
    co_buf_t *p_data = NULL;

    // Get the address of the environment
    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);

    if (BISTO_IDX_CONTROL_TX_NTF_CFG == (hdl - gsound_env->shdl))
    {
        notify_ccc = gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_CONTROL]
                         ? 1
                         : 0;
        prf_buf_alloc(&p_data, dataLen);
        memcpy(co_buf_data(p_data), (uint8_t *)&notify_ccc, dataLen);
    }
    else if ((BISTO_IDX_AUDIO_TX_NTF_CFG == (hdl - gsound_env->shdl)))
    {
        notify_ccc = gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_CONTROL]
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
    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    LOG_I("%s hdl:%d", __func__, (hdl - gsound_env->shdl));
}

static void _cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    LOG_I("%s hdl:%d", __func__, hdl);
    uint16_t status = GAP_ERR_NO_ERROR;
    uint16_t length = 0;
    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);

    if ((BISTO_IDX_CONTROL_TX_NTF_CFG == (hdl - gsound_env->shdl)) ||
        (BISTO_IDX_AUDIO_TX_NTF_CFG == (hdl - gsound_env->shdl)))
    {
        length = 2;
    }
    else if ((BISTO_IDX_CONTROL_RX_VAL == (hdl - gsound_env->shdl)) ||
             (BISTO_IDX_AUDIO_RX_VAL == (hdl - gsound_env->shdl)))
    {
        length = 0;
    }
    else
    {
        length = 0;
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    // Send the confirmation
    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, length);
}

static void _cb_att_val_set(uint8_t conidx, uint8_t user_lid, uint16_t token,
                            uint16_t hdl, uint16_t offset, co_buf_t *p_buf)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    uint8_t *data = co_buf_data(p_buf);
    uint16_t dataLen = p_buf->data_len;
    LOG_I("%s Write req, hdl:%d, conidx:%d", __func__, hdl, conidx);

    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    uint16_t handle = hdl - gsound_env->shdl;

    if ((BISTO_IDX_CONTROL_TX_NTF_CFG == handle) ||
        (BISTO_IDX_AUDIO_TX_NTF_CFG == handle))
    {
        if (CCC_DATA_LEN == dataLen)
        {
            uint16_t cccd_val = (((uint16_t)data[1]) << 8) |
                                ((uint16_t)data[0]);
            bool is_connected = ((cccd_val & PRF_CLI_START_NTF) != 0);

            GSoundChannelType channel = (BISTO_IDX_CONTROL_TX_NTF_CFG == handle)
                                            ? GSOUND_CHANNEL_CONTROL
                                            : GSOUND_CHANNEL_AUDIO;

            bool was_connected = gsound_env->ntfIndEnableFlag[conidx][channel];

            if (is_connected != was_connected)
            {
                ke_msg_id_t msg_id = is_connected
                                         ? GSOUND_CHANNEL_CONNECTED
                                         : GSOUND_CHANNEL_DISCONNECTED;

                gsound_chnl_conn_ind_t *ind =
                    KE_MSG_ALLOC(msg_id,
                                 KE_BUILD_ID(PRF_DST_TASK(VOICEPATH), conidx),
                                 KE_BUILD_ID(PRF_SRC_TASK(VOICEPATH), conidx),
                                 gsound_chnl_conn_ind);

                ind->chnl = channel;
                ind->conidx = conidx;
                ind->user_lid = user_lid;
                ind->token = token;
                ke_msg_send(ind);

                gsound_env->conidx = conidx;
            }
            else
            {
                LOG_W("%s connection status %d not changed", __func__, is_connected);
            }

            return;
        }
        else
        {
            status = ATT_ERR_APP_ERROR;
        }
    }
    else if ((BISTO_IDX_CONTROL_RX_VAL == handle) ||
             ( BISTO_IDX_AUDIO_RX_VAL == handle))
    {
        ke_msg_id_t msg_id = (BISTO_IDX_CONTROL_RX_VAL == handle)
                                 ? GSOUND_CONTROL_RECEIVED
                                 : GSOUND_AUDIO_RECEIVED;

        struct gsound_gatt_rx_data_ind_t *ind =
            KE_MSG_ALLOC_DYN(msg_id,
                             KE_BUILD_ID(PRF_DST_TASK(VOICEPATH), conidx),
                             KE_BUILD_ID(PRF_SRC_TASK(VOICEPATH), conidx),
                             gsound_gatt_rx_data_ind_t,
                             dataLen);

        ind->user = user_lid;
        ind->token = token;
        ind->length = dataLen;
		ind->conidx = conidx;
        memcpy(ind->data, data, dataLen);
        ke_msg_send(ind);
        LOG_I("gbuf + 0x%x", (uint32_t)ind);
        status = GAP_ERR_NO_ERROR;

        if (GSOUND_CONTROL_RECEIVED == msg_id)
        {
            // pending for the rx data handling is completed
            return;
        }
    }
    else
    {
        status = ATT_ERR_APP_ERROR;
    }

    // Inform GATT about handling
    gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
}

/**
 * @brief Get GSound interface
 * 
 * @return const struct prf_task_cbs* Pointer of profile task callback
 */
const struct prf_task_cbs *gsound_prf_itf_get(void)
{
    return &gsound_itf;
}

/**
 * @brief Updata connection status
 * 
 * @param conidx        Connection index
 * @param gsound_env    Environment variable pointer
 * @param channel       GSound channel type
 * @param is_connected  Connection status to update
 */
static void _update_connection(uint8_t conidx,
                               PRF_ENV_T(gsound) *gsound_env,
                               GSoundChannelType channel,
                               bool is_connected)
{
    gsound_env->ntfIndEnableFlag[conidx][channel] = is_connected;
}

static bool __get_conn_status(uint8_t opType, uint8_t opRet)
{
    return __conn_status_map[opType][opRet];
}

static int _chnl_conn_update_cfm_handler(ke_msg_id_t const msgid,
                                         gsound_chnl_conn_ind_t *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    bool connFlag = __get_conn_status(param->op_type,
                                      (GAP_ERR_NO_ERROR == param->status
                                           ? GSOUND_OP_ACCEPT
                                           : GSOUND_OP_REJECT));

    LOG_I("conn CFM: channel=%s, flag:%d",
          GSOUND_CHANNEL_CONTROL == param->chnl ? "CONTROL" : "AUDIO",
          connFlag);

    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (GSOUND_OP_ACCEPT == param->status)
    {
        _update_connection(conidx, gsound_env, param->chnl, connFlag);
        LOG_I("ble cfm handler ok, update state %d %d",
              gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_AUDIO],
              gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_CONTROL]);
    }
    else
    {
        LOG_W("ble cfm handler error, %d %d",
              gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_AUDIO],
              gsound_env->ntfIndEnableFlag[conidx][GSOUND_CHANNEL_CONTROL]);
    }

    gatt_srv_att_val_set_cfm(param->conidx, param->user_lid, param->token, param->status);
    return KE_MSG_CONSUMED;
}

void gsound_send_control_rx_cfm(uint32_t param)
{
    struct gsound_gatt_rx_data_ind_t *pInd = (struct gsound_gatt_rx_data_ind_t *)param;
    uint16_t status = GAP_ERR_NO_ERROR;

    // Inform GATT about handling
    gatt_srv_att_val_set_cfm_t *cfm = KE_MSG_ALLOC(GATT_CFM,
                                                   TASK_GATT,
                                                   KE_BUILD_ID(PRF_SRC_TASK(VOICEPATH), pInd->conidx),
                                                   gatt_srv_att_val_set_cfm);

    cfm->req_ind_code = GATT_SRV_ATT_VAL_SET;
    cfm->token = pInd->token;
    cfm->user_lid = pInd->user;
    cfm->conidx = pInd->conidx;
    cfm->status = status;

    ke_msg_send(cfm);
    ke_msg_free(ke_param2msg((void *)param));
}

/**
 * @brief API used to send indication or notification
 * 
 * @param isNtf         Flag used to mark is notification need to send
 * @param conidx        BLE connection index
 * @param chnl          GSound channel type, @see GSoundChannelType
 * @param ptrData       Pointer of data to send
 * @param length        Length of data to send
 * @return true         Indication/notification send OK
 * @return false        Indication/notification send fail
 */
static bool _send_ind_ntf_generic(bool isNtf, uint8_t conidx, uint8_t chnl,
                                  const uint8_t *ptrData, uint32_t length)
{
    bool ret = false;

    /// Get event type, notification or indication
    enum gatt_evt_type evtType = isNtf ? GATT_NOTIFY : GATT_INDICATE;

    /// Get environment variable
    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);

    LOG_I("ntfIndEnableFlag:%d, event:%s, channel:%s",
          gsound_env->ntfIndEnableFlag[conidx][chnl],
          GATT_NOTIFY == evtType ? "NOTIFY" : "INDICATE",
          GSOUND_CHANNEL_CONTROL == chnl ? "CONTROL" : "AUDIO");

    if (gsound_env->ntfIndEnableFlag[conidx][chnl] & (1 << (uint8_t)evtType))
    {
        co_buf_t *p_buf = NULL;
        prf_buf_alloc(&p_buf, length);

        uint8_t *p_data = co_buf_data(p_buf);
        memcpy(p_data, ptrData, length);

        /// Dummy parameter provided to GATT
        uint16_t dummy = 0;

        /// get the handle index according to the channel type
        uint16_t hdl = (GSOUND_CHANNEL_CONTROL == chnl
                            ? BISTO_IDX_CONTROL_TX_VAL
                            : BISTO_IDX_AUDIO_TX_VAL);

        /// Inform the GATT that notification must be sent
        uint16_t status = gatt_srv_event_send(conidx, gsound_env->user_lid, dummy, evtType,
                                              gsound_env->shdl + hdl, p_buf);

        /// Release the buffer
        co_buf_release(p_buf);

        ret = (GAP_ERR_NO_ERROR == status);
    }

        return ret;
}

/**
 * @brief Send notification through GSound GATT service
 * 
 * @param conidx        Connection index
 * @param chnl          GSound channel type
 * @param ptrData       Pointer of data to send
 * @param length        Length of data to send
 * @return true         GSound notification send OK
 * @return false        GSound notification send failed
 */
bool gsound_gatt_server_send_notification(uint8_t conidx, uint8_t chnl,
                                          const uint8_t *ptrData, uint32_t length)
{
    bool ret = _send_ind_ntf_generic(true, conidx, chnl, ptrData, length);

    return ret;
}

void gsound_refresh_ble_database(void)
{
    PRF_ENV_T(gsound) *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    app_trigger_ble_service_discovery(gsound_env->conidx,
                                      gsound_env->shdl,
                                      gsound_env->shdl + BISTO_IDX_NUM);
}

#endif
