/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/ztest.h>
#include <app_event_manager.h>
#include "test_events.h"
#include <caf/events/sensor_event.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define MODULE main

#include <caf/events/module_state_event.h>

LOG_MODULE_REGISTER(MODULE);

#define PRE_CHANGE_SAMPLING_PERIOD 20
#define SAMPLING_PERIOD 40

static enum test_id cur_test_id;
static K_SEM_DEFINE(test_end_sem, 0, 1);
int64_t first_event_uptime;
uint8_t sensors_tested;
uint8_t sensors_tested_mask;


void test_init(void)
{
	zassert_ok(app_event_manager_init(), "Error when initializing");
	module_set_state(MODULE_STATE_READY);
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

static void test_event_manager(void)
{
	test_start(TEST_EVENT_MANAGER);
}

static void test_basic(void)
{
	test_start(TEST_BASIC);
}

static void test_change_period_pre(void)
{
	test_start(TEST_CHANGE_PERIOD_PRE);

	struct set_sensor_period_event *event = new_set_sensor_period_event();

	event->sampling_period = SAMPLING_PERIOD;
	event->descr = "Simulated sensor 1";
	APP_EVENT_SUBMIT(event);
}

static void test_change_period_post(void)
{
	test_start(TEST_CHANGE_PERIOD_POST);

	struct set_sensor_period_event *event = new_set_sensor_period_event();

	event->sampling_period = SAMPLING_PERIOD;
	event->descr = "Simulated sensor 2";
	APP_EVENT_SUBMIT(event);

	event = new_set_sensor_period_event();
	event->sampling_period = SAMPLING_PERIOD;
	event->descr = "Simulated sensor 3";
	APP_EVENT_SUBMIT(event);
}

static void test_multiple_sensors(void)
{
	test_start(TEST_MULTIPLE_SENSORS);
}

void test_main(void)
{
	ztest_test_suite(caf_sensor_aggregator_tests,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_event_manager),
			 ztest_unit_test(test_basic),
			 ztest_unit_test(test_change_period_pre),
			 ztest_unit_test(test_change_period_post),
			 ztest_unit_test(test_multiple_sensors)
			 );

	ztest_run_test_suite(caf_sensor_aggregator_tests);
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

	if (is_sensor_event(aeh)) {

		struct sensor_event *ev = cast_sensor_event(aeh);

		switch (cur_test_id) {
		case TEST_BASIC:
			cur_test_id = TEST_IDLE;
			k_sem_give(&test_end_sem);
			break;

		case TEST_CHANGE_PERIOD_PRE:
			if (strcmp(ev->descr, "Simulated sensor 1")) {
				zassert_unreachable("Expected sensor 1 event");
			}
			if (first_event_uptime == 0) {
				first_event_uptime = k_uptime_get();
				break;
			}
			int64_t sampled_period_pre = k_uptime_get() - first_event_uptime;

			zassert_between_inclusive(sampled_period_pre,
							PRE_CHANGE_SAMPLING_PERIOD - 1,
							PRE_CHANGE_SAMPLING_PERIOD + 1,
							"Wrong sample time");
			first_event_uptime = 0;
			cur_test_id = TEST_IDLE;
			k_sem_give(&test_end_sem);
			break;

		case TEST_CHANGE_PERIOD_POST:
			if (strcmp(ev->descr, "Simulated sensor 1")) {
				zassert_unreachable("Expected sensor 1 event");
			}
			if (first_event_uptime == 0) {
				first_event_uptime = k_uptime_get();
				break;
			}

			int64_t sampled_period_post = k_uptime_get() - first_event_uptime;

			zassert_between_inclusive(sampled_period_post,
							SAMPLING_PERIOD - 1,
							SAMPLING_PERIOD + 1,
							"Wrong sample time");
			first_event_uptime = 0;
			cur_test_id = TEST_IDLE;
			k_sem_give(&test_end_sem);
			break;

		case TEST_MULTIPLE_SENSORS:
			if (!strcmp(ev->descr, "Simulated sensor 1") &&
					((BIT(0) & sensors_tested_mask) == 0)) {
				sensors_tested++;
				sensors_tested_mask = sensors_tested_mask | BIT(0);
				if (sensors_tested == 3) {
					cur_test_id = TEST_IDLE;
					k_sem_give(&test_end_sem);
				}
				break;
			}
			if (!strcmp(ev->descr, "Simulated sensor 2") &&
					((BIT(1) & sensors_tested_mask) == 0)) {
				sensors_tested++;
				sensors_tested_mask = sensors_tested_mask | BIT(1);
				if (sensors_tested == 3) {
					cur_test_id = TEST_IDLE;
					k_sem_give(&test_end_sem);
				}
				break;
			}
			if (!strcmp(ev->descr, "Simulated sensor 3") &&
					((BIT(2) & sensors_tested_mask) == 0)) {
				sensors_tested++;
				sensors_tested_mask = sensors_tested_mask | BIT(2);
				if (sensors_tested == 3) {
					cur_test_id = TEST_IDLE;
					k_sem_give(&test_end_sem);
				}
				break;
			}

			zassert_unreachable("Expected sensor event from different sensor");

		default:
			break;
		}

		return false;
	}

	zassert_unreachable("Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(test_main, app_event_handler);
APP_EVENT_SUBSCRIBE(test_main, test_end_event);
APP_EVENT_SUBSCRIBE(test_main, sensor_event);
