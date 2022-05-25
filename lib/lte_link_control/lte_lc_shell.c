/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <modem/lte_lc.h>

static int cmd_normal(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = lte_lc_normal();

	if (ret) {
		shell_error(shell, "Error: lte_lc_normal() returned %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_power_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = lte_lc_power_off();

	if (ret) {
		shell_error(shell, "Error: lte_lc_power_off() returned %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_offline(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = lte_lc_offline();

	if (ret) {
		shell_error(shell, "Error: lte_lc_offline() returned %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lte,
	SHELL_CMD(normal, NULL, "Send the modem to normal mode", cmd_normal),
	SHELL_CMD(offline, NULL, "Send the modem to offline mode", cmd_offline),
	SHELL_CMD(power_off, NULL, "Send the modem to power off mode", cmd_power_off),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(lte, &sub_lte, "LTE Link control", NULL);
