/*
 *Copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file aws_fota_json.h
 *
 * @defgroup aws_fota_json
 * @{
 * @brief  Library for parsing AWS Jobs json payloads.
 *
 */

#ifndef AWS_FOTA_JSON_H__
#define AWS_FOTA_JSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/** @brief The max JOB_ID_LEN according to AWS docs
 * https://docs.aws.amazon.com/general/latest/gr/aws_service_limits.html
 */
#define STATUS_MAX_LEN (12)

/**
 * @brief The bit set when the status field in a job execution object is
 * decoded.
 */
#define UPDATE_JOB_EXECUTION_STATUS_DECODED_BIT 1
/**
 * @brief The bit set when the Execution Description object is decoded.
 */
#define EXECUTION_OBJ_DECODED_BIT 2

/** @brief AWS FOTA JSON parse result. */
enum aws_fota_json_result {
	AWS_FOTA_JSON_RES_SKIPPED = 1,		/*!< Not a FOTA job document, skipped */
	AWS_FOTA_JSON_RES_SUCCESS = 0,		/*!< Job document parsed successfully */
	AWS_FOTA_JSON_RES_INVALID_PARAMS = -1,	/*!< Input parameters invalid */
	AWS_FOTA_JSON_RES_INVALID_JOB = -2,	/*!< Job document invalid, could not get job id */
	AWS_FOTA_JSON_RES_INVALID_DOCUMENT = -3,/*!< FOTA update data invalid */
	AWS_FOTA_JSON_RES_URL_TOO_LONG = -4,	/*!< Parts of URL too large for buffer */
};

/**
 * @brief Parse a given AWS IoT DescribeJobExecution response JSON object.
 *	  More information on this object can be found at https://docs.aws.amazon.com/iot/latest/developerguide/jobs-api.html#mqtt-describejobexecution
 *
 * @param[in] job_document  JSON formated string of an AWS IoT
 *			    DescribeJobExecution response.
 * @param[in] payload_len  The length of the provided string.
 * @param[out] job_id_buf  Output buffer for the "jobId" field from the
 *			   JobExecution data type.
 * @param[out] hostname_buf  Output buffer for the "host" field from the Job
 *			     Document
 * @param[in] hostname_buf_size  Size of the output buffer for the "host" field
 * @param[out] file_path_buf  Output buffer for the "file" field from the Job
 *			      Document
 * @param[in] file_path_buf_size  Size of the output buffer for the "file" field
 * @param[out] version_number  Version number from the Job Execution data type.
 *
 * @return aws_fota_json_result specifying the result of the parse operation.
 **/
int aws_fota_parse_DescribeJobExecution_rsp(const char *job_document,
					    uint32_t payload_len,
					    char *job_id_buf,
					    char *hostname_buf,
					    size_t hostname_buf_size,
					    char *file_path_buf,
					    size_t file_path_buf_size,
					    int *version_number);

/**
 * @brief Parse a Job Execution accepted response object. Returned by a call to
 *	  https://docs.aws.amazon.com/iot/latest/developerguide/jobs-api.html#mqtt-updatejobexecution
 *
 * @param[in] job_document  JSON formated string of an AWS IoT Job execution
 *			    update response.
 * @param[in] payload_len  The length of the provided string.
 * @param[out] status  String with the AWS Job status after an update.

 * @return 0 for an successful parsing or a negative error code identicating
 *	   reason of failure.
 */
int aws_fota_parse_UpdateJobExecution_rsp(const char *update_rsp_document,
					  size_t payload_len,
					  char *status);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /*AWS_FOTA_JSON_H__ */
