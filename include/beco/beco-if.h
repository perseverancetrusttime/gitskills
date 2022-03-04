#pragma once
//
// Copyright 2020 BES Technic
//
// Description: beco coprocessor instructions for C
//

#ifndef _BECO_IF_H
#define _BECO_IF_H

#include "beco_common.h"

BECO_C_DECLARATIONS_START

#define BECO_PRE_V1 1
#include <arm_acle.h>

// OPCODES

/*
**Instruction**  |**TYPE**| **OPC1/2**  |**Format**|**Cycles**
-----------------|--------|-------------|----------|----------
READ_CPID        | MRC    | 6'b000/000  | fmt8     | 1
READ_STATUS      | MRC    | 6'b000/001  | fmt8     | 1
READ_ACC         | MRC    | 6'b000/010  | fmt9     | 1
READ_NEXT_ACC    | MRC    | 6'b000/100  | fmt9     | 1
READ_CONFIG      | MRC    | 6'b000/011  | fmt8     | 1
READ_REG         | MRRC   | 4'b0000/    | fmt10    | 1
WRITE_CONFIG     | MCR    | 6'b000/000  | fmt7     | 1
EXEC_MMACGR      | MCR    | 6'b000/001  | fmt11    | 1
MACRO_MMACGR4    | MCR    | 6'b000/010  | fmt11    | 4 *
EXEC_MMAC        | MCRR   | 4'b0000/    | fmt6     | 1
WRITE_REG        | MCRR   | 4'b0001/    | fmt5     | 1
WRITE_OUTCONFIG  | CDP    | 7'b0000/000 | fmt3     | 1
WRITE_ALUCONFIG  | CDP    | 7'b0000/001 | fmt3     | 1
CLEAR_ACC        | CDP    | 7'b0000/010 | fmt2     | 1
SET_ACC_BIAS     | CDP    | 7'b0010/010 | fmt2     | 1
PERLOAD_ACC      | CDP    | 7'b0010/011 | fmt2     | 1
MOVE_ACC_TO_REG  | CDP    | 7'b0000/011 | fmt4     | 1
EXEC_MMACR       | CDP    | 7'b0000/110 | fmt2     | 1
MOVE32           | CDP    | 7'b0000/111 | fmt1     | 1
BSHIFT           | CDP    | 7'b0001/??? | fmt1     | 1
MACRO_MMACRR     | CDP    | 7'b0010/000 | fmt2     | 4
MACRO_MOVEACC    | CDP    | 7'b0010/001 | fmt4     | 2
MACRO_BSHIFT5    | CDP    | 7'b0011/??? | fmt1     | 5
*/

#define OPC_RDCPID  0,0   // MRR
#define OPC_RDSTAT  0,1   // MRR
#define OPC_RDACC   0,2   // MRR
#define OPC_RDCONF  0,3   // MRR
#define OPC_RDNXACC 0,4   // MRR
#define OPC_WRCONF  0,0   // MCR
#define OPC_MMACGR  0,1   // MCR
#define OPC_MMACGR4 0,2   // MCR
#define OPC_MMAC    0     // MCRR
#define OPC_WRREG   1     // MCRR
#define OPC_RDREG   0     // MRRC

#define OPC_SETOCONF 0,0   // CDP
#define OPC_SETACONF 0,1   // CDP
#define OPC_CLRACC   0,2   // CDP
#define OPC_MATOR32  0,3   // CDP
#define OPC_MMACR    0,6   // CDP
#define OPC_MRTOR32  0,7   // CDP
#define OPC_MMACRR   2,0   // CDP
#define OPC_MATOR64  2,1   // CDP
#define OPC_SETBIAS  2,2   // CDP
#define OPC_PSETACC  2,3   // CDP
#define OPC_BSHIFT(N) 1,N  // CDP
#define OPC_BSHIFT5(N) 3,N // CDP


// HELP MACROS

#define BECO_I12_TO_CP_DMN(a) \
    ((a) >> 4) & 0xf,         \
    (a) & 0xf,                \
    ((a) >> 8) & 0xf


#define CDP(ncp, opc1, opc2, crd, crm, crn)     \
        __arm_cdp(ncp, opc1, crd, crn, crm, opc2)
#define MCR(ncp, opc1, opc2, regin, crm, crn)   \
        __arm_mcr(ncp, opc1, (uint32_t)regin, crn, crm, opc2)
#define MRC(ncp, opc1, opc2, crm, crn)          \
        __arm_mrc(ncp, opc1, crn, crm, opc2)

#define BECO_CDP(opc, cpd, cpm, cpn)            \
        CDP(_CPNUM, opc, cpd, cpm, cpn)

#define BECO_CDP_IMM(opc, imm12)                \
        CDP(_CPNUM, opc, imm12)

#define BECO_MCR(opc, rt, cpm, cpn)             \
        MCR(_CPNUM, opc, rt, cpm, cpn);

#define BECO_MRC(opc, cpm, cpn)                 \
        MRC(_CPNUM, opc, cpm, cpn);

#define BECO_MRRC_R(opc, cpm)                   \
        __arm_mrrc(_CPNUM, opc, cpm)

#define BECO_MCRR_R(ncp, opc1, reg, crm)        \
        __arm_mcrr(ncp, opc1, reg, crm)



#define BECO_MCRR_RR(ncp, opc1, rt, rt2, crm)  \
	__asm volatile ("mcrr %c0, %1, %2, %3, cr%c4"  \
	     :: "n"(ncp),"n"(opc1), "r"(rt),"r"(rt2), "n"(crm)  );

// beco_cpid
//
static inline uint32_t beco_cpid(void)
{
    return (uint32_t)BECO_MRC(OPC_RDCPID, cr0, cr0);
}


// beco_status
//
// Read status
static inline uint32_t beco_status(void)
{
    return (uint32_t)BECO_MRC(OPC_RDSTAT, cr0, cr0);
}


// beco_config
//
// Read config register
static inline uint32_t beco_config(void)
{
    return (uint32_t)BECO_MRC(OPC_RDCONF, cr0, cr0);
}


// beco_read_acc
//
// Return : 4x8, 2x16, 32 or float32 accumulator
//
static inline beco_vec32_out_t beco_read_acc(const beco_acc_t acc, const uint8_t lane)
{
    beco_vec32_out_t r;
    r.u32 = (uint32_t)BECO_MRC(OPC_RDACC, acc, lane & 0xf);
    return r;
}


// beco_next_read_acc
//
// Return : 4x8, 2x16, 32 or float32 accumulator
//
static inline beco_vec32_out_t beco_read_next_acc(const beco_acc_t acc, const uint8_t lane)
{
    beco_vec32_out_t r;
    r.u32 = (uint32_t)BECO_MRC(OPC_RDNXACC, acc, lane & 0xf);
    return r;
}

// beco_read_reg
//
// Return : cp register contents.
//
static inline beco_vec64_out_t  beco_read_reg(const beco_reg_t cp_src)
{
    beco_vec64_out_t r;
    r.u64 = BECO_MRRC_R(OPC_RDREG, cp_src);
    return r;
}

// beco_write_config
//
// Write configuration register
static inline void beco_write_config(int32_t config)
{
    BECO_MCR(OPC_WRCONF, config, cr0, cr0);
}


// beco_write_reg
//
static inline void beco_write_reg(const beco_reg_t cp_dst, beco_vec64_in_t v64)
{
    BECO_MCRR_R(_CPNUM, OPC_WRREG, v64.u64, cp_dst);
}


// beco_set_outconfig
//
// Set output format and packing
static inline void beco_set_outconfig(const uint16_t outconfig)
{
    BECO_CDP_IMM(OPC_SETOCONF, BECO_I12_TO_CP_DMN(outconfig) );
}


// beco_set_aluconfig
//
// Set input type and matrix/broadcast mode
static inline void beco_set_aluconfig(const uint16_t aluconfig)
{
    BECO_CDP_IMM(OPC_SETACONF, BECO_I12_TO_CP_DMN(aluconfig) );
}


// beco_move32
//
static inline void beco_move32(const beco_hreg_t cp_dst, const beco_hreg_t cp_src)
{
    BECO_CDP(OPC_MRTOR32, cp_dst, cp_src, 0);
}

// beco_bshift
//
static inline void beco_bshift(
    const beco_reg_t cp_dst,
    const beco_reg_t cp_hi, const beco_reg_t cp_lo, const uint8_t N)
{
    BECO_CDP(OPC_BSHIFT(N), cp_dst, cp_lo, cp_hi);
}

#define REGNEXT(x, n) (const beco_reg_t)((int)x + 2*n)
#define HREGNEXT(x, n) (const beco_hreg_t)((int)x + n)

// beco_bshift5
//
// For normal case cp_hi = cp_lo+1, cp_dst=cp_lo
static inline void beco_bshift5(
    const beco_reg_t cp_dst,
    const beco_reg_t cp_hi, const beco_reg_t cp_lo, const uint8_t N)
{
#if BECO_PRE_V1
	beco_bshift(REGNEXT(cp_dst, 0), REGNEXT(cp_hi, 0), REGNEXT(cp_lo, 0), N);
	beco_bshift(REGNEXT(cp_dst, 1), REGNEXT(cp_hi, 1), REGNEXT(cp_lo, 1), N);
	beco_bshift(REGNEXT(cp_dst, 2), REGNEXT(cp_hi, 2), REGNEXT(cp_lo, 2), N);
	beco_bshift(REGNEXT(cp_dst, 3), REGNEXT(cp_hi, 3), REGNEXT(cp_lo, 3), N);
	beco_bshift(REGNEXT(cp_dst, 4), REGNEXT(cp_hi, 4), REGNEXT(cp_lo, 4), N);
#else
    BECO_CDP(OPC_BSHIFT5(N), cp_dst, cp_lo, cp_hi);
#endif
}

// beco_move (using bshift with shift=0)
//
static inline void beco_move(
    const beco_reg_t cp_dst, const beco_reg_t cp_lo)
{
    beco_bshift(cp_dst, cp_lo, cp_lo, 0);
}

static inline void beco_shift_block5(
    const beco_reg_t cp_dst, const uint8_t N)
{
    const beco_reg_t cp_p1 = (beco_reg_t)((int)cp_dst+2);
    beco_bshift5(cp_dst, cp_p1, cp_dst, N);
}



// ACCUMULATOR ACCESS

// beco_clear_acc
//
// Clear all accumulators in acc
static inline void beco_clear_acc(const beco_acc_t acc)
{
    BECO_CDP(OPC_CLRACC, acc, 0, 0);
}


// beco_preload_acc
//
// Preload accumulators in acc (like clear_acc)
static inline void beco_preload_acc(
    const beco_acc_t acc,
    const beco_reg_t cp_hi, const beco_reg_t cp_lo)
{
    BECO_CDP(OPC_PSETACC, acc, cp_hi, cp_lo);
}


// beco_set_acc_bias
//
// Set bias of single accumulator in acc
static inline void beco_set_acc_bias(
    const beco_acclane_t acc,
    const beco_reg_t cp_hi, const beco_reg_t cp_lo)
{
    BECO_CDP(OPC_SETBIAS, acc, cp_hi, cp_lo);
}


// beco_move_acc32
//
// Compute output and move 32 bit vector to respective register half
static inline void beco_move_acc32(
    const beco_hreg_t cp_dst, const beco_acc_t acc, const uint8_t lane)
{
    BECO_CDP(OPC_MATOR32, cp_dst, acc, lane);
}


// beco_move_acc64
//
// Compute output and move two 32 bit vectors to  64 bit register
static inline void beco_move_acc64(
    const beco_reg_t cp_dst, const beco_acc_t acc, const uint8_t lane)
{
    BECO_CDP(OPC_MATOR64, cp_dst, acc, lane);
}



// MULTIPLY ACCUMULATE

// beco_mmac
//
static inline void beco_mmacgr(
    const beco_acc_t acc, beco_vec32_in_t v32_a, const beco_reg_t cp_v64_b)
{
    BECO_MCR(OPC_MMACGR, v32_a.u32, acc, cp_v64_b);
}

// beco_mmacr
//
static inline void beco_mmacgr4( beco_vec32_in_t v32_a, const beco_reg_t cp_v64_b)
{
#if BECO_PRE_V1
	beco_mmacgr(BECO_ACC0, v32_a, REGNEXT(cp_v64_b, 0));
	beco_mmacgr(BECO_ACC1, v32_a, REGNEXT(cp_v64_b, 1));
	beco_mmacgr(BECO_ACC2, v32_a, REGNEXT(cp_v64_b, 2));
	beco_mmacgr(BECO_ACC3, v32_a, REGNEXT(cp_v64_b, 3));
#else
    BECO_MCR(OPC_MMACGR4, v32_a.u32,  BECO_ACC0, cp_v64_b);
#endif
}

// beco_mmacrr
//
static inline void beco_mmac(
    const beco_acc_t acc, beco_vec32_in_t v32_a, beco_vec32_in_t v32_b)
{
    BECO_MCRR_RR(_CPNUM, OPC_MMAC, v32_a.u32, v32_b.u32, acc);
}

// beco_mmacgr
//
static inline void beco_mmacr(
    const beco_acc_t acc, const beco_hreg_t cp_vec32_a, const beco_hreg_t cp_vec32_b)
{
    BECO_CDP(OPC_MMACR, acc, cp_vec32_a, cp_vec32_b);
}

// beco_mmacgr4
//
static inline void beco_mmacrr(const beco_reg_t cp_vec64_a, const beco_reg_t cp_vec64_b)
{
#if BECO_PRE_V1
    beco_mmacr(BECO_ACC0, HREGNEXT(cp_vec64_a, 0), HREGNEXT(cp_vec64_b, 0));
    beco_mmacr(BECO_ACC1, HREGNEXT(cp_vec64_a, 0), HREGNEXT(cp_vec64_b, 1));
    beco_mmacr(BECO_ACC2, HREGNEXT(cp_vec64_a, 1), HREGNEXT(cp_vec64_b, 0));
    beco_mmacr(BECO_ACC3, HREGNEXT(cp_vec64_a, 1), HREGNEXT(cp_vec64_b, 1));
#undef ENUMNEXT
#else
    BECO_CDP(OPC_MMACRR, BECO_ACC0, cp_vec64_a, cp_vec64_b);
#endif
}

int beco_init(void);
int beco_exit(void);

#define BECO_INIT()         beco_init()
#define BECO_EXIT(ret)      beco_exit()

BECO_C_DECLARATIONS_END

#endif /*_BECO_IF_H*/
