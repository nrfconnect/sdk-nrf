/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <modem/location.h>

#include "location_core.h"
#include "location_utils.h"
#include "cloud_service/cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

static struct net_if *wifi_iface;
static struct net_mgmt_event_callback scan_wifi_net_mgmt_cb;

static struct wifi_scan_result scan_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
static struct wifi_scan_info scan_wifi_info = {
	.ap_info = scan_results,
};
static struct k_sem *scan_wifi_ready;

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

void scan_wifi_execute(int32_t timeout, struct k_sem *wifi_scan_ready)
{
	int ret;

	scan_wifi_ready = wifi_scan_ready;

	LOG_DBG("Triggering start of Wi-Fi scanning");

	scan_wifi_info.cnt = 0;

	__ASSERT_NO_MSG(wifi_iface != NULL);
	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to initiate Wi-Fi scanning: %d", ret);
		ret = -EFAULT;
		k_sem_give(wifi_scan_ready);
		wifi_scan_ready = NULL;
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
	uint32_t mgmt_event,
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
}

int scan_wifi_cancel(void)
{
	if (scan_wifi_ready != NULL) {
		k_sem_reset(scan_wifi_ready);
		scan_wifi_ready = NULL;
	}
	return 0;
}

int scan_wifi_init(void)
{
	const struct device *wifi_dev;

	wifi_iface = NULL;
#if defined(CONFIG_WIFI_NRF700X)
	wifi_dev = device_get_binding("wlan0");
#else
	wifi_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_location_wifi));
#endif
	if (!device_is_ready(wifi_dev)) {
		LOG_ERR("Wi-Fi device %s not ready", wifi_dev->name);
		return -ENODEV;
	}

	wifi_iface = net_if_lookup_by_dev(wifi_dev);
	if (wifi_iface == NULL) {
		LOG_ERR("Could not get the Wi-Fi net interface");
		return -EFAULT;
	}

	net_mgmt_init_event_callback(&scan_wifi_net_mgmt_cb, scan_wifi_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));
	net_mgmt_add_event_callback(&scan_wifi_net_mgmt_cb);

	return 0;
}

static void scan_wifi_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Wi-Fi method specific timeout expired");

	scan_wifi_cancel();
}
