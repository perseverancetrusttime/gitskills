#include "app_gma_state_env.h"

static APP_GMA_STATE_ENV_T app_gma_state_env;

void app_gma_state_env_reset(void)
{
    app_gma_state_env.msg_id = 0;
    app_gma_state_env.data_encry_flag = 0;
    app_gma_state_env.version_num = 0;
}

uint8_t app_gma_state_env_msg_id_receive(uint8_t msg_id)
{
    app_gma_state_env.msg_id = msg_id;
    return 0;
}

uint8_t app_gma_state_env_msg_id_get(void)
{
    return app_gma_state_env.msg_id;
}

uint8_t app_gma_state_env_msg_id_incre(void)
{
    app_gma_state_env.msg_id++;
    if(app_gma_state_env.msg_id > 15)
    {
        app_gma_state_env.msg_id = 0;
    }
    return 0;
}

uint8_t app_gma_state_env_encry_flag_receive(uint8_t flag)
{
    app_gma_state_env.data_encry_flag = flag;
    return 0;
}

uint8_t app_gma_state_env_encry_flag_get(void)
{
    return app_gma_state_env.data_encry_flag;
}

uint8_t app_gma_state_env_version_num_receive(uint8_t ver_num)
{
    app_gma_state_env.version_num = ver_num;
    return 0;
}

uint8_t app_gma_state_Version_num_get(void)
{
    return app_gma_state_env.version_num;
}

uint8_t app_gma_state_env_sequence_receive(uint8_t seq)
{
    app_gma_state_env.sequence = seq;
    return 0;
}

uint8_t app_gma_state_env_sequence_get(void)
{
    return app_gma_state_env.sequence;
}

