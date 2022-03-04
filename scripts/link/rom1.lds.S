/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#include "plat_addr_map.h"

#define STACK_SECTION_SIZE 0
#define HEAP_SECTION_SIZE 0

#if defined(CHIP_BEST1501)
#define ROM1X_ORIGIN (0x30000)
#define ROM1D_ORIGIN (0x24010000)
#define ROM1_SIZE    (0x8000)

#define RESV_RAM_TOP    (0x20040000)
#define RESV_RAM_SIZE   (0x40)
#endif

#if defined(CHIP_BEST1305)
#define ROM1X_ORIGIN (0x28000)
#define ROM1D_ORIGIN (0x28028000)
#define ROM1_SIZE    (0x8000)

#define RESV_RAM_TOP    (0x20080000)
#define RESV_RAM_SIZE   (0x40)
#endif

#define ROMX_TO_ROMD(a)			((a) - ORIGIN(ROMX) + ORIGIN(ROMD))
#define ROMD_TO_ROMX(a)			((a) - ORIGIN(ROMD) + ORIGIN(ROMX))

/* Linker script to configure memory regions. */
MEMORY
{
  ROMX (rx)  : ORIGIN = ROM1X_ORIGIN, LENGTH = ROM1_SIZE
  ROMD (rx)  : ORIGIN = ROM1D_ORIGIN, LENGTH = ROM1_SIZE
  RAM_RSV4ROM (rwx) : ORIGIN = RESV_RAM_TOP - RESV_RAM_SIZE,LENGTH = RESV_RAM_SIZE 
}

/* Library configurations */
/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exidx_start
 *   __exidx_end
 *   __rom_etext
 *   __rom_data_start__
 *   __rom_data_end__
 *   __rom_bss_start__
 *   __rom_bss_end__
 *   __rom_HeapLimit
 *   __rom_StackLimit
 *   __rom_StackTop
 *   __rom_stack
 *   __Vectors_End
 *   __Vectors_Size
 *   __free_rom_ram
 *   __rom_end
 *   __free_rom
 *   __audio_const_rom_start
 *   __audio_const_rom_end
 *   __audio_const_rom_size
 */
ENTRY(rom1_entry)

SECTIONS
{
	.rom_text :
	{
		KEEP(*:main.o(.text.*rom1_entry*))
		KEEP(*:extern_functions.o(.text.*impure*))

        __sbc_text_start__ = .;
        KEEP(*:sbc.o          (.text*))
        __sbc_text_end__ = .;
        __sbc_text_size__ = __sbc_text_end__ - __sbc_text_start__;

		. = ALIGN(4);

	} > ROMX

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ROMX

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ROMX
	__exidx_end = .;

	.rodata (ROMX_TO_ROMD(__exidx_end)) : AT(__exidx_end)
	{
        KEEP(*:main.o(.rodata.*exp_funcs*))

        KEEP(*:sbc.o          (.rodata*))
        *(.rodata*)
        *(.flash_rodata*)
		KEEP(*(.note.gnu.build-id))
		KEEP(*(.eh_frame*))

	} > ROMD

	__rom_etext = (__exidx_end + SIZEOF(.rodata));

	.rom_bss (NOLOAD) :
	{
		. = ALIGN(4);
		__rom_bss_start__ = .;
        __rom_bss_end__ = .;

    } > RAM_RSV4ROM

	.rom_data : AT (__rom_etext)
	{
        . = ALIGN(4);
		__rom_data_start__ = .;
    } > RAM_RSV4ROM

	.rom_heap (NOLOAD) :
	{
        . = ALIGN(4);
		__rom_HeapBase = .;
		__end__ = .;
		end = __end__;
		. = HEAP_SECTION_SIZE;
		__rom_HeapLimit = .;
	} > RAM_RSV4ROM

	.scratch_buf (NOLOAD) :
	{
		. = ALIGN(4);
		*(.scratch_buf)
	} > RAM_RSV4ROM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.rom_stack_dummy (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM_RSV4ROM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__rom_StackTop = ORIGIN(RAM_RSV4ROM) + LENGTH(RAM_RSV4ROM);
	__rom_StackLimit = __rom_StackTop - SIZEOF(.rom_stack_dummy);
	PROVIDE(__rom_stack = __rom_StackTop);

	__StackTop = __rom_StackTop;
	PROVIDE(__stack = __rom_StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__rom_StackLimit >= __rom_HeapLimit, "region RAM overflowed with stack")
	__free_rom_ram = __rom_StackLimit - __rom_HeapLimit;

	__rom_end = __rom_etext + SIZEOF(.rom_data);

	ASSERT(__rom_end < (ORIGIN(ROMX) + LENGTH(ROMX)), "region ROM overflowed")
	__free_rom = ((ORIGIN(ROMX) + LENGTH(ROMX))) - __rom_end;
}

