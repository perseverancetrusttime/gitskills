// Copyright 2018 Google LLC.
// Libgsound version: f5d8a0a
#ifndef GSOUND_TARGET_AUDIO_CONFIG_H
#define GSOUND_TARGET_AUDIO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This header file was generated by Google GSound Services,
 * it should not be altered in any way. Please treat this as
 * a read-only file.
 *
 * Each function listed below must
 * be implemented for each specific
 * platform / SDK
 *
 */
#include "gsound.h"

/**
 * The following specifies the required settings for the
 * SBC encoder. Note the only setting which is not hard-coded
 * is the bitpool which is provided to the Target at runtime using
 * the data structure below.
 *
 * The SBC algorithm used is defined in the A2DP specification.
 * Note, GSound does NOT use mSBC.
 *
 * SBC Encoder Settings:
 *    - Sample frequency  - 16000 Hz
 *    - Bits/Sample       - 16 bits
 *    - Channel mode      - Mono
 *    - Subband           - 8
 *    - Block Length      - 16
 *    - Bitpool           - 12 to 24
 *    - Allocation method - SBC_ALLOC_METHOD_LOUDNESS
 */
typedef struct {
  /**
   * bitpool setting for SBC encoded provided by GSound
   * to Target at runtime.
   *
   * This value can range from 12 to 24 depending on
   * the bandwidth available in the current connection.
   */
  uint32_t sbc_bitpool;

  /**
   * When true the side tone should be enabled at the same time the microphone
   * is opened.
   * Side tone should be disabled when the mic is closed by
   * GSoundTargetAudioInClose.
   */
  bool enable_sidetone;

  /**
   * When true the gsound_target_on_audio_in_ex must be called and raw samples
   * are required. This is used for 1st stage hotword evaluation. If
   * raw_samples_required is ever true on this platform then
   */
  bool raw_samples_required;

  /**
   * When true include sbc headers with encoded audio.
   *
   * By default always strip the 4 byte SBC headers before frames are sent to
   * GSound. If this value is true include the encoded_frame_len parameter to
   * gsound_target_on_audio_in_ex must include the 4 byte header.
   */
  bool include_sbc_headers;
} GSoundTargetAudioInSettings;

#ifdef __cplusplus
}
#endif

#endif  // GSOUND_TARGET_AUDIO_CONFIG_H
