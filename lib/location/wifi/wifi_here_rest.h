/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file wifi_here_rest.h
 *
 * @brief HERE REST API for Wi-Fi positioning.
 */

#ifndef WIFI_HERE_REST_H
#define WIFI_HERE_REST_H

#include "wifi_service.h"

/**
 * @brief HERE Wi-Fi positioning request.
 *
 * @param[in]     rcv_buf     Buffer for storing the REST service response.
 * @param[in]     rcv_buf_len Length of the provided buffer.
 * @param[in]     request     Data to be provided in API call.
 * @param[in,out] result      Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int wifi_here_rest_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct rest_wifi_pos_request *request,
	struct location_data *result);

#endif /* WIFI_HERE_REST_H */
