/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		k_busy_wait(1000000);
		k_msleep(1000);
	}

	return 0;
}
