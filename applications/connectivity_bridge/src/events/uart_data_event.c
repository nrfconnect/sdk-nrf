/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "uart_data_event.h"

static void log_uart_data_event(const struct event_header *eh)
{
	const struct uart_data_event *event = cast_uart_data_event(eh);

	EVENT_MANAGER_LOG(eh,
		"dev:%u buf:%p len:%d",
		event->dev_idx,
		event->buf,
		event->len);
}

EVENT_TYPE_DEFINE(uart_data_event,
		  log_uart_data_event,
		  NULL,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_UART_DATA_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
