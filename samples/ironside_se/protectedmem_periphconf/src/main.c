/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>

#define PERIPHCONF_PARTITION_ADDRESS DT_FIXED_PARTITION_ADDR(DT_NODELABEL(periphconf_partition))
#define PERIPHCONF_PARTITION_SIZE DT_REG_SIZE(DT_NODELABEL(periphconf_partition))
#define MRAM_16BYTE_ALIGN 16
#define PARTITION_END (PERIPHCONF_PARTITION_ADDRESS + PERIPHCONF_PARTITION_SIZE)
#define LAST_16BYTE_AREA_START ((PARTITION_END - MRAM_16BYTE_ALIGN) & ~(MRAM_16BYTE_ALIGN - 1))
#define TEST_WRITE_ADDRESS (LAST_16BYTE_AREA_START + MRAM_16BYTE_ALIGN - sizeof(uint32_t))
#define TEST_PATTERN 0xDEADBEEFUL

int main(void)
{
	printk("Writing test pattern to protected memory...\n");

	*(volatile uint32_t *)TEST_WRITE_ADDRESS = TEST_PATTERN;

	printk("Rebooting. Secondary firmware should boot if protection is working.\n");

	k_msleep(50);
	NVIC_SystemReset();

	CODE_UNREACHABLE;
}
