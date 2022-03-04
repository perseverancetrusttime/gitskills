cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)

ccflags-y += \
		-Iapps/common \
		-Iplatform/hal \
		-Iplatform/hal \
		-Iapps/xspace/driver/battery/bat_adc \
		-Iapps/xspace/application/flash_access \
		-Iapps/xspace/hal/battery \
		-Iapps/xspace/hal/charger  \
		-Iapps/xspace/interface/xspace_interface
