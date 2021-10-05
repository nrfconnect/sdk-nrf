/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLICK_EVENT_H_
#define _CLICK_EVENT_H_

/**
 * @file
 * @defgroup caf_click_event CAF Click Event
 * @{
 * @brief CAF Click Event.
 */

#include <event_manager.h>
#include <event_manager_profiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Click types.
 *
 * The click type refers to the way a button can be pressed.
 */
enum click {
	/** Button pressed and held for a period of time that is too long for CLICK_SHORT,
	 * but too short for CLICK_LONG.
	 */
	CLICK_NONE,

	/** Button pressed and released after a short time. */
	CLICK_SHORT,

	/** Button pressed and held for a long period of time. */
	CLICK_LONG,

	/** Two sequences of the button press and release in a short time interval. */
	CLICK_DOUBLE,

	/** Number of click types. */
	CLICK_COUNT
};

/** @brief Click event.
 *
 * The click event is submitted when a click type is recorded for one of the monitored buttons.
 */
struct click_event {
	/** Event header. */
	struct event_header header;

	/** ID of the button - matching the key_id used by the @ref button_event. */
	uint16_t key_id;

	/** Detected click type. */
	enum click click;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

EVENT_TYPE_DECLARE(click_event);

#ifdef __cplusplus
}
#endif

#endif /* _CLICK_EVENT_H_ */
