/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <app_event_manager.h>

#include "test_config.h"
#include "test_utils.h"

#define MODULE test_utils

static uint32_t test_start_time;
static enum test_id cur_test_id;
static int remote_test_res;
static K_SEM_DEFINE(test_end_sem, 0, 1);
static K_SEM_DEFINE(test_ack_sem, 0, 1);

void test_time_start(void)
{
	test_start_time = k_cycle_get_32();
}

uint32_t test_time_spent_us(void)
{
	uint32_t stop_time;
	uint32_t cycles_spent;

	stop_time = k_cycle_get_32();
	cycles_spent = stop_time - test_start_time;
	return k_cyc_to_us_near32(cycles_spent);
}

enum test_id test_current(void)
{
	return cur_test_id;
}

void test_start(enum test_id test_id)
{
	cur_test_id = test_id;
	remote_test_res = -EALREADY;

	k_sem_reset(&test_end_sem);
	k_sem_reset(&test_ack_sem);
	submit_test_start_event(test_id);
}

void test_start_ack_wait(void)
{
	int err = k_sem_take(&test_ack_sem, K_SECONDS(TEST_TIMEOUT_BASE_S));

	zassert_ok(err, "No test start acknowledge from remote");
}

void test_end(enum test_id test_id)
{
	zassert_equal(cur_test_id, test_id, "Current test ID does not match the id of test to end");
	submit_test_end_event(test_id);

	int err = k_sem_take(&test_end_sem, K_SECONDS(TEST_TIMEOUT_BASE_S));

	zassert_ok(err, "Test execution hanged");
}

void test_end_wait(enum test_id test_id)
{
	int err = k_sem_take(&test_end_sem, K_SECONDS(30 * TEST_TIMEOUT_BASE_S));

	zassert_ok(err, "Test execution hanged");
}

int test_remote_result(void)
{
	return remote_test_res;
}

static bool utils_event_handler(const struct app_event_header *aeh)
{
	if (is_test_end_event(aeh)) {
		struct test_end_event *ev = cast_test_end_event(aeh);

		zassert_equal(cur_test_id, ev->test_id,
			      "End test ID does not equal start test ID");
		cur_test_id = TEST_IDLE;
		k_sem_give(&test_end_sem);

		return false;
	} else if (is_test_end_remote_event(aeh)) {
		struct test_end_remote_event *ev = cast_test_end_remote_event(aeh);

		zassert_equal(cur_test_id, ev->test_id,
			      "End test ID does not equal start test ID");
		cur_test_id = TEST_IDLE;
		remote_test_res = ev->res;
		k_sem_give(&test_end_sem);

		return false;
	} else if (is_test_start_ack_event(aeh)) {
		struct test_start_ack_event *ev = cast_test_start_ack_event(aeh);

		zassert_equal(cur_test_id, ev->test_id,
			      "ACK test ID does not equal the current test ID");
		k_sem_give(&test_ack_sem);

		return false;
	}

	zassert_true(false, "Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(MODULE, utils_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_ack_event);
APP_EVENT_SUBSCRIBE(MODULE, test_end_event);
APP_EVENT_SUBSCRIBE(MODULE, test_end_remote_event);
