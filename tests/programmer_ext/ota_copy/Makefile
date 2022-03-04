cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

PROGRAMMER_EXT_LIB_NAME := libprogrammer_ext
$(PROGRAMMER_EXT_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)
obj-y += $(PROGRAMMER_EXT_LIB_NAME).a

ccflags-y += \
    -Iplatform/drivers/ana \
    -Itests/programmer/inc \
    -Iutils/boot_struct \
	-Iutils/crc32 \
	-Iutils/crc16 \
    -Iutils/rom_utils 
    
ifeq ($(XSPACE_PROJ),1)
ccflags-y += \
    -Iutils/heap \
	-Iapps/xspace/algorithm/xz \
	-Iapps/xspace/application/ota_bin_check
endif	