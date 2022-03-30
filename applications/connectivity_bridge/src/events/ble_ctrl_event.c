/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "ble_ctrl_event.h"

static void log_ble_ctrl_event(const struct application_event_header *aeh)
{
	const struct ble_ctrl_event *event = cast_ble_ctrl_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "cmd:%d", event->cmd);
}

APPLICATION_EVENT_TYPE_DEFINE(ble_ctrl_event,
		  log_ble_ctrl_event,
		  NULL,
		  APPLICATION_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_BLE_CTRL_EVENT,
				(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
