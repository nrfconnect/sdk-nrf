/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BATTERY_EVENT_H_
#define _BATTERY_EVENT_H_

/**
 * @brief Battery Event
 * @defgroup battery_event Battery Event
 * @{
 */

#include <app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Battery voltage level event. */
struct battery_level_event {
	struct app_event_header header;

	uint8_t level;
};

APP_EVENT_TYPE_DECLARE(battery_level_event);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* _BATTERY_EVENT_H_ */
