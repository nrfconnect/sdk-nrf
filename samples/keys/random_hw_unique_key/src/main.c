/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <hw_unique_key.h>

void main(void)
{
	printk("Writing random keys to KMU.\n\r");
	hw_unique_key_write_random();
	printk("Success!\n\r");
}
