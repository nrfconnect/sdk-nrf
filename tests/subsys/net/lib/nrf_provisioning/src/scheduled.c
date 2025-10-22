/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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

#define EXPECT_NO_EVENTS_TIMEOUT_SECONDS 10000

char tok_jwt_plain[] = AUTH_HDR_BEARER_JWT_DUMMY;
char http_auth_hdr[] = AUTH_HDR_BEARER AUTH_HDR_BEARER_JWT_DUMMY CRLF;
char MFW_VER[] = "mfw_nrf9161_99.99.99-DUMMY";

/* [["9685ef84-8cac-4257-b5cc-94bc416a1c1d.d", 2]] */
static unsigned char cbor_cmds1_valid[] = {
	0x81, 0x82, 0x78, 0x26, 0x39, 0x36, 0x38, 0x35, 0x65,
	0x66, 0x38, 0x34, 0x2d, 0x38, 0x63, 0x61, 0x63, 0x2d,
	0x34, 0x32, 0x35, 0x37, 0x2d, 0x62, 0x35, 0x63, 0x63,
	0x2d, 0x39, 0x34, 0x62, 0x63, 0x34, 0x31, 0x36, 0x61,
	0x31, 0x63, 0x31, 0x64, 0x2e, 0x64, 0x02
};

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

	event_data = *event;

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

static int rest_client_request_no_commands_cb(struct rest_client_req_context *req_ctx,
					       struct rest_client_resp_context *resp_ctx,
					       int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_NO_CONTENT;
	return 0;
}

static int rest_client_request_too_many_commands_cb(struct rest_client_req_context *req_ctx,
						 struct rest_client_resp_context *resp_ctx,
						 int cmock_num_calls)
{
	return -ENOMEM;
}

static int rest_client_request_server_error_cb(struct rest_client_req_context *req_ctx,
						struct rest_client_resp_context *resp_ctx,
						int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_INTERNAL_SERVER_ERR;
	return 0;
}

static int rest_client_request_not_found_cb(struct rest_client_req_context *req_ctx,
					     struct rest_client_resp_context *resp_ctx,
					     int cmock_num_calls)
{
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_FORBIDDEN;
	return 0;
}

static int rest_client_request_timeout_cb(struct rest_client_req_context *req_ctx,
					   struct rest_client_resp_context *resp_ctx,
					   int cmock_num_calls)
{
	return -ETIMEDOUT;
}

static int rest_client_request_wrong_root_ca_cb(struct rest_client_req_context *req_ctx,
					     struct rest_client_resp_context *resp_ctx,
					     int cmock_num_calls)
{
	return -ECONNREFUSED;
}

static int rest_client_request_with_commands_cb(struct rest_client_req_context *req_ctx,
						struct rest_client_resp_context *resp_ctx,
						int cmock_num_calls)
{
	static int scenario_call_count;

	/* For scenario 3, we expect exactly 2 calls in sequence:
	 * First call: Get commands - should return CBOR data and return 1
	 * Second call: Upload responses - should return success and return 0
	 */
	if (scenario_call_count % 2 == 0) {
		/* First call of the pair - return commands */
		resp_ctx->response = req_ctx->resp_buff;
		memcpy(resp_ctx->response, cbor_cmds1_valid, sizeof(cbor_cmds1_valid));
		resp_ctx->response_len = sizeof(cbor_cmds1_valid);
		resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;
		scenario_call_count++;
		return 1; /* Positive return indicates commands processed */
	}

	/* Second call of the pair - upload responses, just return success */
	resp_ctx->http_status_code = NRF_PROVISIONING_HTTP_STATUS_OK;
	resp_ctx->response_len = 0; /* No response body needed */
	scenario_call_count++;
	return 0; /* Success */
}

static int time_now(int64_t *unix_time_ms, int cmock_num_calls)
{
	*unix_time_ms = (int64_t)time(NULL) * MSEC_PER_SEC;
	return 0;
}

void setUp(void)
{
	k_sem_reset(&event_received_sem);
	__cmock_date_time_now_Stub(time_now);
}

void test_provisioning_init_should_start_provisioning(void)
{
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_info_init_IgnoreAndReturn(0);
	__cmock_nrf_modem_lib_init_IgnoreAndReturn(0);
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_register_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_date_time_now_IgnoreAndReturn(0);
	__cmock_date_time_is_valid_IgnoreAndReturn(1);
	__cmock_nrf_provisioning_notify_event_and_wait_for_modem_state_IgnoreAndReturn(0);
	__cmock_rest_client_request_defaults_set_Ignore();
	__cmock_modem_info_get_fw_version_IgnoreAndReturn(0);
	__cmock_modem_info_get_fw_version_ReturnArrayThruPtr_buf(MFW_VER, sizeof(MFW_VER));

	__cmock_nrf_provisioning_jwt_generate_IgnoreAndReturn(0);
	__cmock_nrf_provisioning_jwt_generate_CMockReturnMemThruPtr_jwt_buf(
		CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S, tok_jwt_plain,
		strlen(tok_jwt_plain) + 1);

	int ret = nrf_provisioning_init(nrf_provisioning_event_cb);

	TEST_ASSERT_EQUAL_INT(0, ret);

	for (int i = 0; i < 100; i++) {
		/* Wait for scheduled provisioning event */
		wait_for_provisioning_event(NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING);

		/* Verify next attempt time is within expected range */
		TEST_ASSERT_LESS_OR_EQUAL_INT(
			(CONFIG_NRF_PROVISIONING_INTERVAL_S + CONFIG_NRF_PROVISIONING_SPREAD_S),
			event_data.next_attempt_time_seconds);

		/* Wait for the scheduled time (no events should occur during this period) */
		wait_for_no_provisioning_event(event_data.next_attempt_time_seconds);

		/* Provisioning should start */
		wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);

		/* Test different scenarios in rotation */
		int scenario = i % 6;

		switch (scenario) {
		case 0: /* No commands from server */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_no_commands_cb);
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_NO_COMMANDS);
			break;

		case 1: /* Device not found/claimed */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_modem_attest_token_get_ExpectAnyArgsAndReturn(0);
			__cmock_modem_attest_token_free_ExpectAnyArgs();
			__cmock_rest_client_request_AddCallback(
				rest_client_request_not_found_cb);
			wait_for_provisioning_event(
				NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED);
			break;

		case 2: /* Too many commands */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_too_many_commands_cb);
			wait_for_provisioning_event(
				NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS);
			break;

		case 3: /* Successful provisioning with commands */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(1);
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_with_commands_cb);

			__cmock_nrf_provisioning_at_cmee_enable_ExpectAndReturn(true);
			__cmock_nrf_provisioning_at_cmee_control_ExpectAnyArgsAndReturn(0);

			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_DONE);
			break;

		case 4: /* Wrong root CA */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_wrong_root_ca_cb);
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA);
			break;

		case 5: /* Timeout */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(-ETIMEDOUT);
			__cmock_rest_client_request_AddCallback(rest_client_request_timeout_cb);

			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED);
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);

			/* Retry provisioning after timeout */
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);

			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_too_many_commands_cb);
			wait_for_provisioning_event(
				NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS);
			break;

		case 6: /* Server error busy */
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(-EBUSY);
			__cmock_rest_client_request_AddCallback(
				rest_client_request_server_error_cb);

			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_FAILED);
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);

			/* Retry provisioning after server error */
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_START);
			__cmock_rest_client_request_ExpectAnyArgsAndReturn(0);
			__cmock_rest_client_request_AddCallback(rest_client_request_no_commands_cb);
			wait_for_provisioning_event(NRF_PROVISIONING_EVENT_NO_COMMANDS);
			break;
		}

		/* If adding more cases here, increase the scenario count, 'scenario' */

		/* Provisioning should stop after each attempt */
		wait_for_provisioning_event(NRF_PROVISIONING_EVENT_STOP);
	}
}

/*
 * - Get when next provisioning should be executed
 * - The client should follow the interval configured at compile time
 * - Call the function and check that the returned interval is within the expected range.
 *   The compile time configuration is used as we are not reading the value from flash
 *   storage
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
 * - It's not guaranteed that network provides the time information
 * - Call the function and check that the returned interval is within the expected range.
 *   The compile time configuration is used as we are not reading the value from flash.
 *   Internals are configured in a way that it looks like the time information is not
 *   available.
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
