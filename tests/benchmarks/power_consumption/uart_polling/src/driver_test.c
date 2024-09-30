/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

void thread_definition(void)
{
	while (1) {
		uart_poll_out(uart_dev, 'a');
		uart_poll_out(uart_dev, '\n');
		uart_poll_out(uart_dev, '\r');
	}
}
