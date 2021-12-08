/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NET_STATE_EVENT_H_
#define _NET_STATE_EVENT_H_

/**
 * @brief NET State Event
 * @defgroup net_state_event NET State Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Net states. */
enum net_state {
	NET_STATE_DISABLED,
	NET_STATE_DISCONNECTED,
	NET_STATE_CONNECTED,

	NET_STATE_COUNT
};

/** @brief NET state event. */
struct net_state_event {
	struct event_header header;

	enum net_state state;
	const void *id;
};
EVENT_TYPE_DECLARE(net_state_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _NET_STATE_EVENT_H_ */
