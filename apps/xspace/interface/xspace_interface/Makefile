
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
	-Iplatform/drivers/bt \
	-Iservices/app_tws/inc \
	-Iplatform/drivers/ana  \
	$(BT_PROFILES_INCLUDES) \
	-Iapps/audioplayers/a2dp_decoder \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/nv_section/factory_section \
	-Iapps/xspace/interface/sdk_interface \
	-Iservices/ibrt_core_v2/inc \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/resources
	
	
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

ifneq ($(PROJ_NAME),)
ccflags-y += -Iapps/xspace/ui/$(PROJ_NAME)
endif


#obj-y += lib_xspace_interface.a
