/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>

#define WDT_THREAD_STACKSIZE (512)
#define WDT_THREAD_PRIORITY  (1)
#define WDT_THREAD_SLEEP     (200)
#define WDT_WINDOW_MAX	     (2 * WDT_THREAD_SLEEP)
