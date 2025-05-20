/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <app_event_manager.h>

#include "sized_events.h"
#include "test_events.h"

static enum test_id cur_test_id;
static K_SEM_DEFINE(test_end_sem, 0, 1);
static bool expect_assert;


/* Provide custom assert post action handler to handle the assertion on OOM
 * error in Application Event Manager.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_ASSERT_NO_FILE_INFO));
void assert_post_action(const char *file, unsigned int line)
{
	printk("assert_post_action - file: %s (line: %u)\n", file, line);
	if (expect_assert) {
		expect_assert = false;
		printk("Assertion was expected.\n");
	} else {
		zassert(false, "", "Unexpected assertion reached.");
	}
}

static void *test_init(void)
{
	zassert_false(app_event_manager_init(), "Error when initializing");
	return NULL;
}

static void test_start(enum test_id test_id)
{
	cur_test_id = test_id;
	struct test_start_event *ts = new_test_start_event();

	ts->test_id = test_id;
	APP_EVENT_SUBMIT(ts);

	int err = k_sem_take(&test_end_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Test execution hanged");
}

ZTEST(suite0, test_basic)
{
	test_start(TEST_BASIC);
}

ZTEST(suite0, test_data)
{
	test_start(TEST_DATA);
}

ZTEST(suite0, test_event_order)
{
	test_start(TEST_EVENT_ORDER);
}

ZTEST(suite0, test_subs_order)
{
	test_start(TEST_SUBSCRIBER_ORDER);
}

ZTEST(suite0, test_oom)
{
	test_start(TEST_OOM);
}

ZTEST(suite0, test_multicontext)
{
	test_start(TEST_MULTICONTEXT);
}

ZTEST(suite0, test_event_size_static)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_size1_event *ev_s1;
	struct test_size2_event *ev_s2;
	struct test_size3_event *ev_s3;
	struct test_size_big_event *ev_sb;

	ev_s1 = new_test_size1_event();
	zassert_equal(sizeof(*ev_s1), app_event_manager_event_size(&ev_s1->header),
		"Event size1 unexpected size");
	app_event_manager_free(ev_s1);

	ev_s2 = new_test_size2_event();
	zassert_equal(sizeof(*ev_s2), app_event_manager_event_size(&ev_s2->header),
		"Event size2 unexpected size");
	app_event_manager_free(ev_s2);

	ev_s3 = new_test_size3_event();
	zassert_equal(sizeof(*ev_s3), app_event_manager_event_size(&ev_s3->header),
		"Event size3 unexpected size");
	app_event_manager_free(ev_s3);

	ev_sb = new_test_size_big_event();
	zassert_equal(sizeof(*ev_sb), app_event_manager_event_size(&ev_sb->header),
		"Event size_big unexpected size");
	app_event_manager_free(ev_sb);
}

ZTEST(suite0, test_event_size_dynamic)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_dynamic_event *ev;

	ev = new_test_dynamic_event(0);
	zassert_equal(sizeof(*ev) + 0, app_event_manager_event_size(&ev->header),
		"Event dynamic with 0 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_event(10);
	zassert_equal(sizeof(*ev) + 10, app_event_manager_event_size(&ev->header),
		"Event dynamic with 10 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_event(100);
	zassert_equal(sizeof(*ev) + 100, app_event_manager_event_size(&ev->header),
		"Event dynamic with 100 elements unexpected size");
	app_event_manager_free(ev);
}

ZTEST(suite0, test_event_size_dynamic_with_data)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_dynamic_with_data_event *ev;

	ev = new_test_dynamic_with_data_event(0);
	zassert_equal(sizeof(*ev) + 0, app_event_manager_event_size(&ev->header),
		"Event dynamic with 0 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_with_data_event(10);
	zassert_equal(sizeof(*ev) + 10, app_event_manager_event_size(&ev->header),
		"Event dynamic with 10 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_with_data_event(100);
	zassert_equal(sizeof(*ev) + 100, app_event_manager_event_size(&ev->header),
		"Event dynamic with 100 elements unexpected size");
	app_event_manager_free(ev);
}

ZTEST(suite0, test_event_size_disabled)
{
	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_size1_event *ev_s1;

	ev_s1 = new_test_size1_event();
	expect_assert = true;
	zassert_equal(0, app_event_manager_event_size(&ev_s1->header),
		"Event size1 unexpected size");
	zassert_false(expect_assert,
		"Assertion during app_event_manager_event_size function execution was expected");
	app_event_manager_free(ev_s1);
}

ZTEST(suite0, test_name_style_events_sorting)
{
	test_start(TEST_NAME_STYLE_SORTING);
}

ZTEST_SUITE(suite0, NULL, test_init, NULL, NULL, NULL);

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

	zassert_true(false, "Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(test_main, app_event_handler);
APP_EVENT_SUBSCRIBE_FINAL(test_main, test_end_event);
