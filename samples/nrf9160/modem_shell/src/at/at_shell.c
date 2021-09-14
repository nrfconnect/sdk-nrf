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
#include "mosh_print.h"

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
	mosh_print("AT event handler: %s", response);
}

static void at_print_usage(void)
{
	mosh_print_no_format(at_usage_str);
}

static void at_print_error_info(enum at_cmd_state state, int error)
{
	switch (state) {
	case AT_CMD_ERROR:
		mosh_error("ERROR: %d", error);
		break;
	case AT_CMD_ERROR_CMS:
		mosh_error("CMS ERROR: %d", error);
		break;
	case AT_CMD_ERROR_CME:
		mosh_error("CME ERROR: %d", error);
		break;
	case AT_CMD_ERROR_QUEUE:
		mosh_error("QUEUE ERROR: %d", error);
		break;
	case AT_CMD_ERROR_WRITE:
		mosh_error("AT CMD SOCKET WRITE ERROR: %d", error);
		break;
	case AT_CMD_ERROR_READ:
		mosh_error("AT CMD SOCKET READ ERROR: %d", error);
		break;
	case AT_CMD_NOTIFICATION:
		mosh_error("AT CMD NOTIFICATION: %d", error);
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

	if (argc < 2) {
		at_print_usage();
		return 0;
	}

	char *command = argv[1];

	if (!strcmp(command, "events_enable")) {
		int err = at_notif_register_handler((void *)shell,
						    at_cmd_handler);
		if (err == 0) {
			mosh_print("AT command event handler registered successfully");
		} else {
			mosh_print("AT command event handler registeration failed, err=%d", err);
		}
	} else if (!strcmp(command, "events_disable")) {
		at_notif_deregister_handler((void *)shell, at_cmd_handler);
	} else {
		err = at_cmd_write(command, response, sizeof(response), &state);
		if (state != AT_CMD_OK) {
			at_print_error_info(state, err);
			return -EINVAL;
		}

		mosh_print("%sOK", response);
	}

	return 0;
}
