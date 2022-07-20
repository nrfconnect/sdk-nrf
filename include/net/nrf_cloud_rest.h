/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_cloud_rest.h
 *
 * @brief nRF Cloud REST API.
 */
#ifndef NRF_CLOUD_REST_H__
#define NRF_CLOUD_REST_H__

#include <zephyr/types.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_cell_pos.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_rest nRF Cloud REST API
 * @{
 */

/** @brief HTTP status codes returned from nRF Cloud */
enum nrf_cloud_http_status {
	NRF_CLOUD_HTTP_STATUS_UNHANDLED = -1,
	NRF_CLOUD_HTTP_STATUS_NONE = 0,
	NRF_CLOUD_HTTP_STATUS_OK = 200,
	NRF_CLOUD_HTTP_STATUS_ACCEPTED = 202,
	NRF_CLOUD_HTTP_STATUS_PARTIAL = 206,
	NRF_CLOUD_HTTP_STATUS__ERROR_BEGIN = 400,
	NRF_CLOUD_HTTP_STATUS_BAD_REQ = NRF_CLOUD_HTTP_STATUS__ERROR_BEGIN,
	NRF_CLOUD_HTTP_STATUS_UNAUTH = 401,
	NRF_CLOUD_HTTP_STATUS_FORBIDDEN = 403,
	NRF_CLOUD_HTTP_STATUS_NOT_FOUND = 404,
	NRF_CLOUD_HTTP_STATUS_BAD_RANGE = 416,
	NRF_CLOUD_HTTP_STATUS_UNPROC_ENTITY = 422,
};

/** @brief nRF Cloud AGPS REST request types */
enum nrf_cloud_rest_agps_req_type {
	NRF_CLOUD_REST_AGPS_REQ_ASSISTANCE,
	NRF_CLOUD_REST_AGPS_REQ_LOCATION,
	NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
	NRF_CLOUD_REST_AGPS_REQ_UNSPECIFIED,
};

#define NRF_CLOUD_REST_TIMEOUT_NONE		(SYS_FOREVER_MS)

/** @brief Parameters and data for using the nRF Cloud REST API */
struct nrf_cloud_rest_context {

	/** Connection socket; initialize to -1 and library
	 * will make the connection.
	 */
	int connect_socket;
	/** If the connection should remain after API call.
	 * @note A failed API call could result in the socket
	 * being closed.
	 */
	bool keep_alive;
	/** Timeout value, in milliseconds, for REST request. The timeout is set individually
	 * for socket connection creation and data transfer meaning REST request can take
	 * longer than this given timeout.
	 * For no timeout, set to NRF_CLOUD_REST_TIMEOUT_NONE.
	 */
	int32_t timeout_ms;
	/** Authentication string: JWT @ref modem_jwt.
	 * The nRF Cloud device ID must be included in the sub claim.
	 * If the device ID is the device's internal UUID, the sub claim
	 * can be omitted for modem generated JWTs; the UUID is included
	 * in the iss claim.
	 * If no JWT is provided, and CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT
	 * is enabled, then one will be generated automatically with
	 * CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT_VALID_TIME_S as its lifetime in
	 * seconds.
	 */
	char *auth;
	/** User allocated buffer for receiving API response, which
	 * includes the HTTPS headers, any response data and a terminating
	 * NULL.
	 * Buffer size should be limited according to the
	 * maximum TLS receive size of the modem.
	 */
	char *rx_buf;
	/** Size of rx_buf */
	size_t rx_buf_len;
	/** Fragment size for downloads, set to zero to use
	 * CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE.
	 * The rx_buf must be able to hold the HTTPS headers
	 * plus this fragment size.
	 */
	size_t fragment_size;

	/** Results from API call */
	/** HTTP status of API call */
	enum nrf_cloud_http_status status;
	/** Start of response content data in rx_buf */
	char *response;
	/** Length of response content data */
	size_t response_len;
	/** Length of HTTPS headers + response content data */
	size_t total_response_len;

	/** Error code from nRF Cloud */
	enum nrf_cloud_error nrf_err;
};

/** @brief Data required for nRF Cloud cellular positioning request */
struct nrf_cloud_rest_cell_pos_request {
	/** Network information used in request */
	struct lte_lc_cells_info *net_info;
};

/** @brief Data required for nRF Cloud Assisted GPS (A-GPS) request */
struct nrf_cloud_rest_agps_request {
	enum nrf_cloud_rest_agps_req_type type;
	/** Required for custom request type */
	struct nrf_modem_gnss_agps_data_frame *agps_req;
	/** Optional; provide network info or set to NULL. The cloud cannot
	 * provide location assistance data if network info is NULL.
	 */
	struct lte_lc_cells_info *net_info;
	/** Reduce set of ephemerides to only those visible at current
	 * location.  This reduces the overall size of the download, but
	 * may increase fix times towards the end of the validity period
	 * and/or if the device is actively traveling long distances.
	 */
	bool filtered;
	/** Constrain the set of ephemerides to only those currently
	 *  visible at or above the specified elevation threshold
	 *  angle in degrees. Range is 0 to 90.
	 */
	uint8_t mask_angle;
};

/** @brief nRF Cloud Assisted GPS (A-GPS) result */
struct nrf_cloud_rest_agps_result {
	/** User-provided buffer to hold AGPS data */
	char *buf;
	/** Size of user-provided buffer */
	size_t buf_sz;
	/** Size of the AGPS data copied into buffer */
	size_t agps_sz;
};

/** @defgroup nrf_cloud_rest_pgps_omit Omit item from P-GPS request.
 * @{
 */
#define NRF_CLOUD_REST_PGPS_REQ_NO_COUNT	0
#define NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL	0
#define NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY	0
#define NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD	(-1)
/** @} */

/** @brief Data required for nRF Cloud Predicted GPS (P-GPS) request */
struct nrf_cloud_rest_pgps_request {
	/** Data to be included in the P-GPS request. To omit an item
	 * use the appropriate define in @ref nrf_cloud_rest_pgps_omit
	 */
	const struct gps_pgps_request *pgps_req;
};

/**
 * @brief nRF Cloud location request.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result Optional; parsed results of API response.
 *                       If NULL, user should inspect the response buffer of
 *                       @ref nrf_cloud_rest_context.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_cell_pos_get(struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_cell_pos_request const *const request,
	struct nrf_cloud_cell_pos_result *const result);

/**
 * @brief nRF Cloud Assisted GPS (A-GPS) data request.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     request Data to be provided in API call.
 * @param[in,out] result Optional; Additional buffer for A-GPS data. This is
 *                       necessary when the A-GPS data from the cloud is larger
 *                       than the fragment size specified by
 *                       rest_ctx->fragment_size.
 *
 * @retval 0 If successful.
 *           If result is NULL and the A-GPS data is larger than the fragment
 *           size specified by rest_ctx->fragment_size, a positive value is
 *           returned, which indicates the size (in bytes) of the necessary
 *           result buffer.
 *           Otherwise, a (negative) error code is returned:
 *           - -EINVAL will be returned, and an error message printed, if invalid parameters
 *		are given.
 *           - -ENOBUFS will be returned, and an error message printed, if there is not enough
 *             buffer space to store retrieved AGPS data.
 *           - See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for all
 *             other possible error codes.
 *
 */
int nrf_cloud_rest_agps_data_get(struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_agps_request const *const request,
	struct nrf_cloud_rest_agps_result *const result);

/**
 * @brief nRF Cloud Predicted GPS (P-GPS) data request.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 *                         If successful, rest_ctx->result will point to the P-GPS
 *                         data; which, along with rest_ctx->response_len, can be
 *                         passed into @ref nrf_cloud_pgps_process.
 * @param[in]     request Data to be provided in API call.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_pgps_data_get(struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_pgps_request const *const request);

/**
 * @brief Requests nRF Cloud FOTA job info for the specified device.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with
 *                          nRF Cloud.
 * @param[out]    job Optional; parsed job info. If no job exists, type will
 *                    be set to invalid. If a job exists, user must call
 *                    @ref nrf_cloud_rest_fota_job_free to free the memory
 *                    allocated by this function.
 *                    If NULL, user should inspect the response buffer of
 *                    @ref nrf_cloud_rest_context.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_fota_job_get(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, struct nrf_cloud_fota_job_info *const job);

/**
 * @brief Frees memory allocated by @ref nrf_cloud_rest_fota_job_get.
 *
 * @param[in,out] job Job info to be freed.
 *
 */
void nrf_cloud_rest_fota_job_free(struct nrf_cloud_fota_job_info *const job);

/**
 * @brief Updates the status of the specified nRF Cloud FOTA job.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with
 *                          nRF Cloud.
 * @param[in]     job_id Null-terminated FOTA job identifier.
 * @param[in]     status Status of the FOTA job.
 * @param[in]     details Null-terminated string containing details of the
 *                        job, such as an error description.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_fota_job_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char *const job_id,
	const enum nrf_cloud_fota_status status, const char * const details);

/**
 * @brief Updates the device's "state" in the shadow via the UpdateDeviceState endpoint.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     shadow_json Null-terminated JSON string to be written to the device's shadow.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_shadow_state_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char * const shadow_json);

/**
 * @brief Updates the device's "ServiceInfo" in the shadow.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     svc_inf Service info items to be updated in the shadow.
 *
 * @retval 0 If successful.
 *         Otherwise, a (negative) error code is returned.
 *         See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_shadow_service_info_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_svc_info * const svc_inf);

/**
 * @brief Update the device status in the shadow.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     dev_status Device status to be encoded.
 *
 * @retval 0 If successful.
 *         Otherwise, a (negative) error code is returned.
 *         See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_shadow_device_status_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_device_status *const dev_status);

/**
 * @brief Closes the connection to the server.
 *	  The socket pointed to by @p rest_ctx.connect_socket will be closed,
 *	  and @p rest_ctx.connect_socket will be set to -1.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned:
 *	    - -EINVAL, if valid context is not given.
 *	    - -ENOTCONN, if socket was already closed, or never opened.
 *	    - -EIO, for any kind of socket-level closure failure.
 *
 */
int nrf_cloud_rest_disconnect(struct nrf_cloud_rest_context *const rest_ctx);

/**
 * @brief Performs just-in-time provisioning (JITP) with nRF Cloud.
 *
 * @note After a device has been provisioned with nRF Cloud, it must be
 * associated with an nRF Cloud account before using any other functions in this
 * library.
 *
 * @param[in] nrf_cloud_sec_tag Modem sec_tag containing nRF Cloud JITP credentials
 *
 * @retval 0 If successful; wait 30s before associating device with nRF Cloud account.
 * @retval 1 Device is already provisioned.
 *         Otherwise, a (negative) error code is returned:
 *         - Any underlying socket or HTTP response error will be returned directly.
 *	   - -ENODEV will be returned if device JITP immediately rejected.
 */
int nrf_cloud_rest_jitp(const sec_tag_t nrf_cloud_sec_tag);

/**
 * @brief Send a JSON formatted device message using the SendDeviceMessage endpoint.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     json_msg Null-terminated JSON string containing the device message.
 * @param[in]     bulk Use the bulk message topic. If true, the topic parameter is ignored.
 * @param[in]     topic Optional; null-terminated MQTT topic string.
 *                      If NULL, the d2c topic is used.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_send_device_message(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char *const json_msg, const bool bulk,
	const char *const topic);

/**
 * @brief Send GNSS data to nRF Cloud as a device message.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Cloud's REST API.
 * @param[in]     device_id Null-terminated, unique device ID registered with nRF Cloud.
 * @param[in]     gnss GNSS location data.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 *          See @verbatim embed:rst:inline :ref:`nrf_cloud_rest_failure` @endverbatim for details.
 */
int nrf_cloud_rest_send_location(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_gnss_data * const gnss);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_REST_H__ */
