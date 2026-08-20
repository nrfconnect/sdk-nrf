/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/logging/log.h>
#include "bt_test.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MONITORING_DEAD_TIME_MS 2500

atomic_t started_threads;

int main(void)
{

#if defined(CONFIG_BT)
	int ret;
#endif
	uint32_t cpu_load;

	LOG_INF("Power load performance benchmark %s", CONFIG_BOARD_TARGET);

#if defined(CONFIG_BT)
	ret = bt_start();
	if (ret != 0) {
		LOG_ERR("BT advertising failed to start: %d", ret);
	} else {
		LOG_INF("BT advertising started");
		atomic_inc(&started_threads);
	}
#endif

	cpu_load_get(true);

	while (1) {
		cpu_load = cpu_load_get(true);
		LOG_INF("Active threads: %u", (uint32_t)atomic_get(&started_threads));
		LOG_INF("CPU load %%: %u,%u", cpu_load / 10, cpu_load % 10);
		k_msleep(MONITORING_DEAD_TIME_MS);
	}

	return 0;
}
