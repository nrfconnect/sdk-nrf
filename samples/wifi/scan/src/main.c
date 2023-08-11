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

#include <zephyr/kernel.h>
#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M
#include <nrfx_clock.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

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
#define SCAN_TIMEOUT_MS 10000

static uint32_t scan_result;

const struct wifi_scan_params tests[] = {
	{
	.scan_type = WIFI_SCAN_TYPE_ACTIVE,
	.dwell_time_active = CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_ACTIVE
	},
#ifdef CONFIG_WIFI_SCAN_TYPE_PASSIVE
	{
	.scan_type = WIFI_SCAN_TYPE_PASSIVE,
	.dwell_time_passive = CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_PASSIVE
	},
#endif
#ifdef CONFIG_WIFI_SCAN_BAND_2_4_GHZ
	{
	.bands = (1 << WIFI_FREQ_BAND_2_4_GHZ)
	},
#endif
#ifdef CONFIG_WIFI_SCAN_BAND_5GHZ
	{
	.bands = (1 << WIFI_FREQ_BAND_5_GHZ)
	},
#endif
#ifdef CONFIG_WIFI_SCAN_SSID_FILT_MAX
	{
	.ssids = {}
	},
#endif
#ifdef CONFIG_WIFI_SCAN_CHAN
	{
	.chan = { {0, 0} }
	},
#endif
};

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

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
	k_sem_give(&scan_sem);
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

	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		struct wifi_scan_params params = tests[i];
#ifdef CONFIG_WIFI_SCAN_SSID_FILT_MAX
		if (!strlen(params.ssids[0])) {
			char *buf = NULL;

			buf = malloc(strlen(CONFIG_WIFI_SCAN_SSID_FILT) + 1);
			if (!buf) {
				LOG_ERR("Malloc Failed");
				return -EINVAL;
			}

			strcpy(buf, CONFIG_WIFI_SCAN_SSID_FILT);
			if (wifi_utils_parse_scan_ssids(buf,
							params.ssids)) {
				LOG_ERR("Incorrect value(s) in CONFIG_WIFI_SCAN_SSID_FILT: %s",
						CONFIG_WIFI_SCAN_SSID_FILT);
				return -EINVAL;
			}
		}
#endif
#ifdef CONFIG_WIFI_SCAN_CHAN
		if (wifi_utils_parse_scan_chan(CONFIG_WIFI_SCAN_CHAN_LIST,
				params.chan)) {
			LOG_ERR("Chan Parse failed");
			return -ENOEXEC;
		}
#endif
		if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params,
				sizeof(struct wifi_scan_params))) {
			LOG_ERR("Scan request failed");
			return -ENOEXEC;
		}
		printk("Scan requested\n");

		k_sem_take(&scan_sem, K_MSEC(SCAN_TIMEOUT_MS));
	}

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

int main(void)
{
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	k_sleep(K_SECONDS(1));
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

	if (!is_mac_addr_set(net_if_get_default())) {
		struct net_if *iface = net_if_get_default();
		int ret;

		/* Set a local MAC address with Nordic OUI */
		ret = net_if_down(iface);
		if (ret) {
			LOG_ERR("Cannot bring down iface (%d)", ret);
			return ret;
		}

		net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
			 (void *)CONFIG_WIFI_MAC_ADDRESS, sizeof(CONFIG_WIFI_MAC_ADDRESS));

		ret = net_if_up(iface);
		if (ret) {
			LOG_ERR("Cannot bring up iface (%d)", ret);
			return ret;
		}

		LOG_INF("OTP not programmed, proceeding with local MAC: %s", net_sprint_ll_addr(
							net_if_get_link_addr(iface)->addr,
							net_if_get_link_addr(iface)->len));
	}

	wifi_scan();

	return 0;
}
