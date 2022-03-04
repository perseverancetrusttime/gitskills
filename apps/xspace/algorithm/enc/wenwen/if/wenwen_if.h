#ifndef __WENWEN_IF_ADAPTER_H__
#define __WENWEN_IF_ADAPTER_H__
#ifdef __cplusplus
extern "C" {
#endif
void mobvoi_dsp_uplink_cleanup_if(void *instance);
void mobvoi_dsp_downlink_cleanup_if(void* instance);
int mobvoi_set_license_key_if(unsigned char *signature_public_key, int len);
int mobvoi_dsp_set_memory_if(int mode,void* memory, int length);
void* mobvoi_dsp_uplink_init_if(int mode, int sample_rate,int sample_num);
void mobvoi_dsp_uplink_set_echo_delay_if(void* instance, int sample_num);
void* mobvoi_dsp_downlink_init_if(int frame_size, int sample_rate,int mode);
unsigned char* mobvoi_get_lib_version_if(void);
unsigned char* mobvoi_get_lib_built_datetime_if(void);
unsigned int mobvoi_dsp_get_memory_needed_if(int mode);
int mobvoi_dsp_uplink_process_if(void *instance,
										int stage_flag,
                              						short *talk_mic,
                              						short *enc_mic,
                              						short *fb_mic,
                              						short *spk,
                              						short *out);
void mobvoi_dsp_uplink_cmd_if(void* instance, int cmd, void* result);
void mobvoi_dsp_downlink_process_if(void* instance, short* in, short* out);
#ifdef __cplusplus
}
#endif
#endif	

