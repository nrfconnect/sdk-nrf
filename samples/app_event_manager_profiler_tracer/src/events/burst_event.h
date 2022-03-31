/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BURST_EVENT_H_
#define _BURST_EVENT_H_

/**
 * @brief Burst event
 * @defgroup burst_event Burst event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct burst_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(burst_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BURST_EVENT_H_ */
