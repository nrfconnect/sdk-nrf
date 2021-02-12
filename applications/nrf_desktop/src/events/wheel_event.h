/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _WHEEL_EVENT_H_
#define _WHEEL_EVENT_H_

/**
 * @brief Wheel Event
 * @defgroup wheel_event Wheel Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wheel_event {
	struct event_header header;

	int16_t wheel;
};

EVENT_TYPE_DECLARE(wheel_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _WHEEL_EVENT_H_ */
