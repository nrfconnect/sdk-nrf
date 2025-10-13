/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("\n=== Secondary Image: Successfully booted! ===\n");
	printk("The system automatically booted the secondary image due to\n");
	printk("APPLICATION LOCKUP in the primary image.\n");
	printk("\nThis demonstrates UICR.SECONDARY.TRIGGER.APPLICATIONLOCKUP\n");
	printk("automatic failover capability.\n");

	/* Secondary firmware runs continuously */
	while (1) {
		k_msleep(5000);
		printk("Secondary image heartbeat - system is stable\n");
	}

	return 0;
}
