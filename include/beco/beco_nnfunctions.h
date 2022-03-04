#pragma once
//
// Copyright 2020 BES Technic
//
/* ----------------------------------------------------------------------
 * Project:      BECO NN Library
 * Title:        beco_nnfunctions.h
 * Description:  Public header file for BECO NN Library
 *
 * $Date:        March 27, 2021
 * $Revision:    V.1.0
 *
 * Target Processor:  beco coprocessor
 * -------------------------------------------------------------------- */


#ifndef _BECO_BIAS_PRELOAD_H
#define _BECO_BIAS_PRELOAD_H

#include "beco_common.h"
#include "beco_types.h"

#include <stddef.h>

BECO_C_DECLARATIONS_START

void naive_fully_connected_q15_T(const q15_t * pV,
                       const q15_t * pM,
                       const q15_t * bias,
                       const uint16_t dim_vec,
                       const uint16_t num_of_column,
                       const uint16_t bias_shift,
                       const uint16_t out_shift,
                       q15_t * pOut);

void beco_fully_connected_q15_T(const q15_t * pV,
                       const q15_t * pM,
                       const q15_t * bias,
                       const uint16_t dim_vec,
                       const uint16_t num_of_column,
                       const uint16_t bias_shift,
                       const uint16_t out_shift,
                       q15_t * pOut);

void naive_fully_connected_mat_q7_vec_q15(const q15_t * pV,
                       const q7_t * pM,
                       const q7_t * bias,
                       const uint16_t dim_vec,
                       const uint16_t num_of_column,
                       const uint16_t bias_shift,
                       const uint16_t out_shift,
                       q15_t * pOut);

void beco_fully_connected_mat_q7_vec_q15(const q15_t * pV,
                       const q7_t * pM,
                       const q7_t * bias,
                       const uint16_t dim_vec,
                       const uint16_t num_of_column,
                       const uint16_t bias_shift,
                       const uint16_t out_shift,
                       q15_t * pOut);

BECO_C_DECLARATIONS_END


#endif


