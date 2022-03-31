/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MOTION_EVENT_H_
#define _MOTION_EVENT_H_

/**
 * @brief Motion Event
 * @defgroup motion_event Motion Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct motion_event {
	struct app_event_header header;

	int16_t dx;
	int16_t dy;
};

APP_EVENT_TYPE_DECLARE(motion_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MOTION_EVENT_H_ */
