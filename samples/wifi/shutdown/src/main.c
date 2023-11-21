/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi shutdown sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(shutdown, CONFIG_LOG_DEFAULT_LEVEL);

#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include <dk_buttons_and_leds.h>

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE)

#define SCAN_TIMEOUT_MS 10000

static uint32_t scan_result;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

void exit_shutdown_mode(void);
void enter_shutdown_mode(void);

K_SEM_DEFINE(scan_sem, 0, 1);

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	scan_result++;

	if (scan_result == 1U) {
		printk("%-4s | %-32s %-5s | %-4s | %-4s | %-5s | %s\n",
		       "Num", "SSID", "(len)", "Chan", "RSSI", "Security", "BSSID");
	}

	printk("%-4d | %-32s %-5u | %-4u | %-4d | %-5s | %s\n",
	       scan_result, entry->ssid, entry->ssid_length,
	       entry->channel, entry->rssi,
	       (entry->security == WIFI_SECURITY_TYPE_PSK ? "WPA/WPA2" : "Open    "),
	       ((entry->mac_length) ?
			net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN, mac_string_buf,
						sizeof(mac_string_buf)) : ""));
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Scan request failed (%d)", status->status);
	} else {
		LOG_INF("Scan request done\n");
	}

	scan_result = 0U;
	k_sem_give(&scan_sem);
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
	default:
		break;
	}
}

static int wifi_scan(void)
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		LOG_ERR("Scan request failed");

		return -ENOEXEC;
	}

	LOG_INF("Scan requested\n");

	k_sem_take(&scan_sem, K_MSEC(SCAN_TIMEOUT_MS));

	return 0;
}

static bool is_mac_addr_set(struct net_if *iface)
{
	struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);
	struct net_eth_addr wifi_addr;

	if (!linkaddr || linkaddr->len != WIFI_MAC_ADDR_LEN) {
		return false;
	}

	memcpy(wifi_addr.addr, linkaddr->addr, WIFI_MAC_ADDR_LEN);

	return net_eth_is_addr_valid(&wifi_addr);
}

static void button_handler_cb(uint32_t button_state, uint32_t has_changed)
{
	if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK)) {
		exit_shutdown_mode();
	} else if ((has_changed & DK_BTN2_MSK) && (button_state & DK_BTN2_MSK)) {
		enter_shutdown_mode();
	}
}

static void buttons_init(void)
{
	int err;

	err = dk_buttons_init(button_handler_cb);
	if (err) {
		LOG_ERR("Buttons initialization failed.\n");
		return;
	}
}

int shutdown_wifi(struct net_if *iface)
{
	int ret;

	if (!net_if_is_admin_up(iface)) {
		return 0;
	}

	ret = net_if_down(iface);
	if (ret) {
		LOG_ERR("Cannot bring down iface (%d)", ret);
		return ret;
	}

	LOG_INF("Interface down");

	return 0;
}

int startup_wifi(struct net_if *iface)
{
	int ret;

	if (!net_if_is_admin_up(iface)) {
		ret = net_if_up(iface);
		if (ret) {
			LOG_ERR("Cannot bring up iface (%d)", ret);
			return ret;
		}

		LOG_INF("Interface up");
	}

	wifi_scan();

	return 0;
}

void enter_shutdown_mode(void)
{
	struct net_if *iface = net_if_get_default();

	shutdown_wifi(iface);
}

void exit_shutdown_mode(void)
{
	struct net_if *iface = net_if_get_default();

	startup_wifi(iface);
}

int main(void)
{
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	LOG_INF("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

	if (!is_mac_addr_set(net_if_get_default())) {
		struct net_if *iface = net_if_get_default();
		int ret;
		struct ethernet_req_params params;

		if (net_if_is_up(iface)) {
			/* Set a local MAC address with Nordic OUI */
			ret = net_if_down(iface);
			if (ret) {
				LOG_ERR("Cannot bring down iface (%d)", ret);
				return ret;
			}
		}

		ret = net_bytes_from_str(params.mac_address.addr, sizeof(CONFIG_WIFI_MAC_ADDRESS),
					 CONFIG_WIFI_MAC_ADDRESS);
		if (ret) {
			LOG_ERR("Failed to parse MAC address: %s (%d)",
				CONFIG_WIFI_MAC_ADDRESS, ret);
			return ret;
		}

		net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
			 &params, sizeof(params));

		ret = net_if_up(iface);
		if (ret) {
			LOG_ERR("Cannot bring up iface (%d)", ret);
			return ret;
		}

		LOG_INF("OTP not programmed, proceeding with local MAC: %s", net_sprint_ll_addr(
							net_if_get_link_addr(iface)->addr,
							net_if_get_link_addr(iface)->len));
	}

	buttons_init();

#ifdef CONFIG_NRF_WIFI_IF_AUTO_START
	exit_shutdown_mode();

	enter_shutdown_mode();
#endif

	k_sleep(K_FOREVER);

	return 0;
}
