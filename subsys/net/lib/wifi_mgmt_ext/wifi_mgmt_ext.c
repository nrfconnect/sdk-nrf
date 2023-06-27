/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/wifi_mgmt_ext.h>
#include <net/wifi_credentials.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include "wpa_cli_zephyr.h"

/* Ensure 'strnlen' is available even with -std=c99. */
size_t strnlen(const char *buf, size_t bufsz);

LOG_MODULE_REGISTER(wifi_mgmt_ext, CONFIG_WIFI_MGMT_EXT_LOG_LEVEL);

#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)
	BUILD_ASSERT(sizeof(CONFIG_WIFI_CREDENTIALS_STATIC_SSID) != 1,
		     "CONFIG_WIFI_CREDENTIALS_STATIC_SSID required");
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */

static inline const char *wpa_supp_security_txt(enum wifi_security_type security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		return "NONE";
	case WIFI_SECURITY_TYPE_PSK:
		return "WPA-PSK";
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		return "WPA-PSK-SHA256";
	case WIFI_SECURITY_TYPE_SAE:
		return "SAE";
	case WIFI_SECURITY_TYPE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

static int add_network_from_credentials_struct_personal(struct wifi_credentials_personal *creds)
{
	int ret = 0;
	struct add_network_resp resp = {0};

	ret = z_wpa_ctrl_add_network(&resp);
	if (ret) {
		LOG_ERR("Encoding error: %d returned at %s:%d", ret, __FILE__, __LINE__);
		return ret;
	}

	z_wpa_cli_cmd_v("set_network %d ssid \"%s\"", resp.network_id, creds->header.ssid);
	z_wpa_cli_cmd_v("set_network %d key_mgmt %s", resp.network_id,
		     wpa_supp_security_txt(creds->header.type));

	if (creds->header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds->header.type == WIFI_SECURITY_TYPE_PSK_SHA256) {
		z_wpa_cli_cmd_v("set_network %d psk \"%.*s\"", resp.network_id,
			     creds->password_len, creds->password);
	}

	if (creds->header.type == WIFI_SECURITY_TYPE_SAE) {
		z_wpa_cli_cmd_v("set_network %d sae_password \"%.*s\"",
			     resp.network_id, creds->password_len, creds->password);
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		z_wpa_cli_cmd_v("set_network %d bssid %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
			resp.network_id,
			creds->header.bssid[0], creds->header.bssid[1],
			creds->header.bssid[2], creds->header.bssid[3],
			creds->header.bssid[4], creds->header.bssid[5]);
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		z_wpa_cli_cmd_v("set_network %d priority 1", resp.network_id);
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		z_wpa_cli_cmd_v("set_network %d freq_list "
			     "2412 2417 2422 2427 2432 2437 2442 "
			     "2447 2452 2457 2462 2467 2472 2484", resp.network_id);
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		z_wpa_cli_cmd_v("set_network %d freq_list "
			     "5180 5200 5220 5240 5260 5280 5300 "
			     "5310 5320 5500 5520 5540 5560 5580 "
			     "5600 5620 5640 5660 5680 5700 5745 "
			     "5765 5785 5805 5825", resp.network_id);
	}

	z_wpa_cli_cmd_v("enable_network %d", resp.network_id);
	return ret;
}

static void add_stored_network(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds;

	/* load stored data */
	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);

	if (ret) {
		LOG_ERR("Loading WiFi credentials failed for SSID [%.*s], len: %d, err: %d",
			ssid_len, ssid,  ssid_len, ret);
		return;
	}

	add_network_from_credentials_struct_personal(&creds);
}

static int add_static_network_config(void)
{
#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)

	struct wifi_credentials_personal creds = {
		.header = {
			.ssid_len = strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID),
		},
		.password_len = strlen(CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD),
	};

	int ret = wifi_credentials_get_by_ssid_personal_struct(
		CONFIG_WIFI_CREDENTIALS_STATIC_SSID,
		strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID),
		&creds
	);

	if (!ret) {
		LOG_WRN("Statically configured WiFi network was overridden by storage.");
		return 0;
	}

#if	defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_OPEN)
	creds.header.type = WIFI_SECURITY_TYPE_NONE;
#elif	defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK)
	creds.header.type = WIFI_SECURITY_TYPE_PSK;
#elif	defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK_SHA256)
	creds.header.type = WIFI_SECURITY_TYPE_PSK_SHA256;
#elif	defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_SAE)
	creds.header.type = WIFI_SECURITY_TYPE_SAE;
#else
	#error "invalid CONFIG_WIFI_CREDENTIALS_STATIC_TYPE"
#endif

	memcpy(creds.header.ssid, CONFIG_WIFI_CREDENTIALS_STATIC_SSID,
	strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID));
	memcpy(creds.password, CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD,
	strlen(CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD));

	LOG_DBG("Adding statically configured WiFi network [%s] to internal list.",
		creds.header.ssid);

	return add_network_from_credentials_struct_personal(&creds);
#else
	return 0;
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */
}

static int wifi_ext_command(uint32_t mgmt_request, struct net_if *iface,
			    void *data, size_t len)
{
	int ret = 0;

	z_wpa_cli_cmd_v("remove_network all");

	ret = add_static_network_config();
	if (ret) {
		return ret;
	}

	wifi_credentials_for_each_ssid(add_stored_network, NULL);

	return ret;
};

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED, wifi_ext_command);
