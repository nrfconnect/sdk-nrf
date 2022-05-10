/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>

#include <zboss_api.h>
#include <zb_types.h>
#include <zb_osif_platform.h>


/*
 * Get uptime value in ms.
 */
static zb_uint64_t zb_osif_timer_get_ms(void)
{
	return (zb_uint64_t)k_uptime_get();
}

/*
 * Get ZBOSS time value in beacon intervals.
 * The single beacon interval duration in us is 15360us.
 */
zb_time_t zb_timer_get(void)
{
	return (10ULL * zb_osif_timer_get_ms()) / ((zb_uint64_t)(ZB_BEACON_INTERVAL_USEC / 100U));
}

void zb_osif_timer_stop(void)
{
}

void zb_osif_timer_start(void)
{
}

zb_bool_t zb_osif_timer_is_on(void)
{
	return ZB_TRUE;
}

/*
 * Get current time, us.
 */
zb_uint64_t osif_transceiver_time_get_long(void)
{
	return (zb_osif_timer_get_ms() * 1000);
}

zb_time_t osif_transceiver_time_get(void)
{
	return (zb_time_t)osif_transceiver_time_get_long();
}

void osif_sleep_using_transc_timer(zb_time_t timeout_us)
{
	zb_uint64_t tstart = osif_transceiver_time_get_long();
	zb_uint64_t tend = tstart + timeout_us;

	if (tend < tstart) {
		while (tend < osif_transceiver_time_get_long()) {
			zb_osif_busy_loop_delay(10);
		}
	} else {
		while (osif_transceiver_time_get_long() < tend) {
			zb_osif_busy_loop_delay(10);
		}
	}
}
