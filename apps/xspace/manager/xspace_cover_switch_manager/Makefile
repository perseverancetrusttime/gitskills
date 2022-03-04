cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += \
		-Iutils/cqueue \
		-Iplatform/hal \
		-Iapps/xspace/hal \
		$(BT_IF_INCLUDES) \
		$(BT_PROFILES_INCLUDES) \
		-Iapps/xspace/hal/touch \
		-Iapps/xspace/thread/device_thread \
		-Iapps/xspace/interface/xspace_interface
