/**
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdarg.h>

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
#include "wpa_cli_zephyr.h"

extern struct k_sem wpa_supplicant_ready_sem;
extern struct wpa_global *global;

enum requested_ops {
	CONNECT = 0,
	DISCONNECT
};

enum status_thread_state {
	STATUS_THREAD_STOPPED = 0,
	STATUS_THREAD_RUNNING,
};

#define OP_STATUS_POLLING_INTERVAL 1

#define CONNECTION_SUCCESS 0
#define CONNECTION_FAILURE 1
#define CONNECTION_TERMINATED 2

#define DISCONNECT_TIMEOUT_MS 5000

#define _wpa_cli_cmd_v(cmd, ...) \
	do { \
		if (z_wpa_cli_cmd_v(cmd, ##__VA_ARGS__) < 0) { \
			wpa_printf(MSG_ERROR, "Failed to execute wpa_cli command: %s", cmd); \
			goto out; \
		} \
	} while (0)

K_MUTEX_DEFINE(wpa_supplicant_mutex);


struct wpa_supp_api_ctrl {
	const struct device *dev;
	enum requested_ops requested_op;
	enum status_thread_state status_thread_state;
	int connection_timeout; // in seconds
	struct k_work_sync sync;
	bool terminate;
};

static struct wpa_supp_api_ctrl wpa_supp_api_ctrl;

static void supp_shell_connect_status(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(wpa_supp_status_work,
		supp_shell_connect_status);

static inline struct wpa_supplicant * get_wpa_s_handle(const struct device *dev)
{
	return z_wpas_get_handle_by_ifname(dev->name);
}

static int wait_for_disconnect_complete(const struct device *dev)
{
	int ret = 0;
	int timeout = 0;

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		status = CONNECTION_FAILURE;
		goto out;
	}

	while (wpa_s->wpa_state != WPA_DISCONNECTED) {
		if (timeout > DISCONNECT_TIMEOUT_MS) {
			ret = -ETIMEDOUT;
			break;
		}
		k_sleep(K_MSEC(10));
		timeout++;
	}

	return ret;
}

static void supp_shell_connect_status(struct k_work *work)
{
	static int seconds_counter = 0;
	int status = CONNECTION_SUCCESS;
	struct wpa_supplicant *wpa_s;
	struct wpa_supp_api_ctrl *ctrl = &wpa_supp_api_ctrl;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	if (ctrl->status_thread_state == STATUS_THREAD_RUNNING &&  ctrl->terminate) {
		status = CONNECTION_TERMINATED;
		goto out;
	}

	wpa_s = get_wpa_s_handle(ctrl->dev);
	if (!wpa_s) {
		status = CONNECTION_FAILURE;
		goto out;
	}

	if (ctrl->requested_op == CONNECT && wpa_s->wpa_state != WPA_COMPLETED) {
		if (ctrl->connection_timeout > 0 && seconds_counter++ > ctrl->connection_timeout) {
			_wpa_cli_cmd_v("disconnect");
			status = CONNECTION_FAILURE;
			goto out;
		}

		k_work_reschedule(&wpa_supp_status_work, K_SECONDS(OP_STATUS_POLLING_INTERVAL));
		ctrl->status_thread_state = STATUS_THREAD_RUNNING;
		k_mutex_unlock(&wpa_supplicant_mutex);
		return;
	}
out:
	seconds_counter = 0;

	ctrl->status_thread_state = STATUS_THREAD_STOPPED;
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

static inline int chan_to_freq(int chan)
{
	/* We use global channel list here and also use the widest
	 * op_class for 5GHz channels as there is no user input
	 * for these (yet).
	 */
	int freq  = ieee80211_chan_to_freq(NULL, 81, chan);

	if (freq <= 0) {
		freq  = ieee80211_chan_to_freq(NULL, 128, chan);
	}

	if (freq <= 0) {
		wpa_printf(MSG_ERROR, "Invalid channel %d", chan);
		return -1;
	}

	return freq;
}


int z_wpa_supplicant_connect(const struct device *dev,
						struct wifi_connect_req_params *params)
{
	struct wpa_supplicant *wpa_s;
	int ret = 0;
	struct add_network_resp resp = {0};

	if (!net_if_is_admin_up(net_if_lookup_by_dev(dev))) {
		wpa_printf(MSG_ERROR,
			   "Interface %s is down, dropping connect",
			   dev->name);
		return -1;
	}

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -1;
		goto out;
	}

	_wpa_cli_cmd_v("remove_network all");
	ret = z_wpa_ctrl_add_network(&resp);
	if (ret) {
		goto out;
	}

	wpa_printf(MSG_DEBUG, "NET added: %d\n", resp.network_id);

	_wpa_cli_cmd_v("set_network %d ssid \"%s\"", resp.network_id, params->ssid);
	_wpa_cli_cmd_v("set_network %d scan_ssid 1", resp.network_id);
	_wpa_cli_cmd_v("set_network %d key_mgmt NONE", resp.network_id);
	_wpa_cli_cmd_v("set_network %d ieee80211w 0", resp.network_id);
	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		if (params->security == WIFI_SECURITY_TYPE_SAE) {
			if (params->sae_password) {
				_wpa_cli_cmd_v("set_network %d sae_password \"%s\"",
					resp.network_id, params->sae_password);
			} else {
				_wpa_cli_cmd_v("set_network %d sae_password \"%s\"",
					resp.network_id, params->psk);
			}
			_wpa_cli_cmd_v("set_network %d key_mgmt SAE",
				resp.network_id);
		} else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			_wpa_cli_cmd_v("set_network %d psk \"%s\"",
				resp.network_id, params->psk);
			_wpa_cli_cmd_v("set_network %d key_mgmt WPA-PSK-SHA256",
				resp.network_id);
		} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
			_wpa_cli_cmd_v( "set_network %d psk \"%s\"",
			resp.network_id, params->psk);
			_wpa_cli_cmd_v("set_network %d key_mgmt WPA-PSK",
				resp.network_id);
		} else {
			ret = -1;
			wpa_printf(MSG_ERROR, "Unsupported security type: %d",
				params->security);
			goto out;
		}

		if (params->mfp) {
			_wpa_cli_cmd_v("set_network %d ieee80211w %d",
				resp.network_id, params->mfp);
		}
	}

	/* enable and select network */
	_wpa_cli_cmd_v("enable_network %d", resp.network_id);

	if (params->channel != WIFI_CHANNEL_ANY) {
		int freq = chan_to_freq(params->channel);
		if (freq < 0) {
			ret = -1;
			goto out;
		}
		z_wpa_cli_cmd_v("set_network %d scan_freq %d",
			resp.network_id, freq);
	}
	_wpa_cli_cmd_v("select_network %d", resp.network_id);

	z_wpa_cli_cmd_v("select_network %d", resp.network_id);
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

int z_wpa_supplicant_disconnect(const struct device *dev)
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
	_wpa_cli_cmd_v("disconnect");

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	wpa_supp_restart_status_work();

	return wait_for_disconnect_complete(dev);
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


int z_wpa_supplicant_status(const struct device *dev,
				struct wifi_iface_status *status)
{
	struct wpa_supplicant *wpa_s;
	int ret = -1;
	struct wpa_signal_info *si = NULL;
	struct wpa_conn_info *conn_info = NULL;

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
		struct signal_poll_resp signal_poll;

		os_memcpy(status->bssid, wpa_s->bssid, WIFI_MAC_ADDR_LEN);
		status->band = wpas_band_to_zephyr(wpas_freq_to_band(wpa_s->assoc_freq));
		status->security = wpas_key_mgmt_to_zephyr(ssid->key_mgmt);
		status->mfp = ssid->ieee80211w; /* Same mapping */
		ieee80211_freq_to_chan(wpa_s->assoc_freq, &channel);
		status->channel = channel;

		if (ssid) {
			u8 *_ssid = ssid->ssid;
			size_t ssid_len = ssid->ssid_len;
			struct status_resp cli_status;

			if (ssid_len == 0) {
				int _res = z_wpa_ctrl_status(&cli_status);

				if (_res < 0)
					ssid_len = 0;
				else
					ssid_len = cli_status.ssid_len;
				_ssid = cli_status.ssid;
			}
			os_memcpy(status->ssid, _ssid, ssid_len);
			status->ssid_len = ssid_len;
			status->iface_mode = ssid->mode;
			if (wpa_s->connection_set == 1) {
				status->link_mode = wpa_s->connection_he ? WIFI_6 :
								wpa_s->connection_vht ? WIFI_5 :
								wpa_s->connection_ht ? WIFI_4 :
								wpa_s->connection_g ? WIFI_3 :
								wpa_s->connection_a ? WIFI_2 :
								wpa_s->connection_b ? WIFI_1 : WIFI_0;
			} else {
				status->link_mode = WIFI_LINK_MODE_UNKNOWN;
			}
		}
		ret = z_wpa_ctrl_signal_poll(&signal_poll);
		if (!ret) {
			status->rssi = signal_poll.rssi;
		} else {
			wpa_printf(MSG_WARNING, "%s:Failed to read RSSI\n",
				__func__);
			status->rssi = -WPA_INVALID_NOISE;
			ret = 0;
		}

		conn_info = os_zalloc(sizeof(struct wpa_conn_info));
		if (!conn_info) {
			wpa_printf(MSG_ERROR, "%s:Failed to allocate memory\n",
				__func__);
			ret = -ENOMEM;
			goto out;
		}
		ret = wpa_drv_get_conn_info(wpa_s, conn_info);
		if (!ret) {
			status->beacon_interval = conn_info->beacon_interval;
			status->dtim_period = conn_info->dtim_period;
			status->twt_capable = conn_info->twt_capable;
		} else {
			wpa_printf(MSG_WARNING, "%s: Failed to get connection info\n",
				__func__);
				status->beacon_interval = 0;
				status->dtim_period = 0;
				status->twt_capable = false;
				ret = 0;
		}
	} else {
		ret = 0;
	}
out:
	os_free(si);
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
}
