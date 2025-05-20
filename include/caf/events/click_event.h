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

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

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
	CLICK_COUNT,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(CLICK)
};

/** @brief Click event.
 *
 * The click event is submitted when a click type is recorded for one of the monitored buttons.
 */
struct click_event {
	/** Event header. */
	struct app_event_header header;

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

APP_EVENT_TYPE_DECLARE(click_event);

#ifdef __cplusplus
}
#endif

#endif /* _CLICK_EVENT_H_ */
