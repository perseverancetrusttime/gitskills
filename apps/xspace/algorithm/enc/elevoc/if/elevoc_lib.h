/*
 * Elevoc_NS.h
 *
 *  Created on: 2018年11月15日
 *      Author: stone
 */

#ifndef ELEVOC_NS_H_
#define ELEVOC_NS_H_

#include <stdint.h>

/*
 * costing 47W cycle in simulation
 */

#ifdef __cplusplus
extern "C" {
#endif

/******************************************库里的函数******************************************/
// 初始化函数:
void Elevoc_3mic_init_ap(void *addr, uint32_t size, int sample_rate);
void Elevoc_3mic_init_cp(void *addr, uint32_t size, int sample_rate, int algo_type);
void Elevoc_3mic_share_init(void *addr, uint32_t size);

// 处理函数:
int Elevoc_3mic_proc_pre(short *mic1, short *mic2, short *mic3, short *far, int len);
void Elevoc_3mic_proc_mid(void);
int Elevoc_3mic_proc_post(short *out, int len);
void Elevoc_3mic_mid2post(void);
void Elevoc_3mic_pre2mid(void);

// 获取所需内存:
int elevoc_3mic_mem_size_ap(int sample_rate);
int elevoc_3mic_mem_size_cp();
int elevoc_3mic_mem_size_share();

// 获取算法版本：
char *elevoc_get_lib_version(void);

// 获取算法编译日期时间：
char *elevoc_get_lib_built_datetime(void);
// down link ns handle
void ele_rx_ns_proc(short *buf,int len);

//get wind state, 1: wind state  0: none wind 
int get_wind_state(void);

//get wind cofe
float get_wind_value(void);

//get snr, the return value is more large, the snr is more high 
float get_snr_value(void);
/******************************************库里的函数******************************************/

#ifdef __cplusplus
}
#endif

#endif /* ELEVOC_NS_H_ */
