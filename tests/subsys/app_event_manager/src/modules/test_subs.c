/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <order_event.h>

#include "test_config.h"


static enum test_id cur_test_id;

static int first_cnt;
static int early_cnt;
static int normal_cnt;

static bool event_handler_first(const struct application_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *event = cast_test_start_event(aeh);

		cur_test_id = event->test_id;

		return false;
	}

	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			first_cnt++;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");
	return false;
}

/* Create one first listener. */
APPLICATION_EVENT_LISTENER(first, event_handler_first);
APPLICATION_EVENT_SUBSCRIBE_FIRST(first, order_event);
APPLICATION_EVENT_SUBSCRIBE_EARLY(first, test_start_event);


static bool event_handler_early(const struct application_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - early before first");
			early_cnt++;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");
	return false;
}

/* Create 3 early listeners. */
APPLICATION_EVENT_LISTENER(early1, event_handler_early);
APPLICATION_EVENT_SUBSCRIBE_EARLY(early1, order_event);

APPLICATION_EVENT_LISTENER(early2, event_handler_early);
APPLICATION_EVENT_SUBSCRIBE_EARLY(early2, order_event);

APPLICATION_EVENT_LISTENER(early3, event_handler_early);
APPLICATION_EVENT_SUBSCRIBE_EARLY(early3, order_event);


static bool event_handler_normal(const struct application_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - normal before first");
			zassert_equal(early_cnt, 3, "Incorrect subscriber order"
				      " - normal before early");
			normal_cnt++;
		}

		return false;
	}

	zassert_true(false, "Wrong event type received");

	return false;
}

/* Create 3 normal listeners. */
APPLICATION_EVENT_LISTENER(listener1, event_handler_normal);
APPLICATION_EVENT_SUBSCRIBE(listener1, order_event);

APPLICATION_EVENT_LISTENER(listener2, event_handler_normal);
APPLICATION_EVENT_SUBSCRIBE(listener2, order_event);

APPLICATION_EVENT_LISTENER(listener3, event_handler_normal);
APPLICATION_EVENT_SUBSCRIBE(listener3, order_event);

static bool event_handler_final(const struct application_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - late before first");
			zassert_equal(early_cnt, 3, "Incorrect subscriber order"
				      " - late before early");
			zassert_equal(normal_cnt, 3,
				      "Incorrect subscriber order"
				      " - late before normal");

			struct test_end_event *te = new_test_end_event();

			zassert_not_null(te, "Failed to allocate event");
			te->test_id = cur_test_id;
			APPLICATION_EVENT_SUBMIT(te);
		}

		return false;
	}

	zassert_true(false, "Wrong event type received");

	return false;
}

/* Create one final listener. */
APPLICATION_EVENT_LISTENER(final, event_handler_final);
APPLICATION_EVENT_SUBSCRIBE_FINAL(final, order_event);
