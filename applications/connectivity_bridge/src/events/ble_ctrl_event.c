/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "ble_ctrl_event.h"

static int log_ble_ctrl_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct ble_ctrl_event *event = cast_ble_ctrl_event(eh);

	EVENT_MANAGER_LOG(eh, "cmd:%d", event->cmd);
	return 0;
}

EVENT_TYPE_DEFINE(ble_ctrl_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_BLE_CTRL_EVENT),
		  log_ble_ctrl_event,
		  NULL);
