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
	-Iplatform/drivers/ana/ \
	-Iservices/bt_app \
    $(BT_IF_INCLUDES) \
	-Iapps/xspace/hal \
	-Iservices/bt_app \
	-Iservices/ble_app_v2/app_gfps \
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
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	-Iapps/anc/src/assist \
	-Iapps/hearing_detec

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
ccflags-y += -Iapps/xspace/hal/xbus
endif

ifeq ($(XSPACE_PRODUCT_TEST),1)
ccflags-y += -Iapps/xspace/product_test
endif

ifneq ($(findstring __XSPACE_BATTERY_MANAGER__,$(KBUILD_CPPFLAGS)), )
ccflags-y += -Iapps/xspace/hal/charger
endif

ifneq ($(findstring __XSPACE_TWS_SWITCH__,$(KBUILD_CPPFLAGS)), )
ccflags-y += -Iapps/xspace/application/xspace_tws_switch
endif

ifneq ($(findstring __XSPACE_IMU_MANAGER__,$(KBUILD_CPPFLAGS)), )
ccflags-y += -Iapps/xspace/manager/xspace_imu_manager
endif
