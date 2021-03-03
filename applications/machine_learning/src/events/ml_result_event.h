/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _ML_RESULT_EVENT_H_
#define _ML_RESULT_EVENT_H_

/**
 * @brief Machine Learning Result Event
 * @defgroup ei_result_event Machine Learning Result Event
 * @{
 */

#include "event_manager.h"

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

EVENT_TYPE_DECLARE(ml_result_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ML_RESULT_EVENT_H_ */
