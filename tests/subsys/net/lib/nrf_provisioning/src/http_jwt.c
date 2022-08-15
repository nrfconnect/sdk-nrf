/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unity.h"
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>

#include "nrf_provisioning_http.h"

#include "cmock_rest_client.h"
#include "cmock_modem_info.h"
#include "cmock_nrf_provisioning_at.h"
#include "cmock_modem_jwt.h"

#define AUTH_HDR_BEARER_PREFIX_JWT "Authorization: Bearer "
#define AUTH_HDR_BEARER_JWT_DUMMY "dummy_json_web_token"
#define CRLF "\r\n"

static char ok_jwt_fake[] = AUTH_HDR_BEARER_JWT_DUMMY;
static char ok_jwt_http_hdr_fake[] = AUTH_HDR_BEARER_PREFIX_JWT AUTH_HDR_BEARER_JWT_DUMMY CRLF;

void *__wrap_k_malloc(size_t size)
{
	return malloc(size);
}

void __wrap_k_free(void *ptr)
{
	free(ptr);
}

int custom_nrf_modem_at_cmd(void *buf, size_t len, const char *fmt, va_list argp)
{
	return 0;
}

static int modem_jwt_generate_fake(struct jwt_data *const jwt, int cmock_num_calls)
{
	memcpy(jwt->jwt_buf, ok_jwt_fake, strlen(ok_jwt_fake) + 1);

	return 0;
}

static int rest_client_request_auth_hdr_valid(struct rest_client_req_context *req_ctx,
				    struct rest_client_resp_context *resp_ctx, int cmock_num_calls)
{
	int cnt = 0;

	(void)cmock_num_calls;

	for (int i = 0; req_ctx->header_fields && req_ctx->header_fields[i]; i++) {
		if (strcmp(ok_jwt_http_hdr_fake, req_ctx->header_fields[i]) == 0) {
			cnt++;
		}
	}

	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;

	TEST_ASSERT_EQUAL_INT(1, cnt);

	return 0;
}

void setUp(void)
{
}

void tearDown(void)
{
}

/*
 * - JWT is generated and placed as one of the HTTP headers
 * - To have a valid token for authentication
 * - Retrieves a dummy token and writes an authentication header based on it
 */
void test_http_commands_auth_hdr_valid(void)
{
	int ret;
	char mfw_version[] = "mfw_nrf9160_99.99.99-DUMMY";

	struct nrf_provisioning_http_context rest_ctx = {
		.connect_socket = REST_CLIENT_SCKT_CONNECT,
	};

	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(mfw_version));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(mfw_version, strlen(mfw_version) + 1);

	__cmock_modem_jwt_generate_Stub(modem_jwt_generate_fake);

	__cmock_nrf_provisioning_at_time_get_ExpectAnyArgsAndReturn(0);

	__cmock_rest_client_request_Stub(rest_client_request_auth_hdr_valid);

	ret = nrf_provisioning_http_req(&rest_ctx);
}

extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
