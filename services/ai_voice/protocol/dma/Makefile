cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c :=
obj_cpp :=
obj_s :=

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_stream/*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_manager/*.cpp))
ifeq ($(AMA_VOICE),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_protobuf/dma.pb-c.c))
else
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_protobuf/*.c))
endif
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_ble/*.c))

obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_bt/*.cpp))

ifeq ($(BLE_V2),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_gatt_v2/*.c))
else
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dma_gatt/*.c))
endif

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

DMA_LIB_NAME := lib_dma

$(DMA_LIB_NAME)-y := $(src_obj)

obj-y := $(DMA_LIB_NAME).a

ifeq ($(BLE_V2),1)
DMA_BLE_INCLUDES += \
              -Iservices/ai_voice/protocol/dma/dma_gatt_v2
else
DMA_BLE_INCLUDES += \
              -Iservices/ai_voice/protocol/dma/dma_gatt
endif

ccflags-y +=  -Iapps/key \
			  -Iapps/main \
			  -Iapps/audioplayers \
			  -Iplatform/drivers/bt \
			  -Iplatform/hal \
			  -Iservices/ai_voice/protocol/dma/dma_ble \
			  -Iservices/ai_voice/protocol/dma/dma_bt \
			  $(DMA_BLE_INCLUDES) \
              -Iservices/ai_voice/protocol/dma/dma_stream \
			  -Iservices/ai_voice/protocol/dma/dma_protobuf \
			  -Iservices/ai_voice/protocol/dma/dma_manager \
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
			  -Iservices/nvrecord \
	          -Iservices/resources \
			  -Iutils/sha256 \
			  -Iutils/cqueue \
			  -Iutils/heap \
			  -Iutils/kfifo
