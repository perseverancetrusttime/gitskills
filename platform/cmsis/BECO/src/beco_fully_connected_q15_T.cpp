/* ----------------------------------------------------------------------
 * Project:      BECO NN Library
 * Title:        beco_fully_connected_q15.c
 * Description:  Q15 basic fully-connected layer function
 *
 * $Date:        17. March 2021
 * $Revision:    V.1.0.0
 * $Copyright    2021 BES Technic
 *
 * Target Processor:  Cortex-M cores
 *
 * -------------------------------------------------------------------- */
#include "beco.h"
#include "beco_l1.h"
#include "beco_types.h"
#include "beco_bias.hpp"

#include "beco_nnfunctions.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define elem_per_calculate 4 //sizeof(beco_vec64_in_t)/sizeof(*q15);
#define elem_per_calculate_4ACC 16 //sizeof(beco_vec64_in_t)/sizeof(*q15)*4;
  /**
   * @brief Q15 basic fully-connected layer function
   * @param[in]       pV          pointer to input vector
   * @param[in]       pM          pointer to matrix weights which is transposed
   * @param[in]       dim_vec     length of the vector
   * @param[in]       num_of_rows number of rows in weight matrix
   * @param[in]       bias_shift  amount of left-shift for bias
   * @param[in]       out_shift   amount of right-shift for output
   * @param[in]       bias        pointer to bias
   * @param[in,out]   pOut        pointer to output vector
   *
   * @example
   *
   *                 [b0 ,b1 ,b2 ,b3 ]
   * [a0,a1,a2,a3] * [b4 ,b5 ,b6 ,b7 ] = [a0*b0+a1*b4+a2*b8+a3*b12,...]
   *                 [b8 ,b9 ,b10,b11]
   *                 [b12,b13,b14,b15]
   *
   */

void naive_fully_connected_q15_T(const q15_t * pV,
                       const q15_t * pM,
                       const q15_t * bias,

                       const uint16_t dim_vec,
                       const uint16_t num_of_column,
                       const uint16_t bias_shift,
                       const uint16_t out_shift,

                       q15_t * pOut)
{
    int i, j;

    /* Run the following code as reference implementation */
    for (i = 0; i < num_of_column; i++)
    {
        q31_t ip_out = ((q31_t)(bias[i]) << bias_shift) + q_round(out_shift);
        for (j = 0; j < dim_vec; j++)
        {
            ip_out += pV[j] * pM[j * num_of_column + i];
        }
        pOut[i] = (q15_t) q15_sat(ip_out >> out_shift);
    }
}

//read-out function
static inline void beco_read_16x16_T(int16_t *out)
{
    beco_read_acc(BECO_ACC0, -4); // reset beco lane pointer
    for (int i = 0; i < elem_per_calculate; i++) {
        out[i] = beco_read_next_acc(BECO_ACC0, 4).i32; // update pointer
    }
}

static inline void beco_read_16x16_4ACC_T(int16_t *out)
{
    beco_read_acc(BECO_ACC0, -4); // reset beco lane pointer
    out[0] = beco_read_next_acc(BECO_ACC0, 4).i32; // update pointer
    out[4] = beco_read_next_acc(BECO_ACC1, 0).i32;
    out[8] = beco_read_next_acc(BECO_ACC2, 0).i32;
    out[12] = beco_read_next_acc(BECO_ACC3, 0).i32;
    out[1] = beco_read_next_acc(BECO_ACC0, 4).i32; // update pointer
    out[5] = beco_read_next_acc(BECO_ACC1, 0).i32;
    out[9] = beco_read_next_acc(BECO_ACC2, 0).i32;
    out[13] = beco_read_next_acc(BECO_ACC3, 0).i32;
    out[2] = beco_read_next_acc(BECO_ACC0, 4).i32; // update pointer
    out[6] = beco_read_next_acc(BECO_ACC1, 0).i32;
    out[10] = beco_read_next_acc(BECO_ACC2, 0).i32;
    out[14] = beco_read_next_acc(BECO_ACC3, 0).i32;
    out[3] = beco_read_next_acc(BECO_ACC0, 4).i32; // update pointer
    out[7] = beco_read_next_acc(BECO_ACC1, 0).i32;
    out[11] = beco_read_next_acc(BECO_ACC2, 0).i32;
    out[15] = beco_read_next_acc(BECO_ACC3, 0).i32;

}

static inline void beco_add_bias_16x32_4ACC(const beco_vec64_in_t *bias, p31_t shift)
{
    beco_write_reg(BECO_REG0, bias[0]);
    beco_write_reg(BECO_REG1, bias[1]);
    beco_write_reg(BECO_REG2, bias[2]);
    beco_write_reg(BECO_REG3, bias[3]);
    beco_mmacgr4(((beco_vec32_in_t){.u32 = shift}), BECO_REG0);
}

static void beco_fully_connected_remained_impl(
                const q15_t *p_v, const q15_t *p_w,
                uint32_t loop_index, const uint16_t dim_vec, const uint16_t num_of_column)
{
    q15_t p_wr[elem_per_calculate] = {0};
    uint32_t stride_loop = elem_per_calculate * loop_index;

    for (uint32_t i = 0; i < dim_vec; i++) {
        uint32_t stride_w = i * num_of_column + stride_loop;
        for(uint32_t j = 0; j < elem_per_calculate; j++) {
            p_wr[j] = p_w[stride_w + j]; // fill_matrix_tile
        }
        uint32_t pv_32 = *p_v++;
        beco_vec64_in_t *beco_wr = (beco_vec64_in_t *)p_wr;

        beco_write_reg(BECO_REG0, beco_wr[0]);
        beco_mmacgr(BECO_ACC0, ((beco_vec32_in_t){.u32 = pv_32}), BECO_REG0);
    }
}

// fully_connected_impl
//case1: if num_of_column is the multiple of sixteen, use BECO_ACC0~BECO_ACC4
static inline void beco_fully_connected_T_msixteen_impl(
                const q15_t *p_v, const beco_vec64_in_t *beco_w,
                const uint16_t dim_vec, const uint32_t column_for_64)
{

    for (int i = 0; i < dim_vec; i++){
        uint32_t pv_32 = *p_v++;
        //beco_vec64_in_t *beco_w = (beco_vec64_in_t *)p_w;

        beco_write_reg(BECO_REG0, beco_w[0]);
        beco_write_reg(BECO_REG1, beco_w[1]);
        beco_write_reg(BECO_REG2, beco_w[2]);
        beco_write_reg(BECO_REG3, beco_w[3]);
        //beco_mmacgr4(((beco_vec32_in_t){.i16 = {p_v[i], 0}}), BECO_REG0);
        beco_mmacgr4(((beco_vec32_in_t){.u32 = pv_32}), BECO_REG0);
        beco_w += column_for_64;
    }
}

//case2: if num_of_column is the multiple of four, only use BECO_ACC0
static inline void beco_fully_connected_T_mfour_impl(
                const q15_t *p_v, const beco_vec64_in_t *beco_w,
                const uint16_t dim_vec, const uint16_t column_for_64) // if num_of_column is the multiple of 4
{
    for (int i = 0; i < dim_vec; i++){
        uint32_t pv_32 = *p_v++;
        //beco_vec64_in_t *beco_w = (beco_vec64_in_t *)p_w;

        beco_write_reg(BECO_REG0, beco_w[0]);
        beco_mmacgr(BECO_ACC0, ((beco_vec32_in_t){.u32 = pv_32}), BECO_REG0);

        beco_w += column_for_64;
    }
}

//case3: if num_of_column is not the multiple of four, use four 16 bit p_w to make a 64 bit beco_w.
//       it is slower than Pointer
static inline void beco_fully_connected_T_nofour_impl(
                const q15_t *p_v, const q15_t *p_w,
                const uint16_t dim_vec, const uint16_t num_of_column) // if num_of_column is not the multiple of 4
{
    for (int i = 0; i < dim_vec; i++){
        uint32_t pv_32 = *p_v++;
        const beco_vec64_in_t beco_w = (const beco_vec64_in_t){.i16={p_w[0],p_w[1],p_w[2],p_w[3]}};

        beco_write_reg(BECO_REG0, beco_w);
        beco_mmacgr(BECO_ACC0, ((beco_vec32_in_t){.u32 = pv_32}), BECO_REG0);

        p_w += num_of_column;
    }
}

static void beco_fully_connected_T(const q15_t * pV,
                        const q15_t * pM,
                        const q15_t * bias,

                        const uint16_t dim_vec,
                        const uint16_t num_of_column,
                        const uint16_t bias_shift,
                        const uint16_t out_shift,

                        q15_t * pOut)
{
    unsigned j;
    //const beco_vec64_in_t *p_w   = (const beco_vec64_in_t *) pM;
    //constexpr size_t w_elem_sz = sizeof(*pM);
    //constexpr size_t w_elem_per_vec = sizeof(beco_vec64_in_t)/w_elem_sz;
    const uint32_t loopcnt_4 = num_of_column / elem_per_calculate;
    const uint32_t remainder_4 = num_of_column % elem_per_calculate;
    const uint32_t loopcnt_16 = num_of_column / elem_per_calculate_4ACC;
    const uint32_t remainder_16 = num_of_column % elem_per_calculate_4ACC;

    const uint32_t bias_factor = 1 << bias_shift;

    if(likely(0 == remainder_16)) {
        const beco_vec64_in_t *beco_bias = (const beco_vec64_in_t *) bias;
        const beco_vec64_in_t *beco_w = (const beco_vec64_in_t *) pM;
        for (j = 0; j < loopcnt_16; j++) {
            beco_clear_acc(BECO_ACC0);       // clear accumulators
            beco_clear_acc(BECO_ACC1);
            beco_clear_acc(BECO_ACC2);
            beco_clear_acc(BECO_ACC3);

            beco_set_preload_16x16_4ACC(q_round(out_shift));
            beco_add_bias_16x32_4ACC(beco_bias, bias_factor);

            beco_fully_connected_T_msixteen_impl(pV, beco_w + j*4, dim_vec, loopcnt_4);
            beco_read_16x16_4ACC_T(pOut);

            pOut += 16;
            beco_bias += 4;
        }
    } else if(0 == remainder_4){

        const beco_vec64_in_t *beco_w = (const beco_vec64_in_t *) pM;
        for (j = 0; j < loopcnt_4; j++) {
            beco_clear_acc(BECO_ACC0);

            beco_set_preload_16x16(q_round(out_shift));
            beco_add_bias_16x32(bias, bias_factor);

            beco_fully_connected_T_mfour_impl(pV, beco_w + j, dim_vec, loopcnt_4);
            beco_read_16x16_T(pOut);

            pOut += elem_per_calculate;
            bias += elem_per_calculate;
        }
    } else {
        for (j = 0; j < loopcnt_4; j++) {
            beco_clear_acc(BECO_ACC0);

            beco_set_preload_16x16(q_round(out_shift));
            beco_add_bias_16x32(bias, bias_factor);

            beco_fully_connected_T_nofour_impl(pV, pM + j*elem_per_calculate, dim_vec, num_of_column);
            beco_read_16x16_T(pOut);

            pOut += elem_per_calculate;
            bias += elem_per_calculate;
        }

        //fill the matrix to the multiple of 4
        q15_t p_out[elem_per_calculate] = {0};

        beco_clear_acc(BECO_ACC0);

        beco_set_preload_16x16(q_round(out_shift));
        beco_add_bias_16x32(bias, bias_factor);

        beco_fully_connected_remained_impl(pV, pM, j, dim_vec, num_of_column);
        beco_read_16x16_T(p_out);

        for(uint16_t i = 0; i < remainder_4; i++) {
            pOut[i] = p_out[i];
        }
    }
}

void beco_fully_connected_q15_T(const q15_t * pV,
                        const q15_t * pM,
                        const q15_t * bias,

                        const uint16_t dim_vec,
                        const uint16_t num_of_column,
                        const uint16_t bias_shift,
                        const uint16_t out_shift,

                        q15_t * pOut)
{
    // Configure:
    // set input and output configuration, multiply as integers,
    // readout as 16 bits

    uint32_t config0;
    config0 =
        BECO_CONF_BMODE_REP64 | BECO_CONF_AMODE_REP16 |
        BECO_CONF_ATYPE_INT16 | BECO_CONF_BTYPE_INT16 |
        BECO_CONF_RSHIFT(out_shift) |
        BECO_CONF_PACK_INT16 | BECO_CONF_RD_16x16;
    beco_write_config(config0);

    beco_fully_connected_T(pV, pM, bias, dim_vec, num_of_column, bias_shift, out_shift, pOut);

}


