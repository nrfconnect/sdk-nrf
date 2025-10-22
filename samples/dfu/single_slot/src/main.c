/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

int main(void)
{
	printk("Starting single_slot sample\n");

	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	printk("build time: " __DATE__ " " __TIME__ "\n");

	return 0;
}
