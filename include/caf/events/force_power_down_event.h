/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FORCE_POWER_DOWN_EVENT_H_
#define _FORCE_POWER_DOWN_EVENT_H_

/**
 * @file
 * @defgroup caf_force_power_down_event CAF Force Power Down Event
 * @{
 * @brief CAF Force Power Down Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple event to force a quick power down
 *
 * The event is called when the device has to power down quickly,
 * without waiting.
 * The device would power down if no wakeup event is generated in
 * short period of time.
 * The keep_alive event would be ignored.
 */
struct force_power_down_event {
	/** Event header. */
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(force_power_down_event);

/**
 * @brief Force power down now
 */
static inline void force_power_down(void)
{
	struct force_power_down_event *event = new_force_power_down_event();

	APP_EVENT_SUBMIT(event);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FORCE_POWER_DOWN_EVENT_H_ */
