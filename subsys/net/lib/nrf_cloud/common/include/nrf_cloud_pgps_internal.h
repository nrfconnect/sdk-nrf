/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief internal function to request a full set of P-GPS data from nRF Cloud
 *
 * @return int 0 on success, negative error code on failure
 */
int nrf_cloud_pgps_request_internal_all(void);

/**
 * @brief internal function to request P-GPS data from nRF Cloud
 *
 * @param request
 * @return int
 */
int nrf_cloud_pgps_request_internal(const struct gps_pgps_request *request);
