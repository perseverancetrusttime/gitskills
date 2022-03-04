cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)
obj-y := $(obj-y:.cpp=.o)

ccflags-y += -Iapps/audioplayers/a2dp_decoder
ccflags-y += -Iservices/bt_if_enhanced/inc
ccflags-y += -Iinclude/cmsis_dsp



obj-y += lib_lt.a
