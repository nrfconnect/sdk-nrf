/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("=== Hello World from Secondary Image ===\n");

	printk("Secondary image initialization complete\n");

	/* Keep the secondary image running */
	while (1) {
		k_msleep(3000);
		printk("Secondary image heartbeat\n");
	}

	return 0;
}
