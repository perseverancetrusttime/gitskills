#ifndef __ALGORITHM__ADAPTER_H__
#define __ALGORITHM__ADAPTER_H__


#include "stdint.h"
#include "stdbool.h"
#include "speech_cfg.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef void (*HEAP_INIT_CALLBACK)(void * begin_addr, unsigned int  size);
typedef void * (*HEAP_MALLCO_CALLBACK)(unsigned int  size);
typedef void (*HEAP_FREE_CALLBACK)(void * addr);
#if defined __SP_TRIP_MIC__
typedef int (*SCO_CP_DEAL_CALLBACK)(short *out_buf, float *in_buf,float *fb_buf, int *_pcm_len);
typedef int (*SCO_CP_INIT_CALLBACK)(int frame_len, int channel_num,int cvsd_flag);
#else
typedef int (*SCO_CP_DEAL_CALLBACK)(short *pcm_buf, short *ref_buf, int *_pcm_len, sco_status_t *status);
typedef int (*SCO_CP_INIT_CALLBACK)(int frame_len, int channel_num);
#endif
typedef int  (*SCO_CP_DEINIT_CALLBACK)(void);
typedef int (*HFP_VOL_GET)(void);
typedef enum
{
    FADEIN_IDLE_STEP = 0,
    FADEIN_MUTE_STEP,
    FADEIN_ON_STEP,
    FADEIN_STEP_MAX
} sco_fadein_e;
typedef struct {
    unsigned short counter;
    unsigned char status;
} sco_fadein_t;
char xspace_get_alg_mic_channel(void);
char xspace_get_alg_freq(void);
int  xspace_get_alg_mic_sample_rate(void);
int  xspace_get_alg_deal_bit_depth(void);
int  xspace_alg_uplink_process(short *buf, short *ref,int in_len, unsigned int  *out_len,signed int codec_type);
int  xspace_alg_uplink_mic_bypass(short *buf, short *ref,int in_len, unsigned int  *out_len,int codec_type,int which_mic);
int  xspace_alg_dnlink_process(short *out_buf, short *in_buf, int numsamples, int codec_type);
int  xspace_alg_init(bool is_wb, unsigned char *buf, int len);
int  xspace_alg_deinit(void);
#if defined(SCO_CP_ACCEL)
#if defined __SP_TRIP_MIC__
void  xspace_alg_uplink_cp_process(short *out, float *in, float* fb, int AncFlag,int numsamples);
#elif defined (__WENWEN_TRIP_MIC__) ||defined (__ELEVOC_TRIP_MIC__)
int  xspace_alg_uplink_cp_process(short *buf, short *ref,int in_len);
#endif
int  xspace_alg_uplink_ap_process(short *buf, short *ref,int in_len);
#endif
int  xspace_alg_mallco_callback_set(HEAP_INIT_CALLBACK  callback_heap_init,
									HEAP_MALLCO_CALLBACK callback_mallco,
									HEAP_FREE_CALLBACK callback_free);
int  xspace_alg_cp_callback_set(SCO_CP_INIT_CALLBACK  callback_sco_cp_init,
						SCO_CP_DEAL_CALLBACK callback_sco_cp_deal,
						SCO_CP_DEINIT_CALLBACK callback_sco_cp_deinit);
int xspace_alg_skip_uplink_frame_init(unsigned char skip_num);
int  xspace_alg_hfp_vol_get_callback_set(HFP_VOL_GET  callback_hfp_vol_get);
int  xspace_alg_wav_gen_init(int effect_mod,uint8_t *buf, int len); // wenwen wav generate algorithm
int  xspace_alg_wav_gen_deinit(void);
int  xspace_wind_state_get(void);
float  xspace_wind_value_get(void);
float  xspace_snr_value_get(void);
#ifdef __cplusplus
}
#endif
#endif	

