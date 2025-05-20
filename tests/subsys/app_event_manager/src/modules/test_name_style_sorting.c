/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_events.h"
#include "name_style_events.h"

#define MODULE test_name_style_sorting

enum test_name_style_sorting_step {
	TESTING_EVENT_NAME_STYLE,
	TESTING_EVENT_NAME_STYLE_BREAK,
	TESTING_EVENT_NAME_STYLE_END,
};

static enum test_name_style_sorting_step test_current_stage;
static int test_name_style_cnt;

/**
 * @brief Assert that @a e event subscribers does not start inside @a er subscribers range
 *
 *
 */
#define app_zassert_subscriber_not_in(e, er, ...)                         \
	zassert((_EVENT_ID(e)->subs_start < _EVENT_ID(er)->subs_start)  \
		||                                                      \
		(_EVENT_ID(e)->subs_start >= _EVENT_ID(er)->subs_stop), \
		#e " subscriber overlaps " #er " subscriber", ##__VA_ARGS__)

static void run_test_stage(enum test_name_style_sorting_step stage)
{
	test_current_stage = stage;

	switch (stage) {
	case TESTING_EVENT_NAME_STYLE:
	{
		struct event_name_style *et = new_event_name_style();

		test_name_style_cnt = 0;
		APP_EVENT_SUBMIT(et);
		break;
	}
	case TESTING_EVENT_NAME_STYLE_BREAK:
	{
		struct event_name_style_break *et = new_event_name_style_break();

		APP_EVENT_SUBMIT(et);
		break;
	}
	case TESTING_EVENT_NAME_STYLE_END:
	{
		struct test_end_event *et = new_test_end_event();

		et->test_id = TEST_NAME_STYLE_SORTING;
		APP_EVENT_SUBMIT(et);
		break;
	}
	default:
		zassert_true(false, "Requested test stage is out of test scope");
		break;
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		const struct test_start_event *st = cast_test_start_event(aeh);

		switch (st->test_id) {
		case TEST_NAME_STYLE_SORTING:
		{
			/* First, manually check if one event subscribers
			 * is not in the scope of the other
			 */
			app_zassert_subscriber_not_in(event_name_style, event_name_style_break);
			app_zassert_subscriber_not_in(event_name_style_break, event_name_style);

			/* Second - test if we are not calling other event subscriber
			 * while processing one of the events.
			 * If the tests above succeed, the test below should never fail,
			 * but lets be sure even if some more unexpected error appears.
			 */
			run_test_stage(0);
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

	if (is_test_end_event(aeh)) {
		const struct test_end_event *ev = cast_test_end_event(aeh);

		if (ev->test_id == TEST_NAME_STYLE_SORTING) {
			/* When the test ends we expect the number of counted events to be 2,
			 * more would mean that some unexpected event processing jumped in between
			 * the proper event processing and test end.
			 */
			zassert_equal(test_name_style_cnt, 2,
				      "Unexpected number of events counted at the test end");
		}

		return false;
	}

	if (is_event_name_style(aeh)) {
		zassert_equal(test_current_stage, TESTING_EVENT_NAME_STYLE,
			      "Unexpected test stage when event_name_style is processed");
		test_name_style_cnt += 1;
		zassert_equal(test_name_style_cnt, 1,
			      "Unexpected other events when event_name_style is processed");

		run_test_stage(test_current_stage + 1);
		return false;
	}

	if (is_event_name_style_break(aeh)) {
		zassert_equal(test_current_stage, TESTING_EVENT_NAME_STYLE_BREAK,
			      "Unexpected test stage when event_name_style_break is processed");
		test_name_style_cnt += 1;
		zassert_equal(test_name_style_cnt, 2,
			      "Unexpected other events when event_name_style_break is processed");

		run_test_stage(test_current_stage + 1);
		return false;
	}

	zassert_true(false, "Event unhandled");

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
APP_EVENT_SUBSCRIBE(MODULE, test_end_event);
APP_EVENT_SUBSCRIBE(MODULE, event_name_style);
APP_EVENT_SUBSCRIBE(MODULE, event_name_style_break);
