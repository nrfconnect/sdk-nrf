/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief P2P client (CLI) mode - find peer and connect.
 * Compiled only when CONFIG_SAMPLE_P2P_CLI_MODE is set.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(p2p_cli, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#define WIFI_P2P_CLI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT |\
				  NET_EVENT_WIFI_P2P_DEVICE_FOUND)

static struct {
	const struct shell *sh;
	union {
		struct {
			uint8_t connected	: 1;
			uint8_t _unused		: 7;
		};
		uint8_t all;
	};
} context;

static K_SEM_DEFINE(wifi_p2p_device_found_sem, 0, 1);
static K_SEM_DEFINE(ip_ready_sem, 0, 1);
static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback net_shell_mgmt_cb;

static int mac_str_to_bytes(const char *mac_str, uint8_t mac[WIFI_MAC_ADDR_LEN])
{
	return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		     &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
	       == WIFI_MAC_ADDR_LEN ? 0 : -1;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	if (context.connected) {
		return;
	}

	LOG_INF("Connected");
	context.connected = true;
}

static void handle_wifi_p2p_device_found(struct net_mgmt_event_callback *cb)
{
	const struct wifi_p2p_device_info *peer_info =
		(const struct wifi_p2p_device_info *)cb->info;
	uint8_t peer_addr[WIFI_MAC_ADDR_LEN];

	if (!peer_info) {
		return;
	}

	if (mac_str_to_bytes(CONFIG_SAMPLE_P2P_PEER_ADDRESS, peer_addr)) {
		LOG_ERR("Invalid peer MAC in CONFIG_SAMPLE_P2P_PEER_ADDRESS: %s",
			CONFIG_SAMPLE_P2P_PEER_ADDRESS);
		return;
	}

	if (peer_info->mac[0] == peer_addr[0] &&
	    peer_info->mac[1] == peer_addr[1] &&
	    peer_info->mac[2] == peer_addr[2] &&
	    peer_info->mac[3] == peer_addr[3] &&
	    peer_info->mac[4] == peer_addr[4] &&
	    peer_info->mac[5] == peer_addr[5]) {
		k_sem_give(&wifi_p2p_device_found_sem);
	}
}

static void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

	LOG_INF("DHCP IP address: %s", dhcp_info);
	k_sem_give(&ip_ready_sem);
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				   uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(iface);
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_P2P_DEVICE_FOUND:
		handle_wifi_p2p_device_found(cb);
		break;
	default:
		break;
	}
}

static void net_mgmt_callback_init(void)
{
	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_P2P_CLI_MGMT_EVENTS);
	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&net_shell_mgmt_cb,
				     net_mgmt_event_handler,
				     NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&net_shell_mgmt_cb);

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
	k_sleep(K_SECONDS(1));
}

static int wifi_p2p_find(void)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	params.oper = WIFI_P2P_FIND;
#ifdef CONFIG_SAMPLE_P2P_DISCOVERY_FULL
	params.discovery_type = WIFI_P2P_FIND_START_WITH_FULL;
#elif defined(CONFIG_SAMPLE_P2P_DISCOVERY_SOCIAL)
	params.discovery_type = WIFI_P2P_FIND_ONLY_SOCIAL;
#elif defined(CONFIG_SAMPLE_P2P_DISCOVERY_PROGRESSIVE)
	params.discovery_type = WIFI_P2P_FIND_PROGRESSIVE;
#endif
	params.timeout = CONFIG_SAMPLE_P2P_FIND_TIMEOUT_S;

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P find request failed");
		return -1;
	}

	LOG_INF("P2P find started");
	return 0;
}

static int wifi_p2p_connect(void)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	if (mac_str_to_bytes(CONFIG_SAMPLE_P2P_PEER_ADDRESS, params.peer_addr) != 0) {
		LOG_ERR("Invalid peer address: %s", CONFIG_SAMPLE_P2P_PEER_ADDRESS);
		return -1;
	}

#ifdef CONFIG_SAMPLE_P2P_CONNECTION_METHOD_PBC
	params.connect.method = WIFI_P2P_METHOD_PBC;
#elif defined(CONFIG_SAMPLE_P2P_CONNECTION_METHOD_PIN)
#ifdef CONFIG_SAMPLE_P2P_METHOD_KEYPAD
	params.connect.method = WIFI_P2P_METHOD_KEYPAD;
	strncpy(params.connect.pin,
		CONFIG_SAMPLE_P2P_CONNECTION_METHOD_PIN_VALUE, WIFI_WPS_PIN_MAX_LEN);
	params.connect.pin[WIFI_WPS_PIN_MAX_LEN] = '\0';
#elif defined(CONFIG_SAMPLE_P2P_METHOD_DISPLAY)
	params.connect.method = WIFI_P2P_METHOD_DISPLAY;
	params.connect.pin[0] = '\0';
#endif
#else
	LOG_ERR("Invalid connection method");
	return -1;
#endif
	params.connect.go_intent = CONFIG_SAMPLE_P2P_GO_INTENT_VALUE;
	params.connect.freq = CONFIG_SAMPLE_P2P_FREQUENCY;
#ifdef CONFIG_SAMPLE_P2P_JOIN
	params.connect.join = true;
#else
	params.connect.join = false;
#endif
	params.oper = WIFI_P2P_CONNECT;

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_WRN("P2P connect request failed");
		return -1;
	}

	if (params.connect.method == WIFI_P2P_METHOD_DISPLAY && params.connect.pin[0] != '\0') {
		LOG_INF("PIN: %s", params.connect.pin);
	} else {
		LOG_INF("P2P connection initiated");
	}

	return 0;
}

int p2p_cli_run(void)
{
	int ret;

	context.connected = false;

	net_mgmt_callback_init();

	while (1) {
		ret = wifi_p2p_find();
		if (ret < 0) {
			return ret;
		}

		ret = k_sem_take(&wifi_p2p_device_found_sem,
				 K_SECONDS(CONFIG_SAMPLE_P2P_FIND_TIMEOUT_S));
		if (ret == 0) {
			LOG_INF("Peer found, initializing connection");

			ret = wifi_p2p_connect();
			if (ret < 0) {
				LOG_ERR("P2P connect failed");
				return ret;
			}

			while (!context.connected) {
				k_sleep(K_MSEC(100));
			}

			ret = k_sem_take(&ip_ready_sem,
					 K_SECONDS(CONFIG_SAMPLE_P2P_DHCP_TIMEOUT_S));
			if (ret) {
				LOG_ERR("DHCP is not successful : %d", ret);
				return ret;
			}

			LOG_INF("Successfully connected to Peer");
			break;
		} else {
			LOG_INF("No peer found, retrying in 10 seconds...");
			k_sleep(K_SECONDS(10));
		}
	}

	return 0;
}
