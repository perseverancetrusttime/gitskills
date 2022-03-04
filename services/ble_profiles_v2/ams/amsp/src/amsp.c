/**
 * @file amsp.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-23
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
#include "rwapp_config.h"

#if (ANCS_PROXY_ENABLE)
#include "amsp.h"
#include "amsp_task.h"
#include "prf_dbg.h"
#include "ke_mem.h"
#include "bt_if.h"
#include "btgatt_api.h"

/*********************external function declearation************************/

/************************private macro defination***************************/
#define PRF_TAG         "[AMSP]"

// TODO(mfk): what is the right number?  Actually, this is likely irrelevant
// since I think it's a protobuf max length.
#define AMS_MAX_LEN     244

/*
 * 55F80AEF-D89F-41A4-9E36-0FFC88DC81CE
 Remote Command (UUID: 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2) is a direct proxy
 of Apple's AMS Remote Command characteristic and matches its UUID and
 properties. AMS Ready (UUID: 3ADF41AF-F7A1-4E16-863E-53A188D5BF8D), read/notify
 which provides the current state of AMS (0 - not ready, 1 - ready) Entity
 Update (UUID: 2F7CABCE-808D-411F-9A0C-BB92BA96C102) is a direct proxy of
 Apple's AMS Entity Update characteristic and matches its UUID and properties.
 Entity Attribute (UUID: C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7) is a direct proxy
 of Apple's AMS Entity Attribute characteristic and matches its UUID and
 properties.
 */
#define PROXY_SERVICE_UUID                                                                             \
    {                                                                                                  \
        0xCE, 0x81, 0xDC, 0x88, 0xFC, 0x0F, 0x36, 0x9e, 0xa4, 0x41, 0x9f, 0xd8, 0xef, 0x0a, 0xf8, 0x55 \
    }
#define PROXY_CHAR_READY                                                                               \
    {                                                                                                  \
        0x8D, 0xBF, 0xD5, 0x88, 0xA1, 0x53, 0x3E, 0x86, 0x16, 0x4E, 0xA1, 0xF7, 0xAF, 0x41, 0xDF, 0x3A \
    }
#define PROXY_CHAR_REMOTE_CMD                                                                          \
    {                                                                                                  \
        0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8, 0x8A, 0x4A, 0xB1, 0x57, 0xD8, 0x81, 0x3C, 0x9B \
    }
#define PROXY_CHAR_ENT_UPDATE                                                                          \
    {                                                                                                  \
        0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A, 0x1F, 0x41, 0x8D, 0x80, 0xCE, 0xAB, 0x7C, 0x2F \
    }
#define PROXY_CHAR_ENT_ATTR                                                                            \
    {                                                                                                  \
        0xD7, 0xD5, 0xBB, 0x70, 0xA8, 0xA3, 0xAB, 0xA6, 0xD8, 0x46, 0xAB, 0x23, 0x8C, 0xF3, 0xB2, 0xC6 \
    }

/***********************
 * BES Characteristic Property Flags
 ************************
 *   PERM_MASK_BROADCAST       /// Broadcast descriptor present
 *   PERM_MASK_RD              /// Read Access Mask
 *   PERM_MASK_WRITE_COMMAND   /// Write Command Enabled attribute Mask
 *   PERM_MASK_WRITE_REQ       /// Write Request Enabled attribute Mask
 *   PERM_MASK_NTF             /// Notification Access Mask
 *   PERM_MASK_IND             /// Indication Access Mask
 *   PERM_MASK_WRITE_SIGNED    /// Write Signed Enabled attribute Mask
 *   PERM_MASK_EXT             /// Extended properties descriptor present
 */
#define RD_PERM         PROP(RD) | SEC_LVL(RP, AUTH)
#define WRITE_REQ_PERM  PROP(WR) | SEC_LVL(WP, AUTH)
#define WRITE_CMD_PERM  PROP(WC) | SEC_LVL(WP, AUTH)
#define NTF_PERM        PROP(N)  | SEC_LVL(NIP, AUTH)

/************************private type defination****************************/

/************************extern function declearation***********************/

/**********************private function declearation************************/
/**
 * @brief Initialization of the Profile module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 * 
 * @param p_env         Collector or Service allocated environment data.
 * @param p_start_hdl   Service start handle (0 - dynamically allocated), only applies for services.
 * @param sec_lvl       Security level (@see enum enum gatt_svc_info_bf)
 * @param user_prio     GATT User priority
 * @param p_params      Configuration parameters of profile collector or service (32 bits aligned)
 * @param p_cb          Callback structure that handles event from profile
 * @return uint16_t     status code to know if profile initialization succeed or not.
 */
static uint16_t _init(prf_data_t* p_env, uint16_t* p_start_hdl, uint8_t sec_lvl, uint8_t user_prio,
                          const void* p_params, const void* p_cb);

/**
 * @brief Destruction of the profile module - due to a reset or profile remove.
 * 
 * @param p_env         Collector or Service allocated environment data.
 * @param reason        Destroy reason (@see enum prf_destroy_reason)
 * @return uint16_t     status of the destruction, if fails, profile considered not removed.
 */
static uint16_t _destroy(prf_data_t* p_env, uint8_t reason);

/**
 * @brief Handles Connection creation for profile
 * 
 * @param p_env         Collector or Service allocated environment data.
 * @param conidx        Connection index
 * @param p_con_param   Pointer to connection parameters information
 */
static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param);

/**
 * @brief Handles Disconnection for profile
 * 
 * @param p_env         Collector or Service allocated environment data.
 * @param conidx        Connection index
 * @param reason        Detach reason
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

/************************private variable defination************************/
/*
 * TODO(mfk):  Per p63 of bluetooth book:
 *     - value of config descriptors are unique per connection.  So, if we allow
 *       more than one phone to bond with AMS proxy, this must be done.
 *     - value of config descriptors are preserved across connections with
 *       bonded devices.  Should use nvram to hold config values (notifications/
 *       indications on/off).
 */
static const uint8_t ams_uuid_128[GATT_UUID_128_LEN] = PROXY_SERVICE_UUID;

/*
 * TODO(mfk): According to the bluetooth book, the "value" field in the first
 *            row of the table below should be the UUID of the service.  Maybe
 *            BES implementation of attm_svc_create_db_128 does this for us?
 */
/*******************************************************************************
 *******************************************************************************
 * AMS Service - 55F80AEF-D89F-41A4-9E36-0FFC88DC81CE
 *******************************************************************************
 *******************************************************************************/
static const struct gatt_att_desc ams_att_db[] = {

    /**************************************************
     *  AMS Service Declaration
     **************************************************/
    [AMS_PROXY_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_PRIMARY_SERVICE),
        PROP(RD),
        0,
    },

    /**************************************************
     * AMS Remote Command
     *    - UUID: 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2
     *    - Writeable, notifiable (Direct proxy of Apple's AMS characteristic
     *and matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_REMOTE_CMD_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },
    // Characteristic Value
    [AMS_PROXY_REMOTE_CMD_VAL] = {
        PROXY_CHAR_REMOTE_CMD,
        NTF_PERM | WRITE_REQ_PERM | WRITE_CMD_PERM | ATT_UUID(128),
        AMS_MAX_LEN,
    },
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_REMOTE_CMD_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        // 10,
        0,
    },

    /**************************************************
     * AMS Ready (Google specific)
     *    - UUID: 3ADF41AF-F7A1-4E16-863E-53A188D5BF8D
     *    - provides the current state of AMS (0 - not ready, 1 - ready)
     *    - read/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_READY_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },
    // Characteristic Value
    [AMS_PROXY_READY_VAL] = {
        PROXY_CHAR_READY,
        NTF_PERM | RD_PERM | ATT_UUID(128),
        AMS_MAX_LEN,
    },
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_READY_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        // 10,
        0,
    },

    /**************************************************
     * AMS Entity Update
     *    - UUID: 2F7CABCE-808D-411F-9A0C-BB92BA96C102
     *    - Writeable with response, notifiable (Direct proxy of Apple's AMS
     *characteristic and matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_ENT_UPD_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },
    // Characteristic Value
    [AMS_PROXY_ENT_UPD_VAL] = {
        PROXY_CHAR_ENT_UPDATE,
        NTF_PERM | WRITE_REQ_PERM | ATT_UUID(128),
        AMS_MAX_LEN,
    },
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_ENT_UPD_CFG] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        // 10,
        0,
    },

    /**************************************************
     * AMS Entity Attribute
     *    - UUID: C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7
     *    - readable, writeable (Direct proxy of Apple's AMS characteristic and
     *matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_ENT_ATTR_CHAR] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        PROP(RD),
        0,
    },
    // Characteristic Value
    [AMS_PROXY_ENT_ATTR_VAL] = {
        PROXY_CHAR_ENT_ATTR,
        RD_PERM | WRITE_REQ_PERM | WRITE_CMD_PERM | ATT_UUID(128),
        AMS_MAX_LEN,
    },
};

static const struct prf_task_cbs amsp_itf = {
    .cb_init        = _init,
    .cb_destroy     = _destroy,
    .cb_con_create  = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd     = _con_upd,
};

/****************************function defination****************************/
static uint16_t _init(prf_data_t* p_env, uint16_t* p_start_hdl, uint8_t sec_lvl, uint8_t user_prio,
                          const void* p_params, const void* p_cb)
{
    LOG_D("%s start_hdl:%d.", __func__, *p_start_hdl);

    uint8_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(amsp) *ams_env = ke_malloc(sizeof(PRF_ENV_T(amsp)), KE_MEM_ATT_DB);

    do
    {
        /// registor GATT server user
        status = gatt_user_srv_register(PREFERRED_BLE_MTU,
                                        user_prio,
                                        amsp_task_get_srv_cbs(),
                                        &(ams_env->user_lid));

        /// check the GATT server user registation excution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        status = gatt_db_svc_add(ams_env->user_lid,
                                 sec_lvl,
                                 ams_uuid_128,
                                 AMS_PROXY_NUM,
                                 NULL,
                                 ams_att_db,
                                 AMS_PROXY_NUM,
                                 &ams_env->start_hdl);

        /// check the GATT database add execution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        /// initialize environment variable
        p_env->p_env = (prf_hdr_t*)ams_env;
        amsp_task_init(&(p_env->desc), (void *)ams_env);
        // ke_state_set(p_env->prf_task, 0);
    } while (0);

    if(btif_is_gatt_over_br_edr_enabled()) {
        btif_btgatt_addsdp(ATT_SERVICE_UUID, ams_env->start_hdl, ams_env->start_hdl+AMS_PROXY_NUM-1);
    }

    return status;
}

static uint16_t _destroy(prf_data_t* p_env, uint8_t reason)
{
    PRF_ENV_T(amsp) *amsp_env = (PRF_ENV_T(amsp) *)p_env->p_env;
    p_env->p_env = NULL;
    ke_free(amsp_env);

    return 0;
}

static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_D("%s", __func__);

    PRF_ENV_T(amsp) *amsp_env = (PRF_ENV_T(amsp) *)p_env->p_env;
    amsp_env->subEnv[conidx].conidx = conidx;
}

static void _con_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason)
{
    PRF_ENV_T(amsp) *amsp_env = (PRF_ENV_T(amsp) *)p_env->p_env;
    ProxySubEnv_t *pSubEnv = &(amsp_env->subEnv[conidx]);

    pSubEnv->ready = false;
    pSubEnv->ready_ntf_enable = false;
    memset(&pSubEnv->client_handles[0], 0, sizeof(pSubEnv->client_handles));
}

static void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s", __func__);
}

const struct prf_task_cbs *ams_proxy_prf_itf_get(void)
{
    LOG_I("%s entry.", __func__);
    return &amsp_itf;
}

#endif /* (ANCS_PROXY_ENABLE) */
