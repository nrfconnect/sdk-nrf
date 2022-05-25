/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Battery state list. */
#define BATTERY_STATE_LIST	\
	X(IDLE)			\
	X(CHARGING)		\
	X(ERROR)

/** @brief Battery states. */
enum battery_state {
#define X(name) _CONCAT(BATTERY_STATE_, name),
	BATTERY_STATE_LIST
#undef X

	BATTERY_STATE_COUNT
};


/** @brief Battery state event. */
struct battery_state_event {
	struct app_event_header header;

	enum battery_state state;
};

APP_EVENT_TYPE_DECLARE(battery_state_event);


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
