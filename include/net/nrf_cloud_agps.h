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
#include <nrf_modem_gnss.h>
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
 * @retval 0       Request sent successfully.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_agps_request(const struct nrf_modem_gnss_agps_data_frame *request);

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
 *
 * @retval 0 A-GPS data successfully processed.
 * @retval -EFAULT An nRF Cloud A-GPS error code was processed.
 * @return A negative value indicates an error.
 */
int nrf_cloud_agps_process(const char *buf, size_t buf_len);

/**@brief Query which A-GPS elements were actually received
 *
 * @param received_elements return copy of requested elements received
 * since agps request made
 */
void nrf_cloud_agps_processed(struct nrf_modem_gnss_agps_data_frame *received_elements);

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
