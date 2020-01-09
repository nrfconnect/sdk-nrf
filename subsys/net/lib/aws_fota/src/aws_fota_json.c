/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <json.h>
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

struct execution_obj {
	const char *job_id;
	const char *status;
	int queued_at;
	int last_update_at;
	int version_number;
	int execution_number;
	struct job_document_obj job_document;
};

struct notify_next_obj {
	int timestamp;
	struct execution_obj execution;
};

struct status_details_obj {
	const char *next_state;
};

struct execution_state_obj {
	const char *status;
	struct status_details_obj status_details;
	int version_number;
};

static const struct json_obj_descr location_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct location_obj, protocol, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct location_obj, host, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct location_obj, path, JSON_TOK_STRING),
};

static const struct json_obj_descr job_document_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct job_document_obj,
			    operation,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct job_document_obj,
				  "fwversion",
				  fw_version,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct job_document_obj, size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct job_document_obj,
			      location,
			      location_obj_descr),
};

static const struct json_obj_descr execution_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj,
				  "jobId",
				  job_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct execution_obj, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj,
				  "queuedAt",
				  queued_at,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj,
				  "lastUpdatedAt",
				  last_update_at,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj,
				  "versionNumber",
				  version_number,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct execution_obj,
				  "executionNumber",
				  execution_number,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct execution_obj,
				    "jobDocument",
				    job_document,
				    job_document_obj_descr),
};

static const struct json_obj_descr notify_next_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct notify_next_obj, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct notify_next_obj,
			      execution,
			      execution_obj_descr),
};

static const struct json_obj_descr status_details_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct status_details_obj,
				  "nextState",
				  next_state,
				  JSON_TOK_STRING),
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

	JSON_OBJ_DESCR_OBJECT_NAMED(struct update_rsp_obj,
				    "statusDetails",
				    status_details,
				    status_details_obj_descr),

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

int aws_fota_parse_update_job_exec_state_rsp(char *update_rsp_document,
		size_t payload_len, char *status)
{
	struct update_rsp_obj rsp;

	int ret = json_obj_parse(update_rsp_document, payload_len,
			update_job_exec_stat_rsp_descr,
			ARRAY_SIZE(update_job_exec_stat_rsp_descr), &rsp);

	/* Check if the status field(1st field) of the object has been parsed */
	if (ret & 0x01) {
		strncpy_nullterm(status, rsp.status, STATUS_MAX_LEN);
	}

	return ret;
}

int aws_fota_parse_notify_next_document(char *job_document,
		u32_t payload_len, char *job_id_buf, char *hostname_buf,
		char *file_path_buf)
{
	struct notify_next_obj job;
	struct job_document_obj *job_doc_obj;

	int ret = json_obj_parse(job_document,
				 payload_len,
				 notify_next_obj_descr,
				 ARRAY_SIZE(notify_next_obj_descr),
				 &job);
	job_doc_obj = &job.execution.job_document;

	/* Check if the execution field of the object has been parsed */
	if (ret & 0x02) {
		if (job.execution.job_id != 0) {
			strncpy_nullterm(job_id_buf, job.execution.job_id,
				      AWS_JOBS_JOB_ID_MAX_LEN);
		}
		if (job_doc_obj->location.host != 0) {
			strncpy_nullterm(hostname_buf,
					 job_doc_obj->location.host,
					 CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN);
		}
		if (job_doc_obj->location.path != 0) {
			strncpy_nullterm(file_path_buf,
					 job_doc_obj->location.path,
					  CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN);
		}

	}
	return ret;
}
