/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <syscalls/rand32.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>

#include "test_config.h"
#include "common_utils.h"
#include "test_utils.h"
#include "data_events.h"

#define MODULE test_data

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

static K_SEM_DEFINE(waiting_response_sem, 0, 1);

struct data_content {
	int8_t val1;
	int16_t val2;
	int32_t val3;
	uint8_t val1u;
	uint16_t val2u;
	uint32_t val3u;
};

struct data_big_content {
	uint32_t block[DATA_BIG_EVENT_BLOCK_SIZE];
};

static struct data_content data_response;
static struct data_big_content data_big_response;

ZTEST(data_tests, test_data_response)
{
	struct data_event *event = new_data_event();
	struct data_content rand_test_data;

	rand_test_data.val1 = event->val1 = (int8_t)sys_rand32_get();
	rand_test_data.val2 = event->val2 = (int8_t)sys_rand32_get();
	rand_test_data.val3 = event->val3 = (int8_t)sys_rand32_get();
	rand_test_data.val1u = event->val1u = (uint8_t)sys_rand32_get();
	rand_test_data.val2u = event->val2u = (uint8_t)sys_rand32_get();
	rand_test_data.val3u = event->val3u = (uint8_t)sys_rand32_get();

	memset(&data_response, 0, sizeof(data_response));
	k_sem_reset(&waiting_response_sem);
	test_start(TEST_DATA_RESPONSE);

	APP_EVENT_SUBMIT(event);

	int err = k_sem_take(&waiting_response_sem, K_SECONDS(RESPONSE_TIMEOUT_S));
	zassert_ok(err, "No data response event received");

	zassert_equal(rand_test_data.val1, data_response.val1, "Unexpected value in response");
	zassert_equal(rand_test_data.val2, data_response.val2, "Unexpected value in response");
	zassert_equal(rand_test_data.val3, data_response.val3, "Unexpected value in response");
	zassert_equal(rand_test_data.val1u, data_response.val1u, "Unexpected value in response");
	zassert_equal(rand_test_data.val2u, data_response.val2u, "Unexpected value in response");
	zassert_equal(rand_test_data.val3u, data_response.val3u, "Unexpected value in response");

	test_end(TEST_DATA_RESPONSE);
}

ZTEST(data_tests, test_data_big_response)
{
	struct data_big_event *event = new_data_big_event();
	struct data_big_content rand_test_data;

	for (size_t i = 0; i < DATA_BIG_EVENT_BLOCK_SIZE; i++) {
		event->block[i] = sys_rand32_get();
	}

	memset(&data_big_response, 0, sizeof(data_big_response));
	zassert_equal(sizeof(event->block), sizeof(rand_test_data.block), "Wrong data size");
	memcpy(&rand_test_data.block, &event->block, sizeof(event->block));
	k_sem_reset(&waiting_response_sem);
	test_start(TEST_DATA_RESPONSE);

	APP_EVENT_SUBMIT(event);

	int err = k_sem_take(&waiting_response_sem, K_SECONDS(RESPONSE_TIMEOUT_S));
	zassert_ok(err, "No data response event received");

	zassert_mem_equal(&data_big_response.block,
			  &rand_test_data.block,
			  sizeof(data_big_response.block),
			  "Unexpected block in response");

	test_end(TEST_DATA_RESPONSE);
}

ZTEST(data_tests, test_data_burst)
{
	uint32_t us_spent;

	test_start(TEST_DATA_BURST);
	test_start_ack_wait();

	test_time_start();

	proxy_burst_data_events();
	test_end_wait(TEST_DATA_BURST);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BURST_SIZE + 1)) /
		us_spent;
	printk(" Time: %u us\n", us_spent);
	printk(" Test sending data burst speed %lu msg/sec\n", speed);
}

ZTEST(data_tests, test_data_burst_from_remote)
{
	uint32_t us_spent;

	test_start(TEST_DATA_BURST_FROM_REMOTE);
	test_start_ack_wait();
	test_time_start();
	test_end_wait(TEST_DATA_BURST_FROM_REMOTE);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test receiving data burst speed %lu msg/sec\n", speed);
}

ZTEST(data_tests, test_data_ping_pong_performance)
{
	uint32_t us_spent;
	int err;
	struct data_event *event = new_data_event();

	memset(&data_response, -1, sizeof(data_response));
	k_sem_reset(&waiting_response_sem);
	test_start(TEST_DATA_RESPONSE);
	test_start_ack_wait();
	test_time_start();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BURST_SIZE; ++cnt) {
		event->val1 = (int8_t)cnt;
		event->val2 = (int16_t)cnt;
		event->val3 = (int32_t)cnt;
		event->val1u = (uint8_t)cnt;
		event->val2u = (uint16_t)cnt;
		event->val3u = (uint32_t)cnt;

		proxy_direct_submit_event(&event->header);
		err = k_sem_take(&waiting_response_sem, K_SECONDS(RESPONSE_TIMEOUT_S));
		zassert_ok(err, "No data event received");

		zassert_equal(event->val1, data_response.val1, "Unexpected value in response");
		zassert_equal(event->val2, data_response.val2, "Unexpected value in response");
		zassert_equal(event->val3, data_response.val3, "Unexpected value in response");
		zassert_equal(event->val1u, data_response.val1u, "Unexpected value in response");
		zassert_equal(event->val2u, data_response.val2u, "Unexpected value in response");
		zassert_equal(event->val3u, data_response.val3u, "Unexpected value in response");
	}

	us_spent = test_time_spent_us();

	app_event_manager_free(event);

	test_end(TEST_DATA_RESPONSE);

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test data ping pong speed %lu msg/sec\n", speed);
}

ZTEST(data_tests, test_data_big_burst)
{
	uint32_t us_spent;

	test_start(TEST_DATA_BIG_BURST);
	test_start_ack_wait();

	test_time_start();

	proxy_burst_data_big_events();
	test_end_wait(TEST_DATA_BIG_BURST);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BIG_BURST_SIZE + 1)) /
		us_spent;
	printk(" Time: %u us\n", us_spent);
	printk(" Test sending data big burst speed %lu msg/sec\n", speed);
}

ZTEST(data_tests, test_data_big_burst_from_remote)
{
	uint32_t us_spent;

	test_start(TEST_DATA_BIG_BURST_FROM_REMOTE);
	test_start_ack_wait();
	test_time_start();
	test_end_wait(TEST_DATA_BIG_BURST_FROM_REMOTE);

	us_spent = test_time_spent_us();

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BIG_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test receiving data big burst speed %lu msg/sec\n", speed);
}

ZTEST(data_tests, test_data_big_ping_pong_performance)
{
	uint32_t us_spent;
	int err;
	struct data_big_event *event = new_data_big_event();

	memset(&data_response, -1, sizeof(data_response));
	k_sem_reset(&waiting_response_sem);
	test_start(TEST_DATA_RESPONSE);
	test_start_ack_wait();
	test_time_start();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BIG_BURST_SIZE; ++cnt) {
		for(size_t n = 0; n < DATA_BIG_EVENT_BLOCK_SIZE; ++n) {
			event->block[n] = cnt + n;
		}

		proxy_direct_submit_event(&event->header);
		err = k_sem_take(&waiting_response_sem, K_SECONDS(RESPONSE_TIMEOUT_S));
		zassert_ok(err, "No data event received");

		zassert_mem_equal(&data_big_response.block, &event->block, sizeof(event->block),
				  "Unexpected block in response");
	}

	us_spent = test_time_spent_us();

	app_event_manager_free(event);

	test_end(TEST_DATA_RESPONSE);

	unsigned long speed =
				     /* Burst size + test end event */
		((uint64_t)Z_HZ_us * (TEST_CONFIG_DATA_BIG_BURST_SIZE + 1)) /
		us_spent;

	printk(" Time: %u us\n", us_spent);
	printk(" Test data big ping pong speed %lu msg/sec\n", speed);
}

static bool data_event_handler(const struct app_event_header *aeh)
{
	if (is_data_response_event(aeh)) {
		struct data_response_event *event = cast_data_response_event(aeh);

		data_response.val1 = event->val1;
		data_response.val2 = event->val2;
		data_response.val3 = event->val3;
		data_response.val1u = event->val1u;
		data_response.val2u = event->val2u;
		data_response.val3u = event->val3u;

		k_sem_give(&waiting_response_sem);
		return false;
	} else if (is_data_big_response_event(aeh)) {
		struct data_big_response_event *event = cast_data_big_response_event(aeh);

		memcpy(&data_big_response.block, &event->block, sizeof(data_big_response.block));
		k_sem_give(&waiting_response_sem);
		return false;
	}

	zassert_true(false, "Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(MODULE, data_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, data_response_event);
APP_EVENT_SUBSCRIBE(MODULE, data_big_response_event);

static int test_data_events_register(void)
{
	const struct device *ipc_instance  = REMOTE_IPC_DEV;

	REMOTE_EVENT_SUBSCRIBE(ipc_instance, data_response_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, data_big_response_event);

	return 0;
}

SYS_INIT(test_data_events_register, APPLICATION, CONFIG_APP_PROXY_REGISTER_PRIO);

ZTEST_SUITE(data_tests, NULL, NULL, NULL, NULL, NULL);
