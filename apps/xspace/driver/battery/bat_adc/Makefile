cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj-y:.cpp=.o)

ccflags-y += \
		-Iapps/common \
		-Iplatform/hal \
		-Iapps/xspace/hal/battery \
		-Iapps/xspace/thread/device_thread \
		-Iapps/xspace/driver/battery/bat_coulombmeter/adc_bat_cap_coulombmeter \
		-Iapps/xspace/hal/charger \
		-Iapps/xspace/application/flash_access \
		-Iapps/xspace/interface/xspace_interface \
		-Iplatform/drivers/ana

ifeq ($(IBRT),1)
ccflags-y += -Iservices/tws_ibrt/inc
ccflags-y += -Iservices/app_ibrt/inc
ccflags-y += -Iservices/bt_if/inc
ccflags-y += -Iservices/bt_if_enhanced/inc
endif

ifeq ($(XSPACE_BATTERY_MANAGER),1)
ccflags-y += -Iapps/xspace/manager/xspace_battery_manager
endif	