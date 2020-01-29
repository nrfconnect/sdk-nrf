/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <data/json.h>
#include <net/fota_download.h>
#include <net/aws_jobs.h>
#include <net/aws_fota.h>
#include <logging/log.h>
#include <dfu/dfu_target.h>
#include <settings/settings.h>

#include "aws_fota_json.h"

LOG_MODULE_REGISTER(aws_fota, CONFIG_AWS_FOTA_LOG_LEVEL);

/* State variable to ensure that the AWS Fota library does not update the job
 * execution document before the last update is accepted by AWS IoT.
 */
static bool accepted = true;

/* Enum to keep the fota status */
enum fota_status {
	NONE = 0,
	DOWNLOAD_FIRMWARE,
	APPLY_UPDATE,
	PENDING_UPDATE,
};

/* Map of fota status to report back */
static const char * const fota_status_strings[] = {
	[DOWNLOAD_FIRMWARE] = "download_firmware",
	[APPLY_UPDATE] = "apply_update",
	[PENDING_UPDATE] = "pending_update",
	[NONE] = "none",
};

/* Pointer to initialized MQTT client instance */
static struct mqtt_client *c;

/* Enum for tracking the job exectuion state */
static enum execution_status execution_state = AWS_JOBS_QUEUED;
static enum fota_status fota_state = NONE;

/* Document version is read out from the job execution document and is then
 * incremented with each accepted update to the job execution.
 */
static u32_t execution_version_number;

/* Store file offset progress */
static size_t stored_progress;
static bool pending_update;

/* Allocated strings for topics */
static u8_t notify_next_topic[AWS_JOBS_TOPIC_MAX_LEN];
static u8_t update_topic[AWS_JOBS_TOPIC_MAX_LEN];
static u8_t get_topic[AWS_JOBS_TOPIC_MAX_LEN];

/* Allocated buffers for keeping hostname, json payload and file_path */
static u8_t payload_buf[CONFIG_AWS_FOTA_PAYLOAD_SIZE];
static u8_t hostname[CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN];
static u8_t file_path[CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN];
static u8_t job_id[AWS_JOBS_JOB_ID_MAX_LEN];
static aws_fota_callback_t callback;

/* Settings subsystem */
static int img_type;
static u8_t previous_mdm_fw_version[36];

#define MODULE "fota"
#define FILE_PENDING_UPDATE "pending_update"
#define FILE_JOB_ID "job_id"
#define FILE_VERSION_NUMBER "version_number"
#define FILE_PROGRESS "progress"
#define FILE_IMG_TYPE "img_type"
#define FILE_FW_VERSION "fw_version"
#define FILE_MODEM_FW "mdm_fw"
static int store_update_data(void)
{
	LOG_INF("Storing update data");
	char key[] = MODULE "/" FILE_PENDING_UPDATE;

	int err =  settings_save_one(key, &pending_update, sizeof(pending_update));

	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
	}

	char key2[] = MODULE "/" FILE_JOB_ID; 
	err = settings_save_one(key2, &job_id, sizeof(job_id));
	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
	}
	
	char key3[] = MODULE "/" FILE_VERSION_NUMBER; 
	err = settings_save_one(key3, &execution_version_number, sizeof(execution_version_number));
	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
	}

	char key4[] = MODULE "/" FILE_PROGRESS;
	err = settings_save_one(key4, &stored_progress, sizeof(stored_progress));
	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
	}

	return 0;
}

/**
 * @brief Function used by settings_load() to restore the flash_img variable.
 *	  See the Zephyr documentation of the settings subsystem for more
 *	  information.
 */
static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	LOG_INF("Settings set key: %s", log_strdup(key));
	if (!strcmp(key, FILE_PENDING_UPDATE)) {
		ssize_t len = read_cb(cb_arg, &pending_update,
				      sizeof(pending_update));
		if (len != sizeof(pending_update)) {
			LOG_ERR("Can't read flash_img from storage");
			return len;
		}
	}

	if (!strcmp(key, FILE_JOB_ID)) {
		ssize_t len = read_cb(cb_arg, &job_id,
				      sizeof(job_id));
		if (len != sizeof(job_id)) {
			LOG_ERR("Can't read job_id from storage");
			return len;
		}
	}
	
	if (!strcmp(key, FILE_VERSION_NUMBER)) {
		ssize_t len = read_cb(cb_arg, &execution_version_number,
				      sizeof(execution_version_number));
		if (len != sizeof(execution_version_number)) {
			LOG_ERR("Can't read execution_version_number from storage");
			return len;
		}
	}
	
	if (!strcmp(key, FILE_IMG_TYPE)) {
		ssize_t len = read_cb(cb_arg, &img_type,
				      sizeof(img_type));
		if (len != sizeof(img_type)) {
			LOG_ERR("Can't read execution_version_number from storage");
			return len;
		}
	}
	
	if (!strcmp(key, FILE_IMG_TYPE)) {
		ssize_t len = read_cb(cb_arg, &img_type,
				      sizeof(img_type));
		if (len != sizeof(img_type)) {
			LOG_ERR("Can't read execution_version_number from storage");
			return len;
		}
	}
	
	if (!strcmp(key, FILE_MODEM_FW)) {
		ssize_t len = read_cb(cb_arg, &previous_mdm_fw_version,
				      sizeof(previous_mdm_fw_version));
		if (len != sizeof(previous_mdm_fw_version)) {
			LOG_ERR("Can't read execution_version_number from storage");
			return len;
		}
	}

	return 0;
}

static struct settings_handler sh = {
	.name = MODULE,
	.h_set = settings_set,
};


/**
 * @brief Read the payload out of the published MQTT message from the MQTT
 *	  Client instance
 *
 * @param[in] client  Connected MQTT client instance.
 * @param[out] write_buf  Buffer where the MQTT publish message's payload is
 *			  stored.
 * @param[in] length  Length of the payload received.
 *
 * @return 0 If successful otherwise a negative error code is returned.
 */
static int get_published_payload(struct mqtt_client *client, u8_t *write_buf,
				 size_t length)
{
	u8_t *buf = write_buf;
	u8_t *end = buf + length;

	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}
	while (buf < end) {
		int ret = mqtt_read_publish_payload_blocking(client, buf,
							     end - buf);

		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			return -EIO;
		}
		buf += ret;
	}
	return 0;
}

/**
 * @brief Wait for a job update to be accepted.
 */
static inline void wait_for_update_accepted(void)
{
	while (!accepted) {
		k_yield();
	};
}

/**
 * @brief Update an AWS IoT Job Execution with a state and status details
 *
 * @param[in] client  Connected MQTT client instance
 * @param[in] job_id  Unique identifier for the devices Job Execution
 * @param[in] execution_state  The execution state to update the Job Execution
 *			       with.
 * @param[in] fota_status next_state  The next state the device's fota operation
 *				      is going to enter. Reported to the status
 *				      details of the Job Execution.
 * @param[in] progress How many bytes of the upgrade have been downloaded. Will
 *		       be reported to AWS IoT through the status details field
 *		       of the Job Execution.
 * @param[in] client_token  Client identifier which will be repeated in the
 *			    respone of the update.
 *
 * @return 0 If successful otherwise a negative error code is returned.
 */
#define AWS_FOTA_STATUS_DETAILS_TEMPLATE "{\"nextState\":\"%s\","\
					 "\"progress\":%d}"
#define STATUS_DETAILS_MAX_LEN 60

static int update_job_execution(struct mqtt_client *const client,
				const u8_t *job_id,
				enum execution_status state,
				enum fota_status next_state,
				int progress,
				const char *client_token
				)
{
	/* Waiting for the previous call to this function to be accepted. */
	wait_for_update_accepted();
	accepted = false;
	LOG_DBG("%s, state: %d, status: %d, version_number: %d", __func__,
		state, next_state, execution_version_number);
	char status_details[STATUS_DETAILS_MAX_LEN + 1];
	int ret = snprintf(status_details, sizeof(status_details),
			   AWS_FOTA_STATUS_DETAILS_TEMPLATE,
			   fota_status_strings[next_state], progress);
	__ASSERT(ret >= 0, "snprintf returned error %d\n", ret);
	__ASSERT(ret < STATUS_DETAILS_MAX_LEN,
		"Not enough space for status, need %d bytes\n", ret+1);

	ret =  aws_jobs_update_job_execution(client, job_id, state,
					     status_details,
					     execution_version_number,
					     client_token, update_topic);

	if (ret < 0) {
		LOG_ERR("aws_jobs_update_job_execution failed: %d", ret);
	}

	return ret;
}

/**
 * @brief Parsing an AWS IoT Job Execution response received on $next/get MQTT
 *	  topic or notify-next. If it is a valid response the program state is
 *	  updated and the MQTT client instance is subscribed to the update
 *	  topics for the job id received.
 *
 * @param[in] client  Connected MQTT client instance
 * @param[in] payload_len  Length of the payload going to be read out from the
 *			   MQTT message.
 *
 * @return 0 If successful otherwise a negative error code is returned.
 */
static int get_job_execution(struct mqtt_client *const client,
			     u32_t payload_len)
{
	int err = get_published_payload(client, payload_buf, payload_len);

	if (err) {
		LOG_ERR("Error when getting the payload: %d", err);
		return err;
	}
	/* Check if message received is a job. */
	err = aws_fota_parse_DescribeJobExecution_rsp(payload_buf, payload_len,
						      job_id, hostname,
						      file_path,
						     &execution_version_number);

	if (err < 0) {
		LOG_ERR("Error when parsing the json: %d", err);
		return err;
	} else  if (err == 0) {
		LOG_DBG("Got only one field: %s", log_strdup(payload_buf));
		LOG_INF("No queued jobs for this device");
		return 0;
	}

	LOG_DBG("Job ID: %s", log_strdup(job_id));
	LOG_DBG("hostname: %s", log_strdup(hostname));
	LOG_DBG("file_path %s", log_strdup(file_path));
	LOG_DBG("execution_version_number: %d ", execution_version_number);

	/* Subscribe to update topic to receive feedback on whether an
	 * update is accepted or not.
	 */
	err = aws_jobs_subscribe_topic_update(client, job_id, update_topic);
	if (err) {
		LOG_ERR("Error when subscribing job_id_update: "
				"%d", err);
		return err;
	}

	/* Set fota_state to DOWNLOAD_FIRMWARE, when we are subscribed
	 * to job_id topics we will try to publish and if accepted we
	 * can start the download
	 */
	fota_state = DOWNLOAD_FIRMWARE;

	return 0;
}

/**
 * @brief Updating the program state when a job document update is accepted.
 *
 * @param[in] client  Connected MQTT client instance
 * @param[in] payload_len  Length of the payload going to be read out from the
 *			   MQTT message.
 *
 * @return 0 If successful otherwise a negative error code is returned.
 */
static int job_update_accepted(struct mqtt_client *const client,
			       u32_t payload_len)
{
	int err = get_published_payload(client, payload_buf, payload_len);

	if (err) {
		LOG_ERR("Error when getting the payload: %d", err);
		return err;
	}

	accepted = true;
	/* Update accepted, so the execution version number needs to be
	 * incremented. This can also be parsed out from the response but it's
	 * better if the device handles this state so we need to send less data.
	 * Also it means that the device is the driver of updates to this
	 * document.
	 */
	execution_version_number++;

	if (execution_state != AWS_JOBS_IN_PROGRESS
	    && fota_state == DOWNLOAD_FIRMWARE) {
		/*Job document is updated and we are ready to download
		 * the firmware.
		 */
		execution_state = AWS_JOBS_IN_PROGRESS;
		LOG_INF("Start downloading firmware from %s/%s",
			log_strdup(hostname), log_strdup(file_path));
		err = fota_download_start(hostname, file_path);
		if (err) {
			LOG_ERR("Error (%d) when trying to start firmware "
				"download", err);
			return err;
		}
	} else if (execution_state == AWS_JOBS_IN_PROGRESS
		   && fota_state == APPLY_UPDATE) {
		LOG_INF("Firmware download completed");
		execution_state = AWS_JOBS_IN_PROGRESS;
		fota_state = PENDING_UPDATE;
		err = update_job_execution(client, job_id,
				execution_state, fota_state, stored_progress,
				"");
		if (err) {
			LOG_ERR("Unable to update the job execution");
			return err;
		}

	} else if (execution_state == AWS_JOBS_IN_PROGRESS
		   && fota_state == PENDING_UPDATE) {
		LOG_INF("Job document updated with IN_PROGRESS");
		pending_update = true;
		store_update_data();
		LOG_INF("Ready to reboot");
		callback(AWS_FOTA_EVT_DONE);
	}
	return 0;
}


/**
 * @brief Handling of a job document update when it is rejected.
 *
 * @param[in] client  Connected MQTT client instance
 * @param[in] payload_len  Length of the payload going to be read out from the
 *			   MQTT message.
 *
 * @return A negative error code is returned.
 */

static int job_update_rejected(struct mqtt_client *const client,
			       u32_t payload_len)
{
	LOG_ERR("Job document update was rejected");
	execution_version_number--;
	int err = get_published_payload(client, payload_buf, payload_len);

	if (err) {
		LOG_ERR("Error %d when getting the payload", err);
		return err;
	}
	LOG_ERR("%s", log_strdup(payload_buf));
	callback(AWS_FOTA_EVT_ERROR);
	return -EFAULT;
}

/**
 * @brief Handling of a MQTT publish event. It checks whether the topic matches
 *	  any of the expected AWS IoT Jobs topics used for FOTA.
 *
 * @param[in] client  Connected MQTT client instance.
 * @param[in] topic  String containing the received topic.
 * @param[in] topic_len  Length of the topic string.
 * @param[in] payload_len  Length of the received payload.
 *
 * @return 1 If the topic is not a topic used for AWS IoT Jobs. 0 If the content
 *	     in the topic was successfully handled. Otherwise a negative error
 *	     code is returned.
 */
static int on_publish_evt(struct mqtt_client *const client,
				   const u8_t *topic,
				   u32_t topic_len,
				   u32_t payload_len)
{
	bool is_get_next_topic =
		aws_jobs_cmp(get_topic, topic, topic_len, "");
	bool is_get_accepted =
		aws_jobs_cmp(get_topic, topic, topic_len, "accepted");
	bool is_notify_next_topic =
		aws_jobs_cmp(notify_next_topic, topic, topic_len, "");
	bool doc_update_accepted =
		aws_jobs_cmp(update_topic, topic, topic_len, "accepted");
	bool doc_update_rejected =
		aws_jobs_cmp(update_topic, topic, topic_len, "rejected");

	LOG_DBG("Received topic: %s", log_strdup(topic));

	if (is_notify_next_topic || is_get_next_topic || is_get_accepted) {
		LOG_INF("Checking for an available job");
		return get_job_execution(client, payload_len);
	} else if (doc_update_accepted) {
		return job_update_accepted(client, payload_len);
	} else if (doc_update_rejected) {
		LOG_ERR("Job document update was rejected");
		return job_update_rejected(client, payload_len);
	}
	LOG_INF("received an unhandled MQTT publish event on topic: %s",
		log_strdup(topic));

	return 1;
}
#include <nrf_socket.h>
#include <net/socket.h>

int check_modem_fw_string()
{
	int err;
	socklen_t len;
	u8_t version[36];

	int fd = socket(AF_LOCAL, SOCK_STREAM, NPROTO_DFU);
	if (fd < 0) {
		LOG_ERR("Failed to open Modem DFU socket.");
		return fd;
	}
	len = sizeof(version);
	err = getsockopt(fd, SOL_DFU, SO_DFU_FW_VERSION, &version, &len);
	if (err < 0) {
		LOG_ERR("Firmware version request failed, errno %d", errno);
		return err;
	}

	err = close(fd);
	if (err < 0) {
		LOG_ERR("Failed to close modem DFU socket.");
		return err;
	}

	if (strncmp(previous_mdm_fw_version, version, 36) == 0){
		return -EFAULT;
	} else {
		return 0;
	}
}

int check_mcuboot_fw_metadata()
{
}

int check_update(struct mqtt_client *const client)
{
	int ret;
	int success = 0;

	if (!pending_update) {
		return 0;
	}

	switch(img_type)
	{
		case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
			/* Does not really make sense since this situation would never occure */
			/* We could just do success = true */
			success = check_modem_fw_string();
			break;
		case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
			//success = check_mcuboot_fw_metadata();
			break;
		default:
			return -EFAULT;
	}
	
	if (success) {
		update_job_execution(client, job_id, AWS_JOBS_FAILED, NONE,
				     -1, ""); 	
		ret = -EFAULT;
	} else {
		update_job_execution(client, job_id, AWS_JOBS_SUCCEEDED, NONE,
				     stored_progress, "");
		ret = 0;
	}

	pending_update = false;
	store_update_data();
	
	return ret;

}

int aws_fota_mqtt_evt_handler(struct mqtt_client *const client,
			      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			/* Expect more processing of CONNACK event by another
			 * MQTT Event Handler
			 */
			return 1;
		}

		err = check_update(client);
		if (err) {
			LOG_ERR("Check update failed");
		}

		err = aws_jobs_subscribe_topic_notify_next(client,
							   notify_next_topic);
		if (err) {
			LOG_ERR("Unable to subscribe to notify-next topic");
			return err;
		}

		err = aws_jobs_subscribe_topic_get(client, "$next", get_topic);
		if (err) {
			LOG_ERR("Unable to subscribe to jobs/$next/get");
			return err;
		}

		/* This expects that the application's mqtt handler will handle
		 * any situations where you could not connect to the MQTT
		 * broker.
		 */
		return 1;

	case MQTT_EVT_DISCONNECT:
		return 1;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;

		err = on_publish_evt(client,
				     p->message.topic.topic.utf8,
				     p->message.topic.topic.size,
				     p->message.payload.len);

		if (err < 0) {
			return err;
		} else if (err == 1) {
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
		return 0;

	} break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			return 0;
		}
		return 1;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			return evt->result;
		}
		if (evt->param.suback.message_id == SUBSCRIBE_NOTIFY_NEXT) {
			LOG_INF("subscribed to notify-next topic");
			err = aws_jobs_get_job_execution(client, "$next",
							 get_topic);
			if (err) {
				return err;
			}
			return 0;
		}

		if (evt->param.suback.message_id == SUBSCRIBE_GET) {
			LOG_INF("subscribed to get topic");
			return 0;
		}

		if ((fota_state == DOWNLOAD_FIRMWARE) &&
		   (evt->param.suback.message_id == SUBSCRIBE_JOB_ID_UPDATE)) {
			stored_progress = 0;
			err = update_job_execution(client, job_id,
						   AWS_JOBS_IN_PROGRESS,
						   fota_state, stored_progress,
						   "");
			if (err) {
				return err;
			}
			return 0;
		}

		return 1;

	default:
		/* Handling for default case */
		return 1;
	}
	return 1;
}


static void http_fota_handler(const struct fota_download_evt *evt)
{
	__ASSERT_NO_MSG(c != NULL);

	int err = 0;

	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download completed evt received");
		fota_state = APPLY_UPDATE;
		err = update_job_execution(c, job_id, AWS_JOBS_IN_PROGRESS,
					   fota_state, stored_progress, "");
		if (err != 0) {
			callback(AWS_FOTA_EVT_ERROR);
		}
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA download failed, report back");
		(void) update_job_execution(c, job_id, AWS_JOBS_FAILED,
					    fota_state, -1, "");
		callback(AWS_FOTA_EVT_ERROR);
		break;

#ifdef CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		stored_progress = evt->offset;
		err = update_job_execution(c, job_id, AWS_JOBS_IN_PROGRESS,
					   fota_state, stored_progress, "");
		break;
#endif
	}

}

int aws_fota_init(struct mqtt_client *const client,
		  aws_fota_callback_t evt_handler)
{
	int err;

	if (client == NULL || evt_handler == NULL) {
		return -EINVAL;
	}
		
	/* settings_subsys_init is idempotent so this is safe to do. */
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&sh);
	if (err) {
		LOG_ERR("Cannot register settings (err %d)", err);
		return err;
	}

	err = settings_load();
	if (err) {
		LOG_ERR("Cannot load settings (err %d)", err);
		return err;
	}


	LOG_INF("JOB_ID: %s", log_strdup(job_id));
	LOG_INF("pending_update: %d", pending_update);
	LOG_INF("version_number: %d", execution_version_number);
	LOG_INF("img_type: %d", img_type);
	char version_string[37];
	snprintf(version_string, sizeof(version_string), "%.*s",
		 sizeof(previous_mdm_fw_version), previous_mdm_fw_version);
	LOG_INF("modem_fw: %d", version_string);

	/* Store client to make it available in event handlers. */
	c = client;
	callback = evt_handler;

	err = fota_download_init(http_fota_handler);
	if (err != 0) {
		LOG_ERR("fota_download_init error %d", err);
		return err;
	}

	return 0;
}
