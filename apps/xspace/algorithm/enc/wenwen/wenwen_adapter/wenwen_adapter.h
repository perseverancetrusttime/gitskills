#ifndef __WENWEN_ADAPTER_H__
#define __WENWEN_ADAPTER_H__

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef void * (*WENWEN_HEAP_MALLCO_CALLBACK)(unsigned int  size);
typedef void (*WENWEN_HEAP_FREE_CALLBACK)(void * addr);
void wenwen_deinit(void);
int wenwen_ref_deal(short *in_ref, int numsamples);
int wenwen_uplink_deal_cp(short *buf,short *ref ,int len);
int wenwen_uplink_deal_ap(short *buf,short *ref ,int len);
void wenwen_init(int NrwFlag);
int wenwen_dnlink_deal(short *outY, short *inX, int numsamples);
char wenwen_get_mic_channel(void);
char  wenwen_get_mips(void);
int  wenwen_get_uplink_deal_sample_rate(void);
int wenwen_get_deal_bit_depth(void);
void wenwen_wav_gen_init(int effect_mode);
void wenwen_wav_gen_deinit(void);
#ifdef __cplusplus
}
#endif

#endif	

