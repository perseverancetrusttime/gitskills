cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += -Iapps/main \
			  -Iapps/app_anker/device_drivers/bes_hal \
			  -Iapps/audioplayers \
			  -Iapps/battery \
			  -Iapps/key \
			  -Iplatform/hal \
			  -Iplatform/drivers/ana \
			  -Iplatform/drivers/bt \
			  -Iplatform/drivers/norflash \
			  -Iservices/ai_voice/algorithm \
			  -Iservices/ai_voice/audio \
			  -Iservices/ai_voice/codec/compression \
			  -Iservices/ai_voice/codec \
			  -Iservices/ai_voice/codec/sbc \
			  -Iservices/ai_voice/transport \
			  -Iservices/ai_voice/manager \
			  -Iservices/ai_voice/protocol/gma/gma_bt \
			  -Iservices/ai_voice/protocol/gma/gma_crypto \
			  -Iservices/ai_voice/protocol/gma/gma_ble \
			  -Iservices/ai_voice/protocol/gma/gma_gatt \
			  -Iservices/ai_voice/protocol/gma/gma_ota \
              -Iservices/app_ai/inc \
			  -Iservices/ble_app/app_main/  \
			  -Iservices/ble_stack/ble_ip/  \
			  -Iservices/ble_stack/common/api  \
			  -Iservices/ble_stack/hl/api \
			  -Iservices/ble_stack/ke/api  \
			  -Iservices/bt_app \
			  -Iservices/bt_app/a2dp_codecs/include \
			  -Iservices/tws/inc \
			  $(BT_IF_INCLUDES) \
			  -Iservices/multimedia/audio/codec/sbc/inc \
	          -Iservices/multimedia/audio/codec/sbc/src/inc \
			  -Iservices/ai_voice/codec/opus121/include \
			  -Iservices/nv_section/factory_section \
			  -Iservices/nvrecord \
	          -Iservices/resources \
			  -Iutils/cqueue \
			  -Iutils/heap \
			  -Iutils/crc16 \
			  -Iutils/crc32 \
			  -Iservices/norflash_api   \
			  -Iservices/apps/common    \
			  -Iapps/common   \
			  -Iservices/ble_app/app_main   \
			  -Iservices/ble_stack/hl/inc   \
			  -Iservices/ibrt_core/inc \
			  -Iservices/app_ibrt/inc \
              -Iservices/ota
