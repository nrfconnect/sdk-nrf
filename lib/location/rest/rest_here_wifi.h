/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_here_wifi.h
 *
 * @brief Here REST API for WiFi positioning.
 */

#ifndef REST_HERE_WIFI_H
#define REST_HERE_WIFI_H

#include "rest_services_wifi.h"

/**
 * @brief Here WiFi positioning request.
 *
 * @param[in]     rcv_buf     Buffer for storing the REST service response.
 * @param[in]     rcv_buf_len Length of the provided buffer.
 * @param[in]     request     Data to be provided in API call.
 * @param[in,out] result      Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int here_rest_wifi_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct rest_wifi_pos_request *request,
	struct rest_wifi_pos_result *result);

#endif /* REST_HERE_WIFI_H */
