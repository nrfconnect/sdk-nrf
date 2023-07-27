/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief Wi-Fi shell sample
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include <src/utils/common.h>
#include <wpa_supplicant/config.h>
#include <wpa_supplicant/wpa_supplicant_i.h>

static struct {
	const struct shell *shell;

	union {
		struct {
			uint8_t connecting : 1;
			uint8_t disconnecting : 1;
			uint8_t _unused : 6;
		};
		uint8_t all;
	};
} context;


int cli_main(int argc, const char **argv);

extern struct wpa_global *global;

/* TODO: Take this an input from shell */
const char if_name[] = "wlan0";


static int cmd_supplicant(const struct shell *shell,
			  size_t argc,
			  const char *argv[])
{
	struct wpa_supplicant *wpa_s = wpa_supplicant_get_iface(global, if_name);

	if (!wpa_s) {
		shell_fprintf(shell,
			SHELL_ERROR,
			"%s: wpa_supplicant is not initialized, dropping connect\n",
			__func__);
		return -1;
	}

	return cli_main(argc,
			argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	wpa_cli_cmds,
	SHELL_CMD(add_network,
		  NULL,
		  "\"Add Network network id/number\"",
		  cmd_supplicant),
	SHELL_CMD(set_network,
		  NULL,
		  "\"Set Network params\"",
		  cmd_supplicant),
	SHELL_CMD(set,
		  NULL,
		  "\"Set Global params\"",
		  cmd_supplicant),
	SHELL_CMD(get,
		  NULL, "\"Get Global params\"",
		  cmd_supplicant),
	SHELL_CMD(enable_network,
		  NULL,
		  "\"Enable Network\"",
		  cmd_supplicant),
	SHELL_CMD(remove_network,
		  NULL,
		  "\"Remove Network\"",
		  cmd_supplicant),
	SHELL_CMD(get_network,
		  NULL,
		  "\"Get Network\"",
		  cmd_supplicant),
	SHELL_CMD(select_network,
		  NULL,
		  "\"Select Network which will be enabled; rest of the networks will be disabled\"",
		  cmd_supplicant),
	SHELL_CMD(disable_network,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(disconnect,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(reassociate,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(status,
		  NULL,
		  "\"Get client status\"",
		  cmd_supplicant),
	SHELL_CMD(bssid,
		  NULL,
		  "\"Associate with this BSSID\"",
		  cmd_supplicant),
	SHELL_CMD(sta_autoconnect,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(signal_poll,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(wnm_bss_query,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_CMD(list_networks,
		  NULL,
		  "\"\"",
		  cmd_supplicant),
	SHELL_SUBCMD_SET_END);

/* Persisting with "wpa_cli" naming for compatibility with Wi-Fi
 * certification applications and scripts.
 */
SHELL_CMD_REGISTER(wpa_cli,
		   &wpa_cli_cmds,
		   "wpa_supplicant commands (only for internal use)",
		   NULL);


static int wifi_shell_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	context.shell = NULL;
	context.all = 0U;

	return 0;
}


SYS_INIT(wifi_shell_init,
	 APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
