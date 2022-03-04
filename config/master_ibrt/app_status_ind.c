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
#include "stdbool.h"
#include "hal_trace.h"
#include "app_pwl.h"
#include "app_status_ind.h"
#include "string.h"

static APP_STATUS_INDICATION_T app_status = APP_STATUS_INDICATION_NUM;
#if defined(__XSPACE_UI__)
static APP_STATUS_INDICATION_T app_status_ind_filter = APP_STATUS_INDICATION_NUM;
#endif
const char *app_status_indication_str[] = {
    "[POWERON]",
    "[INITIAL]",
    "[PAGESCAN]",
    "[POWEROFF]",
    "[CHARGENEED]",
    "[CHARGING]",
    "[FULLCHARGE]",
    /* repeatable status: */
    "[BOTHSCAN]",
    "[CONNECTING]",
    "[CONNECTED]",
    "[DISCONNECTED]",
    "[CALLNUMBER]",
    "[INCOMINGCALL]",
    "[PAIRSUCCEED]",
    "[PAIRFAIL]",
    "[HANGUPCALL]",
    "[REFUSECALL]",
    "[ANSWERCALL]",
    "[CLEARSUCCEED]",
    "[CLEARFAIL]",
    "[WARNING]",
    "[ALEXA_START]",
    "[ALEXA_STOP]",
    "[GSOUND_MIC_OPEN]",
    "[GSOUND_MIC_CLOSE]",
    "[GSOUND_NC]",
    "[INVALID]",
    "[MUTE]",
    "[TESTMODE]",
    "[TESTMODE1]",
    "[RING_WARNING]",   //30

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
    "[PRESSURE_DOWN]",
    "[PRESSURE_UP]",
#endif

    "[FIND_MY_BUDS]",
    "[TILE_FIND]",   //35

/**add by lmzhang,20201221*/
#if defined(__XSPACE_CUSTOMIZE_ANC__)
//TODO(Mike):define your customize anc 
#endif
    "[CONNECTED_2]",
    "[DISCONNECTED_2]",
/**add by lmzhang,20201221*/
#if defined(__XSPACE_UI__)
    "[TWS_PAIRING]",
    "[SINGLE_PAIRING]",
    "[FREEMAN_PAIRING_MASTER]",
    "[FREEMAN_PAIRING_SLAVE]",
    "[TWSRECONN_SUCCESS]",
    "[TWSRECONN_FAILED]",
    "[RESET_FACTORY]",
#endif
};

const char *status2str(uint16_t status)
{
    const char *str = NULL;

    if (status >= 0 && status < APP_STATUS_INDICATION_NUM) {
        str = app_status_indication_str[status];
    } else {
        str = "[UNKNOWN]";
    }

    return str;
}

int app_status_indication_filter_set(APP_STATUS_INDICATION_T status)
{
#if defined(__XSPACE_UI__)
    app_status_ind_filter = status;
#endif
    return 0;
}

APP_STATUS_INDICATION_T app_status_indication_get(void)
{
    return app_status;
}

int app_status_indication_set(APP_STATUS_INDICATION_T status)
{
#if defined(__XSPACE_UI__)
    struct APP_PWL_CFG_T cfg0;
    struct APP_PWL_CFG_T cfg1;
    struct APP_PWL_CFG_T cfg2;

    TRACE(3, "%s %d%s", __func__, status, status2str((uint16_t)status));
    if (app_status == status)
        return 0;

    if (app_status_ind_filter == status)
        return 0;

    app_status = status;
    memset(&cfg0, 0, sizeof(struct APP_PWL_CFG_T));
    memset(&cfg1, 0, sizeof(struct APP_PWL_CFG_T));
    memset(&cfg2, 0, sizeof(struct APP_PWL_CFG_T));

    switch (status) {
        case APP_STATUS_INDICATION_POWEROFF:
        
            break;
        case APP_STATUS_INDICATION_SINGLE_PAIRING:

            break;

        case APP_STATUS_INDICATION_CONNECTED:

            break;

        case APP_STATUS_INDICATION_TWS_PAIRING:
        case APP_STATUS_INDICATION_FREEMAN_PAIRING_MASTER:

            break;

        case APP_STATUS_INDICATION_FREEMAN_PAIRING_SLAVE:

            break;

        case APP_STATUS_INDICATION_POWERON:
        case APP_STATUS_INDICATION_TWSRECONN_SUCCESS:

            break;

        case APP_STATUS_INDICATION_TWSRECONN_FAILED:

            break;

        case APP_STATUS_INDICATION_RESET_FACTORY:

            break;

        default:
            break;
    }
#endif
    return 0;
}
