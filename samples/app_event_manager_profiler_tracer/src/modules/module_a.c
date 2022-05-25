/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "five_sec_event.h"
#include "one_sec_event.h"
#include "config_event.h"
#include "burst_event.h"

#define MODULE_A_THREAD_STACK_SIZE 800
#define MODULE_A_THREAD_PRIORITY 1
#define MODULE_A_THREAD_SLEEP_MS 1000
#define MODULE_A_WORK_SIMULATE_CALCULATION_US 100000
#define MODULE_A_BURST_EVENT_NUMBER 50
#define MODULE_A_BURST_INTERVAL_US 2000
#define MODULE module_a


static K_THREAD_STACK_DEFINE(module_a_thread_stack,
			     MODULE_A_THREAD_STACK_SIZE);
static struct k_thread module_a_thread;

atomic_t cnt;

void send_one_sec_event(void)
{
	struct one_sec_event *event = new_one_sec_event();

	event->five_sec_timer = atomic_get(&cnt);

	APP_EVENT_SUBMIT(event);
}

static void module_a_thread_fn(void)
{
	while (true) {
		atomic_inc(&cnt);
		send_one_sec_event();
		k_sleep(K_MSEC(MODULE_A_THREAD_SLEEP_MS));
	}
}

static void init(void)
{
	k_thread_create(&module_a_thread,
			module_a_thread_stack,
			MODULE_A_THREAD_STACK_SIZE,
			(k_thread_entry_t)module_a_thread_fn,
			NULL, NULL, NULL,
			MODULE_A_THREAD_PRIORITY,
			0, K_NO_WAIT);
}

static void send_event_burst(size_t number_of_send_events)
{
	for (size_t i = 0; i < number_of_send_events; i++) {
		struct burst_event *event = new_burst_event();

		APP_EVENT_SUBMIT(event);
		k_busy_wait(MODULE_A_BURST_INTERVAL_US);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_config_event(aeh)) {
		init();
		return false;
	}

	if (is_five_sec_event(aeh)) {
		atomic_clear(&cnt);
		send_event_burst(MODULE_A_BURST_EVENT_NUMBER);
		k_busy_wait(MODULE_A_WORK_SIMULATE_CALCULATION_US);
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, config_event);
APP_EVENT_SUBSCRIBE(MODULE, five_sec_event);
