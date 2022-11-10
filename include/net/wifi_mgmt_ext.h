/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/wifi_mgmt.h>

enum net_event_wifi_ext_cmd {
	NET_REQUEST_WIFI_CMD_CONNECT_STORED = NET_REQUEST_WIFI_CMD_IFACE_STATUS + 1,
};

#define NET_REQUEST_WIFI_CONNECT_STORED				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CONNECT_STORED)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED);
