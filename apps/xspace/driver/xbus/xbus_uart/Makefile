cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj-y:.cpp=.o)

ccflags-y += \
        -Iapps/xspace/hal/xbus \
        -Iapps/xspace/interface/xspace_interface \
        -Iapps/xspace/thread/device_thread \
        -Iplatform/hal/best1501 \
        -Iplatform/drivers/ana/best1501
		
ifeq ($(XSPACE_PRODUCT_TEST),1)
ccflags-y += \
        -Iapps/xspace/product_test
endif
