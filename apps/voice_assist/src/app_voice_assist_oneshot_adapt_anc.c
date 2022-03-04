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

#include "hal_trace.h"
#include "app_anc_assist.h"
#include "app_voice_assist_oneshot_adapt_anc.h"
#include "anc_select_mode.h"
#include "anc_assist.h"

#include "aud_section.h"
#include "anc_process.h"
#include "arm_math.h"
#include "audio_dump.h"
#include "app_utils.h"
#include "heap_api.h"






// #define ONESHOT_ADAPT_ANC_DUMP

#ifdef ONESHOT_ADAPT_ANC_DUMP
static short tmp_data[120];
#endif
static int local_close_cnt = 0;
static int g_best_mode_index;

// local anc settings for adaptive anc
static struct_anc_cfg POSSIBLY_UNUSED AncFirCoef_50p7k_mode0;

struct_anc_cfg * POSSIBLY_UNUSED anc_mode_list[6] = {
    &AncFirCoef_50p7k_mode0,
    &AncFirCoef_50p7k_mode0,
    &AncFirCoef_50p7k_mode0,
    &AncFirCoef_50p7k_mode0,
    &AncFirCoef_50p7k_mode0,
    &AncFirCoef_50p7k_mode0,
};

// gain search list for algo
// mode search list for mode gain



static int custom_error_mapping(float diff, int mode){
    return 1;
}

AncSelectModeState * anc_select_mode_st = NULL;
SELECT_MODE_CFG_T anc_select_mode_cfg = {
        .sample_rate = 16000, // sample rate, same as anc_assist_module
        .frame_size = 120, // frame size, same as anc_assist_modult

        .process_debug = 1,  // show debug log for process
        .snr_debug = 0,   // not show debug lof for snr
        .snr_on = 0,  // set snr function not affect procedure

        .mode_scan_method = 1,
        .gain_method = 2,

        .wait_period = 50, // wait period for change anc, here is about 30*7.5 ms
        .test_period = 90, // time for whole test, total time is 80*7.5ms

        .gain_start_point = 8,  // the start gain index, use pnc_gain_list[4] as start point
        .gain_shift_upper_db = 16, // the upper bound for gain adapt, pnc_gain_list[45]
        .gain_shift_lower_db = 0, // the lower bound for gain adapt, pnc_gain_list[0]
        .gain_shift_db = 3,  // not use this time
        .gain_fb_power_thd = 0, // the fb energy lower bound for gain adapt, the lower fb energy result to less accurate result
        .gain_ff_power_thd = 0, // the fb energy upper bound for gain adapt, the lower fb energy result to less accurate result
        .previous_denoise_value = -25, // not use this time


        .mode_start_point = 0,  // the start gain place, use pnc_mode_list[0] as start point
        .mode_shift_upper_db = 3, // the upper bound for gain adapt, pnc_mode_list[49]
        .mode_shift_lower_db = 0, // the lower bound for gain adapt, pnc_mode_list[0]
        .mode_shift_db = 1,  // not use this time

        .mode_fb_power_thd = 0, // the fb energy lower bound for mode adapt
        .mode_ff_power_thd = 0, // the fb energy upper bound for mode adapt

        .mode_freq_upper = 800, 
        .mode_freq_lower = 200,

        .gain_freq_upper = 600,
        .gain_freq_lower = 100,

        .min_db_diff = -5, // the min diff for adaption, avoid the mistaken from in-ear detection
        
        .max_db_diff = 4, // the max diff for adaption, avoid the mistaken from in-ear detection
};

static int32_t _voice_assist_oneshot_adapt_anc_callback(void *buf, uint32_t len, void *other);
static enum ONESHOT_ADAPTIVE_ANC_STATE oneshot_adaptive_anc_state = ONESHOT_ADAPTIVE_ANC_STATE_RUNNING;
int32_t app_voice_assist_oneshot_adapt_anc_init(void)
{
    oneshot_adaptive_anc_state = ONESHOT_ADAPTIVE_ANC_STATE_RUNNING;
    app_anc_assist_register(ANC_ASSIST_USER_ONESHOT_ADAPT_ANC, _voice_assist_oneshot_adapt_anc_callback);
    
    //better to set when anc init, not here
    anc_set_switching_delay(ANC_SWITCHING_DELAY_50ms,ANC_FEEDFORWARD);
    anc_set_switching_delay(ANC_SWITCHING_DELAY_50ms,ANC_FEEDBACK);
    return 0;
}

int32_t app_voice_assist_oneshot_adapt_anc_open(bool gain_on, bool mode_on)
{
    TRACE(0, "[%s] oneshot adapt anc start stream", __func__);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_208M);

    oneshot_adaptive_anc_state = ONESHOT_ADAPTIVE_ANC_STATE_RUNNING;
    app_anc_assist_open(ANC_ASSIST_USER_ONESHOT_ADAPT_ANC);
    local_close_cnt = 0;
    
    //create st
#ifdef ONESHOT_ADAPTIVE_ANC_USE_DYNAMIC_RAM
    syspool_init_specific_size(ANC_SELECT_MODE_HEAP_SIZE);
    syspool_get_buff(&anc_select_mode_heap, ANC_SELECT_MODE_HEAP_SIZE);
#endif
    AncSelectModeState * tmp_st = anc_select_mode_create(&anc_select_mode_cfg,custom_error_mapping);
    if(tmp_st != NULL){
        anc_select_mode_st = tmp_st;
    }

    // set working status and init state
    anc_select_mode_set_auto_status(anc_select_mode_st,mode_on);
    // anc_select_gain_set_auto_status(anc_select_mode_st,gain_on);

    // close fb anc for adaptive anc, it is better not to open it during the init state
    anc_set_gain(0,0,ANC_FEEDBACK);
#ifdef ONESHOT_ADAPT_ANC_DUMP
    audio_dump_init(120,sizeof(short),2);
#endif
    return 0;
}

int32_t app_voice_assist_oneshot_adapt_anc_close(void)
{
    oneshot_adaptive_anc_state = ONESHOT_ADAPTIVE_ANC_STATE_STOP;
    TRACE(0, "[%s] oneshot adapt anc close stream", __func__);

#ifdef ONESHOT_ADAPT_ANC_DUMP
    audio_dump_deinit();
#endif
    return 0;
}


static int32_t _voice_assist_oneshot_adapt_anc_callback(void * buf, uint32_t len, void *other)
{
    if(oneshot_adaptive_anc_state == ONESHOT_ADAPTIVE_ANC_STATE_STOP){
        TRACE(2,"[%s] stop in callback func with state %d",__func__,oneshot_adaptive_anc_state);
        app_anc_assist_close(ANC_ASSIST_USER_ONESHOT_ADAPT_ANC);
        anc_select_mode_destroy(anc_select_mode_st);
        // app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
        oneshot_adaptive_anc_state = ONESHOT_ADAPTIVE_ANC_STATE_POST_STOP;
        return 0;
    } else if(oneshot_adaptive_anc_state == ONESHOT_ADAPTIVE_ANC_STATE_POST_STOP){
        TRACE(2,"[%s] stop in callback func with state %d",__func__,oneshot_adaptive_anc_state);
        return 0;
    }
    // deal with input values
    float ** input_data = buf;
    float * ff_data = input_data[0];
    float * fb_data = input_data[1];
    AncAssistRes * assist_res = (AncAssistRes *)other;
    int wind_status = 0; // will not pause algo
    if(assist_res->wind_status > 1){ // check wind status enum, this algo not work when bigger than small wind
        wind_status = 0; // will pause algo
    }

#ifdef ONESHOT_ADAPT_ANC_DUMP
    audio_dump_clear_up();
    for(int i = 0; i< len; i++){
        tmp_data[i] = (short)(ff_data[i]/256);
    }
    audio_dump_add_channel_data(0,tmp_data,len);
    for(int i = 0; i< len; i++){
        tmp_data[i] = (short)(fb_data[i]/256);
    }
    audio_dump_add_channel_data(1,tmp_data,len); 
    audio_dump_run();
#endif    

    for(int i = 0; i< len; i++){
        ff_data[i] = ff_data[i]/ 256;
        fb_data[i] = fb_data[i]/ 256;
    }

    SELECT_MODE_RESULT_T res = anc_select_process(anc_select_mode_st,ff_data,fb_data,len,wind_status);

    // TRACE(2,"!!!!!!!!!!!!!!!!!!!!! %d %d %d %d",res.snr_stop_flag,res.fb_pwr_stop_flag,res.ff_pwr_stop_flag,res.min_diff_stop_flag == 1);
    if(res.snr_stop_flag == 1 || res.fb_pwr_stop_flag == 1 || res.ff_pwr_stop_flag == 1 || res.min_diff_stop_flag == 1){
        // TRACE(0,"state %d %d %d %d",res.snr_stop_flag,res.fb_pwr_stop_flag,res.ff_pwr_stop_flag,res.min_diff_stop_flag);
        app_voice_assist_oneshot_adapt_anc_close();
        return 0;
    } 

    if(res.current_mode >= 0){ 
        TRACE(2,"[adapt_anc_action] change mode %d ",res.current_mode);
        anc_set_cfg(anc_mode_list[res.current_mode], ANC_FEEDFORWARD, ANC_GAIN_NO_DELAY);
    }

    if(res.best_mode >= 0 && res.normal_stop_flag == 1){
        TRACE(2,"[adapt_anc_action] change mode %d",res.best_mode);
        anc_set_cfg(anc_mode_list[res.best_mode], ANC_FEEDFORWARD, ANC_GAIN_NO_DELAY);
        g_best_mode_index = res.best_mode;
    }


    if(res.current_gain >= 0){ 
        TRACE(2,"[adapt_anc_action] change gain %d ",res.current_gain);
        anc_set_gain(res.current_gain,res.current_gain, ANC_FEEDFORWARD);
    }

    // close stream
    if(anc_select_mode_get_auto_status(anc_select_mode_st) == 0 && anc_select_gain_get_auto_status(anc_select_mode_st) == 0 ){
        // TRACE(0,"!!!!!!!!!!!!!!!!!!!!!!!");
        local_close_cnt ++;   
    }
    if(local_close_cnt >= 5){
        app_voice_assist_oneshot_adapt_anc_close();
    }

    return 0;
}
