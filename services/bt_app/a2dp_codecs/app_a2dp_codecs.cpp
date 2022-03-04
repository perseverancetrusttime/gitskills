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
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"
#include "bt_drv_interface.h"
#include "hci_api.h"

#define _FILE_TAG_ "A2DP"
#include "color_log.h"
#include "app_bt_func.h"
#include "os_api.h"

#if (A2DP_DECODER_VER >= 2)
#include "a2dp_decoder.h"
#endif

#include "btapp.h"
#include "app_a2dp_codecs.h"

btif_avdtp_codec_t * app_a2dp_codec_get_avdtp_codec()
{
    return (btif_avdtp_codec_t *)&a2dp_avdtpcodec;
}

#if defined(IBRT)
enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate);
uint32_t app_a2dp_codec_parse_aac_sample_rate(const a2dp_callback_parms_t *info)
{
    uint32_t ret_sample_rate = AUD_SAMPRATE_44100;
    btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *) info;

    if (p_info->p.configReq->codec.elements[1] & A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100)
    {
        ret_sample_rate = AUD_SAMPRATE_44100;
    }
    else if (p_info->p.configReq->codec.elements[2] & A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000)
    {
        ret_sample_rate = AUD_SAMPRATE_48000;
    }
    return ret_sample_rate;
}
uint32_t app_a2dp_codec_parse_aac_lhdc_sample_rate(const a2dp_callback_parms_t * info)
{
    btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *) info;

    switch (A2DP_LHDC_SR_DATA(p_info->p.configReq->codec.elements[6]))
    {
        case A2DP_LHDC_SR_96000:
            return AUD_SAMPRATE_96000;
        case A2DP_LHDC_SR_48000:
            return AUD_SAMPRATE_48000;
        case A2DP_LHDC_SR_44100:
            return AUD_SAMPRATE_44100;
        default:
            return AUD_SAMPRATE_44100;
    }
}
uint32_t app_a2dp_codec_get_sample_rate(const a2dp_callback_parms_t *info)
{
    btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *) info;
    btif_avdtp_codec_type_t codetype = btif_a2dp_get_codec_type((const a2dp_callback_parms_t *)p_info);

    switch(codetype)
    {
        case BTIF_AVDTP_CODEC_TYPE_SBC:
            return bt_parse_sbc_sample_rate(p_info->p.configReq->codec.elements[0]);
        case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
            return app_a2dp_codec_parse_aac_sample_rate((const a2dp_callback_parms_t *)p_info);
        case BTIF_AVDTP_CODEC_TYPE_LHDC:
            return app_a2dp_codec_parse_aac_lhdc_sample_rate((const a2dp_callback_parms_t *)p_info);
        default:
            ASSERT(0,"btif_a2dp_get_sample_rate codetype error!!");
            return 0;
    }
}
uint8_t app_a2dp_codec_get_sample_bit(const a2dp_callback_parms_t *info)
{
    btif_avdtp_codec_type_t codetype = btif_a2dp_get_codec_type(info);
    btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *) info;

    if ((codetype == BTIF_AVDTP_CODEC_TYPE_LHDC) &&
        A2DP_LHDC_FMT_DATA(p_info->p.configReq->codec.elements[6]))
    {
        return 24;
    }
    else
    {
        //AAC and SBC sample bit eq 16
        return 16;
    }
}
#endif
