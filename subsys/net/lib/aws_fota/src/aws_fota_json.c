/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <cJSON.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/http/parser_url.h>
#include <net/aws_jobs.h>

#include "aws_fota_json.h"

/**@brief Copy max maxlen bytes from src to dst. Insert null-terminator.
 */
static void strncpy_nullterm(char *dst, const char *src, size_t maxlen)
{
	size_t len = strlen(src) + 1;

	memcpy(dst, src, MIN(len, maxlen));
	if (len > maxlen) {
		dst[maxlen - 1] = '\0';
	}
}

int aws_fota_parse_UpdateJobExecution_rsp(const char *update_rsp_document,
					  size_t payload_len, char *status_buf)
{
	if (update_rsp_document == NULL || status_buf == NULL) {
		return -EINVAL;
	}

	int ret;

	cJSON *update_response = cJSON_Parse(update_rsp_document);
	if (update_response == NULL) {
		ret = -ENODATA;
		goto cleanup;
	}

	cJSON *status = cJSON_GetObjectItemCaseSensitive(update_response, "status");
	if (cJSON_IsString(status) && status->valuestring != NULL) {
		strncpy_nullterm(status_buf, status->valuestring,
				 STATUS_MAX_LEN);
	} else {
		ret = -ENODATA;
		goto cleanup;
	}

	ret = 0;
cleanup:
	cJSON_Delete(update_response);
	return ret;
}

int aws_fota_parse_DescribeJobExecution_rsp(const char *job_document,
					   uint32_t payload_len,
					   char *job_id_buf,
					   char *hostname_buf,
					   size_t hostname_buf_size,
					   char *file_path_buf,
					   size_t file_path_buf_size,
					   int *execution_version_number)
{
	if (job_document == NULL
	    || job_id_buf == NULL
	    || hostname_buf == NULL
	    || file_path_buf == NULL
	    || execution_version_number == NULL) {
		return AWS_FOTA_JSON_RES_INVALID_PARAMS;
	}

	int ret;

	cJSON *json_data = cJSON_Parse(job_document);

	if (json_data == NULL) {
		ret = AWS_FOTA_JSON_RES_INVALID_JOB;
		goto cleanup;
	}

	cJSON *execution = cJSON_GetObjectItemCaseSensitive(json_data, "execution");

	if (execution == NULL) {
		ret = AWS_FOTA_JSON_RES_SKIPPED;
		goto cleanup;
	}

	cJSON *job_id = cJSON_GetObjectItemCaseSensitive(execution, "jobId");

	if (cJSON_GetStringValue(job_id) != NULL) {
		strncpy_nullterm(job_id_buf, job_id->valuestring, AWS_JOBS_JOB_ID_MAX_LEN);
	} else {
		ret = AWS_FOTA_JSON_RES_INVALID_JOB;
		goto cleanup;
	}

	cJSON *version_number = cJSON_GetObjectItemCaseSensitive(execution, "versionNumber");

	if (cJSON_IsNumber(version_number)) {
		*execution_version_number = version_number->valueint;
	} else {
		ret = AWS_FOTA_JSON_RES_INVALID_JOB;
		goto cleanup;
	}

	cJSON *job_data = cJSON_GetObjectItemCaseSensitive(execution, "jobDocument");

	if (!cJSON_IsObject(job_data)) {
		ret = AWS_FOTA_JSON_RES_INVALID_DOCUMENT;
		goto cleanup;
	}

	cJSON *location = cJSON_GetObjectItemCaseSensitive(job_data, "location");

	if (!cJSON_IsObject(location)) {
		ret = AWS_FOTA_JSON_RES_INVALID_DOCUMENT;
		goto cleanup;
	}

	cJSON *hostname = cJSON_GetObjectItemCaseSensitive(location, "host");
	cJSON *path = cJSON_GetObjectItemCaseSensitive(location, "path");
	cJSON *url = cJSON_GetObjectItemCaseSensitive(location, "url");

	if (cJSON_GetStringValue(url) != NULL) {
		struct http_parser_url u;

		http_parser_url_init(&u);
		http_parser_parse_url(url->valuestring, strlen(url->valuestring), false, &u);

		/* Determine size of hostname and path (consisting of path + query).
		 * Length increased by one to get null termination at right spot when copying.
		 */
		uint16_t parsed_host_len = u.field_data[UF_HOST].len + 1;
		uint16_t parsed_file_len = u.field_data[UF_PATH].len
					 + u.field_data[UF_QUERY].len + 1;

		if (parsed_host_len > hostname_buf_size || parsed_file_len > file_path_buf_size) {
			ret = AWS_FOTA_JSON_RES_URL_TOO_LONG;
			goto cleanup;
		}

		/* Copy slices of the hostname and path (url path + query) to the
		 * respective buffers. Increase path offset by one to omit initial slash.
		 */
		strncpy_nullterm(hostname_buf,
				url->valuestring + u.field_data[UF_HOST].off,
				parsed_host_len);
		strncpy_nullterm(file_path_buf,
				url->valuestring + u.field_data[UF_PATH].off + 1,
				parsed_file_len);

	} else if ((cJSON_GetStringValue(hostname) != NULL)
	   && (cJSON_GetStringValue(path) != NULL)) {

		if (strlen(hostname->valuestring) >= hostname_buf_size
		    || strlen(path->valuestring) >= file_path_buf_size) {
			ret = AWS_FOTA_JSON_RES_URL_TOO_LONG;
			goto cleanup;
		}

		strncpy_nullterm(hostname_buf, hostname->valuestring, hostname_buf_size);
		strncpy_nullterm(file_path_buf, path->valuestring, file_path_buf_size);
	} else {
		ret = AWS_FOTA_JSON_RES_INVALID_DOCUMENT;
		goto cleanup;
	}

	ret = AWS_FOTA_JSON_RES_SUCCESS;
cleanup:
	cJSON_Delete(json_data);
	return ret;
}
