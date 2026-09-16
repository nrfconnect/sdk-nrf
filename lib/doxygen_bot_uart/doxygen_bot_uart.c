/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "doxygen_bot_uart.h"

static bool uart_initialized;

int doxy_uart_init(void)
{
	uart_initialized = true;

	return 0;
}

int doxy_uart_deinit(void)
{
	uart_initialized = false;

	return 0;
}

int doxy_uart_transmit(const uint8_t *data, size_t len)
{
	if (data == NULL || len == 0) {
		return -EINVAL;
	}

	if (!uart_initialized) {
		return -ENODEV;
	}

	return (int)len;
}

int doxy_uart_receive(uint8_t *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (!uart_initialized) {
		return -ENODEV;
	}

	return 0;
}
