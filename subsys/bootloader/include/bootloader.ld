/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#define _LINKER
#define _ASMLANGUAGE
#include <autoconf.h>
#include <linker/sections.h>
#include <generated_dts_board.h>
#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define RAM SRAM
#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#define ROMABLE_REGION FLASH
#define RAMABLE_REGION SRAM
#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)

MEMORY
{
    FLASH                 (rx) : ORIGIN = CONFIG_FLASH_BASE_ADDRESS + FLASH_AREA_SECURE_BOOT_OFFSET_0, LENGTH = FLASH_AREA_SECURE_BOOT_SIZE_0
    SRAM                  (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
    IDT_LIST  (wx)      : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
}


SECTIONS
    {
	SECTION_PROLOGUE(text0,,)
	{
	. = CONFIG_TEXT_SECTION_OFFSET;

#if defined(CONFIG_SW_VECTOR_RELAY)
	KEEP(*(.vector_relay_table))
	KEEP(*(".vector_relay_table.*"))
	KEEP(*(.vector_relay_handler))
	KEEP(*(".vector_relay_handler.*"))
#endif
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_SW_VECTOR_RELAY)
	/* Reserved 4 bytes to save vector table base address */
	SECTION_PROLOGUE(.vt_pointer,(NOLOAD),)
	{
		*(.vt_pointer_section)
		*(".vt_pointer_section.*")
	} GROUP_LINK_IN(RAMABLE_REGION)
#endif
    }

/* Set this first so we are sure relevant sections are placed where the MDK expect it to be */
#include <nrf_common.ld>

/* Define Zephyr symbols for compatibility with Zephyr code. */
_main_stack = __StackLimit;
__reset = Reset_Handler;

SECTIONS
    {

#include <linker/rel-sections.ld>
    SECTION_PROLOGUE(.plt,,)
	{
	*(.plt)
	}

    SECTION_PROLOGUE(.iplt,,)
	{
	*(.iplt)
	}
	_image_rom_start = ROM_ADDR;

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	_vector_start = .;
	KEEP(*(.exc_vector_table))
	KEEP(*(".exc_vector_table.*"))

	KEEP(*(IRQ_VECTOR_TABLE))

	KEEP(*(.vectors))

	_vector_end = .;
	_image_text_start = .;
	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)

	/*
	 * These are here according to 'arm-zephyr-elf-ld --verbose',
	 * after .gnu.linkonce.t.*
	 */
	*(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)
	} > FLASH
	_image_text_end = .;
	#if defined (CONFIG_CPLUSPLUS)
	SECTION_PROLOGUE(.ARM.extab,,)
	{
	/*
	 * .ARM.extab section containing exception unwinding information.
	 */
	*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH
	#endif
	SECTION_PROLOGUE(.ARM.exidx,,)
	{
	/*
	 * This section, related to stack and exception unwinding, is placed
	 * explicitly to prevent it from being shared between multiple regions.
	 * It  must be defined for gcc to support 64-bit math and avoid
	 * section overlap.
	 */
	__exidx_start = .;
#if defined (__GCC_LINKER_CMD__)
	*(.ARM.exidx* gnu.linkonce.armexidx.*)
#endif
	__exidx_end = .;
	} > FLASH
	_image_rodata_start = .;
	#include <linker/common-rom.ld>
    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)
#ifdef CONFIG_SOC_RODATA_LD
#include <soc-rodata.ld>
#endif
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)
#if defined CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS
#if CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0
       SECTION_PROLOGUE(.gnu.sgstubs,CONFIG_ARM_NSC_REGION_BASE_ADDRESS,)
#else
       SECTION_PROLOGUE(.gnu.sgstubs,,)
#endif /* CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0 */
       {
       . = ALIGN(4);
       __sg_start = .;
       *(.gnu*)
       . = ALIGN(4);
       __sg_end = .;
       __sg_size = __sg_end - __sg_start;
       } GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */

	_image_rodata_end = .;
	_image_rom_end = .;

    GROUP_END(ROMABLE_REGION)
    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    SECTION_PROLOGUE(.got,,)
	{
	*(.got.plt)
	*(.igot.plt)
	*(.got)
	*(.igot)
	}

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
	__bss_start = .;
	__kernel_ram_start = .;

	KERNEL_INPUT_SECTION(.bss)
	KERNEL_INPUT_SECTION(".bss.*")
	KERNEL_INPUT_SECTION(COMMON)

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
	__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
        {
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
        KERNEL_INPUT_SECTION(.noinit)
        KERNEL_INPUT_SECTION(".noinit.*")

#ifdef CONFIG_SOC_NOINIT_LD
#include <soc-noinit.ld>
#endif

        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	__data_ram_start = .;
	KERNEL_INPUT_SECTION(.data)
	KERNEL_INPUT_SECTION(".data.*")
	*(".kernel.*")

#ifdef CONFIG_SOC_RWDATA_LD
#include <soc-rwdata.ld>
#endif

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	#include <linker/debug-sections.ld>

    SECTION_PROLOGUE(.ARM.attributes, 0,)
	{
	KEEP(*(.ARM.attributes))
	KEEP(*(.gnu.attributes))
	}
}
