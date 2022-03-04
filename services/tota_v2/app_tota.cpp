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
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "hal_norflash.h"
#include "pmu.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "cmsis_os.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_spp_tota.h"
#include "cqueue.h"
#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_rx_handler.h"
#include "rwapp_config.h"
#endif
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#include "app_hfp.h"
#include "bt_drv_reg_op.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif
#include "cmsis.h"
#include "app_battery.h"
#include "crc32_c.h"
#include "factory_section.h"
#include "app_ibrt_rssi.h"
#include "tota_stream_data_transfer.h"
#include "app_tota_encrypt.h"
#include "app_tota_flash_program.h"
#include "app_tota_audio_dump.h"
#include "app_tota_mic.h"
#include "app_tota_anc.h"
#include "app_tota_general.h"
#include "app_tota_custom.h"
#include "app_tota_conn.h"
#include "aes.h"
#include "app_bt_cmd.h"
#include <map>
using namespace std;

/*call back sys for modules*/
static map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *>     s_module_map;
static const app_tota_callback_func_t                             *s_module_func;
static APP_TOTA_MODULE_E                                    s_module;

/* register callback module */
void app_tota_callback_module_register(APP_TOTA_MODULE_E module,
                                const app_tota_callback_func_t *tota_callback_func)
{
    map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *>::iterator it = s_module_map.find(module);
    if ( it == s_module_map.end() )
    {
        TOTA_LOG_DBG(0, "add to map");
        s_module_map.insert(make_pair(module, tota_callback_func));
    }
    else
    {
        TOTA_LOG_DBG(0, "already exist, not add");
    }
}
/* set current module */
void app_tota_callback_module_set(APP_TOTA_MODULE_E module)
{
    map<APP_TOTA_MODULE_E, const app_tota_callback_func_t *>::iterator it = s_module_map.find(module);
    if ( it != s_module_map.end() )
    {
        s_module = module;
        s_module_func = it->second;
        TOTA_LOG_DBG(1, "set %d module success", module);
    }
    else
    {
        TOTA_LOG_DBG(0, "not find callback func by module");
    }
}

/* get current module */
APP_TOTA_MODULE_E app_tota_callback_module_get()
{
    return s_module;
}
/*-------------------------------------------------------------------*/

typedef struct{
    bool isSppConnect;
    bool isConnected;
    bool isBusy;

    APP_TOTA_TRANSMISSION_PATH_E dataPath;

    uint8_t decodeBuf[MAX_SPP_PACKET_SIZE];
    uint8_t encodeBuf[MAX_SPP_PACKET_SIZE];
} tota_ctl_t;

static tota_ctl_t tota_ctl = {
    .isSppConnect = false,
    .isConnected = false,
    .isBusy = false,
    .dataPath = APP_TOTA_PATH_IDLE,
};

static void s_app_tota_connected();
static void s_app_tota_disconnected();
static void s_app_tota_tx_done();
static void s_app_tota_rx(uint8_t * cmd_buf, uint16_t len);


static void s_app_tota_connected()
{
    TOTA_LOG_DBG(0,"Tota connected.");
    tota_ctl.dataPath = APP_TOTA_VIA_SPP;
    if (s_module_func->connected_cb != NULL)
    {
        s_module_func->connected_cb();
    }
}

static void s_app_tota_disconnected()
{
    TOTA_LOG_DBG(0,"Tota disconnected.");
    app_tota_set_shake_hand_status(false);
    tota_ctl.dataPath = APP_TOTA_PATH_IDLE;

    if (s_module_func->disconnected_cb != NULL)
    {
        s_module_func->disconnected_cb();
    }
}

static void s_app_tota_tx_done()
{
    TOTA_LOG_DBG(0,"Tota tx done.");
    if (s_module_func->tx_done_cb != NULL)
    {
        s_module_func->tx_done_cb();
    }
}

#ifdef BUDSODIN2_TOTA
#define BUDSODIN2_VENDOR_CODE 0xfd
bool app_tota_if_customers_access_valid(uint8_t access_code)
{
    bool valid =  false;
    if(BUDSODIN2_VENDOR_CODE == access_code)
    {
        valid = true;
    }

    return valid;
}

#define BUDSODIN2_DATA_LEN_LIMIT 40
#define BUDSODIN2_RUBBISH_CODE_LEN 3
#define BUDSODIN2_HEADER_LEN 4
uint8_t g_custom_tota_data[BUDSODIN2_DATA_LEN_LIMIT]= {0};
uint8_t * app_tota_custom_refactor_tota_data(uint8_t* ptrData, uint32_t dataLength)
{
    TOTA_LOG_DBG(0,"TOTA custom hijack! data len=%d", dataLength);
    do{
        if(dataLength >= BUDSODIN2_DATA_LEN_LIMIT || dataLength < 4)
        {
            break;
        }
        memset(g_custom_tota_data, 0 , BUDSODIN2_DATA_LEN_LIMIT);
        //refacor tota data
        APP_TOTA_CMD_PAYLOAD_T* pPayload = (APP_TOTA_CMD_PAYLOAD_T *)&g_custom_tota_data;
        pPayload->cmdCode = OP_TOTA_STRING;
        pPayload->paramLen = dataLength - BUDSODIN2_HEADER_LEN - BUDSODIN2_RUBBISH_CODE_LEN;

        memcpy(pPayload->param, &ptrData[BUDSODIN2_HEADER_LEN], pPayload->paramLen);
    }while(0);

    return g_custom_tota_data;
}
#endif

static void s_app_tota_rx(uint8_t * cmd_buf, uint16_t len)
{
    TOTA_LOG_DBG(0,"Tota rx.");
    uint8_t * buf = cmd_buf;

    //sanity check
    if(buf == NULL)
    {
        return;
    }

#ifdef APP_ANC_TEST
    if (s_module_func->rx_cb != NULL )
        s_module_func->rx_cb(buf, (uint32_t)len);
#endif

#ifdef BUDSODIN2_TOTA
    //hijack the customers tota data
    if(app_tota_if_customers_access_valid(buf[0]))
    {
        buf = app_tota_custom_refactor_tota_data(buf, len);
    }
#endif

    uint16_t opCode = *(uint16_t *)buf;

    if (OP_TOTA_STREAM_DATA == opCode)
    {
        TOTA_LOG_DBG(0,"STREAM RECEIVED");
        // app_tota_data_received(_buf, _bufLen);
        return;
    }
    if (OP_TOTA_STRING == opCode)
    {
        TOTA_LOG_DBG(0,"STRING RECEIVED");
        app_tota_cmd_received(buf, len);
        return;
    }
#if defined(TOTA_ENCODE) && TOTA_ENCODE
    if ( tota_ctl.isConnected )
    {
        TOTA_LOG_DBG(0,"[CONNECT] ENCRYPT DATA RECEIVED");
        uint16_t decodeLen = tota_decrypt(tota_ctl.decodeBuf, buf, len);
        app_tota_cmd_received(tota_ctl.decodeBuf, decodeLen);
        return;
    }
    if ( opCode <= OP_TOTA_CONN_CONFIRM )
    {
        TOTA_LOG_DBG(0,"[DISCONNECT] CMD RECEIVED");
    }
    else
    {
        TOTA_LOG_DBG(1,"[DISCONNECT] ERROR:CMD NOT SUPPORT %x",opCode);
        return;
    }
#endif
    TOTA_LOG_DBG(0,"CMD RECEIVED");
    app_tota_cmd_received(buf, len);
}

static const tota_callback_func_t app_tota_cb = {
    s_app_tota_connected,
    s_app_tota_disconnected,
    s_app_tota_tx_done,
    s_app_tota_rx
};

void app_tota_init(void)
{
    TOTA_LOG_DBG(0,"Init application test over the air.");

#ifdef APP_ANC_TEST
    app_spp_tota_init(&app_tota_cb);
    
    app_tota_anc_init();
    
    app_tota_callback_module_set(APP_TOTA_ANC);
#else
    app_spp_tota_init(&app_tota_cb);
    /* initialize stream thread */
    app_tota_stream_data_transfer_init();
    
    /* register callback modules */
    app_tota_audio_dump_init();
    
    /* set module to access spp callback */
    app_tota_callback_module_set(APP_TOTA_AUDIO_DUMP);
#endif

#if (BLE_APP_TOTA)
    app_ble_rx_handler_init();
#endif

}

void app_tota_set_shake_hand_status(bool set_status)
{
    tota_ctl.isConnected = set_status;
}

static bool app_tota_send_via_datapath(uint8_t * pdata, uint16_t dataLen)
{
    switch (tota_ctl.dataPath)
    {
        case APP_TOTA_VIA_SPP:
            return app_spp_tota_send_data(pdata, dataLen);

        default:
            return false;
    }
}

bool app_tota_send(uint8_t * pdata, uint16_t dataLen, APP_TOTA_CMD_CODE_E opCode)
{
    if ( opCode == OP_TOTA_NONE )
    {
        TOTA_LOG_DBG(0, "Send pure data");
        /* send pure data */
        return app_tota_send_via_datapath(pdata, dataLen);
    }
    if ( opCode == OP_TOTA_DATA_ENCODE )
    {
#if TOTA_ENCODE
        uint16_t len = tota_encrypt(tota_ctl.encodeBuf, pdata, dataLen);
        return app_tota_send_via_datapath(tota_ctl.encodeBuf, len);
#endif
    }
    APP_TOTA_CMD_PAYLOAD_T payload;
    /* sanity check: opcode is valid */
    if (opCode >= OP_TOTA_COMMAND_COUNT)
    {
        TOTA_LOG_DBG(0, "Warning: opcode not found");
        return false;
    }
    /* sanity check: data length */
    if (dataLen > sizeof(payload.param))
    {
        TOTA_LOG_DBG(0, "Warning: the length of the data is too lang");
        return false;
    }
    /* sanity check: opcode entry */
    // becase some cmd only for one side
    uint16_t entryIndex = app_tota_cmd_handler_get_entry_index_from_cmd_code(opCode);
    if (INVALID_TOTA_ENTRY_INDEX == entryIndex)
    {
        TOTA_LOG_DBG(0, "Warning: cmd not registered");
        return false;
    }

    payload.cmdCode = opCode;
    payload.paramLen = dataLen;
    memcpy(payload.param, pdata, dataLen);

    /* if is string, direct send */
    if ( opCode == OP_TOTA_STRING )
    {
        return app_tota_send_via_datapath((uint8_t*)&payload, dataLen+4);
    }

    /* cmd filter */
#if TOTA_ENCODE
    if ( tota_ctl.isConnected )
    {
        // encode here
        TOTA_LOG_DBG(0, "do encode");
        uint16_t len = tota_encrypt(tota_ctl.encodeBuf, (uint8_t*)&payload, dataLen+4);
        if (app_tota_send_via_datapath(tota_ctl.encodeBuf, len))
        {
            APP_TOTA_CMD_INSTANCE_T* pInstance = TOTA_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);
            if (pInstance->isNeedResponse)
            {
                app_tota_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    if ( opCode > OP_TOTA_CONN_CONFIRM )
    {
        TOTA_LOG_DBG(0, "Warning: permission denied");
        return false;
    }
#endif
    TOTA_LOG_DBG(0, "send normal cmd");
    if (app_tota_send_via_datapath((uint8_t*)&payload, dataLen+4))
    {
        APP_TOTA_CMD_INSTANCE_T* pInstance = TOTA_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);
        if (pInstance->isNeedResponse)
        {
            app_tota_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
        }
    }
    return true;
}

bool app_tota_send_rsp(APP_TOTA_CMD_CODE_E rsp_opCode, APP_TOTA_CMD_RET_STATUS_E rsp_status,
                                                            uint8_t * pdata, uint16_t dataLen)
{
    static APP_TOTA_CMD_PAYLOAD_T payload;
    TOTA_LOG_DBG(1,"[%s]",__func__);
    // check responsedCmdCode's validity
    if ( rsp_opCode >= OP_TOTA_COMMAND_COUNT || rsp_opCode < OP_TOTA_STRING)
    {
        return false;
    }
    APP_TOTA_CMD_RSP_T* pResponse = (APP_TOTA_CMD_RSP_T *)&(payload.param);

    // check parameter length
    if (dataLen > sizeof(pResponse->rspData))
    {
        return false;
    }
    pResponse->cmdCodeToRsp = rsp_opCode;
    pResponse->cmdRetStatus = rsp_status;
    pResponse->rspDataLen   = dataLen;
    memcpy(pResponse->rspData, pdata, dataLen);

    payload.cmdCode = OP_TOTA_RESPONSE_TO_CMD;
    payload.paramLen = 3*sizeof(uint16_t) + dataLen;

#if TOTA_ENCODE
    uint16_t len = tota_encrypt(tota_ctl.encodeBuf, (uint8_t*)&payload, payload.paramLen+4);
    return app_tota_send_via_datapath(tota_ctl.encodeBuf, len);
#else
    return app_tota_send_via_datapath((uint8_t*)&payload, payload.paramLen+4);
#endif
}

/*---------------------------------------------------------------------------------------------------------------------------*/

static char strBuf[MAX_SPP_PACKET_SIZE-4];

char *tota_get_strbuf(void)
{
    return strBuf;
}

void tota_printf(const char * format, ...)
{
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strBuf, format, vlist);
    va_end(vlist);
    app_spp_tota_send_data((uint8_t*)strBuf, strlen(strBuf));
}

void tota_print(const char * format, ...)
{
    va_list vlist;
    va_start(vlist, format);
    vsprintf(strBuf, format, vlist);
    va_end(vlist);
    app_tota_send((uint8_t*)strBuf, strlen(strBuf), OP_TOTA_STRING);
}

static void app_tota_demo_cmd_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    switch(funcCode)
    {
        case OP_TOTA_STRING:
            app_bt_cmd_line_handler((char *)ptrParam, paramLen);
            break;
        default:
            break;
    }
}

TOTA_COMMAND_TO_ADD(OP_TOTA_STRING, app_tota_demo_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_DEMO_CMD, app_tota_demo_cmd_handler, false, 0, NULL );
