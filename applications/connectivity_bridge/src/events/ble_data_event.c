/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "ble_data_event.h"

static void log_ble_data_event(const struct app_event_header *aeh)
{
	const struct ble_data_event *event = cast_ble_data_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "buf:%p len:%d", event->buf, event->len);
}

APP_EVENT_TYPE_DEFINE(ble_data_event,
		  log_ble_data_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_BLE_DATA_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
