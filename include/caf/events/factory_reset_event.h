/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _FACTORY_RESET_EVENT_H_
#define _FACTORY_RESET_EVENT_H_

#include <event_manager.h>
#include <event_manager_profiler.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Factory reset event
 *
 * The event that informs that factory reset was requested.
 */
struct factory_reset_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(factory_reset_event);


#ifdef __cplusplus
}
#endif

#endif /* _FACTORY_RESET_EVENT_H_ */
