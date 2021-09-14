/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <shell/shell.h>

#include "mosh_print.h"

extern bool mosh_print_timestamp_use;

static int print_help(const struct shell *shell, size_t argc, char **argv)
{
	shell_help(shell);

	return 1;
}

static int cmd_timestamps_enable(const struct shell *shell, size_t argc, char **argv)
{
	mosh_print_timestamp_use = true;
	mosh_print("Timestamps enabled");
	return 0;
}

static int cmd_timestamps_disable(const struct shell *shell, size_t argc, char **argv)
{
	mosh_print_timestamp_use = false;
	mosh_print("Timestamps disabled");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_timestamps,
	SHELL_CMD(enable, NULL, "Enable timestamps in shell output.", cmd_timestamps_enable),
	SHELL_CMD(disable, NULL, "Disable timestamps in shell output.", cmd_timestamps_disable),
SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_print,
	SHELL_CMD(
		timestamps, &sub_timestamps,
		"Enable/disable timestamps in shell output.", print_help),
SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(print, &sub_print, "Commands for shell output formatting.", print_help);
