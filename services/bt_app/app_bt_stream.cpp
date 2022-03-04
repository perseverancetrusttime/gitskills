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
//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "app_bt_trace.h"
#include "tgt_hardware.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_overlay.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#if defined(__XSPACE_UI__)
#include "xspace_ui.h"
#endif

#ifdef ANC_APP
#include "app_anc.h"
#endif
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "resample_coef.h"
#include "hal_codec.h"
#include "hal_i2s.h"
#include "hal_dma.h"
#include "hal_bootmode.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory_audio.h"
#endif
#ifdef TX_RX_PCM_MASK
#include "hal_chipid.h"
#endif

#ifdef  __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_voice.h"
#endif

#ifdef AI_AEC_CP_ACCEL
#include "app_ai_algorithm.h"
#endif

#include "app_ring_merge.h"
#include "bt_drv.h"
#include "bt_xtal_sync.h"
#include "bt_drv_reg_op.h"
#include "besbt.h"
#include "hal_chipid.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_hfp.h"
#include "app_bt.h"
#include "os_api.h"
#include "audio_process.h"
#include "voice_dev.h"
#include "app_a2dp.h"
#include "app_media_player.h"

#if defined(BT_SOURCE)
#include "bt_source.h"
#endif
#include "audio_dump.h"
#include "a2dp_decoder.h"
#if defined(__AUDIO_SPECTRUM__)
#include "audio_spectrum.h"
#endif

#if defined(SPEECH_BONE_SENSOR)
#include "speech_bone_sensor.h"
#endif

#if defined(ANC_NOISE_TRACKER)
#include "noise_tracker.h"
#include "noise_tracker_callback.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_tws_ibrt_audio_sync.h"
#include "app_ibrt_a2dp.h"
#include "app_ibrt_rssi.h"
#undef MUSIC_DELAY_CONTROL
#endif

#if defined(IBRT_CORE_V2_ENABLE)
#include "app_tws_ibrt_conn_api.h"
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#if defined(BLE_V2)
#include "app_ble_param_config.h"
#endif
#endif

#if defined(AUDIO_ANC_FB_ADJ_MC)
#include "adj_mc.h"
#include "fftfilt2.h"
#endif

#if defined(ANC_ASSIST_ENABLED)
#include "app_anc_assist.h"
#endif

#if defined(AUDIO_PCM_PLAYER)
#include "app_stream_pcm_player.h"
#endif
#ifdef APP_MIC_CAPTURE_LOOPBACK
#include "sealing_check_config.h"
#endif

#ifdef IS_MULTI_AI_ENABLED
#include "app_ai_if.h"
#endif

// #define A2DP_STREAM_AUDIO_DUMP

#if defined(A2DP_STREAM_AUDIO_DUMP)
static uint32_t g_a2dp_pcm_dump_frame_len = 0;
static uint32_t g_a2dp_pcm_dump_channel_num = 0;
static uint32_t g_a2dp_pcm_dump_sample_bytes = 0;
#endif

#if defined(__SW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_sw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_sw_iir_cfg_list[];
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_fir_index = 0;
extern const FIR_CFG_T * const audio_eq_hw_fir_cfg_list[];
#if defined(__HW_FIR_EQ_PROCESS_2CH__)
extern const FIR_CFG_T * const audio_hw_hpfir_cfg_list[];
extern const FIR_CFG_T * const audio_hw_lpfir_cfg_list[];
#endif
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_dac_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_dac_iir_cfg_list[];
#endif

#include "audio_prompt_sbc.h"

#if defined(__HW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_iir_cfg_list[];
#endif

#if defined(HW_DC_FILTER_WITH_IIR)
#include "hw_filter_codec_iir.h"
#include "hw_codec_iir_process.h"

hw_filter_codec_iir_cfg POSSIBLY_UNUSED adc_iir_cfg = {
    .bypass = 0,
    .iir_device = HW_CODEC_IIR_ADC,
#if 1
    .iir_cfg = {
        .iir_filtes_l = {
            .iir_bypass_flag = 0,
            .iir_counter = 2,
            .iir_coef = {
                    {{0.994406, -1.988812, 0.994406}, {1.000000, -1.988781, 0.988843}}, // iir_designer('highpass', 0, 20, 0.7, 16000);
                    {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
            }
        },
        .iir_filtes_r = {
            .iir_bypass_flag = 0,
            .iir_counter = 2,
            .iir_coef = {
                    {{0.994406, -1.988812, 0.994406}, {1.000000, -1.988781, 0.988843}},
                    {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
            }
        }
    }
#else
    .iir_cfg = {
        .gain0 = 0,
        .gain1 = 0,
        .num = 1,
        .param = {
            {IIR_TYPE_HIGH_PASS, 0,   20.0,   0.7},
        }
    }
#endif
};

hw_filter_codec_iir_state *hw_filter_codec_iir_st;
#endif

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST3003)|| \
     defined(CHIP_BEST1400) || defined(CHIP_BEST1402)   || defined(CHIP_BEST1000) || \
     defined(CHIP_BEST2000) || defined(CHIP_BEST3001)   || defined(CHIP_BEST2001) \

#undef AUDIO_RESAMPLE_ANTI_DITHER

#else
#define  AUDIO_RESAMPLE_ANTI_DITHER
#endif

#include "audio_cfg.h"

//#define SCO_DMA_SNAPSHOT_DEBUG
#define SCO_TUNING_NEWMETHOD

extern uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status);
extern uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index);
extern uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status);

#include "app_bt_media_manager.h"

#include "string.h"
#include "hal_location.h"

#include "bt_drv_interface.h"

#include "audio_resample_ex.h"

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0x1FF)
#else
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0xFF)
#endif
#define BT_INIT_XTAL_SYNC_MIN (20)
#define BT_INIT_XTAL_SYNC_MAX (BT_INIT_XTAL_SYNC_FCAP_RANGE - BT_INIT_XTAL_SYNC_MIN)

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#include"anc_process.h"

#ifdef ANC_FB_MC_96KHZ
#define DELAY_SAMPLE_MC (29*2)     //  2:ch
#define SAMPLERATE_RATIO_THRESHOLD (4) //384 = 96*4
#else
#define DELAY_SAMPLE_MC (31*2)     //  2:ch
#define SAMPLERATE_RATIO_THRESHOLD (8) //384 = 48*8
#endif

static int32_t delay_buf_bt[DELAY_SAMPLE_MC];
#endif

#ifdef ANC_APP
static uint8_t anc_status_record = 0xff;
#endif

#if defined(SCO_DMA_SNAPSHOT)

#if defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
#define BTCLK_UNIT (312.5f) //us
#define BTCNT_UNIT (0.5f) //us
#define MAX_BT_CLOCK   ((1L<<28) - 1)
#else
#define BTCLK_UNIT (625.0f) //us
#define BTCNT_UNIT (1.0f) //us
#define MAX_BT_CLOCK   ((1L<<27) - 1)
#endif

#define CODEC_TRIG_DELAY 2500.0f //us

#ifdef PCM_FAST_MODE
#define CODEC_TRIG_BTCLK_OFFSET ((uint32_t)(CODEC_TRIG_DELAY/BTCLK_UNIT))
#elif TX_RX_PCM_MASK
#define CODEC_TRIG_BTCLK_OFFSET (2*(uint32_t)(CODEC_TRIG_DELAY/BTCLK_UNIT))
#else
#define CODEC_TRIG_BTCLK_OFFSET ((uint32_t)(CODEC_TRIG_DELAY/BTCLK_UNIT))
#endif

#if defined(LOW_DELAY_SCO)
#define CODEC_BTPCM_BUF 7500.0f //us
#else
#define CODEC_BTPCM_BUF 15000.0f //us
#endif

#define BUF_BTCLK_NUM ((uint32_t)((CODEC_BTPCM_BUF/BTCLK_UNIT)))

#define WRAPED_CLK_OFFSET  (((MAX_BT_CLOCK/BUF_BTCLK_NUM+1)*BUF_BTCLK_NUM)&MAX_BT_CLOCK)

#define CODEC_TRIG_DELAY_OFFSET (CODEC_TRIG_BTCLK_OFFSET*BTCLK_UNIT)

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) || defined(CHIP_BEST2001)|| defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
#define MUTE_PATTERN (0x55)
#else
#define MUTE_PATTERN (0x00)
#endif

#define A2DP_PLAYER_PLAYBACK_WATER_LINE ((uint32_t)(3.f * a2dp_audio_latency_factor_get() + 0.5f))
#define A2DP_PLAYER_PLAYBACK_WATER_LINE_UPPER (25)

extern void  app_tws_ibrt_audio_mobile_clkcnt_get(uint8_t device_id, uint32_t btclk, uint16_t btcnt,
                                                     uint32_t *mobile_master_clk, uint16_t *mobile_master_cnt);

static uint8_t *playback_buf_codecpcm;
static uint32_t playback_size_codecpcm;
static uint8_t *capture_buf_codecpcm;
static uint32_t capture_size_codecpcm;

static uint8_t *playback_buf_btpcm;
static uint32_t playback_size_btpcm;
static uint8_t *capture_buf_btpcm;
static uint32_t capture_size_btpcm;

#ifdef TX_RX_PCM_MASK
static uint8_t *playback_buf_btpcm_copy=NULL;
static uint32_t playback_size_btpcm_copy=0;
static uint8_t *capture_buf_btpcm_copy=NULL;
static uint32_t capture_size_btpcm_copy=0;
#endif

volatile int sco_btpcm_mute_flag=0;
volatile int sco_disconnect_mute_flag=0;

static uint8_t *playback_buf_btpcm_cache=NULL;

static enum AUD_SAMPRATE_T playback_samplerate_codecpcm;
static int32_t mobile_master_clk_offset_init;
static uint32_t last_mobile_master_clk=0;
#endif

#if defined(TX_RX_PCM_MASK) || defined(PCM_PRIVATE_DATA_FLAG)
struct PCM_DATA_FLAG_T pcm_data_param[4];
#define PCM_PRIVATE_DATA_LENGTH 11
#define BTPCM_PRIVATE_DATA_LENGTH 32
#define BTPCM_PUBLIC_DATA_LENGTH 120
#define BTPCM_TOTAL_DATA_LENGTH (BTPCM_PRIVATE_DATA_LENGTH+BTPCM_PUBLIC_DATA_LENGTH)
#endif

enum PLAYER_OPER_T
{
    PLAYER_OPER_START,
    PLAYER_OPER_STOP,
    PLAYER_OPER_RESTART,
};

#if defined(AF_ADC_I2S_SYNC)
extern "C" void hal_codec_capture_enable(void);
extern "C" void hal_codec_capture_enable_delay(void);
#endif
#if !defined(__XSPACE_UI__)
#if (AUDIO_OUTPUT_VOLUME_DEFAULT < 1) || (AUDIO_OUTPUT_VOLUME_DEFAULT > 17)
#error "AUDIO_OUTPUT_VOLUME_DEFAULT out of range"
#endif
#endif
int8_t stream_local_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#ifdef AUDIO_LINEIN
int8_t stream_linein_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#endif

struct btdevice_volume *btdevice_volume_p;
struct btdevice_volume current_btdevice_volume;

#ifdef __BT_ANC__
static uint32_t bt_sco_upsampling_ratio = 1;
static uint8_t *bt_anc_sco_dec_buf;
extern void us_fir_init(uint32_t upsampling_ratio);
extern uint32_t voicebtpcm_pcm_resample (short *src_samp_buf, uint32_t src_smpl_cnt, short *dst_samp_buf);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static enum AUD_BITS_T sample_size_play_bt;
static enum AUD_SAMPRATE_T sample_rate_play_bt;
static uint32_t data_size_play_bt;

static uint8_t *playback_buf_bt;
static uint32_t playback_size_bt;
static int32_t playback_samplerate_ratio_bt;

static uint8_t *playback_buf_mc;
static uint32_t playback_size_mc;
static enum AUD_CHANNEL_NUM_T  playback_ch_num_bt;
#ifdef AUDIO_ANC_FB_ADJ_MC
uint32_t adj_mc_capture_sample_rate;
#endif
#endif

#if defined(LOW_DELAY_SCO)
static int32_t  btpcm_int_counter;
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
static enum AUD_BITS_T lowdelay_sample_size_play_bt;
static enum AUD_SAMPRATE_T lowdelay_sample_rate_play_bt;
static uint32_t lowdelay_data_size_play_bt;
static enum AUD_CHANNEL_NUM_T  lowdelay_playback_ch_num_bt;
#endif

#if defined(VOICE_DEV)
#if defined(APP_NOISE_ESTIMATION)
int app_noise_estimation_open(void);
int app_noise_estimation_close(void);
int app_noise_estimation_start(void);
int app_noise_estimation_stop(void);
#endif
#endif

extern void bt_media_clear_media_type(uint16_t media_type,enum BT_DEVICE_ID_T device_id);

extern "C" uint8_t is_a2dp_mode(void);
uint8_t bt_a2dp_mode;
extern "C" uint8_t __attribute__((section(".fast_text_sram")))  is_a2dp_mode(void)
{
    return bt_a2dp_mode;
}

extern "C" uint8_t is_sco_mode(void);
uint8_t bt_sco_mode;
extern "C"   uint8_t __attribute__((section(".fast_text_sram")))  is_sco_mode(void)
{
    return bt_sco_mode;
}

#define APP_BT_STREAM_TRIGGER_TIMEROUT (2000)

#define TRIGGER_CHECKER_A2DP_PLAYERBLACK        (1<<0)
#define TRIGGER_CHECKER_A2DP_DONE               (TRIGGER_CHECKER_A2DP_PLAYERBLACK)

#define TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK   (1<<1)
#define TRIGGER_CHECKER_HFP_BTPCM_CAPTURE       (1<<2)
#define TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK  (1<<3)
#define TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE      (1<<4)
#define TRIGGER_CHECKER_HFP_DONE                (TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK|TRIGGER_CHECKER_HFP_BTPCM_CAPTURE|TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK|TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE)

#define SYNCHRONIZE_NEED_DISCARDS_DMA_CNT 4

static APP_STREAM_GET_A2DP_PARAM_CALLBACK app_stream_get_a2dp_param_callback = NULL;

static bool app_bt_stream_trigger_enable = 0;
static uint32_t app_bt_stream_trigger_checker = 0;
static void app_bt_stream_trigger_timeout_cb(void const *n);
osTimerDef(APP_BT_STREAM_TRIGGER_TIMEOUT, app_bt_stream_trigger_timeout_cb);
osTimerId app_bt_stream_trigger_timeout_id = NULL;

static void app_bt_stream_trigger_timeout_cb(void const *n)
{
    TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]timeout_cb\n");
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]-->A2DP_SBC\n");
#if defined(IBRT)
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_TRIGGER_FAIL);
#else
        app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_RESTART, 0, 0);
#endif
    }else if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){
        TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]-->HFP_PCM\n");
#ifndef HFP_AG_TEST
        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_RESTART, 0);
#endif
    }

}

static int app_bt_stream_trigger_checker_init(void)
{
    if (app_bt_stream_trigger_timeout_id == NULL){
        app_bt_stream_trigger_enable = false;
        app_bt_stream_trigger_checker = 0;
        app_bt_stream_trigger_timeout_id = osTimerCreate(osTimer(APP_BT_STREAM_TRIGGER_TIMEOUT), osTimerOnce, NULL);
    }

    return 0;
}

static int app_bt_stream_trigger_checker_start(void)
{
    app_bt_stream_trigger_checker = 0;
    app_bt_stream_trigger_enable = true;
    osTimerStart(app_bt_stream_trigger_timeout_id, APP_BT_STREAM_TRIGGER_TIMEROUT);
    return 0;
}

static int app_bt_stream_trigger_checker_stop(void)
{
    app_bt_stream_trigger_enable = false;
    app_bt_stream_trigger_checker = 0;
    osTimerStop(app_bt_stream_trigger_timeout_id);
    return 0;
}

#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
bool a2dp_player_playback_waterline_enable = true;

bool a2dp_player_playback_waterline_is_enalbe(void)
{
    bool enable = false;
    uint32_t lock = int_lock();
    enable = a2dp_player_playback_waterline_enable;
    int_unlock(lock);
    return enable;
}

void a2dp_player_playback_waterline_enable_set(bool enable)
{
    uint32_t lock = int_lock();
    a2dp_player_playback_waterline_enable = enable;
    int_unlock(lock);
}
#endif


uint32_t app_bt_stream_get_dma_buffer_samples(void);
POSSIBLY_UNUSED static bool is_need_discards_samples = false;

int app_bt_stream_trigger_checker_handler(uint32_t trigger_checker)
{
    bool trigger_ok = false;

    if (app_bt_stream_trigger_enable){
        app_bt_stream_trigger_checker |= trigger_checker;
        if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
            if (app_bt_stream_trigger_checker == TRIGGER_CHECKER_A2DP_DONE){
                trigger_ok = true;
            }
        }else if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){
            if (app_bt_stream_trigger_checker == TRIGGER_CHECKER_HFP_DONE){
                trigger_ok = true;
            }
        }
        if (trigger_ok){
            TRACE_AUD_STREAM_I("[STRM_TRIG][CHK] ok\n");
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
#if defined(IBRT)
            if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) && is_need_discards_samples) {
                //limter to water line upper
                uint32_t list_samples = 0;
                uint32_t limter_water_line_samples = 0;
                A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
                uint32_t dest_discards_samples = app_bt_stream_get_dma_buffer_samples()/2 * SYNCHRONIZE_NEED_DISCARDS_DMA_CNT;
                if (a2dp_audio_lastframe_info_get(&lastframe_info)<0) {
                    TRACE_AUD_STREAM_W("[STRM_TRIG][CHK] force retrigger");
                    app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_DECODE_STATUS_ERROR);
                    return 0;
                }

                limter_water_line_samples = (a2dp_audio_dest_packet_mut_get() * lastframe_info.list_samples);
                a2dp_audio_convert_list_to_samples(&list_samples);
                TRACE_AUD_STREAM_W("[STRM_TRIG][CHK] synchronize:%d/%d", list_samples, limter_water_line_samples);
                if (list_samples > limter_water_line_samples){
                    // if(list_samples >= 2 * limter_water_line_samples) {
                    //     TRACE_AUD_STREAM_W("[STRM_TRIG][CHK] skip discards:%d", limter_water_line_samples);
                    //     a2dp_audio_discards_samples((list_samples / limter_water_line_samples - 1) * limter_water_line_samples);
                    // } else {
                    //     TRACE_AUD_STREAM_W("[STRM_TRIG][CHK] skip discards:%d", dest_discards_samples);
                    //     a2dp_audio_discards_samples(dest_discards_samples);
                    // }
                    TRACE_AUD_STREAM_W("[STRM_TRIG][CHK] skip discards:%d", dest_discards_samples);
                    a2dp_audio_discards_samples(dest_discards_samples);
                    is_need_discards_samples = false;
                }
            }
#endif
#endif
            app_bt_stream_trigger_checker_stop();
        }
    }
    return 0;
}

#ifdef A2DP_LHDC_ON
#if defined(A2DP_LHDC_V3)
#define LHDC_AUDIO_96K_BUFF_SIZE        (256*2*4*8)

//John:20210311:44.1k change from 240 to 220 pcm samples
#define LHDC_AUDIO_44P1K_BUFF_SIZE      (220*2*2*4)
#define LHDC_AUDIO_BUFF_SIZE            (240*2*2*4)
#define LHDC_LLC_AUDIO_BUFF_SIZE        (256*2*2*2)
#else
#define LHDC_AUDIO_BUFF_SIZE            (512*2*4)
//#define LHDC_AUDIO_16BITS_BUFF_SIZE     (512*2*2)
#endif
#endif
uint16_t gStreamplayer = APP_BT_STREAM_INVALID;

uint32_t a2dp_audio_more_data(uint8_t codec_type, uint8_t *buf, uint32_t len);

enum AUD_SAMPRATE_T a2dp_sample_rate = AUD_SAMPRATE_48000;
uint32_t a2dp_data_buf_size;
#ifdef RB_CODEC
extern int app_rbplay_audio_onoff(bool onoff, uint16_t aud_id);
#endif

enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate)
{
    enum AUD_SAMPRATE_T sample_rate;
    sbc_samp_rate = sbc_samp_rate & A2D_STREAM_SAMP_FREQ_MSK;

    switch (sbc_samp_rate)
    {
        case A2D_SBC_IE_SAMP_FREQ_16:
//            sample_rate = AUD_SAMPRATE_16000;
//            break;
        case A2D_SBC_IE_SAMP_FREQ_32:
//            sample_rate = AUD_SAMPRATE_32000;
//            break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            sample_rate = AUD_SAMPRATE_48000;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            sample_rate = AUD_SAMPRATE_44100;
            break;
#if defined(A2DP_LHDC_ON) ||defined(A2DP_SCALABLE_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            sample_rate = AUD_SAMPRATE_96000;
            break;
#endif
#if defined(A2DP_LDAC_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            sample_rate = AUD_SAMPRATE_96000;
            break;
#endif
#if defined(A2DP_LC3_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            sample_rate = AUD_SAMPRATE_96000;
            break;
#endif
        default:
            ASSERT(0, "[%s] 0x%x is invalid", __func__, sbc_samp_rate);
            break;
    }
    return sample_rate;
}

void bt_store_sbc_sample_rate(enum AUD_SAMPRATE_T sample_rate)
{
    a2dp_sample_rate = sample_rate;
}

enum AUD_SAMPRATE_T bt_get_sbc_sample_rate(void)
{
    return a2dp_sample_rate;
}

enum AUD_SAMPRATE_T bt_parse_store_sbc_sample_rate(uint8_t sbc_samp_rate)
{
    enum AUD_SAMPRATE_T sample_rate;

    sample_rate = bt_parse_sbc_sample_rate(sbc_samp_rate);
    bt_store_sbc_sample_rate(sample_rate);

    return sample_rate;
}

int app_bt_stream_register_a2dp_param_callback(APP_STREAM_GET_A2DP_PARAM_CALLBACK callback)
{
    app_stream_get_a2dp_param_callback = callback;

    return 0;
}

int bt_sbc_player_setup(uint8_t freq)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    static uint8_t sbc_samp_rate = 0xff;
    uint32_t ret;

    if (sbc_samp_rate == freq)
        return 0;

    switch (freq)
    {
        case A2D_SBC_IE_SAMP_FREQ_16:
        case A2D_SBC_IE_SAMP_FREQ_32:
        case A2D_SBC_IE_SAMP_FREQ_48:
            a2dp_sample_rate = AUD_SAMPRATE_48000;
            break;
#if defined(A2DP_LHDC_ON) ||defined(A2DP_SCALABLE_ON) || defined(A2DP_LC3_ON)
        case A2D_SBC_IE_SAMP_FREQ_96:
            a2dp_sample_rate = AUD_SAMPRATE_96000;
            TRACE_AUD_STREAM_I("%s:Sample rate :%d", __func__, freq);
            break;
#endif
#ifdef  A2DP_LDAC_ON
        case A2D_SBC_IE_SAMP_FREQ_96:
            a2dp_sample_rate = AUD_SAMPRATE_96000;
            TRACE_AUD_STREAM_I("%s:Sample rate :%d", __func__, freq);
            break;
#endif

        case A2D_SBC_IE_SAMP_FREQ_44:
            a2dp_sample_rate = AUD_SAMPRATE_44100;
            break;
        default:
            break;
    }

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->sample_rate = a2dp_sample_rate;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->sample_rate = a2dp_sample_rate;
        sample_rate_play_bt=stream_cfg->sample_rate;
        af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
        anc_mc_run_setup(hal_codec_anc_convert_rate(sample_rate_play_bt));
    }
#endif

    sbc_samp_rate = freq;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    if (audio_prompt_is_playing_ongoing())
    {
        audio_prompt_forcefully_stop();
    }
#endif

    return 0;
}

void merge_stereo_to_mono_16bits(int16_t *src_buf, int16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2)
    {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}

void merge_stereo_to_mono_24bits(int32_t *src_buf, int32_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2)
    {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}

const char *audio_op_to_str(enum APP_BT_SETTING_T op)
{
    switch (op)
    {
    case APP_BT_SETTING_OPEN: return "[OPEN]";
    case APP_BT_SETTING_CLOSE: return "[CLOSE]";
    case APP_BT_SETTING_SETUP: return "[SETUP]";
    case APP_BT_SETTING_RESTART: return "[RESTART]";
    case APP_BT_SETTING_CLOSEALL: return "[CLOSEALL]";
    case APP_BT_SETTING_CLOSEMEDIA: return "[CLOSEMEDIA]";
    default: return "[N/A]";
    }
}

static char _player_type_str[168];
static char *_catstr(char *dst, const char *src) {
     while(*dst) dst++;
     while((*dst++ = *src++));
     return --dst;
}
const char *player2str(uint16_t player_type) {
    const char *s = NULL;
    char _cat = 0, first = 1, *d = NULL;
    _player_type_str[0] = '\0';
    d = _player_type_str;
    d = _catstr(d, "[");
    if (player_type != 0) {
        for (int i = 15 ; i >= 0; i--) {
            _cat = 1;
            //TRACE_AUD_STREAM_I("i=%d,player_type=0x%d,player_type&(1<<i)=0x%x", i, player_type, player_type&(1<<i));
            switch(player_type&(1<<i)) {
                case 0: _cat = 0; break;
                case APP_BT_STREAM_HFP_PCM: s = "HFP_PCM"; break;
                case APP_BT_STREAM_HFP_CVSD: s = "HFP_CVSD"; break;
                case APP_BT_STREAM_HFP_VENDOR: s = "HFP_VENDOR"; break;
                case APP_BT_STREAM_A2DP_SBC: s = "A2DP_SBC"; break;
                case APP_BT_STREAM_A2DP_AAC: s = "A2DP_AAC"; break;
                case APP_BT_STREAM_A2DP_VENDOR: s = "A2DP_VENDOR"; break;
            #ifdef __FACTORY_MODE_SUPPORT__
                case APP_FACTORYMODE_AUDIO_LOOP: s = "AUDIO_LOOP"; break;
            #endif
                case APP_PLAY_BACK_AUDIO: s = "BACK_AUDIO"; break;
            #ifdef RB_CODEC
                case APP_BT_STREAM_RBCODEC: s = "RBCODEC"; break;
            #endif
            #ifdef AUDIO_LINEIN
                case APP_PLAY_LINEIN_AUDIO: s = "LINEIN_AUDIO"; break;
            #endif
            #if defined(BT_SOURCE)
                case APP_A2DP_SOURCE_LINEIN_AUDIO: s = "SRC_LINEIN_AUDIO"; break;
                case APP_A2DP_SOURCE_I2S_AUDIO: s = "I2S_AUDIO"; break;
            #endif
            #ifdef __AI_VOICE__
                case APP_BT_STREAM_AI_VOICE: s = "AI_VOICE"; break;
            #endif
                default:  s = "UNKNOWN"; break;
            }
            if (_cat) {
                if (!first)
                    d = _catstr(d, "|");
                //TRACE_AUD_STREAM_I("d=%s,s=%s", d, s);
                d = _catstr(d, s);
                first = 0;
            }
        }
    }

    _catstr(d, "]");

    return _player_type_str;
}


#ifdef __HEAR_THRU_PEAK_DET__
#include "peak_detector.h"
// Depend on codec_dac_vol
const float pkd_vol_multiple[18] = {0.281838, 0.000010, 0.005623, 0.007943, 0.011220, 0.015849, 0.022387, 0.031623, 0.044668, 0.063096, 0.089125, 0.125893, 0.177828, 0.251189, 0.354813, 0.501187, 0.707946, 1.000000};
int app_bt_stream_local_volume_get(void);
#endif
//extern void a2dp_get_curStream_remDev(btif_remote_device_t   **           p_remDev);
uint16_t a2dp_Get_curr_a2dp_conhdl(void);


#ifdef PLAYBACK_FORCE_48K
static uint32_t force48k_pcm_bytes = sizeof(int32_t);
static bool force48k_resample_needed = true;
static struct APP_RESAMPLE_T *force48k_resample;
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len);
struct APP_RESAMPLE_T *app_force48k_resample_any_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step);
int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len);
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))

#define BT_USPERCLK (625)
#define BT_MUTIUSPERSECOND (1000000/BT_USPERCLK)

#define CALIB_DEVIATION_MS (2)
#define CALIB_FACTOR_MAX_THRESHOLD (0.0001f)
#define CALIB_BT_CLOCK_FACTOR_STEP (0.0000005f)

#define CALIB_FACTOR_DELAY (0.001f)

//bt time
static int32_t  bt_old_clock_us=0;
static uint32_t bt_old_clock_mutius=0;
static int32_t  bt_old_offset_us=0;

static int32_t  bt_clock_us=0;
static uint32_t bt_clock_total_mutius=0;
static int32_t  bt_total_offset_us=0;

static int32_t bt_clock_ms=0;

//local time
static uint32_t local_total_samples=0;
static uint32_t local_total_frames=0;

//static uint32_t local_clock_us=0;
static int32_t local_clock_ms=0;

//bt and local time
static uint32_t bt_local_clock_s=0;

//calib time
static int32_t calib_total_delay=0;
static int32_t calib_flag=0;

//calib factor
static float   calib_factor_offset=0.0f;
static int32_t calib_factor_flag=0;
static volatile int calib_reset=1;
#endif

bool process_delay(int32_t delay_ms)
{
#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
    if (delay_ms == 0)return 0;

    TRACE_AUD_STREAM_I("delay_ms:%d", delay_ms);

    if(calib_flag==0)
    {
        calib_total_delay=calib_total_delay+delay_ms;
        calib_flag=1;
        return 1;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
void a2dp_clock_calib_process(uint32_t len)
{
//    btif_remote_device_t   * p_a2dp_remDev=NULL;
    uint32_t smplcnt = 0;
    int32_t btoffset = 0;

    uint32_t btclk = 0;
    uint32_t btcnt = 0;
    uint32_t btofs = 0;
    btclk = *((volatile uint32_t*)0xd02201fc);
    btcnt = *((volatile uint32_t*)0xd02201f8);
    btcnt=0;

   // TRACE_AUD_STREAM_I("bt_sbc_player_more_data btclk:%08x,btcnt:%08x\n", btclk, btcnt);


 //  a2dp_get_curStream_remDev(&p_a2dp_remDev);
    if(a2dp_Get_curr_a2dp_conhdl() >=0x80 && a2dp_Get_curr_a2dp_conhdl()<=0x82)
    {
        btofs = btdrv_rf_bitoffset_get( a2dp_Get_curr_a2dp_conhdl() -0x80);

        if(calib_reset==1)
        {
            calib_reset=0;

            bt_clock_total_mutius=0;

            bt_old_clock_us=btcnt;
            bt_old_clock_mutius=btclk;

            bt_total_offset_us=0;


            local_total_samples=0;
            local_total_frames=0;
            local_clock_ms = 0;

            bt_local_clock_s=0;
            bt_clock_us = 0;
            bt_clock_ms = 0;

            bt_old_offset_us=btofs;

            calib_factor_offset=0.0f;
            calib_factor_flag=0;
            calib_total_delay=0;
            calib_flag=0;
        }
        else
        {
            btoffset=btofs-bt_old_offset_us;

            if(btoffset<-BT_USPERCLK/3)
            {
                btoffset=btoffset+BT_USPERCLK;
            }
            else if(btoffset>BT_USPERCLK/3)
            {
                btoffset=btoffset-BT_USPERCLK;
            }

            bt_total_offset_us=bt_total_offset_us+btoffset;
            bt_old_offset_us=btofs;

            local_total_frames++;
            if(lowdelay_sample_size_play_bt==AUD_BITS_16)
            {
                smplcnt=len/(2*lowdelay_playback_ch_num_bt);
            }
            else
            {
                smplcnt=len/(4*lowdelay_playback_ch_num_bt);
            }

            local_total_samples=local_total_samples+smplcnt;

            bt_clock_us=btcnt-bt_old_clock_us-bt_total_offset_us;

            btoffset=btclk-bt_old_clock_mutius;
            if(btoffset<0)
            {
                btoffset=0;
            }
            bt_clock_total_mutius=bt_clock_total_mutius+btoffset;

            bt_old_clock_us=btcnt;
            bt_old_clock_mutius=btclk;

            if((bt_clock_total_mutius>BT_MUTIUSPERSECOND)&&(local_total_samples>lowdelay_sample_rate_play_bt))
            {
                bt_local_clock_s++;
                bt_clock_total_mutius=bt_clock_total_mutius-BT_MUTIUSPERSECOND;
                local_total_samples=local_total_samples-lowdelay_sample_rate_play_bt;
            }

            bt_clock_ms=(bt_clock_total_mutius*BT_USPERCLK/1000)+bt_clock_us/625;
            local_clock_ms=(local_total_samples*1000)/lowdelay_sample_rate_play_bt;

            local_clock_ms=local_clock_ms+calib_total_delay;

            //TRACE_AUD_STREAM_I("A2DP bt_clock_ms:%8d,local_clock_ms:%8d,bt_total_offset_us:%8d\n",bt_clock_ms, local_clock_ms,bt_total_offset_us);

            if(bt_clock_ms>(local_clock_ms+CALIB_DEVIATION_MS))
            {
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
                app_resample_tune(a2dp_resample, CALIB_FACTOR_DELAY);
#else
                af_codec_tune(AUD_STREAM_PLAYBACK, CALIB_FACTOR_DELAY);
#endif
                calib_factor_flag=1;
                //TRACE_AUD_STREAM_I("*************1***************");
            }
            else if(bt_clock_ms<(local_clock_ms-CALIB_DEVIATION_MS))
            {
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
                app_resample_tune(a2dp_resample, -CALIB_FACTOR_DELAY);
#else
                af_codec_tune(AUD_STREAM_PLAYBACK, -CALIB_FACTOR_DELAY);
#endif
                calib_factor_flag=-1;
                //TRACE_AUD_STREAM_I("*************-1***************");
            }
            else
            {
                if((calib_factor_flag==1||calib_factor_flag==-1)&&(bt_clock_ms==local_clock_ms))
                {
                    if(calib_factor_offset<CALIB_FACTOR_MAX_THRESHOLD&&calib_flag==0)
                    {
                        if(calib_factor_flag==1)
                        {
                            calib_factor_offset=calib_factor_offset+CALIB_BT_CLOCK_FACTOR_STEP;
                        }
                        else
                        {
                            calib_factor_offset=calib_factor_offset-CALIB_BT_CLOCK_FACTOR_STEP;
                        }

                    }
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
                    app_resample_tune(a2dp_resample, calib_factor_offset);
#else
                    af_codec_tune(AUD_STREAM_PLAYBACK, calib_factor_offset);
#endif
                    calib_factor_flag=0;
                    calib_flag=0;
                    //TRACE_AUD_STREAM_I("*************0***************");
                }
            }
          //  TRACE_AUD_STREAM_I("factoroffset:%d\n",(int32_t)((factoroffset)*(float)10000000.0f));
        }
    }

    return;
}

#endif

bool app_if_need_fix_target_rxbit(void)
{
    return (!bt_drv_is_enhanced_ibrt_rom());
}

static uint8_t isBtPlaybackTriggered = false;

bool bt_is_playback_triggered(void)
{
    return isBtPlaybackTriggered;
}

static void bt_set_playback_triggered(bool isEnable)
{
    isBtPlaybackTriggered = isEnable;
}

static void inline a2dp_audio_convert_16bit_to_24bit(int32_t *out, int16_t *in, int pcm_len)
{
    for (int i = pcm_len - 1; i >= 0; i--) {
        out[i] = ((int32_t)in[i] << 8);
    }
}
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
static void stream_media_mips_cost_statistic_get(uint32_t init_time,uint32_t start_time,uint32_t end_time);
#endif

#if defined(__XSPACE_UI__)
void xspace_ui_check_a2dp_data(uint8_t *buf, uint32_t len);
#endif
FRAM_TEXT_LOC uint32_t bt_sbc_player_more_data(uint8_t *buf, uint32_t len)
{
    POSSIBLY_UNUSED uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
    uint32_t init_time;
	uint32_t s_time,e_time;
    init_time = hal_sys_timer_get();
#endif

    app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_A2DP_PLAYERBLACK);
#if defined(A2DP_STREAM_AUDIO_DUMP)
    audio_dump_clear_up();
    audio_dump_add_channel_data_from_multi_channels(0, buf, g_a2dp_pcm_dump_frame_len, g_a2dp_pcm_dump_channel_num, 0);
    audio_dump_run();
#endif

    bt_set_playback_triggered(true);

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
    a2dp_clock_calib_process(len);
#endif

#if defined(IBRT) && defined(TOTA_v2)
    app_ibrt_ui_rssi_process();
#endif

#ifdef BT_XTAL_SYNC
#ifdef BT_XTAL_SYNC_NEW_METHOD
    if(a2dp_Get_curr_a2dp_conhdl() >=0x80 && a2dp_Get_curr_a2dp_conhdl()<=0x82)
    {
        uint32_t bitoffset = btdrv_rf_bitoffset_get( a2dp_Get_curr_a2dp_conhdl() -0x80);
        bt_xtal_sync_new(bitoffset,app_if_need_fix_target_rxbit(),BT_XTAL_SYNC_MODE_WITH_MOBILE);
    }

#else
    bt_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC);
#endif
#endif

#ifndef FPGA
    uint8_t codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
    if(app_stream_get_a2dp_param_callback)
    {
        uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
        codec_type = app_stream_get_a2dp_param_callback(device_id);
    }
    else
    {
        codec_type = bt_sbc_player_get_codec_type();
    }
    uint32_t overlay_id = 0;
    if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
    {
        overlay_id = APP_OVERLAY_A2DP_AAC;
    }

#if defined(A2DP_LHDC_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_LHDC;
    }
#endif
#if defined(A2DP_LDAC_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_LDAC;
    }
#endif


#if defined(A2DP_SCALABLE_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_SCALABLE;
    }
#endif
#if defined(A2DP_LC3_ON)
    else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
    {
        overlay_id = APP_OVERLAY_A2DP_LC3;
    }
#endif
    else
        overlay_id = APP_OVERLAY_A2DP;

    memset(buf, 0, len);

    if(app_get_current_overlay() != overlay_id)
    {
        return len;
    }
#endif

#if defined(PLAYBACK_FORCE_48K)
    len = len / (force48k_pcm_bytes / sizeof(int16_t));
    if (force48k_resample_needed) {
        app_playback_resample_run(force48k_resample, buf, len);
    } else
#endif
    {
#if (A2DP_DECODER_VER == 2)
        a2dp_audio_playback_handler(device_id, buf, len);
#else
#ifndef FPGA
        a2dp_audio_more_data(overlay_id, buf, len);
#endif
#endif
    }

#if defined(PLAYBACK_FORCE_48K)
    if (force48k_pcm_bytes == sizeof(int32_t)) {
        a2dp_audio_convert_16bit_to_24bit((int32_t *)buf, (int16_t *)buf, len / sizeof(int16_t));
    }
    len = len * (force48k_pcm_bytes / sizeof(int16_t));
#endif

#ifdef __AUDIO_SPECTRUM__
    audio_spectrum_run(buf, len);
#endif


#ifdef __KWS_AUDIO_PROCESS__
    short* pdata = (short*)buf;
    short pdata_mono = 0;
    for(unsigned int i = 0;i<len/4;i++)
    {
        pdata_mono = pdata[2*i]/2+pdata[2*i+1]/2;
        pdata[2*i] = pdata_mono;
        pdata[2*i+1] = pdata_mono;
    }
#endif

#ifdef __AUDIO_OUTPUT_MONO_MODE__
#ifdef A2DP_EQ_24BIT
    merge_stereo_to_mono_24bits((int32_t *)buf, (int32_t *)buf, len/sizeof(int32_t));
#else
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf, len/sizeof(int16_t));
#endif
#endif

#ifdef __HEAR_THRU_PEAK_DET__
#ifdef ANC_APP
    if(app_anc_work_status())
#endif
    {
        int vol_level = 0;
        vol_level = app_bt_stream_local_volume_get();
        peak_detector_run(buf, len, pkd_vol_multiple[vol_level]);
    }
#endif

#ifndef AUDIO_EQ_TUNING
#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#endif
#endif
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
    s_time = hal_sys_timer_get();
#endif
    audio_process_run(buf, len);
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
    e_time = hal_sys_timer_get();
    TRACE(4,"lenbyte=%d  time %d %d %d %d ",len,e_time-s_time,TICKS_TO_MS(e_time - s_time),e_time-init_time,TICKS_TO_MS(e_time - init_time));
#endif
#if defined(IBRT) && !defined(FPGA)
    app_tws_ibrt_audio_analysis_audiohandler_tick(device_id);
#endif

#if defined(__XSPACE_UI__)
    xspace_ui_check_a2dp_data(buf, len);
#endif

    osapi_notify_evm();

#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
    stream_media_mips_cost_statistic_get(init_time,s_time,e_time);
#endif
    return len;
}

FRAM_TEXT_LOC void bt_sbc_player_playback_post_handler(uint8_t *buf, uint32_t len, void *cfg)
{
    POSSIBLY_UNUSED struct AF_STREAM_CONFIG_T *config = (struct AF_STREAM_CONFIG_T *)cfg;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
#ifdef TWS_PROMPT_SYNC
    tws_playback_ticks_check_for_mix_prompt(device_id);
#endif
    if (audio_prompt_is_playing_ongoing())
    {
        audio_prompt_processing_handler(device_id, len, buf);
    }
#else
    app_ring_merge_more_data(buf, len);
#endif
}

#ifdef __THIRDPARTY
bool start_by_sbc = false;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static int32_t mid_p_8_old_l=0;
static int32_t mid_p_8_old_r=0;
#ifdef AUDIO_ANC_FB_ADJ_MC
#define ADJ_MC_STREAM_ID       AUD_STREAM_ID_1

#define ADJ_MC_SAMPLE_BITS         (16)
#define ADJ_MC_SAMPLE_BYTES      (ADJ_MC_SAMPLE_BITS / 8)
#define ADJ_MC_CHANNEL_NUM       (2)
#define ADJ_MC_FRAME_LEN            (256)
#define ADJ_MC_BUF_SIZE   (ADJ_MC_FRAME_LEN * ADJ_MC_CHANNEL_NUM * ADJ_MC_SAMPLE_BYTES * 2)//pingpong
static uint8_t POSSIBLY_UNUSED adj_mc_buf[ADJ_MC_BUF_SIZE];

static uint32_t audio_adj_mc_data_playback_a2dp(uint8_t *buf, uint32_t mc_len_bytes)
{
   uint32_t begin_time;
   //uint32_t end_time;
   begin_time = hal_sys_timer_get();
   TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL] begin_time: %d",begin_time);

   float left_gain;
   float right_gain;
   int playback_len_bytes,mc_len_bytes_run;
   int i,j,k;
   int delay_sample;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   //TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]playback_samplerate_ratio:  %d",playback_samplerate_ratio);

  // TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

   mc_len_bytes_run=mc_len_bytes/SAMPLERATE_RATIO_THRESHOLD;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_run);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC/2;

        for(i=0,j=0;i<delay_sample;i=i+1)
        {
            mid_p[j++]=delay_buf_bt[i];
        }

        for(i=0;i<playback_len_bytes/2-delay_sample;i=i+1)
        {
            mid_p[j++]=sour_p[i];
        }

        for(j=0;i<playback_len_bytes/2;i=i+1)
        {
            delay_buf_bt[j++]=sour_p[i];
        }

        if(playback_samplerate_ratio_bt<=SAMPLERATE_RATIO_THRESHOLD)
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+(SAMPLERATE_RATIO_THRESHOLD/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+1)
            {
                for(k=0;k<playback_samplerate_ratio_bt/SAMPLERATE_RATIO_THRESHOLD;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
        }

        anc_adj_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_run,AUD_BITS_16);
        for(i=0,j=0;i<(mc_len_bytes_run)/2;i=i+1)
        {
            int32_t l_value=mid_p_8[i];

#ifdef ANC_FB_MC_96KHZ
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
#else
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
#endif
        }

    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_run);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == (playback_buf_mc))
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC/2;

        for(i=0,j=0;i<delay_sample;i=i+1)
        {
            mid_p[j++]=delay_buf_bt[i];
        }

         for(i=0;i<playback_len_bytes/4-delay_sample;i=i+1)
        {
            mid_p[j++]=sour_p[i];
        }

         for(j=0;i<playback_len_bytes/4;i=i+1)
        {
            delay_buf_bt[j++]=sour_p[i];
        }

        if(playback_samplerate_ratio_bt<=SAMPLERATE_RATIO_THRESHOLD)
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+(SAMPLERATE_RATIO_THRESHOLD/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+1)
            {
                for(k=0;k<playback_samplerate_ratio_bt/SAMPLERATE_RATIO_THRESHOLD;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
        }

        anc_adj_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_run,AUD_BITS_24);
        for(i=0,j=0;i<(mc_len_bytes_run)/4;i=i+1)
        {
            int32_t l_value=mid_p_8[i];

#ifdef ANC_FB_MC_96KHZ
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
#else
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
            dest_p[j++]=l_value;
#endif
        }
    }

  //  end_time = hal_sys_timer_get();

 //   TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL] run time: %d", end_time-begin_time);

    return 0;
}
#else
static uint32_t audio_mc_data_playback_a2dp(uint8_t *buf, uint32_t mc_len_bytes)
{
//    uint32_t begin_time;
//    uint32_t end_time;
//    begin_time = hal_sys_timer_get();
//    TRACE_AUD_STREAM_I("music cancel: %d",begin_time);

   float left_gain;
   float right_gain;
   int playback_len_bytes,mc_len_bytes_run;
   int i,j,k;
   int delay_sample;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   //TRACE_AUD_STREAM_I("playback_samplerate_ratio:  %d",playback_samplerate_ratio);

  // TRACE_AUD_STREAM_I("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

   mc_len_bytes_run=mc_len_bytes/SAMPLERATE_RATIO_THRESHOLD;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_run);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_bt[i];
            mid_p[j++]=delay_buf_bt[i+1];
        }

        for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

        for(j=0;i<playback_len_bytes/2;i=i+2)
        {
            delay_buf_bt[j++]=sour_p[i];
            delay_buf_bt[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=SAMPLERATE_RATIO_THRESHOLD)
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(SAMPLERATE_RATIO_THRESHOLD/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/SAMPLERATE_RATIO_THRESHOLD;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_run,left_gain,right_gain,AUD_BITS_16);
        for(i=0,j=0;i<(mc_len_bytes_run)/2;i=i+2)
        {
            int32_t l_value=mid_p_8[i];
            int32_t r_value=mid_p_8[i+1];

#ifdef ANC_FB_MC_96KHZ
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
#else
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
#endif
        }
    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_run);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == (playback_buf_mc))
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf_bt[i];
            mid_p[j++]=delay_buf_bt[i+1];
        }

         for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

         for(j=0;i<playback_len_bytes/4;i=i+2)
        {
            delay_buf_bt[j++]=sour_p[i];
            delay_buf_bt[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio_bt<=SAMPLERATE_RATIO_THRESHOLD)
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(SAMPLERATE_RATIO_THRESHOLD/playback_samplerate_ratio_bt))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio_bt/SAMPLERATE_RATIO_THRESHOLD;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_run,left_gain,right_gain,AUD_BITS_24);
        for(i=0,j=0;i<(mc_len_bytes_run)/4;i=i+2)
        {
            int32_t l_value=mid_p_8[i];
            int32_t r_value=mid_p_8[i+1];

#ifdef ANC_FB_MC_96KHZ
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
#else
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
            dest_p[j++]=l_value;
            dest_p[j++]=r_value;
#endif

        }
    }
  //  end_time = hal_sys_timer_get();

 //   TRACE_AUD_STREAM_I("%s:run time: %d", __FUNCTION__, end_time-begin_time);

    return 0;
}
#endif
#endif

static uint8_t g_current_eq_index = 0xff;
static bool isMeridianEQON = false;

bool app_is_meridian_on()
{
    return isMeridianEQON;
}

uint8_t app_audio_get_eq()
{
    return g_current_eq_index;
}

bool app_meridian_eq(bool onoff)
{
    isMeridianEQON = onoff;
    return onoff;
}

int app_audio_set_eq(uint8_t index)
{
#ifdef __SW_IIR_EQ_PROCESS__
    if (index >=EQ_SW_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_FIR_EQ_PROCESS__
    if (index >=EQ_HW_FIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_DAC_IIR_EQ_PROCESS__
    if (index >=EQ_HW_DAC_IIR_LIST_NUM)
        return -1;
#endif
#ifdef __HW_IIR_EQ_PROCESS__
    if (index >=EQ_HW_IIR_LIST_NUM)
        return -1;
#endif
    g_current_eq_index = index;
    return index;
}

void bt_audio_updata_eq(uint8_t index)
{
    TRACE_AUD_STREAM_I("[EQ] update idx = %d", index);
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    AUDIO_EQ_TYPE_T audio_eq_type;
#ifdef __SW_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_SW_IIR;
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_FIR;
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_DAC_IIR;
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    audio_eq_type = AUDIO_EQ_TYPE_HW_IIR;
#endif
    bt_audio_set_eq(audio_eq_type,index);
#endif
}


#ifdef ANC_APP
uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status)
{
    anc_status = app_anc_work_status();
    if(anc_status_record != anc_status)
    {
        hal_sysfreq_req(HAL_SYSFREQ_USER_ANC, HAL_CMU_FREQ_104M);

        anc_status_record = anc_status;
        TRACE_AUD_STREAM_I("[EQ] update anc_status = %d",  anc_status);
#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,anc_status));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,anc_status));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,anc_status));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,anc_status));
#endif

        hal_sysfreq_req(HAL_SYSFREQ_USER_ANC, HAL_CMU_FREQ_32K);
    }

    return 0;
}
#endif


uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status)
{
    uint8_t index_eq=0;

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if (anc_status) {
                index_eq=audio_eq_sw_iir_index+1;
                if (index_eq >= EQ_SW_IIR_LIST_NUM) {
                    index_eq = 0;
                }
            } else {
                index_eq=audio_eq_sw_iir_index;
            }
        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(a2dp_sample_rate == AUD_SAMPRATE_44100) {
                index_eq = 0;
            } else if(a2dp_sample_rate == AUD_SAMPRATE_48000) {
                index_eq = 1;
            } else if(a2dp_sample_rate == AUD_SAMPRATE_96000) {
                index_eq = 2;
            } else {
                ASSERT(0, "[%s] sample_rate_recv(%d) is not supported", __func__, a2dp_sample_rate);
            }
            audio_eq_hw_fir_index=index_eq;

            if(anc_status) {
                index_eq=index_eq+3;
                if (index_eq >= EQ_HW_FIR_LIST_NUM) {
                    index_eq = 0;
                }
            }
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if (anc_status) {
                index_eq=audio_eq_hw_dac_iir_index+1;
                if (index_eq >= EQ_HW_DAC_IIR_LIST_NUM) {
                    index_eq = 0;
                }
            } else {
                index_eq=audio_eq_hw_dac_iir_index;
            }
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(anc_status) {
                index_eq=audio_eq_hw_iir_index+1;
                if (index_eq >= EQ_HW_IIR_LIST_NUM) {
                    index_eq = 0;
                }
            } else {
                index_eq=audio_eq_hw_iir_index;
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
    return index_eq;
}


uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type, uint8_t index)
{
    const FIR_CFG_T *fir_cfg=NULL;
    const IIR_CFG_T *iir_cfg=NULL;

    #if defined(__HW_FIR_EQ_PROCESS_2CH__)
        const FIR_CFG_T *fir_cfg_2=NULL;
    #endif

    TRACE_AUD_STREAM_I("[EQ] set type=%d,index=%d",  audio_eq_type,index);

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(index >= EQ_SW_IIR_LIST_NUM)
            {
                TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_SW_IIR_LIST_NUM", index);
                return 1;
            }

            iir_cfg=audio_eq_sw_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(index >= EQ_HW_FIR_LIST_NUM)
            {
                TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_FIR_LIST_NUM", index);
                return 1;
            }

        #if defined(__HW_FIR_EQ_PROCESS_2CH__)
            fir_cfg=audio_hw_hpfir_cfg_list[index];
            fir_cfg_2=audio_hw_lpfir_cfg_list[index];
        #else
            fir_cfg=audio_eq_hw_fir_cfg_list[index];
        #endif
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(index >= EQ_HW_DAC_IIR_LIST_NUM)
            {
                TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_DAC_IIR_LIST_NUM", index);
                return 1;
            }

            iir_cfg=audio_eq_hw_dac_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(index >= EQ_HW_IIR_LIST_NUM)
            {
                TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_IIR_LIST_NUM", index);
                return 1;
            }

            iir_cfg=audio_eq_hw_iir_cfg_list[index];
        }
        break;
#endif
        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif

#ifdef AUDIO_SECTION_ENABLE
    const IIR_CFG_T *iir_cfg_from_audio_section = (const IIR_CFG_T *)load_audio_cfg_from_audio_section(AUDIO_PROCESS_TYPE_IIR_EQ);
    if (iir_cfg_from_audio_section)
    {
        iir_cfg = iir_cfg_from_audio_section;
    }
#endif

#if defined(__HW_FIR_EQ_PROCESS_2CH__)
    return audio_eq_set_cfg(fir_cfg,fir_cfg_2,iir_cfg,audio_eq_type);
#else
    return audio_eq_set_cfg(fir_cfg,iir_cfg,audio_eq_type);
#endif
}



/********************************
        AUD_BITS_16
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        scalable delay = 864/sample*1000*n ms
        scalable delay = 864/44100*1000*13 = 117ms
        scalable delay = 864/96000*1000*6 = 118ms
        waterline delay = 864/sample*1000*n ms
        waterline delay = 864/44100*1000*3 = 58ms
        waterline delay = 864/96000*1000*3 = 27ms
        audio_delay = scalable delay + waterline delay
 *********************************/

#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU (13)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE (9000)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_US (A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE*A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU)

#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU  (6)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE (19500)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_US (A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE*A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU)

/********************************
        AUD_BITS_16
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        aac delay = 1024/sample*1000*n ms
        aac delay = 1024/44100*1000*5 = 116ms
        waterline delay = 1024/sample*1000*n ms
        waterline delay = 1024/44100*1000*3 = 69ms
        audio_delay = aac delay + waterline delay
 *********************************/
#ifndef A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU (6)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU (6)
#endif
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE (23000)
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_US (A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE*A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    sbc delay = 128/sample*n ms
    sbc delay = 128/44100*45 = 130ms
    sbc_delay = sbc delay(23219us)
    waterline delay = 128/sample*SBC_FRAME_MTU*n ms
    waterline delay = 128/44100*5*3 = 43ms
    audio_delay = aac delay + waterline delay
*********************************/
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU (5)
#ifndef A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU (35)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU (50)
#endif
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE (2800)
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_US (A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE*A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU)

#if defined(A2DP_LHDC_ON)
/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc delay =    *28 = 149ms
    audio_delay = lhdc delay

    lhdc_v2 delay = 512/96000*1000*38 =  202ms
    lhdc_v3 delay = 256/96000*1000*58 = 154ms
    audio_delay = lhdc_v3 delay
*********************************/
#if defined(IBRT)
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (58)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (38)
#endif
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (28)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE (2666)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE (5333)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_US (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc_v2 delay = 512/48000*1000*14 = 149ms
    lhdc_v3 delay = 256/48000*1000*28 = 149ms
    audio_delay = lhdc delay
*********************************/
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU (28)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU (14)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE (5333)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE (10666)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_US (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc_v2 delay = 512/48000*1000*9 = 96ms
    lhdc_v3 delay = 256/48000*1000*19 = 101ms
    audio_delay = lhdc delay
*********************************/
#if defined(IBRT)
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (15)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (9)
#endif
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (6)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE (5333)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE (10666)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_US (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE)
#endif

#if defined(A2DP_LDAC_ON)
/********************************
    AUD_BITS_16
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    ldac delay = 256/sample*1000*n ms
    ldac delay = 256/96000*1000*56 = 149ms
    audio_delay = ldac delay
*********************************/
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_FRAME_MTU (5)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU  (60)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE (2667)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US (A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE*A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU)
#endif

#if defined(A2DP_LC3_ON)
/********************************
        AUD_BITS_16
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        lc3 delay = 480/sample*1000*n ms
        lc3 delay = 480/44100*1000*13 = 141ms
        lc3 delay = 960/96000*1000*6 = 118ms
        audio_delay = scalable delay
 *********************************/

#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_MTU (13)
#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_BASE (21768)
#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_US (A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_BASE*A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_MTU)

#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_MTU  (6)
#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_BASE (20000)
#define A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_US (A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_BASE*A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_MTU)
#endif

enum BT_STREAM_TRIGGER_STATUS_T {
    BT_STREAM_TRIGGER_STATUS_NULL = 0,
    BT_STREAM_TRIGGER_STATUS_INIT,
    BT_STREAM_TRIGGER_STATUS_WAIT,
    BT_STREAM_TRIGGER_STATUS_OK,
};

static uint32_t tg_acl_trigger_time = 0;
static uint32_t tg_acl_trigger_start_time = 0;
static uint32_t tg_acl_trigger_init_time = 0;
static enum BT_STREAM_TRIGGER_STATUS_T bt_stream_trigger_status = BT_STREAM_TRIGGER_STATUS_NULL;

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream);

inline void app_bt_stream_trigger_stauts_set(enum BT_STREAM_TRIGGER_STATUS_T stauts)
{
    TRACE_AUD_STREAM_I("[STRM_TRIG] stauts_set %d->%d", bt_stream_trigger_status,stauts);
    bt_stream_trigger_status = stauts;
}

inline enum BT_STREAM_TRIGGER_STATUS_T app_bt_stream_trigger_stauts_get(void)
{
    //TRACE(1,"bt_stream_trigger_stauts_get:%d", bt_stream_trigger_status);
    return bt_stream_trigger_status;
}

uint32_t app_bt_stream_get_dma_buffer_delay_us(void)
{
    uint32_t dma_buffer_delay_us = 0;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    if (!af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false)){
    if (stream_cfg->bits <= AUD_BITS_16){
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    }else{
        dma_buffer_delay_us = stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;
    }
    }
    return dma_buffer_delay_us;
}

uint32_t app_bt_stream_get_dma_buffer_samples(void)
{
    uint32_t dma_buffer_delay_samples = 0;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    if (!af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false)){
    if (stream_cfg->bits <= AUD_BITS_16){
        dma_buffer_delay_samples = stream_cfg->data_size/stream_cfg->channel_num/2;
    }else{
        dma_buffer_delay_samples = stream_cfg->data_size/stream_cfg->channel_num/4;
    }
    }
    return dma_buffer_delay_samples;
}

#if defined(IBRT)
typedef enum {
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE,
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_ONPROCESS,
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_SYNCOK,
}APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE;

#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_CNT_LIMIT (100)
void app_bt_stream_ibrt_set_trigger_time(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger);
void app_bt_stream_ibrt_auto_synchronize_initsync_start(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger);
static APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T app_bt_stream_ibrt_auto_synchronize_trigger;
static uint32_t app_bt_stream_ibrt_auto_synchronize_cnt = 0;
int app_bt_stream_ibrt_audio_mismatch_stopaudio(uint8_t device_id);
void app_bt_stream_ibrt_auto_synchronize_hungup(void);
void app_bt_stream_ibrt_auto_synchronize_stop(void);
static APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE ibrt_auto_synchronize_status = APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE;

int app_bt_stream_ibrt_auto_synchronize_status_set(APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE status)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC] status:%d", status);
    ibrt_auto_synchronize_status = status;
    return 0;
}

APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE app_bt_stream_ibrt_auto_synchronize_status_get(void)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC] status:%d", ibrt_auto_synchronize_status);
    return ibrt_auto_synchronize_status;
}

int app_bt_stream_ibrt_auto_synchronize_trigger_start(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_trigger;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    app_bt_stream_ibrt_auto_synchronize_stop();
    TRACE_AUD_STREAM_I("[AUTO_SYNC] trigger:%d Seq:%d timestamp:%d SubSeq:%d/%d currSeq:%d",
                                                                                   sync_trigger->trigger_time,
                                                                                   sync_trigger->audio_info.sequenceNumber,
                                                                                   sync_trigger->audio_info.timestamp,
                                                                                   sync_trigger->audio_info.curSubSequenceNumber,
                                                                                   sync_trigger->audio_info.totalSubSequenceNumber,
                                                                                   header->sequenceNumber
                                                                                   );

    if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
        if (sync_trigger->trigger_time >= bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote))){
            app_bt_stream_ibrt_set_trigger_time(device_id, sync_trigger);
        }else{
            TRACE_AUD_STREAM_W("[AUTO_SYNC]failed trigger(%d)-->tg(%d) need resume",
                    bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)),
                    sync_trigger->trigger_time);

            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_TRIGGER_FAIL);
            //app_tws_ibrt_audio_sync_mismatch_resume_notify(device_id);
        }
    }else{
        TRACE_AUD_STREAM_I("[AUTO_SYNC] ok but currRole:%d mismatch\n", app_tws_get_ibrt_role(&curr_device->remote));
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_MISMATCH);
    }

    return 0;
}

int app_bt_stream_ibrt_auto_synchronize_dataind_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_trigger;
    bool synchronize_ok = false;
    int32_t timestamp_diff = 0;
    int32_t dma_buffer_samples = 0;
    int32_t frame_totle_samples = 0;
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    frame_totle_samples = sync_trigger->audio_info.totalSubSequenceNumber * sync_trigger->audio_info.frame_samples;
    timestamp_diff = sync_trigger->audio_info.timestamp - header->timestamp;

    TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] seq:%d/%d timestamp:%d/%d cnt:%d", header->sequenceNumber,
                                                                                    sync_trigger->audio_info.sequenceNumber,
                                                                                    header->timestamp,
                                                                                    sync_trigger->audio_info.timestamp,
                                                                                    app_bt_stream_ibrt_auto_synchronize_cnt);

    if (++app_bt_stream_ibrt_auto_synchronize_cnt > APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_CNT_LIMIT){
        app_bt_stream_ibrt_auto_synchronize_stop();
        TRACE_AUD_STREAM_W("[AUTO_SYNC][DATAIND] SYNCHRONIZE_CNT_LIMIT, we need force retrigger");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNCHRONIZE_CNT_LIMIT);
    }else if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
        app_bt_stream_ibrt_auto_synchronize_stop();
        TRACE_AUD_STREAM_W("[AUTO_SYNC][DATAIND] find role to master, we need force retrigger");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_MISMATCH);
    }else if (sync_trigger->audio_info.sequenceNumber < header->sequenceNumber){
        app_bt_stream_ibrt_auto_synchronize_stop();
        TRACE_AUD_STREAM_W("[AUTO_SYNC][DATAIND] seq timestamp:%d/%d mismatch need resume", header->timestamp,
                                                                                               sync_trigger->audio_info.timestamp);
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SEQ_MISMATCH);
    }else{
        if (header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber &&
            !sync_trigger->audio_info.totalSubSequenceNumber){
            synchronize_ok = true;
        }else if (header->timestamp == sync_trigger->audio_info.timestamp){
            synchronize_ok = true;
        }

        dma_buffer_samples = app_bt_stream_get_dma_buffer_samples()/2;

        if (sync_trigger->audio_info.timestamp >= header->timestamp && sync_trigger->audio_info.totalSubSequenceNumber){
            if (timestamp_diff < dma_buffer_samples){
                TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp_diff < dma_buffer_samples synchronize ok");
                synchronize_ok = true;
            }else if (timestamp_diff < frame_totle_samples){
                TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp_diff < frame_totle_samples synchronize ok");
                synchronize_ok = true;
            }
        }

        if (!synchronize_ok && header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber){

            TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp %d vs %d", sync_trigger->audio_info.timestamp - header->timestamp,
                                                                      frame_totle_samples);
            if ((sync_trigger->audio_info.timestamp - header->timestamp) <= (uint32_t)(frame_totle_samples*3)){
                sync_trigger->audio_info.sequenceNumber++;
                TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp try sequenceNumber:%d", header->sequenceNumber);
            }
        }

        //flush all
        a2dp_audio_synchronize_dest_packet_mut(0);

        if (synchronize_ok){
            A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
            if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0){
                TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND]synchronize ok but lastframe error");
                goto exit;
            }

            TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND]synchronize ok timestamp_diff:%d frame_samples:%d", timestamp_diff,
                                                                                                    lastframe_info.frame_samples);
            sync_trigger->trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL;
            sync_trigger->audio_info.sequenceNumber = header->sequenceNumber;
            sync_trigger->audio_info.timestamp = header->timestamp;
            if (sync_trigger->audio_info.totalSubSequenceNumber){
                sync_trigger->audio_info.curSubSequenceNumber = timestamp_diff/lastframe_info.frame_samples;
                TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND]synchronize ok tstmp_diff:%d/%d SubSeq:%d", timestamp_diff, sync_trigger->audio_info.frame_samples,
                                                                                           sync_trigger->audio_info.curSubSequenceNumber);
            }else{
                sync_trigger->audio_info.curSubSequenceNumber = 0;
            }
            sync_trigger->audio_info.totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
            sync_trigger->audio_info.frame_samples = lastframe_info.frame_samples;
            if (sync_trigger->audio_info.totalSubSequenceNumber &&
                sync_trigger->audio_info.curSubSequenceNumber >= sync_trigger->audio_info.totalSubSequenceNumber){
                TRACE_AUD_STREAM_W("[AUTO_SYNC][DATAIND]synchronize ok but sbc & timestamp is ms so force trigger");
                app_bt_stream_ibrt_auto_synchronize_stop();
                app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SEQ_MISMATCH);
            }else{
                a2dp_audio_detect_store_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_trigger_start);
            }
        }else{
            a2dp_audio_detect_first_packet();
        }
    }
exit:
    return 0;
}

void app_bt_stream_ibrt_auto_synchronize_start(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] trigger_time:%d seq:%d timestamp:%d SubSeq:%d/%d", sync_trigger->trigger_time,
                                                                                       sync_trigger->audio_info.sequenceNumber,
                                                                                       sync_trigger->audio_info.timestamp,
                                                                                       sync_trigger->audio_info.curSubSequenceNumber,
                                                                                       sync_trigger->audio_info.totalSubSequenceNumber
                                                                                       );
    app_bt_stream_ibrt_auto_synchronize_status_set(APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_ONPROCESS);
    app_bt_stream_ibrt_auto_synchronize_cnt = 0;
    app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
    a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_dataind_cb);
    a2dp_audio_detect_first_packet();
}


void app_bt_stream_ibrt_auto_synchronize_hungup(void)
{
    a2dp_audio_detect_next_packet_callback_register(NULL);
    a2dp_audio_detect_store_packet_callback_register(NULL);
}

void app_bt_stream_ibrt_auto_synchronize_stop(void)
{
    app_bt_stream_ibrt_auto_synchronize_hungup();
    app_bt_stream_ibrt_auto_synchronize_cnt = 0;
    app_bt_stream_ibrt_auto_synchronize_status_set(APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE);
}

bool app_bt_stream_ibrt_auto_synchronize_on_porcess(void)
{
    bool nRet = true;
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE synchronize_status = app_bt_stream_ibrt_auto_synchronize_status_get();
    if (synchronize_status == APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE){
        nRet = false;
    }
    return nRet;
}

void app_bt_stream_ibrt_start_sbc_player_callback(uint8_t device_id, uint32_t status, uint32_t param)
{
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        TRACE_AUD_STREAM_I("start_sbc_player_cb trigger(%d)-->tg(%d)", param, ((APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)param)->trigger_time);
        app_bt_stream_ibrt_set_trigger_time(device_id, (APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)param);
    }else{
        TRACE_AUD_STREAM_I("start_sbc_player_cb try again");
        app_audio_manager_sendrequest_need_callback(
                                                    APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,
                                                    device_id,
                                                    MAX_RECORD_NUM,
                                                    (uint32_t)app_bt_stream_ibrt_start_sbc_player_callback,
                                                    param);
    }
}

int app_bt_stream_ibrt_start_sbc_player(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    TRACE_AUD_STREAM_I("start_sbc_player tg(%d)", sync_trigger->trigger_time);
    app_audio_manager_sendrequest_need_callback(
                                                APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC,
                                                device_id,
                                                MAX_RECORD_NUM,
                                                (uint32_t)app_bt_stream_ibrt_start_sbc_player_callback,
                                                (uint32_t)sync_trigger);
    return 0;
}

uint16_t app_bt_stream_ibrt_trigger_seq_diff_calc(int32_t dma_samples, int32_t frame_samples, int32_t total_subseq, int32_t interval)
{
    float seq_factor = 1.0f;
    if (total_subseq){
        seq_factor = (float)(dma_samples/frame_samples)/(float)total_subseq;
    }else{
        seq_factor = (float)(dma_samples/frame_samples);
    }
    return (uint16_t)(seq_factor * (float)interval);
}

#define MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT (8)
#define SYNCHRONIZE_DATAIND_CNT_LIMIT (25)

static int synchronize_need_discards_dma_cnt = 0;
static int synchronize_dataind_cnt = 0;

int app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb_v2(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    bool synchronize_ok = false;
    bool discards_samples_finished = false;
    int dest_discards_samples = 0;
    uint32_t list_samples = 0;
    uint32_t curr_ticks = 0;
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_trigger;
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    synchronize_dataind_cnt++;
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2] mobile_link is connect retrigger because role switch");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_SWITCH);
        return 0;
    }

    if (synchronize_dataind_cnt >= SYNCHRONIZE_DATAIND_CNT_LIMIT){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2] mobile_link is connect retrigger because CNT_LIMIT");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNCHRONIZE_CNT_LIMIT);
        return 0;
    }

    is_need_discards_samples = false;
    dest_discards_samples = app_bt_stream_get_dma_buffer_samples()/2*synchronize_need_discards_dma_cnt;
    a2dp_audio_convert_list_to_samples(&list_samples);
    if ((int)list_samples > dest_discards_samples){
        discards_samples_finished = true;
        a2dp_audio_discards_samples(dest_discards_samples);
    }
    a2dp_audio_decoder_headframe_info_get(&headframe_info);
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2] sample:%d->%d seq:%d sub_seq:%d/%d",
                                                      list_samples,dest_discards_samples,
                                                      headframe_info.sequenceNumber,
                                                      headframe_info.curSubSequenceNumber,
                                                      headframe_info.totalSubSequenceNumber);

    if (discards_samples_finished){
        uint32_t lock = int_lock();
        bool trig_valid = false;
        curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
        if (sync_trigger->trigger_time > curr_ticks){
            trig_valid =  true;
        }else{
            TRACE_AUD_STREAM_I("[AUTO_SYNCV2] trig:%x/%x", curr_ticks, sync_trigger->trigger_time);
        }
        if (trig_valid){
            synchronize_ok = true;
            tg_acl_trigger_time = sync_trigger->trigger_time;
            btdrv_syn_trigger_codec_en(1);
            btdrv_syn_clr_trigger(0);
            btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
            bt_syn_set_tg_ticks(sync_trigger->trigger_time, APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
        }
        int_unlock(lock);
        if (trig_valid){
            app_tws_ibrt_audio_analysis_start(sync_trigger->handler_cnt, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
            app_tws_ibrt_audio_sync_start();
            app_tws_ibrt_audio_sync_new_reference(sync_trigger->factor_reference);
            TRACE_AUD_STREAM_I("[AUTO_SYNCV2] trigger curr(%d)-->tg(%d)",
                                                                bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)),
                                                                sync_trigger->trigger_time);
            synchronize_need_discards_dma_cnt = 0;
            synchronize_dataind_cnt = 0;
            a2dp_audio_detect_first_packet_clear();
            a2dp_audio_detect_next_packet_callback_register(NULL);
            a2dp_audio_detect_store_packet_callback_register(NULL);
            TRACE_AUD_STREAM_I("[AUTO_SYNCV2] synchronize_ok");
            return 0;
        }else{
            TRACE_AUD_STREAM_I("[AUTO_SYNCV2] synchronize_failed");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNC_FAIL);
            return 0;
        }
    }

    if (!synchronize_ok){
        a2dp_audio_detect_first_packet();
    }

    return 0;
}


void app_bt_stream_ibrt_mobile_link_playback_info_receive(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    uint32_t tg_tick = 0;
    uint32_t next_dma_cnt = 0;
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
    A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger_loc = &app_bt_stream_ibrt_auto_synchronize_trigger;
    struct BT_DEVICE_T* POSSIBLY_UNUSED p_bt_device = app_bt_get_device(device_id);

    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] session:%d hdl:%d clk:%d cnt:%d seq:%d/%d/%d", sync_trigger->a2dp_session,
                                                                                    sync_trigger->handler_cnt,
                                                                                    sync_trigger->trigger_bt_clk,
                                                                                    sync_trigger->trigger_bt_cnt,
                                                                                    sync_trigger->audio_info.sequenceNumber,
                                                                                    sync_trigger->audio_info.curSubSequenceNumber,
                                                                                    sync_trigger->audio_info.totalSubSequenceNumber);

    if (app_bt_stream_ibrt_auto_synchronize_on_porcess()){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] auto_synchronize_on_porcess skip it");
        return;
    }

    if (!app_bt_is_a2dp_streaming(device_id)){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] streaming not ready skip it");
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    if (a2dp_ibrt_session_get(device_id) != sync_trigger->a2dp_session){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] session mismatch skip it loc:%d rmt:%d", a2dp_ibrt_session_get(device_id), sync_trigger->a2dp_session);
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] session froce resume and try retrigger");
        a2dp_ibrt_session_set(sync_trigger->a2dp_session,device_id);
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] lastframe not ready mismatch_stopaudio");
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    *sync_trigger_loc = *sync_trigger;

    sync_info.sequenceNumber = sync_trigger->audio_info.sequenceNumber;
    sync_info.timestamp = sync_trigger->audio_info.timestamp;
    sync_info.curSubSequenceNumber = sync_trigger->audio_info.curSubSequenceNumber;
    sync_info.totalSubSequenceNumber = sync_trigger->audio_info.totalSubSequenceNumber;
    sync_info.frame_samples = sync_trigger->audio_info.frame_samples;
    if (a2dp_audio_synchronize_packet(&sync_info, A2DP_AUDIO_SYNCFRAME_MASK_ALL)){
        TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] synchronize_packe mismatch");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNC_MISMATCH);
        return;
    }

    a2dp_audio_decoder_headframe_info_get(&headframe_info);
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] sync with master packet step1 seq:%d sub_seq:%d/%d",
                                                                  headframe_info.sequenceNumber,
                                                                  headframe_info.curSubSequenceNumber,
                                                                  headframe_info.totalSubSequenceNumber);
    is_need_discards_samples = false;
    a2dp_audio_discards_samples(lastframe_info.list_samples);
    a2dp_audio_decoder_headframe_info_get(&headframe_info);
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] sync with master packet step2 seq:%d sub_seq:%d/%d",
                                                                  headframe_info.sequenceNumber,
                                                                  headframe_info.curSubSequenceNumber,
                                                                  headframe_info.totalSubSequenceNumber);

    uint32_t btclk;
    uint16_t btcnt;
    uint32_t mobile_master_clk = 0;
    uint16_t mobile_master_cnt = 0;
    int64_t mobile_master_us = 0;

#if defined(CHIP_BEST1501) || defined(CHIP_BEST1501SIMU) || defined(CHIP_BEST2003)
    btclk = btdrv_syn_get_curr_ticks();
#else
    btclk = btdrv_syn_get_curr_ticks()/2;
#endif
    btcnt = 0;
    app_tws_ibrt_audio_mobile_clkcnt_get(device_id, btclk, btcnt,
                                         &mobile_master_clk, &mobile_master_cnt);

    mobile_master_us = btdrv_clkcnt_to_us(mobile_master_clk,mobile_master_cnt);
    uint32_t rmt_mobile_master_clk = sync_trigger->trigger_bt_clk;
    uint16_t rmt_mobile_master_cnt = sync_trigger->trigger_bt_cnt;
    int64_t rmt_mobile_master_us = 0;
    int64_t tmp_mobile_master_us = 0;
    rmt_mobile_master_us = btdrv_clkcnt_to_us(rmt_mobile_master_clk,rmt_mobile_master_cnt);

    uint32_t dma_buffer_us = 0;
    dma_buffer_us = app_bt_stream_get_dma_buffer_delay_us()/2;

    tmp_mobile_master_us = rmt_mobile_master_us;
    do{
        if (tmp_mobile_master_us - mobile_master_us >= 0){
            break;
        }
        tmp_mobile_master_us += dma_buffer_us;
        next_dma_cnt++;
    }while(1);
    next_dma_cnt += MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT ;
    tmp_mobile_master_us += dma_buffer_us * (MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT-1);
    synchronize_need_discards_dma_cnt = next_dma_cnt + a2dp_audio_frame_delay_get() - 1;
    synchronize_dataind_cnt = 0;
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] loc:%08x%08x rmt:%08x%08x tg:%08x%08x", (uint32_t)((uint64_t)mobile_master_us>>32U), (uint32_t)((uint64_t)mobile_master_us&0xffffffff),
                                                                             (uint32_t)((uint64_t)rmt_mobile_master_us>>32U), (uint32_t)((uint64_t)rmt_mobile_master_us&0xffffffff),
                                                                             (uint32_t)((uint64_t)tmp_mobile_master_us>>32U), (uint32_t)((uint64_t)tmp_mobile_master_us&0xffffffff));

    tmp_mobile_master_us = tmp_mobile_master_us/SLOT_SIZE;

    tg_tick = tmp_mobile_master_us * 2;
    tg_tick &= 0x0fffffff;

    sync_trigger_loc->trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL;
    sync_trigger_loc->trigger_time = tg_tick;
    sync_trigger_loc->handler_cnt += next_dma_cnt;
    a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb_v2);
    a2dp_audio_detect_first_packet();


    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] mobile clk:%x/%x tg:%x",
    bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&p_bt_device->remote))/2,mobile_master_clk,tg_tick/2);

    TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] master_us:%x/%x/%x dma_cnt:%d/%d", (int32_t)mobile_master_us, (int32_t)rmt_mobile_master_us, (int32_t)tmp_mobile_master_us,
                                                                         next_dma_cnt, sync_trigger_loc->handler_cnt);
}


void app_bt_stream_ibrt_set_trigger_time(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    uint32_t curr_ticks = 0;
    uint32_t tg_tick = sync_trigger->trigger_time;
    A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
    int synchronize_ret;
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;

    if (app_bt_stream_ibrt_auto_synchronize_on_porcess()){
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] auto_synchronize_on_porcess skip it");
        return;
    }

    if (!app_bt_is_a2dp_streaming(device_id)){
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] streaming not ready skip it");
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    if (a2dp_ibrt_session_get(device_id) != sync_trigger->a2dp_session){
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] session mismatch skip it loc:%d rmt:%d", a2dp_ibrt_session_get(device_id), sync_trigger->a2dp_session);
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] session froce resume and try retrigger");
        a2dp_ibrt_session_set(sync_trigger->a2dp_session,device_id);
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0){
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] lastframe not ready mismatch_stopaudio");
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        return;
    }

    struct BT_DEVICE_T* POSSIBLY_UNUSED p_bt_device = app_bt_get_device(device_id);

    if (a2dp_audio_decoder_headframe_info_get(&headframe_info) < 0){
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] lastframe not ready mismatch_stopaudio");
        app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        goto exit;
    }
    TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] info base_seq:%d/%d", headframe_info.sequenceNumber, sync_trigger->sequenceNumberStart);

    a2dp_audio_detect_next_packet_callback_register(NULL);
    a2dp_audio_detect_store_packet_callback_register(NULL);

    sync_info.sequenceNumber = sync_trigger->sequenceNumberStart;
    synchronize_ret = a2dp_audio_synchronize_packet(&sync_info, A2DP_AUDIO_SYNCFRAME_MASK_SEQ);
    if (synchronize_ret){
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize_packet failed");
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] force_audio_retrigger");
        app_bt_stream_ibrt_auto_synchronize_stop();
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNC_FAIL);
        goto exit;
    }

    curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&p_bt_device->remote));
    if (tg_tick < curr_ticks){
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize tick failed:%x->%x", curr_ticks, tg_tick);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] force_audio_retrigger");
        app_bt_stream_ibrt_auto_synchronize_stop();
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SYNC_FAIL);
        goto exit;
    }
    sync_info.sequenceNumber = sync_trigger->audio_info.sequenceNumber;
    sync_info.timestamp = sync_trigger->audio_info.timestamp;
    sync_info.curSubSequenceNumber = sync_trigger->audio_info.curSubSequenceNumber;
    sync_info.totalSubSequenceNumber = sync_trigger->audio_info.totalSubSequenceNumber;
    sync_info.frame_samples = sync_trigger->audio_info.frame_samples;

    if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&p_bt_device->remote)){
        if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
            if (sync_trigger->trigger_type == APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC){
                TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] TRIGGER_TYPE_INIT_SYNC needskip:%d", sync_trigger->trigger_skip_frame);
                //limter to water line upper
                uint32_t list_samples = 0;
                uint32_t limter_water_line_samples = 0;
                limter_water_line_samples = (a2dp_audio_dest_packet_mut_get() * lastframe_info.list_samples);
                a2dp_audio_convert_list_to_samples(&list_samples);
                TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize:%d/%d", list_samples, limter_water_line_samples);
                if (list_samples > limter_water_line_samples){
                    is_need_discards_samples = true;
                    // TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] skip discards:%d", list_samples - limter_water_line_samples);
                    // a2dp_audio_discards_samples(list_samples - limter_water_line_samples);
                }
                app_bt_stream_ibrt_auto_synchronize_initsync_start(device_id, sync_trigger);
                app_bt_stream_ibrt_auto_synchronize_status_set(APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_SYNCOK);
            }
        }else if (!app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){
            TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] sbc player not active, so try to start it");
            app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
            app_bt_stream_ibrt_audio_mismatch_stopaudio(device_id);
        }
    }else{
        TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] Not Connected");
    }
exit:
    return;
}

void app_bt_stream_ibrt_audio_mismatch_resume(uint8_t device_id)
{
    ibrt_a2dp_status_t a2dp_status;

    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    a2dp_ibrt_sync_get_status(device_id, &a2dp_status);

    TRACE_AUD_STREAM_I("[MISMATCH] resume state:%d", a2dp_status.state);

    if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING){
        if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
            TRACE_AUD_STREAM_I("[MISMATCH] resume find role switch so force retrigger");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_SWITCH);
        }else{
            app_tws_ibrt_audio_sync_mismatch_resume_notify(device_id);
        }
    }
}

void app_bt_stream_ibrt_audio_mismatch_stopaudio_cb(uint8_t device_id, uint32_t status, uint32_t param)
{
    TRACE_AUD_STREAM_I("[MISMATCH] stopaudio_cb");

    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        TRACE_AUD_STREAM_I("[MISMATCH] stopaudio_cb try again");
        app_audio_manager_sendrequest_need_callback(
                                                    APP_BT_STREAM_MANAGER_STOP,BT_STREAM_SBC,
                                                    device_id,
                                                    MAX_RECORD_NUM,
                                                    (uint32_t)app_bt_stream_ibrt_audio_mismatch_stopaudio_cb,
                                                    (uint32_t)NULL);
    }else{
        app_bt_stream_ibrt_audio_mismatch_resume(device_id);
    }
}

int app_bt_stream_ibrt_audio_mismatch_stopaudio(uint8_t device_id)
{
    ibrt_a2dp_status_t a2dp_status;
    POSSIBLY_UNUSED struct BT_DEVICE_T* curr_device = app_bt_get_device(device_id);

    a2dp_ibrt_sync_get_status(device_id, &a2dp_status);

    TRACE_AUD_STREAM_I("[MISMATCH] stopaudio state:%d sco:%d sbc:%d media:%d", a2dp_status.state,
                                                 app_audio_manager_hfp_is_active((enum BT_DEVICE_ID_T)device_id),
                                                 app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)device_id),
                                                 app_bt_stream_isrun(APP_PLAY_BACK_AUDIO));

    if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING){
        if (app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)device_id)){
            TRACE_AUD_STREAM_I("[MISMATCH] stopaudio");
            app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_RESTART, 0, APP_SYSFREQ_52M);
            app_bt_stream_ibrt_audio_mismatch_resume(device_id);
        }else{
            if (APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote)){
                if (!bt_media_is_sbc_media_active()){
                    TRACE_AUD_STREAM_I("[MISMATCH] stopaudio not active resume it & force retrigger");
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC, device_id,MAX_RECORD_NUM);
                    app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_PLAYER_NOT_ACTIVE);
                }else{
                    if (app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)
#ifdef MEDIA_PLAYER_SUPPORT
                        &&app_play_audio_get_aud_id() == AUDIO_ID_BT_MUTE
#endif
                        ){
                        TRACE_AUD_STREAM_I("[MISMATCH] stopaudio resum on process skip it");
                    }else{
                        TRACE_AUD_STREAM_I("[MISMATCH] stopaudio cancel_media and force retrigger");
                        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_UNKNOW);
                    }
                }
            }else{
                TRACE_AUD_STREAM_I("[MISMATCH] stopaudio profile not exchanged skip it");
            }
        }
    }

    return 0;
}
#endif

void app_bt_stream_set_trigger_time(uint32_t trigger_time_us)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    uint32_t curr_ticks = 0;
    uint32_t dma_buffer_delay_us = 0;
    uint32_t tg_acl_trigger_offset_time = 0;
    POSSIBLY_UNUSED uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (trigger_time_us){
#if defined(IBRT)
        uint16_t conhandle = INVALID_HANDLE;
        if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
            conhandle = APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote);

            curr_ticks = bt_syn_get_curr_ticks(conhandle);
        }else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
            conhandle = APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote);
            curr_ticks = bt_syn_get_curr_ticks(conhandle);
        }else{
            return;
        }
#else
        uint16_t conhandle = 0xffff;
        conhandle = btif_me_get_conhandle_by_remote_address(&curr_device->remote);
        curr_ticks = bt_syn_get_curr_ticks(conhandle);
#endif
        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger(0);

        btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

        dma_buffer_delay_us = app_bt_stream_get_dma_buffer_delay_us();
        dma_buffer_delay_us /= 2;
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] %d-%d-%d dma_sz:%d dly:%d", stream_cfg->sample_rate, stream_cfg->channel_num, stream_cfg->bits,
                                                                                   stream_cfg->data_size, dma_buffer_delay_us);

        tg_acl_trigger_offset_time = US_TO_BTCLKS(trigger_time_us-dma_buffer_delay_us);
        tg_acl_trigger_time = curr_ticks + tg_acl_trigger_offset_time;
        tg_acl_trigger_start_time = curr_ticks;
#if defined(IBRT)
        bt_syn_set_tg_ticks(tg_acl_trigger_time, conhandle, BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] %d->%d trig_dly:%d aud_dly:%dus",
                                            curr_ticks, tg_acl_trigger_time, trigger_time_us-dma_buffer_delay_us, trigger_time_us+dma_buffer_delay_us);
#else
        bt_syn_set_tg_ticks(tg_acl_trigger_time, conhandle, BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] %d->%d trig_dly:%d aud_dly:%dus",
                                    curr_ticks, tg_acl_trigger_time, trigger_time_us-dma_buffer_delay_us, trigger_time_us+dma_buffer_delay_us);
#endif

        btdrv_syn_trigger_codec_en(1);
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_WAIT);
    }else{
        tg_acl_trigger_time = 0;
        tg_acl_trigger_start_time = 0;
        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger(0);
        bt_syn_cancel_tg_ticks(0);
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_NULL);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] trigger clear");
    }
}

void app_bt_stream_trigger_result(uint8_t device_id)
{
    POSSIBLY_UNUSED uint32_t curr_ticks = 0;

    if(tg_acl_trigger_time){
#if defined(IBRT)
        POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
        if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
            curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote));
            bt_syn_trig_checker(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote));
        }else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
            curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
            bt_syn_trig_checker(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
        }else{
            TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][RESULT] mobile_link:%d %04x ibrt_link:%d %04x", \
                                       APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), \
                                       APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
                                       APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote), \
                                       APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
        }
#else
        curr_ticks = btdrv_syn_get_curr_ticks();
#endif
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][RESULT] trig:%d curr:%d tg:%d start:%d", (curr_ticks - (uint32_t)US_TO_BTCLKS(app_bt_stream_get_dma_buffer_delay_us()/2)),
                                                                         curr_ticks, tg_acl_trigger_time, tg_acl_trigger_start_time);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][RESULT] tg_trig_diff:%d trig_diff:%d", (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_time),
                                              (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_start_time));
        app_bt_stream_set_trigger_time(0);
        app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_OK);
    #if defined(IBRT)
        A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
        a2dp_audio_decoder_headframe_info_get(&headframe_info);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][RESULT] synchronize_ok :%d", headframe_info.sequenceNumber);
    #endif
    }
}

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    if (id != AUD_STREAM_ID_0 || stream != AUD_STREAM_PLAYBACK) {
        return;
    }
    uint8_t a2dp_device = app_bt_audio_get_curr_a2dp_device();
    app_bt_stream_trigger_result(a2dp_device);
#if defined(IBRT)
    app_tws_ibrt_audio_analysis_interrupt_tick();
#endif
}
extern void a2dp_audio_set_mtu_limit(uint8_t mut);
extern float a2dp_audio_latency_factor_get(void);
extern uint8_t a2dp_lhdc_config_llc_get(void);

#define LHDC_EXT_FLAGS_JAS    0x01
#define LHDC_EXT_FLAGS_AR     0x02
#define LHDC_EXT_FLAGS_LLAC   0x04
#define LHDC_EXT_FLAGS_MQA    0x08
#define LHDC_EXT_FLAGS_MBR    0x10
#define LHDC_EXT_FLAGS_LARC   0x20
#define LHDC_EXT_FLAGS_V4     0x40
#define LHDC_SET_EXT_FLAGS(X) (lhdc_ext_flags |= X)
#define LHDC_CLR_EXT_FLAGS(X) (lhdc_ext_flags &= ~X)
#define LHDC_CLR_ALL_EXT_FLAGS() (lhdc_ext_flags = 0)
extern bool a2dp_lhdc_get_ext_flags(uint32_t flags);

void app_bt_stream_trigger_init(void)
{
#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_a2dp_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
        tg_acl_trigger_init_time = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote));
    }else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
        tg_acl_trigger_init_time = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }else{
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][INIT] mobile_link:%d %04x ibrt_link:%d %04x", \
                                    APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), \
                                    APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
                                    APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote), \
                                    APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
   }
#else
    tg_acl_trigger_init_time = btdrv_syn_get_curr_ticks();
#endif
    app_bt_stream_set_trigger_time(0);
#ifdef PLAYBACK_USE_I2S
    af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, false);
    af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, true);
#else
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
#endif
    app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_INIT);
}

void app_bt_stream_trigger_deinit(void)
{
    app_bt_stream_set_trigger_time(0);
}

void app_bt_stream_trigger_start(uint8_t device_id, uint8_t offset)
{
    float tg_trigger_time;
    POSSIBLY_UNUSED uint32_t curr_ticks;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);

#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote));
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        curr_ticks = bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
    else
    {
        return;
    }
#else
    curr_ticks = btdrv_syn_get_curr_ticks();
#endif

    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][START] init(%d)-->set_trig(%d) %dus", tg_acl_trigger_init_time, curr_ticks, (uint32_t)BTCLKS_TO_US(curr_ticks-tg_acl_trigger_init_time));
#if defined(A2DP_SCALABLE_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
        if(stream_cfg->sample_rate > AUD_SAMPRATE_48000){
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_US * a2dp_audio_latency_factor_get();

            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU);
#endif
        }else{
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU);
#endif
        }
    }else
#endif
#if defined(A2DP_AAC_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC){
#ifndef CUSTOM_BITRATE
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_AAC_US * a2dp_audio_latency_factor_get();
#else
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE * app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC) * a2dp_audio_latency_factor_get();
#endif
        tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU);
#endif
    }else
#endif
#if defined(A2DP_LHDC_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
        if (a2dp_lhdc_config_llc_get()){
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU);
#endif
        }else if(stream_cfg->sample_rate > AUD_SAMPRATE_48000){
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU);
#endif
        }else{
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU);
#endif
        }
    }else
#endif
#if defined(A2DP_LDAC_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
        uint32_t frame_mtu = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_FRAME_MTU;
#if (A2DP_DECODER_VER == 2)
        A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
        if (!a2dp_audio_lastframe_info_get(&lastframe_info)){
            frame_mtu = lastframe_info.totalSubSequenceNumber;
        }
#endif
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US * a2dp_audio_latency_factor_get();
        tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE*frame_mtu;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU);
#endif
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][START] [%d,%d,%d]", A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU,A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE,A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US);

    }else
#endif
#if defined(A2DP_LC3_ON)
    if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){

        if(stream_cfg->sample_rate > AUD_SAMPRATE_48000){
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_MTU);
#endif
        }else{
            tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_US * a2dp_audio_latency_factor_get();
            tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE;
#if (A2DP_DECODER_VER < 2)
            a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU);
#endif
        }
    }else
#endif
    {
        uint32_t frame_mtu = A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU;
#if (A2DP_DECODER_VER == 2)
        A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
        if (!a2dp_audio_lastframe_info_get(&lastframe_info)){
            frame_mtu = lastframe_info.totalSubSequenceNumber;
        }
#endif
#ifndef CUSTOM_BITRATE
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SBC_US * a2dp_audio_latency_factor_get();
#else
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE * app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_SBC) * a2dp_audio_latency_factor_get();
#endif
        tg_trigger_time += offset*A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE*frame_mtu;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU);
#endif
    }
    app_bt_stream_set_trigger_time((uint32_t)tg_trigger_time);
}

bool app_bt_stream_trigger_onprocess(void)
{
    if (app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_INIT){
        return true;
    }else{
        return false;
    }
}

#if defined(IBRT)
#ifdef A2DP_CP_ACCEL
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME (8)
#else
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME (4)
#endif
#ifdef A2DP_LHDC_ON
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC (12)
#endif
APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T app_bt_stream_ibrt_auto_synchronize_initsync_trigger;

int app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger = &app_bt_stream_ibrt_auto_synchronize_initsync_trigger;
    bool synchronize_ok = false;
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;

    TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC][DATAIND] dataind_seq:%d/%d timestamp:%d/%d", header->sequenceNumber,
                                                                             sync_trigger->audio_info.sequenceNumber,
                                                                             header->timestamp,
                                                                             sync_trigger->audio_info.timestamp);

    if (a2dp_audio_lastframe_info_get(&lastframe_info)<0){
        TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_DECODE_STATUS_ERROR);
        return 0;
    }

    if (app_bt_stream_trigger_stauts_get() != BT_STREAM_TRIGGER_STATUS_WAIT){
        TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] already end");
        a2dp_audio_detect_next_packet_callback_register(NULL);
        a2dp_audio_detect_store_packet_callback_register(NULL);
        return 0;
    }

    if (sync_trigger->audio_info.sequenceNumber < header->sequenceNumber){
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
        A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
        if (a2dp_audio_decoder_headframe_info_get(&headframe_info) < 0) {
            TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_DECODE_STATUS_ERROR);
            return 0;
        }
        if (sync_trigger->audio_info.sequenceNumber >= headframe_info.sequenceNumber) {
            TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] seq:sync_trigger<header");
            synchronize_ok = true;
        } else {
            TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SEQ_MISMATCH);
        }
#else
        TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_SEQ_MISMATCH);
#endif
    }else{
        if (header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber){
            synchronize_ok = true;
        }

        if (synchronize_ok && app_bt_stream_trigger_stauts_get() != BT_STREAM_TRIGGER_STATUS_WAIT){
            TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize_failed");
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_STATUS_ERROR);
            return 0;
        }
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
        //limter to water line upper
        uint32_t list_samples = 0;
        uint32_t limter_water_line_samples = 0;
        limter_water_line_samples = (a2dp_audio_dest_packet_mut_get() * lastframe_info.list_samples);
        a2dp_audio_convert_list_to_samples(&list_samples);
        TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize:%d/%d", list_samples, limter_water_line_samples);
        if (list_samples > limter_water_line_samples){
            is_need_discards_samples = true;
            // TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] skip discards:%d", list_samples - limter_water_line_samples);
            // a2dp_audio_discards_samples(list_samples - limter_water_line_samples);
        }
#else
        //flush all
        a2dp_audio_synchronize_dest_packet_mut(0);
#endif
    }

    if (synchronize_ok){
        A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
        a2dp_audio_decoder_headframe_info_get(&headframe_info);
        TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize_ok :%d", headframe_info.sequenceNumber);
        a2dp_audio_detect_next_packet_callback_register(NULL);
        a2dp_audio_detect_store_packet_callback_register(NULL);
    }else{
        a2dp_audio_detect_first_packet();
    }
    return 0;
}

void app_bt_stream_ibrt_auto_synchronize_initsync_start(uint8_t device_id, APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC] trigger_time:%d seq:%d timestamp:%d SubSeq:%d/%d", sync_trigger->trigger_time,
                                                                                       sync_trigger->audio_info.sequenceNumber,
                                                                                       sync_trigger->audio_info.timestamp,
                                                                                       sync_trigger->audio_info.curSubSequenceNumber,
                                                                                       sync_trigger->audio_info.totalSubSequenceNumber
                                                                                       );
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    app_bt_stream_ibrt_auto_synchronize_initsync_trigger = *sync_trigger;
    a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb);
    a2dp_audio_detect_first_packet();
    if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        if (sync_trigger->trigger_time > bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)))
        {
            uint32_t tg_tick = sync_trigger->trigger_time;
            btdrv_syn_trigger_codec_en(1);
            btdrv_syn_clr_trigger(0);
            btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
            bt_syn_set_tg_ticks(tg_tick, APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
            tg_acl_trigger_time = tg_tick;
            app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_WAIT);
            TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC] start slave trigger curr(%d)-->tg(%d)", bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)), tg_tick);
            app_tws_ibrt_audio_analysis_start(sync_trigger->handler_cnt, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
            app_tws_ibrt_audio_sync_start();
            app_tws_ibrt_audio_sync_new_reference(sync_trigger->factor_reference);
        }else{
            TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC] slave failed trigger(%d)-->tg(%d) need resume",
                                                            bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(NULL)),
                                                            sync_trigger->trigger_time);
            app_tws_ibrt_audio_sync_mismatch_resume_notify(device_id);
        }
    }
}

int app_bt_stream_ibrt_audio_master_detect_next_packet_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
    A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
#endif
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T sync_trigger;
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;

    if(app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_INIT){
        ibrt_ctrl_t  *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        int32_t dma_buffer_samples = app_bt_stream_get_dma_buffer_samples()/2;
        POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

        if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
            TRACE_AUD_STREAM_W("[AUTO_SYNC][MASTER]cache ok but currRole:%d mismatch\n", APP_IBAT_UI_GET_CURRENT_ROLE(&curr_device->remote));
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_MISMATCH);
        }else if (!APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote) &&
                  !app_ibrt_if_start_ibrt_onporcess(&curr_device->remote) &&
                  !app_ibrt_sync_a2dp_status_onporcess(&curr_device->remote)){
            if (APP_IBRT_UI_GET_MOBILE_MODE(&curr_device->remote) == IBRT_SNIFF_MODE){
                //flush all
                a2dp_audio_synchronize_dest_packet_mut(0);
                a2dp_audio_detect_first_packet();
                TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip delay dma trigger1\n");
                return 0;
            }
            TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache ok use dma trigger1\n");
            a2dp_audio_detect_next_packet_callback_register(NULL);
            a2dp_audio_detect_store_packet_callback_register(NULL);
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
            if(a2dp_player_playback_waterline_is_enalbe() == false){
                app_bt_stream_trigger_start(device_id, 0);
            }else{
                app_bt_stream_trigger_start(device_id, A2DP_PLAYER_PLAYBACK_WATER_LINE);
            }
#else
            app_bt_stream_trigger_start(device_id, 0);
#endif
        }else if (app_ibrt_if_start_ibrt_onporcess(&curr_device->remote) ||
                  app_ibrt_sync_a2dp_status_onporcess(&curr_device->remote) ||
                  !APP_IBRT_IS_A2DP_PROFILE_EXCHNAGED(&curr_device->remote) ||
                  app_ibrt_waiting_cmd_rsp()){
            //flush all
            a2dp_audio_synchronize_dest_packet_mut(0);
            a2dp_audio_detect_first_packet();
            TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] ibrt_onporcess:%d,sync_a2dp_status_onporcess:%d,waiting_cmd_rsp:%d,a2dp_profile_exchanged:%d",
                                app_ibrt_if_start_ibrt_onporcess(&curr_device->remote),
                                app_ibrt_sync_a2dp_status_onporcess(&curr_device->remote),
                                app_ibrt_waiting_cmd_rsp(),
                                APP_IBRT_IS_A2DP_PROFILE_EXCHNAGED(&curr_device->remote));
            TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip profile_exchanged sync_a2dp_status_onporcess\n");
            return 0;
        }else{
            if (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE    ||
                APP_IBRT_UI_GET_MOBILE_MODE(&curr_device->remote) == IBRT_SNIFF_MODE){
                //flush all
                a2dp_audio_synchronize_dest_packet_mut(0);
                a2dp_audio_detect_first_packet();
                TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip delay dma trigger2\n");
                return 0;
            }

#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
            uint32_t dest_waterline_samples = 0;
            uint32_t list_samples = 0;
            if(a2dp_player_playback_waterline_is_enalbe() == true){
                dest_waterline_samples = app_bt_stream_get_dma_buffer_samples()/2*A2DP_PLAYER_PLAYBACK_WATER_LINE;
            
                a2dp_audio_convert_list_to_samples(&list_samples);
                if (list_samples < dest_waterline_samples){
                    a2dp_audio_detect_first_packet();
                    TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip fill data sample:%d\n", list_samples);
                    return 0;
                }
            a2dp_audio_decoder_headframe_info_get(&headframe_info);
#ifdef A2DP_LHDC_ON
            uint8_t codec_type = bt_sbc_player_get_codec_type();
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                sync_info.sequenceNumber = headframe_info.sequenceNumber;
            }
            else
#endif
            {
                sync_info.sequenceNumber = headframe_info.sequenceNumber + 1;
            }
            a2dp_audio_synchronize_packet(&sync_info, A2DP_AUDIO_SYNCFRAME_MASK_SEQ);


            }else{
                a2dp_audio_convert_list_to_samples(&list_samples);
                if(list_samples <= 0){
                    a2dp_audio_detect_first_packet();
                    TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip fill data sample:%d\n", list_samples);
                    return 0;
                }
            }
#else
            a2dp_audio_synchronize_dest_packet_mut(0);
#endif
            TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache ok use dma trigger2\n");
#ifdef A2DP_CP_ACCEL
#ifdef A2DP_LHDC_ON
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC - a2dp_audio_frame_delay_get());
            }
            else
#endif
            {
                app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME - a2dp_audio_frame_delay_get());
            }
#else
            app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME);
#endif
            sync_trigger.trigger_time = tg_acl_trigger_time;
#ifdef A2DP_LHDC_ON
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                sync_trigger.trigger_skip_frame = APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC - a2dp_audio_frame_delay_get();
            }
            else
#endif
            {
                sync_trigger.trigger_skip_frame = APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME - a2dp_audio_frame_delay_get();
            }
            sync_trigger.trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC;
            if (a2dp_audio_lastframe_info_get(&lastframe_info)<0){
                goto exit;
            }

            a2dp_audio_decoder_headframe_info_get(&headframe_info);
            sync_trigger.sequenceNumberStart = headframe_info.sequenceNumber;


            TRACE(4,"sqNumStart = %d lastSeqNum = %d list-len = %d ",sync_trigger.sequenceNumberStart,lastframe_info.sequenceNumber,list_samples);
#ifdef A2DP_LHDC_ON
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                sync_trigger.audio_info.sequenceNumber = lastframe_info.sequenceNumber + APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
                if (lastframe_info.totalSubSequenceNumber){
                    sync_trigger.audio_info.timestamp = lastframe_info.timestamp +
                                                        (lastframe_info.totalSubSequenceNumber * lastframe_info.frame_samples) *
                                                        APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
                }else{
                    sync_trigger.audio_info.timestamp = lastframe_info.timestamp + dma_buffer_samples * APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
                }
            }
            else
#endif
            {
                sync_trigger.audio_info.sequenceNumber = lastframe_info.sequenceNumber + APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
                if (lastframe_info.totalSubSequenceNumber){
                    sync_trigger.audio_info.timestamp = lastframe_info.timestamp +
                                                        (lastframe_info.totalSubSequenceNumber * lastframe_info.frame_samples) *
                                                        APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
                }else{
                    sync_trigger.audio_info.timestamp = lastframe_info.timestamp + dma_buffer_samples * APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
                }
            }
            sync_trigger.audio_info.curSubSequenceNumber = lastframe_info.curSubSequenceNumber;
            sync_trigger.audio_info.totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
            sync_trigger.audio_info.frame_samples = lastframe_info.frame_samples;
            sync_trigger.factor_reference = a2dp_audio_get_output_config()->factor_reference;
            sync_trigger.a2dp_session = a2dp_ibrt_session_get(device_id);
            sync_trigger.handler_cnt = 0;
#if defined(IBRT_V2_MULTIPOINT)
            sync_trigger.mobile_addr = curr_device->remote;
#endif
            app_bt_stream_ibrt_auto_synchronize_initsync_start(device_id, &sync_trigger);

            if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote) &&
                APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote)){
                tws_ctrl_send_cmd_high_priority(APP_TWS_CMD_SET_TRIGGER_TIME, (uint8_t*)&sync_trigger, sizeof(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T));
            }
        }
    }else{
        if(app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_NULL){
            TRACE_AUD_STREAM_W("[AUTO_SYNC][MASTER] audio not ready skip it");
        }else{
            TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] unhandle status:%d", app_bt_stream_trigger_stauts_get());
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_UNKNOW);
        }
    }
exit:
    return 0;
}

int app_bt_stream_ibrt_audio_master_detect_next_packet_start(void)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] start");

    a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_audio_master_detect_next_packet_cb);
    return 0;
}

#define SLAVE_DETECT_NEXT_PACKET_TO_RETRIGGER_THRESHOLD (120)
static uint32_t slave_detect_next_packet_cnt = 0;
int app_bt_stream_ibrt_audio_slave_detect_next_packet_waitforever_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote) || ++slave_detect_next_packet_cnt > SLAVE_DETECT_NEXT_PACKET_TO_RETRIGGER_THRESHOLD){
        TRACE_AUD_STREAM_W("[AUTO_SYNC][SLAVE] detect_next_packet ok but currRole:%d mismatch packet_cnt:%d\n", APP_IBAT_UI_GET_CURRENT_ROLE(&curr_device->remote),
                                                                                slave_detect_next_packet_cnt);
        slave_detect_next_packet_cnt = 0;
        app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_MISMATCH);
    }else{
        TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE] detect_next_packet cnt:%d\n", slave_detect_next_packet_cnt);
        a2dp_audio_detect_first_packet();
    }
    return 0;
}

int app_bt_stream_ibrt_audio_slave_detect_next_packet_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    if(app_bt_stream_trigger_onprocess()){
        ibrt_ctrl_t  *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        int32_t dma_buffer_samples = app_bt_stream_get_dma_buffer_samples()/2;
        POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

        if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
        {
            TRACE_AUD_STREAM_W("[AUTO_SYNC][SLAVE] cache ok but currRole:%d mismatch\n", APP_IBAT_UI_GET_CURRENT_ROLE(&curr_device->remote));
            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_ROLE_MISMATCH);
        }
        else
        {
            if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote) &&
                (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE    ||
                 APP_IBRT_UI_GET_MOBILE_MODE(&curr_device->remote) == IBRT_SNIFF_MODE ||
                !APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote)))
            {
                //flush all
                a2dp_audio_synchronize_dest_packet_mut(0);
                a2dp_audio_detect_first_packet();
                TRACE_AUD_STREAM_W("[AUTO_SYNC][SLAVE]cache skip delay dma trigger2\n");
                return 0;
            }
            TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE]cache ok use dma trigger2\n");
            //flush all
            a2dp_audio_synchronize_dest_packet_mut(0);
#ifdef A2DP_CP_ACCEL
#ifdef A2DP_LHDC_ON
            uint8_t codec_type = bt_sbc_player_get_codec_type();
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC - a2dp_audio_frame_delay_get());
            }
            else
#endif
            {
                app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME - a2dp_audio_frame_delay_get());
            }
#else
            app_bt_stream_trigger_start(device_id, APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME);
#endif
            APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T sync_trigger = {0,};
            A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info = {0,};
            sync_trigger.trigger_time = tg_acl_trigger_time;
#ifdef A2DP_LHDC_ON
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                sync_trigger.trigger_skip_frame = APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC - a2dp_audio_frame_delay_get();
            }
            else
#endif
            {
                sync_trigger.trigger_skip_frame = APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME - a2dp_audio_frame_delay_get();
            }
            sync_trigger.trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC;
            if (a2dp_audio_lastframe_info_get(&lastframe_info)<0){
                goto exit;
            }
#ifdef A2DP_LHDC_ON
            if(codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
            {
                sync_trigger.audio_info.sequenceNumber = lastframe_info.sequenceNumber + APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
                sync_trigger.audio_info.timestamp = lastframe_info.timestamp + dma_buffer_samples * APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
            }
            else
#endif
            {
                sync_trigger.audio_info.sequenceNumber = lastframe_info.sequenceNumber + APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
                sync_trigger.audio_info.timestamp = lastframe_info.timestamp + dma_buffer_samples * APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
            }
            sync_trigger.audio_info.curSubSequenceNumber = lastframe_info.curSubSequenceNumber;
            sync_trigger.audio_info.totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
            sync_trigger.audio_info.frame_samples = lastframe_info.frame_samples;
            sync_trigger.factor_reference = a2dp_audio_get_output_config() ? a2dp_audio_get_output_config()->factor_reference : 1.0f;
            sync_trigger.a2dp_session = a2dp_ibrt_session_get(device_id);
            sync_trigger.handler_cnt = 0;

            app_bt_stream_ibrt_auto_synchronize_initsync_start(device_id, &sync_trigger);
        }
    }
exit:
    return 0;
}

int app_bt_stream_ibrt_audio_slave_detect_next_packet_start(int need_autotrigger)
{
    TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE] start");
    slave_detect_next_packet_cnt = 0;
    if (need_autotrigger){
        a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_audio_slave_detect_next_packet_waitforever_cb);
    }else{
        app_tws_ibrt_audio_analysis_start(0, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
        app_tws_ibrt_audio_sync_start();
        a2dp_audio_detect_next_packet_callback_register(app_bt_stream_ibrt_audio_slave_detect_next_packet_cb);
    }
    return 0;
}

#else
int app_bt_stream_detect_next_packet_cb(uint8_t device_id, btif_media_header_t * header, unsigned char *buf, unsigned int len)
{
    if(app_bt_stream_trigger_onprocess()){
        TRACE_AUD_STREAM_I("[AUTO_SYNC] start");
        app_bt_stream_trigger_start(device_id, 0);
    }
    return 0;
}
#endif

void app_audio_buffer_check(void)
{
    bool buffer_check = (APP_AUDIO_BUFFER_SIZE + APP_CAPTURE_AUDIO_BUFFER_SIZE) <= syspool_original_size();

    TRACE_AUD_STREAM_I("audio buf size[%d] capture buf size[%d] total available space[%d]",
        APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE, syspool_original_size());

    ASSERT(buffer_check,
        "Audio buffer[%d]+Capture buffer[%d] exceeds the maximum ram sapce[%d]",
        APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE, syspool_original_size());
}

typedef struct{
    uint8_t a2dp_sbc_delay_mtu;
    uint8_t a2dp_aac_delay_mtu;
}a2dp_customer_codec_delay_mtu;

static a2dp_customer_codec_delay_mtu a2dp_player_playback_delay_mtu = 
{
    A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU,
    A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU,
};

void app_audio_dynamic_update_dest_packet_mtu_set(uint8_t codec_index, uint8_t packet_mtu, uint8_t user_configure)
{
    if (codec_index >= 2){
        TRACE(0,"error codec type index %d",codec_index);
        return ;
    }

    TRACE(3,"%s index %d mtu %d",__func__,codec_index,packet_mtu);
    if (user_configure)
    {
        switch(codec_index)
        {
            case A2DP_AUDIO_CODEC_TYPE_SBC:
                a2dp_player_playback_delay_mtu.a2dp_sbc_delay_mtu = packet_mtu;
                break;
            case A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC:
                a2dp_player_playback_delay_mtu.a2dp_aac_delay_mtu = packet_mtu;
                break;
            default:
                break;
        }
    }
    else
    {
        a2dp_player_playback_delay_mtu.a2dp_sbc_delay_mtu = A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU;
        a2dp_player_playback_delay_mtu.a2dp_aac_delay_mtu = A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU;
    }
}

uint8_t app_audio_a2dp_player_playback_delay_mtu_get(uint16_t codec_type)
{
    uint8_t mtu = 0;
    if (codec_type == A2DP_AUDIO_CODEC_TYPE_SBC)
        mtu = a2dp_player_playback_delay_mtu.a2dp_sbc_delay_mtu;
    else if (codec_type == A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC)
        mtu = a2dp_player_playback_delay_mtu.a2dp_aac_delay_mtu;
    else{
        TRACE(0,"error codec tpye [%d]",codec_type);
    }
    return mtu;
}

static bool a2dp_is_run =  false;

bool bt_a2dp_is_run(void)
{
    return a2dp_is_run;
}
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
static uint32_t stream_media_mips_cost_statistic_sysfreq_get(void)
{
    enum HAL_CMU_FREQ_T cpu_freq = hal_sysfreq_get();

    switch(cpu_freq)
    {
        case HAL_CMU_FREQ_32K:
            return 0;
            break;
        case HAL_CMU_FREQ_26M:
            return 26;
            break;
        case HAL_CMU_FREQ_52M:
            return 52;
            break;
        case HAL_CMU_FREQ_78M:
            return 78;
            break;
        case HAL_CMU_FREQ_104M:
            return 104;
            break;
        case HAL_CMU_FREQ_208M:
            return 208;
            break;
        default:
            return 0;
            break;
    }
}

#define STREAM_MEDIA_MIPS_COST_STATISTIC_CNTS   100

static uint32_t total_media_mips_cal_time_in_ms;
static uint32_t stream_media_mips_cal_times;
static uint32_t stream_media_mips_cal_start_time_in_ms;
static uint32_t cpu_freq;
static uint32_t cpu_freq_last;
static bool stream_media_mips_cost_statistic_op = false;
static uint32_t stream_media_irq_delta_ms = 0;
static uint32_t stream_meida_irq_sum_raw_time_ms = 0;
static uint32_t stream_media_irq_sum_expect_time_ms = 0;

static void stream_media_mips_cost_statistic_reset(void)
{
    total_media_mips_cal_time_in_ms = 0;
    stream_media_mips_cal_times = 0;
    stream_media_mips_cal_start_time_in_ms = 0;
    cpu_freq =0;
    cpu_freq_last = 0;
    stream_media_mips_cost_statistic_op = false;
    stream_media_irq_delta_ms = 0;
    stream_meida_irq_sum_raw_time_ms = 0;
    stream_media_irq_sum_expect_time_ms = 0;
}

static void stream_media_mips_cost_statistic_get(uint32_t init_time,uint32_t start_time,uint32_t end_time)
{
    if(stream_media_mips_cost_statistic_op == false){
        // do first frame calculation
        stream_media_mips_cost_statistic_op = true;
        stream_media_mips_cal_start_time_in_ms = init_time;
        cpu_freq = stream_media_mips_cost_statistic_sysfreq_get();
        cpu_freq_last = stream_media_mips_cost_statistic_sysfreq_get();
    }

    total_media_mips_cal_time_in_ms += TICKS_TO_MS(end_time - start_time);
    stream_media_mips_cal_times++;
    stream_meida_irq_sum_raw_time_ms = stream_media_irq_delta_ms*stream_media_mips_cal_times;
    stream_media_irq_sum_expect_time_ms = hal_sys_timer_get();
    TRACE(6,"monitor: = %d %d %d <-> %d %d %d",stream_media_irq_delta_ms,stream_media_mips_cal_times,stream_meida_irq_sum_raw_time_ms,
        stream_media_irq_sum_expect_time_ms,stream_media_mips_cal_start_time_in_ms,
        TICKS_TO_MS(stream_media_irq_sum_expect_time_ms-stream_media_mips_cal_start_time_in_ms));
    if (stream_media_mips_cal_times % STREAM_MEDIA_MIPS_COST_STATISTIC_CNTS == 0)
    {
        cpu_freq = stream_media_mips_cost_statistic_sysfreq_get();
        TRACE(3, "STREAM MEDIA MIPS STATISTIC COST MIPS %d %d %d", cpu_freq * total_media_mips_cal_time_in_ms / TICKS_TO_MS(end_time - stream_media_mips_cal_start_time_in_ms),
                    cpu_freq * total_media_mips_cal_time_in_ms / stream_meida_irq_sum_raw_time_ms , 
                    cpu_freq * total_media_mips_cal_time_in_ms/(TICKS_TO_MS(stream_media_irq_sum_expect_time_ms - stream_media_mips_cal_start_time_in_ms)));
        TRACE(4, "stream media mips cost time %d test_times %d total time %d %d ",total_media_mips_cal_time_in_ms, stream_media_mips_cal_times, 
                    stream_meida_irq_sum_raw_time_ms,TICKS_TO_MS(stream_media_irq_sum_expect_time_ms - stream_media_mips_cal_start_time_in_ms));
        TRACE(3, "cpu_freq_last %d cpu_freq %d irq_delta_ms = %d", cpu_freq_last, cpu_freq,stream_media_irq_delta_ms);
        if (cpu_freq_last != cpu_freq)
        {
            cpu_freq_last = cpu_freq;
            stream_media_mips_cost_statistic_op = false;
            total_media_mips_cal_time_in_ms = 0;
            stream_media_mips_cal_times = 0;
            stream_meida_irq_sum_raw_time_ms = 0;
            stream_media_irq_sum_expect_time_ms = 0;
        }
    }
}
#endif

int bt_sbc_player(enum PLAYER_OPER_T on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
#ifdef AUDIO_ANC_FB_ADJ_MC
    struct AF_STREAM_CONFIG_T stream_cfg_adj_mc;
#endif
    enum AUD_SAMPRATE_T sample_rate;
    POSSIBLY_UNUSED const char *g_log_player_oper_str[] =
    {
        "PLAYER_OPER_START",
        "PLAYER_OPER_STOP",
        "PLAYER_OPER_RESTART",
    };

    uint8_t* bt_audio_buff = NULL;

    uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
    uint32_t POSSIBLY_UNUSED eq_buff_size = 0;
    uint8_t POSSIBLY_UNUSED play_samp_size;
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;
    TRACE_AUD_STREAM_I("[A2DP_PLAYER] work:%d op:%s freq:%d :sample:%d \n", a2dp_is_run, g_log_player_oper_str[on], freq,a2dp_sample_rate);

    bt_set_playback_triggered(false);

    if ((a2dp_is_run && on == PLAYER_OPER_START) || (!a2dp_is_run && on == PLAYER_OPER_STOP))
    {
        TRACE_AUD_STREAM_W("[A2DP_PLAYER],fail,isRun=%x,on=%x",a2dp_is_run,on);
        return 0;
    }

    if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART)
    {
        af_set_priority(AF_USER_SBC, osPriorityHigh);
    }

#if defined(A2DP_LHDC_ON) ||defined(A2DP_AAC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON) || defined(A2DP_LC3_ON)
    uint8_t codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
    if(app_stream_get_a2dp_param_callback)
    {
	    uint8_t current_device_id = app_bt_audio_get_curr_a2dp_device();
        codec_type = app_stream_get_a2dp_param_callback(current_device_id);
    }
    else
    {
        codec_type = bt_sbc_player_get_codec_type();
    }
#endif

    if (on == PLAYER_OPER_STOP || on == PLAYER_OPER_RESTART)
    {
#ifdef __THIRDPARTY
        start_by_sbc = false;
#endif

#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_enable();
#endif

#if defined(VOICE_DEV)
#if defined(APP_NOISE_ESTIMATION)
        app_noise_estimation_stop();
        app_noise_estimation_close();
#endif

#if defined(VOICE_DEV_TEST)
        test_voice_dev_stop();
        test_voice_dev_close();
#endif

        // Add more ...
        uint32_t algos = VOICE_DEV_ALGO_NONE;
        voice_dev_ctl(VOICE_DEV_USER0, VOICE_DEV_SET_SUPPORT_ALGOS, &algos);
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        bool isToClearActiveMedia = audio_prompt_clear_pending_stream(PENDING_TO_STOP_A2DP_STREAMING);
        if (isToClearActiveMedia)
        {
            // clear active media mark
            uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
            bt_media_clear_media_type(BT_STREAM_SBC, (enum BT_DEVICE_ID_T)device_id);
            bt_media_current_sbc_set(BT_DEVICE_INVALID_ID);
        }
#endif
#if (A2DP_DECODER_VER == 2)
        a2dp_audio_preparse_stop();
#endif

#ifdef AF_STREAM_ID_0_PLAYBACK_FADEOUT
#if defined(ANC_ASSIST_USE_INT_CODEC)
        af_stream_playback_fadeout(30);
#else
        af_stream_playback_fadeout(10);
#endif
#endif

#if defined(IBRT)
        app_bt_stream_ibrt_auto_synchronize_stop();
        app_tws_ibrt_audio_analysis_stop();
        app_tws_ibrt_audio_sync_stop();
#endif

#if (A2DP_DECODER_VER == 2)
        a2dp_audio_stop();
#endif
        af_codec_set_playback_post_handler(NULL);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#ifdef AUDIO_ANC_FB_ADJ_MC
        af_stream_stop(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
#endif
        af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
        calib_reset = 1;
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xF);
        }

#if defined(__AUDIO_SPECTRUM__)
        audio_spectrum_close();
#endif

        audio_process_close();

        TRACE_AUD_STREAM_I("[A2DP_PLAYER] syspool free size: %d/%d", syspool_free_size(), syspool_total_size());

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
        af_codec_tune(AUD_STREAM_PLAYBACK, 0);
#endif

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#ifdef AUDIO_ANC_FB_ADJ_MC
		af_stream_close(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
        app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, APP_SYSFREQ_32K);
#endif

        af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

        bt_a2dp_mode = 0;

#ifdef AI_AEC_CP_ACCEL
        cp_aec_deinit();
#endif

#if defined(ANC_ASSIST_ENABLED)
        app_anc_assist_set_mode(ANC_ASSIST_MODE_STANDALONE);
#endif

#if defined(CODEC_DAC_MULTI_VOLUME_TABLE)
        hal_codec_set_dac_volume_table(NULL, 0);
#endif

        af_set_irq_notification(NULL);
        if (on == PLAYER_OPER_STOP)
        {
#if 0
            osThreadId ctrl_thread_id = NULL;
            osPriority ctrl_thread_priority;
            ctrl_thread_id = osThreadGetId();
            ctrl_thread_priority = osThreadGetPriority(ctrl_thread_id);
            osThreadSetPriority(ctrl_thread_id, osPriorityLow);
#endif
            
            app_bt_stream_trigger_checker_stop();
#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
            app_bt_stream_trigger_deinit();
#endif
#ifndef FPGA
#ifdef BT_XTAL_SYNC
            bt_term_xtal_sync(false);
            bt_term_xtal_sync_default();
#endif
#endif
            a2dp_audio_deinit();
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)&&defined(AUDIO_ANC_FB_ADJ_MC)
            adj_mc_deinit();
#endif
            app_overlay_unloadall();
#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
            stream_media_mips_cost_statistic_reset();
#endif
#ifdef __THIRDPARTY
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_STOP2MIC, 0);
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START, 0);
#endif
            app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_32K);
#if 0
            osThreadSetPriority(ctrl_thread_id, ctrl_thread_priority);
            af_set_priority(AF_USER_SBC, osPriorityAboveNormal);
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BLE_V2)
            app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_A2DP,
                                           USER_ALL,
                                           BLE_ADV_INVALID_INTERVAL);
#endif
        }
    }

    if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART)
    {
#if defined(__IAG_BLE_INCLUDE__) && defined(BLE_V2)
        app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_A2DP,
                                       USER_ALL,
                                       BLE_ADVERTISING_INTERVAL);
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();
#endif

#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_STOP, 0);
#endif

        bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
        stream_local_volume = btdevice_volume_p->a2dp_vol;
        app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);

        if (BT_DEVICE_NUM > 1 && btif_me_get_activeCons() > 1)
        {
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }

#if defined(__SW_IIR_EQ_PROCESS__)&&defined(__HW_FIR_EQ_PROCESS__)&&defined(CHIP_BEST1000)
        if (audio_eq_hw_fir_cfg_list[bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0)]->len>128)
        {
            if (freq < APP_SYSFREQ_104M)
            {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#if defined(APP_MUSIC_26M) && !defined(__SW_IIR_EQ_PROCESS__)&& !defined(__HW_IIR_EQ_PROCESS__)&& !defined(__HW_FIR_EQ_PROCESS__)
        if (freq < APP_SYSFREQ_26M) {
            freq = APP_SYSFREQ_26M;
        }
#else
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#endif

#if defined(A2DP_LHDC_ON) ||defined(A2DP_AAC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON)
        TRACE_AUD_STREAM_I("[A2DP_PLAYER] codec_type:%d", codec_type);
#endif

#if defined(A2DP_AAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
            if(freq < APP_SYSFREQ_52M) {
                freq = APP_SYSFREQ_52M;
            }
        }
#endif
#if defined(A2DP_SCALABLE_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
        {
            if(a2dp_sample_rate==44100)
            {
                if(freq < APP_SYSFREQ_78M) {
                    freq = APP_SYSFREQ_78M;
                }
            }
            else if(a2dp_sample_rate==96000)
            {
                if (freq < APP_SYSFREQ_208M) {
                    freq = APP_SYSFREQ_208M;
                }
            }
        }
        TRACE_AUD_STREAM_I("[A2DP_PLAYER] a2dp_sample_rate=%d",a2dp_sample_rate);
#endif
#if defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#if defined(A2DP_LDAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#if defined(A2DP_LC3_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif

#if defined(__AUDIO_DRC__) || defined(__AUDIO_LIMITER__)
        freq = (freq < APP_SYSFREQ_208M)?APP_SYSFREQ_208M:freq;
#endif

#ifdef A2DP_CP_ACCEL
        // Default freq for SBC
        freq = APP_SYSFREQ_26M;

#if (BT_SBC_PLAY_DEFAULT_FREQ == 2)
        freq = APP_SYSFREQ_52M;
#endif


#if defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
            freq = APP_SYSFREQ_52M;
        }
#endif
#if defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            freq = APP_SYSFREQ_52M;
        }

        if (a2dp_sample_rate==AUD_SAMPRATE_96000)
        {
            freq = APP_SYSFREQ_104M;
        }
#endif
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif
#if defined(A2DP_LDAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (freq < APP_SYSFREQ_104M) {
                freq = APP_SYSFREQ_104M;
            }
        }
#endif
#endif

#if defined(PLAYBACK_FORCE_48K)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#endif

#if defined(AUDIO_EQ_TUNING)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#endif

#if defined(IBRT)
#if defined(IBRT_V2_MULTIPOINT)
        if(app_ibrt_conn_support_max_mobile_dev()==2)
        {
            if((codec_type == BTIF_AVDTP_CODEC_TYPE_SBC)&&(a2dp_sample_rate==AUD_SAMPRATE_48000))
            {
                TRACE(1,"sbc 48k freq %d",freq);
                freq = APP_SYSFREQ_78M;
            }
        }
#endif
#endif
        if (freq < APP_SYSFREQ_52M)
        {
            if (app_bt_audio_count_straming_mobile_links() > 1)
            {
                freq = APP_SYSFREQ_52M;
            }
        }

        app_audio_set_a2dp_freq(freq);

        TRACE_AUD_STREAM_I("[A2DP_PLAYER] sysfreq %d", freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
        TRACE_AUD_STREAM_I("[A2DP_PLAYER] sysfreq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));
#endif

        if (on == PLAYER_OPER_START)
        {
            af_set_irq_notification(app_bt_stream_playback_irq_notification);
            ASSERT(!app_ring_merge_isrun(), "Ring playback will be abnormal, please check.");
            if (0)
            {
            }
#if defined(A2DP_AAC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
            {
                app_overlay_select(APP_OVERLAY_A2DP_AAC);
            }
#endif
#if defined(A2DP_LHDC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                app_overlay_select(APP_OVERLAY_A2DP_LHDC);
            }
#endif
#if defined(A2DP_SCALABLE_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                app_overlay_select(APP_OVERLAY_A2DP_SCALABLE);
            }
#endif
#if defined(A2DP_LDAC_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                TRACE_AUD_STREAM_I("[A2DP_PLAYER] ldac overlay select \n"); //toto
                app_overlay_select(APP_OVERLAY_A2DP_LDAC);
            }
#endif
#if defined(A2DP_LC3_ON)
            else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            {
                TRACE(0,"bt_sbc_player lc3 overlay select \n");
                app_overlay_select(APP_OVERLAY_A2DP_LC3);
            }
#endif
            else
            {
                app_overlay_select(APP_OVERLAY_A2DP);
            }

#ifdef BT_XTAL_SYNC

#ifdef __TWS__
            if(app_tws_mode_is_only_mobile())
            {
                btdrv_rf_bit_offset_track_enable(false);
            }
            else
#endif
            {
                btdrv_rf_bit_offset_track_enable(true);
            }
            bt_init_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC, BT_INIT_XTAL_SYNC_MIN, BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif // BT_XTAL_SYNC
#ifdef __THIRDPARTY
            app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START2MIC, 0);
#endif

            bt_a2dp_mode = 1;
        }

#ifdef __THIRDPARTY
        start_by_sbc = true;
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START, 0);
#endif
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
        sample_rate = AUD_SAMPRATE_50781;
#else
        sample_rate = a2dp_sample_rate;
#endif

#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_disable();
#endif

#if defined(ANC_ASSIST_ENABLED)
#if 1
        app_anc_assist_set_mode(ANC_ASSIST_MODE_MUSIC);
#else
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
            app_anc_assist_set_mode(ANC_ASSIST_MODE_MUSIC_AAC);
        } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_SBC) {
            app_anc_assist_set_mode(ANC_ASSIST_MODE_MUSIC_SBC);
        } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
            // TODO: Be careful audioflinger callback frame length.
            app_anc_assist_set_mode(ANC_ASSIST_MODE_MUSIC_AAC);
        } else {
            ASSERT(0, "[%s] codec_type(%d) is invalid!!!", __func__, codec_type);
        }
#endif
#endif

#if defined(CODEC_DAC_MULTI_VOLUME_TABLE)
        hal_codec_set_dac_volume_table(codec_dac_a2dp_vol, ARRAY_SIZE(codec_dac_a2dp_vol));
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#ifdef PLAYBACK_FORCE_48K
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#else
        stream_cfg.sample_rate = sample_rate;
#endif

#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
#ifdef PLAYBACK_USE_I2S
        stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
#endif
#ifdef PLAYBACK_USE_I2S
        stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif

        stream_cfg.vol = stream_local_volume;
        stream_cfg.handler = bt_sbc_player_more_data;

#if defined(A2DP_SCALABLE_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            stream_cfg.data_size = SCALABLE_FRAME_SIZE*8;
        }else
#endif
#if defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
#if defined(A2DP_LHDC_V3)
            //stream_cfg.data_size = LHDC_AUDIO_96K_BUFF_SIZE;
            if (a2dp_lhdc_get_ext_flags(LHDC_EXT_FLAGS_LLAC))
            {
                if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_44100) {
                    stream_cfg.data_size = LHDC_AUDIO_44P1K_BUFF_SIZE;
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE 44.1K Buffer (kaiden:220)");

                } else {
                    stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE;
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE Normal Buffer");
                }
            }else if(a2dp_lhdc_get_ext_flags(LHDC_EXT_FLAGS_V4)) {
                if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_44100) {
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] LHDC V4 USE 44.1K Buffer");
                    stream_cfg.data_size = (LHDC_AUDIO_96K_BUFF_SIZE >> 2);
                } else {
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE 96K Buffer");
                    stream_cfg.data_size = LHDC_AUDIO_96K_BUFF_SIZE;
                }
            }else{
                if (a2dp_lhdc_config_llc_get()){
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE LHDC_LLC");
                    stream_cfg.data_size = LHDC_LLC_AUDIO_BUFF_SIZE;
                }
                else if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_44100) {
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] LHDC V4 USE 44.1K Buffer");
                    stream_cfg.data_size = (LHDC_AUDIO_96K_BUFF_SIZE >> 2);
                } else {
                    TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE 96K Buffer");
                    stream_cfg.data_size = LHDC_AUDIO_96K_BUFF_SIZE;
                }
            }
            /*
            if (a2dp_lhdc_config_llc_get()){
                TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE LHDC_LLC");
                stream_cfg.data_size = LHDC_LLC_AUDIO_BUFF_SIZE;
            }
            */
#else
            stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE;
#endif
#if 0
            if(bt_sbc_player_get_sample_bit() == AUD_BITS_16)
            {
#if defined(A2DP_LHDC_V3)
                if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_96000) {
                    stream_cfg.data_size = LHDC_AUDIO_96K_16BITS_BUFF_SIZE;
                } else {
                    stream_cfg.data_size = LHDC_AUDIO_16BITS_BUFF_SIZE;
                }
#else
                stream_cfg.data_size = LHDC_AUDIO_16BITS_BUFF_SIZE;
#endif
            }
#endif
        }else
#endif
#if defined(A2DP_LDAC_ON)
        if(codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
            stream_cfg.data_size = BT_AUDIO_BUFF_SIZE_LDAC;
        else
#endif
#if defined(A2DP_AAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
            stream_cfg.data_size = BT_AUDIO_BUFF_AAC_SIZE;
        else
#endif
#if defined(A2DP_LC3_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
            if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_96000) {
                stream_cfg.data_size = LC3_AUDIO_96K_BUFF_SIZE;
            } else {
                stream_cfg.data_size = LC3_AUDIO_BUFF_SIZE;
            }
        }else
#endif
        {
            if (stream_cfg.sample_rate == AUD_SAMPRATE_44100){
                stream_cfg.data_size = BT_AUDIO_BUFF_SBC_44P1K_SIZE;
            }else{
                stream_cfg.data_size = BT_AUDIO_BUFF_SBC_48K_SIZE;
            }
        }

        stream_cfg.bits = AUD_BITS_16;

#ifdef A2DP_EQ_24BIT
        stream_cfg.data_size *= 2;
        stream_cfg.bits = AUD_BITS_24;
#elif defined(A2DP_SCALABLE_ON) || defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
            stream_cfg.data_size *= 2;
            stream_cfg.bits = AUD_BITS_24;
        }
#endif

#if 0//defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
        {
            if(bt_sbc_player_get_sample_bit() == AUD_BITS_16)
            {
                stream_cfg.bits = AUD_BITS_16;
            }
        }
#endif

        a2dp_data_buf_size = stream_cfg.data_size;

        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
        lowdelay_sample_size_play_bt=stream_cfg.bits;
        lowdelay_sample_rate_play_bt=stream_cfg.sample_rate;
        lowdelay_data_size_play_bt=stream_cfg.data_size;
        lowdelay_playback_ch_num_bt=stream_cfg.channel_num;
#endif


#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        uint8_t* promptTmpSourcePcmDataBuf;
        uint8_t* promptTmpTargetPcmDataBuf;
        uint8_t* promptPcmDataBuf;
        uint8_t* promptResamplerBuf;

        app_audio_mempool_get_buff(&promptTmpSourcePcmDataBuf, AUDIO_PROMPT_SOURCE_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptTmpTargetPcmDataBuf, AUDIO_PROMPT_TARGET_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptPcmDataBuf, AUDIO_PROMPT_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptResamplerBuf, AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);

        audio_prompt_buffer_config(MIX_WITH_A2DP_STREAMING,
                                   stream_cfg.channel_num,
                                   stream_cfg.bits,
                                   promptTmpSourcePcmDataBuf,
                                   promptTmpTargetPcmDataBuf,
                                   promptPcmDataBuf,
                                   AUDIO_PROMPT_PCM_BUFFER_SIZE,
                                   promptResamplerBuf,
                                   AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;
	 if(sample_rate_play_bt==AUD_SAMPRATE_96000)
	 {
		playback_samplerate_ratio_bt=4;
	 }
	 else
	 {
		playback_samplerate_ratio_bt=8;
	 }
        playback_ch_num_bt=stream_cfg.channel_num;
        mid_p_8_old_l=0;
        mid_p_8_old_r=0;
#endif

#if defined(PLAYBACK_FORCE_48K)
        if (stream_cfg.bits == AUD_BITS_16) {
            force48k_pcm_bytes = sizeof(int16_t);
        } else {
            force48k_pcm_bytes = sizeof(int32_t);
        }

        if (sample_rate == AUD_SAMPRATE_48000) {
            force48k_resample_needed = false;
        } else {
            force48k_resample_needed = true;
            force48k_resample= app_force48k_resample_any_open(bt_get_sbc_sample_rate(), stream_cfg.channel_num,
                                app_force48k_resample_iter, stream_cfg.data_size / stream_cfg.channel_num / (force48k_pcm_bytes / sizeof(int16_t)),
                                (float)sample_rate / AUD_SAMPRATE_48000);
            TRACE(3, "[%s] Resample: %d --> %d", __func__, sample_rate, AUD_SAMPRATE_48000);
        }
#endif

        TRACE(4, "A2DP Playback: sample rate: %d, bits = %d, channel number = %d, data size:%d",
                                            stream_cfg.sample_rate,
                                            stream_cfg.bits,
                                            stream_cfg.channel_num,
                                            stream_cfg.data_size);

#ifdef A2DP_STREAM_MEDIA_MIPS_COST_STATISTIC
        if(stream_cfg.bits == AUD_BITS_16){
            stream_media_irq_delta_ms = (stream_cfg.data_size *1000)/ sizeof(int16_t) / 2 / sample_rate / stream_cfg.channel_num;
        }else{
            stream_media_irq_delta_ms = (stream_cfg.data_size *1000)/ sizeof(int32_t) / 2 / sample_rate / stream_cfg.channel_num;
        }

        TRACE(1,"dma irq delta_ms = %d",stream_media_irq_delta_ms);
#endif
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(A2DP_STREAM_AUDIO_DUMP)
        if (stream_cfg.bits == 16) {
            g_a2dp_pcm_dump_sample_bytes = sizeof(int16_t);
        } else {
            g_a2dp_pcm_dump_sample_bytes = sizeof(int32_t);
        }
        g_a2dp_pcm_dump_channel_num = stream_cfg.channel_num;
        g_a2dp_pcm_dump_frame_len = stream_cfg.data_size / 2 / g_a2dp_pcm_dump_sample_bytes / g_a2dp_pcm_dump_channel_num;
        audio_dump_init(g_a2dp_pcm_dump_frame_len, g_a2dp_pcm_dump_sample_bytes, 1);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
	#ifdef AUDIO_ANC_FB_ADJ_MC
         stream_cfg.handler = audio_adj_mc_data_playback_a2dp;
	#else
	  stream_cfg.handler = audio_mc_data_playback_a2dp;
	#endif
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*playback_samplerate_ratio_bt;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

        memset(delay_buf_bt,0,sizeof(delay_buf_bt));

        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
	 //ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#ifdef AUDIO_ANC_FB_ADJ_MC
        app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, APP_SYSFREQ_208M);
        TRACE(0, "[ADJ_MC] sysfreq %d", freq);
        // TRACE(0, "[ADJ_MC] sysfreq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

        adj_mc_capture_sample_rate = sample_rate_play_bt / 3;
        adj_mc_init(ADJ_MC_FRAME_LEN);
		memset(&stream_cfg_adj_mc, 0, sizeof(stream_cfg_adj_mc));
		stream_cfg_adj_mc.channel_num = (enum AUD_CHANNEL_NUM_T)ADJ_MC_CHANNEL_NUM;
		stream_cfg_adj_mc.channel_map = (enum AUD_CHANNEL_MAP_T)(ANC_FB_MIC_CH_L | AUD_CHANNEL_MAP_ECMIC_CH0);
		stream_cfg_adj_mc.data_size = ADJ_MC_BUF_SIZE;
		stream_cfg_adj_mc.sample_rate = (enum AUD_SAMPRATE_T)adj_mc_capture_sample_rate;
		stream_cfg_adj_mc.bits = AUD_BITS_16;
		stream_cfg_adj_mc.vol = 12;
		stream_cfg_adj_mc.chan_sep_buf = true;
		stream_cfg_adj_mc.device = AUD_STREAM_USE_INT_CODEC;
		stream_cfg_adj_mc.io_path = AUD_INPUT_PATH_MAINMIC;
		stream_cfg_adj_mc.handler = adj_mc_filter_estimate;
		stream_cfg_adj_mc.data_ptr = adj_mc_buf;
		TRACE(2,"[A2DP_PLAYER] capture sample_rate:%d, data_size:%d",stream_cfg_adj_mc.sample_rate,stream_cfg_adj_mc.data_size);
		af_stream_open(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg_adj_mc);
#endif

#endif

#ifdef __HEAR_THRU_PEAK_DET__
        PEAK_DETECTOR_CFG_T peak_detector_cfg;
        peak_detector_cfg.fs = stream_cfg.sample_rate;
        peak_detector_cfg.bits = stream_cfg.bits;
        peak_detector_cfg.factor_up = 0.6;
        peak_detector_cfg.factor_down = 2.0;
        peak_detector_cfg.reduce_dB = -30;
        peak_detector_init();
        peak_detector_setup(&peak_detector_cfg);
#endif

#if defined(__AUDIO_SPECTRUM__)
        audio_spectrum_open(stream_cfg.sample_rate, stream_cfg.bits);
#endif

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = a2dp_data_buf_size*2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

        play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
        eq_buff_size = a2dp_data_buf_size * sizeof(int32_t) / play_samp_size;
#elif defined(CHIP_BEST1000)
        eq_buff_size = a2dp_data_buf_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
        eq_buff_size = a2dp_data_buf_size;
#endif
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = a2dp_data_buf_size;
#else
        eq_buff_size = 0;
        bt_eq_buff = NULL;
#endif

        if(eq_buff_size > 0)
        {
            app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
        }

        audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num, stream_cfg.data_size/(stream_cfg.bits <= AUD_BITS_16 ? 2 : 4)/2, bt_eq_buff, eq_buff_size);

// disable audio eq config on a2dp start for audio tuning tools
#ifndef AUDIO_EQ_TUNING
#ifdef ANC_APP
        anc_status_record = 0xff;
        bt_audio_updata_eq_for_anc(app_anc_work_status());
#else   // #ifdef ANC_APP
#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,0));
#endif
#endif  // #ifdef ANC_APP
#endif  // #ifndef AUDIO_EQ_TUNING

#if defined(IBRT)
        APP_TWS_IBRT_AUDIO_SYNC_CFG_T sync_config;
        sync_config.factor_reference  = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;
        sync_config.factor_fast_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_FAST_LIMIT;
        sync_config.factor_slow_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_SLOW_LIMIT;;
        sync_config.dead_zone_us      = TWS_IBRT_AUDIO_SYNC_DEAD_ZONE_US;
        app_tws_ibrt_audio_sync_reconfig(&sync_config);
#else
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
        af_codec_tune(AUD_STREAM_PLAYBACK, 0);
#endif
#endif
        if (on == PLAYER_OPER_START)
        {
            // This might use all of the rest buffer in the mempool,
            // so it must be the last configuration before starting stream.
#if (A2DP_DECODER_VER == 2)
            A2DP_AUDIO_OUTPUT_CONFIG_T output_config;
            A2DP_AUDIO_CODEC_TYPE a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
            output_config.sample_rate = sample_rate;
            output_config.num_channels = 2;
            output_config.bits_depth = stream_cfg.bits;
            output_config.frame_samples = app_bt_stream_get_dma_buffer_samples()/2;
            output_config.factor_reference = 1.0f;
#if defined(IBRT)
            uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
            POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
            ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
            float dest_packet_mut = 0;
            int need_autotrigger = a2dp_ibrt_stream_need_autotrigger_getandclean_flag();
            uint32_t offset_mut = 0;
            uint8_t codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
            if(app_stream_get_a2dp_param_callback)
            {
                codec_type = app_stream_get_a2dp_param_callback(device_id);
            }
            else
            {
                codec_type = bt_sbc_player_get_codec_type();
            }
            switch (codec_type)
            {
                case BTIF_AVDTP_CODEC_TYPE_SBC:
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
//                    dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU;
                    dest_packet_mut = app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_SBC);
            #ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
                    if(a2dp_player_playback_waterline_is_enalbe() == false){
                        offset_mut = 0;
                    }else{
                        offset_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU * A2DP_PLAYER_PLAYBACK_WATER_LINE;
                    }
            #endif
                    break;
                case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC;
//                    dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU;
                    dest_packet_mut = app_audio_a2dp_player_playback_delay_mtu_get(A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC);
            #ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
                    if(a2dp_player_playback_waterline_is_enalbe() == false){
                        offset_mut = 0;
                    }else{
                        offset_mut =  A2DP_PLAYER_PLAYBACK_WATER_LINE;
                    }
            #endif
                    break;
                case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
#if defined(A2DP_LHDC_ON)
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LHDC;
                    if (a2dp_lhdc_config_llc_get()){
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU;
                    }else if (sample_rate > AUD_SAMPRATE_48000){
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU;
                    }else{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU;
                    }
#endif
#if defined(A2DP_LDAC_ON)
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LDAC;
                    dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU;
#endif


#if defined(A2DP_SCALABLE_ON)
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SCALABL;
                    if (sample_rate > AUD_SAMPRATE_48000){
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU;
                    }else{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU;
                    }
            #ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
                    if(a2dp_player_playback_waterline_is_enalbe() == false){
                        offset_mut = 0;
                    }else{
                        offset_mut =  A2DP_PLAYER_PLAYBACK_WATER_LINE;
                    }
            #endif
#endif
#if defined(A2DP_LC3_ON)
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LC3;
                    if (sample_rate > AUD_SAMPRATE_48000){
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LC3_HIRES_MTU;//todo
                    }else{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LC3_BASERES_MTU;//todo
                    }
#endif
                    break;
                default:
                    break;
            }
            output_config.factor_reference = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;

            dest_packet_mut *= a2dp_audio_latency_factor_get();
            A2DP_AUDIO_CHANNEL_SELECT_E a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
            switch ((AUDIO_CHANNEL_SELECT_E)p_ibrt_ctrl->audio_chnl_sel)
            {
                case AUDIO_CHANNEL_SELECT_STEREO:
                    a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
                    break;
                case AUDIO_CHANNEL_SELECT_LRMERGE:
                    a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
                    break;
                case AUDIO_CHANNEL_SELECT_LCHNL:
                    a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
                    break;
                case AUDIO_CHANNEL_SELECT_RCHNL:
                    a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
                    break;
                default:
                    break;
            }
            dest_packet_mut += offset_mut;
            a2dp_audio_init(freq, a2dp_audio_codec_type, &output_config, a2dp_audio_channel_sel, (uint16_t)dest_packet_mut);

            app_tws_ibrt_audio_analysis_interval_set(sample_rate > AUD_SAMPRATE_48000 ? AUDIO_ANALYSIS_INTERVAL*2 : AUDIO_ANALYSIS_INTERVAL);
            if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
            {
                app_tws_ibrt_audio_analysis_start(0, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
                app_tws_ibrt_audio_sync_start();
                app_bt_stream_ibrt_audio_master_detect_next_packet_start();
            }
            else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
            {
                app_bt_stream_ibrt_audio_slave_detect_next_packet_start(need_autotrigger);
            }
            else
            {
                TRACE_AUD_STREAM_E("[A2DP_PLAYER] mobile_link:%d %04x ibrt_link:%d %04x", \
                    APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), \
                    APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
                    APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote),
                    APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
            }
#else
            uint8_t codec_type = BTIF_AVDTP_CODEC_TYPE_SBC;
            if(app_stream_get_a2dp_param_callback)
            {
                uint8_t current_device_id = app_bt_audio_get_curr_a2dp_device();
                codec_type = app_stream_get_a2dp_param_callback(current_device_id);
            }
            else
            {
                codec_type = bt_sbc_player_get_codec_type();
            }
            uint16_t dest_packet_mut = 0;

            switch (codec_type)
            {
                case BTIF_AVDTP_CODEC_TYPE_SBC:
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
                    dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC;
                    dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU;
                    break;
                case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
#if defined(A2DP_LHDC_ON)||defined(A2DP_SCALABLE_ON)
                    a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LHDC;
                    if (a2dp_lhdc_config_llc_get())
					{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU;
                    }
					else if (sample_rate > AUD_SAMPRATE_48000)
					{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU;
                    }
					else
					{
                        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU;
                    }
#endif
                    break;
                default:
                    break;
            }

            a2dp_audio_init(freq,a2dp_audio_codec_type, &output_config, A2DP_AUDIO_CHANNEL_SELECT_STEREO, dest_packet_mut);
            a2dp_audio_detect_next_packet_callback_register(app_bt_stream_detect_next_packet_cb);
#endif
            a2dp_audio_start();
#else
            a2dp_audio_init();
#endif
        }

#if defined(MUSIC_DELAY_CONTROL) && (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)|| defined(CHIP_BEST1400)|| defined(CHIP_BEST1402))
        calib_reset = 1;
#endif

#if 1
        osThreadId localThread = osThreadGetId();
        osPriority currentPriority = osThreadGetPriority(localThread);
        osThreadSetPriority(localThread, osPriorityRealtime);
#endif

        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
        }

        af_codec_set_playback_post_handler(bt_sbc_player_playback_post_handler);

#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
        app_bt_stream_trigger_init();
#endif

#if defined(VOICE_DEV)
        uint32_t algos = VOICE_DEV_ALGO_AEC;
        // TODO: Based on A2DP. SBC: 16ms; AAC: 24ms
        uint32_t algos_frame_len = 256;

#if defined(APP_NOISE_ESTIMATION)
        algos |= VOICE_DEV_ALGO_NOISE_EST;
#endif

        voice_dev_ctl(VOICE_DEV_USER0, VOICE_DEV_SET_ALGO_FRAME_LEN, &algos_frame_len);
        voice_dev_ctl(VOICE_DEV_USER0, VOICE_DEV_SET_SUPPORT_ALGOS, &algos);

#if defined(VOICE_DEV_TEST)
        test_voice_dev_open();
        test_voice_dev_start();
#endif

#if defined(APP_NOISE_ESTIMATION)
        app_noise_estimation_open();
        app_noise_estimation_start();
#endif
#endif

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        osThreadSetPriority(localThread, currentPriority);

#ifdef AI_AEC_CP_ACCEL
    cp_aec_init();
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
		#ifdef AUDIO_ANC_FB_ADJ_MC
        af_stream_start(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
	    #endif
#endif
        app_bt_stream_trigger_checker_start();
    }

    a2dp_is_run = (on != PLAYER_OPER_STOP);
    a2dp_audio_status_updated_callback(a2dp_is_run);
    return 0;
}

#if defined(SCO_DMA_SNAPSHOT)
static uint32_t sco_trigger_wait_codecpcm = 0;
static uint32_t sco_trigger_wait_btpcm = 0;
void app_bt_stream_sco_trigger_set_codecpcm_triggle(uint8_t triggle_en)
{
    sco_trigger_wait_codecpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_codecpcm_triggle(void)
{
    return sco_trigger_wait_codecpcm;
}

void app_bt_stream_sco_trigger_set_btpcm_triggle(uint32_t triggle_en)
{
    sco_trigger_wait_btpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_btpcm_triggle(void)
{
    return sco_trigger_wait_btpcm;
}

int app_bt_stream_sco_trigger_codecpcm_tick(void)
{
    if(app_bt_stream_sco_trigger_wait_codecpcm_triggle()){
        app_bt_stream_sco_trigger_set_codecpcm_triggle(0);
#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x/%x", btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote)));
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x/%x", btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)));
    }
    else
    {
        TRACE_AUD_STREAM_E("[SCO_PLAYER] codecpcm_tick mobile_link:%d %04x ibrt_link:%d %04x", APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), \
                                                    APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
                                                    APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote), \
                                                    APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID)
    {
        conhdl = btif_hf_get_remote_hci_handle(app_audio_manager_get_active_sco_chnl());
    }
    if (conhdl != 0xFFFF){
        TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x", bt_syn_get_curr_ticks(conhdl));
    }
#endif
        btdrv_syn_clr_trigger(0);
        bt_syn_cancel_tg_ticks(0);
        return 1;
    }
    return 0;
}

int app_bt_stream_sco_trigger_btpcm_tick(void)
{
    if(app_bt_stream_sco_trigger_wait_btpcm_triggle()){
        app_bt_stream_sco_trigger_set_btpcm_triggle(0);
        btdrv_set_bt_pcm_triggler_en(0);
#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:%x/%x", btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote)));
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:%x/%x", btdrv_syn_get_curr_ticks(), bt_syn_get_curr_ticks(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote)));
    }
    else
    {
        TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick: mobile_link:%d %04x ibrt_link:%d %04x", \
                                        APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), \
                                        APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
                                        APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote), \
                                        APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID)
    {
        conhdl = btif_hf_get_remote_hci_handle(app_audio_manager_get_active_sco_chnl());
    }
    TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:%x", bt_syn_get_curr_ticks(conhdl));
#endif
        btdrv_syn_clr_trigger(0);
        bt_syn_cancel_tg_ticks(0);
        return 1;
    }
    return 0;
}

void app_bt_stream_sco_trigger_btpcm_start(void )
{
    uint32_t curr_ticks = 0;
    uint32_t tg_acl_trigger_offset_time = 0;
    uint16_t conhdl = 0xFFFF;

    uint32_t lock;

#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote)){
        conhdl = APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        conhdl = APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote);
    }
    else
    {
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start mobile_link:%d %04x ibrt_link:%d %04x", \
            APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), \
            APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote),APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
#else
    int curr_sco;
    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID)
    {
        conhdl = btif_hf_get_remote_hci_handle(app_audio_manager_get_active_sco_chnl());
    }
#endif

    lock = int_lock();
    curr_ticks = bt_syn_get_curr_ticks(conhdl);

    tg_acl_trigger_offset_time = (curr_ticks+0x180) - ((curr_ticks+0x180)%192);

    btdrv_set_bt_pcm_triggler_en(0);
    btdrv_set_bt_pcm_en(0);
    btdrv_syn_clr_trigger(0);
    btdrv_enable_playback_triggler(SCO_TRIGGLE_MODE);

#if defined(IBRT)
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start set ticks:%d,",tg_acl_trigger_offset_time);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote)){
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start set ticks:%d,",tg_acl_trigger_offset_time);
    }
    else
    {
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start mobile_link:%d %04x ibrt_link:%d %04x", \
            APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote),
            APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote),APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start get ticks:%d,",curr_ticks);

#else
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE,0,false);
#endif
    btdrv_set_bt_pcm_triggler_en(1);
    btdrv_set_bt_pcm_en(1);
    app_bt_stream_sco_trigger_set_btpcm_triggle(1);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_start curr clk=%x, triggle_clk=%x, bt_clk=%x",
                                                                       btdrv_syn_get_curr_ticks(),
                                                                       tg_acl_trigger_offset_time,
                                                                       bt_syn_get_curr_ticks(conhdl));
    int_unlock(lock);
}

void app_bt_stream_sco_trigger_btpcm_stop(void)
{
    return;
}


#define TIRG_DELAY_THRESHOLD_325US (15) //total:TIRG_DELAY_THRESHOLD_325US*325us
#define TIRG_DELAY_MAX  (20) //total:20*TIRG_DELAY_325US*325us

#define TIRG_DELAY_325US (96) //total:TIRG_DELAY_325US*325us It' up to the codec and bt pcm pingpang buffer.

void app_bt_stream_sco_trigger_codecpcm_start(uint32_t btclk, uint16_t btcnt )
{
    uint32_t curr_ticks = 0;
    uint32_t tg_acl_trigger_offset_time = 0;
    uint32_t lock;
    uint16_t conhdl = 0xFFFF;
    //must lock the interrupts when set trig ticks.

#if defined(IBRT)
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        conhdl = APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        conhdl = APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote);
    }
    else
    {
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start mobile_link:%d %04x ibrt_link:%d %04x",
            APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote),
            APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote),   APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
#else
    int curr_sco;
    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID)
    {
        conhdl = btif_hf_get_remote_hci_handle(app_audio_manager_get_active_sco_chnl());
    }
#endif

    lock = int_lock();
    curr_ticks = bt_syn_get_curr_ticks(conhdl);
    TRACE_AUD_STREAM_I("[[STRM_TRIG][SCO] codecpcm_start 1 curr:%d clk:%dcnt:%d", curr_ticks,btclk,btcnt);

#ifdef LOW_DELAY_SCO
    tg_acl_trigger_offset_time=btclk+BUF_BTCLK_NUM+CODEC_TRIG_BTCLK_OFFSET;
#else
    tg_acl_trigger_offset_time=btclk+BUF_BTCLK_NUM+CODEC_TRIG_BTCLK_OFFSET;
#endif

#if defined(__FPGA_BT_1500__) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)

#else
    tg_acl_trigger_offset_time = tg_acl_trigger_offset_time * 2;
#endif

    if(tg_acl_trigger_offset_time<curr_ticks+TIRG_DELAY_THRESHOLD_325US)
    {
         int tirg_delay=0;
	  tirg_delay=((curr_ticks+TIRG_DELAY_THRESHOLD_325US)-tg_acl_trigger_offset_time)/TIRG_DELAY_325US;
	  tirg_delay=tirg_delay+1;
	  if(tirg_delay>TIRG_DELAY_MAX)
	  {
		tirg_delay=TIRG_DELAY_MAX;
            TRACE_AUD_STREAM_W("[STRM_TRIG][SCO] codecpcm_start bt clk convolution!");
	  }
	  tg_acl_trigger_offset_time=tg_acl_trigger_offset_time+tirg_delay*TIRG_DELAY_325US;
        TRACE_AUD_STREAM_W("[STRM_TRIG][SCO] codecpcm_start need more tirg_delay:%d offset:%d curr:%d,",tirg_delay,tg_acl_trigger_offset_time,curr_ticks);
    }
    tg_acl_trigger_offset_time &= 0x0fffffff;

    btdrv_syn_trigger_codec_en(0);
//    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
//    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
    btdrv_syn_clr_trigger(0);


#if defined(IBRT)
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start set 2 tg_acl_trigger_offset_time:%d",tg_acl_trigger_offset_time);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote), BT_TRIG_SLAVE_ROLE,0,false);
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start set 2 tg_acl_trigger_offset_time:%d",tg_acl_trigger_offset_time);
    }
    else
    {
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start mobile_link:%d %04x ibrt_link:%d %04x",
            APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote), APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote),
            APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote),   APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote));
    }
#else
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE,0,false);
#endif
    btdrv_syn_trigger_codec_en(1);
    app_bt_stream_sco_trigger_set_codecpcm_triggle(1);

    btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

    int_unlock(lock);

    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start enable curr:%x trig:%x curr:%x",
                                                                       btdrv_syn_get_curr_ticks(),
                                                                       tg_acl_trigger_offset_time,
                                                                       curr_ticks);



//    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
//    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);


}

void app_bt_stream_sco_trigger_codecpcm_stop(void)
{
#ifdef PLAYBACK_USE_I2S
    af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, false);
#else
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
#endif
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
}
#endif

void speech_tx_aec_set_frame_len(int len);
int voicebtpcm_pcm_echo_buf_queue_init(uint32_t size);
void voicebtpcm_pcm_echo_buf_queue_reset(void);
void voicebtpcm_pcm_echo_buf_queue_deinit(void);
#if defined(SCO_DMA_SNAPSHOT)
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int vqe_sample_rate, int codec_sample_rate, int capture_channel_num);
#else
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int codec_sample_rate);
#endif
int voicebtpcm_pcm_audio_deinit(void);
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len);
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);
static uint32_t mic_force_mute = 0;
static uint32_t spk_force_mute = 0;

static enum AUD_CHANNEL_NUM_T sco_play_chan_num;
static enum AUD_CHANNEL_NUM_T sco_cap_chan_num;

static hfp_sco_codec_t g_bt_sco_codec_type = BTIF_HF_SCO_CODEC_NONE;

static void bt_sco_codec_store(void)
{
    g_bt_sco_codec_type = app_audio_manager_get_scocodecid();

    if (g_bt_sco_codec_type == BTIF_HF_SCO_CODEC_NONE) {
        TRACE(2, "[%s] WARNING:%d is invalid sco codec type, use default codec type!", __func__, g_bt_sco_codec_type);
        g_bt_sco_codec_type = BTIF_HF_SCO_CODEC_CVSD;
    }
    TRACE(0, "[%s] Codec Type: %d", __func__, g_bt_sco_codec_type);
}

hfp_sco_codec_t bt_sco_codec_get_type(void)
{
    return g_bt_sco_codec_type;
}

extern "C" bool bt_sco_codec_is_msbc(void)
{
    if (g_bt_sco_codec_type == BTIF_HF_SCO_CODEC_MSBC) {
        return true;
    } else {
        return false;
    }
}

extern "C" bool bt_sco_codec_is_cvsd(void)
{
    if (g_bt_sco_codec_type == BTIF_HF_SCO_CODEC_CVSD) {
        return true;
    } else {
        return false;
    }
}

void bt_sco_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                                     uint32_t *mobile_master_clk, uint16_t *mobile_master_cnt)
{
#if defined(IBRT)
    uint8_t curr_sco_device = app_bt_audio_get_curr_sco_device();
    app_tws_ibrt_audio_mobile_clkcnt_get(curr_sco_device, btclk, btcnt, mobile_master_clk, mobile_master_cnt);
#else
    uint16_t conhdl = 0xFFFF;
    int32_t clock_offset;
    uint16_t bit_offset;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_INVALID_ID){
        conhdl = btif_hf_get_remote_hci_handle(app_audio_manager_get_active_sco_chnl());
    }

    if (conhdl != 0xFFFF){
        bt_drv_reg_op_piconet_clk_offset_get(conhdl, &clock_offset, &bit_offset);
        //TRACE_AUD_STREAM_I("mobile piconet clk:%d bit:%d loc clk:%d cnt:%d", clock_offset, bit_offset, btclk, btcnt);
        btdrv_slave2master_clkcnt_convert(btclk, btcnt,
                                          clock_offset, bit_offset,
                                          mobile_master_clk, mobile_master_cnt);
    }else{
        TRACE_AUD_STREAM_W("[STRM_TRIG][SCO] mobile_clkcnt_get warning conhdl NULL conhdl:%x", conhdl);
        *mobile_master_clk = 0;
        *mobile_master_cnt = 0;
    }
#endif
}


#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)

#ifdef CHIP_BEST1000
#error "Unsupport SW_SCO_RESAMPLE on best1000 by now"
#endif
#ifdef NO_SCO_RESAMPLE
#error "Conflicted config: NO_SCO_RESAMPLE and SW_SCO_RESAMPLE"
#endif

// The decoded playback data in the first irq is output to DAC after the second irq (PING-PONG buffer)
#define SCO_PLAY_RESAMPLE_ALIGN_CNT     2

static uint8_t sco_play_irq_cnt;
static bool sco_dma_buf_err;
static struct APP_RESAMPLE_T *sco_capture_resample;
static struct APP_RESAMPLE_T *sco_playback_resample;

static int bt_sco_capture_resample_iter(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_data_come(buf, len);
    return 0;
}

static int bt_sco_playback_resample_iter(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_more_data(buf, len);
    return 0;
}

#endif
#if defined(SCO_DMA_SNAPSHOT)

extern int process_downlink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t out_len);
extern int process_uplink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len, uint8_t *ref_buf, uint32_t ref_len, uint8_t *out_buf, uint32_t out_len);
#define MSBC_FRAME_LEN (60)
#define PCM_LEN_PER_FRAME (240)

#define CAL_FRAME_NUM (22)



static void bt_sco_codec_sync_tuning(void)
{
    uint32_t btclk;
    uint16_t btcnt;

    uint32_t mobile_master_clk;
    uint16_t mobile_master_cnt;

    uint32_t mobile_master_clk_offset;
    int32_t mobile_master_cnt_offset;

    static float fre_offset=0.0f;

#if defined(SCO_TUNING_NEWMETHOD)
    static float fre_offset_long_time=0.0f;
    static int32_t fre_offset_flag=0;
#endif

    static int32_t mobile_master_cnt_offset_init;
    static int32_t mobile_master_cnt_offset_old;
    static uint32_t first_proc_flag=0;
#if defined( __AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
    static uint32_t frame_counter=0;
    static int32_t mobile_master_cnt_offset_max=0;
    static int32_t mobile_master_cnt_offset_min=0;
    static int32_t mobile_master_cnt_offset_resample=0;

     int32_t offset_max=0;
     int32_t offset_min=0;
#endif

#if defined(CHIP_BEST1501) || defined(CHIP_BEST1501SIMU) || defined(CHIP_BEST2003)
    uint8_t adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xF);
    }
#else
    bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
#endif
    bt_sco_mobile_clkcnt_get(btclk, btcnt,
                                             &mobile_master_clk, &mobile_master_cnt);

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);
#endif

    if((mobile_master_clk < last_mobile_master_clk)&&(mobile_master_clk < BUF_BTCLK_NUM*2))
    {
        //clock wrapped, 0x555556*24=0x8000010, so after clock wrap, offset init need add 16
        mobile_master_clk_offset_init = (mobile_master_clk_offset_init+WRAPED_CLK_OFFSET)%BUF_BTCLK_NUM;
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO]:MAX_CLOCK reach btclk:%d,last btclk:%d",mobile_master_clk,last_mobile_master_clk);
    }
    last_mobile_master_clk = mobile_master_clk;

    mobile_master_clk_offset=(mobile_master_clk-mobile_master_clk_offset_init+BUF_BTCLK_NUM)%BUF_BTCLK_NUM;
    mobile_master_cnt_offset=(int32_t)(mobile_master_clk_offset*BTCLK_UNIT+(625-mobile_master_cnt)*BTCNT_UNIT);
    mobile_master_cnt_offset=(int32_t)(mobile_master_cnt_offset-(CODEC_TRIG_DELAY_OFFSET+mobile_master_cnt_offset_init));

    if(app_bt_stream_sco_trigger_codecpcm_tick())
    {
        fre_offset=0.0f;

#if defined(SCO_TUNING_NEWMETHOD)
        fre_offset_long_time=0.0f;
        fre_offset_flag=0;
#endif

        mobile_master_cnt_offset_old=0;
        first_proc_flag=0;

       if(playback_samplerate_codecpcm==AUD_SAMPRATE_16000)
       {
#ifdef  ANC_APP

#if defined( __AUDIO_RESAMPLE__)
        if (hal_cmu_get_audio_resample_status())
        {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
             mobile_master_cnt_offset_init=171;
#else
             mobile_master_cnt_offset_init=107;
#endif
        }
        else
#endif
        {
            mobile_master_cnt_offset_init=90;
        }

#else

#if defined( __AUDIO_RESAMPLE__)
        if (hal_cmu_get_audio_resample_status())
        {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
             mobile_master_cnt_offset_init=171;
#else
             mobile_master_cnt_offset_init=146;
#endif
        }
        else
#endif
        {
            mobile_master_cnt_offset_init=113;
        }

#endif
       }
       else if(playback_samplerate_codecpcm==AUD_SAMPRATE_8000)
       {
#ifdef  ANC_APP
#if defined( __AUDIO_RESAMPLE__)
        if (hal_cmu_get_audio_resample_status())
        {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
             mobile_master_cnt_offset_init=-270;
#else
             mobile_master_cnt_offset_init=-468;
#endif
        }
        else
#endif
        {
            mobile_master_cnt_offset_init=-445;
        }
#else
#if defined( __AUDIO_RESAMPLE__)
        if (hal_cmu_get_audio_resample_status())
        {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
             mobile_master_cnt_offset_init=-270;
#else
             mobile_master_cnt_offset_init=-327;
#endif
        }
        else
#endif
        {
            mobile_master_cnt_offset_init=-386;
        }
#endif
       }
        mobile_master_cnt_offset=(int32_t)(mobile_master_clk_offset*BTCLK_UNIT+(625-mobile_master_cnt)*BTCNT_UNIT);
        mobile_master_cnt_offset=(int32_t)(mobile_master_cnt_offset-(CODEC_TRIG_DELAY_OFFSET+mobile_master_cnt_offset_init));
        TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d,",mobile_master_cnt_offset);

#if defined( __AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
#ifdef LOW_DELAY_SCO
        fre_offset=(float)mobile_master_cnt_offset/(CAL_FRAME_NUM*7.5f*1000.0f);
#else
        fre_offset=(float)mobile_master_cnt_offset/(CAL_FRAME_NUM*15.0f*1000.0f);
#endif
#endif

#if defined( __AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
      if (hal_cmu_get_audio_resample_status())
      {
            frame_counter=0;
            mobile_master_cnt_offset_max=0;
            mobile_master_cnt_offset_min=0;
            mobile_master_cnt_offset_resample=0;
      }
#endif
    }


#if defined(  __AUDIO_RESAMPLE__) &&!defined(SW_PLAYBACK_RESAMPLE)&& !defined(AUDIO_RESAMPLE_ANTI_DITHER)
      if (hal_cmu_get_audio_resample_status())
      {
           if(playback_samplerate_codecpcm==AUD_SAMPRATE_16000)
           {
            offset_max=28;
            offset_min=-33;
           }
           else if(playback_samplerate_codecpcm==AUD_SAMPRATE_8000)
           {
            offset_max=12;
            offset_min=-112;
           }

           if(mobile_master_cnt_offset>mobile_master_cnt_offset_max)
           {
            mobile_master_cnt_offset_max=mobile_master_cnt_offset;
           }

           if(mobile_master_cnt_offset<mobile_master_cnt_offset_min)
           {
            mobile_master_cnt_offset_min=mobile_master_cnt_offset;
           }

            frame_counter++;

            if(frame_counter>=CAL_FRAME_NUM)
            {
               if(mobile_master_cnt_offset_min<offset_min)
               {
                    mobile_master_cnt_offset_resample=mobile_master_cnt_offset_min-offset_min;
               }
               else if(mobile_master_cnt_offset_max>offset_max)
               {
                   mobile_master_cnt_offset_resample=mobile_master_cnt_offset_max-offset_max;
               }
               else
               {
                   mobile_master_cnt_offset_resample=0;
               }
              TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d/%d",mobile_master_cnt_offset_min,mobile_master_cnt_offset_max);
               mobile_master_cnt_offset=mobile_master_cnt_offset_resample;

            if(first_proc_flag==0)
            {
                  fre_offset=((int32_t)(mobile_master_cnt_offset*0.5f))*0.0000001f
                    +(mobile_master_cnt_offset-mobile_master_cnt_offset_old)*0.0000001f;
                  first_proc_flag=1;
            }
            else
            {
                  fre_offset=fre_offset+((int32_t)(mobile_master_cnt_offset*0.5f))*0.0000001f
                    +(mobile_master_cnt_offset-mobile_master_cnt_offset_old)*0.0000001f;
                  first_proc_flag=1;
            }

            mobile_master_cnt_offset_old=mobile_master_cnt_offset;
#if defined(SCO_DMA_SNAPSHOT_DEBUG)
               TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d", mobile_master_cnt_offset);
#endif
               mobile_master_cnt_offset_max=0;
               mobile_master_cnt_offset_min=0;
               frame_counter=0;
            }
        }
        else
#endif
        {
#if defined(SCO_TUNING_NEWMETHOD)
            if(mobile_master_cnt_offset>0)
            {
#if defined(LOW_DELAY_SCO)
                fre_offset=0.00005f+fre_offset_long_time;
#else
                fre_offset=0.0001f+fre_offset_long_time;
#endif
                first_proc_flag=0;
                fre_offset_flag=1;
                if(mobile_master_cnt_offset>=mobile_master_cnt_offset_old)
                {
                    fre_offset_long_time=fre_offset_long_time+0.0000001f;
                    fre_offset_flag=0;
                }
            }
            else if(mobile_master_cnt_offset<0)
            {
#if defined(LOW_DELAY_SCO)
                fre_offset=-0.00005f+fre_offset_long_time;
#else
                fre_offset=-0.0001f+fre_offset_long_time;
#endif
                first_proc_flag=0;
                fre_offset_flag=-1;
                if(mobile_master_cnt_offset<=mobile_master_cnt_offset_old)
                {
                    fre_offset_long_time=fre_offset_long_time-0.0000001f;
                    fre_offset_flag=0;
                }
            }
            else
            {
                fre_offset=fre_offset_long_time+fre_offset_flag*0.0000001f;
                fre_offset_long_time=fre_offset;
                first_proc_flag=1;
                fre_offset_flag=0;
            }

            if(fre_offset_long_time>0.0001f)fre_offset_long_time=0.0001f;
            if(fre_offset_long_time<-0.0001f)fre_offset_long_time=-0.0001f;

            mobile_master_cnt_offset_old=mobile_master_cnt_offset;
#if defined(SCO_DMA_SNAPSHOT_DEBUG)
            {
                int freq_ppm_int=(int)(fre_offset_long_time*1000000.0f);
                int freq_ppm_Fra=(int)(fre_offset_long_time*10000000.0f)-((int)(fre_offset_long_time*1000000.0f))*10;
                if(freq_ppm_Fra<0)freq_ppm_Fra=-freq_ppm_Fra;

                TRACE(1,"freq_offset_long_time(ppm):%d.%d",freq_ppm_int,freq_ppm_Fra);
            }
#endif

#else
            if(mobile_master_cnt_offset>5&&first_proc_flag==0)
            {
                fre_offset=0.0001f;
            }
            else if(mobile_master_cnt_offset<-5&&first_proc_flag==0)
            {
                fre_offset=-0.0001f;
            }
            else
            {
                if(first_proc_flag==0)
                {
                    fre_offset=0;
                }
                fre_offset=fre_offset+((int32_t)(mobile_master_cnt_offset*0.5f))*0.00000001f
                    +(mobile_master_cnt_offset-mobile_master_cnt_offset_old)*0.00000001f;

                mobile_master_cnt_offset_old=mobile_master_cnt_offset;
                first_proc_flag=1;
            }
#endif
        }

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
        {
            POSSIBLY_UNUSED int freq_ppm_int=(int)(fre_offset*1000000.0f);
            int freq_ppm_Fra=(int)(fre_offset*10000000.0f)-((int)(fre_offset*1000000.0f))*10;
            if(freq_ppm_Fra<0)freq_ppm_Fra=-freq_ppm_Fra;
            TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] time_offset(us):%d",mobile_master_cnt_offset);
            TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] freq_offset(ppm):%d.%d",freq_ppm_int,freq_ppm_Fra);
        }
#endif
        if(first_proc_flag==1)
       {
            if(fre_offset>0.0001f)fre_offset=0.0001f;
            if(fre_offset<-0.0001f)fre_offset=-0.0001f;
       }
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    app_resample_tune(playback_samplerate_codecpcm, fre_offset);
#else
    af_codec_tune(AUD_STREAM_NUM, fre_offset);
#endif

    return;
}
#endif
extern CQueue* get_tx_esco_queue_ptr();

#if defined(ANC_NOISE_TRACKER)
static int16_t *anc_buf = NULL;
#endif

#if defined(SCO_DMA_SNAPSHOT)
// This is reletive with chip.
#ifdef TX_RX_PCM_MASK
#define BTPCM_TX_OFFSET_BYTES   (0)
#else
#if defined (PCM_FAST_MODE)
#define BTPCM_TX_OFFSET_BYTES   (1)
#else
#define BTPCM_TX_OFFSET_BYTES   (2)
#endif
#endif

#define SCO_MSBC_PACKET_SIZE    (60)

#ifndef PCM_PRIVATE_DATA_FLAG
static uint8_t g_btpcm_tx_buf[BTPCM_TX_OFFSET_BYTES];
#endif
static void adjust_btpcm_msbc_tx(uint8_t *buf, uint32_t len)
{
    uint16_t *buf_u16 = (uint16_t *)buf;
#ifndef PCM_PRIVATE_DATA_FLAG
    uint8_t *buf_ptr = (uint8_t *)buf;
    uint32_t loop_cnt = len / SCO_MSBC_PACKET_SIZE;

    ASSERT(len % SCO_MSBC_PACKET_SIZE == 0, "[%s] len(%d) is invalid!", __func__, len);

    // Shift
    for (uint32_t cnt=0; cnt <loop_cnt; cnt++) {
        for (uint32_t i=0; i<BTPCM_TX_OFFSET_BYTES; i++) {
            g_btpcm_tx_buf[i] = buf_ptr[i];
        }

        for (uint32_t i=0; i<SCO_MSBC_PACKET_SIZE - BTPCM_TX_OFFSET_BYTES; i++) {
            buf_ptr[i] = buf_ptr[i + BTPCM_TX_OFFSET_BYTES];
        }

        for (uint32_t i=0; i<BTPCM_TX_OFFSET_BYTES; i++) {
            buf_ptr[i + SCO_MSBC_PACKET_SIZE - BTPCM_TX_OFFSET_BYTES] = g_btpcm_tx_buf[i];
        }

        buf_ptr += SCO_MSBC_PACKET_SIZE;
    }
#endif
    // BTCPM trans data with 16bits format and valid data is in high 8 bits.
    for (int32_t i = len-1; i >= 0; i--) {
        buf_u16[i] = ((int16_t)buf[i]) << 8;
    }

#if 0
    TRACE(0, "[%s] %x, %x, %x, %x, %x, %x | %x, %x, %x, %x, %x, %x,", __func__,
                                                        buf_u16[0],
                                                        buf_u16[1],
                                                        buf_u16[2],
                                                        buf_u16[3],
                                                        buf_u16[4],
                                                        buf_u16[5],
                                                        buf_u16[54],
                                                        buf_u16[55],
                                                        buf_u16[56],
                                                        buf_u16[57],
                                                        buf_u16[58],
                                                        buf_u16[59]);
    TRACE(0, "[%s] %x, %x, %x, %x, %x, %x | %x, %x, %x, %x, %x, %x,", __func__,
                                                        buf_u16[60],
                                                        buf_u16[61],
                                                        buf_u16[62],
                                                        buf_u16[63],
                                                        buf_u16[64],
                                                        buf_u16[65],
                                                        buf_u16[114],
                                                        buf_u16[115],
                                                        buf_u16[116],
                                                        buf_u16[117],
                                                        buf_u16[118],
                                                        buf_u16[119]);
#endif
}
#endif

//#define BT_SCO_HANDLER_PROFILE

//( codec:mic-->btpcm:tx
// codec:mic
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len)
{
    app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE);

#if defined(ANC_NOISE_TRACKER)
    int16_t *pcm_buf = (int16_t *)buf;
    uint32_t pcm_len = len / sizeof(short);
    uint32_t ch_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM + ANC_NOISE_TRACKER_CHANNEL_NUM;
    uint32_t remain_ch_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM;


#if defined(SPEECH_TX_AEC_CODEC_REF)
    ch_num += 1;
    remain_ch_num += 1;
#endif

    ASSERT(pcm_len % ch_num == 0, "[%s] input data length error", __FUNCTION__);

    // assume anc mic in ch0
    for (uint32_t i = 0, j = 0; i < pcm_len; i += ch_num, j += ANC_NOISE_TRACKER_CHANNEL_NUM)
    {
        for (uint32_t ch = 0; ch < ANC_NOISE_TRACKER_CHANNEL_NUM; ch++)
            anc_buf[j + ch] = pcm_buf[i + ch];
    }

    noise_tracker_process(anc_buf, pcm_len / ch_num * ANC_NOISE_TRACKER_CHANNEL_NUM);

    for (uint32_t i = 0, j = 0; i < pcm_len; i += ch_num, j += remain_ch_num)
    {
        for (uint32_t chi = ANC_NOISE_TRACKER_CHANNEL_NUM, cho = 0; chi < ch_num; chi++, cho++)
            pcm_buf[j + cho] = pcm_buf[i + chi];
    }

    len = len / ch_num * remain_ch_num;
#endif

#if defined(SPEECH_DEBUG_ADC_I2S_SYNC)
    speech_bone_sensor_debug_sync(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif

#if defined(ANC_ASSIST_ENABLED)
		if(app_anc_assist_is_runing()) {
		    app_anc_assist_process(buf, len);
		}
        app_anc_assist_parser_app_mic_buf(buf, &len);
#endif

    if (mic_force_mute || btapp_hfp_mic_need_skip_frame() || btapp_hfp_need_mute())
    {
        memset(buf, 0, len);
    }

#if defined(SCO_DMA_SNAPSHOT)

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

    int pingpang;

    //processing  ping pang flag
    if(buf==capture_buf_codecpcm)
    {
     pingpang=0;
    }
    else
    {
     pingpang=1;
    }

#ifndef PCM_PRIVATE_DATA_FLAG
    uint16_t *playback_dst=(uint16_t *)(playback_buf_btpcm+(pingpang)*playback_size_btpcm/2);
    uint16_t *playback_src=(uint16_t *)playback_buf_btpcm_cache;

    for(uint32_t  i =0; i<playback_size_btpcm/4; i++)
    {
        playback_dst[i]=playback_src[i];
    }
#endif

#ifdef TX_RX_PCM_MASK
    //processing btpcm.(It must be from CPU's copy )
   if(btdrv_is_pcm_mask_enable()==1&&bt_sco_codec_is_msbc())
   {
	uint32_t lock;
	uint32_t i;
	//must lock the interrupts when exchanging data.
	lock = int_lock();
	uint16_t *playback_src=(uint16_t *)(playback_buf_btpcm+(pingpang)*playback_size_btpcm/2);
	for( i =0; i<playback_size_btpcm_copy; i++)
	{
		playback_buf_btpcm_copy[i]=(uint8_t)(playback_src[i]>>8);
	}
	int_unlock(lock);
   }
#endif


    //TRACE_AUD_STREAM_I("pcm length:%d",len);

    //processing clock
    bt_sco_codec_sync_tuning();


    //processing mic
    uint8_t *capture_pcm_frame_p = capture_buf_codecpcm + pingpang * capture_size_codecpcm / 2;
    uint8_t *ref_pcm_frame_p = playback_buf_codecpcm + (pingpang^1) * playback_size_codecpcm / 2;

    uint8_t *dst = playback_buf_btpcm_cache;
    uint32_t packet_len = playback_size_btpcm/2;

#if defined(PCM_PRIVATE_DATA_FLAG)
	packet_len=packet_len-(packet_len/BTPCM_TOTAL_DATA_LENGTH)*BTPCM_PRIVATE_DATA_LENGTH;
#endif

    if (bt_sco_codec_is_cvsd()) {
        process_uplink_bt_voice_frames(capture_pcm_frame_p, len, ref_pcm_frame_p, playback_size_codecpcm / 2, dst, packet_len);
    } else {
        process_uplink_bt_voice_frames(capture_pcm_frame_p, len, ref_pcm_frame_p, playback_size_codecpcm / 2, dst, packet_len/2);
        adjust_btpcm_msbc_tx(dst, packet_len/2);
    }

#if defined(PCM_PRIVATE_DATA_FLAG)
    {
        uint8_t *playback_dst=playback_buf_btpcm+(pingpang)*playback_size_btpcm/2;
        uint8_t *playback_src=playback_buf_btpcm_cache;

        for (uint32_t i = 0; i < (playback_size_btpcm/2)/BTPCM_TOTAL_DATA_LENGTH; i++)
        {
		uint8_t *dst=playback_dst+i*BTPCM_TOTAL_DATA_LENGTH+BTPCM_PRIVATE_DATA_LENGTH-(BTPCM_TX_OFFSET_BYTES*2);
		uint8_t *src=playback_src+i*BTPCM_PUBLIC_DATA_LENGTH;
        	memcpy(dst,src,BTPCM_PUBLIC_DATA_LENGTH);
        }
    }
#endif


#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][MIC] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;

#else

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if(hal_cmu_get_audio_resample_status())
    {
        if (af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE)) {
            sco_dma_buf_err = true;
        }
        // The decoded playback data in the first irq is output to DAC after the second irq (PING-PONG buffer),
        // so it is aligned with the capture data after 2 playback irqs.
        if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
            // Skip processing
            return len;
        }
        app_capture_resample_run(sco_capture_resample, buf, len);
    }
    else
#endif
    {
        voicebtpcm_pcm_audio_data_come(buf, len);
    }

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][MIC] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;
#endif
}

#ifdef _SCO_BTPCM_CHANNEL_
// btpcm:tx
static uint32_t bt_sco_btpcm_playback_data(uint8_t *buf, uint32_t len)
{
    app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK);

#if defined(SCO_DMA_SNAPSHOT)
    return len;
#else

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

    get_voicebtpcm_p2m_frame(buf, len);

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][SPK] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;

#endif
}
//)

#if defined(AF_ADC_I2S_SYNC)
void codec_capture_i2s_enable(void)
{
    uint32_t lock;

    TRACE_AUD_STREAM_I("[SCO][IIS] Start...", __func__);

    lock = int_lock();
    hal_codec_capture_enable();
    hal_i2s_enable(HAL_I2S_ID_0);
    int_unlock(lock);

}
#endif

extern CQueue* get_rx_esco_queue_ptr();

static volatile bool is_codec_stream_started = false;
#ifdef PCM_PRIVATE_DATA_FLAG

uint8_t bt_sco_pri_data_get_head_pos(uint8_t *buf, uint32_t len, uint8_t offset, uint8_t frame_num)
{
    uint8_t head_pos = 127;
    uint8_t max_j = 0;
    if(offset==0 && frame_num > 1)
    {
        // 保证至少找到Buf里的一个offset
        max_j = 120;
    }
    else
    {
        max_j = 101;
    }

    for(uint8_t j=0; j<max_j; j++)
    {
        // 查找自定义的私有数据
        if(buf[j+offset]==0xff && buf[j+14+offset]==0xff && buf[j+16+offset]==0xff && buf[j+18+offset]==0xff)
        {
            head_pos=j;
        }
    }
    return head_pos;
}

void bt_sco_btpcm_get_pcm_priv_data(struct PCM_DATA_FLAG_T *pcm_data, uint8_t *buf, uint32_t len)
{
    uint8_t frame_num = len/120;
    for(uint8_t i=0; i<frame_num; i++)
    {
        uint8_t head_pos = 120*i;
        pcm_data[i].offset = 127;
        pcm_data[i].offset = bt_sco_pri_data_get_head_pos(buf, len, head_pos, frame_num);
        if(pcm_data[i].offset == 127)
        {
            TRACE(0,"sco_pri_data: found head offset failed!");
        }
        else
        {
            uint16_t head_offset = head_pos + pcm_data[i].offset;
            pcm_data[i].undef = buf[head_offset];
            pcm_data[i].bitcnt = (buf[head_offset+2]|(buf[head_offset+4]<<8))&0x3ff;
            pcm_data[i].softbit_flag = (buf[head_offset+4]>>5)&3;
            pcm_data[i].btclk = buf[head_offset+6]|(buf[head_offset+8]<<8)|(buf[head_offset+10]<<16)|(buf[head_offset+12]<<24);
            pcm_data[i].reserved = buf[head_offset+14]|(buf[head_offset+16]<<8)|(buf[head_offset+18]<<16)|(buf[head_offset+20]<<24);
            //clear private msg in buffer
            for(uint8_t j=0; j<PCM_PRIVATE_DATA_LENGTH; j++)
                buf[head_offset+2*j] = 0;
        }
    }
}
#endif
//( btpcm:rx-->codec:spk
// btpcm:rx

static uint32_t bt_sco_btpcm_capture_data(uint8_t *buf, uint32_t len)
{
#if 0
    TRACE(0,"[%s] dump source start", __func__);
    uint16_t *source_u16 = (uint16_t *)buf;
    uint16_t pcm_len = len / 2;
#if defined(PCM_PRIVATE_DATA_FLAG)
    DUMP16("%04x,", source_u16, 16);
    hal_sys_timer_delay_us(100);
    source_u16 += 16;
    pcm_len -= 16;
#endif
    ASSERT(pcm_len % 30 == 0, "[%s] input data length error", __FUNCTION__);
    uint16_t dump_cnt = pcm_len / 30;
    for(int i=0; i< dump_cnt; i++){
        DUMP16("%04x,", source_u16, 30);
        hal_sys_timer_delay_us(100);
        source_u16 += 30;
    }
#endif

    app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_BTPCM_CAPTURE);

#if defined(PCM_PRIVATE_DATA_FLAG) && defined(PCM_FAST_MODE)
    //bt_sco_btpcm_get_pcm_priv_data(pcm_data_param, buf, len);
#endif

#if defined(SCO_DMA_SNAPSHOT)
    uint32_t btclk;
    uint16_t btcnt;

    uint32_t mobile_master_clk;
    uint16_t mobile_master_cnt;

    bool  codec_stream_trig = false;
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;

#if defined(LOW_DELAY_SCO)
    if(btpcm_int_counter>0)
    {
       btpcm_int_counter--;
       return len;
    }
#endif

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

    if((is_codec_stream_started == false)&&(buf==capture_buf_btpcm))
    {
        if(!af_stream_buffer_error(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE))
        {
#if defined(CHIP_BEST1501) || defined(CHIP_BEST1501SIMU) || defined(CHIP_BEST2003)
            adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
            if(adma_ch != HAL_DMA_CHAN_NONE)
            {
                bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xF);
            }
#else
            bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
#endif
            bt_sco_mobile_clkcnt_get(btclk, btcnt,&mobile_master_clk, &mobile_master_cnt);
            hal_sys_timer_delay_us(1);
            if(!af_stream_buffer_error(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE))
            {
	          codec_stream_trig = true;
            }
        }
    }
/*
    uint32_t curr_ticks;

    ibbt_ctrl_t *p_ibbt_ctrl = app_tws_ibbt_get_bt_ctrl_ctx();
    if (app_tws_ibbt_tws_link_connected()){
        curr_ticks = bt_syn_get_curr_ticks(IBBT_MASTER == p_ibbt_ctrl->current_role ? p_ibbt_ctrl->mobile_conhandle : p_ibbt_ctrl->ibbt_conhandle);
         TRACE_AUD_STREAM_I("bt_sco_btpcm_capture_data +++++++++++++++++++++++++++++++++curr_ticks:%d,",curr_ticks);
    }else{
        curr_ticks = btdrv_syn_get_curr_ticks();
     TRACE_AUD_STREAM_I("--------------------------------------");
    }
*/
    sco_btpcm_mute_flag=0;

    if (codec_stream_trig) {
        if (app_bt_stream_sco_trigger_btpcm_tick()) {
            af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
            adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
            if(adma_ch != HAL_DMA_CHAN_NONE)
            {
                bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
            }
            af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
            adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
            if(adma_ch != HAL_DMA_CHAN_NONE)
            {
                bt_drv_reg_op_disable_dma_tc(adma_ch&0xF);
            }

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
            af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
            adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
            if(adma_ch != HAL_DMA_CHAN_NONE)
            {
                bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
            }
#endif
            TRACE_AUD_STREAM_I("[SCO][BTPCMRX] buf:%p,capture_buf_btpcm:%p",buf,capture_buf_btpcm);

            // uint16_t *source=(uint16_t *)buf;
            // DUMP16("%02x,", source, MSBC_FRAME_LEN);

            mobile_master_clk_offset_init=mobile_master_clk%BUF_BTCLK_NUM;
            last_mobile_master_clk = mobile_master_clk;
            app_bt_stream_sco_trigger_codecpcm_start(mobile_master_clk,mobile_master_cnt);
            is_codec_stream_started = true;

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
            TRACE_AUD_STREAM_I("[SCO][BTPCMRX] btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);
#endif
        }
    } else {
#if defined(SCO_DMA_SNAPSHOT_DEBUG)
#if defined(CHIP_BEST1501)  || defined(CHIP_BEST1501SIMU) || defined(CHIP_BEST2003)
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt,adma_ch&0xF);
        }
#else
        bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
#endif
        bt_sco_mobile_clkcnt_get(btclk, btcnt,&mobile_master_clk, &mobile_master_cnt);
        TRACE_AUD_STREAM_I("[SCO][BTPCMRX]:btclk:%d,btcnt:%d,",mobile_master_clk,mobile_master_cnt);
#endif
    }

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][BTPCMRX] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;

#else
#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

    if(!is_sco_mode()){
        TRACE_AUD_STREAM_E("[SCO][BTPCMRX] player exit!");
        memset(buf,0x0,len);
        return len;
    }

#if defined(TX_RX_PCM_MASK)
    TRACE_AUD_STREAM_I("[SCO][BTPCMRX] TX_RX_PCM_MASK");
    CQueue* Rx_esco_queue_temp = NULL;
    Rx_esco_queue_temp = get_rx_esco_queue_ptr();
    if(bt_sco_codec_is_msbc() && btdrv_is_pcm_mask_enable() ==1 )
    {
        memset(buf,0,len);
        int status = 0;
        len /= 2;
        uint8_t rx_data[len];
        status = DeCQueue(Rx_esco_queue_temp,rx_data,len);
        for(uint32_t i = 0; i<len; i++)
        {
            buf[2*i+1] = rx_data[i];
        }
        len*=2;
        if(status)
        {
            TRACE_AUD_STREAM_E("[SCO][BTPCMRX] Rx Dec Fail");
         }
    }
#endif

    if (is_codec_stream_started == false) {
        if (bt_sco_codec_is_cvsd())
            hal_sys_timer_delay_us(3000);

        TRACE_AUD_STREAM_I("[SCO][BTPCMRX] start codec %d", FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(SPEECH_BONE_SENSOR)
#if defined(AF_ADC_I2S_SYNC)
        hal_i2s_enable_delay(HAL_I2S_ID_0);
        hal_codec_capture_enable_delay();
#endif
        speech_bone_sensor_start();
#endif

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

#if defined(AF_ADC_I2S_SYNC)
        codec_capture_i2s_enable();
#endif
        is_codec_stream_started = true;

        return len;
    }
    store_voicebtpcm_m2p_buffer(buf, len);

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][BTPCMRX] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;
#endif
}
#endif

#if defined(__BT_ANC__)&&(!defined(SCO_DMA_SNAPSHOT))
static void bt_anc_sco_down_sample_16bits(int16_t *dst, int16_t *src, uint32_t dst_cnt)
{
    for (uint32_t i = 0; i < dst_cnt; i++) {
        dst[i] = src[i * bt_sco_upsampling_ratio * sco_play_chan_num];
    }
}
#endif

static void bt_sco_codec_playback_data_post_handler(uint8_t *buf, uint32_t len, void *cfg)
{
    POSSIBLY_UNUSED struct AF_STREAM_CONFIG_T *config = (struct AF_STREAM_CONFIG_T *)cfg;
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    uint8_t device_id = app_bt_audio_get_curr_sco_device();
#ifdef TWS_PROMPT_SYNC
    tws_playback_ticks_check_for_mix_prompt(device_id);
#endif
    if (audio_prompt_is_playing_ongoing())
    {
        audio_prompt_processing_handler(device_id, len, buf);
    }
#else
    app_ring_merge_more_data(buf, len);
#endif
}

#if defined(HIGH_EFFICIENCY_TX_PWR_CTRL)
#define APP_SCO_LOWER_RSSI_THRESHOLD  (-60)
#define APP_SCO_UPPER_RSSI_THRESHOLD  (-45)

static bool isScoStreamingSwitchedToLowPowerTxGain = true;

void bt_sco_reset_tx_gain_mode(bool sco_on)
{
    isScoStreamingSwitchedToLowPowerTxGain = true;

    app_bt_reset_rssi_collector();

    if(sco_on)
    {
        //sco on set to low power mode
        bt_drv_rf_high_efficency_tx_pwr_ctrl(true, true);
    }
    else
    {
        //sco off set to default value
        bt_drv_rf_high_efficency_tx_pwr_ctrl(false, true);
    }
}

void bt_sco_tx_gain_mode_ajdustment_handler(void)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());

    if (curr_device)
    {
        rx_agc_t mobile_agc;
        bool ret = bt_drv_reg_op_read_rssi_in_dbm(curr_device->acl_conn_hdl, &mobile_agc);
        if (ret)
        {
            int32_t average_rssi = app_bt_tx_rssi_analyzer(mobile_agc.rssi);
            //TRACE(0, "S:    %d  db:  %d  db", average_rssi, mobile_agc.rssi);
            if ((average_rssi < APP_SCO_LOWER_RSSI_THRESHOLD) && isScoStreamingSwitchedToLowPowerTxGain)
            {
                TRACE(0, "switch to normal mode tx gain as rssi is %d", average_rssi);
                isScoStreamingSwitchedToLowPowerTxGain = false;

                bt_drv_rf_high_efficency_tx_pwr_ctrl(false, false);
            }
            else if ((average_rssi >= APP_SCO_UPPER_RSSI_THRESHOLD) && !isScoStreamingSwitchedToLowPowerTxGain)
            {
                TRACE(0, "switch to low power mode tx gain as rssi is %d", average_rssi);
                isScoStreamingSwitchedToLowPowerTxGain = true;

                bt_drv_rf_high_efficency_tx_pwr_ctrl(true, true);
            }
        }
    }
}
#endif

static uint32_t bt_sco_codec_playback_data(uint8_t *buf, uint32_t len)
{
    app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK);

    bt_set_playback_triggered(true);

#if defined(IBRT) && defined(TOTA_v2)
    app_ibrt_ui_rssi_process();
#endif

#ifdef BT_XTAL_SYNC
#ifdef BT_XTAL_SYNC_NEW_METHOD
#ifdef IBRT
    bool valid = false;
    uint32_t bitoffset = 0;
    POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = app_bt_get_device(app_bt_audio_get_curr_sco_device());
    if (APP_IBRT_MOBILE_LINK_CONNECTED(&curr_device->remote))
    {
        valid = true;
        bitoffset = btdrv_rf_bitoffset_get(APP_IBRT_UI_GET_MOBILE_CONNHANDLE(&curr_device->remote) -0x80);
    }
    else if (APP_IBRT_SLAVE_IBRT_LINK_CONNECTED(&curr_device->remote))
    {
        valid = true;
        bitoffset = btdrv_rf_bitoffset_get(APP_IBRT_UI_GET_IBRT_HANDLE(&curr_device->remote) -0x80);
    }

    if(valid)
    {
        bt_xtal_sync_new(bitoffset,app_if_need_fix_target_rxbit(),BT_XTAL_SYNC_MODE_WITH_MOBILE);
    }
#endif
#else
    bt_xtal_sync(BT_XTAL_SYNC_MODE_VOICE);
#endif
#endif

#if defined(SPEECH_DEBUG_ADC_I2S_SYNC)
    speech_bone_sensor_debug_sync(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if defined(SCO_DMA_SNAPSHOT)
    //processing  ping pang flag
    int pingpang;

    if(buf==playback_buf_codecpcm)
    {
     pingpang=0;
    }
    else
    {
     pingpang=1;
    }
#ifdef TX_RX_PCM_MASK
    //processing btpcm.(It must be from CPU's copy )
   if(btdrv_is_pcm_mask_enable()==1&&bt_sco_codec_is_msbc())
   {
	uint32_t lock;
	uint32_t i;
	//must lock the interrupts when exchanging data.
	lock = int_lock();
	uint16_t *capture_dst=(uint16_t *)(capture_buf_btpcm + pingpang * capture_size_btpcm / 2);

	for( i =0; i<capture_size_btpcm/4; i++)
	{
		capture_dst[i]=(uint16_t)capture_buf_btpcm_copy[i]<<8;
	}
	int_unlock(lock);
   }
#endif

#if defined(HIGH_EFFICIENCY_TX_PWR_CTRL)
    bt_sco_tx_gain_mode_ajdustment_handler();
#endif

    //processing spk
    uint8_t *playbakce_pcm_frame_p = playback_buf_codecpcm + pingpang * playback_size_codecpcm / 2;
    uint8_t *source = capture_buf_btpcm + pingpang * capture_size_btpcm / 2;
    uint32_t source_len=playback_size_btpcm / 2;

    //DUMP16("%04x,",source,40);

#if defined(PCM_PRIVATE_DATA_FLAG)
    {
    	uint32_t i;
        for (i = 0; i < source_len/BTPCM_TOTAL_DATA_LENGTH; i++)
        {
		uint8_t *dst=source+i*BTPCM_PUBLIC_DATA_LENGTH;
		uint8_t *src=source+i*BTPCM_TOTAL_DATA_LENGTH+BTPCM_PRIVATE_DATA_LENGTH;
        	memcpy(dst,src,BTPCM_PUBLIC_DATA_LENGTH);
        }
	source_len=source_len-BTPCM_PRIVATE_DATA_LENGTH*i;
    }
#endif

    if(sco_btpcm_mute_flag == 1 || sco_disconnect_mute_flag == 1) {
        for(uint32_t i =0; i < source_len; i++) {
            source[i]=MUTE_PATTERN;
        }
        TRACE_AUD_STREAM_I("[SCO][SPK]mute....................");
    } else {
        sco_btpcm_mute_flag=1;
    }

    if (bt_sco_codec_is_cvsd() == false) {
        uint16_t *source_u16 = (uint16_t *)source;
        for (uint32_t i = 0; i < source_len/2; i++) {
            source[i] = (source_u16[i] >> 8);
        }
        source_len >>= 1;
    }
    process_downlink_bt_voice_frames(source, source_len, playbakce_pcm_frame_p, playback_size_codecpcm / sco_play_chan_num / 2);

    if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
        // Convert mono data to stereo data
#if defined(SPEECH_RX_24BIT)
        app_bt_stream_copy_track_one_to_two_24bits((int32_t *)playbakce_pcm_frame_p, (int32_t *)playbakce_pcm_frame_p, playback_size_codecpcm / 2 / sco_play_chan_num / sizeof(int32_t));
#else
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)playbakce_pcm_frame_p, (int16_t *)playbakce_pcm_frame_p, playback_size_codecpcm / 2 / sco_play_chan_num / sizeof(int16_t));
#endif
    }
    return len;
#else
#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

    uint8_t *dec_buf;
    uint32_t mono_len;

#if defined(SPEECH_RX_24BIT)
    len /= 2;
#endif

#ifdef __BT_ANC__
    mono_len = len / sco_play_chan_num / bt_sco_upsampling_ratio;
    dec_buf = bt_anc_sco_dec_buf;
#else
    mono_len = len / sco_play_chan_num;
    dec_buf = buf;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if(hal_cmu_get_audio_resample_status())
    {
        if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
            sco_play_irq_cnt++;
        }
        if (sco_dma_buf_err || af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK)) {
            sco_dma_buf_err = false;
            sco_play_irq_cnt = 0;
            app_resample_reset(sco_playback_resample);
            app_resample_reset(sco_capture_resample);
            voicebtpcm_pcm_echo_buf_queue_reset();
            TRACE_AUD_STREAM_I("[SCO][SPK]: DMA buffer error: reset resample");
        }
        app_playback_resample_run(sco_playback_resample, dec_buf, mono_len);
    }
    else
#endif
    {
#ifdef __BT_ANC__
        bt_anc_sco_down_sample_16bits((int16_t *)dec_buf, (int16_t *)buf, mono_len / 2);
#else
        if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
            // Convert stereo data to mono data (to save into echo_buf)
            app_bt_stream_copy_track_two_to_one_16bits((int16_t *)dec_buf, (int16_t *)buf, mono_len / 2);
        }
#endif
        voicebtpcm_pcm_audio_more_data(dec_buf, mono_len);
    }

#ifdef __BT_ANC__
    voicebtpcm_pcm_resample((int16_t *)dec_buf, mono_len / 2, (int16_t *)buf);
#endif

#if defined(SPEECH_RX_24BIT)
    len <<= 1;
#endif

    if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
        // Convert mono data to stereo data
#if defined(SPEECH_RX_24BIT)
        app_bt_stream_copy_track_one_to_two_24bits((int32_t *)buf, (int32_t *)buf, len / 2 / sizeof(int32_t));
#else
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, (int16_t *)buf, len / 2 / sizeof(int16_t));
#endif
    }

    if (spk_force_mute)
    {
        memset(buf, 0, len);
    }

#if defined(BT_SCO_HANDLER_PROFILE)
    uint32_t end_ticks = hal_fast_sys_timer_get();
    TRACE_AUD_STREAM_I("[SCO][SPK] takes %d us",
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

    return len;
#endif
}

int bt_sco_player_forcemute(bool mic_mute, bool spk_mute)
{
    mic_force_mute = mic_mute;
    spk_force_mute = spk_mute;
    return 0;
}

int bt_sco_player_get_codetype(void)
{
    if (gStreamplayer & APP_BT_STREAM_HFP_PCM)
    {
        // NOTE: Be careful of this function during sco multi-ponit
        return app_audio_manager_get_scocodecid();
    }
    else
    {
        return 0;
    }
}

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static uint32_t audio_mc_data_playback_sco(uint8_t *buf, uint32_t mc_len_bytes)
{
  // uint32_t begin_time;
   //uint32_t end_time;
  // begin_time = hal_sys_timer_get();
  // TRACE_AUD_STREAM_I("phone cancel: %d",begin_time);

   float left_gain;
   float right_gain;
   int32_t playback_len_bytes,mc_len_bytes_8;
   int32_t i,j,k;
   int delay_sample;

   mc_len_bytes_8=mc_len_bytes/8;

   hal_codec_get_dac_gain(&left_gain,&right_gain);

   TRACE_AUD_STREAM_I("[SCO][SPK][MC] playback_samplerate_ratio:  %d,ch:%d,sample_size:%d.",playback_samplerate_ratio_bt,playback_ch_num_bt,sample_size_play_bt);
   TRACE_AUD_STREAM_I("[SCO][SPK][MC] len:  %d",mc_len_bytes);

  // TRACE_AUD_STREAM_I("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("right_gain: %d",(int)(right_gain*(1<<12)));

   playback_len_bytes=mc_len_bytes/playback_samplerate_ratio_bt;

    if (sample_size_play_bt == AUD_BITS_16)
    {
        int16_t *sour_p=(int16_t *)(playback_buf_bt+playback_size_bt/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int16_t *dest_p=(int16_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int16_t *)playback_buf_bt;
        }

        if(playback_ch_num_bt==AUD_CHANNEL_NUM_2)
        {
            delay_sample=DELAY_SAMPLE_MC;

            for(i=0,j=0;i<delay_sample;i=i+2)
            {
                mid_p[j++]=delay_buf_bt[i];
                mid_p[j++]=delay_buf_bt[i+1];
            }

            for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
            {
                mid_p[j++]=sour_p[i];
                mid_p[j++]=sour_p[i+1];
            }

            for(j=0;i<playback_len_bytes/2;i=i+2)
            {
                delay_buf_bt[j++]=sour_p[i];
                delay_buf_bt[j++]=sour_p[i+1];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                        mid_p_8[j++]=mid_p[i+1];
                    }
                }
            }

            anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/2;i=i+2)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                    dest_p[j++]=mid_p_8[i+1];
                }
            }

        }
        else if(playback_ch_num_bt==AUD_CHANNEL_NUM_1)
        {
            delay_sample=DELAY_SAMPLE_MC/2;

            for(i=0,j=0;i<delay_sample;i=i+1)
            {
                mid_p[j++]=delay_buf_bt[i];
            }

            for(i=0;i<playback_len_bytes/2-delay_sample;i=i+1)
            {
                mid_p[j++]=sour_p[i];
            }

            for(j=0;i<playback_len_bytes/2;i=i+1)
            {
                delay_buf_bt[j++]=sour_p[i];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+1*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/2;i=i+1)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                    }
                }
            }

            anc_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,AUD_BITS_16);

            for(i=0,j=0;i<(mc_len_bytes_8)/2;i=i+1)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                }
            }
        }

    }
    else if (sample_size_play_bt == AUD_BITS_24)
    {
        int32_t *sour_p=(int32_t *)(playback_buf_bt+playback_size_bt/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes_8);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == playback_buf_mc)
        {
            sour_p=(int32_t *)playback_buf_bt;
        }

        if(playback_ch_num_bt==AUD_CHANNEL_NUM_2)
        {
            delay_sample=DELAY_SAMPLE_MC;

            for(i=0,j=0;i<delay_sample;i=i+2)
            {
                mid_p[j++]=delay_buf_bt[i];
                mid_p[j++]=delay_buf_bt[i+1];
            }

            for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
            {
                mid_p[j++]=sour_p[i];
                mid_p[j++]=sour_p[i+1];
            }

            for(j=0;i<playback_len_bytes/4;i=i+2)
            {
                delay_buf_bt[j++]=sour_p[i];
                delay_buf_bt[j++]=sour_p[i+1];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                        mid_p_8[j++]=mid_p[i+1];
                    }
                }
            }

            anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,right_gain,AUD_BITS_24);

            for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+2)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                    dest_p[j++]=mid_p_8[i+1];
                }
            }

        }
        else if(playback_ch_num_bt==AUD_CHANNEL_NUM_1)
        {
            delay_sample=DELAY_SAMPLE_MC/2;

            for(i=0,j=0;i<delay_sample;i=i+1)
            {
                mid_p[j++]=delay_buf_bt[i];
            }

            for(i=0;i<playback_len_bytes/4-delay_sample;i=i+1)
            {
                mid_p[j++]=sour_p[i];
            }

            for(j=0;i<playback_len_bytes/4;i=i+1)
            {
                delay_buf_bt[j++]=sour_p[i];
            }

            if(playback_samplerate_ratio_bt<=8)
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+1*(8/playback_samplerate_ratio_bt))
                {
                    mid_p_8[j++]=mid_p[i];
                }
            }
            else
            {
                for(i=0,j=0;i<playback_len_bytes/4;i=i+1)
                {
                    for(k=0;k<playback_samplerate_ratio_bt/8;k++)
                    {
                        mid_p_8[j++]=mid_p[i];
                    }
                }
            }

            anc_mc_run_mono((uint8_t *)mid_p_8,mc_len_bytes_8,left_gain,AUD_BITS_24);

            for(i=0,j=0;i<(mc_len_bytes_8)/4;i=i+1)
            {
                for(k=0;k<8;k++)
                {
                    dest_p[j++]=mid_p_8[i];
                }
            }
        }

    }

  //  end_time = hal_sys_timer_get();

 //   TRACE_AUD_STREAM_I("[SCO][SPK][MC]:run time: %d", end_time-begin_time);

    return 0;
}
#endif

#if defined(LOW_DELAY_SCO)
int speech_get_frame_size(int fs, int ch, int ms)
{
    return (fs / 1000 * ch * ms)/2;
}
#else
int speech_get_frame_size(int fs, int ch, int ms)
{
    return (fs / 1000 * ch * ms);
}
#endif


int speech_get_codecpcm_buf_len(int fs, int ch, int ms)
{
    return speech_get_frame_size(fs, ch, ms) * 2 * 2;
}

int speech_get_btpcm_buf_len(hfp_sco_codec_t sco_codec, int fs,int ms)
{
    //RAW size per ms(Bytes).
    int buf_len=(fs/1000)*ms*2;

    //after encoding.
    switch(sco_codec)
    {
   	case BTIF_HF_SCO_CODEC_CVSD:
	buf_len=buf_len/2;
	break;
   	case BTIF_HF_SCO_CODEC_MSBC:
	buf_len=buf_len/4;
	break;
   	default:
	ASSERT(0, "sco codec is not correct!");
	break;
    }

    //after BES platform.
    buf_len=buf_len*2;

    //add private data.
#if defined(PCM_PRIVATE_DATA_FLAG)
    buf_len=buf_len+BTPCM_PRIVATE_DATA_LENGTH*((int)(ms/7.5f));
#endif

    //after pingpang
    buf_len=buf_len*2;

    //if low delay. we will use half buffer(should be 7.5ms);
#if defined(LOW_DELAY_SCO)
    buf_len=buf_len/2;
#endif

    TRACE_AUD_STREAM_I("speech_get_btpcm_buf_len:%d", buf_len);

    return buf_len;
}



enum AUD_SAMPRATE_T speech_sco_get_sample_rate(void)
{
    enum AUD_SAMPRATE_T sample_rate;

    if (bt_sco_codec_is_cvsd()) {
        sample_rate = AUD_SAMPRATE_8000;
    } else {
        sample_rate = AUD_SAMPRATE_16000;
    }

    return sample_rate;
}

enum AUD_SAMPRATE_T speech_vqe_get_sample_rate(void)
{
    enum AUD_SAMPRATE_T sample_rate;

#if defined(SPEECH_VQE_FIXED_SAMPLE_RATE)
    sample_rate = (enum AUD_SAMPRATE_T)SPEECH_VQE_FIXED_SAMPLE_RATE;
#else
    if (bt_sco_codec_is_cvsd()) {
        sample_rate = AUD_SAMPRATE_8000;
    } else {
        sample_rate = AUD_SAMPRATE_16000;
    }
#endif

    return sample_rate;
}

enum AUD_SAMPRATE_T speech_codec_get_sample_rate(void)
{
    enum AUD_SAMPRATE_T sample_rate;

#if defined(SPEECH_CODEC_FIXED_SAMPLE_RATE)
    sample_rate = (enum AUD_SAMPRATE_T)SPEECH_CODEC_FIXED_SAMPLE_RATE;
#else
    if (bt_sco_codec_is_cvsd()) {
        sample_rate = AUD_SAMPRATE_8000;
    } else {
        sample_rate = AUD_SAMPRATE_16000;
    }
#endif

    return sample_rate;
}

int app_bt_stream_volumeset(int8_t vol);

enum AUD_SAMPRATE_T sco_sample_rate;

#if defined(AF_ADC_I2S_SYNC)
void bt_sco_bt_trigger_callback(void)
{
    TRACE_AUD_STREAM_I("[SCO][IIS] Start...", __func__);

    hal_i2s_enable(HAL_I2S_ID_0);
}
#endif

#define HFP_BATTERY_REPORT_TIME_MS  (2000)
static void app_bt_hfp_battery_report_timer_handler(void const *param);
osTimerDef (APP_BT_HFP_BATTERY_REPORT_TIMER_ID, app_bt_hfp_battery_report_timer_handler);
osTimerId app_bt_hfp_battery_report_timer_id = NULL;
static bool force_report_battery[BT_DEVICE_NUM];

bool get_force_report_battery_flag(uint8_t hfp_device)
{
    return force_report_battery[hfp_device];
}

void set_force_report_battery_flag(uint8_t hfp_device, bool flag)
{
    force_report_battery[hfp_device] = flag;
}

static void app_bt_hfp_battery_report_timer_handler(void const *param)
{
    uint8_t hfp_device = app_bt_audio_get_curr_hfp_device();
    force_report_battery[hfp_device] = true;
    osapi_notify_evm();
}

void app_bt_hfp_battery_report_timer_start()
{    
    uint8_t hfp_device = app_bt_audio_get_curr_hfp_device();
    bt_bdaddr_t* bt_addr = app_bt_get_remote_device_address(hfp_device);
    if(bt_addr){
        ibrt_mobile_info_t *p_mobile_info = app_ibrt_conn_get_mobile_info_by_addr(bt_addr);
        if(p_mobile_info){
            rem_ver_t * rem_ver = (rem_ver_t *)btif_me_get_remote_device_version(p_mobile_info->p_mobile_remote_dev);

            if(rem_ver){
                if(rem_ver->compid == 0x0002){
                    //Intel Chip: Need to report the battery level regularly during the call to avoid entering the sniff mode at the end of the call.
                    TRACE(1, "%s : Intel Chip", __func__);

                    if(app_bt_hfp_battery_report_timer_id == NULL){
                        app_bt_hfp_battery_report_timer_id = osTimerCreate(osTimer(APP_BT_HFP_BATTERY_REPORT_TIMER_ID), osTimerPeriodic, NULL);
                    }

                    if(app_bt_hfp_battery_report_timer_id != NULL){
                        osTimerStop(app_bt_hfp_battery_report_timer_id);
                        osTimerStart(app_bt_hfp_battery_report_timer_id, HFP_BATTERY_REPORT_TIME_MS);
                    }
                }
            }
        }
    }     
}

void app_bt_hfp_battery_report_timer_stop(void)
{
    if(app_bt_hfp_battery_report_timer_id != NULL){
        osTimerStop(app_bt_hfp_battery_report_timer_id);
    }
}

extern void bt_drv_reg_op_pcm_set(uint8_t en);
extern uint8_t bt_drv_reg_op_pcm_get();
int bt_sco_player(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t * bt_audio_buff = NULL;
    enum AUD_SAMPRATE_T sample_rate;
    uint8_t POSSIBLY_UNUSED adma_ch = HAL_DMA_CHAN_NONE;

    TRACE_AUD_STREAM_I("[SCO_PLAYER] work:%d op:%d freq:%d", isRun, on, freq);

#ifdef CHIP_BEST2000
    btdrv_enable_one_packet_more_head(0);
#endif

    bt_set_playback_triggered(false);

    if (isRun==on)
        return 0;

    if (on)
    {
#ifdef  IS_MULTI_AI_ENABLED
        app_ai_open_mic_user_set(AI_OPEN_MIC_USER_SCO);
#endif
        bt_sco_codec_store();
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_stop_playing();
#endif

#if defined(IBRT)
        app_ibrt_ui_rssi_reset();
#endif
#if defined(__IAG_BLE_INCLUDE__) && defined(BLE_V2)
        app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_SCO,
                                       USER_ALL,
                                       BLE_ADVERTISING_INTERVAL);
#endif
#ifdef TX_RX_PCM_MASK
        if (btdrv_is_pcm_mask_enable() ==1 && bt_sco_codec_is_msbc())
        {
            bt_drv_reg_op_pcm_set(1);
            TRACE_AUD_STREAM_I("[SCO_PLAYER] PCM MASK");
        }
#endif

#if defined(PCM_FAST_MODE)
        btdrv_open_pcm_fast_mode_enable();
#ifdef PCM_PRIVATE_DATA_FLAG
        bt_drv_reg_op_set_pcm_flag();
#endif
#endif

#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_STOP, 0);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_MIC_OPEN, 0);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,THIRDPARTY_STOP, 0);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,THIRDPARTY_CALL_START, 0);
#endif
        //bt_syncerr set to max(0x0a)
//        BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x0f);
//        af_set_priority(AF_USER_SCO, osPriorityRealtime);
        af_set_priority(AF_USER_SCO, osPriorityHigh);
        bt_media_volume_ptr_update_by_mediatype(BT_STREAM_VOICE);
        stream_local_volume = btdevice_volume_p->hfp_vol;
        app_audio_manager_sco_status_checker();

        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        if (freq < APP_SYSFREQ_208M) {
            freq = APP_SYSFREQ_208M;
        }
#endif

#if defined(SCO_CP_ACCEL)
        freq = APP_SYSFREQ_52M;
#endif

#if defined(SPEECH_TX_THIRDPARTY)
        freq = APP_SYSFREQ_104M;
#endif

#if defined(IBRT)
        if (app_bt_audio_count_straming_mobile_links() > 1)
        {
            if (freq < APP_SYSFREQ_78M)
            {
                freq = APP_SYSFREQ_78M;
            }
        }
#endif

        app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
        TRACE_AUD_STREAM_I("[SCO_PLAYER] sysfreq calc[%d]: %d\n", freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

#ifndef FPGA
        app_overlay_select(APP_OVERLAY_HFP);
#ifdef BT_XTAL_SYNC
        bt_init_xtal_sync(BT_XTAL_SYNC_MODE_VOICE, BT_INIT_XTAL_SYNC_MIN, BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif
#endif
        btdrv_rf_bit_offset_track_enable(true);

#if !defined(SCO_DMA_SNAPSHOT)
        int aec_frame_len = speech_get_frame_size(speech_codec_get_sample_rate(), 1, SPEECH_SCO_FRAME_MS);
        speech_tx_aec_set_frame_len(aec_frame_len);
#endif

        bt_sco_player_forcemute(false, false);

        bt_sco_mode = 1;

        app_audio_mempool_init();

#ifndef _SCO_BTPCM_CHANNEL_
        memset(&hf_sendbuff_ctrl, 0, sizeof(hf_sendbuff_ctrl));
#endif

        sample_rate = speech_codec_get_sample_rate();

        sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)SPEECH_CODEC_CAPTURE_CHANNEL_NUM;

#if defined(FPGA)
        sco_cap_chan_num = AUD_CHANNEL_NUM_2;
#endif

#if defined(SPEECH_TX_AEC_CODEC_REF)
        sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)(sco_cap_chan_num + 1);
#endif

#if defined(ANC_NOISE_TRACKER)
        sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)(sco_cap_chan_num + ANC_NOISE_TRACKER_CHANNEL_NUM);
#endif

#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_disable();
#endif

#if defined(CODEC_DAC_MULTI_VOLUME_TABLE)
        hal_codec_set_dac_volume_table(codec_dac_hfp_vol, ARRAY_SIZE(codec_dac_hfp_vol));
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        // codec:mic
#if defined(ANC_ASSIST_ENABLED)
        app_anc_assist_set_mode(ANC_ASSIST_MODE_PHONE_CALL);
        stream_cfg.channel_map  = (enum AUD_CHANNEL_MAP_T)app_anc_assist_get_mic_ch_map(AUD_INPUT_PATH_MAINMIC);
        sco_cap_chan_num        = (enum AUD_CHANNEL_NUM_T)app_anc_assist_get_mic_ch_num(AUD_INPUT_PATH_MAINMIC);
#endif
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.channel_num = sco_cap_chan_num;
        stream_cfg.data_size = speech_get_codecpcm_buf_len(sample_rate, stream_cfg.channel_num, SPEECH_SCO_FRAME_MS);

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
        // When __AUDIO_RESAMPLE__ is defined,
        // resample is off by default on best1000, and on by default on other platforms
#ifndef CHIP_BEST1000
        hal_cmu_audio_resample_disable();
#endif
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        if (sample_rate == AUD_SAMPRATE_8000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_8463;
        }
        else if (sample_rate == AUD_SAMPRATE_16000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_16927;
        }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
        sco_capture_resample = app_capture_resample_any_open( stream_cfg.channel_num,
                            bt_sco_capture_resample_iter, stream_cfg.data_size / 2,
                            (float)CODEC_FREQ_26M / CODEC_FREQ_24P576M);
#else
        sco_capture_resample = app_capture_resample_open(sample_rate, stream_cfg.channel_num,
                            bt_sco_capture_resample_iter, stream_cfg.data_size / 2);
#endif
        uint32_t mono_cap_samp_cnt = stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
        uint32_t cap_irq_cnt_per_frm = ((mono_cap_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) / sample_rate +
            (aec_frame_len - 1)) / aec_frame_len;
        if (cap_irq_cnt_per_frm == 0) {
            cap_irq_cnt_per_frm = 1;
        }
#else
        stream_cfg.sample_rate = sample_rate;
#endif

#if defined(SPEECH_TX_24BIT)
        stream_cfg.bits = AUD_BITS_24;
        stream_cfg.data_size *= 2;
#else
        stream_cfg.bits = AUD_BITS_16;
#endif
        stream_cfg.vol = stream_local_volume;

#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.handler = bt_sco_codec_capture_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
        capture_buf_codecpcm=stream_cfg.data_ptr;
        capture_size_codecpcm=stream_cfg.data_size;
#endif

        TRACE_AUD_STREAM_I("[SCO_PLAYER] Capture: sample_rate: %d, data_size: %d, chan_num: %d", stream_cfg.sample_rate, stream_cfg.data_size, stream_cfg.channel_num);

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#if defined(HW_DC_FILTER_WITH_IIR)
        hw_filter_codec_iir_st = hw_filter_codec_iir_create(stream_cfg.sample_rate, stream_cfg.channel_num, stream_cfg.bits, &adc_iir_cfg);
#endif

#if defined(CHIP_BEST2300)
        btdrv_set_bt_pcm_triggler_delay(60);
#elif defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2001)

#if defined(SCO_DMA_SNAPSHOT)
        btdrv_set_bt_pcm_triggler_delay(2);
#else
        btdrv_set_bt_pcm_triggler_delay(60);
#endif

#elif defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)

#if defined(SCO_DMA_SNAPSHOT)
        btdrv_set_bt_pcm_triggler_delay(2);
#else
        btdrv_set_bt_pcm_triggler_delay(59);
#endif

#else
        btdrv_set_bt_pcm_triggler_delay(55);
#endif
        // codec:spk
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)0;
        sample_rate = speech_codec_get_sample_rate();
#if defined(CHIP_BEST1000)
        sco_play_chan_num = AUD_CHANNEL_NUM_2;
#else
#ifdef PLAYBACK_USE_I2S
        sco_play_chan_num = AUD_CHANNEL_NUM_2;
#else
        sco_play_chan_num = AUD_CHANNEL_NUM_1;
#endif
#endif

        stream_cfg.channel_num = sco_play_chan_num;
        // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;
        stream_cfg.data_size = speech_get_codecpcm_buf_len(sample_rate, sco_play_chan_num, SPEECH_SCO_FRAME_MS);
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        if (sample_rate == AUD_SAMPRATE_8000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_8463;
        }
        else if (sample_rate == AUD_SAMPRATE_16000)
        {
            stream_cfg.sample_rate = AUD_SAMPRATE_16927;
        }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
        sco_playback_resample = app_playback_resample_any_open( AUD_CHANNEL_NUM_1,
                            bt_sco_playback_resample_iter, stream_cfg.data_size / stream_cfg.channel_num / 2,
                            (float)CODEC_FREQ_24P576M / CODEC_FREQ_26M);
#else
        sco_playback_resample = app_playback_resample_open(sample_rate, AUD_CHANNEL_NUM_1,
                            bt_sco_playback_resample_iter, stream_cfg.data_size / stream_cfg.channel_num / 2);
#endif
        sco_play_irq_cnt = 0;
        sco_dma_buf_err = false;

        uint32_t mono_play_samp_cnt = stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
        uint32_t play_irq_cnt_per_frm = ((mono_play_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) / sample_rate +
            (aec_frame_len - 1)) / aec_frame_len;
        if (play_irq_cnt_per_frm == 0) {
            play_irq_cnt_per_frm = 1;
        }
        uint32_t play_samp_cnt_per_frm = mono_play_samp_cnt * play_irq_cnt_per_frm;
        uint32_t cap_samp_cnt_per_frm = mono_cap_samp_cnt * cap_irq_cnt_per_frm;
        uint32_t max_samp_cnt_per_frm = (play_samp_cnt_per_frm >= cap_samp_cnt_per_frm) ? play_samp_cnt_per_frm : cap_samp_cnt_per_frm;
        uint32_t echo_q_samp_cnt = (((max_samp_cnt_per_frm + mono_play_samp_cnt * SCO_PLAY_RESAMPLE_ALIGN_CNT) *
            // convert to 8K/16K sample cnt
             sample_rate +(stream_cfg.sample_rate - 1)) / stream_cfg.sample_rate +
            // aligned with aec_frame_len
            (aec_frame_len - 1)) / aec_frame_len * aec_frame_len;
        if (echo_q_samp_cnt == 0) {
            echo_q_samp_cnt = aec_frame_len;
        }
        voicebtpcm_pcm_echo_buf_queue_init(echo_q_samp_cnt * 2);
#else
        stream_cfg.sample_rate = sample_rate;
#endif

#ifdef __BT_ANC__
        // Mono channel decoder buffer (8K or 16K sample rate)
        app_audio_mempool_get_buff(&bt_anc_sco_dec_buf, stream_cfg.data_size / 2 / sco_play_chan_num);
        // The playback size for the actual sample rate
        bt_sco_upsampling_ratio = 6/(sample_rate/AUD_SAMPRATE_8000);
        stream_cfg.data_size *= bt_sco_upsampling_ratio;
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#endif
        //damic_init();
        //init_amic_dc_bt();
        //ds_fir_init();
        us_fir_init(bt_sco_upsampling_ratio);
#endif
        stream_cfg.bits = AUD_BITS_16;
#ifdef PLAYBACK_USE_I2S
        stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
        stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif
        stream_cfg.handler = bt_sco_codec_playback_data;

#if defined(SPEECH_RX_24BIT)
        stream_cfg.bits = AUD_BITS_24;
        stream_cfg.data_size *= 2;
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        uint8_t* promptTmpSourcePcmDataBuf;
        uint8_t* promptTmpTargetPcmDataBuf;
        uint8_t* promptPcmDataBuf;
        uint8_t* promptResamplerBuf;

        sco_sample_rate = stream_cfg.sample_rate;
        app_audio_mempool_get_buff(&promptTmpSourcePcmDataBuf, AUDIO_PROMPT_SOURCE_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptTmpTargetPcmDataBuf, AUDIO_PROMPT_TARGET_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptPcmDataBuf, AUDIO_PROMPT_PCM_BUFFER_SIZE);
        app_audio_mempool_get_buff(&promptResamplerBuf, AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);

        audio_prompt_buffer_config(MIX_WITH_SCO_STREAMING,
                                   stream_cfg.channel_num,
                                   stream_cfg.bits,
                                   promptTmpSourcePcmDataBuf,
                                   promptTmpTargetPcmDataBuf,
                                   promptPcmDataBuf,
                                   AUDIO_PROMPT_PCM_BUFFER_SIZE,
                                   promptResamplerBuf,
                                   AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

        TRACE_AUD_STREAM_I("[SCO_PLAYER] playback sample_rate:%d, data_size:%d",stream_cfg.sample_rate,stream_cfg.data_size);

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;

#ifdef __BT_ANC__
        playback_samplerate_ratio_bt=8;
#else
        if (sample_rate_play_bt == AUD_SAMPRATE_8000)
        {
            playback_samplerate_ratio_bt=8*3*2;
        }
        else if (sample_rate_play_bt == AUD_SAMPRATE_16000)
        {
            playback_samplerate_ratio_bt=8*3;
        }
#endif

        playback_ch_num_bt=stream_cfg.channel_num;
#endif

#if defined(SCO_DMA_SNAPSHOT)
        playback_buf_codecpcm=stream_cfg.data_ptr;
        playback_size_codecpcm=stream_cfg.data_size;
        playback_samplerate_codecpcm=stream_cfg.sample_rate;
#endif

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
        }

        af_codec_set_playback_post_handler(bt_sco_codec_playback_data_post_handler);

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
        stream_cfg.handler = audio_mc_data_playback_sco;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*playback_samplerate_ratio_bt;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

        memset(delay_buf_bt,0,sizeof(delay_buf_bt));

        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
#endif
#if defined(SCO_DMA_SNAPSHOT)
        btdrv_disable_playback_triggler();
#ifdef PLAYBACK_USE_I2S
        af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, true);
#else
        af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
#endif
        af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif

#if defined(SPEECH_BONE_SENSOR)
        // NOTE: Use VMIC to power on VPU, So must call lis25ba_init after af_stream_start(AUD_STREAM_CAPTURE);
        speech_bone_sensor_open();
#if defined(AF_ADC_I2S_SYNC)
        hal_i2s_enable_delay(HAL_I2S_ID_0);
#elif defined(HW_I2S_TDM_TRIGGER)
        af_i2s_sync_config(AUD_STREAM_CAPTURE, AF_I2S_SYNC_TYPE_BT, true);
#else
        ASSERT(0, "Need to define AF_ADC_I2S_SYNC or HW_I2S_TDM_TRIGGER");
#endif
        speech_bone_sensor_start();

#if defined(AF_ADC_I2S_SYNC)
        af_codec_bt_trigger_config(true, bt_sco_bt_trigger_callback);
#endif
#endif

#if defined(ANC_NOISE_TRACKER)
        app_audio_mempool_get_buff(
            (uint8_t **)&anc_buf,
            speech_get_frame_size(
                speech_codec_get_sample_rate(),
                ANC_NOISE_TRACKER_CHANNEL_NUM,
                SPEECH_SCO_FRAME_MS) * sizeof(int16_t));
        noise_tracker_init(nt_demo_words_cb, ANC_NOISE_TRACKER_CHANNEL_NUM, -20);
#endif

        // Must call this function before af_stream_start
        // Get all free app audio buffer except SCO_BTPCM used(2k)
#if defined(SCO_DMA_SNAPSHOT)
        voicebtpcm_pcm_audio_init(speech_sco_get_sample_rate(), speech_vqe_get_sample_rate(), speech_codec_get_sample_rate(), sco_cap_chan_num);
#else
        voicebtpcm_pcm_audio_init(speech_sco_get_sample_rate(), speech_codec_get_sample_rate());
#endif

        /*
        TRACE_AUD_STREAM_I("[SCO_PLAYER] start codec %d", FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        */

#ifdef SPEECH_SIDETONE
        hal_codec_sidetone_enable();
#endif

#ifdef _SCO_BTPCM_CHANNEL_
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.sample_rate = speech_sco_get_sample_rate();
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;

        if (bt_sco_codec_is_cvsd()) {
            stream_cfg.data_size = speech_get_btpcm_buf_len(BTIF_HF_SCO_CODEC_CVSD,stream_cfg.sample_rate, SPEECH_SCO_FRAME_MS);
        } else {
            stream_cfg.data_size = speech_get_btpcm_buf_len(BTIF_HF_SCO_CODEC_MSBC,stream_cfg.sample_rate, SPEECH_SCO_FRAME_MS);
        }

        // btpcm:rx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_capture_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

        TRACE_AUD_STREAM_I("[SCO_PLAYER] sco btpcm sample_rate:%d, data_size:%d",stream_cfg.sample_rate,stream_cfg.data_size);

#if defined(SCO_DMA_SNAPSHOT)
        sco_btpcm_mute_flag=0;
        sco_disconnect_mute_flag=0;

        capture_buf_btpcm=stream_cfg.data_ptr;
        capture_size_btpcm=stream_cfg.data_size;
#ifdef TX_RX_PCM_MASK
        capture_size_btpcm_copy=stream_cfg.data_size/4;//only need ping or pang;
        app_audio_mempool_get_buff(&capture_buf_btpcm_copy, capture_size_btpcm_copy);
#endif
#endif
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE, &stream_cfg);

        // btpcm:tx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_playback_data;
        app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
        playback_buf_btpcm=stream_cfg.data_ptr;
        playback_size_btpcm=stream_cfg.data_size;
#ifdef TX_RX_PCM_MASK
        playback_size_btpcm_copy=stream_cfg.data_size/4; //only need ping or pang;
        app_audio_mempool_get_buff(&playback_buf_btpcm_copy, playback_size_btpcm_copy);
#endif
         //only need ping or pang;
        app_audio_mempool_get_buff(&playback_buf_btpcm_cache, stream_cfg.data_size/2);
#endif

        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(SCO_DMA_SNAPSHOT)
        app_bt_stream_sco_trigger_btpcm_start();
        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
        }
#endif

#if defined(LOW_DELAY_SCO)
    btpcm_int_counter=1;
#endif

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
        af_codec_tune(AUD_STREAM_NUM, 0);
#endif

        TRACE_AUD_STREAM_I("[SCO_PLAYER] start btpcm %d", FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
        btdrv_pcm_disable();
#endif
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);

#if defined(CVSD_BYPASS)
        btdrv_cvsd_bypass_enable();
#endif

#if defined(CHIP_BEST1501) || defined(CHIP_BEST1501SIMU) || defined(CHIP_BEST2003)
        osDelay(10);
        bt_drv_reg_op_set_btpcm_trig_flag(true);
#endif

        is_codec_stream_started = false;

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
#if !defined(SCO_DMA_SNAPSHOT)
        btdrv_pcm_enable();
#endif
#endif

#endif

#ifdef FPGA
        app_bt_stream_volumeset(stream_local_volume);
        //btdrv_set_bt_pcm_en(1);
#endif
        app_bt_stream_trigger_checker_start();

        app_bt_hfp_battery_report_timer_start();

        TRACE_AUD_STREAM_I("[SCO_PLAYER] on");
    }
    else
    {
#ifdef  IS_MULTI_AI_ENABLED
        app_ai_open_mic_user_set(AI_OPEN_MIC_USER_NONE);
#endif
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        bool isToClearActiveMedia= audio_prompt_clear_pending_stream(PENDING_TO_STOP_SCO_STREAMING);
        if (isToClearActiveMedia)
        {
            uint8_t device_id = app_bt_audio_get_curr_sco_device();
            bt_media_clear_media_type(BT_STREAM_VOICE, (enum BT_DEVICE_ID_T)device_id);
            bt_media_current_sco_set(BT_DEVICE_INVALID_ID);
        }
#endif
        app_bt_stream_trigger_checker_stop();
#if defined(SCO_DMA_SNAPSHOT)
#ifdef TX_RX_PCM_MASK
        playback_buf_btpcm_copy=NULL;
        capture_buf_btpcm_copy=NULL;
        playback_size_btpcm_copy=0;
        capture_size_btpcm_copy=0;
#endif
#endif

#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,THIRDPARTY_MIC_CLOSE, 0);
#endif
#if defined(SCO_DMA_SNAPSHOT)
        app_bt_stream_sco_trigger_codecpcm_stop();
#endif

#ifdef AF_STREAM_ID_0_PLAYBACK_FADEOUT
        // fadeout should be called before af_codec_set_playback_post_handler
        // as pilot tone is merged in playback post handler
#if defined(ANC_ASSIST_USE_INT_CODEC)
        af_stream_playback_fadeout(50);
#else
        af_stream_playback_fadeout(10);
#endif
#endif

        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xF);
        }
        af_codec_set_playback_post_handler(NULL);

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
        af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_disable_dma_tc(adma_ch&0xF);
        }
#endif

#ifdef SPEECH_SIDETONE
        hal_codec_sidetone_disable();
#endif

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
        is_codec_stream_started = false;

#if defined(SPEECH_BONE_SENSOR)
        speech_bone_sensor_stop();
#endif

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
        af_codec_tune(AUD_STREAM_NUM, 0);
#endif

#ifdef _SCO_BTPCM_CHANNEL_
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
#if defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
        bt_drv_reg_op_set_btpcm_trig_flag(false);
#endif
#endif
#ifdef TX_RX_PCM_MASK
        if (btdrv_is_pcm_mask_enable()==1 && bt_drv_reg_op_pcm_get())
        {
            bt_drv_reg_op_pcm_set(0);
            TRACE_AUD_STREAM_I("[SCO_PLAYER] PCM UNMASK");
        }
#endif
#if defined(PCM_FAST_MODE)
        btdrv_open_pcm_fast_mode_disable();
#endif
        bt_drv_reg_op_sco_txfifo_reset(bt_sco_codec_get_type());

#if defined(HW_DC_FILTER_WITH_IIR)
        hw_filter_codec_iir_destroy(hw_filter_codec_iir_st);
#endif

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(SPEECH_BONE_SENSOR)
        speech_bone_sensor_close();
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
        app_capture_resample_close(sco_capture_resample);
        sco_capture_resample = NULL;
        app_capture_resample_close(sco_playback_resample);
        sco_playback_resample = NULL;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
#ifndef CHIP_BEST1000
        // When __AUDIO_RESAMPLE__ is defined,
        // resample is off by default on best1000, and on by default on other platforms
        hal_cmu_audio_resample_enable();
#endif
#endif

#if defined(CODEC_DAC_MULTI_VOLUME_TABLE)
        hal_codec_set_dac_volume_table(NULL, 0);
#endif

#ifdef PLAYBACK_USE_I2S
        hal_cmu_audio_resample_enable();
#endif

        bt_sco_mode = 0;

#ifdef __BT_ANC__
        bt_anc_sco_dec_buf = NULL;
        //damic_deinit();
        //app_cap_thread_stop();
#endif
        voicebtpcm_pcm_audio_deinit();

#ifndef FPGA
#ifdef BT_XTAL_SYNC
        bt_term_xtal_sync(false);
        bt_term_xtal_sync_default();
#endif
#endif

#if defined(ANC_ASSIST_ENABLED)
		app_anc_assist_set_mode(ANC_ASSIST_MODE_STANDALONE);
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BLE_V2)
        app_ble_param_set_adv_interval(BLE_ADV_INTERVALREQ_USER_SCO,
                                       USER_ALL,
                                       BLE_ADV_INVALID_INTERVAL);
#endif

        TRACE_AUD_STREAM_I("[SCO_PLAYER] off");
        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, APP_SYSFREQ_32K);
        af_set_priority(AF_USER_SCO, osPriorityAboveNormal);

        //bt_syncerr set to default(0x07)
//       BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x07);
#ifdef __THIRDPARTY
        //app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,THIRDPARTY_START, 0);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,THIRDPARTY_CALL_STOP, 0);
#endif
#if defined(IBRT)
        app_ibrt_ui_rssi_reset();
#endif
        app_bt_hfp_battery_report_timer_stop();
    }

#if defined(HIGH_EFFICIENCY_TX_PWR_CTRL)
    bt_sco_reset_tx_gain_mode(on);
#endif

    isRun=on;
    return 0;
}

#ifdef AUDIO_LINEIN
#include "app_status_ind.h"
//player channel should <= capture channel number
//player must be 2 channel
#define LINEIN_PLAYER_CHANNEL (2)
#ifdef __AUDIO_OUTPUT_MONO_MODE__
#define LINEIN_CAPTURE_CHANNEL (1)
#else
#define LINEIN_CAPTURE_CHANNEL (2)
#endif

#if (LINEIN_CAPTURE_CHANNEL == 1)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE/2)
#elif (LINEIN_CAPTURE_CHANNEL == 2)
#define LINEIN_PLAYER_BUFFER_SIZE (1024*LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE)
#endif

int8_t app_linein_buffer_is_empty(void)
{
    if (app_audio_pcmbuff_length()){
        return 0;
    }else{
        return 1;
    }
}

uint32_t app_linein_pcm_come(uint8_t * pcm_buf, uint32_t len)
{
    app_audio_pcmbuff_put(pcm_buf, len);

    return len;
}

uint32_t app_linein_need_pcm_data(uint8_t* pcm_buf, uint32_t len)
{
#if (LINEIN_CAPTURE_CHANNEL == 1)
    app_audio_pcmbuff_get((uint8_t *)app_linein_play_cache, len/2);
    app_play_audio_lineinmode_more_data((uint8_t *)app_linein_play_cache,len/2);
    app_bt_stream_copy_track_one_to_two_16bits((int16_t *)pcm_buf, app_linein_play_cache, len/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
    app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
    app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);
#endif

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len/2);
#endif

#ifdef ANC_APP
    bt_audio_updata_eq_for_anc(app_anc_work_status());
#else
    bt_audio_updata_eq(app_audio_get_eq());
#endif

    audio_process_run(pcm_buf, len);

    return len;
}

int app_play_linein_onoff(bool onoff)
{
    static bool isRun =  false;
    uint8_t *linein_audio_cap_buff = 0;
    uint8_t *linein_audio_play_buff = 0;
    uint8_t *linein_audio_loop_buf = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;

    uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
    uint32_t POSSIBLY_UNUSED eq_buff_size;
    uint8_t POSSIBLY_UNUSED play_samp_size;

    TRACE_AUD_STREAM_I("[LINEIN_PLAYER] work:%d op:%d", isRun, onoff);

    if (isRun == onoff)
        return 0;

     if (onoff){
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
        app_overlay_select(APP_OVERLAY_A2DP);
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&linein_audio_cap_buff, LINEIN_CAPTURE_BUFFER_SIZE);
        app_audio_mempool_get_buff(&linein_audio_play_buff, LINEIN_PLAYER_BUFFER_SIZE);
        app_audio_mempool_get_buff(&linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);
        app_audio_pcmbuff_init(linein_audio_loop_buf, LINEIN_PLAYER_BUFFER_SIZE<<2);

#if (LINEIN_CAPTURE_CHANNEL == 1)
        app_audio_mempool_get_buff((uint8_t **)&app_linein_play_cache, LINEIN_PLAYER_BUFFER_SIZE/2/2);
        app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
        app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL, LINEIN_PLAYER_BUFFER_SIZE/2);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_PLAYER_CHANNEL;
#if defined(__AUDIO_RESAMPLE__)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#endif
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = stream_linein_volume;
        TRACE_AUD_STREAM_I("[LINEIN_PLAYER] vol = %d",stream_linein_volume);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_linein_need_pcm_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_play_buff);
        stream_cfg.data_size = LINEIN_PLAYER_BUFFER_SIZE;

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        sample_size_play_bt=stream_cfg.bits;
        sample_rate_play_bt=stream_cfg.sample_rate;
        data_size_play_bt=stream_cfg.data_size;
        playback_buf_bt=stream_cfg.data_ptr;
        playback_size_bt=stream_cfg.data_size;
        if(sample_rate_play_bt==AUD_SAMPRATE_96000)
        {
            playback_samplerate_ratio_bt=4;
        }
        else
        {
            playback_samplerate_ratio_bt=8;
        }
        playback_ch_num_bt=stream_cfg.channel_num;
        mid_p_8_old_l=0;
        mid_p_8_old_r=0;
#endif

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size*2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

        play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
        eq_buff_size = stream_cfg.data_size * sizeof(int32_t) / play_samp_size;
#elif  defined(CHIP_BEST1000)
        eq_buff_size = stream_cfg.data_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
        eq_buff_size = stream_cfg.data_size;
#endif

#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
        eq_buff_size = stream_cfg.data_size;
#else
        eq_buff_size = 0;
        bt_eq_buff = NULL;
#endif

        if(eq_buff_size>0)
        {
            app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
        }

        audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num, stream_cfg.data_size/(stream_cfg.bits <= AUD_BITS_16 ? 2 : 4)/2, bt_eq_buff, eq_buff_size);

#ifdef __SW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,0));
#endif

#ifdef ANC_APP
        anc_status_record = 0xff;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        uint8_t* bt_audio_buff = NULL;
        stream_cfg.bits = sample_size_play_bt;
        stream_cfg.channel_num = playback_ch_num_bt;
        stream_cfg.sample_rate = sample_rate_play_bt;
        stream_cfg.device = AUD_STREAM_USE_MC;
        stream_cfg.vol = 0;
        stream_cfg.handler = audio_mc_data_playback_a2dp;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        app_audio_mempool_get_buff(&bt_audio_buff, data_size_play_bt*playback_samplerate_ratio_bt);
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
        stream_cfg.data_size = data_size_play_bt*playback_samplerate_ratio_bt;

        playback_buf_mc=stream_cfg.data_ptr;
        playback_size_mc=stream_cfg.data_size;

        anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

        memset(delay_buf_bt,0,sizeof(delay_buf_bt));

        af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
        //ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#endif


        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
        af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
#if defined(__AUDIO_RESAMPLE__)
        stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#endif
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
        stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_CAPTURE_CHANNEL;
        stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)hal_codec_get_input_path_cfg(stream_cfg.io_path);
        stream_cfg.handler = app_linein_pcm_come;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_cap_buff);
        stream_cfg.data_size = LINEIN_CAPTURE_BUFFER_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
     }else     {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        audio_process_close();

        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

        app_overlay_unloadall();
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
     }

    isRun = onoff;
    TRACE_AUD_STREAM_I("[LINEIN_PLAYER] end!\n");
    return 0;
}
#endif

#ifdef APP_MIC_CAPTURE_LOOPBACK
/*
use 'MEDIA_PLAY_24BIT' to consider the 24 bit precision,which align with the locol tone
#if defined(MEDIA_PLAY_24BIT)
#endif
*/
enum{
    APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_NULL,
    APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_WORING,
    APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_END,
};
#define APP_MIC_CAPTURE_REF_SIGNAL_QUEUE_SIZE   (1024*4*4)
#define APP_MIC_CAPTURE_OPEN_MIC_SKIP_FRAME_NUM (2)
/*
    the skip time :
    if APP_MIC_CAPTURE_NEED_PLAYBACK_STREAM_SKIP_NUM == 70:
        the time is  = (128*2*2) / 2 / 2 / 16000 * (70)
        in which , 128*2*2 is   APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN
                   /2 , ping-pong dma
                   /2 , by default , 16 bits;
                   16000, sample rate
*/
#define APP_MIC_CAPTURE_NEED_PLAYBACK_STREAM_SKIP_NUM   (50)
/*
    if you want to dump the audio data via uart interface.
    you need :
        #1.in hal_trace.h . 
            define 'AUDIO_DEBUG_V0_1_0'
            define 'AUDIO_DEBUG'
        #2.compile use cmd e.g :'make T=best2500p_ibrt DEBUG=1 '
*/
#define APP_MIC_DATA_DUMP
static bool app_mic_capture_procedure_opened = false;
static bool app_mic_capture_procedure_enalbed = false;
static bool app_mic_capture_procedure_algorithm_initialized = false;
static CQueue app_mic_capture_ref_queue;
#if defined(MEDIA_PLAY_24BIT)
static int32_t *app_mic_capture_ref_signal_proc_buf_ptr;
#else
static uint16_t *app_mic_capture_ref_signal_proc_buf_ptr;
#endif
static uint8_t app_mic_skip_frame = APP_MIC_CAPTURE_OPEN_MIC_SKIP_FRAME_NUM;
uint8_t * app_mic_capture_buf_ptr = NULL;
static uint8_t app_mic_capture_need_playback_stream_skip_num = APP_MIC_CAPTURE_NEED_PLAYBACK_STREAM_SKIP_NUM;
static uint8_t app_mic_capture_procedure_state = APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_NULL;
uint8_t app_mic_capture_procedure_is_working(void)
{
    TRACE(2,"%s state = %d ",__func__,app_mic_capture_procedure_state);
    return (app_mic_capture_procedure_state == APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_WORING)?(true):(false);
}

uint8_t app_mic_capture_need_playback_stream_skip_num_get(void)
{
    return app_mic_capture_need_playback_stream_skip_num;
}

void app_mic_capture_need_playback_stream_skip_num_set(uint8_t num)
{
    app_mic_capture_need_playback_stream_skip_num = APP_MIC_CAPTURE_NEED_PLAYBACK_STREAM_SKIP_NUM;
}

bool app_mic_capture_is_to_need_playback_stream_skip(void)
{
    bool nRet = false;
    if(app_mic_capture_need_playback_stream_skip_num >0 ){
        app_mic_capture_need_playback_stream_skip_num--;
        nRet = true;
    }
    return nRet;
}

void app_mic_capture_procedure_need_set(bool set)
{
    app_mic_capture_procedure_opened = set;
}

bool app_mic_capture_procedure_opened_get(void)
{
    return app_mic_capture_procedure_opened;
}

void app_mic_capture_procedure_opened_clear(void)
{
    app_mic_capture_procedure_opened = false;
}

void app_mic_capture_procedure_enable_set(bool set)
{
    app_mic_capture_procedure_enalbed = set;
}

bool app_mic_capture_procedure_enabled_get(void)
{
    return app_mic_capture_procedure_enalbed;
}

void app_mic_capture_procedure_enabled_clear(void)
{
    app_mic_capture_procedure_enalbed = false;
}

bool app_mic_capture_procedure_enable_flag_set_filter(uint8_t * buf,uint16_t len)
{
    uint16_t aud = *(uint16_t*)buf;
    bool filter_ret = false;
    if(aud == AUD_ID_BT_SEALING_AUDIO){
        app_mic_capture_procedure_enable_set(true);
        filter_ret = true;
    }else{
        app_mic_capture_procedure_enable_set(false);
    }

    return filter_ret;
}

POSSIBLY_UNUSED ESD_ctrl_struct* pstESD_ctrl;

void app_mic_capture_procedure_algorithm_init()
{
    if(app_mic_capture_procedure_algorithm_initialized == true){
        return ;
    }
    // TO DO 
    //med_heap_init(void *begin_addr, size_t size);
//    pstESD_ctrl = sealing_check_init();
    TRACE(0,"sealing_check_init");
    app_mic_capture_procedure_algorithm_initialized = true;
}

void app_mic_capture_procedure_altorithm_deinit(void)
{
//	sealing_check_deinit(pstESD_ctrl);
    TRACE(0,"sealing_check_deinit");
    app_mic_capture_procedure_algorithm_initialized = false;
}

uint16_t success_rate = 0;

void app_mic_capture_procedure_algorithm_process(uint8_t * buf, uint8_t * ref_buf, uint16_t len)
{
    if (success_rate == 0){
        TRACE(0,"sealing_check_process");
        }
#if defined(MEDIA_PLAY_24BIT)
//    success_rate = sealing_check_process((int*)buf, (int*)ref_buf, (int*)buf, len, pstESD_ctrl);
#else
//    success_rate = sealing_check_process((short*)buf, (short*)ref_buf, (short*)buf, len, pstESD_ctrl);
#endif
    //TO DO
}


uint32_t app_mic_capture_reg_signal_avaliable_length(void)
{
    return AvailableOfCQueue(&app_mic_capture_ref_queue);
}

int app_mic_capture_ref_signal_put(uint8_t * buf, uint16_t len)
{
    int nRet;
    
    if(buf == NULL){
        ASSERT(0," buf NULL crash");
    }
    if(len == 0){
        TRACE(0,"capture ref buf len = 0");
        return -1;
    }
    nRet = app_audio_pcmbuff_put(buf,len);

    if(nRet == CQ_ERR){
        TRACE(3,"%s EnCqueue error: %d %d ",__func__,len,app_audio_pcmbuff_length());
    }

    return nRet;

}

int app_mic_capture_ref_signal_get(uint8_t * buf,uint16_t len)
{
    int r = 0;
    if(app_audio_pcmbuff_length() == 0){
        return 0;
    }

    r = app_audio_pcmbuff_get(buf,len);
    if(r == CQ_ERR){
        TRACE(3,"%s peek error : %d %d ",__func__,len,app_audio_pcmbuff_space());
        return 0;
    }

    return len;
}

void app_mic_capture_ref_signal_init(void)
{
    uint8_t *buff = NULL;
    app_audio_mempool_get_buff(&buff, APP_MIC_CAPTURE_REF_SIGNAL_QUEUE_SIZE);

    app_audio_pcmbuff_init(buff, APP_MIC_CAPTURE_REF_SIGNAL_QUEUE_SIZE);
    app_audio_mempool_get_buff((uint8_t**)&app_mic_capture_ref_signal_proc_buf_ptr, APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN);

}

void app_mic_capture_procedure_configure_set(void * ptr)
{
    if(ptr == NULL){
        ASSERT(0,"null pointer crash %s ",__func__);
    }
    app_mic_capture_procedure_struct_t* app_mic_capture_configure = (app_mic_capture_procedure_struct_t *)ptr;
    uint8_t capture_num = AUD_CHANNEL_NUM_1 + 1;
        
#if defined(MEDIA_PLAY_24BIT)

    app_mic_capture_configure->capture_bits = AUD_BITS_24;
#else
    app_mic_capture_configure->capture_bits = AUD_BITS_16;
#endif
    app_mic_capture_configure->capture_num = (enum AUD_CHANNEL_NUM_T)capture_num;
    app_mic_capture_configure->vol = 10;
    app_mic_capture_configure->sample_rate = AUD_SAMPRATE_16000;
    app_mic_capture_configure->device = AUD_STREAM_USE_INT_CODEC;
    app_mic_capture_configure->capture_buf_len = APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN * (app_mic_capture_configure->capture_num);
}

uint32_t app_mic_capture_procedure_handler(uint8_t * in_buf, uint32_t len)
{
#if defined(MEDIA_PLAY_24BIT)
    int32_t *pcm_buf = (int32_t *)in_buf;
    int pcm_len = len / sizeof(int32_t);
#else
    int16_t *pcm_buf = (int16_t *)in_buf;
    int pcm_len = len / sizeof(int16_t);
#endif
#if 0
    if(app_play_audio_pending_stream_get() == STREAM_PENDING_OF_TYPE_AUDIO){
        if((app_mic_capture_buf_ptr) == in_buf){
            app_play_audio_pending_stream_set(STREAM_PENDING_OF_TYPE_AUDIO,false);
            app_play_audio_stream_start();
        }
    }
    if(app_mic_skip_frame > 0){
        memset(in_buf,0,len);
        app_mic_skip_frame--;
        return 0;
    }
#endif

    ASSERT(pcm_len % (1 + 1) == 0, "[%s] pcm_len(%d) should be divided by %d", __FUNCTION__, pcm_len, 1 + 1);
    // copy reference buffer
    for (int i = 1, j = 0; i < pcm_len; i += 1 + 1, j++) {
        app_mic_capture_ref_signal_proc_buf_ptr[j] = pcm_buf[i];
    }
    for (int i = 0, j = 0; i < pcm_len; i += 1 + 1, j += 1) {
        for (int k = 0; k < 1; k++)
            pcm_buf[j + k] = pcm_buf[i + k];
    }
    pcm_len = pcm_len / (1 + 1) * 1;
    
    if(app_mic_capture_is_to_need_playback_stream_skip() >0){
        memset((uint8_t*)pcm_buf,0,len);
        memset((uint8_t*)app_mic_capture_ref_signal_proc_buf_ptr,0,len);
        goto end;
    }

end:
#ifdef APP_MIC_DATA_DUMP
    audio_dump_clear_up();
#if defined(MEDIA_PLAY_24BIT)
    audio_dump_add_channel_data(0, (int *)pcm_buf, pcm_len);
    audio_dump_add_channel_data(1, (int *)app_mic_capture_ref_signal_proc_buf_ptr, pcm_len);
#else
    audio_dump_add_channel_data(0, (short *)pcm_buf, pcm_len);
    audio_dump_add_channel_data(1, (short *)app_mic_capture_ref_signal_proc_buf_ptr, pcm_len);
    //audio_dump_run();
#endif
#endif

    /*
        get the raw pcm data.
        need process put the raw pcm data into the DSP module processor.git
    */
    //TO DO
    app_mic_capture_procedure_algorithm_process((uint8_t*)pcm_buf, (uint8_t*)app_mic_capture_ref_signal_proc_buf_ptr,pcm_len);
#ifdef APP_MIC_DATA_DUMP
#if defined(MEDIA_PLAY_24BIT)
    audio_dump_add_channel_data(2, (int *)pcm_buf, pcm_len);
#else
    audio_dump_add_channel_data(2, (short *)pcm_buf, pcm_len);
#endif
    audio_dump_run();
#endif


    return 0;
}
int app_mic_capture_procedure_onoff(bool onoff,void * ptr)
{
    uint8_t *app_mic_capture_buf = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;
    app_mic_capture_procedure_struct_t * mic_capture_configure = (app_mic_capture_procedure_struct_t*)ptr;
        
    TRACE(2,"app_mic_capture_procedure_onoff work:%d op:%d", app_mic_capture_procedure_opened, onoff);

    if (app_mic_capture_procedure_opened == onoff)
        return 0;

     if (onoff){
        app_sysfreq_req(APP_SYSFREQ_USER_BOOST_FREQ_REQ, APP_SYSFREQ_208M);
#if defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        if(is_play_audio_prompt_use_dac1() == false)
            app_audio_mempool_init();
#endif
        app_audio_mempool_get_buff(&app_mic_capture_buf, mic_capture_configure->capture_buf_len);
        app_mic_capture_ref_signal_init();
#ifdef APP_MIC_DATA_DUMP
#if defined(MEDIA_PLAY_24BIT)
        audio_dump_init(APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN/2/4, sizeof(int), 3);
#else
        audio_dump_init(APP_MIC_CAPTURE_PROCEDURE_DATA_BUF_LEN/2/2, sizeof(short), 3);
#endif
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = mic_capture_configure->capture_bits;
        stream_cfg.channel_num = mic_capture_configure->capture_num;
        stream_cfg.sample_rate = mic_capture_configure->sample_rate;
        stream_cfg.device = mic_capture_configure->device;
        stream_cfg.vol = mic_capture_configure->vol;

        stream_cfg.io_path = AUD_INPUT_PATH_MIC_CAPTURE;
        stream_cfg.handler = app_mic_capture_procedure_handler;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(app_mic_capture_buf);
        stream_cfg.data_size = mic_capture_configure->capture_buf_len;
        app_mic_capture_buf_ptr = app_mic_capture_buf;

        app_mic_skip_frame = APP_MIC_CAPTURE_OPEN_MIC_SKIP_FRAME_NUM;
        app_mic_capture_need_playback_stream_skip_num_set(APP_MIC_CAPTURE_NEED_PLAYBACK_STREAM_SKIP_NUM);
        app_mic_capture_procedure_state = APP_MIC_CAPTURE_PROCEDURE_REF_SIGNAL_NULL;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        //af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
     }else     {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        app_mic_capture_procedure_altorithm_deinit();

        app_sysfreq_req(APP_SYSFREQ_USER_BOOST_FREQ_REQ, APP_SYSFREQ_32K);
     }

    app_mic_capture_procedure_opened = onoff;
    TRACE(1,"%s end!\n", __func__);
    return 0;
}
#endif

POSSIBLY_UNUSED static bool app_bt_stream_is_sco_media(uint16_t player)
{
    return ((player == APP_BT_STREAM_HFP_PCM) || (player == APP_BT_STREAM_HFP_CVSD) || (player == APP_BT_STREAM_HFP_VENDOR));
}

POSSIBLY_UNUSED static bool app_bt_stream_is_music_media(uint16_t player)
{
    return ((player == APP_BT_STREAM_A2DP_SBC) || (player == APP_BT_STREAM_A2DP_AAC) || (player == APP_BT_STREAM_A2DP_VENDOR));
}

static bool app_audio_prompt_no_need_merge(uint16_t aud_id)
{
    bool ret = false;
    switch(aud_id)
    {
        case AUDIO_ID_BT_MUTE:
#ifdef APP_MIC_CAPTURE_LOOPBACK
        case AUD_ID_BT_SEALING_AUDIO:
#endif
            ret = true;
            break;
        default:
            break;
    }
    return ret;
}

int app_bt_stream_open(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    uint16_t player = status->id;
    APP_AUDIO_STATUS next_status = {0};
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] prev:0x%x%s freq:%d", gStreamplayer, player2str(gStreamplayer), freq);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] cur:0x%x%s freq:%d", player, player2str(player), freq);

    APP_AUDIO_STATUS streamToClose = {0};

    if (gStreamplayer != APP_BT_STREAM_INVALID)
    {
        if (gStreamplayer & player)
        {
            TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] 0x%x%s has opened", player, player2str(player));
            return -1;
        }

        if (player >= APP_BT_STREAM_BORDER_INDEX)
        {
            if (APP_BT_INPUT_STREAM_INDEX(gStreamplayer) > 0)
            {
                TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] close 0x%x%s prev opening", gStreamplayer, player2str(gStreamplayer));
            }
        }
#if  defined(AUDIO_PROMPT_USE_DAC2_ENABLED)
        else if (player == APP_PLAY_BACK_AUDIO && !app_audio_prompt_no_need_merge(PROMPT_ID_FROM_ID_VALUE(status->aud_id)))
        {
            // Do nothing
        }else if(app_bt_stream_is_sco_media(player) == true){
            if(gStreamplayer & APP_PLAY_BACK_AUDIO){
                TRACE(1,"current audio = 0x%x",media_GetCurrentPrompt(0));
#ifdef APP_MIC_CAPTURE_LOOPBACK
 //               if(media_GetCurrentPrompt(0) == AUD_ID_BT_SEALING_AUDIO){
                if(app_mic_capture_procedure_opened_get() == true){
                    nRet = app_bt_stream_close(APP_PLAY_BACK_AUDIO);
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_CLEAR_MEDIA, BT_STREAM_MEDIA, 0, 0);
                }
#endif
            }
        }
#endif
        else
        {
            if (APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer) > 0)
            {
                TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] close 0x%x%s prev opening", gStreamplayer, player2str(gStreamplayer));
                uint16_t player2close = APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer);
                nRet = app_bt_stream_close(player2close);
                if (nRet)
                {
                    TRACE_AUD_STREAM_I("nret %d",nRet);
                    return -1;
                }
                else
                {
                #ifdef MEDIA_PLAYER_SUPPORT
                    if (APP_PLAY_BACK_AUDIO&player2close)
                    {
                        TRACE_AUD_STREAM_I("<1>");
                        app_prompt_inform_completed_event();
                    }
                #endif
                    if (((APP_BT_STREAM_A2DP_SBC == player2close) || (APP_BT_STREAM_A2DP_AAC == player2close)) &&
                        (APP_PLAY_BACK_AUDIO == player) && app_audio_prompt_no_need_merge(PROMPT_ID_FROM_ID_VALUE(status->aud_id)))
                    {
                        TRACE_AUD_STREAM_I("<2>");
                    }
                    else
                    {
                        TRACE_AUD_STREAM_I("<4>");
                        streamToClose.id = player2close;
                        app_audio_list_rmv_callback(&streamToClose, &next_status, APP_BT_SETTING_Q_POS_TAIL);
                    }
                }
            }
        }
    }

    switch (player)
    {
    case APP_BT_STREAM_HFP_PCM:
    case APP_BT_STREAM_HFP_CVSD:
    case APP_BT_STREAM_HFP_VENDOR:
        nRet = bt_sco_player(true, freq);
        break;
    case APP_BT_STREAM_A2DP_SBC:
    case APP_BT_STREAM_A2DP_AAC:
    case APP_BT_STREAM_A2DP_VENDOR:
        nRet = bt_sbc_player(PLAYER_OPER_START, freq);
        break;
#ifdef __FACTORY_MODE_SUPPORT__
    case APP_FACTORYMODE_AUDIO_LOOP:
        nRet = app_factorymode_audioloop(true, freq);
        break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
    case APP_PLAY_BACK_AUDIO:
        TRACE_AUD_STREAM_I("<3>");
        nRet = app_play_audio_onoff(true, status);
        break;
#endif

#ifdef RB_CODEC
    case APP_BT_STREAM_RBCODEC:
        nRet = app_rbplay_audio_onoff(true, 0);
        break;
#endif

#ifdef AUDIO_LINEIN
    case APP_PLAY_LINEIN_AUDIO:
        nRet = app_play_linein_onoff(true);
        break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
    case APP_A2DP_SOURCE_LINEIN_AUDIO:
        nRet = app_a2dp_source_linein_on(true);
        break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
    case APP_A2DP_SOURCE_I2S_AUDIO:
        nRet = app_a2dp_source_I2S_onoff(true);
        break;
#endif

#ifdef __AI_VOICE__
    case APP_BT_STREAM_AI_VOICE:
        nRet = app_ai_voice_start_mic_stream();
        break;
#endif
    default:
        nRet = -1;
        break;
    }

    if (!nRet)
    {
        gStreamplayer |= player;
        TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] updated to  0x%x%s", gStreamplayer, player2str(gStreamplayer));
    }
    return nRet;
}

int app_bt_stream_close(uint16_t player)
{
    int nRet = -1;
    TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] gStreamplayer: 0x%x%s", gStreamplayer, player2str(gStreamplayer));
    TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] player:0x%x%s", player, player2str(player));

    if ((gStreamplayer & player) != player)
    {
        return -1;
    }

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, APP_SYSFREQ_32K);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(false, APP_SYSFREQ_32K);
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(false, NULL);
            break;
#endif
#ifdef RB_CODEC
        case APP_BT_STREAM_RBCODEC:
            nRet = app_rbplay_audio_onoff(false, 0);
            break;
#endif

#ifdef AUDIO_LINEIN
        case APP_PLAY_LINEIN_AUDIO:
            nRet = app_play_linein_onoff(false);
            break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
        case APP_A2DP_SOURCE_LINEIN_AUDIO:
            nRet = app_a2dp_source_linein_on(false);
            break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
        case APP_A2DP_SOURCE_I2S_AUDIO:
            nRet = app_a2dp_source_I2S_onoff(false);
            break;
#endif

#ifdef __AI_VOICE__
        case APP_BT_STREAM_AI_VOICE:
            nRet = app_ai_voice_stop_mic_stream();
            break;
#endif
        default:
            nRet = -1;
            break;
    }
    if (!nRet)
    {
        gStreamplayer &= (~player);
        TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] updated to 0x%x%s", gStreamplayer, player2str(gStreamplayer));
    }
    return nRet;
}

int app_bt_stream_setup(uint16_t player, uint8_t status)
{
    int nRet = -1;

    TRACE_AUD_STREAM_I("[STRM_PLAYER][SETUP] prev:%d%s sample:%d", gStreamplayer, player2str(gStreamplayer), status);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][SETUP] cur:%d%s sample:%d", player, player2str(player), status);

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            bt_sbc_player_setup(status);
            break;
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

int app_bt_stream_restart(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    uint16_t player = status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] prev:%d%s freq:%d", gStreamplayer, player2str(gStreamplayer), freq);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] cur:%d%s freq:%d", player, player2str(player), freq);

    if ((gStreamplayer & player) != player)
    {
        return -1;
    }

    switch (player)
    {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, freq);
            nRet = bt_sco_player(true, freq);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            {
#if defined(IBRT)
                uint8_t device_id = 0;
                ibrt_a2dp_status_t a2dp_status;
                POSSIBLY_UNUSED struct BT_DEVICE_T *curr_device = NULL;
                device_id = app_bt_audio_get_curr_a2dp_device();
                curr_device = app_bt_get_device(device_id);
                a2dp_ibrt_sync_get_status(device_id, &a2dp_status);
                TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] state:%d", a2dp_status.state);
                if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING){
                    if (app_audio_manager_a2dp_is_active((enum BT_DEVICE_ID_T)device_id)){
                        TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] resume");
                        nRet = bt_sbc_player(PLAYER_OPER_STOP, freq);
                        nRet = bt_sbc_player(PLAYER_OPER_START, freq);
                    }else{
                        if (APP_IBRT_IS_PROFILE_EXCHANGED(&curr_device->remote)){
                            TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] force_audio_retrigger");
                            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_SBC, device_id,MAX_RECORD_NUM);
                            app_ibrt_if_force_audio_retrigger(RETRIGGER_BY_STREAM_RESTART);
                        }
                    }
                }
#elif BT_DEVICE_NUM > 1
                if (btif_me_get_activeCons() > 1)
                {
                    enum APP_SYSFREQ_FREQ_T sysfreq;

#ifdef A2DP_CP_ACCEL
                    sysfreq = APP_SYSFREQ_26M;
#else
                    sysfreq = APP_SYSFREQ_104M;
#endif
                    app_audio_set_a2dp_freq(sysfreq);
                    bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
                    app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
                }
#endif
            }
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            break;
#endif
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

extern uint8_t bt_media_current_sbc_get(void);
extern uint8_t bt_media_current_sco_get(void);

static uint8_t app_bt_stream_volumeup_generic(bool isToUpdateLocalVolumeLevel)
{
    struct BT_DEVICE_T *curr_device = NULL;
    uint8_t volume_changed_device_id = BT_DEVICE_INVALID_ID;

#if defined AUDIO_LINEIN
    if(app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO))
    {
        stream_linein_volume ++;
        if (stream_linein_volume > TGT_VOLUME_LEVEL_15)
        stream_linein_volume = TGT_VOLUME_LEVEL_15;
        app_bt_stream_volumeset(stream_linein_volume);
        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP] set linein volume %d\n", stream_linein_volume);
    }else
#endif
    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))
    {
        AUD_ID_ENUM prompt_id = AUD_ID_INVALID;
        uint8_t hfp_local_vol = 0;

        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP] hfp volume");

        curr_device = app_bt_get_device(bt_media_current_sco_get());

        if (!curr_device)
        {
            TRACE(2, "%s invalid sco id %x", __func__, bt_media_current_sco_get());
            return BT_DEVICE_INVALID_ID;
        }

        hfp_local_vol = hfp_volume_local_get((enum BT_DEVICE_ID_T)curr_device->device_id);

        if(isToUpdateLocalVolumeLevel)
        {
            // get current local volume
            hfp_local_vol++;
            if (hfp_local_vol > TGT_VOLUME_LEVEL_MAX)
            {
                hfp_local_vol = TGT_VOLUME_LEVEL_MAX;
                prompt_id = AUD_ID_BT_WARNING;
            }
        }
        else
        {
            // get current bt volume
            uint8_t currentBtVol = hfp_convert_local_vol_to_bt_vol(hfp_local_vol);

            // increase bt volume
            if (currentBtVol >= MAX_HFP_VOL)
            {
                currentBtVol = MAX_HFP_VOL;
                prompt_id = AUD_ID_BT_WARNING;
            }
            else
            {
                currentBtVol++;
                //prompt_id = AUD_ID_VOLUME_UP;
            }

            hfp_local_vol = hfp_convert_bt_vol_to_local_vol(currentBtVol);
        }

        hfp_volume_local_set((enum BT_DEVICE_ID_T)curr_device->device_id, hfp_local_vol);

        current_btdevice_volume.hfp_vol = hfp_local_vol;

        app_bt_stream_volumeset(hfp_local_vol);

        volume_changed_device_id = curr_device->device_id;

    #if defined(IBRT)
        if (!app_ibrt_if_is_ui_slave())
    #endif
        {
            if (prompt_id != AUD_ID_INVALID) {
                TRACE(1, "AUD_ID=%d", prompt_id);
                media_PlayAudio(prompt_id, 0);
            }
        }
    }
    else if ((app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) ||
        (app_bt_stream_isrun(APP_BT_STREAM_INVALID)))
    {
        AUD_ID_ENUM prompt_id = AUD_ID_INVALID;
        uint8_t a2dp_local_vol = 0;

        curr_device = app_bt_get_device(bt_media_current_sbc_get());

        if (!curr_device)
        {
            TRACE(2, "%s invalid sbc id %x", __func__, bt_media_current_sbc_get());
            return BT_DEVICE_INVALID_ID;
        }

        TRACE(1, "%s set a2dp volume", __func__);

        a2dp_local_vol = a2dp_volume_local_get((enum BT_DEVICE_ID_T)curr_device->device_id);

        if(isToUpdateLocalVolumeLevel)
        {
            // get current local volume
            a2dp_local_vol++;
            if (a2dp_local_vol > TGT_VOLUME_LEVEL_MAX)
            {
                a2dp_local_vol = TGT_VOLUME_LEVEL_MAX;
                prompt_id = AUD_ID_BT_WARNING;
            }
            a2dp_volume_set_local_vol((enum BT_DEVICE_ID_T)curr_device->device_id, a2dp_local_vol);
        }
        else
        {
            // get current bt volume
            uint8_t currentBtVol = a2dp_abs_volume_get((enum BT_DEVICE_ID_T)curr_device->device_id);

            // increase bt volume
            if (currentBtVol >= MAX_A2DP_VOL)
            {
                currentBtVol = MAX_A2DP_VOL;
                prompt_id = AUD_ID_BT_WARNING;

            }
            else
            {
                currentBtVol++;
                //prompt_id = AUD_ID_VOLUME_UP;
            }


            a2dp_volume_set((enum BT_DEVICE_ID_T)curr_device->device_id, currentBtVol);

            a2dp_local_vol = a2dp_convert_bt_vol_to_local_vol(currentBtVol);
        }

        current_btdevice_volume.a2dp_vol = a2dp_local_vol;

        app_bt_stream_volumeset(a2dp_local_vol);

        volume_changed_device_id = curr_device->device_id;

    #if defined(IBRT)
        if (!app_ibrt_if_is_ui_slave())
    #endif
        {
            if (prompt_id != AUD_ID_INVALID) {
                TRACE(1, "AUD_ID=%d", prompt_id);
                media_PlayAudio(prompt_id, 0);
            }
        }
    }

    TRACE(2,"%s a2dp: %d", __func__, current_btdevice_volume.a2dp_vol);
    TRACE(2,"%s hfp: %d", __func__, current_btdevice_volume.hfp_vol);

#ifndef FPGA
    nv_record_touch_cause_flush();
#endif

    return volume_changed_device_id;
}

void app_bt_stream_bt_volumeup(void)
{
    app_bt_stream_volumeup_generic(false);
}

uint8_t app_bt_stream_local_volumeup(void)
{
    return app_bt_stream_volumeup_generic(true);
}

void app_bt_set_volume(uint16_t type,uint8_t level)
{
    if ((type&APP_BT_STREAM_HFP_PCM) && app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] set hfp volume");
        if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15)
        {
            uint32_t lock = nv_record_pre_write_operation();
            btdevice_volume_p->hfp_vol = level;
            nv_record_post_write_operation(lock);
            app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);
        }
        if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_MUTE)
        {
#ifdef MEDIA_PLAYER_SUPPORT
        #if defined(IBRT)
            if (!app_ibrt_if_is_ui_slave())
        #endif
            {
                media_PlayAudio(AUD_ID_BT_WARNING,0);
            }
#endif
        }
    }
    if ((type&APP_BT_STREAM_A2DP_SBC) && ((app_bt_stream_isrun(APP_BT_STREAM_INVALID)) ||
        (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)))) {
        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] set a2dp volume");
        if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15)
        {
            uint32_t lock = nv_record_pre_write_operation();
            btdevice_volume_p->a2dp_vol = level;
            nv_record_post_write_operation(lock);
            app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
        }
        if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_MUTE)
        {
#ifdef MEDIA_PLAYER_SUPPORT
        #if defined(IBRT)
            if (!app_ibrt_if_is_ui_slave())
        #endif
            {
                media_PlayAudio(AUD_ID_BT_WARNING,0);
            }
#endif
        }
    }

    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] a2dp: %d", btdevice_volume_p->a2dp_vol);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] hfp: %d", btdevice_volume_p->hfp_vol);
#ifndef FPGA
    nv_record_touch_cause_flush();
#endif
}

static uint8_t app_bt_stream_volumedown_generic(bool isToUpdateLocalVolumeLevel)
{
    struct BT_DEVICE_T *curr_device = NULL;
    uint8_t volume_changed_device_id = BT_DEVICE_INVALID_ID;

#if defined AUDIO_LINEIN
    if(app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO))
    {
        stream_linein_volume --;
        if (stream_linein_volume < TGT_VOLUME_LEVEL_MUTE)
            stream_linein_volume = TGT_VOLUME_LEVEL_MUTE;
        app_bt_stream_volumeset(stream_linein_volume);
        TRACE(1,"set linein volume %d\n", stream_linein_volume);
    }else
#endif
    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))
    {
        AUD_ID_ENUM prompt_id = AUD_ID_INVALID;
        uint8_t hfp_local_vol = 0;

        curr_device = app_bt_get_device(bt_media_current_sco_get());

        if (!curr_device)
        {
            TRACE(2, "%s invalid sco id %x", __func__, bt_media_current_sco_get());
            return BT_DEVICE_INVALID_ID;
        }

        TRACE(1, "%s set hfp volume", __func__);

        hfp_local_vol = hfp_volume_local_get((enum BT_DEVICE_ID_T)curr_device->device_id);

        if(isToUpdateLocalVolumeLevel)
        {
            // get current local volume
            if (hfp_local_vol)
            {
                hfp_local_vol--;
            }
            if (hfp_local_vol <= TGT_VOLUME_LEVEL_MUTE)
            {
                hfp_local_vol = TGT_VOLUME_LEVEL_MUTE;
                prompt_id = AUD_ID_BT_WARNING;
            }
        }
        else
        {
            // get current bt volume
            uint8_t currentBtVol = hfp_convert_local_vol_to_bt_vol(hfp_local_vol);

            if (currentBtVol <= 0)
            {
                currentBtVol = 0;
                prompt_id = AUD_ID_BT_WARNING;
            }
            else
            {
                currentBtVol--;
                //prompt_id = AUD_ID_VOLUME_DOWN;
            }

            hfp_local_vol = hfp_convert_bt_vol_to_local_vol(currentBtVol);
        }

        hfp_volume_local_set((enum BT_DEVICE_ID_T)curr_device->device_id, hfp_local_vol);

        current_btdevice_volume.hfp_vol = hfp_local_vol;

        app_bt_stream_volumeset(hfp_local_vol);

        volume_changed_device_id = curr_device->device_id;

    #if defined(IBRT)
        if (!app_ibrt_if_is_ui_slave())
    #endif
        {
            if (prompt_id != AUD_ID_INVALID) {
                TRACE(1, "AUD_ID=%d", prompt_id);
                media_PlayAudio(prompt_id, 0);
            }
        }
    }
    else if ((app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) ||
        (app_bt_stream_isrun(APP_BT_STREAM_INVALID)))
    {
        AUD_ID_ENUM prompt_id = AUD_ID_INVALID;
        uint8_t a2dp_local_vol = 0;

        curr_device = app_bt_get_device(bt_media_current_sbc_get());

        if (!curr_device)
        {
            TRACE(2, "%s invalid sbc id %x", __func__, bt_media_current_sbc_get());
            return BT_DEVICE_INVALID_ID;
        }

        TRACE(1, "%s set a2dp volume", __func__);

        a2dp_local_vol = a2dp_volume_local_get((enum BT_DEVICE_ID_T)curr_device->device_id);

        if(isToUpdateLocalVolumeLevel)
        {
            // get current local volume
            if (a2dp_local_vol)
            {
                a2dp_local_vol--;
            }
            if (a2dp_local_vol <= TGT_VOLUME_LEVEL_MUTE)
            {
                a2dp_local_vol = TGT_VOLUME_LEVEL_MUTE;
                prompt_id = AUD_ID_BT_WARNING;
            }

            a2dp_volume_set_local_vol((enum BT_DEVICE_ID_T)curr_device->device_id, a2dp_local_vol);
        }
        else
        {
            // get current bt volume
            uint8_t currentBtVol = a2dp_abs_volume_get((enum BT_DEVICE_ID_T)curr_device->device_id);

            if (currentBtVol <= 0)
            {
                currentBtVol = 0;
                prompt_id = AUD_ID_BT_WARNING;
            }
            else
            {
                currentBtVol--;
                //prompt_id = AUD_ID_VOLUME_DOWN;
            }

            a2dp_volume_set((enum BT_DEVICE_ID_T)curr_device->device_id, currentBtVol);

            a2dp_local_vol = a2dp_convert_bt_vol_to_local_vol(currentBtVol);
        }

        current_btdevice_volume.a2dp_vol = a2dp_local_vol;

        app_bt_stream_volumeset(a2dp_local_vol);

        volume_changed_device_id = curr_device->device_id;

    #if defined(IBRT)
        if (!app_ibrt_if_is_ui_slave())
    #endif
        {
            if (prompt_id != AUD_ID_INVALID) {
                TRACE(1, "AUD_ID=%d", prompt_id);
                media_PlayAudio(prompt_id, 0);
            }
        }
    }

    TRACE(2,"%s a2dp: %d", __func__, current_btdevice_volume.a2dp_vol);
    TRACE(2,"%s hfp: %d", __func__, current_btdevice_volume.hfp_vol);

#ifndef FPGA
    nv_record_touch_cause_flush();
#endif

    return volume_changed_device_id;
}

void app_bt_stream_bt_volumedown(void)
{
    app_bt_stream_volumedown_generic(false);
}

uint8_t app_bt_stream_local_volumedown(void)
{
    return app_bt_stream_volumedown_generic(true);
}

void app_bt_stream_volumeset_handler(int8_t vol)
{
    uint32_t ret;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
    if (ret == 0 && stream_cfg) {
        stream_cfg->vol = vol;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }
#if (defined(AUDIO_ANC_FB_MC)||defined(AUDIO_ANC_FB_MC_SCO)) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg, false);
    if (ret == 0) {
        stream_cfg->vol = vol;
        af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
    }
#endif
}

void app_bt_stream_volume_edge_check(void)
{
#if defined(IBRT)
    if (app_ibrt_if_is_ui_slave())
    {
        return;
    }
#endif

    bool isHfpStream = (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM) ||
        app_bt_stream_isrun(APP_BT_STREAM_HFP_CVSD) ||
        app_bt_stream_isrun(APP_BT_STREAM_HFP_VENDOR));
    bool isA2dpStream = (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC) ||
        app_bt_stream_isrun(APP_BT_STREAM_A2DP_AAC) ||
        app_bt_stream_isrun(APP_BT_STREAM_A2DP_VENDOR));

    if (isHfpStream || isA2dpStream)
    {
        if (TGT_VOLUME_LEVEL_MUTE == stream_local_volume)
        {
            media_PlayAudio(AUD_ID_BT_WARNING, 0);
        }
        else if (TGT_VOLUME_LEVEL_MAX == stream_local_volume)
        {
            media_PlayAudio(AUD_ID_BT_WARNING, 0);
        }
    }
}

int app_bt_stream_volumeset(int8_t vol)
{
    if (vol > TGT_VOLUME_LEVEL_MAX)
        vol = TGT_VOLUME_LEVEL_MAX;
    if (vol < TGT_VOLUME_LEVEL_MUTE)
        vol = TGT_VOLUME_LEVEL_MUTE;

    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][SET] vol=%d", vol);

    stream_local_volume = vol;
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    if ((!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)) &&
        (audio_prompt_is_allow_update_volume()))
#else
    if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO))
#endif
    {
        app_bt_stream_volumeset_handler(vol);
    }
    return 0;
}

int app_bt_stream_local_volume_get(void)
{
    return stream_local_volume;
}

uint8_t app_bt_stream_a2dpvolume_get(void)
{
   // return btdevice_volume_p->a2dp_vol;
   return current_btdevice_volume.a2dp_vol;
}

uint8_t app_bt_stream_hfpvolume_get(void)
{
    //return btdevice_volume_p->hfp_vol;
    return current_btdevice_volume.hfp_vol;

}

void app_bt_stream_a2dpvolume_reset(void)
{
    btdevice_volume_p->a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT ;
    current_btdevice_volume.a2dp_vol=NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
}

void app_bt_stream_hfpvolume_reset(void)
{
    btdevice_volume_p->hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
    current_btdevice_volume.hfp_vol=NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
}

void app_bt_stream_volume_ptr_update(uint8_t *bdAddr)
{
    static struct btdevice_volume stream_volume = {NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT,NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT};

#ifndef FPGA
    nvrec_btdevicerecord *record = NULL;

    memset(&current_btdevice_volume, 0, sizeof(btdevice_volume));

    if (bdAddr && !nv_record_btdevicerecord_find((bt_bdaddr_t*)bdAddr,&record))
    {
        btdevice_volume_p = &(record->device_vol);
        DUMP8("0x%02x ", bdAddr, BT_ADDR_OUTPUT_PRINT_NUM);
        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UPDATE] a2dp_vol:%d hfp_vol:%d ptr:%p",  btdevice_volume_p->a2dp_vol, btdevice_volume_p->hfp_vol,btdevice_volume_p);
    }
    else
#endif
    {
        btdevice_volume_p = &stream_volume;
        TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UPDATE] default");
        if (bdAddr){
            DUMP8("0x%02x ", bdAddr, BT_ADDR_OUTPUT_PRINT_NUM);
        }
    }
    current_btdevice_volume.a2dp_vol=btdevice_volume_p->a2dp_vol;
    current_btdevice_volume.hfp_vol=btdevice_volume_p->hfp_vol;

}

struct btdevice_volume * app_bt_stream_volume_get_ptr(void)
{
    return btdevice_volume_p;
}

bool app_bt_stream_isrun(uint16_t player)
{
    if ((gStreamplayer & player) == player)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int app_bt_stream_closeall()
{
    TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSEALL]");

    bt_sco_player(false, APP_SYSFREQ_32K);
    bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);

#ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_onoff(false, 0);
#endif
#ifdef __AI_VOICE__
    app_ai_voice_stop_mic_stream();       
#endif


#ifdef RB_CODEC
    app_rbplay_audio_onoff(false, 0);
#endif

    gStreamplayer = APP_BT_STREAM_INVALID;

    return 0;
}

void app_bt_stream_copy_track_one_to_two_24bits(int32_t *dst_buf, int32_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--)
    {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t src_len)
{
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--)
    {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

void app_bt_stream_copy_track_two_to_one_16bits(int16_t *dst_buf, int16_t *src_buf, uint32_t dst_len)
{
    for (uint32_t i = 0; i < dst_len; i++)
    {
        dst_buf[i] = src_buf[i*2];
    }
}

#ifdef PLAYBACK_FORCE_48K
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len)
{
#if (A2DP_DECODER_VER == 2)
    uint8_t device_id = app_bt_audio_get_curr_a2dp_device();
    a2dp_audio_playback_handler(device_id, buf, len);
#else
    uint8_t codec_type = bt_sbc_player_get_codec_type();
    uint32_t overlay_id = 0;
    if(0){
#if defined(A2DP_LHDC_ON)
    }else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_LHDC){
        overlay_id = APP_OVERLAY_A2DP_LHDC;
#endif
#if defined(A2DP_AAC_ON)
    }else if(codec_type ==  BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC){
        overlay_id = APP_OVERLAY_A2DP_AAC;
#endif
    }else if(codec_type == BTIF_AVDTP_CODEC_TYPE_SBC){
        overlay_id = APP_OVERLAY_A2DP;
    }
#ifndef FPGA
    a2dp_audio_more_data(overlay_id, buf, len);
#endif

#endif // #if (A2DP_DECODER_VER == 2)

    return 0;
}

static struct APP_RESAMPLE_T *app_resample_open(enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step);

struct APP_RESAMPLE_T *app_force48k_resample_any_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = NULL;

    if (sample_rate == AUD_SAMPRATE_44100)
    {
        coef = &resample_coef_44p1k_to_48k;
    }
    else
    {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
    }

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, 0);
}
#endif

// =======================================================
// APP RESAMPLE
// =======================================================

#ifndef MIX_MIC_DURING_MUSIC
#include "resample_coef.h"
#endif

static APP_RESAMPLE_BUF_ALLOC_CALLBACK resamp_buf_alloc = app_audio_mempool_get_buff;

static void memzero_int16(void *dst, uint32_t len)
{
    if (dst)
    {
        int16_t *dst16 = (int16_t *)dst;
        int16_t *dst16_end = dst16 + len / 2;

        while (dst16 < dst16_end)
        {
            *dst16++ = 0;
        }
    }
    else
    {
        TRACE_AUD_STREAM_I("WRN: receive null pointer");
    }
}

struct APP_RESAMPLE_T *app_resample_open_with_preallocated_buf(enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step, uint8_t* buf, uint32_t bufSize)
{
    TRACE_AUD_STREAM_I("[STRM_PLAYER][PROMPT_MIXER][OPEN]");
    struct APP_RESAMPLE_T *resamp;
    struct RESAMPLE_CFG_T cfg;
    enum RESAMPLE_STATUS_T status;
    uint32_t size, resamp_size;

    resamp_size = audio_resample_ex_get_buffer_size(chans, AUD_BITS_16, coef->phase_coef_num);

    size = sizeof(struct APP_RESAMPLE_T);
    size += ALIGN(iter_len, 4);
    size += resamp_size;

    ASSERT(size < bufSize, "Pre-allocated buffer size %d is smaller than the needed size %d",
        bufSize, size);

    resamp = (struct APP_RESAMPLE_T *)buf;
    buf += sizeof(*resamp);
    resamp->stream = stream;
    resamp->cb = cb;
    resamp->iter_buf = buf;
    buf += ALIGN(iter_len, 4);
    resamp->iter_len = iter_len;
    resamp->offset = iter_len;
    resamp->ratio_step = ratio_step;

    memset(&cfg, 0, sizeof(cfg));
    cfg.chans = chans;
    cfg.bits = AUD_BITS_16;
    cfg.ratio_step = ratio_step;
    cfg.coef = coef;
    cfg.buf = buf;
    cfg.size = resamp_size;

    status = audio_resample_ex_open(&cfg, (RESAMPLE_ID *)&resamp->id);
    ASSERT(status == RESAMPLE_STATUS_OK, "%s: Failed to open resample: %d", __func__, status);

#ifdef CHIP_BEST1000
    hal_cmu_audio_resample_enable();
#endif

    return resamp;
}


static struct APP_RESAMPLE_T *app_resample_open(enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    TRACE_AUD_STREAM_I("[STRM_PLAYER][RESAMPLE][OPEN] ratio: %d/1000", uint32_t(ratio_step * 1000));
    struct APP_RESAMPLE_T *resamp;
    struct RESAMPLE_CFG_T cfg;
    enum RESAMPLE_STATUS_T status;
    uint32_t size, resamp_size;
    uint8_t *buf;

    resamp_size = audio_resample_ex_get_buffer_size(chans, AUD_BITS_16, coef->phase_coef_num);

    size = sizeof(struct APP_RESAMPLE_T);
    size += ALIGN(iter_len, 4);
    size += resamp_size;

    resamp_buf_alloc(&buf, size);

    resamp = (struct APP_RESAMPLE_T *)buf;
    buf += sizeof(*resamp);
    resamp->stream = stream;
    resamp->cb = cb;
    resamp->iter_buf = buf;
    buf += ALIGN(iter_len, 4);
    resamp->iter_len = iter_len;
    resamp->offset = iter_len;
    resamp->ratio_step = ratio_step;

    memset(&cfg, 0, sizeof(cfg));
    cfg.chans = chans;
    cfg.bits = AUD_BITS_16;
    cfg.ratio_step = ratio_step;
    cfg.coef = coef;
    cfg.buf = buf;
    cfg.size = resamp_size;

    status = audio_resample_ex_open(&cfg, (RESAMPLE_ID *)&resamp->id);
    ASSERT(status == RESAMPLE_STATUS_OK, "%s: Failed to open resample: %d", __func__, status);

#ifdef CHIP_BEST1000
    hal_cmu_audio_resample_enable();
#endif

    return resamp;
}

static int app_resample_close(struct APP_RESAMPLE_T *resamp)
{
#ifdef CHIP_BEST1000
    hal_cmu_audio_resample_disable();
#endif

    if (resamp)
    {
        audio_resample_ex_close((RESAMPLE_ID *)resamp->id);
    }

    return 0;
}

struct APP_RESAMPLE_T *app_playback_resample_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len)
{
    const struct RESAMPLE_COEF_T *coef = NULL;

    if (sample_rate == AUD_SAMPRATE_8000)
    {
        coef = &resample_coef_8k_to_8p4k;
    }
    else if (sample_rate == AUD_SAMPRATE_16000)
    {
        coef = &resample_coef_8k_to_8p4k;
    }
    else if (sample_rate == AUD_SAMPRATE_32000)
    {
        coef = &resample_coef_32k_to_50p7k;
    }
    else if (sample_rate == AUD_SAMPRATE_44100)
    {
        coef = &resample_coef_44p1k_to_50p7k;
    }
    else if (sample_rate == AUD_SAMPRATE_48000)
    {
        coef = &resample_coef_48k_to_50p7k;
    }
    else
    {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
    }

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, 0);
}

#ifdef RESAMPLE_ANY_SAMPLE_RATE
struct APP_RESAMPLE_T *app_playback_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

    return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, ratio_step);
}

struct APP_RESAMPLE_T *app_playback_resample_any_open_with_pre_allocated_buffer(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step, uint8_t* ptrBuf, uint32_t bufSize)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

    return app_resample_open_with_preallocated_buf(
        AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, ratio_step, ptrBuf, bufSize);
}
#endif

int app_playback_resample_close(struct APP_RESAMPLE_T *resamp)
{
    return app_resample_close(resamp);
}

int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len)
{
    uint32_t in_size, out_size;
    struct RESAMPLE_IO_BUF_T io;
    enum RESAMPLE_STATUS_T status;
    int ret;
    //uint32_t lock;

    if (resamp == NULL)
    {
        goto _err_exit;
    }

    io.out_cyclic_start = NULL;
    io.out_cyclic_end = NULL;

    if (resamp->offset < resamp->iter_len)
    {
        io.in = resamp->iter_buf + resamp->offset;
        io.in_size = resamp->iter_len - resamp->offset;
        io.out = buf;
        io.out_size = len;

        //lock = int_lock();
        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        //int_unlock(lock);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        buf += out_size;
        len -= out_size;
        resamp->offset += in_size;

        ASSERT(len == 0 || resamp->offset == resamp->iter_len,
            "%s: Bad resample offset: len=%d offset=%u iter_len=%u",
            __func__, len, resamp->offset, resamp->iter_len);
    }

    while (len)
    {
        ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
        if (ret)
        {
            goto _err_exit;
        }

        io.in = resamp->iter_buf;
        io.in_size = resamp->iter_len;
        io.out = buf;
        io.out_size = len;

        //lock = int_lock();
        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        //int_unlock(lock);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        ASSERT(out_size <= len, "%s: Bad resample out_size: out_size=%u len=%d", __func__, out_size, len);
        ASSERT(in_size <= resamp->iter_len, "%s: Bad resample in_size: in_size=%u iter_len=%u", __func__, in_size, resamp->iter_len);

        buf += out_size;
        len -= out_size;
        if (in_size != resamp->iter_len)
        {
            resamp->offset = in_size;

            ASSERT(len == 0, "%s: Bad resample len: len=%d out_size=%u", __func__, len, out_size);
        }
    }

    return 0;

_err_exit:
    if (resamp)
    {
        app_resample_reset(resamp);
    }

    memzero_int16(buf, len);

    return 1;
}

struct APP_RESAMPLE_T *app_capture_resample_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len)
{
    const struct RESAMPLE_COEF_T *coef = NULL;

    if (sample_rate == AUD_SAMPRATE_8000)
    {
        coef = &resample_coef_8p4k_to_8k;
    }
    else if (sample_rate == AUD_SAMPRATE_16000)
    {
        // Same coef as 8K sample rate
        coef = &resample_coef_8p4k_to_8k;
    }
    else
    {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
    }

    return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len, 0);

}

#ifdef RESAMPLE_ANY_SAMPLE_RATE
struct APP_RESAMPLE_T *app_capture_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
        APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
        float ratio_step)
{
    const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;
    return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len, ratio_step);
}
#endif

int app_capture_resample_close(struct APP_RESAMPLE_T *resamp)
{
    return app_resample_close(resamp);
}

int app_capture_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf, uint32_t len)
{
    uint32_t in_size, out_size;
    struct RESAMPLE_IO_BUF_T io;
    enum RESAMPLE_STATUS_T status;
    int ret;

    if (resamp == NULL)
    {
        goto _err_exit;
    }

    io.out_cyclic_start = NULL;
    io.out_cyclic_end = NULL;

    if (resamp->offset < resamp->iter_len)
    {
        io.in = buf;
        io.in_size = len;
        io.out = resamp->iter_buf + resamp->offset;
        io.out_size = resamp->iter_len - resamp->offset;

        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        buf += in_size;
        len -= in_size;
        resamp->offset += out_size;

        ASSERT(len == 0 || resamp->offset == resamp->iter_len,
            "%s: Bad resample offset: len=%d offset=%u iter_len=%u",
            __func__, len, resamp->offset, resamp->iter_len);

        if (resamp->offset == resamp->iter_len)
        {
            ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
            if (ret)
            {
                goto _err_exit;
            }
        }
    }

    while (len)
    {
        io.in = buf;
        io.in_size = len;
        io.out = resamp->iter_buf;
        io.out_size = resamp->iter_len;

        status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size, &out_size);
        if (status != RESAMPLE_STATUS_OUT_FULL && status != RESAMPLE_STATUS_IN_EMPTY &&
            status != RESAMPLE_STATUS_DONE)
        {
            goto _err_exit;
        }

        ASSERT(in_size <= len, "%s: Bad resample in_size: in_size=%u len=%u", __func__, in_size, len);
        ASSERT(out_size <= resamp->iter_len, "%s: Bad resample out_size: out_size=%u iter_len=%u", __func__, out_size, resamp->iter_len);

        buf += in_size;
        len -= in_size;
        if (out_size == resamp->iter_len)
        {
            ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
            if (ret)
            {
                goto _err_exit;
            }
        }
        else
        {
            resamp->offset = out_size;

            ASSERT(len == 0, "%s: Bad resample len: len=%u in_size=%u", __func__, len, in_size);
        }
    }

    return 0;

_err_exit:
    if (resamp)
    {
        app_resample_reset(resamp);
    }

    memzero_int16(buf, len);

    return 1;
}

void app_resample_reset(struct APP_RESAMPLE_T *resamp)
{
    audio_resample_ex_flush((RESAMPLE_ID *)resamp->id);
    resamp->offset = resamp->iter_len;
}

void app_resample_tune(struct APP_RESAMPLE_T *resamp, float ratio)
{
    float new_step;

    if (resamp == NULL)
    {
        return;
    }

    TRACE_AUD_STREAM_I("%s: stream=%d ratio=%d", __FUNCTION__, resamp->stream, FLOAT_TO_PPB_INT(ratio));

    if (resamp->stream == AUD_STREAM_PLAYBACK) {
        new_step = resamp->ratio_step + resamp->ratio_step * ratio;
    } else {
        new_step = resamp->ratio_step - resamp->ratio_step * ratio;
    }
    audio_resample_ex_set_ratio_step(resamp->id, new_step);
}

APP_RESAMPLE_BUF_ALLOC_CALLBACK app_resample_set_buf_alloc_callback(APP_RESAMPLE_BUF_ALLOC_CALLBACK cb)
{
    APP_RESAMPLE_BUF_ALLOC_CALLBACK old_cb;

    old_cb = resamp_buf_alloc;
    resamp_buf_alloc = cb;

    return old_cb;
}

#ifdef TX_RX_PCM_MASK

#ifdef SCO_DMA_SNAPSHOT

#define MSBC_LEN  60

void store_encode_frame2buff()
{
    if(bt_sco_codec_is_msbc())
    {
    uint32_t len;
    //processing uplink msbc data.
        if(playback_buf_btpcm_copy!=NULL)
        {
            len=playback_size_btpcm_copy-MSBC_LEN;
        memcpy((uint8_t *)(*(volatile uint32_t *)(MIC_BUFF_ADRR_REG)),playback_buf_btpcm_copy,MSBC_LEN);
        memcpy(playback_buf_btpcm_copy,playback_buf_btpcm_copy+MSBC_LEN,len);
        }
    //processing downlink msbc data.
        if(capture_buf_btpcm_copy!=NULL)
        {
            len=capture_size_btpcm_copy-MSBC_LEN;
        memcpy(capture_buf_btpcm_copy,capture_buf_btpcm_copy+MSBC_LEN,len);
        memcpy(capture_buf_btpcm_copy+len,(uint8_t *)(*(volatile uint32_t *)(RX_BUFF_ADRR)),MSBC_LEN);
        }
#if defined(CHIP_BEST2300A)
        uint8_t sco_toggle = *(volatile uint8_t *)(RX_BUFF_ADRR+8);
        pcm_data_param[sco_toggle].curr_time = *(volatile uint32_t *)(RX_BUFF_ADRR+4);
        pcm_data_param[sco_toggle].toggle = sco_toggle;
        pcm_data_param[sco_toggle].flag = *(volatile uint8_t *)(RX_BUFF_ADRR+9);
        pcm_data_param[sco_toggle].counter = *(volatile uint16_t *)(RX_BUFF_ADRR+10);
#endif
    }
    return;
}
#else
extern CQueue* get_tx_esco_queue_ptr();
extern CQueue* get_rx_esco_queue_ptr();
void store_encode_frame2buff()
{
    CQueue* Tx_esco_queue_temp = NULL;
    CQueue* Rx_esco_queue_temp = NULL;
    Tx_esco_queue_temp = get_tx_esco_queue_ptr();
    Rx_esco_queue_temp = get_rx_esco_queue_ptr();
    unsigned int len;
    len= 60;
    int status = 0;
    if(bt_sco_codec_is_msbc())
    {
        status = DeCQueue(Tx_esco_queue_temp,(uint8_t *)(*(volatile uint32_t *)(MIC_BUFF_ADRR_REG)),len);
        if(status){
            //TRACE_AUD_STREAM_I("TX DeC Fail");
        }
        status =EnCQueue(Rx_esco_queue_temp, (uint8_t *)(*(volatile uint32_t *)(RX_BUFF_ADRR)), len);
        if(status){
            //TRACE_AUD_STREAM_I("RX EnC Fail");
        }
    }

}
#endif
#endif

int app_bt_stream_init(void)
{
    app_bt_stream_trigger_checker_init();
    return 0;
}

