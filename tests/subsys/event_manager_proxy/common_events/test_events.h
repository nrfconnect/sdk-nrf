/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEST_EVENTS_H_
#define _TEST_EVENTS_H_

/**
 * @brief Test Start and End Events
 * @defgroup test_events Test Start and End Events
 * @{
 */

#include <app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

enum test_id {
	TEST_IDLE,
	TEST_SIMPLE_PING_PONG,
	TEST_SIMPLE_BURST,
	TEST_SIMPLE_BURST_FROM_REMOTE,
	TEST_DATA_RESPONSE,
	TEST_DATA_BURST,
	TEST_DATA_BURST_FROM_REMOTE,
	TEST_DATA_BIG_BURST,
	TEST_DATA_BIG_BURST_FROM_REMOTE,

	TEST_END,
	TEST_CNT
};

/**
 * @brief Test start event
 *
 * Test start is sent only by the host.
 * It would never be send by remote.
 * Remote signs in host for that event.
 * Host cannot sign for it in the remote to avoid event resenting in a loop.
 */
struct test_start_event {
	struct app_event_header header;

	uint8_t test_id;
};

APP_EVENT_TYPE_DECLARE(test_start_event);

/**
 * @brief Test start acknowledgment from the remote
 *
 * This event is send by the remote to notify the host
 * that remote is ready to backup the test execution.
 */
struct test_start_ack_event {
	struct app_event_header header;

	uint8_t test_id;
};

APP_EVENT_TYPE_DECLARE(test_start_ack_event);

/**
 * @brief Test end event
 *
 * This event is sent by host and listened by remote.
 */
struct test_end_event {
	struct app_event_header header;

	uint8_t test_id;
};

APP_EVENT_TYPE_DECLARE(test_end_event);

/**
 * @brief Test end event sent by remote
 *
 * This event is send by the remote to mark the test end on its side.
 */
struct test_end_remote_event {
	struct app_event_header header;

	uint8_t test_id;
	int res;
};

APP_EVENT_TYPE_DECLARE(test_end_remote_event);


static inline void submit_test_start_event(enum test_id id)
{
	struct test_start_event *event = new_test_start_event();

	__ASSERT_NO_MSG(id < UINT8_MAX);
	event->test_id = (uint8_t)id;
	APP_EVENT_SUBMIT(event);
}

static inline void submit_test_start_ack_event(enum test_id id)
{
	struct test_start_ack_event *event = new_test_start_ack_event();

	__ASSERT_NO_MSG(id < UINT8_MAX);
	event->test_id = (uint8_t)id;
	APP_EVENT_SUBMIT(event);
}

static inline void submit_test_end_event(enum test_id id)
{
	struct test_end_event *event = new_test_end_event();

	__ASSERT_NO_MSG(id < UINT8_MAX);
	event->test_id = (uint8_t)id;
	APP_EVENT_SUBMIT(event);
}

static inline void submit_test_end_remote_event(enum test_id id, int res)
{
	struct test_end_remote_event *event = new_test_end_remote_event();

	__ASSERT_NO_MSG(id < UINT8_MAX);
	event->test_id = (uint8_t)id;
	event->res = res;
	APP_EVENT_SUBMIT(event);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEST_EVENTS_H_ */
