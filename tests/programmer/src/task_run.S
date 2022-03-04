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

    .syntax unified

    .extern cur_task_id
    .extern task_sp

    .text
    .align    2
    .thumb_func
    .globl    task_run
    .type    task_run, %function
/* void task_run(uint32_t task_id) */
task_run:

    push {r4-r12,lr}
    ldr r1, =cur_task_id
    ldr r2, [r1]
    str r0, [r1]
#ifdef __ARM_ARCH_8M_MAIN__
    ldr r3, =task_sp_lim
	ldr r1, [r3, r0, lsl #2]
	msr msplim, r1
#endif
    ldr r3, =task_sp
    str sp, [r3, r2, lsl #2]
    ldr sp, [r3, r0, lsl #2]
    isb
    pop {r4-r12,lr}
    bx lr

    .pool
    .size    task_run, . - task_run

    .end
