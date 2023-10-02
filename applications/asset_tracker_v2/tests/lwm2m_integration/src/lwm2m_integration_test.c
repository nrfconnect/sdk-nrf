/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <string.h>

#include "lwm2m_client_utils/cmock_lwm2m_client_utils.h"
#include "lwm2m_client_utils/cmock_lwm2m_client_utils_location.h"
#include "lwm2m/cmock_lwm2m.h"
#include "lte_lc/cmock_lte_lc.h"
#include "cloud_wrapper.h"

#define LWM2M_INTEGRATION_CLIENT_ID_LEN 20
#define PATH_LEN			5
#define LIFETIME_RID			1
#define REBOOT_RID			4
#define ENDPOINT_NAME_EXPECTED		":urn:id:test"

static struct lwm2m_ctx client;
static char endpoint_name[sizeof(CONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX) +
			  LWM2M_INTEGRATION_CLIENT_ID_LEN] = ":urn:id:test";

/* Structure used to check the last cloud wrap API event callback type. */
static enum cloud_wrap_event_type last_cb_type;

static void cloud_wrap_event_handler(const struct cloud_wrap_event *const evt)
{
	last_cb_type = evt->type;
};

/* Handlers used to invoke specific events in the uut. */
static lwm2m_ctx_event_cb_t rd_client_callback;
static lwm2m_engine_execute_cb_t engine_execute_cb;
static modem_mode_cb_t modem_mode_change_cb;
static lwm2m_firmware_event_cb_t firmware_update_event_cb;

/* Forward declarations. */
static int register_exec_callback_stub(const struct lwm2m_obj_path *path,
				       lwm2m_engine_execute_cb_t cb,
				       int no_of_calls);
static int init_security_callback_stub(struct lwm2m_ctx *ctx,
				       char *endpoint,
				       struct modem_mode_change *mmode,
				       int no_of_calls);
static int init_firmware_cb_stub(lwm2m_firmware_event_cb_t cb,
					   int no_of_calls);

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

/* Setup and teardown functions. */
void setUp(void)
{
	__cmock_lwm2m_init_image_ExpectAndReturn(0);
	__cmock_lwm2m_init_firmware_cb_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_init_firmware_cb_AddCallback(&init_firmware_cb_stub);
	__cmock_lwm2m_init_security_ExpectAndReturn(&client,
						   endpoint_name,
						   NULL,
						   0);
	__cmock_lwm2m_init_security_IgnoreArg_mmode();

	__cmock_lwm2m_init_security_AddCallback(&init_security_callback_stub);

	__cmock_lwm2m_register_exec_callback_ExpectAndReturn(&LWM2M_OBJ(3, 0, 4), NULL, 0);
	__cmock_lwm2m_register_exec_callback_IgnoreArg_cb();

	__cmock_lwm2m_register_exec_callback_AddCallback(&register_exec_callback_stub);

	TEST_ASSERT_EQUAL(0, cloud_wrap_init(cloud_wrap_event_handler));
}

/* Callbacks stubs that latches events handlers in mocked libraries so that they can be triggered
 * from the test.
 */
static int rd_client_set_callback_stub(struct lwm2m_ctx *client_ctx,
				       const char *ep_name,
				       uint32_t flags,
				       lwm2m_ctx_event_cb_t event_cb,
				       lwm2m_observe_cb_t observe_cb,
				       int no_of_calls)
{
	ARG_UNUSED(client_ctx);
	ARG_UNUSED(ep_name);
	ARG_UNUSED(flags);
	ARG_UNUSED(observe_cb);
	ARG_UNUSED(no_of_calls);

	rd_client_callback = event_cb;
	return 0;
}

static int register_exec_callback_stub(const struct lwm2m_obj_path *path,
				       lwm2m_engine_execute_cb_t cb,
				       int no_of_calls)
{
	ARG_UNUSED(path);
	ARG_UNUSED(no_of_calls);

	engine_execute_cb = cb;
	return 0;
}

static int init_security_callback_stub(struct lwm2m_ctx *ctx,
				       char *endpoint,
				       struct modem_mode_change *mmode,
				       int no_of_calls)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(endpoint);
	ARG_UNUSED(no_of_calls);

	modem_mode_change_cb = mmode->cb;
	return 0;
}

int init_firmware_cb_stub(lwm2m_firmware_event_cb_t cb, int no_of_calls)
{
	ARG_UNUSED(no_of_calls);

	firmware_update_event_cb = cb;
	return 0;
}

/* Tests */
void test_lwm2m_integration_connect(void)
{
	uint32_t current_lifetime_expected = 0;
	uint32_t new_lifetime_expected = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;

	__cmock_lwm2m_rd_client_start_AddCallback(&rd_client_set_callback_stub);

	/* After the uut has been put into state CONNECTED, the lwm2m lifetime resource is
	 * updated. This is done by getting the resource that contains the lifetime and setting it
	 * to the configured value if they do not match. This needs to be done when using
	 * bootstrapping because the boostrap server will override the default value set by the
	 * application.
	 */
	__cmock_lwm2m_get_u32_ExpectAndReturn(&LWM2M_OBJ(1, 0, 1), &current_lifetime_expected, 0);
	__cmock_lwm2m_set_u32_ExpectAndReturn(&LWM2M_OBJ(1, 0, 1), new_lifetime_expected, 0);

	__cmock_lwm2m_security_needs_bootstrap_ExpectAndReturn(0);
	__cmock_lwm2m_rd_client_start_ExpectAndReturn(&client,
						     endpoint_name,
						     0,
						     NULL,
						     NULL,
						     0);
	__cmock_lwm2m_rd_client_start_IgnoreArg_event_cb();

	TEST_ASSERT_EQUAL(cloud_wrap_connect(), 0);

	/* Trigger internal events that puts the uut in CONNECTED state. */
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE);
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE);
}

void test_lwm2m_integration_disconnect(void)
{
	test_lwm2m_integration_connect();

	__cmock_lwm2m_rd_client_stop_ExpectAndReturn(&client, NULL, false, 0);
	__cmock_lwm2m_rd_client_stop_IgnoreArg_event_cb();

	TEST_ASSERT_EQUAL(0, cloud_wrap_disconnect());
}

void test_lwm2m_integration_data_send(void)
{
	/* Populate path with random resource path references. */
	struct lwm2m_obj_path paths[] = {
		LWM2M_OBJ(4, 0, 6),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
	};

	__cmock_lwm2m_send_cb_ExpectAndReturn(&client, paths, PATH_LEN, NULL, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_data_send(NULL, PATH_LEN, true, 0, paths));
}

void test_lwm2m_integration_ui_send(void)
{
	/* Populate path with random resource path references. */
	struct lwm2m_obj_path paths[] = {
		LWM2M_OBJ(4, 0, 6),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
		LWM2M_OBJ(4, 0, 7),
	};

	__cmock_lwm2m_send_cb_ExpectAndReturn(&client, paths, PATH_LEN, NULL, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_ui_send(NULL, PATH_LEN, true, 0, paths));
}

void test_lwm2m_integration_neighbor_cells_send(void)
{
	__cmock_location_assistance_ground_fix_request_send_ExpectAndReturn(&client, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_cloud_location_send(NULL, 0, true, 0));
}

void test_lwm2m_integration_agnss_request_send(void)
{
	__cmock_location_assistance_agnss_request_send_ExpectAndReturn(&client, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_agnss_request_send(NULL, 0, true, 0));
}

/* Tests for APIs that are not supported by the lwm2m integration layer (uut), lwm2m_integration.c.
 * These APIs are present in the cloud_wrapper.h header file but provide no functionality in the
 * context of the application's lwm2m configuration.
 */
void test_lwm2m_integration_state_get(void)
{
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_wrap_state_get(true, 0));
}

void test_lwm2m_integration_state_send(void)
{
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_wrap_state_send(NULL, 0, true, 0));
}

void test_lwm2m_integration_batch_send(void)
{
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_wrap_batch_send(NULL, 0, true, 0));
}

void test_lwm2m_integration_pgps_request_send(void)
{
	__cmock_location_assistance_pgps_request_send_ExpectAndReturn(&client, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_pgps_request_send(NULL, 0, true, 0));
}

void test_lwm2m_integration_memfault_data_send(void)
{
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_wrap_memfault_data_send(NULL, 0, true, 0));
}

/* Tests that checks if the correct events are propagated through the event handler passed into
 * cloud_wrap_init(cloud_wrap_event_handler)).
 */
void test_lwm2m_integration_mode_change_offline(void)
{
	/* Simulate a condition where the lte_lc_func_mode_get returns LTE_LC_FUNC_MODE_NORMAL.*/
	enum lte_lc_func_mode mode_current = LTE_LC_FUNC_MODE_NORMAL;

	__cmock_lte_lc_func_mode_get_ExpectAndReturn(NULL, 0);
	__cmock_lte_lc_func_mode_get_IgnoreArg_mode();
	__cmock_lte_lc_func_mode_get_ReturnMemThruPtr_mode(&mode_current, sizeof(mode_current));

	/* Trigger a change to LTE_LC_FUNC_MODE_OFFLINE. */
	modem_mode_change_cb(LTE_LC_FUNC_MODE_OFFLINE, NULL);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_LTE_DISCONNECT_REQUEST, last_cb_type);
}

void test_lwm2m_integration_mode_change_online(void)
{
	/* Simulate a condition where the lte_lc_func_mode_get returns LTE_LC_FUNC_MODE_OFFLINE.*/
	enum lte_lc_func_mode mode_current = LTE_LC_FUNC_MODE_OFFLINE;

	__cmock_lte_lc_func_mode_get_ExpectAndReturn(NULL, 0);
	__cmock_lte_lc_func_mode_get_IgnoreArg_mode();
	__cmock_lte_lc_func_mode_get_ReturnMemThruPtr_mode(&mode_current, sizeof(mode_current));

	/* Trigger a change to LTE_LC_FUNC_MODE_NORMAL. */
	modem_mode_change_cb(LTE_LC_FUNC_MODE_NORMAL, NULL);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_LTE_CONNECT_REQUEST, last_cb_type);
}

void test_lwm2m_integration_reboot_request(void)
{
	engine_execute_cb(0, NULL, 0);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_REBOOT_REQUEST, last_cb_type);
}

void test_lwm2m_integration_bootstrap_registration_failure(void)
{
	test_lwm2m_integration_connect();

	__cmock_lwm2m_rd_client_stop_ExpectAnyArgsAndReturn(0);

	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_DISCONNECTED, last_cb_type);
}

void test_lwm2m_integration_registration_failure(void)
{
	test_lwm2m_integration_connect();

	__cmock_lwm2m_rd_client_stop_ExpectAnyArgsAndReturn(0);

	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_DISCONNECTED, last_cb_type);
}

void test_lwm2m_integration_registration_update_failure(void)
{
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_CONNECTING, last_cb_type);
}

void test_lwm2m_integration_registration_update_success(void)
{
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_CONNECTING, last_cb_type);

	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_CONNECTED, last_cb_type);
}

void test_lwm2m_integration_deregistration_failure(void)
{
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_ERROR, last_cb_type);
}

void test_lwm2m_integration_network_error(void)
{
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_ERROR, last_cb_type);
}

void test_lwm2m_integration_fota_error(void)
{
	struct lwm2m_fota_event event = {
		.id = LWM2M_FOTA_UPDATE_ERROR,
		.failure.obj_inst_id = 0,
		.failure.update_failure = 10
	};

	/* Expect an error event to be returned if FOTA state reverts to STATE_IDLE. */
	firmware_update_event_cb(&event);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_FOTA_ERROR, last_cb_type);
}

void test_lwm2m_integration_fota_downloading(void)
{
	struct lwm2m_fota_event event = {
		.id = LWM2M_FOTA_DOWNLOAD_START,
		.download_start.obj_inst_id = 0
	};

	firmware_update_event_cb(&event);
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_FOTA_START, last_cb_type);
}

void test_lwm2m_integration_fota_downloaded(void)
{
	struct lwm2m_fota_event event = {
		.id = LWM2M_FOTA_DOWNLOAD_FINISHED,
		.download_ready.obj_inst_id = 0
	};

	/* Expect no event to be called by setting last_cb_type to UINT8_MAX and verifying that
	 * the value has not changed after the state change.
	 */
	last_cb_type = UINT8_MAX;

	firmware_update_event_cb(&event);
	TEST_ASSERT_EQUAL(UINT8_MAX, last_cb_type);
}

void test_lwm2m_integration_fota_update_image_req(void)
{
	struct lwm2m_fota_event event = {
		.id = LWM2M_FOTA_UPDATE_IMAGE_REQ,
		.update_req.obj_inst_id = 0
	};

	last_cb_type = UINT8_MAX;

	firmware_update_event_cb(&event);
	TEST_ASSERT_EQUAL(UINT8_MAX, last_cb_type);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
