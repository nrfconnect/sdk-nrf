/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>

#include <shell/shell.h>

#include "ppp_ctrl.h"
#include "ppp_shell.h"

#include <net/promiscuous.h>

enum ppp_shell_command {
	PPP_CMD_UP = 0,
	PPP_CMD_DOWN
};

struct ppp_shell_cmd_args_t {
	enum ppp_shell_command command;
};

static struct ppp_shell_cmd_args_t ppp_cmd_args;

static const char ppp_cmd_usage_str[] =
	"Usage: ppp <sub-command>\n"
	"\n"
	"where <command> is one of the following:\n"
	"  help:                    Show this message\n"
	"  up:                      Set PPP net_if up\n"
	"  down:                    Set PPP net_if down\n"
	"\n";

static void ppp_shell_print_usage(const struct shell *shell)
{
	shell_print(shell, "%s", ppp_cmd_usage_str);
}

static void ppp_shell_cmd_defaults_set(struct ppp_shell_cmd_args_t *ppp_cmd_args)
{
	memset(ppp_cmd_args, 0, sizeof(struct ppp_shell_cmd_args_t));
}

int ppp_shell_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ppp_shell_cmd_defaults_set(&ppp_cmd_args);

	if (argc < 2) {
		goto show_usage;
	}

	if (strcmp(argv[1], "up") == 0) {
		ppp_cmd_args.command = PPP_CMD_UP;
	} else if (strcmp(argv[1], "down") == 0) {
		ppp_cmd_args.command = PPP_CMD_DOWN;
	} else if (strcmp(argv[1], "help") == 0) {
		goto show_usage;
	} else {
		shell_error(shell, "Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	switch (ppp_cmd_args.command) {
	case PPP_CMD_UP:
		ret = ppp_ctrl_start(shell);
		if (ret >= 0) {
			shell_print(shell, "PPP net if up.\n");
		} else {
			shell_error(shell, "PPP net if cannot be started\n");
		}

		break;
	case PPP_CMD_DOWN:
		ppp_ctrl_stop();
		break;
	}

	return 0;

show_usage:
	ppp_shell_print_usage(shell);
	return 0;
}
