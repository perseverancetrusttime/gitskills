/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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

#define HEAP_SECTION_SIZE			0x400
#define STACK_SECTION_SIZE			0x1000

/* Linker script to configure memory regions. */
MEMORY
{
#if defined(RAMRET_SIZE) && (RAMRET_SIZE > 0)
	RAM		(rwx)	: ORIGIN = RAMRET_BASE,		LENGTH = RAMRET_SIZE
	RAMX	(rx)	: ORIGIN = RAMXRET_BASE,	LENGTH = RAMRET_SIZE
	BUFFER	(rw)	: ORIGIN = RAM_BASE,		LENGTH = RAM_SIZE
#else
	RAM		(rwx)	: ORIGIN = RAM_BASE,		LENGTH = RAM_SIZE
	RAMX	(rx)	: ORIGIN = RAMX_BASE,		LENGTH = RAM_SIZE
#endif
}

/* Library configurations */
GROUP(libgcc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
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
 *	 __buffer_start__
 *	 __buffer_end__
 */
ENTRY(Reset_Handler)

SECTIONS
{
	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	. = RAM_TO_RAMX(.);

	.text (.) : AT (RAMX_TO_RAM(ADDR(.text)))
	{
		__text_start__ = .;

		*(.reset_handler_sens)
		*(.text*)
		*(.boot_text_sram*)
		*(.boot_text_flash*)
		*(.sram_text*)
		*(.fast_text_sram*)
		*(.flash_text*)

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

	.rodata (.) :
	{
		*(.rodata*)
		*(.boot_rodata_sram*)
		*(.boot_rodata_flash*)
		*(.flash_rodata*)

		. = ALIGN(4);

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
		*(.boot_data*)
		*(.sram_data*)

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

		__core_bridge_task_cmd_table_start = .;
		KEEP(*(.core_bridge_task_cmd_table))
		__core_bridge_task_cmd_table_end = .;
		. = ALIGN(4);
		__core_bridge_instant_cmd_table_start = .;
		KEEP(*(.core_bridge_instant_cmd_table))
		__core_bridge_instant_cmd_table_end = .;

		/* All data end */
		. = ALIGN(4);
		__data_end__ = .;

	} > RAM

	.build_info (.) :
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > RAM = 0x00000000

	/* The following section be the last loaded section */
	.code_start_addr (.) :
	{
		LONG(ABSOLUTE(RAMX_TO_RAM(__text_start__)));
	} > RAM

	.boot_bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__boot_bss_sram_start__ = .;
		*(.boot_bss*)
		. = ALIGN(4);
		__boot_bss_sram_end__ = .;
	} > RAM

	.bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(.boot_bss*)
		*(.sram_bss*)
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

#if defined(RAMRET_SIZE) && (RAMRET_SIZE > 0)
	.buffer (ORIGIN(BUFFER)) (NOLOAD) :
	{
		__buffer_start__ = .;
		*(.buffer_section)
		__buffer_end__ = LENGTH(BUFFER);
	} > BUFFER
#else
	__buffer_start__ = .;
	__buffer_end__ = __buffer_start__;
#endif

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;
}
