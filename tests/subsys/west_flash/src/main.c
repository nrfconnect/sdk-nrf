/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(west_flash, LOG_LEVEL_INF);

#include <zephyr/kernel.h>

int main(void)
{
	int counter = 1;

	while (true) {
		LOG_INF("%d: Hello from %s", counter, CONFIG_BOARD_TARGET);
		counter++;
		k_msleep(1000);
	}
}
