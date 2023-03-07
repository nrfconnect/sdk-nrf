/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_MGMT_EXT_H__
#define WIFI_MGMT_EXT_H__

#include <zephyr/net/wifi_mgmt.h>

/**
 * @defgroup wifi_mgmt_ext Wi-Fi management extension library
 * @{
 * @brief Library that adds an automatic connection feature to the Wi-Fi stack.

 */

enum net_request_wifi_cmd_ext {
	NET_REQUEST_WIFI_CMD_CONNECT_STORED = NET_REQUEST_WIFI_CMD_REG_DOMAIN + 100,
};

#define NET_REQUEST_WIFI_CONNECT_STORED				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CONNECT_STORED)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED);

/** @} */

#endif /* WIFI_MGMT_EXT_H__ */
