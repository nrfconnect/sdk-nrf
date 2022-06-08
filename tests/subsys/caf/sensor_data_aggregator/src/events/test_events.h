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
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum test_id {
	TEST_IDLE,
	TEST_EVENT_MANAGER,
	TEST_BASIC,
	TEST_ORDER,
	TEST_STATUS,

	TEST_CNT
};

struct test_start_event {
	struct app_event_header header;

	enum test_id test_id;
};

APP_EVENT_TYPE_DECLARE(test_start_event);

struct test_end_event {
	struct app_event_header header;

	enum test_id test_id;
};

APP_EVENT_TYPE_DECLARE(test_end_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEST_EVENTS_H_ */
