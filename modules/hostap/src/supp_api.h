/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#ifndef ZEPHYR_SUPP_MGMT_H
#define ZEPHYR_SUPP_MGMT_H

#include <zephyr/net/wifi_mgmt.h>

#define MAX_SSID_LEN 32
#define MAC_ADDR_LEN 6

/**
 * @brief Request a connection
 *
 * @param iface_name: Wi-Fi interface name to use
 * @param params: Connection details
 *
 * @return: 0 for OK; -1 for ERROR
 */
int zephyr_supp_connect(const struct device *dev,
						struct wifi_connect_req_params *params);
/**
 * @brief Forces station to disconnect and stops any subsequent scan
 *  or connection attempts
 *
 * @param iface_name: Wi-Fi interface name to use
 *
 * @return: 0 for OK; -1 for ERROR
 */
int zephyr_supp_disconnect(const struct device *dev);

/**
 * @brief
 *
 * @param iface_name: Wi-Fi interface name to use
 * @param wifi_iface_status: Status structure to fill
 *
 * @return: 0 for OK; -1 for ERROR
 */
int zephyr_supp_status(const struct device *dev,
	struct wifi_iface_status *status);
#endif /* ZEPHYR_SUPP_MGMT_H */
