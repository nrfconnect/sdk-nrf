/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief internal function to request a full set of PGNSS data from nRF Cloud
 *
 * @return int 0 on success, negative error code on failure
 */
int nrf_cloud_pgnss_request_internal_all(void);

/**
 * @brief internal function to request PGNSS data from nRF Cloud
 *
 * @param request
 * @return int
 */
int nrf_cloud_pgnss_request_internal(const struct gps_pgnss_request *request);
