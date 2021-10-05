/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EI_DATA_FORWARDER_EVENT_H_
#define _EI_DATA_FORWARDER_EVENT_H_

/**
 * @brief Edge Impulse Data Forwarder Event
 * @defgroup ei_data_forwarder_event Edge Impulse Data Forwarder Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Edge Impulse data forwarder states. */
enum ei_data_forwarder_state {
	EI_DATA_FORWARDER_STATE_DISABLED,
	EI_DATA_FORWARDER_STATE_DISCONNECTED,
	EI_DATA_FORWARDER_STATE_CONNECTED,
	EI_DATA_FORWARDER_STATE_TRANSMITTING,

	EI_DATA_FORWARDER_STATE_COUNT
};

/** @brief Edge Impulse data forwarder event. */
struct ei_data_forwarder_event {
	struct event_header header; /**< Event header. */

	enum ei_data_forwarder_state state; /**< Edge Impulse data forwarder state. */
};

EVENT_TYPE_DECLARE(ei_data_forwarder_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EI_DATA_FORWARDER_EVENT_H_ */
