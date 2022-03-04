#pragma once
//
// Copyright 2020 BES Technic
//
// Description: Beco matrix multiply
//

#ifndef _BECO_MATRIX_MULT_ATB_H
#define _BECO_MATRIX_MULT_ATB_H

#include "beco_common.h"
#include "beco_read.h"


// Create a template to automatically insert correct optimized read-out function.
template <size_t a_elem_sz, size_t b_elem_sz, size_t c_elem_sz>
    void beco_read_tile64(beco_vec32_out_t out[], size_t stride);

#define BECO_GEN_TILE64_READ(a,b,c, read_fn) \
	template <> inline                 \
	void beco_read_tile64<a,b,c>(beco_vec32_out_t out[], size_t stride) { \
	    read_fn(out, stride);                                    \
	}

// Instantiate legal variations
//  Element Size:  A   B   C
BECO_GEN_TILE64_READ(1,  1,  1, beco_read_all_8x8_8bit)
BECO_GEN_TILE64_READ(1,  1,  2, beco_read_all_8x8_16bit)
BECO_GEN_TILE64_READ(1,  1,  4, beco_read_all_8x8_32bit)
BECO_GEN_TILE64_READ(1,  2,  2, beco_read_all_8x16_16bit)
BECO_GEN_TILE64_READ(1,  2,  4, beco_read_all_8x16_32bit)
BECO_GEN_TILE64_READ(2,  1,  2, beco_read_all_16x8_16bit)
BECO_GEN_TILE64_READ(2,  1,  4, beco_read_all_16x8_32bit)
BECO_GEN_TILE64_READ(2,  2,  4, beco_read_all_16x16_32bit)


#ifndef MMAC_64x64
#define MMAC_64x64(a, b) \
        beco_write_reg(BECO_REG0, a);      \
        beco_write_reg(BECO_REG1, b);      \
        beco_mmacrr(BECO_REG0, BECO_REG1);
#endif

#define MMACSTEP_64x64_S_S(stride_a, stride_b) {  \
        beco_vec64_in_t a, b;              \
        a = *p_a;                          \
        b = *p_b;                          \
        p_a += stride_a;                   \
        p_b += stride_b;                   \
        MMAC_64x64(a, b);                  \
    }

// matrix-multiply
// Inputs A and B, 64bit elements. A is transposed, B is in natural order
void mmac_tile_aTb(const beco_vec64_in_t *p_a, const beco_vec64_in_t *p_b,
               size_t stride_a, size_t stride_b, int n)
{
    int i;

    i = n & 7;
    n &= ~7;
#pragma GCC unroll 8
#pragma GCC ivdep
    for (; n != 0; n--) {
        MMACSTEP_64x64_S_S(stride_a, stride_b)
    }
    for (; i != 0; i--) {
        MMACSTEP_64x64_S_S(stride_a, stride_b)
    }
}


void mmac_tile_set_offset_zero(void)
{
    beco_clear_acc(BECO_ACC0);
    beco_clear_acc(BECO_ACC1);
    beco_clear_acc(BECO_ACC2);
    beco_clear_acc(BECO_ACC3);
}


// Calculate:
//   C[M][N] = (A[M][K])^T * B[K][N] = At[K][M] * B[K][N]
//
// Output
//    8 bit
//   16 bit
//   32 bit
template <typename Ta, typename Tb, typename Tc>
void beco_mmult_aTb(const beco_vec64_in_t *At, const beco_vec64_in_t *B,
                    beco_vec32_out_t *C,
                    int M, int N, int K,
                    size_t stride_a, size_t stride_b, size_t stride_c)
{
    beco_vec32_out_t *p_c;
    const beco_vec64_in_t *p_at = At,
                          *p_b  = B;
    const size_t a_elem_sz = sizeof(Ta);
    const size_t b_elem_sz = sizeof(Tb);
    const size_t c_elem_sz = sizeof(Tc);
    
    const size_t a_elem_per_vec64 = sizeof(beco_vec64_in_t)/a_elem_sz;
    const size_t b_elem_per_vec64 = sizeof(beco_vec64_in_t)/b_elem_sz;
    const size_t c_elem_per_vec32 = sizeof(beco_vec32_out_t)/c_elem_sz;

    // assert stride_* is divideable by 64bits
    // assert M*a_elem_sz >= stride_a
    // assert N*b_elem_sz >= stride_b
    // assert N*c_elem_sz >= stride_c
    stride_a = stride_a / sizeof(beco_vec64_in_t);
    stride_b = stride_b / sizeof(beco_vec64_in_t);
    stride_c = stride_c / sizeof(beco_vec32_out_t);    

    // Calculate output increment:
    //   for example: 8 elements output per tile, 1 elements per vec32_out:
    size_t tile_w = b_elem_per_vec64 / c_elem_per_vec32;

    // Calculate output tile h increment:
    //   for example: 8 lines of elements output per tile, 'stride_c' vec32_out's per line:
	size_t tile_h = a_elem_per_vec64 * stride_c;

    for (unsigned i = 0; i < stride_a; i++) {
        p_c = C;
        for (unsigned j = 0; j < stride_b; j++) {

            mmac_tile_set_offset_zero();
            mmac_tile_aTb(p_at + i, p_b + j, stride_a, stride_b, K);

            beco_read_tile64<a_elem_sz, b_elem_sz, c_elem_sz>(p_c, stride_c);
#if BECO_DEBUG
            printf("tile (%d,%d)\n", j, i);
            print_matrix((Tc *)p_c, b_elem_per_vec64, a_elem_per_vec64, N);
#endif
            p_c += tile_w;
        }

        // Increment C to point to next tile
        C += tile_h;
    }
}



// Create a template to automatically generate input type configuration
template <typename aType, typename bType> void beco_set_alu_types(int type_mask);

#define BECO_GEN_SET_ALUCONFIG(a,b, read_fn) \
	template <> inline                       \
	void beco_set_alu_types<a,b>(int type_mask) { \
    beco_set_aluconfig(BECO_ALUCNF_BMODE_MAT | BECO_ALUCNF_AMODE_REP32 | type_mask);\
	}

// Instantiate legal variations

BECO_GEN_SET_ALUCONFIG(uint8_t,  uint8_t, BECO_ALUCNF_ATYPE_UINT8  | BECO_ALUCNF_BTYPE_UINT8)
BECO_GEN_SET_ALUCONFIG(uint8_t,   int8_t, BECO_ALUCNF_ATYPE_UINT8  | BECO_ALUCNF_BTYPE_INT8)
BECO_GEN_SET_ALUCONFIG( int8_t,  uint8_t, BECO_ALUCNF_ATYPE_INT8   | BECO_ALUCNF_BTYPE_UINT8)
BECO_GEN_SET_ALUCONFIG( int8_t,   int8_t, BECO_ALUCNF_ATYPE_INT8   | BECO_ALUCNF_BTYPE_INT8)

BECO_GEN_SET_ALUCONFIG(uint16_t, uint8_t, BECO_ALUCNF_ATYPE_UINT16 | BECO_ALUCNF_BTYPE_UINT8)
BECO_GEN_SET_ALUCONFIG(uint16_t,  int8_t, BECO_ALUCNF_ATYPE_UINT16 | BECO_ALUCNF_BTYPE_INT8)
BECO_GEN_SET_ALUCONFIG( int16_t, uint8_t, BECO_ALUCNF_ATYPE_INT16  | BECO_ALUCNF_BTYPE_UINT8)
BECO_GEN_SET_ALUCONFIG( int16_t,  int8_t, BECO_ALUCNF_ATYPE_INT16  | BECO_ALUCNF_BTYPE_INT8)

BECO_GEN_SET_ALUCONFIG(uint8_t, uint16_t, BECO_ALUCNF_ATYPE_UINT8  | BECO_ALUCNF_BTYPE_UINT16)
BECO_GEN_SET_ALUCONFIG(uint8_t,  int16_t, BECO_ALUCNF_ATYPE_UINT8  | BECO_ALUCNF_BTYPE_INT16)
BECO_GEN_SET_ALUCONFIG( int8_t, uint16_t, BECO_ALUCNF_ATYPE_INT8   | BECO_ALUCNF_BTYPE_UINT16)
BECO_GEN_SET_ALUCONFIG( int8_t,  int16_t, BECO_ALUCNF_ATYPE_INT8   | BECO_ALUCNF_BTYPE_INT16)

BECO_GEN_SET_ALUCONFIG(uint16_t, uint16_t,BECO_ALUCNF_ATYPE_UINT16 | BECO_ALUCNF_BTYPE_UINT16)
BECO_GEN_SET_ALUCONFIG(uint16_t,  int16_t,BECO_ALUCNF_ATYPE_UINT16 | BECO_ALUCNF_BTYPE_INT16)
BECO_GEN_SET_ALUCONFIG( int16_t, uint16_t,BECO_ALUCNF_ATYPE_INT16  | BECO_ALUCNF_BTYPE_UINT16)
BECO_GEN_SET_ALUCONFIG( int16_t,  int16_t,BECO_ALUCNF_ATYPE_INT16  | BECO_ALUCNF_BTYPE_INT16)


template <typename aType>  constexpr auto atype_mask = BECO_ALUCNF_ATYPE_INT8;
template <>  constexpr auto atype_mask<int8_t>   = BECO_ALUCNF_ATYPE_INT8;
template <>  constexpr auto atype_mask<uint8_t>  = BECO_ALUCNF_ATYPE_UINT8;
template <>  constexpr auto atype_mask<int16_t>  = BECO_ALUCNF_ATYPE_INT16;
template <>  constexpr auto atype_mask<uint16_t> = BECO_ALUCNF_ATYPE_UINT16;

template <typename bType>  constexpr auto btype_mask = BECO_ALUCNF_BTYPE_INT8;
template <>  constexpr auto btype_mask<int8_t>   = BECO_ALUCNF_BTYPE_INT8;
template <>  constexpr auto btype_mask<uint8_t>  = BECO_ALUCNF_BTYPE_UINT8;
template <>  constexpr auto btype_mask<int16_t>  = BECO_ALUCNF_BTYPE_INT16;
template <>  constexpr auto btype_mask<uint16_t> = BECO_ALUCNF_BTYPE_UINT16;

template <typename cType>  constexpr auto ctype_mask = BECO_OUTCNF_PACK_INT8;
template <>  constexpr auto ctype_mask<int8_t>   = BECO_OUTCNF_PACK_INT8;
template <>  constexpr auto ctype_mask<uint8_t>  = BECO_OUTCNF_PACK_INT8;
template <>  constexpr auto ctype_mask<int16_t>  = BECO_OUTCNF_PACK_INT16;
template <>  constexpr auto ctype_mask<uint16_t> = BECO_OUTCNF_PACK_INT16;
template <>  constexpr auto ctype_mask<int32_t>  = BECO_OUTCNF_PACK_INT32;
template <>  constexpr auto ctype_mask<uint32_t> = BECO_OUTCNF_PACK_INT32;
template <>  constexpr auto ctype_mask<float>    = BECO_OUTCNF_PACK_FLOAT32;

template <typename aType, typename bType>  constexpr auto localrotate_mask = 0;
template <>  constexpr auto localrotate_mask<int16_t,int8_t>   = BECO_OUTCNF_LOCALROTATE;
template <>  constexpr auto localrotate_mask<uint16_t,int8_t>  = BECO_OUTCNF_LOCALROTATE;
template <>  constexpr auto localrotate_mask<int16_t,uint8_t>  = BECO_OUTCNF_LOCALROTATE;
template <>  constexpr auto localrotate_mask<uint16_t,uint8_t> = BECO_OUTCNF_LOCALROTATE;

template <typename aType, typename bType, typename cType>
	static inline void beco_set_matrix_mult_config(int scale)
{
	uint32_t config;

    config = (BECO_ALUCNF_BMODE_MAT     |
                BECO_ALUCNF_AMODE_REP32 |
                atype_mask<aType>       |
                btype_mask<bType> ) << 10;
	config |= BECO_OUTCNF_RSHIFT(scale)  |
	            BECO_OUTCNF_GLOBALROTATE |
                // BECO_OUTCNF_RELU      |
	            ctype_mask<cType>        |
                localrotate_mask<aType, bType>;
    // printf("%x / %x\n", config >> 10,  config & 0x3ff);
	beco_write_config(config);
}

template <typename aType, typename bType, typename cType>
     void beco_matrix_mult_aTb(const aType *At, const bType *B, cType *C,
                               int M, int N, int K,
                               size_t stride_a, size_t stride_b, size_t stride_c)
{
    beco_vec64_in_t *pA = (beco_vec64_in_t*)At,
                    *pB = (beco_vec64_in_t*)B;
    beco_vec32_out_t *pCb = (beco_vec32_out_t*)C;

    beco_set_matrix_mult_config<aType, bType, cType>(0);

    beco_mmult_aTb<aType, bType, cType>(pA, pB, pCb,
                                        M, N, K,
                                        stride_a, stride_b, stride_c);
}

template <typename aType, typename bType, typename cType>
     void beco_matrix_mult_aTb(const aType *At, const bType *B, cType *C,
                               int M, int N, int K)
{
    beco_vec64_in_t *pA = (beco_vec64_in_t*)At,
                    *pB = (beco_vec64_in_t*)B;
    beco_vec32_out_t *pCb = (beco_vec32_out_t*)C;

    beco_set_matrix_mult_config<aType, bType, cType>(0);

    beco_mmult_aTb<aType, bType, cType>(pA, pB, pCb,
                                        M, N, K,
                                        M*sizeof(*At), N*sizeof(*B), M*sizeof(*C));
}


#endif

