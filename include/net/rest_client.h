/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_client.h
 *
 * @defgroup rest_client REST client library
 * @{
 * @brief REST client.
 *
 * @details Provide REST client functionality.
 */

#ifndef REST_CLIENT_H__
#define REST_CLIENT_H__

#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief TLS is not used. */
#define REST_CLIENT_SEC_TAG_NO_SEC -1

/** @brief Use the default TLS peer verification; TLS_PEER_VERIFY_REQUIRED. */
#define REST_CLIENT_TLS_DEFAULT_PEER_VERIFY -1

/** @brief REST client opens a socket connection. */
#define REST_CLIENT_SCKT_CONNECT -1

/** @brief Common HTTP status codes */
enum rest_client_http_status {
	REST_CLIENT_HTTP_STATUS_OK = 200,
	REST_CLIENT_HTTP_STATUS_BAD_REQ = 400,
	REST_CLIENT_HTTP_STATUS_UNAUTH = 401,
	REST_CLIENT_HTTP_STATUS_FORBIDDEN = 403,
	REST_CLIENT_HTTP_STATUS_NOT_FOUND = 404,
};

/**
 * @brief REST client request context.
 *
 * @details Input parameters for using the REST client API should be filled into
 *          this structure before calling @ref rest_client_request.
 */
struct rest_client_req_context {
	/** Socket identifier for the connection. When using the default value,
	 *  the library will open a new socket connection. Default: REST_CLIENT_SCKT_CONNECT.
	 */
	int connect_socket;

	/** Defines whether the connection should remain after API call. Default: false. */
	bool keep_alive;

	/** Security tag. Default: REST_CLIENT_SEC_TAG_NO_SEC. */
	int sec_tag;

	/** Indicates the preference for peer verification.
	 *  Initialize to REST_CLIENT_TLS_DEFAULT_PEER_VERIFY
	 *  and the default (TLS_PEER_VERIFY_REQUIRED) is used.
	 */
	int tls_peer_verify;

	/** Used HTTP method. */
	enum http_method http_method;

	/** Hostname or IP address to be used in the request. */
	const char *host;

	/** Port number to be used in the request. */
	uint16_t port;

	/** The URL for this request, for example: /index.html */
	const char *url;

	/** The HTTP header fields. Similar to the Zephyr HTTP client.
	 *  This is a NULL-terminated list of header fields. May be NULL.
	 */
	const char **header_fields;

	/** Payload/body, may be NULL. */
	const char *body;

	/** Payload/body length */
	size_t body_len;

	/** User-defined timeout value for REST request. The timeout is set individually
	 *  for socket connection creation and data transfer meaning REST request can take
	 *  longer than this given timeout. To disable, set the timeout duration to SYS_FOREVER_MS.
	 *  A value of zero will result in an immediate timeout.
	 *  Default: CONFIG_REST_CLIENT_REST_REQUEST_TIMEOUT.
	 */
	int32_t timeout_ms;

	/** User-allocated buffer for receiving API response. */
	char *resp_buff;

	/** User-defined size of resp_buff. */
	size_t resp_buff_len;
};

/**
 * @brief REST client response context.
 *
 * @details When @ref rest_client_request returns, response-related data can be read from
 *          this structure.
 */
struct rest_client_resp_context {
	/** Length of HTTP headers + response body/content data. */
	size_t total_response_len;

	/** Length of response body/content data. */
	size_t response_len;

	/** Start of response data (the body/content) in resp_buff.*/
	char *response;

	/** Numeric HTTP status code. */
	uint16_t http_status_code;

	/** HTTP status code as a textual description, i.e. the reason-phrase element.
	 * https://tools.ietf.org/html/rfc7230#section-3.1.2
	 * Copied here from http_status[] of http_response.
	 */
	char http_status_code_str[HTTP_STATUS_STR_SIZE];

	/** Used socket identifier. Use this for keepalive connections as
	 *  connect_socket for upcoming requests.
	 */
	int used_socket_id;

	/** True if used_socket_id was kept alive and was not closed after the REST request. */
	int used_socket_is_alive;
};

/**
 * @brief REST client request.
 *
 * @details This function will block the calling thread until the request completes.
 *
 * @param[in] req_ctx Request context containing input parameters to REST request
 * @param[out] resp_ctx Response context for returning the response data.
 *
 * @retval 0, if the REST response was received successfully. If response_len > 0,
 *            there is also body/content data in a response. http_status_code contains the actual
 *            HTTP status code.
 *         Otherwise, a (negative) error code is returned.
 */
int rest_client_request(struct rest_client_req_context *req_ctx,
			struct rest_client_resp_context *resp_ctx);

/**
 * @brief Sets the default values into a given request context.
 *
 * @details Intended to be used before calling rest_client_request().
 *
 * @param[in,out] req_ctx Request context for communicating with REST client API.
 */
void rest_client_request_defaults_set(struct rest_client_req_context *req_ctx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* REST_CLIENT_H__ */
