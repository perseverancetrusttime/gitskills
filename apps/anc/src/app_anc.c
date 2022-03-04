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
#include "plat_types.h"
#include "string.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "aud_section.h"
#include "audioflinger.h"
#include "anc_process.h"
#include "hwtimer_list.h"
#include "app_thread.h"
#include "tgt_hardware.h"
#include "hal_bootmode.h"
#include "hal_codec.h"
#include "app_anc.h"
#include "app_anc_utils.h"
#include "app_anc_table.h"
#include "app_anc_fade.h"
#include "app_anc_sync.h"
#include "app_anc_assist.h"
#include "app_status_ind.h"
#include "hal_aud.h"
#ifdef VOICE_DETECTOR_EN
#include "app_voice_detector.h"
#endif

#if defined(PSAP_APP)
#include "psap_process.h"
// TODO: Clean up this code...
#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV                    CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif
#endif

// NOTE: Can not comment following MACRO.
// Close and open can do same effct.
#define ANC_MODE_SWITCH_WITHOUT_FADE

#define ANC_OPEN_DELAY_TIME     (MS_TO_TICKS(600))
#define ANC_CLOSE_DELAY_TIME    (MS_TO_TICKS(1000 * 6))

enum {
    ANC_STATUS_OFF = 0,
    ANC_STATUS_ON,
    ANC_STATUS_WAITING_ON,  // Wait fadein
    ANC_STATUS_WAITING_OFF  // Wait timer
};

typedef enum {
    ANC_EVENT_INIT = 0,
    ANC_EVENT_OPEN,
    ANC_EVENT_CLOSE,
    ANC_EVENT_FADE_IN,
    ANC_EVENT_FADE_OUT,
    ANC_EVENT_SWITCH_MODE,
    ANC_EVENT_SYNC_STATUS,
    ANC_EVENT_UPDATE_COEF,
    ANC_EVENT_SET_GAIN,
    ANC_EVENT_NONE
} anc_event_t;

typedef struct {
    anc_event_t     event;
    app_anc_mode_t  mode;
} anc_timer_param_t;

static HWTIMER_ID anc_timerid = NULL;
static anc_timer_param_t g_anc_timer_param = {
    .event = ANC_EVENT_NONE,
    .mode = APP_ANC_MODE_QTY,
};

static uint32_t g_anc_work_status = ANC_STATUS_OFF;
static bool g_anc_init_flag = false;
static app_anc_mode_t g_app_anc_mode = APP_ANC_MODE_OFF;
static app_anc_mode_t g_app_anc_mode_before_off = APP_ANC_MODE_OFF;
static enum AUD_SAMPRATE_T g_sample_rate_series = AUD_SAMPRATE_NULL;

static void anc_sample_rate_change(enum AUD_STREAM_T stream, enum AUD_SAMPRATE_T rate, enum AUD_SAMPRATE_T *new_play, enum AUD_SAMPRATE_T *new_cap);

bool app_anc_work_status(void)
{
    return (g_anc_work_status == ANC_STATUS_ON || g_anc_work_status == ANC_STATUS_WAITING_ON);
}

bool app_anc_work_status_is_on(void)
{
    return (g_anc_work_status == ANC_STATUS_ON);
}

app_anc_mode_t app_anc_get_curr_mode(void)
{
    return g_app_anc_mode;
}

uint32_t app_anc_get_curr_types(void)
{
    return app_anc_table_get_types(g_app_anc_mode);
}

static void app_anc_af_open(enum ANC_TYPE_T type)
{
    af_anc_open(type, g_sample_rate_series, g_sample_rate_series, anc_sample_rate_change);
}

static void app_anc_select_coef(enum ANC_TYPE_T type, uint32_t index)
{
    if (0) {
#if defined(PSAP_APP)
    } else if (type == PSAP_FEEDFORWARD) {
        psap_select_coef(g_sample_rate_series, index);
#endif
#if defined(AUDIO_ANC_SPKCALIB_HW)
    } else if (type == ANC_SPKCALIB) {
        spkcalib_select_coef(g_sample_rate_series, index);
#endif
    } else {
        anc_select_coef(g_sample_rate_series, index, type, ANC_GAIN_DELAY);
    }
}

static void app_anc_open_items(enum ANC_TYPE_T type)
{
    if (type == ANC_FEEDFORWARD) {
        app_anc_af_open(ANC_FEEDFORWARD);
        anc_open(ANC_FEEDFORWARD);
    } else if (type == ANC_FEEDBACK) {
        app_anc_af_open(ANC_FEEDBACK);
        anc_open(ANC_FEEDBACK);
#if defined(AUDIO_ANC_FB_MC_HW)
        anc_open(ANC_MUSICCANCLE);
        // NOTE: FB Gain is after MC. So disable FB will disable MC
        app_anc_enable_gain(ANC_MUSICCANCLE);
#endif
    } else if (type == ANC_TALKTHRU) {
        app_anc_af_open(ANC_TALKTHRU);
        anc_open(ANC_TALKTHRU);
#if defined(PSAP_APP)
    } else if (type == PSAP_FEEDFORWARD) {
        app_anc_af_open(ANC_TALKTHRU);  // NOTE: PSAP use TT configure
        psap_open(CODEC_OUTPUT_DEV);
#endif
#if defined(AUDIO_ANC_SPKCALIB_HW)
    } else if (type == ANC_SPKCALIB) {
        // app_anc_af_open(ANC_SPKCALIB);  
        anc_open(ANC_SPKCALIB);
#endif
    } else {
        ASSERT(0, "[%s] type(0x%x) is invalid!", __func__, type);
    }
}

static void app_anc_close_items(enum ANC_TYPE_T type)
{
    if (type == ANC_FEEDFORWARD) {
        anc_close(ANC_FEEDFORWARD);
        af_anc_close(ANC_FEEDFORWARD);
    } else if (type == ANC_FEEDBACK) {
        anc_close(ANC_FEEDBACK);
        af_anc_close(ANC_FEEDBACK);
#if defined(AUDIO_ANC_FB_MC_HW)
        anc_close(ANC_MUSICCANCLE);
#endif
    } else if (type == ANC_TALKTHRU) {
        anc_close(ANC_TALKTHRU);
        af_anc_close(ANC_TALKTHRU);
#if defined(PSAP_APP)
    } else if (type == PSAP_FEEDFORWARD) {
        psap_close();
        af_anc_close(ANC_TALKTHRU);     // NOTE: PSAP use TT configure
#endif
#if defined(AUDIO_ANC_SPKCALIB_HW)
    } else if (type == ANC_SPKCALIB) {
        // af_anc_close(ANC_SPKCALIB);  
        anc_close(ANC_SPKCALIB);
#endif
    } else {
        ASSERT(0, "[%s] type(0x%x) is invalid!", __func__, type);
    }
}

void app_anc_update_coef(void)
{
    app_anc_coef_index_cfg_t index_cfg = app_anc_table_get_cfg(g_app_anc_mode);

#if defined(ANC_FF_ENABLED)
    if (index_cfg.ff != ANC_INVALID_COEF_INDEX) {
        app_anc_select_coef(ANC_FEEDFORWARD, index_cfg.ff);
    }
#endif
#if defined(AUDIO_ANC_TT_HW)
    if (index_cfg.tt != ANC_INVALID_COEF_INDEX) {
        app_anc_select_coef(ANC_TALKTHRU, index_cfg.tt);
    }
#endif
#if defined(ANC_FB_ENABLED)
    if (index_cfg.fb != ANC_INVALID_COEF_INDEX) {
        app_anc_select_coef(ANC_FEEDBACK, index_cfg.fb);
#if defined(AUDIO_ANC_FB_MC_HW)
        app_anc_select_coef(ANC_MUSICCANCLE, index_cfg.fb);
#endif
    }
#endif
#if defined(PSAP_APP)
    if (index_cfg.psap != ANC_INVALID_COEF_INDEX) {
        app_anc_select_coef(PSAP_FEEDFORWARD, index_cfg.psap);
    }
#endif
#if defined(AUDIO_ANC_SPKCALIB_HW)
    if (index_cfg.spk_calib != ANC_INVALID_COEF_INDEX) {
        app_anc_select_coef(ANC_SPKCALIB, index_cfg.spk_calib);
    }
#endif
}

static void app_anc_switch_coef(app_anc_mode_t mode_old, app_anc_mode_t mode_new, bool opt_with_gain)
{
    TRACE(0, "[%s] mode_old: %d, mode_new: %d", __func__, mode_old, mode_new);

    app_anc_coef_index_cfg_t index_cfg_old = app_anc_table_get_cfg(mode_old);
    app_anc_coef_index_cfg_t index_cfg_new = app_anc_table_get_cfg(mode_new);

#if defined(ANC_FF_ENABLED)
    if (index_cfg_old.ff != index_cfg_new.ff) {
        if (index_cfg_old.ff == ANC_INVALID_COEF_INDEX) {
            // open
            app_anc_open_items(ANC_FEEDFORWARD);
            app_anc_select_coef(ANC_FEEDFORWARD, index_cfg_new.ff);
            if (opt_with_gain) {
                app_anc_enable_gain(ANC_FEEDFORWARD);
            }
        } else if (index_cfg_new.ff == ANC_INVALID_COEF_INDEX) {
            // close
            if (opt_with_gain) {
                app_anc_disable_gain(ANC_FEEDFORWARD);
            }
            app_anc_close_items(ANC_FEEDFORWARD);
        } else {
            // switch
            app_anc_select_coef(ANC_FEEDFORWARD, index_cfg_new.ff);
        }
    }
#endif

#if defined(AUDIO_ANC_TT_HW)
    if (index_cfg_old.tt != index_cfg_new.tt) {
        if (index_cfg_old.tt == ANC_INVALID_COEF_INDEX) {
            // open
            app_anc_open_items(ANC_TALKTHRU);
            app_anc_select_coef(ANC_TALKTHRU, index_cfg_new.tt);
            if (opt_with_gain) {
                app_anc_enable_gain(ANC_TALKTHRU);
            }
        } else if (index_cfg_new.tt == ANC_INVALID_COEF_INDEX) {
            // close
            if (opt_with_gain) {
                app_anc_disable_gain(ANC_TALKTHRU);
            }
            app_anc_close_items(ANC_TALKTHRU);
        } else {
            // switch
            app_anc_select_coef(ANC_TALKTHRU, index_cfg_new.tt);
        }
    }
#endif

#if defined(ANC_FB_ENABLED)
    if (index_cfg_old.fb != index_cfg_new.fb) {
        if (index_cfg_old.fb == ANC_INVALID_COEF_INDEX) {
            // open
            app_anc_open_items(ANC_FEEDBACK);
            app_anc_select_coef(ANC_FEEDBACK, index_cfg_new.fb);
#if defined(AUDIO_ANC_FB_MC_HW)
            app_anc_select_coef(ANC_MUSICCANCLE, index_cfg_new.fb);
#endif
            if (opt_with_gain) {
                app_anc_enable_gain(ANC_FEEDBACK);
            }
        } else if (index_cfg_new.fb == ANC_INVALID_COEF_INDEX) {
            // close
            if (opt_with_gain) {
                app_anc_disable_gain(ANC_FEEDBACK);
            }
            app_anc_close_items(ANC_FEEDBACK);
        } else {
            // switch
            app_anc_select_coef(ANC_FEEDBACK, index_cfg_new.fb);
#if defined(AUDIO_ANC_FB_MC_HW)
            app_anc_select_coef(ANC_MUSICCANCLE, index_cfg_new.fb);
#endif
        }
    }
#endif

#if defined(PSAP_APP)
    if (index_cfg_old.psap != index_cfg_new.psap) {
        if (index_cfg_old.psap == ANC_INVALID_COEF_INDEX) {
            // open
            app_anc_open_items(PSAP_FEEDFORWARD);
            app_anc_select_coef(PSAP_FEEDFORWARD, index_cfg_new.psap);
            if (opt_with_gain) {
                app_anc_enable_gain(PSAP_FEEDFORWARD);
            }
        } else if (index_cfg_new.psap == ANC_INVALID_COEF_INDEX) {
            // close
            if (opt_with_gain) {
                app_anc_disable_gain(PSAP_FEEDFORWARD);
            }
            app_anc_close_items(PSAP_FEEDFORWARD);
        } else {
            // switch
            app_anc_select_coef(PSAP_FEEDFORWARD, index_cfg_new.psap);
        }
    }
#endif

#if defined(AUDIO_ANC_SPKCALIB_HW)
    if (index_cfg_old.spk_calib != index_cfg_new.spk_calib) {
        if (index_cfg_old.spk_calib == ANC_INVALID_COEF_INDEX) {
            // open
            app_anc_open_items(ANC_SPKCALIB);
            app_anc_select_coef(ANC_SPKCALIB, index_cfg_new.spk_calib);
        } else if (index_cfg_new.spk_calib == ANC_INVALID_COEF_INDEX) {
            // close
            app_anc_close_items(ANC_SPKCALIB);
        } else {
            // switch
            app_anc_select_coef(ANC_SPKCALIB, index_cfg_new.spk_calib);
        }
    }
#endif
}

static void app_anc_post_msg(anc_event_t event, uint8_t param)
{
    TRACE(0, "[%s] event: %d, param: %d", __func__, event, param);

    if (g_anc_init_flag == false) {
        return;
    }

    APP_MESSAGE_BLOCK msg;
    msg.mod_id = APP_MODUAL_ANC;
    msg.msg_body.message_id = event;
    msg.msg_body.message_Param0 = param;
    app_mailbox_put(&msg);
}

int32_t app_anc_thread_set_gain(enum ANC_TYPE_T type, float gain_l, float gain_r)
{
    TRACE(2, "[%s] type: 0x%x", __func__, type);

    if (g_anc_init_flag == false) {
        return -1;
    }

    APP_MESSAGE_BLOCK msg;
    msg.mod_id = APP_MODUAL_ANC;
    msg.msg_body.message_id = ANC_EVENT_SET_GAIN;
    msg.msg_body.message_Param0 = type;
    memcpy(&msg.msg_body.message_Param1, &gain_l, sizeof(float));
    memcpy(&msg.msg_body.message_Param2, &gain_r, sizeof(float));
    app_mailbox_put(&msg);

    return 0;
}

void app_anc_status_post(uint8_t status)
{
    app_anc_post_msg(ANC_EVENT_SWITCH_MODE, status);
}

static void app_anc_timer_handler(void *param)
{
    anc_timer_param_t *anc_timer_param = (anc_timer_param_t *)param;

    TRACE(0, "[%s] event: %d, mode: %d", __func__, anc_timer_param->event, anc_timer_param->mode);

    switch (anc_timer_param->event) {
        case ANC_EVENT_OPEN:
            app_anc_post_msg(ANC_EVENT_FADE_IN, anc_timer_param->mode);
            break;
        case ANC_EVENT_CLOSE:
            app_anc_post_msg(ANC_EVENT_CLOSE, anc_timer_param->mode);
            break;
        default:
            break;
    }

    anc_timer_param->event = ANC_EVENT_NONE;
    anc_timer_param->mode = APP_ANC_MODE_QTY;
    hwtimer_stop(anc_timerid);
}

static void app_anc_timer_init(void)
{
    if (anc_timerid == NULL)
        anc_timerid = hwtimer_alloc((HWTIMER_CALLBACK_T)app_anc_timer_handler, &g_anc_timer_param);
}

static void app_anc_timer_set(anc_event_t event, app_anc_mode_t mode, uint32_t delay)
{
    TRACE(0, "[%s] event: %d , mode: %d, delay: %dms ", __func__, event, mode, TICKS_TO_MS(delay));
    
    if (anc_timerid == NULL) {
        return;
    }

    g_anc_timer_param.event = event;
    g_anc_timer_param.mode = mode;

    hwtimer_stop(anc_timerid);
    hwtimer_start(anc_timerid, delay);
}

static void app_anc_timer_close(void)
{
    TRACE(0, "[%s] ...", __func__);

    if (anc_timerid == NULL) {
        return;
    }
    
    g_anc_timer_param.event = ANC_EVENT_NONE;
    g_anc_timer_param.mode = APP_ANC_MODE_QTY;

    hwtimer_stop(anc_timerid);
}

/************************************************************************
**                  anc_sample_rate_change()
**
** this function is callback. it is passed as a parameter to af_anc_open()
** function. it is invoked that used to change anc filter if anc sample rate
** currently is not equal to playback stream sample rate incoming during ANC
** ON status. so it can cause POP voice during changing anc filter. so we not
** recommend to change anc filter if POP voice has already occurred. so we just
** pass NULL paramter to af_anc_open() function.
**
************************************************************************/
static void anc_sample_rate_change(enum AUD_STREAM_T stream, enum AUD_SAMPRATE_T rate, enum AUD_SAMPRATE_T *new_play, enum AUD_SAMPRATE_T *new_cap)
{
    ASSERT(new_play == NULL, "[%s] new_play != NULL", __func__);
    ASSERT(new_cap == NULL, "[%s] new_cap != NULL", __func__);

    if (g_sample_rate_series != rate) {
        g_sample_rate_series = rate;
        TRACE(0, "[%s] Update g_sample_rate_series = %d", __func__, g_sample_rate_series);
        // TODO: Send to thread to process
        app_anc_update_coef();
        // app_anc_post_msg(ANC_EVENT_UPDATE_COEF, 0);
    }
}

static void app_anc_open_impl(app_anc_mode_t mode)
{
    TRACE(0, "[%s] mode: %d", __func__, mode);
    (void)mode;

    // app_anc_coef_index_cfg_t index_cfg = app_anc_table_get_cfg(mode);

#if defined(VOICE_DETECTOR_EN)
    app_voice_detector_capture_disable_vad(VOICE_DETECTOR_ID_0, VOICE_DET_USER_ANC);
#endif

// #if defined(ANC_FF_ENABLED)
//     if (index_cfg.ff != ANC_INVALID_COEF_INDEX) {
//         app_anc_open_items(ANC_FEEDFORWARD);
//     }
// #endif

// #if defined(AUDIO_ANC_TT_HW)
//     if (index_cfg.tt != ANC_INVALID_COEF_INDEX) {
//         app_anc_open_items(ANC_TALKTHRU);
//     }
// #endif

// #if defined(ANC_FB_ENABLED)
//     if (index_cfg.fb != ANC_INVALID_COEF_INDEX) {
//         app_anc_open_items(ANC_FEEDBACK);
//     }
// #endif

#if defined(ANC_ASSIST_ENABLED)
    app_anc_assist_open(ANC_ASSIST_USER_ANC);
#endif
}

static void app_anc_close_impl(app_anc_mode_t mode)
{
    TRACE(0, "[%s] mode: %d", __func__, mode);

    app_anc_coef_index_cfg_t index_cfg = app_anc_table_get_cfg(mode);

#if defined(ANC_ASSIST_ENABLED)
    app_anc_assist_close(ANC_ASSIST_USER_ANC);
#endif

#if defined(ANC_FF_ENABLED)
    if (index_cfg.ff != ANC_INVALID_COEF_INDEX) {
        app_anc_close_items(ANC_FEEDFORWARD);
    }
#endif

#if defined(AUDIO_ANC_TT_HW)
    if (index_cfg.tt != ANC_INVALID_COEF_INDEX) {
        app_anc_close_items(ANC_TALKTHRU);
    }
#endif

#if defined(ANC_FB_ENABLED)
    if (index_cfg.fb != ANC_INVALID_COEF_INDEX) {
        app_anc_close_items(ANC_FEEDBACK);
    }
#endif

#if defined(PSAP_APP)
    if (index_cfg.psap != ANC_INVALID_COEF_INDEX) {
        app_anc_close_items(PSAP_FEEDFORWARD);
    }
#endif

#if defined(AUDIO_ANC_SPKCALIB_HW)
    if (index_cfg.spk_calib != ANC_INVALID_COEF_INDEX) {
        app_anc_close_items(ANC_SPKCALIB);
    }
#endif

#if defined(VOICE_DETECTOR_EN)
    app_voice_detector_capture_enable_vad(VOICE_DETECTOR_ID_0, VOICE_DET_USER_ANC);
#endif
}

static void app_anc_switch_mode_impl(app_anc_mode_t mode)
{
    TRACE(0, "[%s] g_app_anc_mode: %d, mode: %d, anc_status: %d", __func__, g_app_anc_mode, mode, g_anc_work_status);

    if (g_app_anc_mode != mode) {
        if (g_app_anc_mode == APP_ANC_MODE_OFF) {
            // Open ...
            if (g_anc_work_status == ANC_STATUS_WAITING_OFF) {
                app_anc_timer_close();          // Stop close flow
                app_anc_switch_coef(g_app_anc_mode_before_off, mode, false);    // Just open anc and set coef. gain is zero
                g_anc_work_status = ANC_STATUS_WAITING_ON;
                app_anc_post_msg(ANC_EVENT_FADE_IN, mode);
            } else if (g_anc_work_status == ANC_STATUS_OFF) {
                app_anc_switch_coef(g_app_anc_mode, mode, false);   // Just open anc and set coef. gain is zero
                g_anc_work_status = ANC_STATUS_WAITING_ON;
                app_anc_open_impl(mode);
                app_anc_timer_set(ANC_EVENT_OPEN, mode, ANC_OPEN_DELAY_TIME);
            } else {
                ASSERT(0, "[%s] g_anc_work_status(%d) is invalid", __func__, g_anc_work_status);
            }
        } else if (mode == APP_ANC_MODE_OFF) {
            // Close ...
            g_app_anc_mode_before_off = g_app_anc_mode;
            app_anc_timer_set(ANC_EVENT_CLOSE, g_app_anc_mode, ANC_CLOSE_DELAY_TIME);
            g_anc_work_status = ANC_STATUS_WAITING_OFF;
            app_anc_post_msg(ANC_EVENT_FADE_OUT, g_app_anc_mode);
        } else {
            // Switch ...
#if defined(ANC_MODE_SWITCH_WITHOUT_FADE)
            app_anc_switch_coef(g_app_anc_mode, mode, true);
            // TODO: Recommend to play "ANC SWITCH" prompt here...
#else
            app_anc_fade_switch_coef(app_anc_table_get_types(mode));
#endif
        }
        g_app_anc_mode = mode;
    }
}

static int app_anc_handle_process(APP_MESSAGE_BODY *msg_body)
{
    uint32_t evt = msg_body->message_id;
    uint32_t arg0 = msg_body->message_Param0;

    TRACE(4, "[%s] evt: %d, arg0: %d, anc status :%d", __func__, evt, arg0, g_anc_work_status);

    switch (evt) {
        case ANC_EVENT_INIT:
            app_anc_timer_init();
#if defined(__AUDIO_SECTION_SUPPT__)
            anc_load_cfg();
#endif
            app_anc_table_init();
            app_anc_utils_init();
            app_anc_fade_init();
            g_anc_work_status = ANC_STATUS_OFF;
            break;
        case ANC_EVENT_FADE_IN: {
            uint32_t types = app_anc_table_get_types(arg0);
#if defined(PSAP_APP)
            // TODO: PSAP get/set gain function is different anc. This is a workaround method.
            if (types & PSAP_FEEDFORWARD) {
                // FIXME: Clean up
                psap_enable(CODEC_OUTPUT_DEV);
                app_anc_enable_gain(PSAP_FEEDFORWARD);
                types &= ~PSAP_FEEDFORWARD;
            }
#endif
            app_anc_fadein(types);
            g_anc_work_status = ANC_STATUS_ON;
            // TODO: Recommend to play "ANC ON" prompt here...

            break;
        }
        case ANC_EVENT_FADE_OUT: {
            uint32_t types = app_anc_table_get_types(arg0);
#if defined(PSAP_APP)
            // TODO: PSAP get/set gain function is different anc. This is a workaround method.
            if (types & PSAP_FEEDFORWARD) {
                // FIXME: Clean up
                psap_disable(CODEC_OUTPUT_DEV);
                app_anc_disable_gain(PSAP_FEEDFORWARD);
                types &= ~PSAP_FEEDFORWARD;
            }
#endif
            app_anc_fadeout(types);
            g_anc_work_status = ANC_STATUS_WAITING_OFF;
            // TODO: Recommend to play "ANC OFF" prompt here...

            break;
        }
        case ANC_EVENT_SWITCH_MODE:
            app_anc_switch_mode_impl(arg0);
            break;
        case ANC_EVENT_SET_GAIN: {
            enum ANC_TYPE_T type = (enum ANC_TYPE_T)msg_body->message_Param0;
            float gain_l = 1.0;
            float gain_r = 1.0;
            memcpy(&gain_l, &msg_body->message_Param1, sizeof(float));
            memcpy(&gain_r, &msg_body->message_Param2, sizeof(float));
            TRACE(4, "[%s] type: 0x%x, gain(x100): %d, %d", __func__, type, (uint32_t)(gain_l * 100), (uint32_t)(gain_r * 100));
            app_anc_set_gain_f32(ANC_GAIN_USER_APP_ANC, type, gain_l, gain_r);
        }
            break;
        case ANC_EVENT_UPDATE_COEF:
            app_anc_update_coef();
            break;
        case ANC_EVENT_SYNC_STATUS: {
            uint32_t mode = arg0;
            TRACE(3, "[%s] status: %d, mode: %d ", __func__, g_anc_work_status, mode);
            app_anc_sync_mode(mode);
            break;
        }
        case ANC_EVENT_CLOSE:
            app_anc_close_impl(arg0);
            g_anc_work_status = ANC_STATUS_OFF;
            break;
        default:break;
    }
    
    return 0;
}

int32_t app_anc_switch(app_anc_mode_t mode)
{
    TRACE(0, "[%s] Mode: %d --> %d", __func__, g_app_anc_mode, mode);

    if (g_app_anc_mode != mode) {
#if defined(IBRT)
        app_anc_post_msg(ANC_EVENT_SYNC_STATUS, mode);
#endif
        app_anc_status_post(mode);
    }

    return 0;
}

int32_t app_anc_loop_switch(void)
{
    app_anc_mode_t mode = g_app_anc_mode + 1;

    if (mode >= APP_ANC_MODE_QTY) {
        mode = APP_ANC_MODE_OFF;
    }

    app_anc_switch(mode);

    return 0;
}

int32_t app_anc_init(void)
{
    TRACE(0, "[%s] ...", __func__);

    g_anc_init_flag = true;
    g_app_anc_mode = APP_ANC_MODE_OFF;
    g_app_anc_mode_before_off = APP_ANC_MODE_OFF;

    if (g_sample_rate_series == AUD_SAMPRATE_NULL) {
        g_sample_rate_series = hal_codec_anc_convert_rate(AUD_SAMPRATE_48000);
    }
    anc_howling_check_enable(true);
    anc_howling_set(ANC_HOWLING_WINDOW_32,ANC_HOWLING_THRESHOLD_0dB);

    app_set_threadhandle(APP_MODUAL_ANC, app_anc_handle_process);
    app_anc_post_msg(ANC_EVENT_INIT, 0);

    return 0;
}

int32_t app_anc_deinit(void)
{
    TRACE(0, "[%s] ...", __func__);

    app_anc_fade_deinit();
    app_anc_table_deinit();

    g_anc_timer_param.event = ANC_EVENT_NONE;
    g_anc_timer_param.mode = APP_ANC_MODE_QTY;

    if (anc_timerid) {
        hwtimer_stop(anc_timerid);
        hwtimer_free(anc_timerid);
        anc_timerid = NULL;
    }
    if (g_anc_work_status != ANC_STATUS_OFF) {
        g_anc_work_status = ANC_STATUS_OFF;
        app_anc_close_impl(g_app_anc_mode);
    }
    app_set_threadhandle(APP_MODUAL_ANC, NULL);

    g_anc_init_flag = false;

    return 0;
}

#if defined(IBRT)
int32_t app_anc_sync_status(void)
{
    app_anc_post_msg(ANC_EVENT_SYNC_STATUS, g_app_anc_mode);

    return 0;
}
#endif
