/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "cmock_date_time.h"
#include "cmock_lte_lc.h"
#include "cmock_modem_key_mgmt.h"
#include "cmock_modem_info.h"
#include "cmock_nrf_modem_lib.h"
#include "cmock_nrf_modem_at.h"
#include "cmock_nrf_provisioning_at.h"
#include "cmock_rest_client.h"
#include "cmock_settings.h"

#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_http.h"
#include "nrf_provisioning_internal.h"
#include "net/nrf_provisioning.h"

#define AUTH_HDR_BEARER "Authorization: Bearer "
#define AUTH_HDR_BEARER_PREFIX_ATT AUTH_HDR_BEARER "att."
#define AUTH_HDR_BEARER_ATT_DUMMY "dummy_attestation_token"

#define CRLF "\r\n"

#define QUERY_ITEMS_MAX 5

char tok_att_plain[] = AUTH_HDR_BEARER_ATT_DUMMY;
char http_auth_hdr[] = AUTH_HDR_BEARER_PREFIX_ATT AUTH_HDR_BEARER_ATT_DUMMY CRLF;
char http_auth_hdr_invalid1[] = AUTH_HDR_BEARER_ATT_DUMMY CRLF;
char http_auth_hdr_invalid2[] = "att." AUTH_HDR_BEARER_ATT_DUMMY CRLF;

char MFW_VER[] = "mfw_nrf9161_99.99.99-DUMMY";
char MFW_VER_NMB[] = "99.99.99";

static void dummy_nrf_provisioning_device_mode_cb(void *user_data)
{
	(void)user_data;
}

static int dummy_nrf_provisioning_modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	(void)user_data;
	(void)new_mode;

	return 0;
}

/* Malloc and free are to be used in native_posix environment */
void *__wrap_k_malloc(size_t size)
{
	return malloc(size);
}

void __wrap_k_free(void *ptr)
{
	free(ptr);
}

void setUp(void)
{
}

void tearDown(void)
{
}

static int nrf_provisioning_mm_cb_dummy(enum lte_lc_func_mode new_mode, void *user_data)
{
	(void)new_mode;
	int ret = *(int *)user_data;

	return ret;
}

/* [["9685ef84-8cac-4257-b5cc-94bc416a1c1d.d", 2]] */
static unsigned char cbor_cmds1_valid[] = {
	0x81, 0x82, 0x78, 0x26, 0x39, 0x36, 0x38, 0x35, 0x65,
	0x66, 0x38, 0x34, 0x2d, 0x38, 0x63, 0x61, 0x63, 0x2d,
	0x34, 0x32, 0x35, 0x37, 0x2d, 0x62, 0x35, 0x63, 0x63,
	0x2d, 0x39, 0x34, 0x62, 0x63, 0x34, 0x31, 0x36, 0x61,
	0x31, 0x63, 0x31, 0x64, 0x2e, 0x64, 0x02
};

/* [["9685ef84-8cac-4257-b5cc-94bc416a1c1d.d", 102]] */
static const unsigned char cbor_rsps1_valid[] = {
	0x81, 0x82, 0x78, 0x26, 0x39, 0x36, 0x38, 0x35, 0x65, 0x66, 0x38, 0x34, 0x2d, 0x38, 0x63,
	0x61, 0x63, 0x2d, 0x34, 0x32, 0x35, 0x37, 0x2d, 0x62, 0x35, 0x63, 0x63, 0x2d, 0x39, 0x34,
	0x62, 0x63, 0x34, 0x31, 0x36, 0x61, 0x31, 0x63, 0x31, 0x64, 0x2e, 0x64, 0x18, 0x66
};
/* [
 *   ["89ed369b-0fed-4194-a703-aabb7fe8130b.e.s", 0, '%CMNG', '3,16842753,2', [513]],
 *   ["89ed369b-0fed-4194-a703-aabb7fe8130b.e" 0, '%KEYGEN', '16842753,2,1', []]
 * ]
 */
static unsigned char cbor_cmds2_valid[] = {
	0x82, 0x85, 0x78, 0x28, 0x38, 0x39, 0x65, 0x64, 0x33, 0x36, 0x39, 0x62, 0x2d, 0x30, 0x66,
	0x65, 0x64, 0x2d, 0x34, 0x31, 0x39, 0x34, 0x2d, 0x61, 0x37, 0x30, 0x33, 0x2d, 0x61, 0x61,
	0x62, 0x62, 0x37, 0x66, 0x65, 0x38, 0x31, 0x33, 0x30, 0x62, 0x2e, 0x65, 0x2e, 0x73, 0x00,
	0x65, 0x25, 0x43, 0x4d, 0x4e, 0x47, 0x6c, 0x33, 0x2c, 0x31, 0x36, 0x38, 0x34, 0x32, 0x37,
	0x35, 0x33, 0x2c, 0x32, 0x81, 0x19, 0x02, 0x01, 0x85, 0x78, 0x26, 0x38, 0x39, 0x65, 0x64,
	0x33, 0x36, 0x39, 0x62, 0x2d, 0x30, 0x66, 0x65, 0x64, 0x2d, 0x34, 0x31, 0x39, 0x34, 0x2d,
	0x61, 0x37, 0x30, 0x33, 0x2d, 0x61, 0x61, 0x62, 0x62, 0x37, 0x66, 0x65, 0x38, 0x31, 0x33,
	0x30, 0x62, 0x2e, 0x65, 0x00, 0x67, 0x25, 0x4b, 0x45, 0x59, 0x47, 0x45, 0x4e, 0x6c, 0x31,
	0x36, 0x38, 0x34, 0x32, 0x37, 0x35, 0x33, 0x2c, 0x32, 0x2c, 0x31, 0x80
};

/* [
 *   ["89ed369b-0fed-4194-a703-aabb7fe8130b.e.s", 100, ""],
 *   ["89ed369b-0fed-4194-a703-aabb7fe8130b.e", 100, ""]
 * ]
 */
static const unsigned char cbor_rsps2_valid[] = {
	0x82, 0x83, 0x78, 0x28, 0x38, 0x39, 0x65, 0x64, 0x33, 0x36, 0x39, 0x62, 0x2d,
	0x30, 0x66, 0x65, 0x64, 0x2d, 0x34, 0x31, 0x39, 0x34, 0x2d, 0x61, 0x37, 0x30,
	0x33, 0x2d, 0x61, 0x61, 0x62, 0x62, 0x37, 0x66, 0x65, 0x38, 0x31, 0x33, 0x30,
	0x62, 0x2e, 0x65, 0x2e, 0x73, 0x18, 0x64, 0x60, 0x83, 0x78, 0x26, 0x38, 0x39,
	0x65, 0x64, 0x33, 0x36, 0x39, 0x62, 0x2d, 0x30, 0x66, 0x65, 0x64, 0x2d, 0x34,
	0x31, 0x39, 0x34, 0x2d, 0x61, 0x37, 0x30, 0x33, 0x2d, 0x61, 0x61, 0x62, 0x62,
	0x37, 0x66, 0x65, 0x38, 0x31, 0x33, 0x30, 0x62, 0x2e, 0x65, 0x18, 0x64, 0x60
};

/* [["89ed369b-0fed-4194-a703-aabb7fe8130b.e.s", 99, 514, ""]] */
static const unsigned char cbor_rsps2_cmee514_valid[] = {
	0x81, 0x84, 0x78, 0x28, 0x38, 0x39, 0x65, 0x64, 0x33, 0x36, 0x39, 0x62, 0x2D,
	0x30, 0x66, 0x65, 0x64, 0x2D, 0x34, 0x31, 0x39, 0x34, 0x2D, 0x61, 0x37, 0x30,
	0x33, 0x2D, 0x61, 0x61, 0x62, 0x62, 0x37, 0x66, 0x65, 0x38, 0x31, 0x33, 0x30,
	0x62, 0x2E, 0x65, 0x2E, 0x73, 0x18, 0x63, 0x19, 0x02, 0x02, 0x60
};

/* [["80b8095e-2714-459e-bd8b-3745d8410a18.f",0,"%KEYGEN","16842753,8,1",[]]] */
static unsigned char cbor_cmds3_valid[] = {
	0x81, 0x85, 0x78, 0x26, 0x38, 0x30, 0x62, 0x38, 0x30, 0x39, 0x35, 0x65, 0x2D,
	0x32, 0x37, 0x31, 0x34, 0x2D, 0x34, 0x35, 0x39, 0x65, 0x2D, 0x62, 0x64, 0x38,
	0x62, 0x2D, 0x33, 0x37, 0x34, 0x35, 0x64, 0x38, 0x34, 0x31, 0x30, 0x61, 0x31,
	0x38, 0x2E, 0x66, 0x00, 0x67, 0x25, 0x4B, 0x45, 0x59, 0x47, 0x45, 0x4E, 0x6C,
	0x31, 0x36, 0x38, 0x34, 0x32, 0x37, 0x35, 0x33, 0x2C, 0x38, 0x2C, 0x31, 0x80
};

/* [["80b8095e-2714-459e-bd8b-3745d8410a18.f", 100, ""]] */
static const unsigned char cbor_rsps3_valid[] = {
	0x81, 0x83, 0x78, 0x26, 0x38, 0x30, 0x62, 0x38, 0x30, 0x39, 0x35, 0x65, 0x2D, 0x32, 0x37,
	0x31, 0x34, 0x2D, 0x34, 0x35, 0x39, 0x65, 0x2D, 0x62, 0x64, 0x38, 0x62, 0x2D, 0x33, 0x37,
	0x34, 0x35, 0x64, 0x38, 0x34, 0x31, 0x30, 0x61, 0x31, 0x38, 0x2E, 0x66, 0x18, 0x64, 0x60
};

/* [["80b8095e-2714-459e-bd8b-3745d8410a18.f", 99, 0, "ERROR"]] */
static unsigned char cbor_rsps3_at_failure_valid[] = {
	0x81, 0x84, 0x78, 0x26, 0x38, 0x30, 0x62, 0x38, 0x30, 0x39, 0x35, 0x65, 0x2D, 0x32, 0x37,
	0x31, 0x34, 0x2D, 0x34, 0x35, 0x39, 0x65, 0x2D, 0x62, 0x64, 0x38, 0x62, 0x2D, 0x33, 0x37,
	0x34, 0x35, 0x64, 0x38, 0x34, 0x31, 0x30, 0x61, 0x31, 0x38, 0x2E, 0x66, 0x18, 0x63, 0x00,
	0x65, 0x45, 0x52, 0x52, 0x4F, 0x52
};

/* [["e5256918-aba9-4d12-8b87-f2c07affec5d.14",1,{"provisioning/interval-sec":"99"}]] */
static unsigned char cbor_cmds4_valid[] = {
	0x81, 0x83, 0x78, 0x27, 0x65, 0x35, 0x32, 0x35, 0x36, 0x39, 0x31, 0x38, 0x2D, 0x61, 0x62,
	0x61, 0x39, 0x2D, 0x34, 0x64, 0x31, 0x32, 0x2D, 0x38, 0x62, 0x38, 0x37, 0x2D, 0x66, 0x32,
	0x63, 0x30, 0x37, 0x61, 0x66, 0x66, 0x65, 0x63, 0x35, 0x64, 0x2E, 0x31, 0x34, 0x01, 0xA1,
	0x78, 0x19, 0x70, 0x72, 0x6F, 0x76, 0x69, 0x73, 0x69, 0x6F, 0x6E, 0x69, 0x6E, 0x67, 0x2F,
	0x69, 0x6E, 0x74, 0x65, 0x72, 0x76, 0x61, 0x6C, 0x2D, 0x73, 0x65, 0x63, 0x62, 0x39, 0x39
};

/* [["e5256918-aba9-4d12-8b87-f2c07affec5d.14", 101]] */
static const unsigned char cbor_rsps4_valid[] = {
	0x81, 0x82, 0x78, 0x27, 0x65, 0x35, 0x32, 0x35, 0x36, 0x39, 0x31, 0x38, 0x2D, 0x61, 0x62,
	0x61, 0x39, 0x2D, 0x34, 0x64, 0x31, 0x32, 0x2D, 0x38, 0x62, 0x38, 0x37, 0x2D, 0x66, 0x32,
	0x63, 0x30, 0x37, 0x61, 0x66, 0x66, 0x65, 0x63, 0x35, 0x64, 0x2E, 0x31, 0x34, 0x18, 0x65
};

static int modem_key_mgmt_exists_true(nrf_sec_tag_t sec_tag,
					enum modem_key_mgmt_cred_type cred_type,
					bool *exists,
					int cmock_num_calls)
{
	*exists = true;

	return 0;
}


static int rest_client_request_resp_no_content(struct rest_client_req_context *req_ctx,
					       struct rest_client_resp_context *resp_ctx,
					       int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;

	return 0;
}

static int rest_client_request_resp_internal_server_err(struct rest_client_req_context *req_ctx,
							struct rest_client_resp_context *resp_ctx,
							int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR;

	return 0;
}

static int rest_client_request_resp_unknown_err1(struct rest_client_req_context *req_ctx,
						 struct rest_client_resp_context *resp_ctx,
						 int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_UNHANDLED;

	return 0;
}

static int rest_client_request_resp_unknown_err2(struct rest_client_req_context *req_ctx,
						 struct rest_client_resp_context *resp_ctx,
						 int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR + 1;

	return 0;
}

static int rest_client_request_finished(struct rest_client_req_context *req_ctx,
					struct rest_client_resp_context *resp_ctx,
					int cmock_num_calls)
{
	if (cmock_num_calls == 1) {
		goto responses_req;
	}

	resp_ctx->response = req_ctx->resp_buff;

	memcpy(resp_ctx->response, cbor_cmds1_valid, sizeof(cbor_cmds1_valid));
	resp_ctx->response_len = sizeof(cbor_cmds1_valid);

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;

	return 0;

responses_req:

	TEST_ASSERT_EQUAL_INT(0, memcmp(req_ctx->body, cbor_rsps1_valid, req_ctx->body_len));

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;

	return 0;
}

static int rest_client_request_auth_hdr_valid(struct rest_client_req_context *req_ctx,
					      struct rest_client_resp_context *resp_ctx,
					      int cmock_num_calls)
{
	int cnt = 0;

	for (int i = 0; req_ctx->header_fields && req_ctx->header_fields[i]; i++) {
		if (strcmp(http_auth_hdr, req_ctx->header_fields[i]) == 0) {
			cnt++;
		}
	}

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;

	TEST_ASSERT_EQUAL_INT(1, cnt);

	return 0;
}

static int rest_client_request_server_first_busy(struct rest_client_req_context *req_ctx,
					      struct rest_client_resp_context *resp_ctx,
					      int cmock_num_calls)
{
	if (cmock_num_calls == 0) {
		resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR;
	} else {
		resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;
	}

	return 0;
}

static int rest_client_request_auth_hdr_invalid(struct rest_client_req_context *req_ctx,
						struct rest_client_resp_context *resp_ctx,
						int cmock_num_calls)
{
	int cnt = 0;

	for (int i = 0; req_ctx->header_fields && req_ctx->header_fields[i]; i++) {
		if (strcmp(http_auth_hdr_invalid1, req_ctx->header_fields[i]) == 0) {
			cnt++;
		} else if (strcmp(http_auth_hdr_invalid2, req_ctx->header_fields[i]) == 0) {
			cnt++;
		}
	}

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;

	TEST_ASSERT_EQUAL_INT(0, cnt);

	return 0;
}

static int rest_client_request_url_valid(struct rest_client_req_context *req_ctx,
					 struct rest_client_resp_context *resp_ctx,
					 int cmock_num_calls)
{
	char *tokens;
	struct url_info {
		char *apiver;
		char *mversion;
		char *cversion;
		char *endpoint;
		char *command;
		char *txMaxSize;
		char *rxMaxSize;
	};

	/* 'txMaxSize', 'rxMaxSize', 'mver', 'cver' and 'after' */
	char *query_items[QUERY_ITEMS_MAX] = { NULL, NULL, NULL, NULL };

	struct url_info info = { .apiver = 0,
				 .mversion = NULL,
				 .endpoint = NULL,
				 .command = NULL,
				 .txMaxSize = NULL,
				 .rxMaxSize = NULL };

	tokens = (char *)malloc(strlen(req_ctx->url) + 1);

	TEST_ASSERT_NOT_NULL(tokens);

	strcpy(tokens, req_ctx->url);

	char *tok = strtok(tokens, "/");

	int sgmtc = 0; /* Path segment count */
	int qcnt = 0; /* Query item count */

	while (tok) {
		switch (sgmtc) {
		case 0:
			info.apiver = tok;
			tok = strtok(NULL, "/");
			sgmtc++;
			break;
		case 1:
			info.endpoint = tok;
			tok = strtok(NULL, "?");
			sgmtc++;
			break;
		case 2:
			info.command = tok;
			tok = strtok(NULL, "&");
			sgmtc++;
			break;
		default:
			query_items[qcnt++] = tok;
			tok = strtok(NULL, "&");
			break;
		}
	}

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;

	/* '/v1/provisioning/commands?txMaxSize=1536&rxMaxSize=1536&mver=2.0.0&cver=1' */

	TEST_ASSERT_EQUAL_INT(3, sgmtc);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(1, atoi(&(info.apiver[1]))); /* No 'v' */
	TEST_ASSERT_EQUAL_STRING("provisioning", info.endpoint);
	TEST_ASSERT_EQUAL_STRING("commands", info.command);

	/* At least 'after' */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(1, qcnt);
	/* No more than 'txMaxSize', 'rxMaxSize', 'mver', 'cver' and 'after'*/
	TEST_ASSERT_LESS_OR_EQUAL_INT(QUERY_ITEMS_MAX, qcnt);

	for (int idx = 0; idx < QUERY_ITEMS_MAX && query_items[idx]; idx++) {
		if (strncmp(query_items[idx], "mver=", strlen("mver=")) == 0) {
			info.mversion = &(query_items[idx][strlen("mver=")]);
			TEST_ASSERT_EQUAL_STRING(MFW_VER_NMB, info.mversion);
		} else if (strncmp(query_items[idx], "cver=", strlen("cver=")) == 0) {
			info.cversion = &(query_items[idx][strlen("cver=")]);
			TEST_ASSERT_GREATER_OR_EQUAL_INT(1, atoi(info.cversion));
		} else if (strncmp(query_items[idx], "txMaxSize=", strlen("txMaxSize=")) == 0) {
			info.txMaxSize = &(query_items[idx][strlen("txMaxSize=")]);
			TEST_ASSERT_EQUAL_INT(
				CONFIG_NRF_PROVISIONING_HTTP_TX_BUF_SZ, atoi(info.txMaxSize));
		} else if (strncmp(query_items[idx], "rxMaxSize=", strlen("rxMaxSize=")) == 0) {
			info.rxMaxSize = &(query_items[idx][strlen("rxMaxSize=")]);
			TEST_ASSERT_EQUAL_INT(
				CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ, atoi(info.rxMaxSize));
		} else if (strncmp(query_items[idx], "after=", strlen("after=")) == 0) {
			;
		} else {
			TEST_ASSERT(false);
		}
	}

	return 0;
}

/*
 * - Attestation token is generated and placed as one of the HTTP headers
 * - To have a valid token for authentication
 * - Retrieves a dummy attestation token and writes an authentication header based on it
 */
void test_http_commands_auth_hdr_valid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_valid);

	nrf_provisioning_http_req(&rest_ctx);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Having plain attestation token as authentication header without the 'att.'-prefix is detected
 * - The protocol requires a token specific prefix.
 * - Retrieves a dummy attestation token and writes an authentication header based on it
 */
void test_http_commands_auth_hdr_invalid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_invalid);

	nrf_provisioning_http_req(&rest_ctx);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Generation of a valid url
 * - To have all the url arguments in place that the server side requires
 * - Retrieves all the required arguments and builds the url based on those. The end result can be
 *   inspected from the REST client request which has the url.
 */
void test_http_commands_url_valid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_url_valid);

	nrf_provisioning_http_req(&rest_ctx);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Check if there is no commands from server
 * - If there is no commands from server side no new requests should be triggered
 * - Response status must indicate there is no content
 */
void test_http_commands_no_content_valid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_no_content);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Check if there is no commands from server
 * - If there is no commands from server side no new requests should be triggered
 * - Response status must indicate there is no content
 */
void test_http_commands_internal_server_error_invalid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_internal_server_err);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(-EBUSY, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Unsupported HTTP response codes are detected
 * - Unknown response error codes shouldn't cause any issues
 * - After receiving a response with unknown error code -ENOTSUP is reported back
 */
void test_http_commands_unknown_error_invalid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();

	/* 1st */
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_unknown_err1);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);

	/* 2nd */
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_unknown_err2);

	ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();

}

/*
 * - Receiving and responding to a known command
 * - To see that a command is fetched, decoded, executed and responded to successfully
 * - Responses request corresponds to the given commands response
 */
void test_http_responses_valid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	__cmock_modem_info_init_ExpectAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);

	nrf_provisioning_http_init(&dummy_cb);

	/* Command request */
	__cmock_rest_client_request_defaults_set_Ignore();

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_finished);

	/* Responses request */
	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_finished);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_GREATER_OR_EQUAL(1, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Receiving and responding to a known command
 * - To see that a command is decoded and executed successfully
 * - Encoded output corresponds to the encoded input
 */
void test_codec_finished_valid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	nrf_provisioning_codec_init(&dummy_cb);

	/* One response to 'FINISHED' */
	cdc_ctx.ipkt = cbor_cmds1_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds1_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_GREATER_OR_EQUAL_INT(1, ret);
	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps1_valid, tx_buff, sizeof(cbor_rsps1_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Receiving and responding to a key generation deletion and generation command. CMEE error caused
 *   by the deletion command is suppressed correctly.
 * - To see that a command is decoded, executed and responded to successfully
 * - Encoded output corresponds to the encoded input
 */
void test_codec_priv_keygen_valid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	/* Two responses to 'KEYGEN'
	 * 1. Delete the existing
	 * 2. Generate a new one
	 */
	cdc_ctx.ipkt = cbor_cmds2_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds2_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(513);
	__cmock_nrf_modem_at_err_ExpectAnyArgsAndReturn(513);

	char at_resp[] = "";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps2_valid, tx_buff, sizeof(cbor_rsps2_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Command to delete the existing key fails to an unexpected CMEE error which isn't filtered
 *   out. Key generation is not executed and only the failure to delete the existing key is reported
 *   back.
 * - To see that the command is decoded, executed and error is reported back successfully. If a
 *   single command fails the other commands shall not be executed.
 * - Encoded error output corresponds to the encoded input
 */
void test_codec_priv_keygen_rejected_invalid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	cdc_ctx.ipkt = cbor_cmds2_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds2_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	char at_resp[] = "";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(514);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	__cmock_nrf_modem_at_err_ExpectAnyArgsAndReturn(514);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps2_cmee514_valid, tx_buff,
					sizeof(cbor_rsps2_cmee514_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Receiving and responding to an endorsement key generation command
 * - To see that a command is decoded, executed and responded to successfully
 * - Encoded output corresponds to the encoded input
 */
void test_codec_endorsement_keygen_valid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	char at_resp[] = "";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps3_valid, tx_buff, sizeof(cbor_rsps3_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Receiving an endorsement key generation command succeeds but AT command fails
 * - To see that a command is decoded, executed and responded to - with an error - successfully
 * - Make the AT command to fail which should lead into corresponging error response
 */
void test_codec_endorsement_keygen_invalid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	char at_resp[] = "ERROR";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(-1);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0,
		memcmp(cbor_rsps3_at_failure_valid, tx_buff, sizeof(cbor_rsps3_at_failure_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Writing a configuration to flash
 * - To see that a configuration is decoded, written and responded to successfully
 * - Encoded output corresponds to the encoded input
 */
void test_codec_config_store1_valid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_HTTP_RX_BUF_SZ];
	int mm_cb_ret = 0;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	cdc_ctx.ipkt = cbor_cmds4_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds4_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	__cmock_settings_save_one_ExpectAndReturn(
		"provisioning/interval-sec", "99", sizeof("99"), 0);

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps4_valid, tx_buff, sizeof(cbor_rsps4_valid)));

	nrf_provisioning_codec_teardown();
}

/*
 * - Detect invalid CBOR payload
 * - To see that an invalid encoding is detected
 * - Receives invalid input which results to an error code
 *    1. an empty CBOR array
 *    2. no payload
 */
void test_codec_empty_array_invalid(void)
{
	struct cdc_context cdc_ctx;
	char *at_buff = NULL;

	/* Empty CBOR array */
	uint8_t cbor_empty_array[1] = { 0x80 };

	cdc_ctx.ipkt = cbor_empty_array;
	cdc_ctx.ipkt_sz = sizeof(cbor_empty_array);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	nrf_provisioning_codec_teardown();

	/* No payload */
	cdc_ctx.ipkt = NULL;
	cdc_ctx.ipkt_sz = 0;

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	nrf_provisioning_codec_teardown();
}

/*
 * - Modem is unresponsive
 * - No sense to generate a payload if the modem isn't able to send it
 * - Get an error from modem and return an error code
 */
void test_codec_modem_unresponsive_invalid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	int mm_cb_ret = -EFAULT;

	struct nrf_provisioning_mm_change dummy_cb = {
		nrf_provisioning_mm_cb_dummy, &mm_cb_ret
	};

	nrf_provisioning_codec_init(&dummy_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = NULL;
	cdc_ctx.opkt_sz = 0;

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_LESS_THAN_INT(0, ret);

	nrf_provisioning_codec_teardown();
}

/*
 * - Trigger provisioning manually before initialization
 * - Provisioning isn't allowed if the module is unitialized
 * - Call the function and check that the return code is a negative error code
 */
void test_provisioning_manual_uninitialized_invalid(void)
{
	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_LESS_THAN_INT(0, ret);
}

/*
 * - Run the init without provisioning a new certificate
 * - To see that the init goes through properly
 * - Call the init function directly. Unfortunately we need to make an assumption that the init
 *   hasn't been called in any of the previous tests. Check that the correct key-label is used.
 */
void test_provisioning_init_wo_cert_change_valid(void)
{
	bool exists;

	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_ExpectAndReturn(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
		MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists, 0);
	__cmock_modem_key_mgmt_exists_IgnoreArg_exists();

	/* No way to check that the correct handler is passed as an argument */
	__cmock_lte_lc_register_handler_ExpectAnyArgs();
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_lte_lc_init_and_connect_IgnoreAndReturn(0);

	__cmock_settings_subsys_init_ExpectAndReturn(0);
	__cmock_settings_register_ExpectAnyArgsAndReturn(0);
	__cmock_settings_load_subtree_ExpectAndReturn("provisioning", 0);

	int ret = nrf_provisioning_init(NULL, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_lte_lc_init_and_connect_StopIgnore();
}

/*
 * - Trigger provisioning manually after initialization
 * - Should succeed if the module has been initialized
 * - Call the function and check that the return code indicates success
 */
void test_provisioning_manual_initialized_valid(void)
{
	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_lte_lc_register_handler_Ignore();
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(NULL, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_lte_lc_register_handler_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();

	ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);
}

/*
 * - Call init twice to switch the callback functions
 * - Switching the callbacks on the fly should be possible
 * - Call the init function twice but with different arguments each time. It shouldn't matter if
 *   init has been called in any previous tests.
 */
void test_provisioning_init_change_cbs_valid(void)
{
	static struct nrf_provisioning_dm_change dm = {
		.cb = dummy_nrf_provisioning_device_mode_cb,
		.user_data = NULL
	};
	static struct nrf_provisioning_mm_change mm = {
		.cb = dummy_nrf_provisioning_modem_mode_cb,
		.user_data = NULL
	};

	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);

	/* No way to check that the correct handler is passed as an argument */
	__cmock_lte_lc_register_handler_Ignore();

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(NULL, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();

	ret = nrf_provisioning_init(&mm, &dm);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

static struct trigger_data {
	/* emulate trigger */
	struct k_work_delayable work;
} trigger_data;

static void provisioning_condvar_signal(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct trigger_data *ctx = CONTAINER_OF(dwork, struct trigger_data, work);

	(void)ctx;

	nrf_provisioning_trigger_manually();

	(void)k_work_schedule(dwork, K_MSEC(100));
}

/*
 * - Trigger provisioning manually after initialization
 * - Should succeed if the module has been initialized
 * - Call the function and check that the return code indicates success. There's need to emulate
 *   triggering of the service because init is called from test process' context and condition
 *   variable can be only signaled from another one.
 */
void test_provisioning_task_valid(void)
{
	struct k_work_sync sync;

	k_work_init_delayable(&trigger_data.work, provisioning_condvar_signal);

	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_key_mgmt_write_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_lte_lc_register_handler_Ignore();
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(NULL, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_lte_lc_func_mode_get_StopIgnore();
	__cmock_lte_lc_func_mode_set_StopIgnore();
	__cmock_lte_lc_register_handler_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_modem_key_mgmt_write_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_valid);

	/* Condition variable needs to be signaled from another context for the service
	 * to be able to proceed
	 */
	k_work_schedule(&trigger_data.work, K_SECONDS(1));

	__cmock_settings_load_subtree_ExpectAnyArgsAndReturn(0);

	ret = nrf_provisioning_req();

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();

	k_work_cancel_delayable_sync(&trigger_data.work, &sync);
}

/*
 * - Server responds with busy but then responds with ok.
 * - To handle the error case
 * - Request gets send twice
 */
void test_provisioning_task_server_busy_invalid(void)
{
	struct k_work_sync sync;

	k_work_init_delayable(&trigger_data.work, provisioning_condvar_signal);

	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_key_mgmt_write_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_lte_lc_register_handler_Ignore();
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(NULL, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_lte_lc_func_mode_get_StopIgnore();
	__cmock_lte_lc_func_mode_set_StopIgnore();
	__cmock_lte_lc_register_handler_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_modem_key_mgmt_write_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();
	__cmock_date_time_now_IgnoreAndReturn(0);

	__cmock_settings_load_subtree_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);
	__cmock_nrf_provisioning_at_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_attest_token_get_ReturnArrayThruPtr_buff(
		tok_att_plain, strlen(tok_att_plain) + 1);

	__cmock_rest_client_request_AddCallback(rest_client_request_server_first_busy);
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-EBUSY);
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);

	/* Condition variable needs to be signaled from another context for the service
	 * to be able to proceed
	 */
	k_work_schedule(&trigger_data.work, K_MSEC(100));

	ret = nrf_provisioning_req();

	TEST_ASSERT_EQUAL_INT(0, ret);


	k_work_cancel_delayable_sync(&trigger_data.work, &sync);

	__cmock_rest_client_request_defaults_set_StopIgnore();
	__cmock_date_time_now_StopIgnore();
}

static int time_now(int64_t *unix_time_ms, int cmock_num_calls)
{
	*unix_time_ms = (int64_t)time(NULL);
	return 0;
}

/*
 * - Get when next provisioning should be executed
 * - The client should follow the interval configured
 * - Call the function and check that the returned interval is the assumed one. The compile time
 *   configuration is to be used as we are not reading the value from flash
 */
void test_provisioning_schedule_valid(void)
{
	/* To avoid being entangled to other tests let's assume that the function has
	 * never been called earlier. If that's not the case let's just ignore the first invocation.
	 */
	__cmock_date_time_now_IgnoreAndReturn(0);
	(void)nrf_provisioning_schedule();

	__cmock_date_time_now_StopIgnore();

	__cmock_date_time_now_AddCallback(time_now);
	__cmock_date_time_now_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_schedule();

	/* Can't have exact match due to the spread factor */
	TEST_ASSERT_GREATER_OR_EQUAL(0, ret);
	TEST_ASSERT_LESS_OR_EQUAL_INT(
		CONFIG_NRF_PROVISIONING_INTERVAL_S + CONFIG_NRF_PROVISIONING_SPREAD_S, ret);
}

/*
 * - Get when next provisioning should be executed when network time is not available
 * - It's not quaranteed that network provides the time information
 * - Call the function and check that the returned interval is the assumed one. The compile time
 *   configuration is to be used as we are not reading the value from flash. Internals are
 *   configured in a way that it looks like the time information is not available.
 */
void test_provisioning_schedule_no_nw_time_valid(void)
{
	/* To avoid being entangled to other tests let's assume that the function has
	 * never been called earlier. If that's not the case let's just ignore the first invocation.
	 */
	__cmock_date_time_now_AddCallback(time_now);
	__cmock_date_time_now_IgnoreAndReturn(0);
	(void)nrf_provisioning_schedule();

	__cmock_date_time_now_StopIgnore();

	__cmock_date_time_now_ExpectAnyArgsAndReturn(-1);

	int ret = nrf_provisioning_schedule();

	/* Can't have exact match due to the spread factor */
	TEST_ASSERT_GREATER_THAN_INT(0, ret);
	TEST_ASSERT_LESS_OR_EQUAL_INT(
		CONFIG_NRF_PROVISIONING_INTERVAL_S + CONFIG_NRF_PROVISIONING_SPREAD_S, ret);
}


extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
