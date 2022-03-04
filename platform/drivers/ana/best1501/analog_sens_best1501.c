/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "analog.h"
#include "hal_timer.h"
#include "hal_trace.h"

static ANALOG_RPC_REQ_CALLBACK rpc_cb;

static enum ANA_AUD_PLL_USER_T ana_aud_pll_map;

void analog_aud_register_rpc_callback(ANALOG_RPC_REQ_CALLBACK cb)
{
    rpc_cb = cb;
}

void analog_aud_freq_pll_config(uint32_t freq, uint32_t div)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_FREQ_PLL_CFG, freq, div, 0);
    }
}

void analog_aud_osc_clk_enable(bool enable)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_OSC_CLK_ENABLE, enable, 0, 0);
    }
}

void analog_aud_pll_open(enum ANA_AUD_PLL_USER_T user)
{
    if (user >= ANA_AUD_PLL_USER_END) {
        return;
    }

#ifdef __AUDIO_RESAMPLE__
    if (user == ANA_AUD_PLL_USER_CODEC &&
            hal_cmu_get_audio_resample_status()) {

        analog_aud_osc_clk_enable(true);
        return;
    }
#endif

    if (ana_aud_pll_map == 0) {
        hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_AUD);
    }
    ana_aud_pll_map |= user;
}

void analog_aud_pll_close(enum ANA_AUD_PLL_USER_T user)
{
    if (user >= ANA_AUD_PLL_USER_END) {
        return;
    }

#ifdef __AUDIO_RESAMPLE__
    if (user == ANA_AUD_PLL_USER_CODEC &&
            hal_cmu_get_audio_resample_status()) {

        analog_aud_osc_clk_enable(false);
        return;
    }
#endif

    ana_aud_pll_map &= ~user;
    if (ana_aud_pll_map == 0) {
        hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_AUD);
    }
}

void analog_aud_codec_adc_enable(enum AUD_IO_PATH_T input_path, enum AUD_CHANNEL_MAP_T ch_map, bool en)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_CODEC_ADC_ENABLE, input_path, ch_map, en);
    }
}

void analog_aud_codec_open(void)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_CODEC_OPEN, true, 0, 0);
    }
}

void analog_aud_codec_close(void)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_CODEC_OPEN, false, 0, 0);
    }
}

void analog_aud_vad_enable(enum AUD_VAD_TYPE_T type, bool en)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_VAD_ENABLE, type, en, 0);
    }
}

void analog_aud_vad_adc_enable(bool en)
{
    if (rpc_cb) {
        rpc_cb(ANALOG_RPC_VAD_ADC_ENABLE, en, 0, 0);
    }
}

