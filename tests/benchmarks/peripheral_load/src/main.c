/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "common.h"

/* Thread status */
atomic_t started_threads;
atomic_t completed_threads;

/* Watchdog status: */
extern volatile uint32_t wdt_status;

/* Main thread */
int main(void)
{
	/* Check status of flag that is set when watchdog fires */
	if (wdt_status == WDT_TAG) {
		LOG_INF("Starting after Watchdog has expired");
	} else {
		LOG_INF("That wasn't Watchdog");
	}
	wdt_status = 0U;

	/* Wait for threads to complete */
	while (atomic_get(&started_threads) < 1) {
		LOG_DBG("%ld threads were started", atomic_get(&started_threads));
		k_msleep(500);
	}
	while (atomic_get(&completed_threads) < atomic_get(&started_threads)) {
		LOG_DBG("%ld threads were started, %ld has completed",
			atomic_get(&started_threads), atomic_get(&completed_threads));
		k_msleep(500);
	}
	LOG_INF("main: all %ld threads have completed", atomic_get(&completed_threads));
}
