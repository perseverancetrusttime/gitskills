#if 0
#include "hal_trace.h"
#include "audioflinger.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "string.h"
#include "cmsis_os.h"
#include "cqueue.h"
extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "btalloc.h"
}
#include "btapp.h"
#include "app_bt.h"
#include "app_bt_media_manager.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#include "app_tencent_voicepath.h"
#include "hal_timer.h"

static uint8_t app_tencent_voicepath_pending_streaming = 0;
static uint8_t app_tencent_voicepath_stream_state = 0;

static uint8_t app_tencent_voicepath_is_resampler_enabled = false;
static enum AUD_SAMPRATE_T audio_output_sample_rate;
static uint32_t audio_output_data_buf_size;

void app_tencent_voicepath_set_audio_output_sample_rate(enum AUD_SAMPRATE_T sampleRate)
{
    audio_output_sample_rate = sampleRate;
}

enum AUD_SAMPRATE_T app_tencent_voicepath_audio_output_sample_rate(void)
{
    return audio_output_sample_rate;    
}

void app_tencent_voicepath_set_audio_output_data_buf_size(uint32_t bufferSize)
{
    audio_output_data_buf_size = bufferSize;
}

uint32_t app_tencent_voicepath_audio_output_data_buf_size(void)
{
    return audio_output_data_buf_size;
}

// mic data captured->capture_stream_mic_data_disctributor->app_voicepath_mic_data_process
uint32_t app_tencent_voicepath_mic_data_process(uint8_t* ptrBuf, uint32_t length)
{
    TRACE(0,"app_tencent_voicepath_mic_data_process");
#if (MIX_MIC_DURING_MUSIC)     
    // if resampler is not needed, do the encoding and xfer once
    // the voice data is captured
    if (!app_tencent_voice_is_resampler_enabled())
    {
        app_tencent_voicepath_mic_data_come(ptrBuf, length);
        app_tencent_voicepath_mic_data_encoding();
    }
    // if a2dp streaming is active, the captured mic data will go through
	// capture_stream_mic_data_disctributor->bt_sbc_codec_capture_data->
	// app_voicepath_resmple->app_voicepath_mic_data_come(with resampled data),
	// so we don't nothing here
	
#else
    // for non-mix-mic feature, there is no re-sample callback on the 
    // function app_voicepath_mic_data_come, so we process the pcm data here
    app_tencent_voicepath_mic_data_come(ptrBuf, length);

    TRACE(0,"audio data is:");
	DUMP8("%02x",ptrBuf, length);
    app_tencent_voicepath_mic_data_encoding();
#endif
    return length;
}

static struct APP_RESAMPLE_T* app_voice_resampler = NULL;

#if (MIX_MIC_DURING_MUSIC)
uint32_t bt_sbc_codec_capture_data(uint8_t *buf, uint32_t len)
{
    if (app_tencent_voice_is_resampler_enabled())
    {
        app_tencent_voicepath_resmple(buf, len);        
    }
    
    return len;
}

#endif

const CAPTURE_STREAM_ENV_T captureStreamEnv = 
{
    CAPTURE_STREAM_MIC_DATA_HANDLER_CNT,
    {        
        {
        #if (MIX_MIC_DURING_MUSIC)    
            NULL,
            NULL,
            bt_sbc_codec_capture_data
        #else
            NULL,
            NULL,
            NULL
        #endif
        },
        
        {
            app_tencent_voicepath_init_voice_encoder,
            app_tencent_voicepath_deinit_voice_encoder,
            app_tencent_voicepath_mic_data_process
        },
    }
};

void capture_stream_init_disctributor(void)
{
    for (uint32_t index = 0;index < CAPTURE_STREAM_MIC_DATA_HANDLER_CNT;index++)
    {
        if (captureStreamEnv.micDataHandler[index].initializer)
        {
            captureStreamEnv.micDataHandler[index].initializer();
        }
    }
}

void capture_stream_deinit_disctributor(void)
{
    for (uint32_t index = 0;index < CAPTURE_STREAM_MIC_DATA_HANDLER_CNT;index++)
    {
        if (captureStreamEnv.micDataHandler[index].deinitializer)
        {
            captureStreamEnv.micDataHandler[index].deinitializer();
        }
    }
}

uint32_t capture_stream_mic_data_disctributor(uint8_t* ptrBuf, uint32_t length)
{
#if (MIX_MIC_DURING_MUSIC)    
    uint8_t currentAttenuationGain = app_tencent_voicepath_get_hw_sidetone_gain();
    if (HW_SIDE_TONE_ATTENUATION_COEF_IN_DISABLE_MODE != currentAttenuationGain)
    {
        if (currentAttenuationGain > HW_SIDE_TONE_USED_ATTENUATION_COEF)
        {
            if (app_tencent_voicepath_is_to_fade_in_hw_sidetone())
            {
                if (currentAttenuationGain > 
                    (HW_SIDE_TONE_FADE_IN_GAIN_STEP + HW_SIDE_TONE_USED_ATTENUATION_COEF))
                {
                    currentAttenuationGain -= HW_SIDE_TONE_FADE_IN_GAIN_STEP;
                }
                else
                {
                    currentAttenuationGain = HW_SIDE_TONE_USED_ATTENUATION_COEF;
                }
                
                app_tencent_voicepath_set_hw_sidetone_gain(currentAttenuationGain);
            }
        }
    }
#endif

    // if the voice stream is already on, we won't start the a2dp stream right-now,
	// otherwise there will be confliction between the mic&speaker audio callback
	// triggering. So we put set the pending flag when opening a2dp stream and 
	// trigger the a2dp stream at the time of catpure stream callback function here.
    if (app_tencent_voicepath_get_stream_pending_state(AUDIO_OUTPUT_STREAMING))
    {
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        app_tencent_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
        app_tencent_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
    }
        
    TRACE(1,"Get mic data %d", length);    //length is 2560;
    for (uint32_t index = 0;index < CAPTURE_STREAM_MIC_DATA_HANDLER_CNT;index++)
    {
        if (captureStreamEnv.micDataHandler[index].handler)
        {
            captureStreamEnv.micDataHandler[index].handler(ptrBuf, length);
        }
    }

    return length;
}

void app_tencent_voicepath_set_pending_started_stream(uint8_t pendingStream, bool isEnabled)
{
    if (isEnabled)
    {
        app_tencent_voicepath_pending_streaming |= (1 << pendingStream);
    }
    else
    {
        app_tencent_voicepath_pending_streaming &= (~(1 << pendingStream));
    }
}

bool app_tencent_voicepath_get_stream_pending_state(uint8_t pendingStream)
{
    return app_tencent_voicepath_pending_streaming & (1 << pendingStream);
}

bool app_tencent_voicepath_get_stream_state(uint8_t stream)
{
    return (app_tencent_voicepath_stream_state & (1 << stream));
}

void app_tencent_voicepath_set_stream_state(uint8_t stream, bool isEnabled)
{
    if (isEnabled)
    {
        app_tencent_voicepath_stream_state |= (1 << stream);
    }
    else
    {
        app_tencent_voicepath_stream_state &= (~(1 << stream));
    }
}

void app_tencent_voice_setup_resampler(bool isEnabled)
{
    app_tencent_voicepath_is_resampler_enabled = isEnabled;
}

bool app_tencent_voice_is_resampler_enabled(void)
{
    return app_tencent_voicepath_is_resampler_enabled;
}

#if (MIX_MIC_DURING_MUSIC)
static uint32_t lastHwSideToneGainUpdateTimeStampInMs = 0;
static uint8_t currentHwSideToneAttenuationGain = 0;
void app_tencent_voicepath_enable_hw_sidetone(uint8_t micChannel, uint8_t attenuationGain)
{
    TRACE(2,"Enable side-tone mic channel %d attenuationGain %d", micChannel, attenuationGain);

    currentHwSideToneAttenuationGain = attenuationGain;
    
    *(volatile uint32_t *)0x40300098 &= (~(1 << 19));
    
    *(volatile uint32_t *)0x403000b8 &= (~(7 << 15));
    *(volatile uint32_t *)0x403000b8 |= (micChannel << 15);
    
    *(volatile uint32_t *)0x40300088 &= (~(0x1F << 12));
    *(volatile uint32_t *)0x40300088 |= ((attenuationGain/2) << 12);

    lastHwSideToneGainUpdateTimeStampInMs = GET_CURRENT_MS();
}

bool app_tencent_voicepath_is_to_fade_in_hw_sidetone(void)
{
    uint32_t passedTimerMs;
	uint32_t currentTimerMs = GET_CURRENT_MS();
	
	if (0 == lastHwSideToneGainUpdateTimeStampInMs)
	{
		passedTimerMs = currentTimerMs;
	}
	else
	{
    	if (currentTimerMs > lastHwSideToneGainUpdateTimeStampInMs)
    	{
    		passedTimerMs = currentTimerMs - lastHwSideToneGainUpdateTimeStampInMs;
    	}
    	else
    	{
    		passedTimerMs = 0xFFFFFFFF - lastHwSideToneGainUpdateTimeStampInMs + currentTimerMs + 1;
    	}
	}

    if (passedTimerMs >= HW_SIDE_TONE_FADE_IN_TIME_STEP_IN_MS)
    {
		lastHwSideToneGainUpdateTimeStampInMs = currentTimerMs;
        return true;
    }
    else
    {
        return false;
    }
}

void app_tencent_voicepath_set_hw_sidetone_gain(uint8_t attenuationGain)
{
    TRACE(1,"Set HW side tone attenuationGain as %d", attenuationGain);
    currentHwSideToneAttenuationGain = attenuationGain;
    *(volatile uint32_t *)0x40300088 &= (~(0x1F << 12));
    *(volatile uint32_t *)0x40300088 |= ((attenuationGain/2) << 12);
}

uint8_t app_tencent_voicepath_get_hw_sidetone_gain(void)
{
    return currentHwSideToneAttenuationGain;
}

void app_tencent_voicepath_disable_hw_sidetone(void)
{
    *(volatile uint32_t *)0x40300098 |= (1 << 19);        
    *(volatile uint32_t *)0x403000b8 |= (7 << 15);
    *(volatile uint32_t *)0x40300088 |= (0x1F << 12);

    currentHwSideToneAttenuationGain = HW_SIDE_TONE_ATTENUATION_COEF_IN_DISABLE_MODE;
}
#endif

void app_tencent_voice_deinit_resampler(void)
{
    if (app_voice_resampler)
    {
        app_capture_resample_close(app_voice_resampler);
        app_voice_resampler = NULL;
    }
}

void app_tencent_voicepath_init_resampler(uint8_t channelCnt, uint32_t outputBufSize,
    uint32_t originalSampleRate, uint32_t targetSampleRate)
{
    if (NULL == app_voice_resampler)
    {
        TRACE(3,"Create the resampler output Size %d original sample rate %d target sample rate %d",
            outputBufSize, originalSampleRate, targetSampleRate);
        
        app_voice_resampler = app_capture_resample_any_open((enum AUD_CHANNEL_NUM_T)channelCnt, app_tencent_voicepath_resampled_data_come, 
            outputBufSize, ((float)originalSampleRate)/((float)(targetSampleRate)));
    }
}

void app_tencent_voicepath_resmple(uint8_t* ptrInput, uint32_t dataLen)
{
    TRACE(1,"Start resmple %d bytes", dataLen);
    uint32_t formerTicks = hal_sys_timer_get();
    app_capture_resample_run(app_voice_resampler, ptrInput, dataLen);
    TRACE(1,"Resmple cost %d ms", TICKS_TO_MS(hal_sys_timer_get()-formerTicks));
}


void app_pop_data_from_cqueue(CQueue* ptrQueue, uint8_t *buff, uint32_t len)
{
    uint8_t *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    PeekCQueue(ptrQueue, len, &e1, &len1, &e2, &len2);
    if (len==(len1+len2)){
        memcpy(buff,e1,len1);
        memcpy(buff+len1,e2,len2);
        DeCQueue(ptrQueue, 0, len);
    }else{
        memset(buff, 0x00, len);
    }
}   //= app_tencent_pop_data_from_cqueue

void app_tencent_voicepath_mailbox_put(uint32_t message_id, uint32_t param0, 
    uint32_t param1, uint32_t ptr)
{
    APP_MESSAGE_BLOCK msg;
    TRACE(2,"%s: %d\n", __FUNCTION__, message_id);
    msg.mod_id = APP_MODUAL_TENCENT_SMARTVOICE;
    msg.msg_body.message_id = message_id;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    msg.msg_body.message_ptr = ptr;
    app_mailbox_put(&msg);
}

static int app_tencent_voicepath_handle_process(APP_MESSAGE_BODY *msg_body)
{
    switch (msg_body->message_id) {
        case APP_VOICEPATH_MESSAGE_ID_CUSTOMER_FUNC:
            (APP_VOICEPATH_MOD_HANDLE_T(msg_body->message_ptr))();
            break;
        default: break;
    }
    
    return 0;
}

#endif
