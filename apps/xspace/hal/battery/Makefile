cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj-y:.cpp=.o)


ccflags-y += \
		-Iapps/xspace/hal \
		-Iapps/xspace/driver/battery/bat_adc