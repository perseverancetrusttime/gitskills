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
/*******************************************************************************
** namer    : Audio Process
** description  : Manage auido process algorithms, include :hw iir eq, sw iir eq,
**                  hw fir eq, drc...
** version  : V1.0
** author   : Yunjie Huo
** modify   : 2018.12.4
** todo     : NULL
** MIPS     :   NO PROCESS: 34M(TWS, one ear, Mono, AAC, 48k)
**              DRC1: 36M(3 bands)
**              DRC2: 12M
*******************************************************************************/
/*
Audio Flow:
    DECODE --> SW IIR EQ --> DRC --> LIMTER --> VOLUME --> HW IIR EQ --> SPK

                                                       +-----------------------------+
                                                       |             DAC             |
                                                       |                             |
+--------+     +-----------+    +-----+    +--------+  | +--------+    +-----------+ |  +-----+
|        | PCM |           |    |     |    |        |  | |        |    |           | |  |     |
| DECODE +---->+ SW IIR EQ +--->+ DRC +--->+ LIMTER +--->+ VOLUME +--->+ HW IIR EQ +--->+ SPK |
|        |     |           |    |     |    |        |  | |        |    |           | |  |     |
+--------+     +-----------+    +-----+    +--------+  | +--------+    +-----------+ |  +-----+
                                                       +-----------------------------+

| ------------ | ------------------------- | -------- | ----------- |
| Algorithm    | description               | MIPS(M)  | RAM(kB)     |
| ------------ | ------------------------- | -------- | ----------- |
| DRC          | Dynamic Range Compression | 12M/band | 13          |
| Limiter/DRC2 | Limiter                   | 12M      | 5           |
| EQ           | Equalizer                 | 1M/band  | Almost zero |
*/

#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "string.h"
#include "audio_process.h"
#include "stdbool.h"
#include "hal_location.h"
#include "hw_codec_iir_process.h"
#include "hw_iir_process.h"
#include "tgt_hardware.h"
#include "drc.h"
#include "limiter.h"
#include "audio_cfg.h"
#include "hal_codec.h"
#include "hw_limiter_cfg.h"

// Enable this to test AUDIO_CUSTOM_EQ with audiotools,
// eq config from audiotools will be merge with default eq config.
// #define TEST_AUDIO_CUSTOM_EQ

#if defined(AUDIO_CUSTOM_EQ)
#include "heap_api.h"
#include "ae_math.h"
#endif

#if defined(ANC_APP)
#include "app_anc.h"
#endif

//#define AUDIO_PROCESS_DUMP
#ifdef AUDIO_PROCESS_DUMP
#include "audio_dump.h"
#endif

#if defined(USB_EQ_TUNING)
#if !defined(__HW_DAC_IIR_EQ_PROCESS__) && !defined(__SW_IIR_EQ_PROCESS__)
#error "Either HW_DAC_IIR_EQ_PROCESS or SW_IIR_EQ_PROCESS should be defined when enabling USB_EQ_TUNING"
#endif
#endif

#if defined(AUDIO_EQ_TUNING)
#include "hal_cmd.h"

#if defined(__SW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_SW_IIR_UPDATE_CFG
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
#define AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_HW_IIR_UPDATE_CFG
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#define AUDIO_EQ_HW_FIR_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_UPDATE_CFG
#endif

#ifdef __AUDIO_LIMITER__
#define AUDIO_LIMITER_UPDATE_CFG
#endif

#endif

#if defined(__SW_IIR_EQ_PROCESS__)
extern const IIR_CFG_T * const audio_eq_sw_iir_cfg_list[EQ_SW_IIR_LIST_NUM];
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
extern const IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_dac_iir_cfg_list[EQ_HW_DAC_IIR_LIST_NUM];
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
extern const IIR_CFG_T * const POSSIBLY_UNUSED audio_eq_hw_iir_cfg_list[EQ_HW_IIR_LIST_NUM];
#endif

#ifdef __HW_FIR_EQ_PROCESS__
extern const FIR_CFG_T * const audio_eq_hw_fir_cfg_list[EQ_HW_FIR_LIST_NUM];
#endif

#ifdef __AUDIO_DRC__
extern const DrcConfig audio_drc_cfg;
#endif

#ifdef __AUDIO_LIMITER__
extern const LimiterConfig audio_limiter_cfg;
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) || defined(AUDIO_EQ_HW_FIR_UPDATE_CFG)|| defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)|| defined(AUDIO_EQ_HW_IIR_UPDATE_CFG) || defined(AUDIO_DRC_UPDATE_CFG) || defined(AUDIO_LIMITER_UPDATE_CFG)
#define AUDIO_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_NEEDED_SIZE (1024*13)
#else
#define AUDIO_DRC_NEEDED_SIZE (0)
#endif

#ifdef __AUDIO_LIMITER__
#define AUDIO_LIMITER_NEEDED_SIZE (1024*5)
#else
#define AUDIO_LIMITER_NEEDED_SIZE (0)
#endif

#define AUDIO_MEMORY_SIZE (AUDIO_DRC_NEEDED_SIZE + AUDIO_LIMITER_NEEDED_SIZE)

#if AUDIO_MEMORY_SIZE > 0
#include "audio_memory.h"
#endif

#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV                    CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif

typedef signed int          pcm_24bits_t;
typedef signed short int    pcm_16bits_t;

#ifdef AUDIO_HEARING_COMPSATN
extern float get_hear_compen_val_fir(int *test_buf, float *gain_buf, int num);
extern float get_hear_compen_val_multi_level(int *test_buf, float *gain_buf, int num);
extern short get_hear_level(void);
#include "hear_fir2.h"
#include "speech_memory.h"
#define HEAR_COMP_BUF_SIZE    1024*17
#ifdef HEARING_USE_STATIC_RAM
extern char HEAR_DET_STREAM_BUF[HEAR_COMP_BUF_SIZE];
#else
uint8_t *hear_comp_buf = NULL;
#endif

//int band_std[6] = {14,10,10,8,9,18};
#if HWFIR_MOD==HEARING_MOD_VAL
float fir_filter[384] = {0.0F};
#else
float hear_iir_coef[30] = {0.0F};
#endif
#define HWFIR_MOD   0
#define SWIIR_MOD   1
#define HWIIR_MOD   2
#endif

typedef struct{
    enum AUD_BITS_T         sample_bits;
    enum AUD_SAMPRATE_T     sample_rate;
    enum AUD_CHANNEL_NUM_T  ch_num;

#if defined(__SW_IIR_EQ_PROCESS__)
    bool sw_iir_enable;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    bool hw_dac_iir_enable;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
    bool hw_iir_enable;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
    bool hw_fir_enable;
#endif

#if AUDIO_MEMORY_SIZE > 0
    uint8_t *audio_heap;
#endif

#ifdef __AUDIO_DRC__
    DrcState *drc_st;
#endif

#ifdef __AUDIO_LIMITER__
    LimiterState *limiter_st;
#endif

#ifdef AUDIO_UPDATE_CFG
    bool update_cfg;
#endif

#ifdef USB_EQ_TUNING
	bool eq_updated_cfg;
#endif

#ifdef AUDIO_HEARING_COMPSATN
    //bool hear_updated_cfg;
#if SWIIR_MOD==HEARING_MOD_VAL || HWIIR_MOD==HEARING_MOD_VAL
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
        IIR_CFG_T hear_iir_cfg;
        IIR_CFG_T hear_iir_cfg_update;
#endif
#elif HWFIR_MOD==HEARING_MOD_VAL
#if defined(__HW_FIR_EQ_PROCESS__)
        FIR_CFG_T hear_hw_fir_cfg;
#endif
#endif
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    IIR_CFG_T sw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    IIR_CFG_T hw_dac_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    IIR_CFG_T hw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    FIR_CFG_T hw_fir_cfg;
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    bool drc_update;
    DrcConfig drc_cfg;
#endif

#ifdef AUDIO_LIMITER_UPDATE_CFG
    bool limiter_update;
    LimiterConfig limiter_cfg;
#endif

#ifdef AUDIO_CUSTOM_EQ
    uint32_t nfft;
    float *work_buffer;
#endif

} AUDIO_PROCESS_T;

static AUDIO_PROCESS_T audio_process = {
    .sample_bits = AUD_BITS_24,
    .sample_rate = AUD_SAMPRATE_44100,
    .ch_num = AUD_CHANNEL_NUM_2,

#if defined(__SW_IIR_EQ_PROCESS__)
    .sw_iir_enable =  false,
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    .hw_dac_iir_enable =  false,
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
    .hw_iir_enable =  false,
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
    .hw_fir_enable =  false,
#endif

#ifdef AUDIO_HEARING_COMPSATN
#if SWIIR_MOD==HEARING_MOD_VAL || HWIIR_MOD==HEARING_MOD_VAL
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
    .hear_iir_cfg = {.num = 0},
    .hear_iir_cfg_update = {.num = 0},
#endif
#elif HWFIR_MOD==HEARING_MOD_VAL
#if defined(__HW_FIR_EQ_PROCESS__)
    .hear_hw_fir_cfg = {.len = 0},
#endif
#endif
#endif


#ifdef AUDIO_UPDATE_CFG
    .update_cfg = false,
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    .sw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    .hw_dac_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    .hw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    .hw_fir_cfg = {.len = 0},
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    .drc_update = false,
    .drc_cfg = {
        .knee = 0,
        .filter_type = {-1, -1},
        .band_num = 1,
        .look_ahead_time = 0,
        .band_settings = {
            {0, 0, 1, 1, 1, 1},
            {0, 0, 1, 1, 1, 1},
        }
    },
#endif

#ifdef AUDIO_LIMITER_UPDATE_CFG
    .limiter_update = false,
    .limiter_cfg = {
        .knee = 0,
        .look_ahead_time = 0,
        .threshold = 0,
        .makeup_gain = 0,
        .ratio = 1000,
        .attack_time = 1,
        .release_time = 1,
    },
#endif
};

#if defined(__HW_FIR_EQ_PROCESS_2CH__)
int audio_eq_set_cfg(const FIR_CFG_T *fir_cfg,const FIR_CFG_T *fir_cfg_2,const IIR_CFG_T *iir_cfg,AUDIO_EQ_TYPE_T audio_eq_type)
#else
int audio_eq_set_cfg(const FIR_CFG_T *fir_cfg,const IIR_CFG_T *iir_cfg,AUDIO_EQ_TYPE_T audio_eq_type)
#endif
{
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(iir_cfg)
            {
                audio_process.sw_iir_enable = false;
#ifdef USB_EQ_TUNING
                if (audio_process.eq_updated_cfg) {
                    iir_set_cfg(&audio_process.sw_iir_cfg);
                } else
#endif
                {
#if defined(AUDIO_HEARING_COMPSATN) && SWIIR_MOD==HEARING_MOD_VAL
                    iir_set_cfg(&audio_process.hear_iir_cfg_update);
#else
                    iir_set_cfg(iir_cfg);
#endif
                }
                audio_process.sw_iir_enable = true;
            }
            else
            {
                audio_process.sw_iir_enable = false;
            }
        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(fir_cfg)
            {
                audio_process.hw_fir_enable = false;
#if defined(AUDIO_HEARING_COMPSATN) && HWFIR_MOD==HEARING_MOD_VAL
                fir_set_cfg(&audio_process.hear_hw_fir_cfg);
#elif __HW_FIR_EQ_PROCESS_2CH__
                fir_set_cfg_ch(fir_cfg,AUD_CHANNEL_NUM_1);
                fir_set_cfg_ch(fir_cfg_2,AUD_CHANNEL_NUM_2);  
#else

                fir_set_cfg(fir_cfg);
#endif
                audio_process.hw_fir_enable = true;
            }
            else
            {
                audio_process.hw_fir_enable = false;
            }
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(iir_cfg)
            {
                HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac=NULL;
                enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
                sample_rate_hw_dac_iir=hal_codec_get_real_sample_rate(audio_process.sample_rate,1);
                TRACE(3,"audio_process.sample_rate:%d, sample_rate_hw_dac_iir: %d.", audio_process.sample_rate, sample_rate_hw_dac_iir);
#else
                sample_rate_hw_dac_iir=audio_process.sample_rate;
#endif
                audio_process.hw_dac_iir_enable = false;
#ifdef USB_EQ_TUNING
                if (audio_process.eq_updated_cfg) {
                    hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir, &audio_process.hw_dac_iir_cfg);
                } else
#endif
                {
#if defined(AUDIO_HEARING_COMPSATN) && HWIIR_MOD==HEARING_MOD_VAL
                    hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,&audio_process.hear_iir_cfg_update);
#else
                    hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,iir_cfg);
#endif
                }
                ASSERT(hw_iir_cfg_dac != NULL, "[%s] codec IIR parameter error!", __func__);
                hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC);
                audio_process.hw_dac_iir_enable = true;
            }
            else
            {
                audio_process.hw_dac_iir_enable = false;
            }
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(iir_cfg)
            {
                HW_IIR_CFG_T *hw_iir_cfg=NULL;
                audio_process.hw_iir_enable = false;
                hw_iir_cfg = hw_iir_get_cfg(audio_process.sample_rate,iir_cfg);
                ASSERT(hw_iir_cfg != NULL,"[%s] 0x%x codec IIR parameter error!", __func__, (unsigned int)hw_iir_cfg);
                hw_iir_set_cfg(hw_iir_cfg);
                audio_process.hw_iir_enable = true;
            }
            else
            {
                audio_process.hw_iir_enable = false;
            }

        }
        break;
#endif

        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif

    return 0;
}

#ifdef AUDIO_PROCESS_DUMP
short dump_buf[1024] = {0};
#endif
int SRAM_TEXT_LOC audio_process_run(uint8_t *buf, uint32_t len)
{
    int POSSIBLY_UNUSED pcm_len = 0;

    if(audio_process.sample_bits == AUD_BITS_16)
    {
        pcm_len = len / sizeof(pcm_16bits_t);
    }
    else if(audio_process.sample_bits == AUD_BITS_24)
    {
        pcm_len = len / sizeof(pcm_24bits_t);
    }
    else
    {
        ASSERT(0, "[%s] bits(%d) is invalid", __func__, audio_process.sample_bits);
    }

#ifdef AUDIO_PROCESS_DUMP
    int *buf32 = (int *)buf;
    for(int i=0;i<1024;i++)
        dump_buf[i] = buf32[2 * i]>>8;
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, dump_buf, 1024);

#endif

    //int32_t s_time,e_time;
    //s_time = hal_fast_sys_timer_get();

#ifdef __SW_IIR_EQ_PROCESS__
    if(audio_process.sw_iir_enable)
    {
        iir_run(buf, pcm_len);
    }
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    if(audio_process.hw_fir_enable)
    {
        fir_run(buf, pcm_len);
    }
#endif

#ifdef AUDIO_PROCESS_DUMP
    for(int i=0;i<1024;i++)
        dump_buf[i] = buf32[2 * i+0]>>8;
    audio_dump_add_channel_data(1, dump_buf, 1024);
#endif

#ifdef __AUDIO_DRC__
#ifdef AUDIO_DRC_UPDATE_CFG
	if(audio_process.drc_update)
	{
        drc_set_config(audio_process.drc_st, &audio_process.drc_cfg);
        audio_process.drc_update =false;
	}
#endif

	drc_process(audio_process.drc_st, buf, pcm_len);
#endif

    //int32_t m_time = hal_fast_sys_timer_get();

#ifdef __AUDIO_LIMITER__
#ifdef AUDIO_LIMITER_UPDATE_CFG
    if(audio_process.limiter_update)
    {
        limiter_set_config(audio_process.limiter_st, &audio_process.limiter_cfg);
        audio_process.limiter_update =false;
    }
#endif

    limiter_process(audio_process.limiter_st, buf, pcm_len);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    if(audio_process.hw_iir_enable)
    {
        hw_iir_run(buf, pcm_len);
    }
#endif
#ifdef AUDIO_PROCESS_DUMP
        //for(int i=0;i<1024;i++)
        //    dump_buf[i] = buf32[2 * i+0]>>8;
        //audio_dump_add_channel_data(1, dump_buf, 1024);
        audio_dump_run();
#endif

    //e_time = hal_fast_sys_timer_get();
    //TRACE(4,"[%s] Sample len = %d, drc1 %d us, limiter %d us",
    //    __func__, pcm_len, FAST_TICKS_TO_US(m_time - s_time), FAST_TICKS_TO_US(e_time - m_time));

    return 0;
}

int audio_process_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits, enum AUD_CHANNEL_NUM_T ch_num, int32_t frame_size, void *eq_buf, uint32_t len)
{
    TRACE(3,"[%s] sample_rate = %d, sample_bits = %d", __func__, sample_rate, sample_bits);
#ifdef AUDIO_PROCESS_DUMP
    audio_dump_init(1024, sizeof(short), 2);
#endif
    audio_process.sample_rate = sample_rate;
    audio_process.sample_bits = sample_bits;
    audio_process.ch_num = ch_num;

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    void *fir_eq_buf = eq_buf;
    uint32_t fir_len = len/2;
    void *iir_eq_buf = (uint8_t *)eq_buf+fir_len;
    uint32_t iir_len = len/2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)
    void *fir_eq_buf = eq_buf;
    uint32_t fir_len = len;
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    void *iir_eq_buf = eq_buf;
    uint32_t iir_len = len;
#endif

#ifdef __SW_IIR_EQ_PROCESS__
    iir_open(sample_rate, sample_bits,ch_num);
#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    audio_eq_set_cfg(NULL, &audio_process.sw_iir_cfg, AUDIO_EQ_TYPE_SW_IIR);
#endif
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
    sample_rate_hw_dac_iir=hal_codec_get_real_sample_rate(audio_process.sample_rate,1);
   TRACE(3,"audio_process.sample_rate:%d, sample_rate_hw_dac_iir: %d.", audio_process.sample_rate, sample_rate_hw_dac_iir);
#else
    sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif

    hw_codec_iir_open(sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC, CODEC_OUTPUT_DEV);
#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    audio_eq_set_cfg(NULL, &audio_process.hw_dac_iir_cfg, AUDIO_EQ_TYPE_HW_DAC_IIR);
#endif
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    hw_iir_open(sample_rate, sample_bits, ch_num, iir_eq_buf, iir_len);
#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    audio_eq_set_cfg(NULL, &audio_process.hw_iir_cfg, AUDIO_EQ_TYPE_HW_IIR);
#endif
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
    hal_cmu_fir_high_speed_enable(HAL_CMU_FIR_USER_EQ);
#endif
    fir_open(sample_rate, sample_bits, ch_num, fir_eq_buf, fir_len);
#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    audio_eq_set_cfg(&audio_process.hw_fir_cfg, NULL, AUDIO_EQ_TYPE_HW_FIR);
#endif
#endif

#if AUDIO_MEMORY_SIZE > 0
    syspool_get_buff(&audio_process.audio_heap, AUDIO_MEMORY_SIZE);
    audio_heap_init(audio_process.audio_heap, AUDIO_MEMORY_SIZE);
#endif

#ifdef __AUDIO_DRC__
    audio_process.drc_st = drc_create(sample_rate, frame_size / ch_num, sample_bits, ch_num,
#ifdef AUDIO_DRC_UPDATE_CFG
        &audio_process.drc_cfg
#else
        &audio_drc_cfg
#endif
        );
#ifdef AUDIO_DRC_UPDATE_CFG
    audio_process.drc_update = false;
#endif
#endif

#ifdef __AUDIO_LIMITER__
    audio_process.limiter_st = limiter_create(sample_rate, frame_size / ch_num, sample_bits, ch_num,
#ifdef AUDIO_LIMITER_UPDATE_CFG
        &audio_process.limiter_cfg
#else
        &audio_limiter_cfg
#endif
        );
#ifdef AUDIO_LIMITER_UPDATE_CFG
    audio_process.limiter_update = false;
#endif
#endif

#ifdef __AUDIO_HW_LIMITER__
    hwlimiter_open(CODEC_OUTPUT_DEV);
#ifdef __AUDIO_RESAMPLE__
    hwlimiter_set_cfg(AUD_SAMPRATE_50781,0);
#else
    hwlimiter_set_cfg(sample_rate,0);
#endif
    hwlimiter_enable(CODEC_OUTPUT_DEV);
#endif

#ifdef AUDIO_CUSTOM_EQ
    audio_process.nfft = 1024;
    syspool_get_buff((uint8_t **)&audio_process.work_buffer, iir_cfg_find_peak_needed_size(IIR_PARAM_NUM, audio_process.nfft));
#endif

#if defined(__PC_CMD_UART__) && defined(USB_AUDIO_APP)
    hal_cmd_open();
#endif

	return 0;
}

int audio_process_close(void)
{
#ifdef __SW_IIR_EQ_PROCESS__
    audio_process.sw_iir_enable = false;
    iir_close();
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    audio_process.hw_dac_iir_enable = false;
    hw_codec_iir_close(HW_CODEC_IIR_DAC);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    audio_process.hw_iir_enable = false;
    hw_iir_close();
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    audio_process.hw_fir_enable = false;
    fir_close();
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
    hal_cmu_fir_high_speed_disable(HAL_CMU_FIR_USER_EQ);
#endif
#endif

#ifdef __AUDIO_DRC__
#ifdef AUDIO_DRC_UPDATE_CFG
    audio_process.drc_update = false;
#endif
    drc_destroy(audio_process.drc_st);
    audio_process.drc_st = NULL;
#endif

#ifdef __AUDIO_LIMITER__
#ifdef AUDIO_LIMITER_UPDATE_CFG
    audio_process.limiter_update = false;
#endif
    limiter_destroy(audio_process.limiter_st);
    audio_process.limiter_st = NULL;
#endif

#if AUDIO_MEMORY_SIZE > 0
    size_t total = 0, used = 0, max_used = 0;
    audio_memory_info(&total, &used, &max_used);
    TRACE(3,"AUDIO MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);
#endif

#ifdef __AUDIO_HW_LIMITER__
    hwlimiter_disable(CODEC_OUTPUT_DEV);
    hwlimiter_close();
#endif

#if defined(__PC_CMD_UART__) && defined(USB_AUDIO_APP)
    hal_cmd_close();
#endif

    return 0;
}

#if defined(AUDIO_CUSTOM_EQ)
/*
 * Current merge strategy:
 * Normally master gain should be the negative of max peak of frequency response, but it is not always true.
 * Sometime we want to reduce the overall output power, sometimes we can accept some distortion.
 * We define **HEADROOM** to be the extra gain applied to master gain, that is
 *
 * headroom = -master_gain - design_peak_dB
 *
 * When we merge design eq with custom eq, we keep the same headroom as design eq.
 */
void audio_eq_merge_custom_eq(IIR_CFG_T *cfg, IIR_CFG_T *custom_cfg, const IIR_CFG_T *design_cfg)
{
    if (custom_cfg->num + design_cfg->num > IIR_PARAM_NUM) {
        TRACE(0, "[%s] custom eq num(%d) exceed max avaliable num(%d)",
            __FUNCTION__, custom_cfg->num, IIR_PARAM_NUM - design_cfg->num);
        return;
    }

    memcpy(cfg, design_cfg, sizeof(IIR_CFG_T));

    if (custom_cfg->num == 0) {
        return;
    }

    for (uint8_t i = 0, j = design_cfg->num; i < custom_cfg->num; i++, j++) {
        memcpy(&cfg->param[j], &custom_cfg->param[i], sizeof(IIR_PARAM_T));
    }

    cfg->num += custom_cfg->num;

    enum AUD_SAMPRATE_T fs = audio_process.sample_rate;
    int32_t nfft = audio_process.nfft;

    float design_peak_idx = 0;
    float design_peak = iir_cfg_find_peak(audio_process.sample_rate, design_cfg, nfft, &design_peak_idx, audio_process.work_buffer);

    float design_headroom = -design_cfg->gain0 - LIN2DB(design_peak);

    TRACE(3, "design idx = %d(e-3), design peak = %d(e-3), f0 = %d(e-3)\n", (int)(design_peak_idx*1000), (int)(design_peak*1000), (int)(design_peak_idx / nfft * fs * 1000));

    float peak_idx = 0;
    float peak = iir_cfg_find_peak(audio_process.sample_rate, cfg, nfft, &peak_idx, audio_process.work_buffer);

    TRACE(3, "idx = %d(e-3), peak = %d(e-3), f0 = %d(e-3)\n", (int)(peak_idx*1000), (int)(peak*1000), (int)(peak_idx / nfft * fs * 1000));

    // update master gain
    float peak_dB = LIN2DB(peak);
    cfg->gain0 = -peak_dB - design_headroom;
    cfg->gain1 = -peak_dB - design_headroom;
}
#endif

#if defined(AUDIO_EQ_TUNING)
int audio_ping_callback(uint8_t *buf, uint32_t len)
{
    //TRACE(0,"");
    return 0;
}

int audio_anc_switch_callback(uint8_t *buf, uint32_t len)
{
    TRACE(0, "[%s] len = %d, sizeof(uint32_t) = %d", __func__, len, sizeof(uint32_t));

    if (len != sizeof(uint32_t)) {
        return 1;
    }

#if defined(ANC_APP)
    uint32_t mode = *((uint32_t *)buf);

    if (mode < APP_ANC_MODE_QTY) {
        TRACE(0, "[%s] mode: %d", __func__, mode);
        app_anc_switch(mode);
    } else {
        TRACE(0, "[%s] WARNING: mode(%d) >= APP_ANC_MODE_QTY", __func__, mode);
    }
#else
    TRACE(0, "[%s] WARNING: ANC_APP is disabled", __func__);
#endif

    return 0;
}

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) && !defined(USB_EQ_TUNING)
#ifndef USB_AUDIO_APP
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

    memcpy(&audio_process.sw_iir_cfg, buf, sizeof(IIR_CFG_T));
    TRACE(3,"band num:%d gain0:%d, gain1:%d",
                                                (int32_t)audio_process.sw_iir_cfg.num,
                                                (int32_t)(audio_process.sw_iir_cfg.gain0*10),
                                                (int32_t)(audio_process.sw_iir_cfg.gain1*10));
    for (uint8_t i = 0; i<audio_process.sw_iir_cfg.num; i++){
        TRACE(5,"band num:%d type %d gain:%d fc:%d q:%d", i,
			                           (int32_t)(audio_process.sw_iir_cfg.param[i].type),
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].gain*10),
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].fc*10),
                                                (int32_t)(audio_process.sw_iir_cfg.param[i].Q*10));
    }

#ifdef __SW_IIR_EQ_PROCESS__
    {
        iir_set_cfg(&audio_process.sw_iir_cfg);
        audio_process.sw_iir_enable = true;
    }
#endif

    return 0;
}
#else
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t  len)
{
    // IIR_CFG_T *rx_iir_cfg = NULL;

    // rx_iir_cfg = (IIR_CFG_T *)buf;

    // TRACE(3,"[%s] left gain = %f, right gain = %f", __func__, rx_iir_cfg->gain0, rx_iir_cfg->gain1);

    // for(int i=0; i<rx_iir_cfg->num; i++)
    // {
    //     TRACE(5,"[%s] iir[%d] gain = %f, f = %f, Q = %f", __func__, i, rx_iir_cfg->param[i].gain, rx_iir_cfg->param[i].fc, rx_iir_cfg->param[i].Q);
    // }

    // audio_eq_set_cfg(NULL,(const IIR_CFG_T *)rx_iir_cfg,AUDIO_EQ_TYPE_SW_IIR);

    iir_update_cfg_tbl(buf, len);

    return 0;
}
#endif
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
int audio_eq_hw_fir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(FIR_CFG_T));

    if (len != sizeof(FIR_CFG_T))
    {
        return 1;
    }

    FIR_CFG_T *rx_fir_cfg = NULL;

    rx_fir_cfg = (FIR_CFG_T *)buf;

    TRACE(3,"[%s] left gain = %d, right gain = %d", __func__, rx_fir_cfg->gain0, rx_fir_cfg->gain1);

    TRACE(6,"[%s] len = %d, coef: %d, %d......%d, %d", __func__, rx_fir_cfg->len, rx_fir_cfg->coef[0], rx_fir_cfg->coef[1], rx_fir_cfg->coef[rx_fir_cfg->len-2], rx_fir_cfg->coef[rx_fir_cfg->len-1]);

    rx_fir_cfg->gain0 = 6;
    rx_fir_cfg->gain1 = 6;

    if(rx_fir_cfg)
    {
        memcpy(&audio_process.fir_cfg, rx_fir_cfg, sizeof(audio_process.fir_cfg));
        audio_process.fir_enable = true;
        fir_set_cfg(&audio_process.fir_cfg);
    }
    else
    {
        audio_process.fir_enable = false;
    }

    return 0;
}
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
int audio_drc_callback(uint8_t *buf, uint32_t  len)
{
	TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(DrcConfig));

    if (len != sizeof(DrcConfig))
    {
        TRACE(1,"[%s] WARNING: Length is different", __func__);

        return 1;
    }

    if (audio_process.drc_st == NULL)
    {
        TRACE(1,"[%s] WARNING: audio_process.limiter_st = NULL", __func__);
        TRACE(1,"[%s] WARNING: Please Play music, then tuning Limiter", __func__);

        return 2;
    }

	memcpy(&audio_process.drc_cfg, buf, sizeof(DrcConfig));
    audio_process.drc_update = true;

    return 0;
}
#endif

#ifdef AUDIO_LIMITER_UPDATE_CFG
int audio_limiter_callback(uint8_t *buf, uint32_t  len)
{
	TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(LimiterConfig));

    if (len != sizeof(LimiterConfig))
    {
        TRACE(1,"[%s] WARNING: Length is different", __func__);

        return 1;
    }

    if (audio_process.limiter_st == NULL)
    {
        TRACE(1,"[%s] WARNING: audio_process.limiter_st = NULL", __func__);
        TRACE(1,"[%s] WARNING: Please Play music, then tuning Limiter", __func__);

        return 2;
    }

	memcpy(&audio_process.limiter_cfg, buf, sizeof(LimiterConfig));
    audio_process.limiter_update = true;

    return 0;
}
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG) && !defined(USB_EQ_TUNING)
int audio_eq_hw_dac_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

#if defined(TEST_AUDIO_CUSTOM_EQ)
    audio_eq_merge_custom_eq(&audio_process.hw_dac_iir_cfg, (IIR_CFG_T *)buf, audio_eq_hw_dac_iir_cfg_list[0]);
#else
    memcpy(&audio_process.hw_dac_iir_cfg, buf, sizeof(IIR_CFG_T));
#endif

    TRACE(0, "num: %d, gain0: %d, gain1: %d",
                                                (int32_t)audio_process.hw_dac_iir_cfg.num,
                                                (int32_t)(audio_process.hw_dac_iir_cfg.gain0*10),
                                                (int32_t)(audio_process.hw_dac_iir_cfg.gain1*10));
    for (uint8_t i = 0; i<audio_process.hw_dac_iir_cfg.num; i++){
        TRACE(0, "[%d] type: %d, gain: %d, fc: %d, q: %d", i,
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].type),
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].gain*10),
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].fc*10),
                                                (int32_t)(audio_process.hw_dac_iir_cfg.param[i].Q*10));
    }

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    {
        HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL;
        enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
        sample_rate_hw_dac_iir=hal_codec_get_real_sample_rate(audio_process.sample_rate,1);
        TRACE(3,"audio_process.sample_rate:%d, sample_rate_hw_dac_iir: %d.", audio_process.sample_rate, sample_rate_hw_dac_iir);
#else
        sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif
        hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,&audio_process.hw_dac_iir_cfg);
        ASSERT(hw_iir_cfg_dac != NULL, "[%s] %d codec IIR parameter error!", __func__, (uint32_t)hw_iir_cfg_dac);

        // hal_codec_iir_dump(hw_iir_cfg_dac);

        hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC);
        audio_process.hw_dac_iir_enable = true;

    }
#endif

    return 0;
}
#endif


#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
int audio_eq_hw_iir_callback(uint8_t *buf, uint32_t  len)
{
    TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof(IIR_CFG_T));

    if (len != sizeof(IIR_CFG_T))
    {
        return 1;
    }

    memcpy(&audio_process.hw_iir_cfg, buf, sizeof(IIR_CFG_T));
    TRACE(3,"band num:%d gain0:%d, gain1:%d",
                                                (int32_t)audio_process.hw_iir_cfg.num,
                                                (int32_t)(audio_process.hw_iir_cfg.gain0*10),
                                                (int32_t)(audio_process.hw_iir_cfg.gain1*10));

    for (uint8_t i = 0; i<audio_process.hw_iir_cfg.num; i++)
    {
        TRACE(5,"band num:%d type %d gain:%d fc:%d q:%d", i,
			                                    (int32_t)(audio_process.hw_iir_cfg.param[i].type),
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].gain*10),
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].fc*10),
                                                (int32_t)(audio_process.hw_iir_cfg.param[i].Q*10));
    }

#ifdef __HW_IIR_EQ_PROCESS__
    {
        HW_IIR_CFG_T *hw_iir_cfg=NULL;
#ifdef __AUDIO_RESAMPLE__
        enum AUD_SAMPRATE_T sample_rate_hw_iir=AUD_SAMPRATE_50781;
#else
        enum AUD_SAMPRATE_T sample_rate_hw_iir=audio_process.sample_rate;
#endif
        hw_iir_cfg = hw_iir_get_cfg(sample_rate_hw_iir,&audio_process.hw_iir_cfg);
        ASSERT(hw_iir_cfg != NULL, "[%s] %d codec IIR parameter error!", __func__, hw_iir_cfg);
        hw_iir_set_cfg(hw_iir_cfg);
        audio_process.hw_iir_enable = true;
    }
#endif

    return 0;
}
#endif
#endif  // #if defined(AUDIO_EQ_TUNING)

#ifdef USB_EQ_TUNING

int audio_eq_usb_iir_callback(uint8_t *buf, uint32_t  len)
{
	IIR_CFG_T* cur_cfg;

	TRACE(2,"usb_iir_cb: len=[%d - %d]", len, sizeof(IIR_CFG_T));

	if (len != sizeof(IIR_CFG_T)) {
        return 1;
    }

	cur_cfg = (IIR_CFG_T*)buf;
	TRACE(2,"-> sample_rate[%d], num[%d]", /*cur_cfg->samplerate,*/ audio_process.sample_rate, cur_cfg->num);

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
	audio_process.sw_iir_cfg.gain0 = cur_cfg->gain0;
	audio_process.sw_iir_cfg.gain1 = cur_cfg->gain1;
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
	audio_process.hw_dac_iir_cfg.gain0 = 0;
	audio_process.hw_dac_iir_cfg.gain1 = 0;
#else
    audio_process.hw_dac_iir_cfg.gain0 = cur_cfg->gain0;
	audio_process.hw_dac_iir_cfg.gain1 = cur_cfg->gain1;
#endif
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
	if (cur_cfg->num > AUD_DAC_IIR_NUM_EQ) {
		audio_process.hw_dac_iir_cfg.num = AUD_DAC_IIR_NUM_EQ;
	} else {
		audio_process.hw_dac_iir_cfg.num = cur_cfg->num;
	}
    TRACE(1,"-> hw_dac_iir_num[%d]", audio_process.hw_dac_iir_cfg.num);
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
	audio_process.sw_iir_cfg.num = cur_cfg->num - audio_process.hw_dac_iir_cfg.num;
    TRACE(1,"-> sw_iir_num[%d]", audio_process.sw_iir_cfg.num);
#endif
	//TRACE(2,"-> iir_num[%d - %d]", audio_process.hw_dac_iir_cfg.num, audio_process.sw_iir_cfg.num);

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
	if (audio_process.hw_dac_iir_cfg.num) {
		memcpy((void*)(&audio_process.hw_dac_iir_cfg.param[0]),
					(void*)(&cur_cfg->param[0]),
					audio_process.hw_dac_iir_cfg.num*sizeof(IIR_PARAM_T));
	}
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
	if (audio_process.sw_iir_cfg.num) {

		memcpy((void*)(&audio_process.sw_iir_cfg.param[0]),
					(void*)(&cur_cfg->param[audio_process.hw_dac_iir_cfg.num]),
					audio_process.sw_iir_cfg.num * sizeof(IIR_PARAM_T));

	} else {

		// set a default filter
		audio_process.sw_iir_cfg.num = 1;

		audio_process.sw_iir_cfg.param[0].fc = 1000.0;
		audio_process.sw_iir_cfg.param[0].gain = 0.0;
		audio_process.sw_iir_cfg.param[0].type = IIR_TYPE_PEAK;
		audio_process.sw_iir_cfg.param[0].Q = 1.0;

	}
#endif

	if (audio_process.sample_rate) {
		audio_process.update_cfg = true;
	}

	audio_process.eq_updated_cfg = true;

	return 0;
}

void audio_eq_usb_eq_update (void)
{
	if (audio_process.update_cfg) {

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
		HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL;

		if (audio_process.hw_dac_iir_cfg.num) {
                    enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
                    sample_rate_hw_dac_iir=hal_codec_get_real_sample_rate(audio_process.sample_rate,1);
                    TRACE(3,"audio_process.sample_rate:%d, sample_rate_hw_dac_iir: %d.", audio_process.sample_rate, sample_rate_hw_dac_iir);
#else
                    sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif
			hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir, &audio_process.hw_dac_iir_cfg);

			hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC);
			audio_process.hw_dac_iir_enable = true;

		} else {

			audio_process.hw_dac_iir_enable = false;
		}
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
		iir_set_cfg(&audio_process.sw_iir_cfg);
		audio_process.sw_iir_enable = true;
#endif

		audio_process.update_cfg = false;

/*
		TRACE(4,"USB EQ Update: en[%d-%d], num[%d-%d]",
				audio_process.hw_dac_iir_enable, audio_process.sw_iir_enable,
				audio_process.hw_dac_iir_cfg.num, audio_process.sw_iir_cfg.num);
*/
	}

}

#endif	// USB_EQ_TUNING


typedef struct {
    uint8_t type;
    // EQ
    uint8_t max_eq_band_num;
    uint16_t sample_rate_num;
    uint8_t sample_rate[20];
    // ANC
    uint32_t anc_mode;
    
    // Add new items in order for compatibility
} query_eq_info_t;


extern int getMaxEqBand(void);
extern int getSampleArray(uint8_t* buf, uint16_t *num);

extern void hal_cmd_set_res_playload(uint8_t* data, int len);
#define CMD_TYPE_QUERY_DUT_EQ_INFO  0x00
int audio_cmd_callback(uint8_t *buf, uint32_t len)
{
    uint8_t type;
    query_eq_info_t  info;

    type = buf[0];

    TRACE(2,"%s type: %d", __func__, type);
    switch (type) {
        case CMD_TYPE_QUERY_DUT_EQ_INFO:
            info.type = CMD_TYPE_QUERY_DUT_EQ_INFO;

            // EQ
            info.max_eq_band_num = getMaxEqBand();
            getSampleArray(info.sample_rate, &info.sample_rate_num);

            // ANC
#if defined(ANC_APP)
            info.anc_mode = app_anc_get_curr_mode();
#else
            info.anc_mode = 0;
#endif

            hal_cmd_set_res_playload((uint8_t*)&info, sizeof(info));
            break;
        default:
            break;
    }

    return 0;
}

#ifdef AUDIO_SECTION_ENABLE
int audio_cfg_burn_callback(uint8_t *buf, uint32_t  len)
{
    TRACE(3,"[%s] len = %d, sizeof(struct) = %d", __func__, len, sizeof_audio_cfg());

    if (len != sizeof_audio_cfg())
    {
        return 1;
    }

    int res = 0;
    res = store_audio_cfg_into_audio_section((AUDIO_CFG_T *)buf);

    if(res)
    {
        TRACE(2,"[%s] ERROR: res = %d", __func__, res);
        res += 100;
    }
    else
    {
        TRACE(1,"[%s] Store audio cfg into audio section!!!", __func__);
    }

    return res;
}
#endif

#ifdef AUDIO_HEARING_COMPSATN

uint8_t hear_cfg_update_flg = 1;
void audio_get_eq_para(int *para_buf, uint8_t cfg_flag)
{
    if(1 == cfg_flag)
        hear_cfg_update_flg = 1;
    else
        hear_cfg_update_flg = 0;

    int *band_compen = (int *)para_buf;
    int iir_num = 6;
    float gain_buf[6] = {0.0F};
#if HWFIR_MOD==HEARING_MOD_VAL
    get_hear_compen_val_fir(band_compen,gain_buf,iir_num);
#else
    get_hear_compen_val_multi_level(band_compen,gain_buf,iir_num);
    iir_num -= 1;
#endif
    for(int i=0;i<iir_num;i++)
        TRACE(1,"gain_buf[%d]=%de-2",i,(int)(gain_buf[i]*100));

#if SWIIR_MOD==HEARING_MOD_VAL || HWIIR_MOD==HEARING_MOD_VAL
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
    float fc_buf[6] = {600.0, 1200.0, 2400.0, 4800.0, 8000.0, 8000.0};
#if defined(__SW_IIR_EQ_PROCESS__)
    const IIR_CFG_T *iir_default_cfg= audio_eq_sw_iir_cfg_list[0];
#else
    const IIR_CFG_T *iir_default_cfg= audio_eq_hw_dac_iir_cfg_list[0];
#endif
    int default_iir_num = iir_default_cfg->num;
    float default_iir_gain0 = iir_default_cfg->gain0;
    float default_iir_gain1 = iir_default_cfg->gain1;
    float gain_tmp0 = default_iir_gain0;
    float gain_tmp1 = default_iir_gain1;
    TRACE(3,"default IIR band num:%d gain0:%d, gain1:%d",
                                    default_iir_num,
                                    (int32_t)default_iir_gain0,
                                    (int32_t)default_iir_gain1);
    audio_process.hear_iir_cfg.num = default_iir_num + iir_num;
    audio_process.hear_iir_cfg.gain0 = gain_tmp0;
    audio_process.hear_iir_cfg.gain1 = gain_tmp1;

    int band_idx = 0 ;
    int iir_coef_idx = 0;
    for(int i=default_iir_num;i<audio_process.hear_iir_cfg.num;i++)
    {
        if(default_iir_num==i)
            audio_process.hear_iir_cfg.param[i].type = IIR_TYPE_LOW_SHELF;
        else if((default_iir_num+iir_num-1)==i)
            audio_process.hear_iir_cfg.param[i].type = IIR_TYPE_HIGH_SHELF;
        else
            audio_process.hear_iir_cfg.param[i].type = IIR_TYPE_PEAK;

        if(default_iir_num==i)
            audio_process.hear_iir_cfg.param[i].Q = 0.707;
        else if((default_iir_num+iir_num-1)==i)
            audio_process.hear_iir_cfg.param[i].Q = 0.707;
        else
            audio_process.hear_iir_cfg.param[i].Q = 1.2;

        audio_process.hear_iir_cfg.param[i].gain = gain_buf[band_idx];
        audio_process.hear_iir_cfg.param[i].fc = fc_buf[band_idx];

        get_iir_coef(&audio_process.hear_iir_cfg, i, 44100, &hear_iir_coef[iir_coef_idx]);
        iir_coef_idx += 6;
        band_idx++;
    }

    TRACE(3,"band num:%d gain0:%d, gain1:%d",
                                            (int32_t)audio_process.hear_iir_cfg.num,
                                            (int32_t)(audio_process.hear_iir_cfg.gain0),
                                            (int32_t)(audio_process.hear_iir_cfg.gain1));
    for (uint8_t i = default_iir_num; i<audio_process.hear_iir_cfg.num; i++)
    {
        TRACE(5,"band num:%d type %d gain:%de-3 fc:%d q:%de-3", i,
                                            (int32_t)(audio_process.hear_iir_cfg.param[i].type),
                                            (int32_t)(audio_process.hear_iir_cfg.param[i].gain*1000),
                                            (int32_t)(audio_process.hear_iir_cfg.param[i].fc),
                                            (int32_t)(audio_process.hear_iir_cfg.param[i].Q*1000));
    }

    for(int i=0;i<6;i++)
        TRACE(1,"hear_iir_coef[%d]=%de-3",i,(int)(hear_iir_coef[i]*1000));

    TRACE(1,"hear_level=%d",get_hear_level());

    if(1 == hear_cfg_update_flg)
    {
        memcpy(&audio_process.hear_iir_cfg_update, &audio_process.hear_iir_cfg, sizeof(IIR_CFG_T));

        TRACE(3,"update band num:%d gain0:%d, gain1:%d",
                                                (int32_t)audio_process.hear_iir_cfg_update.num,
                                                (int32_t)(audio_process.hear_iir_cfg_update.gain0),
                                                (int32_t)(audio_process.hear_iir_cfg_update.gain1));
        for (uint8_t i = default_iir_num; i<audio_process.hear_iir_cfg_update.num; i++)
        {
            TRACE(5,"update band num:%d type %d gain:%de-3 fc:%d q:%de-3", i,
                                                (int32_t)(audio_process.hear_iir_cfg_update.param[i].type),
                                                (int32_t)(audio_process.hear_iir_cfg_update.param[i].gain*1000),
                                                (int32_t)(audio_process.hear_iir_cfg_update.param[i].fc),
                                                (int32_t)(audio_process.hear_iir_cfg_update.param[i].Q*1000));
        }
    }

#endif
#elif HWFIR_MOD==HEARING_MOD_VAL
#if defined(__HW_FIR_EQ_PROCESS__)
    float fir_scale_db = 0.0;
    //For large gain in low freq
    float gain_tmp = -22;
    TRACE(2,"[%s],gain_tmp0=%d",__func__,(int)(gain_tmp));
    for(int i=0;i<iir_num;i++)
        TRACE(2,"gain_buf[%d]=%de-2",i,(int)(gain_buf[i]*100));

    //Get buffer from system pool
#ifdef HEARING_USE_STATIC_RAM
    speech_heap_init(HEAR_DET_STREAM_BUF, HEAR_COMP_BUF_SIZE);
#else
    if(NULL==hear_comp_buf)
        syspool_get_buff(&hear_comp_buf, HEAR_COMP_BUF_SIZE);
    speech_heap_init(hear_comp_buf, HEAR_COMP_BUF_SIZE);
#endif

    HearFir2State* hear_st = HearFir2_create(44100);
    fir_scale_db = HearFir2_process(hear_st, fir_filter,gain_buf);
    TRACE(1,"fir_scale = %de-2",(int)(fir_scale_db*100));
    HearFir2_destroy(hear_st);
    //Free buffer
    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(3,"HEAP INFO: total=%d, used=%d, max_used=%d", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);

    if(1 == hear_cfg_update_flg)
    {
        float fir_gain_tmp = gain_tmp + fir_scale_db;
        if(0 < fir_gain_tmp)
            fir_gain_tmp = 0;
        audio_process.hear_hw_fir_cfg.gain  = fir_gain_tmp;
        audio_process.hear_hw_fir_cfg.len   = 384;

        for(int i=0;i<384;i++)
        {
            audio_process.hear_hw_fir_cfg.coef[i] = (int)(fir_filter[i]*(1<<23));
        }
    }
    float fir_scale_val = hear_val2db(fir_scale_db);
    TRACE(1,"fir_scale_val = %de-2",(int)(fir_scale_val*100));
    for(int i=0;i<384;i++)
    {
        fir_filter[i] = fir_filter[i] * fir_scale_val;
    }

#endif
#endif
}

#endif

int audio_process_init(void)
{
#if defined(AUDIO_EQ_TUNING)
#if defined(USB_EQ_TUNING) || defined(__PC_CMD_UART__)
    hal_cmd_init();
#endif

#if defined(USB_EQ_TUNING)
#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) || defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
    hal_cmd_register("iir_eq", audio_eq_usb_iir_callback);
#endif

#else   // #if defined(USB_EQ_TUNING)
#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    hal_cmd_register("sw_iir_eq", audio_eq_sw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    hal_cmd_register("dac_iir_eq", audio_eq_hw_dac_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    hal_cmd_register("hw_iir_eq", audio_eq_hw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    hal_cmd_register("fir_eq", audio_eq_hw_fir_callback);
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    hal_cmd_register("drc", audio_drc_callback);
#endif

#ifdef AUDIO_LIMITER_UPDATE_CFG
    hal_cmd_register("limiter", audio_limiter_callback);
#endif

#ifdef AUDIO_SECTION_ENABLE
    hal_cmd_register("burn", audio_cfg_burn_callback);
    hal_cmd_register("audio_burn", audio_cfg_burn_callback);
#endif
#endif  // #if defined(USB_EQ_TUNING)

    hal_cmd_register("anc_switch", audio_anc_switch_callback);

    hal_cmd_register("cmd", audio_cmd_callback);
    hal_cmd_register("ping", audio_ping_callback);
#endif  // #if defined(AUDIO_EQ_TUNING)

#ifdef AUDIO_HEARING_COMPSATN
#if SWIIR_MOD==HEARING_MOD_VAL
#if defined(__SW_IIR_EQ_PROCESS__)
    memcpy(&audio_process.hear_iir_cfg, audio_eq_sw_iir_cfg_list[0], sizeof(IIR_CFG_T));
    memcpy(&audio_process.hear_iir_cfg_update, audio_eq_sw_iir_cfg_list[0], sizeof(IIR_CFG_T));
#endif
#elif HWFIR_MOD==HEARING_MOD_VAL
#if defined(__HW_FIR_EQ_PROCESS__)
    memcpy(&audio_process.hear_hw_fir_cfg, audio_eq_hw_fir_cfg_list[0], sizeof(FIR_CFG_T));
#endif
#elif HWIIR_MOD==HEARING_MOD_VAL
#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    memcpy(&audio_process.hear_iir_cfg, audio_eq_hw_dac_iir_cfg_list[0], sizeof(IIR_CFG_T));
    memcpy(&audio_process.hear_iir_cfg_update, audio_eq_hw_dac_iir_cfg_list[0], sizeof(IIR_CFG_T));
#endif
#endif
#endif

    // load default cfg
#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    memcpy(&audio_process.sw_iir_cfg, audio_eq_sw_iir_cfg_list[0], sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    memcpy(&audio_process.hw_dac_iir_cfg, audio_eq_hw_dac_iir_cfg_list[0], sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    memcpy(&audio_process.hw_iir_cfg, audio_eq_hw_iir_cfg_list[0], sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    memcpy(&audio_process.hw_fir_cfg, audio_eq_hw_fir_cfg_list[0], sizeof(FIR_CFG_T));
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    memcpy(&audio_process.drc_cfg, &audio_drc_cfg, sizeof(DrcConfig));
#endif

#ifdef AUDIO_LIMITER_UPDATE_CFG
    memcpy(&audio_process.limiter_cfg, &audio_limiter_cfg, sizeof(LimiterConfig));
#endif

    return 0;
}
