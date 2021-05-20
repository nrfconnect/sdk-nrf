/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <shell/shell.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

#include "mosh_defines.h"

extern const struct shell *shell_global;

static const char at_usage_str[] =
	"Usage: at <subcommand>\n"
	"\n"
	"Subcommands:\n"
	"  events_enable     Enable AT event handler which prints AT notifications\n"
	"  events_disable    Disable AT event handler\n"
	"\n"
	"Any other subcommand is interpreted as AT command and sent to the modem.\n";

static void at_cmd_handler(void *context, const char *response)
{
	shell_print(shell_global, "AT event handler: %s", response);
}

static void at_print_usage(void)
{
	shell_print(shell_global, "%s", at_usage_str, "at", "at");
}

static void at_print_error_info(enum at_cmd_state state, int error)
{
	switch (state) {
	case AT_CMD_ERROR:
		shell_error(shell_global, "ERROR: %d", error);
		break;
	case AT_CMD_ERROR_CMS:
		shell_error(shell_global, "CMS ERROR: %d", error);
		break;
	case AT_CMD_ERROR_CME:
		shell_error(shell_global, "CME ERROR: %d", error);
		break;
	case AT_CMD_ERROR_QUEUE:
		shell_error(shell_global, "QUEUE ERROR: %d", error);
		break;
	case AT_CMD_ERROR_WRITE:
		shell_error(shell_global, "AT CMD SOCKET WRITE ERROR: %d", error);
		break;
	case AT_CMD_ERROR_READ:
		shell_error(shell_global, "AT CMD SOCKET READ ERROR: %d", error);
		break;
	case AT_CMD_NOTIFICATION:
		shell_error(shell_global, "AT CMD NOTIFICATION: %d", error);
		break;
	default:
		break;
	}
}

int at_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	char response[MOSH_AT_CMD_RESPONSE_MAX_LEN + 1];
	enum at_cmd_state state = AT_CMD_OK;

	shell_global = shell;

	if (argc < 2) {
		at_print_usage();
		return 0;
	}

	char *command = argv[1];

	if (!strcmp(command, "events_enable")) {
		int err = at_notif_register_handler((void *)shell,
						    at_cmd_handler);
		if (err == 0) {
			shell_print(shell, "AT command event handler registered successfully");
		} else {
			shell_print(shell,
				    "AT command event handler registeration failed, err=%d",
				    err);
		}
	} else if (!strcmp(command, "events_disable")) {
		at_notif_deregister_handler((void *)shell, at_cmd_handler);
	} else {
		err = at_cmd_write(command, response, sizeof(response), &state);
		if (state != AT_CMD_OK) {
			at_print_error_info(state, err);
			return -EINVAL;
		}

		shell_print(shell, "%sOK", response);
	}

	return 0;
}
