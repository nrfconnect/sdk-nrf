/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <assert.h>

#include "ble_ctrl_event.h"

static int log_ble_ctrl_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct ble_ctrl_event *event = cast_ble_ctrl_event(eh);

	return snprintf(
		buf,
		buf_len,
		"cmd:%d",
		event->cmd);
}

EVENT_TYPE_DEFINE(ble_ctrl_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_BLE_CTRL_EVENT),
		  log_ble_ctrl_event,
		  NULL);
