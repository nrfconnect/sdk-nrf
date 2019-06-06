/*
 *Copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

#include <zephyr.h>
#include <zephyr/types.h>

/** @brief The max JOB_ID_LEN according to AWS docs
 * https://docs.aws.amazon.com/general/latest/gr/aws_service_limits.html
 */
#define JOB_ID_MAX_LEN (64)
#define STATUS_MAX_LEN (12)

int aws_fota_parse_notify_next_document(char *job_document,
		u32_t payload_len, char *job_id_buf, char *hostname_buf,
		char *file_path_buf);

int aws_fota_parse_update_job_exec_state_rsp(char *update_rsp_document,
		size_t payload_len, char *status);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /*AWS_FOTA_JSON_H__ */
