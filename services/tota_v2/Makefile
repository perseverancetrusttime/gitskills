cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

TOTA_LIB_NAME := libtota

ifeq ($(USE_TRACE_ID), 1)
TOTA_LIB_NAME := $(TOTA_LIB_NAME)_log_compressed
endif

$(TOTA_LIB_NAME)-y := $(src_obj)

obj-y += $(TOTA_LIB_NAME).a

export BUDSODIN2_TOTA ?= 0
ifeq ($(BUDSODIN2_TOTA),1)
subdir-ccflags-y += -DBUDSODIN2_TOTA
endif

ifeq ($(TOTA_EQ_TUNING), 1)
CFLAGS_app_tota_general.o += -DTOTA_EQ_TUNING
endif

CFLAGS_app_tota_general.o += $(LDS_SECTION_FLAGS)

subdir-ccflags-y += -Iplatform/drivers/ana \
					-Iplatform/hal \
					-Iservices/bluedroid/adapter/src \
					-Iapps/key \
					-Iservices/ibrt_middleware/inc \
					-Iservices/audio_eq \
					-Iservices/fs/fat \
					-Iservices/fs/sd \
					-Iservices/fs/fat/ChaN \
					$(BT_IF_INCLUDES) \
					-Iservices/overlay \
					-Iservices/nvrecord \
					-Iservices/resources \
					-Iservices/multimedia/audio/codec/sbc/inc \
					-Iservices/bt_app/a2dp_codecs/include \
					-Iplatform/drivers/uarthci \
					-Iplatform/drivers/ana \
					-Iplatform/drivers/bt \
					-Iutils/cqueue \
					-Iutils/heap \
					-Iutils/retention_ram \
					-Iservices/audioflinger \
					-Iutils/lockcqueue \
					-Iutils/intersyshci \
                    -Iutils/boot_struct \
					-Iutils/encrypt \
					-Iutils/sha256 \
					-Iutils/kfifo \
					-Iapps/main \
					-Iapps/common \
					-Iapps/audioplayers \
					-Iservices/app_ai/inc \
					-Iapps/factory \
					-Iservices/bridge/ \
					-Iservices/bt_app \
					-Iservices/multimedia/speech/codec/codecs/adpcm \
					$(BLE_APP_INCLUDES) \
					$(BLE_STACK_INCLUDES) \
					-Iservices/anc_spp_tool \
                    -Iservices/tws/inc \
					-Iservices/app_ibrt_v2/inc \
					-Iservices/ibrt_core_v2/inc \
					-Iservices/ibrt_ui_v2/inc \
					-Iutils/crc32 \
					-Ithirdparty/userapi \
					-Iapps/battery \
					-Iservices/nv_section/factory_section \
					-Iservices/ \
					-Iservices/ibrt_middleware/inc/
					#-Iservices/app_ibrt/inc \
					#-Iservices/ibrt_core/inc \
					
