cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

subdir-ccflags-y += \
		-Iapps/xspace/hal \
		-Iapps/xspace/hal/pressure \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/thread/device_thread \
		-Iplatform/hal \
		-Iservices/interconnection/umm_malloc