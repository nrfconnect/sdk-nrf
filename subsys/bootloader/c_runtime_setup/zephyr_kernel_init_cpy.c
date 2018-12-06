/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <offsets_short.h>
#include <kernel.h>
#include <misc/stack.h>
#include <random/rand32.h>
#include <linker/sections.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <device.h>
#include <init.h>
#include <linker/linker-defs.h>
#include <ksched.h>
#include <version.h>
#include <string.h>
#include <misc/dlist.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <entropy.h>
#include <logging/log_ctrl.h>
#include <tracing.h>
#include <stdbool.h>

#define MAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE
K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
K_THREAD_STACK_DEFINE(_main_stack, MAIN_STACK_SIZE);

void _bss_zero(void)
{
	(void)memset(&__bss_start, 0,
		     ((u32_t) &__bss_end - (u32_t) &__bss_start));
}

void _data_copy(void)
{
	(void)memcpy(&__data_ram_start, &__data_rom_start,
		 ((u32_t) &__data_ram_end - (u32_t) &__data_ram_start));
}
