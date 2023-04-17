/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME net_lwm2m_location_wifi_ap_scanner
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>

#include "ground_fix_obj.h"

#define VISIBLE_WIFI_AP_MAC_ADDRESS_ID		0
#define VISIBLE_WIFI_AP_CHANNEL_ID		1
#define VISIBLE_WIFI_AP_FREQUENCY_ID		2
#define VISIBLE_WIFI_AP_SIGNAL_STRENGTH_ID	3
#define VISIBLE_WIFI_AP_SSID_ID			4

static struct net_if *wifi_iface;

static uint32_t current_scan_result_count;

static void lwm2m_wifi_scan_result_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	struct lwm2m_obj_path path = LWM2M_OBJ(33627, 0, 0);

	LOG_DBG("scan result #%d stored: ssid %s, channel %d, rssi %d,"
		" mac %02x:%02x:%02x:%02x:%02x:%02x",
			current_scan_result_count,
			entry->ssid,
			entry->channel,
			entry->rssi,
			entry->mac[0], entry->mac[1], entry->mac[2],
			entry->mac[3], entry->mac[4], entry->mac[5]);

	if (current_scan_result_count < CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_INSTANCE_COUNT) {
		path.obj_inst_id = current_scan_result_count;
		path.res_id = VISIBLE_WIFI_AP_MAC_ADDRESS_ID;
		lwm2m_set_opaque(&path, entry->mac, 6);

		path.res_id = VISIBLE_WIFI_AP_CHANNEL_ID;
		lwm2m_set_s32(&path, entry->channel);

		path.res_id = VISIBLE_WIFI_AP_FREQUENCY_ID;
		lwm2m_set_s32(&path, entry->band);

		path.res_id = VISIBLE_WIFI_AP_SIGNAL_STRENGTH_ID;
		lwm2m_set_s32(&path, entry->rssi);

		path.res_id = VISIBLE_WIFI_AP_SSID_ID;
		lwm2m_set_string(&path, entry->ssid);
	}

	current_scan_result_count++;
}

static void lwm2m_wifi_scan_done_handle(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_WRN("Wi-Fi scan request failed (%d).", status->status);
	} else {
		LOG_DBG("Scan request done, found %d access points", current_scan_result_count);
	}
}

static struct net_mgmt_event_callback lwm2m_wifi_net_mgmt_cb;

void lwm2m_wifi_net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	ARG_UNUSED(iface);

	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		lwm2m_wifi_scan_result_handle(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		lwm2m_wifi_scan_done_handle(cb);
		break;
	default:
		break;
	}
}

int lwm2m_wifi_request_scan(void)
{
	int ret = 0;

	current_scan_result_count = 0;
	LOG_DBG("Schedule scan");

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to initiate Wi-Fi scanning: %d", ret);
		ret = -EFAULT;
	}
	return ret;
}

static int lwm2m_wifi_scan_init(void)
{
	const struct device *wifi_dev;

	wifi_iface = NULL;
	wifi_dev = device_get_binding("wlan0");
	LOG_DBG("Initialize wifi scan module");

	if (!device_is_ready(wifi_dev)) {
		LOG_ERR("Wi-Fi device %s not ready", wifi_dev->name);
		return -ENODEV;
	}

	wifi_iface = net_if_lookup_by_dev(wifi_dev);
	if (wifi_iface == NULL) {
		LOG_ERR("Could not get the Wi-Fi net interface");
		return -EFAULT;
	}

	net_mgmt_init_event_callback(&lwm2m_wifi_net_mgmt_cb, lwm2m_wifi_net_mgmt_event_handler,
				     (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE));
	net_mgmt_add_event_callback(&lwm2m_wifi_net_mgmt_cb);
	return 0;
}

SYS_INIT(lwm2m_wifi_scan_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
