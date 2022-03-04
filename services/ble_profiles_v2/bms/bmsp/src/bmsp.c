/**
 * @file bmsp.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-10-19
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
#include "bmsp.h"
#include "bmsp_task.h"
#include "ke_mem.h"
#include "prf_dbg.h"

/*********************external function declearation************************/

/************************private macro defination***************************/
#define PRF_TAG         "[BMSP]"
#define BMS_MAX_LEN     244

#define RD_PERM             PROP(RD) | SEC_LVL(RP, AUTH)
#define WRITE_REQ_PERM      PROP(WR) | SEC_LVL(WP, AUTH)
#define NTF_PERM            PROP(N)  | SEC_LVL(NIP, AUTH)

#define BMS_SERVER_SERVICE          {0x3D, 0x21, 0x19, 0x8C, 0x64, 0x8A, 0xD6, 0xB6, 0xBD, 0x42, 0xEF, 0xAA, 0x9C, 0x0D, 0xC1, 0x91}
#define BMS_SERVER_ACTIVE_APP       {0xF1, 0xA1, 0xD1, 0x4D, 0xE6, 0x6B, 0xB4, 0x8F, 0xAA, 0x46, 0x17, 0x45, 0x4E, 0x06, 0xD1, 0x99}
#define BMS_SERVER_MEDIA_COMMAND    {0x85, 0x6E, 0x1F, 0x73, 0xAC, 0x0B, 0x77, 0xB5, 0xB6, 0x44, 0x1A, 0x3F, 0x6C, 0x7F, 0xE8, 0xFB}
#define BMS_SERVER_MEDIA_STATUS     {0x36, 0x11, 0x03, 0x14, 0x16, 0x37, 0x57, 0xB9, 0xD1, 0x4B, 0xF5, 0xBF, 0xC0, 0x91, 0x07, 0x42}
#define BMS_SERVER_BROADCAST        {0x68, 0xB7, 0xAC, 0x66, 0x30, 0x64, 0xE7, 0xA3, 0x87, 0x42, 0xF9, 0x30, 0x46, 0x5A, 0xEF, 0xE4}

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
static uint16_t _init(prf_data_t *p_env, uint16_t *p_start_hdl, uint8_t sec_lvl, uint8_t user_prio,
                      const void *p_params, const void *p_cb);

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
static const uint8_t bms_uuid_128[GATT_UUID_128_LEN] = BMS_SERVER_SERVICE;

/*******************************************************************************
 *******************************************************************************
 * BMS Service
 *******************************************************************************
 *******************************************************************************/
const struct gatt_att_desc bms_att_db[] = {

    /**************************************************
     *  BMS Service Declaration
     **************************************************/

    [BISTO_IDX_BMS_SVC - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_PRIMARY_SERVICE),
        RD_PERM,
        0,
    },
    /**************************************************
     * BMS "Active App"
     *    - UUID: 99D1064E-4517-46AA-8FB4-6BE64DD1A1F1
     *    - read/write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_ACTIVE_APP_CHAR - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },

    // Characteristic Value
    [BISTO_IDX_BMS_ACTIVE_APP_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_ACTIVE_APP,
        RD_PERM | WRITE_REQ_PERM | NTF_PERM | ATT_UUID(128),
        BMS_MAX_LEN,
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        0,
    },

    /**************************************************
     * BMS "Media Command"
     *    - UUID: FBE87F6C-3F1A-44B6-B577-0BAC731F6E85
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_MEDIA_CMD_CHAR - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },
    // Characteristic Value
    [BISTO_IDX_BMS_MEDIA_CMD_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_MEDIA_COMMAND,
        WRITE_REQ_PERM | NTF_PERM | ATT_UUID(128),
        BMS_MAX_LEN,
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        0,
    },

    /**************************************************
     * BMS "Media Status"
     *    - UUID: 420791C0-BFF5-4BD1-B957-371614031136
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_MEDIA_STATUS_CHAR - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM | WRITE_REQ_PERM,
        0,
    },

    // Characteristic Value
    [BISTO_IDX_BMS_MEDIA_STATUS_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_MEDIA_STATUS,
        WRITE_REQ_PERM | NTF_PERM| ATT_UUID(128),
        BMS_MAX_LEN,
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        0,
    },

    /**************************************************
     * BMS "Broadcast"
     *    - UUID: E4EF5A46-30F9-4287-A3E7-643066ACB768
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_BROADCAST_CHAR - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DECL_CHARACTERISTIC),
        RD_PERM,
        0,
    },

    // Characteristic Value
    [BISTO_IDX_BMS_BROADCAST_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_BROADCAST,
        WRITE_REQ_PERM | NTF_PERM| ATT_UUID(128),
        BMS_MAX_LEN,
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_BROADCAST_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        UUID_16_TO_ARRAY(GATT_DESC_CLIENT_CHAR_CFG),
        RD_PERM | WRITE_REQ_PERM,
        0,
    },
};

const struct prf_task_cbs bms_itf = {
    .cb_init        = _init,
    .cb_destroy     = _destroy,
    .cb_con_create  = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd     = _con_upd,
};

/****************************function defination****************************/
static uint16_t _init(prf_data_t *p_env,
                      uint16_t *p_start_hdl,
                      uint8_t sec_lvl,
                      uint8_t user_prio,
                      const void *params,
                      const void *p_cb)
{
    uint16_t status = GAP_ERR_NO_ERROR;
    LOG_I("%s start_hdl:%d", __func__, *p_start_hdl);

    /// allocate&init profile environment data
    PRF_ENV_T(bmsp) *bms_env = ke_malloc(sizeof(PRF_ENV_T(bmsp)), KE_MEM_ATT_DB);
    memset((uint8_t *)bms_env, 0, sizeof(PRF_ENV_T(bmsp)));

    do
    {
        /// registor GATT server user
        status = gatt_user_srv_register(PREFERRED_BLE_MTU,
                                        user_prio,
                                        (gatt_srv_cb_t*)bmsp_task_get_srv_cbs(),
                                        &(bms_env->user_lid));

        /// check the GATT server user registation excution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
    }

        status = gatt_db_svc_add(bms_env->user_lid,
                                 sec_lvl,
                                 bms_uuid_128,
                                 BISTO_IDX_BMS_NUM,
                                 NULL,
                                 bms_att_db,
                                 BISTO_IDX_BMS_NUM,
                                 &bms_env->shdl);

        /// check the GATT database add execution result
        if (GAP_ERR_NO_ERROR != status)
        {
            break;
        }

        /// initialize environment variable
        p_env->p_env = (prf_hdr_t *)bms_env;
        bmsp_task_init(&(p_env->desc), (void *)bms_env);
        // ke_state_set(p_env->prf_task, 0);
    } while (0);

    return status;
}

static uint16_t _destroy(prf_data_t *p_env, uint8_t reason)
{
    LOG_I("%s", __func__);
    struct bms_env_tag *bms_env = (struct bms_env_tag *)p_env->p_env;

    p_env->p_env = NULL;
    ke_free(bms_env);

    return 0;
}

static void _con_create(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s conidx:%d", __func__, conidx);
}

static void _con_cleanup(prf_data_t *p_env, uint8_t conidx, uint16_t reason)
{
    LOG_I("%s conidx:%d, reason:%d", __func__, conidx, reason);
}

static void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_I("%s conidx:%d", __func__, conidx);
}

/**
 * @brief Get BMS interface
 * 
 * @return const struct prf_task_cbs*   Pointer of BMS interface
 */
const struct prf_task_cbs *bms_prf_itf_get(void)
{
    return &bms_itf;
}

#endif /* (BLE_VOICEPATH) */
