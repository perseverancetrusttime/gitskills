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
#ifndef __APP_A2DP_H__
#define __APP_A2DP_H__
#include "btapp.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_A2DP_VOL   (127)

void a2dp_init(void);

bool a2dp_is_music_ongoing(void);

bool app_bt_is_a2dp_disconnected(uint8_t device_id);

void app_avrcp_get_capabilities_start(int device_id);

void app_a2dp_bt_driver_callback(uint8_t device_id, btif_a2dp_event_t event);

void app_bt_a2dp_disable_aac_codec(bool disable);

void app_bt_a2dp_disable_vendor_codec(bool disable);

bool app_bt_a2dp_send_volume_change(int device_id);

bool app_bt_a2dp_report_current_volume(int device_id);

void btapp_a2dp_report_speak_gain(void);

void app_bt_a2dp_reconfig_to_sbc(a2dp_stream_t *stream);

uint8_t a2dp_convert_local_vol_to_bt_vol(uint8_t localVol);

#if defined(A2DP_LDAC_ON)
void app_ibrt_restore_ldac_info(uint8_t device_id, uint8_t sample_freq);
#endif

#ifdef __TENCENT_VOICE__
uint8_t avrcp_get_media_status(void);
void avrcp_set_media_status(uint8_t status);
#endif

#if defined(__FORCE_UPDATE_A2DP_VOL__)
void btapp_a2dp_vol_force_changed_d0(void);
void btapp_a2dp_vol_force_changed_d1(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
