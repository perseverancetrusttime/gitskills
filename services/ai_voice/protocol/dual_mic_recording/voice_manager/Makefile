cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
    -Iapps/main \
    -Iapps/app_anker/device_drivers/bes_hal \
    -Iapps/audioplayers \
    -Iapps/battery \
    -Iapps/key \
    -Iplatform/hal \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iservices/ai_voice/algorithm \
    -Iservices/ai_voice/audio \
    -Iservices/ai_voice/codec/compression \
    -Iservices/ai_voice/codec/sbc \
    -Iservices/ai_voice/transport \
    -Iservices/ai_voice/manager \
    -Iservices/ai_voice/protocol/common_api/voice_bt \
    -Iservices/ai_voice/protocol/common_api/voice_ble \
    -Iservices/ai_voice/protocol/common_api/voice_gatt \
    -Iservices/app_ai/inc \
    -Iservices/app_ibrt/inc/  \
    -Iservices/ble_app/app_main/  \
    -Iservices/ble_stack/ble_ip/  \
    -Iservices/ble_stack/common/api  \
    -Iservices/ble_stack/hl/api \
    -Iservices/ble_stack/ke/api  \
    -Iservices/bt_app \
    -Iservices/bt_app/a2dp_codecs/include \
    $(BT_IF_INCLUDES) \
    -Iservices/ai_voice/codec/opus121/include \
    -Iservices/nvrecord \
    -Iservices/resources \
    -Iservices/ibrt_core/inc \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/multimedia/audio/codec/sbc/src/inc \
    -Iutils/crc16 \
    -Iutils/crc32 \
    -Iservices/app_ai/inc \
    -Iservices/nv_section/factory_section \
    -Ithirdparty/binaural_lib

ifeq ($(IBRT),1)

ifeq ($(IBRT_UI_V1),1)
ccflags-y += \
    -Iservices/ibrt_core/inc \
    -Iservices/ibrt_ui/inc \
    -Iservices/app_ibrt/inc
endif
ifeq ($(IBRT_CORE_V2),1)
ccflags-y += \
    -Iservices/app_ibrt_v2/inc \
    -Iservices/ibrt_core_v2/inc \
    -Iservices/custom_api
endif
ifeq ($(IBRT_UI_V2),1)
ccflags-y += \
    -Iservices/ibrt_ui_v2/inc
endif

endif