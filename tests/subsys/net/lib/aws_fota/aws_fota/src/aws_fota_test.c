/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <stdio.h>
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
#define TOPIC_JOB_UPDATE(job_id) "$aws/things/" CLIENT_ID "/jobs/" job_id "/update"
#define TOPIC_JOB_UPDATE_ACCEPTED(job_id) TOPIC_JOB_UPDATE(job_id) "/accepted"
#define JOB_ID_DEFAULT "INVALID-JOB-ID"
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
static char publish_topic[128];
static char publish_payload[256];

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
	size_t topic_len = MIN(param->message.topic.topic.size, sizeof(publish_topic) - 1);
	size_t payload_len = MIN(param->message.payload.len, sizeof(publish_payload) - 1);

	publish_count++;
	memset(publish_topic, 0, sizeof(publish_topic));
	memset(publish_payload, 0, sizeof(publish_payload));
	memcpy(publish_topic, param->message.topic.topic.utf8, topic_len);
	memcpy(publish_payload, param->message.payload.data, payload_len);

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

/* Beginning of a job execution message from AWS IoT Jobs. The job document follows. */
#define JOB_HEAD(job_id, version)							\
	"{\"clientToken\":\"\",\"timestamp\":1,\"execution\":{\"jobId\":\"" job_id "\"," \
	"\"status\":\"QUEUED\",\"queuedAt\":1,\"lastUpdatedAt\":1,\"versionNumber\":" version "," \
	"\"executionNumber\":1,\"jobDocument\":{\"padding\":\""

/* Let the library finish the update of a failed job, so that it is ready for the next test. */
static void failed_job_complete(const char *update_accepted_topic)
{
	uint8_t msg[] = "{}";
	char job_id[32];

	TEST_ASSERT_EQUAL(0, publish_event(update_accepted_topic, msg, sizeof(msg) - 1));
	TEST_ASSERT_GREATER_THAN(0, aws_fota_get_job_id(job_id, sizeof(job_id)));
	TEST_ASSERT_EQUAL_STRING(JOB_ID_DEFAULT, job_id);
}

/* An oversized job message with a job ID and a version in the beginning fails the job, so that
 * AWS IoT Jobs does not send the job again.
 */
void test_oversized_job_is_failed(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	char job_id[32];
	int err;

	message_build(msg, sizeof(msg), JOB_HEAD("job-big-1", "3"));

	err = publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(1, ack_count);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(AWS_FOTA_ERROR_CAUSE_PAYLOAD_TOO_LARGE, callback_cause);

	/* The job execution is updated with the right job, version, status and reason. */
	TEST_ASSERT_EQUAL(1, publish_count);
	TEST_ASSERT_EQUAL_STRING(TOPIC_JOB_UPDATE("job-big-1"), publish_topic);
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"status\":\"FAILED\""));
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"expectedVersion\": \"3\""));
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"reason\":\"PAYLOAD_TOO_LARGE\""));

	TEST_ASSERT_GREATER_THAN(0, aws_fota_get_job_id(job_id, sizeof(job_id)));
	TEST_ASSERT_EQUAL_STRING("job-big-1", job_id);

	failed_job_complete(TOPIC_JOB_UPDATE_ACCEPTED("job-big-1"));
}

/* The job is failed also when the message is received on the notify-next topic. */
void test_oversized_notify_next_job_is_failed(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	int err;

	message_build(msg, sizeof(msg), JOB_HEAD("job-big-2", "12"));

	err = publish_event(TOPIC_NOTIFY_NEXT, msg, sizeof(msg));

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);
	TEST_ASSERT_EQUAL_STRING(TOPIC_JOB_UPDATE("job-big-2"), publish_topic);
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"expectedVersion\": \"12\""));

	failed_job_complete(TOPIC_JOB_UPDATE_ACCEPTED("job-big-2"));
}

/* Text of a job message from the client token to the version number. */
#define CUT_HEAD_A "{\"clientToken\":\""
#define CUT_HEAD_B "\",\"timestamp\":1,\"execution\":{\"jobId\":\"job-cut\"," \
		   "\"status\":\"QUEUED\",\"versionNumber\":"

/* The message is cut inside the version number, so the number is incomplete. The job must not be
 * failed with a wrong version. The message is dropped like a message without a job.
 */
void test_oversized_job_cut_inside_version_is_dropped(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	/* The library keeps PAYLOAD_BUF_SIZE - 1 bytes. Cut after the first digit of 12. */
	int pad = PAYLOAD_BUF_SIZE - 2 - (sizeof(CUT_HEAD_A) - 1) - (sizeof(CUT_HEAD_B) - 1);
	int n;

	n = snprintf((char *)msg, sizeof(msg),
		     CUT_HEAD_A "%*s" CUT_HEAD_B "12,\"executionNumber\":1,\"jobDocument\":{}", pad,
		     "");
	memset(&msg[n], 'A', sizeof(msg) - n);

	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(1, ack_count);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(AWS_FOTA_ERROR_CAUSE_PAYLOAD_TOO_LARGE, callback_cause);
	TEST_ASSERT_EQUAL(0, publish_count);
}

/* A job ID that is the name of a key, or a key name in a string value, is not a key. */
void test_oversized_job_with_key_name_as_value_is_failed(void)
{
	static uint8_t msg[MSG_OVERSIZED];

	message_build(msg, sizeof(msg),
		      "{\"clientToken\":\"\\\"jobId\\\":\\\"fake\\\","
		      "\\\"versionNumber\\\":9\",\"timestamp\":1,\"execution\":{"
		      "\"jobId\":\"versionNumber\",\"status\":\"QUEUED\",\"versionNumber\":7,"
		      "\"jobDocument\":{\"padding\":\"");

	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);
	TEST_ASSERT_EQUAL_STRING(TOPIC_JOB_UPDATE("versionNumber"), publish_topic);
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"expectedVersion\": \"7\""));

	failed_job_complete(TOPIC_JOB_UPDATE_ACCEPTED("versionNumber"));
}

/* Only the members of the execution object are used. A job document before the version number
 * is skipped, also when it has members with the same names.
 */
void test_oversized_job_with_document_before_version_is_failed(void)
{
	static uint8_t msg[MSG_OVERSIZED];

	message_build(msg, sizeof(msg),
		      "{\"execution\":{\"jobId\":\"job-nest\",\"jobDocument\":{"
		      "\"jobId\":\"x\",\"versionNumber\":99,\"list\":[1,{\"versionNumber\":98}]},"
		      "\"versionNumber\":5,\"padding\":\"");

	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);
	TEST_ASSERT_EQUAL_STRING(TOPIC_JOB_UPDATE("job-nest"), publish_topic);
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"expectedVersion\": \"5\""));

	failed_job_complete(TOPIC_JOB_UPDATE_ACCEPTED("job-nest"));
}

/* If the job document is cut off before the version number of the execution, the version of the
 * job document must not be used. The message is dropped and the job is not updated.
 */
void test_oversized_job_with_cut_document_before_version_is_dropped(void)
{
	static uint8_t msg[MSG_OVERSIZED];

	message_build(msg, sizeof(msg),
		      "{\"execution\":{\"jobId\":\"job-cutdoc\",\"jobDocument\":{"
		      "\"versionNumber\":99,\"padding\":\"");

	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);
}

/* While a failed job is being updated, further oversized messages are dropped without telling
 * the application about another error.
 */
void test_oversized_message_while_job_is_failing_is_ignored(void)
{
	static uint8_t msg[MSG_OVERSIZED];

	message_build(msg, sizeof(msg), JOB_HEAD("job-big-3", "1"));
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);

	message_build(msg, sizeof(msg), JOB_HEAD("job-big-4", "1"));
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_NOTIFY_NEXT, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(2, ack_count);
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);

	failed_job_complete(TOPIC_JOB_UPDATE_ACCEPTED("job-big-3"));
}

/* The job is not failed if its ID or its version is not in the part of the message that was kept.
 * The message is dropped.
 */
void test_oversized_job_that_can_not_be_identified_is_dropped(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	char job_id[32];

	/* No version number. */
	message_build(msg, sizeof(msg),
		      "{\"execution\":{\"jobId\":\"job-big-5\",\"jobDocument\":{\"x\":\"");
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));
	TEST_ASSERT_EQUAL(1, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);

	/* The end of the job ID is not in the kept part of the message. */
	message_build(msg, sizeof(msg), "{\"execution\":{");
	memcpy(&msg[PAYLOAD_BUF_SIZE - 12], "\"jobId\":\"job-big-6\",\"versionNumber\":1", 38);
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));
	TEST_ASSERT_EQUAL(2, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);

	/* The job ID is too long. */
	message_build(msg, sizeof(msg), "{\"execution\":{\"jobId\":\""
		"0123456789012345678901234567890123456789012345678901234567890123456789\","
		"\"versionNumber\":1,\"jobDocument\":{\"x\":\"");
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));
	TEST_ASSERT_EQUAL(3, callback_count);
	TEST_ASSERT_EQUAL(0, publish_count);

	/* The library did not start to handle any job. */
	TEST_ASSERT_GREATER_THAN(0, aws_fota_get_job_id(job_id, sizeof(job_id)));
	TEST_ASSERT_EQUAL_STRING(JOB_ID_DEFAULT, job_id);
}

/* An oversized message for the job that is already being started is ignored. Failing it would
 * use an outdated version and the update would be rejected. This test leaves the job in
 * progress, so it must be the last one.
 */
void test_oversized_message_of_started_job_is_ignored(void)
{
	static uint8_t msg[MSG_OVERSIZED];
	static const char job[] =
		"{\"timestamp\":1,\"execution\":{\"jobId\":\"job-dup\",\"status\":\"QUEUED\","
		"\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":"
		"\"app_fw_update\",\"location\":{\"protocol\":\"https:\",\"host\":\"example.com\","
		"\"path\":\"/fw.bin\"}}}}";

	/* The job fits. The library starts it and waits for AWS to accept the update. */
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_NOTIFY_NEXT, (const uint8_t *)job,
					   sizeof(job) - 1));
	TEST_ASSERT_EQUAL(1, publish_count);
	TEST_ASSERT_NOT_NULL(strstr(publish_payload, "\"status\":\"IN_PROGRESS\""));

	/* The same job arrives again in a message that does not fit. */
	message_build(msg, sizeof(msg), JOB_HEAD("job-dup", "1"));
	TEST_ASSERT_EQUAL(0, publish_event(TOPIC_GET_ACCEPTED, msg, sizeof(msg)));

	TEST_ASSERT_EQUAL(sizeof(msg), rx.pos);
	TEST_ASSERT_EQUAL(2, ack_count);
	TEST_ASSERT_EQUAL(0, callback_count);
	TEST_ASSERT_EQUAL(1, publish_count);
}

int main(void)
{
	(void)unity_main();

	return 0;
}
