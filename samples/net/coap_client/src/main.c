/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/random/random.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(coap_client_sample, CONFIG_COAP_CLIENT_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

/* Variable used to indicate if network is connected. */
static bool is_connected;

/* Mutex and conditional variable used to signal network connectivity. */
K_MUTEX_DEFINE(network_connected_lock);
K_CONDVAR_DEFINE(network_connected);

static int server_resolve(struct sockaddr_storage *server)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};
	char ipv4_addr[NET_IPV4_ADDR_LEN];

	err = getaddrinfo(CONFIG_COAP_SAMPLE_SERVER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo, error: %d", err);
		return err;
	}

	if (result == NULL) {
		LOG_ERR("Address not found");
		return -ENOENT;
	}

	/* IPv4 Address. */
	struct sockaddr_in *server4 = ((struct sockaddr_in *)server);

	server4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_COAP_SAMPLE_SERVER_PORT);

	inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr, sizeof(ipv4_addr));

	LOG_INF("IPv4 Address found %s", ipv4_addr);

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

static void wait_for_network(void)
{
	k_mutex_lock(&network_connected_lock, K_FOREVER);

	if (!is_connected) {
		LOG_INF("Waiting for network connectivity");
		k_condvar_wait(&network_connected, &network_connected_lock, K_FOREVER);
	}

	k_mutex_unlock(&network_connected_lock);
}

static void response_cb(int16_t code, size_t offset, const uint8_t *payload,
			size_t len, bool last_block, void *user_data)
{
	if (code >= 0) {
		LOG_INF("CoAP response: code: 0x%x, payload: %s", code, payload);
	} else {
		LOG_INF("Response received with error code: %d", code);
	}
}

static int periodic_coap_request_loop(void)
{
	int err, sock;
	struct sockaddr_storage server = { 0 };
	struct coap_client coap_client = { 0 };
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.payload = NULL,
		.cb = response_cb,
		.len = 0,
		.path = CONFIG_COAP_SAMPLE_RESOURCE,
	};

	err = server_resolve(&server);
	if (err) {
		LOG_ERR("Failed to resolve server name");
		return err;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create CoAP socket: %d.", -errno);
		return -errno;
	}

	LOG_INF("Initializing CoAP client");

	err = coap_client_init(&coap_client, NULL);
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}

	while (true) {
		wait_for_network();

		/* Send request */
		err = coap_client_req(&coap_client, sock, (struct sockaddr *)&server, &req, NULL);
		if (err) {
			LOG_ERR("Failed to send request: %d", err);
			return err;
		}

		LOG_INF("CoAP GET request sent sent to %s, resource: %s",
			CONFIG_COAP_SAMPLE_SERVER_HOSTNAME, CONFIG_COAP_SAMPLE_RESOURCE);

		k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		k_mutex_lock(&network_connected_lock, K_FOREVER);
		is_connected = true;
		k_condvar_signal(&network_connected);
		k_mutex_unlock(&network_connected_lock);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		k_mutex_lock(&network_connected_lock, K_FOREVER);
		is_connected = false;
		k_mutex_unlock(&network_connected_lock);
		break;
	default:
		/* Don't care */
		return;
	}
}
static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
						uint32_t event,
						struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

int main(void)
{
	int err;

	LOG_INF("The CoAP client sample started");

	/* Setup handler for Zephyr NET Connection Manager events and Connectivity layer. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Bring all network interfaces up.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	wait_for_network();

	err = periodic_coap_request_loop();
	if (err) {
		LOG_ERR("periodic_coap_request_loop, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	return 0;
}
