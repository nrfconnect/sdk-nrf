/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi connection helper file
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_connect, CONFIG_LOG_DEFAULT_LEVEL);

#include <errno.h>
#include <stdlib.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi.h>
#include "net_private.h"

#ifdef CONFIG_WIFI_READY_LIB
#include <net/wifi_ready.h>
#endif /* CONFIG_WIFI_READY_LIB */

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

K_SEM_DEFINE(wait_for_wifi_connection, 0, 1);
K_SEM_DEFINE(wait_for_dhcp, 0, 1);
static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback net_shell_mgmt_cb;

static struct {
	bool connected;
	bool disconnect_requested;
	int connect_result;
} context;

#ifdef CONFIG_WIFI_READY_LIB
static K_SEM_DEFINE(wifi_ready_sem, 0, 1);
static bool wifi_ready_status;

static void promiscuous_wifi_ready_cb(bool wifi_ready)
{
	if (wifi_ready_status != wifi_ready) {
		LOG_INF("Wi-Fi is %sready", wifi_ready ? "" : "not ");
	}

	wifi_ready_status = wifi_ready;
	if (wifi_ready) {
		k_sem_give(&wifi_ready_sem);
	}
}

static int promiscuous_register_wifi_ready(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	wifi_ready_callback_t cb;
	int ret;

	if (!iface) {
		return -ENODEV;
	}

	cb.wifi_ready_cb = promiscuous_wifi_ready_cb;

	ret = register_wifi_ready_callback(cb, iface);
	if (ret == -EALREADY) {
		return 0;
	}

	return ret;
}

static int promiscuous_wait_wifi_ready(k_timeout_t timeout)
{
	if (wifi_ready_status) {
		return 0;
	}

	if (k_sem_take(&wifi_ready_sem, timeout) != 0) {
		return -ETIMEDOUT;
	}

	return wifi_ready_status ? 0 : -EAGAIN;
}

SYS_INIT(promiscuous_register_wifi_ready, APPLICATION, 94);
#endif /* CONFIG_WIFI_READY_LIB */

static int cmd_wifi_status(void)
{
	struct net_if *iface = NULL;
	struct wifi_iface_status status = { 0 };
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -1;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status));
	if (ret) {
		LOG_INF("Status request failed: %d\n", ret);
		return ret;
	}

	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		LOG_INF("Interface Mode: %s",
		       wifi_mode_txt(status.iface_mode));
		LOG_INF("Link Mode: %s",
		       wifi_link_mode_txt(status.link_mode));
		LOG_INF("SSID: %.32s", status.ssid);
		LOG_INF("BSSID: %s",
		       net_sprint_ll_addr_buf(
				status.bssid, WIFI_MAC_ADDR_LEN,
				mac_string_buf, sizeof(mac_string_buf)));
		LOG_INF("Band: %s", wifi_band_txt(status.band));
		LOG_INF("Channel: %d", status.channel);
		LOG_INF("Security: %s", wifi_security_txt(status.security));
		LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp));
		LOG_INF("RSSI: %d", status.rssi);
		LOG_INF("TWT: %s", status.twt_capable ? "Supported" : "Not supported");
	}
	return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.connected) {
		return;
	}

	if (status->status) {
		LOG_ERR("Connection failed (%s)",
			wifi_conn_status_txt((enum wifi_conn_status)status->status));
	} else {
		LOG_INF("Connected");
		context.connected = true;
	}

	context.connect_result = status->status;
	k_sem_give(&wait_for_wifi_connection);
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (!context.connected) {
		return;
	}

	if (context.disconnect_requested) {
		LOG_INF("Disconnection request %s (%d)",
			 status->status ? "failed" : "done",
					status->status);
		context.disconnect_requested = false;
	} else {
		LOG_INF("Received Disconnected");
		context.connected = false;
	}

	cmd_wifi_status();
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
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
	k_sem_give(&wait_for_dhcp);
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

static int wifi_connect(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -1;
	}

	context.connected = false;
	context.connect_result = WIFI_STATUS_CONN_FAIL;

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0)) {
		LOG_ERR("Connection request failed");
		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

	return 0;
}

int try_wifi_connect(void)
{
	int ret;

	memset(&context, 0, sizeof(context));

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&net_shell_mgmt_cb,
				     net_mgmt_event_handler,
				     NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&net_shell_mgmt_cb);

#ifdef CONFIG_WIFI_READY_LIB
	ret = promiscuous_wait_wifi_ready(
		K_SECONDS(CONFIG_PROMISCUOUS_SAMPLE_CONNECTION_TIMEOUT_S));
	if (ret != 0) {
		LOG_ERR("Wi-Fi subsystem not ready (%d)", ret);
		return ret;
	}
#else
	k_sleep(K_SECONDS(1));
#endif /* CONFIG_WIFI_READY_LIB */

	LOG_INF("Static IP address (overridable): %s/%s -> %s",
		CONFIG_NET_CONFIG_MY_IPV4_ADDR,
		CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
		CONFIG_NET_CONFIG_MY_IPV4_GW);

	wifi_connect();

	if (k_sem_take(&wait_for_wifi_connection,
		       K_SECONDS(CONFIG_PROMISCUOUS_SAMPLE_CONNECTION_TIMEOUT_S))) {
		LOG_ERR("Wi-Fi connection timed out");
		return -1;
	}

	if (context.connected) {
		k_sem_take(&wait_for_dhcp,
			   K_SECONDS(CONFIG_PROMISCUOUS_SAMPLE_DHCP_TIMEOUT_S));
		cmd_wifi_status();
	} else if (context.connect_result) {
		LOG_ERR("Connection unsuccessful with reason (%s)",
			wifi_conn_status_txt((enum wifi_conn_status)context.connect_result));
		return -1;
	}

	return 0;
}
