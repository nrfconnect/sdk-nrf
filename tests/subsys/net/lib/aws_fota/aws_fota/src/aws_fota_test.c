/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <string.h>
#include <unity.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <net/aws_fota.h>

#include "cmock_mqtt.h"
#include "cmock_fota_download.h"

#define CLIENT_ID "test-client"
#define TOPIC_GET_ACCEPTED "$aws/things/" CLIENT_ID "/jobs/$next/get/accepted"
#define TOPIC_NOTIFY_NEXT "$aws/things/" CLIENT_ID "/jobs/notify-next"
#define PUBLISH_MESSAGE_ID 77

/* The size of the payload buffer in the library. */
#define PAYLOAD_BUF_SIZE CONFIG_AWS_FOTA_PAYLOAD_SIZE
/* A message that does not fit in the payload buffer of the library. */
#define MSG_OVERSIZED (PAYLOAD_BUF_SIZE * 3 + 5)
/* Maximum number of bytes that the MQTT stub returns in one read. */
#define READ_CHUNK 40

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

static struct mqtt_client client = {
	.client_id.utf8 = CLIENT_ID,
	.client_id.size = sizeof(CLIENT_ID) - 1,
};

/* State of the library under test, set up once because the library keeps static state. */
static bool lib_started;

/* Message that the MQTT stub reads out. */
static struct {
	const uint8_t *data;
	size_t len;
	size_t pos;
	/* The read fails with this error when this many bytes are read. 0 means no failure. */
	size_t fail_at;
	int fail_err;
} rx;

/* Observations of what the library does. */
static int ack_count;
static int callback_count;
static enum aws_fota_error_cause callback_cause;
static int publish_count;

static void fota_callback(struct aws_fota_event *evt)
{
	if (evt->id == AWS_FOTA_EVT_ERROR) {
		callback_count++;
		callback_cause = evt->cause;
	}
}

static int read_stub(struct mqtt_client *c, void *buffer, size_t length, int num_calls)
{
	size_t n = MIN(MIN(length, rx.len - rx.pos), READ_CHUNK);

	if (rx.fail_at != 0 && rx.pos + n >= rx.fail_at) {
		return rx.fail_err;
	}

	memcpy(buffer, &rx.data[rx.pos], n);
	rx.pos += n;

	return n;
}

static int ack_stub(struct mqtt_client *c, const struct mqtt_puback_param *param, int num_calls)
{
	TEST_ASSERT_EQUAL(PUBLISH_MESSAGE_ID, param->message_id);
	ack_count++;

	return 0;
}

static int publish_stub(struct mqtt_client *c, const struct mqtt_publish_param *param,
			int num_calls)
{
	publish_count++;

	return 0;
}

static int subscribe_stub(struct mqtt_client *c, const struct mqtt_subscription_list *param,
			  int num_calls)
{
	return 0;
}

/* Send an MQTT PUBLISH event with a payload to the library. */
static int publish_event(const char *topic, const uint8_t *payload, size_t len)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.result = 0,
	};

	rx.data = payload;
	rx.len = len;
	rx.pos = 0;

	evt.param.publish.message_id = PUBLISH_MESSAGE_ID;
	evt.param.publish.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	evt.param.publish.message.topic.topic.utf8 = topic;
	evt.param.publish.message.topic.topic.size = strlen(topic);
	evt.param.publish.message.payload.len = len;

	return aws_fota_mqtt_evt_handler(&client, &evt);
}

/* Build a message of the given length that starts with the given text. The rest is padding. */
static void message_build(uint8_t *out, size_t len, const char *head)
{
	memset(out, 'A', len);
	memcpy(out, head, strlen(head));
}

void setUp(void)
{
	struct mqtt_evt connack = {
		.type = MQTT_EVT_CONNACK,
		.result = 0,
	};

	memset(&rx, 0, sizeof(rx));
	ack_count = 0;
	callback_count = 0;
	publish_count = 0;

	__cmock_mqtt_read_publish_payload_blocking_Stub(read_stub);
	__cmock_mqtt_publish_qos1_ack_Stub(ack_stub);
	__cmock_mqtt_publish_Stub(publish_stub);
	__cmock_mqtt_subscribe_Stub(subscribe_stub);

	if (!lib_started) {
		__cmock_fota_download_init_ExpectAnyArgsAndReturn(0);
		TEST_ASSERT_EQUAL(0, aws_fota_init(fota_callback));
		/* The CONNACK makes the library create the topics and subscribe to them. */
		TEST_ASSERT_EQUAL(1, aws_fota_mqtt_evt_handler(&client, &connack));
		lib_started = true;
	}
}

void tearDown(void)
{
}

/* An oversized message without a job in it is dropped. The library keeps the connection, reads
 * out the whole message, acknowledges it and reports the error to the application.
 */
void test_oversized_message_without_job_is_dropped(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	int err;

	message_build(msg, sizeof(msg), "{\"clientToken\":\"\",\"timestamp\":1,\"padding\":\"");

	err = publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(1, ack_count);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(AWS_FOTA_ERROR_CAUSE_PAYLOAD_TOO_LARGE, callback_cause);
	/* The job could not be identified, so no job update is sent. */
	TEST_ASSERT_EQUAL(0, publish_count);
}

/* The same applies to a message received on the notify-next topic. */
void test_oversized_notify_next_message_is_dropped(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	int err;

	message_build(msg, sizeof(msg), "{\"timestamp\":1,\"padding\":\"");

	err = publish_event(TOPIC_NOTIFY_NEXT, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(1, ack_count);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);
}

/* A message that is exactly as large as the buffer must not be treated as oversized. */
void test_message_not_larger_than_buffer_is_not_dropped(void)
{
	static uint8_t msg[PAYLOAD_BUF_SIZE];
	int err;

	/* A message with only one field means that there is no queued job. */
	memset(msg, ' ', sizeof(msg));
	memcpy(msg, "{\"timestamp\":1}", sizeof("{\"timestamp\":1}") - 1);

	err = publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(1, ack_count);
	TEST_ASSERT_EQUAL(0, callback_count);
}

/* A failure when the payload is read out is still a connection error. */
void test_payload_read_error_is_returned(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	int err;

	message_build(msg, sizeof(msg), "{\"timestamp\":1,\"padding\":\"");
	rx.fail_at = PAYLOAD_BUF_SIZE + 2 * READ_CHUNK;
	rx.fail_err = -ECONNRESET;

	err = publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(-ECONNRESET, err);
	TEST_ASSERT_EQUAL(0, ack_count);
	TEST_ASSERT_EQUAL(0, callback_count);
}

/* An error from the MQTT library is returned even if it is -EMSGSIZE. The message is not
 * acknowledged and no job is failed.
 */
void test_payload_read_emsgsize_error_is_returned(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	int err;

	message_build(msg, sizeof(msg), "{\"timestamp\":1,\"padding\":\"");
	rx.fail_at = PAYLOAD_BUF_SIZE + 2 * READ_CHUNK;
	rx.fail_err = -EMSGSIZE;

	err = publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(-EMSGSIZE, err);
	TEST_ASSERT_EQUAL(0, ack_count);
	TEST_ASSERT_EQUAL(0, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);
}

int main(void)
{
	(void)unity_main();

	return 0;
}
