cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(VAD_IF_TEST),1)
ccflags-y += -DVAD_IF_TEST
endif

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += -Iplatform/drivers/sensor_hub -Iutils/cqueue -Iutils/heap -Iplatform/hal \
	-Iplatform/drivers/ana \
	-Itests/sensor_hub \
	-Itests/sensor_hub/sensor_hub_ai \
	-Iapps/common \
	-Iservices/audioflinger \
	-Iutils/crc32
