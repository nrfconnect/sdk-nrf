/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

void main(void)
{
	printk("Should not be printed, expected TF-M's NS application to be run instead.\n");
	k_panic();
}
