/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_STATE_EVENT_H_
#define _LED_STATE_EVENT_H_

/**
 * @brief Led State Event
 * @defgroup asset Tracker Led State Event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>


/** @brief Asset Tracker led states in the application. */
enum led_state {
	LED_STATE_LTE_CONNECTING,
	LED_STATE_GNSS_SEARCHING,
	LED_STATE_CLOUD_PUBLISHING,
	LED_STATE_ACTIVE_MODE,
	LED_STATE_PASSIVE_MODE,
	LED_STATE_ERROR_SYSTEM_FAULT,
	LED_STATE_FOTA_UPDATE_REBOOT,
	LED_STATE_TURN_OFF,
	LED_STATE_COUNT
};


/** @brief Led state event. */
struct led_state_event {
	struct event_header header; /**< Event header. */

	enum led_state state;
};

EVENT_TYPE_DECLARE(led_state_event);

/** @} */

#endif /* _LED_STATE_EVENT_H_ */
