/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <string.h>
#include "test_config.h"
#include <test_events.h>
#include <caf/events/sensor_data_aggregator_event.h>
#include <zephyr/drivers/sensor.h>



#define MODULE test_data_receiver

static enum test_id cur_test_id;
int msg_num;
int order_event_indicator = SAMPLES_IN_AGG_BUF * ORDER_TEST_AGG_EVENTS;

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *event = cast_test_start_event(aeh);

		cur_test_id = event->test_id;

		return false;
	}

	if (is_sensor_data_aggregator_event(aeh)) {
		const struct sensor_data_aggregator_event *event =
			cast_sensor_data_aggregator_event(aeh);

		struct sensor_data_aggregator_release_buffer_event *release_evt =
		new_sensor_data_aggregator_release_buffer_event();

		release_evt->samples = event->samples;
		release_evt->sensor_descr = event->sensor_descr;
		APP_EVENT_SUBMIT(release_evt);

		if (strcmp(event->sensor_descr, BASIC_TEST_AGG_DESCR) == 0) {

			msg_num++;
			if (msg_num == BASIC_TEST_AGG_EVENTS) {
				struct test_end_event *te = new_test_end_event();

				zassert_not_null(te, "Failed to allocate event");
				te->test_id = cur_test_id;
				APP_EVENT_SUBMIT(te);
			}
		} else if (strcmp(event->sensor_descr, ORDER_TEST_AGG_DESCR) == 0) {

			for (int j = 0; j < SAMPLES_IN_AGG_BUF; j++) {
				uint8_t *event_data = (uint8_t *)&event->samples[j *
						ORDER_TEST_SENSOR_SAMPLE_SIZE];

				zassert_equal(*event_data, order_event_indicator,
						"Incorrent event order");
				order_event_indicator--;
			}


			if (order_event_indicator == 0) {
				struct test_end_event *te = new_test_end_event();

				zassert_not_null(te, "Failed to allocate event");
				te->test_id = cur_test_id;
				APP_EVENT_SUBMIT(te);
			}

		} else if (strcmp(event->sensor_descr, STATUS_TEST_AGG_DESCR) == 0) {

			for (int k = 0; k < STATUS_TEST_SENSOR_EVENTS; k++) {
				uint8_t *event_data = (uint8_t *)&event->samples[k *
						STATUS_TEST_SENSOR_SAMPLE_SIZE];

				zassert_equal(*event_data, k, "Incorrent event order");
			}

			struct test_end_event *te = new_test_end_event();

			zassert_not_null(te, "Failed to allocate event");
			te->test_id = cur_test_id;
			APP_EVENT_SUBMIT(te);
		}

		return false;
	}


	zassert_unreachable("Event unhandled");

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_data_aggregator_event);
