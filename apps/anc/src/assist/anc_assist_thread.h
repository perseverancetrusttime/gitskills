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
#ifndef __ANC_ASSIST_THREAD_H__
#define __ANC_ASSIST_THREAD_H__

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ANC_ASSIST_MAILBOX_MAX (20)

enum ANC_ASSIST_MODUAL_ID_T {
    ANC_ASSIST_MODUAL_VOICE_ASSIST = 0,

    ANC_ASSIST_MODUAL_NUM
};

typedef struct {
    uint32_t message_id;
    uint32_t message_ptr;
    uint32_t message_Param0;
    uint32_t message_Param1;
    uint32_t message_Param2;
} ANC_ASSIST_MESSAGE_BODY;

typedef struct {
    uint32_t src_thread;
    uint32_t dest_thread;
    uint32_t system_time;
    uint32_t mod_id;
    ANC_ASSIST_MESSAGE_BODY msg_body;
} ANC_ASSIST_MESSAGE_BLOCK;

typedef int (*ANC_ASSIST_MOD_HANDLER_T)(ANC_ASSIST_MESSAGE_BODY *);

int anc_assist_mailbox_put(ANC_ASSIST_MESSAGE_BLOCK* msg_src);

int anc_assist_mailbox_free(ANC_ASSIST_MESSAGE_BLOCK* msg_p);

int anc_assist_mailbox_get(ANC_ASSIST_MESSAGE_BLOCK** msg_p);

int anc_assist_thread_init(void);

int anc_assist_set_threadhandle(enum ANC_ASSIST_MODUAL_ID_T mod_id, ANC_ASSIST_MOD_HANDLER_T handler);

void * anc_assist_thread_tid_get(void);

bool anc_assist_is_module_registered(enum ANC_ASSIST_MODUAL_ID_T mod_id);

#ifdef __cplusplus
    }
#endif

#endif
