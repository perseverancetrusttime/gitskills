cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

subdir-ccflags-y += \
		-Iapps/common \
		-Iplatform/hal \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/hal/charger \
		-Iplatform/hal \
		-Iplatform/drivers/ana \
		-Iapps/xspace/thread/device_thread

#CFLAGS_sy5500_adapter.o += -D__CHARGER_SY5500__