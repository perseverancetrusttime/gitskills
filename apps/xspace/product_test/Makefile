cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
		-Iutils/cqueue \
		-Iservices/bt_app \
		$(BT_IF_INCLUDES) \
		-Iservices/overlay \
		-Iplatform/hal \
		-Iplatform/drivers/bt \
		-Iplatform/drivers/norflash \
		-Iservices/app_tws/inc \
		-Iplatform/drivers/ana  \
		$(BT_PROFILES_INCLUDES) \
		-Iapps/audioplayers/a2dp_decoder \
		-Iservices/bt_app/a2dp_codecs/include \
		-Iservices/nv_section/factory_section \
		-Iapps/xspace/interface/sdk_interface \
		-Iservices/multimedia/audio/codec/sbc/inc \
		-Iservices/multimedia/audio/codec/sbc/src/inc \
		-Iapps/xspace/device_thread \
		-Iapps/xspace/interface/xspace_interface \
		-Iapps/xspace/thread\device_thread \
		-Iplatform/hal \
		-Iapps/xspace/ui/$(PROJ_NAME) \
		-Iapps/xspace/hal \
		-Iapps/xspace/hal/ntc \
		-Iapps/xspace/hal/pmu \
		-Iservices/app_ibrt/inc \
		-Iservices/ibrt_core/inc \
		-Iservices/bt_if_enhanced/inc \
		-Iservices/ibrt_ui/inc

		

ifeq ($(XSPACE_BATTERY_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_battery_manager
endif

ifeq ($(XSPACE_INEAR_DETECTION_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_inear_detect_manager
endif

ifeq ($(XSPACE_GESTURE_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_gesture_manager
endif

ifeq ($(XSPACE_COVER_SWITCH_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_cover_switch_manager
endif

ifeq ($(XSPACE_INOUT_BOX_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_inout_box_manager
endif

ifeq ($(XSPACE_XBUS_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_xbus_manager
endif

ifeq ($(XSPACE_FLASH_ACCESS),1)
ccflags-y += -Iapps/xspace/application/flash_access
endif
