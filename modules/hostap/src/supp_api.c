/**
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

#include "supp_main.h"
#include "supp_api.h"

int cli_main(int, const char **);
extern struct k_sem wpa_supplicant_ready_sem;
extern struct wpa_global *global;

enum requested_ops {
	CONNECT = 0,
	DISCONNECT
};

#define OP_STATUS_POLLING_INTERVAL 1

K_MUTEX_DEFINE(wpa_supplicant_mutex);


struct wpa_supp_api_ctrl {
	const struct device *dev;
	enum requested_ops requested_op;
	int connection_timeout;
	struct k_work_sync sync;
	bool terminate;
};

static struct wpa_supp_api_ctrl wpa_supp_api_ctrl;

static void supp_shell_connect_status(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(wpa_supp_status_work,
		supp_shell_connect_status);

static int send_wpa_supplicant_dummy_event(void)
{
	struct wpa_supplicant_event_msg msg = { 0 };

	msg.ignore_msg = true;

	return send_wpa_supplicant_event(&msg);
}


static inline struct wpa_supplicant * get_wpa_s_handle(const struct device *dev)
{
	struct wpa_supplicant *wpa_s = NULL;
	int ret = k_sem_take(&wpa_supplicant_ready_sem, K_SECONDS(2));

	if (ret) {
		wpa_printf(MSG_ERROR, "%s: WPA supplicant not ready: %d", __func__, ret);
		return NULL;
	}

	k_sem_give(&wpa_supplicant_ready_sem);

	wpa_s = wpa_supplicant_get_iface(global, dev->name);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR,
		"%s: Unable to get wpa_s handle for %s\n", __func__, dev->name);
		return NULL;
	}

	return wpa_s;
}

static void supp_shell_connect_status(struct k_work *work)
{
	int i = 0, status = 0;
	struct wpa_supplicant *wpa_s;
	struct wpa_supp_api_ctrl *ctrl = &wpa_supp_api_ctrl;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	if (ctrl->terminate) {
		status = 2;
		goto out;
	}

	wpa_s = get_wpa_s_handle(ctrl->dev);
	if (!wpa_s) {
		status = 1;
		goto out;
	}

	if (ctrl->requested_op == CONNECT) {
		if (wpa_s->wpa_state != WPA_COMPLETED &&
			   (ctrl->connection_timeout == SYS_FOREVER_MS ||
			    i++ <= ctrl->connection_timeout))
		{
			k_work_reschedule(&wpa_supp_status_work,
				K_SECONDS(OP_STATUS_POLLING_INTERVAL));
			k_mutex_unlock(&wpa_supplicant_mutex);
			return;
		}

		if (i > ctrl->connection_timeout){
			if (wpa_s->wpa_state != WPA_COMPLETED){
				wpas_request_disconnection(wpa_s);
				status = 1;
				goto out;
			}
		}
	}
out:
	if (ctrl->requested_op == CONNECT) {
		wifi_mgmt_raise_connect_result_event(net_if_lookup_by_dev(ctrl->dev), status);
	} else if (ctrl->requested_op == DISCONNECT) {
		/* Disconnect is a synchronous operation i.e., we are already disconnected
		 * we are just using this to post net_mgmt event asynchrnously.
		 */
		wifi_mgmt_raise_disconnect_result_event(net_if_lookup_by_dev(ctrl->dev), 0);
	}

	k_mutex_unlock(&wpa_supplicant_mutex);
}

static inline void wpa_supp_restart_status_work(void)
{
	/* Terminate synchronously */
	wpa_supp_api_ctrl.terminate = 1;
	k_work_flush_delayable(&wpa_supp_status_work,
		&wpa_supp_api_ctrl.sync);
	wpa_supp_api_ctrl.terminate = 0;

	/* Start afresh */
	k_work_reschedule(&wpa_supp_status_work,
		K_MSEC(10));
}


int zephyr_supp_connect(const struct device *dev,
						struct wifi_connect_req_params *params)
{
	struct wpa_ssid *ssid = NULL;
	bool pmf = true;
	struct wpa_supplicant *wpa_s;
	int ret = 0;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		goto out;
	}

	wpa_supplicant_remove_all_networks(wpa_s);

	ssid = wpa_supplicant_add_network(wpa_s);
	ssid->ssid = os_zalloc(sizeof(u8) * MAX_SSID_LEN);

	memcpy(ssid->ssid, params->ssid, params->ssid_length);
	ssid->ssid_len = params->ssid_length;
	ssid->disabled = 1;
	ssid->key_mgmt = WPA_KEY_MGMT_NONE;

	if (params->channel != WIFI_CHANNEL_ANY) {
		/* We use global channel list here and also use the widest
		 * op_class for 5GHz channels as there is no user input
		 * for these.
		 */
		int freq  = ieee80211_chan_to_freq(NULL, 81, params->channel);

		if (freq <= 0) {
			freq  = ieee80211_chan_to_freq(NULL, 128, params->channel);
		}

		if (freq <= 0) {
			wpa_printf(MSG_ERROR, "Invalid channel %d", params->channel);
			ret = -EINVAL;
			goto out;
		}

		ssid->scan_freq = os_zalloc(2 * sizeof(int));
		if (!ssid->scan_freq) {
			ret = -ENOMEM;
			goto out;
		}
		ssid->scan_freq[0] = freq;
		ssid->scan_freq[1] = 0;

		ssid->freq_list = os_zalloc(2 * sizeof(int));
		if (!ssid->freq_list) {
			os_free(ssid->scan_freq);
			ret = -ENOMEM;
			goto out;
		}
		ssid->freq_list[0] = freq;
		ssid->freq_list[1] = 0;
	}

	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	if (params->psk) {
		// TODO: Extend enum wifi_security_type
		if (params->security == 3) {
			ssid->key_mgmt = WPA_KEY_MGMT_SAE;
			str_clear_free(ssid->sae_password);
			ssid->sae_password = dup_binstr(params->psk, params->psk_length);

			if (ssid->sae_password == NULL) {
				wpa_printf(MSG_ERROR, "%s:Failed to copy sae_password\n",
					      __func__);
				ret = -ENOMEM;
				goto out;
			}
		} else {
			if (params->security == 2)
				ssid->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
			else
				ssid->key_mgmt = WPA_KEY_MGMT_PSK;

			str_clear_free(ssid->passphrase);
			ssid->passphrase = dup_binstr(params->psk, params->psk_length);

			if (ssid->passphrase == NULL) {
				wpa_printf(MSG_ERROR, "%s:Failed to copy passphrase\n",
				__func__);
				ret = -ENOMEM;
				goto out;
			}
		}

		wpa_config_update_psk(ssid);

		if (pmf)
			ssid->ieee80211w = 1;

	}

	wpa_supplicant_enable_network(wpa_s,
				      ssid);

	wpa_supplicant_select_network(wpa_s,
				      ssid);

	send_wpa_supplicant_dummy_event();

	wpa_supp_api_ctrl.dev = dev;
	wpa_supp_api_ctrl.requested_op = CONNECT;
	wpa_supp_api_ctrl.connection_timeout = params->timeout;

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	if (!ret) {
		wpa_supp_restart_status_work();
	}

	return ret;
}

int zephyr_supp_disconnect(const struct device *dev)
{
	struct wpa_supplicant *wpa_s;
	int ret = 0;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -EINVAL;
		goto out;
	}
	wpa_supp_api_ctrl.dev = dev;
	wpa_supp_api_ctrl.requested_op = DISCONNECT;
	wpas_request_disconnection(wpa_s);

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	if (!ret) {
		wpa_supp_restart_status_work();
	}

	return ret;
}


static inline enum wifi_frequency_bands wpas_band_to_zephyr(enum wpa_radio_work_band band)
{
	switch(band) {
		case BAND_2_4_GHZ:
			return WIFI_FREQ_BAND_2_4_GHZ;
		case BAND_5_GHZ:
			return WIFI_FREQ_BAND_5_GHZ;
		default:
			return WIFI_FREQ_BAND_UNKNOWN;
	}
}

static inline enum wifi_security_type wpas_key_mgmt_to_zephyr(int key_mgmt)
{
	switch(key_mgmt) {
		case WPA_KEY_MGMT_NONE:
			return WIFI_SECURITY_TYPE_NONE;
		case WPA_KEY_MGMT_PSK:
			return WIFI_SECURITY_TYPE_PSK;
		case WPA_KEY_MGMT_PSK_SHA256:
			return WIFI_SECURITY_TYPE_PSK_SHA256;
		case WPA_KEY_MGMT_SAE:
			return WIFI_SECURITY_TYPE_SAE;
		default:
			return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}


int zephyr_supp_status(const struct device *dev,
				struct wifi_iface_status *status)
{
	struct wpa_supplicant *wpa_s;
	int ret = -1;
	struct wpa_signal_info *si = NULL;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		goto out;
	}

	si = os_zalloc(sizeof(struct wpa_signal_info));
	if (!si) {
		goto out;
	}

	status->state = wpa_s->wpa_state; /* 1-1 Mapping */

	if (wpa_s->wpa_state >= WPA_ASSOCIATED) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		u8 channel;

		os_memcpy(status->bssid, wpa_s->bssid, WIFI_MAC_ADDR_LEN);
		status->band = wpas_band_to_zephyr(wpas_freq_to_band(wpa_s->assoc_freq));
		status->security = wpas_key_mgmt_to_zephyr(ssid->key_mgmt);
		status->mfp = ssid->ieee80211w; /* Same mapping */
		ieee80211_freq_to_chan(wpa_s->assoc_freq, &channel);
		status->channel = channel;

		if (ssid) {
			u8 *_ssid = ssid->ssid;
			size_t ssid_len = ssid->ssid_len;
			u8 ssid_buf[SSID_MAX_LEN] = {0};

			if (ssid_len == 0) {
				int _res = wpa_drv_get_ssid(wpa_s, ssid_buf);

				if (_res < 0)
					ssid_len = 0;
				else
					ssid_len = _res;
				_ssid = ssid_buf;
			}
			os_memcpy(status->ssid, _ssid, ssid_len);
			status->ssid_len = ssid_len;
			status->iface_mode = ssid->mode;
			/* TODO: Derive this based on association IEs */
			status->link_mode = WIFI_6;
		}
		ret = wpa_drv_signal_poll(wpa_s, si);
		if (!ret) {
			status->rssi = si->current_signal;
		} else {
			wpa_printf(MSG_ERROR, "%s:Failed to read RSSI\n",
				__func__);
			status->rssi = -WPA_INVALID_NOISE;
		}
	} else {
		ret = 0;
	}
out:
	os_free(si);
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
}
