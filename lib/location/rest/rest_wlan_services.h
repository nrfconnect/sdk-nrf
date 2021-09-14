/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file rest_wlan_services.h
 *
 * @brief REST API for WLAN positioning.
 */
#ifndef REST_WLAN_SERVICES_H
#define REST_WLAN_SERVICES_H

#include <zephyr/types.h>
#include <net/wifi.h>

/** @brief Item for storing a WLAN MAC address */
struct mac_address_info {
	char mac_addr_str[WIFI_MAC_MAX_LEN];
};

/** @brief Data required for WLAN positioning request */
struct rest_wlan_pos_request {
	/** MAC addresses */
	struct mac_address_info
		mac_addresses[CONFIG_LOCATION_METHOD_WLAN_MAX_MAC_ADDRESSES];
	uint8_t mac_addr_count;
};

/** @brief WLAN positioning request result */
struct rest_wlan_pos_result {
	float latitude;
	float longitude;
	float accuracy;
};

#endif /* REST_WLAN_SERVICES_H */
