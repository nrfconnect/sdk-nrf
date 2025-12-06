/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

volatile int counter = 1;

int main(void)
{
	while (true) {
		k_busy_wait(500000);
		counter++;
		if (counter > 1000) {
			counter = 0;
		}
	}
}
