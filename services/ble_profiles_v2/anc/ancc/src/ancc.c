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
 * @addtogroup ANCC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_ANC_CLIENT)
#include "ancc.h"
#include "ancs.h"
#include "ancc_task.h"
#include "ke_timer.h"
#include "ke_mem.h"
#include "prf_dbg.h"

#define PRF_TAG "[ANCC]"


static void _con_cleanup(prf_data_t* p_env, uint8_t conidx, uint16_t reason);

/**
 ****************************************************************************************
 * @brief Initialization of the Profile module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    p_env        Collector or Service allocated environment data.
 * @param[in|out] p_start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     sec_lvl      Security level (@see enum enum gatt_svc_info_bf)
 * @param[in]     user_prio    GATT User priority
 * @param[in]     p_param      Configuration parameters of profile collector or service (32 bits aligned)
 * @param[in]     p_cb         Callback structure that handles event from profile
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint16_t _init(prf_data_t *p_env, uint16_t *p_start_hdl, uint8_t sec_lvl, uint8_t user_prio,
                      const void *p_params, const void *p_cb)
{
    LOG_D("%s", __func__);

    uint16_t status = GAP_ERR_NO_ERROR; 
    uint8_t idx;
    ///-------------------- allocate memory required for the profile  ---------------------
    PRF_ENV_T(ancc)* ancc_env =
            (PRF_ENV_T(ancc)* ) ke_malloc(sizeof(PRF_ENV_T(ancc)), KE_MEM_ATT_DB);

    /// allocate ANCC required environment variable
    p_env->p_env = (prf_hdr_t*) ancc_env;

    /// register gatt client user
    status = gatt_user_cli_register(PREFERRED_BLE_MTU,
                                    0,
                                    ancc_task_get_cli_cbs(),
                                    &(ancc_env->user_lid));

    if (GAP_ERR_NO_ERROR == status)
    {
        // initialize environment variable
        ancc_task_init(&(p_env->desc), (void *)ancc_env);

        for (idx = 0; idx < ANCC_IDX_MAX; idx++)
        {
            ancc_env->env[idx] = NULL;
            /// service is ready, go into an Idle state
            // ke_state_set(KE_BUILD_ID(p_env->prf_task, idx), ANCC_FREE);
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
 * @brief Destruction of the profile module - due to a reset or profile remove.
 *
 * This function clean-up allocated memory.
 *
 * @param[in|out]    p_env        Collector or Service allocated environment data.
 * @param[in]        reason       Destroy reason (@see enum prf_destroy_reason)
 *
 * @return status of the destruction, if fails, profile considered not removed.
 ****************************************************************************************
 */
static uint16_t _destroy(prf_data_t* p_env, uint8_t reason)
{
    LOG_D("%s", __func__);

    uint8_t idx;
    uint16_t status = GAP_ERR_NO_ERROR;
    PRF_ENV_T(ancc)* ancc_env = (PRF_ENV_T(ancc)*)p_env->p_env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < ANCC_IDX_MAX ; idx++)
    {
        _con_cleanup(p_env, idx, 0);
    }

    // free profile environment variables
    p_env->p_env = NULL;
    ke_free(ancc_env);

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

    /* Put ANC Client in Idle state */
    // ke_state_set(KE_BUILD_ID(p_env->prf_task, conidx), ANCC_IDLE);
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
static void _con_cleanup(prf_data_t* p_env, uint8_t conidx, uint16_t reason)
{
    LOG_D("%s", __func__);

    PRF_ENV_T(ancc)* ancc_env = (PRF_ENV_T(ancc)*)p_env->p_env;

    BLE_FUNC_ENTER();

    // clean-up environment variable allocated for task instance
    if(ancc_env->env[conidx] != NULL)
    {
        ke_timer_clear(ANCC_TIMEOUT_TIMER_IND, KE_BUILD_ID(p_env->prf_task, conidx));
        ke_free(ancc_env->env[conidx]);
        ancc_env->env[conidx] = NULL;
    }

    /* Put ANC Client in Free state */
    // ke_state_set(KE_BUILD_ID(p_env->prf_task, conidx), ANCC_FREE);
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
    LOG_D("%s", __func__);
}

/// ANCC Task interface required by profile manager
const struct prf_task_cbs ancc_itf = {
    .cb_init = _init,
    .cb_destroy = _destroy,
    .cb_con_create = _con_create,
    .cb_con_cleanup = _con_cleanup,
    .cb_con_upd = _con_upd,
};

const struct prf_task_cbs* ancc_prf_itf_get(void)
{
    return &ancc_itf;
}

void ancc_enable_rsp_send(PRF_ENV_T(ancc) *p_env, uint8_t conidx, uint8_t status)
{
    // ASSERT(status == GAP_ERR_NO_ERROR, "%s error %d", __func__, status);

    if (status == GAP_ERR_NO_ERROR)
    {
        // Register ANCC task in gatt for indication/notifications
        gatt_cli_event_register(conidx, p_env->user_lid, p_env->shdl, p_env->shdl + ANCS_PROXY_NUM);

        // Go to connected state
        // ke_state_set(KE_BUILD_ID(PRF_SRC_TASK(ANCC), conidx), ANCC_IDLE);
    }
}

#endif //(BLE_ANC_CLIENT)

/// @} ANCC
