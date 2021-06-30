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

#include "event_manager.h"


/** @brief Asset Tracker led states in the ap. */
enum led_states_type {
	LED_STATES_LTE_CONNECTING,
	LED_STATES_GPS_SEARCHING,
	LED_STATES_CLOUD_PUBLISHING,
	LED_STATES_ACTIVE_MODE,
	LED_STATES_PASSIVE_MODE,
	LED_STATES_ERROR_SYSTEM_FAULT,
	LED_STATES_FOTA_UPDATE_REBOOT,
	LED_STATES_TURN_OFF,

	LED_STATES_STATE_COUNT
};


/** @brief Led state event. */
struct led_state_event {
	struct event_header header; /**< Event header. */

	enum led_states_type state;

};


EVENT_TYPE_DECLARE(led_state_event);


#endif /* _LED_STATE_EVENT_H_ */
