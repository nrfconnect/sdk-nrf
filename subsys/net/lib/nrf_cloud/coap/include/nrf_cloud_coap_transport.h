/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_COAP_TRANSPORT_H_
#define NRF_CLOUD_COAP_TRANSPORT_H_

/** @file nrf_cloud_coap_transport.h
 * @brief Module to provide nRF Cloud CoAP transport functionality
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <zephyr/net/coap_client.h>

/**
 * @defgroup nrf_cloud_coap_transport nRF CoAP API
 * @{
 */

/**@brief Check if device is connected and authorized to use nRF Cloud CoAP.
 *
 * A device is authorized if the JWT it POSTed to the /auth/jwt endpoint is valid
 * and matches credentials for a device already provisioned and associated with nRF Cloud.
 *
 * @retval true Device is allowed to access nRF Cloud services.
 * @retval false Device is disallowed from accessing nRF Cloud services.
 */
bool nrf_cloud_coap_is_connected(void);

/**@brief Perform CoAP GET request.
 *
 * The function will block until the response or an error have been returned.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the request.
 * @param len Length of payload or 0 if none.
 * @param fmt_out CoAP content format for the Content-Format message option of the payload.
 * @param fmt_in CoAP content format for the Accept message option of the returned payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_get(const char *resource, const char *query,
		       uint8_t *buf, size_t len,
		       enum coap_content_format fmt_out,
		       enum coap_content_format fmt_in, bool reliable,
		       coap_client_response_cb_t cb, void *user);

/**@brief Perform CoAP POST request.
 *
 * The function will block until the response or an error have been returned. Use this
 * function to send custom JSON or CBOR messages to nRF Cloud through the
 * https://api.nrfcloud.com/v1#tag/Messages/operation/SendDeviceMessage API.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the request.
 * @param len Length of payload or 0 if none.
 * @param fmt CoAP content format for the Content-Format message option of the payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_post(const char *resource, const char *query,
			uint8_t *buf, size_t len,
			enum coap_content_format fmt, bool reliable,
			coap_client_response_cb_t cb, void *user);

/**@brief Perform CoAP PUT request.
 *
 * The function will block until the response or an error have been returned.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the request.
 * @param len Length of payload or 0 if none.
 * @param fmt CoAP content format for the Content-Format message option of the payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_put(const char *resource, const char *query,
		       uint8_t *buf, size_t len,
		       enum coap_content_format fmt, bool reliable,
		       coap_client_response_cb_t cb, void *user);

/**@brief Perform CoAP DELETE request.
 *
 * The function will block until the response or an error have been returned.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the request.
 * @param len Length of payload or 0 if none.
 * @param fmt CoAP content format for the Content-Format message option of the payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_delete(const char *resource, const char *query,
			  uint8_t *buf, size_t len,
			  enum coap_content_format fmt, bool reliable,
			  coap_client_response_cb_t cb, void *user);

/**@brief Perform CoAP FETCH request.
 *
 * The function will block until the response or an error have been returned.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the request.
 * @param len Length of payload or 0 if none.
 * @param fmt_out CoAP content format for the Content-Format message option of the payload.
 * @param fmt_in CoAP content format for the Accept message option of the returned payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_fetch(const char *resource, const char *query,
			 uint8_t *buf, size_t len,
			 enum coap_content_format fmt_out,
			 enum coap_content_format fmt_in, bool reliable,
			 coap_client_response_cb_t cb, void *user);

/**@brief Perform CoAP PATCH request.
 *
 * The function will block until the response or an error have been returned.
 *
 * @param resource String containing the specific CoAP endpoint to access.
 * @param query Optional string containing REST-style query parameters.
 * @param buf Optional pointer to buffer containing a payload to include with the GET request.
 * @param len Length of payload or 0 if none.
 * @param fmt CoAP content format for the Content-Format message option of the payload.
 * @param reliable True to use a Confirmable message, otherwise, a Non-confirmable message.
 * @param cb Pointer to a callback function to receive the results.
 * @param user Pointer to user-specific data to be passed back to the callback.
 * @return 0 if the request succeeded, a positive value indicating a CoAP result code,
 * or a negative error number.
 */
int nrf_cloud_coap_patch(const char *resource, const char *query,
			 uint8_t *buf, size_t len,
			 enum coap_content_format fmt, bool reliable,
			 coap_client_response_cb_t cb, void *user);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_COAP_TRANSPORT_H_ */
