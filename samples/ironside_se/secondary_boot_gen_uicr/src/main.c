/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <ironside/se/api.h>
#include <string.h>

int main(void)
{
	int err;

	printk("=== Hello World from Primary Image ===\n");

	printk("Booting secondary image\n");
	err = ironside_se_bootmode_secondary_reboot(NULL, 0);
	if (err != 0) {
		printk("Secondary image boot failed: %d\n", err);
		return err;
	}

	while (1) {
		printk("FAILURE: Primary should not be running - secondary boot failed\n");
		k_msleep(1000);
	}

	return 0;
}
