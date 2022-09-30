/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <caf/events/sensor_event.h>
#include <test_events.h>
#include "test_config.h"
#include <zephyr/drivers/sensor.h>

#define MODULE test_basic


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *st = cast_test_start_event(aeh);

		switch (st->test_id) {
		case TEST_EVENT_MANAGER:
		{
			struct test_end_event *et = new_test_end_event();

			zassert_not_null(et, "Failed to allocate event");
			et->test_id = st->test_id;
			APP_EVENT_SUBMIT(et);
			break;
		}
		case TEST_BASIC:
		{
			break;
		}

		case TEST_ORDER:
		{
			break;
		}

		case TEST_STATUS:
		{
			for (size_t i = 0; i < STATUS_TEST_SENSOR_EVENTS; i++) {
				struct sensor_event *se =
					new_sensor_event(sizeof(struct sensor_value) *
						STATUS_TEST_SENSOR_SAMPLE_SIZE);

				zassert_not_null(se, "Failed to allocate event");
				se->descr = STATUS_TEST_AGG_DESCR;
				se->dyndata.size = sizeof(struct sensor_value) *
					STATUS_TEST_SENSOR_SAMPLE_SIZE;
				se->dyndata.data[0] = i;
				APP_EVENT_SUBMIT(se);
			}

			struct sensor_state_event *sse = new_sensor_state_event();

			zassert_not_null(sse, "Failed to allocate event");
			sse->descr = STATUS_TEST_AGG_DESCR;
			sse->state = SENSOR_STATE_ACTIVE;
			APP_EVENT_SUBMIT(sse);

			break;
		}

		default:
			/* Ignore other test cases, check if proper test_id. */
			zassert_true(st->test_id < TEST_CNT,
				     "test_id out of range");
			break;
		}

		return false;
	}

	zassert_unreachable("Event unhandled");

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
