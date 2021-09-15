/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file srest_client.h
 *
 * @brief nRF Cloud REST API.
 */
#ifndef SREST_REST_CLIENT_H__
#define SREST_REST_CLIENT_H__

#include <net/http_parser.h>

#define SREST_CLIENT_NO_SEC -1 /* No TLS */
#define SREST_CLIENT_TLS_DEFAULT_PEER_VERIFY -1

#define SREST_CLIENT_SCKT_CONNECT -1 /* sREST client lib does a sckt connection */

/** @brief Some common HTTP status codes */
enum srest_http_status {
	SREST_HTTP_STATUS_OK = 200,
	SREST_HTTP_STATUS_BAD_REQ = 400,
	SREST_HTTP_STATUS_UNAUTH = 401,
	SREST_HTTP_STATUS_FORBIDDEN = 403,
	SREST_HTTP_STATUS_NOT_FOUND = 404,
};

/** @brief Parameters and data for using the sREST client API */
struct srest_req_resp_context {
	/** Request: */

	/** Connection socket; default SREST_CLIENT_SCKT_CONNECT and
	 * library will make the connection.
	 */
	int connect_socket;

	/** If the connection should remain after API call. Default: false. */
	bool keep_alive;

	/** Security tag. Default SREST_CLIENT_NO_SEC and TLS will not be used. */
	int sec_tag;

	/** Indicates the preference for peer verification.
	 * Initialize to SREST_CLIENT_TLS_DEFAULT_PEER_VERIFY
	 * and default (TLS_PEER_VERIFY_REQUIRED) is used.
	 */
	int tls_peer_verify;

	/** Used HTTP method. */
	enum http_method http_method;

	/** Hostname to be used in the request. */
	const char *host;

	/** Port number to be used in the request. */
	uint16_t port;

	/** The URL for this request, for example: /index.html */
	const char *url;

	/** The HTTP header fields. This is a NULL terminated list of header fields.
	 * May be NULL.
	 */
	const char **header_fields;

	/** Payload/body, may be NULL. */
	char *body;

	/** Response: */

	/** User given timeout value for receiving a response data.
	 * Default: CONFIG_SREST_CLIENT_LIB_REST_REQUEST_TIMEOUT
	 */
	int32_t timeout_ms;

	/** User allocated buffer for receiving API response.*/
	char *resp_buff;

	/** Size of resp_buff */
	size_t resp_buff_len;

	/** Out: start of response data in resp_buff */
	char *response;

	/** Out: Length of response data */
	size_t response_len;

	/** Out: Numeric HTTP status code which corresponds to the
	 * textual description.
	 */
	uint16_t http_status_code;
};

/**
 * @brief Simple REST request.
 *
 * @param[in,out] req_resp_ctx Request and response context for communicating with Simple REST API.
 *
 * @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int srest_client_request(struct srest_req_resp_context *req_resp_ctx);

/**
 * @brief Sets the default values into given contexts.
 *
 * @details Intended to be used before calling srest_client_request() with more custom parameters.
 * 
 * @param[in,out] req_resp_ctx Request and response context for communicating with Simple REST API.
 */
void srest_client_request_defaults_set(struct srest_req_resp_context *req_resp_ctx);

#endif /* SREST_REST_CLIENT_H__ */
