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
#ifndef __REG_VAD_BEST1501_H__
#define __REG_VAD_BEST1501_H__

#include "plat_types.h"

struct VAD_T {
    __IO uint32_t REG_000;
    __IO uint32_t REG_004;
    __IO uint32_t REG_008;
    __IO uint32_t REG_00C;
    __IO uint32_t REG_010;
    __IO uint32_t REG_014;
    __IO uint32_t REG_018;
    __IO uint32_t REG_01C;
    __IO uint32_t REG_020;
    __IO uint32_t REG_024;
    __IO uint32_t REG_028;
    __IO uint32_t REG_02C;
    __IO uint32_t REG_030;
    __IO uint32_t REG_034;
    __IO uint32_t REG_038;
    __IO uint32_t REG_03C;
    __IO uint32_t REG_040;
    __IO uint32_t REG_044;
    __IO uint32_t REG_048;
    __IO uint32_t REG_04C;
    __IO uint32_t REG_050;
    __IO uint32_t REG_054;
    __IO uint32_t REG_058;
    __IO uint32_t REG_05C;
    __IO uint32_t REG_060;
    __IO uint32_t REG_064;
    __IO uint32_t REG_068;
    __IO uint32_t REG_06C;
    __IO uint32_t REG_070;
    __IO uint32_t REG_074;
    __IO uint32_t REG_078;
    __IO uint32_t REG_07C;
    __IO uint32_t REG_080;
    __IO uint32_t REG_084;
    __IO uint32_t REG_088;
    __IO uint32_t REG_08C;
    __IO uint32_t REG_090;
    __IO uint32_t REG_094;
    __IO uint32_t REG_098;
    __IO uint32_t REG_09C;
    __IO uint32_t REG_0A0;
    __IO uint32_t REG_0A4;
    __IO uint32_t REG_0A8;
    __IO uint32_t REG_0AC;
    __IO uint32_t REG_0B0;
    __IO uint32_t REG_0B4;
    __IO uint32_t REG_0B8;
    __IO uint32_t REG_0BC;
    __IO uint32_t REG_0C0;
    __IO uint32_t REG_0C4;
    __IO uint32_t REG_0C8;
    __IO uint32_t REG_0CC;
    __IO uint32_t REG_0D0;
    __IO uint32_t REG_0D4;
    __IO uint32_t REG_0D8;
    __IO uint32_t REG_0DC;
    __IO uint32_t REG_0E0;
    __IO uint32_t REG_0E4;
    __IO uint32_t REG_0E8;
    __IO uint32_t REG_0EC;
    __IO uint32_t REG_0F0;
    __IO uint32_t REG_0F4;
    __IO uint32_t REG_0F8;
    __IO uint32_t REG_0FC;
    __IO uint32_t REG_100;
    __IO uint32_t REG_104;
    __IO uint32_t REG_108;
    __IO uint32_t REG_10C;
    __IO uint32_t REG_110;
    __IO uint32_t REG_114;
    __IO uint32_t REG_118;
    __IO uint32_t REG_11C;
    __IO uint32_t REG_120;
    __IO uint32_t REG_124;
    __IO uint32_t REG_128;
};

// reg_00
#define VAD_CODEC_IF_EN                                     (1 << 0)
#define VAD_ADC_ENABLE                                      (1 << 1)
#define VAD_ADC_ENABLE_CH0                                  (1 << 2)
#define VAD_ADC_ENABLE_CH1                                  (1 << 3)
#define VAD_DMACTRL_RX                                      (1 << 4)

// reg_04
#define VAD_RX_FIFO_FLUSH_CH0                               (1 << 0)
#define VAD_RX_FIFO_FLUSH_CH1                               (1 << 1)
#define VAD_CODEC_RX_THRESHOLD(n)                           (((n) & 0xF) << 2)
#define VAD_CODEC_RX_THRESHOLD_MASK                         (0xF << 2)
#define VAD_CODEC_RX_THRESHOLD_SHIFT                        (2)

// reg_0c
#define VAD_CODEC_RX_OVERFLOW(n)                            (((n) & 0x3) << 0)
#define VAD_CODEC_RX_OVERFLOW_MASK                          (0x3 << 0)
#define VAD_CODEC_RX_OVERFLOW_SHIFT                         (0)
#define VAD_CODEC_RX_UNDERFLOW(n)                           (((n) & 0x3) << 2)
#define VAD_CODEC_RX_UNDERFLOW_MASK                         (0x3 << 2)
#define VAD_CODEC_RX_UNDERFLOW_SHIFT                        (2)
#define VAD_VAD_FIND                                        (1 << 4)
#define VAD_VAD_NOT_FIND                                    (1 << 5)

// reg_10
#define VAD_CODEC_RX_OVERFLOW_MSK(n)                        (((n) & 0x3) << 0)
#define VAD_CODEC_RX_OVERFLOW_MSK_MASK                      (0x3 << 0)
#define VAD_CODEC_RX_OVERFLOW_MSK_SHIFT                     (0)
#define VAD_CODEC_RX_UNDERFLOW_MSK(n)                       (((n) & 0x3) << 2)
#define VAD_CODEC_RX_UNDERFLOW_MSK_MASK                     (0x3 << 2)
#define VAD_CODEC_RX_UNDERFLOW_MSK_SHIFT                    (2)
#define VAD_VAD_FIND_MSK                                    (1 << 4)
#define VAD_VAD_NOT_FIND_MSK                                (1 << 5)

// reg_14
#define VAD_FIFO_COUNT_CH0(n)                               (((n) & 0xF) << 0)
#define VAD_FIFO_COUNT_CH0_MASK                             (0xF << 0)
#define VAD_FIFO_COUNT_CH0_SHIFT                            (0)
#define VAD_FIFO_COUNT_CH1(n)                               (((n) & 0xF) << 4)
#define VAD_FIFO_COUNT_CH1_MASK                             (0xF << 4)
#define VAD_FIFO_COUNT_CH1_SHIFT                            (4)

// reg_1c
#define VAD_RX_FIFO_DATA(n)                                 (((n) & 0xFFFFFFFF) << 0)
#define VAD_RX_FIFO_DATA_MASK                               (0xFFFFFFFF << 0)
#define VAD_RX_FIFO_DATA_SHIFT                              (0)

// reg_20
#define VAD_RX_FIFO_DATA(n)                                 (((n) & 0xFFFFFFFF) << 0)
#define VAD_RX_FIFO_DATA_MASK                               (0xFFFFFFFF << 0)
#define VAD_RX_FIFO_DATA_SHIFT                              (0)

// reg_24
#define VAD_RX_FIFO_DATA(n)                                 (((n) & 0xFFFFFFFF) << 0)
#define VAD_RX_FIFO_DATA_MASK                               (0xFFFFFFFF << 0)
#define VAD_RX_FIFO_DATA_SHIFT                              (0)

// reg_40
#define VAD_MODE_16BIT_ADC                                  (1 << 0)
#define VAD_MODE_24BIT_ADC                                  (1 << 1)
#define VAD_MODE_32BIT_ADC                                  (1 << 2)
#define VAD_MODE_HCLK_ACCESS                                (1 << 3)

// reg_44
#define VAD_RET1N_SRAM                                      (1 << 0)
#define VAD_RET2N_SRAM                                      (1 << 1)
#define VAD_PGEN_SRAM                                       (1 << 2)

// reg_7c
#define VAD_RESERVED_REG2(n)                                (((n) & 0xFFFFFFFF) << 0)
#define VAD_RESERVED_REG2_MASK                              (0xFFFFFFFF << 0)
#define VAD_RESERVED_REG2_SHIFT                             (0)

// reg_80
#define VAD_CODEC_ADC_EN                                    (1 << 0)
#define VAD_CODEC_ADC_EN_CH0                                (1 << 1)
#define VAD_CODEC_ADC_EN_CH1                                (1 << 2)
#define VAD_CODEC_TEST_PORT_SEL(n)                          (((n) & 0x7) << 3)
#define VAD_CODEC_TEST_PORT_SEL_MASK                        (0x7 << 3)
#define VAD_CODEC_TEST_PORT_SEL_SHIFT                       (3)

// reg_84
#define VAD_CODEC_ADC_SIGNED_CH0                            (1 << 0)
#define VAD_CODEC_ADC_IN_SEL_CH0                            (1 << 1)
#define VAD_CODEC_ADC_DOWN_SEL_CH0(n)                       (((n) & 0x3) << 2)
#define VAD_CODEC_ADC_DOWN_SEL_CH0_MASK                     (0x3 << 2)
#define VAD_CODEC_ADC_DOWN_SEL_CH0_SHIFT                    (2)
#define VAD_CODEC_ADC_HBF3_BYPASS_CH0                       (1 << 4)
#define VAD_CODEC_ADC_HBF2_BYPASS_CH0                       (1 << 5)
#define VAD_CODEC_ADC_HBF1_BYPASS_CH0                       (1 << 6)
#define VAD_CODEC_ADC_GAIN_UPDATE_CH0                       (1 << 7)
#define VAD_CODEC_ADC_GAIN_SEL_CH0                          (1 << 8)
#define VAD_CODEC_ADC_GAIN_CH0(n)                           (((n) & 0xFFFFF) << 9)
#define VAD_CODEC_ADC_GAIN_CH0_MASK                         (0xFFFFF << 9)
#define VAD_CODEC_ADC_GAIN_CH0_SHIFT                        (9)
#define VAD_CODEC_ADC_HBF3_SEL_CH0(n)                       (((n) & 0x3) << 29)
#define VAD_CODEC_ADC_HBF3_SEL_CH0_MASK                     (0x3 << 29)
#define VAD_CODEC_ADC_HBF3_SEL_CH0_SHIFT                    (29)

// reg_88
#define VAD_CODEC_ADC_DC_DOUT_CH0_SYNC(n)                   (((n) & 0x1FFFFF) << 0)
#define VAD_CODEC_ADC_DC_DOUT_CH0_SYNC_MASK                 (0x1FFFFF << 0)
#define VAD_CODEC_ADC_DC_DOUT_CH0_SYNC_SHIFT                (0)

// reg_8c
#define VAD_CODEC_ADC_DC_DIN_CH0(n)                         (((n) & 0x7FFF) << 0)
#define VAD_CODEC_ADC_DC_DIN_CH0_MASK                       (0x7FFF << 0)
#define VAD_CODEC_ADC_DC_DIN_CH0_SHIFT                      (0)
#define VAD_CODEC_ADC_DC_UPDATE_CH0                         (1 << 15)
#define VAD_CODEC_ADC_DC_SEL                                (1 << 16)
#define VAD_CODEC_ADC_DCF_BYPASS_CH0                        (1 << 17)
#define VAD_CODEC_ADC_UDC_CH0(n)                            (((n) & 0xF) << 18)
#define VAD_CODEC_ADC_UDC_CH0_MASK                          (0xF << 18)
#define VAD_CODEC_ADC_UDC_CH0_SHIFT                         (18)

// reg_90
#define VAD_CODEC_PDM_ENABLE                                (1 << 0)
#define VAD_CODEC_PDM_DATA_INV                              (1 << 1)
#define VAD_CODEC_PDM_RATE_SEL(n)                           (((n) & 0x3) << 2)
#define VAD_CODEC_PDM_RATE_SEL_MASK                         (0x3 << 2)
#define VAD_CODEC_PDM_RATE_SEL_SHIFT                        (2)
#define VAD_CODEC_PDM_ADC_SEL_CH0                           (1 << 4)
#define VAD_CODEC_PDM_MUX_CH0                               (1 << 5)
#define VAD_CODEC_PDM_CAP_PHASE_CH0(n)                      (((n) & 0x3) << 6)
#define VAD_CODEC_PDM_CAP_PHASE_CH0_MASK                    (0x3 << 6)
#define VAD_CODEC_PDM_CAP_PHASE_CH0_SHIFT                   (6)

// reg_a0
#define VAD_CODEC_SARADC_IN_SIGNED                          (1 << 0)
#define VAD_CODEC_DOWN_SEL_SARADC(n)                        (((n) & 0x3) << 1)
#define VAD_CODEC_DOWN_SEL_SARADC_MASK                      (0x3 << 1)
#define VAD_CODEC_DOWN_SEL_SARADC_SHIFT                     (1)
#define VAD_CODEC_SR_SEL_SARADC(n)                          (((n) & 0x7) << 3)
#define VAD_CODEC_SR_SEL_SARADC_MASK                        (0x7 << 3)
#define VAD_CODEC_SR_SEL_SARADC_SHIFT                       (3)
#define VAD_CODEC_SARADC_INN_IIR_CNT(n)                     (((n) & 0x3) << 6)
#define VAD_CODEC_SARADC_INN_IIR_CNT_MASK                   (0x3 << 6)
#define VAD_CODEC_SARADC_INN_IIR_CNT_SHIFT                  (6)
#define VAD_CODEC_SARADC_DCF_BYPASS                         (1 << 8)
#define VAD_CODEC_SARADC_COEF_UDC(n)                        (((n) & 0xFFFFF) << 9)
#define VAD_CODEC_SARADC_COEF_UDC_MASK                      (0xFFFFF << 9)
#define VAD_CODEC_SARADC_COEF_UDC_SHIFT                     (9)

// reg_a4
#define VAD_CODEC_SARADC_COEF0_A1(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF0_A1_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF0_A1_SHIFT                     (0)

// reg_a8
#define VAD_CODEC_SARADC_COEF0_A2(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF0_A2_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF0_A2_SHIFT                     (0)

// reg_ac
#define VAD_CODEC_SARADC_COEF0_B0(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF0_B0_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF0_B0_SHIFT                     (0)

// reg_b0
#define VAD_CODEC_SARADC_COEF0_B1(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF0_B1_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF0_B1_SHIFT                     (0)

// reg_b4
#define VAD_CODEC_SARADC_COEF0_B2(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF0_B2_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF0_B2_SHIFT                     (0)

// reg_b8
#define VAD_CODEC_SARADC_COEF1_A1(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF1_A1_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF1_A1_SHIFT                     (0)

// reg_bc
#define VAD_CODEC_SARADC_COEF1_A2(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF1_A2_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF1_A2_SHIFT                     (0)

// reg_c0
#define VAD_CODEC_SARADC_COEF1_B0(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF1_B0_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF1_B0_SHIFT                     (0)

// reg_c4
#define VAD_CODEC_SARADC_COEF1_B1(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF1_B1_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF1_B1_SHIFT                     (0)

// reg_c8
#define VAD_CODEC_SARADC_COEF1_B2(n)                        (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_COEF1_B2_MASK                      (0xFFFFF << 0)
#define VAD_CODEC_SARADC_COEF1_B2_SHIFT                     (0)

// reg_cc
#define VAD_CODEC_SARADC_GAIN(n)                            (((n) & 0xFFFFF) << 0)
#define VAD_CODEC_SARADC_GAIN_MASK                          (0xFFFFF << 0)
#define VAD_CODEC_SARADC_GAIN_SHIFT                         (0)
#define VAD_CODEC_SARADC_GAIN_SEL                           (1 << 20)
#define VAD_CODEC_SARADC_GAIN_UPDATE                        (1 << 21)

// reg_d0
#define VAD_CODEC_SARADC_DC_DOUT_SYNC(n)                    (((n) & 0x1FFFF) << 0)
#define VAD_CODEC_SARADC_DC_DOUT_SYNC_MASK                  (0x1FFFF << 0)
#define VAD_CODEC_SARADC_DC_DOUT_SYNC_SHIFT                 (0)

// reg_d4
#define VAD_CODEC_SARADC_DC_DIN(n)                          (((n) & 0x1FFFF) << 0)
#define VAD_CODEC_SARADC_DC_DIN_MASK                        (0x1FFFF << 0)
#define VAD_CODEC_SARADC_DC_DIN_SHIFT                       (0)
#define VAD_CODEC_SARADC_DC_UPDATE                          (1 << 17)

// reg_100
#define VAD_VAD_EN                                          (1 << 0)
#define VAD_VAD_DS_BYPASS                                   (1 << 1)
#define VAD_VAD_DC_CANCEL_BYPASS                            (1 << 2)
#define VAD_VAD_PRE_BYPASS                                  (1 << 3)
#define VAD_VAD_DIG_MODE                                    (1 << 4)
#define VAD_VAD_I2S_EN                                      (1 << 5)
#define VAD_VAD_I2S_DATA_CNT(n)                             (((n) & 0x7) << 6)
#define VAD_VAD_I2S_DATA_CNT_MASK                           (0x7 << 6)
#define VAD_VAD_I2S_DATA_CNT_SHIFT                          (6)
#define VAD_VAD_I2S_DATA_SEL(n)                             (((n) & 0x7) << 9)
#define VAD_VAD_I2S_DATA_SEL_MASK                           (0x7 << 9)
#define VAD_VAD_I2S_DATA_SEL_SHIFT                          (9)
#define VAD_VAD_I2S_DATA_MODE(n)                            (((n) & 0x3) << 12)
#define VAD_VAD_I2S_DATA_MODE_MASK                          (0x3 << 12)
#define VAD_VAD_I2S_DATA_MODE_SHIFT                         (12)
#define VAD_VAD_EXT_EN                                      (1 << 14)
#define VAD_VAD_MIC_SEL(n)                                  (((n) & 0x7) << 15)
#define VAD_VAD_MIC_SEL_MASK                                (0x7 << 15)
#define VAD_VAD_MIC_SEL_SHIFT                               (15)
#define VAD_VAD_SRC_SEL(n)                                  (((n) & 0x3) << 18)
#define VAD_VAD_SRC_SEL_MASK                                (0x3 << 18)
#define VAD_VAD_SRC_SEL_SHIFT                               (18)
#define VAD_VAD_DATA_EXT_SWAP                               (1 << 20)
#define VAD_VAD_MEM_MODE(n)                                 (((n) & 0x7) << 21)
#define VAD_VAD_MEM_MODE_MASK                               (0x7 << 21)
#define VAD_VAD_MEM_MODE_SHIFT                              (21)
#define VAD_VAD_FINISH                                      (1 << 24)

// reg_104
#define VAD_VAD_U_DC(n)                                     (((n) & 0xF) << 0)
#define VAD_VAD_U_DC_MASK                                   (0xF << 0)
#define VAD_VAD_U_DC_SHIFT                                  (0)
#define VAD_VAD_U_PRE(n)                                    (((n) & 0x7) << 4)
#define VAD_VAD_U_PRE_MASK                                  (0x7 << 4)
#define VAD_VAD_U_PRE_SHIFT                                 (4)
#define VAD_VAD_FRAME_LEN(n)                                (((n) & 0xFF) << 7)
#define VAD_VAD_FRAME_LEN_MASK                              (0xFF << 7)
#define VAD_VAD_FRAME_LEN_SHIFT                             (7)
#define VAD_VAD_MVAD(n)                                     (((n) & 0xF) << 15)
#define VAD_VAD_MVAD_MASK                                   (0xF << 15)
#define VAD_VAD_MVAD_SHIFT                                  (15)
#define VAD_VAD_PRE_GAIN(n)                                 (((n) & 0x3F) << 19)
#define VAD_VAD_PRE_GAIN_MASK                               (0x3F << 19)
#define VAD_VAD_PRE_GAIN_SHIFT                              (19)
#define VAD_VAD_STH(n)                                      (((n) & 0x3F) << 25)
#define VAD_VAD_STH_MASK                                    (0x3F << 25)
#define VAD_VAD_STH_SHIFT                                   (25)

// reg_108
#define VAD_VAD_FRAME_TH1(n)                                (((n) & 0xFF) << 0)
#define VAD_VAD_FRAME_TH1_MASK                              (0xFF << 0)
#define VAD_VAD_FRAME_TH1_SHIFT                             (0)
#define VAD_VAD_FRAME_TH2(n)                                (((n) & 0x3FF) << 8)
#define VAD_VAD_FRAME_TH2_MASK                              (0x3FF << 8)
#define VAD_VAD_FRAME_TH2_SHIFT                             (8)
#define VAD_VAD_FRAME_TH3(n)                                (((n) & 0x3FFF) << 18)
#define VAD_VAD_FRAME_TH3_MASK                              (0x3FFF << 18)
#define VAD_VAD_FRAME_TH3_SHIFT                             (18)

// reg_10c
#define VAD_VAD_RANGE1(n)                                   (((n) & 0x1F) << 0)
#define VAD_VAD_RANGE1_MASK                                 (0x1F << 0)
#define VAD_VAD_RANGE1_SHIFT                                (0)
#define VAD_VAD_RANGE2(n)                                   (((n) & 0x7F) << 5)
#define VAD_VAD_RANGE2_MASK                                 (0x7F << 5)
#define VAD_VAD_RANGE2_SHIFT                                (5)
#define VAD_VAD_RANGE3(n)                                   (((n) & 0x1FF) << 12)
#define VAD_VAD_RANGE3_MASK                                 (0x1FF << 12)
#define VAD_VAD_RANGE3_SHIFT                                (12)
#define VAD_VAD_RANGE4(n)                                   (((n) & 0x3FF) << 21)
#define VAD_VAD_RANGE4_MASK                                 (0x3FF << 21)
#define VAD_VAD_RANGE4_SHIFT                                (21)

// reg_110
#define VAD_VAD_PSD_TH1(n)                                  (((n) & 0x7FFFFFF) << 0)
#define VAD_VAD_PSD_TH1_MASK                                (0x7FFFFFF << 0)
#define VAD_VAD_PSD_TH1_SHIFT                               (0)

// reg_114
#define VAD_VAD_PSD_TH2(n)                                  (((n) & 0x7FFFFFF) << 0)
#define VAD_VAD_PSD_TH2_MASK                                (0x7FFFFFF << 0)
#define VAD_VAD_PSD_TH2_SHIFT                               (0)

// reg_118
#define VAD_VAD_MEM_ADDR_CNT(n)                             (((n) & 0xFFFF) << 0)
#define VAD_VAD_MEM_ADDR_CNT_MASK                           (0xFFFF << 0)
#define VAD_VAD_MEM_ADDR_CNT_SHIFT                          (0)
#define VAD_VAD_MEM_DATA_CNT(n)                             (((n) & 0xFFFF) << 16)
#define VAD_VAD_MEM_DATA_CNT_MASK                           (0xFFFF << 16)
#define VAD_VAD_MEM_DATA_CNT_SHIFT                          (16)

// reg_11c
#define VAD_SMIN_SYC(n)                                     (((n) & 0x7FFFFFF) << 0)
#define VAD_SMIN_SYC_MASK                                   (0x7FFFFFF << 0)
#define VAD_SMIN_SYC_SHIFT                                  (0)

// reg_120
#define VAD_PSD_SYC(n)                                      (((n) & 0x7FFFFFF) << 0)
#define VAD_PSD_SYC_MASK                                    (0x7FFFFFF << 0)
#define VAD_PSD_SYC_SHIFT                                   (0)

// reg_124
#define VAD_VAD_DELAY1(n)                                   (((n) & 0x3FF) << 0)
#define VAD_VAD_DELAY1_MASK                                 (0x3FF << 0)
#define VAD_VAD_DELAY1_SHIFT                                (0)

// reg_128
#define VAD_VAD_DELAY2(n)                                   (((n) & 0xFFFFFF) << 0)
#define VAD_VAD_DELAY2_MASK                                 (0xFFFFFF << 0)
#define VAD_VAD_DELAY2_SHIFT                                (0)

#endif
