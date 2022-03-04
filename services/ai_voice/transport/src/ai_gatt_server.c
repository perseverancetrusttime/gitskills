/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#ifdef __AI_VOICE_BLE_ENABLE__ 
#include "app_ai_if_ble.h"
#include "app_ai_if.h"
#include "app_ai_ble.h"
#include "ai_gatt_server.h"

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

AI_GATT_SERVER_INSTANCE_T* ai_gatt_server_get_entry_pointer_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__ai_gatt_server_table_end-(uint32_t)__ai_gatt_server_table_start)/sizeof(AI_GATT_SERVER_INSTANCE_T);index++) {
        if (AI_GATT_SERVER_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code) {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return AI_GATT_SERVER_PTR_FROM_ENTRY_INDEX(index);
        }           
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return NULL;
}

uint32_t ai_gatt_server_get_entry_index_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
        index < ((uint32_t)__ai_gatt_server_table_end-(uint32_t)__ai_gatt_server_table_start)/sizeof(AI_GATT_SERVER_INSTANCE_T);index++) {
        if (AI_GATT_SERVER_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code) {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return index;
        }           
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return INVALID_AI_GATT_SERVER_ENTRY_INDEX;
}

const struct prf_task_cbs* ai_prf_itf_get(void)
{
    AI_GATT_SERVER_INSTANCE_T *pInstance = NULL;
    uint8_t ai_spec = app_ai_if_get_ai_spec();

    pInstance = ai_gatt_server_get_entry_pointer_from_ai_code(ai_spec);

    TRACE(2, "%s ai_spec = %d", __func__, ai_spec);

    if(pInstance)
    {
        return pInstance->ai_itf;
    }
    else
    {
        return NULL;
    }
}
#endif


