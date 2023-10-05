/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_AGNSS_H_
#define NRF_CLOUD_AGNSS_H_

/** @file nrf_cloud_agnss.h
 * @brief Module to provide nRF Cloud A-GNSS support to nRF9160 SiP.
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <net/nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Exclude the mask angle from the A-GNSS request */
#define NRF_CLOUD_AGNSS_MASK_ANGLE_NONE	0xFF

/** @defgroup nrf_cloud_agnss nRF Cloud A-GNSS
 * @{
 */

#if defined(CONFIG_NRF_CLOUD_MQTT)
/** @brief Requests specified A-GNSS data from nRF Cloud via MQTT.
 *
 * @param request Structure containing specified A-GNSS data to be requested.
 *
 * @retval 0       Request sent successfully.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_agnss_request(const struct nrf_modem_gnss_agnss_data_frame *request);

/** @brief Requests all available A-GNSS data from nRF Cloud via MQTT.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_agnss_request_all(void);
#endif /* CONFIG_NRF_CLOUD_MQTT */

/** @brief Processes binary A-GNSS data received from nRF Cloud.
 *
 * @param buf Pointer to data received from nRF Cloud.
 * @param buf_len Buffer size of data to be processed.
 *
 * @retval 0 A-GNSS data successfully processed.
 * @retval -EFAULT An nRF Cloud A-GNSS error code was processed.
 * @retval -ENOMSG No nRF Cloud A-GNSS error code found, invalid error code or wrong
 *         app_id/msg_type.
 * @retval -EINVAL buf was NULL or buf_len was zero.
 * @retval -EBADMSG Non-JSON payload is not in the A-GNSS format.
 * @return A negative value indicates an error.
 */
int nrf_cloud_agnss_process(const char *buf, size_t buf_len);

/** @brief Query which A-GNSS elements were actually received
 *
 * @param received_elements return copy of requested elements received
 * since A-GNSS request was made
 */
void nrf_cloud_agnss_processed(struct nrf_modem_gnss_agnss_data_frame *received_elements);

/** @brief Query whether A-GNSS data has been requested from cloud
 *
 * @return True if request is outstanding.
 */
bool nrf_cloud_agnss_request_in_progress(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_AGNSS_H_ */
