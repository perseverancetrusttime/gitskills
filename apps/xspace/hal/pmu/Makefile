cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj-y := $(obj-y:.cpp=.o)


subdir-ccflags-y += \
		-Iplatform/hal \
		-Iapps/xspace/hal \
		-Iplatform/drivers/ana	\
		-Iapps/xspace/interface/xspace_interface

