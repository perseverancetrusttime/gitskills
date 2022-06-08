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





///////所有存放音频txt的数组
///绝对路径：Sunwinon_2500YP\config\_default_cfg_src_\res\en


#ifdef MEDIA_PLAYER_SUPPORT

const uint8_t CN_PROMPT_ADAPTIVE [] = {
#include "res/en/SOUND_PROMPT_ADAPTIVE.txt"
};

const uint8_t EN_POWER_ON [] = {
#include "res/en/SOUND_POWER_ON.txt"
};

const uint8_t EN_POWER_OFF [] = {
#include "res/en/SOUND_POWER_OFF.txt"
};

const uint8_t EN_SOUND_ZERO[] = {
#include "res/en/SOUND_ZERO.txt"
};

const uint8_t EN_SOUND_ONE[] = {
#include "res/en/SOUND_ONE.txt"
};

const uint8_t EN_SOUND_TWO[] = {
#include "res/en/SOUND_TWO.txt"
};

const uint8_t EN_SOUND_THREE[] = {
#include "res/en/SOUND_THREE.txt"
};

const uint8_t EN_SOUND_FOUR[] = {
#include "res/en/SOUND_FOUR.txt"
};

const uint8_t EN_SOUND_FIVE[] = {
#include "res/en/SOUND_FIVE.txt"
};

const uint8_t EN_SOUND_SIX[] = {
#include "res/en/SOUND_SIX.txt"
};

const uint8_t EN_SOUND_SEVEN [] = {
#include "res/en/SOUND_SEVEN.txt"
};

const uint8_t EN_SOUND_EIGHT [] = {
#include "res/en/SOUND_EIGHT.txt"
};

const uint8_t EN_SOUND_NINE [] = {
#include "res/en/SOUND_NINE.txt"
};

const uint8_t EN_BT_PAIR_ENABLE[] = {
#include "res/en/SOUND_PAIR_ENABLE.txt"
};

const uint8_t EN_BT_PAIRING[] = {
#include "res/en/SOUND_PAIRING.txt"
};

const uint8_t EN_BT_PAIRING_FAIL[] = {
#include "res/en/SOUND_LANGUAGE_SWITCH.txt"
};

const uint8_t EN_BT_PAIRING_SUCCESS[] = {
#include "res/en/SOUND_PAIRING_SUCCESS.txt"
};

const uint8_t EN_BT_REFUSE[] = {
#include "res/en/SOUND_REFUSE.txt"
};

const uint8_t EN_BT_OVER[] = {
#include "res/en/SOUND_OVER.txt"
};

const uint8_t EN_BT_ANSWER[] = {
#include "res/en/SOUND_ANSWER.txt"
};

const uint8_t EN_BT_HUNG_UP[] = {
#include "res/en/SOUND_HUNG_UP.txt"
};

const uint8_t EN_BT_CONNECTED [] = {
#include "res/en/SOUND_CONNECTED.txt"
};

const uint8_t EN_BT_DIS_CONNECT [] = {
#include "res/en/SOUND_DIS_CONNECT.txt"
};

const uint8_t EN_BT_INCOMING_CALL [] = {
#include "res/en/SOUND_INCOMING_CALL.txt"
};

const uint8_t EN_CHARGE_PLEASE[] = {
#include "res/en/SOUND_CHARGE_PLEASE.txt"
};

const uint8_t EN_CHARGE_FINISH[] = {
#include "res/en/SOUND_CHARGE_FINISH.txt"
};

const uint8_t EN_LANGUAGE_SWITCH[] = {

};

const uint8_t EN_BT_WARNING[] = {
#include "res/en/SOUND_WARNING.txt"
};

const uint8_t EN_BT_ALEXA_START[] = {
#include "res/en/SOUND_ALEXA_START.txt"
};

const uint8_t EN_BT_ALEXA_STOP[] = {
#include "res/en/SOUND_ALEXA_STOP.txt"
};

const uint8_t EN_BT_ALEXA_NC[] = {
#include "res/en/SOUND_ALEXA_NC.txt"
};
const uint8_t EN_BT_GSOUND_MIC_OPEN[] = {
#include "res/en/SOUND_GSOUND_MIC_OPEN.txt"
};

const uint8_t EN_BT_GSOUND_MIC_CLOSE[] = {
#include "res/en/SOUND_GSOUND_MIC_CLOSE.txt"
};

const uint8_t EN_BT_GSOUND_NC[] = {
#include "res/en/SOUND_GSOUND_NC.txt"
};

const uint8_t EN_BT_SEALING_AUDIO[] = {
#include "res/en/SOUND_HM_SEALING_AUDIO.txt"
};

#if defined (__GESTURE_MANAGER_USE_PRESSURE__)
const uint8_t EN_BT_PROSSURE_DOWN[] = {
#include "res/en/SOUND_KEY_DOWN_MONO_16k.txt"
};

const uint8_t EN_BT_PROSSURE_UP[] = {
#include "res/en/SOUND_KEY_UP_MONO_16k.txt"
};
#endif

#ifdef __INTERACTION__
const uint8_t EN_BT_FINDME[] = {
#include "res/en/SOUND_FINDME.txt"
};
#endif
const U8 EN_FIND_MY_BUDS[] = {
#include "res/en/SOUND_FINDMYBUDS.txt"
};

//jinyao_learning:播放welcome提示音

#ifdef jinyao_learning
const uint8_t EN_BT_WELCOME[] = {
#include "res/en/SOUNDWELCOME.txt"
};



#endif
