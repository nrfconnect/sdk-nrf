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

extern struct wpa_supplicant *wpa_s_0;

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
static int __wifi_args_to_params(size_t argc,
				 const char *argv[],
				 struct wifi_connect_req_params *params)
{
	char *endptr;
	int idx = 1;

	if (argc < 1) {
		return -EINVAL;
	}

	/* SSID */
	params->ssid = (char *)argv[0];
	params->ssid_length = strlen(params->ssid);

	/* PSK (optional) */
	if (idx < argc) {
		params->psk = (char *)argv[idx];
		params->psk_length = strlen(argv[idx]);
		params->security = WIFI_SECURITY_TYPE_PSK;
		idx++;

		if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
			params->security = strtol(argv[idx], &endptr, 10);
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	return 0;
}


static int cmd_supplicant_connect(const struct shell *shell,
				  size_t argc,
				  const char *argv[])
{
	static struct wifi_connect_req_params cnx_params;
	struct wifi_connect_req_params *params;
	struct wpa_ssid *ssid = NULL;
	bool pmf = true;

	if (__wifi_args_to_params(argc - 1,
				  &argv[1],
				  &cnx_params)) {
		shell_help(shell);
		return -ENOEXEC;
	}

	params = &cnx_params;

	if (!wpa_s_0) {
		shell_fprintf(context.shell,
			      SHELL_ERROR,
			      "%s: wpa_supplicant is not initialized, dropping connect\n",
			      __func__);
		return -1;
	}

	ssid = wpa_supplicant_add_network(wpa_s_0);
	ssid->ssid = os_zalloc(sizeof(u8) * MAX_SSID_LEN);

	memcpy(ssid->ssid, params->ssid, params->ssid_length);
	ssid->ssid_len = params->ssid_length;
	ssid->disabled = 1;
	ssid->key_mgmt = WPA_KEY_MGMT_NONE;

	wpa_s_0->conf->filter_ssids = 1;
	wpa_s_0->conf->ap_scan = 1;

	if (params->psk) {
		// TODO: Extend enum wifi_security_type
		if (params->security == 3) {
			ssid->key_mgmt = WPA_KEY_MGMT_SAE;
			str_clear_free(ssid->sae_password);
			ssid->sae_password = dup_binstr(params->psk, params->psk_length);

			if (ssid->sae_password == NULL) {
				shell_fprintf(context.shell,
					      SHELL_ERROR,
					      "%s:Failed to copy sae_password\n",
					      __func__);
				return -1;
			}
		} else {
			if (params->security == 2)
				ssid->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
			else
				ssid->key_mgmt = WPA_KEY_MGMT_PSK;

			str_clear_free(ssid->passphrase);
			ssid->passphrase = dup_binstr(params->psk, params->psk_length);

			if (ssid->passphrase == NULL) {
				shell_fprintf(context.shell,
					      SHELL_ERROR,
					      "%s:Failed to copy passphrase\n",
					      __func__);
				return -1;
			}
		}

		wpa_config_update_psk(ssid);

		if (pmf)
			ssid->ieee80211w = 1;

	}

	wpa_supplicant_enable_network(wpa_s_0,
				      ssid);

	wpa_supplicant_select_network(wpa_s_0,
				      ssid);

	return 0;
}


static int cmd_supplicant(const struct shell *shell,
			  size_t argc,
			  const char *argv[])
{
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
	SHELL_CMD(connect,
		  NULL,
		  " \"<SSID>\""
		  "\nPassphrase (optional: valid only for secured SSIDs)>"
		  "\nKEY_MGMT (optional: 0-None, 1-WPA2, 2-WPA2-256, 3-WPA3)",
		  cmd_supplicant_connect),
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
		   "Wi-Fi commands",
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
