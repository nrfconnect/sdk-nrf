/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/wifi_mgmt_ext.h>
#include <net/wifi_credentials.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

extern struct k_sem wpa_supplicant_ready_sem;
extern struct k_mutex wpa_supplicant_mutex;
extern struct wpa_global *global;


LOG_MODULE_REGISTER(wifi_mgmt_ext, CONFIG_WIFI_MGMT_EXT_LOG_LEVEL);

/* helper functions {*/

static inline struct wpa_supplicant *get_wpa_s_handle(const struct device *dev)
{
	struct wpa_supplicant *wpa_s = NULL;
	int ret = k_sem_take(&wpa_supplicant_ready_sem, K_SECONDS(2));

	if (ret) {
		LOG_ERR("%s: WPA supplicant not ready: %d", __func__, ret);
		return NULL;
	}

	k_sem_give(&wpa_supplicant_ready_sem);

	wpa_s = wpa_supplicant_get_iface(global, dev->name);
	if (!wpa_s) {
		LOG_ERR("%s: Unable to get wpa_s handle for %s\n", __func__, dev->name);
		return NULL;
	}

	return wpa_s;
}

static inline void band_to_freq(struct wifi_credentials_personal *creds, struct wpa_ssid *ssid)
{
	/** 2.4 GHz channels: 1-14 */
	const int channels_2_4ghz[] = {
		2412, 2417, 2422, 2427,
		2432, 2437, 2442, 2447,
		2452, 2457, 2462, 2467,
		2472, 2484
	};

	/** 5 GHz channels:
	 *   36,  40,  44,  48,
	 *   52,  56,  60,  62,
	 *   64, 100, 104, 108,
	 *  112, 116, 120, 124,
	 *  128, 132, 136, 140,
	 *  149, 153, 157, 161,
	 *  165
	 */
	const int channels_5ghz[] = {
		5180, 5200, 5220, 5240,
		5260, 5280, 5300, 5310,
		5320, 5500, 5520, 5540,
		5560, 5580, 5600, 5620,
		5640, 5660, 5680, 5700,
		5745, 5765, 5785, 5805,
		5825
	};

	if (!(creds->header.flags & (WIFI_CREDENTIALS_FLAG_2_4GHz | WIFI_CREDENTIALS_FLAG_5GHz))) {
		/* there are no flags indicating band - skip */
		return;
	}
	if ((creds->header.flags & (WIFI_CREDENTIALS_FLAG_2_4GHz | WIFI_CREDENTIALS_FLAG_5GHz)) ==
	    (WIFI_CREDENTIALS_FLAG_2_4GHz | WIFI_CREDENTIALS_FLAG_5GHz)) {
		/* both bands are indicated - skip */
		return;
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		ssid->freq_list = os_zalloc((ARRAY_SIZE(channels_2_4ghz) + 1) * sizeof(int));
		if (!ssid->freq_list) {
			LOG_ERR("unable to allocate buffer for freq_list");
			return;
		}
		memcpy(ssid->freq_list, channels_2_4ghz, ARRAY_SIZE(channels_2_4ghz) * sizeof(int));
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		ssid->freq_list = os_zalloc((ARRAY_SIZE(channels_5ghz) + 1) * sizeof(int));
		if (!ssid->freq_list) {
			LOG_ERR("unable to allocate buffer for freq_list");
			return;
		}
		memcpy(ssid->freq_list, channels_5ghz, ARRAY_SIZE(channels_5ghz) * sizeof(int));
	}
}

static inline int copy_psk(enum wifi_security_type security,
			   const uint8_t *password,
			   size_t password_len,
			   struct wpa_ssid *ssid)
{
	if (!password) {
		return 0;
	}
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		ssid->key_mgmt = WPA_KEY_MGMT_NONE;
		break;
	case WIFI_SECURITY_TYPE_SAE:
		ssid->key_mgmt = WPA_KEY_MGMT_SAE;
		str_clear_free(ssid->sae_password);
		ssid->sae_password = dup_binstr(password, password_len);

		if (ssid->sae_password == NULL) {
			LOG_ERR("%s:Failed to copy sae_password\n", __func__);
			return -ENOMEM;
		}
		break;
	case WIFI_SECURITY_TYPE_PSK:
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		if (security == WIFI_SECURITY_TYPE_PSK) {
			ssid->key_mgmt = WPA_KEY_MGMT_PSK;
		} else {
			ssid->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
		}
		str_clear_free(ssid->passphrase);
		ssid->passphrase = dup_binstr(password, password_len);

		if (ssid->passphrase == NULL) {
			LOG_ERR("%s:Failed to copy passphrase\n", __func__);
			return -ENOMEM;
		}
		break;
	default:
		LOG_ERR("%s:Security type %d is not supported\n", __func__, security);
		return -EINVAL;
	}
	wpa_config_update_psk(ssid);

	return 0;
}

static void add_stored_network(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wpa_supplicant *wpa_s = cb_arg;
	struct wpa_ssid *network = wpa_supplicant_add_network(wpa_s);
	struct wifi_credentials_personal creds;

	if (network == NULL) {
		return;
	}

	network->ssid = os_zalloc(sizeof(u8) * WIFI_SSID_MAX_LEN);

	if (network->ssid == NULL) {
		goto exit;
	}

	/* load stored data */
	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);
	if (ret) {
		LOG_ERR("Loading WiFi credentials failed for SSID [%.*s], len: %d, err: %d",
			ssid_len, ssid,  ssid_len, ret);
		goto exit;
	}

	memcpy(network->ssid, creds.header.ssid, creds.header.ssid_len);
	network->ssid_len = creds.header.ssid_len;
	network->disabled = 1;
	network->ieee80211w = 1;

	/* apply priority */
	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		network->priority = 1;
	}

	/* apply BSSID restriction */
	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		memcpy(network->bssid, creds.header.bssid, WIFI_MAC_ADDR_LEN);
		network->bssid_set = 1;
	}
	/* apply band restriction */
	band_to_freq(&creds, network);

	ret = copy_psk(creds.header.type, creds.password, creds.password_len, network);
	if (ret) {
		goto exit;
	}

	wpa_supplicant_enable_network(wpa_s, network);
	return;
exit:
	wpa_supplicant_remove_network(wpa_s, network->id);
}

/* } */

static int wifi_ext_command(uint32_t mgmt_request, struct net_if *iface,
			    void *data, size_t len)
{
	struct wpa_supplicant *wpa_s = NULL;
	const struct device *dev = net_if_get_device(iface);
	int ret = 0;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		goto out;
	}

	ret = wpa_supplicant_remove_all_networks(wpa_s);
	if (ret) {
		goto out;
	}

	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	wifi_credentials_for_each_ssid(add_stored_network, wpa_s);
	wpas_request_connection(wpa_s);

out:
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
};

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED, wifi_ext_command);
