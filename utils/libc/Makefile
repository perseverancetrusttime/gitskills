cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(NOSTD)-$(NUTTX_BUILD),1-0)
# Use relative path to specify the include path (so that no change is needed after rename)
ccflags-y := -I$(obj)/inc

ifeq ($(LIBC_ROM),1)
	obj-y := libc_rom.o
	obj-y += libc_rom_mem.o
	obj-y += strlen.o
	CFLAGS_libc_rom.o += -Iutils/rom_utils
	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
else
	obj-y := strsep.o strtok.o strncat.o strnicmp.o strstr.o memset.o memcpy.o memmove.o \
				atoi.o strtol.o strtoul.o qsort.o bsearch.o \
				vsprintf.o vsscanf.o
endif

# obj-y := $(filter-out $(EXCLUDE_OBJS),$(obj-y))

else ifeq ($(LIBC_ROM),1)

ifeq ($(NUTTX_BUILD),1)
	obj-y := libc_rom_mem.o
	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
else
	obj-y := libc_rom.o libc_rom_mem.o
	CFLAGS_libc_rom.o += -Iutils/rom_utils
	CFLAGS_libc_rom_mem.o += -Iutils/rom_utils
	# Avoid conflicting with libc in toolchain
	CFLAGS_libc_rom.o += -DNO_STRTOL
	CFLAGS_libc_rom.o += -DNO_STRTOUL
	CFLAGS_libc_rom.o += -DNO_VSSCANF
	CFLAGS_libc_rom.o += -DNO_ASSERT
endif

else ifeq ($(LIBC_OVERRIDE),1)

# Add more LIBC functions to override
	obj-y += machine/arm/
else

$(error Missing LIBC options)

endif

