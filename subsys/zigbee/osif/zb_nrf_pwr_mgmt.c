/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/atomic.h>

#include <zboss_api.h>
#include "zb_nrf_platform.h"


#ifdef ZB_USE_SLEEP
static volatile atomic_t is_sleeping = ATOMIC_INIT(0);

/**
 * Enable ability to stop timer to save power.
 */
void zb_timer_enable_stop(void);


/**
 *  SoC sleep subsystem initialization
 */
void zb_osif_sleep_init(void)
{
	/* Disable timer in inactivity periods on all device types. */
	if (IS_ENABLED(CONFIG_ZIGBEE_TIME_COUNTER)) {
		zb_timer_enable_stop();
	}
}

/**@brief Function which tries to put the MMCU into sleep mode,
 *        caused by an empty Zigbee stack scheduler queue.
 *
 * Function is defined as weak; to be redefined if someone wants
 * to implement their own going-to-sleep policy.
 */
__weak zb_uint32_t zb_osif_sleep(zb_uint32_t sleep_tmo)
{
	/* The amount of microseconds that the ZBOSS task was blocked. */
	zb_uint32_t time_slept_us;
	/* The amount of milliseconds that the ZBOSS task was blocked. */
	zb_uint32_t time_slept_ms;

	if (!sleep_tmo) {
		return sleep_tmo;
	}

#if (ZIGBEE_TRACE_LEVEL != 0)
	/* In case of trace libraries - the Zigbee stack may generate
	 * logs on each loop iteration, resulting in immediate
	 * return from zigbee_event_poll() each time. In such case,
	 * Zigbee stack should not be forced to increment counters.
	 * Such solution may break the internal logic of the stack.
	 */
	ZVUNUSED(time_slept_ms);
	ZVUNUSED(time_slept_us);
	return ZB_SLEEP_INVALID_VALUE;
#else

#if ZB_TRACE_LEVEL
	ZB_SET_TRACE_OFF();
#endif /* ZB_TRACE_LEVEL */

	/* Lock timer value from updating during sleep period. */
	ZVUNUSED(atomic_set((atomic_t *)&is_sleeping, 1));

	/* Wait for an event. */
	time_slept_us = zigbee_event_poll(sleep_tmo * USEC_PER_MSEC);

	/* Calculate sleep duration in milliseconds. Round up the result
	 * using the basic time unit of ZBOSS API to avoid possible errors
	 * in the time unit conversion.
	 */
	time_slept_ms = ZB_TIME_BEACON_INTERVAL_TO_MSEC(
		DIV_ROUND_UP(time_slept_us, ZB_BEACON_INTERVAL_USEC));

	/* Unlock timer value updates. */
	ZVUNUSED(atomic_set((atomic_t *)&is_sleeping, 0));

	/* Enable Zigbee stack-related peripherals. */
	/*zb_osif_priph_enable();*/

	return time_slept_ms;
#endif
}

/**@brief Function which is called after zb_osif_sleep
 *        finished and ZBOSS timer is re-enabled.
 *
 * Function is defined as weak; to be redefined if someone
 * wants to implement their own going-to-deep-sleep policy/share resources
 * between Zigbee stack and other components.
 */
__weak void zb_osif_wake_up(void)
{
#if ZB_TRACE_LEVEL
	ZB_SET_TRACE_ON();
#endif /* ZB_TRACE_LEVEL */
	/* Restore trace interrupts. TODO: Restore something else if needed */
}

zb_bool_t zb_osif_is_sleeping(void)
{
	return atomic_get((atomic_t *)&is_sleeping) ? ZB_TRUE : ZB_FALSE;
}

#endif /* ZB_USE_SLEEP */
