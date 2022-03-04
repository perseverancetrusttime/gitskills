cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
	-Iapps/key \
	-Iplatform/drivers/bt \
	-Iservices/ai_voice/manager \
	-Iservices/nv_section/factory_section \
	$(BT_IF_INCLUDES) \
	-Iservices/nvrecord \
	-Iservices/ble_profiles/gfps/gfps_crypto/ \
	-Iservices/ai_voice/protocol/gma/encrypt \
	-Iservices/ai_voice/protocol/gma/gma_manager \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/ai_voice/codec/opus121/include \
	-Iutils/cqueue \
	-Iutils/sha256 \
	-Iutils/encrypt

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI_V1),1)
subdir-ccflags-y += \
    -Iservices/app_ibrt/inc \
    -Iservices/ibrt_core/inc
endif
ifeq ($(IBRT_CORE_V2),1)
subdir-ccflags-y += \
    -Iservices/app_ibrt_v2/inc \
    -Iservices/ibrt_core_v2/inc
endif
endif