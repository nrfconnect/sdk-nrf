/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/shell/shell.h>

#include "desh_print.h"

#include "auto_connect.h"

static int auto_connect_shell_enable(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = auto_connect_sett_enable_disable(true);

	if (err) {
		desh_error("Failed to enable auto connect");
		return err;
	}

	desh_print("Auto connect enabled");
	return 0;
}
static int auto_connect_shell_disable(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int err = auto_connect_sett_enable_disable(false);

	if (err) {
		desh_error("Failed to disable auto connect");
		return err;
	}

	desh_print("Auto connect disabled");
	return 0;
}

static int auto_connect_shell_sett_read(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	bool enabled = auto_connect_sett_is_enabled();
	int delay = auto_connect_sett_delay_get();

	desh_print("Auto connect settings:");
	desh_print("  Enabled: %s", enabled ? "true" : "false");
	if (delay == AUTO_CONNECT_DELAY_USE_DEFAULT_TRIGGER) {
		desh_print("  Delay:   %d seconds (default: trigger on "
			   "NET_EVENT_L4_CONNECTED when available)",
			   AUTO_CONNECT_DELAY_USE_DEFAULT_TRIGGER);
	} else {
		desh_print("  Delay:   %d seconds (after NET_EVENT_DECT_ACTIVATE_DONE)",
			   delay);
	}

	return 0;
}

static int auto_connect_shell_delay(const struct shell *shell, size_t argc, char **argv)
{
	int delay;
	int err;

	if (argc != 2) {
		desh_error("Usage: auto_connect delay <seconds>");
		return -EINVAL;
	}

	delay = atoi(argv[1]);
	err = auto_connect_sett_delay_set(delay);
	if (err) {
		desh_error("Failed to set auto connect delay: %d", err);
		return err;
	}

	if (delay == AUTO_CONNECT_DELAY_USE_DEFAULT_TRIGGER) {
		desh_print("Auto connect delay cleared; "
			   "default trigger (NET_EVENT_L4_CONNECTED) restored");
	} else {
		desh_print("Auto connect delay set to %d seconds (overrides L4-event trigger)",
			   delay);
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_auto_connect_shell,
	SHELL_CMD_ARG(
		enable, NULL,
		"DeSh specific settings to enable auto connect after modem activation.\n"
		"Default: off.\n",
		auto_connect_shell_enable, 1, 0),
	SHELL_CMD(disable, NULL, "Setting to disable auto connect.\n", auto_connect_shell_disable),
	SHELL_CMD_ARG(
		delay, NULL,
		"Set auto connect delay in seconds.\n"
		"  <seconds> = 0 (default): trigger on NET_EVENT_L4_CONNECTED.\n"
		"  <seconds> > 0:           schedule that many seconds after\n"
		"                           NET_EVENT_DECT_ACTIVATE_DONE\n"
		"                           (overrides L4 trigger).\n",
		auto_connect_shell_delay, 2, 0),
	SHELL_CMD(sett_read, NULL, "Read auto connect settings\n", auto_connect_shell_sett_read),

	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(auto_connect, &sub_auto_connect_shell,
		   "Enable and disable auto-connect feature for DECT NR+ Shell.",
		   desh_print_help_shell);
