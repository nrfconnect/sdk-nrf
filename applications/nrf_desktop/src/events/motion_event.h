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

#include <app_evt_mgr.h>
#include <app_evt_mgr_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct motion_event {
	struct application_event_header header;

	int16_t dx;
	int16_t dy;
};

APPLICATION_EVENT_TYPE_DECLARE(motion_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MOTION_EVENT_H_ */
