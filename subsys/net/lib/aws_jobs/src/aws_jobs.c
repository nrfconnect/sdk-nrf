/*
 *copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdio.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>
#include <net/aws_jobs.h>

LOG_MODULE_REGISTER(aws_jobs, CONFIG_AWS_JOBS_LOG_LEVEL);

/** @brief Mapping of enum to strings for Job Execution Status. */
static const char *execution_status_strings[] = {
	[AWS_JOBS_QUEUED]      = "QUEUED",
	[AWS_JOBS_IN_PROGRESS] = "IN_PROGRESS",
	[AWS_JOBS_SUCCEEDED]   = "SUCCEEDED",
	[AWS_JOBS_FAILED]      = "FAILED",
	[AWS_JOBS_TIMED_OUT]   = "TIMED_OUT",
	[AWS_JOBS_REJECTED]    = "REJECTED",
	[AWS_JOBS_REMOVED]     = "REMOVED",
	[AWS_JOBS_CANCELED]    = "CANCELED"
};

struct topic_conf {
	int msg_id;
	const uint8_t *name;
	const char *suffix;
};

const struct topic_conf TOPIC_NOTIFY_NEXT_CONF = {
	.msg_id = SUBSCRIBE_NOTIFY_NEXT,
	.name = "notify-next",
	.suffix = "",
};

const struct topic_conf TOPIC_NOTIFY_CONF = {
	.msg_id = SUBSCRIBE_NOTIFY,
	.name = "notify",
	.suffix = "",
};

const struct topic_conf TOPIC_GET_CONF = {
	.msg_id = SUBSCRIBE_JOB_ID_GET,
	.name = "get",
	.suffix = "/#",
};

const struct topic_conf TOPIC_UPDATE_CONF = {
	.msg_id = SUBSCRIBE_JOB_ID_UPDATE,
	.name = "update",
	.suffix = "/#",
};

/**
 * @brief Constructs a topic for AWS jobs.
 *
 * @param[in] client_id  Client ID of the current connected MQTT session
 * @param[in] job_id     Job ID of the current accepted job. If no job_id
 *                       should be added to the topic "" must be set as the
 *                       value.
 * @param[in] topic_conf Configuration of topic.
 * @param[out] out_buf   Buffer where the topic string is stored.
 * @param[out] topic     Pointer to mqtt topic struct to update.
 *
 * @retval 0 If sucessful otherwise an error message.
 *
 */
#define TOPIC_TEMPLATE "$aws/things/%s/jobs/%s%s%s%s"
static int construct_topic(const uint8_t *client_id, const uint8_t *job_id,
			   const struct topic_conf *conf, uint8_t *out_buf,
			   struct mqtt_topic *topic, bool remove_suffix)
{
	if (client_id == NULL || job_id == NULL || conf == NULL ||
	    conf->name == NULL || conf->suffix == NULL ||
	    out_buf == NULL || topic == NULL) {
		return -EINVAL;
	}

	const char *suffix = remove_suffix  ? "" : conf->suffix;
	const char *slash_after_job_id = strlen(job_id) > 0 ? "/" : "";
	int ret = snprintf(out_buf, AWS_JOBS_TOPIC_MAX_LEN, TOPIC_TEMPLATE,
			client_id, job_id, slash_after_job_id, conf->name,
			suffix);

	if (ret >= AWS_JOBS_TOPIC_MAX_LEN) {
		LOG_ERR("Unable to fit formated string into to allocate "
			"memory for %s", (char *)conf->name);
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Output error for %s was encountered with return value "
			"%d", (char *)conf->name, ret);
		return ret;
	}

	topic->topic.size = ret;
	topic->topic.utf8 = out_buf;
	topic->qos = MQTT_QOS_1_AT_LEAST_ONCE;
	return 0;
}

static int reg_topic(struct mqtt_client *const client, uint8_t *topic_buf,
		     struct topic_conf const *conf, const uint8_t *job_id,
		     bool subscribe)
{
	if (client == NULL) {
		return -EINVAL;
	}

	struct mqtt_topic topic;
	int err = construct_topic(client->client_id.utf8, job_id, conf,
				  topic_buf, &topic, false);

	const struct mqtt_subscription_list subscription_list = {
		.list = &topic,
		.list_count = 1,
		.message_id = conf->msg_id
	};

	if (err) {
		return err;
	}

	if (subscribe) {
		LOG_DBG("Subscribe: %s", (char *)topic.topic.utf8);
		return mqtt_subscribe(client, &subscription_list);
	}

	LOG_DBG("Unsubscribe: %s", (char *)topic.topic.utf8);
	return mqtt_unsubscribe(client, &subscription_list);
}


int aws_jobs_create_topic_notify_next(struct mqtt_client *const client,
					 uint8_t *topic_buf)
{
	struct mqtt_topic topic;

	return construct_topic(client->client_id.utf8, "",
			       &TOPIC_NOTIFY_NEXT_CONF, topic_buf, &topic,
			       false);
}

int aws_jobs_subscribe_topic_notify_next(struct mqtt_client *const client,
					 uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_NOTIFY_NEXT_CONF, "", true);
}

int aws_jobs_unsubscribe_topic_notify_next(struct mqtt_client *const client,
					   uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_NOTIFY_NEXT_CONF, "", false);
}

int aws_jobs_create_topic_notify(struct mqtt_client *const client,
				 uint8_t *topic_buf)
{
	struct mqtt_topic topic;

	return construct_topic(client->client_id.utf8, "",
			       &TOPIC_NOTIFY_CONF, topic_buf, &topic, false);
}

int aws_jobs_subscribe_topic_notify(struct mqtt_client *const client,
				    uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf,  &TOPIC_NOTIFY_CONF, "", true);
}

int aws_jobs_unsubscribe_topic_notify(struct mqtt_client *const client,
				      uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_NOTIFY_CONF, "", false);
}

int aws_jobs_create_topic_get(struct mqtt_client *const client,
				 const uint8_t *job_id, uint8_t *topic_buf)
{
	struct mqtt_topic topic;

	return construct_topic(client->client_id.utf8, job_id,
			       &TOPIC_GET_CONF, topic_buf, &topic, false);
}

int aws_jobs_subscribe_topic_get(struct mqtt_client *const client,
				 const uint8_t *job_id, uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_GET_CONF, job_id, true);
}

int aws_jobs_unsubscribe_topic_get(struct mqtt_client *const client,
				   const uint8_t *job_id, uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_GET_CONF, job_id, false);
}

int aws_jobs_subscribe_topic_update(struct mqtt_client *const client,
				    const uint8_t *job_id, uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_UPDATE_CONF, job_id, true);
}

int aws_jobs_unsubscribe_topic_update(struct mqtt_client *const client,
				      const uint8_t *job_id, uint8_t *topic_buf)
{
	return reg_topic(client, topic_buf, &TOPIC_UPDATE_CONF, job_id, false);
}

static int publish(struct mqtt_client *const client, const uint8_t *job_id,
		   const struct topic_conf *conf, uint8_t *payload_data,
		   size_t payload_data_len, uint8_t *topic_buf)
{
	struct mqtt_topic topic;

	int ret = construct_topic(client->client_id.utf8, job_id, conf,
				  topic_buf, &topic, true);

	if (ret) {
		return ret;
	}

	LOG_DBG("Publish topic: %s", (char *)topic.topic.utf8);
	LOG_DBG("Publish payload %s", (char *)payload_data);

	struct mqtt_publish_param param = {
		.message.topic = topic,
		.message.payload.data = payload_data,
		.message.payload.len = payload_data_len,
		.message_id = k_cycle_get_32(),
		.dup_flag = 0,
		.retain_flag = 0,
	};

	return mqtt_publish(client, &param);
}

#define UPDATE_JOB_PAYLOAD "{\"status\":\"%s\",\"statusDetails\": %s,"\
"\"expectedVersion\": \"%d\",\"clientToken\": \"%s\"}"
int aws_jobs_update_job_execution(struct mqtt_client *const client,
				  const uint8_t *job_id,
				  enum execution_status status,
				  const uint8_t *status_details,
				  int expected_version,
				  const uint8_t *client_token, uint8_t *topic_buf)
{
	/* The rest of the parameters are checked later */
	if (client_token == NULL) {
		return -EINVAL;
	}

	/* Max size document is 1350 char but the max size of the JSON document
	 * is actually 32kb set it to what is the limiting factor which is the
	 * MQTT buffer size for reception.
	 */
	uint8_t update_job_payload[CONFIG_UPDATE_JOB_PAYLOAD_LEN];

	int ret = snprintf(update_job_payload, sizeof(update_job_payload),
			   UPDATE_JOB_PAYLOAD, execution_status_strings[status],
			   (status_details ? (char *)status_details : "null"),
			   expected_version, client_token);

	if (ret >= CONFIG_UPDATE_JOB_PAYLOAD_LEN) {
		LOG_ERR("Unable to fit formated string in provided buffer.");
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Error when creating payload %d", ret);
		return ret;
	}

	return publish(client, job_id, &TOPIC_UPDATE_CONF, update_job_payload,
		       ret, topic_buf);

}

#define JOB_ID_GET_PAYLOAD "{\"clientToken\": \"\"}"
int aws_jobs_get_job_execution(struct mqtt_client *const client,
			       const char *job_id, uint8_t *topic_buf)
{
	return publish(client, job_id, &TOPIC_GET_CONF, JOB_ID_GET_PAYLOAD,
		       strlen(JOB_ID_GET_PAYLOAD), topic_buf);
}

bool aws_jobs_cmp(const char *sub, const char *pub, size_t pub_len,
		 const uint8_t *suffix)
{
	int ret;

	if (sub == NULL || pub == NULL || suffix == NULL ||
	    sub[0] == '\0' || pub[0] == '\0') {
		return false;
	}

	size_t sub_len = strlen(sub);
	size_t suff_len = strlen(suffix);

	if (sub[sub_len - 1] == '#') {
		/* Strip trailing '/#' */
		sub_len -= 2;
	}

	ret = strncmp(sub, pub, sub_len);
	if (ret == 0 && suff_len > 0) {
		/* Everything up until suffix is correct, check suffix */
		return strncmp(&pub[pub_len - suff_len], suffix, suff_len) == 0;
	} else {
		return ret == 0;
	}
}
