/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <device.h>
#include "uart.h"

void disable_uarts(void)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER, NULL, NULL);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER, NULL, NULL);
	}
}

void enable_uarts(void)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
	}
}
