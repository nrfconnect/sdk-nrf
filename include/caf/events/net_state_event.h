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

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Net states. */
enum net_state {
	NET_STATE_DISABLED,
	NET_STATE_DISCONNECTED,
	NET_STATE_CONNECTED,

	NET_STATE_COUNT,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(NET_STATE)
};

/** @brief NET state event. */
struct net_state_event {
	struct app_event_header header;

	enum net_state state;
	const void *id;
};
APP_EVENT_TYPE_DECLARE(net_state_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _NET_STATE_EVENT_H_ */
