/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_LOCATION_COMMON_H_
#define WIFI_LOCATION_COMMON_H_

/** @file wifi_location_common.h */

#include <zephyr/net/wifi_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup wifi_location_common Wi-Fi location
 *  Common components for Wi-Fi location.
 * @{
 */

/** Size of Wi-Fi MAC address string: 2 chars per byte, colon separated */
#define WIFI_MAC_ADDR_STR_LEN	((WIFI_MAC_ADDR_LEN * 2) + 5)
/** Print template for creating a MAC address string from a byte array */
#define WIFI_MAC_ADDR_TEMPLATE	"%02x:%02x:%02x:%02x:%02x:%02x"

/** @brief Access points found during a Wi-Fi scan. */
struct wifi_scan_info {
	/** Array of access points found during scan. */
	struct wifi_scan_result *ap_info;
	/** The number of access points found during scan. */
	uint16_t cnt;
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* WIFI_LOCATION_COMMON_H_ */
