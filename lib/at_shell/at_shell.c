/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>

static struct shell *global_shell;
static const char at_usage_str[] =
	"Usage: at <subcommand>\n"
	"\n"
	"Subcommands:\n"
	"  events_enable     Enable AT event handler which prints AT notifications\n"
	"  events_disable    Disable AT event handler\n"
	"\n"
	"Any other subcommand is interpreted as AT command and sent to the modem.\n";

AT_MONITOR(at_shell_monitor, ANY, at_notif_handler, PAUSED);

static void at_notif_handler(const char *response)
{
	/* The AT monitor is initialized in PAUSED state, which means that `at events_enable`
	 * must be run first, at which point `global_shell` will be set to `shell`.
	 * The assert below is added to catch the case where the monitor is initialized to
	 * ACTIVE and no shell command has been invoked before the first AT notification arrives.
	 */
	__ASSERT(global_shell != NULL, "global_shell has not been set to a valid shell");
	shell_print(global_shell, "AT event handler:\n%s", response);
}

int at_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static char response[CONFIG_AT_SHELL_CMD_RESPONSE_MAX_LEN + 1];

	global_shell = (struct shell *)shell;

	if (argc < 2) {
		shell_print(shell, "%s", at_usage_str);
		return 0;
	}

	char *command = argv[1];

	if (!strcmp(command, "events_enable")) {
		at_monitor_resume(&at_shell_monitor);
		shell_print(shell, "AT command event handler enabled");
	} else if (!strcmp(command, "events_disable")) {
		at_monitor_pause(&at_shell_monitor);
		shell_print(shell, "AT command event handler disabled");
	} else {
		err = nrf_modem_at_cmd(response, sizeof(response), "%s", command);
		if (err < 0) {
			shell_print(shell, "Sending AT command failed with error code %d", err);
			return err;
		} else if (err) {
			shell_print(shell, "%s", response);
			return -EINVAL;
		}

		shell_print(shell, "%s", response);
	}

	return 0;
}

SHELL_CMD_REGISTER(at, NULL, "Execute an AT command", at_shell);
