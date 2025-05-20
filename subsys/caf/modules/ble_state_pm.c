/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <caf/events/ble_common_event.h>

#define MODULE ble_state_pm
#include <caf/events/module_state_event.h>
#include <caf/events/power_manager_event.h>
#include <caf/events/keep_alive_event.h>


static bool app_event_handler(const struct app_event_header *aeh)
{
	__ASSERT_NO_MSG(is_ble_peer_event(aeh));

	static unsigned int connection_count;
	const struct ble_peer_event *event = cast_ble_peer_event(aeh);

	switch (event->state) {
	case PEER_STATE_CONNECTED:
		connection_count++;
		__ASSERT_NO_MSG(connection_count < UINT_MAX);
		keep_alive();
		power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
		break;
	case PEER_STATE_DISCONNECTED:
		__ASSERT_NO_MSG(connection_count > 0);
		--connection_count;
		if (!connection_count) {
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
		}
		break;
	case PEER_STATE_SECURED:
	case PEER_STATE_CONN_FAILED:
	case PEER_STATE_DISCONNECTING:
		/* No action */
		break;
	default:
		__ASSERT_NO_MSG(false);
		break;
	}
	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
