/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _WHEEL_EVENT_H_
#define _WHEEL_EVENT_H_

/**
 * @brief Wheel Event
 * @defgroup wheel_event Wheel Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wheel_event {
	struct app_event_header header;

	int16_t wheel;
};

APP_EVENT_TYPE_DECLARE(wheel_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _WHEEL_EVENT_H_ */
