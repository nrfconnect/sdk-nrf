/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include <net/wifi_credentials.h>
#include <net/wifi_mgmt_ext.h>

#define MACSTR "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

static void print_network_info_helper(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds = { 0 };

	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);
	if (ret) {
		printk("an error occurred when trying to load credentials for network \"%.*s\""
			". err: %d", ssid_len, ssid, ret);
		return;
	}
	printk("  network ssid: \"%.*s\", ssid_len: %d, type: %s",
		ssid_len, ssid, ssid_len,
		wifi_security_txt(creds.header.type));


	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE) {
		printk(", password: \"%.*s\", password_len: %d",
		       creds.password_len, creds.password, creds.password_len);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		printk(", bssid: " MACSTR,
		       creds.header.bssid[0], creds.header.bssid[1],
		       creds.header.bssid[2], creds.header.bssid[3],
		       creds.header.bssid[4], creds.header.bssid[5]);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		printk(", band: 2.4GHz");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		printk(", band: 5GHz");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		printk(", favorite");
	}
	printk("\n");
}

static enum wifi_security_type parse_sec_type(const char *s)
{
	if (strcmp("OPEN", s) == 0) {
		return WIFI_SECURITY_TYPE_NONE;
	}
	if (strcmp("WPA2-PSK", s) == 0) {
		return WIFI_SECURITY_TYPE_PSK;
	}
	if (strcmp("WPA2-PSK-SHA256", s) == 0) {
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	}
	if (strcmp("WPA3-SAE", s) == 0) {
		return WIFI_SECURITY_TYPE_SAE;
	}
	return WIFI_SECURITY_TYPE_UNKNOWN;
}

static enum wifi_frequency_bands parse_band(const char *s)
{
	if (strcmp("2.4GHz", s) == 0) {
		return WIFI_FREQ_BAND_2_4_GHZ;
	}
	if (strcmp("5GHz", s) == 0) {
		return WIFI_FREQ_BAND_5_GHZ;
	}
	if (strcmp("6GHz", s) == 0) {
		return WIFI_FREQ_BAND_6_GHZ;
	}
	return WIFI_FREQ_BAND_UNKNOWN;
}

static int cmd_auto_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (rc) {
		printk("an error occurred when trying to auto-connect to a network. err: %d", rc);
	}
	return 0;
}

static int cmd_add_network(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;

	if (argc < 3) {
		goto help;
	}

	struct wifi_credentials_personal creds = {
		.header.ssid_len = strlen(argv[1]),
		.header.type = parse_sec_type(argv[2]),
	};

	memcpy(creds.header.ssid, argv[1], creds.header.ssid_len);

	if (creds.header.type == WIFI_SECURITY_TYPE_UNKNOWN) {
		printk("cannot parse security type!\n");
		goto help;
	}

	size_t arg_idx = 3;

	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE) {
		/* parse passphrase */
		if (argc < 4) {
			printk("missing password\n");
			goto help;
		}
		creds.password_len = strlen(argv[3]);
		if (creds.password_len > CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH ||
			(
				(creds.header.type == WIFI_SECURITY_TYPE_PSK ||
				 creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256) &&
				creds.password_len > WIFI_PSK_MAX_LEN
			)
		) {
			printk("psk is too long for this security type!\n");
			goto help;
		}
		memcpy(creds.password, argv[3], creds.password_len);
		++arg_idx;
	}

	if (arg_idx < argc) {
		/* look for bssid */
		ret = sscanf(argv[arg_idx], MACSTR,
			     &creds.header.bssid[0], &creds.header.bssid[1],
			     &creds.header.bssid[2], &creds.header.bssid[3],
			     &creds.header.bssid[4], &creds.header.bssid[5]);
		if (ret == 6) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_BSSID;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for band */
		enum wifi_frequency_bands band = parse_band(argv[arg_idx]);

		if (band == WIFI_FREQ_BAND_2_4_GHZ) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
			++arg_idx;
		}
		if (band == WIFI_FREQ_BAND_5_GHZ) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_5GHz;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for favorite flag */
		if (strcmp("favorite", argv[arg_idx]) == 0) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_FAVORITE;
			++arg_idx;
		}
	}

	if (arg_idx != argc) {
		for (size_t i = arg_idx; i < argc; ++i) {
			printk("unparsed arg: [%s]\n", argv[i]);
		}
	}

	return wifi_credentials_set_personal_struct(&creds);
help:
	printk("usage: wifi_cred add \"network name\""
		       " {OPEN, WPA2-PSK, WPA2-PSK-SHA256, WPA3-SAE}"
		       " [psk]"
		       " [bssid]"
		       " [{2.4GHz, 5GHz}]"
		       " [favorite]\n");
	return -EINVAL;
}

static int cmd_delete_network(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc != 2) {
		printk("usage: wifi_cred delete \"network name\"\n");
		return -EINVAL;
	}
	printk("  deleting network ssid: \"%s\", ssid_len: %d\n", argv[1], strlen(argv[1]));
	return wifi_credentials_delete_by_ssid(argv[1], strlen(argv[1]));
}

static int cmd_list_networks(const struct shell *sh, size_t argc, char *argv[])
{
	wifi_credentials_for_each_ssid(print_network_info_helper, NULL);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_wifi_cred,
	SHELL_CMD(auto_connect,	NULL, "Connect to any stored network.",	cmd_auto_connect),
	SHELL_CMD(add,		NULL, "Add network to storage.",	cmd_add_network),
	SHELL_CMD(delete,	NULL, "Delete network from storage.",	cmd_delete_network),
	SHELL_CMD(list,		NULL, "List stored networks.",		cmd_list_networks),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi_cred, &sub_wifi_cred, "Wi-Fi Credentials commands", NULL);
