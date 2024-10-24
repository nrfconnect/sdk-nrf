/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/wifi_mgmt_ext.h>
#include <net/wifi_credentials.h>

#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>


static int cmd_auto_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (rc) {
		shell_error(shell,
			    "An error occurred when trying to auto-connect to a network. err: %d",
			    rc);
	}
	return 0;
}

SHELL_SUBCMD_ADD((nwifi_cred),
		 auto_connect, NULL, "Connect to any stored network.", cmd_auto_connect, 0, 0);
