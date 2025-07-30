/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULE_SUSPEND_EVENT_H_
#define _MODULE_SUSPEND_EVENT_H_

/**
 * @file
 * @defgroup caf_module_suspend_event CAF Module Suspend Event
 * @{
 * @brief CAF Module Suspend Event.
 */


#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Module suspend request event
 *
 * The event is submitted by a module to request the suspension of another module.
 */
struct module_suspend_req_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the module to be suspended. */
	const void *sink_module_id;

	/** ID of the module that requests the suspension. */
	const void *src_module_id;
};

APP_EVENT_TYPE_DECLARE(module_suspend_req_event);

/**
 * @brief Module resume request event
 *
 * The event is submitted by a module to request the resumption of another module.
 */
struct module_resume_req_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the module to be resumed. */
	const void *sink_module_id;

	/** ID of the module that requests the resumption. */
	const void *src_module_id;
};

APP_EVENT_TYPE_DECLARE(module_resume_req_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODULE_SUSPEND_EVENT_H_ */
