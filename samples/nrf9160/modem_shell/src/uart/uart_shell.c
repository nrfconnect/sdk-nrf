/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <device.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <modem/lte_lc.h>

#include "uart_shell.h"
#include "uart.h"

bool uart_disable_during_sleep_requested;

static int print_help(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		shell_error(shell, "%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}

static int cmd_uart_disable(const struct shell *shell, size_t argc, char **argv)
{
	int sleep_time;

	sleep_time = atoi(argv[1]);
	if (sleep_time < 0) {
		shell_error(shell, "disable: invalid sleep time");
		return -EINVAL;
	}

	if (sleep_time > 0) {
		shell_print(shell, "disable: disabling UARTs for %d seconds", sleep_time);
	} else {
		shell_print(shell, "disable: disabling UARTs indefinitely");
	}

	k_sleep(K_MSEC(500)); /* allow little time for printing the notification */
	disable_uarts();

	if (sleep_time > 0) {
		k_sleep(K_SECONDS(sleep_time));

		enable_uarts();

		shell_print(shell, "disable: UARTs enabled");
	}

	return 0;
}

void uart_toggle_power_state_at_event(const struct shell *shell, const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_MODEM_SLEEP_ENTER) {
		shell_print(shell, "Modem sleep enter: disabling UARTs requested");
		k_sleep(K_MSEC(500)); /* allow little time for printing the notification */
		disable_uarts();
	} else if (evt->type == LTE_LC_EVT_MODEM_SLEEP_EXIT) {
		enable_uarts();
		shell_print(shell, "Modem sleep exit: re-enabling UARTs");
	}
}

static int cmd_uart_disable_when_sleep(const struct shell *shell)
{
	shell_print(shell, "during_sleep: disabling UARTs during the modem sleep mode");

	/* Setting the flag to indicate that UARTs are requested to be disabled should the
	 * modem enter sleep mode.
	 */
	uart_disable_during_sleep_requested = true;
	return 0;
}

static int cmd_uart_enable_when_sleep(const struct shell *shell)
{
	shell_print(shell, "during_sleep: enabling UARTs during the modem sleep mode");

	/* Reset the flag, no dot disable UARTs when the modem enters sleep mode. */
	uart_disable_during_sleep_requested = false;
	return 0;
}

static int cmd_uart_during_sleep(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
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
		"exits sleep mode. Modem sleep notifications need to be subscribed to "
		"using 'link msleep --subscribe'.",
		cmd_uart_during_sleep),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(uart, &sub_uart, "Commands for disabling UARTs for power measurement.", NULL);
