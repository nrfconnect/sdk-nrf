/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#include <net/rest_client.h>
#include <zephyr/logging/log.h>
#include <cJSON.h>

#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"

LOG_MODULE_REGISTER(nrf_cloud_rest, CONFIG_NRF_CLOUD_REST_LOG_LEVEL);

#define HTTPS_PORT 443
#define BASE64_PAD_CHAR '='

#define UINT32_MAX_STR_SZ		10
#define RANGE_MAX_BYTES			CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE

#define API_VER				"/v1"

#define CONTENT_RANGE_RSP		"content-range: bytes"
#define CONTENT_RANGE_RSP_MIXED_CASE	"Content-Range: bytes"
#define CONTENT_RANGE_TOTAL_TOK		'/'

#define LF_TOK				'\n'
#define CR_TOK				'\r'
#define CRLF				"\r\n"

#define HDR_TYPE_TXT_PLAIN		"text/plain"
#define HDR_TYPE_ALL			 "*/*"
#define HDR_TYPE_APP_JSON		"application/json"
#define HDR_TYPE_APP_OCT_STR		"application/octet-stream"

#define CONTENT_TYPE			"Content-Type: "
#define CONTENT_TYPE_TXT_PLAIN		(CONTENT_TYPE HDR_TYPE_TXT_PLAIN CRLF)
#define CONTENT_TYPE_ALL		(CONTENT_TYPE HDR_TYPE_ALL CRLF)
#define CONTENT_TYPE_APP_JSON		(CONTENT_TYPE HDR_TYPE_APP_JSON CRLF)
#define CONTENT_TYPE_APP_OCT_STR	(CONTENT_TYPE HDR_TYPE_APP_OCT_STR CRLF)

#define AUTH_HDR_BEARER_PREFIX		"Authorization: Bearer "
#define HOST_HDR_TEMPLATE		"Host: %s" CRLF
#define HTTP_HDR_ACCEPT			"Accept: "
#define HDR_ACCEPT_APP_JSON		(HTTP_HDR_ACCEPT HDR_TYPE_APP_JSON CRLF)
#define HDR_ACCEPT_ALL			(HTTP_HDR_ACCEPT HDR_TYPE_ALL CRLF)
#define HDR_RANGE_BYTES_TEMPLATE	("Range: bytes=%u-%u" CRLF)
#define HDR_RANGE_BYTES_SZ		(sizeof(HDR_RANGE_BYTES_TEMPLATE) + \
					UINT32_MAX_STR_SZ + UINT32_MAX_STR_SZ)

#define API_FOTA_JOB_EXEC		"/fota-job-executions"
#define API_GET_FOTA_URL_TEMPLATE	(API_VER API_FOTA_JOB_EXEC "/%s/current")

#define API_LOCATION			"/location"
#define API_GET_LOCATION		API_VER API_LOCATION "/ground-fix"
#define API_GET_LOCATION_NO_REPLY	API_VER API_LOCATION "/ground-fix?doReply=0"
#define API_GET_AGPS_BASE		API_VER API_LOCATION "/agps"
#define API_GET_PGPS_BASE		API_VER API_LOCATION "/pgps"

#define API_DEVICES_BASE		"/devices"
#define API_DEVICES_STATE_TEMPLATE	API_VER API_DEVICES_BASE "/%s/state"
#define API_DEVICES_MSGS_TEMPLATE	API_VER API_DEVICES_BASE "/%s/messages"
#define API_DEVICES_MSGS_D2C_TPC_TMPLT	"d/%s/d2c%s"
#define API_DEVICES_MSGS_BULK		NRF_CLOUD_BULK_MSG_TOPIC

#define JITP_HOSTNAME_TLS	CONFIG_NRF_CLOUD_HOST_NAME
#define JITP_PORT		8443
#define JITP_URL		"/topics/jitp?qos=1"
#define JITP_CONTENT_TYPE_HDR	(CONTENT_TYPE HDR_TYPE_ALL CRLF)
#define JITP_HOST_HDR		"Host: " JITP_HOSTNAME_TLS ":" STRINGIFY(JITP_PORT) CRLF
#define JITP_CONNECTION_HDR	"Connection: close" CRLF
#define JITP_HTTP_TIMEOUT_MS	(15000)
#define JITP_RX_BUF_SZ		(400)

/* Generate an authorization header value string in the form:
 * "Authorization: Bearer JWT \r\n"
 */
static int generate_auth_header(const char *const tok, char **auth_hdr_out)
{
	if ((!tok && !IS_ENABLED(CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT))) {
		LOG_ERR("Cannot generate auth header, no token was given, "
				"and JWT autogen is not enabled");
		return -EINVAL;
	}
	if (!auth_hdr_out) {
		LOG_ERR("Cannot generate auth header, no output pointer given.");
		return -EINVAL;
	}

	/* These lengths NOT including null terminators */
	int tok_len = tok ? strlen(tok) : -1;
	int prefix_len = sizeof(AUTH_HDR_BEARER_PREFIX) - 1;
	int postfix_len = sizeof(CRLF) - 1;


#ifdef CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT
	if (!tok) {
		tok_len = CONFIG_MODEM_JWT_MAX_LEN;
	}
#endif /* CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT */

	if (tok_len <= 0) {
		LOG_ERR("Cannot generate auth header, non-positive token length of %d", tok_len);
		return -EINVAL;
	}

	*auth_hdr_out = nrf_cloud_malloc(prefix_len + tok_len + postfix_len + 1);
	if (!*auth_hdr_out) {
		return -ENOMEM;
	}

	char *prefix_ptr = *auth_hdr_out;
	char *tok_ptr = prefix_ptr + prefix_len;
	char *postfix_ptr;

	/* Write the prefix */
	memcpy(prefix_ptr, AUTH_HDR_BEARER_PREFIX, prefix_len);

	/* Copy the given token if it exists*/
	if (tok) {
		memcpy(tok_ptr, tok, tok_len);
	}

#ifdef CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT
	/* Generate a token, if none was given */
	if (!tok) {
		int err = nrf_cloud_jwt_generate(
			CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT_VALID_TIME_S,
			tok_ptr, tok_len + 1);

		if (err < 0) {
			LOG_ERR("Failed to auto-generate JWT, error: %d", err);
			nrf_cloud_free(*auth_hdr_out);
			*auth_hdr_out = NULL;
			return err;
		}
		tok_len = strlen(tok_ptr);
	}
#endif /* CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT */

	/* Write the postfix */
	postfix_ptr = tok_ptr + tok_len;
	memcpy(postfix_ptr, CRLF, postfix_len + 1);

	return 0;
}

static void close_connection(struct nrf_cloud_rest_context *const rest_ctx)
{
	if (!rest_ctx->keep_alive) {
		(void)nrf_cloud_rest_disconnect(rest_ctx);
	}
}

static void init_rest_client_request(struct nrf_cloud_rest_context const *const rest_ctx,
	struct rest_client_req_context *const req, const enum http_method meth)
{
	memset(req, 0, sizeof(*req));

	req->connect_socket	= rest_ctx->connect_socket;
	req->keep_alive		= rest_ctx->keep_alive;

	req->resp_buff		= rest_ctx->rx_buf;
	req->resp_buff_len	= rest_ctx->rx_buf_len;

	req->sec_tag		= CONFIG_NRF_CLOUD_SEC_TAG;
	req->port		= HTTPS_PORT;
	req->host		= CONFIG_NRF_CLOUD_REST_HOST_NAME;
	req->tls_peer_verify	= TLS_PEER_VERIFY_REQUIRED;

	if (rest_ctx->timeout_ms <= 0) {
		req->timeout_ms = SYS_FOREVER_MS;
	} else {
		req->timeout_ms = rest_ctx->timeout_ms;
	}

	req->http_method	= meth;

	(void)nrf_cloud_codec_init(NULL);
}

static void sync_rest_client_data(struct nrf_cloud_rest_context *const rest_ctx,
	struct rest_client_req_context const *const req,
	struct rest_client_resp_context const *const resp)
{
	rest_ctx->status		= resp->http_status_code;
	rest_ctx->response		= resp->response;
	rest_ctx->response_len		= resp->response_len;
	rest_ctx->total_response_len	= resp->total_response_len;

	rest_ctx->connect_socket	= req->connect_socket;
}

static int do_rest_client_request(struct nrf_cloud_rest_context *const rest_ctx,
	struct rest_client_req_context *const req,
	struct rest_client_resp_context *const resp,
	bool check_status_good, bool expect_body)
{
	int ret;

	ret = rest_client_request(req, resp);

	sync_rest_client_data(rest_ctx, req, resp);

	/* Check for an nRF Cloud specific error code */
	rest_ctx->nrf_err = NRF_CLOUD_ERROR_NONE;
	if ((ret == 0) && (rest_ctx->status >= NRF_CLOUD_HTTP_STATUS__ERROR_BEGIN) &&
	    rest_ctx->response && rest_ctx->response_len) {
		(void)nrf_cloud_rest_error_decode(rest_ctx->response, &rest_ctx->nrf_err);

		if ((rest_ctx->nrf_err != NRF_CLOUD_ERROR_NONE) &&
		    (rest_ctx->nrf_err != NRF_CLOUD_ERROR_NOT_FOUND_NO_ERROR)) {
			LOG_ERR("nRF Cloud REST error code: %d", rest_ctx->nrf_err);
		}
	}

	if (ret) {
		LOG_DBG("REST client request failed with error code %d", ret);
		return ret;
	} else if (rest_ctx->status == NRF_CLOUD_HTTP_STATUS_NONE) {
		LOG_DBG("REST request endpoint closed connection without reply.");
		return -ESHUTDOWN;
	} else if (check_status_good && (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK) &&
					(rest_ctx->status != NRF_CLOUD_HTTP_STATUS_ACCEPTED)) {
		LOG_DBG("REST request was rejected. Response status: %d", rest_ctx->status);
		return -EBADMSG;
	} else if (expect_body && (!rest_ctx->response || !rest_ctx->response_len)) {
		return -ENODATA;
	}

	return 0;
}
int nrf_cloud_rest_shadow_state_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char * const shadow_json)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	__ASSERT_NO_MSG(shadow_json != NULL);

	int ret;
	size_t buff_sz;
	char *auth_hdr = NULL;
	char *url = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_PATCH);

	/* Format API URL with device ID */
	buff_sz = sizeof(API_DEVICES_STATE_TEMPLATE) + strlen(device_id);
	url = nrf_cloud_malloc(buff_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}

	ret = snprintk(url, buff_sz, API_DEVICES_STATE_TEMPLATE, device_id);
	if ((ret < 0) || (ret >= buff_sz)) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	req.url = url;

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_APP_JSON,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Set payload */
	req.body = shadow_json;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, false);

clean_up:

	nrf_cloud_free(url);
	nrf_cloud_free(auth_hdr);

	close_connection(rest_ctx);

	return ret;
}

int nrf_cloud_rest_shadow_device_status_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_device_status *const dev_status)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	__ASSERT_NO_MSG(dev_status != NULL);

	int ret;
	struct nrf_cloud_data data_out;

	(void)nrf_cloud_codec_init(NULL);

	ret = nrf_cloud_shadow_dev_status_encode(dev_status, &data_out, false, true);
	if (ret) {
		LOG_ERR("Failed to encode device status, error: %d", ret);
		return ret;
	}

	ret = nrf_cloud_rest_shadow_state_update(rest_ctx, device_id, data_out.ptr);
	if (ret) {
		LOG_ERR("Failed to update device shadow, error: %d", ret);
	}

	nrf_cloud_device_status_free(&data_out);

	return ret;
}

int nrf_cloud_rest_shadow_service_info_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_svc_info * const svc_inf)
{
	if (svc_inf == NULL) {
		return -EINVAL;
	}

	const struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = (struct nrf_cloud_svc_info *)svc_inf
	};

	return nrf_cloud_rest_shadow_device_status_update(rest_ctx, device_id, &dev_status);
}

int nrf_cloud_rest_fota_job_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char *const job_id,
	const enum nrf_cloud_fota_status status, const char * const details)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	__ASSERT_NO_MSG(job_id != NULL);

	int ret;
	char *auth_hdr = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;
	struct nrf_cloud_fota_job_update update;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_PATCH);

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_APP_JSON,
		NULL
	};

	ret = nrf_cloud_fota_job_update_create(device_id, job_id, status, details, &update);
	if (ret) {
		LOG_ERR("Error creating FOTA job update structure: %d", ret);
		goto clean_up;
	}

	req.header_fields = (const char **)headers;
	req.url = update.url;
	req.body = update.payload;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, false);

	nrf_cloud_fota_job_update_free(&update);

clean_up:
	nrf_cloud_free(auth_hdr);

	close_connection(rest_ctx);

	return ret;
}

int nrf_cloud_rest_fota_job_get(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, struct nrf_cloud_fota_job_info *const job)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	/* job is not required, user can parse the response if they wish */

	int ret;
	size_t url_sz;
	char *auth_hdr = NULL;
	char *url = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_GET);

	/* Format API URL with device ID */
	url_sz = sizeof(API_GET_FOTA_URL_TEMPLATE) +
		    strlen(device_id);
	url = nrf_cloud_malloc(url_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	req.url = url;

	ret = snprintk(url, url_sz, API_GET_FOTA_URL_TEMPLATE, device_id);
	if ((ret < 0) || (ret >= url_sz)) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_ALL,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, false, false);
	if (ret) {
		goto clean_up;
	}

	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK &&
	    rest_ctx->status != NRF_CLOUD_HTTP_STATUS_NOT_FOUND) {
		ret = -EBADMSG;
		goto clean_up;
	}

	if (!job) {
		ret = 0;
		goto clean_up;
	}

	job->type = NRF_CLOUD_FOTA_TYPE__INVALID;

	if (rest_ctx->status == NRF_CLOUD_HTTP_STATUS_OK) {
		ret = nrf_cloud_rest_fota_execution_decode(rest_ctx->response, job);
		if (ret) {
			LOG_ERR("Failed to parse job execution response, error: %d", ret);
		}
	}

clean_up:
	nrf_cloud_free(url);
	nrf_cloud_free(auth_hdr);

	close_connection(rest_ctx);

	return ret;
}

void nrf_cloud_rest_fota_job_free(struct nrf_cloud_fota_job_info *const job)
{
	nrf_cloud_fota_job_free(job);
}

int nrf_cloud_rest_location_get(struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_location_request const *const request,
	struct nrf_cloud_location_result *const result)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG((request->cell_info != NULL) || (request->wifi_info != NULL));

	int ret;
	char *auth_hdr = NULL;
	char *payload = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_POST);

	req.url = request->disable_response ? API_GET_LOCATION_NO_REPLY : API_GET_LOCATION;

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);

	if (ret) {
		LOG_ERR("Could not format HTTP auth header, err: %d", ret);
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_APP_JSON,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Get payload */
	ret = nrf_cloud_location_req_json_encode(request->cell_info, request->wifi_info, &payload);
	if (ret) {
		LOG_ERR("Failed to generate location request, err: %d", ret);
		goto clean_up;
	}

	req.body = payload;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, !request->disable_response);

	if (ret) {
		goto clean_up;
	}

	if (result && request->disable_response) {
		LOG_WRN("A result struct is provided but location response is disabled");
		result->type = LOCATION_TYPE__INVALID;
	} else if (result && !request->disable_response) {
		ret = nrf_cloud_location_response_decode(rest_ctx->response, result);
		if (ret != 0) {
			if (ret > 0) {
				ret = -EBADMSG;
			}
			goto clean_up;
		}
	}

clean_up:
	nrf_cloud_free(auth_hdr);
	if (payload) {
		cJSON_free(payload);
	}

	if (result) {
		/* Add the nRF Cloud error to the response */
		result->err = rest_ctx->nrf_err;
	}

	close_connection(rest_ctx);

	return ret;
}

#if defined(CONFIG_NRF_CLOUD_AGPS)
static int get_content_range_total_bytes(char *const buf)
{
	char *end;
	char *start = strstr(buf, CONTENT_RANGE_RSP);

	/* nRF Cloud currently uses lower-case in content-range
	 * response, but check for mixed-case to be complete.
	 */
	if (!start) {
		start = strstr(buf, CONTENT_RANGE_RSP_MIXED_CASE);
	}

	if (!start) {
		return -EBADMSG;
	}

	end = strchr(start, (int)CR_TOK);
	if (!end) {
		end = strchr(start, (int)LF_TOK);
		if (!end) {
			return -EBADMSG;
		}
	}

	*end = 0;
	start = strrchr(start, (int)CONTENT_RANGE_TOTAL_TOK);

	if (!start) {
		return -EBADMSG;
	}

	return atoi(start + 1);
}

static int format_range_header(char *const buf, size_t buf_sz, size_t start_byte, size_t end_byte)
{
	int ret = snprintk(buf, buf_sz, HDR_RANGE_BYTES_TEMPLATE, start_byte, end_byte);

	if (ret < 0 || ret >= buf_sz) {
		return -EIO;
	}

	return 0;
}

int nrf_cloud_rest_agps_data_get(struct nrf_cloud_rest_context *const rest_ctx,
				 struct nrf_cloud_rest_agps_request const *const request,
				 struct nrf_cloud_rest_agps_result *const result)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(request != NULL);

	int ret;
	int type_count = 0;
	size_t url_sz;
	size_t total_bytes = 0;
	size_t rcvd_bytes = 0;
	size_t remain = 0;
	size_t pos = 0;
	size_t frag_size = (rest_ctx->fragment_size ? rest_ctx->fragment_size : RANGE_MAX_BYTES);
	char *auth_hdr = NULL;
	char *url = NULL;
	cJSON *agps_obj;
	enum nrf_cloud_agps_type types[NRF_CLOUD_AGPS__LAST];
	char range_hdr[HDR_RANGE_BYTES_SZ];
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;
	static int64_t last_request_timestamp;
	bool filtered = false;
	uint8_t mask_angle = NRF_CLOUD_AGPS_MASK_ANGLE_NONE;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_GET);

#if defined(CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME)
	filtered = request->filtered;
	mask_angle = request->mask_angle;
#elif defined(CONFIG_NRF_CLOUD_AGPS_FILTERED)
	filtered = CONFIG_NRF_CLOUD_AGPS_FILTERED;
	mask_angle = CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK;
#endif

	if (filtered && (mask_angle != NRF_CLOUD_AGPS_MASK_ANGLE_NONE) && (mask_angle > 90)) {
		LOG_ERR("Mask angle %u out of range (must be <= 90)", mask_angle);
		ret = -EINVAL;
		goto clean_up;
	}

	if ((request->type == NRF_CLOUD_REST_AGPS_REQ_CUSTOM) &&
	    (request->agps_req == NULL)) {
		LOG_ERR("Custom request type requires A-GPS request data");
		ret = -EINVAL;
		goto clean_up;
	} else if (result && !result->buf) {
		LOG_ERR("Invalid result buffer");
		ret = -EINVAL;
		goto clean_up;
	}

/** In filtered ephemeris mode, request A-GPS data no more often than
 *  every 2 hours (time in milliseconds). Without this, the GNSS unit will
 *  request for ephemeris every hour because a full set was not received.
 */
#define MARGIN_MINUTES 10
#define AGPS_UPDATE_PERIOD ((120 - MARGIN_MINUTES) * 60 * MSEC_PER_SEC)

	if (filtered && (last_request_timestamp != 0) &&
	    ((k_uptime_get() - last_request_timestamp) < AGPS_UPDATE_PERIOD)) {
		LOG_WRN("A-GPS request was sent less than 2 hours ago");
		ret = 0;
		result->agps_sz = 0;
		goto clean_up;
	}

	/* Get the A-GPS type array */
	switch (request->type) {
	case NRF_CLOUD_REST_AGPS_REQ_CUSTOM:
		type_count = nrf_cloud_agps_type_array_get(request->agps_req,
							   types, ARRAY_SIZE(types));
		break;
	case NRF_CLOUD_REST_AGPS_REQ_LOCATION:
		type_count = 1;
		types[0] = NRF_CLOUD_AGPS_LOCATION;
		break;
	case NRF_CLOUD_REST_AGPS_REQ_ASSISTANCE: {
		struct nrf_modem_gnss_agps_data_frame assist;
		/* Set all request flags */
		memset(&assist, 0xFF, sizeof(assist));
		type_count = nrf_cloud_agps_type_array_get(&assist, types, ARRAY_SIZE(types));
		break;
	}
	default:
		break;
	}

	if (type_count <= 0) {
		LOG_ERR("No A-GPS request data found for type: %u", request->type);
		ret = -ENOENT;
		goto clean_up;
	}

	agps_obj = cJSON_CreateObject();
	ret = nrf_cloud_agps_req_data_json_encode(types, type_count,
						  &request->net_info->current_cell, false,
						  filtered, mask_angle,
						  agps_obj);

	/* Create a parameterized URL from the JSON data to use for the GET request.
	 * The HTTP request body is not used in GET requests.
	 * Use the rx_buf temporarily.
	 */
	ret = nrf_cloud_json_to_url_params_convert(rest_ctx->rx_buf, rest_ctx->rx_buf_len,
						   agps_obj);

	/* Cleanup JSON obj */
	cJSON_Delete(agps_obj);
	agps_obj = NULL;

	if (ret) {
		LOG_ERR("Could not create A-GPS request URL");
		goto clean_up;
	}

	url_sz = sizeof(API_GET_AGPS_BASE) + strlen(rest_ctx->rx_buf);
	url = nrf_cloud_malloc(url_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}

	ret = snprintk(url, url_sz, "%s%s", API_GET_AGPS_BASE, rest_ctx->rx_buf);
	if (ret < 0 || ret >= url_sz) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	/* Set the URL */
	req.url = url;

	LOG_DBG("URL: %s", url);

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}

	char *const headers[] = {
		HDR_ACCEPT_ALL,
		(char *const)auth_hdr,
		(char *const)range_hdr,
		CONTENT_TYPE_APP_OCT_STR,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Do as many REST calls as needed to receive entire payload */
	do {
		/* Format range header */
		ret = format_range_header(range_hdr, sizeof(range_hdr),
					  rcvd_bytes,
					  (rcvd_bytes + frag_size - 1));
		if (ret) {
			LOG_ERR("Could not format range header");
			goto clean_up;
		}

		/* Send request, do not check for good response status  */
		ret = do_rest_client_request(rest_ctx, &req, &resp, false, false);
		if (ret) {
			goto clean_up;
		}

		if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_PARTIAL) {
			ret = -EBADMSG;
			goto clean_up;
		}

		if (total_bytes == 0) {
			total_bytes = get_content_range_total_bytes(rest_ctx->rx_buf);
			if (total_bytes <= 0) {
				ret = -EBADMSG;
				goto clean_up;
			}
			LOG_DBG("Total bytes in payload: %d", total_bytes);

			if (!result) {
				/* If all data was able to be downloaded without
				 * a result buffer, return 0.  Otherwise, return
				 * the total bytes needed for the result.
				 */
				if (total_bytes > frag_size) {
					ret = total_bytes;
				} else {
					ret = 0;
				}

				goto clean_up;

			} else if (result->buf_sz < total_bytes) {
				LOG_ERR("Result buffer too small for %d bytes of A-GPS data",
					total_bytes);
				ret = -ENOBUFS;
				goto clean_up;
			}
		}

		rcvd_bytes += rest_ctx->response_len;

		LOG_DBG("A-GPS data rx: %u/%u", rcvd_bytes, total_bytes);
		if (rcvd_bytes > total_bytes) {
			ret = -EFBIG;
			goto clean_up;
		}

		memcpy(&result->buf[pos],
		       rest_ctx->response,
		       rest_ctx->response_len);

		pos += rest_ctx->response_len;
		remain = total_bytes - rcvd_bytes;

	} while (remain);

	/* Set output size */
	result->agps_sz = total_bytes;
	last_request_timestamp = k_uptime_get();

clean_up:
	nrf_cloud_free(url);
	nrf_cloud_free(auth_hdr);

	close_connection(rest_ctx);

	return ret;
}
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
int nrf_cloud_rest_pgps_data_get(struct nrf_cloud_rest_context *const rest_ctx,
				 struct nrf_cloud_rest_pgps_request const *const request)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(request != NULL);

	int ret;

	size_t url_sz;
	char *auth_hdr = NULL;
	char *url = NULL;
	cJSON *data_obj;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_GET);

	/* Encode the request data as JSON */
	data_obj = cJSON_CreateObject();
	ret = nrf_cloud_pgps_req_data_json_encode(request->pgps_req, data_obj);
	if (ret) {
		goto clean_up;
	}

	/* Create a parameterized URL from the JSON data to use for the GET request.
	 * The HTTP request body is not used in GET requests.
	 * Use the rx_buf temporarily.
	 */
	ret = nrf_cloud_json_to_url_params_convert(rest_ctx->rx_buf, rest_ctx->rx_buf_len,
						   data_obj);

	/* Cleanup JSON obj */
	cJSON_Delete(data_obj);
	data_obj = NULL;

	if (ret) {
		LOG_ERR("Could not create P-GPS request URL");
		goto clean_up;
	}

	url_sz = sizeof(API_GET_PGPS_BASE) + strlen(rest_ctx->rx_buf);
	url = nrf_cloud_malloc(url_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}

	ret = snprintk(url, url_sz, "%s%s", API_GET_PGPS_BASE, rest_ctx->rx_buf);
	if (ret < 0 || ret >= url_sz) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	/* Set the URL */
	req.url = url;

	LOG_DBG("URL: %s", url);

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_ALL,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, true);

clean_up:
	nrf_cloud_free(url);
	nrf_cloud_free(auth_hdr);
	if (req.body) {
		cJSON_free((void *)req.body);
	}
	cJSON_Delete(data_obj);

	close_connection(rest_ctx);

	return ret;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

int nrf_cloud_rest_disconnect(struct nrf_cloud_rest_context *const rest_ctx)
{
	if (!rest_ctx) {
		return -EINVAL;
	} else if (rest_ctx->connect_socket < 0) {
		return -ENOTCONN;
	}

	int err = close(rest_ctx->connect_socket);

	if (err) {
		LOG_ERR("Failed to close socket, error: %d", errno);
		err = -EIO;
	} else {
		rest_ctx->connect_socket = -1;
	}

	return err;
}

int nrf_cloud_rest_jitp(const sec_tag_t nrf_cloud_sec_tag)
{
	__ASSERT_NO_MSG(nrf_cloud_sec_tag >= 0);

	int ret;
	char rx_buf[JITP_RX_BUF_SZ];
	char *const headers[] = {
		HDR_ACCEPT_ALL,
		JITP_CONTENT_TYPE_HDR,
		JITP_HOST_HDR,
		JITP_CONNECTION_HDR,
		NULL
	};
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	rest_client_request_defaults_set(&req);

	req.body		= NULL;
	req.sec_tag		= nrf_cloud_sec_tag;
	req.port		= JITP_PORT;
	req.header_fields	= (const char **)headers;
	req.url			= JITP_URL;
	req.host		= JITP_HOSTNAME_TLS;
	req.timeout_ms		= JITP_HTTP_TIMEOUT_MS;
	req.http_method		= HTTP_POST;
	req.resp_buff		= rx_buf;
	req.resp_buff_len	= sizeof(rx_buf);
	req.tls_peer_verify	= TLS_PEER_VERIFY_REQUIRED;
	req.keep_alive		= false;

	ret = rest_client_request(&req, &resp);
	if (ret == 0) {
		if ((resp.http_status_code == NRF_CLOUD_HTTP_STATUS_NONE) &&
		    (resp.response_len == 0)) {
			/* Expected response for an unprovisioned device.
			 * Wait 30s before associating device with account
			 */
		} else if ((resp.http_status_code == NRF_CLOUD_HTTP_STATUS_FORBIDDEN) &&
			   (resp.response_len > 0) &&
			   (strstr(resp.response, "\"message\":null"))) {
			/* Expected response for an already provisioned device.
			 * User can proceed to use the API.
			 */
			ret = 1;
		} else {
			ret = -ENODEV;
		}
	}

	return ret;
}

int nrf_cloud_rest_send_location(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_gnss_data * const gnss)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	__ASSERT_NO_MSG(gnss != NULL);

	int err = -ENOMEM;
	char *json_msg = NULL;
	cJSON *msg_obj = NULL;

	(void)nrf_cloud_codec_init(NULL);

	msg_obj = cJSON_CreateObject();
	err = nrf_cloud_gnss_msg_json_encode(gnss, msg_obj);
	if (err) {
		goto clean_up;
	}

	json_msg = cJSON_PrintUnformatted(msg_obj);
	if (!json_msg) {
		LOG_ERR("Failed to print JSON");
		goto clean_up;
	}
	cJSON_Delete(msg_obj);
	msg_obj = NULL;

	err = nrf_cloud_rest_send_device_message(rest_ctx, device_id, json_msg, false, NULL);

clean_up:
	cJSON_Delete(msg_obj);
	if (json_msg) {
		cJSON_free((void *)json_msg);
	}

	return err;
}

int nrf_cloud_rest_send_device_message(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const char *const json_msg, const bool bulk,
	const char *const topic)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);
	__ASSERT_NO_MSG(json_msg != NULL);

	int ret;
	size_t buff_sz;
	char *auth_hdr = NULL;
	char *url = NULL;
	char *d2c = NULL;
	cJSON *root_obj;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_POST);

	root_obj = cJSON_CreateObject();

	if (cJSON_AddRawToObjectCS(root_obj, NRF_CLOUD_REST_MSG_KEY, json_msg) == NULL) {
		ret = -ENOMEM;
		goto clean_up;
	}

	/* Format the d2c topic if topic was not provided or bulk topic was specified */
	if (!topic || bulk) {
		buff_sz = strlen(device_id) + sizeof(API_DEVICES_MSGS_D2C_TPC_TMPLT) +
			  (bulk ? sizeof(API_DEVICES_MSGS_BULK) : 0);

		d2c = nrf_cloud_malloc(buff_sz);
		if (!d2c) {
			ret = -ENOMEM;
			goto clean_up;
		}

		ret = snprintk(d2c, buff_sz, API_DEVICES_MSGS_D2C_TPC_TMPLT, device_id,
			       (bulk ? API_DEVICES_MSGS_BULK : ""));
		if (ret < 0 || ret >= buff_sz) {
			LOG_ERR("Could not format topic");
			ret = -ETXTBSY;
			goto clean_up;
		}
	}

	if (cJSON_AddStringToObjectCS(root_obj, NRF_CLOUD_REST_TOPIC_KEY,
				      (d2c ? d2c : topic)) == NULL) {
		ret = -ENOMEM;
		goto clean_up;
	}

	/* Set payload */
	req.body = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	root_obj = NULL;

	if (!req.body) {
		ret = -ENOMEM;
		goto clean_up;
	}

	/* Format API URL with device ID */
	buff_sz = sizeof(API_DEVICES_MSGS_TEMPLATE) + strlen(device_id);
	url = nrf_cloud_malloc(buff_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	req.url = url;

	ret = snprintk(url, buff_sz, API_DEVICES_MSGS_TEMPLATE, device_id);
	if ((ret < 0) || (ret >= buff_sz)) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		CONTENT_TYPE_APP_JSON,
		NULL
	};

	req.header_fields = (const char **)headers;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, false);

clean_up:
	nrf_cloud_free(url);
	nrf_cloud_free(auth_hdr);
	nrf_cloud_free(d2c);

	if (req.body) {
		cJSON_free((void *)req.body);
	}

	cJSON_Delete(root_obj);

	close_connection(rest_ctx);

	return ret;
}

int nrf_cloud_rest_device_status_message_send(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_device_status *const dev_status,
	const int64_t timestamp_ms)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(device_id != NULL);

	int err = -ENOMEM;
	cJSON *msg_obj;
	char *json_msg = NULL;

	(void)nrf_cloud_codec_init(NULL);

	msg_obj = cJSON_CreateObject();
	if (!msg_obj) {
		goto clean_up;
	}

	err = nrf_cloud_dev_status_json_encode(dev_status, timestamp_ms, msg_obj);
	if (err) {
		goto clean_up;
	}

	json_msg = cJSON_PrintUnformatted(msg_obj);

	cJSON_Delete(msg_obj);
	msg_obj = NULL;

	if (!json_msg) {
		err = -ENOMEM;
		goto clean_up;
	}

	err = nrf_cloud_rest_send_device_message(rest_ctx, device_id, json_msg, false, NULL);

clean_up:
	cJSON_Delete(msg_obj);
	if (json_msg) {
		cJSON_free((void *)json_msg);
	}
	return err;
}
