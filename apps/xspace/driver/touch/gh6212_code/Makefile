cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
#obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

#$(warning "$(cur_dir)")

obj-y := $(obj_c:.c=.o)
#obj-y += src/

APP_CTRL_LIB_NAME := libgh621x_drv_cortexM4l_band_gcc_hard
obj-y += $(APP_CTRL_LIB_NAME).a

subdir-ccflags-y += \
		-Iapps/xspace/hal \
		-Iapps/xspace/hal/touch \
		-Iapps/xspace/driver/touch/gh6212_code/lib \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/thread/device_thread \
		-Iservices/nv_section/factory_section \
		-Iservices/ai_voice/protocol/ama/ama_manager \
		-Iapps/xspace/interface/xspace_interface \
		-Iapps/xspace/application/flash_access \
		-Iplatform/hal
		
