/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/ethernet.h>

#include <net/wifi_credentials.h>
#include <net/wifi_mgmt_ext.h>

#define MAX_BANDS_STR_LEN 64

/* Ensure 'strnlen' is available even with -std=c99. */
size_t strnlen(const char *buf, size_t bufsz);

#define MACSTR "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

static void print_network_info(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds = {0};
	const struct shell *shell = (const struct shell *)cb_arg;

	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);
	if (ret) {
		shell_error(shell,
			    "An error occurred when trying to load credentials for network \"%.*s\""
			    ". err: %d",
			    ssid_len, ssid, ret);
		return;
	}
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "  network ssid: \"%.*s\", ssid_len: %d, type: %s", ssid_len, ssid, ssid_len,
		      wifi_security_txt(creds.header.type));

	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE ||
	    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
			      ", password: \"%.*s\", password_len: %d", creds.password_len,
			      creds.password, creds.password_len);
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	if (creds.header.type == WIFI_SECURITY_TYPE_EAP_TLS) {
		if (creds.header.key_passwd_length > 0) {
			shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
				      ", key_passwd: \"%.*s\", key_passwd_len: %d",
				      creds.header.key_passwd_length, creds.header.key_passwd,
				      creds.header.key_passwd_length);
		}
		if (creds.header.aid_length > 0) {
			shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
				      ", anon_id: \"%.*s\", anon_id_len: %d",
				      creds.header.aid_length, creds.header.anon_id,
				      creds.header.aid_length);
		}
	}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", bssid: " MACSTR,
			      creds.header.bssid[0], creds.header.bssid[1], creds.header.bssid[2],
			      creds.header.bssid[3], creds.header.bssid[4], creds.header.bssid[5]);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", band: 2.4GHz");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", band: 5GHz");
	}

	if (creds.header.channel) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", channel: %d",
			      creds.header.channel);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", favorite");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_REQUIRED) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", MFP_REQUIRED");
	} else if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_DISABLED) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", MFP_DISABLED");
	} else {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", MFP_OPTIONAL");
	}

	if (creds.header.timeout) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, ", timeout: %d",
			      creds.header.timeout);
	}

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "\n");
}

static int cmd_add_network(const struct shell *shell, size_t argc, char *argv[])
{

	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"ssid", required_argument, 0, 's'},	 {"passphrase", required_argument, 0, 'p'},
		{"key-mgmt", required_argument, 0, 'k'}, {"ieee-80211w", required_argument, 0, 'w'},
		{"bssid", required_argument, 0, 'm'},	 {"band", required_argument, 0, 'b'},
		{"channel", required_argument, 0, 'c'},	 {"timeout", required_argument, 0, 't'},
		{"identity", required_argument, 0, 'a'}, {"key-passwd", required_argument, 0, 'K'},
		{"help", no_argument, 0, 'h'},		 {0, 0, 0, 0}};
	char *endptr;
	bool secure_connection = false;
	uint8_t band;
	struct wifi_credentials_personal creds = {0};

	const uint8_t all_bands[] = {WIFI_FREQ_BAND_2_4_GHZ, WIFI_FREQ_BAND_5_GHZ,
				     WIFI_FREQ_BAND_6_GHZ};
	bool found = false;
	char bands_str[MAX_BANDS_STR_LEN] = {0};
	size_t offset = 0;
	long channel;

	while ((opt = getopt_long(argc, argv, "s:p:k:w:b:c:m:t:a:K:h", long_options, &opt_index)) !=
	       -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			creds.header.ssid_len = strlen(state->optarg);
			if (creds.header.ssid_len > WIFI_SSID_MAX_LEN) {
				shell_warn(shell, "SSID too long (max %d characters)\n",
					   WIFI_SSID_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.ssid, state->optarg, creds.header.ssid_len);
			break;
		case 'k':
			creds.header.type = atoi(state->optarg);
			if (creds.header.type) {
				secure_connection = true;
			}
			break;
		case 'p':
			creds.password_len = strlen(state->optarg);
			if (creds.password_len < WIFI_PSK_MIN_LEN) {
				shell_warn(shell, "Passphrase should be minimum %d characters\n",
					   WIFI_PSK_MIN_LEN);
				return -EINVAL;
			}
			if (creds.password_len > WIFI_PSK_MAX_LEN) {
				shell_warn(shell, "Passphrase too long (max %d characters)\n",
					   WIFI_PSK_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.password, state->optarg, creds.password_len);
			break;
		case 'c':
			channel = strtol(state->optarg, &endptr, 10);
			if (*endptr != '\0') {
				shell_error(shell, "Invalid channel: %s\n", state->optarg);
				return -EINVAL;
			}

			for (band = 0; band < ARRAY_SIZE(all_bands); band++) {
				offset += snprintf(bands_str + offset, sizeof(bands_str) - offset,
						   "%s%s", band ? "," : "",
						   wifi_band_txt(all_bands[band]));
				if (offset >= sizeof(bands_str)) {
					shell_error(shell,
						    "Failed to parse channel: %ld: "
						    "band string too long\n",
						    channel);
					return -EINVAL;
				}

				if (wifi_utils_validate_chan(all_bands[band], channel)) {
					found = true;
					break;
				}
			}

			if (!found) {
				shell_error(shell, "Invalid channel: %ld, checked bands: %s\n",
					    channel, bands_str);
				return -EINVAL;
			}

			creds.header.channel = channel;
			break;
		case 'b':
			switch (atoi(state->optarg)) {
			case 2:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
				break;
			case 5:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_5GHz;
				break;
			case 6:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_6GHz;
				break;
			default:
				shell_error(shell, "Invalid band: %d\n", atoi(state->optarg));
				return -EINVAL;
			}
			break;
		case 'w':
			if (creds.header.type == WIFI_SECURITY_TYPE_NONE ||
			    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
				shell_error(shell, "MFP not supported for security type %s\n",
					    wifi_security_txt(creds.header.type));
				return -EINVAL;
			}
			if (!strncmp(state->optarg, "disabled", 8)) {
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_DISABLED;
			} else if (!strncmp(state->optarg, "required", 8)) {
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_REQUIRED;
			} else {
				shell_error(shell, "Invalid IEEE 802.11w value: %s\n",
					    state->optarg);
				return -EINVAL;
			}
			break;
		case 'm':
			if (net_bytes_from_str(creds.header.bssid, sizeof(creds.header.bssid),
					       state->optarg) < 0) {
				shell_warn(shell, "Invalid MAC address\n");
				return -EINVAL;
			}
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_BSSID;
			break;
		case 'a':
			creds.header.aid_length = strlen(state->optarg);
			if (creds.header.aid_length > WIFI_ENT_IDENTITY_MAX_LEN) {
				shell_warn(shell, "anon_id too long (max %d characters)\n",
					   WIFI_ENT_IDENTITY_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.anon_id, state->optarg, creds.header.aid_length);
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_ANONYMOUS_IDENTITY;
			break;
		case 'K':
			creds.header.key_passwd_length = strlen(state->optarg);
			if (creds.header.key_passwd_length > WIFI_ENT_PSWD_MAX_LEN) {
				shell_warn(shell, "key_passwd too long (max %d characters)\n",
					   WIFI_ENT_PSWD_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.key_passwd, state->optarg,
			       creds.header.key_passwd_length);
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_KEY_PASSWORD;
			break;
		case 'h':
			shell_help(shell);
			return -ENOEXEC;
		default:
			shell_error(shell, "Invalid option %c\n", state->optopt);
			return -EINVAL;
		}
	}
	if (creds.password_len > 0 && !secure_connection) {
		shell_warn(shell, "Passphrase provided without security configuration\n");
	}

	if (creds.header.ssid_len == 0) {
		shell_error(shell, "SSID not provided\n");
		shell_help(shell);
		return -EINVAL;
	}

	return wifi_credentials_set_personal_struct(&creds);
}

static int cmd_delete_network(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print(shell, "Usage: wifi_cred delete \"network name\"");
		return -EINVAL;
	}

	if (strnlen(argv[1], WIFI_SSID_MAX_LEN + 1) > WIFI_SSID_MAX_LEN) {
		shell_error(shell, "SSID too long");
		return -EINVAL;
	}

	shell_print(shell, "\tDeleting network ssid: \"%s\", ssid_len: %d", argv[1],
		    strlen(argv[1]));
	return wifi_credentials_delete_by_ssid(argv[1], strlen(argv[1]));
}

static int cmd_list_networks(const struct shell *shell, size_t argc, char *argv[])
{
	wifi_credentials_for_each_ssid(print_network_info, (void *)shell);
	return 0;
}

SHELL_SUBCMD_SET_CREATE(sub_wifi_cred, (wifi_cred));
SHELL_SUBCMD_ADD((wifi_cred), add, NULL,
		 "Add network to storage.\n"
		 "<-s --ssid \"<SSID>\">: SSID.\n"
		 "[-c --channel]: Channel that needs to be scanned for connection. 0:any channel.\n"
		 "[-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]\n"
		 "[-p, --psk]: Passphrase (valid only for secure SSIDs)\n"
		 "[-k, --key-mgmt]: Key Management type (valid only for secure SSIDs)\n"
		 "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, 5:SAE-AUTO, 6:WAPI,"
		 " 7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10: WPA-Auto-Personal, 11: DPP\n"
		 "[-w, --ieee-80211w]: MFP (optional: needs security type to be specified)\n"
		 ": 0:Disable, 1:Optional, 2:Required.\n"
		 "[-m, --bssid]: MAC address of the AP (BSSID).\n"
		 "[-t, --timeout]: Timeout for the connection attempt (in seconds).\n"
		 "[-a, --anon-id]: Anonymous identity for enterprise mode.\n"
		 "[-K, --key-passwd]: Private key passwd for enterprise mode.\n"
		 "[-h, --help]: Print out the help for the connect command.\n",
		 cmd_add_network, 2, 12);
SHELL_SUBCMD_ADD((wifi_cred), delete, NULL, "Delete network from storage.", cmd_delete_network, 0,
		 0);
SHELL_SUBCMD_ADD((wifi_cred), list, NULL, "List stored networks.", cmd_list_networks, 0, 0);

SHELL_CMD_REGISTER(wifi_cred, &sub_wifi_cred, "Wi-Fi Credentials commands", NULL);
