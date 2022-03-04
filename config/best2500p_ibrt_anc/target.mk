include config/best2500p_ibrt/target.mk

#### ANC DEFINE START ######
export ANC_APP ?= 1

#### ANC CONFIG ######
export ANC_FB_CHECK         ?= 0
export ANC_FF_CHECK         ?= 0
export ANC_FF_ENABLED	    ?= 1
export ANC_FB_ENABLED	    ?= 1
export ANC_ASSIST_ENABLED   ?= 1
export AUDIO_ANC_TT_HW      ?= 1
export AUDIO_ANC_FB_MC      ?= 0
export AUDIO_ANC_FB_MC_HW   ?= 1
export AUDIO_ANC_FB_ADJ_MC  ?= 0
export AUDIO_SECTION_SUPPT  ?= 1
export AUDIO_ANC_SPKCALIB_HW ?= 0
export AUD_SECTION_STRUCT_VERSION ?= 3
##### ANC DEFINE END ######

export PSAP_APP  ?= 0

AUDIO_HW_LIMITER ?= 0
ifeq ($(AUDIO_HEARING_COMPSATN),1)
AUDIO_HW_LIMITER := 1
export PSAP_APP_ONLY_MUSIC := 1
endif
ifeq ($(AUDIO_HW_LIMITER),1)
export PSAP_APP := 1
endif

export AUDIO_OUTPUT_DAC2 ?= 0

APP_ANC_TEST ?= 1
ifeq ($(APP_ANC_TEST),1)
export TOTA_v2 := 1
endif

ifeq ($(ANC_APP),1)
KBUILD_CPPFLAGS += \
    -DANC_APP \
    -D__BT_ANC_KEY__
endif

ifeq ($(USE_CYBERON),1)

export THIRDPARTY_LIB ?= cyberon
KBUILD_CPPFLAGS += -D__CYBERON

export KWS_IN_RAM := 0
ifeq ($(KWS_IN_RAM),1)
CPPFLAGS_${LDS_FILE} += -DKWS_IN_RAM
endif #KWS_IN_RAM

endif #USE_CYBERON

export HAS_BT_SYNC = 1