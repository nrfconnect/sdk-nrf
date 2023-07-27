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

#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>
#include <zephyr_fmac_main.h>

#ifdef CONFIG_WPA_SUPP
#include <src/utils/common.h>
#include <wpa_supplicant/config.h>
#include <wpa_supplicant/wpa_supplicant_i.h>
#endif /* CONFIG_WPA_SUPP */

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

static uint32_t scan_result;

int cli_main(int argc, const char **argv);

extern struct wpa_global *global;

/* TODO: Take this an input from shell */
const char if_name[] = "wlan0";

struct wpa_ssid *ssid_0;

#define MAX_SSID_LEN 32


static void scan_result_cb(struct net_if *iface,
			   int status,
			   struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		if (status) {
			shell_fprintf(context.shell,
				      SHELL_WARNING,
				      "Scan request failed (%d)\n",
				      status);
		} else {
			shell_fprintf(context.shell,
				      SHELL_NORMAL,
				      "Scan request done\n");
		}

		return;
	}

	scan_result++;

	if (scan_result == 1U) {
		shell_fprintf(context.shell,
			      SHELL_NORMAL,
			      "\n%-4s | %-32s %-5s | %-4s | %-4s | %-5s\n", "Num", "SSID",
			      "(len)", "Chan", "RSSI", "Sec");
	}

	shell_fprintf(context.shell,
		      SHELL_NORMAL,
		      "%-4d | %-32s %-5u | %-4u | %-4d | %-5s\n",
		      scan_result,
		      entry->ssid,
		      entry->ssid_length,
		      entry->channel,
		      entry->rssi,
		      (entry->security == WIFI_SECURITY_TYPE_PSK ? "WPA/WPA2" : "Open"));
}


static int cmd_wifi_scan(const struct shell *shell,
			       size_t argc,
			       const char *argv[])
{
	struct net_if *iface = net_if_get_default();
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_nrf_dev_ops *dev_ops = dev->api;

	context.shell = shell;

	return dev_ops->off_api.disp_scan(dev,
					  scan_result_cb);
}


#ifdef CONFIG_WPA_SUPP
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
#endif /* CONFIG_WPA_SUPP */


SHELL_STATIC_SUBCMD_SET_CREATE(
	wpa_cli_cmds,
	SHELL_CMD(scan,
		  NULL,
		  "Scan AP",
		  cmd_wifi_scan),
#ifdef CONFIG_WPA_SUPP
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
#endif /* CONFIG_WPA_SUPP */
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
	scan_result = 0U;

	return 0;
}


SYS_INIT(wifi_shell_init,
	 APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
