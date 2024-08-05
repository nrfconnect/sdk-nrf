/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SR coexistence sample
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coex, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <net/wifi_mgmt_ext.h>

/* For net_sprint_ll_addr_buf */
#include "net_private.h"

#include <coex.h>

#include "bt_throughput_test.h"

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
				NET_EVENT_WIFI_DISCONNECT_RESULT)

#define MAX_SSID_LEN 32
#define WIFI_CONNECTION_TIMEOUT 30

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(CONFIG_NET_CONFIG_PEER_IPV4_PORT),
};

static struct net_mgmt_event_callback wifi_sta_mgmt_cb;
static struct net_mgmt_event_callback net_addr_mgmt_cb;

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} context;

K_SEM_DEFINE(wait_for_next, 0, 1);
K_SEM_DEFINE(udp_callback, 0, 1);

static void run_bt_benchmark(void);

K_THREAD_DEFINE(run_bt_traffic,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_bt_benchmark,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);

struct wifi_iface_status status = { 0 };

static int cmd_wifi_status(void)
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status))) {
		LOG_INF("Status request failed\n");

		return -ENOEXEC;
	}

	LOG_INF("Status: successful\n");
	LOG_INF("==================\n");
	LOG_INF("State: %s\n", wifi_state_txt(status.state));

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
	}

	return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		LOG_ERR("Connection request failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
		context.connected = true;
	}

	cmd_wifi_status();
	k_sem_give(&wait_for_next);
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.disconnect_requested) {
		LOG_INF("Disconnection request %s (%d)",
			 status->status ? "failed" : "done",
					status->status);
		context.disconnect_requested = false;
	} else {
		LOG_INF("Disconnected");
		context.connected = false;
	}

	cmd_wifi_status();
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface)
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

	LOG_INF("IP address: %s", dhcp_info);
	k_sem_give(&wait_for_next);
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
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

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0)) {
		LOG_ERR("Connection request failed");

		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

	return 0;
}

static int wifi_disconnect(void)
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnect_requested = true;

	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnect_requested = false;

		if (status == -EALREADY) {
			LOG_INF("Already disconnected");
		} else {
			LOG_ERR("Disconnect request failed");
			return -ENOEXEC;
		}
	} else {
		LOG_INF("Disconnect requested");
	}

	return 0;
}

static int parse_ipv4_addr(char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IPv4 address %s\n", host);
		return -EINVAL;
	}

	LOG_INF("IPv4 address %s", host);

	return 0;
}

int wait_for_next_event(const char *event_name, int timeout)
{
	int wait_result;

	if (event_name) {
		LOG_INF("Waiting for %s", event_name);
	}

	wait_result = k_sem_take(&wait_for_next, K_SECONDS(timeout));
	if (wait_result) {
		LOG_ERR("Timeout waiting for %s -> %d", event_name, wait_result);
		return -1;
	}

	LOG_INF("Got %s", event_name);
	k_sem_reset(&wait_for_next);

	return 0;
}

static void udp_upload_results_cb(enum zperf_status status,
			  struct zperf_results *result,
			  void *user_data)
{
	unsigned int client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: Upload completed!");
		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}
		/* print results */
		LOG_INF("Upload results:");
		LOG_INF("%u bytes in %llu ms",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		LOG_INF("%u packets sent", result->nb_packets_sent);
		LOG_INF("%u packets lost", result->nb_packets_lost);
		LOG_INF("%u packets received", result->nb_packets_rcvd);
		k_sem_give(&udp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

static void run_bt_benchmark(void)
{
	bt_throughput_test_run();
}

enum nrf_wifi_pta_wlan_op_band wifi_mgmt_to_pta_band(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ;
	case WIFI_FREQ_BAND_5_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ;
	default:
		return NRF_WIFI_PTA_WLAN_OP_BAND_NONE;
	}
}

int main(void)
{
	int ret = 0;
	bool test_wlan = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_ble = IS_ENABLED(CONFIG_TEST_TYPE_BLE);
#ifdef CONFIG_NRF70_SR_COEX
	enum nrf_wifi_pta_wlan_op_band wlan_band;
	bool separate_antennas = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
#endif /* CONFIG_NRF70_SR_COEX */
	bool is_sr_protocol_ble = IS_ENABLED(CONFIG_SR_PROTOCOL_BLE);

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && \
	!(defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	   defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP))
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif

	memset(&context, 0, sizeof(context));

	net_mgmt_init_event_callback(&wifi_sta_mgmt_cb,
				wifi_mgmt_event_handler,
				WIFI_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_sta_mgmt_cb);

	net_mgmt_init_event_callback(&net_addr_mgmt_cb,
				net_mgmt_event_handler,
				NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&net_addr_mgmt_cb);

#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
#endif

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
	k_sleep(K_SECONDS(1));

	LOG_INF("test_wlan = %d and test_ble = %d\n", test_wlan, test_ble);


#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
	/* Configure SR side (nRF5340 side) switch for nRF700x DK */
	ret = nrf_wifi_config_sr_switch(separate_antennas);
	if (ret != 0) {
		LOG_ERR("Unable to configure SR side switch: %d\n", ret);
		goto err;
	}
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */

	if (test_wlan) {
		/* Wi-Fi connection */
		wifi_connect();

		if (wait_for_next_event("Wi-Fi Connection", WIFI_CONNECTION_TIMEOUT)) {
			goto err;
		}

		if (wait_for_next_event("Wi-Fi DHCP", 10)) {
			goto err;
		}

#ifdef CONFIG_NRF70_SR_COEX
		/* Configure Coexistence Hardware */
		LOG_INF("\n");
		LOG_INF("Configuring non-PTA registers.\n");
		ret = nrf_wifi_coex_config_non_pta(separate_antennas, is_sr_protocol_ble);
		if (ret != 0) {
			LOG_ERR("Configuring non-PTA registers of CoexHardware FAIL\n");
			goto err;
		}

		wlan_band = wifi_mgmt_to_pta_band(status.band);
		if (wlan_band == NRF_WIFI_PTA_WLAN_OP_BAND_NONE) {
			LOG_ERR("Invalid Wi-Fi band: %d\n", wlan_band);
			goto err;
		}

		LOG_INF("Configuring PTA registers for %s\n", wifi_band_txt(status.band));
		ret = nrf_wifi_coex_config_pta(wlan_band, separate_antennas, is_sr_protocol_ble);
		if (ret != 0) {
			LOG_ERR("Failed to configure PTA coex hardware: %d\n", ret);
			goto err;
		}
#endif /* CONFIG_NRF70_SR_COEX */
	}

	if (test_ble) {
		/* BLE connection */
		LOG_INF("Configure BLE throughput test\n");
		ret = bt_throughput_test_init();
		if (ret != 0) {
			LOG_ERR("Failed to configure BLE throughput test: %d\n", ret);
			goto err;
		}
	}

	if (test_wlan) {
		struct zperf_upload_params params;

		/* Start Wi-Fi traffic */
		LOG_INF("Starting Wi-Fi benchmark: Zperf client");
		params.duration_ms = CONFIG_WIFI_TEST_DURATION;
		params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
		params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;
		parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			&in4_addr_my);
		net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

		memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));

		ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d\n", ret);
			goto err;
		}
	}

	if (test_ble) {
		/*  In case BLE is peripheral, skip running BLE traffic */
		if (IS_ENABLED(CONFIG_COEX_BT_CENTRAL)) {
			/* Start BLE traffic */
			k_thread_start(run_bt_traffic);
		}
	}

	if (test_wlan) {
		/* Run Wi-Fi traffic */
		if (k_sem_take(&udp_callback, K_FOREVER) != 0) {
			LOG_ERR("Results are not ready");
		} else {
			LOG_INF("UDP SESSION FINISHED");
		}
	}

	if (test_ble) {
		/*  In case BLE is peripheral, skip running BLE traffic */
		if (IS_ENABLED(CONFIG_COEX_BT_CENTRAL)) {
			/* Run BLE traffic */
			k_thread_join(run_bt_traffic, K_FOREVER);
		}
	}

	if (test_wlan) {
		/* Wi-Fi disconnection */
		LOG_INF("Disconnecting Wi-Fi\n");
		wifi_disconnect();
	}

	if (test_ble) {
		/* BLE disconnection */
		LOG_INF("Disconnecting BLE\n");
		bt_throughput_test_exit();
	}

	/* Disable coexistence hardware */
	nrf_wifi_coex_hw_reset();

	LOG_INF("\nCoexistence test complete\n");

	return 0;
err:
	return ret;
}
