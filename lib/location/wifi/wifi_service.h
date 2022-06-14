/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file wifi_service.h
 *
 * @brief REST API for Wi-Fi positioning.
 */

#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/net/wifi.h>
#include <modem/location.h>

#define WIFI_MAC_ADDR_STR_LEN 17

/** @brief Item for passing a Wi-Fi scanning result */
struct wifi_scanning_result_info {
	char mac_addr_str[WIFI_MAC_ADDR_STR_LEN + 1];
	char ssid_str[WIFI_SSID_MAX_LEN + 1];
	uint8_t channel;
	int8_t rssi;
};

/** @brief Data used for Wi-Fi positioning request */
struct rest_wifi_pos_request {
	/** Scanning results */
	struct wifi_scanning_result_info
		scanning_results[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
	uint8_t wifi_scanning_result_count; /* Count of wifi access points returned in scanning */
	int32_t timeout_ms; /* REST request timeout (in mseconds) */
};

/**
 * @brief Wi-Fi location request by using Wi-Fi positioning service.
 *
 * @param[in]     service Used Wi-Fi positioning service.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int rest_services_wifi_location_get(enum location_service service,
			   const struct rest_wifi_pos_request *request,
			   struct location_data *result);

#endif /* WIFI_SERVICE_H */
