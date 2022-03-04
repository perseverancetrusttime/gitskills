cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)


ccflags-y += \
	-Iapps/main \
	-Iutils/cqueue \
	-Iapps/anc/inc \
	-Iservices/ota \
    $(BT_IF_INCLUDES) \
	-Iapps/xspace/hal \
	-Iservices/bt_app \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iplatform/drivers/bt \
	-Iapps/xspace/hal/pmu \
	-Iservices/app_tws/inc \
    $(BT_PROFILES_INCLUDES) \
	$(BLE_STACK_INCLUDES) \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iapps/xspace/interface/sdk_interface \
	-Iapps/xspace/interface/xspace_interface \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/resources \
	-Iservices/ota//bes_ota/inc \
	-Iplatform/drivers/norflash \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc

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
	-Iservices/ibrt_middleware/inc \
    -Iservices/custom_api
endif
ifeq ($(IBRT_UI_V2),1)
ccflags-y += \
    -Iservices/ibrt_ui_v2/inc
endif
endif

ifneq ($(PROJ_NAME),)
ccflags-y += -Iconfig/$(PROJ_NAME)
ccflags-y += -Iapps/xspace/ui/$(PROJ_NAME)
ccflags-y += -Iapps/xspace/thread/device_thread
endif