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

#define HEAP_SECTION_SIZE			0x1000

#define STACK_SECTION_SIZE			0x1100
#define CP_STACK_SECTION_SIZE		0x1000

#define NANDFLASH_REGION_BASE       FLASH_BASE
#define NANDFLASH_REGION_SIZE       FLASH_SIZE
#define ROM_HEAP_END		        0x2000

/* Linker script to configure memory regions. */
MEMORY
{
  ROM       (rx)    : ORIGIN = ROM_BASE,                LENGTH = ROM_SIZE	/* see plat_addr_map.h and common.mk */
  RAM       (rwx)   : ORIGIN = RAM_BASE,                LENGTH = RAM_SIZE
  RAMX      (rx)    : ORIGIN = RAMX_BASE,               LENGTH = RAM_SIZE
  NANDFLASH (r)	    : ORIGIN = NANDFLASH_REGION_BASE,   LENGTH = NANDFLASH_REGION_SIZE
}

/* Library configurations */
GROUP(libgcc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exec_struct_start
 *   __cust_cmd_init_tbl_start
 *   __cust_cmd_init_tbl_end
 *   __cust_cmd_hldr_tbl_start
 *   __cust_cmd_hldr_tbl_end
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 */
ENTRY(Boot_Loader)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.dummy_buffer (.) (NOLOAD) :
	{
		/* The size of .dummy_buffer should be greater than __rom_HeapLimit. In most cases 8K is enough. */
		. = ROM_HEAP_END - SIZEOF(.vector_table);
		. = ALIGN(4);
	} > RAM

    __nandflash_bin_load_start__ = .;
	.boot_struct (.) :
	{
		KEEP(*(.boot_struct))
		. = ALIGN(4);
	} > RAM
	.nand_boot_struct (.) :
	{
		LONG(__nandflash_bin_load_start__);
		LONG(__nandflash_bin_load_end__);
	} > RAM

	. = RAM_TO_RAMX(.);

	.text (.) : AT (RAMX_TO_RAM(ADDR(.text)))
	{
		*(.boot_loader)
		*(.text*)

#ifndef NOSTD
		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)
#endif

		. = ALIGN(4);
	} > RAMX

	.ARM.extab (.) : AT (RAMX_TO_RAM(ADDR(.ARM.extab)))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > RAMX

	__exidx_start = .;
	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx (.) : AT (RAMX_TO_RAM(ADDR(.ARM.exidx)))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > RAMX
	__exidx_end = .;

	. = RAMX_TO_RAM(.);

	.got_info (.) :
	{
		__got_info_start = .;
		__got_start = .;
		*(.got)
		. = ALIGN(4);
		__got_end = .;
		__got_plt_start = .;
		*(.got.plt)
		. = ALIGN(4);
		__igot_plt_start = .;
		*(.igot.plt)
		. = ALIGN(4);
		__dynamic_start = .;
		*(.dynamic)
		. = ALIGN(4);
		__got_info_end = .;
	} > RAM

	.rodata (.) :
	{
		*(.rodata*)

		. = ALIGN(4);
		__cust_cmd_init_tbl_start = .;
		KEEP(*(.cust_cmd_init_tbl))
		__cust_cmd_init_tbl_end = .;

		. = ALIGN(4);
		__cust_cmd_hldr_tbl_start = .;
		KEEP(*(.cust_cmd_hldr_tbl))
		__cust_cmd_hldr_tbl_end = .;

#ifndef NOSTD
		KEEP(*(.eh_frame*))
#endif
		*(.note.gnu.build-id)
		. = ALIGN(4);
	} > RAM

	__etext = .;

	.data (.) :
	{
		__data_start__ = .;
		*(.data*)
		. = ALIGN(4);

#ifndef NOSTD
		*(vtable)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
#endif
		/* All data end */
		__data_end__ = .;

	} > RAM

	.system_info (.) :
	{
		KEEP(*(.system_info_list_size))
		KEEP(*(.system_info_list))
		. = ALIGN(4);
	}  > RAM

	.build_info (.) :
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > RAM = 0x00000000

	/* The following section MUST be the last loaded section */
	.code_start_addr (.) :
	{
#ifdef TRACE_STR_SECTION
		LONG(ABSOLUTE(__rodata_str_start__));
#else
		LONG(0);
#endif
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(ORIGIN(NANDFLASH)));
	} > RAM

    __nandflash_bin_load_end__ = .;

	.bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.heap (.) (NOLOAD) :
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. += HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (.) (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;
}
