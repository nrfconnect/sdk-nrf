/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: timer configuring and processing */

#include <zephyr/kernel.h>

#include "timer_proc.h"

#include "ptt.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(timer);

static void ptt_process_timer_handler(struct k_timer *timer);
static void schedule_ptt_process(struct k_work *work);

K_TIMER_DEFINE(app_timer, ptt_process_timer_handler, NULL);

K_WORK_DEFINE(schedule_ptt_processor, schedule_ptt_process);

static void ptt_process_timer_handler(struct k_timer *timer)
{
	k_work_submit(&schedule_ptt_processor);
}

ptt_time_t ptt_get_current_time(void)
{
	return k_uptime_get_32();
}

ptt_time_t ptt_get_max_time(void)
{
	/* The maximum possible time is the maximum for a uint32_t */
	return UINT32_MAX;
}

void launch_ptt_process_timer(ptt_time_t timeout)
{
	if (timeout == 0) {
		/* schedule immediately */
		k_work_submit(&schedule_ptt_processor);
	} else {
		/* Schedule a single shot timer to trigger after the timeout */
		k_timer_start(&app_timer, K_MSEC(timeout), K_MSEC(0));
	}
}

/* scheduler context */
static void schedule_ptt_process(struct k_work *work)
{
	ptt_process(ptt_get_current_time());
}
