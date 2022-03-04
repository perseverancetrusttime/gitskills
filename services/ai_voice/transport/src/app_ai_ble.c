/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "string.h"
#include "co_bt.h"
#include "gatt.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "app_ai_ble.h"
#include "app_ai_if_ble.h"
#include "app_ai_tws.h"
#include "ai_transport.h"
#include "app_ble_mode_switch.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct app_ai_env_tag app_ai_env[2];

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_ai_connected_evt_handler(uint8_t conidx, uint8_t connect_index)
{
    TRACE(1,"%s", __func__);
    if (app_ai_ble_is_connected(connect_index) && (conidx != app_ai_env[connect_index].connectionIndex))
    {
        TRACE(0, "ai ble has connected");
        return;
    }

    app_ai_env[connect_index].connectionIndex = conidx;
}

void app_ai_disconnected_evt_handler(uint8_t conidx)
{
    for(uint8_t connect_index = 0; connect_index < AI_CONNECT_NUM_MAX; connect_index++)
    {
        TRACE(0, "%s index %d conidx 0x%x 0x%x", __func__, connect_index, conidx, app_ai_env[connect_index].connectionIndex);
        if (conidx == app_ai_env[connect_index].connectionIndex)
        {
            app_ai_env[connect_index].connectionIndex = BLE_INVALID_CONNECTION_INDEX;
            app_ai_env[connect_index].isCmdNotificationEnabled = false;
            app_ai_env[connect_index].isDataNotificationEnabled = false;
            app_ai_env[connect_index].mtu[conidx] = 0;

            AI_connect_info *ai_info = app_ai_get_connect_info(connect_index);
            if(NULL == ai_info) {
                return;
            }
            uint8_t dest_id = app_ai_get_dest_id(ai_info);
            app_ai_update_connect_state(AI_IS_DISCONNECTING, connect_index);
            TRACE(3,"%s %d %d", __func__, app_ai_get_transport_type(ai_info->ai_spec, connect_index), dest_id);
            if (app_ai_get_transport_type(ai_info->ai_spec, connect_index) == AI_TRANSPORT_BLE)
            {
                TRACE(1,"%s", __func__);
                ai_function_handle(CALLBACK_AI_DISCONNECT, NULL, AI_TRANSPORT_BLE, ai_info->ai_spec, dest_id);
            }
        }
    }
}

bool app_ai_ble_is_connected(uint8_t connect_index)
{
    return (BLE_INVALID_CONNECTION_INDEX != app_ai_env[connect_index].connectionIndex);
}

uint8_t app_ai_ble_get_connection_index(uint8_t connect_index)
{
    return app_ai_env[connect_index].connectionIndex;
}

void app_ai_disconnect_ble(uint8_t connect_index)
{
    if (app_ai_ble_is_connected(connect_index))
    {
        app_ai_if_ble_disconnect_ble(app_ai_env[connect_index].connectionIndex);
    }
}

uint16_t app_ai_ble_get_conhdl(uint8_t connect_index)
{
    return app_ai_if_ble_get_conhdl_from_conidx(app_ai_env[connect_index].connectionIndex);
}

bool app_ai_ble_notification_processing_is_busy(uint8_t ai_index)
{
    return app_ai_if_ble_check_if_notification_processing_is_busy(app_ai_env[ai_index].connectionIndex);
}

void app_ai_ble_update_conn_param_mode(bool isEnabled, uint8_t ai_index, uint8_t connect_index)
{
    TRACE(2,"%s isEnabled %d", __func__, isEnabled);

    if (AI_TRANSPORT_BLE == app_ai_get_transport_type(ai_index, connect_index))
    {
        app_ai_if_ble_update_conn_param_mode(isEnabled);
    }
}

void app_ai_ble_add_ai(void)
{
#ifdef __AI_VOICE_BLE_ENABLE__
    TRACE(0, "%s %d", __func__, TASK_ID_AI);

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    // Fill message
#ifdef BLE_V2
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = SVC_SEC_LVL(NOT_ENC); //!< todo: Add UUID type flag
    req->user_prio = 0;
    req->start_hdl = 0;
    req->prf_api_id = TASK_ID_AI;
#else //BLE_V2
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, UNAUTH) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, UNAUTH);
#endif
    req->prf_task_id = TASK_ID_AI;

    req->app_task = TASK_APP;
    req->start_hdl = 0;
#endif // BLE_V2

    // Send the message
    ke_msg_send(req);

#endif // __AI_VOICE_BLE_ENABLE__
}

static bool app_ai_is_ble_adv_uuid_enabled(void)
{
    bool adv_enable = false;
#if defined(SLAVE_ADV_BLE)
    adv_enable = true;
#endif

    if (app_ai_is_in_tws_mode(0) && !app_ai_tws_init_done())
    {
        TRACE(1,"%s ibrt don't init", __func__);
    }
    else if (app_ai_is_in_tws_mode(0) && app_ai_tws_get_local_role() == APP_AI_TWS_SLAVE)
    {
        TRACE(2,"%s role %d isn't MASTER or UNKNOW", __func__, app_ai_tws_get_local_role());
    }
    else
    {
        //uint8_t ai_spec = app_ai_if_get_ai_spec();
        //if (app_is_ai_voice_connected(ai_spec))
        //{
        //    TRACE(1,"%s ai has connected", __func__);
        //}
        //else if (app_ai_is_role_switching(ai_spec))
        //{
        //    TRACE(1,"%s ai is role switching", __func__);
        //}
        //else
        //{
            adv_enable = true;
        //}
    }

    return adv_enable;
}

/****************************function defination****************************/
static void app_ai_ble_data_fill_handler(void *param)
{
    bool adv_enable = false;

    adv_enable = app_ai_is_ble_adv_uuid_enabled();
    if (adv_enable && param)
    {
        uint32_t ai_spec = app_ai_if_get_ai_spec();
#ifdef __MUTLI_POINT_AI__
        if(ai_spec & 0xFF)
        {
            adv_enable = ai_function_handle(API_START_ADV, param, 0, (uint8_t)(ai_spec & 0xFF), 0);
        }
        if(ai_spec >> 8)
        {
            adv_enable = ai_function_handle(API_START_ADV, param, 0, (uint8_t)(ai_spec >> 8), 0)?true:adv_enable;
        }
#else
        adv_enable = ai_function_handle(API_START_ADV, param, 0, (uint8_t)ai_spec, 0);
#endif
    }

    TRACE(2, "%s adv_enable %d", __func__, adv_enable);
    app_ai_if_ble_data_fill_enable(adv_enable);
}

void app_ai_mtu_exchanged_handler(uint8_t conidx, uint16_t mtu)
{
    for(uint8_t connect_index=0; connect_index < AI_CONNECT_NUM_MAX; connect_index++)
    {
        TRACE(3,"%s conidx %d index %d", __func__, conidx, app_ai_env[connect_index].connectionIndex);

        if (conidx == app_ai_env[connect_index].connectionIndex) {
            AI_connect_info *ai_info = app_ai_get_connect_info(connect_index);
            uint8_t dest_id = app_ai_get_dest_id(ai_info);
            ai_function_handle(CALLBACK_UPDATE_MTU, NULL, mtu, ai_info->ai_spec, dest_id);
        }
        else {
            app_ai_env[connect_index].mtu[conidx] = mtu;
        }
    }
}

AI_BLE_HANDLER_INSTANCE_T *ai_ble_handler_get_entry_pointer_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
         index < ((uint32_t)__ai_ble_handler_table_end - (uint32_t)__ai_ble_handler_table_start) / sizeof(AI_BLE_HANDLER_INSTANCE_T);
         index++)
    {
        if (AI_BLE_HANDLER_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code)
        {
            TRACE(3, "%s ai_code %d index %d", __func__, ai_code, index);
            return AI_BLE_HANDLER_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    //ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    TRACE(0, "%s fail ai_code %d", __func__, ai_code);
    return NULL;
}

uint32_t ai_ble_handler_get_entry_index_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
         index < ((uint32_t)__ai_ble_handler_table_end - (uint32_t)__ai_ble_handler_table_start) / sizeof(AI_BLE_HANDLER_INSTANCE_T);
         index++)
    {
        if (AI_BLE_HANDLER_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code)
        {
            TRACE(3, "%s ai_code %d index %d", __func__, ai_code, index);
            return index;
        }
    }

    //ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    TRACE(0, "%s fail ai_code %d", __func__, ai_code);
    return INVALID_AI_BLE_HANDLER_ENTRY_INDEX;
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
//const ai_ble_ke_state_handler_t *app_ai_table_handler[MUTLI_AI_NUM];
const ai_ble_ke_state_handler_t *app_ai_table_handler;

void ai_ble_handler_init(uint32_t ai_spec)
{
#if 0
    AI_BLE_HANDLER_INSTANCE_T *pInstance = NULL;

    pInstance = ai_ble_handler_get_entry_pointer_from_ai_code(ai_spec);
    if (pInstance)
    {
        app_ai_table_handler[ai_spec] = pInstance->ai_table_handler;
    }
    else
    {
        app_ai_table_handler[ai_spec] = NULL;
    }
#endif
}

void app_ai_ble_init(uint32_t ai_spec)
{
    // Reset the environment
    for(uint8_t ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        app_ai_env[ai_connect_index].connectionIndex =  APP_AI_IF_BLE_INVALID_CONNECTION_INDEX;
        app_ai_env[ai_connect_index].isCmdNotificationEnabled = false;
        app_ai_env[ai_connect_index].isDataNotificationEnabled = false;
        memset((uint8_t *)&(app_ai_env[ai_connect_index].mtu), 0, sizeof(app_ai_env[ai_connect_index].mtu));
    }

    app_ai_if_ble_register_data_fill_handle((void *)app_ai_ble_data_fill_handler, false);
}

/// @} APP

