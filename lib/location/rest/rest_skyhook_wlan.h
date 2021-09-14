/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_skyhook_wlan.h
 *
 * @brief Skyhook REST API for WLAN positioning.
 */

#ifndef REST_SKYHOOK_WLAN_H
#define REST_SKYHOOK_WLAN_H

#include "rest_services_wlan.h"

/**
 * @brief Skyhook wlan positioning request.
 *
 * @param[in]     rcv_buf     Buffer for storing the REST service response.
 * @param[in]     rcv_buf_len Length of the provided buffer.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result  Parsed results of API response.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int skyhook_rest_wlan_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct rest_wlan_pos_request *request,
	struct rest_wlan_pos_result *result);

#endif /* REST_SKYHOOK_WLAN_H */
