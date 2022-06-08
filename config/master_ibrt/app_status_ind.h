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
#ifndef __APP_STATUS_IND_H__
#define __APP_STATUS_IND_H__

#ifdef RTOS
#include "cmsis_os.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__XSPACE_UI__)
#define LED_G   APP_PWL_ID_0
#define LED_B   APP_PWL_ID_1
#define LED_R   APP_PWL_ID_2
#endif

typedef enum APP_STATUS_INDICATION_T {
    APP_STATUS_INDICATION_POWERON = 0,
    APP_STATUS_INDICATION_INITIAL,
    APP_STATUS_INDICATION_PAGESCAN,
    APP_STATUS_INDICATION_POWEROFF,
    APP_STATUS_INDICATION_CHARGENEED,
    APP_STATUS_INDICATION_CHARGING,
    APP_STATUS_INDICATION_FULLCHARGE,
    APP_STATUS_INDICATION_NO_REPEAT_NUM,
    /* repeatable status: */
    APP_STATUS_INDICATION_BOTHSCAN = APP_STATUS_INDICATION_NO_REPEAT_NUM,
    APP_STATUS_INDICATION_CONNECTING,
    APP_STATUS_INDICATION_CONNECTED,
    APP_STATUS_INDICATION_DISCONNECTED,
    APP_STATUS_INDICATION_CALLNUMBER,
    APP_STATUS_INDICATION_INCOMINGCALL,
    APP_STATUS_INDICATION_PAIRSUCCEED,
    APP_STATUS_INDICATION_PAIRFAIL,
    APP_STATUS_INDICATION_HANGUPCALL,
    APP_STATUS_INDICATION_REFUSECALL,
    APP_STATUS_INDICATION_ANSWERCALL,
    APP_STATUS_INDICATION_CLEARSUCCEED,
    APP_STATUS_INDICATION_CLEARFAIL,
    APP_STATUS_INDICATION_WARNING,
    APP_STATUS_INDICATION_ALEXA_START,
    APP_STATUS_INDICATION_ALEXA_STOP,
    APP_STATUS_INDICATION_GSOUND_MIC_OPEN,
    APP_STATUS_INDICATION_GSOUND_MIC_CLOSE,
    APP_STATUS_INDICATION_GSOUND_NC,
    APP_STATUS_INDICATION_INVALID,
    APP_STATUS_INDICATION_MUTE,
    APP_STATUS_INDICATION_TESTMODE,
    APP_STATUS_INDICATION_TESTMODE1,
    APP_STATUS_RING_WARNING,

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
    APP_STATUS_RING_PRESSURE_DOWN,
    APP_STATUS_RING_PRESSURE_UP,
#endif
    APP_STATUS_INDICATION_FIND_MY_BUDS,
    APP_STATUS_INDICATION_TILE_FIND,   //35

#if defined(__XSPACE_CUSTOMIZE_ANC__)
//TODO(Mike):define your customize anc 
#endif
    APP_STATUS_INDICATION_CONNECTED_2,
    APP_STATUS_INDICATION_DISCONNECTED_2,

#if defined(__XSPACE_UI__)
    APP_STATUS_INDICATION_TWS_PAIRING,   //45
    APP_STATUS_INDICATION_SINGLE_PAIRING,
    APP_STATUS_INDICATION_FREEMAN_PAIRING_MASTER,
    APP_STATUS_INDICATION_FREEMAN_PAIRING_SLAVE,
    APP_STATUS_INDICATION_TWSRECONN_SUCCESS,
    APP_STATUS_INDICATION_TWSRECONN_FAILED,
    APP_STATUS_INDICATION_RESET_FACTORY,
#endif

#ifdef jinyao_learning//add:jinyao 2022.5.30
    APP_STATUS_INDICATION_PROMPT_WELCOME,//自制的提示音
#endif

    APP_STATUS_INDICATION_NUM
} APP_STATUS_INDICATION_T;

const char *status2str(uint16_t status);
int app_status_indication_filter_set(APP_STATUS_INDICATION_T status);
APP_STATUS_INDICATION_T app_status_indication_get(void);
int app_status_indication_set(APP_STATUS_INDICATION_T status);

#ifdef __cplusplus
}
#endif

#endif
