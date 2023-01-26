/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_dongle_peer_event.h"

static void log_ble_dongle_peer_event(const struct app_event_header *aeh)
{
	const struct ble_dongle_peer_event *event = cast_ble_dongle_peer_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "Bluetooth dongle peer app ID: %" PRIu8, event->bt_app_id);
}

APP_EVENT_TYPE_DEFINE(ble_dongle_peer_event,
		      log_ble_dongle_peer_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_DONGLE_PEER_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
