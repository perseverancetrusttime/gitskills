#ifndef __LT__API_H__
#define __LT__API_H__

#ifdef __cplusplus
extern "C" {
#endif
int lt_init(short mtu_frame_num,short normal_delay_mtu);
float lt_get_fast_clock_limit(void);
float lt_get_slow_clock_limit(void);
int lt_buffer_deal0(unsigned char *buffer, unsigned int  buffer_bytes);
int lt_buffer_deal1(unsigned char *buffer, unsigned int  buffer_bytes);
int lt_buffer_deal2(void);
int lt_buffer_deal3(void);
int lt_buffer_deal4(void);
bool lt_if_force_trigger(void);
int lt_set_trigger_flag(void);
int lt_set_latency_para(void);
#ifdef __cplusplus
}
#endif
#endif	

