cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj-y:.cpp=.o)


subdir-ccflags-y += \
		-Iapps/common \
		-Iplatform/hal \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/hal/charger \
		-Iplatform/hal \
		-Iplatform/drivers/ana \
		-Iapps/xspace/thread/device_thread \
		-Iapps/xspace/application/flash_access 
		