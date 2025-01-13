/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_flpr, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Multicore idle_flpr test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Main sleeps for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);

	return 0;
}
