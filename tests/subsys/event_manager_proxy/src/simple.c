/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>

#include "test_config.h"
#include "common_utils.h"
#include "test_utils.h"
#include "simple_events.h"

#define MODULE test_simple

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

static K_SEM_DEFINE(waiting_pong_sem, 0, 1);


ZTEST(simple_tests, test_simple_ping_pong)
{
	struct simple_ping_event *ping = new_simple_ping_event();

	k_sem_reset(&waiting_pong_sem);
	APP_EVENT_SUBMIT(ping);

	int err = k_sem_take(&waiting_pong_sem, K_SECONDS(1));
	zassert_ok(err, "No pong event received");
}

ZTEST(simple_tests, test_simple_burst)
{
	uint32_t us_spent;

	test_start(TEST_SIMPLE_BURST);
	test_start_ack_wait();

	test_time_start();

	proxy_burst_simple_events();
	test_end_wait(TEST_SIMPLE_BURST);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_SIMPLE_BURST_SIZE + 1)) /
		us_spent;
	printk(" Time: %u us\n", us_spent);
	printk(" Test sending simple burst speed %lu msg/sec\n", speed);
}

ZTEST(simple_tests, test_simple_burst_from_remote)
{
	uint32_t us_spent;

	test_time_start();

	test_start(TEST_SIMPLE_BURST_FROM_REMOTE);
	test_start_ack_wait();
	test_time_start();
	test_end_wait(TEST_SIMPLE_BURST_FROM_REMOTE);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_SIMPLE_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test receiving simple burst speed %lu msg/sec\n", speed);
}

ZTEST(simple_tests, test_simple_ping_pong_performance)
{
	uint32_t us_spent;
	int err;
	struct simple_ping_event *event = new_simple_ping_event();

	k_sem_reset(&waiting_pong_sem);

	test_time_start();

	for (size_t cnt = 0; cnt < TEST_CONFIG_SIMPLE_BURST_SIZE; ++cnt) {
		proxy_direct_submit_event(&event->header);
		err = k_sem_take(&waiting_pong_sem, K_SECONDS(1));
		zassert_ok(err, "No pong event received");
	}

	us_spent = test_time_spent_us();

	app_event_manager_free(event);

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_SIMPLE_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test ping pong speed %lu msg/sec\n", speed);
}

static bool simple_event_handler(const struct app_event_header *aeh)
{
	if (is_simple_pong_event(aeh)) {
		k_sem_give(&waiting_pong_sem);
		return false;
	}

	zassert_true(false, "Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(MODULE, simple_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, simple_pong_event);

int test_simple_events_register(void)
{
	const struct device *ipc_instance  = REMOTE_IPC_DEV;

	REMOTE_EVENT_SUBSCRIBE(ipc_instance, simple_pong_event);

	return 0;
}

SYS_INIT(test_simple_events_register, APPLICATION, CONFIG_APP_PROXY_REGISTER_PRIO);

ZTEST_SUITE(simple_tests, NULL, NULL, NULL, NULL, NULL);
