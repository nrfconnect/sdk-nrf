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

#include "rest/rest_services_wifi.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK),
	"At least one WiFi positioning service must be enabled");

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work_args {
	struct k_work work_item;
	struct loc_wifi_config wifi_config;
};

static struct net_if *wifi_iface;

static struct k_work_args method_wifi_start_work;

static bool running;

static uint32_t current_scan_result_count;
static uint32_t latest_scan_result_count;

struct method_wifi_scan_result {
	char mac_addr_str[WIFI_MAC_ADDR_STR_LEN + 1];
	char ssid_str[WIFI_SSID_MAX_LEN + 1];
	uint8_t channel;
	int8_t rssi;
};

static struct method_wifi_scan_result
	latest_scan_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
static K_SEM_DEFINE(wifi_scanning_ready, 0, 1);

/******************************************************************************/

static int method_wifi_scanning_start(void)
{
	int ret;

	LOG_DBG("Triggering start of WiFi scanning");
	assert(wifi_iface != NULL);

	latest_scan_result_count = 0;
	current_scan_result_count = 0;

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to initiate WiFi scanning: %d", ret);
		return ret;
	}
	return 0;
}

/******************************************************************************/

static void method_wifi_handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	struct method_wifi_scan_result *current;

	current_scan_result_count++;
	current = &latest_scan_results[current_scan_result_count - 1];

	if (current_scan_result_count <= CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT) {
		sprintf(current->mac_addr_str,
			"%02x:%02x:%02x:%02x:%02x:%02x",
			entry->mac[0], entry->mac[1], entry->mac[2],
			entry->mac[3], entry->mac[4], entry->mac[5]);
		snprintf(current->ssid_str, entry->ssid_length + 1, "%s", entry->ssid);

		current->channel = entry->channel;
		current->rssi = entry->rssi;

		LOG_DBG("scan result #%d stored: ssid %s, mac address: %s, channel %d,",
			current_scan_result_count,
			log_strdup(current->ssid_str),
			log_strdup(current->mac_addr_str),
			current->channel);
	} else {
		LOG_WRN("Scanning result (mac %02x:%02x:%02x:%02x:%02x:%02x) "
			"did not fit to result buffer - dropping it",
				entry->mac[0], entry->mac[1], entry->mac[2],
				entry->mac[3], entry->mac[4], entry->mac[5]);
	}
}

static void method_wifi_handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_WRN("WiFi scan request failed (%d).", status->status);
	} else {
		LOG_INF("Scan request done.");
	}
	k_sem_give(&wifi_scanning_ready);

	latest_scan_result_count =
		(current_scan_result_count > CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT) ?
			CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT :
			current_scan_result_count;
	current_scan_result_count = 0;
}

static struct net_mgmt_event_callback method_wifi_net_mgmt_cb;

void method_wifi_net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	if (running) {
		switch (mgmt_event) {
		case NET_EVENT_WIFI_SCAN_RESULT:
			method_wifi_handle_wifi_scan_result(cb);
			break;
		case NET_EVENT_WIFI_SCAN_DONE:
			method_wifi_handle_wifi_scan_done(cb);
			break;
		default:
			break;
		}
	}
}

/******************************************************************************/

static void method_wifi_positioning_work_fn(struct k_work *work)
{
	struct loc_location location_result = { 0 };
	struct k_work_args *work_data = CONTAINER_OF(work, struct k_work_args, work_item);
	const struct loc_wifi_config wifi_config = work_data->wifi_config;
	int err;

	loc_core_timer_start(wifi_config.timeout);

	err = method_wifi_scanning_start();
	if (err) {
		LOG_WRN("Cannot start WiFi scanning, err %d", err);
		goto return_error;
	}

	k_sem_take(&wifi_scanning_ready, K_FOREVER);
	if (running) {
		struct rest_wifi_pos_request request;
		struct rest_wifi_pos_result result;

		if (!loc_utils_is_default_pdn_active()) {
			/* No worth to start trying to fetch with the REST api over cellular.
			 * Thus, fail faster in this case and safe the trying "costs".
			 */
			LOG_WRN("Default PDN context is NOT active, cannot retrieve a location");
			err = -1;
			goto return_error;
		}

		if (latest_scan_result_count > 1) {
			/* Fill scanning results: */
			request.wifi_scanning_result_count = latest_scan_result_count;

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

			err = rest_services_wifi_location_get(wifi_config.service, &request,
							      &result);
			if (err) {
				LOG_ERR("Failed to acquire a location by using "
					"WiFi positioning, err: %d",
						err);
				err = -1;
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
			if (latest_scan_result_count == 1) {
				/* Following statement seems to be true at least with Here
				 * (400: bad request) and also with Skyhook (404: not found).
				 * Thus, fail faster in this case and safe the data transfer costs.
				 */
				LOG_WRN("Retrieving a location based on a single WiFi access point "
					"is not possible");
			} else {
				LOG_WRN("No WiFi scanning results");
			}
			err = -1;
		}
	}
return_error:
	if (err) {
		loc_core_event_cb_error();
		running = false;
	}
}

/******************************************************************************/

int method_wifi_cancel(void)
{
	running = false;

	(void)k_work_cancel(&method_wifi_start_work.work_item);
	return 0;
}

int method_wifi_location_get(const struct loc_method_config *config)
{
	const struct loc_wifi_config wifi_config = config->wifi;
	bool service_ok = false;

	if (running) {
		LOG_ERR("Previous operation ongoing.");
		return -EBUSY;
	}

	/* Validate requested service: */
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE)
	if (wifi_config.service == LOC_WIFI_SERVICE_HERE) {
		service_ok = true;
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK)
	if (wifi_config.service == LOC_WIFI_SERVICE_SKYHOOK) {
		service_ok = true;
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD)
	if (wifi_config.service == LOC_WIFI_SERVICE_NRF_CLOUD) {
		service_ok = true;
	}
#endif
	if (!service_ok) {
		LOG_ERR("Requested WiFi positioning service not configured on.");
		return -EINVAL;
	}

	k_work_init(&method_wifi_start_work.work_item, method_wifi_positioning_work_fn);
	method_wifi_start_work.wifi_config = wifi_config;
	k_work_submit_to_queue(loc_core_work_queue_get(), &method_wifi_start_work.work_item);

	running = true;

	return 0;
}

int method_wifi_init(void)
{
	wifi_iface = net_if_get_default();
	if (wifi_iface == NULL) {
		LOG_ERR("Could not get the default interface");
		return -1;
	}

	/* No possibility to check if this is really a wifi iface, thus doing
	 * some extra checking:
	 */
	if (!net_if_is_ip_offloaded(wifi_iface)) {
		LOG_ERR("Given interface not overloaded");
		return -1;
	}

	running = false;
	current_scan_result_count = 0;
	latest_scan_result_count = 0;

	net_mgmt_init_event_callback(&method_wifi_net_mgmt_cb, method_wifi_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));

	net_mgmt_add_event_callback(&method_wifi_net_mgmt_cb);
	return 0;
}
