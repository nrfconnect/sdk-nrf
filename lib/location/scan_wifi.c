/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi.h>
#include <modem/location.h>
#include <net/wifi_ready.h>

#include "location_core.h"
#include "location_utils.h"
#include "cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT((int)LOCATION_WIFI_SCAN_TYPE_ACTIVE == (int)WIFI_SCAN_TYPE_ACTIVE);
BUILD_ASSERT((int)LOCATION_WIFI_SCAN_TYPE_PASSIVE == (int)WIFI_SCAN_TYPE_PASSIVE);
BUILD_ASSERT(LOCATION_WIFI_BAND_2_4_GHZ == BIT(WIFI_FREQ_BAND_2_4_GHZ));
BUILD_ASSERT(LOCATION_WIFI_BAND_5_GHZ == BIT(WIFI_FREQ_BAND_5_GHZ));

static struct net_if *wifi_iface;
static struct net_mgmt_event_callback scan_wifi_net_mgmt_cb;

static struct wifi_scan_result scan_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
static struct wifi_scan_info scan_wifi_info = {
	.ap_info = scan_results,
};
static struct k_sem *scan_wifi_ready;

#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
/* Timeout for waiting for Wi-Fi to be ready, max 10s in nRF70 + buffer */
#define WIFI_READY_TIMEOUT_SEC 15

static K_SEM_DEFINE(wifi_ready_sem, 0, 1);

static void scan_wifi_ready_cb(bool ready)
{
	if (ready) {
		LOG_DBG("Wi-Fi is ready");
		k_sem_give(&wifi_ready_sem);
	} else {
		LOG_DBG("Wi-Fi is not ready");
		k_sem_reset(&wifi_ready_sem);
	}
}

static wifi_ready_callback_t scan_wifi_ready_cb_data = {
	.wifi_ready_cb = scan_wifi_ready_cb,
};
#endif /* defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN) */

/** Handler for timeout. */
static void scan_wifi_timeout_work_fn(struct k_work *work);

/** Work item for timeout handler. */
K_WORK_DELAYABLE_DEFINE(scan_wifi_timeout_work, scan_wifi_timeout_work_fn);

struct wifi_scan_info *scan_wifi_results_get(void)
{
	if (scan_wifi_info.cnt  <= 1) {
		if (scan_wifi_info.cnt == 1) {
			/* Following statement seems to be true at least with HERE
			 * (400: bad request).
			 */
			LOG_WRN("Retrieving a location based on a single Wi-Fi "
				"access point is not possible, using only cellular data");
		} else {
			LOG_WRN("No Wi-Fi scanning results, using only cellular data");
		}
		return NULL;
	}

	return &scan_wifi_info;
}

#if defined(CONFIG_LOCATION_DATA_DETAILS)
void scan_wifi_details_get(struct location_data_details *details)
{
	details->wifi.ap_count = scan_wifi_info.cnt;
}
#endif

#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
static int scan_wifi_interface_shutdown(struct net_if *iface)
{
	int ret;

	LOG_DBG("Shutting down Wi-Fi interface");

	if (!net_if_is_admin_up(iface)) {
		return 0;
	}

	ret = net_if_down(iface);
	if (ret) {
		LOG_ERR("Cannot bring down Wi-Fi interface (%d)", ret);
		return ret;
	}

	LOG_DBG("Interface down");

	return 0;
}

static int scan_wifi_startup_interface(struct net_if *iface)
{
	int ret;

	LOG_DBG("Starting up Wi-Fi interface");

	if (!net_if_is_admin_up(iface)) {
		ret = net_if_up(iface);
		if (ret) {
			LOG_ERR("Cannot bring up Wi-Fi interface (%d)", ret);
			return ret;
		}
		LOG_DBG("Interface up");
	}

	/* Wait for Wi-Fi to be ready before scanning */
	LOG_DBG("Waiting for Wi-Fi to be ready (timeout: %d sec)",
		WIFI_READY_TIMEOUT_SEC);
	ret = k_sem_take(&wifi_ready_sem, K_SECONDS(WIFI_READY_TIMEOUT_SEC));
	if (ret) {
		LOG_ERR("Timeout waiting for Wi-Fi to be ready (%d)", ret);
		return -ETIMEDOUT;
	}

	LOG_DBG("Wi-Fi interface is ready");
	return 0;
}
#endif /* defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN) */

static void scan_wifi_params_set(struct wifi_scan_params *scan_params,
				 const struct location_wifi_config *config)
{
	int i;

	scan_params->scan_type = (enum wifi_scan_type)config->scan_type;
	scan_params->bands = config->bands;
	scan_params->dwell_time_active = config->dwell_time_active;
	scan_params->dwell_time_passive = config->dwell_time_passive;
	scan_params->max_bss_cnt = config->max_bss_cnt;

#if defined(CONFIG_LOCATION_METHOD_WIFI_SCANNING_CHANNELS_MAX) && \
	CONFIG_LOCATION_METHOD_WIFI_SCANNING_CHANNELS_MAX > 0
	for (i = 0; i < CONFIG_LOCATION_METHOD_WIFI_SCANNING_CHANNELS_MAX; i++) {
		if (config->channels[i].channel == 0) {
			break;
		}

		scan_params->band_chan[i].band = config->channels[i].band;
		scan_params->band_chan[i].channel = config->channels[i].channel;
	}
#endif
}

void scan_wifi_execute(const struct location_wifi_config *config, struct k_sem *wifi_scan_ready)
{
	int ret;
	int32_t timeout;
	struct wifi_scan_params scan_params = {0};

	__ASSERT_NO_MSG(config != NULL);

	scan_wifi_ready = wifi_scan_ready;
	timeout = config->timeout;

	LOG_DBG("Triggering start of Wi-Fi scanning");

	scan_wifi_info.cnt = 0;

	__ASSERT_NO_MSG(wifi_iface != NULL);

#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
	ret = scan_wifi_startup_interface(wifi_iface);
	if (ret) {
		LOG_ERR("Wi-Fi interface startup failed, cannot scan");
		k_sem_give(scan_wifi_ready);
		scan_wifi_ready = NULL;
		return;
	}
#endif /* defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN) */

	scan_wifi_params_set(&scan_params, config);

	LOG_DBG("Scan params: type=%d, bands=0x%x, dwell_active=%u, dwell_passive=%u, "
		"max_bss=%u",
		scan_params.scan_type, scan_params.bands, scan_params.dwell_time_active,
		scan_params.dwell_time_passive, scan_params.max_bss_cnt);

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, &scan_params, sizeof(scan_params));
	if (ret) {
		LOG_ERR("Failed to initiate Wi-Fi scanning: %d", ret);
		k_sem_give(scan_wifi_ready);
		scan_wifi_ready = NULL;
		return;
	}

	if (timeout != SYS_FOREVER_MS && timeout > 0) {
		LOG_DBG("Starting Wi-Fi timer with timeout=%d", timeout);
		k_work_schedule(&scan_wifi_timeout_work, K_MSEC(timeout));
	}
}

static void scan_wifi_result_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	struct wifi_scan_result *current;

	if (scan_wifi_info.cnt < CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT) {
		current = &scan_wifi_info.ap_info[scan_wifi_info.cnt];
		*current = *entry;
		scan_wifi_info.cnt++;

		LOG_DBG("scan result #%d stored: ssid %s, channel %d,"
			" mac %02x:%02x:%02x:%02x:%02x:%02x",
				scan_wifi_info.cnt,
				current->ssid,
				current->channel,
				current->mac[0], current->mac[1], current->mac[2],
				current->mac[3], current->mac[4], current->mac[5]);
	} else {
		LOG_WRN("Scanning result (mac %02x:%02x:%02x:%02x:%02x:%02x) "
			"did not fit to result buffer - dropping it",
				entry->mac[0], entry->mac[1], entry->mac[2],
				entry->mac[3], entry->mac[4], entry->mac[5]);
	}
}

static void scan_wifi_done_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_WRN("Wi-Fi scan request failed (%d)", status->status);
	} else {
		LOG_DBG("Scan request done with %d Wi-Fi APs", scan_wifi_info.cnt);
	}

	k_sem_give(scan_wifi_ready);
	scan_wifi_ready = NULL;
	k_work_cancel_delayable(&scan_wifi_timeout_work);
}

void scan_wifi_net_mgmt_event_handler(
	struct net_mgmt_event_callback *cb,
	uint64_t mgmt_event,
	struct net_if *iface)
{
	ARG_UNUSED(iface);

	if (scan_wifi_ready != NULL) {
		switch (mgmt_event) {
		case NET_EVENT_WIFI_SCAN_RESULT:
			scan_wifi_result_handle(cb);
			break;
		case NET_EVENT_WIFI_SCAN_DONE:
			scan_wifi_done_handle(cb);
			break;
		default:
			break;
		}
	}
#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
	/* This is a workaround for devices like the Thingy:91 X.
	 * Usually, libraries should not shut down the interface in case there might be other
	 * users of the device.
	 */
	if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
		scan_wifi_interface_shutdown(wifi_iface);
	}
#endif /* defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN) */
}

int scan_wifi_cancel(void)
{
	if (scan_wifi_ready != NULL) {
		k_sem_give(scan_wifi_ready);
		scan_wifi_ready = NULL;
	}
	return 0;
}

int scan_wifi_init(void)
{
	const struct device *wifi_dev;
#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
	int ret;
#endif

	wifi_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	if (!wifi_dev) {
		LOG_ERR("Wi-Fi device not found");
		return -ENODEV;
	}

	if (!device_is_ready(wifi_dev)) {
		LOG_ERR("Wi-Fi device %s not ready", wifi_dev->name);
		return -ENODEV;
	}

	wifi_iface = net_if_lookup_by_dev(wifi_dev);
	if (wifi_iface == NULL) {
		LOG_ERR("No Wi-Fi interface found: %s", wifi_dev->name);
		return -EFAULT;
	}

	net_mgmt_init_event_callback(&scan_wifi_net_mgmt_cb, scan_wifi_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));
	net_mgmt_add_event_callback(&scan_wifi_net_mgmt_cb);

#if defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN)
	scan_wifi_ready_cb_data.iface = wifi_iface;
	ret = register_wifi_ready_callback(scan_wifi_ready_cb_data, wifi_iface);
	if (ret) {
		LOG_ERR("Failed to register Wi-Fi ready callback (%d)", ret);
		return ret;
	}

	return scan_wifi_interface_shutdown(wifi_iface);
#else
	return 0;
#endif /* defined(CONFIG_LOCATION_METHOD_WIFI_NET_IF_UPDOWN) */
}

static void scan_wifi_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Wi-Fi method specific timeout expired");

	scan_wifi_cancel();
}
