/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include "message_channel.h"

/* Register log module */
LOG_MODULE_REGISTER(network, CONFIG_MQTT_SAMPLE_NETWORK_LOG_LEVEL);

/* This module does not subscribe to any channels */

BUILD_ASSERT((sizeof(CONFIG_MQTT_SAMPLE_NETWORK_WIFI_SSID) > 1), "SSID must be set");
BUILD_ASSERT((sizeof(CONFIG_MQTT_SAMPLE_NETWORK_WIFI_PSK) > 1), "PSK must be set");

#define MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback net_mgmt_callback;

static void connect(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("Returned network interface is NULL");
		SEND_FATAL_ERROR();
		return;
	}

	LOG_INF("Connecting to SSID: %s", CONFIG_MQTT_SAMPLE_NETWORK_WIFI_SSID);

	static struct wifi_connect_req_params cnx_params = {
		.ssid = CONFIG_MQTT_SAMPLE_NETWORK_WIFI_SSID,
		.ssid_length = sizeof(CONFIG_MQTT_SAMPLE_NETWORK_WIFI_SSID) - 1,
		.psk = CONFIG_MQTT_SAMPLE_NETWORK_WIFI_PSK,
		.psk_length = sizeof(CONFIG_MQTT_SAMPLE_NETWORK_WIFI_PSK) - 1,
		.channel = WIFI_CHANNEL_ANY,
		.security = WIFI_SECURITY_TYPE_PSK,
		.timeout = CONFIG_MQTT_SAMPLE_NETWORK_WIFI_CONNECTION_REQUEST_TIMEOUT_SECONDS,
	};

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Connecting to Wi-Fi failed");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	int err;
	enum network_status status;
	const struct wifi_status *wifi_status = (const struct wifi_status *)cb->info;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		if (wifi_status->status) {
			LOG_INF("Connection attempt failed, status code: %d", wifi_status->status);
			return;
		}

		LOG_INF("Wi-Fi Connected");

		status = NETWORK_CONNECTED;
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("Disconnected");

		status = NETWORK_DISCONNECTED;
		break;
	default:
		LOG_ERR("Unknown event: %d", mgmt_event);
		return;
	}

	err = zbus_chan_pub(&NETWORK_CHAN, &status, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

static void network_task(void)
{
	net_mgmt_init_event_callback(&net_mgmt_callback, wifi_mgmt_event_handler, MGMT_EVENTS);
	net_mgmt_add_event_callback(&net_mgmt_callback);

	connect();
}

K_THREAD_DEFINE(network_task_id,
		CONFIG_MQTT_SAMPLE_NETWORK_THREAD_STACK_SIZE,
		network_task, NULL, NULL, NULL, 3, 0, 0);
