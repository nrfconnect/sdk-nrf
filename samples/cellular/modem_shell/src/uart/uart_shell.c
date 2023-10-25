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
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib_trace.h>

#include "uart_shell.h"
#include "mosh_print.h"
#include "link.h"

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const shell_uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

bool uart_shell_disable_during_sleep_requested;
extern bool link_shell_msleep_notifications_subscribed;

static void uart_disable_handler(struct k_work *work)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART
	int err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_OFF);
	if (err) {
		mosh_error("nrf_modem_lib_trace_level_set() failed with err = %d.", err);
	}
#endif

	if (device_is_ready(shell_uart_dev)) {
		pm_device_action_run(shell_uart_dev, PM_DEVICE_ACTION_SUSPEND);
	}
}

static void uart_enable_handler(struct k_work *work)
{
	if (device_is_ready(shell_uart_dev)) {
		pm_device_action_run(shell_uart_dev, PM_DEVICE_ACTION_RESUME);
	}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART
	int err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_FULL);
	if (err) {
		mosh_error("nrf_modem_lib_trace_level_set() failed with err = %d.", err);
	}
#endif

	mosh_print("UARTs enabled\n");
}

static K_WORK_DEFINE(uart_disable_work, &uart_disable_handler);

static K_WORK_DELAYABLE_DEFINE(uart_enable_work, &uart_enable_handler);

static int cmd_uart_disable(const struct shell *shell, size_t argc, char **argv)
{
	int sleep_time;

	sleep_time = atoi(argv[1]);
	if (sleep_time < 0) {
		mosh_error("disable: invalid sleep time");
		return -EINVAL;
	}

	if (sleep_time > 0) {
		mosh_print("disable: disabling UARTs for %d seconds", sleep_time);
	} else {
		mosh_print("disable: disabling UARTs indefinitely");
	}
	k_sleep(K_MSEC(500)); /* allow little time for printing the notification */
	k_work_submit(&uart_disable_work);

	if (sleep_time > 0) {
		k_work_schedule(&uart_enable_work, K_SECONDS(sleep_time));
	}

	return 0;
}

void uart_toggle_power_state_at_event(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_MODEM_SLEEP_ENTER) {
		mosh_print("Modem sleep enter: disabling UARTs requested");
		k_work_submit(&uart_disable_work);
	} else if (evt->type == LTE_LC_EVT_MODEM_SLEEP_EXIT) {
		k_work_schedule(&uart_enable_work, K_NO_WAIT);
	}
}

void uart_toggle_power_state(void)
{
	enum pm_device_state shell_uart_power_state;
	int err;

	if (!device_is_ready(shell_uart_dev)) {
		mosh_print("Shell UART device not ready");
		return;
	}

	err = pm_device_state_get(shell_uart_dev, &shell_uart_power_state);
	if (err) {
		mosh_error("Failed to assess shell UART power state, pm_device_state_get: %d.",
			   err);
		return;
	}

	if (shell_uart_power_state == PM_DEVICE_STATE_ACTIVE) {
		k_work_submit(&uart_disable_work);
	} else {
		k_work_schedule(&uart_enable_work, K_NO_WAIT);
	}
}

static int cmd_uart_disable_when_sleep(void)
{
	/* Modem sleep notification must be subscribed to. Check if the user has already subscribed
	 * to avoid resubscribing and potentially altering the configured notification threshold.
	 */
	if (!link_shell_msleep_notifications_subscribed) {
		link_modem_sleep_notifications_subscribe(
			CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS,
			CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS);
	}

	mosh_print("during_sleep: disabling UARTs during the modem sleep mode");

	/* Setting the flag to indicate that UARTs are requested to be disabled should the
	 * modem enter sleep mode.
	 */
	uart_shell_disable_during_sleep_requested = true;
	return 0;
}

static int cmd_uart_enable_when_sleep(void)
{
	/* If the sleep notification subscription was triggered by 'uart disable during_sleep' and
	 * not explicitly by the user, unsubscribe from notification if user re-enables UARTs.
	 */
	if (!link_shell_msleep_notifications_subscribed) {
		link_modem_sleep_notifications_unsubscribe();
	}

	mosh_print("during_sleep: enabling UARTs during the modem sleep mode");

	/* Reset the flag, no dot disable UARTs when the modem enters sleep mode. */
	uart_shell_disable_during_sleep_requested = false;
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_uart_during_sleep,
	SHELL_CMD_ARG(enable, NULL, "Enable UARTs during sleep mode.",
		      cmd_uart_enable_when_sleep, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable UARTs during sleep mode.",
		      cmd_uart_disable_when_sleep, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_uart,
	SHELL_CMD_ARG(
		disable,
		NULL,
		"<time in seconds>\nDisable UARTs for a given number of seconds. 0 means that "
		"UARTs remain disabled indefinitely.",
		cmd_uart_disable,
		2,
		0),
	SHELL_CMD(
		during_sleep,
		&sub_uart_during_sleep,
		"Disable UARTs during the modem sleep mode. UARTs are re-enabled once the modem "
		"exits sleep mode.",
		mosh_print_help_shell),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(
	uart,
	&sub_uart,
	"Commands for disabling UARTs for power measurement.",
	mosh_print_help_shell);
