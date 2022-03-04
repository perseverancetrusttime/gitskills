cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c :=
obj_cpp :=
obj_s :=

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_ota/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_manager/*.cpp))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_manager/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_crypto/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_ble/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_bt/*.cpp))

ifeq ($(BLE_V2),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_gatt_v2/*.c))
else
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gma_gatt/*.c))
endif

ifeq ($(BLE_V2),1)
GMA_BLE_INCLUDES += \
              -Iservices/ai_voice/protocol/gma/gma_gatt_v2
else
GMA_BLE_INCLUDES += \
              -Iservices/ai_voice/protocol/gma/gma_gatt
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

GMA_LIB_NAME := lib_gma

$(GMA_LIB_NAME)-y := $(src_obj)

obj-y := $(GMA_LIB_NAME).a

ccflags-y +=  -Iapps/key \
			  -Iapps/main \
			  -Iapps/common   \
			  -Iapps/audioplayers \
			  -Iapps/app_anker/device_drivers/bes_hal \
			  -Iapps/battery \
			  -Iplatform/drivers/bt \
			  -Iplatform/hal \
			  -Iplatform/drivers/ana \
			  -Iplatform/drivers/norflash \
			  -Iservices/ai_voice/protocol/gma/gma_ble \
			  -Iservices/ai_voice/protocol/gma/gma_bt \
			  -Iservices/ai_voice/protocol/gma/gma_gatt \
              -Iservices/ai_voice/protocol/gma/gma_ota \
			  -Iservices/ai_voice/protocol/gma/gma_crypto \
			  -Iservices/ai_voice/protocol/gma/gma_manager \
			  $(GMA_BLE_INCLUDES) \
			  -Iservices/ai_voice/algorithm \
			  -Iservices/ai_voice/audio \
			  -Iservices/ai_voice/codec/compression \
              -Iservices/ai_voice/codec/opus121/include \
			  -Iservices/ai_voice/transport \
			  -Iservices/ai_voice/manager \
              -Iservices/app_ai/voice_sbc \
              -Iservices/app_ai/inc \
			  -Iservices/bt_app \
			  -Iservices/bt_app/a2dp_codecs/include \
			  -Iservices/multimedia/speech/process/common/include \
			  -Iservices/ble_app/app_main \
			  -Iservices/ble_stack/ble_ip/  \
			  -Iservices/ble_stack/common/api  \
			  -Iservices/ble_stack/hl/api  \
			  -Iservices/ble_stack/hl/inc  \
			  -Iservices/ble_stack/ke/api  \
			  -Iservices/nvrecord \
	          -Iservices/resources \
              -Iservices/norflash_api   \
			  -Iservices/apps/common    \
			  -Itests/rom_cust \
			  -Iutils/cqueue \
			  -Iutils/heap \
			  -Iutils/kfifo \
	          -Iservices/multimedia/audio/codec/sbc/inc \
	          -Iservices/multimedia/audio/codec/sbc/src/inc \
			  $(BT_IF_INCLUDES) \
			  -Iservices/nv_section/factory_section \
			  -Iutils/encrypt \
			  -Iutils/crc32  \
			  -Iutils/crc16  \
			  -Iutils/sha256
			  
			  
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