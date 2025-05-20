/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CPU_LOAD_EVENT_H_
#define _CPU_LOAD_EVENT_H_

/**
 * @brief CPU Load Event
 * @defgroup cpu_load_event CPU Load Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief CPU load event. */
struct cpu_load_event {
	struct app_event_header header; /**< Event header. */

	uint32_t load; /**< CPU load [in 0,001% units]. */
};

APP_EVENT_TYPE_DECLARE(cpu_load_event);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* _CPU_LOAD_EVENT_H_ */
