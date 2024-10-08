/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include "desh_print.h"

extern bool desh_print_timestamp_use;

static int cmd_timestamps_enable(const struct shell *shell, size_t argc, char **argv)
{
	desh_print_timestamp_use = true;
	desh_print("Timestamps enabled");
	return 0;
}

static int cmd_timestamps_disable(const struct shell *shell, size_t argc, char **argv)
{
	desh_print_timestamp_use = false;
	desh_print("Timestamps disabled");
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
		"Enable/disable timestamps in shell output.", desh_print_help_shell),
SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(
	print,
	&sub_print,
	"Commands for shell output formatting.",
	desh_print_help_shell);
