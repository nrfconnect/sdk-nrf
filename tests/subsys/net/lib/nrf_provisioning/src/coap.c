/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>
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
#include "cmock_coap_client.h"
#include "cmock_settings.h"
#include "cmock_nrf_provisioning_jwt.h"

#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_coap.h"
#include "nrf_provisioning_internal.h"
#include "net/nrf_provisioning.h"

#define JWT_DUMMY "dummy_json_web_token"
#define VER_NMB	  "99.99.99"
#define RESP_CODE 10

char tok_jwt_plain[] = JWT_DUMMY;

char MFW_VER[] = "mfw_nrf9161_99.99.99-DUMMY";
char MFW_VER_NMB[] = VER_NMB;
char DUMMY_ADDR[] = "dummy_address";

static const char *auth_path = "p/auth-jwt";
static const char *cmd_path = "p/cmd";
static const char *resp_path = "p/rsp";

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

static int modem_key_mgmt_exists_true(nrf_sec_tag_t sec_tag,
				      enum modem_key_mgmt_cred_type cred_type, bool *exists,
				      int cmock_num_calls)
{
	*exists = true;

	return 0;
}

static int modem_key_mgmt_exists_false(nrf_sec_tag_t sec_tag,
				       enum modem_key_mgmt_cred_type cred_type, bool *exists,
				       int cmock_num_calls)
{
	*exists = false;

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

int z_impl_zsock_setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen)
{
	return 0;
}

int z_impl_zsock_socket(int family, int type, int proto)
{
	return 0;
}

int z_impl_zsock_close(int sock)
{
	return 0;
}

static struct zsock_addrinfo addrinfo;
static struct sockaddr ai_addr;

int zsock_getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	addrinfo.ai_addr = &ai_addr;
	*res = &addrinfo;
	return 0;
}

int z_impl_zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return 0;
}

char *z_impl_net_addr_ntop(sa_family_t family, const void *src, char *dst, size_t size)
{
	memcpy(dst, DUMMY_ADDR, sizeof(DUMMY_ADDR));
	return "";
}

void setUp(void)
{
}

void tearDown(void)
{
}

/* [["9685ef84-8cac-4257-b5cc-94bc416a1c1d.d", 2]] */
static unsigned char cbor_cmds1_valid[] = {
	0x81, 0x82, 0x78, 0x26, 0x39, 0x36, 0x38, 0x35, 0x65, 0x66, 0x38, 0x34, 0x2d, 0x38, 0x63,
	0x61, 0x63, 0x2d, 0x34, 0x32, 0x35, 0x37, 0x2d, 0x62, 0x35, 0x63, 0x63, 0x2d, 0x39, 0x34,
	0x62, 0x63, 0x34, 0x31, 0x36, 0x61, 0x31, 0x63, 0x31, 0x64, 0x2e, 0x64, 0x02};

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
	0x36, 0x38, 0x34, 0x32, 0x37, 0x35, 0x33, 0x2c, 0x32, 0x2c, 0x31, 0x80};

static int coap_client_auth_cb(struct coap_client *client, int sock, const struct sockaddr *addr,
			       struct coap_client_request *req, int retries, int cmock_num_calls)
{
	char path[] = "p/auth-jwt?mver=" VER_NMB "&cver=1";

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		TEST_ASSERT_EQUAL_STRING(path, req->path);
		TEST_ASSERT_EQUAL_STRING(JWT_DUMMY, req->payload);
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		req->cb(COAP_RESPONSE_CODE_OK, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_auth_failed_cb(struct coap_client *client, int sock,
				      const struct sockaddr *addr, struct coap_client_request *req,
				      int retries, int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_FORBIDDEN, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_auth_server_error_cb(struct coap_client *client, int sock,
					    const struct sockaddr *addr,
					    struct coap_client_request *req, int retries,
					    int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_INTERNAL_ERROR, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_auth_unsupported_code_cb(struct coap_client *client, int sock,
						const struct sockaddr *addr,
						struct coap_client_request *req, int retries,
						int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(RESP_CODE, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_bad_request_cb(struct coap_client *client, int sock,
					   const struct sockaddr *addr,
					   struct coap_client_request *req, int retries,
					   int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		req->cb(COAP_RESPONSE_CODE_BAD_REQUEST, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_server_error_cb(struct coap_client *client, int sock,
					    const struct sockaddr *addr,
					    struct coap_client_request *req, int retries,
					    int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		req->cb(COAP_RESPONSE_CODE_INTERNAL_ERROR, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_unsupported_code_cb(struct coap_client *client, int sock,
						const struct sockaddr *addr,
						struct coap_client_request *req, int retries,
						int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		req->cb(RESP_CODE, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_valid_path_cb(struct coap_client *client, int sock,
					  const struct sockaddr *addr,
					  struct coap_client_request *req, int retries,
					  int cmock_num_calls)
{
	char path[] = "p/cmd?after=&rxMaxSize=" STRINGIFY(CONFIG_NRF_PROVISIONING_RX_BUF_SZ)
		"&txMaxSize=" STRINGIFY(CONFIG_NRF_PROVISIONING_TX_BUF_SZ);

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		TEST_ASSERT_EQUAL_STRING(path, req->path);
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_no_commands_cb(struct coap_client *client, int sock,
				      const struct sockaddr *addr, struct coap_client_request *req,
				      int retries, int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else {
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, NULL, 0, true, req->user_data);
	}

	return 0;
}

static int coap_client_rsp_bad_request_cb(struct coap_client *client, int sock,
					  const struct sockaddr *addr,
					  struct coap_client_request *req, int retries,
					  int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, cbor_cmds1_valid, sizeof(cbor_cmds1_valid),
			true, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_BAD_REQUEST, 0, NULL, 0, true, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_rsp_server_error_cb(struct coap_client *client, int sock,
					   const struct sockaddr *addr,
					   struct coap_client_request *req, int retries,
					   int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, cbor_cmds1_valid, sizeof(cbor_cmds1_valid),
			true, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_INTERNAL_ERROR, 0, NULL, 0, true, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_rsp_unsupported_code_cb(struct coap_client *client, int sock,
					       const struct sockaddr *addr,
					       struct coap_client_request *req, int retries,
					       int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, cbor_cmds1_valid, sizeof(cbor_cmds1_valid),
			true, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		req->cb(RESP_CODE, 0, NULL, 0, true, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_ok_cb(struct coap_client *client, int sock, const struct sockaddr *addr,
			     struct coap_client_request *req, int retries, int cmock_num_calls)
{
	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CONTENT, 0, cbor_cmds1_valid, sizeof(cbor_cmds1_valid),
			true, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		req->cb(COAP_RESPONSE_CODE_CHANGED, 0, NULL, 0, true, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_commands_cb(struct coap_client *client, int sock,
				   const struct sockaddr *addr, struct coap_client_request *req,
				   int retries, int cmock_num_calls)
{
	if (cmock_num_calls < 4) {
		if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
			req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
		} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
			req->cb(COAP_RESPONSE_CODE_CONTENT, 0, cbor_cmds2_valid,
				sizeof(cbor_cmds2_valid), true, req->user_data);
		} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
			req->cb(COAP_RESPONSE_CODE_CHANGED, 0, NULL, 0, true, req->user_data);
		} else {
			TEST_ASSERT(false);
		}
	} else {
		if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
			req->cb(COAP_RESPONSE_CODE_CREATED, 0, NULL, 0, true, req->user_data);
		} else {
			req->cb(COAP_RESPONSE_CODE_CONTENT, 0, NULL, 0, true, req->user_data);
		}
	}

	return 0;
}

/*
 * - JWT token is generated and placed to coap request payload
 * - To have a valid token for authentication
 * - Retrieves a dummy JWT token and generates coap request based on it
 */
void test_coap_auth_valid(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_auth_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	nrf_provisioning_coap_req(&coap_ctx);
}

/*
 * - Server returns 4.03 Forbidden (Authentication failed)
 */
void test_coap_auth_failed(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_auth_failed_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EACCES, ret);
}

/*
 * - Server returns 5.00 Internal server error when authenticating
 */
void test_coap_auth_server_error(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_auth_server_error_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EBUSY, ret);
}

/*
 * - Server returns unsupported response code when authenticating
 */
void test_coap_auth_unsupported_code(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_auth_unsupported_code_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);
}

/*
 * - Server returns 2.05 Content with empty payload when requesting provisioning
 * commands. No new commands available.
 */
void test_coap_no_more_commands(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_no_commands_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(0, ret);
}

/*
 * - Test valid path for get commands request
 */
void test_coap_cmds_valid_path(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_cmds_valid_path_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(0, ret);
}

/*
 * - Server returns 4.00 Bad request when requesting provisioning
 * commands
 */
void test_coap_cmds_bad_request(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_cmds_bad_request_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);
}

/*
 * - Server returns 5.00 Server error when requesting provisioning
 * commands
 */
void test_coap_cmds_server_error(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_cmds_server_error_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EBUSY, ret);
}

/*
 * - Server returns unsupported response code when requesting provisioning
 * commands
 */
void test_coap_cmds_unsupported_code(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_cmds_unsupported_code_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);
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

static int lte_lc_func_mode_get_offline_cb(enum lte_lc_func_mode *mode, int cmock_num_calls)
{
	*mode = LTE_LC_FUNC_MODE_OFFLINE;
	return 0;
}

/*
 * - Call init twice to switch the callback functions
 * - Switching the callbacks on the fly should be possible
 * - Call the init function twice but with different arguments each time. It shouldn't matter if
 *   init has been called in any previous tests.
 */
void test_provisioning_init_change_cbs_valid(void)
{
	static struct nrf_provisioning_dm_change dm = {.cb = dummy_nrf_provisioning_device_mode_cb,
						       .user_data = NULL};
	static struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						       .user_data = NULL};

	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_coap_client_init_IgnoreAndReturn(0);
	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_false);
	__cmock_modem_key_mgmt_exists_ExpectAnyArgsAndReturn(0);
	__cmock_modem_key_mgmt_write_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_AddCallback(lte_lc_func_mode_get_offline_cb);
	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_lte_lc_func_mode_get_ExpectAnyArgsAndReturn(0);
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
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
	static struct nrf_provisioning_dm_change dm = {.cb = dummy_nrf_provisioning_device_mode_cb,
						       .user_data = NULL};
	static struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						       .user_data = NULL};

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
	__cmock_nrf_provisioning_at_cmee_enable_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_IgnoreAndReturn(0);

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(&mm, &dm);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_lte_lc_register_handler_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_modem_key_mgmt_write_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_ok_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	/* Condition variable needs to be signaled from another context for the service
	 * to be able to proceed
	 */
	k_work_schedule(&trigger_data.work, K_SECONDS(1));

	__cmock_settings_load_subtree_ExpectAnyArgsAndReturn(0);
	__cmock_settings_save_one_ExpectAnyArgsAndReturn(0);

	ret = nrf_provisioning_req();

	TEST_ASSERT_EQUAL_INT(1, ret);

	k_work_cancel_delayable_sync(&trigger_data.work, &sync);
}

void test_provisioning_commands(void)
{
	struct k_work_sync sync;
	static struct nrf_provisioning_dm_change dm = {.cb = dummy_nrf_provisioning_device_mode_cb,
						       .user_data = NULL};
	static struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						       .user_data = NULL};

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
	__cmock_nrf_provisioning_at_cmee_enable_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_IgnoreAndReturn(0);

	/* To make certain init has been called at least once beforehand */
	int ret = nrf_provisioning_init(&mm, &dm);

	TEST_ASSERT_EQUAL_INT(0, ret);

	__cmock_lte_lc_register_handler_StopIgnore();
	__cmock_modem_info_init_StopIgnore();
	__cmock_nrf_modem_lib_init_StopIgnore();
	__cmock_settings_load_subtree_StopIgnore();
	__cmock_settings_register_StopIgnore();
	__cmock_settings_subsys_init_StopIgnore();
	__cmock_modem_key_mgmt_write_StopIgnore();
	__cmock_modem_key_mgmt_exists_StopIgnore();
	__cmock_date_time_now_IgnoreAndReturn(0);

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(513);
	__cmock_nrf_modem_at_err_ExpectAnyArgsAndReturn(513);

	char at_resp[] = "";

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_at_cmd_ReturnArrayThruPtr_resp(at_resp, sizeof(at_resp));

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_commands_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	/* Condition variable needs to be signaled from another context for the service
	 * to be able to proceed
	 */
	k_work_schedule(&trigger_data.work, K_SECONDS(1));

	__cmock_settings_load_subtree_ExpectAnyArgsAndReturn(0);

	ret = nrf_provisioning_req();

	TEST_ASSERT_EQUAL_INT(0, ret);

	k_work_cancel_delayable_sync(&trigger_data.work, &sync);
}

/*
 * - Server returns 4.00 Bad request when sending response to the server
 */
void test_coap_rps_bad_request(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};
	struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						.user_data = NULL};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	nrf_provisioning_coap_init(&mm);

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_rsp_bad_request_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);
}

/*
 * - Server returns 5.00 Server error when requesting provisioning
 * commands
 */
void test_coap_rsp_server_error(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};
	struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						.user_data = NULL};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	nrf_provisioning_coap_init(&mm);

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_rsp_server_error_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EBUSY, ret);
}

/*
 * - Server returns unsupported response code when requesting provisioning
 * commands
 */
void test_coap_rsp_unsupported_code(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};
	struct nrf_provisioning_mm_change mm = {.cb = dummy_nrf_provisioning_modem_mode_cb,
						.user_data = NULL};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	nrf_provisioning_coap_init(&mm);

	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);
	__cmock_modem_info_string_get_ExpectAnyArgsAndReturn(sizeof(MFW_VER));
	__cmock_modem_info_string_get_ReturnArrayThruPtr_buf(MFW_VER, strlen(MFW_VER) + 1);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	__cmock_coap_client_req_AddCallback(coap_client_rsp_unsupported_code_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);
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

int main(void)
{
	(void)unity_main();

	return 0;
}
