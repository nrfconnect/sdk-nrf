/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BUTTON_EVENT_H_
#define _BUTTON_EVENT_H_

/**
 * @file
 * @defgroup caf_button_event CAF Button Event
 * @{
 * @brief CAF Button Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Button event.
 *
 * The button event is submitted when a button is pressed or released.
 */
struct button_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the button. */
	uint16_t key_id;

	/** Information if the button was pressed or released. */
	bool pressed;
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

APP_EVENT_TYPE_DECLARE(button_event);

#ifdef __cplusplus
}
#endif

#endif /* _BUTTON_EVENT_H_ */
