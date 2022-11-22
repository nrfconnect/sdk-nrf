/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file wifi_nrf_cloud.h
 *
 * @brief nRF Cloud API for Wi-Fi positioning.
 */

#ifndef WIFI_NRF_CLOUD_H
#define WIFI_NRF_CLOUD_H

#include "wifi_service.h"

/**
 * @brief nRF Cloud Wi-Fi positioning request.
 *
 * @param[in]     rcv_buf     Buffer for storing the service response.
 * @param[in]     rcv_buf_len Length of the provided buffer.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int wifi_nrf_cloud_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct location_wifi_serv_pos_req *request,
	struct location_data *result);

#endif /* WIFI_NRF_CLOUD_H */
