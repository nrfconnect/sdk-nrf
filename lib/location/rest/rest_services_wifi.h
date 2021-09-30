/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_services_wifi.h
 *
 * @brief REST API for WiFi positioning.
 */

#ifndef REST_SERVICES_WIFI_H
#define REST_SERVICES_WIFI_H

#include <zephyr/types.h>
#include <net/wifi.h>
#include <modem/location.h>

#define WIFI_MAC_ADDR_STR_LEN 17

/** @brief Item for passing a WiFi scanning result */
struct wifi_scanning_result_info {
	char mac_addr_str[WIFI_MAC_ADDR_STR_LEN + 1];
	char ssid_str[WIFI_SSID_MAX_LEN + 1];
	uint8_t channel;
	int8_t rssi;
};

/** @brief Data used for WiFi positioning request */
struct rest_wifi_pos_request {
	/** Scanning results */
	struct wifi_scanning_result_info
		scanning_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
	uint8_t wifi_scanning_result_count;
};

/** @brief WiFi positioning request result */
struct rest_wifi_pos_result {
	float latitude;
	float longitude;
	float accuracy;
};

/**
 * @brief WiFi location request by using WiFi positioning service.
 *
 * @param[in]     service Used WiFi positioning service.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int rest_services_wifi_location_get(enum loc_wifi_service service,
			   const struct rest_wifi_pos_request *request,
			   struct rest_wifi_pos_result *result);

#endif /* REST_SERVICES_WIFI_H */
