/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
/* Define _POSIX_C_SOURCE before including <string.h> in order to use `strtok_r`. */
#define _POSIX_C_SOURCE 200809L

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncs_version.h>

#include "cmock_date_time.h"
#include "cmock_lte_lc.h"
#include "cmock_modem_attest_token.h"
#include "cmock_modem_key_mgmt.h"
#include "cmock_modem_info.h"
#include "cmock_nrf_modem_lib.h"
#include "cmock_nrf_modem_at.h"
#include "cmock_nrf_provisioning_at.h"
#include "cmock_rest_client.h"
#include "cmock_settings.h"
#include "cmock_nrf_provisioning_jwt.h"
#include "cmock_nrf_provisioning_internal.h"

#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_http.h"
#include "nrf_provisioning_internal.h"
#include "net/nrf_provisioning.h"

#define AUTH_HDR_BEARER "Authorization: Bearer "
#define AUTH_HDR_BEARER_JWT_DUMMY "dummy_json_web_token"

#define CRLF "\r\n"

#define QUERY_ITEMS_MAX 6
#define EXPECT_NO_EVENTS_TIMEOUT_SECONDS 10000

char tok_jwt_plain[] = AUTH_HDR_BEARER_JWT_DUMMY;
char http_auth_hdr[] = AUTH_HDR_BEARER AUTH_HDR_BEARER_JWT_DUMMY CRLF;
char http_auth_hdr_invalid1[] = AUTH_HDR_BEARER_JWT_DUMMY CRLF;
char http_auth_hdr_invalid2[] = "att." AUTH_HDR_BEARER_JWT_DUMMY CRLF;
char MFW_VER[] = "mfw_nrf9161_99.99.99-DUMMY";

K_SEM_DEFINE(event_received_sem, 0, 1);

static struct nrf_provisioning_callback_data event_data = { 0 };

static void nrf_provisioning_event_cb(const struct nrf_provisioning_callback_data *event)
{
	switch (event->type) {
	case NRF_PROVISIONING_EVENT_START:
		printk("Provisioning started\n");
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		printk("Provisioning stopped\n");
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		printk("Provisioning done\n");
		break;
	case NRF_PROVISIONING_EVENT_FAILED:
		printk("Provisioning failed\n");
		break;
	case NRF_PROVISIONING_EVENT_NO_COMMANDS:
		printk("No commands from server\n");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS:
		printk("Too many commands\n");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED:
		printk("Device not claimed\n");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA:
		printk("Wrong root CA\n");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME:
		printk("No valid datetime\n");
		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED:
		printk("Need LTE deactivated\n");
		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED:
		printk("Need LTE activated\n");
		break;
	case NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING:
		printk("Scheduled provisioning\n");
		break;
	case NRF_PROVISIONING_EVENT_FATAL_ERROR:
		printk("Fatal error\n");
		break;
	}

	event_data.type = event->type;

	k_sem_give(&event_received_sem);
}

static void wait_for_provisioning_event(enum nrf_provisioning_event event)
{
	k_sem_take(&event_received_sem, K_FOREVER);

	if (event_data.type != event) {
		printk("Expected event: %d, received event: %d\n", event, event_data.type);
		TEST_FAIL_MESSAGE("Unexpected provisioning event received");
	}
}

static void wait_for_no_provisioning_event(int timeout_seconds)
{
	int ret;

	ret = k_sem_take(&event_received_sem, K_SECONDS(timeout_seconds));
	if (ret == 0) {
		printk("Unexpected provisioning event received, event: %d\n", event_data.type);
		TEST_FAIL_MESSAGE("Unexpected provisioning event received");
	}
}

/* Malloc and free are to be used in native_sim environment */
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
		char *limit;
	};

	/* 'txMaxSize', 'rxMaxSize', 'mver', 'cver', limit and 'after' */
	char *query_items[QUERY_ITEMS_MAX] = {};

	struct url_info info = { .apiver = 0,
				 .mversion = NULL,
				 .endpoint = NULL,
				 .command = NULL,
				 .txMaxSize = NULL,
				 .rxMaxSize = NULL,
				 .limit = NULL };

	tokens = (char *)malloc(strlen(req_ctx->url) + 1);

	TEST_ASSERT_NOT_NULL(tokens);

	strcpy(tokens, req_ctx->url);

	char *save_tok;
	char *tok = strtok_r(tokens, "/", &save_tok);

	int sgmtc = 0; /* Path segment count */
	int qcnt = 0; /* Query item count */

	while (tok) {
		switch (sgmtc) {
		case 0:
			info.apiver = tok;
			tok = strtok_r(NULL, "/", &save_tok);
			sgmtc++;
			break;
		case 1:
			info.endpoint = tok;
			tok = strtok_r(NULL, "?", &save_tok);
			sgmtc++;
			break;
		case 2:
			info.command = tok;
			tok = strtok_r(NULL, "&", &save_tok);
			sgmtc++;
			break;
		default:
			query_items[qcnt++] = tok;
			tok = strtok_r(NULL, "&", &save_tok);
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
			TEST_ASSERT_EQUAL_STRING(MFW_VER, info.mversion);
		} else if (strncmp(query_items[idx], "cver=", strlen("cver=")) == 0) {
			info.cversion = &(query_items[idx][strlen("cver=")]);
			TEST_ASSERT_EQUAL_STRING(NCS_VERSION_STRING, info.cversion);
		} else if (strncmp(query_items[idx], "txMaxSize=", strlen("txMaxSize=")) == 0) {
			info.txMaxSize = &(query_items[idx][strlen("txMaxSize=")]);
			TEST_ASSERT_EQUAL_INT(
				CONFIG_NRF_PROVISIONING_TX_BUF_SZ, atoi(info.txMaxSize));
		} else if (strncmp(query_items[idx], "rxMaxSize=", strlen("rxMaxSize=")) == 0) {
			info.rxMaxSize = &(query_items[idx][strlen("rxMaxSize=")]);
			TEST_ASSERT_EQUAL_INT(
				CONFIG_NRF_PROVISIONING_RX_BUF_SZ, atoi(info.rxMaxSize));
		} else if (strncmp(query_items[idx], "limit=", strlen("limit=")) == 0) {
			info.limit = &(query_items[idx][strlen("limit=")]);
			TEST_ASSERT_EQUAL_INT(
				CONFIG_NRF_PROVISIONING_CBOR_RECORDS, atoi(info.limit));
		} else if (strncmp(query_items[idx], "after=", strlen("after=")) == 0) {
			;
		} else {
			TEST_ASSERT(false);
		}
	}

	free(tokens);
	return 0;
}

/*
 * - JWT token is generated and placed as one of the HTTP headers
 * - To have a valid token for authentication
 * - Retrieves a dummy JWT token and writes an authentication header based on it
 */
void test_http_commands_auth_hdr_valid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_valid);

	nrf_provisioning_http_req(&rest_ctx);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Having JWT token as authentication header with the 'att.'-prefix is detected
 * - Retrieves a dummy JWT token and writes an authentication header based on it
 */
void test_http_commands_auth_hdr_invalid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

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
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

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
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_no_content);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(-ENODATA, ret);

	__cmock_rest_client_request_defaults_set_StopIgnore();
}

/*
 * - Check if server responds with internal server error
 * - If server is experiencing issues, provisioning should return -EBUSY for retry
 * - Response status must indicate internal server error (500)
 */
void test_http_commands_internal_server_error_invalid(void)
{
	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

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
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_resp_unknown_err1);

	int ret = nrf_provisioning_http_req(&rest_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);

	/* 2nd */
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

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

	__cmock_modem_info_init_ExpectAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	nrf_provisioning_http_init(nrf_provisioning_event_cb);

	/* Command request */
	__cmock_rest_client_request_defaults_set_Ignore();

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_finished);

	/* Responses request */
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);

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
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

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
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	/* Two responses to 'KEYGEN'
	 * 1. Delete the existing
	 * 2. Generate a new one
	 */
	cdc_ctx.ipkt = cbor_cmds2_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds2_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

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
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	cdc_ctx.ipkt = cbor_cmds2_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds2_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

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
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

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
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

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
 * - Writing a configuration to flash storage
 * - To see that a configuration command is decoded, written to settings and responded to
 *   successfully
 * - Encoded output corresponds to the encoded input
 */

void test_codec_config_store1_valid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	cdc_ctx.ipkt = cbor_cmds4_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds4_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	/* CBOR data is not null terminated */
	char interval[] = {'9', '9'};

	__cmock_settings_save_one_ExpectAndReturn(
		"provisioning/interval-sec", interval, sizeof(interval), 0);

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, memcmp(cbor_rsps4_valid, tx_buff, sizeof(cbor_rsps4_valid)));

	nrf_provisioning_codec_teardown();
}
/*
 * - Detect invalid CBOR payload formats
 * - To see that invalid encoding is detected and handled gracefully
 * - Receives invalid input which results to an error code:
 *    1. an empty CBOR array
 *    2. no payload at all
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
 * - Modem is unresponsive during provisioning
 * - No sense to generate a response payload if the modem isn't able to send it
 * - Get an error from modem and return an appropriate error code
 */
void test_codec_modem_unresponsive_invalid(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	cdc_ctx.ipkt = cbor_cmds3_valid;
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = NULL;
	cdc_ctx.opkt_sz = 0;

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmd_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_LESS_THAN_INT(0, ret);

	nrf_provisioning_codec_teardown();
}

/*
 * - Trigger provisioning manually before initialization
 * - Provisioning isn't allowed if the module is uninitialized
 * - Call the function and check that the return code is a negative error code
 */
void test_provisioning_manual_uninitialized_invalid(void)
{
	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_LESS_THAN_INT(0, ret);
}

/*
 * - Run the initialization without provisioning a new certificate
 * - To see that the init goes through properly when certificate already exists
 * - Call the init function directly. Check that the correct key-label is used and no new
 *   certificate is written.
 */
void test_provisioning_init_wo_cert_change_valid(void)
{
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_settings_subsys_init_ExpectAndReturn(0);
	__cmock_settings_register_ExpectAnyArgsAndReturn(0);
	__cmock_settings_load_subtree_ExpectAndReturn("provisioning", 0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_valid);

	__cmock_lte_lc_register_handler_Ignore();

	int ret = nrf_provisioning_init(nrf_provisioning_event_cb);

	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_NO_COMMANDS);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Trigger provisioning manually after initialization
 * - Should succeed if the module has been initialized
 * - Call the function and check that the return code indicates success.
 */
void test_provisioning_task_valid(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_date_time_now_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_auth_hdr_valid);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_NO_COMMANDS);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Server responds with busy status but then responds with ok on retry
 * - To handle the error case and verify retry mechanism works correctly
 * - Request gets sent twice, first fails with busy, second succeeds
 */
void test_provisioning_task_server_busy_invalid(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_date_time_now_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);
	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_AddCallback(rest_client_request_server_first_busy);
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-EBUSY);
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);

	ret = nrf_provisioning_trigger_manually();
	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_NO_COMMANDS);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

static int rest_client_request_device_not_claimed(struct rest_client_req_context *req_ctx,
						  struct rest_client_resp_context *resp_ctx,
						  int cmock_num_calls)
{
	resp_ctx->http_status_code = 403; /* Unauthorized - device not claimed */
	return -EACCES;
}

static int rest_client_request_too_many_commands(struct rest_client_req_context *req_ctx,
						 struct rest_client_resp_context *resp_ctx,
						 int cmock_num_calls)
{
	return -ENOMEM;
}

static int rest_client_request_wrong_root_ca(struct rest_client_req_context *req_ctx,
					     struct rest_client_resp_context *resp_ctx,
					     int cmock_num_calls)
{
	return -ECONNREFUSED;
}

static int rest_client_request_timeout(struct rest_client_req_context *req_ctx,
				       struct rest_client_resp_context *resp_ctx,
				       int cmock_num_calls)
{
	return -ETIMEDOUT;
}

static int rest_client_request_with_commands(struct rest_client_req_context *req_ctx,
					     struct rest_client_resp_context *resp_ctx,
					     int cmock_num_calls)
{
	if (cmock_num_calls == 0) {
		/* First call - return commands */
		resp_ctx->response = req_ctx->resp_buff;
		memcpy(resp_ctx->response, cbor_cmds1_valid, sizeof(cbor_cmds1_valid));
		resp_ctx->response_len = sizeof(cbor_cmds1_valid);
		resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;
		return 1; /* Positive return indicates commands processed */
	}

	/* Second call - responses request */
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;
	return 0; /* Success */
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED
 * - Verify that device not claimed error triggers the correct event
 * - Mock REST client to return 403 Unauthorized to trigger device not claimed event
 */
void test_provisioning_event_failed_device_not_claimed(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);
	__cmock_modem_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_modem_attest_token_free_ExpectAnyArgs();

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-EACCES);
	__cmock_rest_client_request_AddCallback(rest_client_request_device_not_claimed);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS
 * - Verify that too many commands error triggers the correct event
 * - Mock REST client to return -ENOMEM to trigger too many commands event
 */
void test_provisioning_event_failed_too_many_commands(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-ENOMEM);
	__cmock_rest_client_request_AddCallback(rest_client_request_too_many_commands);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA
 * - Verify that wrong root CA error triggers the correct event
 * - Mock REST client to return -ECONNREFUSED to trigger wrong root CA event
 */
void test_provisioning_event_failed_wrong_root_ca(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-ECONNREFUSED);
	__cmock_rest_client_request_AddCallback(rest_client_request_wrong_root_ca);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME
 * - Verify that no valid datetime error triggers the correct event
 * - Mock date_time_is_valid to return false to trigger no valid datetime event
 */
void test_provisioning_event_failed_no_valid_datetime(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(0); /* Return false - no valid datetime */
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);


	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED with timeout
 * - Verify that timeout error triggers the correct failed event
 * - Mock REST client to return -ETIMEDOUT to trigger timeout failure event
 */
void test_provisioning_event_failed_timeout(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-ETIMEDOUT);
	__cmock_rest_client_request_AddCallback(rest_client_request_timeout);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_DONE
 * - Verify that successful provisioning with commands triggers the done event
 * - Mock REST client to return positive value indicating commands were processed
 */
void test_provisioning_event_done(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_IgnoreAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	/* Allow multiple JWT generations - one for command request, one for response */
	__cmock_nrf_provisioning_jwt_generate_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	/* Mock the codec functions for processing commands */
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	/* Set up REST client request expectations - will be called twice (commands + responses) */
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(1);
	__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
	__cmock_rest_client_request_AddCallback(rest_client_request_with_commands);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_DONE);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

/*
 * - Test NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED and NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED
 * - Verify that LTE deactivation/activation events are triggered during AT command processing
 * - Process commands that require LTE mode changes
 */
void test_provisioning_event_lte_deactivation_activation(void)
{
	struct cdc_context cdc_ctx;
	char at_buff[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	char tx_buff[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	nrf_provisioning_codec_init(nrf_provisioning_event_cb);

	/* Use AT command that requires LTE state change */
	cdc_ctx.ipkt = cbor_cmds3_valid;  /* KEYGEN command */
	cdc_ctx.ipkt_sz = sizeof(cbor_cmds3_valid);
	cdc_ctx.opkt = tx_buff;
	cdc_ctx.opkt_sz = sizeof(tx_buff);

	/* Mock LTE function mode to return NORMAL (online) to trigger deactivation */
	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	enum lte_lc_func_mode online_mode = LTE_LC_FUNC_MODE_NORMAL;

	__cmock_lte_lc_func_mode_get_ReturnThruPtr_mode(&online_mode);

	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	/* Mock the LTE deactivation event notification */
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_ExpectAndReturn(
		CONFIG_NRF_PROVISIONING_MODEM_STATE_WAIT_TIMEOUT_SECONDS,
		NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED,
		nrf_provisioning_event_cb, 0);

	/* Mock the AT command execution */
	char at_resp[] = "";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	/* Mock the LTE activation event notification */
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_ExpectAndReturn(
		CONFIG_NRF_PROVISIONING_MODEM_STATE_WAIT_TIMEOUT_SECONDS,
		NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED,
		nrf_provisioning_event_cb, 0);

	nrf_provisioning_codec_setup(&cdc_ctx, at_buff, sizeof(at_buff));

	int ret = nrf_provisioning_codec_process_commands();

	TEST_ASSERT_EQUAL_INT(0, ret);

	nrf_provisioning_codec_teardown();
}

/*
 * - Test NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED with attestation token
 * - Verify that device not claimed error with token provides the token data
 * - Mock REST client to return 403 and modem_attest_token_get to provide token
 */
void test_provisioning_event_failed_device_not_claimed_with_token(void)
{
	struct nrf_attestation_token test_token = {
		.attest = "test_attest_string",
		.attest_sz = 18,
		.cose = "test_cose_string",
		.cose_sz = 16
	};

	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	/* Mock attestation token retrieval */
	__cmock_modem_attest_token_get_ExpectAnyArgsAndReturn(0);
	__cmock_modem_attest_token_get_ReturnMemThruPtr_token(&test_token, sizeof(test_token));
	__cmock_modem_attest_token_free_ExpectAnyArgs();

	__cmock_rest_client_request_ExpectAnyArgsAndReturn(-EACCES);
	__cmock_rest_client_request_AddCallback(rest_client_request_device_not_claimed);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED);
	wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	wait_for_no_provisioning_event(EXPECT_NO_EVENTS_TIMEOUT_SECONDS);
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
