/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <aws_jobs.h>
#include <net/mqtt.h>

int mqtt_subscribe(struct mqtt_client *client,
		   const struct mqtt_subscription_list *param)
{
	return 0;
}

int mqtt_unsubscribe(struct mqtt_client *client,
		     const struct mqtt_subscription_list *param)
{
	return 0;
}

static void test_aws_jobs_cmp__null(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	int ret;

	ret = aws_jobs_cmp(NULL, published, strlen(published), "");
	zassert_false(ret, "");

	ret = aws_jobs_cmp(subscribed, NULL, strlen(published), "");
	zassert_false(ret, "");

	ret = aws_jobs_cmp(subscribed, published, strlen(published), NULL);
	zassert_false(ret, "");
}

static void test_aws_jobs_cmp__empty(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	int ret;

	ret = aws_jobs_cmp("", published, strlen(published), "");
	zassert_false(ret, "Should not be equal, blank subscribed matched");

	ret = aws_jobs_cmp(subscribed, "", strlen(published), "");
	zassert_false(ret, "Should not be equal, blank published matched");
}

static void test_aws_jobs_cmp__no_suffix(void)
{
	const char *published = "$aws/things/nrf-id/jobs/notify-next";
	const char *subscribed = "$aws/things/nrf-id/jobs/notify-next";
	int ret = aws_jobs_cmp(subscribed, published, strlen(published), "");

	zassert_true(ret, "");
}

static void test_aws_jobs_cmp__suffix(void)
{
	const char *accepted = "$aws/things/nrf-id/jobs/123/update/accepted";
	const char *subscribe = "$aws/things/nrf-id/jobs/123/update/#";
	const char *rejected = "$aws/things/nrf-id/jobs/123/update/rejected";
	const char *no_match = "$aws/things/nrf-id/jobs/666/update/accepted";
	int ret = aws_jobs_cmp(subscribe, accepted, strlen(accepted),
			"accepted");

	zassert_true(ret > 0, "Should be equal");

	ret = aws_jobs_cmp(subscribe, rejected, strlen(rejected), "rejected");
	zassert_true(ret > 0, "Should be equal");

	ret = aws_jobs_cmp(subscribe, accepted, strlen(accepted), "rejected");
	zassert_false(ret > 0, "Should not be equal, looking for rejected");

	ret = aws_jobs_cmp(subscribe, no_match, strlen(no_match), "accepted");
	zassert_false(ret > 0, "Should not be equal");

}

static void test_aws_jobs_subscribe_topic_update(void)
{
	const char *expected = "$aws/things/client_id_123/jobs/job_id/update/#";
	char *client_id = "client_id_123";
	struct mqtt_client client = {.client_id.utf8 = client_id};
	char topic_buf[AWS_JOBS_TOPIC_MAX_LEN];

	int ret = aws_jobs_subscribe_topic_update(&client, "job_id", topic_buf);

	zassert_equal(ret, 0, "Should be 0");
	zassert_true(strcmp(expected, topic_buf) == 0, "Should be equal");
}

static void test_aws_jobs_subscribe_topic_get(void)
{
	const char *expected = "$aws/things/client_id_123/jobs/job_id/get/#";
	char *client_id = "client_id_123";
	struct mqtt_client client = {.client_id.utf8 = client_id};
	char topic_buf[AWS_JOBS_TOPIC_MAX_LEN];

	int ret = aws_jobs_subscribe_topic_get(&client, "job_id", topic_buf);

	zassert_equal(ret, 0, "Should be 0");
	zassert_true(strcmp(expected, topic_buf) == 0, "Should be equal");
}

void test_main(void)
{
	ztest_test_suite(aws_jobs_test,
			 ztest_unit_test(test_aws_jobs_cmp__null),
			 ztest_unit_test(test_aws_jobs_cmp__empty),
			 ztest_unit_test(test_aws_jobs_cmp__no_suffix),
			 ztest_unit_test(test_aws_jobs_cmp__suffix),
			 ztest_unit_test(test_aws_jobs_subscribe_topic_update),
			 ztest_unit_test(test_aws_jobs_subscribe_topic_get)
			 );
	ztest_run_test_suite(aws_jobs_test);
}
