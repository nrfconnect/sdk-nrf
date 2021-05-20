/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <data_event.h>

#define MODULE test_oom
#define TEST_EVENTS_CNT 150

static struct data_event *event_tab[TEST_EVENTS_CNT];
static bool oom_error;

/* Custom reboot handler to check if sys_reboot is called when OOM. */
void sys_reboot(int type)
{
	oom_error = true;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_test_start_event(eh)) {
		struct test_start_event *st = cast_test_start_event(eh);

		switch (st->test_id) {
		case TEST_OOM_RESET:
		{
			/* Sending large number of events to cause out of memory error. */
			int i;

			for (i = 0; i < ARRAY_SIZE(event_tab); i++) {
				event_tab[i] = new_data_event();
				if (event_tab[i] == NULL) {
					break;
				}
			}

			zassert_true(i < ARRAY_SIZE(event_tab),
				     "No OOM detected, increase TEST_EVENTS_CNT");
			zassert_true(oom_error, "OOM error not detected");

			/* Freeing memory to enable further testing. */
			while (i >= 0) {
				k_free(event_tab[i]);
				i--;
			}

			struct test_end_event *et = new_test_end_event();

			zassert_not_null(et, "Failed to allocate event");
			et->test_id = st->test_id;
			EVENT_SUBMIT(et);
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

	zassert_true(false, "Event unhandled");

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, test_start_event);
