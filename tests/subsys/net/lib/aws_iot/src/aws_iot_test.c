/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <net/aws_iot.h>

#include "cmock_mqtt_helper.h"
#include "cmock_aws_fota.h"

/* MQTT related parameters */
#define MQTT_RESULT_SUCCESS 0
#define MQTT_RESULT_FAIL 1
#define MQTT_SESSION_VALID 1
#define MQTT_SESSION_INVALID 0
#define MQTT_MESSAGE_ID 2469
#define MQTT_PAYLOAD "payload"
#define MQTT_HOSTNAME "abracadabra"
#define MQTT_CLIENT_ID "agraba"

#define AWS_SUBS_MESSAGE_ID 1984
#define APP_SUBS_MESSAGE_ID 2469

#define SHADOW_TOPIC_GET_ACCEPTED "$aws/things/" MQTT_CLIENT_ID "/shadow/get/accepted"
#define SHADOW_TOPIC_GET_REJECTED "$aws/things/" MQTT_CLIENT_ID "/shadow/get/rejected"
#define SHADOW_TOPIC_UPDATE_DELTA "$aws/things/" MQTT_CLIENT_ID "/shadow/update/delta"
#define SHADOW_TOPIC_UPDATE_ACCEPTED "$aws/things/" MQTT_CLIENT_ID "/shadow/update/accepted"
#define SHADOW_TOPIC_UPDATE_REJECTED "$aws/things/" MQTT_CLIENT_ID "/shadow/update/rejected"
#define SHADOW_TOPIC_DELETE_ACCEPTED "$aws/things/" MQTT_CLIENT_ID "/shadow/delete/accepted"
#define SHADOW_TOPIC_DELETE_REJECTED "$aws/things/" MQTT_CLIENT_ID "/shadow/delete/rejected"

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

static struct aws_iot_data tx_data = {
	.qos = MQTT_QOS_1_AT_LEAST_ONCE,
	.topic.type = AWS_IOT_SHADOW_TOPIC_GET,
	.topic.str = "app-specific-topic",
	.topic.len = sizeof("app-specific-topic") - 1,
	.message_id = MQTT_MESSAGE_ID,
	.ptr = MQTT_PAYLOAD,
	.len = sizeof(MQTT_PAYLOAD) - 1,
	.dup_flag = true,
	.retain_flag = true
};

/* Local handlers used in stubs to invoke events in the UUT. */
aws_fota_callback_t test_aws_fota_handler;
static struct mqtt_helper_cfg test_mqtt_helper_cfg;

/* Variables used to expect some value being set by the UUT. */
static enum aws_iot_evt_type event_type_expected;
static enum aws_iot_shadow_topic_received_type topic_type_expected;
static char *topic_expected = "";
static char *hostname_expected = "";
static char *client_id_expected = "";

/* Forward declarations */
static void event_handler(const struct aws_iot_evt *const evt)
{
	TEST_ASSERT_EQUAL(evt->type, event_type_expected);
}

static void event_handler_topic_type_check(const struct aws_iot_evt *const evt)
{
	TEST_ASSERT_EQUAL(evt->type, AWS_IOT_EVT_DATA_RECEIVED);
	TEST_ASSERT_EQUAL(evt->data.msg.topic.type_received, topic_type_expected);
}

/* Stubs used to register test local handlers used to invoke events in the UUT. */
static int mqtt_helper_init_stub(struct mqtt_helper_cfg *cfg, int num_of_calls)
{
	test_mqtt_helper_cfg = *cfg;
	return 0;
}

static int aws_fota_init_stub(aws_fota_callback_t evt_handler, int num_of_calls)
{
	test_aws_fota_handler = evt_handler;
	return 0;
}

/* Convenience function used to stub internal callbacks prior to running a testcase. */
static void mqtt_helper_handlers_register(aws_iot_evt_handler_t handler)
{
	__cmock_mqtt_helper_init_Stub(&mqtt_helper_init_stub);
	__cmock_aws_fota_init_Stub(&aws_fota_init_stub);

	TEST_ASSERT_EQUAL(0, aws_iot_init(handler));
}

int mqtt_helper_connect_stub(struct mqtt_helper_conn_params *conn_params, int num_of_calls)
{
	TEST_ASSERT_EQUAL_STRING(client_id_expected, conn_params->device_id.ptr);
	TEST_ASSERT_EQUAL(strlen(client_id_expected), conn_params->device_id.size);

	TEST_ASSERT_EQUAL_STRING(hostname_expected, conn_params->hostname.ptr);
	TEST_ASSERT_EQUAL(strlen(hostname_expected), conn_params->hostname.size);

	TEST_ASSERT_NULL(conn_params->user_name.ptr);
	TEST_ASSERT_EQUAL(0, conn_params->user_name.size);

	return 0;
}

int mqtt_helper_publish_stub(const struct mqtt_publish_param *param, int num_of_calls)
{
	TEST_ASSERT_EQUAL(MQTT_QOS_1_AT_LEAST_ONCE, param->message.topic.qos);
	TEST_ASSERT_EQUAL(MQTT_MESSAGE_ID, param->message_id);
	TEST_ASSERT_TRUE(param->dup_flag);
	TEST_ASSERT_TRUE(param->retain_flag);

	TEST_ASSERT_EQUAL_STRING(MQTT_PAYLOAD, param->message.payload.data);
	TEST_ASSERT_EQUAL(strlen(MQTT_PAYLOAD), param->message.payload.len);

	TEST_ASSERT_EQUAL_STRING(topic_expected, param->message.topic.topic.utf8);
	TEST_ASSERT_EQUAL(strlen(topic_expected), param->message.topic.topic.size);

	return 0;
}

/* Verify aws_iot_init() */

void test_init_should_return_error_when_handler_is_null(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_init(NULL));
}

void test_init_should_return_error_when_mqtt_helper_init_fails(void)
{
	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(-EOPNOTSUPP);
	__cmock_aws_fota_init_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, aws_iot_init(event_handler));
}

void test_init_should_return_success(void)
{
	__cmock_mqtt_helper_init_ExpectAnyArgsAndReturn(0);
	__cmock_aws_fota_init_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(0, aws_iot_init(event_handler));
}

/* Verify aws_iot_connect() */

void test_connect_should_use_static_connection_parameters(void)
{
	mqtt_helper_handlers_register(event_handler);

	event_type_expected = AWS_IOT_EVT_CONNECTED;
	hostname_expected = CONFIG_AWS_IOT_BROKER_HOST_NAME;
	client_id_expected = CONFIG_AWS_IOT_CLIENT_ID_STATIC;

	__cmock_mqtt_helper_connect_Stub(&mqtt_helper_connect_stub);
	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_SUCCESS, MQTT_SESSION_VALID);

	TEST_ASSERT_EQUAL(0, aws_iot_connect(NULL));
}

void test_connect_should_use_run_time_parameters(void)
{
	mqtt_helper_handlers_register(event_handler);

	struct aws_iot_config config = {
		.host_name = MQTT_HOSTNAME,
		.client_id = MQTT_CLIENT_ID
	};

	event_type_expected = AWS_IOT_EVT_CONNECTED;
	hostname_expected = config.host_name;
	client_id_expected = config.client_id;

	__cmock_mqtt_helper_connect_Stub(&mqtt_helper_connect_stub);
	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_SUCCESS, MQTT_SESSION_VALID);

	TEST_ASSERT_EQUAL(0, aws_iot_connect(&config));
}

void test_connect_should_return_error_if_mqtt_helper_connect_fails(void)
{
	__cmock_mqtt_helper_connect_ExpectAnyArgsAndReturn(-EINVAL);
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_connect(NULL));
}

void test_connect_should_return_success(void)
{
	mqtt_helper_handlers_register(event_handler);

	event_type_expected = AWS_IOT_EVT_CONNECTED;

	__cmock_mqtt_helper_connect_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_SUCCESS, MQTT_SESSION_VALID);

	TEST_ASSERT_EQUAL(0, aws_iot_connect(NULL));
}

/* Verify aws_iot_disconnect() */

void test_disconnect_should_return_error_if_mqtt_helper_disconnect_fails(void)
{
	__cmock_mqtt_helper_disconnect_ExpectAndReturn(-EINVAL);
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_disconnect());
}

void test_disconnect_should_return_success(void)
{
	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);
	TEST_ASSERT_EQUAL(0, aws_iot_disconnect());
}

/* Verify aws_iot_send() */

void test_send_should_return_error_on_invalid_params(void)
{
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_send(NULL));
}

void test_send_should_publish_to_shadow_get_topic(void)
{
	/* Connect is a pre-requisite before publish.
	 * This is needed in order to construct the shadow topics.
	 */
	test_connect_should_use_run_time_parameters();

	tx_data.topic.type = AWS_IOT_SHADOW_TOPIC_GET;

	topic_expected = "$aws/things/" MQTT_CLIENT_ID "/shadow/get";

	__cmock_mqtt_helper_publish_Stub(&mqtt_helper_publish_stub);

	TEST_ASSERT_EQUAL(0, aws_iot_send(&tx_data));
}

void test_send_should_publish_to_shadow_update_topic(void)
{
	test_connect_should_use_run_time_parameters();

	tx_data.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE;

	topic_expected = "$aws/things/" MQTT_CLIENT_ID "/shadow/update";

	__cmock_mqtt_helper_publish_Stub(&mqtt_helper_publish_stub);

	TEST_ASSERT_EQUAL(0, aws_iot_send(&tx_data));
}

void test_send_should_publish_to_shadow_delete_topic(void)
{
	test_connect_should_use_run_time_parameters();

	tx_data.topic.type = AWS_IOT_SHADOW_TOPIC_DELETE;

	topic_expected = "$aws/things/" MQTT_CLIENT_ID "/shadow/delete";

	__cmock_mqtt_helper_publish_Stub(&mqtt_helper_publish_stub);

	TEST_ASSERT_EQUAL(0, aws_iot_send(&tx_data));
}

void test_send_should_publish_to_shadow_custom_topic(void)
{
	test_connect_should_use_run_time_parameters();

	tx_data.topic.type = AWS_IOT_SHADOW_TOPIC_NONE;

	topic_expected = "app-specific-topic";

	__cmock_mqtt_helper_publish_Stub(&mqtt_helper_publish_stub);
	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(0, aws_iot_send(&tx_data));
}

/* Verify aws_iot_application_topics_set() */
void test_application_topics_set_should_return_error_on_invalid_params(void)
{
	static const struct mqtt_topic topic_list[] = {
		{
			.topic.utf8 = "test-topic",
			.topic.size = strlen("test-topic"),
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		}
	};

	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_application_topics_set(NULL, 1));
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_application_topics_set(topic_list, 0));
	TEST_ASSERT_EQUAL(-EINVAL, aws_iot_application_topics_set(topic_list, 9));
}

void test_application_topics_set_should_return_success(void)
{
	static const struct mqtt_topic topic_list[] = {
		{
			.topic.utf8 = "test-topic",
			.topic.size = strlen("test-topic"),
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		}
	};

	TEST_ASSERT_EQUAL(0, aws_iot_application_topics_set(topic_list, ARRAY_SIZE(topic_list)));
}

/* Verify internal MQTT helper callback handlers. */

void test_on_connack_should_notify_error_on_failure(void)
{
	mqtt_helper_handlers_register(event_handler);

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	event_type_expected = AWS_IOT_EVT_ERROR;

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_FAIL, MQTT_SESSION_INVALID);

}

void test_on_connack_should_notify_connected_without_subscribing(void)
{
	mqtt_helper_handlers_register(event_handler);

	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	event_type_expected = AWS_IOT_EVT_CONNECTED;

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_SUCCESS, MQTT_SESSION_VALID);
}

void test_on_connack_should_notify_connected_with_subscribing(void)
{
	mqtt_helper_handlers_register(event_handler);

	__cmock_mqtt_helper_subscribe_ExpectAnyArgsAndReturn(0);
	__cmock_mqtt_helper_subscribe_ExpectAnyArgsAndReturn(0);

	event_type_expected = AWS_IOT_EVT_CONNECTED;

	test_mqtt_helper_cfg.cb.on_connack(MQTT_RESULT_SUCCESS, MQTT_SESSION_INVALID);
}

void test_on_disconnect_should_notify_disconnect(void)
{
	mqtt_helper_handlers_register(event_handler);

	event_type_expected = AWS_IOT_EVT_DISCONNECTED;

	test_mqtt_helper_cfg.cb.on_disconnect(0);
}

void test_on_puback_should_notify_puback(void)
{
	mqtt_helper_handlers_register(event_handler);

	event_type_expected = AWS_IOT_EVT_PUBACK;

	test_mqtt_helper_cfg.cb.on_puback(0, 0);
}

void test_on_suback_should_notify_error_on_fail(void)
{
	mqtt_helper_handlers_register(event_handler);

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	event_type_expected = AWS_IOT_EVT_ERROR;

	test_mqtt_helper_cfg.cb.on_suback(0, -1);
}

void test_on_suback_should_notify_error_on_unknown_messagge_id(void)
{
	test_on_connack_should_notify_connected_with_subscribing();

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	event_type_expected = AWS_IOT_EVT_ERROR;

	test_mqtt_helper_cfg.cb.on_suback(APP_SUBS_MESSAGE_ID + 1, 0);
}

void test_on_suback_should_notify_connected_on_acked_subscription_lists(void)
{
	test_on_connack_should_notify_connected_with_subscribing();

	__cmock_mqtt_helper_publish_ExpectAnyArgsAndReturn(0);

	event_type_expected = AWS_IOT_EVT_CONNECTED;

	test_mqtt_helper_cfg.cb.on_suback(APP_SUBS_MESSAGE_ID, 0);
	test_mqtt_helper_cfg.cb.on_suback(AWS_SUBS_MESSAGE_ID, 0);
}

void test_on_all_events_should_call_aws_fota_mqtt_evt_handler(void)
{
	__cmock_aws_fota_mqtt_evt_handler_ExpectAnyArgsAndReturn(0);
	test_mqtt_helper_cfg.cb.on_all_events(NULL, NULL);
}

void test_on_pingresp_should_notify_pingresp(void)
{
	mqtt_helper_handlers_register(event_handler);

	event_type_expected = AWS_IOT_EVT_PINGRESP;

	test_mqtt_helper_cfg.cb.on_pingresp();
}

void test_on_error_should_notify_error(void)
{
	mqtt_helper_handlers_register(event_handler);

	__cmock_mqtt_helper_disconnect_ExpectAndReturn(0);

	event_type_expected = AWS_IOT_EVT_ERROR;

	test_mqtt_helper_cfg.cb.on_error(-1);
}

void test_on_publish_should_data_received(void)
{
	mqtt_helper_handlers_register(event_handler);
	char *topic_name = "some-topic";

	struct mqtt_helper_buf topic;
	struct mqtt_helper_buf payload;

	topic.ptr = topic_name;

	event_type_expected = AWS_IOT_EVT_DATA_RECEIVED;

	test_mqtt_helper_cfg.cb.on_publish(topic, payload);
}

void test_aws_fota_events_should_be_propagated(void)
{
	mqtt_helper_handlers_register(event_handler);

	struct aws_fota_event event = { 0 };

	event.id = AWS_FOTA_EVT_START;
	event_type_expected = AWS_IOT_EVT_FOTA_START;
	test_aws_fota_handler(&event);

	event.id = AWS_FOTA_EVT_DONE;
	event_type_expected = AWS_IOT_EVT_FOTA_DONE;
	test_aws_fota_handler(&event);

	event.id = AWS_FOTA_EVT_ERASE_PENDING;
	event_type_expected = AWS_IOT_EVT_FOTA_ERASE_PENDING;
	test_aws_fota_handler(&event);

	event.id = AWS_FOTA_EVT_ERASE_DONE;
	event_type_expected = AWS_IOT_EVT_FOTA_ERASE_DONE;
	test_aws_fota_handler(&event);

	event.id = AWS_FOTA_EVT_ERROR;
	event_type_expected = AWS_IOT_EVT_FOTA_ERROR;
	test_aws_fota_handler(&event);

	event.id = AWS_FOTA_EVT_DL_PROGRESS;
	event_type_expected = AWS_IOT_EVT_FOTA_DL_PROGRESS;
	test_aws_fota_handler(&event);
}

void test_on_publish_should_set_topic_type(void)
{
	mqtt_helper_handlers_register(event_handler_topic_type_check);

	struct mqtt_helper_buf topic = { 0 };
	struct mqtt_helper_buf payload = { 0 };

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_GET_ACCEPTED;
	topic.ptr = SHADOW_TOPIC_GET_ACCEPTED;
	topic.size = strlen(SHADOW_TOPIC_GET_ACCEPTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_GET_REJECTED;
	topic.ptr = SHADOW_TOPIC_GET_REJECTED;
	topic.size = strlen(SHADOW_TOPIC_GET_REJECTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_UPDATE_DELTA;
	topic.ptr = SHADOW_TOPIC_UPDATE_DELTA;
	topic.size = strlen(SHADOW_TOPIC_UPDATE_DELTA);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_UPDATE_ACCEPTED;
	topic.ptr = SHADOW_TOPIC_UPDATE_ACCEPTED;
	topic.size = strlen(SHADOW_TOPIC_UPDATE_ACCEPTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_UPDATE_REJECTED;
	topic.ptr = SHADOW_TOPIC_UPDATE_REJECTED;
	topic.size = strlen(SHADOW_TOPIC_UPDATE_REJECTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_DELETE_ACCEPTED;
	topic.ptr = SHADOW_TOPIC_DELETE_ACCEPTED;
	topic.size = strlen(SHADOW_TOPIC_DELETE_ACCEPTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_DELETE_REJECTED;
	topic.ptr = SHADOW_TOPIC_DELETE_REJECTED;
	topic.size = strlen(SHADOW_TOPIC_DELETE_REJECTED);
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);

	topic_type_expected = AWS_IOT_SHADOW_TOPIC_APPLICATION_SPECIFIC;
	topic.ptr = "some-application-specific-topic";
	topic.size = strlen("some-application-specific-topic");
	test_mqtt_helper_cfg.cb.on_publish(topic, payload);
}

int main(void)
{
	(void)unity_main();

	return 0;
}
