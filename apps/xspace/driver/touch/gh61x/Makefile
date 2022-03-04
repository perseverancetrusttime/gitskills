cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)
		
APP_CTRL_LIB_NAME := libgh61X_ctrl_hard
#APP_CTRL_LIB_NAME := libgh61x_ctrl_soft

obj-y += $(APP_CTRL_LIB_NAME).a

subdir-ccflags-y += \
		-Iapps/xspace/hal \
		-Iapps/xspace/hal/touch \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/thread/device_thread \
		-Iservices/nv_section/factory_section \
		-Iapps/xspace/interface/xspace_interface \
		-Iapps/xspace/application/flash_access