/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

#include <zephyr.h>
#include <zephyr/types.h>
#include <net/mqtt.h>


/** @brief The max JOB_ID_LEN according to AWS docs
 * https://docs.aws.amazon.com/general/latest/gr/aws_service_limits.html
 */
#define JOB_ID_MAX_LEN (64)
#define STATUS_MAX_LEN (12)


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

/**
 * @name String templates for constructing AWS Jobs topics and their max
 * length
 * @{
 */
#define AWS "$aws/things/"
#define AWS_LEN (sizeof(AWS) - 1)
/** @} */

/**
 * @brief String template for constructing AWS Jobs notify topic.
 *
 * @param[in] client_id Client ID of the connected MQTT session.
 */
#define NOTIFY_TOPIC_TEMPLATE AWS "%s/jobs/notify"
#define NOTIFY_TOPIC_MAX_LEN (AWS_LEN +\
			      CONFIG_CLIENT_ID_MAX_LEN +\
			      (sizeof("/jobs/notify") - 1))

/**
 * @brief String template for constructing AWS Jobs notify-next topic.
 *
 * @param[in] client_id Client ID of the connected MQTT session.
 */
#define NOTIFY_NEXT_TOPIC_TEMPLATE AWS "%s/jobs/notify-next"
#define NOTIFY_NEXT_TOPIC_MAX_LEN (AWS_LEN +\
				   CONFIG_CLIENT_ID_MAX_LEN +\
				   (sizeof("/jobs/notify-next") - 1))

/**
 * @brief String template for constructing AWS Jobs get jobs topics.
 *
 * @param[in] client_id  Client ID of the connected MQTT session.
 * @param[in] identifier The identifier of the topic you want to subscribe to.
 * This can be either "accepted", "rejected", or "#" for both.
 */
#define GET_TOPIC_TEMPLATE AWS "%s/jobs/get/%s"
#define GET_TOPIC_LEN (AWS_LEN +\
		       CONFIG_CLIENT_ID_MAX_LEN +\
		       (sizeof("/jobs/get/accepted") - 1))

/**
 * @brief String template for constructing AWS Jobs get topics for a specific
 *        job.
 *
 * @param[in] client_id  Client ID of the connected MQTT session.
 * @param[in] job_id     Job ID of the currently received job.
 * @param[in] identifier The identifier of the topic you want to subscribe to.
 *	This can be either "accepted", "rejected", or "#" for both.
 */
#define JOB_ID_GET_TOPIC_TEMPLATE AWS "%s/jobs/%s/get/%s"
#define JOB_ID_GET_TOPIC_MAX_LEN (AWS_LEN +\
				 CONFIG_CLIENT_ID_MAX_LEN +\
				 JOB_ID_MAX_LEN +\
				 (sizeof("/jobs//get/accepted") - 1))

/**
 * @brief String template for constructing AWS Jobs update topic for receiving
 *        and publishing updates to the job document status and information.
 *
 * @param[in] client_id Client ID of the connected MQTT session.
 * @param[in] job_id    Job ID of the currently received job.
 */
#define JOB_ID_UPDATE_TOPIC_TEMPLATE AWS "%s/jobs/%s/update%s"
#define JOB_ID_UPDATE_TOPIC_MAX_LEN (AWS_LEN +\
				     CONFIG_CLIENT_ID_MAX_LEN +\
				     JOB_ID_MAX_LEN +\
				     (sizeof("/jobs//update/accepted") - 1))

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

/**
 * @brief Construct the get topic for accepted/rejected AWS Jobs and subscribe
 *	      to it.
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *	               snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_get(struct mqtt_client *const client);


/**
 * @brief Construct the get topic for accepted/rejected AWS Jobs
 *        and unsubscribe.
 *
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0 If successful, otherwise the return code of mqtt_subscribe or
 *	         snprintf.
 */
int aws_jobs_unsubscribe_get(struct mqtt_client *const client);


/**
 * @brief Construct the notify-next topic and subscribe to it for receiving
 *	      AWS IoT jobs.
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_notify_next(struct mqtt_client *const client);


/**
 * @brief Construct the notify-next topic and unsubscribe from it to stop
 *	      receiving AWS IoT jobs.
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_unsubscribe_notify_next(struct mqtt_client *const client);


/**
 * @brief Construct the notify topic and subscribe to it for receiving
 *	      AWS IoT jobs lists.
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *	               snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_notify(struct mqtt_client *const client);


/**
 * @brief Construct the notify topic and unsubscribe to stop receiving
 *	      AWS IoT jobs lists.
 *
 * @param[in] client Connected MQTT client instance.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_unsubscribe_notify(struct mqtt_client *const client);


/**@brief Subscribe to the expected topics of the AWS documentation that does
 *	      not require a jobId in the topic field.
 *	https://docs.aws.amazon.com/iot/latest/developerguide/jobs-devices.html
 *
 * @param[in] client      Connected MQTT client instance.
 * @param[in] notify_next Subscribe to notify-next if true and notify if false.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *	               snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_expected_topics(struct mqtt_client *const client,
				       bool notify_next);


/**@brief Unsubscribe from the expected topics of the AWS documentation
 *        that does not require a jobId in the topic field.
 *
 * https://docs.aws.amazon.com/iot/latest/developerguide/jobs-devices.html
 *
 * @param[in] client      Connected MQTT client instance.
 * @param[in] notify_next Subscribe to notify-next if true and notify if false.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_unsubscribe_expected_topics(struct mqtt_client *const client,
					 bool notify_next);


/**
 * @brief Construct the get topic for a job ID and subscribe to it for both
 *	      accepted and rejected.
 *
 * @param[in] client Connected MQTT client instance.
 * @param[in] job_id Job ID of the currently accepted job.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *	               snprintf.
 * @retval -EINVAL If the provided input parameter is not valid.
 */
int aws_jobs_subscribe_job_id_get_topic(struct mqtt_client *const client,
					const u8_t *job_id);


/**
 * @brief Construct the get topic for a job ID and unsubscribe from it for both
 *	      accepted and rejected.
 *
 * @param[in] client Connected MQTT client instance.
 * @param[in] job_id Job ID of the currently accepted job.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_unsubscribe_job_id_get_topic(struct mqtt_client *const client,
					  const u8_t *job_id);


/**
 * @brief Construct the update topic for a job ID and subscribe to rejected and
 *	      accepted to receive feedback when submitting a job execution
 *	      update.
 *
 * @param[in] client Connected MQTT client instance.
 * @param[in] job_id Job ID of the currently accepted job.
 *
 * @retval 0       If successful, otherwise the return code of mqtt_subscribe or
 *	               snprintf.
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_subscribe_job_id_update(struct mqtt_client *const client,
				     const u8_t *job_id);


/**
 * @brief Construct the update topic for a job ID and unsubscribe from rejected
 *	      and accepted to stop receiving feedback from the job execution.
 *
 * @param[in] client Connected MQTT client instance.
 * @param[in] job_id Job ID of the currently accepted job.
 *
 * @retval 0       If successful, otherwise the return code of
 *                 mqtt_unsubscribe or snprintf.
 *
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_unsubscribe_job_id_update(struct mqtt_client *const client,
				       const u8_t *job_id);

/**
 * @brief AWS Jobs update job execution status function.
 *
 * This implements the minimal requirements for running an AWS job.
 * Only updating status and status details is supported.
 *
 * @param[in] client           Initialized and connected MQTT Client instance.
 * @param[in] job_id           The ID of the job which you are updating.
 *		                 This is apart of the MQTT topic used to update the job.
 * @param[in] status           The job execution status.
 * @param[in] status_details   JSON object in string format containing
 *		                   additional information. This object can contain up to
 *		                  10 fields according to the AWS IoT Jobs documentation.
 * @param[in] expected_version The expected job document version. This needs to
 *		                       be incremented by 1 for every update.
 * @param[in] client_token     This can be an arbitrary value and
 *		                       will be reflected in the response to the update.
 *
 * @retval 0       If the update is published successfully, otherwise return
 *	               mqtt_publish error code or the error code of snprintf.
 * @retval -EINVAL If the provided input parameters are not valid.
 */
int aws_jobs_update_job_execution(struct mqtt_client *const client,
			     const u8_t *job_id,
			     enum execution_status status,
			     const u8_t *status_details,
			     int expected_version,
			     const u8_t *client_token);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* AWS_JOBS_H__ */

