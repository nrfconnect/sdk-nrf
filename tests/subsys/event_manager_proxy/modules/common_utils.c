/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <app_event_manager.h>

#include "test_config.h"
#include "common_utils.h"
#include "simple_events.h"
#include "data_events.h"

#define MODULE common_utils

void proxy_direct_submit_event(struct app_event_header *eh)
{
	BUILD_ASSERT(IS_ENABLED(CONFIG_APP_EVENT_MANAGER_POSTPROCESS_HOOKS));

	STRUCT_SECTION_FOREACH(event_postprocess_hook, h) {
		h->hook(eh);
	}
}

int proxy_burst_simple_events(void)
{
	struct simple_event *event = new_simple_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_SIMPLE_BURST_SIZE; ++cnt) {
		proxy_direct_submit_event(&event->header);
	}
	app_event_manager_free(event);

	return 0;
}

int proxy_burst_simple_pong_events(void)
{
	struct simple_pong_event *event = new_simple_pong_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_SIMPLE_BURST_SIZE; ++cnt) {
		proxy_direct_submit_event(&event->header);
	}
	app_event_manager_free(event);

	return 0;
}

int proxy_burst_data_events(void)
{
	struct data_event *event = new_data_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BURST_SIZE; ++cnt) {
		event->val1 = (int8_t)cnt;
		event->val2 = (int16_t)cnt;
		event->val3 = (int32_t)cnt;
		event->val1u = (uint8_t)cnt;
		event->val2u = (uint16_t)cnt;
		event->val3u = (uint32_t)cnt;

		proxy_direct_submit_event(&event->header);
	}

	app_event_manager_free(event);

	return 0;
}

int proxy_burst_data_response_events(void)
{
	struct data_response_event *event = new_data_response_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BURST_SIZE; ++cnt) {
		event->val1 = (int8_t)cnt;
		event->val2 = (int16_t)cnt;
		event->val3 = (int32_t)cnt;
		event->val1u = (uint8_t)cnt;
		event->val2u = (uint16_t)cnt;
		event->val3u = (uint32_t)cnt;

		proxy_direct_submit_event(&event->header);
	}

	app_event_manager_free(event);

	return 0;
}

int proxy_burst_data_big_events(void)
{
	struct data_big_event *event = new_data_big_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BIG_BURST_SIZE; ++cnt) {
		for (size_t n = 0; n < DATA_BIG_EVENT_BLOCK_SIZE; ++n) {
			event->block[n] = (uint32_t)(cnt + n);
		}

		proxy_direct_submit_event(&event->header);
	}

	app_event_manager_free(event);

	return 0;
}

int proxy_burst_data_big_response_events(void)
{
	struct data_big_response_event *event = new_data_big_response_event();

	for (size_t cnt = 0; cnt < TEST_CONFIG_DATA_BIG_BURST_SIZE; ++cnt) {
		for (size_t n = 0; n < DATA_BIG_EVENT_BLOCK_SIZE; ++n) {
			event->block[n] = (uint32_t)(cnt + n);
		}

		proxy_direct_submit_event(&event->header);
	}

	app_event_manager_free(event);

	return 0;
}
