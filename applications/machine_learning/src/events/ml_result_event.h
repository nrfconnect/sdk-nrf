/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Machine Learning Result event header file.
 */

#ifndef _ML_RESULT_EVENT_H_
#define _ML_RESULT_EVENT_H_

/**
 * @brief Machine Learning Result Event
 * @defgroup ml_result_event Machine Learning Result Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Machine learning classification result event. */
struct ml_result_event {
	/** Event header. */
	struct app_event_header header;

	/** Classification label. */
	const char *label;
	/** Classification value. */
	float value;
	/** Anomaly value. */
	float anomaly;
};

APP_EVENT_TYPE_DECLARE(ml_result_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ML_RESULT_EVENT_H_ */
