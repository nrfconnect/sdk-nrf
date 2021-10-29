/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file wifi_nrf_cloud_rest.h
 *
 * @brief nRF Cloud REST API for Wi-Fi positioning.
 */

#ifndef WIFI_NRF_CLOUD_REST_H
#define WIFI_NRF_CLOUD_REST_H

#include "wifi_service.h"

/**
 * @brief nRF Cloud Wi-Fi positioning request.
 *
 * @param[in]     rcv_buf     Buffer for storing the REST service response.
 * @param[in]     rcv_buf_len Length of the provided buffer.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int wifi_nrf_cloud_rest_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct rest_wifi_pos_request *request,
	struct location_data *result);

#endif /* WIFI_NRF_CLOUD_REST_H */
