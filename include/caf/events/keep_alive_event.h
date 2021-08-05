/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _KEEP_ALIVE_EVENT_H_
#define _KEEP_ALIVE_EVENT_H_

#include <event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple event to keep the power manager alive
 *
 * The event called would keep power manager from power downing
 * for configured period of time.
 * The event does not take any argument.
 */
struct keep_alive_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(keep_alive_event);

/**
 * @brief Keep the device alive
 *
 * The function generates the power manager event that would
 * reset the power down counter when processed.
 */
static inline void keep_alive(void)
{
	struct keep_alive_event *event = new_keep_alive_event();

	EVENT_SUBMIT(event);
}

#ifdef __cplusplus
}
#endif

#endif /* _KEEP_ALIVE_EVENT_H_ */
