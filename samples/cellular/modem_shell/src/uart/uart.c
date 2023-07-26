/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include "uart.h"

/* Utility function to enable/disable UART1.
 * This function selects the used API based on the enabled UART trace backend.
 */
static void uart1_set_enable(bool enable)
{
#if defined(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR)
	const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

	if (!device_is_ready(uart_dev)) {
		return;
	}

	pm_device_action_run(uart_dev, enable ? PM_DEVICE_ACTION_RESUME : PM_DEVICE_ACTION_SUSPEND);
#elif defined(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ASYNC) || \
	defined(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC)
	if (!IS_ENABLED(CONFIG_NRFX_UARTE1)) {
		return;
	}

	if (enable) {
		NRF_UARTE1_NS->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
	} else {
		/* Check if transmission is ongoing, otherwise there will never be a
		 * TX Stopped event.
		 */
		if (NRF_UARTE1_NS->EVENTS_TXSTARTED == 1) {
			/* Transmission ongoing, stop it.*/
			NRF_UARTE1_NS->TASKS_STOPTX = 1;
			while (NRF_UARTE1_NS->EVENTS_TXSTOPPED == 0) {
				/* Wait until TX Stopped event is raised. */
			}
		}
		NRF_UARTE1_NS->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
	}
#endif
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
	const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

	if (!device_is_ready(uart_dev)) {
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
