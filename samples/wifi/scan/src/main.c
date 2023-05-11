/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi scan sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(scan, CONFIG_LOG_DEFAULT_LEVEL);

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

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY
#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_RAW_SCAN_RESULT |                \
				NET_EVENT_WIFI_SCAN_DONE)
#else
#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_RAW_SCAN_RESULT)
#endif

static uint32_t scan_result;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

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

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
static int wifi_freq_to_channel(int frequency)
{
	int channel = 0;

	if (frequency == 2484) { /* channel 14 */
		channel = 14;
	} else if ((frequency <= 2472) && (frequency >= 2412)) {
		channel = ((frequency - 2412) / 5) + 1;
	} else if ((frequency <= 5895) && (frequency >= 5180)) {
		channel = ((frequency - 5000) / 5);
	} else {
		channel = frequency;
	}

	return channel;
}

static enum wifi_frequency_bands wifi_frequency_to_band(int frequency)
{
	enum wifi_frequency_bands band = WIFI_FREQ_BAND_2_4_GHZ;

	if ((frequency  >= 2401) && (frequency <= 2495)) {
		band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if ((frequency  >= 5170) && (frequency <= 5895)) {
		band = WIFI_FREQ_BAND_5_GHZ;
	}

	return band;
}

static void handle_raw_scan_result(struct net_mgmt_event_callback *cb)
{
	struct wifi_raw_scan_result *raw =
		(struct wifi_raw_scan_result *)cb->info;
	int channel;
	int band;
	int rssi;
	int i = 0;
	int raw_scan_size = raw->frame_length;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	scan_result++;

	if (scan_result == 1U) {
		printk("%-4s | %-13s | %-4s |  %-15s | %-15s | %-32s\n",
		      "Num", "Channel (Band)", "RSSI", "BSSID", "Frame length", "Frame Body");
	}

	rssi = raw->rssi;
	channel = wifi_freq_to_channel(raw->frequency);
	band = wifi_frequency_to_band(raw->frequency);

	printk("%-4d | %-4u (%-6s) | %-4d | %s |      %-4d        ",
	      scan_result, channel,
	      wifi_band_txt(band),
	      rssi,
	      net_sprint_ll_addr_buf(raw->data + 10, WIFI_MAC_ADDR_LEN, mac_string_buf,
				     sizeof(mac_string_buf)), raw_scan_size);

	if (raw->frame_length > CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH) {
		raw_scan_size = CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH;
	}

	if (raw_scan_size) {
		for (i = 0; i < 32; i++) {
			printk("%02X ", *(raw->data + i));
		}
	}

	printk("\n");
}
#endif

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Scan request failed (%d)", status->status);
	} else {
		printk("Scan request done\n");
	}

	scan_result = 0U;
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	case NET_EVENT_WIFI_RAW_SCAN_RESULT:
		handle_raw_scan_result(cb);
		break;
#endif
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

	printk("Scan requested\n");

	return 0;
}

int main(void)
{
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	k_sleep(K_SECONDS(1));
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

	wifi_scan();

	return 0;
}
