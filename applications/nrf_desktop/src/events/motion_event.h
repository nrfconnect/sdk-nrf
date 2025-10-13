/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MOTION_EVENT_H_
#define _MOTION_EVENT_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

/**
 * @brief Motion Event
 * @defgroup nrf_desktop_motion_event Motion Event
 *
 * The @ref motion_event is used to propagate information about user input (motion sensor samples).
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Motion event. */
struct motion_event {
	/** Event header. */
	struct app_event_header header;

	/** Relative motion over the X axis. */
	int16_t dx;

	/** Relative motion over the Y axis. */
	int16_t dy;

	/** Information if motion source is still active (fetching) or goes to idle state.
	 *
	 * Once HID input report subscription is enabled, an active motion source keeps providing
	 * motion samples for subsequent HID input reports. The active motion source goes to idle
	 * state when no motion is detected for some time. The motion source in idle state becomes
	 * active when motion is detected.
	 */
	bool active;
};

APP_EVENT_TYPE_DECLARE(motion_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MOTION_EVENT_H_ */
