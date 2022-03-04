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
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "bt_drv.h"
#include "app_audio.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "hal_location.h"
#include "a2dp_api.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#include "a2dp_api.h"
#include "avrcp_api.h"
#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"
#include "bt_drv_interface.h"
#include "hci_api.h"

#include "a2dp_codec_sbc.h"
#include "avdtp_api.h"

btif_avdtp_codec_t a2dp_avdtpcodec;

const unsigned char a2dp_codec_elements[] = {
    A2D_SBC_IE_SAMP_FREQ_48 | A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_CH_MD_MONO | A2D_SBC_IE_CH_MD_STEREO | A2D_SBC_IE_CH_MD_JOINT | A2D_SBC_IE_CH_MD_DUAL,
    A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 | A2D_SBC_IE_BLOCKS_8  |A2D_SBC_IE_BLOCKS_4 | A2D_SBC_IE_SUBBAND_8 | A2D_SBC_IE_SUBBAND_4 | A2D_SBC_IE_ALLOC_MD_L | A2D_SBC_IE_ALLOC_MD_S,
    A2D_SBC_IE_MIN_BITPOOL,
    BTA_AV_CO_SBC_MAX_BITPOOL
};

static unsigned char a2dp_codec_elements_user_configure[] = {
    A2D_SBC_IE_SAMP_FREQ_48 | A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_CH_MD_MONO | A2D_SBC_IE_CH_MD_STEREO | A2D_SBC_IE_CH_MD_JOINT | A2D_SBC_IE_CH_MD_DUAL,
    A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 | A2D_SBC_IE_BLOCKS_8  |A2D_SBC_IE_BLOCKS_4 | A2D_SBC_IE_SUBBAND_8 | A2D_SBC_IE_SUBBAND_4 | A2D_SBC_IE_ALLOC_MD_L | A2D_SBC_IE_ALLOC_MD_S,
    A2D_SBC_IE_MIN_BITPOOL,
    BTA_AV_CO_SBC_MAX_BITPOOL
};

void a2dp_avdtpcodec_sbc_user_configure_set(uint32_t bitpool,uint8_t user_configure)
{
    int32_t lock;
    lock = int_lock();
    memcpy((uint8_t*)&a2dp_codec_elements_user_configure,(uint8_t*)a2dp_codec_elements,sizeof(a2dp_codec_elements));

    if(user_configure)
    {
        if(bitpool > BTA_AV_CO_SBC_MAX_BITPOOL)
        {
            bitpool = BTA_AV_CO_SBC_MAX_BITPOOL;
        }
        a2dp_codec_elements_user_configure[3] = bitpool;
    }
    else
    {
        a2dp_codec_elements_user_configure[3] = BTA_AV_CO_SBC_MAX_BITPOOL;
    }
    int_unlock(lock);
}

uint8_t a2dp_avdtpcodec_sbc_user_bitpool_get()
{
    return a2dp_codec_elements_user_configure[3];
}
void a2dp_codec_sbc_common_init(void)
{
    a2dp_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_SBC;
    a2dp_avdtpcodec.discoverable = 1;
    a2dp_avdtpcodec.elements = (U8 *)&a2dp_codec_elements_user_configure;
    a2dp_avdtpcodec.elemLen  = 4;
}

bt_status_t a2dp_codec_sbc_init(int index)
{
    struct BT_DEVICE_T *bt_dev = app_bt_get_device(index);
    btif_avdtp_content_prot_t *cp = NULL;

#ifdef __A2DP_AVDTP_CP__
    bt_dev->a2dp_avdtp_cp.cpType = BTIF_AVDTP_CP_TYPE_SCMS_T;
    bt_dev->a2dp_avdtp_cp.data = bt_dev->a2dp_avdtp_cp_security_data;
    bt_dev->a2dp_avdtp_cp.dataLen = 0;
    cp = &bt_dev->a2dp_avdtp_cp;
#endif /* __A2DP_AVDTP_CP__ */

    a2dp_codec_sbc_common_init();

    return btif_a2dp_register(bt_dev->btif_a2dp_stream, BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_avdtpcodec, cp, 0, a2dp_callback);
}
