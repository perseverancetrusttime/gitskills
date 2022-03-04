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
#include "cmsis_os.h"
#include "cmsis.h"
#include "hal_trace.h"
#include <string.h>
#include "app_utils.h"
#include "hal_timer.h"
#include <assert.h>
#include "cqueue.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "speech_ssat.h"
#include "speech_memory.h"
#include "audio_dump.h"
#include "app_thirdparty.h"

static uint32_t kws_demo_stream_handler(uint8_t *buf, uint32_t length);

// For 'Awaken lib'
// #define KWD_RECOG_RTOS
// #define NO_RTOS_10FRAME
#ifdef KWD_RECOG_RTOS
#include "command_recognition.h" // Awaken
#else
#include "aqe_kws.h"
AqeKwsState *aqe_kws_st = NULL;
#endif

//#define USE_CAPTURE_RAM
// For FF process
#ifdef __KWS_AUDIO_PROCESS__

//2MIC denoise
#ifdef __KWS_NS__
#include "speech_2mic_ns6.h"
Speech2MicNs6State *speech_tx_2mic_ns6_st1 = NULL;
#endif//__KWS_NS__

//AEC
#ifdef __KWS_AEC2__
#include "echo_canceller.h"
Ec2FloatState *kws_aec2float_st1 = NULL;
Ec2FloatState *kws_aec2float_st2 = NULL;
static short aec_out1[256];
static short aec_out2[256];
Ec2FloatConfig kws_aec2float_cfg = {
    /*  .bypass         =   */  0,
    /*  .hpf_enabled    =   */  true,
    /*  .af_enabled     =   */  true,
    /*  .nlp_enabled    =   */  false,
    /*  .clip_enabled   =   */  false,
    /*  .stsupp_enabled =   */  false,
    /*  .ns_enabled     =   */  false,
    /*  .cng_enabled    =   */  false,
    /*  .blocks         =   */  4,
    /*  .delay          =   */  0,
    /*  .min_ovrd       =   */  2,
    /*  .target_supp    =   */  -40,
    /*  .noise_supp     =   */  -15,
    /*  .cng_type       =   */  1,
    /*  .cng_level      =   */  -60,
    /*  .clip_threshold =   */  -20.f,
    /*  .banks          =   */  32,
};
#endif//__KWS_AEC2__
#define KWS_ABS(x) (x>0?x:(-x))
#endif//_KWS_AUDIO_PROCESS__


/**< KWS Engine hooks                    */
#define KWS_ENGINE_INIT(callback, treshold, words, priority)    AwakenInit(callback, treshold, words, priority)
#define KWS_ENGINE_FEED(frame, n_samples)                       AwakenBuffMicData(frame, n_samples)
#define KWS_ENGINE_DEINIT()                                     AwakenDestory()


/**< KWS configuration settings          */
#define KWS_MIC_BITS_PER_SAMPLE           (AUD_BITS_16)
#define KWS_MIC_SAMPLE_RATE               (16000) // (AUD_SAMPRATE_8000)
#define KWS_MIC_VOLUME                    (16)


/**< KWS Engine configuration settings   */
#ifdef NO_RTOS_10FRAME
#define KWS_ENGINE_SAMPLES_PER_FRAME      (128*10)
#else
 #define KWS_ENGINE_SAMPLES_PER_FRAME      (128)
#endif
#define KWS_ENGINE_SAMPLE_RATE            (8000)
#define KWS_ENGINE_SYSFREQ                (APP_SYSFREQ_208M)  // (APP_SYSFREQ_26M) 


/* Utility functions                     */
//#define TRACE(s, ...) TRACE(1,"%s: " s, __FUNCTION__, ## __VA_ARGS__);
#define ALIGN4 __attribute__((aligned(4)))

static bool kws_ai_connected = false;
static bool kws_ai_in_sco = false;
static bool kws_has_detected_key_words = false;
static bool kws_stop_speech = false;
static bool kws_provide_speech = false;

#define KWS_DATA_BUF_SIZE 320*4
CQueue kws_data_buf_queue;
uint8_t kws_data_buf[KWS_DATA_BUF_SIZE];

/* Add a memory to memory pool (enough to handle allocations by 'Awaken')*/
#define KWS_DEMO_MED_MEM_POOL_SIZE (1024*140)
#define MED_MEM_POOL_SIZE        (KWS_DEMO_MED_MEM_POOL_SIZE)
#if defined(USE_CAPTURE_RAM)
static uint8_t *kws_medMemPool;
#else
static uint8_t kws_medMemPool[KWS_DEMO_MED_MEM_POOL_SIZE];
#endif

/* Calcluate the audio-capture frame length, KWS_FRAME_LEN, based of decimation rate (1 or 2): */
#define KWS_FRAME_LEN                (KWS_ENGINE_SAMPLES_PER_FRAME * (KWS_MIC_SAMPLE_RATE / KWS_ENGINE_SAMPLE_RATE))
#ifdef __KWS_AUDIO_PROCESS__
#define CAPTURE_CHANNEL_NUM      (AUD_CHANNEL_NUM_3)
#else
#define CAPTURE_CHANNEL_NUM      (AUD_CHANNEL_NUM_1)
#endif
#define CAPTURE_BUF_SIZE         (KWS_FRAME_LEN * CAPTURE_CHANNEL_NUM * 2 * 2)
static uint8_t codec_capture_buf[CAPTURE_BUF_SIZE] ALIGN4;

#if (KWS_MIC_SAMPLE_RATE == KWS_ENGINE_SAMPLE_RATE)
#if (KWS_FRAME_LEN != KWS_ENGINE_SAMPLES_PER_FRAME)
#error "KWS_ENGINE Config error"
#endif
    //_Static_assert(KWS_FRAME_LEN == KWS_ENGINE_SAMPLES_PER_FRAME, "KWS_ENGINE Config error");
#else
     #if (KWS_FRAME_LEN != KWS_ENGINE_SAMPLES_PER_FRAME*2)
     #error "KWS_ENGINE Config error"
     #endif
#endif

#if defined(AUDIO_DEBUG)
#define KWS_DUMP
#endif

/* Forward declaration */
#ifdef KWD_RECOG_RTOS
static void kws_demo_words_cb(int word, int score);
#endif

static bool kws_init = false;

/**
 * @breif Handle one-time initialization, like setting up the memory pool
 */
static int kws_demo_init(bool init, const void *)
{
    if (init)
    {
        TRACE(0,"Initialize kws_demo");

#if defined(USE_CAPTURE_RAM)
        app_capture_audio_mempool_init();
        app_capture_audio_mempool_get_buff(&kws_medMemPool, KWS_DEMO_MED_MEM_POOL_SIZE);
#endif
        med_heap_init(kws_medMemPool, KWS_DEMO_MED_MEM_POOL_SIZE);

        // Initialize the KWS ENGINE and install word-callback
#ifdef KWD_RECOG_RTOS
#if defined(AQE_KWS_TIANMAO)
        int threshod_buf[1]={95};
        KWS_ENGINE_INIT(kws_demo_words_cb, threshod_buf, 1, 0);
#elif defined(AQE_KWS_ALEXA)
        int threshod_buf[6]={66,95,71,80,100,91};
        KWS_ENGINE_INIT(kws_demo_words_cb, threshod_buf, 6, 0);
#elif defined(AQE_KWS_XIAOWEI)
        int threshod_buf[7]={87,120,120,120,120,120,120};
        KWS_ENGINE_INIT(kws_demo_words_cb, threshod_buf, 7, 0);
#elif defined(AQE_KWS_ANC)
        int threshod_buf[5]={60,60,60,62,97};
        KWS_ENGINE_INIT(kws_demo_words_cb, threshod_buf, 5, 0);
#else
#error NO_KWS NAME
#endif        

#else
        aqe_kws_st = aqe_kws_create(AUD_SAMPRATE_8000, 128, NULL);
#endif


#ifdef __KWS_AUDIO_PROCESS__

#ifdef __KWS_NS__
        TRACE(0,"-----speech_2mic_ns6_create start-----");
            speech_tx_2mic_ns6_st1 = speech_2mic_ns6_create(16000,128);
        TRACE(0,"-----speech_2mic_ns6_create finish-----");        
#endif

#ifdef __KWS_AEC2__
        TRACE(0,"-----ec2float_create start1-----");
        kws_aec2float_st1 = ec2float_create(16000, KWS_FRAME_LEN, &kws_aec2float_cfg);
        TRACE(0,"-----ec2float_create start2-----");
        kws_aec2float_st2 = ec2float_create(16000, KWS_FRAME_LEN, &kws_aec2float_cfg);
        TRACE(0,"-----ec2float_create end-----");
#endif

#endif
    
    } else {
        TRACE(0,"Destroy kws_demo");
#ifdef KWD_RECOG_RTOS
        KWS_ENGINE_DEINIT();
#else
        aqe_kws_destroy(aqe_kws_st);
#endif

#ifdef __KWS_AUDIO_PROCESS__

#ifdef __KWS_NS__
		TRACE(0,"-----speech_2mic_ns6_destroy start-----");
        speech_2mic_ns6_destroy(speech_tx_2mic_ns6_st1);
		TRACE(0,"-----speech_2mic_ns6_destroy start-----");		
#endif

#ifdef __KWS_AEC2__
        ec2float_destroy(kws_aec2float_st1);
        ec2float_destroy(kws_aec2float_st2);
#endif

#endif
        
        size_t total = 0, used = 0, max_used = 0;
        speech_memory_info(&total, &used, &max_used);
        TRACE(3,"SPEECH MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
        ASSERT(used == 0, "[%s] used != 0", __func__);
    }

    kws_init = init;

    return 0;
}

#if (KWS_MIC_SAMPLE_RATE != KWS_ENGINE_SAMPLE_RATE)
/**
 * @breif Half-band filter for 16->8KHz sample-rate conversion
 */
static short filter_iir_halfband(int32_t PcmValue)
{
    const float NUM[3] = {0.22711796, 0.45423593, 0.22711796};
    const float DEN[3] = {1.000000, -0.2766646, 0.18513647};
    static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;

    x0 = PcmValue;
    y0 = x0*NUM[0] + x1*NUM[1] + x2*NUM[2] - y1*DEN[1] - y2*DEN[2];
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    return speech_ssat_int16((int32_t)(y0));
}


/**
 * @brief 2:1 decimation filter with halfband filter
 *
 * @param in        Pointer of the PCM data.
 * @param out       Output resampled PCM data
 * @param in_len    Length of the PCM data in the buffer in samples.
 *
 * @return int      Size of data in output buffer
 */
static int filter_decimation_2to1(const short *in, short *out, uint32_t in_len)
{
    short tmp;

    for(uint32_t i = 0; i < in_len; i+=2) {
        tmp = filter_iir_halfband(in[i]);
        tmp = filter_iir_halfband(in[i+1]);
        *out++ = tmp;
    }

    return in_len/2;
}
#endif

/**
 * @brief DC block IIR filter
 *
 * @param inout     Pointer of the PCM data (modify inplace).
 * @param in_len    Length of the PCM data in the buffer in samples.
 *
 */
static void filter_iir_dc_block(short *inout, int in_len)
{
    static int z = 0;
    int tmp;

    for (int i = 0; i < in_len; i++)
    {
        z = (15 * z + inout[i]) >> 4;
        tmp = inout[i] - z;
        inout[i] = speech_ssat_int16(tmp);
    }
}

/**
 * @brief Trigger word callback from kws lib
 *
 * @param word      Detected word
 * @param score     Score of word
 *
 */
#ifdef KWD_RECOG_RTOS
static void kws_demo_words_cb(int word, int score)
{
    static int wake_count = 0;
    AIV_WAKEUP_INFO_T wake_word;
    wake_word.score = score;
    wake_word.wake_up_type = THIRDPARTY_TYPE__TAP;
#if defined(AQE_KWS_TIANMAO)
    if (1 == word)
    {
        wake_count++;
        TRACE(2,"---------------TianMaoJingLing KWS recog score = %d "
                  "wakeup_ count = %d------------", score, wake_count);

        app_thirdparty_callback_handle(THIRDPARTY_WAKE_UP_CALLBACK, (void *)&wake_word, 0);
    }
    else
    {
        //TRACE(2,"Word: %d, score = %d", word, score);
    }

#elif defined(AQE_KWS_ALEXA)
    if (1 == word) {
        wake_count++;
        TRACE(2,"---------------Alexa KWS recog score = %d "
                  "wakeup_ count = %d------------", score, wake_count);

        if (!app_thirdparty_callback_handle(THIRDPARTY_WAKE_UP_CALLBACK, (void *)&wake_word, 0))
        {
            if (!app_thirdparty_callback_handle(THIRDPARTY_START_SPEECH_CALLBACK, NULL, 0))
            {
                kws_has_detected_key_words = true;
            }
        }
    }
    else 
    {
        if(2 == word)
        {           
            TRACE(1,"HeyBreeno, score = %d", score);
        }       
        else if(3 == word)
        {
            TRACE(1,"NiHaoXiaoBu, score = %d", score);
        }
        else if(4 == word)
        {           
            TRACE(1,"XiaoBuXiaoBu, score = %d", score);
        }       
        else if(5 == word)
        {           
            TRACE(1,"TianMaoJingLing, score = %d", score);
        }       
        else if(6 == word)
        {           
            TRACE(1,"TuYaZhiNeng, score = %d", score);
        }       
    }

#else
#error NO_KWS NAME
#endif 


}

#endif
/**
 * @brief Setup audio streaming from MIC 
 *
 * @param do_stream   start / stop streaming
 *
 * @return int 0 means no error happened
 */
static int kws_demo_stream_start(bool open_stream, const void *)
{
    static bool stream_is_runing =  false;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(2,"Is running:%d enable:%d",
              stream_is_runing, open_stream);

    if (!kws_ai_connected)
    {
        TRACE(1,"%s AI don't connect", __func__);
    }

    if (stream_is_runing == open_stream)
    {
        return 0;
    }

    if (open_stream)
    {
        stream_is_runing = true;
        // Request sufficient system clock
        TRACE(1,"KWS_ENGINE_SYSFREQ is %d",KWS_ENGINE_SYSFREQ);
        app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, KWS_ENGINE_SYSFREQ);
        // TRACE(1,"Yi sys freq calc: %d Hz", hal_sys_timer_calc_cpu_freq(5, 0));
#if defined(AUDIO_DEBUG)
        //audio_dump_init(KWS_FRAME_LEN, sizeof(short), 2);
        audio_dump_init(KWS_FRAME_LEN, sizeof(short), 4);
#endif

        InitCQueue(&kws_data_buf_queue, KWS_DATA_BUF_SIZE, (CQItemType *)kws_data_buf);
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.sample_rate = (AUD_SAMPRATE_T)KWS_MIC_SAMPLE_RATE;
        stream_cfg.bits        = KWS_MIC_BITS_PER_SAMPLE;
        stream_cfg.vol         = KWS_MIC_VOLUME;
        stream_cfg.device      = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path     = AUD_INPUT_PATH_ASRMIC;
        stream_cfg.channel_num = CAPTURE_CHANNEL_NUM;
        stream_cfg.handler     = kws_demo_stream_handler;
        stream_cfg.data_ptr    = codec_capture_buf;
        stream_cfg.data_size   = CAPTURE_BUF_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        TRACE(0,"audio capture ON");
    }
    else
    {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, APP_SYSFREQ_32K);
        // TRACE(1,"sys freq calc:(32K) %d Hz", hal_sys_timer_calc_cpu_freq(5, 0));

        stream_is_runing = false;
        TRACE(0,"audio capture OFF");
    }

    return 0;
}

static void kws_demo_provide_speech_handle(void)
{
    uint32_t len_of_queue = 0;

    if (kws_ai_connected && !kws_ai_in_sco)
    {
        len_of_queue = LengthOfCQueue(&kws_data_buf_queue);
        TRACE(2,"%s len %d", __func__, len_of_queue);
        if (len_of_queue)
        {
            DeCQueue(&kws_data_buf_queue, NULL, len_of_queue);
        }

        app_thirdparty_callback_handle(THIRDPARTY_START_SPEECH_CALLBACK, NULL, 0);
        kws_has_detected_key_words = true;
    }
}

static int kws_demo_provide_speech(bool status, void *param)
{
    TRACE(2,"%s wake %d",__func__, kws_has_detected_key_words);
    if (!kws_has_detected_key_words)
    {
        kws_provide_speech = true;
    }
    return 0;
}

static int kws_demo_stop_speech(bool status, void *param)
{
    TRACE(2,"%s wake %d",__func__, kws_has_detected_key_words);
    if (kws_has_detected_key_words)
    {
        kws_stop_speech = true;
    }
    return 0;
}

static int kws_demo_ai_connect(bool status, void *param)
{
    int ret = 0;

    TRACE(3, "%s ai connect %d sco %d", __func__, kws_ai_connected, kws_ai_in_sco);

    if (!kws_ai_connected)
    {
        kws_ai_connected = true;
        if (!kws_ai_in_sco)
        {
            kws_demo_init(true, NULL);
            ret = kws_demo_stream_start(true, NULL);
        }
    }

    return ret;
}

static int kws_demo_ai_disconnect(bool status, void *param)
{
    int ret = 0;

    TRACE(3, "%s ai connect %d sco %d", __func__, kws_ai_connected, kws_ai_in_sco);

    if (kws_ai_connected)
    {
        if (!kws_ai_in_sco)
        {
            ret = kws_demo_stream_start(false, NULL);
            kws_demo_init(false, NULL);
            app_thirdparty_callback_handle(THIRDPARTY_STOP_SPEECH_CALLBACK, NULL, 0);
        }
        kws_ai_connected = false;
    }

    return ret;
}

static int kws_demo_call_start(bool status, void *param)
{
    int ret = 0;
    TRACE(3, "%s ai connect %d sco %d", __func__, kws_ai_connected, kws_ai_in_sco);
    if (!kws_ai_in_sco)
    {
        kws_ai_in_sco = true;
        if (kws_ai_connected)
        {
            ret = kws_demo_stream_start(false, NULL);
            kws_demo_init(false, NULL);
            app_thirdparty_callback_handle(THIRDPARTY_STOP_SPEECH_CALLBACK, NULL, 0);
        }
    }

    return ret;
}

static int kws_demo_call_stop(bool status, void *param)
{
    TRACE(1, "%s", __func__);
    kws_demo_init(true, NULL);
    return kws_demo_stream_start(true, NULL);


    int ret = 0;
    TRACE(3, "%s ai connect %d sco %d", __func__, kws_ai_connected, kws_ai_in_sco);
    if (kws_ai_in_sco)
    {
        kws_ai_in_sco = false;
        if (kws_ai_connected)
        {
            kws_demo_init(true, NULL);
            ret = kws_demo_stream_start(true, NULL);
        }
    }

    return ret;
}

/**
 * @brief Process the collected PCM data from MIC.
 *        Resample audio stream to 8KHz and pass audio to kws lib.
 *
 * @param buf       Pointer of the PCM data buffer to access.
 * @param length    Length of the PCM data in the buffer in bytes.
 *
 * @return uint32_t 0 means no error happened
 */
int awk_cnt=0;
uint8_t ama_buf[640];

#ifdef __KWS_AUDIO_PROCESS__
short dump_mic1[KWS_FRAME_LEN];
short dump_mic2[KWS_FRAME_LEN];
short dump_mic3[KWS_FRAME_LEN];
//short dump_mic4[KWS_FRAME_LEN];
short echo_data[KWS_FRAME_LEN];
short denoi_buf[KWS_FRAME_LEN*2];
#endif
static uint32_t kws_demo_stream_handler(uint8_t *buf, uint32_t length)
{
    //TRACE(1,"Free audio buf = %d",app_audio_mempool_free_buff_size());
    //TRACE(1,"Free capture audio buf = %d",app_capture_audio_mempool_free_buff_size());

    if (!kws_init) {
        TRACE(2, "[%s] kws_init: %d", __func__, kws_init);
        return 0;
    }

#if (KWS_MIC_SAMPLE_RATE == KWS_ENGINE_SAMPLE_RATE)
    short *frame = (short*)buf;
#else
    short frame[KWS_ENGINE_SAMPLES_PER_FRAME];
 #ifdef __KWS_AUDIO_PROCESS__
    short *dump_buf = (short*)buf;
    for(int i=0;i<KWS_FRAME_LEN;i++)
    {
        dump_mic1[i]=dump_buf[3*i+0];
        dump_mic2[i]=dump_buf[3*i+1];
        dump_mic3[i]=dump_buf[3*i+2];   
        //dump_mic4[i]=dump_buf[4*i+3];
    }
    filter_iir_dc_block(dump_mic1, KWS_FRAME_LEN);  
    filter_iir_dc_block(dump_mic2, KWS_FRAME_LEN);
    filter_iir_dc_block(dump_mic3, KWS_FRAME_LEN);  
 #endif
#endif

#ifdef __KWS_AUDIO_PROCESS__

#if defined(KWS_DUMP)    
        audio_dump_clear_up();
        audio_dump_add_channel_data(0, dump_mic1, KWS_FRAME_LEN);
        //audio_dump_add_channel_data(1, dump_mic2, KWS_FRAME_LEN);    
        //audio_dump_add_channel_data(2, dump_mic3, KWS_FRAME_LEN);
#endif
#ifdef __KWS_AEC2__
    float echo_vol=0;

    //AEC
    for(unsigned i=0;i<KWS_FRAME_LEN;i++)
    {
        echo_data[i] = dump_mic3[i];
        echo_vol += KWS_ABS((float)dump_mic3[i])/KWS_FRAME_LEN;
    }
    //TRACE(1,"echo_vol=%d",(int)echo_vol);
    
    if(1)//100<echo_vol)
    {
        if(1 && NULL!=kws_aec2float_st1 && NULL!=kws_aec2float_st2)
        {
            ec2float_process(kws_aec2float_st1, dump_mic1, echo_data, KWS_FRAME_LEN, aec_out1);
            for(unsigned i=0;i<KWS_FRAME_LEN;i++)
            {
                echo_data[i] = dump_mic3[i];
            }
            if(false==kws_has_detected_key_words)
            {
            	ec2float_process(kws_aec2float_st2, dump_mic2, echo_data, KWS_FRAME_LEN, aec_out2);
            }
        }
        else
        {
            for(unsigned i=0;i<KWS_FRAME_LEN;i++)
            {
                aec_out1[i] = dump_mic1[i];
                aec_out2[i] = dump_mic2[i];
            }           
        }
    }
    else
    {
        for(unsigned i=0;i<KWS_FRAME_LEN;i++)
        {
            aec_out1[i] = dump_mic1[i];
            aec_out2[i] = dump_mic2[i];
        }
    }

#if defined(KWS_DUMP) 
        if(kws_has_detected_key_words)
        {
            kws_has_detected_key_words = false;
            for(unsigned i=0;i<KWS_FRAME_LEN;i++)
                aec_out1[i]=32767;
        }

        ///audio_dump_add_channel_data(1, dump_mic3, KWS_FRAME_LEN);    
        //audio_dump_add_channel_data(2, dump_mic2, KWS_FRAME_LEN);
        //audio_dump_add_channel_data(3, aec_out2, KWS_FRAME_LEN);
#endif
#else //__KWS_AEC2__
    for(unsigned i=0;i<KWS_FRAME_LEN;i++)
    {
        aec_out1[i] = dump_mic1[i];
        aec_out2[i] = dump_mic2[i];
    }
#endif //__KWS_AEC2__

    for(unsigned i=0;i<KWS_FRAME_LEN;i++)
    {
        denoi_buf[2*i+0] = aec_out1[i];
        denoi_buf[2*i+1] = aec_out2[i];
    }

#ifdef __KWS_NS__
    //Denoise

    if(kws_has_detected_key_words)
    {
        for(unsigned i=0;i<KWS_FRAME_LEN;i++)
        {
            dump_mic1[i] = aec_out1[i];
        }
    }
    else
    {
        if(1 && NULL!=speech_tx_2mic_ns6_st1)
        {
            speech_2mic_ns6_process(speech_tx_2mic_ns6_st1, denoi_buf, KWS_FRAME_LEN*2, denoi_buf);
        }
        for(unsigned i=0;i<KWS_FRAME_LEN;i++)
        {
            dump_mic1[i] = denoi_buf[i];
        }
    }

#endif//__KWS_NS__

#if defined(KWS_DUMP)        
        audio_dump_add_channel_data(1, aec_out1, KWS_FRAME_LEN);   
        audio_dump_add_channel_data(2, dump_mic1, KWS_FRAME_LEN);
        audio_dump_add_channel_data(3, aec_out2, KWS_FRAME_LEN);
        audio_dump_run();
#endif  

    
#endif //__KWS_AUDIO_PROCESS__

    //TRACE(1,"awk_call=%d",awk_call);
    if(true == kws_has_detected_key_words)
    {
        if (kws_stop_speech)
        {
            TRACE(1,"%s kws_stop_speech", __func__);
            kws_stop_speech = false;
            app_thirdparty_callback_handle(THIRDPARTY_STOP_SPEECH_CALLBACK, NULL, 0);
            ResetCQueue(&kws_data_buf_queue);
            kws_has_detected_key_words = false;
        }
        else
        {
            EnCQueue_AI(&kws_data_buf_queue, buf, length);
            if (LengthOfCQueue(&kws_data_buf_queue) >= 640)
            {
                //stime=hal_sys_timer_get();
                DeCQueue(&kws_data_buf_queue, ama_buf, 640);
                //TRACE(4,"ama_buf[0]=%d,ama_buf[50]=%d,ama_buf[150]=%d,ama_buf[250]=%d,",ama_buf[0],ama_buf[50],ama_buf[150],ama_buf[250]);
                app_thirdparty_callback_handle(THIRDPARTY_DATA_COME_CALLBACK, (void *)ama_buf, 640);
            }
            return 0;
        }
    }
    else
    {
        if (kws_provide_speech)
        {
            kws_provide_speech = false;
            kws_demo_provide_speech_handle();
            kws_has_detected_key_words = true;
        }
    }


#if (KWS_MIC_SAMPLE_RATE != KWS_ENGINE_SAMPLE_RATE)
#ifdef __KWS_AUDIO_PROCESS__
    filter_decimation_2to1((const short *)dump_mic1, frame, KWS_FRAME_LEN);
#else
    filter_decimation_2to1((const short *)buf, frame, KWS_FRAME_LEN);
#endif
#endif
    filter_iir_dc_block(frame, KWS_ENGINE_SAMPLES_PER_FRAME);

#ifdef KWD_RECOG_RTOS
    KWS_ENGINE_FEED(frame, KWS_ENGINE_SAMPLES_PER_FRAME);
#else
    //TRACE(0,"123");
    short thres_tmp[1] = {77};
    int awk_flg=0;
    if(NULL!=aqe_kws_st)
    {
#ifndef NO_RTOS_10FRAME
        awk_flg = aqe_kws_process(aqe_kws_st, frame, 128, thres_tmp);
#else
#if 0
        int stime = hal_fast_sys_timer_get();       
#endif
        awk_flg = aqe_kws_process(aqe_kws_st, frame, 128 * 10, thres_tmp);
#if 0
    int etime = hal_fast_sys_timer_get();
    TRACE(1,"dnn: %5u us", FAST_TICKS_TO_US(etime - stime));
    // TRACE(1,"sys freq calc: %d Hz", hal_sys_timer_calc_cpu_freq(5, 0));
#endif
#endif
    }
    if(awk_flg)
    {
        awk_cnt++;
        TRACE(2,"awk_flg is %d,awk_cnt=%d",awk_flg,awk_cnt);
    }
#endif

#if defined(KWS_DUMP)        
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, frame, KWS_ENGINE_SAMPLES_PER_FRAME);
    //audio_dump_add_channel_data(1, aec_out1, KWS_FRAME_LEN);
    //audio_dump_add_channel_data(2, dump_mic3, KWS_FRAME_LEN);
    //audio_dump_add_channel_data(3, dump_mic3, KWS_FRAME_LEN);         
    audio_dump_run();
#endif

    return 0;
}

#ifndef __AMA_VOICE__
#define THIRDPARTY_FUNC_TYPE THIRDPARTY_FUNC_NO1
THIRDPARTY_HANDLER_TAB(KWS_ALEXA_LIB_NAME)
{
    {TP_EVENT(THIRDPARTY_INIT,             kws_demo_init),         true,   NULL},
    {TP_EVENT(THIRDPARTY_START,            kws_demo_stream_start), true,   NULL},
    {TP_EVENT(THIRDPARTY_STOP,             kws_demo_stream_start), false,  NULL},
    {TP_EVENT(THIRDPARTY_DEINIT,           kws_demo_init),         false,  NULL},
};
#else
#define THIRDPARTY_FUNC_TYPE THIRDPARTY_FUNC_KWS
THIRDPARTY_HANDLER_TAB(KWS_ALEXA_LIB_NAME)
{
    {TP_EVENT(THIRDPARTY_AI_PROVIDE_SPEECH, kws_demo_provide_speech),   false,  NULL},
    {TP_EVENT(THIRDPARTY_AI_STOP_SPEECH,    kws_demo_stop_speech),      false,  NULL},
    {TP_EVENT(THIRDPARTY_AI_CONNECT,        kws_demo_ai_connect),       false,  NULL},
    {TP_EVENT(THIRDPARTY_AI_DISCONNECT,     kws_demo_ai_disconnect),    false,  NULL},
    {TP_EVENT(THIRDPARTY_CALL_START,        kws_demo_call_start),       false,  NULL},
    {TP_EVENT(THIRDPARTY_CALL_STOP,         kws_demo_call_stop),        false,  NULL},
};
#endif


THIRDPARTY_HANDLER_TAB_SIZE(KWS_ALEXA_LIB_NAME)

