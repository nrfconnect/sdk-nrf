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
#include <ncs_version.h>

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
#include "cmock_nrf_provisioning_internal.h"

#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_coap.h"
#include "nrf_provisioning_internal.h"
#include "net/nrf_provisioning.h"

#define JWT_DUMMY "dummy_json_web_token"
#define MFW "mfw_nrf9161_99.99.99-DUMMY"
#define RESP_CODE 10

char tok_jwt_plain[] = JWT_DUMMY;
char MFW_VER[] = MFW;
char DUMMY_ADDR[] = "dummy_address";

static const char *auth_path = "p/auth-jwt";
static const char *cmd_path = "p/cmd";
static const char *resp_path = "p/rsp";

static int modem_key_mgmt_exists_true(nrf_sec_tag_t sec_tag,
				      enum modem_key_mgmt_cred_type cred_type, bool *exists,
				      int cmock_num_calls)
{
	*exists = true;

	return 0;
}

K_SEM_DEFINE(stopped_sem, 0, 1);

static void nrf_provisioning_callback_handler(const struct nrf_provisioning_callback_data *event)
{
	switch (event->type) {
	case NRF_PROVISIONING_EVENT_START:
		printk("Provisioning started\n");
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		printk("Provisioning stopped\n");
		k_sem_give(&stopped_sem);
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
	default:
		printk("Unknown event %d\n", event->type);
		break;
	}
}

static void wait_for_stopped(void)
{
	int ret;

	ret = k_sem_take(&stopped_sem, K_SECONDS(CONFIG_NRF_PROVISIONING_INTERVAL_S * 2));
	if (ret) {
		TEST_FAIL_MESSAGE("Provisioning did not stop in time");
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

int z_impl_zsock_setsockopt(int sock, int level, int optname, const void *optval,
			    net_socklen_t optlen)
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
static struct net_sockaddr ai_addr;

int zsock_getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	addrinfo.ai_addr = &ai_addr;
	*res = &addrinfo;
	return 0;
}

void zsock_freeaddrinfo(struct zsock_addrinfo *ai)
{
}

int z_impl_zsock_connect(int sock, const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	return 0;
}

char *z_impl_net_addr_ntop(net_sa_family_t family, const void *src, char *dst, size_t size)
{
	memcpy(dst, DUMMY_ADDR, sizeof(DUMMY_ADDR));
	return "";
}

struct coap_transmission_parameters coap_transmission_params = {
	.ack_timeout = 2000,
	.coap_backoff_percent = 200,
	.max_retransmission = 2};

struct coap_transmission_parameters coap_get_transmission_parameters(void)
{
	return coap_transmission_params;
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

static int coap_client_auth_cb(struct coap_client *client, int sock,
			       const struct net_sockaddr *addr,
			       struct coap_client_request *req,
			       struct coap_transmission_parameters *params, int cmock_num_calls)
{
	char path[] = "p/auth-jwt?mver=" MFW "&cver=" NCS_VERSION_STRING;
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		TEST_ASSERT_EQUAL_STRING(path, req->path);
		TEST_ASSERT_EQUAL_STRING(JWT_DUMMY, req->payload);
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		data.result_code = COAP_RESPONSE_CODE_OK;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_auth_failed_cb(struct coap_client *client, int sock,
				      const struct net_sockaddr *addr,
				      struct coap_client_request *req,
				      struct coap_transmission_parameters *params,
				      int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_FORBIDDEN;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_auth_server_error_cb(struct coap_client *client, int sock,
					    const struct net_sockaddr *addr,
					    struct coap_client_request *req,
					    struct coap_transmission_parameters *params,
					    int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_INTERNAL_ERROR;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_auth_unsupported_code_cb(struct coap_client *client, int sock,
						const struct net_sockaddr *addr,
						struct coap_client_request *req,
						struct coap_transmission_parameters *params,
						int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = RESP_CODE;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_bad_request_cb(struct coap_client *client, int sock,
					   const struct net_sockaddr *addr,
					   struct coap_client_request *req,
					   struct coap_transmission_parameters *params,
					   int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		data.result_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_server_error_cb(struct coap_client *client, int sock,
					    const struct net_sockaddr *addr,
					    struct coap_client_request *req,
					    struct coap_transmission_parameters *params,
					    int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		data.result_code = COAP_RESPONSE_CODE_INTERNAL_ERROR;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_unsupported_code_cb(struct coap_client *client, int sock,
						const struct net_sockaddr *addr,
						struct coap_client_request *req,
						struct coap_transmission_parameters *params,
						int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		data.result_code = RESP_CODE;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_cmds_valid_path_cb(struct coap_client *client, int sock,
					  const struct net_sockaddr *addr,
					  struct coap_client_request *req,
					  struct coap_transmission_parameters *params,
					  int cmock_num_calls)
{
	char path[] = "p/cmd?after=&rxMaxSize=" STRINGIFY(CONFIG_NRF_PROVISIONING_RX_BUF_SZ)
		"&txMaxSize=" STRINGIFY(CONFIG_NRF_PROVISIONING_TX_BUF_SZ)
		"&limit=" STRINGIFY(CONFIG_NRF_PROVISIONING_CBOR_RECORDS);
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		TEST_ASSERT_EQUAL_STRING(path, req->path);
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_no_commands_cb(struct coap_client *client, int sock,
				      const struct net_sockaddr *addr,
				      struct coap_client_request *req,
				      struct coap_transmission_parameters *params,
				      int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.last_block = true;
		req->cb(&data, req->user_data);
	}

	return 0;
}

static int coap_client_rsp_bad_request_cb(struct coap_client *client, int sock,
					  const struct net_sockaddr *addr,
					  struct coap_client_request *req,
					  struct coap_transmission_parameters *params,
					  int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.payload = cbor_cmds1_valid;
		data.payload_len = sizeof(cbor_cmds1_valid);
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_rsp_server_error_cb(struct coap_client *client, int sock,
					   const struct net_sockaddr *addr,
					   struct coap_client_request *req,
					   struct coap_transmission_parameters *params,
					   int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.payload = cbor_cmds1_valid;
		data.payload_len = sizeof(cbor_cmds1_valid);
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_INTERNAL_ERROR;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_rsp_unsupported_code_cb(struct coap_client *client, int sock,
						       const struct net_sockaddr *addr,
					       struct coap_client_request *req,
					       struct coap_transmission_parameters *params,
					       int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.payload = cbor_cmds1_valid;
		data.payload_len = sizeof(cbor_cmds1_valid);
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		data.result_code = RESP_CODE;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_ok_cb(struct coap_client *client, int sock,
			     const struct net_sockaddr *addr,
			     struct coap_client_request *req,
			     struct coap_transmission_parameters *params, int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CREATED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CONTENT;
		data.payload = cbor_cmds1_valid;
		data.payload_len = sizeof(cbor_cmds1_valid);
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
		data.result_code = COAP_RESPONSE_CODE_CHANGED;
		data.last_block = true;
		req->cb(&data, req->user_data);
	} else {
		TEST_ASSERT(false);
	}

	return 0;
}

static int coap_client_commands_cb(struct coap_client *client, int sock,
				   const struct net_sockaddr *addr,
				   struct coap_client_request *req,
				   struct coap_transmission_parameters *params, int cmock_num_calls)
{
	struct coap_client_response_data data = {0};

	if (cmock_num_calls < 4) {
		if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
			data.result_code = COAP_RESPONSE_CODE_CREATED;
			data.last_block = true;
			req->cb(&data, req->user_data);
		} else if (strncmp(req->path, cmd_path, strlen(cmd_path)) == 0) {
			data.result_code = COAP_RESPONSE_CODE_CONTENT;
			data.payload = cbor_cmds2_valid;
			data.payload_len = sizeof(cbor_cmds2_valid);
			data.last_block = true;
			req->cb(&data, req->user_data);
		} else if (strncmp(req->path, resp_path, strlen(resp_path)) == 0) {
			data.result_code = COAP_RESPONSE_CODE_CHANGED;
			data.last_block = true;
			req->cb(&data, req->user_data);
		} else {
			TEST_ASSERT(false);
		}
	} else {
		if (strncmp(req->path, auth_path, strlen(auth_path)) == 0) {
			data.result_code = COAP_RESPONSE_CODE_CREATED;
			data.last_block = true;
			req->cb(&data, req->user_data);
		} else {
			data.result_code = COAP_RESPONSE_CODE_CONTENT;
			data.last_block = true;
			req->cb(&data, req->user_data);
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

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_auth_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_auth_failed_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-EPERM, ret);
}

/*
 * - Server returns 5.00 Internal server error when authenticating
 */
void test_coap_auth_server_error(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

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

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

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

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_no_commands_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENODATA, ret);
}

/*
 * - Test valid path for get commands request
 */
void test_coap_cmds_valid_path(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_cmds_valid_path_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENODATA, ret);
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

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_cmds_bad_request_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_cmds_server_error_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

	struct coap_client_option block2_option = {};

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_req_AddCallback(coap_client_cmds_unsupported_code_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

/*
 * - Trigger provisioning manually after initialization
 * - Should succeed if the module has been initialized
 * - Call the function and check that the return code indicates success.
 */
void test_provisioning_task_valid(void)
{
	struct coap_client_option block2_option = {};

	__cmock_modem_key_mgmt_exists_AddCallback(modem_key_mgmt_exists_true);
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_key_mgmt_write_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_coap_client_init_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_lte_lc_register_handler_Ignore();
	__cmock_lte_lc_func_mode_set_IgnoreAndReturn(0);
	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

	__cmock_coap_client_req_AddCallback(coap_client_ok_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_settings_save_one_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_init(nrf_provisioning_callback_handler);

	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_stopped();
}

void test_provisioning_commands(void)
{
	struct coap_client_option block2_option = {};

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
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(513);
	__cmock_nrf_modem_at_err_ExpectAnyArgsAndReturn(513);
	__cmock_nrf_modem_at_err_ExpectAnyArgsAndReturn(513);
	__cmock_nrf_modem_at_err_type_ExpectAnyArgsAndReturn(0);

	__cmock_nrf_provisioning_at_cmd_ExpectAnyArgsAndReturn(0);

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

	__cmock_coap_client_req_AddCallback(coap_client_commands_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_trigger_manually();

	TEST_ASSERT_EQUAL_INT(0, ret);

	wait_for_stopped();
}

/*
 * - Server returns 4.00 Bad request when sending response to the server
 */
void test_coap_rps_bad_request(void)
{
	struct nrf_provisioning_coap_context coap_ctx = {
		.connect_socket = -1,
	};

	struct coap_client_option block2_option = {};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

	struct coap_client_option block2_option = {};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

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

	struct coap_client_option block2_option = {};

	__cmock_lte_lc_func_mode_get_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(0);
	__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_modem_info_get_fw_version_ExpectAnyArgsAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);

	__cmock_nrf_provisioning_jwt_generate_ExpectAnyArgsAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain, sizeof(tok_jwt_plain));

	__cmock_coap_client_option_initial_block2_CMockIgnoreAndReturn(0, block2_option);

	__cmock_coap_client_req_AddCallback(coap_client_rsp_unsupported_code_cb);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);
	__cmock_coap_client_req_ExpectAnyArgsAndReturn(0);

	int ret = nrf_provisioning_coap_req(&coap_ctx);

	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret);
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
