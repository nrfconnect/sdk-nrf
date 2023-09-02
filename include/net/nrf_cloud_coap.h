/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_COAP_H_
#define NRF_CLOUD_COAP_H_

/** @file nrf_cloud_coap.h
 * @brief Module to provide nRF Cloud CoAP API
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_codec.h>
#include <zephyr/net/coap_client.h>

/**
 * @defgroup nrf_cloud_coap nRF CoAP API
 * @{
 */

/* Transport functions */
/** @brief Initialize nRF Cloud CoAP library.
 *
 * @return 0 if initialization was successful, otherwise, a negative error number.
 */
int nrf_cloud_coap_init(void);

/**
 * @brief Connect to and obtain authorization to access the nRF Cloud CoAP server.
 *
 * This function must return 0 indicating success so that the other functions below,
 * other than nrf_cloud_coap_disconnect(), will not immediately return an error when called.
 *
 * @param app_ver Version to report to the shadow; can be NULL.
 *
 * @return 0 if authorized successfully, otherwise, a negative error number.
 */
int nrf_cloud_coap_connect(const char * const app_ver);

/**
 * @brief Pause CoAP connection.
 *
 * This function temporarily pauses the nRF Cloud CoAP connection so that
 * another DTLS socket can be opened and used. Once the new socket is no longer needed,
 * close it and use nrf_cloud_coap_resume() to resume using CoAP.
 * Do not call nrf_cloud_coap_disconnect() nor shut down the LTE connection,
 * or the requisite data for the socket will be discarded in the modem,
 * and the connection cannot be resumed. In that case, call nrf_cloud_coap_connect(),
 * which results in a full DTLS handshake.
 *
 * @retval -EACCES if DTLS CID was not active or the connection was not authenticated.
 * @retval -EAGAIN if an error occurred while saving the connection; it is still usable.
 * @retval -EINVAL if the operation could not be performed.
 * @retval -ENOMEM if too many connections are already saved (four).
 * @retval 0 If successful.
 */
int nrf_cloud_coap_pause(void);

/**
 * @brief Resume CoAP connection.
 *
 * This function restores a previous connection for use.
 *
 * @retval -EACCES if the connection was not previously paused.
 * @retval -EAGAIN if an error occurred while loading the connection.
 * @retval -EINVAL if the operation could not be performed.
 * @retval 0 If successful.
 */
int nrf_cloud_coap_resume(void);

/**
 * @brief Disconnect the nRF Cloud CoAP connection.
 *
 * This does not teardown the thread in coap_client, as there is no way to do so.
 * The thread's call to poll(sock) will fail, resulting in an error message.
 * This is expected. Call nrf_cloud_coap_connect() to re-establish the connection, and
 * the thread in coap_client will resume.
 *
 * @return 0 if the socket was closed successfully, or a negative error number.
 */
int nrf_cloud_coap_disconnect(void);

/* nRF Cloud service functions */

/**
 * @brief Request nRF Cloud CoAP Assisted GPS (A-GPS) data.
 *
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result Structure pointing to caller-provided buffer in which to store A-GPS data.
 *
 *  @retval -EINVAL will be returned, and an error message printed, if invalid parameters
 *          are given.
 *  @retval -ENOENT will be returned if there was no A-GPS data requested for the specified
 *          request type.
 *  @retval -ENOBUFS will be returned, and an error message printed, if there is not enough
 *          buffer space to store retrieved AGPS data.
 * @retval 0 If successful.
 */
int nrf_cloud_coap_agps_data_get(struct nrf_cloud_rest_agps_request const *const request,
				 struct nrf_cloud_rest_agps_result *result);

/**
 * @brief Request URL for nRF Cloud Predicted GPS (P-GPS) data.
 *
 *  After a successful call to this function, pass the file_location to
 *  nrf_cloud_pgps_update(), which then downloads and processes the file's binary P-GPS data.
 *
 * @param[in]     request       Data to be provided in API call.
 * @param[in,out] file_location Structure that will contain the host and path to
 *                              the prediction file.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_coap_pgps_url_get(struct nrf_cloud_rest_pgps_request const *const request,
				 struct nrf_cloud_pgps_result *file_location);

/**
 * @brief Send a sensor value to nRF Cloud.
 *
 *  The CoAP message is sent as a non-confirmable CoAP message.
 *
 * @param[in]     app_id The app_id identifying the type of data. See the values in nrf_cloud_defs.h
 *                       that begin with  NRF_CLOUD_JSON_APPID_. You may also use custom names.
 * @param[in]     value  Sensor reading.
 * @param[in]     ts_ms  Timestamp the data was measured, or NRF_CLOUD_NO_TIMESTAMP.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_coap_sensor_send(const char *app_id, double value, int64_t ts_ms);

/**
 * @brief Send a message string to nRF Cloud.
 *
 *  The CoAP message is sent as a non-confirmable CoAP message.
 *
 * @param[in]     app_id     The app_id identifying the type of data. See the values in
 *                           nrf_cloud_defs.h that begin with  NRF_CLOUD_JSON_APPID_.
 *                           You may also use custom names.
 * @param[in]     message    The string to send.
 * @param[in]     json       Set true if the data should be sent in JSON format, otherwise CBOR.
 * @param[in]     ts_ms      Timestamp the data was measured, or NRF_CLOUD_NO_TIMESTAMP.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_coap_message_send(const char *app_id, const char *message, bool json, int64_t ts_ms);

/**
 * @brief Send a preencoded JSON message to nRF Cloud.
 *
 *  The CoAP message is sent as a non-confirmable CoAP message.
 *
 * @param[in]     message    The string to send.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_coap_json_message_send(const char *message);

/**
 * @brief Send the device location in the @ref nrf_cloud_gnss_data PVT field to nRF Cloud.
 *
 *  The CoAP message is sent as a non-confirmable CoAP message. Only
 *  @ref NRF_CLOUD_GNSS_TYPE_PVT is supported.
 *
 * @param[in]     gnss A pointer to an @ref nrf_cloud_gnss_data struct indicating the device
 *                     location, usually as determined by the GNSS unit.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_coap_location_send(const struct nrf_cloud_gnss_data * const gnss);

/**
 * @brief Request device location from nRF Cloud.
 *
 * At least one of cell_info or wifi_info must be provided within the request.
 *
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result Location information.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_location_get(struct nrf_cloud_rest_location_request const *const request,
				struct nrf_cloud_location_result *const result);

/**
 * @brief Request current nRF Cloud FOTA job info for the specified device.
 *
 * @param[out]    job Parsed job info. If no job exists, type will
 *                    be set to invalid. If a job exists, user must call
 *                    @ref nrf_cloud_coap_fota_job_free to free the memory
 *                    allocated by this function.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job);

/**
 * @brief Free memory allocated by nrf_cloud_coap_current_fota_job_get().
 *
 * @param[in,out] job Job info to be freed.
 *
 */
void nrf_cloud_coap_fota_job_free(struct nrf_cloud_fota_job_info *const job);

/**
 * @brief Update the status of the specified nRF Cloud FOTA job.
 *
 * @param[in]     job_id Null-terminated FOTA job identifier.
 * @param[in]     status Status of the FOTA job.
 * @param[in]     details Null-terminated string containing details of the
 *                job, such as an error description.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_fota_job_update(const char *const job_id,
	const enum nrf_cloud_fota_status status, const char * const details);

/**
 * @brief Query the device's shadow delta.
 *
 * If there is no delta, the return value will be 0 and the length of the string stored
 * in buf will be 0.
 *
 * @param[in,out] buf     Pointer to memory in which to receive the delta.
 * @param[in]     buf_len Size of buffer.
 * @param[in]     delta   True to request only changes in the shadow, if any; otherwise,
 *                        all of desired part.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_shadow_get(char *buf, size_t buf_len, bool delta);

/**
 * @brief Update the device's "state" in the shadow via the UpdateDeviceState endpoint.
 *
 * @param[in]     shadow_json Null-terminated JSON string to be written to the device's shadow.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_shadow_state_update(const char * const shadow_json);

/**
 * @brief Update the device status in the shadow.
 *
 * @param[in]     dev_status Device status to be encoded.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_shadow_device_status_update(const struct nrf_cloud_device_status
					       *const dev_status);

/**
 * @brief Update the device's "serviceInfo" in the shadow.
 *
 * @param[in]     svc_inf Service info items to be updated in the shadow.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_shadow_service_info_update(const struct nrf_cloud_svc_info * const svc_inf);

/**
 * @brief Send an nRF Cloud object
 *
 * This only supports sending of the CoAP CBOR or JSON type or a pre-encoded CBOR buffer.
 *
 * @param[in]     obj An nRF Cloud object. Will be encoded first if obj->enc_src is
 * NRF_CLOUD_ENC_SRC_NONE.
 *
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_obj_send(struct nrf_cloud_obj *const obj);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_COAP_H_ */
