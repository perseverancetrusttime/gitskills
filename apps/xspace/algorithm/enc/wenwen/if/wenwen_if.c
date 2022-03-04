#ifdef __WENWEN_TRIP_MIC__
#include "mobvoi_dsp.h"
#include "wenwen_if.h"
void mobvoi_dsp_uplink_cleanup_if(void *instance){
	mobvoi_dsp_uplink_cleanup(instance); 
}

void mobvoi_dsp_downlink_cleanup_if(void *instance){ 
	mobvoi_dsp_downlink_cleanup(instance); 
}

int mobvoi_set_license_key_if(unsigned char *signature_public_key, int len){
	return 0;//mobvoi_set_license_key(signature_public_key,len);
}

int mobvoi_dsp_set_memory_if(int mode,void* memory, int length){
	return mobvoi_dsp_set_memory(mode,memory, length); 
}

void mobvoi_dsp_uplink_set_echo_delay_if(void* instance, int sample_num){
	mobvoi_dsp_uplink_set_echo_delay(instance,sample_num);
}

void* mobvoi_dsp_uplink_init_if(int mode,int sample_rate, int sample_num){
	return mobvoi_dsp_uplink_init(mode,sample_rate,sample_num);
}

void* mobvoi_dsp_downlink_init_if(int frame_size, int sample_rate,int mode){
	return mobvoi_dsp_downlink_init(frame_size,sample_rate,mode);
}

unsigned char* mobvoi_get_lib_version_if(void){
	return mobvoi_get_lib_version();
}
unsigned char* mobvoi_get_lib_built_datetime_if(void){
	return mobvoi_get_lib_built_datetime();
}

unsigned int mobvoi_dsp_get_memory_needed_if(int mode){
	return mobvoi_dsp_get_memory_needed(mode);
}

int mobvoi_dsp_uplink_process_if(void *instance,
										int stage_flag,
                              						short *talk_mic,
                              						short *enc_mic,
                              						short *fb_mic,
                              						short *spk,
                              						short *out){
	return mobvoi_dsp_uplink_process(instance,stage_flag,talk_mic,enc_mic,fb_mic,spk,out);
}

void mobvoi_dsp_uplink_cmd_if(void* instance, int cmd, void* result){
	return mobvoi_dsp_uplink_cmd(instance, cmd, result);
}

void mobvoi_dsp_downlink_process_if(void* instance, short* in, short* out){
	return mobvoi_dsp_downlink_process(instance, in,out);
}
#endif

