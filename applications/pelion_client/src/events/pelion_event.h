/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PELION_STATE_EVENT_H_
#define _PELION_STATE_EVENT_H_

/**
 * @brief PELION State Event
 * @defgroup pelion_state_event PELION State Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Pelion states. */
enum pelion_state {
	PELION_STATE_DISABLED,
	PELION_STATE_INITIALIZED,
	PELION_STATE_REGISTERED,
	PELION_STATE_UNREGISTERED,
	PELION_STATE_SUSPENDED,

	PELION_STATE_COUNT
};

/** @brief Pelion state event. */
struct pelion_state_event {
	struct event_header header;

	enum pelion_state state;
};
EVENT_TYPE_DECLARE(pelion_state_event);


/** @brief Create pelion objects event. */
struct pelion_create_objects_event {
	struct event_header header;

	void *object_list;
};
EVENT_TYPE_DECLARE(pelion_create_objects_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PELION_STATE_EVENT_H_ */
