/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Wi-Fi driver for the nRF92 Series.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_wifi_scan.h>

#define DT_DRV_COMPAT nordic_nrf92_wifi

LOG_MODULE_REGISTER(wifi_nrf92, CONFIG_WIFI_LOG_LEVEL);

static struct net_if *net_iface;
static scan_result_cb_t scan_cb;

static int64_t scan_start_time;
static uint16_t scan_result_count;

static int status_event_to_errno(enum nrf_modem_wifi_scan_status status)
{
	switch (status) {
	case NRF_MODEM_WIFI_SCAN_STATUS_SUCCESS:
		return 0;
	case NRF_MODEM_WIFI_SCAN_STATUS_ABORTED:
		return -ECANCELED;
	case NRF_MODEM_WIFI_SCAN_STATUS_TIMEOUT:
		return -ETIMEDOUT;
	case NRF_MODEM_WIFI_SCAN_STATUS_NO_MEMORY:
		return -ENOMEM;
	case NRF_MODEM_WIFI_SCAN_STATUS_FAILURE:
	default:
		return -EFAULT;
	}
}

static void nrf92_wifi_scan_evt_handler(const struct nrf_modem_wifi_scan_event *evt)
{
	switch (evt->id) {
	case NRF_MODEM_WIFI_SCAN_EVT_RESULT:
		if (!evt->result.finished) {
			struct wifi_scan_result res = { 0 };

			strncpy(res.ssid, evt->result.ssid, sizeof(res.ssid));
			res.ssid_length = strlen(evt->result.ssid);
			memcpy(res.mac, evt->result.mac_address, WIFI_MAC_ADDR_LEN);
			res.mac_length = WIFI_MAC_ADDR_LEN;
			res.rssi = evt->result.rssi;
			res.channel = evt->result.channel;

			LOG_DBG("ssid: %s, mac: %02x:%02x:%02x:%02x:%02x:%02x, "
				"rssi: %d, channel: %d",
				res.ssid,
				res.mac[0], res.mac[1], res.mac[2],
				res.mac[3], res.mac[4], res.mac[5],
				res.rssi,
				res.channel);

			scan_cb(net_iface, 0, &res);

			scan_result_count++;
		} else {
			int64_t scan_duration = k_uptime_get() - scan_start_time;

			LOG_INF("Scan finished, status: %d, ap count: %u, scan time: %lld ms",
				evt->status, scan_result_count, scan_duration);

			scan_cb(net_iface, status_event_to_errno(evt->status), NULL);

			scan_cb = NULL;
		}
		break;
	default:
		break;
	}
}

static int nrf92_wifi_scan(
	const struct device *dev,
	struct wifi_scan_params *params,
	scan_result_cb_t cb)
{
	int err;
	struct nrf_modem_wifi_scan_start_params nrf_modem_params = { 0 };

	LOG_INF("Scan request received");

	if (scan_cb != NULL) {
		LOG_ERR("nRF92 Wi-Fi scan operation already in progress");
		return -EINPROGRESS;
	}

	/* Set scan actions based on Kconfig options */
	if (IS_ENABLED(CONFIG_WIFI_NRF92_IGNORE_PROBES)) {
		nrf_modem_params.actions |= NRF_MODEM_WIFI_SCAN_ACTIONS_IGNORE_PROBES;
	}
	if (IS_ENABLED(CONFIG_WIFI_NRF92_IGNORE_LOCAL_ADDR)) {
		nrf_modem_params.actions |= NRF_MODEM_WIFI_SCAN_ACTIONS_IGNORE_LOCAL_ADDR;
	}
	if (IS_ENABLED(CONFIG_WIFI_NRF92_INCREASE_MAX_GAIN)) {
		nrf_modem_params.actions |= NRF_MODEM_WIFI_SCAN_ACTIONS_INCREASE_MAX_GAIN;
	}
	if (IS_ENABLED(CONFIG_WIFI_NRF92_OFFLINE_PROCESSING)) {
		nrf_modem_params.actions |= NRF_MODEM_WIFI_SCAN_ACTIONS_ENABLE_OFFLINE_PROCESSING;
	}

	if (params) {
		/* Channels */
		nrf_modem_params.channels_bitmap = 0;
		for (int i = 0; i < ARRAY_SIZE(params->band_chan); i++) {
			if (params->band_chan[i].band == WIFI_FREQ_BAND_2_4_GHZ &&
			    params->band_chan[i].channel > 0 &&
			    params->band_chan[i].channel <= 14) {
				nrf_modem_params.channels_bitmap |=
					BIT(params->band_chan[i].channel - 1);
			}
		}

		/* Scan time */
		nrf_modem_params.scan_time_desired_ms = params->dwell_time_passive;

		/* Max. AP count */
		nrf_modem_params.ap_max_count = params->max_bss_cnt;
	}

	/* If no channels were specified, default to all channels */
	if (nrf_modem_params.channels_bitmap == 0) {
		nrf_modem_params.channels_bitmap = NRF_MODEM_WIFI_SCAN_WIFI_ALL_CHANNELS;
	}

	LOG_DBG("actions: 0x%x, channels: 0x%x, desired scan time: %d ms, max ap count: %d",
		nrf_modem_params.actions,
		nrf_modem_params.channels_bitmap,
		nrf_modem_params.scan_time_desired_ms,
		nrf_modem_params.ap_max_count);

	scan_start_time = k_uptime_get();
	scan_result_count = 0;

	scan_cb = cb;

	err = nrf_modem_wifi_scan_start(&nrf_modem_params);
	if (err) {
		LOG_ERR("Starting Wi-Fi scan failed: %d\n", err);
		scan_cb = NULL;
		return err;
	}

	return 0;
};

static struct net_offload nrf92_wifi_offload = {
};

static void nrf92_wifi_iface_init(struct net_if *iface)
{
	LOG_DBG("iface: %p", iface);

	iface->if_dev->offload = &nrf92_wifi_offload;

	net_if_carrier_off(iface);
	net_iface = iface;

	nrf_modem_wifi_scan_event_handler_set(nrf92_wifi_scan_evt_handler);
}

static enum offloaded_net_if_types nrf92_wifi_offload_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

static const struct wifi_mgmt_ops nrf92_wifi_mgmt_ops = {
	.scan = nrf92_wifi_scan,
};

static const struct net_wifi_mgmt_offload nrf92_wifi_offload_ops = {
	.wifi_iface.iface_api.init = nrf92_wifi_iface_init,
	.wifi_iface.get_type = nrf92_wifi_offload_get_type,
	.wifi_mgmt_api = &nrf92_wifi_mgmt_ops,
};

static int nrf92_wifi_init(const struct device *dev)
{
	return 0;
}

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(
	0,
	nrf92_wifi_init, /* init_fn */
	NULL, /* pm */
	NULL, /* data */
	NULL, /* config */
	CONFIG_WIFI_INIT_PRIORITY, /* prio */
	&nrf92_wifi_offload_ops, /* api */
	1500); /* mtu */
