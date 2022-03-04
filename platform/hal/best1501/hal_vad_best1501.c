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
#ifdef CHIP_SUBSYS_SENS

#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_vad)
#include "hal_codec.h"
#include "hal_cmu.h"
#include "hal_psc.h"
#include "hal_aud.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "analog.h"
#include "cmsis.h"
#include "string.h"
#include "tgt_hardware.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_chipid.h"

#define CODEC_PLL_CLOCK                     (CODEC_FREQ_48K_SERIES * CODEC_CMU_DIV)

// Recudce 0.57dB in 100Hz@16kHz SampleRate
#ifdef VAD_USE_SAR_ADC
#define CODEC_ADC_DC_FILTER_FACTOR          (6)
#endif

#define RS_CLOCK_FACTOR                     200

// Trigger DMA request when RX-FIFO count >= threshold
#define HAL_CODEC_RX_FIFO_TRIGGER_LEVEL     (4)

#define MAX_DIG_DBVAL                       (50)
#define ZERODB_DIG_DBVAL                    (0)
#define MIN_DIG_DBVAL                       (-99)

#ifndef CODEC_DIGMIC_PHASE
#define CODEC_DIGMIC_PHASE                  3
#endif

#define MAX_DIG_MIC_CH_NUM                  1

#define NORMAL_ADC_CH_NUM                   2

#define MAX_ADC_CH_NUM                      (NORMAL_ADC_CH_NUM)

#define NORMAL_MIC_MAP                      (AUD_CHANNEL_MAP_CH4 | AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_DIGMIC_CH0)

#define NORMAL_ADC_MAP                      (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)

#define VALID_MIC_MAP                       (NORMAL_MIC_MAP)
#define VALID_ADC_MAP                       (NORMAL_ADC_MAP)

#define RSTN_ADC_FREE_RUNNING_CLK           CODEC_SOFT_RSTN_ADC(1 << (MAX_ADC_CH_NUM + 1))

enum CODEC_IRQ_TYPE_T {
    CODEC_IRQ_TYPE_VAD,

    CODEC_IRQ_TYPE_QTY,
};

struct CODEC_ADC_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t codec_freq;
    uint8_t codec_div;
    uint8_t cmu_div;
    uint8_t adc_down;
    uint8_t bypass_cnt;
};

static const struct CODEC_ADC_SAMPLE_RATE_T codec_adc_sample_rate[] = {
    {AUD_SAMPRATE_8000,   CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 6, 0},
    {AUD_SAMPRATE_16000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 3, 0},
    {AUD_SAMPRATE_48000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 0},
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 1},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 2},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, CODEC_CMU_DIV, 1, 3},
};

const CODEC_ADC_VOL_T WEAK codec_adc_vol[TGT_ADC_VOL_LEVEL_QTY] = {
    -99, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
};

static struct VAD_T * const vad = (struct VAD_T *)SENS_VAD_BASE;

static bool codec_init = false;
static bool codec_opened = false;

static bool codec_adc_mute;

static int8_t digadc_gain[NORMAL_ADC_CH_NUM];

static uint8_t codec_irq_map;
STATIC_ASSERT(sizeof(codec_irq_map) * 8 >= CODEC_IRQ_TYPE_QTY, "codec_irq_map size too small");
static HAL_CODEC_IRQ_CALLBACK codec_irq_callback[CODEC_IRQ_TYPE_QTY];

static enum AUD_CHANNEL_MAP_T codec_adc_ch_map;
static enum AUD_CHANNEL_MAP_T codec_mic_ch_map;

static uint8_t codec_digmic_phase = CODEC_DIGMIC_PHASE;

enum CODEC_ADC_USER_MAP_T {
    CODEC_ADC_USER_STREAM,
    CODEC_ADC_USER_VAD,

    CODEC_ADC_USER_QTY,
};

static enum CODEC_ADC_USER_MAP_T codec_adc_user_map[MAX_ADC_CH_NUM];

#ifdef VOICE_DETECTOR_EN
#ifdef CODEC_VAD_CFG_BUF_SIZE
#define CODEC_VAD_BUF_SIZE    CODEC_VAD_CFG_BUF_SIZE // 0x18000
#else
#define CODEC_VAD_BUF_SIZE    0
#endif
#define CODEC_VAD_BUF_ADDR    (SENS_VAD_BASE + 0xB0000 + CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE)
static enum AUD_CHANNEL_MAP_T vad_mic_ch;
static enum AUD_VAD_TYPE_T vad_type;
static bool vad_enabled;
static bool ana_vad_trig;
static uint8_t stream_started;
static uint8_t vad_adc_down;
static uint8_t codec_adc_down;
static enum AUD_BITS_T vad_adc_bits;
static enum AUD_BITS_T codec_adc_bits;
static AUD_VAD_CALLBACK vad_handler;
static uint32_t vad_data_cnt;
static uint32_t vad_addr_cnt;
#endif

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain);
static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val);
static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map, uint32_t cnt);
static uint32_t hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map);

static void hal_codec_reg_update_delay(void)
{
    hal_sys_timer_delay_us(2);
}

int hal_codec_open(enum HAL_CODEC_ID_T id)
{
    int i;
    bool first_open;

    first_open = !codec_init;

    analog_aud_pll_open(ANA_AUD_PLL_USER_CODEC);

    if (!codec_init) {
        for (i = 0; i < CFG_HW_AUD_INPUT_PATH_NUM; i++) {
            if (cfg_audio_input_path_cfg[i].cfg & AUD_CHANNEL_MAP_ALL & ~VALID_MIC_MAP) {
                ASSERT(false, "Invalid input path cfg: i=%d io_path=%d cfg=0x%X",
                    i, cfg_audio_input_path_cfg[i].io_path, cfg_audio_input_path_cfg[i].cfg);
            }
        }
#ifdef VOICE_DETECTOR_EN
        vad_mic_ch = hal_codec_get_input_path_cfg(AUD_INPUT_PATH_VADMIC) & AUD_CHANNEL_MAP_NORMAL_ALL;
        ASSERT((vad_mic_ch & NORMAL_MIC_MAP) && ((vad_mic_ch & (vad_mic_ch - 1)) == 0),
            "Bad vad mic ch: 0x%X", vad_mic_ch);
#endif
        codec_init = true;
    }
    hal_cmu_codec_clock_enable();
    hal_cmu_codec_reset_clear();

    codec_opened = true;

    vad->REG_000 = 0;
    vad->REG_004 = ~0UL;
    hal_codec_reg_update_delay();
    vad->REG_004 = 0;
    vad->REG_000 |= VAD_CODEC_IF_EN;

    if (first_open) {
        // Enable ADC zero-crossing gain adjustment
        vad->REG_084 |= VAD_CODEC_ADC_GAIN_SEL_CH0;
        vad->REG_0CC |= VAD_CODEC_ADC_GAIN_UPDATE_CH0;

#if defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL)
        const CODEC_ADC_VOL_T *adc_gain_db;

        adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
        if (adc_gain_db) {
            hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
        }
#endif
    }

    return 0;
}

int hal_codec_close(enum HAL_CODEC_ID_T id)
{
    vad->REG_000 = 0;

    codec_opened = false;

    // NEVER reset or power down CODEC registers, for the CODEC driver expects that last configurations
    // still exist in the next stream setup
    hal_cmu_codec_clock_disable();

    analog_aud_pll_close(ANA_AUD_PLL_USER_CODEC);

    return 0;
}

void hal_codec_crash_mute(void)
{
}

int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_CAPTURE) {
        if (codec_adc_user_map[0] || codec_adc_user_map[1]) {
            // Reset ADC ANA
            hal_cmu_reset_pulse(HAL_CMU_MOD_O_ADC_ANA);
            hal_cmu_reset_pulse(HAL_CMU_MOD_O_SADC_ANA);
            vad->REG_080 |= VAD_CODEC_ADC_EN;
        }
    }

    return 0;
}

int hal_codec_stop_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_CAPTURE) {
        if ((!codec_adc_user_map[0]) && (!codec_adc_user_map[1])) {
            vad->REG_080 &= ~VAD_CODEC_ADC_EN;
        }
    }

    return 0;
}

static void hal_codec_enable_dig_mic(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint32_t phase = 0;
    uint32_t line_map = 0;

    phase = vad->REG_090;
    for (int i = 0; i < MAX_DIG_MIC_CH_NUM; i++) {
        if (mic_map & (AUD_CHANNEL_MAP_DIGMIC_CH0 << i)) {
            line_map |= (1 << (i / 2));
        }
        phase = (phase & ~(VAD_CODEC_PDM_CAP_PHASE_CH0_MASK << (i * 2))) |
            (VAD_CODEC_PDM_CAP_PHASE_CH0(codec_digmic_phase) << (i * 2));
    }
    vad->REG_090 = phase | VAD_CODEC_PDM_ENABLE;
    hal_iomux_set_dig_mic(line_map);
}

static void hal_codec_disable_dig_mic(void)
{
    vad->REG_090 &= ~VAD_CODEC_PDM_ENABLE;
}

#ifdef VOICE_DETECTOR_EN
static int hal_codec_vad_switch_off(bool adc_state);
#endif

int hal_codec_start_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, int dma)
{
    uint32_t fifo_flush = 0;

    if (stream == AUD_STREAM_CAPTURE) {
#ifdef VOICE_DETECTOR_EN
        if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG) {
            ASSERT(vad_adc_bits == codec_adc_bits, "%s: Cap bits conflict: vad=%d codec=%d", __func__, vad_adc_bits, codec_adc_bits);
            ASSERT(vad_adc_down == codec_adc_down, "%s: Adc down conflict: vad=%d codec=%d", __func__, vad_adc_down, codec_adc_down);
            // Stop vad buffering
            hal_codec_vad_switch_off(true);
        }
#endif
        if (codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
            hal_codec_enable_dig_mic(codec_mic_ch_map);
        }
        for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                if ((vad->REG_080 & (VAD_CODEC_ADC_EN_CH0 << i)) == 0) {
                    if (codec_adc_user_map[i] == 0) {
                        // Reset ADC channel
                        hal_cmu_reset_pulse(HAL_CMU_MOD_O_ADC_CH0 + i);
                        vad->REG_080 |= (VAD_CODEC_ADC_EN_CH0 << i);
                    }
                    codec_adc_user_map[i] |= (1<<CODEC_ADC_USER_STREAM);
                }
                vad->REG_000 |= (VAD_ADC_ENABLE_CH0 << i);
            }
        }
        fifo_flush = VAD_RX_FIFO_FLUSH_CH0 | VAD_RX_FIFO_FLUSH_CH1;
        vad->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        vad->REG_004 &= ~fifo_flush;
        if (dma) {
            vad->REG_004 = SET_BITFIELD(vad->REG_004, VAD_CODEC_RX_THRESHOLD, HAL_CODEC_RX_FIFO_TRIGGER_LEVEL);
            vad->REG_000 |= VAD_DMACTRL_RX;
        }
        vad->REG_000 |= VAD_ADC_ENABLE;
    }

    return 0;
}

int hal_codec_stop_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t fifo_flush = 0;

    if (stream == AUD_STREAM_CAPTURE) {
        vad->REG_000 &= ~(VAD_ADC_ENABLE | VAD_ADC_ENABLE_CH0 | VAD_ADC_ENABLE_CH1);
        vad->REG_000 &= ~VAD_DMACTRL_RX;
        for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (i < NORMAL_ADC_CH_NUM &&
                    (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) == 0) {

                codec_adc_user_map[i] &= (~(1<<CODEC_ADC_USER_STREAM));
                if (codec_adc_user_map[i] == 0) {
                    vad->REG_080 &= ~(VAD_CODEC_ADC_EN_CH0 << i);
                }
            }
        }
        if (codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
            hal_codec_disable_dig_mic();
        }
        fifo_flush = VAD_RX_FIFO_FLUSH_CH0 | VAD_RX_FIFO_FLUSH_CH1;
        vad->REG_004 |= fifo_flush;
        hal_codec_reg_update_delay();
        vad->REG_004 &= ~fifo_flush;
    }

    return 0;
}

static float db_to_amplitude_ratio(int32_t db)
{
    float coef;

    if (db == ZERODB_DIG_DBVAL) {
        coef = 1;
    } else if (db <= MIN_DIG_DBVAL) {
        coef = 0;
    } else {
        if (db > MAX_DIG_DBVAL) {
            db = MAX_DIG_DBVAL;
        }
        coef = db_to_float(db);
    }

    return coef;
}

#if defined(CODEC_ADC_DC_FILTER_FACTOR)
void hal_codec_enable_adc_dc_filter(enum AUD_CHANNEL_MAP_T map, uint32_t enable)
{
    if (map & AUD_CHANNEL_MAP_CH0) {
        if (enable) {
            vad->REG_08C = SET_BITFIELD(vad->REG_08C, VAD_CODEC_ADC_UDC_CH0, CODEC_ADC_DC_FILTER_FACTOR) & ~VAD_CODEC_ADC_DCF_BYPASS_CH0;
        } else {
            vad->REG_08C |= VAD_CODEC_ADC_DCF_BYPASS_CH0;
        }
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        if (enable) {
            vad->REG_0A0 = SET_BITFIELD(vad->REG_0A0, VAD_CODEC_SARADC_COEF_UDC, 0x0F000) & ~VAD_CODEC_SARADC_DCF_BYPASS;
        } else {
            vad->REG_0A0 |= VAD_CODEC_SARADC_DCF_BYPASS;
        }
    }
}
#endif

static void hal_codec_set_adc_gain_value(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    if (map & AUD_CHANNEL_MAP_CH0) {
        vad->REG_084 = SET_BITFIELD(vad->REG_084, VAD_CODEC_ADC_GAIN_CH0, val);
        vad->REG_084 &= ~VAD_CODEC_ADC_GAIN_UPDATE_CH0;
        hal_codec_reg_update_delay();
        vad->REG_084 |= VAD_CODEC_ADC_GAIN_UPDATE_CH0;
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        vad->REG_0CC = SET_BITFIELD(vad->REG_0CC, VAD_CODEC_SARADC_GAIN, val);
        vad->REG_0CC &= ~VAD_CODEC_SARADC_GAIN_UPDATE;
        hal_codec_reg_update_delay();
        vad->REG_0CC |= VAD_CODEC_SARADC_GAIN_UPDATE;
    }
}

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map, int32_t gain)
{
    uint32_t val;
    float coef;
    bool mute;
    int i;
    int32_t s_val;

    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        if (map & (1 << i)) {
            digadc_gain[i] = gain;
        }
    }

    mute = codec_adc_mute;

    if (mute) {
        val = 0;
    } else {
        if (map) {
            coef = db_to_amplitude_ratio(gain);
            // Gain format: 8.12
            s_val = (int32_t)(coef * (1 << 12));
            val = __SSAT(s_val, 20);
        } else {
            val = 0;
        }
    }

    hal_codec_set_adc_gain_value(map, val);
}

static void hal_codec_restore_dig_adc_gain(void)
{
    int i;

    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
        hal_codec_set_dig_adc_gain((1 << i), digadc_gain[i]);
    }
}

static void POSSIBLY_UNUSED hal_codec_get_adc_gain(enum AUD_CHANNEL_MAP_T map, float *gain)
{
    struct ADC_GAIN_T {
        int32_t v : 20;
    };

    struct ADC_GAIN_T adc_val;

    if (map & AUD_CHANNEL_MAP_CH0) {
        adc_val.v = GET_BITFIELD(vad->REG_084, VAD_CODEC_ADC_GAIN_CH0);
    } else if (map & AUD_CHANNEL_MAP_CH1) {
        adc_val.v = GET_BITFIELD(vad->REG_0CC, VAD_CODEC_SARADC_GAIN);
    } else {
        adc_val.v = 0;
    }

    *gain = adc_val.v;
    // Gain format: 8.12
    *gain /= (1 << 12);
}

void hal_codec_adc_mute(bool mute)
{
    codec_adc_mute = mute;

    if (mute) {
        hal_codec_set_adc_gain_value(NORMAL_ADC_MAP, 0);
    } else {
        hal_codec_restore_dig_adc_gain();
    }
}

int hal_codec_set_chan_vol(enum AUD_STREAM_T stream, enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol)
{
    if (stream == AUD_STREAM_CAPTURE) {
#ifdef SINGLE_CODEC_ADC_VOL
        ASSERT(false, "%s: Cannot set cap chan vol with SINGLE_CODEC_ADC_VOL", __func__);
#else
        uint8_t mic_ch, adc_ch;
        enum AUD_CHANNEL_MAP_T map;
        const CODEC_ADC_VOL_T *adc_gain_db;

        adc_gain_db = hal_codec_get_adc_volume(vol);
        if (adc_gain_db) {
            map = ch_map;
            while (map) {
                mic_ch = get_lsb_pos(map);
                map &= ~(1 << mic_ch);
                adc_ch = hal_codec_get_adc_chan(1 << mic_ch);
                ASSERT(adc_ch < NORMAL_ADC_CH_NUM, "%s: Bad cap ch_map=0x%X (ch=%u)", __func__, ch_map, mic_ch);
                hal_codec_set_dig_adc_gain((1 << adc_ch), *adc_gain_db);
            }
        }
#endif
    }

    return 0;
}

static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val)
{
    uint32_t sel = 0;

    if (val == 3) {
        sel = 0;
    } else if (val == 6) {
        sel = 1;
    } else if (val == 1) {
        sel = 2;
    } else {
        ASSERT(false, "%s: Invalid adc down: %u", __FUNCTION__, val);
    }
    if (map & AUD_CHANNEL_MAP_CH0) {
        vad->REG_084 = SET_BITFIELD(vad->REG_084, VAD_CODEC_ADC_DOWN_SEL_CH0, sel);
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        vad->REG_0A0 = SET_BITFIELD(vad->REG_0A0, VAD_CODEC_DOWN_SEL_SARADC, sel);
    }
    return 0;
}

static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map, uint32_t cnt)
{
    uint32_t bypass = 0;
    uint32_t bypass_mask = VAD_CODEC_ADC_HBF1_BYPASS_CH0 | VAD_CODEC_ADC_HBF2_BYPASS_CH0 | VAD_CODEC_ADC_HBF3_BYPASS_CH0;

    if (cnt == 0) {
    } else if (cnt == 1) {
        bypass = VAD_CODEC_ADC_HBF3_BYPASS_CH0;
    } else if (cnt == 2) {
        bypass = VAD_CODEC_ADC_HBF2_BYPASS_CH0 | VAD_CODEC_ADC_HBF3_BYPASS_CH0;
    } else if (cnt == 3) {
        bypass = VAD_CODEC_ADC_HBF1_BYPASS_CH0 | VAD_CODEC_ADC_HBF2_BYPASS_CH0 | VAD_CODEC_ADC_HBF3_BYPASS_CH0;
    } else {
        ASSERT(false, "%s: Invalid bypass cnt: %u", __FUNCTION__, cnt);
    }
    if (map & AUD_CHANNEL_MAP_CH0) {
        vad->REG_084 = (vad->REG_084 & ~bypass_mask) | bypass;
    }
    if (map & AUD_CHANNEL_MAP_CH1) {
        ASSERT(cnt == 0, "%s: SARADC not support bypass cnt: %u", __FUNCTION__, cnt);
    }
    return 0;
}

int hal_codec_setup_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, const struct HAL_CODEC_CONFIG_T *cfg)
{
    int i;
    enum AUD_SAMPRATE_T sample_rate;
    POSSIBLY_UNUSED uint32_t mask, val;

    if (stream == AUD_STREAM_CAPTURE) {
        enum AUD_CHANNEL_MAP_T mic_map;
        uint8_t cnt;
        uint8_t ch_idx;
        uint32_t cfg_set_mask;
        uint32_t cfg_clr_mask;
        bool alloc_adc = false;

        mic_map = 0;
        if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) & cfg->set_flag) {
            codec_adc_ch_map = 0;
            codec_mic_ch_map = 0;
            mic_map = cfg->channel_map;
            alloc_adc = true;
        }

        if (alloc_adc) {
            codec_mic_ch_map = mic_map;

            if (mic_map & AUD_CHANNEL_MAP_CH4) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH0;
                mic_map &= ~AUD_CHANNEL_MAP_CH4;

                vad->REG_090 &= ~VAD_CODEC_PDM_ADC_SEL_CH0;
            } else if (mic_map & AUD_CHANNEL_MAP_DIGMIC_CH0) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH0;
                mic_map &= ~AUD_CHANNEL_MAP_DIGMIC_CH0;

                vad->REG_090 |= VAD_CODEC_PDM_ADC_SEL_CH0;
            }

            if (mic_map & AUD_CHANNEL_MAP_CH5) {
                codec_adc_ch_map |= AUD_CHANNEL_MAP_CH1;
                mic_map &= ~AUD_CHANNEL_MAP_CH5;
            }

#if defined(CODEC_ADC_DC_FILTER_FACTOR)
            if (codec_adc_ch_map) {
                hal_codec_enable_adc_dc_filter(codec_adc_ch_map, true);
            }
#endif

            ASSERT(mic_map == 0, "%s: Bad cap chan map: 0x%X", __func__, mic_map);
        }

        if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
            cfg_set_mask = 0;
            cfg_clr_mask = VAD_MODE_16BIT_ADC | VAD_MODE_24BIT_ADC | VAD_MODE_32BIT_ADC;
            if (cfg->bits == AUD_BITS_16) {
                cfg_set_mask |= VAD_MODE_16BIT_ADC;
            } else if (cfg->bits == AUD_BITS_24) {
                cfg_set_mask |= VAD_MODE_24BIT_ADC;
            } else if (cfg->bits == AUD_BITS_32) {
                cfg_set_mask |= VAD_MODE_32BIT_ADC;
            } else {
                ASSERT(false, "%s: Bad cap bits: %d", __func__, cfg->bits);
            }
            vad->REG_040 = (vad->REG_040 & ~cfg_clr_mask) | cfg_set_mask;
#ifdef VOICE_DETECTOR_EN
            codec_adc_bits = cfg->bits;
#endif
        }

        cnt = 0;
        for (i = 0; i < MAX_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
                cnt++;
            }
        }
        ASSERT(cnt == cfg->channel_num, "%s: Invalid capture stream chan cfg: mic_map=0x%X adc_map=0x%X num=%u",
            __func__, codec_mic_ch_map, codec_adc_ch_map, cfg->channel_num);

        if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
            sample_rate = cfg->sample_rate;

            for(i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
                if(codec_adc_sample_rate[i].sample_rate == sample_rate) {
                    break;
                }
            }
            ASSERT(i < ARRAY_SIZE(codec_adc_sample_rate), "%s: Invalid capture sample rate: %d", __func__, sample_rate);

            TRACE(2,"[%s] capture sample_rate=%d", __func__, sample_rate);

            hal_codec_set_adc_down(codec_adc_ch_map, codec_adc_sample_rate[i].adc_down);
            hal_codec_set_adc_hbf_bypass_cnt(codec_adc_ch_map, codec_adc_sample_rate[i].bypass_cnt);
#ifdef VOICE_DETECTOR_EN
            codec_adc_down = codec_adc_sample_rate[i].adc_down;
#endif
        }

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
        if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
#ifdef SINGLE_CODEC_ADC_VOL
            const CODEC_ADC_VOL_T *adc_gain_db;
            adc_gain_db = hal_codec_get_adc_volume(cfg->vol);
            if (adc_gain_db) {
                hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
            }
#else // !SINGLE_CODEC_ADC_VOL
            uint32_t vol;

            mic_map = codec_mic_ch_map;
            while (mic_map) {
                ch_idx = get_lsb_pos(mic_map);
                mic_map &= ~(1 << ch_idx);
                vol = hal_codec_get_mic_chan_volume_level(1 << ch_idx);
                hal_codec_set_chan_vol(AUD_STREAM_CAPTURE, (1 << ch_idx), vol);
            }
#endif // !SINGLE_CODEC_ADC_VOL
        }
#endif
    }

    return 0;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map)
{
    uint8_t adc_ch;

    adc_ch = MAX_ADC_CH_NUM;

    if (mic_map & AUD_CHANNEL_MAP_CH4) {
        adc_ch = 0;
    } else if (mic_map & AUD_CHANNEL_MAP_DIGMIC_CH0) {
        adc_ch = 0;
    } else if (mic_map & AUD_CHANNEL_MAP_CH5) {
        adc_ch = 1;
    }
    return adc_ch;
}

int hal_codec_config_digmic_phase(uint8_t phase)
{
#ifdef ANC_PROD_TEST
    codec_digmic_phase = phase;
#endif
    return 0;
}

static void hal_codec_general_irq_handler(void)
{
    uint32_t status;

    status = vad->REG_00C;
    vad->REG_00C = status;

    for (int i = 0; i < CODEC_IRQ_TYPE_QTY; i++) {
        if (codec_irq_callback[i]) {
            codec_irq_callback[i](status);
        }
    }
}

POSSIBLY_UNUSED
static void hal_codec_set_irq_handler(enum CODEC_IRQ_TYPE_T type, HAL_CODEC_IRQ_CALLBACK cb)
{
    uint32_t lock;
    IRQn_Type irqt = VAD_IRQn;

    ASSERT(type < CODEC_IRQ_TYPE_QTY, "%s: Bad type=%d", __func__, type);

    lock = int_lock();

    codec_irq_callback[type] = cb;

    if (cb) {
        if (codec_irq_map == 0) {
            NVIC_SetVector(irqt, (uint32_t)hal_codec_general_irq_handler);
            NVIC_SetPriority(irqt, IRQ_PRIORITY_NORMAL);
            NVIC_ClearPendingIRQ(irqt);
            NVIC_EnableIRQ(irqt);
        }
        codec_irq_map |= (1 << type);
    } else {
        codec_irq_map &= ~(1 << type);
        if (codec_irq_map == 0) {
            NVIC_DisableIRQ(irqt);
            NVIC_ClearPendingIRQ(irqt);
        }
    }
    int_unlock(lock);
}

#ifdef VOICE_DETECTOR_EN
//#define CODEC_VAD_DEBUG

static inline void hal_codec_vad_set_udc(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_U_DC, v);
}

static inline void hal_codec_vad_set_upre(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_U_PRE, v);
}

static inline void hal_codec_vad_set_frame_len(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_FRAME_LEN, v);
}

static inline void hal_codec_vad_set_mvad(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_MVAD, v);
}

static inline void hal_codec_vad_set_pre_gain(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_PRE_GAIN, v);
}

static inline void hal_codec_vad_set_sth(int v)
{
    vad->REG_104 = SET_BITFIELD(vad->REG_104, VAD_VAD_STH, v);
}

static inline void hal_codec_vad_set_frame_th1(int v)
{
    vad->REG_108 = SET_BITFIELD(vad->REG_108, VAD_VAD_FRAME_TH1, v);
}

static inline void hal_codec_vad_set_frame_th2(int v)
{
    vad->REG_108 = SET_BITFIELD(vad->REG_108, VAD_VAD_FRAME_TH2, v);
}

static inline void hal_codec_vad_set_frame_th3(int v)
{
    vad->REG_108 = SET_BITFIELD(vad->REG_108, VAD_VAD_FRAME_TH3, v);
}

static inline void hal_codec_vad_set_range1(int v)
{
    vad->REG_10C = SET_BITFIELD(vad->REG_10C, VAD_VAD_RANGE1, v);
}

static inline void hal_codec_vad_set_range2(int v)
{
    vad->REG_10C = SET_BITFIELD(vad->REG_10C, VAD_VAD_RANGE2, v);
}

static inline void hal_codec_vad_set_range3(int v)
{
    vad->REG_10C = SET_BITFIELD(vad->REG_10C, VAD_VAD_RANGE3, v);
}

static inline void hal_codec_vad_set_range4(int v)
{
    vad->REG_10C = SET_BITFIELD(vad->REG_10C, VAD_VAD_RANGE4, v);
}

static inline void hal_codec_vad_set_psd_th1(int v)
{
    vad->REG_110 = SET_BITFIELD(vad->REG_110, VAD_VAD_PSD_TH1, v);
}

static inline void hal_codec_vad_set_psd_th2(int v)
{
    vad->REG_114 = SET_BITFIELD(vad->REG_114, VAD_VAD_PSD_TH2, v);
}

static inline void hal_codec_vad_en(int enable)
{
    if (enable) {
        vad->REG_100 |= VAD_VAD_EN; //enable vad
    } else {
        vad->REG_100 &= ~VAD_VAD_EN; //disable vad
        vad->REG_100 |= VAD_VAD_FINISH;
    }
}

static inline void hal_codec_vad_bypass_ds(int bypass)
{
    if (bypass)
        vad->REG_100 |= VAD_VAD_DS_BYPASS; //bypass ds
    else
        vad->REG_100 &= ~VAD_VAD_DS_BYPASS; //not bypass ds
}

static inline void hal_codec_vad_bypass_dc(int bypass)
{
    if (bypass)
        vad->REG_100 |= VAD_VAD_DC_CANCEL_BYPASS; // bypass dc
    else
        vad->REG_100 &= ~VAD_VAD_DC_CANCEL_BYPASS; //not bypass dc
}

static inline void hal_codec_vad_bypass_pre(int bypass)
{
    if (bypass)
        vad->REG_100 |= VAD_VAD_PRE_BYPASS; //bypass pre
    else
        vad->REG_100 &= ~VAD_VAD_PRE_BYPASS; //not bypass pre
}

static inline void hal_codec_vad_dig_mode(int enable)
{
    if (enable)
        vad->REG_100 |= VAD_VAD_DIG_MODE; //digital mode
    else
        vad->REG_100 &= ~VAD_VAD_DIG_MODE; //not digital mode
}

static inline void hal_codec_vad_mic_sel(int v)
{
    vad->REG_100 = SET_BITFIELD(vad->REG_100, VAD_VAD_MIC_SEL, v);
}

static inline void hal_codec_vad_buf_clear(void)
{
    vad->REG_07C |= (1<<31); //clear VAD buffer
}

static inline void hal_codec_vad_adc_en(int enable)
{
    uint32_t adc, idx;

    if (vad_mic_ch & AUD_CHANNEL_MAP_CH5) {
        adc = VAD_CODEC_ADC_EN_CH1;
        idx = 1;
    } else {
        adc = VAD_CODEC_ADC_EN_CH0;
        idx = 0;
    }

    if (enable) {
        if (codec_adc_user_map[idx] == 0) {
            vad->REG_080 |= (VAD_CODEC_ADC_EN | adc);
        }
        codec_adc_user_map[idx] |= (1<<CODEC_ADC_USER_VAD);
    } else {
        codec_adc_user_map[idx] &= (~(1<<CODEC_ADC_USER_VAD));
        if (codec_adc_user_map[idx] == 0) {
            uint32_t val;

            val = vad->REG_080;
            val &= ~adc;
            if ((val & (VAD_CODEC_ADC_EN_CH0 | VAD_CODEC_ADC_EN_CH1)) == 0) {
                val &= ~VAD_CODEC_ADC_EN;
            }
            vad->REG_080 = val;
        }
    }
}

static inline void hal_codec_vad_irq_en(int enable)
{
    if (enable){
        vad->REG_010 |= (VAD_VAD_FIND_MSK | VAD_VAD_NOT_FIND_MSK);
    }
    else{
        vad->REG_010 &= ~(VAD_VAD_FIND_MSK | VAD_VAD_NOT_FIND_MSK);
    }

    vad->REG_00C = VAD_VAD_FIND | VAD_VAD_NOT_FIND;
}

static inline void hal_codec_vad_adc_if_en(int enable)
{
    uint32_t adc;

    if (vad_mic_ch & AUD_CHANNEL_MAP_CH5) {
        adc = VAD_ADC_ENABLE_CH1;
    } else {
        adc = VAD_ADC_ENABLE_CH0;
    }

    if (enable) {
        vad->REG_000 |= (VAD_DMACTRL_RX | adc | VAD_ADC_ENABLE);
    } else {
        vad->REG_000 &= ~(VAD_DMACTRL_RX | adc | VAD_ADC_ENABLE);
    }
}

static inline void hal_codec_vad_set_mem_mode(uint32_t mode)
{
    vad->REG_100 = SET_BITFIELD(vad->REG_100, VAD_VAD_MEM_MODE, mode);
}

#ifdef VAD_VAD_DEBUG
void hal_codec_vad_reg_dump(void)
{
    TRACE(1,"codec base = %8x\n", (int)&(vad->REG_000));
    TRACE(1,"vad->REG_000 = %x\n", vad->REG_000);
    TRACE(1,"vad->REG_00C = %x\n", vad->REG_00C);
    TRACE(1,"vad->REG_010 = %x\n", vad->REG_010);
    TRACE(1,"vad->REG_080 = %x\n", vad->REG_080);
    TRACE(1,"vad->REG_084 = %x\n", vad->REG_084);
    TRACE(1,"vad->REG_0A0 = %x\n", vad->REG_0A0);
    TRACE(1,"vad->REG_100 = %x\n", vad->REG_100);
    TRACE(1,"vad->REG_104 = %x\n", vad->REG_104);
    TRACE(1,"vad->REG_108 = %x\n", vad->REG_108);
    TRACE(1,"vad->REG_10C = %x\n", vad->REG_10C);
    TRACE(1,"vad->REG_110 = %x\n", vad->REG_110);
    TRACE(1,"vad->REG_114 = %x\n", vad->REG_114);
}
#endif

static inline void hal_codec_vad_data_info(uint32_t *data_cnt, uint32_t *addr_cnt)
{
    uint32_t regval = vad->REG_118;

    *addr_cnt = GET_BITFIELD(regval, VAD_VAD_MEM_ADDR_CNT) * 2;
    ASSERT((CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE) <= *addr_cnt && *addr_cnt + 2 <= CODEC_VAD_MAX_BUF_SIZE,
        "%s: Bad addr_cnt=%u (min=%u max=%u)", __func__, (uint32_t)*addr_cnt, (CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE), CODEC_VAD_MAX_BUF_SIZE);

    *data_cnt = GET_BITFIELD(regval, VAD_VAD_MEM_DATA_CNT) * 2;
    ASSERT(*data_cnt + 2 <= CODEC_VAD_BUF_SIZE, "%s: Bad data_cnt=%u (max=%u)", __func__, (uint32_t)*data_cnt, CODEC_VAD_BUF_SIZE);

    if (*data_cnt + 2 == CODEC_VAD_BUF_SIZE) {
        if (*addr_cnt + 2 < CODEC_VAD_MAX_BUF_SIZE) {
            // Addr count has wrapped
            *data_cnt = CODEC_VAD_BUF_SIZE;
        }
    }
}

uint32_t hal_codec_vad_recv_data(uint8_t *dst, uint32_t dst_size)
{
    uint8_t *src = (uint8_t *)CODEC_VAD_BUF_ADDR;
    const uint32_t src_size = CODEC_VAD_BUF_SIZE;
    uint32_t len;
    uint32_t start_pos;

    TRACE(5,"%s, dst=%x, dst_size=%d, vad_data_cnt=%d, vad_addr_cnt=%d",
        __func__, (uint32_t)dst, dst_size, vad_data_cnt, vad_addr_cnt);

    if (vad_addr_cnt < (CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE)) {
        return 0;
    }
    if (vad_addr_cnt + 2 > CODEC_VAD_MAX_BUF_SIZE) {
        return 0;
    }
    if (vad_data_cnt > src_size) {
        return 0;
    }

    if (dst == NULL) {
        return vad_data_cnt;
    }

    if (vad_data_cnt < src_size) {
        start_pos = 0;
    } else {
        start_pos = vad_addr_cnt - (CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE);
    }

    len = MIN(dst_size, vad_data_cnt);
    if (len == 0) {
        return 0;
    }

    if (start_pos + len <= src_size) {
        memcpy(dst, src + start_pos, len);
    } else {
        uint32_t len1, len2;
        len1 = src_size - start_pos;
        len2 = len - len1;
        memcpy(dst, src + start_pos, len1);
        memcpy(dst + len1, src, len2);
    }

    TRACE(2,"%s, len=%d", __func__, len);
    return len;
}

void hal_codec_get_vad_data_info(struct CODEC_VAD_BUF_INFO_T* vad_buf_info)
{
    vad_buf_info->base_addr = CODEC_VAD_BUF_ADDR;
    vad_buf_info->buf_size = CODEC_VAD_BUF_SIZE;
    vad_buf_info->data_count = vad_data_cnt;
    vad_buf_info->addr_count = vad_addr_cnt;
}

static void hal_codec_vad_isr(uint32_t irq_status)
{
    if ((irq_status & (VAD_VAD_FIND | VAD_VAD_NOT_FIND)) == 0) {
        return;
    }

    TRACE(2,"%s VAD_FIND=%d", __func__, !!(irq_status & VAD_VAD_FIND));

    ana_vad_trig = true;

    if (vad_handler) {
        vad_handler(!!(irq_status & VAD_VAD_FIND));
    }
}

int hal_codec_vad_config(const struct AUD_VAD_CONFIG_T *conf)
{
    unsigned int cfg_set_mask = 0;
    unsigned int cfg_clr_mask = 0;
    enum AUD_CHANNEL_MAP_T adc_ch;

    if (!conf)
        return -1;

    vad_handler = conf->handler;

    hal_codec_vad_en(0);
    hal_codec_vad_irq_en(0);

    if (conf->type == AUD_VAD_TYPE_ANA) {
        return 0;
    }

    hal_codec_vad_set_mem_mode((CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_BUF_SIZE) / 0x4000);
    hal_codec_vad_set_udc(conf->udc);
    hal_codec_vad_set_upre(conf->upre);
    hal_codec_vad_set_frame_len(conf->frame_len);
    hal_codec_vad_set_mvad(conf->mvad);
    hal_codec_vad_set_pre_gain(conf->pre_gain);
    hal_codec_vad_set_sth(conf->sth);
    hal_codec_vad_set_frame_th1(conf->frame_th[0]);
    hal_codec_vad_set_frame_th2(conf->frame_th[1]);
    hal_codec_vad_set_frame_th3(conf->frame_th[2]);
    hal_codec_vad_set_range1(conf->range[0]);
    hal_codec_vad_set_range2(conf->range[1]);
    hal_codec_vad_set_range3(conf->range[2]);
    hal_codec_vad_set_range4(conf->range[3]);
    hal_codec_vad_set_psd_th1(conf->psd_th[0]);
    hal_codec_vad_set_psd_th2(conf->psd_th[1]);
    hal_codec_vad_dig_mode(0);
    hal_codec_vad_bypass_dc(0);
    hal_codec_vad_bypass_pre(0);

    if (conf->sample_rate == AUD_SAMPRATE_8000) {
        // select adc down 8KHz
        hal_codec_vad_bypass_ds(1);
        vad_adc_down = 6;
    } else if (conf->sample_rate == AUD_SAMPRATE_16000) {
        // select adc down 16KHz
        hal_codec_vad_bypass_ds(0);
        vad_adc_down = 3;
    } else {
        ASSERT(false, "%s: Bad sample rate: %u", __func__, conf->sample_rate);
    }
    if (vad_mic_ch & AUD_CHANNEL_MAP_CH5) {
        adc_ch = AUD_CHANNEL_MAP_CH1;
    } else {
        adc_ch = AUD_CHANNEL_MAP_CH0;
    }
    hal_codec_set_adc_down(adc_ch, vad_adc_down);
    hal_codec_vad_mic_sel(get_lsb_pos(adc_ch));

    vad_adc_bits = conf->bits;
    cfg_clr_mask = VAD_MODE_16BIT_ADC| VAD_MODE_24BIT_ADC | VAD_MODE_32BIT_ADC;
    if (vad_adc_bits == AUD_BITS_16) {
        cfg_set_mask |= VAD_MODE_16BIT_ADC;
    } else if (vad_adc_bits == AUD_BITS_24) {
        cfg_set_mask |= VAD_MODE_24BIT_ADC;
    } else if (vad_adc_bits == AUD_BITS_32) {
        cfg_set_mask |= VAD_MODE_32BIT_ADC;
    } else {
        ASSERT(false, "%s: Bad cap bits: %d", __func__, vad_adc_bits);
    }
    vad->REG_040 = (vad->REG_040 & ~cfg_clr_mask) | cfg_set_mask;

    vad->REG_124 = VAD_VAD_DELAY1(320 / 4); // 10 ms (value will be shifted left by 2 bits since 2300a)
    vad->REG_128 = VAD_VAD_DELAY2(32000 * 3); //vad timeout value
#ifdef I2C_VAD
    vad->REG_100 |= VAD_VAD_EXT_EN | VAD_VAD_SRC_SEL(0);
#endif

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
    const CODEC_ADC_VOL_T *adc_gain_db;

#ifdef SINGLE_CODEC_ADC_VOL
    adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
#else
    adc_gain_db = hal_codec_get_adc_volume(hal_codec_get_mic_chan_volume_level(vad_mic_ch));
#endif
    if (adc_gain_db) {
        hal_codec_set_dig_adc_gain(adc_ch, *adc_gain_db);
    }
#endif

    return 0;
}

int hal_codec_vad_open(const struct AUD_VAD_CONFIG_T *conf)
{
#ifndef __AUDIO_RESAMPLE__
#error "VOICE_DETECTOR_EN must work with AUDIO_RESAMPLE"
#endif

    vad_type = conf->type;

    TRACE(0, "%s: type=%d", __func__, vad_type);

    if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_ANA) {
        ASSERT((vad_mic_ch == AUD_CHANNEL_MAP_CH4) || (vad_mic_ch == AUD_CHANNEL_MAP_CH5),
            "%s: ANA VAD MIC must be MIC_E (CH4) or MIC_SARADC (CH5) : 0x%X", __func__, vad_mic_ch);
    }

    // open analog vad
    analog_aud_vad_adc_enable(true);

    hal_codec_vad_config(conf);

    return 0;
}

int hal_codec_vad_close(void)
{
    TRACE(0, "%s: type=%d", __func__, vad_type);

#ifdef I2C_VAD
    vad->REG_100 &= ~(VAD_VAD_EXT_EN | VAD_VAD_SRC_SEL(0));
#endif

    // close analog vad
    analog_aud_vad_adc_enable(false);

    vad_type = AUD_VAD_TYPE_NONE;

    return 0;
}

int hal_codec_vad_start(void)
{
    if (vad_enabled) {
        return 0;
    }
    vad_enabled = true;
    vad_data_cnt = 0;
    vad_addr_cnt = 0;

    if (vad_type == AUD_VAD_TYPE_MIX) {
        ASSERT(stream_started == 0, "VAD mix mode must use codec exclusively before ana vad trig: stream_started=0x%X", stream_started);
        ana_vad_trig = false;
    }

    // Enable TOP_OSC/VAD_32K
    // MIX-MODE: Disable codec clock before enabling VAD (otherwise DIG VAD will start working right now)
    hal_cmu_codec_vad_clock_enable(vad_type);

    hal_codec_vad_buf_clear();
    hal_codec_vad_irq_en(1);
    hal_codec_set_irq_handler(CODEC_IRQ_TYPE_VAD, hal_codec_vad_isr);

    if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG) {
        if (vad->REG_000 & VAD_ADC_ENABLE) {
            ASSERT(false, "%s: VAD cannot work with cap stream", __func__);
        }
        // digital vad
        hal_codec_vad_en(1);
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE != 0)
        // enable adc if
        hal_codec_vad_adc_if_en(1);
#endif
        // enable adc
        hal_codec_vad_adc_en(1);
    }

    analog_aud_vad_enable(vad_type, true);

    return 0;
}

static int hal_codec_vad_switch_off(bool adc_state)
{
    if (!vad_enabled) {
        return 0;
    }
    vad_enabled = false;

    // Disable IRQ first
    hal_codec_vad_irq_en(0);
    hal_codec_set_irq_handler(CODEC_IRQ_TYPE_VAD, NULL);

    // Disable TOP_OSC/VAD_32K
    // MIX-MODE: Enable codec_clock (might trigger DIG VAD to work -- it is OK since IRQ has been disabled)
    hal_cmu_codec_vad_clock_disable(vad_type);

    if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG) {
        // Get VAD data info before disabling VAD
        hal_codec_vad_data_info(&vad_data_cnt, &vad_addr_cnt);
        hal_codec_vad_en(0);
#if defined(CODEC_VAD_CFG_BUF_SIZE) && (CODEC_VAD_CFG_BUF_SIZE != 0)
        hal_codec_vad_adc_if_en(0);
#endif
        if (adc_state == false) {
            hal_codec_vad_adc_en(0);
        }
    }

    analog_aud_vad_enable(vad_type, false);

    if (vad_type == AUD_VAD_TYPE_MIX) {
        ana_vad_trig = false;
    }

    return 0;
}

int hal_codec_vad_stop(void)
{
    return hal_codec_vad_switch_off(false);
}

#endif

#endif

