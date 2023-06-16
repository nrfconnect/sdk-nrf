/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <version.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <modem/modem_key_mgmt.h>
#include <net/rest_client.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_at.h>

#include <modem/modem_attest_token.h>

#include "nrf_provisioning_at.h"
#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_cbor_decode.h"
#include "nrf_provisioning_cbor_decode_types.h"
#include "nrf_provisioning_cbor_encode_types.h"
#include "nrf_provisioning_http.h"
#include "nrf_provisioning_jwt.h"

LOG_MODULE_REGISTER(nrf_provisioning_http, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

#if !defined(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)
#error "CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG not given"
#endif

#define API_VER "/v1"
#define API_PRV "/provisioning"

#define API_GET_CMDS API_VER API_PRV "/commands"
#define CMDS_AFTER "after=%s"
#define CMDS_CVER "cver=%s"
#define CMDS_MAX_RX_SZ "rxMaxSize=%s"
#define CMDS_MAX_TX_SZ "txMaxSize=%s"
#define CMDS_MVER "mver=%s"
#define API_CMDS_TEMPLATE (API_GET_CMDS "?" \
	CMDS_AFTER "&" CMDS_MAX_RX_SZ "&" CMDS_MAX_TX_SZ "&" CMDS_MVER "&" CMDS_CVER)

#define API_POST_RSLT API_VER API_PRV "/response"

#define LF_TOK '\n'
#define CR_TOK '\r'
#define CRLF "\r\n"

#define HDR_TYPE_ALL "*/*"
#define HDR_TYPE_APP_CBOR "application/cbor"
#define HDR_TYPE_USER_AGENT "User-Agent: Zephyr"

#define CONTENT_TYPE "Content-Type: "
#define CONTENT_TYPE_ALL (CONTENT_TYPE HDR_TYPE_ALL CRLF)
#define CONTENT_TYPE_APP_CBOR (CONTENT_TYPE HDR_TYPE_APP_CBOR CRLF)

#define AUTH_HDR_BEARER_PREFIX "Authorization: Bearer "
#define AUTH_HDR_BEARER_PREFIX_ATT "Authorization: Bearer att."
#define AUTH_HDR_BEARER_PREFIX_JWT "Authorization: Bearer "
#define HTTP_HDR_ACCEPT "Accept: "
#define HDR_ACCEPT_APP_CBOR (HTTP_HDR_ACCEPT HDR_TYPE_APP_CBOR CRLF)
#define HDR_ACCEPT_ALL (HTTP_HDR_ACCEPT HDR_TYPE_ALL CRLF)

#define PRV_CONTENT_TYPE_HDR (CONTENT_TYPE HDR_TYPE_ALL CRLF)
#define PRV_CONNECTION_HDR "Connection: close" CRLF

#define ZEPHYR_VER STRINGIFY(KERNEL_VERSION_MAJOR) "." STRINGIFY(KERNEL_VERSION_MINOR) " (" \
	STRINGIFY(BUILD_VERSION) " " CONFIG_BOARD ")"
#define USER_AGENT_HDR (HDR_TYPE_USER_AGENT "/" ZEPHYR_VER CRLF)


int nrf_provisioning_http_init(struct nrf_provisioning_mm_change *mmode)
{
	static bool initialized;
	int ret;

	nrf_provisioning_codec_init(mmode);

	if (initialized) {
		return 0;
	}

	/* Modem info library is used to obtain the modem FW version */
	ret = modem_info_init();
	if (ret) {
		LOG_ERR("Modem info initialization failed, error: %d", ret);
		return ret;
	}

	initialized = true;
	return 0;
}

static void print_req_info(struct rest_client_req_context *req, int hdr_cnt)
{
	LOG_DBG("req->connect_socket: %d", req->connect_socket);
	LOG_DBG("req->keep_alive: %d", req->keep_alive);
	LOG_DBG("req->resp_buff: %s", req->resp_buff ? "exists" : "null");
	LOG_DBG("req->resp_buff_len: %d", req->resp_buff_len);
	LOG_DBG("req->host: %s", req->host);
	LOG_DBG("req->port: %d", req->port);
	LOG_DBG("req->tls_peer_verify: %d", req->tls_peer_verify);
	LOG_DBG("req->sec_tag: %d", req->sec_tag);
	LOG_DBG("req->timeout_ms: %d", req->timeout_ms);
	LOG_DBG("req->http_method: %d", req->http_method);
	LOG_DBG("req->url: %s", req->url);
	for (int i = 0; i < hdr_cnt; i++) {
		LOG_DBG("req->header_field %d: %s", i, req->header_fields[i]);
	}
}

static int max_auth_prefix_len(void)
{
	int prefix_len = sizeof(AUTH_HDR_BEARER_PREFIX) - 1;

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_ATTESTTOKEN)) {
		prefix_len = MAX(prefix_len, sizeof(AUTH_HDR_BEARER_PREFIX_ATT) - 1);
	}
	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT)) {
		prefix_len = MAX(prefix_len, sizeof(AUTH_HDR_BEARER_PREFIX_JWT) - 1);
	}

	return prefix_len;
}

static int max_token_len(void)
{
	int token_len = 0;

#if defined(CONFIG_NRF_PROVISIONING_AT_ATTESTTOKEN_MAX_LEN)
	token_len = MAX(token_len, CONFIG_NRF_PROVISIONING_AT_ATTESTTOKEN_MAX_LEN);
#endif

#if defined(CONFIG_MODEM_JWT_MAX_LEN)
	token_len = MAX(token_len, CONFIG_MODEM_JWT_MAX_LEN);
#endif

	return token_len;
}

/* Generate an authorization header value string in the form:
 * "Authorization: Bearer JWT/ATTESTTOKEN\r\n"
 */
static int generate_auth_header(const char *const tok, char **auth_hdr_out)
{
	int ret;
	/* These lengths NOT including null terminators */
	int tok_len;
	int prefix_len;
	int postfix_len = sizeof(CRLF) - 1;
	int hdr_size;

	char *prefix_ptr;
	char *tok_ptr;
	char *postfix_ptr;

	if (!(tok ||
		IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT) ||
		IS_ENABLED(CONFIG_NRF_PROVISIONING_ATTESTTOKEN))) {
		LOG_ERR("Cannot generate auth header, no token was given, "
			"and generation is not enabled");
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}
	if (!auth_hdr_out) {
		LOG_ERR("Cannot generate auth header, no output pointer given.");
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	if (tok) {
		tok_len = strlen(tok);
		prefix_len = sizeof(AUTH_HDR_BEARER_PREFIX) - 1;
	} else {
		/* Any of the generatable tokens */
		tok_len = max_token_len();
		prefix_len = max_auth_prefix_len();
	}

	hdr_size = prefix_len + tok_len + postfix_len + 1;
	*auth_hdr_out = k_malloc(hdr_size);
	if (!*auth_hdr_out) {
		return -ENOMEM;
	}

	memset(*auth_hdr_out, 0x0, hdr_size);

	prefix_ptr = *auth_hdr_out;

	/* Write the prefix and copy the given token if it exists*/
	if (tok) {
		tok_ptr = prefix_ptr + prefix_len;
		memcpy(prefix_ptr, AUTH_HDR_BEARER_PREFIX, prefix_len);
		memcpy(tok_ptr, tok, tok_len);
		goto postfix;
	}

	/* Generate a token, if none was given */
	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_ATTESTTOKEN)) {
		prefix_len = strlen(AUTH_HDR_BEARER_PREFIX_ATT);
		tok_ptr = prefix_ptr + prefix_len;

		memcpy(prefix_ptr, AUTH_HDR_BEARER_PREFIX_ATT, prefix_len);

		ret = nrf_provisioning_at_attest_token_get(tok_ptr, tok_len);

		if (ret != 0) {
			LOG_INF("Failed to generate attestation token, error: %d", ret);
			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT)) {
				goto a_alt_tok;
			}

			goto fail;
		}

		tok_len = strlen(tok_ptr);
		goto postfix;
	}
a_alt_tok:
	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT)) {
		prefix_len = strlen(AUTH_HDR_BEARER_PREFIX_JWT);
		tok_ptr = prefix_ptr + prefix_len;

		memcpy(prefix_ptr, AUTH_HDR_BEARER_PREFIX, prefix_len);

		ret = nrf_provisioning_jwt_generate(0, tok_ptr, tok_len + 1);

		if (ret < 0) {
			LOG_INF("Failed to generate JWT, error: %d", ret);
			goto fail;
		}
		tok_len = strlen(tok_ptr);
	}

postfix:
	postfix_ptr = tok_ptr + tok_len;
	memcpy(postfix_ptr, CRLF, postfix_len + 1);

	return 0;

fail:
	k_free(*auth_hdr_out);
	*auth_hdr_out = NULL;

	return ret;
}

static void init_rest_client_request(struct nrf_provisioning_http_context const *const rest_ctx,
				     struct rest_client_req_context *const req,
				     const enum http_method meth)
{
	memset(req, 0, sizeof(*req));

	rest_client_request_defaults_set(req);

	req->connect_socket = rest_ctx->connect_socket;
	req->keep_alive = rest_ctx->keep_alive;

	req->resp_buff = rest_ctx->rx_buf;
	req->resp_buff_len = rest_ctx->rx_buf_len;

	req->host = CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME;
	req->port = CONFIG_NRF_PROVISIONING_HTTP_PORT;
	req->tls_peer_verify = TLS_PEER_VERIFY_REQUIRED;
	req->sec_tag = CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG;
	req->timeout_ms = CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS;

	req->http_method = meth;

	req->header_fields = NULL;
	req->url = NULL;
	req->body = NULL;
}

static int gen_result_url(struct rest_client_req_context *const req)
{
	char *url;
	size_t buff_sz;
	int ret;

	buff_sz = sizeof(API_POST_RSLT);
	url = k_malloc(buff_sz);
	if (!url) {
		ret = -ENOMEM;
		return ret;
	}

	req->url = url;

	ret = snprintk(url, buff_sz, API_POST_RSLT);

	if ((ret < 0) || (ret >= buff_sz)) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	return 0;
}

static int gen_provisioning_url(struct rest_client_req_context *const req)
{
	char *url;
	size_t buff_sz;
	char *rx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ);
	char *tx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_HTTP_TX_BUF_SZ);
	char mver[128];
	char *cver = STRINGIFY(1);
	int ret;
	char *mvernmb;
	int cnt;
	char after[NRF_PROVISIONING_CORRELATION_ID_SIZE];

	memcpy(after, nrf_provisioning_codec_get_latest_cmd_id(),
		NRF_PROVISIONING_CORRELATION_ID_SIZE);

	ret = modem_info_string_get(MODEM_INFO_FW_VERSION, mver, sizeof(mver));

	if (ret <= 0) {
		LOG_ERR("Failed to get modem FW version");
		return ret ? ret : -ENODATA;
	}

	mvernmb = strtok(mver, "_-");
	cnt = 1;

	/* mfw_nrf9160_1.3.2-FOTA-TEST - for example */
	while (cnt++ < 3) {
		mvernmb = strtok(NULL, "_-");
	}

	buff_sz = sizeof(API_CMDS_TEMPLATE) +
		strlen(after) + strlen(rx_buf_sz) + strlen(tx_buf_sz) +
		strlen(mvernmb) + strlen(cver);
	url = k_malloc(buff_sz);
	if (!url) {
		ret = -ENOMEM;
		return ret;
	}

	req->url = url;

	ret = snprintk(url, buff_sz,
		API_CMDS_TEMPLATE, after, rx_buf_sz, tx_buf_sz, mvernmb, cver);

	if ((ret < 0) || (ret >= buff_sz)) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	return 0;
}

/**
 * @brief Reports responses to server.
 *
 * @param rest_ctx Connection information.
 * @param req HTTP request.
 * @param resp HTTP response.
 * @param cdc_ctx Payload information.
 * @return <0 on error.
 */
static int nrf_provisioning_responses_req(struct nrf_provisioning_http_context *const rest_ctx,
					struct rest_client_req_context *req,
					struct rest_client_resp_context *resp,
					struct cdc_context *const cdc_ctx)
{
	int ret;
	char *auth_hdr = NULL;


	init_rest_client_request(rest_ctx, req, HTTP_POST);
	memset(resp, 0, sizeof(*resp));

	ret = gen_result_url(req);
	if (ret) {
		goto clean_up;
	}

	ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header");
		goto clean_up;
	}

	char *const headers[] = { CONTENT_TYPE_APP_CBOR,
				  HDR_ACCEPT_ALL,
				  (char *const)auth_hdr,
				  USER_AGENT_HDR,
				  PRV_CONNECTION_HDR,
				  NULL };

	req->header_fields = (const char **)headers;

	/* Set the CBOR payload */
	req->body = cdc_ctx->opkt;
	req->body_len = cdc_ctx->opkt_sz;

	print_req_info(req, sizeof(headers) / sizeof(*headers) - 1);

	ret = rest_client_request(req, resp);
	if (ret < 0) {
		goto clean_up;
	}

	if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_OK) {
		ret = 0;
	} else if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_BAD_REQ) {
		LOG_ERR("Bad request");
		ret = -EINVAL;
	} else if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_UNAUTH) {
		LOG_ERR("Device didn't send auth credentials");
		ret = -EACCES;
	} else if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_FORBIDDEN) {
		LOG_ERR("Device provided wrong auth credentials");
		ret = -EACCES;
	} else if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_UNS_MEDIA_TYPE) {
		LOG_ERR("Wrong content format");
		ret = -ENOMSG;
	} else if (resp->http_status_code == NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR) {
		LOG_WRN("Server busy");
		ret = -EBUSY;
	} else {
		__ASSERT(false, "Unsupported HTTP response code");
		LOG_ERR("Unsupported HTTP response code");
		ret = -ENOSYS;
	}

clean_up:
	if (req->url) {
		k_free((char *)req->url);
		req->url = NULL;
	}
	if (auth_hdr) {
		k_free(auth_hdr);
	}

	return ret;
}

int nrf_provisioning_http_req(struct nrf_provisioning_http_context *const rest_ctx)
{
	__ASSERT_NO_MSG(rest_ctx != NULL);

	/* Only one provisioning ongoing at a time*/
	static union {
		char http[CONFIG_NRF_PROVISIONING_HTTP_TX_BUF_SZ];
		char at[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	} tx_buf;
	static char rx_buf[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];

	char *auth_hdr = NULL;
	struct rest_client_req_context req;
	struct rest_client_resp_context resp;
	int ret;
	struct cdc_context cdc_ctx;
	bool finished = false;

	rest_ctx->rx_buf = rx_buf;
	rest_ctx->rx_buf_len = sizeof(rx_buf);
	rest_ctx->keep_alive = false;

	/* While there are commands to be processed */
	while (true) {
		init_rest_client_request(rest_ctx, &req, HTTP_GET);
		memset(&resp, 0, sizeof(resp));

		ret = gen_provisioning_url(&req);
		if (ret) {
			break;
		}

		ret = generate_auth_header(rest_ctx->auth, &auth_hdr);
		if (ret) {
			break;
		}

		char *const headers[] = { HDR_ACCEPT_APP_CBOR,
					(char *const)auth_hdr,
					PRV_CONTENT_TYPE_HDR,
					USER_AGENT_HDR,
					PRV_CONNECTION_HDR,
					NULL };

		req.header_fields = (const char **)headers;

		print_req_info(&req, sizeof(headers) / sizeof(*headers) - 1);

		ret = rest_client_request(&req, &resp);
		k_free(auth_hdr);
		auth_hdr = NULL;
		k_free((char *)req.url);
		req.url = NULL;

		if (ret < 0) {
			break;
		}

		LOG_INF("Connected");

		if (resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT) {
			LOG_INF("No more commands to process on server side");
			ret = 0;
		} else if (resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_BAD_REQ) {
			LOG_ERR("Bad request");
			ret = -EINVAL;
		} else if (resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_UNAUTH) {
			LOG_ERR("Device didn't send auth credentials");
			ret = -EACCES;
		} else if (resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_FORBIDDEN) {
			LOG_ERR("Device provided wrong auth credentials");
			ret = -EACCES;
		} else if (resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_OK) {

			/* Codec state tracking spawns over
			 * - Commands request
			 * - Commands processing
			 * - Responses/error reporting
			 *
			 * To avoid allocating big chunks of extra memory for AT commands - which
			 * might include certificates etc - lets use the same buffer which is used
			 * by the HTTP requests. RX buffer can't be used because the decoded CBOR
			 * strings are still in the buffer. The TX buffer is free to be used as an
			 * intermediate storage as long as the codec does not start encoding the
			 * responses. This does break the layering of code but needs to be done
			 * to achieve the memory savings.
			 */
			nrf_provisioning_codec_setup(&cdc_ctx, tx_buf.at, sizeof(tx_buf));

			/* Codec input and output buffers */
			cdc_ctx.ipkt = resp.response;
			cdc_ctx.ipkt_sz = resp.response_len;
			cdc_ctx.opkt = tx_buf.http;
			cdc_ctx.opkt_sz = sizeof(tx_buf);

			ret = nrf_provisioning_codec_process_commands();
			if (ret < 0) {
				break;
			} else if (ret > 0) {
				/* Need to send responses before we are done */
				finished = true;
			}

			ret = nrf_provisioning_responses_req(rest_ctx, &req, &resp, &cdc_ctx);
			if (ret < 0) {
				finished = false;
			}

			/* Provisioning finished */
			if (finished) {
				ret = NRF_PROVISIONING_FINISHED;
				break;
			}
			nrf_provisioning_codec_teardown();
			continue;
		} else if (
			resp.http_status_code == NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR) {
			LOG_WRN("Internal server error");
			ret = -EBUSY;
		} else if (!resp.http_status_code) {
			LOG_WRN("Null response - socket closed");
			ret = -ESHUTDOWN;
		} else {
			LOG_ERR("Unknown HTTP response code: %d", resp.http_status_code);
			__ASSERT(false, "Unknown HTTP response code: %d", resp.http_status_code);
			ret = -ENOTSUP;
		}
		break;
	}

	nrf_provisioning_codec_teardown();

	if (req.url) {
		k_free((char *)req.url);
	}
	if (auth_hdr) {
		k_free(auth_hdr);
	}

	return ret;
}
