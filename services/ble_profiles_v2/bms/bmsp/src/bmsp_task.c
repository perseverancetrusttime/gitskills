/**
 * @file bmsp_task.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-27
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
#include "rwip_config.h"

#if (BLE_VOICEPATH)
#include <string.h>
#include "gatt.h"
#include "prf_dbg.h"
#include "bmsp.h"
#include "bmsp_task.h"
#include "gsound.h"

/*********************external function declearation************************/

/************************private macro defination***************************/

/************************private type defination****************************/
#define PRF_TAG "[BMS]"

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
 * Returns reference to CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
static uint8_t *BmsGetCccdInternal(uint32_t handle_idx);

/************************private variable defination************************/
static BmsState bms_state;

/// Set of callbacks functions for communication with GATT as a GATT User Server
__STATIC const gatt_srv_cb_t _gatt_srv_cb = {
    .cb_event_sent      = _cb_event_sent,
    .cb_att_read_get    = _cb_att_read_get,
    .cb_att_event_get   = _cb_att_event_get,
    .cb_att_info_get    = _cb_att_info_get,
    .cb_att_val_set     = _cb_att_val_set,
};

/****************************function defination****************************/
static void _cb_event_sent(uint8_t conidx, uint8_t user_lid, uint16_t dummy, uint16_t status)
{
    LOG_I("%s", __func__);

    if (GAP_ERR_NO_ERROR != status)
    {
        LOG_W("Notification Failed: status=%d", status);
    }
}

static void _cb_att_read_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                             uint16_t hdl, uint16_t offset, uint16_t max_length)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(bmsp) *bms_env = PRF_ENV_GET(BMS, bmsp);

    int handle_idx = hdl - bms_env->shdl;
    LOG_I("%s Read, handle_idx=%d", __func__, handle_idx);

    /// allocate memory resources needed
    co_buf_t *p_buf = NULL;
    uint16_t data_len = BMS_SERVER_READ_RESP_MAX;
    prf_buf_alloc(&p_buf, data_len);
    uint8_t *p_data = co_buf_data(p_buf);

    /// fill the information
    if (BmsIsHandleValid(handle_idx))
    {
        BmsAttributeData att;
        att.value = p_data;
        BmsHandleReadReq(handle_idx, &att);
        data_len = att.length;
        status = (uint16_t)att.status;
    }
    else
    {
        data_len = 0;
        status = ATT_ERR_REQUEST_NOT_SUPPORTED;
    }

    /// send confirm
    gatt_srv_att_read_get_cfm(conidx, user_lid, token, status, data_len, p_buf);

    /// release memory resources
    co_buf_release(p_buf);

    /// set task state
    // ke_state_set(KE_BUILD_ID(prf_get_task_from_id(TASK_ID_BMS), conidx), 0);
}

static void _cb_att_event_get(uint8_t conidx, uint8_t user_lid, uint16_t token,
                              uint16_t dummy, uint16_t hdl, uint16_t max_length)
{
    LOG_I("%s", __func__);
}

static void _cb_att_info_get(uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    uint16_t data_len = 0;

    PRF_ENV_T(bmsp) *bms_env = PRF_ENV_GET(BMS, bmsp);
    int handle_idx = hdl - bms_env->shdl;
    LOG_I("%s handle_idx=%d", __func__, handle_idx);

    if (BmsIsHandleValid(handle_idx))
    {
        // TODO(mosesd): will need to return len and status for each characteristic
    }
    else
    {
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    gatt_srv_att_info_get_cfm(conidx, user_lid, token, status, data_len);
}

static void _cb_att_val_set(uint8_t conidx, uint8_t user_lid, uint16_t token,
                            uint16_t hdl, uint16_t offset, co_buf_t *p_buf)
{
    uint32_t status = GAP_ERR_NO_ERROR;

    PRF_ENV_T(bmsp) *bms_env = PRF_ENV_GET(BMS, bmsp);
    int handle_idx = hdl - bms_env->shdl;
    LOG_I("%s handle_idx:%d", __func__, handle_idx);

    uint8_t *data = co_buf_data(p_buf);

    if (BmsIsHandleValid(handle_idx))
    {
        bool notifiable;
        notifiable = BmsHandleWriteRequest(handle_idx, data,
                                           p_buf->data_len, &status);

        if (status == GAP_ERR_NO_ERROR)
        {
            if (notifiable)
            {
                LOG_I("send ntf %d:0x%02X conidx=%d", p_buf->data_len, data[0], conidx);
                // Now echo back exactly what we received
                co_buf_t* p_ntf = NULL;
                prf_buf_alloc(&p_ntf, p_buf->data_len);

                uint8_t* p_data = co_buf_data(p_ntf);
                memcpy(p_data, data, p_buf->data_len);

                // Dummy parameter provided to GATT
                uint16_t dummy = 0;
                gatt_srv_event_send(conidx, bms_env->user_lid, dummy, GATT_NOTIFY, hdl, p_ntf);
            }
        }
    }
    else
    {
        status = ATT_ERR_APP_ERROR;
    }

    gatt_srv_att_val_set_cfm(conidx, user_lid, token, status);
}

static uint8_t *BmsGetCccdInternal(uint32_t handle_idx)
{
    switch (handle_idx)
    {
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
        return &bms_state.aapp_cccd;
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
        return &bms_state.bc_cccd;
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
        return &bms_state.mc_cccd;
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
        return &bms_state.ms_cccd;
    default:
        return NULL;
    }
}

/**
 * Checks if handle is tied to BMS server
 */
bool BmsIsHandleValid(uint32_t handle_idx)
{
    return (handle_idx >= BISTO_IDX_BMS_START &&
            handle_idx <= BISTO_IDX_BMS_END);
}

/**
 * Returns CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
uint8_t BmsGetCccd(uint32_t handle_idx)
{
    uint8_t *cccd_ref = BmsGetCccdInternal(handle_idx);
    ASSERT(NULL != cccd_ref, "cccd_ref is null");
    return *cccd_ref;
}

/**
 * Sets CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
void BmsSetCccd(uint32_t handle_idx, uint8_t cccd)
{
    uint8_t *cccd_ref = BmsGetCccdInternal(handle_idx);
    ASSERT(NULL != cccd_ref, "cccd_ref is null");
    *cccd_ref = (cccd & (BLE_ATT_CCC_NTF_BIT | BLE_ATT_CCC_IND_BIT));
}

/**
 * Checks if CCCD of specific handle/characteristic is configured
 * to be notifiable i.e. client has indicated they want notification
 * to be sent
 */
bool BmsIsNotifiable(uint32_t handle_idx)
{
    return (((BmsGetCccd(handle_idx)) & BLE_ATT_CCC_NTF_BIT) != 0);
}

/**
 * Handles a Bms Read Request
 */
void BmsHandleReadReq(uint32_t handle_idx,
                      BmsAttributeData *const att_result)
{
    ASSERT((NULL != att_result) && (NULL != att_result->value),
           "att_result:0x%08x, value:0x%08x",
           (uint32_t)att_result,
           (uint32_t)att_result->value);

    LOG_I("read req %d", handle_idx);

    switch (handle_idx)
    {
    /* For reads of Active App, return the stored value */
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
        memcpy(att_result->value, bms_state.bms_server_aapp, sizeof(bms_state.bms_server_aapp));
        att_result->length = sizeof(bms_state.bms_server_aapp);
        att_result->status = BLE_ATT_OK;
        LOG_I("read AAPP %d", att_result->length);
        break;

    /* The Broadcast Command, Media Command, and
     * Media Status characteristics should never be read */
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
        LOG_I("%s: read not permitted", __func__);
        att_result->length = 0;
        att_result->status = BLE_ATT_READ_NOT_ALLOWED;
        break;

    /* Handle client config (CCCD) reads correctly */
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
    {
        att_result->value[0] = BmsGetCccd(handle_idx);
        att_result->value[1] = 0;
        att_result->status = BLE_ATT_OK;
        att_result->length = 2; // CCCD reads always 2
        LOG_I("read CCCD %d", att_result->value[0]);
        break;
    }

    /* Everything else is invalid */
    default:
        LOG_I("read invalid");
        att_result->status = BLE_ATT_INVALID_HANDLE;
        att_result->length = 0;
        break;
    }
}

/**
 * Handles the BMS write request.
 *  For CCCD writes, store config locally
 *  For Value writes, store value locally and echo back to client
 *  via ATT notify
 *
 * Return
 *  - true:  if a notification is required - write req was on a Characteristic value
 *           and notification for that characteristic was enabled in CCCD
 *  - false: otherwise
 */
bool BmsHandleWriteRequest(uint32_t handle_idx, uint8_t const *data, uint32_t length, uint32_t *const status)
{
    uint32_t result = BLE_ATT_OK;
    bool notifiable = false;

    LOG_I("write req %d", handle_idx);
    switch (handle_idx)
    {
    /* Handle Active App Write Request */
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
        // Length must be the exact size of Active App
        if (length != BMS_SERVER_AAPP_SIZE)
        {
            result = BLE_ATT_INVALID_ATT_VAL_EN;
            LOG_I("write APP invalid len %d", length);
        }
        else
        {
            memcpy(bms_state.bms_server_aapp, data, BMS_SERVER_AAPP_SIZE);
            notifiable = BmsIsNotifiable(handle_idx);
            LOG_I("write AAPP");
        }
        break;

        /* Handle write request for other Characteristics */
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
        // Although we don't store anything, client must have sent a non-0 len
        if (length == 0)
        {
            result = BLE_ATT_INVALID_ATT_VAL_EN;
            LOG_I("write Val invalid len %d", length);
        }
        else
        {
            notifiable = BmsIsNotifiable(handle_idx);
        }
        break;

    /* Handle client config (CCCD) write requests */
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
        // Store CCCD, unless we have invalid length
        if (length != 2)
        {
            result = BLE_ATT_INVALID_ATT_VAL_EN;
        }
        else
        {
            BmsSetCccd(handle_idx,
                       (data[0] & (BLE_ATT_CCC_NTF_BIT | BLE_ATT_CCC_IND_BIT)));
            LOG_I("write CCCD %d",
                  (data[0] & (BLE_ATT_CCC_NTF_BIT | BLE_ATT_CCC_IND_BIT)));
        }
        break;

    default:
        LOG_I("write invalid");
        result = BLE_ATT_INVALID_HANDLE;
        break;
    }

    if (status)
    {
        *status = result;
    }
    return notifiable;
}

void bmsp_task_init(struct ke_task_desc *task_desc, void *p_env)
{
    PRF_ENV_T(bmsp) *bms_env = (PRF_ENV_T(bmsp) *)p_env;
    task_desc->msg_handler_tab = NULL; // bms_msg_handler_tab;
    task_desc->msg_cnt = 0; // ARRAY_SIZE(bms_msg_handler_tab);
    task_desc->state = &(bms_env->state);
    task_desc->idx_max = BLE_CONNECTION_MAX;
}

const void* bmsp_task_get_srv_cbs(void)
{
    return (void *)&_gatt_srv_cb;
}

#endif /// #if (BLE_VOICEPATH)
