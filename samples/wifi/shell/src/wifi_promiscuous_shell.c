/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Promiscuous mode shell
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(promiscuous_mode, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"

static int cmd_wifi_promisc(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = NULL;
	bool mode_val = 0;

	if (argc != 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "-h") == 0) {
		shell_help(sh);
		return 0;
	} else if ((strcmp(argv[1], "1") == 0) || (strcmp(argv[1], "0") == 0)) {
		mode_val = atoi(argv[1]);
	} else {
		LOG_ERR("Entered wrong input");
		shell_help(sh);
		return -ENOEXEC;
	}

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -ENOEXEC;
	}

	if (net_eth_promisc_mode(iface, mode_val)) {
		LOG_ERR("Promiscuous mode %s failed", mode_val ? "enabling" : "disabling");
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	promisc_cmd,
	SHELL_CMD_ARG(mode, NULL,
		      "This command may be used to enable or disable promiscuous mode\n"
		      "[enable: 1, disable: 0]\n"
		      "[-h, --help] : Print out the help for the mode command\n"
		      "Usage: promiscuous_set mode 1 or 0\n",
		      cmd_wifi_promisc,
		      2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(promiscuous_set,
		   &promisc_cmd,
		   "Promiscuous mode command (To set promiscuous mode)",
		   NULL);
