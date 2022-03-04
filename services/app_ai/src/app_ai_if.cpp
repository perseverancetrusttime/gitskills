#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_audio.h"
#include "app_through_put.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "dip_api.h"
#include "app_dip.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#include "app_ai_if_config.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_if_custom_ui.h"
#include "app_bt.h"

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif

#ifdef __AI_VOICE__
#ifdef __IAG_BLE_INCLUDE__
#include "app_ai_ble.h"
#endif
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_bt.h"
#endif

#ifdef IBRT
#include "app_ibrt_if.h"
#endif

#define CASE_S(s) case s:return "["#s"]";
#define CASE_D()  default:return "[INVALID]";

AI_CAPTURE_BUFFER_T app_ai_if_capture_buf;

const char *ai_spec_type2str(AI_SPEC_TYPE_E ai_spec)
{
    switch(ai_spec) {
        CASE_S(AI_SPEC_INIT)
        CASE_S(AI_SPEC_GSOUND)
        CASE_S(AI_SPEC_AMA)
        CASE_S(AI_SPEC_BES)
        CASE_S(AI_SPEC_BAIDU)
        CASE_S(AI_SPEC_TENCENT)
        CASE_S(AI_SPEC_ALI)
        CASE_S(AI_SPEC_COMMON)
        CASE_D()
    }
}

bool ai_if_is_ai_stream_mic_open(void)
{
    bool mic_open = false;

#ifdef __AI_VOICE__
    mic_open = app_ai_voice_is_mic_open();
#endif

    return mic_open;
}

uint8_t app_ai_if_get_ble_connection_index(void)
{
    uint8_t ble_connection_index = 0xFF;

#if defined(__AI_VOICE__) && defined(__IAG_BLE_INCLUDE__)
    ble_connection_index = app_ai_ble_get_connection_index(ai_manager_get_foreground_ai_conidx());
#endif

    return ble_connection_index;
}

static void app_ai_if_set_ai_spec(uint32_t *ai_spec, uint32_t set_spec)
{
    if(*ai_spec == AI_SPEC_INIT)
    {
        *ai_spec = set_spec;
    }
    else
    {
        *ai_spec = (*ai_spec << 8) | set_spec;
        TRACE(2, "%s, AI SPEC = %04x", __func__, *ai_spec);
    }
}

uint32_t app_ai_if_get_ai_spec(void)
{
    uint32_t ai_spec = AI_SPEC_INIT;

    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_INIT);//Avoid not using this function in some cases, resulting in compilation failure

#ifdef __AMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_AMA);
#endif
#ifdef __DMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_BAIDU);
#endif
#ifdef __SMART_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_BES);
#endif
#ifdef __TENCENT_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_TENCENT);
#endif
#ifdef __GMA_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_ALI);
#endif
#ifdef __CUSTOMIZE_VOICE__
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_COMMON);
#endif
#ifdef DUAL_MIC_RECORDING
    app_ai_if_set_ai_spec(&ai_spec, AI_SPEC_RECORDING);
#endif
    //TRACE(2, "%s, AI SPEC = %04x", __func__, ai_spec);
    return ai_spec;
}

static POSSIBLY_UNUSED void app_ai_if_mempool_init(void)
{
    uint8_t *buf = NULL;
    app_capture_audio_mempool_get_buff(&buf, APP_CAPTURE_AUDIO_BUFFER_SIZE);

    memset((uint8_t *)buf, 0, APP_CAPTURE_AUDIO_BUFFER_SIZE);

    app_ai_if_capture_buf.buff = buf;
    app_ai_if_capture_buf.buff_size_total = APP_CAPTURE_AUDIO_BUFFER_SIZE;
    app_ai_if_capture_buf.buff_size_used = 0;
    app_ai_if_capture_buf.buff_size_free = APP_CAPTURE_AUDIO_BUFFER_SIZE;

    TRACE(3, "%s buf %p size %d", __func__, buf, APP_CAPTURE_AUDIO_BUFFER_SIZE);
}

void app_ai_if_mempool_deinit(void)
{
    TRACE(2, "%s size %d", __func__, app_ai_if_capture_buf.buff_size_total);

    memset((uint8_t *)app_ai_if_capture_buf.buff, 0, app_ai_if_capture_buf.buff_size_total);

    app_ai_if_capture_buf.buff_size_used = 0;
    app_ai_if_capture_buf.buff_size_free = app_ai_if_capture_buf.buff_size_total;
}

void app_ai_if_mempool_get_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    buff_size_free = app_ai_if_capture_buf.buff_size_free;

    if (size % 4){
        size = size + (4 - size % 4);
    }

    TRACE(3,"%s free %d to allocate %d", __func__, buff_size_free, size);

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    *buff = app_ai_if_capture_buf.buff + app_ai_if_capture_buf.buff_size_used;

    app_ai_if_capture_buf.buff_size_used += size;
    app_ai_if_capture_buf.buff_size_free -= size;

    TRACE(3, "AI allocate %d, now used %d left %d", size,
          app_ai_if_capture_buf.buff_size_used,
          app_ai_if_capture_buf.buff_size_free);
}

void app_ai_if_mobile_connect_handle(void *addr)
{
    POSSIBLY_UNUSED bt_bdaddr_t * _addr = (bt_bdaddr_t *)addr;

#ifdef BISTO_ENABLED
    gsound_custom_bt_link_connected_handler(_addr->address);
#endif

    if(app_dip_function_enable() == true){
        POSSIBLY_UNUSED MOBILE_CONN_TYPE_E type = MOBILE_CONNECT_IDLE;
        bt_dip_pnp_info_t pnp_info;
        pnp_info.vend_id = 0;

        bool isExsit = nv_record_get_pnp_info(_addr, &pnp_info);
        TRACE(3, "%s vend_id 0x%x vend_id_source 0x%x", __func__, 
            pnp_info.vend_id, pnp_info.vend_id_source);
        if (isExsit)
        {
            type = btif_dip_check_is_ios_by_vend_id(pnp_info.vend_id, pnp_info.vend_id_source)?MOBILE_CONNECT_IOS:MOBILE_CONNECT_ANDROID;
#ifdef __AI_VOICE__
            app_ai_mobile_connect_handle(type, _addr->address);
#endif
#ifdef BISTO_ENABLED
            gsound_mobile_type_get_callback(type);
#endif
        }
    }

#ifdef __IAG_BLE_INCLUDE__
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}

void app_ai_if_hfp_connected_handler(uint8_t device_id)
{
#ifdef __AI_VOICE__
    uint8_t ai_index = 0;

    for (uint8_t ai_connect_index = 0; ai_connect_index < AI_CONNECT_NUM_MAX; ai_connect_index++)
    {
        ai_index = app_ai_get_ai_index_from_connect_index(ai_connect_index);

        if(ai_index)
        {
            AI_connect_info *ai_info = app_ai_get_connect_info(ai_connect_index);
            if(ai_info) {
                uint8_t dest_id = app_ai_get_dest_id(ai_info);
                ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0, ai_index, dest_id);
            }
        }
    }
#endif
}

void app_ai_voice_stay_active(uint8_t aiIndex)
{
#ifdef __AI_VOICE__
#ifdef IBRT
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        if (app_tws_ibrt_tws_link_connected() && \
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) && \
            p_ibrt_ctrl->p_tws_remote_dev)
        {
            btif_me_stop_sniff(p_ibrt_ctrl->p_tws_remote_dev);
        }
    
        uint8_t* mobileAddr;
        for (uint8_t device_id = 0;device_id < BT_DEVICE_NUM;device_id++)
        {
            if (ai_struct[aiIndex].ai_mobile_info[device_id].used)
            {
                mobileAddr = ai_struct[aiIndex].ai_mobile_info[device_id].addr;
                app_ibrt_if_prevent_sniff_set(mobileAddr, AI_VOICE_RECORD);
            }
        }
#else
        app_bt_active_mode_set(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif
#endif
}

void app_ai_voice_resume_sleep(uint8_t aiIndex)
{
#ifdef __AI_VOICE__
#ifdef IBRT
    uint8_t* mobileAddr;
    for (uint8_t device_id = 0;device_id < BT_DEVICE_NUM;device_id++)
    {
        if (ai_struct[aiIndex].ai_mobile_info[device_id].used)
        {
            mobileAddr = ai_struct[aiIndex].ai_mobile_info[device_id].addr;
            app_ibrt_if_prevent_sniff_clear(mobileAddr, AI_VOICE_RECORD);
        }
    }
#else
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_AI_VOICE_STREAM, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif
#endif
}

void app_ai_init(void)
{
    TRACE(1,"%s",__func__);
#ifdef __AI_VOICE__
    app_ai_manager_init();

#ifdef BISTO_ENABLED
    bool gsound_enable = true;
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (app_ai_manager_is_in_multi_ai_mode())
    {
        if ((app_ai_manager_get_current_spec() != AI_SPEC_GSOUND))
        {
            TRACE(1,"%s set gsound false", __func__);
            gsound_enable = false;
        }
    }
#endif    
    gsound_custom_init(gsound_enable);
#endif
#ifdef __AMA_VOICE__
    ai_open_specific_ai(AI_SPEC_AMA);
#endif
#ifdef __DMA_VOICE__
    ai_open_specific_ai(AI_SPEC_BAIDU);
#endif
#ifdef __SMART_VOICE__
    ai_open_specific_ai(AI_SPEC_BES);
#endif
#ifdef __TENCENT_VOICE__
    ai_open_specific_ai(AI_SPEC_TENCENT);
#endif
#ifdef __GMA_VOICE__
    ai_open_specific_ai(AI_SPEC_ALI);
#endif
#ifdef __CUSTOMIZE_VOICE__
    ai_open_specific_ai(AI_SPEC_COMMON);
#endif
#ifdef DUAL_MIC_RECORDING
    ai_open_specific_ai(AI_SPEC_RECORDING);
#endif

    app_ai_tws_init();
    app_ai_if_custom_init();

#ifdef __THROUGH_PUT__
    app_throughput_test_init();
#endif

    app_ai_voice_thread_init();

    app_capture_audio_mempool_init();

#ifdef __THIRDPARTY
    app_ai_if_thirdparty_init();
#endif

    app_ai_if_mempool_init();
#endif
}

uint8_t app_ai_get_codec_type(uint8_t spec)
{
    /// init as invalid value
    uint8_t codec = 0;

    if (0)
    {
    }
#ifdef __AMA_VOICE__
    else if (AI_SPEC_AMA == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __DMA_VOICE__
    else if (AI_SPEC_BAIDU == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __SMART_VOICE__
    else if (AI_SPEC_BES == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __TENCENT_VOICE__
    else if (AI_SPEC_TENCENT == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __GMA_VOICE__
    else if (AI_SPEC_ALI == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef __CUSTOMIZE_VOICE__
    else if (AI_SPEC_COMMON == spec)
    {
        codec = VOC_ENCODE_OPUS;
    }
#endif
#ifdef DUAL_MIC_RECORDING
    else if (AI_SPEC_RECORDING == spec)
    {
#ifdef RECORDING_USE_OPUS
    codec = VOC_ENCODE_OPUS;
#else
    codec = VOC_ENCODE_SCALABLE;
#endif
    }
#endif

    return codec;
}

#ifdef IS_MULTI_AI_ENABLED
static AI_OPEN_MIC_USER_E g_ai_open_mic_user = AI_OPEN_MIC_USER_NONE;

void app_ai_open_mic_user_set(AI_OPEN_MIC_USER_E user)
{
    g_ai_open_mic_user = user;
}

AI_OPEN_MIC_USER_E app_ai_open_mic_user_get()
{
    return g_ai_open_mic_user;
}
#endif
