/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ML_STATE_EVENT_H_
#define _ML_STATE_EVENT_H_

/**
 * @brief Machine Learning State Event
 * @defgroup ml_state_event Machine Learning State Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Machine learning states. */
enum ml_state {
	ML_STATE_MODEL_RUNNING,
	ML_STATE_DATA_FORWARDING,

	ML_STATE_COUNT
};


/** @brief Machine learning state event. */
struct ml_state_event {
	struct event_header header; /**< Event header. */

	enum ml_state state; /**< Machine learning state. */
};


EVENT_TYPE_DECLARE(ml_state_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ML_STATE_EVENT_H_ */
