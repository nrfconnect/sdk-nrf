/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _FORCE_POWER_DOWN_EVENT_H_
#define _FORCE_POWER_DOWN_EVENT_H_

#include <event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple event to force a quick power down
 *
 * The event is called when the device has to power down quickly,
 * without waiting.
 * The device would power down if no wakeup event is generated in
 * short period of time.
 * The keep_alive event would be ignored.
 */
struct force_power_down_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(force_power_down_event);

/**
 * @brief Force power down now
 */
static inline void force_power_down(void)
{
	struct force_power_down_event *event = new_force_power_down_event();

	EVENT_SUBMIT(event);
}


#ifdef __cplusplus
}
#endif

#endif /* _FORCE_POWER_DOWN_EVENT_H_ */
