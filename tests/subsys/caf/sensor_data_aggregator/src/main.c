/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <app_event_manager.h>

#include "test_events.h"
#include <caf/events/sensor_event.h>
#include "test_config.h"
#include <zephyr/drivers/sensor.h>

static enum test_id cur_test_id;
static K_SEM_DEFINE(test_end_sem, 0, 1);


static void *test_init(void)
{
	zassert_ok(app_event_manager_init(), "Error when initializing");
	return NULL;
}

static void test_start(enum test_id test_id)
{
	cur_test_id = test_id;
	struct test_start_event *ts = new_test_start_event();

	zassert_not_null(ts, "Failed to allocate event");
	ts->test_id = test_id;
	APP_EVENT_SUBMIT(ts);

	int err = k_sem_take(&test_end_sem, K_SECONDS(30));

	zassert_ok(err, "Test execution hanged");
}

ZTEST(caf_sensor_aggregator_tests, test_event_manager)
{
	test_start(TEST_EVENT_MANAGER);
}

ZTEST(caf_sensor_aggregator_tests, test_basic)
{
	cur_test_id = TEST_BASIC;
	struct test_start_event *ts = new_test_start_event();

	zassert_not_null(ts, "Failed to allocate event");
	ts->test_id = cur_test_id;
	APP_EVENT_SUBMIT(ts);


	size_t i = SAMPLES_IN_AGG_BUF * BASIC_TEST_AGG_EVENTS;

	for (; i > 0; i--) {
		struct sensor_event *se = new_sensor_event(sizeof(struct sensor_value) *
			BASIC_TEST_SENSOR_SAMPLE_SIZE);

		zassert_not_null(se, "Failed to allocate event");
		se->descr = BASIC_TEST_AGG_DESCR;
		se->dyndata.size = sizeof(struct sensor_value) * BASIC_TEST_SENSOR_SAMPLE_SIZE;
		APP_EVENT_SUBMIT(se);
		k_yield();
	}

	int err = k_sem_take(&test_end_sem, K_SECONDS(30));

	zassert_ok(err, "Test execution hanged");
}

ZTEST(caf_sensor_aggregator_tests, test_order)
{
	cur_test_id = TEST_ORDER;
	struct test_start_event *ts = new_test_start_event();

	zassert_not_null(ts, "Failed to allocate event");
	ts->test_id = cur_test_id;
	APP_EVENT_SUBMIT(ts);

	size_t i = SAMPLES_IN_AGG_BUF * ORDER_TEST_AGG_EVENTS;

	for (; i > 0; i--) {
		struct sensor_event *se = new_sensor_event(sizeof(struct sensor_value) *
				ORDER_TEST_SENSOR_SAMPLE_SIZE);

		zassert_not_null(se, "Failed to allocate event");
		se->descr = ORDER_TEST_AGG_DESCR;
		se->dyndata.size = sizeof(struct sensor_value) * ORDER_TEST_SENSOR_SAMPLE_SIZE;
		se->dyndata.data[0] = i;
		APP_EVENT_SUBMIT(se);
	}

	int err = k_sem_take(&test_end_sem, K_SECONDS(30));

	zassert_ok(err, "Test execution hanged");
}

ZTEST(caf_sensor_aggregator_tests, test_status)
{
	test_start(TEST_STATUS);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_end_event(aeh)) {
		struct test_end_event *ev = cast_test_end_event(aeh);

		zassert_equal(cur_test_id, ev->test_id,
			"End test ID does not equal start test ID");
		cur_test_id = TEST_IDLE;
		k_sem_give(&test_end_sem);

		return false;
	}

	zassert_unreachable("Wrong event type received");
	return false;
}

ZTEST_SUITE(caf_sensor_aggregator_tests, NULL, test_init, NULL, NULL, NULL);

APP_EVENT_LISTENER(test_main, app_event_handler);
APP_EVENT_SUBSCRIBE(test_main, test_end_event);
