#ifndef _MOBVOI_EARBUDS_H_
#define _MOBVOI_EARBUDS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
  MOBVOI_KWS_MODE     = 0,
  MOBVOI_ASR_MODE     = 1,
  MOBVOI_CALL_MODE_WB = 2, // 16k samplerate output
  MOBVOI_CALL_MODE_NB = 3, // 8k samplerate output

  MOBVOI_MAX_MODE,

  MOBVOI_EFFECT_WN    = 0x10,
  MOBVOI_EFFECT_SWEEP = 0x20,
  MOBVOI_EFFECT_RAIN  = 0x30,
};

enum {
  MOBVOI_CMD_GET_AUDIO_QUALITY = 0,
  MOBVOI_CMD_GET_WIND_STATUS   = 1,
};

/**
 * Mobvoi library version get function.
 */
unsigned char* mobvoi_get_lib_version(void);

/**
 * Mobvoi library built time get function.
 */
unsigned char* mobvoi_get_lib_built_datetime(void);

/**
 * Set a block of pre-alloced memory which will be used by mobvoi dsp.
 * @param mode: dsp work mode.
 * @param memory: The memory header pointer.
 * @param length: The memory length in bytes.
 */
int mobvoi_dsp_set_memory(int mode, void* memory, int length);

/**
 * Get the size of memory needed by mobvoi dsp.
 * @param mode: dsp work mode.
 * @return result: the total memory size needed by dsp.
 */
unsigned int mobvoi_dsp_get_memory_needed(int mode);

/**
 * Mobvoi dsp uplink initialization function.
 * @param mode: dsp work mode.
 * @param sample_rate: support 8K, 16K
 * @param sample_num: the sample number of 1 frame, eg: 120 for 7.5ms, 64 for 4ms.
 * @return The dsp processor instance.
 */
void* mobvoi_dsp_uplink_init(int mode, int sample_rate, int sample_num);

/**
 * Mobvoi dsp uplink process function.
 * @param instance: The dsp processor instance.
 * @param stage_flag:
 *                    0 multi-stage process disabled;
 *                    1 1st stage process;
 *                    2 2nd stage process.
 * @param talk_mic: The talk_mic audio data.
 * @param enc_mic: The enc_mic audio data.
 * @param fb_mic: The fb_mic audio data.
 * @param spk: The speaker audio data.
 * @param out: The processed audio data.
 * @return The detected command id. 0 means invalid command.
 */
int mobvoi_dsp_uplink_process(void *instance,
                              int stage_flag,
                              short *talk_mic,
                              short *enc_mic,
                              short *fb_mic,
                              short *spk,
                              short *out);

/**
 * Mobvoi dsp uplink cleanup function.
 * @param instance: The dsp processor instance.
 */
void mobvoi_dsp_uplink_cleanup(void *instance);

/**
 * Set echo dealy, assuming speaker signal is always earlier.
 * @param instance: The dsp processor instance.
 * @param sample_num: The sample number of echo delay.
 */
void mobvoi_dsp_uplink_set_echo_delay(void* instance, int sample_num);

/**
 * Set echo dealy, assuming speaker signal is always earlier.
 * @param instance: The dsp processor instance.
 * @param cmd: The command index.
 * @param result: The result of the command.
 */
void mobvoi_dsp_uplink_cmd(void* instance, int cmd, void* result);

/**
 * Mobvoi dsp downlink processor initialization function.
 * @param frame_size: The frame size. 120 for 16K, 60 for 8K sample rate.
 * @param sample_rate: The audio sample rate.
 * @param mode: 0 real time processing mode, others for generating audio.
 * @return  The dsp downlink processor instance.
 */
void* mobvoi_dsp_downlink_init(int frame_size, int sample_rate, int mode);

/**
 * Mobvoi dsp downlink frames processing function.
 * @param instance: The dsp downlink processor instance.
 * @param in: The input audio data buffer.
 * @param out: The processed data buffer.
 */
void mobvoi_dsp_downlink_process(void* instance, short* in, short* out);

/**
 * Mobvoi dsp downlink processor cleanup function.
 * @param instance: The dsp downlink processor instance.
 */
void mobvoi_dsp_downlink_cleanup(void* instance);

#ifdef __cplusplus
}
#endif

#endif  // _MOBVOI_EARBUDS_H_