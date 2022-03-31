/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MEASUREMENT_EVENT_H_
#define _MEASUREMENT_EVENT_H_

/**
 * @brief Measurement Event
 * @defgroup measurement_event Measurement Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct measurement_event {
	struct app_event_header header;

	int8_t value1;
	int16_t value2;
	int32_t value3;
};

APP_EVENT_TYPE_DECLARE(measurement_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MEASUREMENT_EVENT_H_ */
