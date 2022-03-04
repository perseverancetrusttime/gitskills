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
#include "string.h"
#include "hal_trace.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_ota_cmd.h"

#if BES_OTA
#include "ota_control.h"
#include "ota_spp.h"
#include "ota_bes.h"
#include "nvrecord_ble.h"
#endif

#ifdef __GMA_OTA_TWS__
#include "app_gma_ota_tws.h"
#include "app_gma_ota.h"
#include "app_gma_cmd_handler.h"
#endif

#ifdef AI_OTA
#include "ota_common.h"
#endif

#if defined(IBRT)
#if defined(BES_OTA)

static void app_ibrt_ota_get_version_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_get_version_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_get_version_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_get_version_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_select_side_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_select_side_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_select_side_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_select_side_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_bp_check_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_bp_check_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_bp_check_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_bp_check_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_start_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_start_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_start_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_start_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_config_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_config_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_config_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_config_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_segment_crc_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_segment_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_segment_crc_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_segment_crc_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_crc_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_image_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_crc_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_crc_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_image_overwrite_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_image_overwrite_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_overwrite_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_overwrite_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_set_user_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_set_user_cmd_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_set_user_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_set_user_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_get_ota_version_cmd_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_ota_get_ota_version_cmd_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_get_ota_version_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_get_ota_version_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_start_role_switch_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_start_role_switch_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_mobile_disconnected_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_mobile_disconnected_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

static void app_ibrt_ota_update_rd_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_update_rd_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

static void app_ibrt_ota_image_buff_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_image_buff_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

#ifdef __GMA_OTA_TWS__
static void app_ibrt_gmaOta_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_gmaOta_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_gmaOta_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_gmaOta_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif
#ifdef BISTO_ENABLED
static void app_ibrt_common_ota_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_common_ota_cmd_received(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

static const app_tws_cmd_instance_t g_ibrt_ota_tws_cmd_handler_table[]=
{
#ifdef BES_OTA
    {
        IBRT_OTA_TWS_GET_VERSION_CMD,                               "OTA_GET_VERSION",
        app_ibrt_ota_get_version_cmd_send,
        app_ibrt_ota_get_version_cmd_send_handler,                  RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_get_version_cmd_send_rsp_timeout_handler,      app_ibrt_ota_get_version_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_SELECT_SIDE_CMD,                               "OTA_SELECT_SIDE",
        app_ibrt_ota_select_side_cmd_send,
        app_ibrt_ota_select_side_cmd_send_handler,                  RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_select_side_cmd_send_rsp_timeout_handler,      app_ibrt_ota_select_side_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_BP_CHECK_CMD,                                  "OTA_BP_CHECK",//Break-point
        app_ibrt_ota_bp_check_cmd_send,
        app_ibrt_ota_bp_check_cmd_send_handler,                     RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_bp_check_cmd_send_rsp_timeout_handler,         app_ibrt_ota_bp_check_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_START_OTA_CMD,                                 "OTA_START",
        app_ibrt_ota_start_cmd_send,
        app_ibrt_ota_start_cmd_send_handler,                        RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_start_cmd_send_rsp_timeout_handler,            app_ibrt_ota_start_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_OTA_CONFIG_CMD,                                "OTA_CONFIG",
        app_ibrt_ota_config_cmd_send,
        app_ibrt_ota_config_cmd_send_handler,                       RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_config_cmd_send_rsp_timeout_handler,           app_ibrt_ota_config_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_SEGMENT_CRC_CMD,                               "OTA_SEGMENT_CRC",
        app_ibrt_ota_segment_crc_cmd_send,
        app_ibrt_ota_segment_crc_cmd_send_handler,                  RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_segment_crc_cmd_send_rsp_timeout_handler,      app_ibrt_ota_segment_crc_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_IMAGE_CRC_CMD,                                 "OTA_IMAGE_CRC",
        app_ibrt_ota_image_crc_cmd_send,
        app_ibrt_ota_image_crc_cmd_send_handler,                    RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_image_crc_cmd_send_rsp_timeout_handler,        app_ibrt_ota_image_crc_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD,                           "OTA_IMAGE_OVERWRITE",
        app_ibrt_ota_image_overwrite_cmd_send,
        app_ibrt_ota_image_overwrite_cmd_send_handler,              RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_image_overwrite_cmd_send_rsp_timeout_handler,  app_ibrt_ota_image_overwrite_cmd_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_SET_USER_CMD,                                  "OTA_SET_USER",
        app_ibrt_ota_set_user_cmd_send,
        app_ibrt_ota_set_user_cmd_handler,                          RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_set_user_send_rsp_timeout_handler,             app_ibrt_ota_set_user_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_GET_OTA_VERSION_CMD,                           "OTA_GET_OTA_VERSION",
        app_ibrt_ota_get_ota_version_cmd_send,
        app_ibrt_ota_get_ota_version_cmd_handler,                   RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_get_ota_version_send_rsp_timeout_handler,      app_ibrt_ota_get_ota_version_send_rsp_handler
    },
    {
        IBRT_OTA_TWS_ROLE_SWITCH_CMD,                              "OTA_ROLE_SWITCH",
        app_ibrt_ota_start_role_switch_cmd_send,
        app_ibrt_ota_start_role_switch_cmd_send_handler,           0,
        app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null,      app_ibrt_ota_image_buff_cmd_rsp_handler_null
    },
    {
        IBRT_OTA_TWS_MOBILE_DISC_CMD,                               "OTA_MOBILE_DISC",
        app_ibrt_ota_mobile_disconnected_cmd_send,
        app_ibrt_ota_mobile_disconnected_cmd_send_handler,              0,
        app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null,       app_ibrt_ota_image_buff_cmd_rsp_handler_null
    },
    {
        IBRT_OTA_TWS_UPDATE_RD_CMD,                                 "OTA_UPDATE_RD",
        app_ibrt_ota_update_rd_cmd_send,
        app_ibrt_ota_update_rd_cmd_send_handler,                      0,
        app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null,       app_ibrt_ota_image_buff_cmd_rsp_handler_null
    },
#endif
    {
        IBRT_OTA_TWS_IMAGE_BUFF,                                    "OTA_IMAGE_BUFF",
        app_ibrt_ota_image_buff_cmd_send,
        app_ibrt_ota_image_buff_cmd_send_handler,                   0,
        app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null,       app_ibrt_ota_image_buff_cmd_rsp_handler_null
    },
#ifdef __GMA_OTA_TWS__
    {
        IBRT_GMA_OTA,                                               "GMA_OTA_START",
        app_ibrt_gmaOta_cmd_send,
        app_ibrt_gmaOta_cmd_send_handler,                        RSP_TIMEOUT_DEFAULT,
        app_ibrt_gmaOta_cmd_send_rsp_timeout_handler,            app_ibrt_gmaOta_cmd_send_rsp_handler
    },
/*    {
        IBRT_IMAGE_CHECK_GMA_OTA,                                   "GMA_OTA_IMAGE_CHECK",
        app_ibrt_ota_start_cmd_send,
        app_ibrt_ota_start_cmd_send_handler,                        RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_start_cmd_send_rsp_timeout_handler,            app_ibrt_ota_start_cmd_send_rsp_handler
    },
    {
        IBRT_CRC_CHECK_GMA_OTA,                                     "GMA_OTA_CRC_CHECK",
        app_ibrt_ota_start_cmd_send,
        app_ibrt_ota_start_cmd_send_handler,                        RSP_TIMEOUT_DEFAULT,
        app_ibrt_ota_start_cmd_send_rsp_timeout_handler,            app_ibrt_ota_start_cmd_send_rsp_handler
    },*/
#endif
#ifdef BISTO_ENABLED
    {
        IBRT_COMMON_OTA,                                             "COMMON_OTA_CMD",
        app_ibrt_common_ota_cmd_send,
        app_ibrt_common_ota_cmd_received,                            0,
        app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null,       app_ibrt_ota_image_buff_cmd_rsp_handler_null
    },
#endif
};

int app_ibrt_ota_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size)
{
    *cmd_tbl = (void *)&g_ibrt_ota_tws_cmd_handler_table;
    *cmd_size = ARRAY_SIZE(g_ibrt_ota_tws_cmd_handler_table);
    return 0;
}

#ifdef BES_OTA
extern OTA_IBRT_TWS_CMD_EXECUTED_RESULT_FROM_SLAVE_T receivedResultAlreadyProcessedBySlave;

uint32_t ibrt_ota_cmd_type = 0;
uint32_t twsBreakPoint = 0;
uint8_t errOtaCode = 0;
static uint8_t get_version_resend_num = RESEND_TIME;
static uint8_t select_side_resend_num = RESEND_TIME;
static uint8_t bp_check_resend_num = RESEND_TIME;
static uint8_t ota_start_resend_num = RESEND_TIME;
static uint8_t ota_config_resend_num = RESEND_TIME;
static uint8_t segment_crc_resend_num = RESEND_TIME;
static uint8_t image_crc_resend_num = RESEND_TIME;
static uint8_t image_overwrite_resend_num = RESEND_TIME;
static uint8_t set_user_resend_num = RESEND_TIME;
static uint8_t get_ota_version_resend_num = RESEND_TIME;

static void app_ibrt_ota_cache_slave_info(uint8_t typeCode, uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    memcpy(&receivedResultAlreadyProcessedBySlave.typeCode, &typeCode, sizeof(typeCode));
    memcpy(&receivedResultAlreadyProcessedBySlave.rsp_seq, &rsp_seq, sizeof(rsp_seq));
    memcpy(&receivedResultAlreadyProcessedBySlave.length, &length, sizeof(length));
    memcpy(receivedResultAlreadyProcessedBySlave.p_buff, p_buff, length);
}

static void app_ibrt_ota_get_version_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_GET_VERSION_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_get_version_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_VERSION)
    {
        tws_ctrl_send_rsp(IBRT_OTA_TWS_GET_VERSION_CMD, rsp_seq, p_buff, length);
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_VERSION, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_get_version_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    get_version_resend_num = RESEND_TIME;
    ibrt_ota_send_version_rsp();
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_get_version_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(get_version_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_GET_VERSION_CMD, p_buff, length);
        get_version_resend_num--;
    }
    else
    {
        get_version_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, get_version_resend_num);
}

static void app_ibrt_ota_select_side_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_SELECT_SIDE_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_select_side_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_SIDE_SELECTION)
    {
        tws_ctrl_send_rsp(IBRT_OTA_TWS_SELECT_SIDE_CMD, rsp_seq, p_buff, length);
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_SIDE_SELECTION, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_select_side_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    select_side_resend_num = RESEND_TIME;
    if(1 == p_buff[1])
    {
        ota_control_side_selection_rsp(true);
    }
    else
    {
        ota_control_side_selection_rsp(false);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_select_side_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(select_side_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_SELECT_SIDE_CMD, p_buff, length);
        select_side_resend_num--;
    }
    else
    {
        select_side_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, select_side_resend_num);
}

static void app_ibrt_ota_bp_check_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_BP_CHECK_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_bp_check_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    OTA_LEGACY_PACKET_T* tRsp = (OTA_LEGACY_PACKET_T *)p_buff;
    if(ibrt_ota_cmd_type == OTA_RSP_RESUME_VERIFY)
    {
        TRACE(2,"breakPoint:%x, received breakPoint:%x", twsBreakPoint, tRsp->payload.rspResumeBreakpoint.breakPoint);
        if(twsBreakPoint == tRsp->payload.rspResumeBreakpoint.breakPoint)
        {
            if(twsBreakPoint == 0)
            {
                TRACE(0,"reset random code:");
                DUMP8("%02x ", (uint8_t *)tRsp, length);
                ota_randomCode_log((uint8_t *)&tRsp->payload.rspResumeBreakpoint.randomCode);
            }
            ibrt_ota_cmd_type = 0;
        }
        else
        {
            TRACE(1,"%s tws break-point is not synchronized", __func__);
            ota_upgradeLog_destroy();
            ota_randomCode_log((uint8_t *)&tRsp->payload.rspResumeBreakpoint.randomCode);
            ota_control_reset_env(false);
            ota_status_change(true);
            tRsp->payload.rspResumeBreakpoint.breakPoint = 0xFFFFFFFF;
            twsBreakPoint = 0;
        }
        tws_ctrl_send_rsp(IBRT_OTA_TWS_BP_CHECK_CMD, rsp_seq, (uint8_t *)tRsp, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_bp_check_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    bp_check_resend_num = RESEND_TIME;
    OTA_LEGACY_PACKET_T* tRsp = (OTA_LEGACY_PACKET_T *)p_buff;
    if(tRsp->payload.rspResumeBreakpoint.breakPoint == 0xFFFFFFFF)
    {
        TRACE(1,"%s tws break-point is not synchronized", __func__);
        ota_upgradeLog_destroy();
        ota_randomCode_log((uint8_t *)&tRsp->payload.rspResumeBreakpoint.randomCode);
        ota_control_reset_env(false);
        ota_status_change(true);
        tRsp->payload.rspResumeBreakpoint.breakPoint = 0;
        tws_ctrl_send_cmd(IBRT_OTA_TWS_BP_CHECK_CMD, (uint8_t *)tRsp, length);
    }
    else
    {
        ota_control_send_resume_response(tRsp->payload.rspResumeBreakpoint.breakPoint,
                                    tRsp->payload.rspResumeBreakpoint.randomCode);
    }
    TRACE(1,"%s", __func__);
}
static void app_ibrt_ota_bp_check_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(bp_check_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_BP_CHECK_CMD, p_buff, length);
        bp_check_resend_num--;
    }
    else
    {
        bp_check_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, bp_check_resend_num);
}

static void app_ibrt_ota_start_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_START_OTA_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_start_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_START)
    {
        ibrt_ota_cmd_type = 0;
        tws_ctrl_send_rsp(IBRT_OTA_TWS_START_OTA_CMD, rsp_seq, p_buff, length);
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_START, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_start_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ota_start_resend_num = RESEND_TIME;
    ibrt_ota_send_start_response(*p_buff);
    TRACE(1,"%s", __func__);
}
static void app_ibrt_ota_start_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ota_start_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_START_OTA_CMD, p_buff, length);
        ota_start_resend_num--;
    }
    else
    {
        ota_start_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, ota_start_resend_num);
}

static void app_ibrt_ota_config_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_config_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_CONFIG)
    {
        if(*p_buff == 1 && errOtaCode == 1)
        {
            tws_ctrl_send_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, rsp_seq, p_buff, length);
        }
        else if(*p_buff == OTA_RESULT_ERR_IMAGE_SIZE || errOtaCode == OTA_RESULT_ERR_IMAGE_SIZE)
        {
            uint8_t errImageSize = OTA_RESULT_ERR_IMAGE_SIZE;
            tws_ctrl_send_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, rsp_seq, (uint8_t *)&errImageSize, length);
        }
        else if(*p_buff == OTA_RESULT_ERR_BREAKPOINT || errOtaCode == OTA_RESULT_ERR_BREAKPOINT)
        {
            uint8_t errBreakPoint = OTA_RESULT_ERR_BREAKPOINT;
            ota_upgradeLog_destroy();
            tws_ctrl_send_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, rsp_seq, (uint8_t *)&errBreakPoint, length);
        }
        else
        {
            tws_ctrl_send_rsp(IBRT_OTA_TWS_OTA_CONFIG_CMD, rsp_seq, p_buff, length);
        }
        errOtaCode = 0;
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_CONFIG, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_config_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ota_config_resend_num = RESEND_TIME;

    if(*p_buff == 1)
    {
        ibrt_ota_send_configuration_response(true);
    }
    else if(*p_buff == OTA_RESULT_ERR_IMAGE_SIZE || errOtaCode == OTA_RESULT_ERR_IMAGE_SIZE)
    {
        ibrt_ota_send_result_response(OTA_RESULT_ERR_IMAGE_SIZE);
    }
    else if(*p_buff == OTA_RESULT_ERR_BREAKPOINT || errOtaCode == OTA_RESULT_ERR_BREAKPOINT)
    {
        ota_upgradeLog_destroy();
        ibrt_ota_send_result_response(OTA_RESULT_ERR_BREAKPOINT);
    }
    else
    {
        ibrt_ota_send_configuration_response(false);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_config_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ota_config_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_OTA_CONFIG_CMD, p_buff, length);
        ota_config_resend_num--;
    }
    else
    {
        ota_config_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, ota_config_resend_num);
}

static void app_ibrt_ota_segment_crc_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_segment_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_SEGMENT_VERIFY)
    {
        if(*p_buff == 1 && errOtaCode == 1)
        {
            ota_update_flash_offset_after_segment_crc(true);
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, rsp_seq, p_buff, length);
        }
        else if(*p_buff == OTA_RESULT_ERR_SEG_VERIFY || errOtaCode == OTA_RESULT_ERR_SEG_VERIFY)
        {
            uint8_t errSegVerify = OTA_RESULT_ERR_SEG_VERIFY;
            ota_upgradeLog_destroy();
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, rsp_seq, (uint8_t *)&errSegVerify, length);
        }
        else if(*p_buff == OTA_RESULT_ERR_FLASH_OFFSET || errOtaCode == OTA_RESULT_ERR_FLASH_OFFSET)
        {
            uint8_t errFlashOffset = OTA_RESULT_ERR_FLASH_OFFSET;
            ota_upgradeLog_destroy();
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, rsp_seq, (uint8_t *)&errFlashOffset, length);
        }
        else
        {
            ota_update_flash_offset_after_segment_crc(false);
            *p_buff = OTA_ONE_BUD_MESSAGE_ERR;
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SEGMENT_CRC_CMD, rsp_seq, p_buff, length);
        }
        errOtaCode = 0;
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_SEGMENT_VERIFY, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_segment_crc_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    segment_crc_resend_num = RESEND_TIME;
    if(*p_buff == 1)
    {
        ibrt_ota_send_segment_verification_response(true);
        ota_update_flash_offset_after_segment_crc(true);
    }
    else if(*p_buff == OTA_ONE_BUD_MESSAGE_ERR)
    {
        ota_update_flash_offset_after_segment_crc(false);
        ibrt_ota_send_segment_verification_response(false);
    }
    else if(*p_buff == OTA_RESULT_ERR_SEG_VERIFY)
    {
        ota_upgradeLog_destroy();
        ibrt_ota_send_result_response(OTA_RESULT_ERR_SEG_VERIFY);
    }
    else if(*p_buff == OTA_RESULT_ERR_FLASH_OFFSET)
    {
        ota_upgradeLog_destroy();
        ibrt_ota_send_result_response(OTA_RESULT_ERR_FLASH_OFFSET);
    }

    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_segment_crc_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(segment_crc_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_SEGMENT_CRC_CMD, p_buff, length);
        segment_crc_resend_num--;
    }
    else
    {
        segment_crc_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, segment_crc_resend_num);
}

static void app_ibrt_ota_image_crc_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_IMAGE_CRC_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_image_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_RESULT)
    {
        if(*p_buff == 1 && errOtaCode == 1)
        {
            tws_ctrl_send_rsp(IBRT_OTA_TWS_IMAGE_CRC_CMD, rsp_seq, p_buff, length);
            errOtaCode = 0;
        }
        else
        {
            uint8_t crcErr = 0;
            if (ota_is_in_progress())
            {
                ota_upgradeLog_destroy();
                ota_control_reset_env(true);
            }
            tws_ctrl_send_rsp(IBRT_OTA_TWS_IMAGE_CRC_CMD, rsp_seq, (uint8_t *)&crcErr, length);
        }
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_RESULT, rsp_seq, p_buff, length);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_image_crc_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    image_crc_resend_num = RESEND_TIME;
    if(*p_buff == 1)
    {
        ibrt_ota_send_result_response(true);
    }
    if(*p_buff == 0)
    {
        if (ota_is_in_progress())
        {
            ibrt_ota_send_result_response(false);
            ota_upgradeLog_destroy();
            ota_control_reset_env(true);
        }
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_image_crc_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(image_crc_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_IMAGE_CRC_CMD, p_buff, length);
        image_crc_resend_num--;
    }
    else
    {
        image_crc_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, image_crc_resend_num);
}

static void app_ibrt_ota_image_overwrite_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_image_overwrite_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_IMAGE_APPLY)
    {
        if(*p_buff == 1 && errOtaCode == 1)
        {
            int ret = tws_ctrl_send_rsp(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, rsp_seq, p_buff, length);
            if(0 == ret)
            {
                ota_apply();
            }
        }
        else
        {
            uint8_t cantOverWrite = 0;
            tws_ctrl_send_rsp(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, rsp_seq, (uint8_t *)&cantOverWrite, length);
        }
        errOtaCode = 0;
        ibrt_ota_cmd_type = 0;
        Bes_exit_ota_state();
    }
}

static void app_ibrt_ota_image_overwrite_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    image_overwrite_resend_num = RESEND_TIME;
    if(*p_buff == 1)
    {
        ota_control_image_apply_rsp(true);
        ota_apply();
    }
    else
    {
        ota_control_image_apply_rsp(false);
    }
    Bes_exit_ota_state();
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_image_overwrite_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(image_overwrite_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD, p_buff, length);
        image_overwrite_resend_num--;
    }
    else
    {
        image_overwrite_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, image_overwrite_resend_num);
}

static void app_ibrt_ota_set_user_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_SET_USER_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_set_user_cmd_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(3,"%s received user:%d, errOtaCode:%d", __func__, *p_buff, errOtaCode);
    if(ibrt_ota_cmd_type == OTA_RSP_SET_USER)
    {
        if(*p_buff != 0 && errOtaCode == 1)
        {
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SET_USER_CMD, rsp_seq, p_buff, length);
        }
        else
        {
            uint8_t userSet = 0;
            tws_ctrl_send_rsp(IBRT_OTA_TWS_SET_USER_CMD, rsp_seq, (uint8_t *)&userSet, length);
        }
        errOtaCode = 0;
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_SET_USER, rsp_seq, p_buff, length);
    }
}

static void app_ibrt_ota_set_user_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(set_user_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_SET_USER_CMD, p_buff, length);
        set_user_resend_num--;
    }
    else
    {
        set_user_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, set_user_resend_num);
}

static void app_ibrt_ota_set_user_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    set_user_resend_num = RESEND_TIME;

    if(*p_buff != 0)
    {
        ibrt_ota_send_set_user_response(true);
    }
    else
    {
        ibrt_ota_send_set_user_response(false);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_get_ota_version_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_OTA_TWS_GET_OTA_VERSION_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

void app_ibrt_ota_get_ota_version_cmd_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(ibrt_ota_cmd_type == OTA_RSP_GET_OTA_VERSION)
    {
        tws_ctrl_send_rsp(IBRT_OTA_TWS_GET_OTA_VERSION_CMD, rsp_seq, p_buff, length);
        ibrt_ota_cmd_type = 0;
    }
    else
    {
        app_ibrt_ota_cache_slave_info(OTA_RSP_GET_OTA_VERSION, rsp_seq, p_buff, length);
        TRACE(1,"Slave_typecode = %d",receivedResultAlreadyProcessedBySlave.typeCode);
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_get_ota_version_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(get_ota_version_resend_num > 0)
    {
        tws_ctrl_send_cmd(IBRT_OTA_TWS_GET_OTA_VERSION_CMD, p_buff, length);
        get_ota_version_resend_num--;
    }
    else
    {
        get_ota_version_resend_num = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, get_ota_version_resend_num);
}

static void app_ibrt_ota_get_ota_version_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    get_ota_version_resend_num = RESEND_TIME;
    ibrt_ota_send_ota_version_rsp(p_buff, length);
    TRACE(1,"%s", __func__);
}
#endif

static void app_ibrt_ota_image_buff_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_IMAGE_BUFF, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_image_buff_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
#ifdef __GMA_OTA_TWS__
    app_gma_ota_tws_peer_data_handler(p_buff, length);
#elif defined(BES_OTA)
    ota_control_handle_received_data(p_buff, true, length);
#endif
    TRACE(1,"%s", __func__);
}

#ifdef __GMA_OTA_TWS__
static uint8_t resend = RESEND_TIME;
uint8_t gma_start_ota = 0;
bool gma_crc = false;
static void app_ibrt_gmaOta_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_with_rsp(IBRT_GMA_OTA, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_gmaOta_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(gma_start_ota == GMA_OP_OTA_UPDATE_RSP)
    {
        app_gma_send_command(GMA_OP_OTA_UPDATE_RSP, p_buff, (uint32_t)length, false,0);
        gma_start_ota = 0;
        tws_ctrl_send_rsp(IBRT_GMA_OTA, rsp_seq, p_buff, length);
    }
    else if(gma_start_ota == GMA_OP_OTA_PKG_RECV_RSP)
    {
        GMA_OTA_REPORT_CMD_T *pRsp = (GMA_OTA_REPORT_CMD_T *)p_buff;
        if(pRsp->total_received_size == app_gma_ota_get_already_received_size())
        {
            app_gma_send_command(GMA_OP_OTA_PKG_RECV_RSP, p_buff, (uint32_t)length, false,0);
        }
        tws_ctrl_send_rsp(IBRT_GMA_OTA, rsp_seq, p_buff, length);
        gma_start_ota = 0;
    }
    else if(gma_start_ota == GMA_OP_OTA_COMP_CHECK_RSP)
    {
        if(gma_crc == *p_buff)
        {
            app_gma_send_command(GMA_OP_OTA_COMP_CHECK_RSP, p_buff, (uint32_t)length, false,0);
            app_gma_ota_reboot();
            *p_buff = GMA_OP_OTA_COMP_CHECK_RSP;
            tws_ctrl_send_rsp(IBRT_GMA_OTA, rsp_seq, p_buff, length);
        }
        else
        {
            uint8_t crc_result = 0;
            app_gma_send_command(GMA_OP_OTA_COMP_CHECK_RSP, (uint8_t *)&crc_result, (uint32_t)length, false,0);
            tws_ctrl_send_rsp(IBRT_GMA_OTA, rsp_seq, (uint8_t *)&crc_result, length);
        }
        gma_start_ota = 0;
    }
    TRACE(1,"%s", __func__);
}

static void app_ibrt_gmaOta_cmd_send_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    resend = RESEND_TIME;
    if(*p_buff == GMA_OP_OTA_COMP_CHECK_RSP && length == 1)
    {
        app_gma_ota_reboot();
    }
    TRACE(1,"%s", __func__);
}
static void app_ibrt_gmaOta_cmd_send_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if(resend > 0)
    {
        tws_ctrl_send_cmd(IBRT_GMA_OTA, p_buff, length);
        resend--;
    }
    else
    {
        resend = RESEND_TIME;
    }
    TRACE(2,"%s, %d", __func__, resend);
}
#endif

#ifdef AI_OTA
static void app_ibrt_common_ota_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_COMMON_OTA, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_common_ota_cmd_received(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ota_common_on_relay_data_received(p_buff, length);
}
#endif

#ifdef BES_OTA
static void app_ibrt_ota_start_role_switch_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_ROLE_SWITCH_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_start_role_switch_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    TRACE(2,"%s:%d %d", __func__,p_buff[0],length);

    if ((p_buff[0] == OTA_RS_INFO_MASTER_SEND_RS_REQ_CMD)&&(length == 1))
    {
       bes_ota_send_role_switch_req();
    }
    else if ((p_buff[0] == OTA_RS_INFO_MASTER_DISCONECT_CMD)&&(length == 1))
    {
        ota_disconnect();
    }
}
static void app_ibrt_ota_update_rd_cmd_send(uint8_t *p_buff, uint16_t length)
{
#ifdef BLE_RPA_OTA_WORKROUND
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_UPDATE_RD_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
#endif
}
static void app_ibrt_ota_update_rd_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
#ifdef BLE_RPA_OTA_WORKROUND
    uint32_t ota_rv = *((uint32_t*)(p_buff));
    TRACE(2,"%s %x", __func__,ota_rv);
    nv_record_update_self_ble_rand(false,ota_rv);
    ota_control_send_get_random_response(ota_rv);
#endif
}

static void app_ibrt_ota_mobile_disconnected_cmd_send(uint8_t *p_buff, uint16_t length)
{
    app_ibrt_send_cmd_without_rsp(IBRT_OTA_TWS_MOBILE_DISC_CMD, p_buff, length);
    TRACE(1,"%s", __func__);
}

static void app_ibrt_ota_mobile_disconnected_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    uint8_t disc_link_type = *p_buff;
    TRACE(2,"%s type %d", __func__, disc_link_type);
    if (APP_OTA_LINK_TYPE_BLE == disc_link_type)
    {
        Bes_exit_ota_state();
    }
}
#endif
#endif
