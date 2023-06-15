/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file nrf_provisioning_http.h
 *
 * @brief nRF Provisioning HTTP REST API.
 */
#ifndef NRF_PROVISIONING_HTTP_H__
#define NRF_PROVISIONING_HTTP_H__

#include <stdbool.h>

#include <net/nrf_provisioning.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_provisioning_http nRF Device Provisioning API
 * @ingroup nrf_provisioning
 * @{
 */

/** @brief HTTP status codes returned from nRF Device Provisioning Service. */
enum nrf_provisioning_http_status {
	NRF_PROVISIONING_HTTP_STATUS_UNHANDLED = -1,
	NRF_PROVISIONING_HTTP_STATUS_NONE = 0,
	NRF_PROVISIONING_HTTP_STATUS_OK = 200,
	NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT = 204,
	NRF_PROVISIONING_HTTP_STATUS__ERROR_BEGIN = 400,
	NRF_PROVISIONING_HTTP_STATUS_BAD_REQ = NRF_PROVISIONING_HTTP_STATUS__ERROR_BEGIN,
	NRF_PROVISIONING_HTTP_STATUS_UNAUTH = 401,
	NRF_PROVISIONING_HTTP_STATUS_FORBIDDEN = 403,
	NRF_PROVISIONING_HTTP_STATUS_NOT_FOUND = 404,
	NRF_PROVISIONING_HTTP_STATUS_UNS_MEDIA_TYPE = 415,
	NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR = 500,
};

#define NRF_PROVISIONING_TIMEOUT_MINIMUM (5000)
#define NRF_PROVISIONING_TIMEOUT_NONE (SYS_FOREVER_MS)

int nrf_provisioning_http_init(struct nrf_provisioning_mm_change *mmode);

/** @brief Parameters and data for using the nRF Cloud REST API */
struct nrf_provisioning_http_context {
	/** Connection socket; initialize to -1 and library
	 * will make the connection.
	 */
	int connect_socket;
	/** If the connection should remain after API call.
	 * @note A failed API call could result in the socket
	 * being closed.
	 */
	bool keep_alive;
	/** Timeout value, in milliseconds, for receiving response data.
	 * Minimum timeout value specified by NRF_PROVISIONING_TIMEOUT_MINIMUM.
	 * For no timeout, set to NRF_PROVISIONING_TIMEOUT_NONE.
	 * @note This parameter is currently not used; set
	 * CONFIG_REST_CLIENT_SCKT_RECV_TIMEOUT instead.
	 */
	int32_t timeout_ms;
	/** Authentication string: Bearer Token
	 * If no JWT is provided, and CONFIG_NRF_PROVISIONING_AUTOGEN_JWT
	 * is enabled, then one will be generated automatically with
	 * CONFIG_NRF_PROVISIONING_AUTOGEN_JWT_VALID_TIME_S as its lifetime in
	 * seconds.
	 * If Attestation Token based authentication is enabled leaving this empty is mandatory,
	 * unless a static token is used for testing purposes.
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

	/** Results from API call */
	/** HTTP status of API call */
	enum nrf_provisioning_http_status status;
	/** Start of response content data in rx_buf */
	char *response;
	/** Length of response content data */
	size_t response_len;
	/** Length of HTTPS headers + response content data */
	size_t total_response_len;
};

/**
 * @brief Closes the connection to the server.
 *	  The socket pointed to by @p rest_ctx.connect_socket will be closed,
 *	  and @p rest_ctx.connect_socket will be set to -1.
 *
 * @param[in,out] rest_ctx Context for communicating with nRF Provisioning service's HTTP API.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned:
 *	    - -EINVAL, if valid context is not given.
 *	    - -ENOTCONN, if socket was already closed, or never opened.
 *	    - -EIO, for any kind of socket-level closure failure.
 *
 */
int nrf_provisioning_http_close(struct nrf_provisioning_http_context *const rest_ctx);

/**
 * @brief Performs Provisioning of the device.
 *
 * @retval 0 if received commands have been processed successfully
 * @retval Positive value if provisioning is finished successfully
 * @retval Otherwise, a negative error code is returned.
 */
int nrf_provisioning_http_req(struct nrf_provisioning_http_context *const rest_ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_HTTP_H__ */
