obj-y := cmsis/ drivers/ hal/

ifneq ($(DUAL_BOOT),1)
obj-y += main/
endif

subdir-ccflags-y += -Iplatform/cmsis/inc -Iplatform/hal -Iutils/hwtimer_list -Iapps/common -Iapps/xspace/interface/xspace_interface -Iapps/xspace/product_test