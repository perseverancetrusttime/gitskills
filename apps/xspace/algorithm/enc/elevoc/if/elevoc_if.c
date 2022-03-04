#ifdef __ELEVOC_TRIP_MIC__
#include "elevoc_lib.h"
#include "elevoc_if.h"
void Elevoc_3mic_init_ap_if(void *addr, uint32_t size, int sample_rate){
	Elevoc_3mic_init_ap(addr,size,sample_rate);
}
void Elevoc_3mic_init_cp_if(void *addr, uint32_t size, int sample_rate, int algo_type){
	Elevoc_3mic_init_cp(addr,size,sample_rate,algo_type);
}
void Elevoc_3mic_share_init_if(void *addr, uint32_t size){
	Elevoc_3mic_share_init(addr,size);
}
int Elevoc_3mic_proc_pre_if(short *mic1, short *mic2, short *mic3, short *far, int len){
	return Elevoc_3mic_proc_pre(mic1,mic2,mic3,far,len);
}
void Elevoc_3mic_proc_mid_if(void){
	Elevoc_3mic_proc_mid();
}
int Elevoc_3mic_proc_post_if(short *out, int len){
	return Elevoc_3mic_proc_post(out,len);
}
void Elevoc_3mic_mid2post_if(void){
	Elevoc_3mic_mid2post();
}
void Elevoc_3mic_pre2mid_if(void){
	 Elevoc_3mic_pre2mid();
}
int elevoc_3mic_mem_size_ap_if(int sample_rate){
	return elevoc_3mic_mem_size_ap(sample_rate);
}
int elevoc_3mic_mem_size_cp_if(void){
	return elevoc_3mic_mem_size_cp();
}
int elevoc_3mic_mem_size_share_if(void){
	return elevoc_3mic_mem_size_share();
}
char *elevoc_get_lib_version_if(void){
	return elevoc_get_lib_version();
}
char *elevoc_get_lib_built_datetime_if(void){
	return elevoc_get_lib_built_datetime();
}

void ele_rx_ns_proc_if(short *buf,int len){
	ele_rx_ns_proc(buf,len);
}

int get_wind_state_if(void){
	return get_wind_state();
}

float get_wind_value_if(void){
	return get_wind_value();
}

float get_snr_value_if(void){
	return get_snr_value();
}
#endif

