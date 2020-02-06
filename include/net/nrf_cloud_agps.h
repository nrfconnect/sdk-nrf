/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_CLOUD_AGPS_H_
#define NRF_CLOUD_AGPS_H_

/** @file nrf_cloud_agps.h
 * @brief Module to provide nRF Cloud A-GPS support to nRF9160 SiP.
 * @{
 */

#include <zephyr.h>
#include <drivers/gps.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Requests A-GPS data from nRF Cloud. */
int nrf_cloud_agps_request(enum gps_agps_type *types, size_t type_count);

/**@brief Requests A-GPS data from nRF Cloud. */
int nrf_cloud_agps_request_all(void);

/**@brief Receive binary A-GPS data. */
int nrf_cloud_agps_process(const char *buf, size_t buf_len, const int *socket);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF_CLOUD_AGPS_H_ */
