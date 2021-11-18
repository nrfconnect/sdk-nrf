/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <shell/shell.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>
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

AT_MONITOR(mosh_at_handler, ANY, at_cmd_handler, PAUSED);

static void at_cmd_handler(const char *response)
{
	mosh_print("AT event handler: %s", response);
}

static void at_cmd_handler_old(void *context, const char *response)
{
	mosh_print("Old AT event handler: %s", response);
}

int at_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	char response[MOSH_AT_CMD_RESPONSE_MAX_LEN + 1];

	if (argc < 2) {
		mosh_print_no_format(at_usage_str);
		return 0;
	}

	char *command = argv[1];

	if (!strcmp(command, "events_enable")) {
		at_monitor_resume(mosh_at_handler);
		/* TODO: at_notif should be removed once all used libraries use at_monitor */
		at_notif_register_handler((void *)shell, at_cmd_handler_old);
		mosh_print("AT command events enabled");
	} else if (!strcmp(command, "events_disable")) {
		at_monitor_pause(mosh_at_handler);
		/* TODO: at_notif should be removed once all used libraries use at_monitor */
		at_notif_deregister_handler((void *)shell, at_cmd_handler_old);
		mosh_print("AT command events disabled");
	} else {
		err = nrf_modem_at_cmd(response, sizeof(response), "%s", command);
		if (err == 0) {
			mosh_print("%s", response);
		} else if (err > 0) {
			mosh_error("%s", response);
			err = -EINVAL;
		} else {
			/* Negative values are error codes */
			mosh_error("Failed to send AT command, err %d", err);
		}
	}

	return 0;
}
