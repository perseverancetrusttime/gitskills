#pragma once
//
// Copyright 2020 BES Technic
//
// Description: beco coprocessor instructions bindings for verilator sim
//

#ifndef _BECO_SIM_IF_H
#define _BECO_SIM_IF_H

#include "beco_common.h"

BECO_C_DECLARATIONS_START


// beco_cpid
//
uint32_t beco_cpid(void);


// beco_status
//
// Read status
uint32_t beco_status(void);


// beco_config
//
// Read config register
uint32_t beco_config(void);


// beco_read_acc
//
// Return : 4x8, 2x16, 32 or float32 accumulator
//
beco_vec32_out_t   beco_read_acc(beco_acc_t acc, uint8_t lane);

// beco_next_read_acc
//
// Return : 4x8, 2x16, 32 or float32 accumulator
//
beco_vec32_out_t   beco_read_next_acc(beco_acc_t acc, uint8_t lane);


// beco_read_reg
//
// Return : cp register contents.
//
beco_vec64_out_t  beco_read_reg(beco_reg_t cp_src);


// beco_write_config
//
// Write configuration register
void beco_write_config(int32_t config);


// beco_write_reg
//
void beco_write_reg(beco_reg_t cp_dst, beco_vec64_in_t  v64);


// beco_set_outconfig
//
// Set output format and packing
void beco_set_outconfig(uint16_t outconfig);


// beco_set_aluconfig
//
// Set input type and matrix/broadcast mode
void beco_set_aluconfig(uint16_t aluconfig);


// beco_move32
//
void beco_move32(beco_hreg_t cp_dst, beco_hreg_t cp_src);


// beco_bshift
//
void beco_bshift(beco_reg_t cp_dst, beco_reg_t cp_hi, beco_reg_t cp_lo, uint8_t N);

// beco_bshift5
//
// For normal case cp_hi = cp_lo+1, cp_dst=cp_lo
void beco_bshift5(beco_reg_t cp_dst, beco_reg_t cp_hi, beco_reg_t cp_lo, uint8_t N);

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
void beco_clear_acc(beco_acc_t acc);


// beco_preload_acc
//
// Preload accumulators in acc (like clear_acc)
void beco_preload_acc(beco_acc_t acc, beco_reg_t cp_hi, beco_reg_t cp_lo);


// beco_set_acc_bias
//
// Set bias of single accumulator in acc
void beco_set_acc_bias(beco_acclane_t acc, beco_reg_t cp_hi, beco_reg_t cp_lo);


// beco_move_acc32
//
// Compute output and move 32 bit vector to respective register half
void beco_move_acc32(beco_hreg_t cp_dst, beco_acc_t acc, uint8_t lane);


// beco_move_acc64
//
// Compute output and move two 32 bit vectors to  64 bit register
void beco_move_acc64(beco_reg_t cp_dst, beco_acc_t acc, uint8_t lane);



// MULTIPLY ACCUMULATE

// beco_mmac
//
void beco_mmac(beco_acc_t acc, beco_vec32_in_t v32_a, beco_vec32_in_t v32_b);


// beco_mmacr
//
void beco_mmacr(beco_acc_t acc, beco_hreg_t cp_vec32_a, beco_hreg_t cp_vec32_b);


// beco_mmacrr
//
void beco_mmacrr(beco_reg_t cp_vec64_a, beco_reg_t cp_vec64_b);


// beco_mmacgr
//
void beco_mmacgr(beco_acc_t acc, beco_vec32_in_t v32_a, beco_reg_t cp_v64_b);


// beco_mmacgr4
//
void beco_mmacgr4(beco_vec32_in_t v32_a, beco_reg_t cp_v64_b);



// SIMULATOR HOOK

int beco_init(int argc, char** argv, char** env);
int beco_exit(int ret);

#define BECO_INIT() beco_init(argc, argv, env)
#define BECO_EXIT(ret) beco_exit(ret)

BECO_C_DECLARATIONS_END

#endif /*_BECO_SIM_IF_H*/

