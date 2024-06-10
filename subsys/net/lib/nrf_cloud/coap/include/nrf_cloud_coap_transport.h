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
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include <net/nrf_cloud_coap.h>

struct nrf_cloud_coap_client {
	struct k_mutex mutex;
	struct coap_client cc;
	int sock;
	bool initialized;
	bool authenticated;
	bool cid_saved;
	bool paused;
};

#define NRF_CLOUD_COAP_PROXY_RSC "proxy"

/**
 * @defgroup nrf_cloud_coap_transport nRF CoAP API
 * @{
 */

/**@brief Initialize the provided nrf_cloud_coap_client.
 *
 * @param client Client to initialize.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_init(struct nrf_cloud_coap_client *const client);

/**@brief Connect to the cloud.
 *
 * @param client Client to connect.

 * @retval 1 Socket is already open.
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_connect(struct nrf_cloud_coap_client *const client);

/**@brief Disconnect from the cloud.
 *
 * @param client Client to disconnect.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_disconnect(struct nrf_cloud_coap_client *const client);

/**@brief Perform authentication with the cloud.
 *
 * @param client Client to authenticate.
 *
 * @retval -ENOTCONN Client is not connected.
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_authenticate(struct nrf_cloud_coap_client *const client);

/**@brief Pause the CoAP connection.
 *
 * @param client Client to pause.
 *
 * @retval -EINVAL Client cannot be NULL.
 * @retval -EBADF Device is disconnected.
 * @retval -EACCES Unable to pause; device was not using CID or not authenticated.
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_pause(struct nrf_cloud_coap_client *const client);

/**@brief Resume a paused CoAP connection.
 *
 * @param client Client to resume.
 *
 * @retval -EINVAL Client cannot be NULL or DTLS CID not supported with current mfw.
 * @retval -EAGAIN Failed to load DTLS CID session.
 * @retval -EACCES Unable to resume because DTLS CID was not previously saved.
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_resume(struct nrf_cloud_coap_client *const client);

/**@brief Get the CoAP options required to perform a proxy download.
 *
 * @param opt_accept	Option to be populated with COAP_OPTION_ACCEPT details.
 * @param opt_proxy_uri	Option to be populated with COAP_OPTION_PROXY_URI details.
 * @param host		Download host.
 * @param path		Download file path.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
int nrf_cloud_coap_transport_proxy_dl_opts_get(struct coap_client_option *const opt_accept,
					       struct coap_client_option *const opt_proxy_uri,
					       char const *const host, char const *const path);

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
		       const uint8_t *buf, size_t len,
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
			const uint8_t *buf, size_t len,
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
		       const uint8_t *buf, size_t len,
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
			  const uint8_t *buf, size_t len,
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
			 const uint8_t *buf, size_t len,
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
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt, bool reliable,
			 coap_client_response_cb_t cb, void *user);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_COAP_TRANSPORT_H_ */
