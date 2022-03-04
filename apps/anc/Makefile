
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/assist/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

ifeq ($(ANC_APP),1)
CFLAGS_app_anc.o += -DANC_APP
endif

ifeq ($(ANC_FF_ENABLED),1)
CFLAGS_app_anc.o += -DANC_FF_ENABLED
CFLAGS_app_anc_fade.o += -DANC_FF_ENABLED
endif

ifeq ($(ANC_FB_ENABLED),1)
CFLAGS_app_anc.o += -DANC_FB_ENABLED
CFLAGS_app_anc_fade.o += -DANC_FB_ENABLED
endif

ifeq ($(PSAP_APP),1)
CFLAGS_app_anc.o += -DPSAP_APP
endif

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_app_anc.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
CFLAGS_app_anc.o += -DSW_PLAYBACK_RESAMPLE
endif
ifeq ($(SW_CAPTURE_RESAMPLE),1)
CFLAGS_app_anc.o += -DSW_CAPTURE_RESAMPLE
endif
ifeq ($(AUDIO_SECTION_SUPPT),1)
CFLAGS_app_anc.o += -D__AUDIO_SECTION_SUPPT__
endif

ifeq ($(ANC_ASSIST_PILOT_TONE_ALWAYS_ON),1)
CFLAGS_app_anc_assist.o += -DANC_ASSIST_PILOT_TONE_ALWAYS_ON
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)


subdir-ccflags-y += \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/ibrt_middleware/inc \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/integer_resampling/include \
    -Iservices/multimedia/audio/process/anc/include \
	-Iservices/multimedia/audio/process/psap/include \
    -Iservices/multimedia/speech/inc \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/anc/inc \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/factory \
	-Iapps/voice_detector \
	-Iservices/ble_app \
	-Iservices/bt_app \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/anc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/nv_section/aud_section \
    -Iservices/nv_section/include   \
    -Iutils/hwtimer_list \
	-Iservices/tota \
	$(BT_IF_INCLUDES) \
	$(BLE_STACK_INCLUDES) \
	-Iinclude/cmsis_dsp \
	-Iservices/audio_dump/include/

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI_V1),1)
subdir-ccflags-y += \
    -Iservices/app_ibrt/inc \
    -Iservices/ibrt_core/inc \
    -Iservices/ibrt_ui/inc
endif
ifeq ($(IBRT_CORE_V2),1)
subdir-ccflags-y += \
    -Iservices/app_ibrt_v2/inc \
    -Iservices/ibrt_core_v2/inc \
    -Iservices/ibrt_ui_v2/inc
endif
ifeq ($(IBRT_UI_V2),1)
subdir-ccflags-y += \
    -Iservices/ibrt_conn_mgr/inc
endif
endif

ifeq ($(ENABLE_CALCU_CPU_FREQ_LOG),1)
CFLAGS_app_anc_assist.o += -DENABLE_CALCU_CPU_FREQ_LOG
endif

ifeq ($(VOICE_ASSIST_WD_ENABLED),1)
CFLAGS_app_anc_assist.o += -DVOICE_ASSIST_WD_ENABLED
endif
