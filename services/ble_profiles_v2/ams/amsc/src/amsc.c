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
 * @addtogroup AMSC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_AMS_CLIENT)
#include "ams_common.h"
#include "amsc.h"
#include "amsp.h"
#include "amsc_task.h"
#include "ke_mem.h"
#include "ke_timer.h"
#include "prf_dbg.h"

#define PRF_TAG     "[AMSC]"

extern const gatt_cli_cb_t *amsc_task_get_cli_cbs(void);

/**
 ****************************************************************************************
 * @brief Initialization of the AMSC module.
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
static uint16_t _init(prf_data_t *p_env,
                      uint16_t *p_start_hdl,
                      uint8_t sec_lvl,
                      uint8_t user_prio,
                      const void *p_params,
                      const void *p_cb)
{
    LOG_D("%s", __func__);

    uint16_t status = GAP_ERR_NO_ERROR; 
    uint8_t idx;
    ///-------------------- allocate memory required for the profile  ---------------------
    PRF_ENV_T(amsc) *amsc_env = (PRF_ENV_T(amsc) *)ke_malloc(sizeof(PRF_ENV_T(amsc)), KE_MEM_ATT_DB);

    /// allocate AMSC required environment variable
    p_env->p_env = (prf_hdr_t *)amsc_env;

    /// register gatt client user
    status = gatt_user_cli_register(PREFERRED_BLE_MTU,
                                    0,
                                    amsc_task_get_cli_cbs(),
                                    &(amsc_env->user_lid));

    if (GAP_ERR_NO_ERROR == status)
    {
        // initialize environment variable
        amsc_task_init(&(p_env->desc), (void *)amsc_env);

        for (idx = 0; idx < AMSC_IDX_MAX; idx++)
        {
            amsc_env->env[idx] = NULL;
            // service is ready, go into an Idle state
            // ke_state_set(p_env->prf_task, AMSC_FREE);
        }
    }
    else
    {
        LOG_W("user client register failed");
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void _con_cleanup(prf_data_t *env, uint8_t conidx, uint16_t reason)
{
    LOG_D("%s", __func__);

    PRF_ENV_T(amsc) *amsc_env = (PRF_ENV_T(amsc) *)env->p_env;

    // clean-up environment variable allocated for task instance
    if (amsc_env->env[conidx] != NULL)
    {
        ke_timer_clear(AMSC_TIMEOUT_TIMER_IND, KE_BUILD_ID(env->prf_task, conidx));
        ke_free(amsc_env->env[conidx]);
        amsc_env->env[conidx] = NULL;
    }

    /* Put AMS Client in Free state */
    // ke_state_set(KE_BUILD_ID(env->prf_task, conidx), AMSC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the AMSC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static uint16_t _destroy(prf_data_t *env, uint8_t reason)
{
    LOG_D("%s", __func__);

    uint16_t status = GAP_ERR_NO_ERROR;
    uint8_t idx;
    PRF_ENV_T(amsc) *amsc_env = (PRF_ENV_T(amsc) *)env->p_env;

    // cleanup environment variable for each task instances
    for (idx = 0; idx < AMSC_IDX_MAX; idx++)
    {
        _con_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->p_env = NULL;
    ke_free(amsc_env);

    return status;
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
    LOG_D("%s", __func__);

    /* Put AMS Client in Idle state */
    // ke_state_set(KE_BUILD_ID(p_env->prf_task, conidx), AMSC_IDLE);
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
void _con_upd(prf_data_t* p_env, uint8_t conidx, const gap_con_param_t* p_con_param)
{
    LOG_D("%s", __func__);
}

/// AMSC Task interface required by profile manager
const struct prf_task_cbs amsc_itf = {
    .cb_init = _init,
    .cb_destroy = _destroy,
    .cb_con_create = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd = _con_upd,
};

/**
 ****************************************************************************************
 * @brief Retrieve AMS client profile interface
 *
 * @return AMS client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs *amsc_prf_itf_get(void)
{
    return &amsc_itf;
}

void amsc_enable_rsp_send(PRF_ENV_T(amsc) *amsc_env, uint8_t conidx, uint8_t status)
{
    // ASSERT(status == GAP_ERR_NO_ERROR, "%s error %d", __func__, status);

    if (status == GAP_ERR_NO_ERROR)
    {
        /// Register AMSC task in gatt for indication/notifications
        gatt_cli_event_register(conidx, amsc_env->user_lid, amsc_env->shdl, amsc_env->shdl + AMS_PROXY_NUM);

        /// Go to connected state
        // ke_state_set(KE_BUILD_ID(PRF_SRC_TASK(AMSC), conidx), AMSC_IDLE);
    }
    else
    {
        LOG_W("%s status:%d", __func__, status);
    }
}

#endif //(BLE_AMS_CLIENT)

/// @} AMSC
