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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "anc_assist_thread.h"

#ifndef ANC_ASSIST_THREAD_STACK_SIZE
#define ANC_ASSIST_THREAD_STACK_SIZE 3072
#endif

static ANC_ASSIST_MOD_HANDLER_T mod_handler[ANC_ASSIST_MODUAL_NUM];

static void anc_assist_thread(void const *argument);
osThreadDef(anc_assist_thread, osPriorityNormal, 1, ANC_ASSIST_THREAD_STACK_SIZE, "anc_assist_thread");

osMailQDef (anc_assist_mailbox, ANC_ASSIST_MAILBOX_MAX, ANC_ASSIST_MESSAGE_BLOCK);
static osMailQId anc_assist_mailbox = NULL;
static uint8_t anc_assist_mailbox_cnt = 0;
osThreadId anc_assist_thread_tid;

static int anc_assist_mailbox_init(void)
{
    anc_assist_mailbox = osMailCreate(osMailQ(anc_assist_mailbox), NULL);
    if (anc_assist_mailbox == NULL)  {
        TRACE(0,"Failed to Create anc_assist_mailbox\n");
        return -1;
    }
    anc_assist_mailbox_cnt = 0;
    return 0;
}

int anc_assist_mailbox_put(ANC_ASSIST_MESSAGE_BLOCK* msg_src)
{
    osStatus status;

    ANC_ASSIST_MESSAGE_BLOCK *msg_p = NULL;

    msg_p = (ANC_ASSIST_MESSAGE_BLOCK*)osMailAlloc(anc_assist_mailbox, 0);

    if (!msg_p){
        osEvent evt;
        TRACE_IMM(0,"osMailAlloc error dump");
        for (uint8_t i=0; i<ANC_ASSIST_MAILBOX_MAX; i++){
            evt = osMailGet(anc_assist_mailbox, 0);
            if (evt.status == osEventMail) {
                TRACE_IMM(9,"cnt:%d mod:%d src:%08x tim:%d id:%8x ptr:%08x para:%08x/%08x/%08x", 
                                                                       i,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->mod_id,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->src_thread,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->system_time,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->msg_body.message_id,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->msg_body.message_ptr,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->msg_body.message_Param0,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->msg_body.message_Param1,
                                                                       ((ANC_ASSIST_MESSAGE_BLOCK *)(evt.value.p))->msg_body.message_Param2);
            }else{                
                TRACE_IMM(2,"cnt:%d %d", i, evt.status); 
                break;
            }
        }
        TRACE_IMM(0,"osMailAlloc error dump end");
    }
    
    ASSERT(msg_p, "osMailAlloc error");
    msg_p->src_thread = (uint32_t)osThreadGetId();
    msg_p->dest_thread = (uint32_t)NULL;
    msg_p->system_time = hal_sys_timer_get();
    msg_p->mod_id = msg_src->mod_id;
    msg_p->msg_body.message_id = msg_src->msg_body.message_id;
    msg_p->msg_body.message_ptr = msg_src->msg_body.message_ptr;
    msg_p->msg_body.message_Param0 = msg_src->msg_body.message_Param0;
    msg_p->msg_body.message_Param1 = msg_src->msg_body.message_Param1;
    msg_p->msg_body.message_Param2 = msg_src->msg_body.message_Param2;

    status = osMailPut(anc_assist_mailbox, msg_p);
    if (osOK == status)
        anc_assist_mailbox_cnt++;
    return (int)status;
}

int anc_assist_mailbox_free(ANC_ASSIST_MESSAGE_BLOCK* msg_p)
{
    osStatus status;

    status = osMailFree(anc_assist_mailbox, msg_p);
    if (osOK == status)
        anc_assist_mailbox_cnt--;

    return (int)status;
}

int anc_assist_mailbox_get(ANC_ASSIST_MESSAGE_BLOCK** msg_p)
{
    osEvent evt;
    evt = osMailGet(anc_assist_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (ANC_ASSIST_MESSAGE_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

static void anc_assist_thread(void const *argument)
{
    while(1){
        ANC_ASSIST_MESSAGE_BLOCK *msg_p = NULL;

        if (!anc_assist_mailbox_get(&msg_p)) {
            if (msg_p->mod_id < ANC_ASSIST_MODUAL_NUM) {
                if (mod_handler[msg_p->mod_id]) {
                    int ret = mod_handler[msg_p->mod_id](&(msg_p->msg_body));
                    if (ret)
                        TRACE(2,"mod_handler[%d] ret=%d", msg_p->mod_id, ret);
                }
            }
            anc_assist_mailbox_free(msg_p);
        }
    }
}

int anc_assist_thread_init(void)
{
    if (anc_assist_mailbox_init())
        return -1;

    anc_assist_thread_tid = osThreadCreate(osThread(anc_assist_thread), NULL);
    if (anc_assist_thread_tid == NULL)  {
        TRACE(0,"Failed to Create anc_assist_thread\n");
        return 0;
    }
    return 0;
}

int anc_assist_set_threadhandle(enum ANC_ASSIST_MODUAL_ID_T mod_id, ANC_ASSIST_MOD_HANDLER_T handler)
{
    if (mod_id>=ANC_ASSIST_MODUAL_NUM)
        return -1;

    mod_handler[mod_id] = handler;
    return 0;
}

void * anc_assist_thread_tid_get(void)
{
    return (void *)anc_assist_thread_tid;
}

bool anc_assist_is_module_registered(enum ANC_ASSIST_MODUAL_ID_T mod_id)
{
    return mod_handler[mod_id];
}

