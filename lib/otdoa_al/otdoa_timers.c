/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <zephyr/kernel.h>

#include <stdint.h>
#include <otdoa_al/otdoa_al2otdoa_api.h>

struct k_timer otdoa_timers[OTDOA_MAX_TIMERS];

/* Timer "user data".  For now, it is just an array of uint32's that
 * holds the timer number.
 */
typedef uint32_t tTIMER_USER_DATA;

tTIMER_USER_DATA otdoa_timer_user_data[OTDOA_MAX_TIMERS];

/*
 * Expiry function
 */
void otdoa_timer_expiry_function(struct k_timer *pTimer)
{
	if (pTimer) {
		tTIMER_USER_DATA *pUserData = k_timer_user_data_get(pTimer);

		otdoa_handle_timeout(*pUserData);
	}
}

/*
 * Initialize the timers
 *
 */
int otdoa_timers_init(void)
{
	for (unsigned int ui = 0; ui < OTDOA_MAX_TIMERS; ui++) {
		otdoa_timer_user_data[ui] = (uint8_t)ui;
		k_timer_init(otdoa_timers + ui, otdoa_timer_expiry_function, NULL);
		/* set the timer user data to point to user data array */
		k_timer_user_data_set(otdoa_timers + ui, (void *)(otdoa_timer_user_data + ui));
	}
	return 0;
}

/*
 * Start / Stop the timers
 */
int otdoa_timer_start(unsigned int u_timer_no, unsigned int u_duration_ms)
{
	if (u_timer_no >= OTDOA_MAX_TIMERS) {
		return -1;
	}

	/* timer is started as one-shot, so period=K_NO_WAIT */
	k_timer_start(otdoa_timers + u_timer_no, K_MSEC(u_duration_ms), K_NO_WAIT);
	return 0;
}

int otdoa_timer_stop(unsigned int u_timer_no)
{
	if (u_timer_no >= OTDOA_MAX_TIMERS) {
		return -1;
	}

	k_timer_stop(otdoa_timers + u_timer_no);
	return 0;
}

/*
 * Check if a timer is running
 */
int otdoa_timer_active(unsigned int u_timer_no)
{
	if (u_timer_no >= OTDOA_MAX_TIMERS) {
		return 0;
	}
	if (k_timer_remaining_ticks(otdoa_timers + u_timer_no) > 0) {
		return 1;
	}
	return 0;
}
