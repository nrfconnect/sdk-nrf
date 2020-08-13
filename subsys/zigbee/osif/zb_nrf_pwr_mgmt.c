/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/atomic.h>

#include <zboss_api.h>
#include "zb_nrf_platform.h"


#ifdef ZB_USE_SLEEP
static volatile atomic_t is_sleeping = ATOMIC_INIT(0);


/**
 *  SoC sleep subsystem initialization
 */
void zb_osif_sleep_init(void)
{
}

/**@brief Function which tries to put the MMCU into sleep mode,
 *        caused by an empty Zigbee stack scheduler queue.
 *
 * Function is defined as weak; to be redefined if someone wants
 * to implement their own going-to-sleep policy.
 */
__weak zb_uint32_t zb_osif_sleep(zb_uint32_t sleep_tmo)
{
	zb_uint32_t time_slept_ms = 0;

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
	return ZB_SLEEP_INVALID_VALUE;
#else

#if ZB_TRACE_LEVEL
	ZB_SET_TRACE_OFF();
#endif /* ZB_TRACE_LEVEL */

	/* Lock timer value from updating during sleep period. */
	ZVUNUSED(atomic_set((atomic_t *)&is_sleeping, 1));

	/* Wait for an event. */
	time_slept_ms = zigbee_event_poll(sleep_tmo);

	/* Unlock timer value updaes. */
	ZVUNUSED(atomic_set((atomic_t *)&is_sleeping, 0));

	/* Enable Zigbee stack-related peripherals. */
	/*zb_osif_priph_enable();*/

	return time_slept_ms;
#endif
}

/**@brief Function which is called after zb_osif_sleep
 *        finished and ZBOSS timer is reenabled.
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
