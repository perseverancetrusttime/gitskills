#include <string.h>
#include "cmsis_os.h"
#include "hal_bootmode.h"
#include "hal_trace.h"
#include "app_bt.h"
#include "spp_api.h"
#include "app_spp.h"
#include "app_bt_func.h"
#include "me_api.h"
#include "app_ai_if_gsound.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_thread.h"
#include "app_ai_tws.h"
#include "app_rfcomm_mgr.h"
#include "app_ai_ble.h"
#include "app_ai_voice.h"
#include "app_thirdparty.h"
#include "app_ai_if_thirdparty.h"
#include "app_ibrt_if.h"
#ifdef BISTO_ENABLED
#include "gsound_custom_actions.h"
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif

#ifdef __AMA_VOICE__
#include "ama_stream.h"
#endif

extern "C" int app_reset(void);

AI_MANAGER_T aiManager;
volatile uint8_t current_adv_type_temp_selected = AI_SPEC_GSOUND;
#define GVA_AMA_CROSS_ADV_TIME_INTERVAL (1500)

uint8_t ForegroundAI = AI_CONNECT_INVALID;

static void ai_manager_cross_adv(void)
{
    if (current_adv_type_temp_selected == AI_SPEC_GSOUND)
    {
        current_adv_type_temp_selected = AI_SPEC_AMA;
    }
    else if (current_adv_type_temp_selected == AI_SPEC_AMA)
    {
        current_adv_type_temp_selected = AI_SPEC_GSOUND;
    }

    app_ai_if_ble_set_adv(GVA_AMA_CROSS_ADV_TIME_INTERVAL);
}
static void ble_gva_ama_cross_adv_timer_handler(void const *param)
{
    TRACE(0,"cross adv timer callback handler");

    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              (uint32_t)ai_manager_cross_adv);
}

osTimerId gva_ama_cross_adv_timer               = NULL;
static void ble_gva_ama_cross_adv_timer_handler(void const *param);
osTimerDef(BLE_GVA_AMA_CROSS_ADV_TIMER, ( void (*)(void const *) )ble_gva_ama_cross_adv_timer_handler);


static void ai_manager_reset(void)
{
    if (app_ai_is_in_tws_mode(0))
    {
        app_ai_tws_reboot_record_box_state();
    }
    app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
    app_reset();
}
static void ai_manager_reset_timer_handler(void const *param)
{
    TRACE(1,"%s", __func__);

    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              (uint32_t)ai_manager_reset);
}

osTimerId ai_manager_reset_timer_id = NULL;
#define AI_MANAGER_RESET_TIME_IN_MS (500)
osTimerDef(AI_MANAGER_RESET_TIMER, ( void (*)(void const *) )ai_manager_reset_timer_handler);

// ai manager disconnect all bt connection
static void ai_manager_disconnect_all_bt_connection(void)
{
    /// disconnect all BLE connections before reboot
    app_ai_if_ble_disconnect_all();

    app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
    if (app_ai_is_in_tws_mode(0))
    {
        app_ai_tws_disconnect_all_bt_connection();
    }
    else
    {
        app_ai_disconnect_all_mobile_link();
    }
}

static void ai_manager_disconnect_timer_handler(void const *param)
{
    TRACE(1,"%s", __func__);

    app_ai_if_ble_force_switch_adv(false);

    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              (uint32_t)ai_manager_disconnect_all_bt_connection);

    osTimerStart(ai_manager_reset_timer_id, 
                    AI_MANAGER_RESET_TIME_IN_MS);
}

osTimerId ai_manager_disconnect_timer_id = NULL;
#define AI_MANAGER_DISCONNECT_TIME_IN_MS (5000)    //2000->5000 bisto need more time to exchange information.
osTimerDef(AI_MANAGER_DISCONNECT_TIMER, ( void (*)(void const *) )ai_manager_disconnect_timer_handler);

bool ai_voicekey_is_enable(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    TRACE(2,"%s voicekey_en %d", __func__, nvrecord_env->aiManagerInfo.voice_key_enable);
    return (nvrecord_env->aiManagerInfo.voice_key_enable);
}

void ai_voicekey_save_status(bool state)
{
    TRACE(2,"%s state: %d", __func__, state);
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);
    if (nvrecord_env->aiManagerInfo.voice_key_enable != state)
    {
        nvrecord_env->aiManagerInfo.voice_key_enable = state;
        aiManager.info.voice_key_enable = state;

        nv_record_env_set(nvrecord_env);
    }
}

static void ai_manager_save_info_section(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    if (nvrecord_env->aiManagerInfo.currentAiSpec != aiManager.info.currentAiSpec || \
        nvrecord_env->aiManagerInfo.aiStatusDisableFlag != aiManager.disableFlag || \
        nvrecord_env->aiManagerInfo.amaAssistantEnableStatus != aiManager.rebootAmaEnable || \
        nvrecord_env->aiManagerInfo.voice_key_enable != aiManager.info.voice_key_enable)
    {
        nvrecord_env->aiManagerInfo.currentAiSpec            = aiManager.info.currentAiSpec;
        nvrecord_env->aiManagerInfo.aiStatusDisableFlag      = aiManager.disableFlag;
        nvrecord_env->aiManagerInfo.amaAssistantEnableStatus = aiManager.rebootAmaEnable;
        nvrecord_env->aiManagerInfo.voice_key_enable         = aiManager.info.voice_key_enable;
        nv_record_env_set(nvrecord_env);
    }
}

void ai_manager_init(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    TRACE(5,"%s spec: %d saved: %d ai_disable:%d ama_assistant:%d",
          __func__,
          nvrecord_env->aiManagerInfo.currentAiSpec,
          nvrecord_env->aiManagerInfo.setedCurrentAi,
          nvrecord_env->aiManagerInfo.aiStatusDisableFlag,
          nvrecord_env->aiManagerInfo.amaAssistantEnableStatus);

    memset(&aiManager, 0, sizeof(aiManager));
    if (!nvrecord_env->aiManagerInfo.setedCurrentAi)
    {
        aiManager.info.currentAiSpec = DEFAULT_AI_SPEC;
        nvrecord_env->aiManagerInfo.setedCurrentAi = 1;
        nv_record_env_set(nvrecord_env);
    }
    else
    {
        aiManager.info.currentAiSpec = nvrecord_env->aiManagerInfo.currentAiSpec;
    }

    if (nvrecord_env->aiManagerInfo.aiStatusDisableFlag)
    {
        aiManager.disableFlag = true;
    }
    else
    {
        aiManager.disableFlag = false;
    }

    aiManager.need_reboot = false;
    aiManager.reboot_ongoing = false;
    aiManager.rebootAmaEnable = nvrecord_env->aiManagerInfo.amaAssistantEnableStatus;
    aiManager.info.voice_key_enable = nvrecord_env->aiManagerInfo.voice_key_enable;

    if (gva_ama_cross_adv_timer == NULL)
    {
        gva_ama_cross_adv_timer = osTimerCreate(osTimer(BLE_GVA_AMA_CROSS_ADV_TIMER), osTimerOnce, NULL);
    }

    if (ai_manager_reset_timer_id == NULL)
    {
        ai_manager_reset_timer_id = osTimerCreate(osTimer(AI_MANAGER_RESET_TIMER), osTimerOnce, NULL);
    }

    if (ai_manager_disconnect_timer_id == NULL)
    {
        ai_manager_disconnect_timer_id = osTimerCreate(osTimer(AI_MANAGER_DISCONNECT_TIMER), osTimerOnce, NULL);
    }
}

void ai_manager_switch_spec(AI_SPEC_TYPE_E ai_spec)
{
    if (ai_spec == aiManager.info.currentAiSpec)
    {
        return;
    }
    
    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    uint8_t dest_id = app_ai_get_dest_id(ai_info);
    uint8_t spec_temp = aiManager.info.currentAiSpec;

    if (AI_SPEC_AMA == spec_temp)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_AMA))
        {
            ai_function_handle(CALLBACK_AI_ENABLE, NULL, false, AI_SPEC_AMA, dest_id);
        }
    }
    else if (AI_SPEC_GSOUND == spec_temp)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_GSOUND))
        {
            app_ai_if_gsound_service_enable_switch(false);
        }
    }

    aiManager.info.currentAiSpec = ai_spec;
    aiManager.disableFlag = false;
    if (AI_SPEC_AMA == ai_spec)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_AMA))
        {
            ai_function_handle(CALLBACK_AI_ENABLE, NULL, true, AI_SPEC_AMA, dest_id);
        }
    }
    else if (AI_SPEC_GSOUND == ai_spec)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_GSOUND))
        {
            app_ai_if_gsound_service_enable_switch(true);
        }
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if(AI_SPEC_INIT != spec_temp)
    {
        if(spec_temp != ai_spec)
        {
            //need_reboot = true;
            app_ai_manager_spec_update_start_reboot();
        }
    }
#endif    
}

void ai_manager_set_current_spec(AI_SPEC_TYPE_E ai_spec)
{
#ifndef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    // TODO: implement me
    return;
#endif

    TRACE(3,"%s %d%s", __func__, ai_spec, ai_spec_type2str(ai_spec));

    if (aiManager.info.currentAiSpec == ai_spec)
    {
        TRACE(0,"ai_spec has set");
        return;
    }

    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    uint8_t dest_id = app_ai_get_dest_id(ai_info);

    if (AI_SPEC_AMA == ai_spec)
    {
        if (AI_SPEC_GSOUND == aiManager.info.currentAiSpec)
        {
            aiManager.need_reboot = true;
            if (ai_manager_get_spec_connected_status(AI_SPEC_GSOUND))
            {
                app_ai_if_gsound_service_enable_switch(false);
            }
            else
            {
                aiManager.reboot_ongoing = true;
                ai_manager_set_spec_update_flag(true);
            }
        }
    }
    else if (AI_SPEC_GSOUND == ai_spec)
    {
        if (AI_SPEC_AMA == aiManager.info.currentAiSpec)
        {
            aiManager.need_reboot = true;
            if (ai_manager_get_spec_connected_status(AI_SPEC_AMA))
            {
                ai_function_handle(CALLBACK_AI_ENABLE, NULL, false, AI_SPEC_AMA, dest_id);
            }
            else
            {
                aiManager.reboot_ongoing = true;
                ai_manager_set_spec_update_flag(true);
            }
        }
    }
#if defined(__TENCENT_VOICE__)
    else if(AI_SPEC_TENCENT == ai_spec)
    {

    }
    else if(AI_SPEC_INIT == ai_spec)
    {
        if(AI_SPEC_TENCENT == aiManager.info.currentAiSpec)
        {
            ai_function_handle(CALLBACK_DISCONNECT_APP, NULL, 0, AI_SPEC_TENCENT, dest_id);
            app_ble_force_switch_adv(BLE_SWITCH_USER_AI, TRUE);
        }
    }
#endif

    aiManager.info.currentAiSpec  = ai_spec;
    aiManager.disableFlag         = false;
    ai_manager_save_info_section();

    if (!app_ai_is_in_tws_mode(0))
    {
        if (aiManager.reboot_ongoing)
        {
            ai_function_handle(API_AI_RESET, NULL, 10, AI_SPEC_AMA, dest_id);
            osTimerStart(ai_manager_disconnect_timer_id, 
                            AI_MANAGER_DISCONNECT_TIME_IN_MS);
        }
    }
}

uint8_t ai_manager_get_current_spec(void)
{
    TRACE(3,"%s %d%s", __func__, \
                    aiManager.info.currentAiSpec, \
                    ai_spec_type2str((AI_SPEC_TYPE_E)aiManager.info.currentAiSpec));
    return aiManager.info.currentAiSpec;
}

bool ai_manager_is_need_reboot(void)
{
    TRACE(2, "%s need_reboot %d", __func__, aiManager.need_reboot);
    return aiManager.need_reboot;
}

void ai_manager_set_spec_connected_status(AI_SPEC_TYPE_E ai_spec, uint8_t connected)
{
    if (ai_spec < 0 || ai_spec >= AI_SPEC_COUNT)
    {
        TRACE(1,"%s invaild parameter", __func__);
        return;
    }
    TRACE(4,"%s %d%s connected %d", __func__, ai_spec, ai_spec_type2str(ai_spec), connected);
    aiManager.status[ai_spec].connectedStatus = connected;
}

int8_t ai_manager_get_spec_connected_status(uint8_t ai_spec)
{
    if (ai_spec < 0 || ai_spec >= AI_SPEC_COUNT)
    {
        TRACE(0,"invaild parameter");
        return -1;
    }
    return aiManager.status[ai_spec].connectedStatus;
}

void ai_manager_set_spec_assistant_status(uint8_t ai_spec, uint8_t value)
{
    if (ai_spec < 0 || ai_spec >= AI_SPEC_COUNT)
    {
        TRACE(0,"invaild parameter");
        return;
    }
    aiManager.status[ai_spec].assistantStatus = value;
}

int8_t ai_manager_get_spec_assistant_status(uint8_t ai_spec)
{
    if (ai_spec < 0 || ai_spec >= AI_SPEC_COUNT)
    {
        TRACE(0,"invaild parameter");
        return -1;
    }
    return aiManager.status[ai_spec].assistantStatus;
}

bool ai_manager_spec_get_status_is_in_invalid(void)
{
    return aiManager.disableFlag;
}

void ai_manager_set_ama_assistant_enable_status(uint8_t ama_assistant_value)
{
    TRACE(1,"ai_manager_set_ama_assistant_status:%d", ama_assistant_value);
    aiManager.rebootAmaEnable = ama_assistant_value;
}

void ai_manager_set_spec_update_flag(bool onOff)
{
    TRACE(2,"%s %d", __func__, onOff);
    aiManager.specUpdateStart = onOff;
}

bool ai_manager_get_spec_update_flag(void)
{
    //TRACE(2,"%s %d", __func__, aiManager.specUpdateStart);
    return aiManager.specUpdateStart;
}

uint8_t ai_manager_get_ama_assistant_enable_status(void)
{
    return aiManager.rebootAmaEnable;
}

void ai_manager_enable(bool isEnabled, AI_SPEC_TYPE_E ai_spec)
{
    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    uint8_t dest_id = app_ai_get_dest_id(ai_info);
    if (AI_SPEC_AMA == ai_spec)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_AMA))
            ai_function_handle(CALLBACK_AI_ENABLE, NULL, isEnabled, AI_SPEC_AMA, dest_id);
    }
    else if (AI_SPEC_GSOUND == ai_spec)
    {
        if (1 == ai_manager_get_spec_connected_status(AI_SPEC_GSOUND))
        {
            app_ai_if_gsound_service_enable_switch(isEnabled);
        }
    }

    if (isEnabled)
    {
        aiManager.info.currentAiSpec = ai_spec;
        aiManager.disableFlag = false;
    }
    else
    {
        aiManager.info.currentAiSpec = AI_SPEC_INIT;
        aiManager.disableFlag = true;
    }
}

void ai_manager_spec_update_start_reboot()
{
    AI_SPEC_TYPE_E ai_spec = (AI_SPEC_TYPE_E)aiManager.info.currentAiSpec;
    TRACE(3,"ai_manager_reboot saveSpec:%d%s, ai_invalid_flag:%d",
                                      ai_spec,
                                      ai_spec_type2str(ai_spec),
                                      aiManager.disableFlag);

    if (!aiManager.reboot_ongoing)
    {
        aiManager.reboot_ongoing = true;
        ai_manager_save_info_section();

        if (app_ai_is_in_tws_mode(0))
        {
            if (app_ai_tws_link_connected() && app_ai_tws_get_local_role() == APP_AI_TWS_MASTER)
            {
                app_ai_tws_sync_ai_manager_info();
            }
        }
        else
        {
            AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
            uint8_t dest_id = app_ai_get_dest_id(ai_info);
            ai_function_handle(API_AI_RESET, NULL, 10, AI_SPEC_AMA, dest_id);
            osTimerStart(ai_manager_disconnect_timer_id, 
                            AI_MANAGER_DISCONNECT_TIME_IN_MS);
        }
    }
}

void ai_manager_set_foreground_ai_conidx(uint8_t ai_connect_index)
{
    AI_connect_info *ai_info =  app_ai_get_connect_info(ai_connect_index);
    if(NULL == ai_info) {
        return;
    }
    if (AI_IS_CONNECTED != ai_info->ai_connect_state)
    {
        TRACE(2, "%s, can not set foreground ai, index%d has not ai connected", __func__, ai_connect_index);
        return;
    }

    if(ai_connect_index < AI_CONNECT_NUM_MAX)
    {
        TRACE(2, "Foreground AI update:%d->%d", ForegroundAI, ai_connect_index);
        ForegroundAI = ai_connect_index;
    }
    else
    {
        TRACE(2, "%s, error connection index %d", __func__, ai_connect_index);
        ForegroundAI = AI_CONNECT_INVALID;
    }
}

void ai_manager_update_foreground_ai_conidx(uint8_t ai_connect_index, bool AddorRemove)
{
    //add to ai manager
    TRACE(3,"%s ai_idx:%d, isAdd:%d", __func__, ai_connect_index, AddorRemove);
    if(AddorRemove) {
        ai_manager_set_foreground_ai_conidx(ai_connect_index);
    }else {
    //remove from ai manager
        if(ai_connect_index < AI_CONNECT_NUM_MAX && (ForegroundAI == ai_connect_index))
        {
            for(uint8_t i = 0; i < AI_CONNECT_NUM_MAX && (i != ForegroundAI); i++){
                AI_connect_info *ai_info =  app_ai_get_connect_info(i);
                if (AI_IS_CONNECTED == ai_info->ai_connect_state) {
                    ForegroundAI = i;
                    TRACE(2, "Remove current AI, Foreground AI update:%d->%d", ai_connect_index, ForegroundAI);
                    return;
                }
            }
            ForegroundAI = AI_CONNECT_INVALID;
            TRACE(2, "Remove current AI, Foreground AI update:%d->%d", ai_connect_index, ForegroundAI);
        }
    }
}


uint8_t ai_manager_get_foreground_ai_conidx(void)
{
    //TRACE(1, "ForegroundAI:%d", ForegroundAI);
    return ForegroundAI;
}

uint32_t ai_manager_save_ctx(uint8_t *buf, uint32_t buf_len)
{

    memcpy(buf, (void *)&aiManager, sizeof(aiManager));
    buf_len = sizeof(aiManager);
    TRACE(2,"%s len %d", __func__, buf_len);

    return buf_len;
}



uint32_t ai_manager_restore_ctx(uint8_t *buf, uint32_t buf_len)
{
    TRACE(1,"%s", __func__);

    if (buf_len == sizeof(aiManager))
    {
        memcpy((void *)&aiManager, buf, buf_len);
        ai_manager_save_info_section();

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
        if (aiManager.reboot_ongoing)
        {
            TRACE(1,"%s reboot", __func__);
            osTimerStart(ai_manager_reset_timer_id, 
                            (AI_MANAGER_RESET_TIME_IN_MS+AI_MANAGER_DISCONNECT_TIME_IN_MS));
        }
#endif        
    }
    else
    {
        TRACE(2,"%s len error %d", __func__, buf_len);
    }

    return buf_len;
}

uint32_t ai_manager_save_ctx_rsp_handle(uint8_t *buf, uint32_t buf_len)
{
    TRACE(2,"%s reboot %d", __func__, aiManager.reboot_ongoing);
    if (aiManager.reboot_ongoing)
    {
        AI_connect_info *ai_info = app_ai_get_connect_info(app_ai_get_connect_index_from_ai_spec(AI_SPEC_AMA));
        if(ai_info != NULL){
            if(ai_info ->ai_connect_type == AI_TRANSPORT_BLE)
                ai_function_handle(API_AI_RESET, NULL, 0, AI_SPEC_AMA, SET_BLE_FLAG(ai_info->conidx));
            else
                ai_function_handle(API_AI_RESET, NULL, 0, AI_SPEC_AMA, ai_info->device_id);
        }    
        osTimerStart(ai_manager_disconnect_timer_id,
                     AI_MANAGER_DISCONNECT_TIME_IN_MS);
    }

    return buf_len;
}

void ai_manager_update_gsound_spp_info(uint8_t* _addr, uint8_t device_id, uint8_t connect_state)
{
    uint8_t ai_connect_index = AI_CONNECT_INVALID;
    if (RFCOMM_CONNECTED == connect_state)
    {
        ai_connect_index = app_ai_set_spp_connect_info(_addr, device_id, AI_TRANSPORT_SPP, AI_SPEC_GSOUND);
    }
    else if (RFCOMM_DISCONNECTED == connect_state)
    {
        ai_connect_index = app_ai_get_connect_index_from_device_id(device_id, AI_SPEC_GSOUND);
        app_ai_clear_connect_info(ai_connect_index);
    }
    //ai_manager_update_foreground_ai_conidx(ai_connect_index,isAdd);
}

#ifdef __IAG_BLE_INCLUDE__
void ai_manager_update_gsound_ble_info(uint8_t con_index, uint8_t connect_state)
{
    uint8_t ai_connect_index = AI_CONNECT_INVALID;
    bool isAdd = false;
    if (BLE_CONNECTED == connect_state)
    {
        ai_connect_index = app_ai_set_ble_connect_info(AI_TRANSPORT_BLE, AI_SPEC_GSOUND, con_index);
        isAdd = true;
    }
    else if (BLE_DISCONNECTED == connect_state)
    {
        ai_connect_index = app_ai_get_connect_index_from_ble_conidx(con_index, AI_SPEC_GSOUND);
        app_ai_clear_connect_info(ai_connect_index);
        isAdd = false;
    }
    ai_manager_update_foreground_ai_conidx(ai_connect_index,isAdd);
}
#endif


void ai_manager_key_event_handle(APP_KEY_STATUS *status, void *param)
{
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        return;
    }
    
    if (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER)
    {
        TRACE(2, "%s isn't master %d", __func__, app_ai_tws_get_local_role());
        app_ibrt_if_keyboard_notify_v2(NULL, status, param);
        return;
    }

    TRACE(2,"%s event %d", __func__, status->event);
    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(NULL == ai_info) {
        return;
    }
    uint8_t ai_index = ai_info->ai_spec;
    uint8_t dest_id = app_ai_get_dest_id(ai_info);
    TRACE(1, "%s ai_index = %d", __func__, ai_index);

    if (app_ai_dont_have_mobile_connect(ai_index))
    {
        TRACE(1, "%s no mobile connected", __func__);
        return;
    }

    if (AI_IS_CONNECTED != ai_info->ai_connect_state)
    {
        TRACE(1, "%s ai no connected", __func__);
        return;
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (app_ai_manager_get_spec_update_flag())
    {
        TRACE(0,"device reboot is ongoing...");
        return;
    }
#endif

    if (AI_SPEC_GSOUND == ai_info->ai_spec)
    {
        TRACE(1, "%s foreground ai is Gsound", __func__);
        #ifdef BISTO_ENABLED
        gsound_custom_actions_handle_key(status, param);
        #endif
        return;
    }

#ifdef PUSH_AND_HOLD_ENABLED
    app_ai_set_wake_up_type(AIV_WAKEUP_TRIGGER_PRESS_AND_HOLD, ai_index);
#else
    app_ai_set_wake_up_type(AIV_WAKEUP_TRIGGER_TAP, ai_index);
#endif

    if (0 == ai_function_handle(CALLBACK_KEY_EVENT_HANDLE, status, (uint32_t)param, ai_index, dest_id))
    {
        if (app_ai_is_use_thirdparty(ai_index))
        {
            app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS,THIRDPARTY_AI_PROVIDE_SPEECH, ai_index);
        }
    }
#endif
}

AI_DEVICE_ACTION_TYPE_T ai_manager_switch_to_device_action(AI_SPEC_TYPE_E spec,uint32_t event)
{
    AI_DEVICE_ACTION_TYPE_T device_action = AI_DEVICE_ACTION_UNKNOWN;
#ifdef __AMA_VOICE__
    if(spec == AI_SPEC_AMA) {
        switch (event)
        {
            case STATE_ANC_LEVEL:
                device_action = AI_DEVICE_ACTION_ANC_LEVEL;
                break;
            case STATE_ANC_ENABLE:
                device_action = AI_DEVICE_ACTION_ANC_ENABLE;
                break;
            case STATE_PASSTHROUGH:
                device_action = AI_DEVICE_ACTION_PASS_THROUGH;
                break;
            case STATE_FB_ANC_LEVEL:
                device_action = AI_DEVICE_ACTION_FB_LEVEL;
                break;
            case STATE_EQUALIZER_BASS:
                device_action = AI_DEVICE_ACTION_EQUALIZER_BASS;
                break;
            case STATE_EQUALIZER_MID:
                device_action = AI_DEVICE_ACTION_EQUALIZER_MID;
                break;
            case STATE_EQUALIZER_TREBLE:
                device_action = AI_DEVICE_ACTION_EQUALIZER_TREBLE;
                break;
            default:
                break;
        }
    }
#endif

    TRACE(2,"%s device_action=%d",__func__, device_action);
    return device_action;
}

void ai_manager_get_battery_info(AI_BATTERY_INFO_T *battery)
{
    battery->status = AI_BATTERY_DISCHARGING;
    battery->level = 100;
    battery->scale = 100;
    TRACE(1,"%s",__func__);
}
bool ai_manager_get_device_status(AI_SPEC_TYPE_E spec, AI_DEVICE_ACTION_PARAM_T *param)
{
    bool ret = true;
    AI_DEVICE_ACTION_TYPE_T device_action = ai_manager_switch_to_device_action(spec,param->event);

    switch(device_action)
    {
        case AI_DEVICE_ACTION_ANC_ENABLE:
            {
                param->value_type = AI_VALUE_BOOL;
                param->p.booler = false;
                TRACE(1,"%s anc enable",__func__);
            }
        break;
        case AI_DEVICE_ACTION_ANC_LEVEL:
            {
                param->value_type = AI_VALUE_INT;
                param->p.inter = 0;
                TRACE(2,"%s anc level %d",__func__, param->p.inter);
            }
        break;
        case AI_DEVICE_ACTION_FB_LEVEL:
            {
                param->value_type = AI_VALUE_INT;
                param->p.inter = 0;
                TRACE(2,"%s fb anc level %d",__func__, param->p.inter);
            }
        break;
        case AI_DEVICE_ACTION_PASS_THROUGH:
            {
                param->value_type = AI_VALUE_INT;
                param->p.inter = 0;
                TRACE(2,"%s pass_through enable %d",__func__, param->p.inter);
            }
        break;
        case AI_DEVICE_ACTION_BATTERY_LEVEL:
            {
                param->value_type = AI_VALUE_INT;
                param->p.inter = 0;
                TRACE(2,"%s battery_level %d",__func__, param->p.inter);
            }
        break;
        case AI_DEVICE_ACTION_UNKNOWN:
        default:
            {
                ret = false;
            }
        break;
    }
    return ret;
}

bool ai_manager_set_device_status(AI_SPEC_TYPE_E spec, AI_DEVICE_ACTION_PARAM_T *param)
{
    bool ret = true;
    AI_DEVICE_ACTION_TYPE_T device_action = ai_manager_switch_to_device_action(spec,param->event);

    switch(device_action)
    {
        case AI_DEVICE_ACTION_ANC_ENABLE:
            if(param->p.booler) {
                TRACE(1,"%s anc enable",__func__);
            }else {
                TRACE(1,"%s anc disable",__func__);
            }
        break;
        case AI_DEVICE_ACTION_ANC_LEVEL:
            TRACE(2,"%s anc level %d",__func__, param->p.inter);
        break;
        case AI_DEVICE_ACTION_FB_LEVEL:
            TRACE(2,"%s fb anc level %d",__func__, param->p.inter);
        break;
        case AI_DEVICE_ACTION_PASS_THROUGH:
            if(param->p.booler) {
                TRACE(1,"%s pass_through enable",__func__);
            }else {
                TRACE(1,"%s pass_through disable",__func__);
            }
        break;
        case AI_DEVICE_ACTION_BATTERY_LEVEL:
            TRACE(2,"%s battery_level %d",__func__, param->p.inter);
        break;
        case AI_DEVICE_ACTION_POWER:
        break;
        case AI_DEVICE_ACTION_EQUALIZER_BASS:
        case AI_DEVICE_ACTION_EQUALIZER_MID:
        case AI_DEVICE_ACTION_EQUALIZER_TREBLE:
        case AI_DEVICE_ACTION_UNKNOWN:
        default:
            {
                ret = false;
            }
        break;
    }
    return ret;
}
