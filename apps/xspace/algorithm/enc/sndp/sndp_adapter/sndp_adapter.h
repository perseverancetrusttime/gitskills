#ifndef __SOUNDPLUS_ADAPTER_H__
#define __SOUNDPLUS_ADAPTER_H__

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef void * (*SNDP_HEAP_MALLCO_CALLBACK)(unsigned int  size);
typedef void (*SNDP_HEAP_FREE_CALLBACK)(void * addr);
int sp_deinit(void);
int sp_deal_Aec(float *out, float* out_echos, short *in, short *ref, int buf_len, int ref_len);
void sp_uplink_deal(short* out, float* in, float* in_echos, int AncFlag, int numsamples);
int sp_init(int NrwFlag);
int sp_dnlink_deal(short *inX, int len, int AncFlag);
char sp_get_mic_channel(void);
char  sp_get_mips(void);
int  sp_get_uplink_deal_sample_rate(void);
int sp_get_deal_bit_depth(void);
int sp_auth_status(void);
int sp_auth(uint8_t* key, int Key_len);
#ifdef __cplusplus
}
#endif

#endif	

