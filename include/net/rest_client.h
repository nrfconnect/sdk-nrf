/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file rest_client.h
 *
 * @brief REST Client.
 */
#ifndef REST_CLIENT_H__
#define REST_CLIENT_H__

#include <net/http_parser.h>

#define REST_CLIENT_NO_SEC -1 /* No TLS */
#define REST_CLIENT_TLS_DEFAULT_PEER_VERIFY -1

#define REST_CLIENT_SCKT_CONNECT -1 /* REST client does a sckt connection */

/** @brief Some common HTTP status codes */
enum rest_client_http_status {
	REST_CLIENT_HTTP_STATUS_OK = 200,
	REST_CLIENT_HTTP_STATUS_BAD_REQ = 400,
	REST_CLIENT_HTTP_STATUS_UNAUTH = 401,
	REST_CLIENT_HTTP_STATUS_FORBIDDEN = 403,
	REST_CLIENT_HTTP_STATUS_NOT_FOUND = 404,
};

/** @brief Parameters and data for using the REST client API */
struct rest_client_req_resp_context {
	/** Request: */

	/** In: socket identifier for the connection. default: REST_CLIENT_SCKT_CONNECT and
	 *      library will make a new socket connection.
	 */
	int connect_socket;

	/** In: If the connection should remain after API call. Default: false. */
	bool keep_alive;

	/** In: Security tag. Default REST_CLIENT_NO_SEC and TLS will not be used. */
	int sec_tag;

	/** In: Indicates the preference for peer verification.
	 *      Initialize to REST_CLIENT_TLS_DEFAULT_PEER_VERIFY
	 *      and the default (TLS_PEER_VERIFY_REQUIRED) is used.
	 */
	int tls_peer_verify;

	/** In: Used HTTP method. */
	enum http_method http_method;

	/** In: Hostname to be used in the request. */
	const char *host;

	/** In: Port number to be used in the request. */
	uint16_t port;

	/** In: The URL for this request, for example: /index.html */
	const char *url;

	/** In: The HTTP header fields. Similar to Zephyr http client.
	 *      This is a NULL terminated list of header fields. May be NULL.
	 */
	const char **header_fields;

	/** In: Payload/body, may be NULL. */
	char *body;

	/** Response: */

	/** In: User given timeout value for receiving a response data.
	 *      Default: CONFIG_REST_CLIENT_REST_REQUEST_TIMEOUT
	 */
	int32_t timeout_ms;

	/** In: User allocated buffer for receiving API response.*/
	char *resp_buff;

	/** In: User given size of resp_buff */
	size_t resp_buff_len;

	/** Out: Length of HTTP headers + response body/content data */
	size_t total_response_len;

	/** Out: Length of response content data */
	size_t response_len;

	/** Out: start of response data (i.e. the body/content) in resp_buff */
	char *response;

	/** Out: Numeric HTTP status code */
	uint16_t http_status_code;

	/** Out: Used socket identifier. Use this for keepalived connections as
	 *       connect_socket.
	 */
	int used_socket_id;

	/** Out: True if used_socket_id was kept alive and wasn't closed after the
	 *       REST request.
	 */
	int used_socket_is_alive;
};

/**
 * @brief REST client request.
 *
 * @param[in,out] req_resp_ctx Request and response context for communicating with REST Client API.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int rest_client_request(struct rest_client_req_resp_context *req_resp_ctx);

/**
 * @brief Sets the default values into given contexts.
 *
 * @details Intended to be used before calling rest_client_request() with more custom parameters.
 * 
 * @param[in,out] req_resp_ctx Request and response context for communicating with REST Client API.
 */
void rest_client_request_defaults_set(struct rest_client_req_resp_context *req_resp_ctx);

#endif /* REST_CLIENT_H__ */
