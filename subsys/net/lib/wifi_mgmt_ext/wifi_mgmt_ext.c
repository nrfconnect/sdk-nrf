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
#include <stdio.h>

#include "wpa_cli_zephyr.h"

/* Ensure 'strnlen' is available even with -std=c99. */
size_t strnlen(const char *buf, size_t bufsz);

LOG_MODULE_REGISTER(wifi_mgmt_ext, CONFIG_WIFI_MGMT_EXT_LOG_LEVEL);

#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)
	BUILD_ASSERT(sizeof(CONFIG_WIFI_CREDENTIALS_STATIC_SSID) != 1,
		     "CONFIG_WIFI_CREDENTIALS_STATIC_SSID required");
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */

static int __stored_creds_to_params(struct wifi_credentials_personal *creds,
				struct wifi_connect_req_params *params)
{
	char *ssid = NULL;
	char *psk = NULL;
	int ret;

	/* SSID */
	ssid = (char *)k_malloc(creds->header.ssid_len + 1);
	if (!ssid) {
		LOG_ERR("Failed to allocate memory for SSID\n");
		ret = -ENOMEM;
		goto err_out;
	}
	memset(ssid, 0, creds->header.ssid_len + 1);
	ret = snprintf(ssid, creds->header.ssid_len + 1, "%s", creds->header.ssid);
	if (ret > creds->header.ssid_len) {
		LOG_ERR("SSID string truncated\n");
		ret = -EINVAL;
		goto err_out;
	}
	params->ssid = ssid;
	params->ssid_length = creds->header.ssid_len;

	/* PSK (optional) */
	if (creds->password_len > 0) {
		psk = (char *)k_malloc(creds->password_len + 1);
		if (!psk) {
			LOG_ERR("Failed to allocate memory for PSK\n");
			ret = -ENOMEM;
			goto err_out;
		}
		memset(psk, 0, creds->password_len + 1);
		ret = snprintf(psk,  creds->password_len + 1, "%s", creds->password);
		if (ret > creds->password_len) {
			LOG_ERR("PSK string truncated\n");
			ret = -EINVAL;
			goto err_out;
		}
		params->psk = psk;
		params->psk_length = creds->password_len;
	}

	/* Defaults */
	params->security = creds->header.type;

	/* If channel is set to 0 we default to ANY. 0 is not a valid Wi-Fi channel. */
	params->channel = (creds->header.channel != 0) ? creds->header.channel : WIFI_CHANNEL_ANY;
	params->timeout = CONFIG_WIFI_MGMT_EXT_CONNECTION_TIMEOUT;

	/* Security type (optional) */
	if (creds->header.type > WIFI_SECURITY_TYPE_MAX) {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		params->band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if (creds->header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		params->band = WIFI_FREQ_BAND_5_GHZ;
	} else {
		params->band = WIFI_FREQ_BAND_UNKNOWN;
	}

	/* MFP setting (default: optional) */
	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_MFP_DISABLED) {
		params->mfp = WIFI_MFP_DISABLE;
	} else if (creds->header.flags & WIFI_CREDENTIALS_FLAG_MFP_REQUIRED) {
		params->mfp = WIFI_MFP_REQUIRED;
	} else {
		params->mfp = WIFI_MFP_OPTIONAL;
	}

	return 0;
err_out:
	if (ssid) {
		k_free(ssid);
		ssid = NULL;
	}
	if (psk) {
		k_free(psk);
		psk = NULL;
	}
	return ret;
}

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

static int add_network_from_credentials_struct_personal(struct wifi_credentials_personal *creds,
							struct net_if *iface)
{
	int ret = 0;
	struct wifi_connect_req_params cnx_params = {0};

	if (__stored_creds_to_params(creds, &cnx_params)) {
		ret = -ENOEXEC;
		goto out;
	}

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Connection request failed\n");

		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

out:
	if (cnx_params.psk) {
		k_free((void *)cnx_params.psk);
	}

	if (cnx_params.ssid) {
		k_free((void *)cnx_params.ssid);
	}

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

	add_network_from_credentials_struct_personal(&creds, (struct net_if *)cb_arg);
}

static int add_static_network_config(struct net_if *iface)
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

	return add_network_from_credentials_struct_personal(&creds, iface);
#else
	return 0;
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */
}

static int wifi_ext_command(uint32_t mgmt_request, struct net_if *iface,
			    void *data, size_t len)
{
	int ret = 0;

	ret = add_static_network_config(iface);
	if (ret) {
		return ret;
	}

	wifi_credentials_for_each_ssid(add_stored_network, iface);

	return ret;
};

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED, wifi_ext_command);
