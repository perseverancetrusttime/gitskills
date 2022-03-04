#ifndef __ELEVOC_ADAPTER_H__
#define __ELEVOC_ADAPTER_H__

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#define max_eq_num (10)
#define Fs		(16000)
#define max_data_len  (256)
#define max_value (32767)
#define min_value (-32768)
#define ELE_TX_CH_NUM               3
#define ELE_FRAME_LEN               240
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_VQE_EQ_BAND_ELE 6
enum IIR_EQ_TYPE {
	iir_peak = 0,
	iir_lowshelf,
	iir_highshelf,
	iir_lowpass,
	iir_highpass,
};

typedef struct
{
    enum IIR_EQ_TYPE type;
    struct
    {
    	float f0; float gain; float q;
    } design;
} ELE_EQParam;

typedef struct
{
    int     enable;
    int     num;
    ELE_EQParam params[MAX_VQE_EQ_BAND_ELE];
} ELE_EqConfig;

typedef struct {
	int num;
	float a[max_eq_num*2];
	float b[max_eq_num*3];
	float delay[max_eq_num*2];
}iir_eq;

typedef enum
{
    ELEVOC_KEY_LEN_ERR = -7,        /**< The key len is invalid. */
    ELEVOC_NULL_PTR = -6,           /**< The key ptr is null. */
    ELEVOC_INVALID_PARAMETER = -5,  /**< The user parameter is invalid. */
    ELEVOC_ITEM_NOT_FOUND = -4,     /**< The data item wasn't found by the NVDM. */
    ELEVOC_INSUFFICIENT_SPACE = -3, /**< No space is available in the flash. */
    ELEVOC_INCORRECT_CHECKSUM = -2, /**< The NVDM found a checksum error when reading the data item or the buffer has changed when writing the data item. */
    ELEVOC_ERROR = -1,              /**< An unknown error occurred. */
    ELEVOC_STATUS_OK = 0,           /**< The operation was successful. */
}ELEVOC_ADDR_AUTH_STATE_E;

typedef enum
{
    AUTH_VERIFY_OK = 0,     // Auth verify ok
    AUTH_PACK_ERROR = 1,    // Package len/mem/read is wrong
    AUTH_BT_READ_ERROR = 2, // BT address read is wrong
    AUTH_TAG_ERROR = 3,     // TAG is not match
    AUTH_WRONG = 4,         // BT address beyond limited address
    AUTH_CRC_ERROR = 5,     // Package crc is wrong
}ELEVOC_ADDR_VERIFY_STATE_E;
typedef void * (*ELEVOC_HEAP_MALLCO_CALLBACK)(unsigned int  size);
typedef void (*ELEVOC_HEAP_FREE_CALLBACK)(void * addr);
void elevoc_deinit(void);
int elevoc_ref_deal(short *in_ref, int numsamples);
int elevoc_uplink_deal_part1(short *pcm_buf,short *far,int pcm_len);
int elevoc_uplink_deal_part2(short *pcm_buf,int pcm_len);
int elevoc_uplink_deal_part3(short *pcm_buf,int pcm_len);
int elevoc_uplink_deal_part4(short *pcm_buf,int pcm_len);
void elevoc_init(int NrwFlag);
int elevoc_dnlink_deal(short *outY, short *inX, int numsamples);
char elevoc_get_mic_channel(void);
char  elevoc_get_mips(void);
int  elevoc_get_uplink_deal_sample_rate(void);
int elevoc_get_deal_bit_depth(void);
void ele_wind_eq_set(const ELE_EqConfig *ELE_EqConfig_wind,float gain);
void AecDelay_ms_set(float delayMs);
void iireq_proc(short *input,short *output, int len);
void post_gain_set(float gain_db);
ELEVOC_ADDR_VERIFY_STATE_E elevoc_auth_init(void);
void elevoc_downlink_deal(short *pcm_buf,int pcm_len);
int elevoc_get_wind_state(void);
float elevoc_get_wind_value(void);
float elevoc_get_snr_value(void);
#ifdef __cplusplus
}
#endif

#endif	

