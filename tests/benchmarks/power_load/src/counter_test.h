/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/counter.h>

#define COUNTER_THREAD_STACKSIZE (512)
#define COUNTER_THREAD_PRIORITY	 (1)
#define COUNTER_THREAD_SLEEP	 (100)

#define ALARM_CHANNEL_ID (0)
