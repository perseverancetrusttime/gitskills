#include "app_gma_ota.h"
#include "app_gma_extended_handler.h"
#include "app_gma_cmd_handler.h"
#include "app_gma_data_handler.h"
#include "app_bt.h"
//#include "app_tws_ui.h"
#include "app_gma_state_env.h"
#include "hal_aud.h"
#include "app_battery.h"
#include "app_bt_stream.h"
#include "btapp.h"
#include "ai_thread.h"
#include "hal_trace.h"

extern char remote_dev_name[30];
extern GMA_DEV_FW_VER_T g_FW_Version;

void app_gma_media_control_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"%s funcCode:0x%02x", __func__, funcCode);

    APP_GMA_CMD_TLV_T* cmd = (APP_GMA_CMD_TLV_T*)ptrParam;
    APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_MEDIA_RSP;
    APP_GMA_STAT_CHANGE_RSP_T resp = {cmd->cmdType, sizeof(APP_GMA_STAT_CHANGE_RSP_T) - GMA_TLV_HEADER_SIZE, GMA_DEV_STA_CHANGE_SUCS};

    TRACE(2,"%s cmdType:0x%02x", __func__, cmd->cmdType);
    switch(cmd->cmdType)
    {
        case GMA_MEDIA_GET_STATE:
        {
            //resp.value = a2dp_service_is_connected();
            break;
        }
        case GMA_MEDIA_CONTROL_VOLUME:
        {
            TRACE(2,"%s adjust volume:0x%02x", __func__, cmd->payLoad[0]);
            if (cmd->payLoad[0] == 0)
            {
                TRACE(1,"%s warning!", __func__);
                break;
            }
            else
            {
                 TRACE(1,"%s not warning", __func__);
                break;
            }
        }
        case GMA_MEDIA_CONTROL_START:
        {
            a2dp_handleKey(AVRCP_KEY_PLAY);
            break;
        }
        case GMA_MEDIA_CONTROL_PAUSE:
        {
            a2dp_handleKey(AVRCP_KEY_PAUSE);
            break;
        }
        case GMA_MEDIA_CONTROL_PRE:
        {
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        }
        case GMA_MEDIA_CONTROL_NEXT:
        {
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        }
        case GMA_MEDIA_CONTROL_STOP:
        {
            a2dp_handleKey(AVRCP_KEY_STOP);
            break;
        }
    }

    app_gma_send_command(rspCmdcode, (uint8_t*)&resp, (uint32_t)sizeof(APP_GMA_STAT_CHANGE_RSP_T), false, device_id);
}

void app_gma_hfp_control_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    APP_GMA_CMD_TLV_T* cmd = (APP_GMA_CMD_TLV_T*)ptrParam;
    APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_HFP_RSP;
    APP_GMA_STAT_CHANGE_RSP_T resp = {cmd->cmdType, sizeof(APP_GMA_STAT_CHANGE_RSP_T) - GMA_TLV_HEADER_SIZE, GMA_DEV_STA_CHANGE_SUCS};

    // we assume that there is onle one TLV in the payLoad param
    TRACE(3,"%s line:%d len:%d", __func__, __LINE__, paramLen);
    DUMP8("%02x ", ptrParam, paramLen);

    TRACE(4,"%s line:%d funcCode:0x%02x cmdType:0x%02x", __func__, __LINE__, funcCode, cmd->cmdType);
    switch(cmd->cmdType)
    {
        case GMA_HFP_GET_STATE:
        {
            resp.value = (app_is_hfp_service_connected(BT_DEVICE_ID_1)) ? GMA_HFP_OPERATION_SUCS : GMA_HFP_OPERATION_FAIL;
            break;
        }
        case GMA_HFP_DIAL:
        {
            btif_hf_channel_t* hf_chan = app_bt_get_device(BT_DEVICE_ID_1)->hf_channel;
            static uint8_t    number[GMA_BT_HFP_NUMBER_MAX];
            memcpy(number, cmd->payLoad, cmd->cmdLen);
            TRACE(3,"%s line:%d cmdLen:0x%02x", __func__, __LINE__, cmd->cmdLen);
            DUMP8("%02x", number, cmd->cmdLen);
            if (app_is_hfp_service_connected(BT_DEVICE_ID_1))
            {
                resp.value = GMA_HFP_OPERATION_SUCS;
                btif_hf_dial_number(hf_chan, number, (uint16_t)paramLen);
            }
            else
            {
                resp.value = GMA_HFP_OPERATION_FAIL;
            }
            break;
        }
        case GMA_HFP_PICK_UP:
        {
            hfp_handle_key(HFP_KEY_ANSWER_CALL);
            break;
        }
        case GMA_HFP_HANG_UP:
        {
            hfp_handle_key(HFP_KEY_HANGUP_CALL);
            break;
        }
        case GMA_HFP_REDIAL:
        {
            hfp_handle_key(HFP_KEY_REDIAL_LAST_CALL);
            break;
        }
        case GMA_HFP_HANGUP_AND_ANSWER:
        {
            hfp_handle_key(HFP_KEY_THREEWAY_HANGUP_AND_ANSWER);
            break;
        }
        case GMA_HFP_HOLD_REL_INCOMING:
        {
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_REL_INCOMING);
            break;
        }
        case GMA_HFP_HOLD_AND_ANSWER:
        {
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);
            break;
        }
        case GMA_HFP_HOLD_ADD_HELD_CALL:
        {
            hfp_handle_key(HFP_KEY_THREEWAY_HOLD_ADD_HELD_CALL);
            break;
        }
    }
    app_gma_send_command(rspCmdcode, (uint8_t*)&resp, (uint32_t)sizeof(APP_GMA_STAT_CHANGE_RSP_T), false, device_id);

}

void app_gma_extended_cmd_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(3,"%s line:%d funcCode:0x%02x", __func__, __LINE__, funcCode);

    APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_EXTENDED_RSP;

    // parse ptrParam in case of multiple TLV
    TRACE(3,"%s line:%d len:%d", __func__, __LINE__, paramLen);
    DUMP8("%02x ", ptrParam, paramLen);

    // decoded date: 1a 40 00 10 04 00 05 00 06 00 03 00 00 00 06 06 06 06 06 06
    // paramLen:0x10
    // ptrParam-> 04 00 05 00 06 00 03 00 00 00 06 06 06 06 06 06
    // 04 00 | 05 00 | 06 00 | 03 00 | 00 00 | 06 06 06 06 06 06                
    // 04 04 00 00 00 02 | 05 1C Remote_Bluetooth_Device_Name | 06 01 00 | 03 07 12345.6 | 00 02 01 00
    // len: 52

    uint16_t remain = paramLen;
    uint8_t parse_idx = 0;
    uint8_t tlv_cmdType;
    uint8_t tlv_len;

    uint8_t rsp_load[APP_GMA_CMD_MAXIMUM_PAYLOAD_SIZE - 2*sizeof(uint8_t)] = {0x00};
    uint8_t rsp_len = 0;

    while(remain > 0)
    {
        tlv_cmdType = ptrParam[parse_idx++];
        tlv_len = ptrParam[parse_idx++];
        TRACE(4,"%s line:%d tlv_cmdType:0x%02x tlv_len:%d", __func__, __LINE__, tlv_cmdType, tlv_len);
        switch(tlv_cmdType)
        {
            case GMA_EXT_POWER_LEL:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;
                rsp_load[rsp_len++] = sizeof(GMA_EXT_POW_RSP_T);

                GMA_EXT_POW_RSP_T para;
                uint8_t level;
                APP_BATTERY_STATUS_T battery;
                app_battery_get_info(NULL, &level, &battery);
                para.power_percent = 100 * level /(APP_BATTERY_LEVEL_MAX - APP_BATTERY_LEVEL_MIN);
                if(APP_BATTERY_STATUS_UNDERVOLT == battery || level<=3)
                {
                    para.power_state = GMA_EXT_POW_RSP_LOW;
                }
                else
                {
                    para.power_state = GMA_EXT_POW_RSP_NORM;
                }

                memcpy(&rsp_load[rsp_len], &para, sizeof(GMA_EXT_POW_RSP_T));
                rsp_len += sizeof(GMA_EXT_POW_RSP_T);

                break;
            }
            case GMA_EXT_BATTERY_STAT:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;
                rsp_load[rsp_len++] = sizeof(GMA_EXT_RESULT_RSP_T);

                GMA_EXT_RESULT_RSP_T para;
                APP_BATTERY_STATUS_T batt_state;
                app_battery_get_info(NULL, NULL, &batt_state);
                if(APP_BATTERY_STATUS_CHARGING == batt_state)
                {
                    para.result = GMA_EXT_BATT_RSP_CHARGE;
                }
                else if(APP_BATTERY_STATUS_NORMAL == batt_state)
                {
                    para.result = GMA_EXT_BATT_RSP_USE_BAT;
                }
                else
                {
                    para.result = GMA_EXT_BATT_RSP_USE_AC;
                }

                memcpy(&rsp_load[rsp_len], &para, sizeof(GMA_EXT_RESULT_RSP_T));
                rsp_len += sizeof(GMA_EXT_RESULT_RSP_T);

                break;
            }
            case GMA_EXT_FM_FREQ_SET:
            {
                if (!tlv_len)
                {
                    TRACE(4,"%s line:%d cmdType:0x%02x. tlv_len(%d) incorrect!", __func__, __LINE__, tlv_cmdType, tlv_len);
                    goto end_while;
                }
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;
                rsp_load[rsp_len++] = sizeof(GMA_EXT_RESULT_RSP_T);

                GMA_EXT_RESULT_RSP_T para;
                para.result = GMA_DEV_STA_CHANGE_SUCS;

                uint8_t fm_freq[10] = {0};
                memcpy(fm_freq, &ptrParam[parse_idx], tlv_len);
                parse_idx += tlv_len;
                TRACE(3,"%s line:%d set FM:%s", __func__, __LINE__, fm_freq);
                if (!GMA_CAPACITY_FM)   // we don't support FM
                {
                    TRACE(2,"%s  line:%d however, I don't support FM", __func__, __LINE__);
                    para.result = GMA_DEV_STA_CHANGE_FAIL;
                }

                memcpy(&rsp_load[rsp_len], &para, sizeof(GMA_EXT_RESULT_RSP_T));
                rsp_len += sizeof(GMA_EXT_RESULT_RSP_T);

                break;
            }
            case GMA_EXT_FM_FREQ_GET:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;

                if (GMA_CAPACITY_FM)    // we support FM
                //if (1)
                {
                    // don't support FM but receive the FM get command 
                    char fm_freq[10] = "12345.6";

                    rsp_load[rsp_len++] = strlen(fm_freq);
                    memcpy(&rsp_load[rsp_len], fm_freq, strlen(fm_freq));
                    rsp_len += strlen(fm_freq);
                }
                else
                {
                    rsp_load[rsp_len++] = 1;
                    rsp_load[rsp_len++] = '0';
                    TRACE(2,"%s line:%d however, I don't support FM", __func__, __LINE__);
                }

                break;
            }
            case GMA_EXT_VERSION_GET:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;
                rsp_load[rsp_len++] = sizeof(GMA_DEV_FW_VER_T)-1;

                GMA_DEV_FW_VER_T para;
                app_gma_ota_FW_Ver_get(&para);
                memcpy(&rsp_load[rsp_len], (uint8_t*)&para+1, sizeof(GMA_DEV_FW_VER_T)-1);
                rsp_len += sizeof(GMA_DEV_FW_VER_T)-1;

                break;
            }
            case GMA_EXT_BT_NAME:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;

                char remote_dev_name[50] = "Remote_Bluetooth_Device_Name";  // need to get the connected BT device name
                rsp_load[rsp_len++] = strlen(remote_dev_name);
                memcpy(&rsp_load[rsp_len], remote_dev_name, strlen(remote_dev_name));
                rsp_len += strlen(remote_dev_name);

                break;
            }
            case GMA_EXT_MIC_STAT:
            {
                if (tlv_len != 0)
                {
                    TRACE(3,"%s line:%d cmdType:0x%02x. tlv_len incorrect!", __func__, __LINE__, tlv_cmdType);
                    goto end_while;
                }
                parse_idx += tlv_len;
                remain -= (sizeof(tlv_cmdType) + sizeof(tlv_len) + tlv_len);

                rsp_load[rsp_len++] = tlv_cmdType;
                rsp_load[rsp_len++] = sizeof(GMA_EXT_RESULT_RSP_T);

                GMA_EXT_RESULT_RSP_T para;
                para.result = ai_struct[AI_SPEC_ALI].ai_stream.ai_stream_opened;

                memcpy(&rsp_load[rsp_len], &para, sizeof(GMA_EXT_RESULT_RSP_T));
                rsp_len += sizeof(GMA_EXT_RESULT_RSP_T);                

                break;
            }
            default:
            TRACE(3,"%s line:%d unsupported cmd:0x%02x", __func__, __LINE__, tlv_cmdType);
            goto end_while;
        }

        if (parse_idx > paramLen)
        {
            TRACE(3,"%s line:%d len:%d", __func__, __LINE__, parse_idx);
            break;
        }
    }
end_while:
    TRACE(4,"%s line:%d remain:%d rsp_len:%d", __func__, __LINE__, remain, rsp_len);
    DUMP8("%02x ", rsp_load, rsp_len);

    app_gma_send_command(rspCmdcode, (uint8_t*)&rsp_load, rsp_len, false, device_id);
}

CUSTOM_COMMAND_TO_ADD(GMA_OP_MEDIA_CMD, app_gma_media_control_handler, false, 0, NULL );
CUSTOM_COMMAND_TO_ADD(GMA_OP_HFP_CMD, app_gma_hfp_control_handler, false, 0, NULL );
CUSTOM_COMMAND_TO_ADD(GMA_OP_EXTENDED_CMD, app_gma_extended_cmd_handler, false, 0, NULL );
