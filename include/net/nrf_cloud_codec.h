/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CODEC_H__
#define NRF_CLOUD_CODEC_H__

#include <net/nrf_cloud.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_codec nRF Cloud Codec
 * @{
 */

/**
 * @brief Add GNSS location data, formatted as an nRF Cloud device message,
 *        to the provided cJSON object.
 *
 * @param[in]     gnss     Service info to add.
 * @param[in,out] gnss_msg_obj cJSON object to which GNSS data will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_gnss_msg_json_encode(const struct nrf_cloud_gnss_data * const gnss,
				   cJSON * const gnss_msg_obj);

/**
 * @brief Check for a JSON error message in data received from nRF Cloud via MQTT.
 *
 * @param[in] buf Data received from nRF Cloud.
 * @param[in] app_id appId value to check for.
 *                   Set to NULL to skip appID check.
 * @param[in] msg_type messageType value to check for.
 *                     Set to NULL to skip messageType check.
 * @param[out] err Error code found in message.
 *
 * @retval 0 Error code found (and matched app_id and msg_type if provided).
 * @retval -ENOENT Error code found, but did not match specified app_id and msg_type.
 * @retval -ENOMSG No error code found.
 * @retval -EBADMSG Invalid error code data format.
 * @retval -ENODATA JSON data was not found.
 * @return A negative value indicates an error.
 */
int nrf_cloud_error_msg_decode(const char * const buf,
			       const char * const app_id,
			       const char * const msg_type,
			       enum nrf_cloud_error * const err);

/**
 * @brief Add service info into the provided cJSON object.
 *
 * @param[in]     svc_inf     Service info to add.
 * @param[in,out] svc_inf_obj cJSON object to which service info will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_service_info_json_encode(const struct nrf_cloud_svc_info * const svc_inf,
				       cJSON * const svc_inf_obj);

/**
 * @brief Add modem info into the provided cJSON object.
 *
 * @note To add modem info, CONFIG_MODEM_INFO must be enabled.
 *
 * @param[in]     mod_inf     Modem info to add.
 * @param[in,out] mod_inf_obj cJSON object to which modem info will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_modem_info_json_encode(const struct nrf_cloud_modem_info * const mod_inf,
				     cJSON * const mod_inf_obj);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CODEC_H__ */
