/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file here_rest.h
 *
 * @brief Here REST API for WLAN positioning.
 */
#ifndef REST_HERE_WLAN_H
#define REST_HERE_WLAN_H

#include <zephyr/types.h>
#include <net/wifi.h>

/** @brief Item for storing a WLAN MAC address */
struct mac_address_info {
	char mac_addr_str[WIFI_MAC_MAX_LEN];
};

/** @brief Data required for Here WLAN positioning request */
struct here_rest_wlan_pos_request {
	/** MAC addresses */
	struct mac_address_info
		mac_addresses[CONFIG_LOCATION_METHOD_WLAN_MAX_MAC_ADDRESSES];
	uint8_t mac_addr_count;
};

/** @brief Here WLAN positioning request result */
struct here_rest_wlan_pos_result {
	float latitude;
	float longitude;
	float accuracy;
};

/**
 * @brief Here wlan positioning request.
 *
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int here_rest_wlan_pos_get(const struct here_rest_wlan_pos_request *request,
			   struct here_rest_wlan_pos_result *result);

#endif /* REST_HERE_WLAN_H */
