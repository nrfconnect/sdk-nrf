/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("PROTECTEDMEM integrity check failed - secondary firmware booted\n");

	while (1) {
		k_msleep(1000);
	}

	return 0;
}
