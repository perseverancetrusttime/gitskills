#include "string.h"
#include "stdio.h"
#include "app_gma_cmd_code.h"
#include "app_gma_cmd_handler.h"
#include "gma_crypto.h"
#include "hal_trace.h"
#include "factory_section.h"
#include "nvrecord_env.h"
#include "app_tws_ctrl_thread.h"
#include "app_ibrt_customif_cmd.h"

static uint8_t ble_key[16];
static char secret[] = GMA_CRYPTO_SECRET;
static char pid[] = GMA_CRYPTO_PID;
static uint8_t isGmaSecurityOn = false;
static char iv[] = GMA_IV_ORIGIN;

void gma_secret_key_send(void)
{
    TRACE(1,"%s", __func__);
    tws_ctrl_send_cmd(APP_TWS_CMD_GMA_SECRET_KEY, (uint8_t *)secret, sizeof(secret)/sizeof(char));
}

void gma_set_secret_key(char *cryptoSercet)
{
    memcpy((void *)secret, (void *)cryptoSercet, sizeof(secret)/sizeof(char));
    TRACE(2,"%s %s", __func__, secret);
}

void app_gma_enable_security(bool isEnable)
{
    TRACE(2,"%s Enable switch %d", __func__, isEnable);
    isGmaSecurityOn = isEnable;
}

bool app_gma_is_security_enabled(void)
{
    return isGmaSecurityOn;
}

void app_gma_encrypt_reset(void)
{
    TRACE(1,"%s", __func__);

    memset(ble_key, 0, sizeof(ble_key));

    app_gma_enable_security(false);
}

void app_gma_encrypt(uint8_t* input, uint8_t* output, uint32_t len)
{
    gma_crypto_encrypt(output, input, len, ble_key, (uint8_t*)iv);
}

void app_gma_decrypt(uint8_t* input, uint8_t* output, uint32_t len)
{
    gma_crypto_decrypt(output, input, len, ble_key, (uint8_t*)iv);
}

char hex_to_char(int num)
{
    char char_num;
  switch(num)
  {
    case 0:
        char_num = '0';
        break;
    case 1:
        char_num = '1';
        break;
    case 2:
        char_num = '2';
        break;
    case 3:
        char_num = '3';
        break;
    case 4:
        char_num = '4';
        break;
    case 5:
        char_num = '5';
        break;

    case 6:
        char_num = '6';
        break;

    case 7:
        char_num = '7';
        break;

    case 8:
        char_num = '8';
        break;
    case 9:
        char_num = '9';
        break;
    case 0xa:
        char_num = 'a';
        break;
    case 0xb:
        char_num = 'b';
        break;
    case 0xc:
        char_num = 'c';
        break;
    case 0xd:
        char_num = 'd';
        break;

    case 0xe:
        char_num ='e';
        break;
    case 0xf:
        char_num ='f';
        break;
    default:
        char_num = '\0';
  }

    return char_num;
}
void  bt_address_convert_to_char_2(uint8_t *_bt_addr, char *out_str)
{
    int i ,j=0;
    for(i=5 ; i>=0;i--)
    {
        out_str[ j++] = hex_to_char((_bt_addr[i]>>4)&0xf);
        out_str[ j++] = hex_to_char(_bt_addr[i]&0xf);
    }
}

extern struct nvrecord_env_t *nvrecord_env_p;
void app_gma_cal_sha256(const uint8_t* sec_random, uint8_t* out)
{
    char seq[100];
    char _bt_addr[6];
    char _bt_addr_str[12];
    int len = 0;
    
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    memcpy((void *)_bt_addr, (void *)p_ibrt_ctrl->local_addr.address, 6);
    bt_address_convert_to_char_2((uint8_t *)_bt_addr, _bt_addr_str);
    memcpy(seq, sec_random, 16);
    len += 16;
    memset(seq+len, ',', 1);
    len++;

    memcpy(seq+len, pid, sizeof(pid)-1);
    len += (sizeof(pid)-1);
    memset(seq+len, ',', 1);
    len++;
    memcpy(seq+len, _bt_addr_str, sizeof(_bt_addr_str));
    len += sizeof(_bt_addr_str);
    memset(seq+len, ',', 1);
    len++;
    memcpy(seq+len, secret, sizeof(secret)-1);
    len += (sizeof(secret)-1);

    TRACE(2,"[%s] len =%d", __func__, len);
    DUMP8("%c", seq, len);

    gma_SHA256_hash(seq, len, out);
    TRACE(1,"[%s] in data", __func__);
    DUMP8("0x%02x ", seq, 40);
    TRACE(1,"[%s] in data goon", __func__);
    DUMP8("0x%02x ", seq+40, len-40);
    TRACE(1,"[%s] out data", __func__);
    DUMP8("0x%02x ", out, 32);
}

void app_gma_cal_aes128(const uint8_t* sec_random, uint8_t dataLen, uint8_t* key, uint8_t* out)
{
    gma_crypto_encrypt((uint8_t*)out, (uint8_t*)sec_random, dataLen, (uint8_t*)key,(uint8_t*)iv);
}

void app_gma_crypto_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"[%s] funcCode:0x%02x", __func__, funcCode);
    
    if (GMA_OP_SEC_CERT_RANDOM == funcCode)
    {
        APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_SEC_CERT_CIPHER;
        if(16 != paramLen)
        {
            TRACE(2,"[%s] Wrong len, cmd=%d", __func__, funcCode);
            return ;
        }
        else
        {
            uint8_t shaResult[32];
            uint8_t get_random[16];
            uint8_t cipher[16];
            app_gma_encrypt_reset();

            memcpy(get_random, ptrParam, 16);

            app_gma_cal_sha256(get_random, shaResult);

            memcpy(ble_key, shaResult, sizeof(ble_key));
            TRACE(1,"[%s] ble key", __func__);
            DUMP8("0x%02x ", ble_key, 16);
            app_gma_cal_aes128(get_random, 16, ble_key, cipher);
            TRACE(1,"[%s] aes out", __func__);
            DUMP8("0x%02x ", cipher, 16);
            app_gma_send_command(rspCmdcode, (uint8_t*)cipher, sizeof(cipher), false, device_id);
        }
        
    }
    else if (GMA_OP_SEC_CERT_RESULT == funcCode)
    {
        APP_GMA_CMD_CODE_E rspCmdcode = GMA_OP_SEC_CERT_RESULT_RSP;
        APP_GMA_CRYPTO_CHEACK_RST_T* rsp = (APP_GMA_CRYPTO_CHEACK_RST_T*)ptrParam;
        app_gma_send_command(rspCmdcode, (uint8_t*)rsp, sizeof(APP_GMA_CRYPTO_CHEACK_RST_T), false, device_id);

        // enable the security state after call app_gma_send_command,
        // as the last response of the authentication should not be encrypted
        if(GMA_CRYPTO_FAIL == rsp->result)
        {
            app_gma_encrypt_reset();
        }
        else
        {
            app_gma_enable_security(true);
            TRACE(1,"[%s] ENCRYPTO ON", __func__);
        }
    }
}

CUSTOM_COMMAND_TO_ADD(GMA_OP_SEC_CERT_RANDOM, app_gma_crypto_handler, false, 0, NULL );
CUSTOM_COMMAND_TO_ADD(GMA_OP_SEC_CERT_RESULT, app_gma_crypto_handler, false, 0, NULL );