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

/**
 ****************************************************************************************
 * @addtogroup ANCCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_ANC_CLIENT)
#include "anc_common.h"
#include "ancc.h"
#include "ancc_task.h"
#include "ancs_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "prf_dbg.h"

#if (ANCS_PROXY_ENABLE)
#include "ancs_task.h"
#endif

#define PRF_TAG         "[ANCC]"
#define HAVE_DEFERED_OP 1
#define NO_DEFERED_OP   0

/*
 * STRUCTURES
 ****************************************************************************************
 */
/// ANCS UUID
const uint8_t anc_svc_uuid[GATT_UUID_128_LEN] = {
    0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
    0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79,
};

/// State machine used to retrieve ANCS characteristics information
const prf_char128_def_t ancc_anc_char[ANCC_CHAR_MAX] = {
    /// Notification Source
    [ANCC_CHAR_NTF_SRC] = {
        {0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C,
         0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F},
        PRF_ATT_REQ_PRES_MAND,
        PROP(N),
    },

    /// Control Point
    [ANCC_CHAR_CTRL_PT] = {
        {0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98,
         0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69},
        PRF_ATT_REQ_PRES_OPT,
        PROP(WR),
    },

    /// Data Source
    [ANCC_CHAR_DATA_SRC] = {
        {0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE,
         0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22},
        PRF_ATT_REQ_PRES_OPT,
        PROP(N),
    },
};

/// State machine used to retrieve ANCS characteristic descriptor information
const prf_desc_def_t ancc_anc_desc[ANCC_DESC_MAX] = {
    /// Notification Source Char. - Client Characteristic Configuration
    [ANCC_DESC_NTF_SRC_CL_CFG] = {
        GATT_DESC_CLIENT_CHAR_CFG,
        PRF_ATT_REQ_PRES_OPT,
        ANCC_CHAR_NTF_SRC,
    },
    /// Data Source Char. - Client Characteristic Configuration
    [ANCC_DESC_DATA_SRC_CL_CFG] = {
        GATT_DESC_CLIENT_CHAR_CFG,
        PRF_ATT_REQ_PRES_OPT,
        ANCC_CHAR_DATA_SRC,
    },
};

/*
 * LOCAL FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int _enable_req_handler(ke_msg_id_t const msgid,
                               struct ancc_enable_req *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int _write_cmd_handler(ke_msg_id_t const msgid,
                              gatt_cli_write_cmd_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int _read_cmd_handler(ke_msg_id_t const msgid,
                             anc_cli_rd_cmd_t *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int _write_cmd_handler(ke_msg_id_t const msgid,
                              gatt_cli_write_cmd_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id);

/**
 * @brief Handles reception of the @ref GATT_CMP_EVT message.
 * 
 * @param msgid         msgid Id of the message received.
 * @param param         param Pointer to the parameters of the message.
 * @param dest_id       dest_id ID of the receiving task instance.
 * @param src_id        src_id ID of the sending task instance.
 * @return int          If the message was consumed or not.
 */
static int _gatt_cmp_evt_handler(ke_msg_id_t const msgid,
                                 gatt_proc_cmp_evt_t *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

/**
 * @brief This function is called when GATT client user discovery procedure is over.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param status        Status of the procedure (@see enum hl_err)
 */
static void _cb_discover_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

/**
 * @brief This function is called when GATT client user read procedure is over.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param status        Status of the procedure (@see enum hl_err)
 */
static void _cb_read_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

/**
 * @brief This function is called when GATT client user write procedure is over.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param status        Status of the procedure (@see enum hl_err)
 */
static void _cb_write_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status);

/**
 * @brief This function is called when GATT client user has initiated a write procedure.
 *
 *        @see gatt_cli_att_val_get_cfm shall be called to provide attribute value.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param dummy         Dummy parameter provided by upper layer for command execution - 0x0000 else.
 * @param hdl           Attribute handle
 * @param offset        Value offset
 * @param max_length    Maximum value length to return
 */
static void _cb_att_val_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy,
                            uint16_t hdl, uint16_t offset, uint16_t max_length);

/**
 * @brief This function is called when a full service has been found during a discovery procedure.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param hdl           First handle value of following list
 * @param disc_info     Discovery information (@see enum gatt_svc_disc_info)
 * @param nb_att        Number of attributes
 * @param p_atts        Pointer to attribute information present in a service
 */
static void _cb_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint8_t disc_info,
                    uint8_t nb_att, const gatt_svc_att_t* p_atts);

/**
 * @brief This function is called when a service has been found during a discovery procedure.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param start_hdl     Service start handle
 * @param end_hdl       Service end handle
 * @param uuid_type     UUID Type (@see enum gatt_uuid_type)
 * @param p_uuid        Pointer to service UUID (LSB first)
 */
static void _cb_svc_info(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t start_hdl, uint16_t end_hdl,
                         uint8_t uuid_type, const uint8_t* p_uuid);

/**
 * @brief This function is called when an include service has been found during a discovery procedure.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param inc_svc_hdl   Include service attribute handle
 * @param start_hdl     Service start handle
 * @param end_hdl       Service end handle
 * @param uuid_type     UUID Type (@see enum gatt_uuid_type)
 * @param p_uuid        Pointer to service UUID (LSB first)
 */
static void _cb_inc_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t inc_svc_hdl,
                        uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid);

/**
 * @brief This function is called when a characteristic has been found during a discovery procedure.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param char_hdl      Characteristic attribute handle
 * @param val_hdl       Value handle
 * @param prop          Characteristic properties (@see enum gatt_att_info_bf - bits [0-7])
 * @param uuid_type     UUID Type (@see enum gatt_uuid_type)
 * @param p_uuid        Pointer to characteristic value UUID (LSB first)
 */
static void _cb_char(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t char_hdl, uint16_t val_hdl,
                     uint8_t prop, uint8_t uuid_type, const uint8_t* p_uuid);

/**
 * @brief This function is called when a descriptor has been found during a discovery procedure.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param desc_hdl      Characteristic descriptor attribute handle
 * @param uuid_type     UUID Type (@see enum gatt_uuid_type)
 * @param p_uuid        Pointer to attribute UUID (LSB first)
 */
static void _cb_desc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t desc_hdl,
                     uint8_t uuid_type, const uint8_t* p_uuid);

/**
 * @brief This function is called during a read procedure when attribute value is retrieved
 *        form peer device.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param dummy         Dummy parameter provided by upper layer for command execution
 * @param hdl           Attribute handle
 * @param offset        Value offset
 * @param p_data        Pointer to buffer that contains attribute value starting from offset
 */
static void _cb_att_val(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint16_t offset,
                        co_buf_t* p_data);

/**
 * @brief This function is called when a notification or an indication is received onto
 *        register handle range (@see gatt_cli_event_register).
 * 
 *        @see gatt_cli_val_event_cfm must be called to confirm event reception.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param token         Procedure token that must be returned in confirmation function
 * @param evt_type      Event type triggered (@see enum gatt_evt_type)
 * @param complete      True if event value if complete value has been received
 *                      False if data received is equals to maximum attribute protocol value.
 *                      In such case GATT Client User should perform a read procedure.
 * @param hdl           Attribute handle
 * @param p_data        Pointer to buffer that contains attribute value
 */
static void _cb_att_val_evt(uint8_t conidx, uint8_t user_lid, uint16_t token, uint8_t evt_type, bool complete, 
                            uint16_t hdl, co_buf_t* p_data);

/**
 * @brief Event triggered when a service change has been received or if an attribute
 *        transaction triggers an out of sync error.
 * 
 * @param conidx        Connection index
 * @param user_lid      GATT user local identifier
 * @param out_of_sync   True if an out of sync error has been received
 * @param start_hdl     Service start handle
 * @param end_hdl       Service end handle
 */
static void _cb_svc_changed(uint8_t conidx, uint8_t user_lid, bool out_of_sync, uint16_t start_hdl, uint16_t end_hdl);

/*
 * PRIVATE VARIABLE DECLARATIONS
 ****************************************************************************************
 */
static const gatt_cli_cb_t ancc_gatt_cli_cbs = {
    .cb_discover_cmp    = _cb_discover_cmp,
    .cb_read_cmp        = _cb_read_cmp,
    .cb_write_cmp       = _cb_write_cmp,
    .cb_att_val_get     = _cb_att_val_get,
    .cb_svc             = _cb_svc,
    .cb_svc_info        = _cb_svc_info,
    .cb_inc_svc         = _cb_inc_svc,
    .cb_char            = _cb_char,
    .cb_desc            = _cb_desc,
    .cb_att_val         = _cb_att_val,
    .cb_att_val_evt     = _cb_att_val_evt,
    .cb_svc_changed     = _cb_svc_changed,
};

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(ancc) { /// AKA ancc_msg_handler_tab
    /// handlers for command from upper layer
    {ANCC_ENABLE_REQ,       (ke_msg_func_t)_enable_req_handler},
    {ANCC_WRITE_CMD,        (ke_msg_func_t)_write_cmd_handler},
    {ANCC_READ_CMD,         (ke_msg_func_t)_read_cmd_handler},

    /// handlers for command from GATT layer
    {GATT_CMP_EVT,          (ke_msg_func_t)_gatt_cmp_evt_handler},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static int _enable_req_handler(ke_msg_id_t const msgid,
                               struct ancc_enable_req *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
    // Get connection index
    uint8_t conidx = param->conidx;
    TRACE(3, "ANCSC %s Entry.conidx=%d",__func__,  conidx);

    if (ancc_env->env[conidx] == NULL)
    {
        TRACE(1, "ANCSC %s passed state check", __func__);
        // allocate environment variable for task instance
        ancc_env->env[conidx] = (struct ancc_cnx_env *)ke_malloc(sizeof(struct ancc_cnx_env), KE_MEM_ATT_DB);
        ASSERT(ancc_env->env[conidx], "%s alloc error", __func__);
        memset(ancc_env->env[conidx], 0, sizeof(struct ancc_cnx_env));

        ancc_env->env[conidx]->last_char_code = ANCC_ENABLE_OP_CODE;

        /// Start discovering
        gatt_cli_discover_svc_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                        TASK_GATT,
                                                        dest_id,
                                                        gatt_cli_discover_svc_cmd);

        cmd->cmd_code = GATT_CLI_DISCOVER_SVC;
        cmd->dummy = 0;
        cmd->user_lid = ancc_env->user_lid;
        cmd->conidx = conidx;
        cmd->disc_type = GATT_DISCOVER_SVC_PRIMARY_BY_UUID;
        cmd->full = true;
        cmd->start_hdl = GATT_MIN_HDL;
        cmd->end_hdl = GATT_MAX_HDL;
        cmd->uuid_type = GATT_UUID_128;
        memcpy(cmd->uuid, anc_svc_uuid, GATT_UUID_128_LEN);

        /// send msg to GATT layer
        ke_msg_send(cmd);

        // Go to DISCOVERING state
        // ke_state_set(dest_id, ANCC_DISCOVERING);

        // Configure the environment for a discovery procedure
        ancc_env->env[conidx]->last_req = GATT_DECL_PRIMARY_SERVICE;
    }

    // send an error if request fails
    if (status != GAP_ERR_NO_ERROR)
    {
        ancc_enable_rsp_send(ancc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

static int _write_cmd_handler(ke_msg_id_t const msgid,
                              gatt_cli_write_cmd_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    LOG_I("%s hdl=0x%4.4x, conidx=%d, len=%d",
          __func__, param->hdl, param->conidx, param->value_length);

    uint8_t conidx = KE_IDX_GET(dest_id);

    // Get the address of the environment
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);

    if (ancc_env != NULL)
    { /// ask GATT layer to read server ATT value
        gatt_cli_write_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                     TASK_GATT,
                                                     dest_id,
                                                     gatt_cli_write_cmd,
                                                     param->value_length);

        cmd->cmd_code = GATT_CLI_WRITE;
        cmd->dummy = HAVE_DEFERED_OP;
        cmd->user_lid = ancc_env->user_lid;
        cmd->conidx = param->conidx;
        cmd->write_type = param->write_type;
        cmd->hdl = param->hdl;
        cmd->offset = param->offset;
        cmd->value_length = param->value_length;
        memcpy((uint8_t*)cmd->value, (uint8_t*)param->value, param->value_length);

        ancc_env->wrInfo[conidx].conidx = param->conidx;
        ancc_env->wrInfo[conidx].user_lid = param->user_lid;
        ancc_env->wrInfo[conidx].token = param->dummy;
        ancc_env->wrInfo[conidx].status = GAP_ERR_NO_ERROR;

        // Send the message
        ke_msg_send(cmd);
    }
    else
    {
        //        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->handle,
        //        ANCC_WRITE_CL_CFG_OP_CODE);
        ASSERT(0, "%s implement me", __func__);
    }

    return KE_MSG_CONSUMED;
}


static int _read_cmd_handler(ke_msg_id_t const msgid,
                             anc_cli_rd_cmd_t *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id)
{
    LOG_I("%s Entry. hdl=0x%4.4x", __func__, param->hdl);

    // Get the address of the environment
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);

    if (ancc_env != NULL)
    {
        // Send the read request
        gatt_cli_read_cmd_t *cmd = KE_MSG_ALLOC(GATT_CMD,
                                                TASK_GATT,
                                                dest_id,
                                                gatt_cli_read_cmd);

        cmd->cmd_code = GATT_CLI_READ;
        cmd->dummy = 0;
        cmd->user_lid = ancc_env->user_lid;
        cmd->conidx = param->conidx;
        cmd->hdl = param->hdl;
        cmd->offset = 0;
        cmd->length = 0;

        ke_msg_send(cmd);
    }
    else
    {
        ASSERT(0, "%s implement me", __func__);
    }

    return KE_MSG_CONSUMED;
}

static int _gatt_cmp_evt_handler(ke_msg_id_t const msgid,
                                 gatt_proc_cmp_evt_t *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    LOG_I("%s", __func__);

    return KE_MSG_CONSUMED;
}

static void _cb_discover_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s status=%d, conidx=%d", __func__, status, conidx);;

    // Get the address of the environment
    PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
    ASSERT(ancc_env->env[conidx], "%s %d", __func__, __LINE__);

    uint8_t ret = status;
    LOG_I("%s", __func__);

    if ((ATT_ERR_ATTRIBUTE_NOT_FOUND == status) ||
        (GAP_ERR_NO_ERROR == status))
    {
        // Discovery
        // check characteristic validity
        if (ancc_env->env[conidx]->nb_svc == 1)
        {
            ret = prf_check_svc128_validity(ANCC_CHAR_MAX,
                                            ancc_env->env[conidx]->anc.chars, ancc_anc_char,
                                            ANCC_DESC_MAX,
                                            ancc_env->env[conidx]->anc.descs, ancc_anc_desc);
        }
        // too much services
        else if (ancc_env->env[conidx]->nb_svc > 1)
        {
            ret = PRF_ERR_MULTIPLE_SVC;
        }
        // no services found
        else
        {
            ret = PRF_ERR_STOP_DISC_CHAR_MISSING;
        }
    }

    ancc_enable_rsp_send(ancc_env, conidx, ret);
#if (ANCS_PROXY_ENABLE)
    LOG_I("NSVal=0x%4.4x, NSCfg=0x%4.4x",
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl);
    LOG_I("DSVal=0x%4.4x, DSCfg=0x%4.4x",
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl);
    LOG_I("CPVal=0x%4.4x",
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl);
    ancs_proxy_set_ready_flag(conidx,
                              0,
                              ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl,
                              ancc_env->env[conidx]->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl,
                              0,
                              ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl,
                              ancc_env->env[conidx]->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl,
                              0,
                              ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl);
    // ke_state_set(KE_BUILD_ID(PRF_SRC_TASK(ANCC), conidx), ANCC_IDLE);
#endif
}

static void _cb_read_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s", __func__);
}

static void _cb_write_cmp(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s", __func__);

    if (HAVE_DEFERED_OP == dummy)
    {
        // Get the address of the environment
        PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
        ASSERT(ancc_env->env[conidx], "%s %d", __func__, __LINE__);

        defer_cfm_info_t *cfm = KE_MSG_ALLOC(ANCS_WRITE_CFM,
                                             KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSP), conidx),
                                             KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSC), conidx),
                                             defer_cfm_info);

        /// restore defered confirm op
        memcpy((uint8_t *)cfm, (uint8_t *)&ancc_env->wrInfo[conidx], sizeof(defer_cfm_info_t));
        ke_msg_send(cfm);
    }
}

static void _cb_att_val_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t dummy,
                            uint16_t hdl, uint16_t offset, uint16_t max_length)
{
    LOG_I("%s", __func__);
}

static void _cb_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint8_t disc_info,
                    uint8_t nb_att, const gatt_svc_att_t* p_atts)
{
    LOG_I("%s incoming hdl=0x%4.4x, nb_att=%d",
          __func__, hdl, nb_att);

    LOG_I("charac[0] info: prop=%d, handle=0x%4.4x, att_type=%d",
          p_atts[0].info.charac.prop, p_atts[0].info.charac.val_hdl, p_atts[0].att_type);
    LOG_I("svc[0] info: att_type=%d, inc_svc.end_hdl=0x%4.4x, inc_svc.start_hdl=0x%4.4x",
          p_atts[0].att_type, p_atts[0].info.svc.end_hdl, p_atts[0].info.svc.start_hdl);

        PRF_ENV_T(ancc) *ancc_env = PRF_ENV_GET(ANCC, ancc);
        ASSERT_INFO(ancc_env, 0, 0);
        ASSERT_INFO(ancc_env->env[conidx], 0, 0);
        ancc_env->shdl = hdl;

        if (ancc_env->env[conidx]->nb_svc == 0)
        {
            LOG_I("retrieving characteristics and descriptors.");
            // Retrieve ANC characteristics and descriptors
            // TODO: freddie to confirm start handle value
            prf_extract_svc128_info(hdl, nb_att, p_atts,
                                    ANCC_CHAR_MAX, ancc_anc_char, ancc_env->env[conidx]->anc.chars,
                                    ANCC_DESC_MAX, ancc_anc_desc, ancc_env->env[conidx]->anc.descs);

            // Even if we get multiple responses we only store 1 range
            // TODO: freddie to confirm start handle and end handle value
            ancc_env->env[conidx]->anc.svc.shdl = p_atts[0].info.svc.start_hdl;
            ancc_env->env[conidx]->anc.svc.ehdl = p_atts[0].info.svc.end_hdl;
        }

        ancc_env->env[conidx]->nb_svc++;
}

static void _cb_svc_info(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t start_hdl, uint16_t end_hdl,
                        uint8_t uuid_type, const uint8_t* p_uuid)
{
    LOG_I("%s", __func__);
}

static void _cb_inc_svc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t inc_svc_hdl,
                        uint16_t start_hdl, uint16_t end_hdl, uint8_t uuid_type, const uint8_t* p_uuid)
{
    LOG_I("%s", __func__);
}

static void _cb_char(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t char_hdl, uint16_t val_hdl,
                    uint8_t prop, uint8_t uuid_type, const uint8_t* p_uuid)
{
    LOG_I("%s", __func__);
}

static void _cb_desc(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t desc_hdl,
                    uint8_t uuid_type, const uint8_t* p_uuid)
{
    LOG_I("%s", __func__);
}

static void _cb_att_val(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t hdl, uint16_t offset,
                        co_buf_t* p_data)
{
    LOG_I("%s hdl:%d, received dataLen:%d, data:", __func__, hdl, p_data->data_len);
    DUMP8("0x%02x ", co_buf_data(p_data), p_data->data_len);

    // TODO: should write back to peer? how?
}

static void _cb_att_val_evt(uint8_t conidx, uint8_t user_lid, uint16_t token, uint8_t evt_type, bool complete, 
                            uint16_t hdl, co_buf_t* p_data)
{
    LOG_I("%s evt_type:%d", __func__, evt_type);
    ASSERT(complete, "%s %d, event not complete", __func__, __LINE__);

    gatt_srv_event_send_cmd_t *cmd =
        KE_MSG_ALLOC_DYN(ANCS_IND_EVT,
                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCSP), conidx),
                         KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCC), conidx),
                         gatt_srv_event_send_cmd,
                         p_data->data_len);

    cmd->conidx = conidx;
    cmd->hdl = hdl;
    cmd->value_length = p_data->data_len;
    memcpy(cmd->value, co_buf_data(p_data), p_data->data_len);
    ke_msg_send(cmd);
}

static void _cb_svc_changed(uint8_t conidx, uint8_t user_lid, bool out_of_sync, uint16_t start_hdl, uint16_t end_hdl)
{
    LOG_I("%s", __func__);
}

void ancc_task_init(struct ke_task_desc *task_desc, void *p_env)
{
    TRACE(1, "ANCSC %s Entry.", __func__);
    // Get the address of the environment
    PRF_ENV_T(ancc) *ancc_env = (PRF_ENV_T(ancc) *)p_env;

    task_desc->msg_handler_tab = ancc_msg_handler_tab;
    task_desc->msg_cnt = ARRAY_LEN(ancc_msg_handler_tab);
    task_desc->state = ancc_env->state;
    task_desc->idx_max = ANCC_IDX_MAX;
}

const gatt_cli_cb_t* ancc_task_get_cli_cbs(void)
{
    return (&ancc_gatt_cli_cbs);
}

#endif //(BLE_ANC_CLIENT)

/// @} ANCCTASK
