/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include "mosh_print.h"

extern bool mosh_print_timestamp_use;
#if defined(CONFIG_MOSH_CLOUD_MQTT)
extern bool mosh_print_cloud_echo;
#endif

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

#if defined(CONFIG_MOSH_CLOUD_MQTT)
static int cmd_cloud_echo_enable(const struct shell *shell, size_t argc, char **argv)
{
	mosh_print_cloud_echo = true;
	mosh_print("Echoing shell prints to cloud enabled");
	return 0;
}

static int cmd_cloud_echo_disable(const struct shell *shell, size_t argc, char **argv)
{
	mosh_print_cloud_echo = false;
	mosh_print("Echoing shell prints to cloud disabled");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_cloud_echo,
	SHELL_CMD(enable, NULL, "Enable shell output to cloud.", cmd_cloud_echo_enable),
	SHELL_CMD(disable, NULL, "Disable shell output to cloud.", cmd_cloud_echo_disable),
SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_timestamps,
	SHELL_CMD(enable, NULL, "Enable timestamps in shell output.", cmd_timestamps_enable),
	SHELL_CMD(disable, NULL, "Disable timestamps in shell output.", cmd_timestamps_disable),
SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_print,
	SHELL_CMD(
		timestamps, &sub_timestamps,
		"Enable/disable timestamps in shell output.", mosh_print_help_shell),
#if defined(CONFIG_MOSH_CLOUD_MQTT)
	SHELL_CMD(
		cloud, &sub_cloud_echo,
		"Enable/disable echoing shell output to cloud over MQTT.", mosh_print_help_shell),
#endif
SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(
	print,
	&sub_print,
	"Commands for shell output formatting.",
	mosh_print_help_shell);
