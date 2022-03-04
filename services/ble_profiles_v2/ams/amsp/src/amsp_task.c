/**
 * @file amsp_task.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-24
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
#if (ANCS_PROXY_ENABLE)
#include "ams_common.h"
#include "amsp.h"
#include "amsp_task.h"
#include "amsc_task.h"
#include "rwapp_config.h"
#include "prf_dbg.h"
#include "ke_mem.h"
#include "gatt.h"

/*********************external function declearation************************/

/************************private macro defination***************************/
#define PRF_TAG         "[AMSP]"
#define PROXY_TASK_ID   (TASK_ID_AMSP)
#define CLIENT_TASK_ID  (TASK_ID_AMSC)
#define PROXY_ENV_GET() PRF_ENV_GET(AMSP, amsp)

#define AMSP_CFG_LEN 2

/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/
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

/**
 * @brief Handle AMSP_WRITE_CFM event from amsc, execute the defered read confirmation op
 * 
 * @param msgid         Message ID
 * @param param         Pointer of parameter
 * @param dest_id       Destination task ID
 * @param src_id        Source task ID
 * @return int          Execution result
 */
static int _write_cfm_handler(ke_msg_id_t const msgid,
                              defer_cfm_info_t const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id);

/**
 * @brief Handle AMSP_READ_CFM event from amsc, execute the defered read confirmation op
 * 
 * @param msgid         Message ID
 * @param param         Pointer of parameter
 * @param dest_id       Destination task ID
 * @param src_id        Source task ID
 * @return int          Execution result
 */
static int _read_cfm_handler(ke_msg_id_t const msgid,
                             defer_cfm_info_t const *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id);

/**
 * @brief Handle AMSP_IND_EVT event from amsc
 * 
 * @param msgid         Message ID
 * @param param         Pointer of parameter
 * @param dest_id       Destination task ID
 * @param src_id        Source task ID
 * @return int          Execution result
 */
static int _ind_evt_handler(ke_msg_id_t const msgid,
                            gatt_srv_event_send_cmd_t *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id);

/**
 * @brief Get client handle from server handle index
 * 
 * @param conidx        Connection index
 * @param srv_hdl       Server handle index
 * @return int          Client handle
 */
static int _get_client_handle_from_server_handle_idx(uint8_t conidx, int srv_hdl);

/**
 * @brief Get server handle from client handle
 * 
 * @param conidx        Connection index
 * @param cli_hdl       Client handle
 * @return int          Server handle
 */
static int _get_server_handle_from_client_handle(uint8_t conidx, int cli_hdl);

/************************private variable defination************************/
// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t _gatt_srv_cb = {
    .cb_event_sent      = _cb_event_sent,
    .cb_att_read_get    = _cb_att_read_get,
    .cb_att_event_get   = _cb_att_event_get,
    .cb_att_info_get    = _cb_att_info_get,
    .cb_att_val_set     = _cb_att_val_set,
};

/// AKA amsp_msg_handler_tab
KE_MSG_HANDLER_TAB(amsp){
    /// handler for event from AMS client
    {AMSP_READ_CFM,             (ke_msg_func_t)_read_cfm_handler},
    {AMSP_WRITE_CFM,            (ke_msg_func_t)_write_cfm_handler},
    {AMSP_IND_EVT,              (ke_msg_func_t)_ind_evt_handler},
};

/****************************function defination****************************/
void amsp_task_init(struct ke_task_desc *task_desc, void *p_env)
{
    LOG_I("%s entry.", __func__);

    PRF_ENV_T(amsp) *amsp_env = (PRF_ENV_T(amsp) *)p_env;
    task_desc->msg_handler_tab = amsp_msg_handler_tab;
    task_desc->msg_cnt = ARRAY_SIZE(amsp_msg_handler_tab);
    task_desc->state = &(amsp_env->state);
    task_desc->idx_max = BLE_CONNECTION_MAX;

    for (uint32_t conidx = 0; conidx < BLE_CONNECTION_MAX; conidx++)
    {
        memset(&amsp_env->subEnv[conidx].client_handles[0],
               0,
               sizeof(amsp_env->subEnv[conidx].client_handles));
    }
}

static void _cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s", __func__);
}

static void _cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                             uint16_t hdl, uint16_t offset, uint16_t max_length)
{
    co_buf_t* p_buf = NULL;
    uint8_t *p_data = NULL;
    uint16_t data_len = 0;
    uint8_t status = GAP_ERR_NO_ERROR;

    PRF_ENV_T(amsp) *ams_env = PRF_ENV_GET(AMSP, amsp);
    int handle_idx = hdl - ams_env->start_hdl;
    int client_handle = _get_client_handle_from_server_handle_idx(conidx, handle_idx);
    ProxySubEnv_t *pSubEnv = &(ams_env->subEnv[conidx]);

    LOG_I("%s hdl=0x%4.4x, hdl_idx=%d, cliHdl=0x%x",
          __func__,
          hdl,
          handle_idx,
          client_handle);

    switch (handle_idx)
    {
    case AMS_PROXY_READY_VAL:
    case AMS_PROXY_READY_CFG:
    {
        data_len = 2;
        prf_buf_alloc(&p_buf, data_len);
        p_data = co_buf_data(p_buf);

        // process locally
        if (AMS_PROXY_READY_CFG == handle_idx)
        {
            LOG_I("%s read cccd val=0x%x",
                  __func__,
                  pSubEnv->ready_ntf_enable);

            p_data[0] = pSubEnv->ready_ntf_enable ? PRF_CLI_START_NTF : 0;
        }
        else if (AMS_PROXY_READY_VAL == handle_idx)
        {
            LOG_I("%s read ready val=0x%x",
                  __func__,
                  pSubEnv->ready);

            p_data[0] = pSubEnv->ready ? PRF_CLI_START_NTF : 0;
        }
        else
        {
            status = ATT_ERR_APP_ERROR;
        }

        p_data[1] = 0;
        gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, data_len, p_buf);

        /// Release the buffer
        co_buf_release(p_buf);
    }
    break;
    case AMS_PROXY_REMOTE_CMD_CFG:
    case AMS_PROXY_ENT_UPD_CFG:
    case AMS_PROXY_ENT_ATTR_VAL:
    {
        if (pSubEnv->ready)
        {/// ask client to send read command
            ke_task_id_t dest_task = prf_get_task_from_id(CLIENT_TASK_ID);
            ke_task_id_t src_task = prf_get_task_from_id(PROXY_TASK_ID);

            ams_cli_rd_cmd_t *cmd = KE_MSG_ALLOC(AMSC_READ_CMD,
                                                 KE_BUILD_ID(dest_task, conidx),
                                                 src_task,
                                                 ams_cli_rd_cmd);

            cmd->token = token;
            cmd->user_lid = user_lid;
            cmd->conidx = conidx;
            cmd->hdl = client_handle;
            ke_msg_send(cmd);
        }
        else
        {
            data_len = 0;
            status = ATT_ERR_APP_ERROR; // TODO(mfk): Is this the right error code?
            gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, data_len, p_buf);

            /// Release the buffer
            co_buf_release(p_buf);
        }
    }
    break;
    default:
    {
        data_len = 0;
        status = ATT_ERR_INVALID_HANDLE;
        gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, data_len, p_buf);

        /// Release the buffer
        co_buf_release(p_buf);
    }
    break;
    }
}

static void _cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                                   uint16_t dummy, uint16_t hdl, uint16_t max_length)
{
    LOG_I("%s", __func__);
    /// do nothing 
}

static void _cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    LOG_I("%s hdl=0x%4.4x", __func__, hdl);
    ASSERT(0, "%s not yet implemented", __func__);

    uint16_t length = 0;
    uint16_t status = GAP_ERR_NO_ERROR;

    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();

    if ((hdl == (amsp_env->start_hdl + AMS_PROXY_REMOTE_CMD_CFG)) ||
        (hdl == (amsp_env->start_hdl + AMS_PROXY_ENT_UPD_CFG)))
    {
        // CCC attribute length = 2
        length = 2;
        status = GAP_ERR_NO_ERROR;
    }
    else if ((hdl == (amsp_env->start_hdl + AMS_PROXY_READY_VAL)) ||
             (hdl == (amsp_env->start_hdl + AMS_PROXY_ENT_ATTR_VAL)))
    {
        length = 0;
        status = GAP_ERR_NO_ERROR;
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
                             uint16_t hdl, uint16_t offset, co_buf_t* p_buf)
{
    LOG_I("%s", __func__);

    uint8_t status = ATT_ERR_APP_ERROR;
    bool cfmDefer = false;
    uint8_t *data = co_buf_data(p_buf);

    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();
    ProxySubEnv_t *pSubEnv = &(amsp_env->subEnv[conidx]);
    int handle_idx = hdl - amsp_env->start_hdl;
    int client_handle = _get_client_handle_from_server_handle_idx(conidx, handle_idx);

    LOG_I("hdl=0x%4.4x, hdl_idx=%d, cliHdl=0x%x, dataLen=%d,val0=0x%x",
          hdl, handle_idx, client_handle, p_buf->data_len, data[0]);

    switch (handle_idx)
    {
    case AMS_PROXY_READY_CHAR:
    case AMS_PROXY_READY_VAL:
        // Just break. These are errors.
        status = ATT_ERR_WRITE_NOT_PERMITTED;
        break;
    case AMS_PROXY_READY_CFG:
        // process locally
        if (AMSP_CFG_LEN == p_buf->data_len)
        {
            uint16_t cccd_val = (((uint16_t)data[1]) << 8) |
                                ((uint16_t)data[0]);
            pSubEnv->ready_ntf_enable = ((cccd_val & PRF_CLI_START_NTF) != 0);
            status = GAP_ERR_NO_ERROR;
            LOG_I("write cccd_val=0x%x", pSubEnv->ready_ntf_enable);
        }
        break;
    default:
    {
        // TODO(jkessinger): Maybe check for valid handles.
        if (pSubEnv->ready)
        {
            //cfmDefer = true;

            ke_task_id_t dest_task = prf_get_task_from_id(CLIENT_TASK_ID);
            ke_task_id_t src_task  = prf_get_task_from_id(PROXY_TASK_ID);

            /// ask client to write peer's ATT
            gatt_cli_write_cmd_t *cmd =
                (gatt_cli_write_cmd_t *)KE_MSG_ALLOC_DYN(AMSC_WRITE_CMD,
                                                         KE_BUILD_ID(dest_task, pSubEnv->conidx),
                                                         src_task,
                                                         gatt_cli_write_cmd,
                                                         p_buf->data_len);

            cmd->cmd_code = GATT_CLI_WRITE;
            cmd->dummy = token;
            cmd->user_lid = user_lid;
            cmd->conidx = conidx;
            cmd->write_type = GATT_WRITE;
            cmd->hdl = client_handle;
            cmd->offset = offset;
            cmd->value_length = p_buf->data_len;
            memcpy(cmd->value, data, cmd->value_length);
            ke_msg_send(cmd);

            status = GAP_ERR_NO_ERROR;
        }
        else
        {
            status = ATT_ERR_APP_ERROR; // TODO(mfk): Is this the right error code?
        }
    }
    break;
    }

    if (!cfmDefer)
    {
        LOG_I("%s send_write_cfm status=%d", __func__, status);
        gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
    }
}

static int _write_cfm_handler(ke_msg_id_t const msgid,
                              defer_cfm_info_t const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    gatt_srv_att_val_set_cfm(param->conidx, param->user_lid, param->token, param->status);

    return KE_MSG_CONSUMED;
}

static int _read_cfm_handler(ke_msg_id_t const msgid,
                             defer_cfm_info_t const *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id)
{
    co_buf_t *p_buf = NULL;
    uint16_t data_len = 0;
    gatt_srv_att_read_get_cfm(param->conidx, param->user_lid, param->token, param->status, data_len, p_buf);

    return KE_MSG_CONSUMED;
}

static int _ind_evt_handler(ke_msg_id_t const msgid,
                            gatt_srv_event_send_cmd_t *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();
    uint8_t conidx = KE_IDX_GET(src_id);
    int srv_hdl = _get_server_handle_from_client_handle(conidx, param->hdl);

    gatt_srv_event_send_cmd_t *cmd = KE_MSG_ALLOC_DYN(GATT_CMD,
                                                      TASK_GATT,
                                                      dest_id,
                                                      gatt_srv_event_send_cmd,
                                                      param->value_length);

    cmd->cmd_code = GATT_SRV_EVENT_SEND;
    cmd->dummy = 0;
    cmd->user_lid = amsp_env->user_lid;
    cmd->conidx = param->conidx;
    cmd->evt_type = GATT_NOTIFY;
    cmd->hdl = srv_hdl;
    cmd->value_length = param->value_length;
    memcpy(cmd->value, param->value, cmd->value_length);

    /// send the command to GATT layer
    ke_msg_send(cmd);

    return KE_MSG_CONSUMED;
}

static void __notify_ready_flag(uint8_t conidx)
{
    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();
    ProxySubEnv_t *pSubEnv = &(amsp_env->subEnv[conidx]);

    LOG_I("%s pvReady=%d ntf_enabled=%d", 
          __func__, pSubEnv->ready, pSubEnv->ready_ntf_enable);

    if (pSubEnv->ready_ntf_enable)
    {
        LOG_I("Notify Ready=%d", pSubEnv->ready);

        co_buf_t *p_buf = NULL;
        uint8_t *data = NULL;
        uint16_t data_len = AMSP_CFG_LEN;
        prf_buf_alloc(&p_buf, data_len);
        uint16_t dummy = 0;

        data = co_buf_data(p_buf);
        data[0] = pSubEnv->ready ? PRF_CLI_START_NTF : 0;
        data[1] = 0;

        gatt_srv_event_send(conidx,
                            amsp_env->user_lid,
                            dummy,
                            GATT_NOTIFY,
                            amsp_env->start_hdl + AMS_PROXY_READY_VAL,
                            p_buf);
    }
}

void amsp_set_ready_flag(uint8_t conidx,
                         int handle_rem_cmd_char, int handle_rem_cmd_val, int handle_rem_cmd_cfg,
                         int handle_ent_upd_char, int handle_ent_upd_val, int handle_ent_upd_cfg,
                         int handle_ent_att_char, int handle_ent_att_val)
{
    // TODO(jkessinger): This is called with zeros if the service does not exist.
    // Maybe set ready = false.
    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();
    ProxySubEnv_t *pSubEnv = &(amsp_env->subEnv[conidx]);

    LOG_I("%s entry. pvReady=%d enable=%d",
          __func__,
          pSubEnv->ready,
          pSubEnv->ready_ntf_enable);

    pSubEnv->client_handles[AMS_PROXY_REMOTE_CMD_CHAR]  = handle_rem_cmd_char;
    pSubEnv->client_handles[AMS_PROXY_REMOTE_CMD_VAL]   = handle_rem_cmd_val;
    pSubEnv->client_handles[AMS_PROXY_REMOTE_CMD_CFG]   = handle_rem_cmd_cfg;
    pSubEnv->client_handles[AMS_PROXY_ENT_UPD_CHAR]     = handle_ent_upd_char;
    pSubEnv->client_handles[AMS_PROXY_ENT_UPD_VAL]      = handle_ent_upd_val;
    pSubEnv->client_handles[AMS_PROXY_ENT_UPD_CFG]      = handle_ent_upd_cfg;
    pSubEnv->client_handles[AMS_PROXY_ENT_ATTR_CHAR]    = handle_ent_att_char;
    pSubEnv->client_handles[AMS_PROXY_ENT_ATTR_VAL]     = handle_ent_att_val;
    pSubEnv->ready = (handle_rem_cmd_val && handle_rem_cmd_cfg &&
                      handle_ent_upd_val && handle_ent_upd_cfg &&
                      handle_ent_att_val);


    LOG_I("%s done. ready=%d", __func__, pSubEnv->ready);
    __notify_ready_flag(conidx);
}

static int _get_client_handle_from_server_handle_idx(uint8_t conidx, int server_handle_idx)
{
    if (server_handle_idx > AMS_PROXY_NUM)
    {
        return 0;
    }

    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();

    return (amsp_env->subEnv[conidx].client_handles[server_handle_idx]);
}

static int _get_server_handle_from_client_handle(uint8_t conidx, int client_handle)
{
    PRF_ENV_T(amsp) *amsp_env = PROXY_ENV_GET();
    for (int i = 0; i < AMS_PROXY_NUM; i++)
    {
        if (amsp_env->subEnv[conidx].client_handles[i] == client_handle)
        {
            return amsp_env->start_hdl + i;
        }
    }

    // Not found.
    return 0;
}

const gatt_srv_cb_t* amsp_task_get_srv_cbs(void)
{
    return &_gatt_srv_cb;
}

#endif
