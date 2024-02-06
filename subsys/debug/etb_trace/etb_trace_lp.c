/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <timeout_q.h>
#include <debug/etb_trace.h>

#include <etb_trace_private.h>

/* Flag to signal that we suspect a long enough sleep duration to justify turning off tracing. */
static bool trace_stopped;

/**
 * @brief Resume tracing on wakeup if it was stopped previously.
 *
 */
void etb_trace_on_idle_exit(void)
{
	if (trace_stopped) {
		etb_trace_start();

		trace_stopped = false;
	}
}

/**
 * @brief Turn off debug unit and tracing if going to sleep for a long period of time.
 *
 * @return always true to allow going to idle
 */
bool z_arm_on_enter_cpu_idle(void)
{
	/* Stop only if traces are enabled */
	if (trace_stopped) {
		return true;
	}

	/* FIXME: Zephyr internal APIs should be avoided */
	if (z_get_next_timeout_expiry() >
	    k_ms_to_ticks_ceil32(CONFIG_ETB_TRACE_LOW_POWER_MIN_IDLE_TIME_MS)) {
		etb_trace_stop();
		NRF_TAD_S->TASKS_CLOCKSTOP = TAD_TASKS_CLOCKSTOP_TASKS_CLOCKSTOP_Msk;
		trace_stopped = true;
	}

	return true;
}
