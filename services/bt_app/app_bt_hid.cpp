/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifdef BT_HID_DEVICE
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_trace_rx.h"
#include "app_status_ind.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "besbt.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "bt_if.h"
#include "os_api.h"
#include "app_bt_func.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "besbt_cfg.h"
#include "me_api.h"
#include "app_bt_hid.h"
#include "hid_api.h"

#ifdef IBRT
#include "app_tws_besaud.h"
#include "app_ibrt_if.h"
#include "app_ibrt_keyboard.h"
#endif

#define APP_BT_HID_CAPTURE_WAIT_SEND_MS 2000
static void app_bt_hid_send_capture_handler(const void *param);
osTimerDef (APP_BT_HID_CAPTURE_WAIT_TIMER0, app_bt_hid_send_capture_handler);
#if BT_DEVICE_NUM > 1
osTimerDef (APP_BT_HID_CAPTURE_WAIT_TIMER1, app_bt_hid_send_capture_handler);
#endif
#if BT_DEVICE_NUM > 2
osTimerDef (APP_BT_HID_CAPTURE_WAIT_TIMER2, app_bt_hid_send_capture_handler);
#endif

#define APP_BT_HID_DISC_CHANNEL_WAIT_MS (30*1000)
static void app_bt_hid_disc_chan_timer_handler(const void *param);
osTimerDef (APP_BT_HID_DISC_CHAN_TIMER0, app_bt_hid_disc_chan_timer_handler);
#if BT_DEVICE_NUM > 1
osTimerDef (APP_BT_HID_DISC_CHAN_TIMER1, app_bt_hid_disc_chan_timer_handler);
#endif
#if BT_DEVICE_NUM > 2
osTimerDef (APP_BT_HID_DISC_CHAN_TIMER2, app_bt_hid_disc_chan_timer_handler);
#endif

static void app_bt_hid_start_disc_wait_timer(uint8_t device_id);

static void _app_bt_hid_send_capture(uint32_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    btif_hid_keyboard_input_report(curr_device->hid_channel, HID_MOD_KEY_NULL, HID_KEY_CODE_ENTER);
    btif_hid_keyboard_input_report(curr_device->hid_channel, HID_MOD_KEY_NULL, HID_KEY_CODE_NULL);

    btif_hid_keyboard_send_ctrl_key(curr_device->hid_channel, HID_CTRL_KEY_VOLUME_INC);
    btif_hid_keyboard_send_ctrl_key(curr_device->hid_channel, HID_CTRL_KEY_NULL);

    if (app_bt_manager.config.hid_capture_non_invade_mode)
    {
        app_bt_hid_start_disc_wait_timer((uint8_t)device_id);
    }
}

static void app_bt_hid_send_capture_handler(const void *param)
{
    uint8_t device_id = (uint8_t)(uintptr_t)param;
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (curr_device && curr_device->hid_conn_flag)
    {
        TRACE(2, "(d%x) %s", device_id, __func__);
        app_bt_start_custom_function_in_bt_thread((uint32_t)device_id, (uint32_t)NULL, (uint32_t)_app_bt_hid_send_capture);
    }
    else
    {
        TRACE(2, "(d%x) %s failed", device_id, __func__);
    }
}

static void app_bt_hid_start_capture_wait_timer(uint8_t device_id, hid_channel_t chan)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->capture_wait_timer_id)
    {
        TRACE(1, "%s timer is NULL", __func__);
        return;
    }

    TRACE(3, "%s channel %p %p", __func__, curr_device->hid_channel, chan);

    osTimerStop(curr_device->capture_wait_timer_id);

    osTimerStart(curr_device->capture_wait_timer_id, APP_BT_HID_CAPTURE_WAIT_SEND_MS);
}

static void app_bt_hid_callback(uint8_t device_id, hid_channel_t chan, const hid_callback_parms_t *param)
{
    btif_hid_callback_param_t *info = (btif_hid_callback_param_t *)param;
    struct BT_DEVICE_T *curr_device = NULL;

    if (device_id == BT_DEVICE_INVALID_ID && info->event == BTIF_HID_EVENT_CONN_CLOSED)
    {
        // hid profile is closed due to acl created fail
        TRACE(1,"::HID_EVENT_CONN_CLOSED acl created error %x", info->error_code);
        return;
    }

    curr_device = app_bt_get_device(device_id);
    ASSERT(device_id < BT_DEVICE_NUM && curr_device->hid_channel == chan, "hid device channel must match");

    TRACE(10, "(d%x) %s channel %p event %d errno %02x %02x:%02x:%02x:%02x:%02x:%02x",
                device_id, __func__, chan, info->event, info->error_code,
                info->remote.address[0], info->remote.address[1], info->remote.address[2],
                info->remote.address[3], info->remote.address[4], info->remote.address[5]);

    switch (info->event)
    {
        case BTIF_HID_EVENT_CONN_OPENED:
            curr_device->hid_conn_flag = true;
            if (curr_device->wait_send_capture_key)
            {
                app_bt_hid_start_capture_wait_timer(device_id, chan);
                curr_device->wait_send_capture_key = false;
            }
            break;
        case BTIF_HID_EVENT_CONN_CLOSED:
            curr_device->hid_conn_flag = false;
            curr_device->wait_send_capture_key = false;
            break;
        default:
            break;
    }
}

void app_bt_hid_init(void)
{
    struct BT_DEVICE_T *curr_device = NULL;
    TRACE(2, "%s sink_enable %d", __func__, besbt_cfg.bt_sink_enable);
    if (besbt_cfg.bt_sink_enable)
    {
        btif_hid_init(app_bt_hid_callback, HID_DEVICE_ROLE);

        for (int i = 0; i < BT_DEVICE_NUM; ++i)
        {
            curr_device = app_bt_get_device(i);
            curr_device->hid_channel = btif_hid_channel_alloc();
            if (i == 0)
            {
                curr_device->capture_wait_timer_id = osTimerCreate(osTimer(APP_BT_HID_CAPTURE_WAIT_TIMER0), osTimerOnce, (void *)i);
                curr_device->hid_wait_disc_timer_id = osTimerCreate(osTimer(APP_BT_HID_DISC_CHAN_TIMER0), osTimerOnce, (void *)i);
            }
#if BT_DEVICE_NUM > 1
            else if (i == 1)
            {
                curr_device->capture_wait_timer_id = osTimerCreate(osTimer(APP_BT_HID_CAPTURE_WAIT_TIMER1), osTimerOnce, (void *)i);
                curr_device->hid_wait_disc_timer_id = osTimerCreate(osTimer(APP_BT_HID_DISC_CHAN_TIMER1), osTimerOnce, (void *)i);
            }
#endif
#if BT_DEVICE_NUM > 2
            else if (i == 2)
            {
                curr_device->capture_wait_timer_id = osTimerCreate(osTimer(APP_BT_HID_CAPTURE_WAIT_TIMER2), osTimerOnce, (void *)i);
                curr_device->hid_wait_disc_timer_id = osTimerCreate(osTimer(APP_BT_HID_DISC_CHAN_TIMER2), osTimerOnce, (void *)i);
            }
#endif
            curr_device->hid_conn_flag = false;
            curr_device->wait_send_capture_key = false;
        }                
    }
}

void app_bt_hid_create_channel(bt_bdaddr_t *remote)
{
    TRACE(7, "%s address %02x:%02x:%02x:%02x:%02x:%02x", __func__,
        remote->address[0], remote->address[1], remote->address[2],
        remote->address[3], remote->address[4], remote->address[5]);

    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)remote, (uint32_t)NULL, (uint32_t)btif_hid_connect);
}

void app_bt_hid_profile_connect(uint8_t device_id, bool capture)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->acl_is_connected)
    {
        TRACE(3, "(d%x) %s acl is not connected %d", device_id, __func__, capture);
        return;
    }

    TRACE(8, "(d%x) %s %d %02x:%02x:%02x:%02x:%02x:%02x", device_id, __func__, capture,
            curr_device->remote.address[0], curr_device->remote.address[1], curr_device->remote.address[2],
            curr_device->remote.address[3], curr_device->remote.address[4], curr_device->remote.address[5]);

    curr_device->wait_send_capture_key = capture ? true : false;

    app_bt_start_custom_function_in_bt_thread((uint32_t)(uintptr_t)&curr_device->remote, (uint32_t)NULL, (uint32_t)btif_hid_connect);
}

void app_bt_hid_profile_disconnect(hid_channel_t chnl)
{
    TRACE(1, "%s channel %p", __func__, chnl);

    app_bt_start_custom_function_in_bt_thread((uint32_t)chnl, (uint32_t)NULL, (uint32_t)btif_hid_disconnect);
}

int app_bt_nvrecord_get_latest_device_addr(bt_bdaddr_t *addr)
{
    btif_device_record_t record;
    int found_addr_count = 0;
    int paired_dev_count = nv_record_get_paired_dev_count();

    if (paired_dev_count > 0 && BT_STS_SUCCESS == nv_record_enum_dev_records(0, &record))
    {
        *addr = record.bdAddr;
        found_addr_count = 1;
    }

    return found_addr_count;
}

void app_bt_hid_capture_process(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->hid_conn_flag)
    {
        TRACE(3, "(d%x) %s hid not connected", device_id, __func__);
        return;
    }

    curr_device->wait_send_capture_key = false;

#if defined(IBRT_V2_MULTIPOINT)
    if (tws_besaud_is_connected())
    {
        uint8_t ibrt_role = app_tws_get_ibrt_role(&curr_device->remote);
        TRACE(3, "(d%x) %s role %d", device_id, __func__, ibrt_role);

        if (IBRT_MASTER == ibrt_role)
        {
            app_bt_hid_send_capture_handler((void *)(uintptr_t)device_id);
        }
        else if (IBRT_SLAVE == ibrt_role)
        {
            app_ibrt_if_start_user_action_v2(device_id, IBRT_ACTION_HID_SEND_CAPTURE, 0, 0);
        }
    }
    else
    {
        app_bt_hid_send_capture_handler((void *)(uintptr_t)device_id);
    }
#else
    app_bt_hid_send_capture_handler((void *)(uintptr_t)device_id);
#endif
}

static void app_bt_hid_open_shutter_mode(uint8_t device_id, bool capture)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->acl_is_connected)
    {
        TRACE(3, "(d%x) %s acl is not connected %d", device_id, __func__, capture);
        return;
    }

    if (!curr_device->hid_conn_flag)
    {
#if defined(IBRT_V2_MULTIPOINT)
        app_ibrt_if_profile_connect(device_id, APP_IBRT_HID_PROFILE_ID, capture);
#else
        app_bt_hid_profile_connect(device_id, capture);
#endif
        return;
    }

    if (capture)
    {
        app_bt_hid_capture_process(device_id);
    }
}

static void app_bt_hid_close_shutter_mode(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    TRACE(3, "(d%x) %s hid_conn_flag %d", device_id, __func__, curr_device->hid_conn_flag);

    if (curr_device->hid_conn_flag)
    {
#if defined(IBRT_V2_MULTIPOINT)
        app_ibrt_if_profile_disconnect(device_id, APP_IBRT_HID_PROFILE_ID);
#else
        app_bt_hid_profile_disconnect(curr_device->hid_channel);
#endif
    }

    curr_device->wait_send_capture_key = false;
}

static void app_bt_hid_disc_chan_timer_handler(const void *param)
{
    uint8_t device_id = (uint8_t)(uintptr_t)param;

    TRACE(2, "(d%x) %s", device_id, __func__);

    app_bt_hid_close_shutter_mode(device_id);
}

static void app_bt_hid_start_disc_wait_timer(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->hid_wait_disc_timer_id)
    {
        TRACE(1, "%s timer is NULL", __func__);
        return;
    }

    TRACE(2, "(d%x) %s", device_id, __func__);

    osTimerStop(curr_device->hid_wait_disc_timer_id);

    osTimerStart(curr_device->hid_wait_disc_timer_id, APP_BT_HID_DISC_CHANNEL_WAIT_MS);
}

static void app_bt_hid_stop_wait_disc_chan_timer(uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);

    if (!curr_device->hid_wait_disc_timer_id)
    {
        return;
    }

    osTimerStop(curr_device->hid_wait_disc_timer_id);
}

bool app_bt_hid_is_in_shutter_mode(void)
{
    struct BT_DEVICE_T *curr_device = NULL;

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);

        if (curr_device->hid_conn_flag)
        {
            return true;
        }
    }

    return false;
}

void app_bt_hid_enter_shutter_mode(void)
{
    struct BT_DEVICE_T *curr_device = NULL;
    int i = 0;

    TRACE(1, "%s", __func__);

    for (i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (!curr_device->acl_is_connected)
        {
            TRACE(2, "%s acl d%x is not connected", __func__, i);
            continue;
        }
        app_bt_hid_open_shutter_mode(i, false);
    }
}

void app_bt_hid_exit_shutter_mode(void)
{
    int i = 0;

    TRACE(1, "%s", __func__);

    for (i = 0; i < BT_DEVICE_NUM; ++i)
    {
        app_bt_hid_stop_wait_disc_chan_timer(i);

        app_bt_hid_close_shutter_mode(i);
    }
}

void app_bt_hid_disconnect_all_channels(void)
{
    struct BT_DEVICE_T *curr_device = NULL;

    TRACE(1, "%s", __func__);

    for (int i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);

        if (curr_device->hid_conn_flag)
        {
            app_bt_hid_profile_disconnect(curr_device->hid_channel);
        }

        app_bt_hid_stop_wait_disc_chan_timer(i);

        curr_device->wait_send_capture_key = false;
    }
}

void app_bt_hid_send_capture(void)
{
    struct BT_DEVICE_T *curr_device = NULL;
    int i = 0;

    TRACE(1, "%s", __func__);

    for (i = 0; i < BT_DEVICE_NUM; ++i)
    {
        curr_device = app_bt_get_device(i);
        if (!curr_device->acl_is_connected)
        {
            TRACE(2, "%s acl d%x is not connected", __func__, i);
            continue;
        }
        app_bt_hid_stop_wait_disc_chan_timer(i);
        app_bt_hid_open_shutter_mode(i, true);
    }
}

#endif /* BT_HID_DEVICE */

