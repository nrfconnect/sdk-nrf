/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define MODULE stats

#include "measurement_event.h"
#include "control_event.h"

#include <zephyr/logging/log.h>
#define STATS_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, STATS_LOG_LEVEL);

#define STATS_ARR_SIZE 10

static int32_t val_arr[STATS_ARR_SIZE];
static unsigned int control_cnt;

static void add_to_stats(int32_t value)
{
	static size_t ind;

	/* Add new value */
	val_arr[ind] = value;
	ind++;

	if (ind == ARRAY_SIZE(val_arr)) {
		ind = 0;
		long long sum = 0;

		for (size_t i = 0; i < ARRAY_SIZE(val_arr); i++) {
			sum += val_arr[i];
		}
		LOG_INF("Average value3: %d", (int)(sum/ARRAY_SIZE(val_arr)));
	}
}

static void count_control_events(void)
{
	++control_cnt;
	LOG_INF("Control event count: %u", control_cnt);
}

static bool event_handler(const struct app_event_header *eh)
{
	if (is_measurement_event(eh)) {
		struct measurement_event *me = cast_measurement_event(eh);

		add_to_stats(me->value3);
		return false;
	}
	if (is_control_event(eh)) {
		count_control_events();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, measurement_event);
APP_EVENT_SUBSCRIBE(MODULE, control_event);
