/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_events.h"
#include "data_event.h"
#include "test_event_allocator.h"

#define MODULE test_oom
#define TEST_EVENTS_CNT 150

static struct data_event *event_tab[TEST_EVENTS_CNT];


static void oom_test_run(void)
{
	size_t i;

	/* Allocate large number of events to cause out of memory error. */
	test_event_allocator_oom_expect(true);
	for (i = 0; i < ARRAY_SIZE(event_tab); i++) {
		event_tab[i] = new_data_event();
		if (!event_tab[i]) {
			break;
		}
	}

	zassert_true(i < ARRAY_SIZE(event_tab), "Expected OOM error");
	test_event_allocator_oom_expect(false);

	/* Freeing memory to enable further testing. */
	for (i = 0; (i < ARRAY_SIZE(event_tab)) && event_tab[i]; i++) {
		app_event_manager_free(event_tab[i]);
		event_tab[i] = NULL;
	}
}

static bool event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *st = cast_test_start_event(aeh);

		switch (st->test_id) {
		case TEST_OOM:
			oom_test_run();

			struct test_end_event *et = new_test_end_event();

			et->test_id = st->test_id;
			APP_EVENT_SUBMIT(et);
			break;

		default:
			/* Ignore other test cases. */
			zassert_true(st->test_id < TEST_CNT, "test_id out of range");
			break;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");
	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
