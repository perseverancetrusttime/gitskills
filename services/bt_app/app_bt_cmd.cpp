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
#include <string.h>
#include "app_trace_rx.h"
#include "app_bt_cmd.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "bluetooth.h"
#include "crc16_c.h"
#include "heap_api.h"
#include "hci_api.h"
#include "me_api.h"
#include "l2cap_api.h"
#include "rfcomm_api.h"
#include "conmgr_api.h"
#include "bt_if.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "app_bt.h"
#include "btapp.h"
#include "app_factory_bt.h"
#include "apps.h"
#include "app_a2dp.h"
#include "nvrecord_extension.h"

#if defined(BT_HID_DEVICE)
#include "app_bt_hid.h"
#endif

#if defined(BT_SOURCE)
#include "bt_source.h"
#include "app_a2dp_source.h"
#endif

#if defined(BT_PBAP_SUPPORT)
#include "app_pbap.h"
#endif

static bt_bdaddr_t g_app_bt_pts_addr = {{
#if 0
        0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
        0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
    }
};

bt_bdaddr_t *app_bt_get_pts_address(void)
{
    return &g_app_bt_pts_addr;
}

#define APP_BT_CMD_MAX_TEST_TABLES (16)
static app_bt_cmd_handle_with_parm_table_t g_bt_cmd_with_parm_tables[APP_BT_CMD_MAX_TEST_TABLES];
static app_bt_cmd_handle_table_t g_bt_cmd_tables[APP_BT_CMD_MAX_TEST_TABLES];

void app_bt_cmd_add_test_table(const app_bt_cmd_handle_t *start_cmd, uint16_t cmd_count)
{
    int i = 0;
    app_bt_cmd_handle_table_t *table = NULL;
    app_bt_cmd_handle_table_t *found = NULL;

    if (start_cmd == NULL || cmd_count == 0)
    {
        TRACE(3, "%s invalid params %p %d", __func__, start_cmd, cmd_count);
        return;
    }

    for (; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
    {
        table = g_bt_cmd_tables + i;
        if (!table->inuse)
        {
            found = table;
            break;
        }
    }

    if (!found)
    {
        TRACE(1, "%s cannot alloc free table", __func__);
        return;
    }

    found->inuse = true;
    found->start_cmd = start_cmd;
    found->cmd_count = cmd_count;
}

void app_bt_cmd_add_test_table_with_param(const app_bt_cmd_handle_with_parm_t *start_cmd, uint16_t cmd_count)
{
    int i = 0;
    app_bt_cmd_handle_with_parm_table_t *table = NULL;
    app_bt_cmd_handle_with_parm_table_t *found = NULL;

    if (start_cmd == NULL || cmd_count == 0)
    {
        TRACE(3, "%s invalid params %p %d", __func__, start_cmd, cmd_count);
        return;
    }

    for (; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
    {
        table = g_bt_cmd_with_parm_tables + i;
        if (!table->inuse)
        {
            found = table;
            break;
        }
    }

    if (!found)
    {
        TRACE(1, "%s cannot alloc free table", __func__);
        return;
    }

    found->inuse = true;
    found->start_cmd = start_cmd;
    found->cmd_count = cmd_count;
}

static const app_bt_cmd_handle_with_parm_t app_factory_bt_tota_hande_table_with_param[]=
{
    {"LE_TX_TEST",                      app_factory_enter_le_tx_test},
    {"LE_RX_TEST",                      app_factory_enter_le_rx_test},
    {"LE_CONT_TX_TEST",                 app_factory_enter_le_continueous_tx_test},
    {"LE_CONT_RX_TEST",                 app_factory_enter_le_continueous_rx_test},
};

static const app_bt_cmd_handle_t app_factory_bt_tota_hande_table[]=
{
    {"LE_TEST_RESULT",                  app_factory_remote_fetch_le_teset_result},
    {"LE_TEST_END",                     app_factory_terminate_le_test},
    {"BT_DUT_MODE",                     app_enter_signalingtest_mode},
    {"NON_SIGNALING_TEST",              app_enter_non_signalingtest_mode},
    {"0091",                            app_factory_enter_bt_38chanl_dh5_prbs9_tx_test},
    {"0094",                            app_factory_enter_bt_38chanl_dh1_prbs9_rx_test},
    {"BT_RX_TEST_RESULT",               app_factory_remote_fetch_bt_nonsig_test_result},
};

void app_bt_add_string_test_table(void)
{
    app_bt_cmd_add_test_table(app_factory_bt_tota_hande_table, ARRAY_SIZE(app_factory_bt_tota_hande_table));
    app_bt_cmd_add_test_table_with_param(app_factory_bt_tota_hande_table_with_param,
            ARRAY_SIZE(app_factory_bt_tota_hande_table_with_param));
}

bool app_bt_cmd_table_execute(const app_bt_cmd_handle_t *start_cmd, uint16_t count, char *cmd, unsigned int cmd_length)
{
    const app_bt_cmd_handle_t *command = NULL;

    for (int j = 0; j < count; j += 1)
    {
        command = start_cmd + j;
        if (strncmp((char *)cmd, command->string, cmd_length) == 0 || strstr(command->string, cmd))
        {
            command->function();
            return true;
        }
    }

    return false;
}

bool app_bt_cmd_table_with_param_execute(const app_bt_cmd_handle_with_parm_t *start_cmd, uint16_t count,
        char *cmd_prefix, unsigned int cmd_prefix_length, char *cmd_param, unsigned int param_len)
{
    const app_bt_cmd_handle_with_parm_t *command_with_parm = NULL;

    for (int j = 0; j < count; j += 1)
    {
        command_with_parm = start_cmd + j;
        if (strncmp((char *)cmd_prefix, command_with_parm->string, cmd_prefix_length) == 0 || strstr(command_with_parm->string, cmd_prefix))
        {
            command_with_parm->function(cmd_param, param_len);
            return true;
        }
    }

    return false;
}

void app_bt_cmd_line_handler(char *cmd, unsigned int cmd_length)
{
    const app_bt_cmd_handle_table_t *table = NULL;
    const app_bt_cmd_handle_with_parm_table_t *table_with_parm = NULL;
    int i = 0;
    int param_len = 0;
    char* cmd_param = NULL;
    char* cmd_end = cmd + cmd_length;

    cmd_param = strstr((char*)cmd, (char*)"|");

    if (cmd_param)
    {
        *cmd_param = '\0';
        cmd_length = cmd_param - cmd;
        cmd_param += 1;


        param_len = cmd_end - cmd_param;

        for (i = 0; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
        {
            table_with_parm = g_bt_cmd_with_parm_tables + i;
            if (table_with_parm->inuse)
            {
                if (app_bt_cmd_table_with_param_execute(table_with_parm->start_cmd, table_with_parm->cmd_count, cmd, cmd_length, cmd_param, param_len))
                {
                    return;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < APP_BT_CMD_MAX_TEST_TABLES; i += 1)
        {
            table = g_bt_cmd_tables + i;
            if (table->inuse)
            {
                if (app_bt_cmd_table_execute(table->start_cmd, table->cmd_count, cmd, cmd_length))
                {
                    return;
                }
            }
        }
    }

    TRACE(2, "%s cmd not found %s", __func__, cmd);
}

#if defined(APP_TRACE_RX_ENABLE) || defined(IBRT)

#ifdef IBRT
#define IBRT_V1_LEGACY
#if defined(IBRT_V2_MULTIPOINT)
#undef IBRT_V1_LEGACY
#endif
#include "app_tws_ibrt.h"
#include "app_tws_besaud.h"
#include "app_vendor_cmd_evt.h"
#include "tws_role_switch.h"
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_handler.h"
#ifdef IBRT_V1_LEGACY
#include "app_tws_ibrt_trace.h"
#include "app_ibrt_ui.h"
#include "app_ibrt_ui_test.h"
#endif
#ifdef IBRT_V2_MULTIPOINT
#include "app_tws_ibrt_raw_ui_test.h"
#endif
#endif

osThreadId app_bt_cmd_tid;
static void app_bt_cmd_thread(void const *argument);
osThreadDef(app_bt_cmd_thread, osPriorityNormal, 1, 2048, "app_bt_cmd_thread");

#define APP_BT_CMD_MAILBOX_MAX (10)
osMailQDef (app_bt_cmd_mailbox, APP_BT_CMD_MAILBOX_MAX, APP_BT_CMD_MSG_BLOCK);
static osMailQId app_bt_cmd_mailbox = NULL;
static uint8_t app_bt_cmd_mailbox_cnt = 0;

static int app_bt_cmd_mailbox_init(void)
{
    app_bt_cmd_mailbox = osMailCreate(osMailQ(app_bt_cmd_mailbox), NULL);
    if (app_bt_cmd_mailbox == NULL)  {
        TRACE(0,"Failed to Create app_bt_cmd_mailbox\n");
        return -1;
    }
    app_bt_cmd_mailbox_cnt = 0;
    return 0;
}

int app_bt_cmd_mailbox_put(APP_BT_CMD_MSG_BLOCK* msg_src)
{
    if(!msg_src){
        TRACE(0,"msg_src is a null pointer in app_bt_cmd_mailbox_put!");
        return -1;
    }

    osStatus status;
    APP_BT_CMD_MSG_BLOCK *msg_p = NULL;
    msg_p = (APP_BT_CMD_MSG_BLOCK*)osMailAlloc(app_bt_cmd_mailbox, 0);
    ASSERT(msg_p, "osMailAlloc error");
    *msg_p = *msg_src;
    status = osMailPut(app_bt_cmd_mailbox, msg_p);
    if (osOK == status)
        app_bt_cmd_mailbox_cnt++;
    return (int)status;
}

int app_bt_cmd_mailbox_free(APP_BT_CMD_MSG_BLOCK* msg_p)
{
    if(!msg_p){
        TRACE(0,"msg_p is a null pointer in app_bt_cmd_mailbox_free!");
        return -1;
    }
    osStatus status;

    status = osMailFree(app_bt_cmd_mailbox, msg_p);
    if (osOK == status)
        app_bt_cmd_mailbox_cnt--;

    return (int)status;
}

int app_bt_cmd_mailbox_get(APP_BT_CMD_MSG_BLOCK** msg_p)
{
    if(!msg_p){
        TRACE(0,"msg_p is a null pointer in app_bt_cmd_mailbox_get!");
        return -1;
    }

    osEvent evt;
    evt = osMailGet(app_bt_cmd_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (APP_BT_CMD_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

extern "C" void app_bt_cmd_perform_test(const char* bt_cmd, uint32_t cmd_len)
{
    APP_BT_CMD_MSG_BLOCK msg = {{0}};
    uint8_t *cmd_buffer = ((uint8_t *)&msg) + sizeof(msg.msg_body.message_id); // max 19-byte
    uint32_t msg_max_extra_length = sizeof(APP_BT_CMD_MSG_BLOCK) - sizeof(msg.msg_body.message_id);

    if (cmd_len == 0)
    {
        return;
    }

    if (cmd_len > msg_max_extra_length)
    {
        cmd_len = msg_max_extra_length;
    }

    msg.msg_body.message_id = 0xff;

    /**
     * Max 19-byte cmd match string, the match can be (1) prefix match
     * or (2) substr match.
     *
     * If the destination command function string is "test_hfp_call_answer":
     *
     * (1) firstly perform prefix match, found the 1st one which matched
     *
     *      "test_hfp" "test_hfp_call" "test_hfp_call_an" (max 16-byte) can
     *      all be matched if previous commands dont have the same string prefix.
     *
     * (2) do substr match, also found the 1st one with matched
     *
     *      "hfp_call", "call_answer", "hfp_call_answer" can all be
     *      matched if previous commands dont have the same substr.
     */

    memcpy(cmd_buffer, bt_cmd, cmd_len);

    app_bt_cmd_mailbox_put(&msg);
}


#ifdef IBRT
typedef void (*app_ibrt_peripheral_cb0)(void);
typedef void (*app_ibrt_peripheral_cb1)(void *);
typedef void (*app_ibrt_peripheral_cb2)(void *, void *);
#if defined(BES_AUTOMATE_TEST) || defined(HAL_TRACE_RX_ENABLE)
#define APP_IBRT_PERIPHERAL_BUF_SIZE 2048
static heap_handle_t app_ibrt_peripheral_heap;
uint8_t app_ibrt_peripheral_buf[APP_IBRT_PERIPHERAL_BUF_SIZE]__attribute__((aligned(4)));
bool app_ibrt_auto_test_started = false;
void app_ibrt_peripheral_heap_init(void)
{
    app_ibrt_auto_test_started = true;
    app_ibrt_peripheral_heap = heap_register(app_ibrt_peripheral_buf, APP_IBRT_PERIPHERAL_BUF_SIZE);
}

void *app_ibrt_peripheral_heap_malloc(uint32_t size)
{
    void *ptr = heap_malloc(app_ibrt_peripheral_heap,size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    return ptr;
}

void *app_ibrt_peripheral_heap_cmalloc(uint32_t size)
{
    void *ptr = heap_malloc(app_ibrt_peripheral_heap,size);
    ASSERT(ptr, "%s size:%d", __func__, size);
    memset(ptr, 0, size);
    return ptr;
}

void *app_ibrt_peripheral_heap_realloc(void *rmem, uint32_t newsize)
{
    void *ptr = heap_realloc(app_ibrt_peripheral_heap, rmem, newsize);
    ASSERT(ptr, "%s rmem:%p size:%d", __func__, rmem,newsize);
    return ptr;
}

void app_ibrt_peripheral_heap_free(void *rmem)
{
    ASSERT(rmem, "%s rmem:%p", __func__, rmem);
    heap_free(app_ibrt_peripheral_heap, rmem);
}
#endif

void app_ibrt_peripheral_auto_test_stop(void)
{
#ifdef BES_AUTOMATE_TEST
    app_ibrt_auto_test_started = false;
#endif
}

void app_ibrt_peripheral_automate_test_handler(uint8_t* cmd_buf, uint32_t cmd_len)
{
#ifdef BES_AUTOMATE_TEST
    AUTO_TEST_CMD_T *test_cmd = (AUTO_TEST_CMD_T *)cmd_buf;
    static uint8_t last_group_code = 0xFF;
    static uint8_t last_operation_code = 0xFF;

    //TRACE(4, "%s group 0x%x op 0x%x times %d len %d", __func__,
                //test_cmd->group_code, test_cmd->opera_code, test_cmd->test_times, test_cmd->param_len);
    //TRACE(2, "last group 0x%x last op 0x%x", last_group_code, last_operation_code);
    if (last_group_code != test_cmd->group_code || last_operation_code != test_cmd->opera_code)
    {
        for (uint8_t i=0; i<test_cmd->test_times; i++)
        {
            last_group_code = test_cmd->group_code;
            last_operation_code = test_cmd->opera_code;
            app_ibrt_ui_automate_test_cmd_handler(test_cmd->group_code, test_cmd->opera_code, test_cmd->param, test_cmd->param_len);
        }
    }
    app_ibrt_peripheral_heap_free(cmd_buf);
#endif
}

extern "C" void app_ibrt_peripheral_automate_test(const char* ibrt_cmd, uint32_t cmd_len)
{
#ifdef BES_AUTOMATE_TEST
    uint16_t crc16_rec = 0;
    uint16_t crc16_result = 0;
    uint8_t *cmd_buf = NULL;
    uint32_t _cmd_data_len = 0;
    uint32_t _cmd_data_min_len = sizeof(AUTO_TEST_CMD_T)+AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
    APP_BT_CMD_MSG_BLOCK msg;

    if (ibrt_cmd && cmd_len>=_cmd_data_min_len && cmd_len<=(_cmd_data_min_len+AUTOMATE_TEST_CMD_PARAM_MAX_LEN))
    {
        _cmd_data_len = cmd_len-AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
        crc16_rec = *(uint16_t *)(&ibrt_cmd[_cmd_data_len]);
        crc16_result = _crc16(crc16_result, (const unsigned char *)ibrt_cmd, _cmd_data_len);
        //DUMP8("0x%x ", ibrt_cmd, cmd_len);
        //TRACE(4, "%s crc16 rec 0x%x result 0x%x buf_len %d", __func__, crc16_rec, crc16_result, cmd_len);
        if (crc16_rec == crc16_result && app_ibrt_auto_test_started)
        {
            app_ibrt_auto_test_inform_cmd_received(ibrt_cmd[0], ibrt_cmd[1]);
            cmd_buf = (uint8_t *)app_ibrt_peripheral_heap_cmalloc(_cmd_data_len);
            memcpy(cmd_buf, ibrt_cmd, _cmd_data_len);
            msg.msg_body.message_id = 0xfe;
            msg.msg_body.message_Param0 = (uint32_t)cmd_buf;
            msg.msg_body.message_Param1 = _cmd_data_len;
            app_bt_cmd_mailbox_put(&msg);
        }
        return;
    }
#endif
}

extern "C" void app_ibrt_peripheral_perform_test(const char* ibrt_cmd, uint32_t cmd_len)
{
    uint8_t* p;
    TRACE(1,"Auto test current receive command: %s\n",ibrt_cmd);
    p = (uint8_t* )strstr((char*)ibrt_cmd,(char*)"=");
    if(p)
    {
#ifdef HAL_TRACE_RX_ENABLE
        uint8_t *cmd_name = NULL;
        uint8_t *cmd_param = NULL;
        uint8_t *cmd_param_buf = NULL;
        uint32_t cmd_len;
        uint32_t cmd_param_len;
        cmd_param = (p+1);
        cmd_param_len = strlen((char*)cmd_param);
        cmd_len = strlen(ibrt_cmd) - cmd_param_len - 1;
        cmd_name = (uint8_t *)app_ibrt_peripheral_heap_cmalloc(cmd_len);
        if(NULL == cmd_name){
            TRACE(0,"alloc memory fail\n");
            return;
        }
        memcpy(cmd_name, ibrt_cmd, cmd_len);
        //TRACE(2,"%s auto test command: %s\n",__func__,cmd_name);
        //TRACE(3,"%s rec parameter: %s, parameter len: %d\n",__func__,cmd_param,cmd_param_len);
        cmd_param_buf = (uint8_t *)app_ibrt_peripheral_heap_malloc(cmd_param_len);
        if(NULL == cmd_param_buf){
            TRACE(0,"alloc memory fail\n");
            return;
        }
        memcpy(cmd_param_buf, cmd_param, cmd_param_len);
        //TRACE(1,"auto test command memory copy parameter: %s\n",cmd_param_buf);
        APP_BT_CMD_MSG_BLOCK msg;
        msg.msg_body.message_id = 0xfd;
        msg.msg_body.message_Param0 = (uint32_t)cmd_name;
        msg.msg_body.message_Param1 = (uint32_t)cmd_param_buf;
        msg.msg_body.message_Param2 = (uint32_t)cmd_param_len;
        app_bt_cmd_mailbox_put(&msg);
#endif
    }
    else
    {
        app_bt_cmd_perform_test(ibrt_cmd, cmd_len);
    }
}

void app_ibrt_peripheral_run0(uint32_t ptr)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 0;
    msg.msg_body.message_ptr = ptr;
    app_bt_cmd_mailbox_put(&msg);
}

void app_ibrt_peripheral_run1(uint32_t ptr, uint32_t param0)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 1;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    app_bt_cmd_mailbox_put(&msg);
}

void app_ibrt_peripheral_run2(uint32_t ptr, uint32_t param0, uint32_t param1)
{
    APP_BT_CMD_MSG_BLOCK msg;
    msg.msg_body.message_id = 2;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    app_bt_cmd_mailbox_put(&msg);
}
#endif // IBRT


void app_bt_cmd_thread(void const *argument)
{
    while (1)
    {
        APP_BT_CMD_MSG_BLOCK *msg_p = NULL;
        if ((!app_bt_cmd_mailbox_get(&msg_p)) && (!argument))
        {
            switch (msg_p->msg_body.message_id)
            {
#ifdef IBRT
                case 0:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb0)(msg_p->msg_body.message_ptr))();
                    }
                    break;
                case 1:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb1)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0);
                    }
                    break;
                case 2:
                    if (msg_p->msg_body.message_ptr)
                    {
                        ((app_ibrt_peripheral_cb2)(msg_p->msg_body.message_ptr))((void *)msg_p->msg_body.message_Param0,
                                                                                 (void *)msg_p->msg_body.message_Param1);
                    }
                    break;
#if defined(IBRT_CORE_V2_ENABLE) && defined(HAL_TRACE_RX_ENABLE)
                case 0xfd:
                    app_ibrt_raw_ui_test_cmd_handler_with_param((unsigned char*)msg_p->msg_body.message_Param0,
                                                                 (unsigned char*)msg_p->msg_body.message_Param1,
                                                                 (uint32_t)msg_p->msg_body.message_Param2);
                    app_ibrt_peripheral_heap_free((uint8_t *)msg_p->msg_body.message_Param0);
                    app_ibrt_peripheral_heap_free((uint8_t *)msg_p->msg_body.message_Param1);
                    break;
#endif
                case 0xfe:
                    app_ibrt_peripheral_automate_test_handler((uint8_t*)msg_p->msg_body.message_Param0,
                                                              (uint32_t)msg_p->msg_body.message_Param1);
                    break;
#endif
                case 0xff: // bt common test
                    {
                        char best_cmd[40] = {0}; // max 19-byte cmd prefix match string or substr match string
                        uint32_t cmd_max_length = sizeof(APP_BT_CMD_MSG_BLOCK) - sizeof(msg_p->msg_body.message_id);
                        unsigned int i = 0;
                        memcpy(best_cmd, ((char *)msg_p) + sizeof(msg_p->msg_body.message_id), cmd_max_length);
                        for (; i < sizeof(best_cmd); i += 1)
                        {
                            if (best_cmd[i] == '\r' || best_cmd[i] == '\n')
                            {
                                best_cmd[i] = '\0';
                            }
                        }
                        TRACE(2, "%s process: %s\n", __func__, best_cmd);
                        app_bt_cmd_line_handler(best_cmd, strlen(best_cmd));
                    }
                    break;
                default:
                    break;
            }
            app_bt_cmd_mailbox_free(msg_p);
        }
    }
}

void app_bt_shutdown_test(void)
{
    app_shutdown();
}

void app_bt_flush_nv_test(void)
{
    nv_record_flash_flush();
}

void app_bt_pts_create_hf_channel(void)
{
    app_bt_reconnect_hfp_profile(app_bt_get_pts_address());
}

void app_bt_pts_create_av_channel(void)
{
    btif_pts_av_create_channel(app_bt_get_pts_address());
}

void app_bt_pts_create_ar_channel(void)
{
    btif_pts_ar_connect(app_bt_get_pts_address());
}

void app_bt_enable_tone_intrrupt_a2dp(void)
{
    app_bt_manager.config.a2dp_prompt_play_only_when_avrcp_play_received = false;
}

void app_bt_disable_tone_intrrupt_a2dp(void)
{
    app_bt_manager.config.a2dp_prompt_play_only_when_avrcp_play_received = true;
}

void app_bt_enable_a2dp_delay_prompt(void)
{
    app_bt_manager.config.a2dp_delay_prompt_play = true;
}

void app_bt_disable_a2dp_delay_prompt(void)
{
    app_bt_manager.config.a2dp_delay_prompt_play = false;
}

void app_bt_disable_a2dp_aac_codec_test(void)
{
    app_bt_a2dp_disable_aac_codec(true);
}

void app_bt_disable_a2dp_vendor_codec_test(void)
{
    app_bt_a2dp_disable_vendor_codec(true);
}

#if defined(BT_SOURCE)
void app_bt_pts_create_av_source_channel(void)
{
    bt_source_reconnect_a2dp_profile(app_bt_get_pts_address());
}

void app_bt_source_disconnect_link(void)
{
    app_bt_source_disconnect_all_connections(true);
}

extern void a2dp_source_pts_send_sbc_packet(void);
void app_bt_pts_source_send_sbc_packet(void)
{
    a2dp_source_pts_send_sbc_packet();
}

void app_bt_source_reconfig_codec_to_sbc(void)
{
    struct BT_SOURCE_DEVICE_T *source_device = app_bt_source_get_device(BT_SOURCE_DEVICE_ID_1);
    app_bt_a2dp_reconfig_to_sbc(source_device->base_device->a2dp_connected_stream);
}
#endif

#if defined(HFP_AG_ROLE)
void app_bt_pts_hf_ag_set_connectable_state(void)
{
    app_bt_source_set_connectable_state(true);
}

void app_bt_pts_create_hf_ag_channel(void)
{
    //app_bt_source_set_connectable_state(true);
    bt_source_reconnect_hfp_profile(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_create_audio_link(void)
{
    bt_source_create_audio_link(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_disc_audio_link(void)
{
    bt_source_disc_audio_link(app_bt_get_pts_address());
}

void app_bt_pts_hf_ag_send_mobile_signal_level(void)
{
    btif_ag_send_mobile_signal_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_send_mobile_signal_level_0(void)
{
    btif_ag_send_mobile_signal_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_service_status(void)
{
    btif_ag_send_service_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_send_service_status_0(void)
{
    btif_ag_send_service_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_call_active_status(void)
{
    btif_ag_send_call_active_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_hangup_call(void)
{
    btif_ag_send_call_active_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_callsetup_status(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,1);
}

void app_bt_pts_hf_ag_send_callsetup_status_0(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_callsetup_status_2(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,2);
}

void app_bt_pts_hf_ag_send_callsetup_status_3(void)
{
    btif_ag_send_callsetup_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_enable_roam(void)
{
    btif_ag_send_mobile_roam_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_disable_roam(void)
{
    btif_ag_send_mobile_roam_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_mobile_battery_level(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,3);
}

void app_bt_pts_hf_ag_send_full_battery_level(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,5);
}

void app_bt_pts_hf_ag_send_battery_level_0(void)
{
    btif_ag_send_mobile_battery_level(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_calling_ring(void)
{
    //const char* number = NULL;
    char number[] = "1234567";
    btif_ag_send_calling_ring(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,number);
}

void app_bt_pts_hf_ag_enable_inband_ring_tone(void)
{
    btif_ag_set_inband_ring_tone(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,true);
}

void app_bt_pts_hf_ag_place_a_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status();
    app_bt_pts_hf_ag_send_calling_ring();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_create_audio_link();
}

void app_bt_pts_hf_ag_ongoing_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_2();
    app_bt_pts_hf_ag_send_callsetup_status_3();
}

void app_bt_pts_hf_ag_ongoing_call_setup(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_2();
    app_bt_pts_hf_ag_create_audio_link();
    osDelay(100);
    app_bt_pts_hf_ag_send_callsetup_status_3();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
}

void app_bt_pts_hf_ag_answer_incoming_call(void)
{
    app_bt_pts_hf_ag_send_callsetup_status();
    app_bt_pts_hf_ag_send_call_active_status();
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_create_audio_link();
}

void app_bt_pts_hf_ag_clear_last_dial_number(void)
{
    btif_ag_set_last_dial_number(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,false);
}

void app_bt_pts_hf_ag_send_call_waiting_notification(void)
{
    char number[] = "7654321";
    btif_ag_send_call_waiting_notification(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,number);
}

void app_bt_pts_hf_ag_send_callheld_status(void)
{
    btif_ag_send_callheld_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,1);
}

void app_bt_pts_hf_ag_send_callheld_status_0(void)
{
    btif_ag_send_callheld_status(app_bt_source_find_device(app_bt_get_pts_address())->base_device->hf_channel,0);
}

void app_bt_pts_hf_ag_send_status_3_0_4_1(void)
{
    app_bt_pts_hf_ag_send_callsetup_status_0();
    app_bt_pts_hf_ag_send_callheld_status();
}

void app_bt_pts_hf_ag_set_pts_enable(void)
{
    app_bt_source_set_hfp_ag_pts_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecs_01(void)
{
    app_bt_source_set_hfp_ag_pts_esc_01_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecs_02(void)
{
    app_bt_source_set_hfp_ag_pts_esc_02_enable(true);
}

void app_bt_pts_hf_ag_set_pts_ecc(void)
{
    app_bt_source_set_hfp_ag_pts_ecc_enable(true);
}
#endif

#ifdef IBRT_V2_MULTIPOINT
void app_ibrt_set_access_mode_test(void)
{
    app_ibrt_if_set_access_mode(IBRT_BAM_GENERAL_ACCESSIBLE);
}

void app_ibrt_get_pnp_info_test(void)
{
#if defined(BTIF_DIP_DEVICE)
    ibrt_if_pnp_info* pnp_info = NULL;
    struct BT_DEVICE_T *curr_device = NULL;

    curr_device = app_bt_get_device(BT_DEVICE_ID_1);

    if (curr_device)
    {
        pnp_info = app_ibrt_if_get_pnp_info(&curr_device->remote);
    }

    if (pnp_info)
    {
        TRACE(4, "%s vendor_id %04x product_id %04x product_version %04x",
                __func__, pnp_info->vend_id, pnp_info->prod_id, pnp_info->prod_ver);
    }
    else
    {
        TRACE(1, "%s N/A", __func__);
    }
#endif
}

void app_ibrt_send_pause_test(void)
{
    app_ibrt_if_a2dp_send_pause(BT_DEVICE_ID_1);
}

void app_ibrt_send_play_test(void)
{
    app_ibrt_if_a2dp_send_play(BT_DEVICE_ID_1);
}

void app_ibrt_send_forward_test(void)
{
    app_ibrt_if_a2dp_send_forward(BT_DEVICE_ID_1);
}

void app_ibrt_send_backward_test(void)
{
    app_ibrt_if_a2dp_send_backward(BT_DEVICE_ID_1);
}

void app_ibrt_send_volume_up_test(void)
{
    app_ibrt_if_a2dp_send_volume_up(BT_DEVICE_ID_1);
}

void app_ibrt_send_volume_down_test(void)
{
    app_ibrt_if_a2dp_send_volume_down(BT_DEVICE_ID_1);
}

void app_ibrt_send_set_abs_volume_test(void)
{
    static uint8_t abs_vol = 80;
    app_ibrt_if_a2dp_send_set_abs_volume(BT_DEVICE_ID_1, abs_vol++);
}

void app_ibrt_adjust_local_volume_up_test(void)
{
    app_ibrt_if_set_local_volume_up();
}

void app_ibrt_adjust_local_volume_down_test(void)
{
    app_ibrt_if_set_local_volume_down();
}

void app_ibrt_hf_call_redial_test(void)
{
    app_ibrt_if_hf_call_redial(BT_DEVICE_ID_1);
}

void app_ibrt_hf_call_answer_test(void)
{
    app_ibrt_if_hf_call_answer(BT_DEVICE_ID_1);
}

void app_ibrt_hf_call_hangup_test(void)
{
    app_ibrt_if_hf_call_hangup(BT_DEVICE_ID_1);
}

void app_ibrt_a2dp_profile_conn_test(void)
{
    app_ibrt_if_connect_a2dp_profile(BT_DEVICE_ID_1);
}

void app_ibrt_a2dp_profile_disc_test(void)
{
    app_ibrt_if_disconnect_a2dp_profile(BT_DEVICE_ID_1);
}

void app_ibrt_send_tota_data_test(void)
{
#ifdef TOTA_v2
    app_ibrt_if_tota_printf("TOTA TEST");
#endif
}

void app_ibrt_connect_tws_test(void)
{
    app_ibrt_conn_tws_connect_request(false, 0);
}

void app_ibrt_connect_mobile_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
        (uint8_t)(bytes[0]&0xff),
        (uint8_t)(bytes[1]&0xff),
        (uint8_t)(bytes[2]&0xff),
        (uint8_t)(bytes[3]&0xff),
        (uint8_t)(bytes[4]&0xff),
        (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_remote_dev_connect_request(&addr, OUTGOING_CONNECTION_REQ, true, 0);
}

void app_ibrt_connect_ibrt_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
            (uint8_t)(bytes[0]&0xff),
            (uint8_t)(bytes[1]&0xff),
            (uint8_t)(bytes[2]&0xff),
            (uint8_t)(bytes[3]&0xff),
            (uint8_t)(bytes[4]&0xff),
            (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_connect_ibrt(&addr);
}

void app_ibrt_role_switch_test(const char* param, uint32_t len)
{
    int bytes[sizeof(bt_bdaddr_t)] = {0};

    if (len < 17)
    {
        TRACE(0, "%s wrong len %d '%s'", __func__, len, param);
        return;
    }

    sscanf(param, "%x:%x:%x:%x:%x:%x", bytes+0, bytes+1, bytes+2, bytes+3, bytes+4, bytes+5);

    bt_bdaddr_t addr = {{
            (uint8_t)(bytes[0]&0xff),
            (uint8_t)(bytes[1]&0xff),
            (uint8_t)(bytes[2]&0xff),
            (uint8_t)(bytes[3]&0xff),
            (uint8_t)(bytes[4]&0xff),
            (uint8_t)(bytes[5]&0xff)}};

    DUMP8("%02x ", addr.address, BD_ADDR_LEN);

    app_ibrt_conn_tws_role_switch(&addr);
}

#endif /* IBRT_V2_MULTIPOINT */

static const app_bt_cmd_handle_t app_bt_common_test_handle[]=
{
    {"bt_shutdown",             app_bt_shutdown_test},
    {"bt_flush_nv",             app_bt_flush_nv_test},
    {"hf_create_service_link",  app_bt_pts_create_hf_channel},
    {"hf_disc_service_link",    btif_pts_hf_disc_service_link},
    {"hf_create_audio_link",    btif_pts_hf_create_audio_link},
    {"hf_disc_audio_link",      btif_pts_hf_disc_audio_link},
    {"hf_answer_call",          btif_pts_hf_answer_call},
    {"hf_hangup_call",          btif_pts_hf_hangup_call},
    {"rfc_register",            btif_pts_rfc_register_channel},
    {"rfc_close",               btif_pts_rfc_close},
    {"av_create_channel",       app_bt_pts_create_av_channel},
    {"av_disc_channel",         btif_pts_av_disc_channel},
    {"ar_connect",              app_bt_pts_create_ar_channel},
    {"ar_disconnect",           btif_pts_ar_disconnect},
    {"ar_panel_play",           btif_pts_ar_panel_play},
    {"ar_panel_pause",          btif_pts_ar_panel_pause},
    {"ar_panel_stop",           btif_pts_ar_panel_stop},
    {"ar_panel_forward",        btif_pts_ar_panel_forward},
    {"ar_panel_backward",       btif_pts_ar_panel_backward},
    {"ar_volume_up",            btif_pts_ar_volume_up},
    {"ar_volume_down",          btif_pts_ar_volume_down},
    {"ar_volume_notify",        btif_pts_ar_volume_notify},
    {"ar_volume_change",        btif_pts_ar_volume_change},
    {"ar_set_absolute_volume",  btif_pts_ar_set_absolute_volume},
    {"av_en_tone_interrupt",    app_bt_enable_tone_intrrupt_a2dp},
    {"av_de_tone_interrupt",    app_bt_disable_tone_intrrupt_a2dp},
    {"av_en_delay_prompt",      app_bt_enable_a2dp_delay_prompt},
    {"av_de_delay_prompt",      app_bt_disable_a2dp_delay_prompt},
    {"av_de_aac_codec",         app_bt_disable_a2dp_aac_codec_test},
    {"av_de_vnd_codec",         app_bt_disable_a2dp_vendor_codec_test},
#if defined(BT_HID_DEVICE)
    {"hid_enter",               app_bt_hid_enter_shutter_mode},
    {"hid_exit",                app_bt_hid_exit_shutter_mode},
    {"hid_capture",             app_bt_hid_send_capture},
#endif
#if defined(BT_SOURCE)
    {"av_source_connect",       app_bt_pts_create_av_source_channel},
    {"source_disc_link",        app_bt_source_disconnect_link},
    {"reconfig_to_sbc",         app_bt_source_reconfig_codec_to_sbc},
    {"source_send_sbc_pkt",     app_bt_pts_source_send_sbc_packet},
    {"source_create_media_chnl",btif_pts_source_cretae_media_channel},
    {"source_send_close_cmd",   btif_pts_source_send_close_cmd},
#endif
#if defined(HFP_AG_ROLE)
    {"ag_set_connect_state",    app_bt_pts_hf_ag_set_connectable_state},
    {"ag_create_service_link",  app_bt_pts_create_hf_ag_channel},
    {"ag_create_audio_link",    app_bt_pts_hf_ag_create_audio_link},
    {"ag_disc_audio_link",      app_bt_pts_hf_ag_disc_audio_link},
    {"ag_send_mobile_signal",   app_bt_pts_hf_ag_send_mobile_signal_level},
    {"ag_send_mobile_signal_0", app_bt_pts_hf_ag_send_mobile_signal_level_0},
    {"ag_send_service_status",  app_bt_pts_hf_ag_send_service_status},
    {"ag_send_service_status_0",app_bt_pts_hf_ag_send_service_status_0},
    {"ag_send_callsetup_status",app_bt_pts_hf_ag_send_callsetup_status},
    {"ag_send_callsetup_status_0",app_bt_pts_hf_ag_send_callsetup_status_0},
    {"ag_send_callactive_status",app_bt_pts_hf_ag_send_call_active_status},
    {"ag_send_hangup_call",     app_bt_pts_hf_ag_hangup_call},
    {"ag_send_enable_roam",     app_bt_pts_hf_ag_enable_roam},
    {"ag_send_disable_roam",    app_bt_pts_hf_ag_disable_roam},
    {"ag_send_batt_level",      app_bt_pts_hf_ag_send_mobile_battery_level},
    {"ag_send_full_batt_level", app_bt_pts_hf_ag_send_full_battery_level},
    {"ag_send_batt_level_0",    app_bt_pts_hf_ag_send_battery_level_0},
    {"ag_send_calling_ring",    app_bt_pts_hf_ag_send_calling_ring},
    {"ag_enable_inband_ring",   app_bt_pts_hf_ag_enable_inband_ring_tone},
    {"ag_place_a_call",         app_bt_pts_hf_ag_place_a_call},
    {"ag_ongoing_call",         app_bt_pts_hf_ag_ongoing_call},
    {"ag_ongoing_call_setup",   app_bt_pts_hf_ag_ongoing_call_setup},
    {"ag_clear_dial_num",       app_bt_pts_hf_ag_clear_last_dial_number},
    {"ag_send_ccwa",            app_bt_pts_hf_ag_send_call_waiting_notification},
    {"ag_send_callheld_status", app_bt_pts_hf_ag_send_callheld_status},
    {"ag_send_callheld_status_0",app_bt_pts_hf_ag_send_callheld_status_0},
    {"ag_send_status_3_0_4_1",  app_bt_pts_hf_ag_send_status_3_0_4_1},
    {"ag_set_pts_enable",       app_bt_pts_hf_ag_set_pts_enable},
    {"ag_answer_incoming_call", app_bt_pts_hf_ag_answer_incoming_call},
    {"ag_set_pts_ecs_01",       app_bt_pts_hf_ag_set_pts_ecs_01},
    {"ag_set_pts_ecs_02",       app_bt_pts_hf_ag_set_pts_ecs_02},
    {"ag_set_pts_ecc",          app_bt_pts_hf_ag_set_pts_ecc},
#endif
#if defined(BT_PBAP_SUPPORT)
    {"pb_connect",              app_bt_pts_pbap_create_channel},
    {"pb_disconnect",           app_bt_pts_pbap_disconnect_channel},
    {"pb_obex_disc",            app_bt_pts_pbap_send_obex_disc_req},
    {"pb_get_req",              app_bt_pts_pbap_send_obex_get_req},
    {"pb_auth_req",             app_bt_pts_pbap_send_auth_conn_req},
    {"pb_abort",                app_bt_pts_pbap_send_abort},
    {"pb_pull_pb",              app_bt_pts_pbap_pull_phonebook_pb},
    {"pb_pull_size",            app_bt_pts_pbap_get_phonebook_size},
    {"pb_to_root",              app_bt_pts_pbap_set_path_to_root},
    {"pb_to_parent",            app_bt_pts_pbap_set_path_to_parent},
    {"pb_to_telecom",           app_bt_pts_pbap_enter_path_to_telecom},
    {"pb_list_pb",              app_bt_pts_pbap_list_phonebook_pb},
    {"pb_list_size",            app_bt_pts_pbap_list_phonebook_pb_size},
    {"pb_entry_n_tel",          app_bt_pts_pbap_pull_pb_entry_n_tel},
    {"pb_entry_uid",            app_bt_pts_pbap_pull_uid_entry},
#endif
#ifdef IBRT_V2_MULTIPOINT
    {"ib_set_access_mode",      app_ibrt_set_access_mode_test},
    {"ib_enable_bluetooth",     app_ibrt_if_enable_bluetooth},
    {"ib_disable_bluetooth",    app_ibrt_if_disable_bluetooth},
    {"ib_get_pnp_info",         app_ibrt_get_pnp_info_test},
    {"ib_send_pause",           app_ibrt_send_pause_test},
    {"ib_send_play",            app_ibrt_send_play_test},
    {"ib_send_forward",         app_ibrt_send_forward_test},
    {"ib_send_backward",        app_ibrt_send_backward_test},
    {"ib_send_volumeup",        app_ibrt_send_volume_up_test},
    {"ib_send_volumedn",        app_ibrt_send_volume_down_test},
    {"ib_send_setabsvol",       app_ibrt_send_set_abs_volume_test},
    {"ib_adj_local_volup",      app_ibrt_adjust_local_volume_up_test},
    {"ib_adj_local_voldn",      app_ibrt_adjust_local_volume_down_test},
    {"ib_call_redial",          app_ibrt_hf_call_redial_test},
    {"ib_call_answer",          app_ibrt_hf_call_answer_test},
    {"ib_call_hangup",          app_ibrt_hf_call_hangup_test},
    {"ib_switch_sco",           app_ibrt_if_switch_streaming_sco},
    {"ib_switch_a2dp",          app_ibrt_if_switch_streaming_a2dp},
    {"ib_conn_a2dp",            app_ibrt_a2dp_profile_conn_test},
    {"ib_disc_a2dp",            app_ibrt_a2dp_profile_disc_test},
    {"ib_tota_send",            app_ibrt_send_tota_data_test},
    {"ib_conn_tws",             app_ibrt_connect_tws_test},
    {"ib_a2dp_recheck",         app_bt_audio_recheck_a2dp_streaming},
#endif
};

static const app_bt_cmd_handle_with_parm_t app_bt_common_test_with_param_handle[]=
{
#if defined(BT_PBAP_SUPPORT)
    {"pb_to",                   app_bt_pts_pbap_set_path_to},
    {"pb_list",                 app_bt_pts_pbap_list_phonebook},
    {"pb_pull",                 app_bt_pts_pbap_pull_phonebook},
    {"pb_reset",                app_bt_pts_pbap_pull_reset_missed_call},
    {"pb_vcsel",                app_bt_pts_pbap_pull_vcard_select},
    {"pb_list_reset",           app_bt_pts_pbap_list_reset_missed_call},
    {"pb_list_vcsel",           app_bt_pts_pbap_list_vcard_select},
    {"pb_list_vcsch",           app_bt_pts_pbap_list_vcard_search},
#endif
#ifdef IBRT_V2_MULTIPOINT
    {"ib_c_m",                  app_ibrt_connect_mobile_test},
    {"ib_c_i",                  app_ibrt_connect_ibrt_test},
    {"ib_r_s",                  app_ibrt_role_switch_test},
#endif
};

void app_bt_add_common_test_table(void)
{
    app_bt_cmd_add_test_table(app_bt_common_test_handle, ARRAY_SIZE(app_bt_common_test_handle));
    app_bt_cmd_add_test_table_with_param(app_bt_common_test_with_param_handle, ARRAY_SIZE(app_bt_common_test_with_param_handle));
}

void app_bt_cmd_init(void)
{
    if (app_bt_cmd_mailbox_init())
        return;

    app_bt_cmd_tid = osThreadCreate(osThread(app_bt_cmd_thread), NULL);
    if (app_bt_cmd_tid == NULL)  {
        TRACE(0,"Failed to Create app_bt_cmd_thread\n");
        return;
    }

    memset(g_bt_cmd_tables, 0, sizeof(g_bt_cmd_tables));
    memset(g_bt_cmd_with_parm_tables, 0, sizeof(g_bt_cmd_with_parm_tables));

#ifdef IBRT
#if defined(BES_AUTOMATE_TEST) || defined(HAL_TRACE_RX_ENABLE)
    app_ibrt_peripheral_heap_init();
#endif
#ifdef IBRT_V1_LEGACY
    app_ibrt_ui_add_test_cmd_table();
#endif
#ifdef IBRT_V2_MULTIPOINT
    app_ibrt_ui_v2_add_test_cmd_table();
#endif
#endif

    app_bt_add_common_test_table();

    return;
}

#endif // APP_TRACE_RX_ENABLE || IBRT

