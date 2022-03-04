cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

PROGRAMMER_INFLASH_LIB_NAME := libprogrammer_inflash
$(PROGRAMMER_INFLASH_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)
obj-y += $(PROGRAMMER_INFLASH_LIB_NAME).a

ccflags-y := \
	-Iutils/boot_struct

ifeq ($(SINGLE_WIRE_DOWNLOAD),1)
CFLAGS_programmer_inflash_main.o += -DSINGLE_WIRE_DOWNLOAD
endif
