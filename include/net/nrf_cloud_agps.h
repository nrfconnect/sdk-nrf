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
#include <net/nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_agps nRF Cloud A-GPS
 * @{
 */

#if defined(CONFIG_NRF_CLOUD_MQTT)
/**@brief Requests specified A-GPS data from nRF Cloud via MQTT.
 *
 * @param request Structure containing specified A-GPS data to be requested.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_request(const struct gps_agps_request request);

/**@brief Requests all available A-GPS data from nRF Cloud via MQTT.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agps_request_all(void);
#endif /* CONFIG_NRF_CLOUD_MQTT */

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

/**@brief Query which A-GPS elements were actually received
 *
 * @param received_elements return copy of requested elements received
 * since agps request made
 */
void nrf_cloud_agps_processed(struct gps_agps_request *received_elements);

/**@brief Query whether A-GPS data has been requested from cloud
 *
 * @return True if request is outstanding.
 */
bool nrf_cloud_agps_request_in_progress(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_AGPS_H_ */
