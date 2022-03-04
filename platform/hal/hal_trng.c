/***************************************************************************
 *
 * Copyright 2015-2019 BES.
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
#ifdef CHIP_HAS_TRNG
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "plat_addr_map.h"
#include "reg_trng.h"
#include "hal_trng.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_cmu.h"
#include "string.h"

static struct TRNG_T * const trng = (struct TRNG_T *)TRNG_BASE;

#define MIN_SAMPLE_CNT1 17
int hal_trng_open(const struct HAL_TRNG_CFG_T *cfg)
{
    uint32_t sample_cntr1;
    if (cfg == NULL) {
        return -1;
    }

    if (cfg->sample_cntr1 < MIN_SAMPLE_CNT1)
        sample_cntr1 = MIN_SAMPLE_CNT1;
    else
        sample_cntr1 = cfg->sample_cntr1;

    hal_cmu_clock_enable(HAL_CMU_MOD_P_TRNG);
    hal_cmu_reset_clear(HAL_CMU_MOD_P_TRNG);

    trng->TRNG_CONFIG = cfg->rnd_src_sel & RND_SRC_SEL_MASK;
    trng->SAMPLE_CNT1 = sample_cntr1;

    trng->RNG_IMR = 0xF;

    return 0;
}

void hal_trng_start()
{
    trng->RND_SOURCE_ENABLE = 0;
    trng->RNG_ICR = 0xF; //clear int status
    trng->RST_BITS_COUNTER = 1; // reset the bits counter and TRNG valid registers
    trng->RND_SOURCE_ENABLE = RND_SOURCE_EN;
}

void hal_trng_stop()
{
    trng->RND_SOURCE_ENABLE = 0;
}

void hal_trng_close(void)
{
    trng->RND_SOURCE_ENABLE = 0;
    trng->RST_BITS_COUNTER = 1; // reset the bits counter and TRNG valid registers

    hal_cmu_reset_set(HAL_CMU_MOD_P_TRNG);
    hal_cmu_clock_disable(HAL_CMU_MOD_P_TRNG);

    return;
}

int hal_get_trngdata(uint8_t *buf, uint32_t buf_len)
{
    uint32_t i;
    uint32_t len, read_len;
    uint32_t ehr_data;
    uint32_t err_cnt, timeout_cnt;

    read_len = 0;
    err_cnt = 0;
    while (read_len < buf_len) {
        hal_trng_start();

        timeout_cnt = 0;
        while ((trng->RNG_ISR & 0xF) == 0) {
            if (++timeout_cnt > 100) {
                return -2; // timer out
            }
            osDelay(1);
        };
        if (trng->RNG_ISR & 0xE) { //error
            if (++err_cnt > 100) {
                return -1;
            }
            osDelay(1);
            continue;
        }
        err_cnt = 0;

        if (read_len + 24 > buf_len)
            len = buf_len - read_len;
        else
            len = 24;
        for (i=0; i<len/4; i++) {
            *(uint32_t *)(buf+read_len+i*4) = trng->EHR_DATA[i];
        }
        if (len % 4) {
            ehr_data = trng->EHR_DATA[i];
            memcpy(buf+read_len+i*4, (uint8_t *)&ehr_data, len % 4);
        }
        read_len += len;
    }
    hal_trng_stop();
    return 0;
}

#endif // CHIP_HAS_TRNG
