#pragma once
//
// Copyright 2020 BES Technic
//
// Description: Native convolve with CMSIS data-layout
//

#ifndef _NATIVE_CONVOLVE_H
#define _NATIVE_CONVOLVE_H

#include "beco_common.h"
#include "beco_types.h"
#include "beco_asm.h"

template <
    int ICH, int IW, int IH,
    int OCH, int OW, int OH,
    int PX,  int PY,
    int SX,  int SY,
    int KX,  int KY>
int ref_convolve_HWC_q15(const q15_t *in,
                                   const q15_t *wt,
                                   const q15_t *bias,
                                   const uint16_t bias_shift,
                                   const uint16_t out_shift,
                                   q15_t *out)
{
    int i, j, k, l, m, n;
    int acc, iy, ix;

    for (j = 0; j < OH; j++) {
        for (k = 0; k < OW; k++) {
            for (i = 0; i < OCH; i++) {

                acc = ((q31_t)bias[i] << bias_shift) + q_round(out_shift);
                for (m = 0; m < KY; m++) {
                    iy = SY * j + m - PY;
                    if (iy >= 0 && iy < IH) {
                        for (n = 0; n < KX; n++) {
                            ix = SX * k + n - PX;
                            if (ix >= 0 && ix < IW) {

                            	// Optimize for ICH % 2 == 0:
                                if (ICH % 2 != 0) {
                                    for (l = 0; l < ICH; l++) {
                                        acc += in[ iy*IW*ICH + ix*ICH + l] *
                                            wt[ i*ICH*KX*KY + m*KX*ICH + n*ICH + l];
                                    }
                                }
                                else {
                                	const uint32_t *pin = (const uint32_t *)&in[ iy*IW*ICH + ix*ICH ];
                                	const uint32_t *pwt = (const uint32_t *)&wt[ i*ICH*KX*KY + m*KX*ICH + n*ICH ];
                                    for (l = 0; l < ICH; l += 2)
                                        acc = BECO_SMLAD(*pin++, *pwt++, acc);
                                }
                            }
                        }
                    }
                }
                *out++ = (q15_t)BECO_SSAT((acc >> out_shift), 16);
            }

        }
    }
    return BECO_SUCCESS;
}

template <
    int ICH, int IW, int IH,
    int OCH, int OW, int OH,
    int PX,  int PY,
    int SX,  int SY,
    int KX,  int KY>
int ref_convolve_HWC_q7(const q7_t *in,
                                   const q7_t *wt,
                                   const q7_t *bias,
                                   const uint16_t bias_shift,
                                   const uint16_t out_shift,
                                   q7_t *out)
{
    int i, j, k, l, m, n;
    int acc, iy, ix;

    for (j = 0; j < OH; j++) {
        for (k = 0; k < OW; k++) {
            for (i = 0; i < OCH; i++) {

                acc = ((q31_t)bias[i] << bias_shift) + q_round(out_shift);
                for (m = 0; m < KY; m++) {
                    iy = SY * j + m - PY;
                    if (iy >= 0 && iy < IH) {
                        for (n = 0; n < KX; n++) {
                            ix = SX * k + n - PX;
                            if (ix >= 0 && ix < IW) {

                            	// Optimize for ICH % 2 == 0:
                                if (ICH % 4 != 0) {
                                    for (l = 0; l < ICH; l++) {
                                        acc += in[ iy*IW*ICH + ix*ICH + l] *
                                            wt[ i*ICH*KX*KY + m*KX*ICH + n*ICH + l];
                                    }
                                }
                                else {
                                	const uint32_t *pin = (const uint32_t *)&in[ iy*IW*ICH + ix*ICH ];
                                	const uint32_t *pwt = (const uint32_t *)&wt[ i*ICH*KX*KY + m*KX*ICH + n*ICH ];
                                    for (l = 0; l < ICH; l += 4) {
                                        uint32_t a = *pin++;
                                        uint32_t b = *pwt++;
                                        acc = BECO_SMLAD(BECO_SXTB16(a, 0), BECO_SXTB16(b, 0), acc);
                                        acc = BECO_SMLAD(BECO_SXTB16(a, 8), BECO_SXTB16(b, 8), acc);
                                    }
                                }
                            }
                        }
                    }
                }
                *out++ = (q7_t)BECO_SSAT((acc >> out_shift), 8);
            }

        }
    }
    return BECO_SUCCESS;
}

#endif
