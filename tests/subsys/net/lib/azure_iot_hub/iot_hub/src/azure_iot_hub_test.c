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

#include <net/azure_iot_hub.h>

#include "cmock_mqtt_helper.h"

#define TEST_DEVICE_ID			"run-time-test-device-id"
#define TEST_DEVICE_ID_LEN		(sizeof(TEST_DEVICE_ID) - 1)

#define TEST_HOSTNAME			"run-time-test-hostname.test-server.com"
#define TEST_HOSTNAME_LEN		(sizeof(TEST_HOSTNAME) - 1)

#define	TEST_REQUEST_ID			"6549877"
#define	TEST_REQUEST_ID_LEN		(sizeof(TEST_REQUEST_ID) - 1)

#define EXPECTED_USER_NAME_RUN_TIME	TEST_HOSTNAME "/"					\
					TEST_DEVICE_ID						\
					"/?api-version=2020-09-30"

#define EXPECTED_USER_NAME_KCONFIG	CONFIG_AZURE_IOT_HUB_HOSTNAME "/"			\
					CONFIG_AZURE_IOT_HUB_DEVICE_ID				\
					"/?api-version=2020-09-30"

#define TEST_EXPECTED_TOPIC_EVENTS	"devices/" CONFIG_AZURE_IOT_HUB_DEVICE_ID		\
					"/messages/events/"
#define TEST_EXPECTED_TOPIC_EVENTS_LEN	(sizeof(TEST_EXPECTED_TOPIC_EVENTS) - 1)

#define TEST_EXPECTED_TOPIC_TWIN_GET	"$iothub/twin/GET/?$rid=" TEST_REQUEST_ID
#define TEST_EXPECTED_TOPIC_TWIN_GET_LEN	(sizeof(TEST_EXPECTED_TOPIC_TWIN_GET) - 1)

#define TEST_EXPECTED_TOPIC_TWIN_REPORTED	"$iothub/twin/PATCH/properties/reported/?$rid=" \
						TEST_REQUEST_ID
#define TEST_EXPECTED_TOPIC_TWIN_REPORTED_LEN	(sizeof(TEST_EXPECTED_TOPIC_TWIN_REPORTED) - 1)

#define TEST_EXPECTED_TOPIC_METHOD_RESPOND	"$iothub/methods/res/200/?$rid=" TEST_REQUEST_ID
#define TEST_EXPECTED_TOPIC_METHOD_RESPOND_LEN	(sizeof(TEST_EXPECTED_TOPIC_METHOD_RESPOND) - 1)

#define TEST_MSG			"This is a test message from the device to the cloud: "	\
					"1-2-testing-1-2"
#define TEST_MSG_LEN			(sizeof(TEST_MSG) - 1)

#define TEST_UPDATE_REPORTED_MSG	"{ \"doorLockState\": \"unlocked\"}"
#define TEST_UPDATE_REPORTED_MSG_LEN	(sizeof(TEST_UPDATE_REPORTED_MSG) - 1)

#define TEST_MSG_PROP_1_KEY		"property_key_1"
#define TEST_MSG_PROP_1_VALUE		"property_value_1"
#define TEST_MSG_PROP_2_KEY		"property_key_2"
#define TEST_MSG_PROP_2_VALUE		"property_value_2"
#define TEST_MSG_PROP_3_KEY		"very_much_a_quite_long_message_property_key"
#define TEST_MSG_PROP_3_VALUE		"and_this_is_a_fairly_long_message_property_value"

#define TEST_EXPECTED_TOPIC_1_PROP	TEST_EXPECTED_TOPIC_EVENTS TEST_MSG_PROP_1_KEY "="	\
					TEST_MSG_PROP_1_VALUE
#define TEST_EXPECTED_TOPIC_1_PROP_LEN	(sizeof(TEST_EXPECTED_TOPIC_1_PROP) - 1)

#define TEST_EXPECTED_TOPIC_2_PROPS	TEST_EXPECTED_TOPIC_1_PROP "&"				\
					TEST_MSG_PROP_2_KEY "=" TEST_MSG_PROP_2_VALUE
#define TEST_EXPECTED_TOPIC_2_PROPS_LEN	(sizeof(TEST_EXPECTED_TOPIC_2_PROPS) - 1)

/* Copy of the Azure IoT Hub internal states definitions. */
enum iot_hub_state {
	STATE_UNINIT,
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_TRANSPORT_CONNECTED,
	STATE_CONNECTED,
	STATE_DISCONNECTING,

	STATE_COUNT,
};

/* Pull in variables and functions from the Azure IoT Hub library. */
/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);
extern void iot_hub_state_set(enum iot_hub_state state);
extern enum iot_hub_state iot_hub_state;

void setUp(void)
{
	iot_hub_state = STATE_UNINIT;
}

/* Stubs */

static int mqtt_helper_connect_run_time_stub(struct mqtt_helper_conn_params *conn_params,
					     int num_calls)
{
	/* Verify that the incoming connection parameters are as expected when run-time provided
	 * values are used.
	 */
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_HOSTNAME,
				     conn_params->hostname.ptr,
				     TEST_HOSTNAME_LEN);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_DEVICE_ID,
				     conn_params->device_id.ptr,
				    TEST_DEVICE_ID_LEN);
	TEST_ASSERT_EQUAL_STRING_LEN(EXPECTED_USER_NAME_RUN_TIME,
				     conn_params->user_name.ptr,
				     sizeof(EXPECTED_USER_NAME_RUN_TIME) - 1);

	return 0;
}

static int mqtt_helper_connect_kconfig_stub(struct mqtt_helper_conn_params *conn_params,
					     int num_calls)
{
	/* Verify that the incoming connection parameters are as expected when Kconfig values are
	 * used.
	 */
	TEST_ASSERT_EQUAL_STRING_LEN(CONFIG_AZURE_IOT_HUB_HOSTNAME,
				     conn_params->hostname.ptr,
				     sizeof(CONFIG_AZURE_IOT_HUB_HOSTNAME) - 1);
	TEST_ASSERT_EQUAL_STRING_LEN(CONFIG_AZURE_IOT_HUB_DEVICE_ID,
				     conn_params->device_id.ptr,
				    sizeof(EXPECTED_USER_NAME_KCONFIG) - 1);
	TEST_ASSERT_EQUAL_STRING_LEN(EXPECTED_USER_NAME_KCONFIG,
				     conn_params->user_name.ptr,
				     sizeof(EXPECTED_USER_NAME_KCONFIG) - 1);

	return 0;
}

static int mqtt_helper_publish_stub(const struct mqtt_publish_param *param, int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_EVENTS_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_EVENTS, param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_MSG, param->message.payload.data,
				     param->message.payload.len);

	return 0;
}

static int mqtt_helper_publish_fail_stub(const struct mqtt_publish_param *param, int num_calls)
{
	return -1;
}

static int mqtt_helper_publish_1_prop_stub(const struct mqtt_publish_param *param, int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_1_PROP_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_1_PROP, param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_MSG, param->message.payload.data,
				     param->message.payload.len);

	return 0;
}

static int mqtt_helper_publish_2_props_stub(const struct mqtt_publish_param *param, int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_2_PROPS_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_2_PROPS, param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_MSG, param->message.payload.data,
				     param->message.payload.len);

	return 0;
}

static int mqtt_helper_publish_twin_request_stub(const struct mqtt_publish_param *param,
						 int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_TWIN_GET_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_TWIN_GET, param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL(0, param->message.payload.len);

	return 0;
}

static int mqtt_helper_publish_twin_update_reported_stub(const struct mqtt_publish_param *param,
							 int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_TWIN_REPORTED_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_TWIN_REPORTED,
				     param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL(TEST_UPDATE_REPORTED_MSG_LEN, param->message.payload.len);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_UPDATE_REPORTED_MSG, param->message.payload.data,
				     param->message.payload.len);

	return 0;
}

static int mqtt_helper_publish_method_respond_stub(const struct mqtt_publish_param *param,
						   int num_calls)
{
	TEST_ASSERT_EQUAL(0, param->dup_flag);
	TEST_ASSERT_EQUAL(0, param->retain_flag);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_TOPIC_METHOD_RESPOND_LEN, param->message.topic.topic.size);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_EXPECTED_TOPIC_METHOD_RESPOND,
				     param->message.topic.topic.utf8,
				     param->message.topic.topic.size);
	TEST_ASSERT_EQUAL(TEST_MSG_LEN, param->message.payload.len);
	TEST_ASSERT_EQUAL_STRING_LEN(TEST_MSG, param->message.payload.data,
				     param->message.payload.len);

	return 0;
}


/* Event handler used in tests. */
void evt_handler(struct azure_iot_hub_evt *evt)
{
	/* Nothing to do yet. */
}

void test_azure_iot_hub_init_valid_handler(void)
{
	TEST_ASSERT_EQUAL(0, azure_iot_hub_init(evt_handler));
	TEST_ASSERT_EQUAL(STATE_DISCONNECTED, iot_hub_state);
}

void test_azure_iot_hub_init_when_disconnected(void)
{
	iot_hub_state = STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_init(evt_handler));
}

void test_azure_iot_hub_init_when_connected(void)
{
	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EALREADY, azure_iot_hub_init(NULL));
}

void test_azure_iot_hub_init_invalid_handler(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_init(NULL));
}

void test_azure_iot_hub_connect_runtime_config_values(void)
{
	struct azure_iot_hub_config config = {
		.device_id = {
			.ptr = TEST_DEVICE_ID,
			.size = TEST_DEVICE_ID_LEN,
		},
		.hostname = {
			.ptr = TEST_HOSTNAME,
			.size = TEST_HOSTNAME_LEN,
		},
	};

	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_connect_Stub(mqtt_helper_connect_run_time_stub);

	iot_hub_state = STATE_DISCONNECTED;

	/* Verification of connection parameters passed to mqtt_helper_connect()
	 * os done in mqtt_helper_connect_run_time_stub.
	 */

	TEST_ASSERT_EQUAL(0, azure_iot_hub_connect(&config));
	TEST_ASSERT_EQUAL(STATE_CONNECTING, iot_hub_state);
}

void test_azure_iot_hub_connect_kconfig_values(void)
{
	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_connect_Stub(mqtt_helper_connect_kconfig_stub);

	iot_hub_state = STATE_DISCONNECTED;

	/* Verification of connection parameters passed to mqtt_helper_connect()
	 * os done in mqtt_helper_connect_run_time_stub.
	 */

	TEST_ASSERT_EQUAL(0, azure_iot_hub_connect(NULL));
	TEST_ASSERT_EQUAL(STATE_CONNECTING, iot_hub_state);
}

void test_azure_iot_hub_connect_when_not_initialized(void)
{
	TEST_ASSERT_EQUAL(-ENOENT, azure_iot_hub_connect(NULL));
}

void test_azure_iot_hub_connect_already_connected(void)
{
	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EALREADY, azure_iot_hub_connect(NULL));
}

void test_azure_iot_hub_connect_not_disconnected(void)
{
	iot_hub_state = STATE_UNINIT;

	TEST_ASSERT_EQUAL(-ENOENT, azure_iot_hub_connect(NULL));
	TEST_ASSERT_EQUAL(STATE_UNINIT, iot_hub_state);
}

void test_azure_iot_hub_disconnect(void)
{
	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_disconnect());
	TEST_ASSERT_EQUAL(STATE_DISCONNECTING, iot_hub_state);
}

void test_azure_iot_hub_disconnect_not_connected(void)
{
	TEST_ASSERT_EQUAL(-ENOTCONN, azure_iot_hub_disconnect());
}

void test_azure_iot_hub_send(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
	};

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_stub);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_1_property(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.properties = &(struct azure_iot_hub_property) {
				.key = {
					.ptr = TEST_MSG_PROP_1_KEY,
					.size = (sizeof(TEST_MSG_PROP_1_KEY) - 1),
				},
				.value = {
					.ptr = TEST_MSG_PROP_1_VALUE,
					.size = (sizeof(TEST_MSG_PROP_1_VALUE) - 1),
				},
		},
		.topic.property_count = 1,
	};

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_1_prop_stub);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_2_properties(void)
{
	struct azure_iot_hub_property properties[] = {
		{
			.key = {
				.ptr = TEST_MSG_PROP_1_KEY,
				.size = (sizeof(TEST_MSG_PROP_1_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_1_VALUE,
				.size = (sizeof(TEST_MSG_PROP_1_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_2_KEY,
				.size = (sizeof(TEST_MSG_PROP_2_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_2_VALUE,
				.size = (sizeof(TEST_MSG_PROP_2_VALUE) - 1),
			},
		},
	};
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.properties = properties,
		.topic.property_count = ARRAY_SIZE(properties),
	};

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_2_props_stub);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_too_long_topic(void)
{
	struct azure_iot_hub_property properties[] = {
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
		{
			.key = {
				.ptr = TEST_MSG_PROP_3_KEY,
				.size = (sizeof(TEST_MSG_PROP_3_KEY) - 1),
			},
			.value = {
				.ptr = TEST_MSG_PROP_3_VALUE,
				.size = (sizeof(TEST_MSG_PROP_3_VALUE) - 1),
			},
		},
	};
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.properties = properties,
		.topic.property_count = ARRAY_SIZE(properties),
	};

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EMSGSIZE, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_twin_get_request(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST,
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = TEST_REQUEST_ID_LEN,
		},
	};

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_twin_request_stub);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_twin_update_reported(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
		.payload = {
			.ptr = TEST_UPDATE_REPORTED_MSG,
			.size = TEST_UPDATE_REPORTED_MSG_LEN,
		},
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = TEST_REQUEST_ID_LEN,
		},
	};

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_twin_update_reported_stub);

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_invalid_topic_type(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_UNKNOWN + 10,
	};

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-ENOMSG, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_not_connected(void)
{
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
		.payload = {
			.ptr = TEST_UPDATE_REPORTED_MSG,
			.size = TEST_UPDATE_REPORTED_MSG_LEN,
		},
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = TEST_REQUEST_ID_LEN,
		},
	};

	TEST_ASSERT_EQUAL(-ENOTCONN, azure_iot_hub_send(&msg));
}

void test_azure_iot_hub_send_invalid_msg(void)
{
	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_send(NULL));
}

void test_azure_iot_hub_method_respond(void)
{
	struct azure_iot_hub_result result = {
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = TEST_REQUEST_ID_LEN,
		},
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.status = 200,
	};

	iot_hub_state = STATE_CONNECTED;

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_method_respond_stub);

	TEST_ASSERT_EQUAL(0, azure_iot_hub_method_respond(&result));
}

void test_azure_iot_hub_method_respond_not_connected(void)
{
	struct azure_iot_hub_result result_dummy;

	TEST_ASSERT_EQUAL(-ENOTCONN, azure_iot_hub_method_respond(&result_dummy));
}

void test_azure_iot_hub_method_respond_too_long_request_id(void)
{
	struct azure_iot_hub_result result = {
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = 2000,
		},
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.status = 200,
	};

	iot_hub_state = STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EFAULT, azure_iot_hub_method_respond(&result));
}

void test_azure_iot_hub_method_respond_null_pointer(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, azure_iot_hub_method_respond(NULL));
}

void test_azure_iot_hub_method_respond_mqtt_fail(void)
{
	struct azure_iot_hub_result result = {
		.request_id = {
			.ptr = TEST_REQUEST_ID,
			.size = TEST_REQUEST_ID_LEN,
		},
		.payload = {
			.ptr = TEST_MSG,
			.size = TEST_MSG_LEN,
		},
		.status = 200,
	};

	iot_hub_state = STATE_CONNECTED;

	__cmock_mqtt_helper_publish_Stub(mqtt_helper_publish_fail_stub);

	TEST_ASSERT_EQUAL(-ENXIO, azure_iot_hub_method_respond(&result));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
