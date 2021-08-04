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
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER);
	}
}

void enable_uarts(void)
{
	const struct device *uart_dev;
	uint32_t current_state;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));

	int err = pm_device_state_get(uart_dev, &current_state);

	if (err) {
		printk("Failed to assess UART power state, pm_device_state_get: %d", err);
		return;
	}

	/* If UARTs are already enabled, do nothing */
	if (current_state == PM_DEVICE_STATE_ACTIVE) {
		return;
	}

	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE);
	}

	printk("UARTs enabled\n");
}
