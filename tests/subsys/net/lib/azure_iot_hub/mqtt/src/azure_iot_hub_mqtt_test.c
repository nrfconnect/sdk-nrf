/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <kernel.h>
#include <string.h>
#include <init.h>

#include "azure_iot_hub_mqtt.h"

#include "net/mock_socket.h"
#include "mock_mqtt.h"

#define TEST_PORT		8883
#define TEST_HOSTNAME		"test-hostname.azure-devices.net"
#define TEST_HOSTNAME_LEN	(sizeof(TEST_HOSTNAME) - 1)

#define TEST_DEVICE_ID		"test-device"
#define TEST_DEVICE_ID_LEN	(sizeof(TEST_DEVICE_ID) - 1)

#define TEST_USER_NAME		"test-username"
#define TEST_USER_NAME_LEN	(sizeof(TEST_USER_NAME) - 1)

#define TEST_MESSAGE_ID		12346

#define TEST_TOPIC_1		"test/topic_1"
#define TEST_TOPIC_1_LEN	(sizeof(TEST_TOPIC_1) - 1)

#define TEST_TOPIC_2		"test/topic_2"
#define TEST_TOPIC_2_LEN	(sizeof(TEST_TOPIC_2) - 1)

#define TEST_PAYLOAD		"This is a test payload"
#define TEST_PAYLOAD_LEN	(sizeof(TEST_PAYLOAD) - 1)

/* Pull in variables and functions from the MQTT helper library. */
extern struct mqtt_client mqtt_client;
extern enum mqtt_state mqtt_state;
extern struct k_sem connection_poll_sem;
extern k_tid_t azure_iot_hub_mqtt_thread;
extern int unity_main(void);
extern enum mqtt_state mqtt_state_get(void);
extern void mqtt_state_set(enum mqtt_state state);
extern void mqtt_evt_handler(struct mqtt_client *const mqtt_client,
			     const struct mqtt_evt *mqtt_evt);
extern void mqtt_helper_poll_loop(void);
extern void on_publish(const struct mqtt_evt *mqtt_evt);
extern char payload_buf[];

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

/* Semaphores used by tests to wait for a certain callbacks */
static K_SEM_DEFINE(connack_success_sem, 0, 1);
static K_SEM_DEFINE(connack_failed_sem, 0, 1);
static K_SEM_DEFINE(disconnect_sem, 0, 1);
static K_SEM_DEFINE(puback_sem, 0, 1);
static K_SEM_DEFINE(suback_sem, 0, 1);
static K_SEM_DEFINE(publish_sem, 0, 1);
static K_SEM_DEFINE(error_msg_size_sem, 0, 1);

void setUp(void)
{
	mock_mqtt_Init();
	__wrap_mqtt_keepalive_time_left_IgnoreAndReturn(0);

	/* Suspend the polling thread to have full control over polling. */
	k_thread_suspend(azure_iot_hub_mqtt_thread);

	/* Force all tests to start in uninitialized state. */
	mqtt_state = MQTT_STATE_UNINIT;
}

void tearDown(void)
{
	mock_mqtt_Verify();
}

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

/* Stubs */
static int mqtt_readall_publish_payload_stub(struct mqtt_client *client, uint8_t *buffer,
					     size_t length, int num_calls)
{
	memcpy(payload_buf, TEST_PAYLOAD, TEST_PAYLOAD_LEN);

	return 0;
}

static int poll_stub_pollin(struct pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = fds[0].events & POLLIN;

	/* The stub should only return 1 the first time, and otherwise return
	 * and error to let the calling function return.
	 */

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollnval(struct pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = POLLNVAL;

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollhup(struct pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = POLLHUP;

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollerr(struct pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = POLLERR;

	return num_calls == 0 ? 1 : -1;
}


/* Helper functions */
static void send_publish_event(int message_id)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.result = 0,
		.param.publish.message = {
			.topic = {
				.topic = {
					.utf8 = TEST_TOPIC_1,
					.size = TEST_TOPIC_1_LEN,
				},
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			},
			.payload = {
				.data = TEST_PAYLOAD,
				.len = TEST_PAYLOAD_LEN,
			},
		}
	};

	mqtt_evt_handler(&mqtt_client, &evt);
}

static void send_mqtt_event(enum mqtt_evt_type type, int optional_data)
{
	struct mqtt_evt evt = {
		.type = type,
		.result = 0,
	};

	switch (type) {
	case MQTT_EVT_CONNACK:
		evt.param.connack.return_code = optional_data;
		break;
	case MQTT_EVT_DISCONNECT:
		break;
	case MQTT_EVT_PUBACK:
		evt.param.puback.message_id = optional_data;
		break;
	case MQTT_EVT_SUBACK:
		evt.param.suback.message_id = optional_data;
		break;
	case MQTT_EVT_PUBLISH:
		send_publish_event(optional_data);
		return;
	default:
		/* Unahndled event type, should not happen and considered bug
		 * in the test.
		 */
		TEST_ASSERT_TRUE(false);
	}

	mqtt_evt_handler(&mqtt_client, &evt);
}

/* Callbacks used in tests. */
static void cb_on_publish(struct azure_iot_hub_buf topic,
		       struct azure_iot_hub_buf payload)
{
	TEST_ASSERT_EQUAL(TEST_TOPIC_1_LEN, topic.size);
	TEST_ASSERT_EQUAL_MEMORY(TEST_TOPIC_1, topic.ptr, TEST_TOPIC_1_LEN);
	TEST_ASSERT_EQUAL(TEST_PAYLOAD_LEN, payload.size);
	TEST_ASSERT_EQUAL_MEMORY(TEST_PAYLOAD, payload.ptr, TEST_PAYLOAD_LEN);

	k_sem_give(&publish_sem);
}

static void cb_on_connack(enum mqtt_conn_return_code return_code)
{
	switch (return_code) {
	case MQTT_CONNECTION_ACCEPTED:
		k_sem_give(&connack_success_sem);
		break;

	case MQTT_NOT_AUTHORIZED:
		k_sem_give(&connack_failed_sem);
		break;
	default:
		/* Unexpected return code, should not happen and considered bug in the test. */
		TEST_ASSERT_TRUE(false);
	}
}

static void cb_on_disconnect(int result)
{
	k_sem_give(&disconnect_sem);
}

static void cb_on_puback(uint16_t message_id, int result)
{
	if (message_id == TEST_MESSAGE_ID) {
		k_sem_give(&puback_sem);
	}
}

static void cb_on_suback(uint16_t message_id, int result)
{
	if (message_id == TEST_MESSAGE_ID) {
		k_sem_give(&suback_sem);
	}
}

static void cb_on_error(enum mqtt_helper_error error)
{
	if (error == MQTT_HELPER_ERROR_MSG_SIZE) {
		k_sem_give(&error_msg_size_sem);
	}
}

/* Tests */

void test_mqtt_helper_init_when_unitialized(void)
{
	struct mqtt_helper_cfg cfg = {
		.cb = {
			.on_connack = cb_on_connack,
			.on_disconnect = cb_on_disconnect,
			.on_publish = cb_on_publish,
			.on_puback = cb_on_puback,
			.on_suback = cb_on_suback,
			.on_error = cb_on_error,
		},
	};

	TEST_ASSERT_EQUAL(0, mqtt_helper_init(&cfg));
	TEST_ASSERT_EQUAL(mqtt_state_get(), MQTT_STATE_DISCONNECTED);
}

void test_mqtt_helper_init_when_connected(void)
{
	struct mqtt_helper_cfg cfg;

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_init(&cfg));
}

void test_mqtt_helper_connect_when_disconnected(void)
{
	struct mqtt_helper_conn_params conn_params = {
		.port = TEST_PORT,
		.hostname = {
			.ptr = TEST_HOSTNAME,
			.size = TEST_HOSTNAME_LEN,
		},
		.user_name = {
			.ptr = TEST_USER_NAME,
			.size = TEST_USER_NAME_LEN,
		},
		.device_id = {
			.ptr = TEST_DEVICE_ID,
			.size = TEST_DEVICE_ID_LEN,
		},
	};

	__wrap_mqtt_client_init_Expect(&mqtt_client);
	__wrap_getaddrinfo_ExpectAnyArgsAndReturn(0);
	__wrap_freeaddrinfo_ExpectAnyArgs();
	__wrap_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
}

void test_mqtt_helper_connect_when_disconnected_mqtt_api_error(void)
{
	struct mqtt_helper_conn_params conn_params_dummy;

	__wrap_mqtt_client_init_Expect(&mqtt_client);
	__wrap_getaddrinfo_ExpectAnyArgsAndReturn(0);
	__wrap_freeaddrinfo_ExpectAnyArgs();
	__wrap_mqtt_connect_ExpectAndReturn(&mqtt_client, -2);

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-2, mqtt_helper_connect(&conn_params_dummy));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_mqtt_helper_connect_when_uninitialized(void)
{
	struct mqtt_helper_conn_params conn_params;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_connect(&conn_params));
}

void test_on_connack_successful(void)
{
	mqtt_state = MQTT_STATE_CONNECTING;

	send_mqtt_event(MQTT_EVT_CONNACK, MQTT_CONNECTION_ACCEPTED);

	TEST_ASSERT_EQUAL(0, k_sem_take(&connack_success_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTED, mqtt_state_get());
}

void test_on_connack_failed(void)
{
	mqtt_state = MQTT_STATE_CONNECTING;

	send_mqtt_event(MQTT_EVT_CONNACK, MQTT_NOT_AUTHORIZED);

	TEST_ASSERT_EQUAL(0, k_sem_take(&connack_failed_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_on_disconnect(void)
{
	mqtt_state = MQTT_STATE_CONNECTED;

	send_mqtt_event(MQTT_EVT_DISCONNECT, 0);

	TEST_ASSERT_EQUAL(0, k_sem_take(&disconnect_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_on_puback(void)
{
	send_mqtt_event(MQTT_EVT_PUBACK, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&puback_sem, K_SECONDS(1)));
}

void test_on_suback(void)
{
	send_mqtt_event(MQTT_EVT_SUBACK, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&suback_sem, K_SECONDS(1)));
}

void test_on_publish(void)
{
	__wrap_mqtt_readall_publish_payload_Stub(mqtt_readall_publish_payload_stub);
	__wrap_mqtt_publish_qos1_ack_ExpectAnyArgsAndReturn(0);

	send_mqtt_event(MQTT_EVT_PUBLISH, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&publish_sem, K_SECONDS(1)));
}

void test_on_publish_too_large_incoming_msg(void)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.param.publish.message = {
			.payload = {
				.len = CONFIG_AZURE_IOT_HUB_MQTT_PAYLOAD_BUFFER_LEN + 1,
			},
		}
	};

	mqtt_evt_handler(&mqtt_client, &evt);
	TEST_ASSERT_EQUAL(0, k_sem_take(&error_msg_size_sem, K_SECONDS(1)));
}

void test_mqtt_helper_disconnect_when_connected(void)
{
	__wrap_mqtt_disconnect_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_disconnect());
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTING, mqtt_state_get());
}

void test_mqtt_helper_disconnect_when_connected_mqtt_api_error(void)
{
	__wrap_mqtt_disconnect_ExpectAndReturn(&mqtt_client, -1);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-1, mqtt_helper_disconnect());
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_mqtt_helper_disconnect_when_disconnected(void)
{
	mqtt_state = MQTT_STATE_UNINIT;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_disconnect());
}

void test_mqtt_helper_subscribe_when_connected(void)
{
	struct mqtt_topic sub_topics[] = {
		{
			.topic.utf8 = TEST_TOPIC_1,
			.topic.size = TEST_TOPIC_1_LEN,
		},
		{
			.topic.utf8 = TEST_TOPIC_2,
			.topic.size = TEST_TOPIC_2_LEN,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = TEST_MESSAGE_ID,
	};

	__wrap_mqtt_subscribe_ExpectAndReturn(&mqtt_client, &sub_list, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_subscribe(&sub_list));
}

void test_mqtt_helper_subscribe_when_disconnected(void)
{
	struct mqtt_subscription_list sub_list_dummy;

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_subscribe(&sub_list_dummy));
}

void test_mqtt_helper_subscribe_mqtt_api_error(void)
{
	struct mqtt_subscription_list sub_list_dummy;

	__wrap_mqtt_subscribe_ExpectAndReturn(&mqtt_client, &sub_list_dummy, -EINVAL);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EINVAL, mqtt_helper_subscribe(&sub_list_dummy));
}

void test_mqtt_helper_publish_when_connected(void)
{
	struct mqtt_publish_param pub_param = {
		.message = {
			.payload = {
				.data = TEST_PAYLOAD,
				.len = TEST_PAYLOAD_LEN,
			},
			.topic = {
				.topic = {
					.utf8 = TEST_TOPIC_1,
					.size = TEST_TOPIC_1_LEN,
				},
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			},
		},
		.message_id = TEST_MESSAGE_ID,
	};

	__wrap_mqtt_publish_ExpectAndReturn(&mqtt_client, &pub_param, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_publish(&pub_param));
}

void test_mqtt_helper_publish_when_disconnected(void)
{
	struct mqtt_publish_param pub_param_dummy;

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_publish(&pub_param_dummy));
}

void test_mqtt_helper_deinit_when_disconnected(void)
{
	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_deinit());
	TEST_ASSERT_EQUAL(MQTT_STATE_UNINIT, mqtt_state_get());
}

void test_mqtt_helper_deinit_when_connected(void)
{
	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_deinit());
}

/* Test that the polling stops and state is set to _DISCONNECTED when the
 * library has already initiated disconnection.
 * It's expected that no socket or MQTT APIs are called.
 */
void test_mqtt_helper_poll_loop_disconnecting(void)
{
	mqtt_state = MQTT_STATE_DISCONNECTING;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

/* The test verifies that mqtt_live() is called whn poll() returns 0. */
void test_mqtt_helper_poll_loop_timeout(void)
{
	/* Let poll() return 0 first and then -1 on subsequent call to end the test. */
	__wrap_poll_ExpectAnyArgsAndReturn(0);
	__wrap_poll_ExpectAnyArgsAndReturn(-1);
	__wrap_mqtt_live_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to POLLIN and that mqtt_input is subsequently called when poll() returns
 * POLLIN in the revents.
 */
void test_mqtt_helper_poll_loop_pollin(void)
{
	__wrap_poll_Stub(poll_stub_pollin);
	__wrap_mqtt_input_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to POLLNVAL and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollnval(void)
{
	__wrap_poll_Stub(poll_stub_pollnval);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to POLLHUP and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollhup(void)
{
	__wrap_poll_Stub(poll_stub_pollhup);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to POLLERR and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollerr(void)
{
	__wrap_poll_Stub(poll_stub_pollerr);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void main(void)
{
	(void)unity_main();
}
