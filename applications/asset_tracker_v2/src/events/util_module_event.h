/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UTIL_MODULE_EVENT_H_
#define _UTIL_MODULE_EVENT_H_

/**
 * @brief Util module event
 * @defgroup util_module_event Utility module event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Event types submitted by the utility module. */
enum util_module_event_type {
	/** Shutdown request sent to all modules in the system upon an irrecoverable error or
	 *  finished FOTA update.
	 *  It is expected that each module performs the necessary shutdown routines and reports
	 *  back upon this event.
	 */
	UTIL_EVT_SHUTDOWN_REQUEST
};

/** @brief Shutdown reason included in shutdown requests from the utility module. */
enum shutdown_reason {
	/** Generic reason, typically an irrecoverable error. */
	REASON_GENERIC,
	/** The application shuts down because a FOTA update finished. */
	REASON_FOTA_UPDATE,
};

/** @brief Utility module event. */
struct util_module_event {
	/** Utility module application event header. */
	struct app_event_header header;
	/** Utility module event type. */
	enum util_module_event_type type;
	/** Variable that contains the reason for the shutdown request. */
	enum shutdown_reason reason;
};

APP_EVENT_TYPE_DECLARE(util_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _UTIL_MODULE_EVENT_H_ */
