/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <assert.h>

#include "uart_data_event.h"

static int log_uart_data_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct uart_data_event *event = cast_uart_data_event(eh);

	return snprintf(
		buf,
		buf_len,
		"dev:%u buf:%p len:%d",
		event->dev_idx,
		event->buf,
		event->len);
}

EVENT_TYPE_DEFINE(uart_data_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_UART_DATA_EVENT),
		  log_uart_data_event,
		  NULL);
