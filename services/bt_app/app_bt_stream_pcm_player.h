/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#ifndef __PCM_PLAYER__
#define __PCM_PLAYER__

/* PCM_PLAYER is a pcm player based on app_bt_stream framework */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pcm_player_config {
    int bits;
    int sample_rate;
    int channel_num;
    int frame_length_samples;
} pcm_player_config_t;

typedef enum pcm_player_event {
    PCM_PLAYER_EVENT_MORE_DATA = 0,
} pcm_player_event_t;

typedef struct pcm_player_event_param {
    struct pcm_player *pcm_player;
    union {
        struct {
            unsigned char *buff;
            unsigned int   buff_len;
        } more_data;
    } p;
} pcm_player_event_param_t;

typedef int (*pcm_player_event_callback_t)(pcm_player_event_t event, pcm_player_event_param_t *param);

typedef struct pcm_player {
    void *priv;
    pcm_player_event_callback_t cb;
    pcm_player_config_t config;
} pcm_player_t;

// API
int pcm_player_open(pcm_player_t *pcm_player);
int pcm_player_start(pcm_player_t *pcm_player);
int pcm_player_stop(pcm_player_t *pcm_player);
int pcm_player_close(pcm_player_t *pcm_player);
int pcm_player_setup(pcm_player_t *pcm_player, pcm_player_config_t *config);

// app_bt_stream layer
int pcm_player_onoff(char onoff);

#ifdef __cplusplus
}
#endif

#endif /* __PCM_PLAYER__ */