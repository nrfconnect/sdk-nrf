/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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


#include <zephyr/sys/atomic.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct module_suspend_req_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the module. */
	const void *module_id;
};

APP_EVENT_TYPE_DECLARE(module_suspend_req_event);

struct module_resume_req_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the module. */
	const void *module_id;
};

APP_EVENT_TYPE_DECLARE(module_resume_req_event);

#endif /* _MODULE_SUSPEND_EVENT_H_ */
