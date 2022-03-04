#ifndef __ELEVOC_IF_H__
#define __ELEVOC_IF_H__
#ifdef __cplusplus
extern "C" {
#endif
void Elevoc_3mic_init_ap_if(void *addr, uint32_t size, int sample_rate);
void Elevoc_3mic_init_cp_if(void *addr, uint32_t size, int sample_rate, int algo_type);
void Elevoc_3mic_share_init_if(void *addr, uint32_t size);

int Elevoc_3mic_proc_pre_if(short *mic1, short *mic2, short *mic3, short *far, int len);
void Elevoc_3mic_proc_mid_if(void);
int Elevoc_3mic_proc_post_if(short *out, int len);
void Elevoc_3mic_mid2post_if(void);
void Elevoc_3mic_pre2mid_if(void);

int elevoc_3mic_mem_size_ap_if(int sample_rate);
int elevoc_3mic_mem_size_cp_if(void);
int elevoc_3mic_mem_size_share_if(void);

char *elevoc_get_lib_version_if(void);
char *elevoc_get_lib_built_datetime_if(void);
void ele_rx_ns_proc_if(short *buf,int len);
//get wind state, 1: wind state  0: none wind 
int get_wind_state_if(void);

//get wind cofe
float get_wind_value_if(void);

//get snr, the return value is more large, the snr is more high 
float get_snr_value_if(void);
#ifdef __cplusplus
}
#endif
#endif	

