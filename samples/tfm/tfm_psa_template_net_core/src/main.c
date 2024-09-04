/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Network core firmware version: %d\r\n", CONFIG_FW_INFO_FIRMWARE_VERSION);
	return 0;
}
