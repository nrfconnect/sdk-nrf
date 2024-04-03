/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Promiscuous sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(promiscuous, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/net/ethernet.h>
#include "wifi_connection.h"

static int wifi_set_mode(int mode_val)
{
	struct net_if *iface = NULL;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("No Wi-Fi interface found");
		return -1;
	}

	if (net_eth_promisc_mode(iface, true)) {
		LOG_ERR("Promiscuous mode enable failed");
		return -1;
	}

	LOG_INF("Promiscuous mode enabled");
	return 0;
}

int main(void)
{
	int ret;

	if (wifi_set_mode(true)) {
		return -1;
	}

	/* TODO: Implement waiting for WPA supplicant to manage the Wi-Fi interface.*/
	ret = try_wifi_connect();
	if (ret < 0) {
		return ret;
	}

	return 0;
}
