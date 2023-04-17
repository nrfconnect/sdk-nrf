/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/init.h>

#include <azure/az_core.h>

#include "azure_iot_hub_dps.h"
#include "azure_iot_hub_dps_private.h"

#include "cmock_mqtt_helper.h"
#include "cmock_settings.h"

#define TEST_DPS_HOSTNAME			CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME

#define TEST_IOT_HUB_HOSTNAME			"test-iot-hub.azure-devices.net"
#define TEST_IOT_HUB_HOSTNAME_LEN		(sizeof(TEST_IOT_HUB_HOSTNAME) - 1)

#define TEST_ID_SCOPE				CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE "-custom"
#define TEST_ID_SCOPE_LEN			(sizeof(TEST_ID_SCOPE) - 1)

#define TEST_ID_SCOPE_DEFAULT			CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE
#define TEST_ID_SCOPE_DEFAULT_LEN		(sizeof(TEST_ID_SCOPE_DEFAULT) - 1)

#define TEST_REGISTRATION_ID			CONFIG_AZURE_IOT_HUB_DPS_REG_ID "-custom"
#define TEST_REGISTRATION_ID_LEN		(sizeof(TEST_REGISTRATION_ID) - 1)

#define TEST_REGISTRATION_ID_DEFAULT		CONFIG_AZURE_IOT_HUB_DPS_REG_ID
#define TEST_REGISTRATION_ID_DEFAULT_LEN	(sizeof(TEST_REGISTRATION_ID_DEFAULT) - 1)

#define TEST_EXPECTED_DEVICE_ID			TEST_REGISTRATION_ID
#define TEST_EXPECTED_DEVICE_ID_LEN		TEST_REGISTRATION_ID_LEN

#define TEST_EXPECTED_USER_NAME			TEST_ID_SCOPE "/registrations/"			\
						TEST_EXPECTED_DEVICE_ID	"/api-version=2019-03-31"
#define TEST_EXPECTED_USER_NAME_LEN		(sizeof(TEST_EXPECTED_USER_NAME) - 1)

#define TEST_EXPECTED_USER_NAME_DEFAULT		TEST_ID_SCOPE_DEFAULT "/registrations/"		\
						TEST_REGISTRATION_ID_DEFAULT			\
						"/api-version=2019-03-31"
#define TEST_EXPECTED_USER_NAME_DEFAULT_LEN	(sizeof(TEST_EXPECTED_USER_NAME_DEFAULT) - 1)

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

/* Pull in variables and functions from the DPS library. */
extern enum dps_state dps_state;
extern struct dps_reg_ctx dps_reg_ctx;
extern char assigned_hub_buf[];
extern int dps_save_hostname(char *hostname_ptr, size_t hostname_len);
extern int dps_save_device_id(char *device_id_ptr, size_t device_id_len);
extern void on_reg_completed(az_iot_provisioning_client_register_response *msg);
extern void on_publish(struct azure_iot_hub_buf topic, struct azure_iot_hub_buf payload);
extern int provisioning_client_init(struct mqtt_helper_conn_params *conn_params);

/* Static functions */
static void az_precondition_failed_cb(void);

/* Semaphores used by tests to wait for a certain callbacks */
static K_SEM_DEFINE(reg_status_assigned_sem, 0, 1);
static K_SEM_DEFINE(reg_failed_sem, 0, 1);
static K_SEM_DEFINE(reg_assigning_sem, 0, 1);

/* Test suite configuration functions */
void setUp(void)
{
	__cmock_settings_subsys_init_IgnoreAndReturn(0);
	__cmock_settings_load_subtree_IgnoreAndReturn(0);
	__cmock_settings_save_one_IgnoreAndReturn(0);
	__cmock_settings_delete_IgnoreAndReturn(0);
	az_precondition_failed_set_callback(az_precondition_failed_cb);

	dps_state = DPS_STATE_UNINIT;
}

void tearDown(void)
{
	__cmock_mqtt_helper_disconnect_IgnoreAndReturn(0);
	azure_iot_hub_dps_reset();
}

/* Stubs */

int mqtt_helper_connect_stub(struct mqtt_helper_conn_params *conn_params, int num_calls)
{
	TEST_ASSERT_EQUAL_STRING(TEST_DPS_HOSTNAME, conn_params->hostname.ptr);
	TEST_ASSERT_EQUAL_MEMORY(conn_params->device_id.ptr, TEST_REGISTRATION_ID,
				 TEST_REGISTRATION_ID_LEN);
	TEST_ASSERT_EQUAL_MEMORY(conn_params->user_name.ptr, TEST_EXPECTED_USER_NAME,
				 TEST_EXPECTED_USER_NAME_LEN);

	return 0;
}

int mqtt_helper_connect_stub_defaults(struct mqtt_helper_conn_params *conn_params, int num_calls)
{
	TEST_ASSERT_EQUAL_STRING(TEST_DPS_HOSTNAME, conn_params->hostname.ptr);
	TEST_ASSERT_EQUAL_MEMORY(conn_params->device_id.ptr, TEST_REGISTRATION_ID_DEFAULT,
				 TEST_REGISTRATION_ID_DEFAULT_LEN);
	TEST_ASSERT_EQUAL_MEMORY(conn_params->user_name.ptr, TEST_EXPECTED_USER_NAME_DEFAULT,
				 TEST_EXPECTED_USER_NAME_DEFAULT_LEN);

	return 0;
}

/* Helper functions */

static void az_precondition_failed_cb(void)
{
	/* Empty, used to override default while-1 loop in the SDK. */
}

/* Callback used in tests. */
static void dps_handler(enum azure_iot_hub_dps_reg_status state)
{
	switch (state) {
	case AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED:
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING:
		k_sem_give(&reg_assigning_sem);
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED:
		TEST_ASSERT_EQUAL_MEMORY(az_span_ptr(dps_reg_ctx.assigned_hub),
					 TEST_IOT_HUB_HOSTNAME, TEST_IOT_HUB_HOSTNAME_LEN);
		k_sem_give(&reg_status_assigned_sem);
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_FAILED:
		k_sem_give(&reg_failed_sem);
		break;
	default:
		printk("Unknown state: %d\n", state);
		break;
	}
}

/* Tests */

void test_azure_iot_hub_dps_init_invalid_config(void)
{
	struct azure_iot_hub_dps_config config = {0};

	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_dps_init(NULL));
	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_dps_init(&config));
}

void test_azure_iot_hub_dps_init(void)
{
	struct azure_iot_hub_dps_config config = {
		.handler = dps_handler,
		.reg_id.ptr = TEST_REGISTRATION_ID,
		.reg_id.size = TEST_REGISTRATION_ID_LEN,
	};

	TEST_ASSERT_EQUAL(0, azure_iot_hub_dps_init(&config));
	TEST_ASSERT_EQUAL(DPS_STATE_DISCONNECTED, dps_state);
}

void test_azure_iot_hub_dps_hostname_get_not_registered(void)
{
	struct azure_iot_hub_buf hostname_dummy;

	TEST_ASSERT_EQUAL(-ENOENT, azure_iot_hub_dps_hostname_get(&hostname_dummy));
}

void test_azure_iot_hub_dps_hostname_get_invalid_buffer(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_dps_hostname_get(NULL));
}

void test_azure_iot_hub_dps_hostname_get_registered(void)
{
	int err;
	char hostname_buf[128];
	struct azure_iot_hub_buf hostname = {
		.ptr = hostname_buf,
		.size = sizeof(hostname_buf),
	};

	/* Set the hostname to a valid string. */
	err = dps_save_hostname(TEST_IOT_HUB_HOSTNAME, TEST_IOT_HUB_HOSTNAME_LEN);
	TEST_ASSERT_EQUAL(0, err);

	err = azure_iot_hub_dps_hostname_get(&hostname);
	TEST_ASSERT_EQUAL(0, err);

	TEST_ASSERT_EQUAL_MEMORY(hostname.ptr, TEST_IOT_HUB_HOSTNAME, hostname.size);
	TEST_ASSERT_EQUAL(TEST_IOT_HUB_HOSTNAME_LEN, hostname.size);
}

void test_azure_iot_hub_dps_device_id_get_not_registered(void)
{
	struct azure_iot_hub_buf device_id_dummy;

	TEST_ASSERT_EQUAL(-ENOENT, azure_iot_hub_dps_device_id_get(&device_id_dummy));
}

void test_azure_iot_hub_dps_device_id_get_invalid_buffer(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_dps_device_id_get(NULL));
}

void test_azure_iot_hub_dps_device_id_get_registered(void)
{
	int err;
	char device_id_buf[128];
	struct azure_iot_hub_buf device_id = {
		.ptr = device_id_buf,
		.size = sizeof(device_id_buf),
	};

	/* Set the device ID to a valid string. */
	err = dps_save_device_id(TEST_EXPECTED_DEVICE_ID, TEST_EXPECTED_DEVICE_ID_LEN);
	TEST_ASSERT_EQUAL(0, err);

	err = azure_iot_hub_dps_device_id_get(&device_id);
	TEST_ASSERT_EQUAL(0, err);

	TEST_ASSERT_EQUAL_MEMORY(device_id.ptr, TEST_EXPECTED_DEVICE_ID, device_id.size);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_DEVICE_ID_LEN, device_id.size);
}

void test_azure_iot_hub_dps_device_id_delete(void)
{
	dps_reg_ctx.assigned_device_id = az_span_create_from_str(TEST_EXPECTED_DEVICE_ID);

	TEST_ASSERT_EQUAL_MEMORY(az_span_ptr(dps_reg_ctx.assigned_device_id),
				 TEST_EXPECTED_DEVICE_ID, TEST_EXPECTED_DEVICE_ID_LEN);
	TEST_ASSERT_EQUAL(0, azure_iot_hub_dps_device_id_delete());
	TEST_ASSERT_EQUAL(NULL, az_span_ptr(dps_reg_ctx.assigned_device_id));
	TEST_ASSERT_EQUAL(0, az_span_size(dps_reg_ctx.assigned_device_id));
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED, dps_reg_ctx.status);
	TEST_ASSERT_EQUAL(DPS_STATE_UNINIT, dps_state);
}

void test_azure_iot_hub_dps_reset_connected(void)
{
	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	dps_reg_ctx.assigned_hub = az_span_create_from_str(TEST_IOT_HUB_HOSTNAME);
	dps_reg_ctx.assigned_device_id = az_span_create_from_str(TEST_EXPECTED_DEVICE_ID);
	dps_state = DPS_STATE_CONNECTED;

	TEST_ASSERT_EQUAL_MEMORY(az_span_ptr(dps_reg_ctx.assigned_hub),
				 TEST_IOT_HUB_HOSTNAME, TEST_IOT_HUB_HOSTNAME_LEN);
	TEST_ASSERT_EQUAL_MEMORY(az_span_ptr(dps_reg_ctx.assigned_device_id),
				 TEST_EXPECTED_DEVICE_ID, TEST_EXPECTED_DEVICE_ID_LEN);
	TEST_ASSERT_EQUAL(0, azure_iot_hub_dps_reset());
	TEST_ASSERT_EQUAL(NULL, az_span_ptr(dps_reg_ctx.assigned_hub));
	TEST_ASSERT_EQUAL(NULL, az_span_ptr(dps_reg_ctx.assigned_device_id));
	TEST_ASSERT_EQUAL(0, az_span_size(dps_reg_ctx.assigned_hub));
	TEST_ASSERT_EQUAL(0, az_span_size(dps_reg_ctx.assigned_device_id));
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED, dps_reg_ctx.status);
	TEST_ASSERT_EQUAL(DPS_STATE_UNINIT, dps_state);
}

void test_azure_iot_hub_dps_start_wrong_state(void)
{
	TEST_ASSERT_EQUAL(-EACCES, azure_iot_hub_dps_start());
}

void test_azure_iot_hub_dps_start_settings_not_loaded(void)
{
	dps_state = DPS_STATE_DISCONNECTED;

	/* Initialize the sempahore to zero as the code under test will try to take it. */
	k_sem_init(&dps_reg_ctx.settings_loaded, 0, 1);

	TEST_ASSERT_EQUAL(-ETIMEDOUT, azure_iot_hub_dps_start());
}

void test_azure_iot_hub_dps_start(void)
{
	int err;
	struct azure_iot_hub_dps_config config = {
		.handler = dps_handler,
		.reg_id.ptr = TEST_REGISTRATION_ID,
		.reg_id.size = TEST_REGISTRATION_ID_LEN,
		.id_scope.ptr = TEST_ID_SCOPE,
		.id_scope.size = TEST_ID_SCOPE_LEN,
	};

	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_connect_Stub(mqtt_helper_connect_stub);

	err = azure_iot_hub_dps_init(&config);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(DPS_STATE_DISCONNECTED, dps_state);

	/* Give the semaphore to simulate that settings have been loaded from flash. */
	k_sem_give(&dps_reg_ctx.settings_loaded);

	err = azure_iot_hub_dps_start();

	k_work_cancel_delayable(&dps_reg_ctx.timeout_work);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(DPS_STATE_CONNECTING, dps_state);
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED, dps_reg_ctx.status);
}

void test_azure_iot_hub_dps_start_kconfig_defaults(void)
{
	int err;
	struct azure_iot_hub_dps_config config = {
		.handler = dps_handler,
	};

	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_connect_Stub(mqtt_helper_connect_stub_defaults);

	err = azure_iot_hub_dps_init(&config);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(DPS_STATE_DISCONNECTED, dps_state);

	/* Give the semaphore to simulate that settings have been loaded from flash. */
	k_sem_give(&dps_reg_ctx.settings_loaded);

	err = azure_iot_hub_dps_start();

	k_work_cancel_delayable(&dps_reg_ctx.timeout_work);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(DPS_STATE_CONNECTING, dps_state);
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED, dps_reg_ctx.status);
}

void test_on_reg_completed_no_msg(void)
{
	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	dps_state = DPS_STATE_CONNECTED;
	dps_reg_ctx.cb = dps_handler;

	on_reg_completed(NULL);
	TEST_ASSERT_EQUAL(DPS_STATE_DISCONNECTING, dps_state);
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_FAILED, dps_reg_ctx.status);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_failed_sem, K_SECONDS(1)));
}

void test_on_publish_assigned(void)
{
	struct azure_iot_hub_buf topic = {
		.ptr = "$dps/registrations/res/200/?$rid=1",
		.size = sizeof("$dps/registrations/res/200/?$rid=1") - 1,
	};
	char payload_reg_update[] =
		"{\"operationId\":\"5.9727b80d37b9e609.9b573313-2f79-43ab-9392-4af8f9612f7a\","
		"\"status\":\"assigned\","
		"\"registrationState\":{"
			"\"registrationId\":\"" TEST_REGISTRATION_ID "\","
			"\"assignedHub\":\"" TEST_IOT_HUB_HOSTNAME "\","
			"\"deviceId\":\"" TEST_EXPECTED_DEVICE_ID "\","
			"\"status\":\"assigned\""
		"}}";
	struct azure_iot_hub_buf payload = {
		.ptr = payload_reg_update,
		.size = sizeof(payload_reg_update) - 1,
	};

	dps_reg_ctx.cb = dps_handler;

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);
	on_publish(topic, payload);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_status_assigned_sem, K_SECONDS(1)));
}

void test_on_publish_request_error(void)
{
	struct azure_iot_hub_buf topic = {
		.ptr = "$dps/registrations/res/401/?$rid=1",
		.size = sizeof("$dps/registrations/res/401/?$rid=1") - 1,
	};
	char payload_reg_update[] =
		"{\"errorCode\":404203,\"trackingId\":\"86824c47-4811-48fc-85d6-3053d4f2627a\","
		"\"message\":\"Operation not found.\"}";
	struct azure_iot_hub_buf payload = {
		.ptr = payload_reg_update,
		.size = sizeof(payload_reg_update) - 1,
	};

	dps_reg_ctx.cb = dps_handler;

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);
	on_publish(topic, payload);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_failed_sem, K_SECONDS(1)));
}

void test_on_publish_assigning(void)
{
	int err;
	struct azure_iot_hub_dps_config config = {
		.handler = dps_handler,
	};
	struct azure_iot_hub_buf topic = {
		.ptr = "$dps/registrations/res/202/?$rid=1&retry-after=1",
		.size = sizeof("$dps/registrations/res/202/?$rid=1&retry-after=1") - 1,
	};
	char payload_reg_update[] =
		"{\"operationId\":\"5.6727b80d37b9e609.9b573313-2f79-43ab-9392-4af8f9612f7a\","
		"\"status\":\"assigning\"}";
	struct azure_iot_hub_buf payload = {
		.ptr = payload_reg_update,
		.size = sizeof(payload_reg_update) - 1,
	};

	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_connect_Stub(mqtt_helper_connect_stub_defaults);
	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	err = azure_iot_hub_dps_init(&config);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(DPS_STATE_DISCONNECTED, dps_state);
	/* Give the semaphore to simulate that settings have been loaded from flash. */
	k_sem_give(&dps_reg_ctx.settings_loaded);

	err = azure_iot_hub_dps_start();
	k_work_cancel_delayable(&dps_reg_ctx.timeout_work);

	TEST_ASSERT_EQUAL(0, err);

	TEST_ASSERT_EQUAL(DPS_STATE_CONNECTING, dps_state);
	TEST_ASSERT_EQUAL(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED, dps_reg_ctx.status);

	on_publish(topic, payload);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_assigning_sem, K_SECONDS(2)));
}

void test_on_publish_invalid_topic(void)
{
	struct azure_iot_hub_buf topic = {
		.ptr = "not-a-valid-topic",
		.size = sizeof("not-a-valid-topic") - 1,
	};
	char payload_reg_update[] =
		"dummy";
	struct azure_iot_hub_buf payload = {
		.ptr = payload_reg_update,
		.size = sizeof(payload_reg_update) - 1,
	};

	dps_reg_ctx.cb = dps_handler;

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);
	on_publish(topic, payload);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_failed_sem, K_SECONDS(2)));
}

void test_on_publish_invalid_payload(void)
{
	struct azure_iot_hub_buf topic = {
		.ptr = "$dps/registrations/res/202/?$rid=1&retry-after=1",
		.size = sizeof("$dps/registrations/res/202/?$rid=1&retry-after=1") - 1,
	};
	char payload_reg_update[] =
		"dummy";
	struct azure_iot_hub_buf payload = {
		.ptr = payload_reg_update,
		.size = sizeof(payload_reg_update) - 1,
	};

	dps_reg_ctx.cb = dps_handler;

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);
	on_publish(topic, payload);
	TEST_ASSERT_EQUAL(0, k_sem_take(&reg_failed_sem, K_SECONDS(2)));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
