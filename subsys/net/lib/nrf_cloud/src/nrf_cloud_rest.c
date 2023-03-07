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
#include <net/nrf_cloud_rest.h>
#include <net/rest_client.h>
#include <zephyr/logging/log.h>
#include <cJSON.h>

#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec.h"

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
#define API_UPDATE_FOTA_URL_TEMPLATE	(API_VER API_FOTA_JOB_EXEC "/%s/%s")
#define API_UPDATE_FOTA_BODY_TEMPLATE	"{\"status\":\"%s\"}"
#define API_UPDATE_FOTA_DETAILS_TMPLT	"{\"status\":\"%s\", \"details\":\"%s\"}"

#define API_LOCATION			"/location"
#define API_GET_LOCATION_TEMPLATE	API_VER API_LOCATION "/ground-fix"
#define API_GET_AGPS_BASE		API_VER API_LOCATION "/agps?"
#define AGPS_FILTERED			"filtered=true"
#define AGPS_ELEVATION_MASK		"&mask=%u"
#define AGPS_REQ_TYPE			"&requestType=%s"
#define NET_INFO_PRINT_SZ		(3 + 3 + 5 + UINT32_MAX_STR_SZ)
#define AGPS_NET_INFO			"&mcc=%u&mnc=%u&tac=%u&eci=%u"
#define AGPS_CUSTOM_TYPE		"&customTypes=%s"
#define AGPS_REQ_TYPE_STR_CUSTOM	"custom"
#define AGPS_REQ_TYPE_STR_LOC		"rtLocation"
#define AGPS_REQ_TYPE_STR_ASSIST	"rtAssistance"

#define AGPS_CUSTOM_TYPE_CNT		9
/* Custom type format is a comma separated list of
 * @ref enum nrf_cloud_agps_type digits
 * digits.
 */
#define AGPS_CUSTOM_TYPE_STR_SZ		(AGPS_CUSTOM_TYPE_CNT * 2)

#define API_GET_PGPS_BASE		API_VER "/location/pgps?"
#define PGPS_REQ_PREDICT_CNT		"&" NRF_CLOUD_JSON_PGPS_PRED_COUNT "=%u"
#define PGPS_REQ_PREDICT_INT_MIN	"&" NRF_CLOUD_JSON_PGPS_INT_MIN "=%u"
#define PGPS_REQ_START_GPS_DAY		"&" NRF_CLOUD_JSON_PGPS_GPS_DAY "=%u"
#define PGPS_REQ_START_GPS_TOD_S	"&" NRF_CLOUD_JSON_PGPS_GPS_TIME "=%u"

#define API_DEVICES_BASE		"/devices"
#define API_DEVICES_STATE_TEMPLATE	API_VER API_DEVICES_BASE "/%s/state"
#define API_DEVICES_MSGS_TEMPLATE	API_VER API_DEVICES_BASE "/%s/messages"
#define API_DEVICES_MSGS_MSG_KEY	"message"
#define API_DEVICES_MSGS_TOPIC_KEY	"topic"
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

/* Mapping of enum to strings for Job Execution Status. */
static const char *const job_status_strings[] = {
	[NRF_CLOUD_FOTA_QUEUED]      = "QUEUED",
	[NRF_CLOUD_FOTA_IN_PROGRESS] = "IN_PROGRESS",
	[NRF_CLOUD_FOTA_FAILED]      = "FAILED",
	[NRF_CLOUD_FOTA_SUCCEEDED]   = "SUCCEEDED",
	[NRF_CLOUD_FOTA_TIMED_OUT]   = "TIMED_OUT",
	[NRF_CLOUD_FOTA_REJECTED]    = "REJECTED",
	[NRF_CLOUD_FOTA_CANCELED]    = "CANCELLED",
	[NRF_CLOUD_FOTA_DOWNLOADING] = "DOWNLOADING",
};
#define JOB_STATUS_STRING_COUNT (sizeof(job_status_strings) / \
				 sizeof(*job_status_strings))

/* Mapping of enum to strings for AGPS request type. */
static const char *const agps_req_type_strings[] = {
	[NRF_CLOUD_REST_AGPS_REQ_ASSISTANCE]	= AGPS_REQ_TYPE_STR_ASSIST,
	[NRF_CLOUD_REST_AGPS_REQ_LOCATION]	= AGPS_REQ_TYPE_STR_LOC,
	[NRF_CLOUD_REST_AGPS_REQ_CUSTOM]	= AGPS_REQ_TYPE_STR_CUSTOM,
};

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
	int ret = rest_client_request(req, resp);

	sync_rest_client_data(rest_ctx, req, resp);

	/* Check for an nRF Cloud specific error code */
	rest_ctx->nrf_err = NRF_CLOUD_ERROR_NONE;
	if ((ret == 0) && (rest_ctx->status >= NRF_CLOUD_HTTP_STATUS__ERROR_BEGIN) &&
	    rest_ctx->response && rest_ctx->response_len) {
		(void)nrf_cloud_parse_rest_error(rest_ctx->response, &rest_ctx->nrf_err);

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
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}

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

	ret = nrf_cloud_device_status_shadow_encode(dev_status, &data_out, false);
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
	__ASSERT_NO_MSG(status < JOB_STATUS_STRING_COUNT);

	int ret;
	size_t buff_sz;
	char *auth_hdr = NULL;
	char *url = NULL;
	char *payload = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_PATCH);

	/* Format API URL with device and job ID */
	buff_sz = sizeof(API_UPDATE_FOTA_URL_TEMPLATE) +
		  strlen(device_id) +
		  strlen(job_id);
	url = nrf_cloud_malloc(buff_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	req.url = url;

	ret = snprintk(url, buff_sz, API_UPDATE_FOTA_URL_TEMPLATE,
		       device_id, job_id);
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

	/* Format payload */
	if (details) {
		buff_sz = sizeof(API_UPDATE_FOTA_DETAILS_TMPLT) +
			  strlen(job_status_strings[status]) +
			  strlen(details);
	} else {
		buff_sz = sizeof(API_UPDATE_FOTA_BODY_TEMPLATE) +
			  strlen(job_status_strings[status]);
	}

	payload = nrf_cloud_malloc(buff_sz);
	if (!payload) {
		ret = -ENOMEM;
		goto clean_up;
	}

	if (details) {
		ret = snprintk(payload, buff_sz, API_UPDATE_FOTA_DETAILS_TMPLT,
			       job_status_strings[status], details);
	} else {
		ret = snprintk(payload, buff_sz, API_UPDATE_FOTA_BODY_TEMPLATE,
			       job_status_strings[status]);
	}
	if ((ret < 0) || (ret >= buff_sz)) {
		LOG_ERR("Could not format payload");
		ret = -ETXTBSY;
		goto clean_up;
	}
	req.body = payload;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, false);

clean_up:
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}
	if (payload) {
		nrf_cloud_free(payload);
	}

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
		ret = nrf_cloud_rest_fota_execution_parse(rest_ctx->response, job);
		if (ret) {
			LOG_ERR("Failed to parse job execution response, error: %d", ret);
		}
	}

clean_up:
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}

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

	req.url = API_GET_LOCATION_TEMPLATE;

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
	ret = nrf_cloud_format_location_req(request->cell_info, request->wifi_info, &payload);
	if (ret) {
		LOG_ERR("Failed to generate location request, err: %d", ret);
		goto clean_up;
	}

	req.body = payload;

	/* Make REST call */
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, true);

	if (ret) {
		goto clean_up;
	}

	if (result) {
		ret = nrf_cloud_parse_location_response(rest_ctx->response, result);
		if (ret != 0) {
			if (ret > 0) {
				ret = -EBADMSG;
			}
			goto clean_up;
		}
	}

clean_up:
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}
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

/* AGPS_TYPE_PRINT macro assumes single digit values, check for the rare case that the
 * enum is modified.
 */
BUILD_ASSERT((NRF_CLOUD_AGPS_UTC_PARAMETERS <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_EPHEMERIDES <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_ALMANAC <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_NEQUICK_CORRECTION <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_INTEGRITY <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_LOCATION <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK <= AGPS_CUSTOM_TYPE_CNT) &&
	     (NRF_CLOUD_AGPS_GPS_TOWS <= AGPS_CUSTOM_TYPE_CNT),
	     "A-GPS enumeration values have changed, update format_agps_custom_types_str()");

/* Macro to print the comma separated list of custom types */
#define AGPS_TYPE_PRINT(buf, type)		\
	if (pos != 0) {				\
		buf[pos++] = ',';		\
	}					\
	buf[pos++] = (char)('0' + type)

static int format_agps_custom_types_str(struct nrf_modem_gnss_agps_data_frame const *const req,
	char *const types_buf)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(types_buf != NULL);

	int pos = 0;

	if (req->data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_UTC_PARAMETERS);
	}
	if (req->sv_mask_ephe) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_EPHEMERIDES);
	}
	if (req->sv_mask_alm) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_ALMANAC);
	}
	if (req->data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION);
	}
	if (req->data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_NEQUICK_CORRECTION);
	}
	if (req->data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_GPS_TOWS);
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK);
	}
	if (req->data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_LOCATION);
	}
	if (req->data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		AGPS_TYPE_PRINT(types_buf, NRF_CLOUD_AGPS_INTEGRITY);
	}

	types_buf[pos] = '\0';

	return pos ? 0 : -EBADF;
}

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
	size_t total_bytes;
	size_t rcvd_bytes;
	size_t url_sz;
	size_t remain;
	size_t pos;
	size_t frag_size;
	char *auth_hdr = NULL;
	char *url = NULL;
	char const *req_type = NULL;
	char custom_types[AGPS_CUSTOM_TYPE_STR_SZ];
	char range_hdr[HDR_RANGE_BYTES_SZ];
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;
	static int64_t last_request_timestamp;
	bool filtered;
	uint8_t mask_angle;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_GET);

#if defined(CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME)
	filtered = request->filtered;
	mask_angle = request->mask_angle;
#elif defined(CONFIG_NRF_CLOUD_AGPS_FILTERED)
	filtered = CONFIG_NRF_CLOUD_AGPS_FILTERED;
	mask_angle = CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK;
#else
	filtered = false;
	mask_angle = 0;
#endif

	if (filtered && (mask_angle > 90)) {
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

	/* Determine size of URL buffer and allocate */
	url_sz = sizeof(API_GET_AGPS_BASE);
	if (request->net_info) {
		url_sz += strlen(AGPS_NET_INFO) + NET_INFO_PRINT_SZ;
	}
	if (filtered) {
		url_sz += strlen(AGPS_FILTERED) + strlen(AGPS_ELEVATION_MASK);
	}
	switch (request->type) {
	case NRF_CLOUD_REST_AGPS_REQ_CUSTOM:
		ret = format_agps_custom_types_str(request->agps_req, custom_types);
		if (ret) {
			LOG_ERR("No A-GPS types requested");
			goto clean_up;
		}
		url_sz += strlen(AGPS_CUSTOM_TYPE) + strlen(custom_types);
		/* Fall-through */
	case NRF_CLOUD_REST_AGPS_REQ_LOCATION:
	case NRF_CLOUD_REST_AGPS_REQ_ASSISTANCE:
		req_type = agps_req_type_strings[request->type];
		url_sz += strlen(AGPS_REQ_TYPE) + strlen(req_type);
		break;
	case NRF_CLOUD_REST_AGPS_REQ_UNSPECIFIED:
		break;

	default:
		ret = -EINVAL;
		goto clean_up;
	}

	url = nrf_cloud_malloc(url_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	req.url = url;

	/* Format API URL */
	ret = snprintk(url, url_sz, API_GET_AGPS_BASE);
	if ((ret < 0) || (ret >= url_sz)) {
		LOG_ERR("Could not format URL: device id");
		ret = -ETXTBSY;
		goto clean_up;
	}
	pos = ret;
	remain = url_sz - ret;

	if (filtered) {
		ret = snprintk(&url[pos], remain, AGPS_FILTERED);
		if ((ret < 0) || (ret >= remain)) {
			LOG_ERR("Could not format URL: filtered");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
		ret = snprintk(&url[pos], remain, AGPS_ELEVATION_MASK, mask_angle);
		if ((ret < 0) || (ret >= remain)) {
			LOG_ERR("Could not format URL: mask angle");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	if (req_type) {
		ret = snprintk(&url[pos], remain, AGPS_REQ_TYPE, req_type);
		if ((ret < 0) || (ret >= remain)) {
			LOG_ERR("Could not format URL: request type");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	if (request->type == NRF_CLOUD_REST_AGPS_REQ_CUSTOM) {
		ret = snprintk(&url[pos], remain, AGPS_CUSTOM_TYPE, custom_types);
		if ((ret < 0) || (ret >= remain)) {
			LOG_ERR("Could not format URL: custom types");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	if (request->net_info) {
		ret = snprintk(&url[pos], remain, AGPS_NET_INFO,
			       request->net_info->current_cell.mcc,
			       request->net_info->current_cell.mnc,
			       request->net_info->current_cell.tac,
			       request->net_info->current_cell.id);
		if ((ret < 0) || (ret >= remain)) {
			LOG_ERR("Could not format URL: network info");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	LOG_DBG("req url:%s", url);

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

	pos = 0;
	remain = 0;
	rcvd_bytes = 0;
	total_bytes = 0;
	frag_size = rest_ctx->fragment_size ?
		    rest_ctx->fragment_size : RANGE_MAX_BYTES;

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
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}

	close_connection(rest_ctx);

	return ret;
}

int nrf_cloud_rest_pgps_data_get(struct nrf_cloud_rest_context *const rest_ctx,
				 struct nrf_cloud_rest_pgps_request const *const request)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(request != NULL);

	int ret;
	size_t url_sz;
	size_t remain;
	size_t pos;
	char *auth_hdr = NULL;
	char *url = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;

	memset(&resp, 0, sizeof(resp));
	init_rest_client_request(rest_ctx, &req, HTTP_GET);

	/* Determine size of URL buffer and allocate */
	url_sz = sizeof(API_GET_PGPS_BASE);

	if (request->pgps_req) {
		if (request->pgps_req->prediction_count !=
		NRF_CLOUD_REST_PGPS_REQ_NO_COUNT) {
			url_sz += sizeof(PGPS_REQ_PREDICT_CNT) +
				  UINT32_MAX_STR_SZ;
		}
		if (request->pgps_req->prediction_period_min !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL) {
			url_sz += sizeof(PGPS_REQ_PREDICT_INT_MIN) +
				  UINT32_MAX_STR_SZ;
		}
		if (request->pgps_req->gps_day !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY) {
			url_sz += sizeof(PGPS_REQ_START_GPS_DAY) +
				  UINT32_MAX_STR_SZ;
		}
		if (request->pgps_req->gps_time_of_day !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD) {
			url_sz += sizeof(PGPS_REQ_START_GPS_TOD_S) +
				  UINT32_MAX_STR_SZ;
		}
	}

	url = nrf_cloud_malloc(url_sz);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	req.url = url;

	/* Format API URL */
	ret = snprintk(url, url_sz, API_GET_PGPS_BASE);
	if ((ret < 0) || (ret >= url_sz)) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}
	pos = ret;
	remain = url_sz - ret;

	if (request->pgps_req) {
		if (request->pgps_req->prediction_count !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_COUNT) {
			ret = snprintk(&url[pos], remain, PGPS_REQ_PREDICT_CNT,
				request->pgps_req->prediction_count);
			if ((ret < 0) || (ret >= remain)) {
				LOG_ERR("Could not format URL: prediction count");
				ret = -ETXTBSY;
				goto clean_up;
			}
			pos += ret;
			remain -= ret;
		}

		if (request->pgps_req->prediction_period_min !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL) {
			ret = snprintk(&url[pos], remain, PGPS_REQ_PREDICT_INT_MIN,
				       request->pgps_req->prediction_period_min);
			if ((ret < 0) || (ret >= remain)) {
				LOG_ERR("Could not format URL: prediction interval");
				ret = -ETXTBSY;
				goto clean_up;
			}
			pos += ret;
			remain -= ret;
		}

		if (request->pgps_req->gps_day !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY) {
			ret = snprintk(&url[pos], remain, PGPS_REQ_START_GPS_DAY,
				       request->pgps_req->gps_day);
			if ((ret < 0) || (ret >= remain)) {
				LOG_ERR("Could not format URL: GPS day");
				ret = -ETXTBSY;
				goto clean_up;
			}
			pos += ret;
			remain -= ret;
		}

		if (request->pgps_req->gps_time_of_day !=
		    NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD) {
			ret = snprintk(&url[pos], remain, PGPS_REQ_START_GPS_TOD_S,
				       request->pgps_req->gps_time_of_day);
			if ((ret < 0) || (ret >= remain)) {
				LOG_ERR("Could not format URL: GPS time");
				ret = -ETXTBSY;
				goto clean_up;
			}
			pos += ret;
			remain -= ret;
		}
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
	ret = do_rest_client_request(rest_ctx, &req, &resp, true, true);

clean_up:
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}

	close_connection(rest_ctx);

	return ret;
}

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

	if (cJSON_AddRawToObjectCS(root_obj, API_DEVICES_MSGS_MSG_KEY, json_msg) == NULL) {
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

	if (cJSON_AddStringToObjectCS(root_obj, API_DEVICES_MSGS_TOPIC_KEY,
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
	if (url) {
		nrf_cloud_free(url);
	}
	if (auth_hdr) {
		nrf_cloud_free(auth_hdr);
	}
	if (req.body) {
		cJSON_free((void *)req.body);
	}
	if (d2c) {
		nrf_cloud_free(d2c);
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

	err = nrf_cloud_device_status_msg_encode(dev_status, timestamp_ms, msg_obj);
	if (err) {
		goto clean_up;
	}

	json_msg = cJSON_PrintUnformatted(msg_obj);
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
