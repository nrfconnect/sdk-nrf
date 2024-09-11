/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(load, LOG_LEVEL_INF);

#include "common.h"

static struct k_timer load_timer;
static atomic_t load_active;

void load_timer_handler(struct k_timer *dummy)
{
	atomic_set(&load_active, 0);
}

/* CPU load thread */
static void load_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	atomic_inc(&started_threads);

	atomic_set(&load_active, 1);

	k_timer_init(&load_timer, load_timer_handler, NULL);

	/* start a one-shot timer that expires after LOAD_THREAD_DURATION */
	k_timer_start(&load_timer, K_MSEC(LOAD_THREAD_DURATION), K_FOREVER);

	while (atomic_get(&load_active) == 1) {
		k_busy_wait(1000);
		k_yield();
	}

	LOG_INF("CPU load thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_load_id, LOAD_THREAD_STACKSIZE, load_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(LOAD_THREAD_PRIORITY), 0, 0);
