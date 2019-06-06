/*
 *copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <stdio.h>
#include <net/mqtt.h>
#include <logging/log.h>
#include <net/aws_jobs.h>

LOG_MODULE_REGISTER(aws_jobs, CONFIG_AWS_JOBS_LOG_LEVEL);

/** @brief Mapping of enum to strings for Job Execution Status. */
static const char *execution_status_strings[] = {
	[AWS_JOBS_QUEUED] = "QUEUED",
	[AWS_JOBS_IN_PROGRESS] = "IN_PROGRESS",
	[AWS_JOBS_SUCCEEDED] = "SUCCEEDED",
	[AWS_JOBS_FAILED] = "FAILED",
	[AWS_JOBS_TIMED_OUT] = "TIMED_OUT",
	[AWS_JOBS_REJECTED] = "REJECTED",
	[AWS_JOBS_REMOVED] = "REMOVED",
	[AWS_JOBS_CANCELED] = "CANCELED"
};

/**
 * @brief Local function used to error check snprintf outputs.
 *
 * @param[in] ret Return value of snpritnf.
 * @param[in] size Max size of what would have been the final string.
 * @param[in] entry Name of the function it was called from.
 *
 * @retval 0 If sucessful
 * @retval -ENOMEM If the buffer was too small for the string. Otherwise
 *	the error code of snprintf if the formating failed.
 */
static inline int report_snprintf_err(u32_t ret, u32_t size, const char *entry)
{
	if (ret >= size) {
		LOG_ERR("Unable to fit formated string into to allocate "
			"memory for %s", log_strdup(entry));
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Output error for %s was encountered with return value "
			"%d", log_strdup(entry), ret);
		return ret;
	}
	return 0;
}


/**
 * @brief Constructs the get topic for accepted and rejeceted AWS jobs.
 *
 * @param[in] client_id Client ID of the current connected MQTT session.
 * @param[in] topic_buf buffer for storing the topic string until mqtt_subscribe
 *	is called.
 * @param[out] topic Outputs the constructed mqtt_topic into the given buffer.
 *
 * @retval 0 If sucessful otherwise the return value of report_snprintf_err
 */
static int construct_get_topic(const u8_t *client_id,
			       u8_t *topic_buf,
			       struct mqtt_topic *topic)
{
	u32_t get_topic_len;
	int ret = snprintf(topic_buf,
			   GET_TOPIC_LEN,
			   GET_TOPIC_TEMPLATE,
			   client_id,
			   "#");
	get_topic_len = ret;
	ret = report_snprintf_err(ret, GET_TOPIC_LEN, "get_topic");

	if (ret) {
		return ret;
	}

	topic->topic.utf8 = topic_buf;
	topic->topic.size = get_topic_len;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}


int aws_jobs_subscribe_get(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t get_topic[GET_TOPIC_LEN + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_get_topic(client->client_id.utf8,
				      get_topic,
				      &subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_GET
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_subscribe(client, &subscription_list);
}

int aws_jobs_unsubscribe_get(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t get_topic[GET_TOPIC_LEN + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_get_topic(client->client_id.utf8,
				      get_topic,
				      &subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_GET
	};
	LOG_DBG("Unsubscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_unsubscribe(client, &subscription_list);
}



/**
 * @brief Constructs the notify-next topic for reciving AWS IoT jobs.
 *
 * @param[in] client_id Client ID of the current connected MQTT session.
 * @param[in] topic_buf buffer for storing the topic string until mqtt_subscribe
 *	is called.
 * @param[out] topic Outputs the constructed mqtt_topic into the given buffer.
 *
 * @retval 0 If sucessful otherwise the return value of report_snprintf_err
 */
static int construct_notify_next_topic(const u8_t *client_id,
				       u8_t *topic_buf,
				       struct mqtt_topic *topic)
{
	u32_t notify_next_topic_len;
	int ret = snprintf(topic_buf,
			   NOTIFY_NEXT_TOPIC_MAX_LEN,
			   NOTIFY_NEXT_TOPIC_TEMPLATE,
			   client_id);
	notify_next_topic_len = ret;
	ret = report_snprintf_err(ret,
			    NOTIFY_NEXT_TOPIC_MAX_LEN,
			    "notify_next_topic");
	if (ret) {
		return ret;
	}

	topic->topic.utf8 = topic_buf;
	topic->topic.size = notify_next_topic_len;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}


int aws_jobs_subscribe_notify_next(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t notify_next_topic[NOTIFY_NEXT_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_notify_next_topic(client->client_id.utf8,
					      notify_next_topic,
					      &subscribe_topic);

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_NOTIFY_NEXT
	};

	if (err) {
		return err;
	}

	LOG_DBG("%s Subscribe: %s", __func__,
		log_strdup(subscribe_topic.topic.utf8));

	return mqtt_subscribe(client, &subscription_list);
}


int aws_jobs_unsubscribe_notify_next(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t notify_next_topic[NOTIFY_NEXT_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_notify_next_topic(client->client_id.utf8,
					      notify_next_topic,
					      &subscribe_topic);

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_NOTIFY_NEXT
	};

	if (err) {
		return err;
	}

	LOG_DBG("Unsubscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_unsubscribe(client, &subscription_list);
}



/**
 * @brief Constructs the notify topic for reciving AWS IoT jobs.
 *
 * @param[in] client_id Client ID of the current connected MQTT session
 *	formatted.
 * @param[in] topic_buf buffer for storing the topic string until mqtt_subscribe
 *	is called.
 * @param[out] topic Outputs the constructed mqtt_topic into the given buffer.
 *
 * @retval 0 If sucessful otherwise the return value of report_snprintf_err
 */
static int construct_notify_topic(const u8_t *client_id,
				  u8_t *topic_buf,
				  struct mqtt_topic *topic)
{
	u32_t notify_topic_len;
	int ret = snprintf(topic_buf,
			   NOTIFY_TOPIC_MAX_LEN,
			   NOTIFY_TOPIC_TEMPLATE,
			   client_id);
	notify_topic_len = ret;
	ret = report_snprintf_err(ret, NOTIFY_TOPIC_MAX_LEN, "notify_topic");

	if (ret) {
		return ret;
	}

	topic->topic.utf8 = topic_buf;
	topic->topic.size = notify_topic_len;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}


int aws_jobs_subscribe_notify(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t notify_topic[NOTIFY_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_notify_topic(client->client_id.utf8,
					 notify_topic,
					 &subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_NOTIFY
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_subscribe(client, &subscription_list);
}


int aws_jobs_unsubscribe_notify(struct mqtt_client *const client)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t notify_topic[NOTIFY_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_notify_topic(client->client_id.utf8,
					 notify_topic,
					 &subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_NOTIFY
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_unsubscribe(client, &subscription_list);
}


int aws_jobs_subscribe_expected_topics(struct mqtt_client *const client,
				       bool notify_next)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t get_topic[GET_TOPIC_LEN + 1];
	struct mqtt_topic subscribe_topics[2];
	char *client_id = client->client_id.utf8;

	int err = construct_get_topic(client_id,
				      get_topic,
				      &subscribe_topics[0]);

	if (err) {
		return err;
	}

	if (notify_next) {
		u8_t notify_next_topic[NOTIFY_NEXT_TOPIC_MAX_LEN  + 1];

		err = construct_notify_next_topic(client_id,
						  notify_next_topic,
						  &subscribe_topics[1]);
	} else {
		u8_t notify_topic[NOTIFY_TOPIC_MAX_LEN  + 1];

		err = construct_notify_topic(client_id,
					     notify_topic,
					     &subscribe_topics[1]);
	}

	if (err) {
		return err;
	}

	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topics[0].topic.utf8));
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topics[1].topic.utf8));

	const struct mqtt_subscription_list subscription_list = {
		.list = subscribe_topics,
		.list_count = ARRAY_SIZE(subscribe_topics),
		.message_id = SUBSCRIBE_EXPECTED
	};

	return mqtt_subscribe(client, &subscription_list);
}


int aws_jobs_unsubscribe_expected_topics(struct mqtt_client *const client,
				       bool notify_next)
{
	if (client == NULL) {
		return -EINVAL;
	}

	u8_t get_topic[GET_TOPIC_LEN + 1];
	struct mqtt_topic subscribe_topics[2];
	char *client_id = client->client_id.utf8;

	int err = construct_get_topic(client_id,
				      get_topic,
				      &subscribe_topics[0]);

	if (err) {
		return err;
	}

	if (notify_next) {
		u8_t notify_next_topic[NOTIFY_NEXT_TOPIC_MAX_LEN  + 1];

		err = construct_notify_next_topic(client_id,
						  notify_next_topic,
						  &subscribe_topics[1]);
	} else {
		u8_t notify_topic[NOTIFY_TOPIC_MAX_LEN  + 1];

		err = construct_notify_topic(client_id,
					     notify_topic,
					     &subscribe_topics[1]);
	}

	if (err) {
		return err;
	}

	LOG_DBG("Unubscribe: %s", log_strdup(subscribe_topics[0].topic.utf8));
	LOG_DBG("Unubscribe: %s", log_strdup(subscribe_topics[1].topic.utf8));

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&subscribe_topics,
		.list_count = ARRAY_SIZE(subscribe_topics),
		.message_id = SUBSCRIBE_EXPECTED
	};

	return mqtt_unsubscribe(client, &subscription_list);
}


/**
 * @brief Constructs the get topic for accepted and rejeceted AWS jobs.
 *
 * @param[in] client_id Client ID of the current connected MQTT session
 *	formatted.
 * @param[in] job_id Job ID of the current accepted job ormatted.
 * @param[in] topic_buf buffer for storing the topic string until mqtt_subscribe
 *	is called.
 * @param[out] topic Outputs the constructed mqtt_topic into the given buffer.
 *
 * @retval 0 If sucessful otherwise the return value of report_snprintf_err
 *
 */
static int construct_job_id_get_topic(const u8_t *client_id,
				      const u8_t *job_id,
				      u8_t *topic_buf,
				      struct mqtt_topic *topic)
{
	u32_t job_id_get_topic_len;
	int ret = snprintf(topic_buf,
			  JOB_ID_GET_TOPIC_MAX_LEN,
			  JOB_ID_GET_TOPIC_TEMPLATE,
			  client_id,
			  job_id,
			  "#");
	job_id_get_topic_len = ret;
	ret = report_snprintf_err(ret,
				  JOB_ID_GET_TOPIC_MAX_LEN,
				  "job_id_get_topic");
	if (ret) {
		return ret;
	}

	topic->topic.utf8 = topic_buf;
	topic->topic.size = job_id_get_topic_len;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}


int aws_jobs_subscribe_job_id_get_topic(struct mqtt_client *const client,
					const u8_t *job_id)
{
	if (client == NULL || job_id == NULL) {
		return -EINVAL;
	}

	u8_t job_id_get_topic[JOB_ID_GET_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_job_id_get_topic(client->client_id.utf8,
					     job_id,
					     job_id_get_topic,
					     &subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_JOB_ID_GET
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_subscribe(client, &subscription_list);
}


int aws_jobs_unsubscribe_job_id_get_topic(struct mqtt_client *const client,
					  const u8_t *job_id)
{
	if (client == NULL || job_id == NULL) {
		return -EINVAL;
	}

	u8_t job_id_get_topic[JOB_ID_GET_TOPIC_MAX_LEN  + 1];
	struct mqtt_topic subscribe_topic;
	int err = construct_job_id_get_topic(client->client_id.utf8,
					     job_id,
					     job_id_get_topic,
					     &subscribe_topic);

	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_JOB_ID_GET
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_unsubscribe(client, &subscription_list);
}


/**
 * @brief Constructs the update topic for AWS jobs.
 *
 * @param[in] client_id Client ID of the current connected MQTT session.
 * @param[in] job_id The JobId of the current recived job.
 * @param[in] topic_buf Buffer for storing the topic string until mqtt_subscribe
 *	is called.
 * @param[in] suffix Suffix appended to the end of the topic should start with /
 * @param[out] topic Outputs the constructed mqtt_topic into the given buffer.
 *
 * @retval 0 If sucessful otherwise the return value of report_snprintf_err
 */
static int construct_job_id_update_topic(const u8_t *client_id,
					 const u8_t *job_id,
					 const u8_t *suffix,
					 u8_t *topic_buf,
					 struct mqtt_topic *topic)
{
	u32_t job_id_update_topic_len;
	int ret = snprintf(topic_buf,
			   JOB_ID_UPDATE_TOPIC_MAX_LEN,
			   JOB_ID_UPDATE_TOPIC_TEMPLATE,
			   client_id,
			   job_id,
			   suffix);
	job_id_update_topic_len = ret;

	ret = report_snprintf_err(ret,
				  JOB_ID_UPDATE_TOPIC_MAX_LEN,
				  "job_id_update_topic");
	if (ret) {
		return ret;
	}

	topic->topic.utf8 = topic_buf;
	topic->topic.size = job_id_update_topic_len;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}

int aws_jobs_subscribe_job_id_update(struct mqtt_client *const client,
				     const u8_t *job_id)
{
	if (client == NULL || job_id == NULL) {
		return -EINVAL;
	}

	u8_t job_id_update_topic[JOB_ID_UPDATE_TOPIC_MAX_LEN + 1];
	struct mqtt_topic subscribe_topic;
	/* Subscribe to both update and rejected topic */
	int err = construct_job_id_update_topic(client->client_id.utf8,
						job_id,
						"/#",
						job_id_update_topic,
						&subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_JOB_ID_UPDATE
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_subscribe(client, &subscription_list);
}

int aws_jobs_unsubscribe_job_id_update(struct mqtt_client *const client,
				       const u8_t *job_id)
{
	if (client == NULL || job_id == NULL) {
		return -EINVAL;
	}

	u8_t job_id_update_topic[JOB_ID_UPDATE_TOPIC_MAX_LEN + 1];
	struct mqtt_topic subscribe_topic;
	/* Unsubscribe from both update and rejected topic */
	int err = construct_job_id_update_topic(client->client_id.utf8,
						job_id,
						"/#",
						job_id_update_topic,
						&subscribe_topic);
	if (err) {
		return err;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&subscribe_topic,
		.list_count = 1,
		.message_id = SUBSCRIBE_JOB_ID_UPDATE
	};
	LOG_DBG("Subscribe: %s", log_strdup(subscribe_topic.topic.utf8));

	return mqtt_unsubscribe(client, &subscription_list);
}


#define UPDATE_JOB_PAYLOAD "{\"status\":\"%s\",\
			    \"statusDetails\": %s,\
			    \"expectedVersion\": \"%d\",\
			    \"clientToken\": \"%s\"}"
int aws_jobs_update_job_execution(struct mqtt_client *const client,
			     const u8_t *job_id,
			     enum execution_status status,
			     const u8_t *status_details,
			     int expected_version,
			     const u8_t *client_token)
{
	if (client == NULL || job_id == NULL || client_token == NULL) {
		return -EINVAL;
	}

	/*Max size document is 1350 char but the max size of the JSON document
	 *is actually 32kb set it to what is the limiting factor which is the
	 *MQTT buffer size for reception.
	 */
	u32_t update_job_payload_len;
	u8_t update_job_payload[CONFIG_UPDATE_JOB_PAYLOAD_LEN];
	struct mqtt_topic job_id_update_topic;

	int ret = snprintf(update_job_payload,
		       ARRAY_SIZE(update_job_payload),
		       UPDATE_JOB_PAYLOAD,
		       execution_status_strings[status],
		       status_details,
		       expected_version,
		       client_token);
	update_job_payload_len = ret;
	ret = report_snprintf_err(ret,
				  CONFIG_UPDATE_JOB_PAYLOAD_LEN,
				  "update_job_payload");
	if (ret) {
		return ret;
	}

	u8_t job_id_update_topic_buf[JOB_ID_UPDATE_TOPIC_MAX_LEN + 1];

	ret = construct_job_id_update_topic(client->client_id.utf8,
					    job_id,
					    "",
					    job_id_update_topic_buf,
					    &job_id_update_topic);

	if (ret) {
		return ret;
	}

	LOG_DBG("Topic: %s", log_strdup(job_id_update_topic.topic.utf8));
	LOG_DBG("Payload: %s", log_strdup(update_job_payload));

	struct mqtt_publish_param param = {
		.message.topic = job_id_update_topic,
		.message.payload.data = update_job_payload,
		.message.payload.len = update_job_payload_len,
		.message_id = sys_rand32_get(),
		.dup_flag = 0,
		.retain_flag = 0,
	};

	return mqtt_publish(client, &param);
}
