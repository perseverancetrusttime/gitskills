#include "gsound_target.h"
#include "gsound_dbg.h"
#include "ke_msg.h"
#include "app_bt_func.h"

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#include "gsound_gatt_server.h"
#include "gsound_custom_ble.h"
#include "app.h"
#endif

#ifndef BLE_V2
#include "gapc_task.h"
#endif

void GSoundTargetBleRxComplete(GSoundChannelType type,
                               const uint8_t *buffer,
                               uint32_t len)
{
#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
#ifdef __IAG_BLE_INCLUDE__
    struct gsound_gatt_rx_data_ind_t *packet2free =
        (struct gsound_gatt_rx_data_ind_t *)(buffer - OFFSETOF(struct gsound_gatt_rx_data_ind_t, data));
        // CONTAINER_OF(buffer, struct gsound_gatt_rx_data_ind_t, data);

    app_bt_start_custom_function_in_bt_thread((uint32_t)packet2free,
                                              0,
                                              (uint32_t)gsound_send_control_rx_cfm);
#endif
#endif
}

GSoundStatus GSoundTargetBleTransmit(GSoundChannelType channel,
                                     const uint8_t *data,
                                     uint32_t length,
                                     uint32_t *bytes_consumed)
{
#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
    GSoundStatus status = GSOUND_STATUS_ERROR;
    // GLOG_I("[%s]channel=%s, len=%d",
    //       __func__,
    //       gsound_chnl2str(channel),
    //       length);
#ifdef __IAG_BLE_INCLUDE__
    ASSERT(GSoundTargetOsMqActive(), "Mq not active");
    *bytes_consumed = 0;

    // If no valid connection, do not transmit - we don't want to
    // alter the shared Tx buffer in case in use by BT Classic
    if (!app_gsound_ble_is_connected())
    {
        GLOG_I("%s: TX - INVALID CONNECTION !!!", __func__);
    }
    else
    {
        bool isConsumedImmediately = app_gsound_ble_send_notification(channel, length, data);

        if (isConsumedImmediately)
        {
            *bytes_consumed = length;
            status = GSOUND_STATUS_OK;
        }
        else
        {
            status = GSOUND_STATUS_OUT_OF_MEMORY;
        }
    }
#endif

    return status;
#else
    return GSOUND_STATUS_OK;
#endif
}

uint16_t GSoundTargetBleGetMtu()
{
    uint16_t mtu = 0;

#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
#ifdef __IAG_BLE_INCLUDE__
    mtu = app_gsound_ble_get_mtu();
#endif
#endif
    return mtu;
}

GSoundStatus GSoundTargetBleUpdateConnParams(bool valid_interval,
                                             uint32_t min_interval,
                                             uint32_t max_interval,
                                             bool valid_slave_latency,
                                             uint32_t slave_latency)
{
#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
#ifdef __IAG_BLE_INCLUDE__
    uint8_t conidx = app_gsound_ble_get_connection_index();

    if (valid_interval)
    {
        l2cap_update_param(conidx, min_interval*8/5, 
            max_interval*8/5, 6000, slave_latency);
    }
#endif
#endif
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetBleInit(const GSoundBleInterface *handlers)
{
    GLOG_I("%s ", __func__);
#if defined(BISTO_ENABLED) && (!defined(BISTO_IOS_DISABLED))
#ifdef __IAG_BLE_INCLUDE__
    gsound_ble_register_target_handle(handlers);

    // add gsound related data into adv
    app_ble_register_data_fill_handle(USER_GSOUND, ( BLE_DATA_FILL_FUNC_T )app_gsound_ble_data_fill_handler, false);
#endif
#endif
    return GSOUND_STATUS_OK;
}
