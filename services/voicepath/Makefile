cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iplatform/drivers/ana \
    -Iplatform/hal \
    -Iservices/bluedroid/adapter/src \
    -Iapps/key \
    -Iapps/voice_assist/inc \
    -Iservices/audio_process \
    $(BT_PROFILES_INCLUDES) \
    $(BLE_APP_INCLUDES) \
    $(BLE_PROFILES_INCLUDES) \
    $(BLE_STACK_INCLUDES) \
    -Iservices/overlay \
    -Iservices/nvrecord \
    -Iservices/resources \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/retention_ram \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iapps/main \
    -Iapps/common \
    -Iapps/audioplayers \
    -Iapps/factory \
    -Iservices/voice_dev \
    -Iservices/voicepath/gsound \
    -Iservices/voicepath/gsound/gsound_target/ \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/bridge/ \
    $(BT_IF_INCLUDES) \
    -Iservices/multimedia/speech/application/Pcm8kCvsd/include \
    -Iservices/multimedia/speech/codec/codecs/adpcm \
    -Iservices/bt_app/ \
    -Iutils/crc32 \
    -Iservices/multimedia/opus121/include \
    -Iservices/multimedia/speech/process/lc_mmse_ns/include \
    -Iservices/multimedia/speech/process/ns_and_ec/include \
    -Iservices/multimedia/speech/process/speech_2mic_ns2_denoise/include \
    -Iservices/multimedia/speech/process/dual_mic_denoise/include \
    -Iservices/multimedia/speech/process/far_field_speech_enhancement/include \
    -Iservices/multimedia/speech/process/plc/include \
    -Iservices/multimedia/speech/process/common/include \
    -Ithirdparty/userapi \
    -Iutils/heap \
    -Iservices/voicepath \
    -Iservices/ota \
    -Iservices/ai_voice/manager \
    -Iservices/app_ai/inc \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/multimedia/audio/codec/sbc/src/inc \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/ibrt_ui_v2/inc \
    -Iapps/voice_detector

ifeq ($(GSOUND_HOTWORD_EXTERNAL),1)
subdir-ccflags-y += -Iplatform/drivers/cp_accel
endif
