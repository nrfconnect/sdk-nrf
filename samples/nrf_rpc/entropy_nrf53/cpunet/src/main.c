/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

int main(void)
{
	/* The only activity of this application is interaction with the APP
	 * core using serialized communication through the nRF RPC library.
	 * The necessary handlers are registered through nRF RPC interface
	 * and start at system boot.
	 */
	printk("Entropy sample started[NET Core].\n");
	return 0;
}
