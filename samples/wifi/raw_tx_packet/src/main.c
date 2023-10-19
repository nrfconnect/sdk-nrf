/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Raw Tx Packet sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_tx_packet, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"
#include "wifi_connection.h"

static void wifi_set_mode(int mode_val)
{
	int ret;
	struct net_if *iface = NULL;
	struct wifi_mode_info mode_info = {0};

	mode_info.oper = WIFI_MGMT_SET;

	iface = net_if_get_first_wifi();
	if (iface == NULL) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	mode_info.if_index = net_if_get_by_iface(iface);
	mode_info.mode = mode_val;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed %d", ret);
	}
}

int main(void)
{
	int mode;
#ifdef CONFIG_RAW_TX_PACKET_SAMPLE_STA_ONLY_MODE
	mode = BIT(0);
#elif CONFIG_RAW_TX_PACKET_SAMPLE_STA_TX_INJECTION_MODE
	mode = BIT(0) | BIT(2);
#endif
	wifi_set_mode(mode);

#ifdef CONFIG_RAW_TX_PACKET_SAMPLE_CONNECTION_MODE
	int status;

	status = try_wifi_connect();
	if (status < 0) {
		return status;
	}
#endif

	return 0;
}
