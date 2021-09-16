/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_services_wlan.h
 *
 * @brief REST API for WLAN positioning.
 */

#ifndef REST_SERVICES_WLAN_H
#define REST_SERVICES_WLAN_H

#include <zephyr/types.h>
#include <net/wifi.h>
#include <modem/location.h>

/** @brief Item for passing a WLAN scanning result */
struct wlan_scanning_result_info {
	char mac_addr_str[WIFI_MAC_MAX_LEN + 1];
	char ssid_str[WIFI_SSID_MAX_LEN + 1];
	uint8_t channel;
	int8_t rssi;
};

/** @brief Data used for WLAN positioning request */
struct rest_wlan_pos_request {
	/** Scanning results */
	struct wlan_scanning_result_info
		scanning_results[CONFIG_LOCATION_METHOD_WLAN_SCANNING_RESULTS_MAX_CNT];
	uint8_t wlan_scanning_result_count;
};

/** @brief WLAN positioning request result */
struct rest_wlan_pos_result {
	float latitude;
	float longitude;
	float accuracy;
};

/**
 * @brief WLAN location request by using WLAN positioning service.
 *
 * @param[in]     service Used WLAN positioning service.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int rest_services_wlan_location_get(enum loc_wlan_service service,
			   const struct rest_wlan_pos_request *request,
			   struct rest_wlan_pos_result *result);

#endif /* REST_SERVICES_WLAN_H */
