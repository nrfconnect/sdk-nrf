/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <net/wifi_mgmt_ext.h>
#include <modem/lte_lc.h>

#include "network_common.h"

LOG_MODULE_REGISTER(network, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

#define MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback net_mgmt_callback;
static struct net_mgmt_event_callback net_mgmt_ipv4_callback;

static void connect(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("Returned network interface is NULL");
		return;
	}

	int err = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (err) {
		LOG_ERR("Connecting to Wi-Fi failed. error: %d", err);
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	const struct wifi_status *wifi_status = (const struct wifi_status *)cb->info;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		if (wifi_status->status) {
			LOG_INF("Connection attempt failed, status code: %d", wifi_status->status);
			return;
		}

		LOG_INF("Wi-Fi Connected, waiting for IP address");

		return;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("Disconnected");
		network_disconnected();
		break;
	default:
		LOG_ERR("Unknown event: %d", mgmt_event);
		return;
	}
}

static void ipv4_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t event, struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_IPV4_ADDR_ADD:
		LOG_INF("IPv4 address acquired");
		network_connected();
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		LOG_INF("IPv4 address lost");
		network_disconnected();
		break;
	default:
		LOG_DBG("Unknown event: 0x%08X", event);
		return;
	}
}

void network_init(void)
{
	net_mgmt_init_event_callback(&net_mgmt_callback, wifi_mgmt_event_handler, MGMT_EVENTS);
	net_mgmt_add_event_callback(&net_mgmt_callback);
	net_mgmt_init_event_callback(&net_mgmt_ipv4_callback, ipv4_mgmt_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&net_mgmt_ipv4_callback);

	/* Add temporary fix to prevent using Wi-Fi before WPA supplicant is ready. */
	k_sleep(K_SECONDS(1));

	if (IS_ENABLED(CONFIG_WIFI_CREDENTIALS_STATIC)) {
		connect();
	}
}
