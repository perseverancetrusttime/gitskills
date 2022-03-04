#pragma once
//
// Copyright 2020 BES Technic
//
// Description: Beco convolve with CMSIS data-layout
//

#ifndef _BECO_CONVOLVE_H
#define _BECO_CONVOLVE_H

#include "beco_common.h"

#include <assert.h>

#include "beco.h"
#include "beco_asm.h"
#include "beco_types.h"

#include "beco_rotate.h"
#include "beco_read.h"
#include "beco_bias.hpp"
#include "beco_print_matrix.hpp"


namespace Beco {

/*
Use macro below to convert Tensorflow macro parameters to Beco template parameters

    #define IN_CONV2_IN_DIM_X 256
    #define IN_CONV2_IN_DIM_Y 9

    Beco::Convolve< BECO_TFCONVNET(IN_CONV2) > conv2;

Once the bias and weights in CMSIS HWC layout have to be converted, parameters saved:

    // Convert weights once.
    conv2.setPlan(wt, bias, bias_shift, out_shift);

Then the convolve layer can be called:
    conv2.convolve(im, out);


The size of memory pools can be used to allocate memory.

    q15_t im[ conv2.imageSize ];
    q15_t out[ conv2.outSize ];
    q15_t wt[ conv2.weightSize ];
    q15_t bias[ conv2.biasSize ];

If rounding is already folded into the bias[], rounding should be
disabled. This is done by calling the setTruncate() method

        conv2.setTruncate(false);

If the Convolve layer is followed by a ReLU layer, then instantiate this Layer
with a ReLU layer merged into the output layer :

    Beco::Convolve< BECO_TFCONVNET(IN_CONV2), Beco::CopyReLU > conv2;

*/

#define BECO_TFCONVNET(id) \
            id ##_IN_CH,    id ##_IN_DIM_X,  id ##_IN_DIM_Y,  \
            id ##_OUT_CH,   id ##_OUT_DIM_X, id ##_OUT_DIM_Y, \
            id ##_PAD_X,    id ##_PAD_Y,                      \
            id ##_STRIDE_X, id ##_STRIDE_Y,                   \
            id ##_KER_DIM_X,id ##_KER_DIM_Y

#ifndef BECO_DEBUG
#define BECO_DEBUG_PRIVATE private
#else
#define BECO_DEBUG_PRIVATE public
#endif

class Copy
{
public:
    static void copy_out(int16_t *op, const int32_t *ip, int n)
    {
        for (int i = 0; i < n; i+=2) {
            *op++ = BECO_SSAT(*ip++, 16);
            *op++ = BECO_SSAT(*ip++, 16);
        }
    }
};

class CopyReLU {
public:
    static void copy_out(int16_t *op, const int32_t *ip, int n)
    {
        for (int i = 0; i < n; i+=2) {
            *op++ = BECO_USAT(*ip++, 15);
            *op++ = BECO_USAT(*ip++, 15);
        }
    }
};


template <
    int ICH, int IW, int IH,
    int OCH, int OW, int OH,
    int PX,  int PY,
    int SX,  int SY,
    int KX,  int KY,
    typename CPY = Copy>
class Convolve {

    static_assert(OCH % 2 == 0, "Check input dimension meets the constraints.");
    static_assert(OCH == 4, "TODO: this implementation only support 4 output channels");
    static_assert(OCH*ICH % 4 ==0, "TODO: Check input channel meets the constraints.");
    static_assert(OW % 4 == 0, "Output stride must be multiple of 4");

public:

    // Return strip size in number of 16bit elements
    // Strip need 4*ICH*KX*KY*OH 16 bit elements
    static constexpr int SIMDWidth     { 4 };
    static constexpr int stripSize     { SIMDWidth * ICH * KX*KY * OW};
    static constexpr int imageSize     { ICH * IW*IH };
    static constexpr int outSize       { OCH * OW*OH };
    static constexpr int biasSize      { OCH };
    static constexpr int weightSize    { ICH * KX*KY * OCH };
    static constexpr int matmulSize    { ICH * KX * OCH };

    static constexpr int outHeight     { OH };
    static constexpr int outWidth      { OW };
    static constexpr int inChannels    { ICH };
    static constexpr int outChannels   { OCH };
    static constexpr int kernelWidth   { KX };
    static constexpr int kernelHeight  { KY };

    static constexpr int padStartX     { PX };
    static constexpr int padStartY     { PY };
    static constexpr int padEndX       { OW*SX + KX - 1 - IW - PX };
    static constexpr int padEndY       { OH*SY + KY - 1 - IH - PY };

    static_assert(PX < KX && PY < KY, "Padding must be smaller than kernel.");
    static_assert(padEndX < KX && padEndY < KY, "Padding must be smaller than kernel.");

BECO_DEBUG_PRIVATE:
    // Variables

    // FIXME: Maybe use the template parameters directly?
    q15_t    wide_bias[ outChannels * SIMDWidth ];
    q15_t    weights[ weightSize ];

    int bias_lshift, out_rshift, beco_config;
    int round;

private:

//    Convolve(const Convolve&) = delete;

    // Inline methods

    static inline void rotate_words_90(uint32_t *pout, uint32_t a, uint32_t b)
    {
            BECO_ROT(a, b, 0xffff, 16);
            pout[0] = a;
            pout[2] = b; // FIXME: SIMDWidth != 4
    }

    static inline void copy_channels(uint32_t *pout, const uint32_t * restrict pin)
    {
        if (ICH == 1 && SX == 1) {
            // require unaligned access:
            *pout++ = *pin++;    // FIXME: SIMDWidth != 4
            *pout++ = *pin++;
            return;
        }
        if (ICH == 1) {
            uint16_t *pi = (uint16_t *)pin;
            uint16_t *po = (uint16_t *)pout;
            for (int c = 0; c < SIMDWidth; pi += SX, c++)
                *po++ = *pi;
            return;
        }
        // move from HWC to strip  2x64bit (C0: X0 X1 X2 X3 \\ C1: X0 X1 X2 X3)
        for (int c = 0; c < ICH; c += 2) {
            rotate_words_90(pout + 0, pin[ICH/2*SX*0], pin[ICH/2*SX*1]);// FIXME: SIMDWidth != 4
            rotate_words_90(pout + 1, pin[ICH/2*SX*2], pin[ICH/2*SX*3]);
            pin++; // next C
            pout += 4;
        }
    }

BECO_DEBUG_PRIVATE:
    // Create IM2COL strip.
    // Output is an array of [ICH*OW*KX] elements.
    // Layout is:
    //   KX0: [ CH0: [X0 X1 X2 X3], CH1: [X0 X1 X2 X3] ...]
    static void getStrip(int y, const int16_t * restrict pim, int16_t * restrict pout)
    {
        int ky, kx;
        const int16_t * restrict p;
        int16_t * restrict po = pout;

        pim += ICH*IW*y - ICH*padStartX;

        for (int x = 0; x < OW; x+= SIMDWidth) {
            p = pim;
            for (kx = 0; kx < KX; kx++) {
                copy_channels((uint32_t*)pout, (const uint32_t *)p);
                p += ICH;
                pout += ICH*SIMDWidth;
            }
            pim += SIMDWidth*SX*ICH;
        }

        // Clear values outside input matrix
        for (int px = 0; px < padStartX; px++) {
            for (int c = 0; c < (padStartX - px)*ICH; c++)
                po[c*SIMDWidth + px] = 0;
        }
//        printf("erase x = %d \n", padEndX);    
        for (int px = 0; px < padEndX; px++) {
            for (int c = 0; c < (padEndX - px)*ICH; c++)
                pout[-(c*SIMDWidth + px) - 1] = 0;
        }
    }
    
    void deb(const int16_t *pout, int y)
    {
        int x, kx, i;
        printf("y=%d\n", y);
        for (x = 0; x < outWidth; x += SIMDWidth) {
            for (kx = 0; kx < kernelWidth; kx++) {
                for (i = 0; i < inChannels; i++) {
                    printf("ch%2d kx%2d x%2d: %2d %2d %2d %2d\n", i, kx, x, pout[0], pout[1], pout[2], pout[3]);
                    pout += 4;
                }
            }
        }
    }


    struct StripCache {
    
        int16_t *imcache[KY];
        bool     cache_valid[KY];
        int      starty;
        uint32_t im_kernel[ stripSize ];

        template <typename T> void swap(T &a, T&b) {T c; c=a; a=b; b=c; }

        void flush()
        {
            for (int i = 0; i < KY; i++) {
                imcache[i] = (int16_t*)im_kernel + i*ICH*KX*OW;
                cache_valid[i] = false;
            }
        }
        
        inline bool valid(int ky)
        {
            return (starty+ky >= 0 && cache_valid[ky]);
        }
        
        void update(int iy, const q15_t *im)
        {
            starty = iy;

            // Discard expired entries.
            if (iy != -PY) {
                for (int i = 0; i < KY-SY; i++) {
                    swap(imcache[i], imcache[i+SY]);
                    cache_valid[i] = cache_valid[i+SY];
                    cache_valid[i+SY] = false;
                }
            }

            // Load new entries.
            for  (int i = 0; i < KY; i++) {
                if (iy+i >= IH)
                    cache_valid[i] = false;
                else if (iy+i >= 0 && cache_valid[i] == false) {
                    assert (iy+i < IH);
//                    printf("  GEN strip slot:%d (y=%d)\n",i, iy+i);
                    Convolve::getStrip(iy+i, im, imcache[i]);
                    cache_valid[i] = true;
                }
                //deb(imcache[i], iy+i);
            }
        }

        inline bool get(int ky, int16_t *&col)
        {
            if (!valid(ky)) return false;
            col = imcache[ky];

            return true;
        }

    } stripcache;

public:

    void setTruncate(bool trunc)
    {
        round = (trunc)? 0: q_round(out_rshift);
    }

    int setPlan(
            const q15_t *wt,
            const q15_t *bias,
            const uint16_t _bias_lshift,
            const uint16_t _out_rshift)
    {
        assert(_bias_lshift < 16);

        // TODO: Prefer Wider than Narrow
        // TODO: Prefer x-stride = 1

        bias_lshift = _bias_lshift;
        out_rshift = _out_rshift;

        setTruncate(false);

        beco_config = BECO_CONF_BMODE_MAT | BECO_CONF_AMODE_REP32 |
                BECO_CONF_ATYPE_INT16 | BECO_CONF_BTYPE_INT16 |
                BECO_CONF_RSHIFT(out_rshift) |
                BECO_CONF_PACK_INT32 | BECO_CONF_RD_16x16_ROT90;


        for (int i = 0; i < outChannels; i++) {
            wide_bias[i*2+0] = bias[i];
            wide_bias[i*2+1] = bias[i];
            wide_bias[i*2+8] = bias[i];
            wide_bias[i*2+9] = bias[i];
        }

        // Convert layout of weights from CMSIS to Beco
        // CMSIS weights:
        //
        //  ky\kx: 0      1        KX-1
        //   0   CCC..C CCC..C ... CCC.C    <- Channel 0
        //   1   CCC..C CCC..C ... CCC.C
        //       ...
        //  KY-1 CCC..C CCC..C ... CCC.C
        //   0   CCC..C CCC..C ... CCC.C    <- Channel 1
        //  ...

        q15_t *w = weights;
        for (int ky = 0; ky < KY; ky ++)
        for (int kx = 0; kx < KX; kx ++)
        for (int ich = 0; ich < ICH; ich ++)
        for (int och = 0; och < OCH; och ++) { // FIXME: assume 4 == one 64 bit vector
            *w++ = wt[ och*ICH*KX*KY + ky*KX*ICH + kx*ICH + ich];
        }

        return BECO_SUCCESS;
    }

    void set_bias(q15_t *bias, p15_t bias_factor)
    {
        beco_preload_acc(BECO_ACC0, BECO_REG0, BECO_REG1);
        beco_preload_acc(BECO_ACC1, BECO_REG0, BECO_REG1);
        beco_preload_acc(BECO_ACC2, BECO_REG0, BECO_REG1);
        beco_preload_acc(BECO_ACC3, BECO_REG0, BECO_REG1);
        beco_add_bias_4x4_16x16(bias, bias_factor);
    }

    void matmul_tile(const uint32_t *w, const uint32_t *p, int n)
    {
        // TODO: When stride_x = 1, loop can use shift instead of load.
        while (n >= 8) {
            beco_write_reg(BECO_REG2, ((beco_vec64_in_t){.u32 = { p[0], p[1] } }));
            beco_write_reg(BECO_REG3, ((beco_vec64_in_t){.u32 = { w[0], w[1] } }));
            beco_mmacrr(BECO_REG2, BECO_REG3);

            beco_write_reg(BECO_REG2, ((beco_vec64_in_t){.u32 = { p[2], p[3] } }));
            beco_write_reg(BECO_REG3, ((beco_vec64_in_t){.u32 = { w[2], w[3] } }));
            beco_mmacrr(BECO_REG2, BECO_REG3);
            p += 4;
            w += 4;
            n -= 8;
        }
        if (n == 4) {
            beco_write_reg(BECO_REG2, ((beco_vec64_in_t){.u32 = { p[0], p[1] } }));
            beco_write_reg(BECO_REG3, ((beco_vec64_in_t){.u32 = { w[0], w[1] } }));
            beco_mmacrr(BECO_REG2, BECO_REG3);
        }
    }


public:

    void convolve(const q15_t *im, q15_t *out)
    {
        int x, y;   // coord in out[]
        int ix, iy; // coord in im[]
        const p15_t bias_factor = 1 << bias_lshift;
        beco_vec32_out_t  tile[outChannels*SIMDWidth];

        stripcache.flush();

        // Configure Beco
        beco_write_config(beco_config);

        // Set rounding
        beco_set_preload_16x16( round ); // load BECO_REG0 and BECO_REG1

        iy = - PY;
        for (y = 0; y < outHeight; y++, iy += SY) {
            stripcache.update(iy, im);

            ix = 0;
            for (x = 0; x < outWidth; x += SIMDWidth, ix += SIMDWidth*ICH*KX) {
                int16_t *w = weights;

                set_bias(wide_bias, bias_factor);

//                printf("---------\n");
                for (int ky = 0; ky < KY; ky++, w += ICH*KX*OCH) {
                    int16_t *col;
                    if (stripcache.get(ky, col)) {
//                        print_matrix(col, SIMDWidth, ICH*KX, SIMDWidth);
                        matmul_tile((const uint32_t *)w,
                                    (const uint32_t*)(col + ix), 
                                    matmulSize);
                    }
                }

                beco_read_all_16x16_32bit(tile, 4);
                CPY::copy_out(out, &tile[0].i32, 16);
                out += 16;
            }
        }
    }

};


}
#endif
