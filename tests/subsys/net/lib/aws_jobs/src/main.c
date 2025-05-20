/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <unity.h>
#include <aws_jobs.h>
#include <zephyr/net/mqtt.h>

#include "cmock_mqtt.h"

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

void test_aws_jobs_cmp__null(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	int ret;

	ret = aws_jobs_cmp(NULL, published, strlen(published), "");
	TEST_ASSERT_FALSE(ret);

	ret = aws_jobs_cmp(subscribed, NULL, strlen(published), "");
	TEST_ASSERT_FALSE(ret);

	ret = aws_jobs_cmp(subscribed, published, strlen(published), NULL);
	TEST_ASSERT_FALSE(ret);
}

void test_aws_jobs_cmp__empty(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	bool ret;

	ret = aws_jobs_cmp("", published, strlen(published), "");
	TEST_ASSERT_FALSE_MESSAGE(ret, "Should not be equal, blank subscribed matched");

	ret = aws_jobs_cmp(subscribed, "", strlen(published), "");
	TEST_ASSERT_FALSE_MESSAGE(ret, "Should not be equal, blank published matched");
}

void test_aws_jobs_cmp__no_suffix(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	bool ret = aws_jobs_cmp(subscribed, published, strlen(published), "");

	TEST_ASSERT(ret);
}

void test_aws_jobs_cmp__suffix(void)
{
	const char *accepted = "$aws/things/nrf-id/jobs/123/update/accepted";
	const char *subscribe = "$aws/things/nrf-id/jobs/123/update/#";
	const char *rejected = "$aws/things/nrf-id/jobs/123/update/rejected";
	const char *no_match = "$aws/things/nrf-id/jobs/666/update/accepted";
	bool ret = aws_jobs_cmp(subscribe, accepted, strlen(accepted), "accepted");

	TEST_ASSERT_MESSAGE(ret, "Should be equal");

	ret = aws_jobs_cmp(subscribe, rejected, strlen(rejected), "rejected");
	TEST_ASSERT_MESSAGE(ret, "Should be equal");

	ret = aws_jobs_cmp(subscribe, accepted, strlen(accepted), "rejected");
	TEST_ASSERT_FALSE_MESSAGE(ret, "Should not be equal, looking for rejected");

	ret = aws_jobs_cmp(subscribe, no_match, strlen(no_match), "accepted");
	TEST_ASSERT_FALSE_MESSAGE(ret, "Should not be equal");
}

void test_aws_jobs_subscribe_topic_update(void)
{
	const char *expected = "$aws/things/client_id_123/jobs/job_id/update/#";
	char *client_id = "client_id_123";
	struct mqtt_client client = {.client_id.utf8 = client_id};
	char topic_buf[AWS_JOBS_TOPIC_MAX_LEN];

	__cmock_mqtt_subscribe_ExpectAndReturn(&client, NULL, 0);
	/* Skip checking param which is internally constructed by the unit under test. */
	__cmock_mqtt_subscribe_IgnoreArg_param();

	int ret = aws_jobs_subscribe_topic_update(&client, "job_id", topic_buf);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_STRING(expected, topic_buf);
}

void test_aws_jobs_subscribe_topic_get(void)
{
	const char *expected = "$aws/things/client_id_123/jobs/job_id/get/#";
	char *client_id = "client_id_123";
	struct mqtt_client client = {.client_id.utf8 = client_id};
	char topic_buf[AWS_JOBS_TOPIC_MAX_LEN];

	__cmock_mqtt_subscribe_ExpectAndReturn(&client, NULL, 0);
	/* Skip checking param which is internally constructed by the unit under test. */
	__cmock_mqtt_subscribe_IgnoreArg_param();

	int ret = aws_jobs_subscribe_topic_get(&client, "job_id", topic_buf);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_STRING(expected, topic_buf);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
