/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _MEASUREMENT_EVENT_H_
#define _MEASUREMENT_EVENT_H_

/**
 * @brief Measurement Event
 * @defgroup measurement_event Measurement Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct measurement_event {
	struct event_header header;

	s8_t value1;
	s16_t value2;
	s32_t value3;
};

EVENT_TYPE_DECLARE(measurement_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MEASUREMENT_EVENT_H_ */
