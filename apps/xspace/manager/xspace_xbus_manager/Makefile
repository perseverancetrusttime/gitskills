cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
		-Iservices/bt_app \
		-Iutils/cqueue \
		-Iservices/ota \
		-Iapps/xspace/hal \
		-Iservices/nvrecord \
		-Iplatform/drivers/bt \
		-Iservices/app_tws/inc \
		-Iplatform/drivers/ana  \
		-Iapps/xspace/hal/pmu \
		-Iapps/xspace/hal/xbus \
		-Iapps/xspace/device_thread \
		-Iapps/xspace/thread/device_thread \
		-Iservices/nv_section/factory_section \
		-Iapps/xspace/interface/xspace_interface \
		-Iapps/xspace/ui/$(PROJ_NAME) \
		-Iservices/app_tws/inc \
		$(BT_PROFILES_INCLUDES) \
		-Iservices/bt_if_enhanced/inc \
		-Iservices/bt_app/a2dp_codecs/include \
		-Iapps/xspace/interface/sdk_interface \
		-Iapps/xspace/interface/xspace_interface \
		-Iapps/xspace/algorithm/crc16 \
		-Iservices/multimedia/audio/codec/sbc/inc \
		-Iservices/multimedia/audio/codec/sbc/src/inc
		
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


ifeq ($(XSPACE_BATTERY_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_battery_manager
endif

ifeq ($(XSPACE_PRODUCT_TEST),1)
ccflags-y += -Iapps/xspace/product_test
endif

ifeq ($(XSPACE_XBUS_OTA),1)
ccflags-y += -Iapps/xspace/application/xbus_ota
endif

ifneq ($(findstring __XSPACE_BATTERY_MANAGER__,$(KBUILD_CPPFLAGS)), )
ccflags-y += -Iapps/xspace/hal/charger
endif

