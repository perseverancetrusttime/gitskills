/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#if defined(IBRT)
#include "btapp.h"
#include "app_bt.h"
#include "audio_policy.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_tws_ext_cmd.h"

#define APP_TWS_EXT_CMD_A2DP_PLAYING_DEVICE (APP_TWS_BESAPP_EXT_CMD_PREFIX + 1)

typedef struct
{
    app_tws_ext_cmd_head_t head;
    bt_bdaddr_t playing_device_addr;
} app_tws_ext_cmd_a2dp_playing_device_t;

void app_ibrt_send_ext_cmd_a2dp_playing_device(uint8_t curr_playing_device, bool is_response)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(curr_playing_device);
    app_tws_ext_cmd_a2dp_playing_device_t cmd;
    if (curr_device)
    {
        cmd.playing_device_addr = curr_device->remote;
        if (is_response)
        {
            app_ibrt_tws_send_ext_cmd_rsp(APP_TWS_EXT_CMD_A2DP_PLAYING_DEVICE, &cmd.head, sizeof(app_tws_ext_cmd_a2dp_playing_device_t));
        }
        else
        {
            app_ibrt_tws_send_ext_cmd(APP_TWS_EXT_CMD_A2DP_PLAYING_DEVICE, &cmd.head, sizeof(app_tws_ext_cmd_a2dp_playing_device_t));
        }
    }
}

static void app_ibrt_ext_cmd_a2dp_playing_device_handler(bool is_response, app_tws_ext_cmd_head_t *header, uint32_t length)
{
    app_tws_ext_cmd_a2dp_playing_device_t *cmd = (app_tws_ext_cmd_a2dp_playing_device_t *)header;
    uint8_t device_id = app_bt_get_device_id_byaddr(&cmd->playing_device_addr);
    if (device_id != BT_DEVICE_INVALID_ID)
    {
        app_bt_audio_receive_peer_a2dp_playing_device(is_response, device_id);
    }
}

static const app_tws_ext_cmd_handler_t g_app_ibrt_tws_ext_cmd_table[] =
{
    {
        APP_TWS_EXT_CMD_A2DP_PLAYING_DEVICE,
        "A2DP_PLAYING_DEVICE",
        app_ibrt_ext_cmd_a2dp_playing_device_handler,
    },
};

void app_ibrt_tws_ext_cmd_init(void)
{
    app_ibrt_register_ext_cmd_table(g_app_ibrt_tws_ext_cmd_table, sizeof(g_app_ibrt_tws_ext_cmd_table)/sizeof(app_tws_ext_cmd_handler_t));
}

#endif

