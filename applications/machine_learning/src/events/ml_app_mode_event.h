/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ML_APP_MODE_EVENT_H_
#define _ML_APP_MODE_EVENT_H_

/**
 * @brief Machine Learning Application Mode Event
 * @defgroup ml_app_mode_event Machine Learning Application Mode Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Machine learning application modes. */
enum ml_app_mode {
	ML_APP_MODE_MODEL_RUNNING,
	ML_APP_MODE_DATA_FORWARDING,

	ML_APP_MODE_COUNT
};


/** @brief Machine learning application mode event. */
struct ml_app_mode_event {
	struct event_header header; /**< Event header. */

	enum ml_app_mode mode; /**< Machine learning application mode. */
};


EVENT_TYPE_DECLARE(ml_app_mode_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ML_APP_MODE_EVENT_H_ */
