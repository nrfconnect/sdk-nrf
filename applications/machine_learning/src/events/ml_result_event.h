/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ML_RESULT_EVENT_H_
#define _ML_RESULT_EVENT_H_

/**
 * @brief Machine Learning Result Event
 * @defgroup ml_result_event Machine Learning Result Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler.h>
#include <caf/events/module_state_event.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Machine learning classification result event. */
struct ml_result_event {
	struct event_header header; /**< Event header. */

	const char *label; /**< Classification label. */
	float value; /**< Classification value. */
	float anomaly; /**< Anomaly value. */
};

/** @brief Sign in event
 *
 * The event that is called by modules
 * to mark that the module actively listens for the result event.
 */
struct ml_result_signin_event {
	struct event_header header; /**< Event header. */

	size_t module_idx; /**< @brief The index of the module */
	bool state;        /**< @brief  Requested state: true to sign in, false to sign off */
};

EVENT_TYPE_DECLARE(ml_result_event);
EVENT_TYPE_DECLARE(ml_result_signin_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ML_RESULT_EVENT_H_ */
