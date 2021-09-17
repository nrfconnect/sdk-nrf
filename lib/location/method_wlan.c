/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <assert.h>

#include <logging/log.h>

#include <net/net_if.h>
#include <net/net_event.h>
#include <net/wifi_mgmt.h>

#include <modem/location.h>

#include "loc_core.h"
#include "loc_utils.h"

#include "rest/rest_services_wlan.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_METHOD_WLAN_SERVICE_HERE) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK),
	"At least one WLAN positioning service must be enabled");

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work_args {
	struct k_work work_item;
	struct loc_wlan_config wlan_config;
};

static struct net_if *wlan_iface;

static struct k_work_args method_wlan_start_work;

static bool running;

static uint32_t current_scan_result_count;
static uint32_t latest_scan_result_count;
struct method_wlan_scan_result {
	char mac_addr_str[WIFI_MAC_MAX_LEN + 1];
	char ssid_str[WIFI_SSID_MAX_LEN + 1];
	uint8_t channel;
	int8_t rssi;
};

static struct method_wlan_scan_result
	latest_scan_results[CONFIG_LOCATION_METHOD_WLAN_SCANNING_RESULTS_MAX_CNT];
static K_SEM_DEFINE(wlan_scanning_ready, 0, 1);

/******************************************************************************/

static int method_wlan_scanning_start(void)
{
	int ret;

	LOG_DBG("Triggering start of WLAN scanning");
	assert(wlan_iface != NULL);

	latest_scan_result_count = 0;
	current_scan_result_count = 0;

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wlan_iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to initiate WLAN scanning: %d", ret);
		return ret;
	}
	return 0;
}

/******************************************************************************/

static void method_wlan_handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	struct method_wlan_scan_result *current; 

	current_scan_result_count++;
	current = &latest_scan_results[current_scan_result_count - 1];

	if (current_scan_result_count <= CONFIG_LOCATION_METHOD_WLAN_SCANNING_RESULTS_MAX_CNT) {
		/* TODO: this seems to count that mac and ssid entries are null terminated */
		sprintf(current->mac_addr_str, "%s", entry->mac);
		sprintf(current->ssid_str, "%s", entry->ssid);
		current->channel = entry->channel;
		current->rssi = entry->rssi;

		LOG_DBG("scan result #%d stored: ssid %s, mac address: %s, channel %d,",
			current_scan_result_count,
			log_strdup(
				current->ssid_str),
			log_strdup(
				current->mac_addr_str),
			current->channel);
	} else {
		LOG_WRN("Scanning result (mac %s) did not fit to result buffer - dropping it",
			log_strdup(entry->mac));
	}
}

static void method_wlan_handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_WRN("WLAN scan request failed (%d).", status->status);
	} else {
		LOG_INF("Scan request done.");
	}
	k_sem_give(&wlan_scanning_ready);

	latest_scan_result_count =
		(current_scan_result_count > CONFIG_LOCATION_METHOD_WLAN_SCANNING_RESULTS_MAX_CNT) ?
			CONFIG_LOCATION_METHOD_WLAN_SCANNING_RESULTS_MAX_CNT :
			current_scan_result_count;
	current_scan_result_count = 0;
}

static struct net_mgmt_event_callback method_wlan_net_mgmt_cb;

void method_wlan_net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	if (running) {
		switch (mgmt_event) {
		case NET_EVENT_WIFI_SCAN_RESULT:
			method_wlan_handle_wifi_scan_result(cb);
			break;
		case NET_EVENT_WIFI_SCAN_DONE:
			method_wlan_handle_wifi_scan_done(cb);
			break;
		default:
			break;
		}
	}
}

/******************************************************************************/

static void method_wlan_positioning_work_fn(struct k_work *work)
{
	struct loc_location location_result = { 0 };
	struct k_work_args *work_data = CONTAINER_OF(work, struct k_work_args, work_item);
	const struct loc_wlan_config wlan_config = work_data->wlan_config;
	int ret;

	loc_core_timer_start(wlan_config.timeout);

	ret = method_wlan_scanning_start();
	if (ret) {
		LOG_WRN("Cannot start WLAN scanning, err %d", ret);
		loc_core_event_cb_error();
		k_sem_give(&wlan_scanning_ready);
		running = false;
		return;
	}

	k_sem_take(&wlan_scanning_ready, K_FOREVER);
	
	if (running) {
		struct rest_wlan_pos_request request;
		struct rest_wlan_pos_result result;

		if (!loc_utils_is_default_pdn_active()) {
			LOG_WRN("Default PDN context is NOT active, cannot retrieve location");
			loc_core_event_cb_error();
			running = false;
			return;
		}

		if (latest_scan_result_count > 0) {
			/* Fill scanning results: */
			request.wlan_scanning_result_count = latest_scan_result_count;
			ret = -1;

			for (int i = 0; i < latest_scan_result_count; i++) {
				strcpy(request.scanning_results[i].mac_addr_str,
				       latest_scan_results[i].mac_addr_str);
				strcpy(request.scanning_results[i].ssid_str,
				       latest_scan_results[i].ssid_str);
				request.scanning_results[i].channel =
					latest_scan_results[i].channel;
				request.scanning_results[i].rssi =
					latest_scan_results[i].rssi;
			}

			ret = rest_services_wlan_location_get(wlan_config.service, &request, &result);
			if (ret) {
				LOG_ERR("Failed to acquire location from WLAN positioning service REST api, error: %d",
					ret);
				ret = -1;
			} else {
				location_result.latitude = result.latitude;
				location_result.longitude = result.longitude;
				location_result.accuracy = result.accuracy;
				if (running) {
					loc_core_event_cb(&location_result);
					running = false;
				}
			}
		} else {
			LOG_ERR("No scanning results");
			ret = -1;
		}
	}

	if (ret) {
		loc_core_event_cb_error();
		running = false;
	}
}

/******************************************************************************/

int method_wlan_cancel(void)
{
	running = false;

	(void)k_work_cancel(&method_wlan_start_work.work_item);
	return 0;
}

int method_wlan_location_get(const struct loc_method_config *config)
{
	const struct loc_wlan_config wlan_config = config->wlan;
	bool service_ok = false;

	if (running) {
		LOG_ERR("Previous operation ongoing.");
		return -EBUSY;
	}

	/* Validate requested service: */
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_HERE)
	if (wlan_config.service == LOC_WLAN_SERVICE_HERE) {
		service_ok = true;
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK)
	if (wlan_config.service == LOC_WLAN_SERVICE_SKYHOOK) {
		service_ok = true;
	}
#endif
	if (wlan_config.service == LOC_WLAN_SERVICE_NRF_CLOUD) {
		LOG_ERR("nRF Cloud location service is not supported for WLAN.");
		return -EINVAL;
	}
	if (!service_ok) {
		LOG_ERR("Requested WLAN positioning service not configured on.");
		return -EINVAL;
	}

	k_work_init(&method_wlan_start_work.work_item, method_wlan_positioning_work_fn);
	method_wlan_start_work.wlan_config = wlan_config;
	k_work_submit_to_queue(loc_core_work_queue_get(), &method_wlan_start_work.work_item);

	running = true;

	return 0;
}

int method_wlan_init(void)
{
	wlan_iface = net_if_get_default();
	if (wlan_iface == NULL) {
		LOG_ERR("Could not get the default interface");
		return -1;
	}

	/* No possibility to check if this is really a wlan iface, thus doing
	 * some extra checking:
	 */
	if (!net_if_is_ip_offloaded(wlan_iface)) {
		LOG_ERR("Given interface not overloaded");
		return -1;
	}

	running = false;
	current_scan_result_count = 0;
	latest_scan_result_count = 0;

	net_mgmt_init_event_callback(&method_wlan_net_mgmt_cb, method_wlan_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));

	net_mgmt_add_event_callback(&method_wlan_net_mgmt_cb);
	return 0;
}
