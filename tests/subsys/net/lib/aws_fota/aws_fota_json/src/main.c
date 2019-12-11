/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <aws_fota_json.h>

static void test_parse_job_execution(void)
{
	int expected_version_number = 1;
	char expected_job_id[] = "9b5caac6-3e8a-45dd-9273-c1b995762f4a";
	char expected_hostname[] = "fota-update-bucket.s3.eu-central-1.amazonaws.com";
	char expected_file_path[] = "/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host";

	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";
	int ret;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	int version_number;

	/* Memset to ensure correct null-termination */
	memset(job_id, 0xff, sizeof(job_id));
	memset(hostname, 0xff, sizeof(hostname));
	memset(file_path, 0xff, sizeof(file_path));

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id, hostname,
						      file_path,
						      &version_number);

	zassert_equal(ret, 1, NULL);
	zassert_true(!strcmp(job_id, expected_job_id), NULL);
	zassert_equal(version_number, expected_version_number, NULL);
	zassert_true(!strcmp(hostname, expected_hostname), NULL);
	zassert_true(!strcmp(file_path, expected_file_path), NULL);
}

static void test_parse_job_execution_missing_field(void)
{
	char expected_job_id[] = "9b5caac6-3e8a-45dd-9273-c1b995762f4a";
	char expected_hostname[] = "fota-update-bucket.s3.eu-central-1.amazonaws.com";
	char expected_file_path[] = "/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host";

	int ret;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	int version_number;

	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";
/* Memset to ensure correct null-termination */
	memset(job_id, 0xff, sizeof(job_id));
	memset(hostname, 0xff, sizeof(hostname));
	memset(file_path, 0xff, sizeof(file_path));

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id, hostname,
						      file_path,
						      &version_number);

	zassert_equal(ret, -ENODATA, NULL);
	zassert_true(!strcmp(job_id, expected_job_id), NULL);
	zassert_equal(version_number, 1, NULL);
	zassert_true(!strcmp(hostname, expected_hostname), NULL);
	zassert_true(!strcmp(file_path, expected_file_path), NULL);
}

static void test_update_job_longer_than_max(void)
{
	char expected_status[STATUS_MAX_LEN] = "12345678901";
	char encoded[] = "{\"status\":\"12345678901234567890\","
			  "\"statusDetails\":{\"nextState\":"
			  "\"download_firmware\"},\"expectedVersion\":"
			  "\"1\",\"clientToken\": \"\"}";
	char status[100];
	int ret;

	memset(status, 0xff, sizeof(status));

	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
			sizeof(encoded) - 1, status);
	zassert_true(!strcmp(status, expected_status), NULL);
}


static void test_timestamp_only(void)
{
	int ret;
	char job_id[100];
	char hostname[100];
	char file_path[100];
	char encoded[] = "{\"timestamp\":1559808907}";
	int version_number;

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname,
						      file_path,
						      &version_number);

	zassert_equal(ret, 0, "Timestamp decoded correctly");
}

static void test_update_job_exec_rsp_minimal(void)
{
	char encoded[] = "{\"timestamp\":4096,\"clientToken\":\"token\"}";
	char status[100];
	int ret;

	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
						    sizeof(encoded) - 1,
						    status);

	/* Only two last fields are set */
	zassert_equal(ret, -ENODATA, "All fields decoded correctly");
}

static void test_update_job_exec_rsp(void)
{
	char expected_status[] = "IN_PROGRESS";
	char encoded[] = "{\"status\":\"IN_PROGRESS\",\"statusDetails\":{\"nextState\":\"download_firmware\"},\"expectedVersion\":\"1\",\"clientToken\": \"\"}";
	char status[100];
	int ret;

	memset(status, 0xff, sizeof(status));

	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
						    sizeof(encoded) - 1,
						    status);
	zassert_true(!strcmp(status, expected_status), NULL);
}


void test_main(void)
{
	ztest_test_suite(lib_json_test,
			 ztest_unit_test(test_update_job_exec_rsp_minimal),
			 ztest_unit_test(test_update_job_exec_rsp),
			 ztest_unit_test(test_update_job_longer_than_max),
			 ztest_unit_test(test_timestamp_only),
			 ztest_unit_test(test_parse_job_execution)
			 );

	ztest_run_test_suite(lib_json_test);
}
