cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

ccflags-y += -DMAKE_CRC32_TABLE

ccflags-y += -Iutils/heap -Iutils/libc/inc

