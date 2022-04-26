/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SIMPLE_EVENTS_H_
#define _SIMPLE_EVENTS_H_

/**
 * @brief Test Start and End Events
 * @defgroup test_events Test Start and End Events
 * @{
 */

#include <app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple event
 *
 * Simple event to be sent from host to remote.
 * The event is intended to be ignored by the remote.
 */
struct simple_event {
	struct app_event_header header;
};
APP_EVENT_TYPE_DECLARE(simple_event);

/**
 * @brief Simple ping event
 *
 * The remote is expected to respond by @ref simple_pong_event.
 */
struct simple_ping_event {
	struct app_event_header header;
};
APP_EVENT_TYPE_DECLARE(simple_ping_event);

/**
 * @brief Simple event from remote
 *
 * The event is intended to be sent by the remote as a simple event.
 * Remote responds this event in response to @ref simple_ping_event
 * and uses it for simple burst tests.
 */
struct simple_pong_event {
	struct app_event_header header;
};
APP_EVENT_TYPE_DECLARE(simple_pong_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SIMPLE_EVENTS_H_ */
