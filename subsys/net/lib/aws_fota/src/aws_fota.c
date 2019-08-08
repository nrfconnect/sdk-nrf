/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <json.h>
#include <net/fota_download.h>
#include <net/aws_jobs.h>
#include <net/aws_fota.h>
#include <logging/log.h>

#include "aws_fota_json.h"

LOG_MODULE_REGISTER(aws_fota, CONFIG_AWS_FOTA_LOG_LEVEL);

/* Enum to keep the fota status */
enum fota_status {
	NONE = 0,
	DOWNLOAD_FIRMWARE,
	APPLY_UPDATE,
};

/* Map of fota status to report back */
static const char * const fota_status_strings[] = {
	[DOWNLOAD_FIRMWARE] = "download_firmware",
	[APPLY_UPDATE] = "apply_update",
	[NONE] = "none",
};

/* Pointer to initialized MQTT client instance */
static struct mqtt_client *c;

/* Enum for tracking the job exectuion state */
static enum execution_status execution_state = AWS_JOBS_QUEUED;
static enum fota_status fota_state = NONE;

/* Document version starts at 1 and is incremented with each accepted update */
static u32_t doc_version_number = 1;

/* Buffer for reporting the current application version */
static char version[CONFIG_VERSION_STRING_MAX_LEN + 1];

/* Allocated strings for checking topics */
static u8_t notify_next_topic[NOTIFY_NEXT_TOPIC_MAX_LEN + 1];
static u8_t job_id_update_accepted_topic[JOB_ID_UPDATE_TOPIC_MAX_LEN + 1];
static u8_t job_id_update_rejected_topic[JOB_ID_UPDATE_TOPIC_MAX_LEN + 1];

/* Allocated strings for fetching a job */
static u8_t get_next_job_execution_topic[JOB_ID_GET_TOPIC_MAX_LEN + 1];

/* Allocated buffers for keeping hostname, json payload and file_path */
static u8_t payload_buf[CONFIG_AWS_FOTA_PAYLOAD_SIZE + 1];
static u8_t hostname[CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN + 1];
static u8_t file_path[CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN + 1];
static u8_t job_id[JOB_ID_MAX_LEN + 1];
static aws_fota_callback_t callback;


/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *client,
			       u8_t *write_buf,
			       size_t length)
{
	u8_t *buf = write_buf;
	u8_t *end = buf + length;

	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}
	while (buf < end) {
		int ret = mqtt_read_publish_payload_blocking(client, buf, end - buf);

		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			return -EIO;
		}
		buf += ret;
	}
	return 0;
}

static int construct_job_id_get_topic(const u8_t *client_id, const u8_t *job_id,
				      const u8_t *suffix, u8_t *topic_buf)
{
	__ASSERT_NO_MSG(client_id != NULL);
	__ASSERT_NO_MSG(topic_buf != NULL);
	int ret = snprintf(topic_buf, JOB_ID_GET_TOPIC_MAX_LEN,
			   JOB_ID_GET_TOPIC_TEMPLATE, client_id, job_id,
			   suffix);
	if (ret >= NOTIFY_NEXT_TOPIC_MAX_LEN) {
		LOG_ERR("Unable to fit formated string into to allocate "
			"memory for notify_next_topic");
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Formatting error for notify_next_topic %d", ret);
		return ret;
	}

	return 0;
}

static int construct_notify_next_topic(const u8_t *client_id, u8_t *topic_buf)
{
	__ASSERT_NO_MSG(client_id != NULL);
	__ASSERT_NO_MSG(topic_buf != NULL);

	int ret = snprintf(topic_buf, NOTIFY_NEXT_TOPIC_MAX_LEN,
			   NOTIFY_NEXT_TOPIC_TEMPLATE, client_id);
	if (ret >= NOTIFY_NEXT_TOPIC_MAX_LEN) {
		LOG_ERR("Unable to fit formated string into to allocate "
			"memory for notify_next_topic");
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Formatting error for notify_next_topic %d", ret);
		return ret;
	}
	return 0;
}

static int construct_job_id_update_topic(const u8_t *client_id,
		const u8_t *job_id, const u8_t *suffix, u8_t *topic_buf)
{
	__ASSERT_NO_MSG(client_id != NULL);
	__ASSERT_NO_MSG(job_id != NULL);
	__ASSERT_NO_MSG(suffix != NULL);
	__ASSERT_NO_MSG(topic_buf != NULL);

	int ret = snprintf(topic_buf,
			   JOB_ID_UPDATE_TOPIC_MAX_LEN,
			   JOB_ID_UPDATE_TOPIC_TEMPLATE,
			   client_id,
			   job_id,
			   suffix);

	if (ret >= JOB_ID_UPDATE_TOPIC_MAX_LEN) {
		LOG_ERR("Unable to fit formated string into to allocate "
			"memory for construct_job_id_update_topic");
		return -ENOMEM;
	} else if (ret < 0) {
		LOG_ERR("Formatting error for job_id_update topic: %d", ret);
		return ret;
	}
	return 0;
}

/* Topic for updating shadow topic with version number */
#define UPDATE_DELTA_TOPIC AWS "%s/shadow/update"
#define UPDATE_DELTA_TOPIC_LEN (AWS_LEN +\
				CONFIG_CLIENT_ID_MAX_LEN +\
				(sizeof("/shadow/update") - 1))
#define SHADOW_STATE_UPDATE \
	"{\"state\":{\"reported\":{\"nrfcloud__dfu_v1__app_v\":\"%s\"}}}"

static int update_device_shadow_version(struct mqtt_client *const client)
{
	struct mqtt_publish_param param;
	char update_delta_topic[UPDATE_DELTA_TOPIC_LEN + 1];
	u8_t shadow_update_payload[CONFIG_DEVICE_SHADOW_PAYLOAD_SIZE];

	int ret = snprintf(update_delta_topic,
			   sizeof(update_delta_topic),
			   UPDATE_DELTA_TOPIC,
			   client->client_id.utf8);
	u32_t update_delta_topic_len = ret;

	if (ret >= sizeof(update_delta_topic)) {
		return -ENOMEM;
	} else if (ret < 0) {
		return ret;
	}

	ret = snprintf(shadow_update_payload,
		       sizeof(shadow_update_payload),
		       SHADOW_STATE_UPDATE,
		       version);
	u32_t shadow_update_payload_len = ret;

	if (ret >= sizeof(shadow_update_payload)) {
		return -ENOMEM;
	} else if (ret < 0) {
		return ret;
	}

	param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	param.message.topic.topic.utf8 = update_delta_topic;
	param.message.topic.topic.size = update_delta_topic_len;
	param.message.payload.data = shadow_update_payload;
	param.message.payload.len = shadow_update_payload_len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(client, &param);
}

#define AWS_FOTA_STATUS_DETAILS_TEMPLATE "{\"nextState\":\"%s\"}"
#define STATUS_DETAILS_MAX_LEN  (sizeof("{\"nextState\":\"\"}") \
				+ (sizeof("download_firmware") + 2))

static int update_job_execution(struct mqtt_client *const client,
				const u8_t *job_id,
				enum execution_status state,
				enum fota_status next_state,
				int version_number,
				const char *client_token)
{
	char status_details[STATUS_DETAILS_MAX_LEN + 1];
	int ret = snprintf(status_details,
			   sizeof(status_details),
			   AWS_FOTA_STATUS_DETAILS_TEMPLATE,
			   fota_status_strings[next_state]);
	__ASSERT(ret >= 0, "snprintf returned error %d\n", ret);
	__ASSERT(ret < STATUS_DETAILS_MAX_LEN,
		"Not enough space for status, need %d bytes\n", ret+1);

	ret =  aws_jobs_update_job_execution(client,
					     job_id,
					     state,
					     status_details,
					     version_number,
					     client_token);

	if (ret < 0) {
		LOG_ERR("aws_jobs_update_job_execution failed: %d", ret);
	}

	return ret;
}


static int aws_fota_on_publish_evt(struct mqtt_client *const client,
				   const u8_t *topic,
				   u32_t topic_len,
				   u32_t payload_len)
{
	int err;
	LOG_DBG("%s recived", __func__);

	/* If not processing job */
	/* Receiving a publish on notify_next_topic could be a job */
	bool is_notify_next_topic = (topic_len <= NOTIFY_NEXT_TOPIC_MAX_LEN) &&
				    !strncmp(notify_next_topic, topic,
					     topic_len);
	bool is_get_next_topic = (topic_len <= JOB_ID_GET_TOPIC_MAX_LEN) &&
				 !strncmp(get_next_job_execution_topic, topic,
					  topic_len);
	if (is_notify_next_topic || is_get_next_topic) {
		err = publish_get_payload(client, payload_buf, payload_len);
		if (err) {
			LOG_ERR("Error when getting the payload: %d", err);
			return err;
		}
		/*
		 * Check if the current message received on notify-next is a
		 * job.
		 */
		err = aws_fota_parse_notify_next_document(payload_buf,
							  payload_len, job_id,
							  hostname, file_path);
		if (err < 0) {
			LOG_ERR("Error when parsing the json: %d", err);
			return err;
		} else  if (err == 1) {
			LOG_INF("Job document with only timestamp on notify_next "
				"meaning that the current device job was canceled");
			return 1;
		}

		/* Unsubscribe from notify_next_topic to not recive more jobs
		 * while processing the current job.
		 */
		err = aws_jobs_unsubscribe_notify_next(client);
		if (err) {
			LOG_ERR("Error when unsubscribing notify_next_topic: "
			       "%d", err);
			return err;
		}

		/* Subscribe to update topic to recive feedback on wether an
		 * update is accepted or not.
		 */
		err = aws_jobs_subscribe_job_id_update(client, job_id);
		if (err) {
			LOG_ERR("Error when subscribing job_id_update: "
				"%d", err);
			return err;
		}

		/* Construct job_id topics to be used for filtering publish
		 * messages.
		 */
		err = construct_job_id_update_topic(client->client_id.utf8,
			job_id, "/accepted", job_id_update_accepted_topic);
		if (err) {
			LOG_ERR("Error when constructing_job_id_update_accepted: %d",
				err);
			return err;
		}

		err = construct_job_id_update_topic(client->client_id.utf8,
			job_id, "/rejected", job_id_update_rejected_topic);
		if (err) {
			LOG_ERR("Error when constructing_job_id_update_rejected: %d",
				err);
			return err;
		}

		/* Set fota_state to DOWNLOAD_FIRMWARE, when we are subscribed
		 * to job_id topics we will try to publish and if accepted we
		 * can start the download
		 */
		fota_state = DOWNLOAD_FIRMWARE;

		/* Handled by the library*/
		return 1;

	} else if (topic_len <= JOB_ID_UPDATE_TOPIC_MAX_LEN &&
		   !strncmp(job_id_update_accepted_topic, topic, topic_len)) {

		LOG_DBG("Job document update was accepted");
		err = publish_get_payload(client, payload_buf, payload_len);
		if (err) {
			return err;
		}
		/* Increment document version counter as the update was accepted */
		doc_version_number++;

		if (fota_state == DOWNLOAD_FIRMWARE) {
			/* Job document is updated and we are ready to download
			 * the firmware. */
			execution_state = AWS_JOBS_IN_PROGRESS;
			LOG_INF("Start downloading firmware from %s%s",
				log_strdup(hostname), log_strdup(file_path));
			err = fota_download_start(hostname, file_path);
			if (err) {
				LOG_ERR("Error when trying to start firmware"
				       "download: %d", err);
				return err;
			}
		} else if (execution_state == AWS_JOBS_IN_PROGRESS &&
		    fota_state == APPLY_UPDATE) {
			LOG_INF("Firmware download completed");
			execution_state = AWS_JOBS_SUCCEEDED;
			err = update_job_execution(client, job_id,
						   execution_state, fota_state,
						   doc_version_number, "");
			if (err) {
				return err;
			}
		} else if (execution_state == AWS_JOBS_SUCCEEDED &&
			   fota_state == APPLY_UPDATE) {
			LOG_INF("Job document updated with SUCCEDED");
			LOG_INF("Ready to reboot");
			callback(AWS_FOTA_EVT_DONE);
		}
		return 1;
	} else if (topic_len <= JOB_ID_UPDATE_TOPIC_MAX_LEN &&
		   !strncmp(job_id_update_rejected_topic, topic, topic_len)) {
		LOG_ERR("Job document update was rejected");
		err = publish_get_payload(client, payload_buf, payload_len);
		if (err) {
			LOG_ERR("Error when getting the payload: %d", err);
			return err;
		}
		callback(AWS_FOTA_EVT_ERROR);
		return -EFAULT;
	}
	return 0;

}

int aws_fota_mqtt_evt_handler(struct mqtt_client *const client,
			      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			return 0;
		}

		err = aws_jobs_subscribe_notify_next(client);
		if (err) {
			LOG_ERR("Unable to subscribe to notify-next topic");
			return err;
		}

		err = aws_jobs_subscribe_job_id_get_topic(client, "$next");
		if (err) {
			LOG_ERR("Unable to subscribe to jobs/$next/get");
			return err;
		}

		err = aws_jobs_subscribe_job_id_get_topic(client, "$next");
		if (err) {
			LOG_ERR("Unable to subscribe to jobs/$next/get");
		}

		err = update_device_shadow_version(client);
		if (err) {
			LOG_ERR("Unable to update device shadow");
			return err;
		}

		return 0;
		/* This expects that the application's mqtt handler will handle
		 * any situations where you could not connect to the MQTT
		 * broker.
		 */

	case MQTT_EVT_DISCONNECT:
		return 0;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;

		err = aws_fota_on_publish_evt(client,
					      p->message.topic.topic.utf8,
					      p->message.topic.topic.size,
					      p->message.payload.len);
		if (err < 1) {
			return err;
		}

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgment. */
			err = mqtt_publish_qos1_ack(c, &ack);
			if (err) {
				return err;
			}
		}
		return 1;

	} break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			return 0;
		}
		return 0;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			return 0;
		}
		if (evt->param.suback.message_id == SUBSCRIBE_NOTIFY_NEXT) {
			LOG_INF("subscribed to notify_next topic");
			err = aws_jobs_get_job_execution(client, "$next");
			if (err) {
				return err;
			}
			return 1;
		}

		if ((fota_state == DOWNLOAD_FIRMWARE) &&
		   (evt->param.suback.message_id == SUBSCRIBE_JOB_ID_UPDATE)) {
			err = update_job_execution(client, job_id,
						   AWS_JOBS_IN_PROGRESS,
						   fota_state,
						   doc_version_number,
						   "");
			if (err) {
				return err;
			}
			return 1;
		}

		return 0;

	default:
		/* Handling for default case? */
		return 0;
	}
	return 0;
}

static void http_fota_handler(enum fota_download_evt_id evt)
{
	__ASSERT_NO_MSG(c != NULL);

	int err = 0;

	switch (evt) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download completed evt recived");
		fota_state = APPLY_UPDATE;
		err = update_job_execution(c, job_id, AWS_JOBS_IN_PROGRESS,
				     fota_state, doc_version_number, "");
		if (err != 0) {
			callback(AWS_FOTA_EVT_ERROR);
		}
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA download failed, report back");
		(void) update_job_execution(c, job_id, AWS_JOBS_FAILED,
				     fota_state, doc_version_number, "");
		callback(AWS_FOTA_EVT_ERROR);
		break;
	}

}

int aws_fota_init(struct mqtt_client *const client,
		  const char *app_version,
		  aws_fota_callback_t evt_handler)
{
	int err;

	if (client == NULL || app_version == NULL || evt_handler == NULL) {
		return -EINVAL;
	}

	if (strlen(app_version) >= CONFIG_VERSION_STRING_MAX_LEN) {
		return -EINVAL;
	}

	/* Store client to make it available in event handlers. */
	c = client;
	callback = evt_handler;

	err = construct_notify_next_topic(client->client_id.utf8,
					  notify_next_topic);
	if (err != 0) {
		LOG_ERR("construct_notify_next_topic error %d", err);
		return err;
	}

	err = construct_job_id_get_topic(client->client_id.utf8,
					 "$next",
					 "",
					 get_next_job_execution_topic);
	if (err != 0) {
		LOG_ERR("construct_job_id_get_topic error %d", err);
		return err;
	}

	err = fota_download_init(http_fota_handler);
	if (err != 0) {
		LOG_ERR("fota_download_init error %d", err);
		return err;
	}

	strncpy(version, app_version, CONFIG_VERSION_STRING_MAX_LEN);

	return 0;
}
