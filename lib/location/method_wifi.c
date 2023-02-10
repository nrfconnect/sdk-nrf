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

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_SERVICE_NRF_CLOUD) ||
	IS_ENABLED(CONFIG_LOCATION_SERVICE_HERE) ||
	IS_ENABLED(CONFIG_LOCATION_SERVICE_EXTERNAL),
	"At least one Wi-Fi positioning service, or handling the service externally "
	"must be enabled");

struct method_wifi_start_work_args {
	struct k_work work_item;
	struct location_wifi_config wifi_config;
	int64_t starting_uptime_ms;
};

static struct net_if *wifi_iface;

static struct method_wifi_start_work_args method_wifi_start_work;

static bool running;

static uint32_t current_scan_result_count;
static uint32_t latest_scan_result_count;

static struct wifi_scan_result
	latest_scan_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
static struct wifi_scan_info latest_wifi_info = {
	.ap_info = latest_scan_results,
};

static K_SEM_DEFINE(wifi_scanning_ready, 0, 1);

/******************************************************************************/

static int method_wifi_scanning_start(void)
{
	int ret;

	LOG_DBG("Triggering start of Wi-Fi scanning");

	latest_scan_result_count = 0;
	current_scan_result_count = 0;

	__ASSERT_NO_MSG(wifi_iface != NULL);
	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to initiate Wi-Fi scanning: %d", ret);
		ret = -EFAULT;
	}
	return ret;
}

/******************************************************************************/

static void method_wifi_scan_result_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	struct wifi_scan_result *current;

	current_scan_result_count++;

	if (current_scan_result_count <= CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT) {
		current = &latest_scan_results[current_scan_result_count - 1];
		*current = *entry;

		LOG_DBG("scan result #%d stored: ssid %s, channel %d,"
			" mac %02x:%02x:%02x:%02x:%02x:%02x",
				current_scan_result_count,
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

static void method_wifi_scan_done_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_WRN("Wi-Fi scan request failed (%d).", status->status);
	} else {
		LOG_DBG("Scan request done.");
	}

	latest_scan_result_count =
		(current_scan_result_count > CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT) ?
			CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT :
			current_scan_result_count;
	current_scan_result_count = 0;
	k_sem_give(&wifi_scanning_ready);
}

static struct net_mgmt_event_callback method_wifi_net_mgmt_cb;

void method_wifi_net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	ARG_UNUSED(iface);

	if (running) {
		switch (mgmt_event) {
		case NET_EVENT_WIFI_SCAN_RESULT:
			method_wifi_scan_result_handle(cb);
			break;
		case NET_EVENT_WIFI_SCAN_DONE:
			method_wifi_scan_done_handle(cb);
			break;
		default:
			break;
		}
	}
}

/******************************************************************************/

static void method_wifi_positioning_work_fn(struct k_work *work)
{
	struct location_data location_result = { 0 };
	struct method_wifi_start_work_args *work_data =
		CONTAINER_OF(work, struct method_wifi_start_work_args, work_item);
	struct cloud_service_pos_req request = { 0 };
	const struct location_wifi_config wifi_config = work_data->wifi_config;
	int64_t starting_uptime_ms = work_data->starting_uptime_ms;
	int err;

	location_core_timer_start(wifi_config.timeout);

	err = method_wifi_scanning_start();
	if (err) {
		LOG_WRN("Cannot start Wi-Fi scanning, err %d", err);
		goto end;
	}

	k_sem_take(&wifi_scanning_ready, K_FOREVER);
	if (!running) {
		goto end;
	}
	/* Stop the timer and let rest_client timer handle the request */
	location_core_timer_stop();

	/* Scanning done at this point of time. Store current time to response. */
	location_utils_systime_to_location_datetime(&location_result.datetime);

	if (!location_utils_is_default_pdn_active()) {
		/* Not worth to start trying to fetch with the REST api over cellular.
		 * Thus, fail faster in this case and save the trying "costs".
		 */
		LOG_WRN("Default PDN context is NOT active, cannot retrieve a location");
		err = -EFAULT;
		goto end;
	}
	if (latest_scan_result_count > 1) {

		request.timeout_ms = wifi_config.timeout;
		if (wifi_config.timeout != SYS_FOREVER_MS) {
			/* Calculate remaining time as a REST timeout (in milliseconds) */
			request.timeout_ms = (wifi_config.timeout -
				(k_uptime_get() - starting_uptime_ms));
			if (request.timeout_ms < 0) {
				/* No remaining time at all */
				LOG_WRN("No remaining time left for requesting a position");
				err = -ETIMEDOUT;
				goto end;
			}
		}

		/* Fill scanning results: */
		latest_wifi_info.cnt = latest_scan_result_count;
		request.wifi_data = &latest_wifi_info;

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
		location_core_event_cb_wifi_request(&latest_wifi_info);
#else
		struct location_data result;

		request.service = wifi_config.service;

		err = cloud_service_location_get(&request, &result);
		if (err) {
			LOG_ERR("Failed to acquire a location by using "
				"Wi-Fi positioning, err: %d",
				err);
			err = -ENODATA;
		} else {
			location_result.latitude = result.latitude;
			location_result.longitude = result.longitude;
			location_result.accuracy = result.accuracy;
			if (running) {
				running = false;
				location_core_event_cb(&location_result);
			}
		}
#endif
	} else {
		if (latest_scan_result_count == 1) {
			/* Following statement seems to be true at least with HERE
			 * (400: bad request).
			 * Thus, fail faster in this case and save the data transfer costs.
			 */
			LOG_WRN("Retrieving a location based on a single Wi-Fi "
				"access point is not possible");
		} else {
			LOG_WRN("No Wi-Fi scanning results");
		}
		err = -EFAULT;
	}
end:
	if (err == -ETIMEDOUT) {
		location_core_event_cb_timeout();
		running = false;
	} else if (err) {
		location_core_event_cb_error();
		running = false;
	}
}

/******************************************************************************/

int method_wifi_cancel(void)
{
	running = false;

	(void)k_work_cancel(&method_wifi_start_work.work_item);
	k_sem_reset(&wifi_scanning_ready);
	return 0;
}

int method_wifi_location_get(const struct location_method_config *config)
{
	k_work_init(&method_wifi_start_work.work_item, method_wifi_positioning_work_fn);
	method_wifi_start_work.wifi_config = config->wifi;
	method_wifi_start_work.starting_uptime_ms = k_uptime_get();
	k_work_submit_to_queue(location_core_work_queue_get(), &method_wifi_start_work.work_item);

	running = true;

	return 0;
}

int method_wifi_init(void)
{
	running = false;
	current_scan_result_count = 0;
	latest_scan_result_count = 0;
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

	net_mgmt_init_event_callback(&method_wifi_net_mgmt_cb, method_wifi_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));
	net_mgmt_add_event_callback(&method_wifi_net_mgmt_cb);

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	cloud_service_init();
#endif

	return 0;
}
