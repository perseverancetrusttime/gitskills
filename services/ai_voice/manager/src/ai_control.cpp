#include "cmsis_os.h"
#include "spp_api.h"
#include "hal_trace.h"
#include "ai_control.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "ai_thread.h"
#include "ai_manager.h"
#include "app_ai_voice.h"
#include "app_recording_handle.h"

const struct ai_handler_func_table *ai_function_table[MUTLI_AI_NUM] = {NULL};

uint32_t ai_function_handle(AI_FUNC_CMD_M func_id, void *param1, uint32_t param2, uint8_t ai_index, uint8_t dest_id)
{
    // Counter
    uint8_t counter;
    uint32_t ret = ERROR_RETURN;
    TRACE(3,"%s id %d ai_index %d", __func__, func_id,ai_index);

    if (ai_function_table[ai_index])
    {
        // Get the AI handler function by parsing the function table
        for (counter = ai_function_table[ai_index]->func_cnt; 0 < counter; counter--)
        {
            struct ai_func_handler handler = (struct ai_func_handler)(*(ai_function_table[ai_index]->func_table + counter - 1));
            if(handler.id == func_id)
            {
                // If handler is NULL, message should not have been received in this state
                //ASSERT((handler.func), "%s no this function %d", __func__, func_id);
                if (NULL == handler.func)
                {
                    TRACE(2,"%s func %d no register", __func__, func_id);
                    break;
                }

                ret = handler.func(param1, param2, dest_id);
            }
        }

        app_ai_ui_global_handler_ind((uint32_t)func_id, param1, param2, ai_index, dest_id);
    }
    else
    {
        // ASSERT(0, "AI function table not found, spec:%d", ai_index);
    }

    return ret;
}

void app_ai_send_cmd_to_peer(ai_cmd_instance_t *ai_cmd)
{
#ifdef IBRT
    uint8_t buff[672];
    uint16_t length = 0;

    memcpy(buff, ai_cmd, AI_CMD_HEAD_SIZE);
    if (ai_cmd->param_length)
    {
        memcpy(&buff[AI_CMD_HEAD_SIZE], ai_cmd->param, ai_cmd->param_length);
    }

    length = AI_CMD_HEAD_SIZE + ai_cmd->param_length;
    app_ai_tws_send_cmd_to_peer(buff, length);

    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd->cmdcode, length);
#endif
}

void app_ai_send_cmd_to_peer_with_rsp(ai_cmd_instance_t *ai_cmd)
{
    uint8_t buff[672];
    uint16_t length = 0;
    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd->cmdcode, length);

    memcpy(buff, ai_cmd, AI_CMD_HEAD_SIZE);
    if (ai_cmd->param_length)
    {
        memcpy(&buff[AI_CMD_HEAD_SIZE], ai_cmd->param, ai_cmd->param_length);
    }

    length = AI_CMD_HEAD_SIZE + ai_cmd->param_length;
    app_ai_tws_send_cmd_with_rsp_to_peer(buff, length);

    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd->cmdcode, length);
}

void app_ai_send_cmd_rsp_to_peer(ai_cmd_instance_t *ai_cmd, uint16_t rsp_seq)
{
    uint8_t buff[672];
    uint16_t length = 0;
    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd->cmdcode, length);

    memcpy(buff, ai_cmd, AI_CMD_HEAD_SIZE);
    if (ai_cmd->param_length)
    {
        memcpy(&buff[AI_CMD_HEAD_SIZE], ai_cmd->param, ai_cmd->param_length);
    }

    length = AI_CMD_HEAD_SIZE + ai_cmd->param_length;
    app_ai_tws_send_cmd_rsp_to_peer(buff, rsp_seq, length);

    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd->cmdcode, length);
}

void app_ai_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    uint8_t dest_id = 0;
    ai_cmd_instance_t ai_cmd;
    ai_cmd.param = NULL;
    memcpy(&ai_cmd, p_buff, AI_CMD_HEAD_SIZE);
    if (length > AI_CMD_HEAD_SIZE)
    {
        ai_cmd.param = &p_buff[AI_CMD_HEAD_SIZE];
    }

    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(ai_cmd.cmdcode >= AI_CMD_GMA_SEND_SECRET && NULL == ai_info) {
        TRACE(1, "%s ignore cmd code:%x as ai info is null.", __func__, ai_cmd.cmdcode);
        return;
    }
    if(ai_info)
        dest_id = app_ai_get_dest_id(ai_info);

    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd.cmdcode, ai_cmd.param_length);
    switch (ai_cmd.cmdcode)
    {
    /// mic of peer is opened
    case AI_CMD_PEER_MIC_OPEN:
        app_ai_voice_block_a2dp();
        break;
    /// mic of peer is closed
    case AI_CMD_PEER_MIC_CLOSE:
        app_ai_voice_resume_blocked_a2dp();
        break;
#ifdef DUAL_MIC_RECORDING
    /// mic of both side need open
    case AI_CMD_DUAL_MIC_OPEN:
        app_recording_start_capture_stream_on_specific_ticks(*(uint32_t *)ai_cmd.param);
        break;
    /// mic of both side need close
    case AI_CMD_DUAL_MIC_CLOSE:
        break;
#endif
    case AI_CMD_GMA_SEND_SECRET:
        ai_function_handle(API_AI_EXC_SECRET_KEY, (void *)ai_cmd.param, ai_cmd.param_length, AI_SPEC_ALI, dest_id);
        break;
    case AI_CMD_GMA_OTA_DATA:
        ai_function_handle(CALLBACK_PEER_OTA_DATA_REV, (void *)ai_cmd.param, ai_cmd.param_length, AI_SPEC_ALI, dest_id);
        break;
    case AI_CMD_GMA_OTA_CMD:
        ai_function_handle(CALLBACK_PEER_OTA_CMD_REV, (void *)&ai_cmd, rsp_seq, AI_SPEC_ALI, dest_id);
        break;
    case AI_CMD_GMA_OTA_CMD_RSP:
        ai_function_handle(CALLBACK_PEER_OTA_CMD_RSP, (void *)ai_cmd.param, ai_cmd.param_length, AI_SPEC_ALI, dest_id);
        break;
    case AI_CMD_TENCENT_SEND_FLAG:
        ai_function_handle(API_AI_EXC_FLAG, (void *)ai_cmd.param, ai_cmd.param_length, AI_SPEC_TENCENT, dest_id);
        break;
    default:
        break;
    }
}

void app_ai_rev_peer_cmd_rsp_timeout_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    ai_cmd_instance_t ai_cmd;
    ai_cmd.param = NULL;
    memcpy(&ai_cmd, p_buff, AI_CMD_HEAD_SIZE);
    if (length > AI_CMD_HEAD_SIZE)
    {
        ai_cmd.param = &p_buff[AI_CMD_HEAD_SIZE];
    }

    AI_connect_info *ai_info = app_ai_get_connect_info(ai_manager_get_foreground_ai_conidx());
    if(NULL == ai_info) {
        return;
    }
    uint8_t dest_id = app_ai_get_dest_id(ai_info);

    TRACE(3,"%s cmd 0x%x len %d", __func__, ai_cmd.cmdcode, ai_cmd.param_length);
    switch (ai_cmd.cmdcode)
    {
        case AI_CMD_GMA_OTA_CMD:
            ai_function_handle(CALLBACK_REV_PEER_OTA_RSP_OUT, (char *)ai_cmd.param, ai_cmd.param_length, AI_SPEC_ALI, dest_id);
            break;
        default:
            break;
    }
}

void ai_control_init(const struct ai_handler_func_table *func_table, uint8_t ai_index)
{
    TRACE(2, "ai_index:%d, func_table:%p", ai_index, func_table);

    ai_function_table[ai_index] = func_table;
    ai_function_handle(API_HANDLE_INIT, NULL, 0, ai_index, 0);
}
