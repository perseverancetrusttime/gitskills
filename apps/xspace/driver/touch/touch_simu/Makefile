cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

subdir-ccflags-y += \
		-Iapps/xspace/hal \
		-Iapps/xspace/hal/touch \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/thread/device_thread \
		-Iservices/nv_section/factory_section \
		-Iapps/main \
		-Iapps/xspace/application/flash_access
