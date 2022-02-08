/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "ble_data_event.h"

static void log_ble_data_event(const struct event_header *eh)
{
	const struct ble_data_event *event = cast_ble_data_event(eh);

	EVENT_MANAGER_LOG(eh, "buf:%p len:%d", event->buf, event->len);
}

EVENT_TYPE_DEFINE(ble_data_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_BLE_DATA_EVENT),
		  log_ble_data_event,
		  NULL);
