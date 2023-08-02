/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <unity.h>
#include <aws_fota_json.h>

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

void test_parse_job_execution(void)
{
	int ret;
	int version_number;
	int expected_version_number = 1;
	char expected_job_id[] = "9b5caac6-3e8a-45dd-9273-c1b995762f4a";
	char expected_hostname[] = "fota-update-bucket.s3.eu-central-1.amazonaws.com";
	char expected_file_path[] = "/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host";
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";
	char job_id[100];
	char hostname[100];
	char file_path[1000];

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL_STRING(expected_job_id, job_id);
	TEST_ASSERT_EQUAL(expected_version_number, version_number);
	TEST_ASSERT_EQUAL_STRING(expected_hostname, hostname);
	TEST_ASSERT_EQUAL_STRING(expected_file_path, file_path);
}

void test_parse_job_execution_single_url(void)
{
	int ret;
	int version_number;
	int expected_version_number = 1;
	char expected_job_id[] = "9b5caac6-3e8a-45dd-9273-c1b995762f4a";
	char expected_hostname[] = "fota-update-bucket.s3.eu-central-1.amazonaws.com";
	char expected_file_path[] = "update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential="
								"AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request"
								"&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature="
								"913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc"
								"&X-Amz-SignedHeaders=host";
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":"
					 "\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\","
					 "\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,"
					 "\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\","
					 "\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\","
					 "\"url\":\"https://fota-update-bucket.s3.eu-central-1.amazonaws.com/"
					 "update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256"
					 "&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu"
					 "-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z"
					 "&X-Amz-Expires=604800"
					 "&X-Amz-Signature="
					 "913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc"
					 "&X-Amz-SignedHeaders=host\"}}}}";
	char job_id[100];
	char hostname[100];
	char file_path[1000];

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL_STRING(expected_job_id, job_id);
	TEST_ASSERT_EQUAL(expected_version_number, version_number);
	TEST_ASSERT_EQUAL_STRING(expected_hostname, hostname);
	TEST_ASSERT_EQUAL_STRING(expected_file_path, file_path);
}

void test_parse_malformed_job_execution(void)
{
	int ret;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	int version_number;
	char malformed[] = "{\"timestamp\":1559808907,\"execution\":{jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\"\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";

	ret = aws_fota_parse_DescribeJobExecution_rsp(malformed,
						      sizeof(malformed) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_parse_job_execution_missing_host_field(void)
{
	int ret;
	int version_number;
	char job_id[100];
	char hostname[100];
	char file_path[1000];

	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_parse_job_execution_missing_path_field(void)
{
	int ret;
	int version_number;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\"}}}}";

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_parse_job_execution_missing_job_id_field(void)
{
	int ret;
	int version_number;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_parse_job_execution_missing_location_obj(void)
{
	int ret;
	int version_number;
	char job_id[100];
	char hostname[100];
	char file_path[1000];
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124}}}}";

	ret = aws_fota_parse_DescribeJobExecution_rsp(encoded,
						      sizeof(encoded) - 1,
						      job_id,
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_update_job_longer_than_max(void)
{
	int ret;
	char status[100];
	char expected_status[STATUS_MAX_LEN] = "12345678901";
	char encoded[] = "{\"status\":\"12345678901234567890\","
			  "\"statusDetails\":{\"nextState\":"
			  "\"download_firmware\"},\"expectedVersion\":"
			  "\"1\",\"clientToken\": \"\"}";


	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
			sizeof(encoded) - 1, status);
	TEST_ASSERT_EQUAL_STRING(expected_status, status);
}


void test_timestamp_only(void)
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
						      hostname, sizeof(hostname),
						      file_path, sizeof(file_path),
						      &version_number);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_update_job_exec_rsp_minimal(void)
{
	char encoded[] = "{\"timestamp\":4096,\"clientToken\":\"token\"}";
	char status[100];
	int ret;

	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
						    sizeof(encoded) - 1,
						    status);
	/* Only two last fields are set */
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_update_job_exec_rsp(void)
{
	int ret;
	char status[100];
	char expected_status[] = "IN_PROGRESS";
	char encoded[] = "{\"status\":\"IN_PROGRESS\",\"statusDetails\":{\"nextState\":\"download_firmware\"},\"expectedVersion\":\"1\",\"clientToken\": \"\"}";

	ret = aws_fota_parse_UpdateJobExecution_rsp(encoded,
						    sizeof(encoded) - 1,
						    status);
	TEST_ASSERT_EQUAL_STRING(expected_status, status);
}


int main(void)
{
	(void)unity_main();
	return 0;
}
