/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("=== Primary Image: Demonstrating APPLICATIONLOCKUP trigger ===\n");
	printk("This image will intentionally trigger a CPU lockup.\n");
	printk("The UICR.SECONDARY.TRIGGER.APPLICATIONLOCKUP configuration will\n");
	printk("automatically boot the secondary image.\n");
	printk("\nTriggering CPU lockup now!\n");
	printk("Step 1: Disabling fault exceptions and then accessing invalid memory...\n");

	/* Set FAULTMASK to disable all exceptions except NMI.
	 * Then trigger a fault - since faults are disabled, this causes lockup.
	 * Any fault (unaligned access, invalid memory, etc.) will trigger lockup.
	 */
	__asm volatile("cpsid f" ::: "memory");

	/* Trigger fault by accessing invalid memory */
	*(volatile uint32_t *)0xFFFFFFFF = 0xDEADBEEF;

	/* This line will never be reached */
	printk("ERROR: Should have triggered lockup!\n");

	return 0;
}
