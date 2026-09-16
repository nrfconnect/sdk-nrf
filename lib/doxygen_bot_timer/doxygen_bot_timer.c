/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "doxygen_bot_timer.h"

static bool timer_running;
static doxy_timer_callback_t timer_callback;
static uint32_t timer_remaining_ms;

int doxy_timer_start(uint32_t timeout_ms)
{
	timer_running = true;
	timer_remaining_ms = timeout_ms;

	return 0;
}

int doxy_timer_stop(void)
{
	timer_running = false;
	timer_remaining_ms = 0;

	return 0;
}

int doxy_timer_get_remaining(uint32_t *remaining_ms)
{
	if (remaining_ms == NULL) {
		return -EINVAL;
	}

	*remaining_ms = timer_running ? timer_remaining_ms : 0;

	return 0;
}

int doxy_timer_set_callback(doxy_timer_callback_t callback)
{
	timer_callback = callback;

	return 0;
}
