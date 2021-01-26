/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_AGPS_H_
#define NRF_CLOUD_AGPS_H_

/** @file nrf_cloud_agps.h
 * @brief Module to provide nRF Cloud A-GPS support to nRF9160 SiP.
 */

#include <zephyr.h>
#include <drivers/gps.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_agps nRF Cloud AGPS
 * @{
 */

/** @brief Cell-based location request type */
enum cell_based_location_type {
	CELL_LOC_TYPE_INVALID = -1,
	CELL_LOC_TYPE_SINGLE,
	CELL_LOC_TYPE_MULTI /* Not yet supported */
};

/**@brief Requests specified A-GPS data from nRF Cloud.
 *
 * @param request Structure containing specified A-GPS data to be requested.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_request(const struct gps_agps_request request);

/**@brief Requests all available A-GPS data from nRF Cloud.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_request_all(void);

/**@brief Request a cell-based location query from nRF Cloud.
 *
 * @param type        Type of cell-based location to request.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_request_cell_location(enum cell_based_location_type type,
					 const bool request_loc);

/**@brief Gets most recent location from single-cell request.
 *
 * @param lat Pointer where last single cell latitude is to be copied.
 * @param lon Pointer where last single cell longitude is to be copied.
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_get_last_cell_location(double *const lat,
					  double *const lon);

/**@brief Processes binary A-GPS data received from nRF Cloud.
 *
 * @param buf Pointer to data received from nRF Cloud.
 * @param buf_len Buffer size of data to be processed.
 * @param socket Pointer to GNSS socket to which A-GPS data will be injected.
 *		 If NULL, the nRF9160 GPS driver is used to inject the data.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_process(const char *buf, size_t buf_len, const int *socket);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_AGPS_H_ */
