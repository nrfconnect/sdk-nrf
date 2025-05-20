/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <app_event_manager.h>
#include <event_manager_proxy.h>
#include <zephyr/logging/log.h>

#include "test_config.h"
#include "common_utils.h"
#include "data_events.h"
#include "simple_events.h"
#include "test_events.h"


#define MODULE main_remote

LOG_MODULE_REGISTER(MODULE);


static enum test_id cur_test_id;
static unsigned int cur_test_pos;


int main(void)
{
	printk("Event Manager Proxy test remote_core started\n");

	int ret;
	const struct device *ipc_instance  = REMOTE_IPC_DEV;

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event manager initialized");

	ret = event_manager_proxy_add_remote(ipc_instance);
	if (ret) {
		LOG_ERR("Cannot add remote: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event proxy remote added");

	REMOTE_EVENT_SUBSCRIBE(ipc_instance, data_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, simple_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, simple_ping_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, data_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, data_big_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, test_start_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, test_end_event);

	ret = event_manager_proxy_start();
	if (ret) {
		LOG_ERR("Cannot start event manager proxy: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}

	/* We do not have to wait for remotes here as we do not send any event that
	 * would be fatal when missed.
	 */
	LOG_INF("Initialization finished successfully");

	return 0;
}

static void proxy_direct_submit_pong_event(void)
{
	struct simple_pong_event *event = new_simple_pong_event();

	proxy_direct_submit_event(&event->header);
	app_event_manager_free(event);
}

/*
 * We need to push this event direct to the proxy,
 * as some burst tests are pushing messages direct to the proxy also.
 * Using event manager for that may lead to the ack event to come after
 * the burst messages from the test.
 */
static void proxy_direct_submit_start_ack_event(void)
{
	struct test_start_ack_event *event = new_test_start_ack_event();

	event->test_id = cur_test_id;
	proxy_direct_submit_event(&event->header);
	app_event_manager_free(event);
}

static bool event_handler(const struct app_event_header *eh)
{
	int ret;

	LOG_DBG("Event received: %s", eh->type_id->name);

	if (is_test_start_event(eh)) {
		struct test_start_event *event = cast_test_start_event(eh);

		LOG_INF("Test start: %d", event->test_id);
		cur_test_id = event->test_id;
		cur_test_pos = 0;
		switch (event->test_id) {
		case TEST_SIMPLE_BURST:
			LOG_INF("Starting counting burst");
			proxy_direct_submit_start_ack_event();
			break;
		case TEST_SIMPLE_BURST_FROM_REMOTE:
			LOG_INF("Starting remote burst");
			proxy_direct_submit_start_ack_event();
			ret = proxy_burst_simple_pong_events();
			submit_test_end_remote_event(TEST_SIMPLE_BURST_FROM_REMOTE, ret);
			break;
		case TEST_DATA_RESPONSE:
			LOG_INF("Enabling data response");
			proxy_direct_submit_start_ack_event();
			break;
		case TEST_DATA_BURST:
			LOG_INF("Starting counting data burst");
			proxy_direct_submit_start_ack_event();
			break;
		case TEST_DATA_BURST_FROM_REMOTE:
			LOG_INF("Starting remote data burst");
			proxy_direct_submit_start_ack_event();
			ret = proxy_burst_data_response_events();
			submit_test_end_remote_event(TEST_DATA_BURST_FROM_REMOTE, ret);
			break;
		case TEST_DATA_BIG_BURST:
			LOG_INF("Starting counting data big burst");
			proxy_direct_submit_start_ack_event();
			break;
		case TEST_DATA_BIG_BURST_FROM_REMOTE:
			LOG_INF("Starting remote data big burst");
			proxy_direct_submit_start_ack_event();
			ret = proxy_burst_data_big_response_events();
			submit_test_end_remote_event(TEST_DATA_BIG_BURST_FROM_REMOTE, ret);
			break;
		default:
			/* Ignore */
			break;
		}
	} else if (is_test_end_event(eh)) {
		LOG_INF("Test finished");
		cur_test_id = TEST_IDLE;
	} else if (is_test_end_remote_event(eh)) {
		LOG_INF("Test remote finished");
		cur_test_id = TEST_IDLE;
	} else if (is_simple_event(eh)) {
		if (cur_test_id == TEST_SIMPLE_BURST) {
			if (++cur_test_pos >= TEST_CONFIG_SIMPLE_BURST_SIZE) {
				submit_test_end_remote_event(TEST_SIMPLE_BURST, 0);
			}
		}
	} else if (is_simple_ping_event(eh)) {
		proxy_direct_submit_pong_event();
	} else if (is_data_event(eh)) {
		if (cur_test_id == TEST_DATA_BURST) {
			if (++cur_test_pos >= TEST_CONFIG_DATA_BURST_SIZE) {
				submit_test_end_remote_event(TEST_DATA_BURST, 0);
			}
		} else if (cur_test_id == TEST_DATA_RESPONSE) {
			struct data_event *src = cast_data_event(eh);
			struct data_response_event *event = new_data_response_event();

			event->val1 = src->val1;
			event->val2 = src->val2;
			event->val3 = src->val3;
			event->val1u = src->val1u;
			event->val2u = src->val2u;
			event->val3u = src->val3u;

			LOG_INF("Sending data response");
			APP_EVENT_SUBMIT(event);
		}
	} else if (is_data_big_event(eh)) {
		if (cur_test_id == TEST_DATA_BIG_BURST) {
			if (++cur_test_pos >= TEST_CONFIG_DATA_BIG_BURST_SIZE) {
				submit_test_end_remote_event(TEST_DATA_BIG_BURST, 0);
			}
		} else if (cur_test_id == TEST_DATA_RESPONSE) {
			struct data_big_event *src = cast_data_big_event(eh);
			struct data_big_response_event *event = new_data_big_response_event();

			memcpy(&event->block, &src->block, sizeof(event->block));

			LOG_INF("Sending data big response");
			APP_EVENT_SUBMIT(event);
		}
	}
	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, data_event);
APP_EVENT_SUBSCRIBE(MODULE, data_big_event);
APP_EVENT_SUBSCRIBE(MODULE, simple_ping_event);
APP_EVENT_SUBSCRIBE(MODULE, simple_event);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
APP_EVENT_SUBSCRIBE(MODULE, test_end_event);
APP_EVENT_SUBSCRIBE(MODULE, test_end_remote_event);
