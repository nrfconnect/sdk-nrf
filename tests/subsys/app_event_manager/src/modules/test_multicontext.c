/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_events.h"
#include "multicontext_event.h"

#include "test_multicontext_config.h"

#define MODULE test_multictx
#define THREAD_STACK_SIZE 400


static void send_event(int a, bool sleep)
{
	struct multicontext_event *ev = new_multicontext_event();

	/* For every event both values should be the same -
	 * used to check if Application Event Manager sends proper values.
	 */
	ev->val1 = a;
	if (sleep) {
		k_sleep(K_MSEC(5));
	}
	ev->val2 = a;

	APP_EVENT_SUBMIT(ev);
}

static void timer_handler(struct k_timer *timer_id)
{
	send_event(SOURCE_ISR, false);
}

static K_TIMER_DEFINE(test_timer, timer_handler, NULL);
static K_THREAD_STACK_DEFINE(thread_stack1, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(thread_stack2, THREAD_STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;

static void thread1_fn(void)
{
	send_event(SOURCE_T1, true);
}

static void thread2_fn(void)
{
	k_timer_start(&test_timer, K_MSEC(2), K_NO_WAIT);
	send_event(SOURCE_T2, true);
}

static void start_test(void)
{
	k_thread_create(&thread1, thread_stack1,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)thread1_fn,
			NULL, NULL, NULL,
			THREAD1_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&thread2, thread_stack2,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)thread2_fn,
			NULL, NULL, NULL,
			THREAD2_PRIORITY, 0, K_NO_WAIT);
}


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *st = cast_test_start_event(aeh);

		switch (st->test_id) {
		case TEST_MULTICONTEXT:
		{
			start_test();

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

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
