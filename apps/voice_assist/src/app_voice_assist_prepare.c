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
#include "app_voice_assist_prepare.h"
#include "anc_select_mode.h"
#include "anc_assist.h"
#include "arm_math.h"
#include "audio_dump.h"
#include "app_utils.h"
#include "heap_api.h"


static int custom_error_mapping(float diff, int mode){
    return 1;
}
static int local_close_cnt = 0;
static AncSelectModeState * anc_select_mode_st = NULL;
static SELECT_MODE_CFG_T anc_select_mode_cfg = {
        .sample_rate = 16000, // sample rate, same as anc_assist_module
        .frame_size = 120, // frame size, same as anc_assist_modult

        .process_debug = 1,  // show debug log for process
        .snr_debug = 0,   // not show debug lof for snr
        .snr_on = 0,  // set snr function not affect procedure

        .mode_scan_method = 1,
        .gain_method = 2,

        .wait_period = 50, // wait period for change anc, here is about 30*7.5 ms
        .test_period = 90, // time for whole test, total time is 80*7.5ms

        .mode_start_point = 0,  // the start gain place, use pnc_mode_list[0] as start point
        .mode_shift_upper_db = 20, // the upper bound for gain adapt, pnc_mode_list[49]
        .mode_shift_lower_db = 0, // the lower bound for gain adapt, pnc_mode_list[0]
        .mode_shift_db = 1,  // not use this time

        .mode_fb_power_thd = 0, // the fb energy lower bound for mode adapt
        .mode_ff_power_thd = 0, // the fb energy upper bound for mode adapt

        .mode_freq_upper = 800, 
        .mode_freq_lower = 200,
};

static int32_t _voice_assist_prepare_callback(void *buf, uint32_t len, void *other);
static enum PREPARE_STATE oneshot_adaptive_anc_state = PREPARE_STATE_RUNNING;
int32_t app_voice_assist_prepare_init(void)
{
    oneshot_adaptive_anc_state = PREPARE_STATE_RUNNING;
    app_anc_assist_register(ANC_ASSIST_USER_PREPARE, _voice_assist_prepare_callback);
    

    return 0;
}

int32_t app_voice_assist_prepare_open()
{
    TRACE(0, "[%s] oneshot adapt anc start stream", __func__);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_208M);

    oneshot_adaptive_anc_state = PREPARE_STATE_RUNNING;
    app_anc_assist_open(ANC_ASSIST_USER_PREPARE);
    local_close_cnt = 0;
    
    //create st
    AncSelectModeState * tmp_st = anc_select_mode_create(&anc_select_mode_cfg,custom_error_mapping);
    if(tmp_st != NULL){
        anc_select_mode_st = tmp_st;
    }

    anc_select_mode_set_auto_status(anc_select_mode_st,1);

    // close fb anc for adaptive anc, it is better not to open it during the init state
#ifdef ONESHOT_ADAPT_ANC_DUMP
    audio_dump_init(120,sizeof(short),2);
#endif
    return 0;
}

int32_t app_voice_assist_prepare_close(void)
{
    oneshot_adaptive_anc_state = PREPARE_STATE_STOP;
    TRACE(0, "[%s] oneshot adapt anc close stream", __func__);

#ifdef ONESHOT_ADAPT_ANC_DUMP
    audio_dump_deinit();
#endif
    return 0;
}


static int32_t _voice_assist_prepare_callback(void * buf, uint32_t len, void *other)
{
    if(oneshot_adaptive_anc_state == PREPARE_STATE_STOP){
        TRACE(2,"[%s] stop in callback func with state %d",__func__,oneshot_adaptive_anc_state);
        app_anc_assist_close(ANC_ASSIST_USER_PREPARE);
        anc_select_mode_destroy(anc_select_mode_st);
        // app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
        oneshot_adaptive_anc_state = PREPARE_STATE_POST_STOP;
        return 0;
    } else if(oneshot_adaptive_anc_state == PREPARE_STATE_POST_STOP){
        TRACE(2,"[%s] stop in callback func with state %d",__func__,oneshot_adaptive_anc_state);
        return 0;
    }
    // deal with input values
    float ** input_data = buf;
    float * ff_data = input_data[0];
    float * fb_data = input_data[1];

    for(int i = 0; i< len; i++){
        ff_data[i] = ff_data[i]/ 256;
        fb_data[i] = fb_data[i]/ 256;
    }

    SELECT_MODE_RESULT_T res = anc_select_process(anc_select_mode_st,ff_data,fb_data,len,0);

    if(res.ff_pwr >= 0){ 
        TRACE(2,"[adapt_anc_prepare] ff pwr %d ",(int)res.ff_pwr);
    }

    // close stream
    if(anc_select_mode_get_auto_status(anc_select_mode_st) == 0 && anc_select_gain_get_auto_status(anc_select_mode_st) == 0 ){
        // TRACE(0,"!!!!!!!!!!!!!!!!!!!!!!!");
        local_close_cnt ++;   
    }
    if(local_close_cnt >= 5){
        app_voice_assist_prepare_close();
    }

    return 0;
}
