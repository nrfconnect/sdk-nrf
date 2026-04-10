/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief P2P Group Owner (GO) mode.
 * Compiled only when CONFIG_SAMPLE_P2P_GO_MODE is set.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(p2p_go, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include <string.h>

#define WIFI_P2P_GO_MGMT_EVENTS (NET_EVENT_WIFI_AP_STA_CONNECTED |\
			      NET_EVENT_WIFI_AP_STA_DISCONNECTED)
static struct {
	const struct shell *sh;
	union {
		struct {
			uint8_t connected    : 1;
			uint8_t _unused      : 7;
		};
		uint8_t all;
	};
} context;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static K_SEM_DEFINE(wifi_p2p_peer_disconnect_sem, 0, 1);

static void handle_wifi_ap_sta_connected(struct net_mgmt_event_callback *cb)
{
	if (context.connected) {
		return;
	}

	context.connected = true;
}

static void handle_wifi_ap_sta_disconnected(struct net_mgmt_event_callback *cb)
{
	if (!context.connected) {
		return;
	}

	context.connected = false;
	k_sem_give(&wifi_p2p_peer_disconnect_sem);
}

static int configure_dhcp_server(void)
{
	struct net_if *iface;
	struct in_addr pool_start;
	int ret = -1;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		goto out;
	}

	if (net_addr_pton(AF_INET, CONFIG_SAMPLE_P2P_DHCPV4_POOL_START, &pool_start.s_addr)) {
		LOG_ERR("Invalid address: %s", CONFIG_SAMPLE_P2P_DHCPV4_POOL_START);
		goto out;
	}

	ret = net_dhcpv4_server_start(iface, &pool_start);
	if (ret == -EALREADY) {
		LOG_ERR("DHCPv4 server already running on interface");
	} else if (ret < 0) {
		LOG_ERR("DHCPv4 server failed to start and returned %d error", ret);
	} else {
		LOG_INF("DHCPv4 server started and pool address starts from %s",
			CONFIG_SAMPLE_P2P_DHCPV4_POOL_START);
	}
out:
	return ret;
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_AP_STA_CONNECTED:
		handle_wifi_ap_sta_connected(cb);
		break;
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
		handle_wifi_ap_sta_disconnected(cb);
		break;
	default:
		break;
	}
}

static int go_bssid_from_str(const char *mac_str, uint8_t mac[WIFI_MAC_ADDR_LEN])
{
	if (!mac_str || mac_str[0] == '\0') {
		return -1;
	}

	return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		      &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
		      == WIFI_MAC_ADDR_LEN ? 0 : -1;
}

static int wifi_p2p_group_add(void)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	params.oper = WIFI_P2P_GROUP_ADD;
	params.group_add.freq = CONFIG_SAMPLE_P2P_GO_FREQ;
	params.group_add.persistent = -1;
	params.group_add.vht = IS_ENABLED(CONFIG_SAMPLE_P2P_GO_VHT);
	params.group_add.go_bssid_length = 0;

	if (strlen(CONFIG_SAMPLE_P2P_GO_BSSID) > 0) {
		uint8_t bssid[WIFI_MAC_ADDR_LEN];

		if (go_bssid_from_str(CONFIG_SAMPLE_P2P_GO_BSSID, bssid) == 0) {
			memcpy(params.group_add.go_bssid, bssid, WIFI_MAC_ADDR_LEN);
			params.group_add.go_bssid_length = WIFI_MAC_ADDR_LEN;
		} else {
			LOG_WRN("Invalid P2P_GO_BSSID, using auto");
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P group add failed");
		return -1;
	}

	LOG_INF("P2P group add initiated (freq=%d persistent=%d)",
		params.group_add.freq, params.group_add.persistent);
	return 0;
}

static int wifi_p2p_group_remove(void)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	params.oper = WIFI_P2P_GROUP_REMOVE;
	strncpy(params.group_remove.ifname, CONFIG_SAMPLE_P2P_IFACE_NAME,
		CONFIG_NET_INTERFACE_NAME_LEN);
	params.group_remove.ifname[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P group remove failed");
		return -1;
	}

	LOG_INF("P2P group remove initiated: %s", params.group_remove.ifname);
	return 0;
}

static void net_mgmt_callback_init(void)
{
	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_P2P_GO_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
	k_sleep(K_SECONDS(1));
}

int p2p_go_run(void)
{
	int ret;

	context.connected = false;

	configure_dhcp_server();
	net_mgmt_callback_init();

	ret = wifi_p2p_group_add();
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Waiting for Peer to connect....");
	while (!context.connected) {
		k_sleep(K_MSEC(100));
	}
	LOG_INF("Peer connected");
	k_sleep(K_SECONDS(2));

	LOG_INF("Waiting for peer to disconnect...");
	ret = k_sem_take(&wifi_p2p_peer_disconnect_sem,
			 K_SECONDS(CONFIG_SAMPLE_P2P_PEER_DISCONN_TIMEOUT_S));
	if (ret == 0) {
		LOG_INF("Peer disconnected, removing group");
	} else if (ret == -EAGAIN) {
		LOG_WRN("Timeout waiting for disconnect, removing group");
	}

	ret = wifi_p2p_group_remove();
	if (ret < 0) {
		return ret;
	}

	return 0;
}
