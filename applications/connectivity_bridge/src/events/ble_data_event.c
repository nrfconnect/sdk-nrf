/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <assert.h>

#include "ble_data_event.h"

static int log_ble_data_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct ble_data_event *event = cast_ble_data_event(eh);

	return snprintf(
		buf,
		buf_len,
		"buf:%p len:%d",
		event->buf,
		event->len);
}

EVENT_TYPE_DEFINE(ble_data_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_BLE_DATA_EVENT),
		  log_ble_data_event,
		  NULL);
