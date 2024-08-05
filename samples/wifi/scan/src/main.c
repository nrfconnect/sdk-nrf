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
#ifdef CONFIG_WIFI_SCAN_PROFILE_DEFAULT
	{
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_ACTIVE
	{
	.scan_type = WIFI_SCAN_TYPE_ACTIVE,
	.dwell_time_active = CONFIG_WIFI_SCAN_DWELL_TIME_ACTIVE
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_PASSIVE
	{
	.scan_type = WIFI_SCAN_TYPE_PASSIVE,
	.dwell_time_passive = CONFIG_WIFI_SCAN_DWELL_TIME_PASSIVE
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_2_4GHz_ACTIVE
	{
	.scan_type = WIFI_SCAN_TYPE_ACTIVE,
	.bands = 0
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_2_4GHz_PASSIVE
	{
	.scan_type = WIFI_SCAN_TYPE_PASSIVE,
	.bands = 0
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_5GHz_ACTIVE
	{
	.scan_type = WIFI_SCAN_TYPE_ACTIVE,
	.bands = 0
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_5GHz_PASSIVE
	{
	.scan_type = WIFI_SCAN_TYPE_PASSIVE,
	.bands = 0
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_2_4GHz_NON_OVERLAP_CHAN
	{
	.bands = 0,
	.chan = { {0, 0} }
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_5GHz_NON_DFS_CHAN
	{
	.bands = 0,
	.chan = { {0, 0} }
	},
#endif
#ifdef CONFIG_WIFI_SCAN_PROFILE_2_4GHz_NON_OVERLAP_AND_5GHz_NON_DFS_CHAN
	{
	.bands = 0,
	.chan = { {0, 0} }
	},
#endif
};

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

K_SEM_DEFINE(scan_sem, 0, 1);

#if defined CONFIG_WIFI_NRF70_SKIP_LOCAL_ADMIN_MAC
static bool local_mac_check(const uint8_t *const mac)
{
return ((mac[0] & 0x02) ||
((mac[0] == 0x00) && (mac[1] == 0x00) && (mac[2] == 0x5E)));
}
#endif /* CONFIG_WIFI_NRF70_SKIP_LOCAL_ADMIN_MAC */

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

	scan_result++;

	if (scan_result == 1U) {
		printk("%-4s | %-32s %-5s | %-4s | %-4s | %-5s | %s\n",
		       "Num", "SSID", "(len)", "Chan", "RSSI", "Security", "BSSID");
	}

	strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
	ssid_print[sizeof(ssid_print) - 1] = '\0';

#if defined CONFIG_WIFI_NRF70_SKIP_LOCAL_ADMIN_MAC
	__ASSERT(!local_mac_check(entry->mac), "Locally administered MAC found: %s\n", ssid_print);
#endif /* CONFIG_WIFI_NRF70_SKIP_LOCAL_ADMIN_MAC */

	printk("%-4d | %-32s %-5u | %-4u | %-4d | %-5s | %s\n",
	       scan_result, ssid_print, entry->ssid_length,
	       entry->channel, entry->rssi,
	       wifi_security_txt(entry->security),
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
	int band_str_len;

	struct wifi_scan_params params = tests[0];

	band_str_len = sizeof(CONFIG_WIFI_SCAN_BANDS_LIST);
	if (band_str_len - 1) {
		char *buf = malloc(band_str_len);

		if (!buf) {
			LOG_ERR("Malloc Failed");
			return -EINVAL;
		}
		strcpy(buf, CONFIG_WIFI_SCAN_BANDS_LIST);
		if (wifi_utils_parse_scan_bands(buf, &params.bands)) {
			LOG_ERR("Incorrect value(s) in CONFIG_WIFI_SCAN_BANDS_LIST: %s",
					CONFIG_WIFI_SCAN_BANDS_LIST);
			free(buf);
			return -ENOEXEC;
		}
		free(buf);
	}

	if (sizeof(CONFIG_WIFI_SCAN_CHAN_LIST) - 1) {
		if (wifi_utils_parse_scan_chan(CONFIG_WIFI_SCAN_CHAN_LIST,
						params.band_chan, ARRAY_SIZE(params.band_chan))) {
			LOG_ERR("Incorrect value(s) in CONFIG_WIFI_SCAN_CHAN_LIST: %s",
					CONFIG_WIFI_SCAN_CHAN_LIST);
			return -ENOEXEC;
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params,
			sizeof(struct wifi_scan_params))) {
		LOG_ERR("Scan request failed");
		return -ENOEXEC;
	}

	printk("Scan requested\n");

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

int main(void)
{
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	k_sleep(K_SECONDS(1));
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

	if (!is_mac_addr_set(net_if_get_default())) {
		struct net_if *iface = net_if_get_default();
		int ret;
		struct ethernet_req_params params;

		/* Set a local MAC address with Nordic OUI */
		if (net_if_is_admin_up(iface)) {
			ret = net_if_down(iface);
			if (ret < 0 && ret != -EALREADY) {
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
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("Cannot bring up iface (%d)", ret);
			return ret;
		}

		LOG_INF("OTP not programmed, proceeding with local MAC: %s", net_sprint_ll_addr(
							net_if_get_link_addr(iface)->addr,
							net_if_get_link_addr(iface)->len));
	}

	while (1) {
		wifi_scan();
		k_sleep(K_SECONDS(CONFIG_WIFI_SCAN_INTERVAL_S));
	}

	return 0;
}
