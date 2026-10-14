/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include "message_channel.h"

/* Register log module */
LOG_MODULE_REGISTER(network, CONFIG_MQTT_SAMPLE_NETWORK_LOG_LEVEL);

/* This module does not subscribe to any channels */

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

static void l4_event_handler(uint64_t event, struct net_if *iface, void *info,
			     size_t info_length, void *user_data)
{
	int err;
	enum network_status status;

	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		status = NETWORK_CONNECTED;
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		status = NETWORK_DISCONNECTED;
		break;
	default:
		/* Don't care */
		return;
	}

	err = zbus_chan_pub(&NETWORK_CHAN, &status, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

static void connectivity_event_handler(uint64_t event, struct net_if *iface, void *info,
				       size_t info_length, void *user_data)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		SEND_FATAL_ERROR();
		return;
	}
}

/* Registered statically to not miss events raised before the module starts */
NET_MGMT_REGISTER_EVENT_HANDLER(l4_events, L4_EVENT_MASK, l4_event_handler, NULL);
NET_MGMT_REGISTER_EVENT_HANDLER(conn_events, CONN_LAYER_EVENT_MASK, connectivity_event_handler,
				NULL);

static void network_task(void)
{
	int err;

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		SEND_FATAL_ERROR();
		return;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		SEND_FATAL_ERROR();
		return;
	}
}

K_THREAD_DEFINE(network_task_id,
		CONFIG_MQTT_SAMPLE_NETWORK_THREAD_STACK_SIZE,
		network_task, NULL, NULL, NULL, 3, 0, 0);
