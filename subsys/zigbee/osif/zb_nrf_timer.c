/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <drivers/counter.h>
#include <soc.h>
#include <sys/atomic.h>

#include <zboss_api.h>
#include <zb_types.h>

#define ALARM_CHANNEL_ID  0
#define TIMER_INSTANCE    DT_LABEL(DT_NODELABEL(timer3))

typedef struct {
	struct device *device;
	struct counter_alarm_cfg alarm_cfg;
	u8_t alarm_ch_id;
	volatile zb_bool_t is_init;
	volatile atomic_t is_running;
} zb_timer_t;

static zb_timer_t zb_timer = {
	.is_init = ZB_FALSE,
	.is_running = ATOMIC_INIT(0)
};

/* Forward declaration, dependency to ZBOSS */
zb_void_t zb_osif_zboss_timer_tick(void);

/* Timer interrupt handler. */
static void zb_timer_alarm_handler(struct device *counter_dev, u8_t chan_id,
				   u32_t ticks, void *user_data)
{
	switch (chan_id) {
	case ALARM_CHANNEL_ID:
		if (atomic_set((atomic_t *)&zb_timer.is_running, 0) == 1) {
			/* The atomic flag is_running was 1 and now set to 0. */
			zb_osif_zboss_timer_tick();
		}
		break;

	default:
		/*Do nothing*/
		break;
	}
}

static void zb_osif_timer_set_default_config(zb_timer_t *timer)
{
	timer->alarm_ch_id = ALARM_CHANNEL_ID;
	timer->alarm_cfg.flags = 0;
	timer->alarm_cfg.ticks = counter_us_to_ticks(timer->device,
						     ZB_BEACON_INTERVAL_USEC);
	timer->alarm_cfg.callback = zb_timer_alarm_handler;
}

static void zb_timer_init(void)
{
	zb_timer.device = device_get_binding(TIMER_INSTANCE);
	__ASSERT(zb_timer.device != NULL, "Cannot bind timer device");

	zb_osif_timer_set_default_config(&zb_timer);

	zb_timer.is_init = ZB_TRUE;
}

void zb_osif_timer_stop(void)
{
	if (atomic_set((atomic_t *)&zb_timer.is_running, 0) == 1) {
		/* The atomic flag is_running was 1 and now set to 0. */
		counter_stop(zb_timer.device);
		counter_cancel_channel_alarm(zb_timer.device,
			zb_timer.alarm_ch_id);
	}
}

void zb_osif_timer_start(void)
{
	if (atomic_set((atomic_t *)&zb_timer.is_running, 1) == 0) {
		/* The atomic flag is_running was 0 and now set to 1. */
		if (zb_timer.is_init == ZB_FALSE) {
			zb_timer_init();
		}
		counter_start(zb_timer.device);

		/* Set a single shot alarm. */
		counter_set_channel_alarm(zb_timer.device,
			zb_timer.alarm_ch_id,
			&zb_timer.alarm_cfg);
	}
}

zb_bool_t zb_osif_timer_is_on(void)
{
	return atomic_get((atomic_t *)&zb_timer.is_running)
		? ZB_TRUE : ZB_FALSE;
}

/*
 * Get time value in us.
 * Time is calculated as number of beacon intervals
 * multiplied by beacon interval duration in us (15360us) and
 * accumulated with current timer value.
 * TODO: Consider using the RTC from Zephyr.
 */
zb_uint32_t zb_osif_timer_get(void)
{
	zb_uint32_t time_sys;
	zb_uint32_t time_cur;

	if (zb_osif_timer_is_on() == ZB_TRUE) {
		u32_t ticks;
		(void)counter_get_value(zb_timer.device, &ticks);
		time_cur = (zb_uint32_t)counter_ticks_to_us(zb_timer.device,
							    ticks);
	} else {
		time_cur = 0;
	}
	time_sys = ZB_TIME_BEACON_INTERVAL_TO_USEC(ZB_TIMER_GET());

	return time_sys + time_cur;
}

/*
 * Get current time, us.
 * TODO: Remove this wrapper and the zb_timer_get_value()
 *       should be used directly.
 */
zb_time_t osif_transceiver_time_get(void)
{
	return zb_osif_timer_get();
}

void osif_sleep_using_transc_timer(zb_time_t timeout_us)
{
	zb_time_t tstart = zb_osif_timer_get();
	zb_time_t tend = tstart + timeout_us;

	if (tend < tstart) {
		while (tend < zb_osif_timer_get()) {
			zb_osif_busy_loop_delay(10);
		}
	} else {
		while (zb_osif_timer_get() < tend) {
			zb_osif_busy_loop_delay(10);
		}
	}
}
