/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>

#include "lwm2m_client_utils/mock_lwm2m_client_utils.h"
#include "lwm2m/mock_lwm2m.h"
#include "lte_lc/mock_lte_lc.h"
#include "cloud_wrapper.h"

#define LWM2M_INTEGRATION_CLIENT_ID_LEN 20
#define PATH_LEN			5
#define LIFETIME_RID			1
#define REBOOT_RID			4
#define LIFETIME_PATH			"1/0/1"
#define REBOOT_PATH			"3/0/4"
#define ENDPOINT_NAME_EXPECTED		":urn:id:test"

static struct lwm2m_ctx client;
static char endpoint_name[sizeof(CONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX) +
			  LWM2M_INTEGRATION_CLIENT_ID_LEN] = ":urn:id:test";
static cloud_wrap_evt_handler_t wrapper_evt_handler;

static void cloud_wrap_event_handler(const struct cloud_wrap_event *const evt) {};

/* Handlers used to invoke specific events in the uut. */
static lwm2m_ctx_event_cb_t rd_client_callback;
static lwm2m_engine_execute_cb_t engine_execute_cb;
static modem_mode_cb_t modem_mode_change_cb;

/* Forward declarations. */
static int register_exec_callback_stub(const char *pathstr,
				       lwm2m_engine_execute_cb_t cb,
				       int no_of_calls);
static int init_security_callback_stub(struct lwm2m_ctx *ctx,
				       char *endpoint,
				       struct modem_mode_change *mmode,
				       int no_of_calls);

extern int unity_main(void);

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

/* Setup and teardown functions. */
void setUp(void)
{
	mock_lwm2m_client_utils_Init();
	mock_lwm2m_Init();
	mock_lte_lc_Init();

	__wrap_lwm2m_init_image_ExpectAndReturn(0);
	__wrap_lwm2m_init_firmware_ExpectAndReturn(0);
	__wrap_lwm2m_init_security_ExpectAndReturn(&client,
						   endpoint_name,
						   NULL,
						   0);
	__wrap_lwm2m_init_security_IgnoreArg_mmode();

	__wrap_lwm2m_init_security_AddCallback(&init_security_callback_stub);

	__wrap_lwm2m_engine_register_exec_callback_ExpectAndReturn(REBOOT_PATH, NULL, 0);
	__wrap_lwm2m_engine_register_exec_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_exec_callback_AddCallback(&register_exec_callback_stub);
	TEST_ASSERT_EQUAL(0, cloud_wrap_init(cloud_wrap_event_handler));
}

void tearDown(void)
{
	mock_lwm2m_client_utils_Verify();
	mock_lwm2m_Verify();
	mock_lte_lc_Verify();
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

static int register_exec_callback_stub(const char *pathstr,
				       lwm2m_engine_execute_cb_t cb,
				       int no_of_calls)
{
	ARG_UNUSED(pathstr);
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

/* Handlers used to verify that specific events are called back via the cloud wrapper API. */
static void event_handler_mode_change_offline(const struct cloud_wrap_event *const evt)
{
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_LTE_DISCONNECT_REQUEST, evt->type);
};

static void event_handler_mode_change_online(const struct cloud_wrap_event *const evt)
{
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_LTE_CONNECT_REQUEST, evt->type);
};

static void event_handler_reboot_request(const struct cloud_wrap_event *const evt)
{
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_REBOOT_REQUEST, evt->type);
}

static void event_handler_disconnect(const struct cloud_wrap_event *const evt)
{
	TEST_ASSERT_EQUAL(CLOUD_WRAP_EVT_DISCONNECTED, evt->type);
}

/* Tests */
void test_lwm2m_integration_connect(void)
{
	uint32_t current_lifetime_expected = 0;
	uint32_t new_lifetime_expected = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;

	__wrap_lwm2m_rd_client_start_AddCallback(&rd_client_set_callback_stub);

	/* After the uut has been put into state CONNECTED, the lwm2m lifetime resource is
	 * updated. This is done by getting the resource that contains the lifetime and setting it
	 * to the configured value if they do not match. This needs to be done when using
	 * bootstrapping because the boostrap server will override the default value set by the
	 * application.
	 */
	__wrap_lwm2m_engine_get_u32_ExpectAndReturn(LIFETIME_PATH, &current_lifetime_expected, 0);
	__wrap_lwm2m_engine_set_u32_ExpectAndReturn(LIFETIME_PATH, new_lifetime_expected, 0);

	__wrap_lwm2m_security_needs_bootstrap_ExpectAndReturn(0);
	__wrap_lwm2m_rd_client_start_ExpectAndReturn(&client,
						     endpoint_name,
						     0,
						     NULL,
						     NULL,
						     0);
	__wrap_lwm2m_rd_client_start_IgnoreArg_event_cb();

	TEST_ASSERT_EQUAL(cloud_wrap_connect(), 0);

	/* Trigger internal events that puts the uut in CONNECTED state. */
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE);
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE);
}

void test_lwm2m_integration_disconnect(void)
{
	test_lwm2m_integration_connect();

	__wrap_lwm2m_rd_client_stop_ExpectAndReturn(&client, NULL, false, 0);
	__wrap_lwm2m_rd_client_stop_IgnoreArg_event_cb();

	TEST_ASSERT_EQUAL(0, cloud_wrap_disconnect());
}

void test_lwm2m_integration_data_send(void)
{
	/* Populate path with random resource path references. */
	char *paths[PATH_LEN] = {
		"4/0/6",
		"4/0/7",
		"4/0/7",
		"4/0/7",
		"4/0/7",
	};

	__wrap_lwm2m_engine_send_ExpectAndReturn(&client, (const char **)paths, PATH_LEN, true, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_data_send(NULL, PATH_LEN, true, 0, paths));
}

void test_lwm2m_integration_ui_send(void)
{
	/* Populate path with random resource path references. */
	char *paths[PATH_LEN] = {
		"4/0/6",
		"4/0/7",
		"4/0/7",
		"4/0/7",
		"4/0/7",
	};

	__wrap_lwm2m_engine_send_ExpectAndReturn(&client, (const char **)paths, PATH_LEN, true, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_ui_send(NULL, PATH_LEN, true, 0, paths));
}

void test_lwm2m_integration_neighbor_cells_send(void)
{
	/* Populate path with random resource path references. */
	char *paths[PATH_LEN] = {
		"4/0/6",
		"4/0/7",
		"4/0/7",
		"4/0/7",
		"4/0/7",
	};

	__wrap_lwm2m_engine_send_ExpectAndReturn(&client, (const char **)paths, PATH_LEN, true, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_neighbor_cells_send(NULL, PATH_LEN, true, 0, paths));
}

void test_lwm2m_integration_agps_request_send(void)
{
	/* Populate path with random resource path references. */
	char *paths[PATH_LEN] = {
		"4/0/6",
		"4/0/7",
		"4/0/7",
		"4/0/7",
		"4/0/7",
	};

	__wrap_lwm2m_engine_send_ExpectAndReturn(&client, (const char **)paths, PATH_LEN, true, 0);

	TEST_ASSERT_EQUAL(0, cloud_wrap_agps_request_send(NULL, PATH_LEN, true, 0, paths));
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
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_wrap_pgps_request_send(NULL, 0, true, 0));
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
	enum lte_lc_func_mode mode_current;

	__wrap_lte_lc_func_mode_get_ExpectAndReturn(&mode_current, 0);

	wrapper_evt_handler = event_handler_mode_change_offline;
	modem_mode_change_cb(LTE_LC_FUNC_MODE_OFFLINE, NULL);
}

void test_lwm2m_integration_mode_change_online(void)
{
	enum lte_lc_func_mode mode_current;

	__wrap_lte_lc_func_mode_get_ExpectAndReturn(&mode_current, 0);

	wrapper_evt_handler = event_handler_mode_change_online;
	modem_mode_change_cb(LTE_LC_FUNC_MODE_NORMAL, NULL);
}

void test_lwm2m_integration_reboot_request(void)
{
	wrapper_evt_handler = event_handler_reboot_request;
	engine_execute_cb(0, NULL, 0);
}

void test_lwm2m_integration_bootstrap_registration_failure(void)
{
	wrapper_evt_handler = event_handler_disconnect;
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE);
}

void test_lwm2m_integration_registration_failure(void)
{
	wrapper_evt_handler = event_handler_disconnect;
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE);
}

void test_lwm2m_integration_registration_update_failure(void)
{
	wrapper_evt_handler = event_handler_disconnect;
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE);
}

void test_lwm2m_integration_deregistration_failure(void)
{
	wrapper_evt_handler = event_handler_disconnect;
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE);
}

void test_lwm2m_integration_network_error(void)
{
	wrapper_evt_handler = event_handler_disconnect;
	rd_client_callback(&client, LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR);
}

void main(void)
{
	(void)unity_main();
}
