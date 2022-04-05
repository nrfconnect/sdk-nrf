/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>

int main(void)
{
	printk("Hello world from %s\n", CONFIG_BOARD);

	return 0;
}
