/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(idle);

int main(void)
{
	LOG_INF("SCFW HPU temperature measurement feature test, %s", CONFIG_BOARD_TARGET);
	while (1) {
		LOG_INF("Going to sleep forever");
		k_sleep(K_FOREVER);
	}

	return 0;
}
