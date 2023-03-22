/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <zephyr/sys/atomic.h>

#include <zboss_api.h>
#include <zb_types.h>
#include <zb_osif_platform.h>

#define ALARM_CHANNEL_ID  0

typedef struct {
	const struct device *device;
	struct counter_alarm_cfg alarm_cfg;
	uint32_t counter_period_us;
	uint32_t counter_acc_us;
	uint8_t alarm_ch_id;
	volatile zb_bool_t is_init;
	volatile atomic_t is_running;
} zb_timer_t;

static zb_timer_t zb_timer = {
	.device = DEVICE_DT_GET(DT_CHOSEN(ncs_zigbee_timer)),
	.is_init = ZB_FALSE,
	.is_running = ATOMIC_INIT(0)
};

/* Forward declaration, dependency to ZBOSS */
void zb_osif_zboss_timer_tick(void);

/* Timer interrupt handler. */
static void zb_timer_alarm_handler(const struct device *counter_dev,
				   uint8_t chan_id,
				   uint32_t ticks, void *user_data)
{
	switch (chan_id) {
	case ALARM_CHANNEL_ID:
		if (atomic_set((atomic_t *)&zb_timer.is_running, 0) == 1) {
			/* The atomic flag is_running was 1 and now set to 0. */
			zb_timer.counter_acc_us += zb_timer.counter_period_us;

			/* ZBOSS reschedules the timer inside the
			 * zb_osif_zboss_timer_tick(), so it is required to
			 * reshedule it manually if the function will not be
			 * called at the end of this function.
			 */
			if (zb_timer.counter_acc_us < ZB_BEACON_INTERVAL_USEC) {
				zb_osif_timer_start();
			}
		}
		break;

	default:
		/*Do nothing*/
		break;
	}

	if (zb_timer.counter_acc_us >= ZB_BEACON_INTERVAL_USEC) {
		zb_timer.counter_acc_us -= ZB_BEACON_INTERVAL_USEC;
		zb_osif_zboss_timer_tick();
	}
}

static void zb_osif_timer_set_default_config(zb_timer_t *timer)
{
	timer->alarm_ch_id = ALARM_CHANNEL_ID;
	timer->alarm_cfg.flags = 0;
	timer->alarm_cfg.ticks = counter_us_to_ticks(timer->device,
						   ZB_BEACON_INTERVAL_USEC / 2);
	timer->counter_period_us = counter_ticks_to_us(timer->device,
						       timer->alarm_cfg.ticks);
	timer->counter_acc_us = 0;
	timer->alarm_cfg.callback = zb_timer_alarm_handler;
}

static void zb_timer_init(void)
{
	__ASSERT(device_is_ready(zb_timer.device), "Timer device not ready");

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
 * Get current time, us.
 */
zb_uint64_t osif_transceiver_time_get_long(void)
{
	zb_uint64_t time_sys;
	zb_uint64_t time_cur;

	if (zb_osif_timer_is_on() == ZB_TRUE) {
		uint32_t ticks = 0;
		(void)counter_get_value(zb_timer.device, &ticks);
		time_cur = (zb_uint64_t)counter_ticks_to_us(zb_timer.device,
							    ticks);
	} else {
		time_cur = 0;
	}
	time_sys = ZB_TIME_BEACON_INTERVAL_TO_USEC(ZB_TIMER_GET());

	return time_sys + time_cur;
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
