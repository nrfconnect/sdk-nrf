/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <posix/arpa/inet.h>
#include <posix/unistd.h>
#include <posix/netdb.h>
#include <posix/sys/socket.h>
#else
#include <net/socket.h>
#endif
#include <net/net_ip.h>
#include <modem/nrf_modem_lib.h>
#include <net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include <net/http_client.h>
#include <net/http_parser.h>
#include <net/nrf_cloud_rest.h>
#include <sys/base64.h>
#include <logging/log.h>

#include "nrf_cloud_codec.h"

LOG_MODULE_REGISTER(nrf_cloud_rest, CONFIG_NRF_CLOUD_REST_LOG_LEVEL);

#define HTTPS_PORT 443
#define BASE64_PAD_CHAR '='

#define UINT32_MAX_STR_SZ		10
#define RANGE_MAX_BYTES			CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE

#define API_VER				"/v1"

#define CONTENT_TYPE_TXT_PLAIN		"text/plain"
#define CONTENT_TYPE_ALL		"*/*"
#define CONTENT_TYPE_APP_JSON		"application/json"
#define CONTENT_TYPE_APP_OCT_STR	"application/octet-stream"
#define CONTENT_RANGE_RSP		"content-range: bytes"
#define CONTENT_RANGE_TOTAL_TOK		'/'
#define LF_TOK				'\n'
#define CR_TOK				'\r'

#define AUTH_HDR_BEARER_TEMPLATE	"Authorization: Bearer %s\r\n"
#define HOST_HDR_TEMPLATE		"Host: %s\r\n"
#define HDR_ACCEPT_APP_JSON		"accept: application/json\r\n"
#define HDR_ACCEPT_ALL			"accept: " CONTENT_TYPE_ALL "\r\n"
#define HDR_RANGE_BYTES_TEMPLATE	"Range: bytes=%u-%u\r\n"
#define HDR_RANGE_BYTES_SZ		(sizeof(HDR_RANGE_BYTES_TEMPLATE) + \
					UINT32_MAX_STR_SZ + UINT32_MAX_STR_SZ)

#define API_FOTA_JOB_EXEC		"/fota-job-executions"
#define API_GET_FOTA_URL_TEMPLATE	API_VER API_FOTA_JOB_EXEC "/%s/current"
#define API_UPDATE_FOTA_URL_TEMPLATE	API_VER API_FOTA_JOB_EXEC "/%s/%s"
#define API_UPDATE_FOTA_BODY_TEMPLATE	"{\"status\":\"%s\"}"
#define API_UPDATE_FOTA_DETAILS_TMPLT	"{\"status\":\"%s\", \"details\":\"%s\"}"

#define API_LOCATION			"/location"
#define API_GET_CELL_POS_TEMPLATE	API_VER API_LOCATION "/cell"
#define API_GET_AGPS_BASE		API_VER API_LOCATION "/agps?"
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

#define HTTP_PROTOCOL		"HTTP/1.1"
#define SOCKET_PROTOCOL		IPPROTO_TLS_1_2

#define JITP_HOSTNAME_TLS	CONFIG_NRF_CLOUD_HOST_NAME
#define JITP_PORT		8443
#define JITP_URL		"/topics/jitp?qos=1"
#define JITP_CONTENT		"*/*\r\n" \
				"Connection: close\r\n" \
				"Host: " JITP_HOSTNAME_TLS ":" STRINGIFY(JITP_PORT) "\r\n"
#define JITP_HTTP_TIMEOUT_MS	(15000)
#define JITP_RX_BUF_SZ		(255)

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

static void http_response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	struct nrf_cloud_rest_context *rest_ctx = NULL;

	if (user_data) {
		rest_ctx = (struct nrf_cloud_rest_context *)user_data;
	}

	/* If the entire HTTP response is not received in a single "recv" call
	 * then this could be called multiple times, with a different value in
	 * rsp->body_start. Only set rest_ctx->response once, the first time,
	 * which will be the start of the body.
	 */
	if (rest_ctx && !rest_ctx->response && rsp->body_found && rsp->body_start) {
		rest_ctx->response = rsp->body_start;
	}

	rest_ctx->total_response_len += rsp->data_len;

	if (final_data == HTTP_DATA_FINAL) {
		LOG_DBG("HTTP: All data received, status: %u %s",
			rsp->http_status_code,
			log_strdup(rsp->http_status));

		if (!rest_ctx) {
			LOG_WRN("User data not provided");
			return;
		}

		rest_ctx->status = rsp->http_status_code;
		rest_ctx->response_len = rsp->content_length;

		LOG_DBG("Content/Total: %d/%d",
			rest_ctx->response_len,
			rest_ctx->total_response_len);
	}
}

static int generate_auth_header(const char *const tok, char **auth_hdr_out)
{
	if (!tok || !auth_hdr_out) {
		return -EINVAL;
	}

	int ret;
	size_t buff_size = sizeof(AUTH_HDR_BEARER_TEMPLATE) + strlen(tok);

	*auth_hdr_out = k_calloc(buff_size, 1);
	if (!*auth_hdr_out) {
		return -ENOMEM;
	}
	ret = snprintk(*auth_hdr_out, buff_size, AUTH_HDR_BEARER_TEMPLATE, tok);
	if (ret < 0 || ret >= buff_size) {
		k_free(*auth_hdr_out);
		*auth_hdr_out = NULL;
		return -ETXTBSY;
	}

	return 0;
}

static int tls_setup(int fd, const char *const tls_hostname, const sec_tag_t sec_tag)
{
	int err;
	int verify = TLS_PEER_VERIFY_REQUIRED;
	const sec_tag_t tls_sec_tag[] = {
		sec_tag,
	};

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, error: %d", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, error: %d", errno);
		return err;
	}

	if (tls_hostname) {
		err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, tls_hostname,
				 strlen(tls_hostname));
		if (err) {
			LOG_ERR("Failed to setup TLS hostname, error: %d", errno);
			return err;
		}
	}
	return 0;
}

static int socket_timeouts_set(int fd)
{
	int err;
	struct timeval timeout = {0};

	if (CONFIG_NRF_CLOUD_REST_SEND_TIMEOUT > -1) {
		/* Send TO also affects TCP connect */
		timeout.tv_sec = CONFIG_NRF_CLOUD_REST_SEND_TIMEOUT;
		err = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket send timeout, error: %d", errno);
			return err;
		}
	}

	if (CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT > -1) {
		timeout.tv_sec = CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT;
		err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket recv timeout, error: %d", errno);
			return err;
		}
	}

	return 0;
}

static int do_connect(int *const fd, const char *const hostname,
		      const uint16_t port_num, const char *const ip_address,
		      const sec_tag_t sec_tag)
{
	int ret;
	struct addrinfo *addr_info;
	char peer_addr[INET_ADDRSTRLEN];
	/* Use IP to connect if provided, always use hostname for TLS (SNI) */
	const char *const connect_addr = ip_address ? ip_address : hostname;

	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_next =  NULL,
	};

	/* Make sure fd is always initialized when this function is called */
	*fd = -1;

	ret = getaddrinfo(connect_addr, NULL, &hints, &addr_info);
	if (ret) {
		LOG_ERR("getaddrinfo() failed, error: %d", ret);
		return -EFAULT;
	}

	inet_ntop(AF_INET, &net_sin(addr_info->ai_addr)->sin_addr,
			peer_addr, INET_ADDRSTRLEN);
	LOG_DBG("getaddrinfo() %s", log_strdup(peer_addr));

	((struct sockaddr_in *)addr_info->ai_addr)->sin_port = htons(port_num);

	*fd = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL);
	if (*fd == -1) {
		LOG_ERR("Failed to open socket, error: %d", errno);
		ret = -ENOTCONN;
		goto clean_up;
	}

	if (sec_tag >= 0) {
		ret = tls_setup(*fd, hostname, sec_tag);
		if (ret) {
			ret = -EACCES;
			goto clean_up;
		}
	}

	ret = socket_timeouts_set(*fd);
	if (ret) {
		LOG_ERR("Failed to set socket timeouts, error: %d", errno);
		ret = -EINVAL;
		goto clean_up;
	}

	LOG_DBG("Connecting to %s", log_strdup(connect_addr));

	ret = connect(*fd, addr_info->ai_addr, sizeof(struct sockaddr_in));
	if (ret) {
		LOG_ERR("Failed to connect socket, error: %d", errno);
		ret = -ECONNREFUSED;
	}

clean_up:

	freeaddrinfo(addr_info);

	if (ret) {
		if (*fd > -1) {
			(void)close(*fd);
			*fd = -1;
		}
	}

	return ret;
}

static void close_connection(struct nrf_cloud_rest_context *const rest_ctx)
{
	if (!rest_ctx->keep_alive) {
		(void)nrf_cloud_rest_disconnect(rest_ctx);
	}
}

static int do_api_call(struct http_request *http_req, struct nrf_cloud_rest_context *const rest_ctx,
	const uint16_t port_num, const sec_tag_t sec_tag)
{
	int err = 0;

	if (rest_ctx->connect_socket < 0) {
		err = do_connect(&rest_ctx->connect_socket,
				 http_req->host,
				 port_num,
				 NULL,
				 sec_tag);
		if (err) {
			return err;
		}
	}

	/* Assign the user provided receive buffer into the http request */
	http_req->recv_buf	= rest_ctx->rx_buf;
	http_req->recv_buf_len	= rest_ctx->rx_buf_len;

	memset(http_req->recv_buf, 0, http_req->recv_buf_len);
	/* Ensure receive buffer stays NULL terminated */
	--http_req->recv_buf_len;

	rest_ctx->response		= NULL;
	rest_ctx->response_len		= 0;
	rest_ctx->total_response_len	= 0;

	/* The http_client timeout does not seem to work correctly, so
	 * for now do not use a timeout.
	 */
	err = http_client_req(rest_ctx->connect_socket,
			      http_req,
			      NRF_CLOUD_REST_TIMEOUT_NONE,
			      rest_ctx);

	if (err < 0) {
		LOG_ERR("http_client_req() error: %d", err);
		err = -EIO;
	} else if (rest_ctx->total_response_len >= rest_ctx->rx_buf_len) {
		/* 1 byte is reserved to NULL terminate the response */
		LOG_ERR("Receive buffer too small, %d bytes are required",
			rest_ctx->total_response_len + 1);
		err = -ENOBUFS;
	} else {
		err = 0;
	}

	return err;
}

static void init_request(struct http_request *const req, const enum http_method meth,
			 const char *const content_type)
{
	memset(req, 0, sizeof(struct http_request));

	req->host		= CONFIG_NRF_CLOUD_REST_HOST_NAME;
	req->protocol		= HTTP_PROTOCOL;

	req->response		= http_response_cb;
	req->method		= meth;
	req->content_type_value	= content_type;

	(void)nrf_cloud_codec_init();
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
	struct http_request http_req;

	init_request(&http_req, HTTP_PATCH, CONTENT_TYPE_APP_JSON);

	/* Format API URL with device ID */
	buff_sz = sizeof(API_DEVICES_STATE_TEMPLATE) + strlen(device_id);
	url = k_calloc(buff_sz, 1);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	http_req.url = url;

	ret = snprintk(url, buff_sz, API_DEVICES_STATE_TEMPLATE, device_id);
	if (ret < 0 || ret >= buff_sz) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	LOG_DBG("URL: %s", log_strdup(http_req.url));

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		NULL
	};

	http_req.header_fields = (const char **)headers;

	/* Set payload */
	http_req.payload = shadow_json;
	http_req.payload_len = strlen(shadow_json);
	LOG_DBG("Payload: %s", log_strdup(http_req.payload));

	/* Make REST call */
	ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
	if (ret) {
		ret = -EIO;
		goto clean_up;
	}

	LOG_DBG("API status: %d", rest_ctx->status);
	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_ACCEPTED) {
		ret = -EBADMSG;
		goto clean_up;
	}

	LOG_DBG("API call response len: %u bytes", rest_ctx->response_len);

clean_up:
	if (url) {
		k_free(url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
	}

	close_connection(rest_ctx);

	return ret;
}

int nrf_cloud_rest_shadow_service_info_update(struct nrf_cloud_rest_context *const rest_ctx,
	const char *const device_id, const struct nrf_cloud_svc_info * const svc_inf)
{
	if (svc_inf == NULL) {
		return -EINVAL;
	}

	int ret;
	struct nrf_cloud_data data_out;
	const struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = (struct nrf_cloud_svc_info *)svc_inf
	};

	(void)nrf_cloud_codec_init();

	ret = nrf_cloud_device_status_encode(&dev_status, &data_out, false);
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
	struct http_request http_req;

	init_request(&http_req, HTTP_PATCH, CONTENT_TYPE_APP_JSON);

	/* Format API URL with device and job ID */
	buff_sz = sizeof(API_UPDATE_FOTA_URL_TEMPLATE) +
		    strlen(device_id) + strlen(job_id);
	url = k_calloc(buff_sz, 1);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	http_req.url = url;

	ret = snprintk(url, buff_sz, API_UPDATE_FOTA_URL_TEMPLATE,
		       device_id, job_id);
	if (ret < 0 || ret >= buff_sz) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	LOG_DBG("URL: %s", log_strdup(http_req.url));

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		NULL
	};

	http_req.header_fields = (const char **)headers;

	/* Format payload */
	if (details) {
		buff_sz = sizeof(API_UPDATE_FOTA_DETAILS_TMPLT) +
			  strlen(job_status_strings[status]) +
			  strlen(details);
	} else {
		buff_sz = sizeof(API_UPDATE_FOTA_BODY_TEMPLATE) +
			  strlen(job_status_strings[status]);
	}

	payload = k_calloc(buff_sz, 1);
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
	if (ret < 0 || ret >= buff_sz) {
		LOG_ERR("Could not format payload");
		ret = -ETXTBSY;
		goto clean_up;
	}
	http_req.payload = payload;
	http_req.payload_len = strlen(http_req.payload);
	LOG_DBG("Payload: %s", log_strdup(http_req.payload));

	/* Make REST call */
	ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
	if (ret) {
		ret = -EIO;
		goto clean_up;
	}

	LOG_DBG("API status: %d", rest_ctx->status);
	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK) {
		ret = -EBADMSG;
		goto clean_up;
	}

	LOG_DBG("API call response len: %u bytes", rest_ctx->response_len);

clean_up:
	if (url) {
		k_free(url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
	}
	if (payload) {
		k_free(payload);
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
	struct http_request http_req;

	init_request(&http_req, HTTP_GET, CONTENT_TYPE_ALL);

	/* Format API URL with device ID */
	url_sz = sizeof(API_GET_FOTA_URL_TEMPLATE) +
		    strlen(device_id);
	url = k_calloc(url_sz, 1);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	http_req.url = url;

	ret = snprintk(url, url_sz, API_GET_FOTA_URL_TEMPLATE, device_id);
	if (ret < 0 || ret >= url_sz) {
		LOG_ERR("Could not format URL");
		ret = -ETXTBSY;
		goto clean_up;
	}

	LOG_DBG("URL: %s", log_strdup(http_req.url));

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		NULL
	};

	http_req.header_fields = (const char **)headers;

	/* Make REST call */
	ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
	if (ret) {
		ret = -EIO;
		goto clean_up;
	}

	LOG_DBG("API status: %d", rest_ctx->status);
	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK &&
	    rest_ctx->status != NRF_CLOUD_HTTP_STATUS_NOT_FOUND) {
		ret = -EBADMSG;
		goto clean_up;
	}

	LOG_DBG("API call response len: %u bytes", rest_ctx->response_len);

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
		k_free(url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
	}

	close_connection(rest_ctx);

	return ret;
}

void nrf_cloud_rest_fota_job_free(struct nrf_cloud_fota_job_info *const job)
{
	nrf_cloud_fota_job_free(job);
}

int nrf_cloud_rest_cell_pos_get(struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_cell_pos_request const *const request,
	struct nrf_cloud_cell_pos_result *const result)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(request->net_info != NULL);

	int ret;
	char *auth_hdr = NULL;
	char *payload = NULL;
	struct http_request http_req;

	init_request(&http_req, HTTP_POST, CONTENT_TYPE_APP_JSON);

	http_req.url = API_GET_CELL_POS_TEMPLATE;
	LOG_DBG("URL: %s", log_strdup(http_req.url));

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);

	if (ret) {
		LOG_ERR("Could not format HTTP auth header, err: %d", ret);
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		NULL
	};

	http_req.header_fields = (const char **)headers;

	/* Get payload */
	ret = nrf_cloud_format_cell_pos_req(request->net_info, 1, &payload);
	if (ret) {
		LOG_ERR("Failed to generate cellular positioning request, err: %d", ret);
		goto clean_up;
	}

	http_req.payload = payload;
	http_req.payload_len = strlen(http_req.payload);
	LOG_DBG("Payload: %s", log_strdup(http_req.payload));

	/* Make REST call */
	ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
	if (ret) {
		ret = -EIO;
		goto clean_up;
	}
	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK) {
		ret = -EBADMSG;
		goto clean_up;
	}
	if (!rest_ctx->response || !rest_ctx->response_len) {
		ret = -ENODATA;
		goto clean_up;
	}

	LOG_DBG("API call response len: %u bytes", rest_ctx->response_len);

	if (result) {
		ret = nrf_cloud_parse_cell_pos_response(rest_ctx->response, result);
		if (ret != 0) {
			if (ret > 0) {
				ret = -EBADMSG;
			}
			goto clean_up;
		}
		result->type = request->net_info->ncells_count ?
			       CELL_POS_TYPE_MULTI : CELL_POS_TYPE_SINGLE;
	}

clean_up:
	if (auth_hdr) {
		k_free(auth_hdr);
	}
	if (payload) {
		cJSON_free(payload);
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
	struct http_request http_req;
	char const *req_type = NULL;
	char custom_types[AGPS_CUSTOM_TYPE_STR_SZ];
	char range_hdr[HDR_RANGE_BYTES_SZ];

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

	init_request(&http_req, HTTP_GET, CONTENT_TYPE_APP_OCT_STR);

	/* Determine size of URL buffer and allocate */
	url_sz = sizeof(API_GET_AGPS_BASE);
	if (request->net_info) {
		url_sz += strlen(AGPS_NET_INFO) + NET_INFO_PRINT_SZ;
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

	url = k_calloc(url_sz, 1);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	http_req.url = url;

	/* Format API URL */
	ret = snprintk(url, url_sz, API_GET_AGPS_BASE);
	if (ret < 0 || ret >= url_sz) {
		LOG_ERR("Could not format URL: device id");
		ret = -ETXTBSY;
		goto clean_up;
	}
	pos = ret;
	remain = url_sz - ret;

	if (req_type) {
		ret = snprintk(&url[pos], remain, AGPS_REQ_TYPE, req_type);
		if (ret < 0 || ret >= remain) {
			LOG_ERR("Could not format URL: request type");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	if (request->type == NRF_CLOUD_REST_AGPS_REQ_CUSTOM) {
		ret = snprintk(&url[pos], remain, AGPS_CUSTOM_TYPE, custom_types);
		if (ret < 0 || ret >= remain) {
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
		if (ret < 0 || ret >= remain) {
			LOG_ERR("Could not format URL: network info");
			ret = -ETXTBSY;
			goto clean_up;
		}
		pos += ret;
		remain -= ret;
	}

	LOG_DBG("URL: %s", log_strdup(http_req.url));

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
		NULL
	};

	http_req.header_fields = (const char **)headers;

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

		ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
		if (ret) {
			ret = -EIO;
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

clean_up:
	if (url) {
		k_free(url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
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
	struct http_request http_req;

	init_request(&http_req, HTTP_GET, CONTENT_TYPE_ALL);

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

	url = k_calloc(url_sz, 1);
	if (!url) {
		ret = -ENOMEM;
		goto clean_up;
	}
	http_req.url = url;

	/* Format API URL */
	ret = snprintk(url, url_sz, API_GET_PGPS_BASE);
	if (ret < 0 || ret >= url_sz) {
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
			if (ret < 0 || ret >= remain) {
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
			if (ret < 0 || ret >= remain) {
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
			if (ret < 0 || ret >= remain) {
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
			if (ret < 0 || ret >= remain) {
				LOG_ERR("Could not format URL: GPS time");
				ret = -ETXTBSY;
				goto clean_up;
			}
			pos += ret;
			remain -= ret;
		}
	}
	LOG_DBG("URL: %s", log_strdup(http_req.url));

	/* Format auth header */
	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}
	char *const headers[] = {
		HDR_ACCEPT_APP_JSON,
		(char *const)auth_hdr,
		NULL
	};

	http_req.header_fields = (const char **)headers;

	/* Make REST call */
	ret = do_api_call(&http_req, rest_ctx, HTTPS_PORT, CONFIG_NRF_CLOUD_SEC_TAG);
	if (ret) {
		ret = -EIO;
		goto clean_up;
	}

	LOG_DBG("API status: %d", rest_ctx->status);
	if (rest_ctx->status != NRF_CLOUD_HTTP_STATUS_OK ||
	    !rest_ctx->response || !rest_ctx->response_len) {
		ret = -EBADMSG;
		goto clean_up;
	}

	LOG_DBG("API call response: %s", log_strdup(rest_ctx->response));

clean_up:
	if (url) {
		k_free(url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
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
	struct http_request http_req;
	char rx_buf[JITP_RX_BUF_SZ];
	char *const headers[] = {
		HDR_ACCEPT_ALL,
		NULL
	};
	struct nrf_cloud_rest_context rest_ctx = {
		.keep_alive = false,
		.connect_socket = -1,
		.timeout_ms = JITP_HTTP_TIMEOUT_MS,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf)
	};

	init_request(&http_req, HTTP_POST, JITP_CONTENT);
	http_req.host		= JITP_HOSTNAME_TLS;
	http_req.url		= JITP_URL;
	http_req.header_fields	= (const char **)headers;

	ret = do_api_call(&http_req, &rest_ctx, JITP_PORT, nrf_cloud_sec_tag);
	if (ret == 0) {
		if ((rest_ctx.status == 0) && (rest_ctx.response_len == 0)) {
			/* Expected response for an unprovisioned device.
			 * Wait 30s before associating device with account
			 */
		} else if ((rest_ctx.status == NRF_CLOUD_HTTP_STATUS_FORBIDDEN) &&
			   (rest_ctx.response_len > 0) &&
			   (strstr(rest_ctx.response, "\"message\":null"))) {
			/* Expected response for an already provisioned device.
			 * User can proceed to use the API.
			 */
			ret = 1;
		} else {
			ret = -ENODEV;
		}
	}

	close_connection(&rest_ctx);

	return ret;
}
