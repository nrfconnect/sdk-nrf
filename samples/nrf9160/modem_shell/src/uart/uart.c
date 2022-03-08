/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <device.h>
#include <pm/device.h>
#include "uart.h"

/* Utility function to enable/disable UART1.
 * This function uses direct register accesses to enable/disable UART1.
 * The device management API and power management APIs can not be used because the zephyr driver
 * is not used for accessing UART1. NRFX driver is used instead by the modem trace handling code.
 */
static void uart1_set_enable(bool enable)
{
	if (enable) {
		NRF_UARTE1_NS->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
		NRF_UARTE1_NS->TASKS_STARTRX = 1;
		NRF_UARTE1_NS->TASKS_STARTTX = 1;
	} else {
		/* Stop TX and RX operation and wait for the corresponding events to be generated
		 * before disabling the peripheral.
		 * Ref: https://infocenter.nordicsemi.com/topic/ps_nrf9160/uarte.html
		 */
		NRF_UARTE1_NS->TASKS_STOPRX = 1;
		while (NRF_UARTE1_NS->EVENTS_RXTO == 0) {
			/* Wait until RX Timeout event is raised. */
		}
		NRF_UARTE1_NS->TASKS_STOPTX = 1;
		while (NRF_UARTE1_NS->EVENTS_TXSTOPPED == 0) {
			/* Wait until TX Stopped event is raised. */
		}
		NRF_UARTE1_NS->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
	}
}

/* Utility function to enable/disable UART0.
 * This function uses power management API to enable/disable UART0.
 * This is because the modem shell example uses UART0 for shell operations and console prints
 * (see mosh_print() function). The state of the UART0 is propagated to the shell only when
 * the power management API is used to change the state of UART0. If direct register accesses are
 * used to disable/enable UART0, it will lead the shell to hang when sending via UART0.
 */
static void uart0_set_enable(bool enable)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));

	if (!uart_dev) {
		return;
	}

	pm_device_action_run(uart_dev, enable ? PM_DEVICE_ACTION_RESUME : PM_DEVICE_ACTION_SUSPEND);
}

void disable_uarts(void)
{
	uart0_set_enable(false);
	uart1_set_enable(false);
}

void enable_uarts(void)
{
	uart0_set_enable(true);
	uart1_set_enable(true);
}
