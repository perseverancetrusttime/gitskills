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
#include "string.h"
#include "app_utils.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "anc_process.h"
#include "audioflinger.h"
#include "hwtimer_list.h"
#include "audio_dump.h"
#include "speech_cfg.h"
#include "anc_assist.h"
#include "app_anc_assist.h"
#include "assist/anc_assist_defs.h"
#include "assist/anc_assist_utils.h"
#include "assist/anc_assist_anc.h"
#include "assist/anc_assist_mic.h"
#include "assist/anc_assist_tws_sync.h"
#include "assist/anc_assist_resample.h"
#include "assist/anc_assist_thread.h"
#include "assist_ultrasound.h"
#include "app_anc_assist_trigger.h"
#include "app_voice_assist_ultrasound.h"

// #define ANC_ASSIST_AUDIO_DUMP_96K

// #define ANC_ASSIST_AUDIO_DUMP

#define _LOOP_CNT_FIXED_MAX

#define ANC_ASSIST_UPDATE_SYSFREQ

#define ALGO_SAMPLE_RATE  	(16000)
#define ALGO_CHANNEL_NUM   	(3)
#define ALGO_FRAME_LEN   	(120)
#define ALGO_FRAME_MS     	(1000.0 * ALGO_FRAME_LEN / ALGO_SAMPLE_RATE)

#ifdef DUAL_MIC_RECORDING
#define SAMPLE_RATE_MAX   	(48000)
#elif defined (VOICE_ASSIST_WD_ENABLED)
#define SAMPLE_RATE_MAX   	(96000)
#else
#define SAMPLE_RATE_MAX   	(16000)
#endif

#define _SAMPLE_BITS        (24)
#define _CHANNEL_NUM        (ALGO_CHANNEL_NUM + 1)
#define _FRAME_LEN          (ALGO_FRAME_LEN * SAMPLE_RATE_MAX / ALGO_SAMPLE_RATE)
#ifdef VOICE_ASSIST_WD_ENABLED
#define _LOOP_CNT			(2)
#else
#define _LOOP_CNT			(3)
#endif

#ifdef VOICE_ASSIST_WD_ENABLED
#define _PLAY_SAMPLE_RATE	(96000)
#else
#define _PLAY_SAMPLE_RATE	(48000)
#endif
#define _PLAY_CHANNEL_NUM  	(1)
#define _PLAY_FRAME_LEN		(ALGO_FRAME_LEN * _PLAY_SAMPLE_RATE / ALGO_SAMPLE_RATE)

#if _SAMPLE_BITS != 24
#pragma message ("APP ANC ASSIST: Just support 24 bits!!!")
#endif

typedef enum {
    _ASSIST_MSG_OPEN_CLOSE = 0,
	_ASSIST_MSG_CLOSE_SPK,
} _ASSIST_MSG_T;

// Capture stream
#define AF_PINGPONG					(2)
#define STREAM_CAPTURE_ID			AUD_STREAM_ID_0
#define CODEC_CAPTURE_BUF_SIZE    	(_FRAME_LEN * sizeof(_PCM_T) * _CHANNEL_NUM * AF_PINGPONG * _LOOP_CNT)
static uint8_t __attribute__((aligned(4))) codec_capture_buf[CODEC_CAPTURE_BUF_SIZE];

#define CAPTURE_BUF_LEN 			(_FRAME_LEN * _LOOP_CNT)
static float capture_buf[_CHANNEL_NUM][CAPTURE_BUF_LEN];

// Play stream
#if !defined(ANC_ASSIST_PILOT_TONE_ALWAYS_ON)
#define STREAM_PLAY_ID				AUD_STREAM_ID_0
#define STREAM_PLAY_CODEC			AUD_STREAM_USE_INT_CODEC
#else
#define STREAM_PLAY_ID				AUD_STREAM_ID_3
#define STREAM_PLAY_CODEC			AUD_STREAM_USE_INT_CODEC2
#endif

#define CODEC_PLAY_BUF_SIZE    		(_PLAY_FRAME_LEN * sizeof(_PCM_T) * _PLAY_CHANNEL_NUM * AF_PINGPONG * _LOOP_CNT)
static uint8_t __attribute__((aligned(4))) codec_play_buf[CODEC_PLAY_BUF_SIZE];

#define PLAY_BUF_LEN 				(_PLAY_FRAME_LEN * _LOOP_CNT)
static float play_buf[PLAY_BUF_LEN];

static int32_t g_sample_rate = ALGO_SAMPLE_RATE;
static int32_t g_sample_bits = _SAMPLE_BITS;
static int32_t g_chan_num = _CHANNEL_NUM;
static int32_t g_frame_len = ALGO_FRAME_LEN;
static int32_t g_loop_cnt = _LOOP_CNT;
static int32_t g_capture_buf_size = CODEC_CAPTURE_BUF_SIZE;

static int32_t g_play_buf_size = CODEC_PLAY_BUF_SIZE;

static enum APP_SYSFREQ_FREQ_T g_sys_freq = APP_SYSFREQ_32K;
bool g_opened_flag = false;
static anc_assist_mode_t g_anc_assist_mode = ANC_ASSIST_MODE_QTY;

static bool g_need_open_mic = true;
static bool g_need_open_spk = true;
static bool g_mic_open_flag = false;
static bool g_spk_open_flag = false;

static float *g_ff_mic_buf 		= NULL;
static float *g_fb_mic_buf 		= NULL;
static float *g_talk_mic_buf 	= NULL;
static float *g_ref_mic_buf 	= NULL;

static AncAssistState *anc_assist_st = NULL;
static AncAssistRes anc_assist_res;
extern AncAssistConfig anc_assist_cfg;
static AncAssistConfig anc_assist_cfg_new;

static uint32_t g_user_status = 0;
static anc_assist_user_callback_t g_user_callback[ANC_ASSIST_USER_QTY];

static void _open_mic(void);
static void _open_spk(void);
static void _close_mic(void);
static void _close_spk(void);

static int32_t _anc_assist_open_impl(AncAssistConfig *cfg);
static int32_t _anc_assist_close_impl(void);

#ifdef ANC_ASSIST_UPDATE_SYSFREQ
static void _anc_assist_update_sysfreq(void);
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP
typedef short		_DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[ALGO_FRAME_LEN];
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
typedef int16_t		_DUMP_PCM_T;
static _DUMP_PCM_T audio_dump_buf[_FRAME_LEN * _LOOP_CNT];
#endif

#include "cmsis_os.h"

bool ultrasound_process_flag = true;
osMutexId _anc_assist_mutex_id = NULL;
osMutexDef(_anc_assist_mutex);

static void _anc_assist_create_lock(void)
{
    if (_anc_assist_mutex_id == NULL) {
        _anc_assist_mutex_id = osMutexCreate((osMutex(_anc_assist_mutex)));
    }
}

static void _anc_assist_lock(void)
{
    osMutexWait(_anc_assist_mutex_id, osWaitForever);
}

static void _anc_assist_unlock(void)
{
    osMutexRelease(_anc_assist_mutex_id);
}

static int32_t update_stream_cfg(AncAssistConfig *cfg)
{
	_anc_assist_lock();
	g_need_open_mic = true;
	g_need_open_spk = true;
	g_loop_cnt      = _LOOP_CNT;
#ifdef VOICE_ASSIST_WD_ENABLED
	g_sample_rate   = AUD_SAMPRATE_96000;
#else
	g_sample_rate   = ALGO_SAMPLE_RATE;
#endif
    enum AUD_IO_PATH_T app_path = AUD_IO_PATH_NULL;

	if (g_anc_assist_mode == ANC_ASSIST_MODE_STANDALONE) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_STANDALONE", __func__);
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_PHONE_CALL", __func__);
		g_need_open_mic = false;
		g_loop_cnt = 2;
        app_path = AUD_INPUT_PATH_MAINMIC;
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_RECORD) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_RECORD", __func__);
		g_need_open_mic = false;
		// g_need_open_spk = false;
		g_loop_cnt = 2;
		g_sample_rate = AUD_SAMPRATE_48000;
        app_path = AUD_INPUT_PATH_ASRMIC;
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC", __func__);
		g_loop_cnt = 3;
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_AAC) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC_AAC", __func__);
		g_loop_cnt = 3;
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_SBC) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_MUSIC_SBC", __func__);
		g_loop_cnt = 2;
	} else if (g_anc_assist_mode == ANC_ASSIST_MODE_NONE) {
		TRACE(0, "[%s] ANC_ASSIST_MODE_NONE", __func__);
		g_need_open_mic = false;
		g_need_open_spk = false;
	} else {
		ASSERT(0, "[%s] g_anc_assist_mode(%d) is invalid!", __func__, g_anc_assist_mode);
	}

    if (app_path == AUD_IO_PATH_NULL) {
        anc_assist_mic_update_anc_cfg(cfg);
        anc_assist_mic_set_app_cfg(AUD_IO_PATH_NULL);
        anc_assist_mic_parser_index(AUD_INPUT_PATH_ANC_ASSIST);
        g_chan_num = anc_assist_mic_get_ch_num(AUD_INPUT_PATH_ANC_ASSIST);
    } else {
        anc_assist_mic_set_anc_cfg(AUD_INPUT_PATH_ANC_ASSIST);
        anc_assist_mic_set_app_cfg(app_path);
        anc_assist_mic_parser_index(app_path);
        g_chan_num = anc_assist_mic_get_ch_num(app_path);
    }

	if (anc_assist_spk_parser_anc_cfg(cfg) == 0) {
		g_need_open_spk = false;
	}

#if defined(_LOOP_CNT_FIXED_MAX)
    g_loop_cnt = _LOOP_CNT;
#endif

	g_frame_len = ALGO_FRAME_LEN * (g_sample_rate / ALGO_SAMPLE_RATE);
	g_capture_buf_size = g_frame_len * sizeof(_PCM_T) * g_chan_num * AF_PINGPONG * g_loop_cnt;
	g_play_buf_size = (CODEC_PLAY_BUF_SIZE / _LOOP_CNT) * g_loop_cnt;

    ANC_ASSIST_TRACE(0, "[%s] Need to open MIC(%d), SPK(%d)", __func__, g_need_open_mic, g_need_open_spk);
    ANC_ASSIST_TRACE(0, "[%s] fs: %d, chan_num: %d, loop_cnt: %d", __func__, g_sample_rate, g_chan_num, g_loop_cnt);
    _anc_assist_unlock();

    return 0;
}

#ifdef VOICE_ASSIST_WD_ENABLED
static void update_trigger_status(void)
{
	app_anc_assist_trigger_init();

	if (g_mic_open_flag) {
		af_stream_start(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
	}

	if (g_spk_open_flag) {
		af_stream_start(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
	}
}
#endif

static int32_t update_codec_status(void)
{
	// MIC
	uint32_t mic_cfg_old = anc_assist_mic_parser_anc_cfg(&anc_assist_cfg);
	uint32_t mic_cfg_new = anc_assist_mic_parser_anc_cfg(&anc_assist_cfg_new);

	// SPK
	uint32_t spk_cfg_old = anc_assist_spk_parser_anc_cfg(&anc_assist_cfg);
	uint32_t spk_cfg_new = anc_assist_spk_parser_anc_cfg(&anc_assist_cfg_new);

	// Temporary
	app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, HAL_CMU_FREQ_104M);

	if (spk_cfg_old != spk_cfg_new) {
		if (g_spk_open_flag) {
			_close_spk();
		}
	}

	if (mic_cfg_old != mic_cfg_new) {
		if (g_mic_open_flag) {
			_close_mic();
		}
	}

    anc_assist_set_cfg_sync(anc_assist_st, &anc_assist_cfg_new);

	if (mic_cfg_old != mic_cfg_new) {
		if (mic_cfg_new) {
			_open_mic();
		}
	}

	if (spk_cfg_old != spk_cfg_new) {
		if (spk_cfg_new) {
			_open_spk();
		}
	}

#ifdef VOICE_ASSIST_WD_ENABLED
	update_trigger_status();
#endif

#ifdef ANC_ASSIST_UPDATE_SYSFREQ
    _anc_assist_update_sysfreq();
#else
    g_sys_freq = APP_SYSFREQ_26M;
	app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);
    // TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

	return 0;
}

static int32_t update_algo_cfg(uint32_t user_status, AncAssistConfig *cfg)
{
	cfg->ff_howling_en = false;
	cfg->fb_howling_en = false;
	cfg->noise_en = false;
	cfg->noise_classify_en = false;
	cfg->wind_en = false;
	cfg->pilot_en = false;
	cfg->pnc_en = false;
	cfg->wsd_en = false;
	cfg->extern_kws_en = false;
	cfg->ultrasound_en = false;
	cfg->prompt_adaptive_en = false;

	for (uint32_t i=0; i<ANC_ASSIST_USER_QTY; i++) {
		if (user_status & (0x1 << i)) {
			if (i ==  ANC_ASSIST_USER_ANC) {
				TRACE(0, "[%s] ANC_ASSIST_USER_ANC", __func__);
				cfg->ff_howling_en = false;
				cfg->fb_howling_en = false;
				cfg->wind_en = true;
			} else if (i ==  ANC_ASSIST_USER_PSAP) {
				TRACE(0, "[%s] ANC_ASSIST_USER_PSAP", __func__);
				cfg->ff_howling_en = true;
				cfg->fb_howling_en = true;
				cfg->wind_en = true;
			} else if (i ==  ANC_ASSIST_USER_KWS) {
				TRACE(0, "[%s] ANC_ASSIST_USER_KWS", __func__);
				// cfg->wsd_en = true;
				cfg->extern_kws_en = true;
			} else if (i ==  ANC_ASSIST_USER_ULTRASOUND) {
				TRACE(0, "[%s] ANC_ASSIST_USER_ULTRASOUND", __func__);
				cfg->ultrasound_en = true;
			} else if (i ==  ANC_ASSIST_USER_WD) {
				TRACE(0, "[%s] ANC_ASSIST_USER_WD", __func__);
				cfg->pilot_en = true;
				cfg->pilot_cfg.adaptive_anc_en = 0;
				cfg->pilot_cfg.wd_en = 1;
				cfg->pilot_cfg.custom_leak_detect_en = 0;
			} else if (i ==  ANC_ASSIST_USER_CUSTOM_LEAK_DETECT) {
				TRACE(0, "[%s] ANC_ASSIST_USER_CUSTOM_LEAK_DETECT", __func__);
				cfg->pilot_en = true;
				cfg->pilot_cfg.adaptive_anc_en = 0;
				cfg->pilot_cfg.wd_en = 0;
				cfg->pilot_cfg.custom_leak_detect_en = 1;
				// cfg->fb_howling_en = true;
			} else if(i == ANC_ASSIST_USER_ONESHOT_ADAPT_ANC){
				TRACE(0, "[%s] ANC_ASSIST_USER_ONESHOT_ADAPT_ANC", __func__);
				cfg->ff_howling_en = true;
				cfg->fb_howling_en = true;
				cfg->wind_en = true;
			} else if(i == ANC_ASSIST_USER_PROMPT_LEAK_DETECT){
				TRACE(0, "[%s] ANC_ASSIST_USER_PROMPT_LEAK_DETECT", __func__);
				cfg->prompt_adaptive_en = true;
			} else if(i == ANC_ASSIST_USER_PREPARE){
				TRACE(0, "[%s] ANC_ASSIST_USER_PREPARE", __func__);
				cfg->ff_howling_en = true;
				cfg->fb_howling_en = true;
				cfg->wind_en = true;
			} else {
				ASSERT(0, "[%s] i(%d) is invalid!", __func__, i);
			}
		}
	}

	return 0;
}

static int32_t _assist_open_close_msg_post(anc_assist_user_t user, bool open_opt)
{
    ANC_ASSIST_TRACE(0, "[%s] ", __func__);

    ANC_ASSIST_MESSAGE_BLOCK msg;
    msg.mod_id = ANC_ASSIST_MODUAL_VOICE_ASSIST;
    msg.msg_body.message_id = _ASSIST_MSG_OPEN_CLOSE;
    msg.msg_body.message_Param0 = user;
    msg.msg_body.message_Param1 = open_opt;
    anc_assist_mailbox_put(&msg);

    return 0;
}

static bool ultrasound_reset_needed = true;
static int app_anc_assist_thread_handler(ANC_ASSIST_MESSAGE_BODY *msg_body)
{
    uint32_t id = msg_body->message_id;
	// anc_assist_user_t user = ANC_ASSIST_USER_QTY;

    ANC_ASSIST_TRACE(0, "[%s] id :%d", __func__, id);

    switch (id) {
		case _ASSIST_MSG_OPEN_CLOSE: {
			anc_assist_user_t user = (uint32_t)(msg_body->message_Param0);
			bool open_opt = msg_body->message_Param1;
			uint32_t user_status_old = g_user_status;
			uint32_t user_status_new = 0;

			if (open_opt == true) {
				user_status_new = user_status_old | (0x1 << user);
			} else {
				user_status_new = user_status_old & ~(0x1 << user);
			}

			ANC_ASSIST_TRACE(0, "[%s] opt: %d, old: 0x%x, new: 0x%x", __func__, open_opt, user_status_old, user_status_new);

			if (user_status_old != user_status_new) {
				update_algo_cfg(user_status_new, &anc_assist_cfg_new);

				if (open_opt == true && user == ANC_ASSIST_USER_ULTRASOUND) {
					ultrasound_reset_needed = true;
				}

#ifdef VOICE_ASSIST_WD_ENABLED
				if (open_opt == false && user == ANC_ASSIST_USER_WD) {
					anc_assist_pilot_set_play_fadeout(anc_assist_st);
				}
#endif

				if ((user_status_old == 0) && (user_status_new != 0)) {		// Open
					update_stream_cfg(&anc_assist_cfg_new);
					_anc_assist_open_impl(&anc_assist_cfg_new);
				} else if ((user_status_old != 0) && (user_status_new == 0)) {	// Close
					_anc_assist_close_impl();
				} else {	// Update
					update_stream_cfg(&anc_assist_cfg_new);
					update_codec_status();
				}

				// Save parameters
				anc_assist_cfg = anc_assist_cfg_new;
				g_user_status = user_status_new;
			}
			break;
		}
		default:
			ASSERT(0, "[%s] id(%d) is invalid", __func__, id);
			break;
    }

    return 0;
}

int32_t app_anc_assist_init(void)
{
	g_opened_flag = false;
	g_anc_assist_mode = ANC_ASSIST_MODE_STANDALONE;

	g_ff_mic_buf 	= capture_buf[MIC_INDEX_FF];
	g_fb_mic_buf 	= capture_buf[MIC_INDEX_FB];
	g_talk_mic_buf 	= capture_buf[MIC_INDEX_TALK];
	g_ref_mic_buf 	= capture_buf[MIC_INDEX_REF];

	g_user_status = 0;
	for (uint32_t i = 0; i<ANC_ASSIST_USER_QTY; i++) {
		g_user_callback[i] = NULL;
	}

	anc_assist_cfg_new = anc_assist_cfg;

	_anc_assist_create_lock();
	anc_assist_mic_reset();

	anc_assist_thread_init();
	anc_assist_set_threadhandle(ANC_ASSIST_MODUAL_VOICE_ASSIST, app_anc_assist_thread_handler);

	return 0;
}

int32_t app_anc_assist_deinit(void)
{
	return 0;
}

int32_t app_anc_assist_register(anc_assist_user_t user, anc_assist_user_callback_t func)
{
	TRACE(0, "[%s] user: %d", __func__, user);

	g_user_callback[user] = func;

	return 0;
}

// TODO: Currently, just used by sco. Can extend
uint32_t app_anc_assist_get_mic_ch_num(enum AUD_IO_PATH_T path)
{
	return anc_assist_mic_get_ch_num(path);
}

uint32_t app_anc_assist_get_mic_ch_map(enum AUD_IO_PATH_T path)
{
	return anc_assist_mic_get_cfg(path);
}

int32_t app_anc_assist_parser_app_mic_buf(void *buf, uint32_t *len)
{
	_anc_assist_lock();
	anc_assist_mic_parser_app_buf(buf, len, capture_buf, sizeof(capture_buf));
	_anc_assist_unlock();

	return 0;
}

static bool _need_switch_mode(anc_assist_mode_t old_mode, anc_assist_mode_t new_mode)
{
    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC)         &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC_AAC)     &&
        (g_loop_cnt == 3)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_STANDALONE)    &&
        (new_mode == ANC_ASSIST_MODE_MUSIC_SBC)     &&
        (g_loop_cnt == 2)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC_AAC)     &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }

    if ((old_mode == ANC_ASSIST_MODE_MUSIC_SBC)     &&
        (new_mode == ANC_ASSIST_MODE_STANDALONE)) {
            return false;
    }

    return true;
}

int32_t app_anc_assist_set_mode(anc_assist_mode_t mode)
{
    ANC_ASSIST_TRACE(0, "[%s] %d --> %d", __func__, g_anc_assist_mode, mode);

    if (g_anc_assist_mode == mode) {
        TRACE(0, "[%s] WARNING: Same mode = %d", __func__, mode);
        return 1;
    }

    if (_need_switch_mode(g_anc_assist_mode, mode) == false) {
        g_anc_assist_mode = mode;
#ifdef ANC_ASSIST_UPDATE_SYSFREQ
        if (g_opened_flag) {
            _anc_assist_update_sysfreq();
        }
#endif
        return 0;
    }

    g_anc_assist_mode = mode;
    if (g_opened_flag) {
        ANC_ASSIST_TRACE(0, "[%s] ------ START ------", __func__);
        _anc_assist_close_impl();
        update_stream_cfg(&anc_assist_cfg);
        _anc_assist_open_impl(&anc_assist_cfg);
        ANC_ASSIST_TRACE(0, "[%s] ------ END ------", __func__);
    } else {
        update_stream_cfg(&anc_assist_cfg);
    }

    return 0;
}

int32_t app_anc_assist_is_runing(void)
{
	return g_opened_flag;
}

static void _anc_assist_callback(anc_assist_notify_t msg, void *data, uint32_t value)
{
    switch (msg) {
        case ANC_ASSIST_NOTIFY_FREQ:
#ifdef ANC_ASSIST_UPDATE_SYSFREQ
            _anc_assist_update_sysfreq();
#else
            TRACE(0, "[%s] TODO: Impl ANC_ASSIST_NOTIFY_FREQ", __func__);
#endif
            break;
        default:
            break;
    }
}

static int32_t _anc_assist_open_impl(AncAssistConfig *cfg)
{
    ANC_ASSIST_TRACE(0, "[%s] ...", __func__);

	if (g_opened_flag == true) {
		TRACE(0, "[%s] WARNING: g_opened_flag is true", __func__);
		return 1;
	}

	// Temporary
	app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, HAL_CMU_FREQ_104M);

	_anc_assist_lock();
	anc_assist_st = anc_assist_create(ALGO_SAMPLE_RATE, g_sample_bits, ALGO_CHANNEL_NUM, ALGO_FRAME_LEN, cfg, _anc_assist_callback);
	anc_assist_pilot_set_play_sample_rate(anc_assist_st, _PLAY_SAMPLE_RATE);

#ifdef ANC_ASSIST_AUDIO_DUMP
	audio_dump_init(ALGO_FRAME_LEN, sizeof(_DUMP_PCM_T), 4);
#endif

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
	// audio_dump_init(g_frame_len * g_loop_cnt, sizeof(_DUMP_PCM_T), 2);

	audio_dump_init(193, sizeof(_DUMP_PCM_T), 2);
#endif

	anc_assist_anc_init();
	anc_assist_tws_sync_init(ALGO_FRAME_MS * g_loop_cnt);
    // TODO: If need to resample, call process many times to reduce RAM usage.
    // anc_assist_resample_init(g_sample_rate, g_frame_len * g_loop_cnt);
	anc_assist_resample_init(g_sample_rate, g_frame_len);

	g_opened_flag = true;
	_anc_assist_unlock();

	_open_mic();
	_open_spk();

#ifdef VOICE_ASSIST_WD_ENABLED
	update_trigger_status();
#endif

#ifdef ANC_ASSIST_UPDATE_SYSFREQ
    _anc_assist_update_sysfreq();
#else
    g_sys_freq = APP_SYSFREQ_26M;
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif
#endif

    return 0;
}

static int32_t _anc_assist_close_impl(void)
{
    ANC_ASSIST_TRACE(0, "[%s] ...", __func__);

	if (g_opened_flag == false) {
		TRACE(0, "[%s] WARNING: g_opened_flag is false", __func__);
		return 1;
	}

	// Temporary
	app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, HAL_CMU_FREQ_104M);

	_close_spk();
	_close_mic();

#ifdef VOICE_ASSIST_WD_ENABLED
	app_anc_assist_trigger_deinit();
#endif

	_anc_assist_lock();
	g_opened_flag = false;

	anc_assist_anc_deinit();
	anc_assist_resample_deinit();

	anc_assist_destroy(anc_assist_st);
	_anc_assist_unlock();

	g_sys_freq = APP_SYSFREQ_32K;
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif

    return 0;
}

int32_t app_anc_assist_open(anc_assist_user_t user)
{
	ANC_ASSIST_TRACE(0, "[%s] g_user: 0x%x, user: %d", __func__, g_user_status, user);

	_assist_open_close_msg_post(user, true);

	return 0;
}

int32_t app_anc_assist_close(anc_assist_user_t user)
{
	ANC_ASSIST_TRACE(0, "[%s] g_user: 0x%x, user: %d", __func__, g_user_status, user);

	_assist_open_close_msg_post(user, false);

	return 0;
}

bool ultrasound_changed = false;
static int32_t POSSIBLY_UNUSED _process_frame_ultrasound(float *fb_mic, float *ref, uint32_t frame_len)
{
	if (ultrasound_reset_needed) {
		assist_ultrasound_reset();
		ultrasound_reset_needed = false;
	}

	ultrasound_changed = assist_ultrasound_process(fb_mic, ref, frame_len);

	if ((g_user_callback[ANC_ASSIST_USER_ULTRASOUND] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_ULTRASOUND))) {
		uint32_t res = ultrasound_changed;
		g_user_callback[ANC_ASSIST_USER_ULTRASOUND](&res, 1, NULL);
	}

	return 0;
}

void app_anc_assist_reset_pilot_state(void){
	if(anc_assist_st!= NULL){
		anc_assist_reset_pilot_state(anc_assist_st);
	}
	
}

static int32_t POSSIBLY_UNUSED _process_frame(float *ff_mic, float *fb_mic, float *talk_mic, float *ref, uint32_t frame_len)
{
    anc_assist_res = anc_assist_process(anc_assist_st, ff_mic, fb_mic, talk_mic, ref, frame_len);

#if 0
	for (uint32_t i=0; i<ANC_ASSIST_USER_QTY; i++) {
		if ((g_user_callback[i] != NULL) && (g_user_status & (0x1 << i))) {
			g_user_callback[i](talk_mic, frame_len, NULL);
		}
	}
#else
	if ((g_user_callback[ANC_ASSIST_USER_WD] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_WD))) {
		uint32_t res[2];
		res[0] = anc_assist_res.wd_changed;
		res[1] = anc_assist_res.wd_status;
		g_user_callback[ANC_ASSIST_USER_WD](res, 2, NULL);
	}

	if ((g_user_callback[ANC_ASSIST_USER_KWS] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_KWS))) {
		g_user_callback[ANC_ASSIST_USER_KWS](talk_mic, frame_len, NULL);
	}

	if ((g_user_callback[ANC_ASSIST_USER_CUSTOM_LEAK_DETECT] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_CUSTOM_LEAK_DETECT))) {
		float res[2];
		res[0] = anc_assist_res.custom_leak_detect_result;
		res[1] = (float)anc_assist_res.custom_leak_detect_status;
		g_user_callback[ANC_ASSIST_USER_CUSTOM_LEAK_DETECT](res, 2, NULL);
	}

	if ((g_user_callback[ANC_ASSIST_USER_ONESHOT_ADAPT_ANC] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_ONESHOT_ADAPT_ANC))){
		float * input_data[2];
		input_data[0] = ff_mic;
		input_data[1] = fb_mic;
		g_user_callback[ANC_ASSIST_USER_ONESHOT_ADAPT_ANC](input_data, frame_len, &anc_assist_res);
	}

	if ((g_user_callback[ANC_ASSIST_USER_PREPARE] != NULL) &&
		(g_user_status & (0x1 << ANC_ASSIST_USER_PREPARE))){
		float * input_data[2];
		input_data[0] = ff_mic;
		input_data[1] = fb_mic;
		g_user_callback[ANC_ASSIST_USER_PREPARE](input_data, frame_len, &anc_assist_res);
	}
#endif

	if (anc_assist_res.ff_gain_changed) {
		anc_assist_anc_set_gain_coef(anc_assist_res.ff_gain_id, anc_assist_res.ff_gain);
	} else if (anc_assist_res.fb_gain_changed) {
		anc_assist_anc_set_gain_coef(anc_assist_res.fb_gain_id, anc_assist_res.fb_gain);
	} else {
		;
	}

	if (anc_assist_res.curve_changed) {
		anc_assist_anc_switch_curve(anc_assist_res.curve_id, anc_assist_res.curve_index);
	} else {
		;
	}

	anc_assist_tws_sync_heartbeat();

	return 0;
}

extern bool infrasound_fadeout_flag;
int32_t app_anc_assist_process(void *buf, uint32_t len)
{
	_anc_assist_lock();
	uint32_t pcm_len = len / sizeof(_PCM_T);
	uint32_t frame_len = pcm_len / g_chan_num;
	uint32_t loop_cnt = frame_len / ALGO_FRAME_LEN;
	_PCM_T *pcm_buf = (_PCM_T *)buf;

	if (g_opened_flag == false) {
		TRACE(1, "[%s] WARNING: g_opened_flag is false", __func__);
		return -1;
	}

	// TRACE(3, "[%s] len = %d, loop_cnt = %d", __func__, len, g_loop_cnt);

	if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
		anc_assist_mic_parser_anc_buf(AUD_INPUT_PATH_MAINMIC, (float *)capture_buf, CAPTURE_BUF_LEN, pcm_buf, pcm_len);
	} else {
		anc_assist_mic_parser_anc_buf(AUD_INPUT_PATH_ANC_ASSIST, (float *)capture_buf, CAPTURE_BUF_LEN, pcm_buf, pcm_len);
	}

#ifdef ANC_ASSIST_AUDIO_DUMP_96K
	uint32_t dump_ch = 0;
	audio_dump_clear_up();
	int offseti = 105;

	for (uint32_t i = offseti,j = 0; i < offseti + 193; i++,j++) {
		audio_dump_buf[j] = (_PCM_T)g_fb_mic_buf[0][i] >> 8;
	}
	audio_dump_add_channel_data(dump_ch++, audio_dump_buf, 193);

	for (uint32_t i=offseti, j=0; i<193+offseti; i++, j++) {
		audio_dump_buf[j] = (_PCM_T)g_ref_mic_buf[i] >> 8;
	}
	audio_dump_add_channel_data(dump_ch++, audio_dump_buf, 193);

	audio_dump_run();
#endif

	if (g_sample_rate != ALGO_SAMPLE_RATE) {
		ASSERT(g_sample_rate == SAMPLE_RATE_MAX, "[%s] g_sample_rate(%d) is invalid!", __func__, g_sample_rate);

#ifdef VOICE_ASSIST_WD_ENABLED
		_process_frame_ultrasound(g_fb_mic_buf, g_ref_mic_buf, frame_len);
#endif
		anc_assist_resample_process((float *)capture_buf, CAPTURE_BUF_LEN, frame_len);
		loop_cnt /= (g_sample_rate / ALGO_SAMPLE_RATE);
		frame_len /= (g_sample_rate / ALGO_SAMPLE_RATE);
	}
	_anc_assist_unlock();

	for (uint32_t offset = 0, cnt=0; cnt<loop_cnt; cnt++) {
#ifdef ANC_ASSIST_AUDIO_DUMP
		uint32_t dump_ch = 0;
		audio_dump_clear_up();

		// TODO: Use capture buf
		for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
			audio_dump_buf[i] = (_PCM_T)g_ff_mic_buf[offset + i] >> 8;
		}
		audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

		for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
			audio_dump_buf[i] = (_PCM_T)g_fb_mic_buf[offset + i] >> 8;
		}
		audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

		for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
			audio_dump_buf[i] = (_PCM_T)g_talk_mic_buf[offset + i] >> 8;
		}
		audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

		for (uint32_t i=0; i<ALGO_FRAME_LEN; i++) {
			audio_dump_buf[i] = (_PCM_T)g_ref_mic_buf[offset + i] >> 8;
		}
		audio_dump_add_channel_data(dump_ch++, audio_dump_buf, ALGO_FRAME_LEN);

		audio_dump_run();
#endif

		_process_frame(	g_ff_mic_buf 	+ offset,
					g_fb_mic_buf 	+ offset,
					g_talk_mic_buf 	+ offset,
					g_ref_mic_buf 	+ offset,
					ALGO_FRAME_LEN);
		offset += ALGO_FRAME_LEN;
	}

    return 0;
}

static uint32_t codec_capture_stream_callback(uint8_t *buf, uint32_t len)
{
	app_anc_assist_process(buf, len);

    return len;
}

#ifdef VOICE_ASSIST_WD_ENABLED
static const int16_t local_96kHz_pcm_data[] = {
	#include "pilot_oed_pcm.h"
};
#endif

#define ULTRASOUND_VOL (DB2LIN(-40))

static uint32_t codec_play_stream_callback(uint8_t *buf, uint32_t len)
{
	uint32_t pcm_len = len / sizeof(_PCM_T);
	uint32_t frame_len = pcm_len;
	_PCM_T *pcm_buf = (_PCM_T *)buf;

#ifdef VOICE_ASSIST_WD_ENABLED
	app_anc_assist_trigger_checker();
#endif

	anc_assist_pilot_get_play_data(anc_assist_st, play_buf, frame_len);

	for (int32_t i=0; i<frame_len; i++) {
		pcm_buf[i] = (_PCM_T)play_buf[i];
	}

#ifdef VOICE_ASSIST_WD_ENABLED
	for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
		pcm_buf[i] = speech_ssat_int24(pcm_buf[i] + (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8)));
	}
#endif

#if 0//def ANC_ASSIST_AUDIO_DUMP_96K
	audio_dump_clear_up();
	for (uint32_t i=0; i<frame_len; i++) {
		audio_dump_buf[i] = (_PCM_T)pcm_buf[i] >> 8;
	}
	audio_dump_add_channel_data(0, audio_dump_buf, frame_len);
	audio_dump_run();
#endif

    return len;
}

static void _open_mic(void)
{
	if (g_mic_open_flag == true) {
		TRACE(0, "[%s] WARNING: MIC is opened", __func__);
		return;
	}

	if (g_need_open_mic) {
		struct AF_STREAM_CONFIG_T stream_cfg;

		memset(&stream_cfg, 0, sizeof(stream_cfg));
		stream_cfg.channel_num 	= (enum AUD_CHANNEL_NUM_T)g_chan_num;
		stream_cfg.sample_rate 	= (enum AUD_SAMPRATE_T)g_sample_rate;
		stream_cfg.bits 		= (enum AUD_BITS_T)g_sample_bits;
		stream_cfg.vol 			= 12;
		stream_cfg.chan_sep_buf = false;
		stream_cfg.device       = AUD_STREAM_USE_INT_CODEC;
		stream_cfg.io_path      = AUD_INPUT_PATH_ANC_ASSIST;
		stream_cfg.channel_map	= anc_assist_mic_get_cfg(AUD_INPUT_PATH_ANC_ASSIST);
		stream_cfg.handler      = codec_capture_stream_callback;
		stream_cfg.data_size    = g_capture_buf_size;
		stream_cfg.data_ptr     = codec_capture_buf;

        ANC_ASSIST_TRACE(0, "[%s] sample_rate:%d, data_size:%d", __func__, stream_cfg.sample_rate, stream_cfg.data_size);

		af_stream_open(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE, &stream_cfg);
#ifndef VOICE_ASSIST_WD_ENABLED
		af_stream_start(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
#endif
		g_mic_open_flag = true;
	}
}

static void _open_spk(void)
{
	if (g_spk_open_flag == true) {
		TRACE(0, "[%s] WARNING: SPK is opened", __func__);
		return;
	}

	if (g_need_open_spk) {
		struct AF_STREAM_CONFIG_T stream_cfg;

		memset(&stream_cfg, 0, sizeof(stream_cfg));
		stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
		stream_cfg.channel_map = (enum AUD_CHANNEL_MAP_T)AUD_CHANNEL_MAP_CH0;
		stream_cfg.sample_rate = _PLAY_SAMPLE_RATE;
		stream_cfg.bits = AUD_BITS_24;
		stream_cfg.vol = TGT_VOLUME_LEVEL_MAX;
		stream_cfg.chan_sep_buf = false;
		stream_cfg.device       = STREAM_PLAY_CODEC;
		stream_cfg.handler      = codec_play_stream_callback;
		stream_cfg.data_size    = g_play_buf_size;
		stream_cfg.data_ptr     = codec_play_buf;

        ANC_ASSIST_TRACE(0, "[%s] sample_rate:%d, data_size:%d", __func__, stream_cfg.sample_rate, stream_cfg.data_size);

		af_stream_open(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK, &stream_cfg);

		// add the pingpang
#ifdef VOICE_ASSIST_WD_ENABLED
		int32_t *pcm_buf = (int32_t *)codec_play_buf;
		for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
			pcm_buf[i] = (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8));
		}

		pcm_buf = (int32_t *)(codec_play_buf + g_play_buf_size / 2);
		for (int32_t i=0; i<ARRAY_SIZE(local_96kHz_pcm_data); i++) {
			pcm_buf[i] = (int32_t)(ULTRASOUND_VOL * (local_96kHz_pcm_data[i] << 8));
		}
#endif

#ifndef VOICE_ASSIST_WD_ENABLED
		af_stream_start(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
#endif

		g_spk_open_flag = true;
	}
}

static void _close_mic(void)
{
	if (g_mic_open_flag == false) {
		TRACE(0, "[%s] WARNING: MIC is closed", __func__);
		return;
	}

	ANC_ASSIST_TRACE(0, "[%s] ...", __func__);
	af_stream_stop(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);
	af_stream_close(STREAM_CAPTURE_ID, AUD_STREAM_CAPTURE);

	g_mic_open_flag = false;
}

extern bool pilot_play_get_fadeout_state(void);
static void _close_spk(void)
{
	if (g_spk_open_flag == false) {
		TRACE(0, "[%s] WARNING: SPK is closed", __func__);
		return;
	}

#ifndef VOICE_ASSIST_WD_ENABLED
	anc_assist_pilot_set_play_fadeout(anc_assist_st);
	osDelay(anc_assist_cfg.pilot_cfg.gain_smooth_ms + 300);     // 300: More time to fadeout
#else
	if (pilot_play_get_fadeout_state()) {
		osDelay(anc_assist_cfg.pilot_cfg.gain_smooth_ms + 300);
	} else {
		osDelay(10);
	}
#endif

	ANC_ASSIST_TRACE(0, "[%s] ...", __func__);
	af_stream_stop(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);
	af_stream_close(STREAM_PLAY_ID, AUD_STREAM_PLAYBACK);

	g_spk_open_flag = false;
}


#ifdef ANC_ASSIST_UPDATE_SYSFREQ

#define ANC_ASSIST_BASE_MIPS (3)

#define ULTRA_INFRASOUND_BASE_MIPS (20)     //need to be changed

static enum APP_SYSFREQ_FREQ_T _anc_assist_get_sysfreq(void)
{
    enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_32K;
#ifdef VOICE_ASSIST_WD_ENABLED
    int32_t required_mips = anc_assist_get_required_mips(anc_assist_st) + ANC_ASSIST_BASE_MIPS + ULTRA_INFRASOUND_BASE_MIPS;
#else
	int32_t required_mips = anc_assist_get_required_mips(anc_assist_st) + ANC_ASSIST_BASE_MIPS;
#endif
    ANC_ASSIST_TRACE(0, "[%s] Required mips: %dM", __func__,  required_mips);

    if (required_mips >= 96) {
        freq = APP_SYSFREQ_208M;
    } else if(required_mips >= 72) {
        freq = APP_SYSFREQ_104M;
    } else if (required_mips >= 48) {
        freq = APP_SYSFREQ_78M;
    } else if (required_mips >= 24) {
        freq = APP_SYSFREQ_52M;
    } else {
        freq = APP_SYSFREQ_26M;
    }

    // NOTE: Optimize power consumption for special project
    if (g_anc_assist_mode == ANC_ASSIST_MODE_PHONE_CALL) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_RECORD) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }
    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }

    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_AAC) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }

    } else if (g_anc_assist_mode == ANC_ASSIST_MODE_MUSIC_SBC) {
        // if (required_mips < 24) {
        //     freq = APP_SYSFREQ_32K;
        // }

    }

    return freq;
}

static void _anc_assist_update_sysfreq(void)
{
    g_sys_freq = _anc_assist_get_sysfreq();
    app_sysfreq_req(APP_SYSFREQ_USER_VOICE_ASSIST, g_sys_freq);

#if defined(ENABLE_CALCU_CPU_FREQ_LOG)
    TRACE(0, "[%s] Sys freq[%d]: %d", __func__, g_sys_freq, hal_sys_timer_calc_cpu_freq(5, 0));
#endif
}
#endif