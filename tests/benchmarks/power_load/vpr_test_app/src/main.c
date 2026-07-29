/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define ACTIVE_TIME_US 2000 * 1000
#define SLEEP_TIME_MS 10

LOG_MODULE_REGISTER(vpr_main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Power load performance benchmark %s", CONFIG_BOARD_TARGET);

	while (1) {
		LOG_INF("Ping");
		k_busy_wait(ACTIVE_TIME_US);
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
