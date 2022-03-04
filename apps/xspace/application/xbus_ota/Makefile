cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
    -Iapps/xspace/application/online_ota \
    -Iapps/xspace/ui/$(PROJ_NAME) \
    -Iplatform/cmsis/inc \
	-Iinclude/cmsis_dsp \
    -Iplatform/drivers/ana/best1501 \
    -Iplatform/drivers/ana \
    -Iapps/xspace/algorithm/crc16 \
    -Iapps/xspace/algorithm/sha256 \
    -Iapps/xspace/algorithm/cache \
	-Iservices/ota/bes_ota/inc \
	-Iservices/ota \
	-Iservices/ble_stack_v2/modules/common/api