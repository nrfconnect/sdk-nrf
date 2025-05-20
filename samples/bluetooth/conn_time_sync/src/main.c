/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include "conn_time_sync.h"

int main(void)
{
	char role;

	printk("Starting connection time sync sample.\n");
	console_init();

	do {
		printk("Choose device role - type c (central) or p (peripheral): ");

		role = console_getchar();

		switch (role) {
		case 'p':
			printk("\nPeripheral. Starting advertising\n");
			peripheral_start();
			break;
		case 'c':
			printk("\nCentral. Starting scanning\n");
			central_start();
			break;
		default:
			printk("\n");
			break;
		}
	} while (role != 'c' && role != 'p');
}
