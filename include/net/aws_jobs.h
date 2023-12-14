/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 */

#ifndef AWS_JOBS_H__
#define AWS_JOBS_H__

/**
 * @defgroup aws_jobs AWS Jobs library
 * @{
 * @brief Library for interacting with AWS Jobs.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/mqtt.h>

/** @brief Job Execution Status. */
enum execution_status {
	AWS_JOBS_QUEUED = 0,
	AWS_JOBS_IN_PROGRESS,
	AWS_JOBS_SUCCEEDED,
	AWS_JOBS_FAILED,
	AWS_JOBS_TIMED_OUT,
	AWS_JOBS_REJECTED,
	AWS_JOBS_REMOVED,
	AWS_JOBS_CANCELED
};

/** @name MQTT message IDs used for identifying subscribe message ACKs
 * @{
 */
#define SUBSCRIBE_ID_BASE (2110)
#define SUBSCRIBE_NOTIFY (SUBSCRIBE_ID_BASE + 1)
#define SUBSCRIBE_NOTIFY_NEXT (SUBSCRIBE_ID_BASE + 2)
#define SUBSCRIBE_GET (SUBSCRIBE_ID_BASE + 3)
#define SUBSCRIBE_JOB_ID_GET (SUBSCRIBE_ID_BASE + 4)
#define SUBSCRIBE_JOB_ID_UPDATE (SUBSCRIBE_ID_BASE + 5)
#define SUBSCRIBE_EXPECTED (SUBSCRIBE_ID_BASE + 10)
/** @}*/


/** @brief The max JOB_ID_LEN according to AWS docs
 * https://docs.aws.amazon.com/general/latest/gr/aws_service_limits.html
 */
#define AWS "$aws/things/"
#define AWS_LEN (sizeof(AWS) - 1)
#define AWS_JOBS_JOB_ID_MAX_LEN (64 + 1)
#define AWS_JOBS_TOPIC_STATIC_LEN 20 /* '$aws/token' etc... */
#define AWS_JOBS_TOPIC_MAX_LEN (CONFIG_CLIENT_ID_MAX_LEN + \
		AWS_JOBS_TOPIC_STATIC_LEN + AWS_JOBS_JOB_ID_MAX_LEN)

/**
 * @brief Construct the notify-next topic for receiving AWS IoT
 *        jobs.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful.
 * @retval -EINVAL If the provided input parameter is not valid.
 * @retval -ENOMEM If cannot fit in the buffer, which is assumed
 *                 to be AWS_JOBS_TOPIC_MAX_LEN in size.
 */
int aws_jobs_create_topic_notify_next(struct mqtt_client *const client,
					 uint8_t *topic_buf);

/**
 * @brief Construct the notify-next topic and subscribe to it for receiving
 *        AWS IoT jobs.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_subscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_topic_notify_next(struct mqtt_client *const client,
					 uint8_t *topic_buf);

/**
 * @brief Construct the notify-next topic and unsubscribe from it to stop
 *	  receiving AWS IoT jobs.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_unsubscribe_topic_notify_next(struct mqtt_client *const client,
					   uint8_t *topic_buf);

/**
 * @brief Construct the notify topic for receiving AWS IoT jobs.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful.
 * @retval -EINVAL If the provided input parameter is not valid.
 * @retval -ENOMEM If cannot fit in the buffer, which is assumed
 *                 to be AWS_JOBS_TOPIC_MAX_LEN in size.
 */
int aws_jobs_create_topic_notify(struct mqtt_client *const client,
				 uint8_t *topic_buf);

/**
 * @brief Construct the notify topic and subscribe to it for receiving
 *        AWS IoT jobs lists.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *                 snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_topic_notify(struct mqtt_client *const client,
				    uint8_t *topic_buf);

/**
 * @brief Construct the notify topic and unsubscribe to stop receiving
 *	      AWS IoT jobs lists.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_unsubscribe_topic_notify(struct mqtt_client *const client,
				      uint8_t *topic_buf);

/**
 * @brief Construct the get topic for a job ID.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[in]  job_id Job ID of the currently accepted job.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful.
 * @retval -EINVAL If the provided input parameter is not valid.
 * @retval -ENOMEM If cannot fit in the buffer, which is assumed
 *                 to be AWS_JOBS_TOPIC_MAX_LEN in size.
 */
int aws_jobs_create_topic_get(struct mqtt_client *const client,
			      const uint8_t *job_id, uint8_t *topic_buf);

/**
 * @brief Construct the get topic for a job ID and subscribe to it for both
 *        accepted and rejected.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[in]  job_id Job ID of the currently accepted job.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0 If successful, otherwise the return code of mqtt_subscribe or
 *           snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_topic_get(struct mqtt_client *const client,
				 const uint8_t *job_id, uint8_t *topic_buf);

/**
 * @brief Construct the get topic for a job ID and unsubscribe from it for both
 *        accepted and rejected.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[in]  job_id Job ID of the currently accepted job.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_unsubscribe_topic_get(struct mqtt_client *const client,
				   const uint8_t *job_id, uint8_t *topic_buf);

/**
 * @brief Construct the update topic for a job ID and subscribe to rejected and
 *        accepted to receive feedback when submitting a job execution update.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[in]  job_id Job ID of the currently accepted job.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *                 snprintf.
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_subscribe_topic_update(struct mqtt_client *const client,
				    const uint8_t *job_id, uint8_t *topic_buf);

/**
 * @brief Construct the update topic for a job ID and unsubscribe from rejected
 *        and accepted to stop receiving feedback from the job execution.
 *
 * @param[in]  client Connected MQTT client instance.
 * @param[in]  job_id Job ID of the currently accepted job.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_unsubscribe_topic_update(struct mqtt_client *const client,
				      const uint8_t *job_id, uint8_t *topic_buf);

/**
 * @brief AWS Jobs update job execution status function.
 *
 * This implements the minimal requirements for running an AWS job.
 * Only updating status and status details is supported.
 *
 * @param[in] client           Initialized and connected MQTT Client instance.
 * @param[in] job_id           The ID of the job which you are updating.
 * @param[in] status           The job execution status.
 * @param[in] status_details   JSON object in string format containing
 *                             additional information. Max 10 fields.
 *                             Can be NULL.
 * @param[in] expected_version The expected job document version. Must be
 *                             incremented by 1 for every update.
 * @param[in] client_token     This can be an arbitrary value and will be
 *                             reflected in the response to the update.
 * @param[out] topic_buf       Buffer to store the topic in.
 *
 * @retval 0       If the update is published successfully, otherwise return
 *                 mqtt_publish error code or the error code of snprintf.
 *
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_update_job_execution(struct mqtt_client *const client,
				  const uint8_t *job_id,
				  enum execution_status status,
				  const uint8_t *status_details,
				  int expected_version,
				  const uint8_t *client_token,
				  uint8_t *topic_buf);

/**
 * @brief AWS Jobs get job execution.
 *
 * This implements fetching a job from the queue in AWS.
 *
 * @param[in]  client Initialized and connected MQTT Client instance.
 * @param[in]  job_id The ID of the job you are fetching use "$next" for
 *                    fetching the next IN_PROGRESS. or QUEUED job. It will
 *                    fetch the first created job if multiple exists.
 * @param[out] topic_buf Buffer to store the topic in.
 *
 * @retval 0       If the update is published successfully, otherwise return
 *                 mqtt_publish error code or the error code of snprintf.
 */
int aws_jobs_get_job_execution(struct mqtt_client *const client,
			       const char *job_id, uint8_t *topic_buf);

/**
 * @brief Compare topics
 *
 * Check if topics match
 *
 * @param[in] sub     Topic subscribed to
 * @param[in] pub     Published topic
 * @param[in] pub_len Length of published topic
 * @param[in] suffix  Suffix to match. Cannot be NULL. Must be empty string
 *                    for sub topics not ending with '#'.
 *
 * @return true if topics with given suffix match, false otherwise.
 */
bool aws_jobs_cmp(const char *sub, const char *pub, size_t pub_len,
		  const uint8_t *suffix);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* AWS_JOBS_H__ */
