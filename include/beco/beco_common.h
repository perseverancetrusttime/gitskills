#pragma once
//
// Copyright 2020 BES Technic
//
// Description: Beco coprocessor types and constants
//
#ifndef _BECO_COMMON_H
#define _BECO_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
#define BECO_C_DECLARATIONS_START extern "C" {
#define BECO_C_DECLARATIONS_END   }
#else
#define BECO_C_DECLARATIONS_START
#define BECO_C_DECLARATIONS_END
#endif

BECO_C_DECLARATIONS_START

#define _CPNUM 0

#define cr0 0

// ACC's
typedef enum {
    BECO_ACC0,
    BECO_ACC1,
    BECO_ACC2,
    BECO_ACC3
} beco_acc_t;

// ACC+LANE's
typedef enum {
    BECO_ACC0_0,
    BECO_ACC1_0,
    BECO_ACC2_0,
    BECO_ACC3_0,
    BECO_ACC0_4,
    BECO_ACC1_4,
    BECO_ACC2_4,
    BECO_ACC3_4,
    BECO_ACC0_8,
    BECO_ACC1_8,
    BECO_ACC2_8,
    BECO_ACC3_8,
    BECO_ACC0_12,
    BECO_ACC1_12,
    BECO_ACC2_12,
    BECO_ACC3_12,
} beco_acclane_t;

// REG's
typedef enum {
    BECO_REG0 = 0,
    BECO_REG1 = 2,
    BECO_REG2 = 4,
    BECO_REG3 = 6,
    BECO_REG4 = 8,
    BECO_REG5 = 10,
    BECO_REG6 = 12,
    BECO_REG7 = 14
} beco_reg_t;

// HALF REG's
typedef enum {
    BECO_REG0L,
    BECO_REG0H,
    BECO_REG1L,
    BECO_REG1H,
    BECO_REG2L,
    BECO_REG2H,
    BECO_REG3L,
    BECO_REG3H,
    BECO_REG4L,
    BECO_REG4H,
    BECO_REG5L,
    BECO_REG5H,
    BECO_REG6L,
    BECO_REG6H,
    BECO_REG7L,
    BECO_REG7H
} beco_hreg_t;

// ALUCONF

#define BECO_ALUCNF_BMODE_MAT     (0 << 0)   // B-MODE
#define BECO_ALUCNF_BMODE_FIR     (1 << 0)
#define BECO_ALUCNF_BMODE_REP64   BECO_ALUCNF_BMODE_FIR
#define BECO_ALUCNF_BMODE_REP32   BECO_ALUCNF_BMODE_MAT
#define BECO_ALUCNF_AMODE_REP32   (0 << 1)   // A-MODE
#define BECO_ALUCNF_AMODE_REP16   (2 << 1)
#define BECO_ALUCNF_AMODE_REP64   (3 << 1)
#define BECO_ALUCNF_BTYPE_UINT8   (0 << 3)   // B-TYPE
#define BECO_ALUCNF_BTYPE_UINT16  (1 << 3)
#define BECO_ALUCNF_BTYPE_INT8    (2 << 3)
#define BECO_ALUCNF_BTYPE_INT16   (3 << 3)
#define BECO_ALUCNF_ATYPE_UINT8   (0 << 5)   // A-TYPE
#define BECO_ALUCNF_ATYPE_UINT16  (1 << 5)
#define BECO_ALUCNF_ATYPE_INT8    (2 << 5)
#define BECO_ALUCNF_ATYPE_INT16   (3 << 5)
#define BECO_ALUCNF_N_SHIFT       10

// OUTCONF

#define BECO_OUTCNF_PACK_INT8     (0 << 0)   // Pack 8x8 ALU OP into v4q or v8q output
#define BECO_OUTCNF_PACK_INT16    (1 << 0)   // Pack 8x8 or 8x16 ALU OP into v2h or v4h output
#define BECO_OUTCNF_PACK_INT32    (2 << 0)   // All ALU OP into 32 bit int or v2s
#define BECO_OUTCNF_PACK_FLOAT32  (3 << 0)   // All ALU OP into 32 float or v2f
#define BECO_OUTCNF_RSHIFT(N)     (((N) & 0x1f) << 2) // Right shift
#define BECO_OUTCNF_GLOBALROTATE  (1 << 7)   // Rotate 2x2 blocks
#define BECO_OUTCNF_LOCALROTATE   (1 << 8)   // Rotate accumulators
#define BECO_OUTCNF_UNSIGNED      (1 << 9)   // Convert to unsigned
#define BECO_OUTCNF_RELU          (1 << 9)
#define BECO_OUTCNF_N_SHIFT       0

// Short hand for normal read-out order

#define BECO_OUTCNF_RD_8x8         BECO_OUTCNF_LOCALROTATE
#define BECO_OUTCNF_RD_8x8_ROT90   BECO_OUTCNF_GLOBALROTATE
#define BECO_OUTCNF_RD_8x16        0
#define BECO_OUTCNF_RD_8x16_ROT90  BECO_OUTCNF_GLOBALROTATE
#define BECO_OUTCNF_RD_16x8        BECO_OUTCNF_LOCALROTATE
#define BECO_OUTCNF_RD_16x8_ROT90  (BECO_OUTCNF_LOCALROTATE | BECO_OUTCNF_GLOBALROTATE)
#define BECO_OUTCNF_RD_16x16       0
#define BECO_OUTCNF_RD_16x16_ROT90 BECO_OUTCNF_GLOBALROTATE


// COMBINED CONF

#define BECO_CONF_BMODE_MAT     (BECO_ALUCNF_BMODE_MAT    << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BMODE_FIR     (BECO_ALUCNF_BMODE_FIR    << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BMODE_REP64   BECO_CONF_BMODE_FIR
#define BECO_CONF_BMODE_REP32   BECO_CONF_BMODE_MAT
#define BECO_CONF_AMODE_REP32   (BECO_ALUCNF_AMODE_REP32  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_AMODE_REP16   (BECO_ALUCNF_AMODE_REP16  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_AMODE_REP64   (BECO_ALUCNF_AMODE_REP64  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BTYPE_UINT8   (BECO_ALUCNF_BTYPE_UINT8  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BTYPE_UINT16  (BECO_ALUCNF_BTYPE_UINT16 << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BTYPE_INT8    (BECO_ALUCNF_BTYPE_INT8   << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_BTYPE_INT16   (BECO_ALUCNF_BTYPE_INT16  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_ATYPE_UINT8   (BECO_ALUCNF_ATYPE_UINT8  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_ATYPE_UINT16  (BECO_ALUCNF_ATYPE_UINT16 << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_ATYPE_INT8    (BECO_ALUCNF_ATYPE_INT8   << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_ATYPE_INT16   (BECO_ALUCNF_ATYPE_INT16  << BECO_ALUCNF_N_SHIFT)
#define BECO_CONF_PACK_INT8     (BECO_OUTCNF_PACK_INT8    << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_PACK_INT16    (BECO_OUTCNF_PACK_INT16   << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_PACK_INT32    (BECO_OUTCNF_PACK_INT32   << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_PACK_FLOAT32  (BECO_OUTCNF_PACK_FLOAT32 << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RSHIFT(N)     (BECO_OUTCNF_RSHIFT(N)    << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_GLOBALROTATE  (BECO_OUTCNF_GLOBALROTATE << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_LOCALROTATE   (BECO_OUTCNF_LOCALROTATE  << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_UNSIGNED      (BECO_OUTCNF_UNSIGNED     << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RELU          (BECO_OUTCNF_RELU         << BECO_OUTCNF_N_SHIFT)

// Short hand for normal read-out order

#define BECO_CONF_RD_8x8         (BECO_OUTCNF_RD_8x8         << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_8x8_ROT90   (BECO_OUTCNF_RD_8x8_ROT90   << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_8x16        (BECO_OUTCNF_RD_8x16        << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_8x16_ROT90  (BECO_OUTCNF_RD_8x16_ROT90  << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_16x8        (BECO_OUTCNF_RD_16x8        << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_16x8_ROT90  (BECO_OUTCNF_RD_16x8_ROT90  << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_16x16       (BECO_OUTCNF_RD_16x16       << BECO_OUTCNF_N_SHIFT)
#define BECO_CONF_RD_16x16_ROT90 (BECO_OUTCNF_RD_16x16_ROT90 << BECO_OUTCNF_N_SHIFT)

// Masks for type and sign

#define BECO_CONF_ATYPE_SIGNED    BECO_CONF_ATYPE_INT8  // 0b10 << ats
#define BECO_CONF_BTYPE_SIGNED    BECO_CONF_BTYPE_INT8  // 0b10 << bts
#define BECO_CONF_ATYPE_MASK      BECO_CONF_ATYPE_INT16 // 0b11 << ats
#define BECO_CONF_BTYPE_MASK      BECO_CONF_BTYPE_INT16 // 0b11 << bts

#define BECO_ACC_IS_SIGNED(conf) ((conf & BECO_CONF_ATYPE_SIGNED) || (conf & BECO_CONF_ATYPE_SIGNED))



// Error codes

#define BECO_SUCCESS         0
#define BECO_SIZE_MISMATCH   1


#ifndef restrict
#  define restrict __restrict
#endif


// Declare vector types
//     v2sf b;
//     float e, f;
//     b = (v2sf) {e, f};


typedef int8_t   v4i8   __attribute__ ((vector_size(4)));
typedef uint8_t  v4u8   __attribute__ ((vector_size(4)));
typedef int16_t  v2i16  __attribute__ ((vector_size(4)));
typedef uint16_t v2u16  __attribute__ ((vector_size(4)));

typedef int8_t   v8i8   __attribute__ ((vector_size(8)));
typedef uint8_t  v8u8   __attribute__ ((vector_size(8)));
typedef int16_t  v4i16  __attribute__ ((vector_size(8)));
typedef uint16_t v4u16  __attribute__ ((vector_size(8)));
typedef int32_t  v2i32  __attribute__ ((vector_size(8)));
typedef uint32_t v2u32  __attribute__ ((vector_size(8)));
typedef float    v2sf   __attribute__ ((vector_size(8)));

typedef union {
    v4i8     i8;
    v4u8     u8;
    v2i16    i16;
    v2u16    u16;
    float    f32;
    int32_t  i32;
    uint32_t u32;
} beco_vec32_out_t;

typedef union {
    v8i8     i8;
    v8u8     u8;
    v4i16    i16;
    v4u16    u16;
    v2i32    i32;
    v2u32    u32;
    v2sf     f32;
    uint64_t u64;
    beco_vec32_out_t v32[2];
} beco_vec64_out_t;

typedef union {
    v4i8     i8;
    v4u8     u8;
    v2i16    i16;
    v2u16    u16;
    uint32_t u32;
} beco_vec32_in_t;

typedef union {
    v8i8     i8;
    v8u8     u8;
    v4i16    i16;
    v4u16    u16;
    v2i32    i32;
    v2u32    u32;
    uint64_t u64;
    beco_vec32_in_t v32[2];
} beco_vec64_in_t;

BECO_C_DECLARATIONS_END

#endif

