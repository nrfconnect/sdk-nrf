/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <data/json.h>
#include <sys/util.h>
#include <net/aws_jobs.h>

#include "aws_fota_json.h"

struct location_obj {
	const char *protocol;
	const char *host;
	const char *path;
};

struct job_document_obj {
	const char *operation;
	const char *fw_version;
	int size;
	struct location_obj location;
};

struct status_details_obj {
	const char *next_state;
};

struct execution_state_obj {
	const char *status;
	struct status_details_obj status_details;
	int version_number;
};

struct execution_obj {
	const char *job_id;
	const char *status;
	struct status_details_obj status_details;
	int queued_at;
	int last_update_at;
	int version_number;
	int execution_number;
	struct job_document_obj job_document;
};

struct desc_job_execution_obj {
	const char *client_token;
	int timestamp;
	struct execution_obj execution;
};

static const struct json_obj_descr location_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct location_obj, protocol, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct location_obj, host, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct location_obj, path, JSON_TOK_STRING),
};

static const struct json_obj_descr job_document_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct job_document_obj, operation,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct job_document_obj, "fwversion",
				  fw_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct job_document_obj, size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct job_document_obj, location,
			      location_obj_descr),
};

static const struct json_obj_descr status_details_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct status_details_obj, "nextState",
				  next_state, JSON_TOK_STRING),
};

static const struct json_obj_descr execution_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj, "jobId", job_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct execution_obj, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct execution_obj, "statusDetails",
				    status_details, status_details_obj_descr),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj, "queuedAt", queued_at,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj, "lastUpdatedAt",
				  last_update_at, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj, "versionNumber",
				  version_number, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj, "executionNumber",
				  execution_number, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct execution_obj, "jobDocument",
				    job_document, job_document_obj_descr),
};

static const struct json_obj_descr desc_job_execution_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct desc_job_execution_obj, "clientToken",
				  client_token, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct desc_job_execution_obj, timestamp,
			    JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct desc_job_execution_obj, execution,
			      execution_obj_descr),
};

struct update_rsp_obj {
	const char *status;
	struct status_details_obj status_details;
	const char *expected_version;
	int timestamp;
	const char *client_token;
};

static const struct json_obj_descr update_job_exec_stat_rsp_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct update_rsp_obj, "status",
				  status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct update_rsp_obj, "statusDetails",
				    status_details, status_details_obj_descr),
	JSON_OBJ_DESCR_PRIM_NAMED(struct update_rsp_obj, "expectedVersion",
				  expected_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct update_rsp_obj, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct update_rsp_obj, "clientToken",
				  client_token, JSON_TOK_STRING),
};

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

int aws_fota_parse_UpdateJobExecution_rsp(char *update_rsp_document,
					  size_t payload_len, char *status)
{
	if (update_rsp_document == NULL || status == NULL) {
		return -EINVAL;
	}

	struct update_rsp_obj rsp;

	int ret = json_obj_parse(update_rsp_document, payload_len,
			update_job_exec_stat_rsp_descr,
			ARRAY_SIZE(update_job_exec_stat_rsp_descr), &rsp);

	if (ret < 0) {
		return ret;
	}

	/* Check if the status field(1st field) of the object has been parsed */
	if (ret & BIT(UPDATE_JOB_EXECUTION_STATUS_DECODED_BIT)) {
		strncpy_nullterm(status, rsp.status, STATUS_MAX_LEN);
	} else {
		return -ENODATA;
	}

	return 0;
}

int aws_fota_parse_DescribeJobExecution_rsp(char *job_document,
					   u32_t payload_len,
					   char *job_id_buf,
					   char *hostname_buf,
					   char *file_path_buf,
					   int *version_number)
{
	if (job_document == NULL
	    || job_id_buf == NULL
	    || hostname_buf == NULL
	    || file_path_buf == NULL
	    || version_number == NULL) {
		return -EINVAL;
	}

	struct desc_job_execution_obj root_obj;
	struct job_document_obj *job_doc_obj;
	struct execution_obj *exec_obj;

	int ret = json_obj_parse(job_document, payload_len,
				 desc_job_execution_obj_descr,
				 ARRAY_SIZE(desc_job_execution_obj_descr),
				 &root_obj);

	if (ret < 0) {
		return ret;
	}

	if (!(ret & BIT(EXECUTION_OBJ_DECODED_BIT))) {
		/* No execution object in JSON */
		return 0;
	}

	/* Check if the execution field of the object has been parsed. This
	 * test is currently non-functional as Zephyr's JSON parser does not
	 * populate fields as NULL in nested structure if the field in the
	 * nested structure is not found.
	 */
	exec_obj = &root_obj.execution;
	job_doc_obj = &exec_obj->job_document;
	if (exec_obj->job_id != 0
	    && exec_obj->version_number != 0
	    && job_doc_obj->location.host != 0
	    && job_doc_obj->location.path != 0) {
		strncpy_nullterm(job_id_buf, exec_obj->job_id,
				AWS_JOBS_JOB_ID_MAX_LEN);
		*version_number = exec_obj->version_number;
		strncpy_nullterm(hostname_buf,
				job_doc_obj->location.host,
				CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN);
		strncpy_nullterm(file_path_buf,
				job_doc_obj->location.path,
				CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN);
	} else {
		/* A field was not decoded correctly which means we are missing
		 * data.
		 */
		return -ENODATA;
	}

	return 1;
}
