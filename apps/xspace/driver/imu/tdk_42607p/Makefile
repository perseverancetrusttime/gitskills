cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

subdir-ccflags-y += \
		-Iapps/common \
		-Iplatform/hal \
		-Iapps/xspace/driver/bus \
		-Iapps/xspace/hal/imu \
		-Iapps/xspace/thread/device_thread \
		-Iapps/xspace/driver/imu/tdk_42607p/app/ \
        -Iapps/xspace/driver/imu/tdk_42607p/drivers/ \
        -Iapps/xspace/driver/imu/tdk_42607p/utils/

obj-y += app/ drivers/ utils/
