/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(timer, LOG_LEVEL_INF);

#include "common.h"

static struct k_timer my_timer;
static int timer_expire_count;

void my_work_handler(struct k_work *work)
{
	timer_expire_count++;
	LOG_INF("Timer expired %d times", timer_expire_count);
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&my_work);
}

/* Timer thread */
static void timer_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	atomic_inc(&started_threads);

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* start a periodic timer that expires once every TIMER_THREAD_PERIOD */
	k_timer_start(&my_timer, K_MSEC(TIMER_THREAD_PERIOD), K_MSEC(TIMER_THREAD_PERIOD));

	while (timer_expire_count < TIMER_THREAD_COUNT_MAX) {
		/* Wait for condition to be meet */
		k_msleep(100);
	}
	k_timer_stop(&my_timer);

	LOG_INF("Timer thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_timer_id, TIMER_THREAD_STACKSIZE, timer_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(TIMER_THREAD_PRIORITY), 0, 0);
