/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_fota.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_download.h"

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/fota_download.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#include <zephyr/dfu/mcuboot.h>
#endif

#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>
#endif

LOG_MODULE_REGISTER(nrf_cloud_fota, CONFIG_NRF_CLOUD_FOTA_LOG_LEVEL);

#define TOPIC_FOTA_RCV    "/jobs/rcv"
#define TOPIC_FOTA_REQ    "/jobs/req"
#define TOPIC_FOTA_UPDATE "/jobs/update"

#define BLE_TOPIC_FOTA_RCV    "/jobs/ble/rcv"
#define BLE_TOPIC_FOTA_REQ    "/jobs/ble/req"
#define BLE_TOPIC_FOTA_UPDATE "/jobs/ble/update"

#define JOB_REQUEST_LATEST_PAYLOAD "[\"\"]"

enum subscription_topic_index {
	SUB_TOPIC_IDX_RCV,
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
	SUB_TOPIC_IDX_BLE_RCV,
#endif
	SUB_TOPIC_IDX__COUNT,
};

static void http_fota_handler(const struct fota_download_evt *evt);
static void send_event(const enum nrf_cloud_fota_evt_id id,
		       const struct nrf_cloud_fota_job *const job);
static void cleanup_job(struct nrf_cloud_fota_job *const job);
static int start_job(struct nrf_cloud_fota_job *const job, const bool send_evt);
static int publish_job_status(struct nrf_cloud_fota_job *const job);
static int publish(const struct mqtt_publish_param *const pub);
static int save_validate_status(const char *const job_id,
			   const enum nrf_cloud_fota_type job_type,
			   const enum nrf_cloud_fota_validate_status status);
static int publish_validated_job_status(void);
static void reset_topics(void);

static struct mqtt_client *client_mqtt;
static nrf_cloud_fota_callback_t event_cb;

static struct mqtt_topic topic_updt = { .qos = MQTT_QOS_1_AT_LEAST_ONCE };
static struct mqtt_topic topic_req = { .qos = MQTT_QOS_1_AT_LEAST_ONCE };
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
static nrf_cloud_fota_ble_callback_t ble_cb;
static struct mqtt_topic topic_ble_updt = { .qos = MQTT_QOS_1_AT_LEAST_ONCE };
static struct mqtt_topic topic_ble_req = { .qos = MQTT_QOS_1_AT_LEAST_ONCE };
#endif

static struct mqtt_topic sub_topics[SUB_TOPIC_IDX__COUNT] = {
	{.qos = MQTT_QOS_1_AT_LEAST_ONCE},
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
	{.qos = MQTT_QOS_1_AT_LEAST_ONCE},
#endif
};

static enum fota_download_evt_id last_fota_dl_evt = FOTA_DOWNLOAD_EVT_ERROR;
static struct nrf_cloud_fota_job current_fota = {
	.info = { .type = NRF_CLOUD_FOTA_TYPE__INVALID }
};

static struct k_spinlock last_job_slock;
static uint8_t last_job[NRF_CLOUD_FOTA_JOB_ID_SIZE];
static struct nrf_cloud_settings_fota_job saved_job = {
	.type = NRF_CLOUD_FOTA_TYPE__INVALID
};
static bool initialized;
static bool fota_dl_initialized;
/** Flag to indicate that a pending FOTA job was validated and a reboot is required */
static bool reboot_on_init;

/** Flag to indicate that the device is waiting on an ACK after sending a job status
 * update to the cloud
 */
static bool fota_report_ack_pending;

static void update_last_job(char const *const job_id)
{
	if ((job_id == NULL) || (job_id[0] == '\0')) {
		return;
	}

	K_SPINLOCK(&last_job_slock) {
		strncpy(last_job, job_id, sizeof(last_job) - 1);
		last_job[sizeof(last_job) - 1] = '\0';
	}

	LOG_DBG("Updated last job: %s", job_id);
}

static bool is_last_job(char const *const job_id)
{
	bool match;

	K_SPINLOCK(&last_job_slock) {
		match = (strncmp(last_job, job_id, sizeof(last_job)) == 0);
	}

	return match;
}

static void send_fota_done_event_if_done(void)
{
	/* This event should cause reboot or modem-reinit.
	 * Do not cleanup the job since a reboot should
	 * occur or the user will call:
	 * nrf_cloud_modem_fota_completed() or nrf_cloud_fota_pending_job_validate()
	 * The validated job status will be sent after the reboot or upon
	 * reconnection to nRF Cloud (see nrf_cloud_fota_endpoint_set_and_report())
	 */
	if ((current_fota.status == NRF_CLOUD_FOTA_IN_PROGRESS) &&
	    (saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_PENDING)) {
		send_event(NRF_CLOUD_FOTA_EVT_DONE, &current_fota);
	}
}

static int pending_fota_job_validate(void)
{
	bool reboot = false;
	int err;

	err = nrf_cloud_pending_fota_job_process(&saved_job, &reboot);
	if (err) {
		return err;
	}

	/* Do not enforce a reboot for modem updates since they can also be handled by
	 * reinitializing the modem lib
	 */
	if (!nrf_cloud_fota_is_type_modem(saved_job.type)) {
		reboot_on_init = reboot;
	}

	if (save_validate_status(saved_job.id, saved_job.type, saved_job.validate)) {
		LOG_WRN("Failed to save status, job will be completed without validation");
	}

	return (int)reboot;
}

int nrf_cloud_fota_pending_job_type_get(enum nrf_cloud_fota_type * const pending_fota_type)
{
	if (!pending_fota_type) {
		return -EINVAL;
	}

	int err = nrf_cloud_fota_settings_load(&saved_job);

	*pending_fota_type = (err ? NRF_CLOUD_FOTA_TYPE__INVALID : saved_job.type);

	return err;
}

int nrf_cloud_fota_pending_job_validate(enum nrf_cloud_fota_type * const fota_type_out)
{
	int err = 0;

	if (fota_type_out) {
		err = nrf_cloud_fota_pending_job_type_get(fota_type_out);
	} else {
		err = nrf_cloud_fota_settings_load(&saved_job);
	}

	if (err) {
		return -EIO;
	}

	err = pending_fota_job_validate();

	cleanup_job(&current_fota);

	return err;
}

static void fota_reboot(void)
{
	LOG_INF("Rebooting to complete FOTA update...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

int nrf_cloud_fota_init(nrf_cloud_fota_callback_t cb)
{
	int ret;

	if (reboot_on_init) {
		fota_reboot();
	}

	if (cb == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	event_cb = cb;

	if (initialized) {
		return 0;
	}

	/* Ensure the codec is initialized */
	(void)nrf_cloud_codec_init(NULL);

	if (!fota_dl_initialized) {
		ret = fota_download_init(http_fota_handler);
		if (ret != 0) {
			LOG_ERR("fota_download_init error: %d", ret);
			return ret;
		}
		fota_dl_initialized = true;
	}

	/* Load FOTA settings and validate any pending job */
	ret = nrf_cloud_fota_pending_job_validate(NULL);

	/* This function returns 0 on success.
	 * If successful AND a FOTA job has completed, this function returns 1.
	 * Update ret variable appropriately.
	 */
	if (ret == 1) {
		fota_reboot();
	} else if (ret == 0) {
		/* Indicate that a FOTA update has occurred */
		ret = 1;
	} else if (ret == -ENODEV) {
		/* No job was pending validation, check for already validated modem job */
		if (nrf_cloud_fota_is_type_modem(saved_job.type) &&
		    (saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_PASS ||
		     saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_FAIL ||
		     saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_UNKNOWN)) {
			/* Device has just rebooted from a modem FOTA */
			LOG_INF("FOTA updated modem");
			ret = 1;
		} else {
			ret = 0;
		}
	} else {
		LOG_ERR("Failed to process pending FOTA job, error: %d", ret);
	}

	fota_report_ack_pending = false;

	initialized = true;
	return ret;
}

int nrf_cloud_fota_uninit(void)
{
	if (nrf_cloud_fota_is_active()) {
		return -EBUSY;
	}

	event_cb = NULL;
	initialized = false;

	(void)nrf_cloud_fota_unsubscribe();
	reset_topics();
	cleanup_job(&current_fota);

	return 0;
}

int nrf_cloud_modem_fota_completed(const bool fota_success)
{
	if (!nrf_cloud_fota_is_type_modem(saved_job.type) ||
	    saved_job.validate != NRF_CLOUD_FOTA_VALIDATE_PENDING) {
		return -EOPNOTSUPP;
	}

	cleanup_job(&current_fota);

	/* Failure to save settings here is not critical, ignore return */
	(void)save_validate_status(saved_job.id, saved_job.type,
		fota_success ? NRF_CLOUD_FOTA_VALIDATE_PASS : NRF_CLOUD_FOTA_VALIDATE_FAIL);

	return publish_validated_job_status();
}

static void reset_topic(struct mqtt_utf8 *const topic)
{
	if (!topic) {
		return;
	}

	nrf_cloud_free((void *)topic->utf8);
	topic->utf8 = NULL;
	topic->size = 0;
}

static void reset_topics(void)
{
	reset_topic(&sub_topics[SUB_TOPIC_IDX_RCV].topic);
	reset_topic(&topic_updt.topic);
	reset_topic(&topic_req.topic);
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
	reset_topic(&sub_topics[SUB_TOPIC_IDX_BLE_RCV].topic);
	reset_topic(&topic_ble_updt.topic);
	reset_topic(&topic_ble_req.topic);
#endif
}

static int build_topic(const char *const client_id,
		       const struct mqtt_utf8 *const endpoint,
		       const char *const topic_str,
		       struct mqtt_utf8 *const topic_out)
{
	int ret;

	char *buf;
	size_t size = endpoint->size + strlen(client_id) +
		      strlen(topic_str) + 1;

	buf = nrf_cloud_calloc(size, 1);

	if (!buf) {
		ret = -ENOMEM;
		reset_topic(topic_out);
		return ret;
	}

	ret = snprintk(buf, size, "%s%s%s",
		       endpoint->utf8, client_id, topic_str);

	if (ret <= 0 || ret >= size) {
		ret = -E2BIG;
		nrf_cloud_free(buf);
		return ret;
	}

	topic_out->utf8 = buf;
	topic_out->size = ret;

	return 0;
}

static int publish_validated_job_status(void)
{
	int ret = 0;

	if (saved_job.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		return 1;
	}

	if (saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_PENDING) {
		return -EOPNOTSUPP;
	}

	struct nrf_cloud_fota_job job = {
		.info = {
			.type = saved_job.type,
			.id = saved_job.id
		}
	};

	switch (saved_job.validate) {
	case NRF_CLOUD_FOTA_VALIDATE_UNKNOWN:
		job.error = NRF_CLOUD_FOTA_ERROR_UNABLE_TO_VALIDATE;
		/* Fall-through */
	case NRF_CLOUD_FOTA_VALIDATE_PASS:
		job.status = NRF_CLOUD_FOTA_SUCCEEDED;
		break;
	case NRF_CLOUD_FOTA_VALIDATE_ERROR:
		if (current_fota.info.type != NRF_CLOUD_FOTA_TYPE__INVALID) {
			/* Use current FOTA info if valid */
			job.status = current_fota.status;
			job.error = current_fota.error;
		} else {
			/* Default to download error */
			job.status = NRF_CLOUD_FOTA_FAILED;
			job.error = NRF_CLOUD_FOTA_ERROR_DOWNLOAD;
		}
		break;
	case NRF_CLOUD_FOTA_VALIDATE_FAIL:
		job.status = NRF_CLOUD_FOTA_FAILED;
		break;
	default:
		LOG_ERR("Unexpected job validation status: %d",
			saved_job.validate);
		ret = save_validate_status(job.info.id, job.info.type,
					   NRF_CLOUD_FOTA_VALIDATE_DONE);
		job.info.type = NRF_CLOUD_FOTA_TYPE__INVALID;
		break;
	}

	if (job.info.type != NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_DBG("Sending job update for validated (%d) job", saved_job.validate);
		ret = publish_job_status(&job);
		if (ret) {
			LOG_ERR("Error sending job update: %d", ret);
		} else {
			/* Validated status sent to the cloud, update the last job string */
			update_last_job(job.info.id);
		}
	}

	return ret;
}

int nrf_cloud_fota_endpoint_set_and_report(struct mqtt_client *const client,
	const char *const client_id, const struct mqtt_utf8 *const endpoint)
{
	int ret = nrf_cloud_fota_endpoint_set(client, client_id, endpoint);

	if (ret) {
		LOG_ERR("Failed to set FOTA endpoint: %d", ret);
		return ret;
	}

	if ((current_fota.status == NRF_CLOUD_FOTA_IN_PROGRESS) &&
	    (saved_job.validate == NRF_CLOUD_FOTA_VALIDATE_PENDING)) {
		/* In case of a prior disconnection from nRF Cloud during the
		 * FOTA update, publish job status to the cloud
		 */
		ret = publish_job_status(&current_fota);
		if (ret < 0) {
			/* If the job update fails to be sent here, proceed to inform
			 * the application of a complete job so it can be validated.
			 * Job update will be sent on reboot/re-init.
			 */
			send_fota_done_event_if_done();
		}
	} else {
		ret = publish_validated_job_status();
		/* Positive value indicates no job exists, ignore */
		if (ret > 0) {
			ret = 0;
		}
	}

	return ret;
}

int nrf_cloud_fota_endpoint_set(struct mqtt_client *const client,
				const char *const client_id,
				const struct mqtt_utf8 *const endpoint)
{
	int ret;

	if (client == NULL || endpoint == NULL ||
	    endpoint->utf8 == NULL || endpoint->size == 0 ||
	    client_id == NULL)  {
		return -EINVAL;
	}

	client_mqtt = client;

	reset_topics();

	ret = build_topic(client_id, endpoint, TOPIC_FOTA_RCV,
			  &sub_topics[SUB_TOPIC_IDX_RCV].topic);
	if (ret) {
		goto error_cleanup;
	}

	ret = build_topic(client_id, endpoint, TOPIC_FOTA_UPDATE,
			  &topic_updt.topic);
	if (ret) {
		goto error_cleanup;
	}

	ret = build_topic(client_id, endpoint, TOPIC_FOTA_REQ,
			  &topic_req.topic);
	if (ret) {
		goto error_cleanup;
	}

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
	ret = build_topic(client_id, endpoint, BLE_TOPIC_FOTA_RCV,
			  &sub_topics[SUB_TOPIC_IDX_BLE_RCV].topic);
	if (ret) {
		goto error_cleanup;
	}

	ret = build_topic(client_id, endpoint, BLE_TOPIC_FOTA_UPDATE,
			  &topic_ble_updt.topic);
	if (ret) {
		goto error_cleanup;
	}

	ret = build_topic(client_id, endpoint, BLE_TOPIC_FOTA_REQ,
			  &topic_ble_req.topic);
	if (ret) {
		goto error_cleanup;
	}
#endif

	return 0;

error_cleanup:
	reset_topics();
	return ret;
}

void nrf_cloud_fota_endpoint_clear(void)
{
	client_mqtt = NULL;
	reset_topics();
}

int nrf_cloud_fota_subscribe(void)
{
	struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = NCT_MSG_ID_FOTA_SUB
	};

	for (int i = 0; i < sub_list.list_count; ++i) {
		if (sub_list.list[i].topic.size == 0 ||
		    sub_list.list[i].topic.utf8 == NULL) {
			return -EFAULT;
		}
		LOG_DBG("Subscribing to topic: %s", (char *)sub_list.list[i].topic.utf8);
	}

	return mqtt_subscribe(client_mqtt, &sub_list);
}

int nrf_cloud_fota_unsubscribe(void)
{
	struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = NCT_MSG_ID_FOTA_UNSUB
	};

	for (int i = 0; i < sub_list.list_count; ++i) {
		if (sub_list.list[i].topic.size == 0 ||
		    sub_list.list[i].topic.utf8 == NULL) {
			return -EFAULT;
		}
	}

	return mqtt_unsubscribe(client_mqtt, &sub_list);
}

bool nrf_cloud_fota_is_active(void)
{
	return ((current_fota.info.type != NRF_CLOUD_FOTA_TYPE__INVALID) &&
		(current_fota.status != NRF_CLOUD_FOTA_QUEUED));
}

bool nrf_cloud_fota_is_available(void)
{
	return current_fota.info.type != NRF_CLOUD_FOTA_TYPE__INVALID;
}

static int save_validate_status(const char *const job_id,
			   const enum nrf_cloud_fota_type job_type,
			   const enum nrf_cloud_fota_validate_status validate)
{
#if defined(CONFIG_SHELL)
	if (job_id == NULL) {
		LOG_WRN("No job_id; assuming CLI-invoked FOTA.");
		return 0;
	}
#endif
	__ASSERT_NO_MSG(job_id != NULL);

	int ret;

	LOG_DBG("%s() - %s, %d, %d", __func__, job_id, job_type, validate);

	if (validate == NRF_CLOUD_FOTA_VALIDATE_DONE) {
		/* Saved FOTA job has been validated, clear it */
		saved_job.type = NRF_CLOUD_FOTA_TYPE__INVALID;
		saved_job.validate = NRF_CLOUD_FOTA_VALIDATE_NONE;
		saved_job.bl_flags = NRF_CLOUD_FOTA_BL_STATUS_CLEAR;
		memset(saved_job.id, 0, sizeof(saved_job.id));
	} else {
		saved_job.type = job_type;
		saved_job.validate = validate;
		if (job_id != saved_job.id) {
			strncpy(saved_job.id, job_id, sizeof(saved_job.id) - 1);
			saved_job.id[sizeof(saved_job.id) - 1] = '\0';
		}

		ret = nrf_cloud_bootloader_fota_slot_set(&saved_job);
		if (ret) {
			LOG_WRN("Failed to set active bootloader (B1) slot flag");
		}
	}

	ret = nrf_cloud_fota_settings_save(&saved_job);
	if (ret) {
		LOG_ERR("settings_save_one failed: %d", ret);
	}

	return ret;
}

static void http_fota_handler(const struct fota_download_evt *evt)
{
	__ASSERT_NO_MSG(evt != NULL);

	int ret = 0;

	LOG_DBG("evt: %d", evt->id);

	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("Download complete");
		nrf_cloud_download_end();

		if (current_fota.status == NRF_CLOUD_FOTA_DOWNLOADING &&
		    current_fota.sent_dl_progress != 100) {
			/* Send 100% downloaded update */
			current_fota.dl_progress = 100;
			current_fota.sent_dl_progress = 100;
			send_event(NRF_CLOUD_FOTA_EVT_DL_PROGRESS, &current_fota);
			(void)publish_job_status(&current_fota);
		}

		/* MCUBOOT or MODEM full: download finished, update job status and install/reboot */
		current_fota.status = NRF_CLOUD_FOTA_IN_PROGRESS;
		save_validate_status(current_fota.info.id,
				     current_fota.info.type,
				     NRF_CLOUD_FOTA_VALIDATE_PENDING);
		ret = publish_job_status(&current_fota);
		break;

	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		/* MODEM delta: DFU area erasure started, before download can start. */
		send_event(NRF_CLOUD_FOTA_EVT_ERASE_PENDING, &current_fota);
		break;

	case FOTA_DOWNLOAD_EVT_ERASE_TIMEOUT:
		/* MODEM delta: this event is received when the erasure
		 * has been going on for CONFIG_DFU_TARGET_MODEM_TIMEOUT already.
		 */
		send_event(NRF_CLOUD_FOTA_EVT_ERASE_TIMEOUT, &current_fota);
		break;

	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		/* MODEM delta: this event is received when the initial
		 * fragment is downloaded and dfu_target_modem_init() is
		 * called.
		 */
		send_event(NRF_CLOUD_FOTA_EVT_ERASE_DONE, &current_fota);
		break;

	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_DBG("FOTA_DOWNLOAD_EVT_ERROR cause: %d", evt->cause);

		nrf_cloud_download_end();
		current_fota.status = NRF_CLOUD_FOTA_FAILED;

		if (last_fota_dl_evt == FOTA_DOWNLOAD_EVT_ERASE_DONE ||
		    evt->cause == FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE) {
			current_fota.status = NRF_CLOUD_FOTA_REJECTED;
		} else if (evt->cause == FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH) {
			current_fota.error = NRF_CLOUD_FOTA_ERROR_MISMATCH;
		} else {
			current_fota.error = NRF_CLOUD_FOTA_ERROR_DOWNLOAD;
		}

		/* Save validate status as ERROR in case the cloud does not
		 * receive the job status update; it can be sent later.
		 */
		save_validate_status(current_fota.info.id,
				     current_fota.info.type,
				     NRF_CLOUD_FOTA_VALIDATE_ERROR);
		ret = publish_job_status(&current_fota);
		send_event(NRF_CLOUD_FOTA_EVT_ERROR, &current_fota);
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
		current_fota.status = NRF_CLOUD_FOTA_DOWNLOADING;
		current_fota.dl_progress = evt->progress;

		/* Do not send complete status more than once */
		if ((current_fota.sent_dl_progress == 100) &&
		    (current_fota.dl_progress == 100)) {
			break;
		}

		/* Reset if new progress is less than previous */
		if (current_fota.sent_dl_progress >
		    current_fota.dl_progress) {
			current_fota.sent_dl_progress = 0;
		}

		/* Only send progress update when finished and if increment
		 * threshold is met
		 */
		if (current_fota.dl_progress != 100 &&
		    (((current_fota.dl_progress -
		      current_fota.sent_dl_progress) <
		     CONFIG_NRF_CLOUD_FOTA_PROGRESS_PCT_INCREMENT) ||
		     (CONFIG_NRF_CLOUD_FOTA_PROGRESS_PCT_INCREMENT == 0))) {
			break;
		}

		current_fota.sent_dl_progress = current_fota.dl_progress;
		send_event(NRF_CLOUD_FOTA_EVT_DL_PROGRESS, &current_fota);
		ret = publish_job_status(&current_fota);
		break;
	default:
		break;
	}

	if (ret) {
		LOG_ERR("Failed to send job status to cloud: %d", ret);
		/* The done event is normally sent on the MQTT ACK, but if the
		 * status update fails there will be no ACK. Send the done event
		 * so the device can proceed to install the FOTA update.
		 */
		send_fota_done_event_if_done();
	}

	last_fota_dl_evt = evt->id;
}

static void send_event(const enum nrf_cloud_fota_evt_id id,
		       const struct nrf_cloud_fota_job *const job)
{
	__ASSERT_NO_MSG(job != NULL);

	struct nrf_cloud_fota_evt evt = {
		.id = id,
		.status = job->status,
		.type = job->info.type
	};

	switch (id) {
	case NRF_CLOUD_FOTA_EVT_ERROR:
		evt.evt_data.error = job->error;
		break;
	case NRF_CLOUD_FOTA_EVT_DL_PROGRESS:
		evt.evt_data.dl_progress = job->dl_progress;
		break;
	default:
		break;
	}

	if (event_cb) {
		event_cb(&evt);
	}

}

int nrf_cloud_fota_job_start(void)
{
	if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB)) {
		return -ENOTSUP;
	}

	/* No job is available */
	if (current_fota.info.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_INF("No pending FOTA job");
		return -ENOENT;
	}

	/* Job is in progress */
	if (current_fota.status != NRF_CLOUD_FOTA_QUEUED) {
		LOG_INF("FOTA job already in progress");
		return -EINPROGRESS;
	}

	return start_job(&current_fota, false);
}

static int start_job(struct nrf_cloud_fota_job *const job, const bool send_evt)
{
	__ASSERT_NO_MSG(job != NULL);
	static const int sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;
	int ret = 0;

	enum dfu_target_image_type img_type;

	switch (job->info.type) {
	case NRF_CLOUD_FOTA_BOOTLOADER:
	case NRF_CLOUD_FOTA_APPLICATION:
		img_type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
		break;
	case NRF_CLOUD_FOTA_MODEM_DELTA:
		img_type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
		break;
	case NRF_CLOUD_FOTA_MODEM_FULL:
		if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)) {
			img_type = DFU_TARGET_IMAGE_TYPE_FULL_MODEM;
		} else {
			LOG_ERR("Not configured for full modem FOTA");
			ret = -EFTYPE;
		}
		break;
	default:
		LOG_ERR("Unhandled FOTA type: %d", job->info.type);
		ret = -EFTYPE;
	}

	if (ret == -EFTYPE) {
		job->status = NRF_CLOUD_FOTA_REJECTED;
		job->error = NRF_CLOUD_FOTA_ERROR_BAD_TYPE;
		return ret;
	}

	struct nrf_cloud_download_data dl = {
		.type = NRF_CLOUD_DL_TYPE_FOTA,
		.host = job->info.host,
		.path = job->info.path,
		.dl_cfg = {
			.sec_tag_list = &sec_tag,
			.sec_tag_count = (sec_tag < 0 ? 0 : 1),
			.pdn_id = 0,
			.frag_size_override = CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE,
		},
		.fota = { .expected_type = img_type }
	};

	ret = nrf_cloud_download_start(&dl);
	if (ret) {
		LOG_ERR("Failed to start FOTA download: %d", ret);
		ret = -EPIPE;
		job->status = NRF_CLOUD_FOTA_FAILED;
		job->error = NRF_CLOUD_FOTA_ERROR_DOWNLOAD_START;
		if (send_evt) {
			send_event(NRF_CLOUD_FOTA_EVT_ERROR, job);
		}
	} else {
		LOG_INF("Downloading update");
		job->dl_progress = 0;
		job->sent_dl_progress = 0;
		job->status = NRF_CLOUD_FOTA_DOWNLOADING;
		if (send_evt) {
			send_event(NRF_CLOUD_FOTA_EVT_START, job);
		}
	}

	/* Send job status to the cloud */
	(void)publish_job_status(job);

	return ret;
}

static void cleanup_job(struct nrf_cloud_fota_job *const job)
{
	__ASSERT_NO_MSG(job != NULL);
	LOG_DBG("%s() - ID: %s", __func__,
		job->info.id ? job->info.id : "N/A");

	/* Free the job info memory */
	nrf_cloud_fota_job_free(&job->info);

	/* Reset the struct */
	memset(job, 0, sizeof(*job));
	job->info.type = NRF_CLOUD_FOTA_TYPE__INVALID;
	/* Initial status state */
	job->status = NRF_CLOUD_FOTA_QUEUED;
}

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
static void cleanup_ble_job(struct nrf_cloud_fota_ble_job *const job)
{
	__ASSERT_NO_MSG(job != NULL);
	LOG_DBG("%s() - ID: %s", __func__,
		job->info.id ? job->info.id : "N/A");

	/* Free the job info memory */
	nrf_cloud_fota_job_free(&job->info);

	/* Reset the struct */
	memset(job, 0, sizeof(*job));
	job->info.type = NRF_CLOUD_FOTA_TYPE__INVALID;
}
#endif

static int publish(const struct mqtt_publish_param *const pub)
{
	__ASSERT_NO_MSG(pub != NULL);

	int ret;

	LOG_DBG("Topic: %s", (char *)pub->message.topic.topic.utf8);
	LOG_DBG("Payload (%d bytes): %s",
		pub->message.payload.len,
		(char *)pub->message.payload.data);

	ret = mqtt_publish(client_mqtt, pub);
	if (ret) {
		LOG_ERR("Publish failed: %d", ret);
	}
	return ret;
}

static int publish_and_free_obj(struct nrf_cloud_obj *const pub_data,
			    const struct mqtt_topic *const topic,
			    struct mqtt_publish_param *const pub_param)
{
	int err;

	if (!pub_data || !topic || !pub_param) {
		err = -EINVAL;
	} else {
		err = nrf_cloud_obj_cloud_encode(pub_data);
	}

	(void)nrf_cloud_obj_free(pub_data);

	if (!err) {
		pub_param->message.topic = *topic;
		pub_param->message.payload.data = (uint8_t *)pub_data->encoded_data.ptr;
		pub_param->message.payload.len = pub_data->encoded_data.len;
		err = publish(pub_param);

		if ((!err) && (pub_param->message_id == NCT_MSG_ID_FOTA_REPORT)) {
			fota_report_ack_pending = true;
		}

		(void)nrf_cloud_obj_cloud_encoded_free(pub_data);
	}

	return err;
}

static bool is_job_status_terminal(const enum nrf_cloud_fota_status status)
{
	switch (status) {
	case NRF_CLOUD_FOTA_FAILED:
	case NRF_CLOUD_FOTA_SUCCEEDED:
	case NRF_CLOUD_FOTA_REJECTED:
	case NRF_CLOUD_FOTA_TIMED_OUT:
	case NRF_CLOUD_FOTA_CANCELED:
		return true;
	default:
		return false;
	}
}

static int publish_job_status(struct nrf_cloud_fota_job *const job)
{
	/* ensure shell-invoked fota doesn't crash below */
	if ((job == NULL) || (job->info.id == NULL)) {
		return -EINVAL;
	} else if (client_mqtt == NULL) {
		return -ENXIO;
	} else if (topic_updt.topic.utf8 == NULL) {
		return -EHOSTUNREACH;
	}

	int ret;
	struct mqtt_publish_param param = {
		.message_id = NCT_MSG_ID_FOTA_REPORT,
		.dup_flag = 0,
		.retain_flag = 0,
	};
	NRF_CLOUD_OBJ_JSON_DEFINE(update_obj);

	ret = nrf_cloud_obj_fota_job_update_create(&update_obj, job);
	if (ret) {
		return -ENOMEM;
	}

	return publish_and_free_obj(&update_obj, &topic_updt, &param);
}

int nrf_cloud_fota_update_check(void)
{
	if (client_mqtt == NULL) {
		return -ENXIO;
	} else if (topic_req.topic.utf8 == NULL) {
		return -EHOSTUNREACH;
	}

	struct mqtt_publish_param param = {
		.message_id = NCT_MSG_ID_FOTA_REQUEST,
		.dup_flag = 0,
		.retain_flag = 0,
	};

	param.message.topic = topic_req;
	param.message.payload.data = JOB_REQUEST_LATEST_PAYLOAD;
	param.message.payload.len = sizeof(JOB_REQUEST_LATEST_PAYLOAD) - 1;

	return publish(&param);
}

static int handle_mqtt_evt_publish(const struct mqtt_evt *evt)
{
	int ret;
	char *payload;
	size_t payload_len;
	bool skip = false;
	bool reject_job = false;
	bt_addr_t *ble_id = NULL;
	struct nrf_cloud_fota_job_info *job_info = &current_fota.info;
	const struct mqtt_publish_param *p = &evt->param.publish;
	struct mqtt_puback_param ack = {
		.message_id = p->message_id
	};

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
	struct nrf_cloud_fota_ble_job ble_job;

	if (strstr(sub_topics[SUB_TOPIC_IDX_BLE_RCV].topic.utf8,
		   p->message.topic.topic.utf8) != NULL) {
		job_info = &ble_job.info;
		ble_id = &ble_job.ble_id;
	}
#endif

	if (strstr(sub_topics[SUB_TOPIC_IDX_RCV].topic.utf8,
		   p->message.topic.topic.utf8) == NULL && !ble_id) {
		/* This MQTT event is not intended nRF Cloud FOTA */
		return 1;
	}

	LOG_DBG("MQTT_EVT_PUBLISH: id = %d len = %d",
		p->message_id,
		p->message.payload.len);
	payload_len = p->message.payload.len + 1;
	payload = nrf_cloud_calloc(payload_len, 1);
	if (!payload) {
		LOG_ERR("Unable to allocate memory for job");
		ret = -ENOMEM;
		goto send_ack;
	}

	/* Always read the MQTT payload even if it is not needed */
	ret = mqtt_readall_publish_payload(client_mqtt, payload, p->message.payload.len);
	if (ret) {
		LOG_ERR("Error reading MQTT payload: %d", ret);
		goto send_ack;
	}

	if (nrf_cloud_fota_is_available() && !ble_id) {
		LOG_INF("A job is already available or in progress... skipping");
		skip = true;
		goto send_ack;
	}

	struct nrf_cloud_data input = { .ptr = payload, .len = payload_len };

	if (nrf_cloud_fota_job_decode(job_info, ble_id, &input)) {
		/* Error parsing job, if a job ID exists, reject the job */
		reject_job = (job_info->id != NULL);
	} else if (is_last_job(job_info->id)) {
		/* Job parsed and already processed */
		skip = true;
		LOG_INF("Job %s already completed... skipping", last_job);
		nrf_cloud_fota_job_free(job_info);
	} else {
		LOG_DBG("Job ID: %s, type: %d, size: %d",
			job_info->id,
			job_info->type,
			job_info->file_size);
		LOG_DBG("File: %s/%s",
			job_info->host,
			job_info->path);
	}

send_ack:
	/* Cleanup the payload buffer which is no longer needed */
	nrf_cloud_free((void *)payload);

	/* Send the ACK if required and then continue processing, which may result
	 * in an additional MQTT transaction
	 */
	if (p->message.topic.qos == MQTT_QOS_0_AT_MOST_ONCE) {
		LOG_DBG("No ack required");
	} else {
		int ack_res = mqtt_publish_qos1_ack(client_mqtt, &ack);

		if (ack_res) {
			LOG_ERR("MQTT ACK failed: %d", ack_res);
			if (!ret) {
				ret = ack_res;
			}
		}
	}

	if (ble_id && reject_job) {
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
		ble_job.error = NRF_CLOUD_FOTA_ERROR_BAD_JOB_INFO;
		(void)nrf_cloud_fota_ble_job_update(&ble_job, NRF_CLOUD_FOTA_REJECTED);
		cleanup_ble_job(&ble_job);
#endif
	} else if (reject_job) {
		current_fota.error = NRF_CLOUD_FOTA_ERROR_BAD_JOB_INFO;
		current_fota.status = NRF_CLOUD_FOTA_REJECTED;
		(void)publish_job_status(&current_fota);
		cleanup_job(&current_fota);
	}

	if (skip || job_info->type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		return ret;
	}

	if (ble_id) {
#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
		if (ble_cb) {
			ble_cb(&ble_job);
			cleanup_ble_job(&ble_job);
		}
#endif
	} else if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB)) {
		/* Start update */
		(void)start_job(&current_fota, true);
	} else {
		/* Inform application of job */
		send_event(NRF_CLOUD_FOTA_EVT_JOB_RCVD, &current_fota);
	}

	return 0;
}

int nrf_cloud_fota_mqtt_evt_handler(const struct mqtt_evt *evt)
{
	if (sub_topics[SUB_TOPIC_IDX_RCV].topic.utf8 == NULL ||
	    sub_topics[SUB_TOPIC_IDX_RCV].topic.size == 0) {
		/* Ignore MQTT until a topic has been set */
		return 1;
	}

	switch (evt->type) {
	case MQTT_EVT_PUBLISH: {
		return handle_mqtt_evt_publish(evt);
	}
	case MQTT_EVT_SUBACK:
	{
		int ret = -1;

		if (evt->param.suback.message_id != NCT_MSG_ID_FOTA_SUB) {
			return 1;
		}
		LOG_DBG("MQTT_EVT_SUBACK");

		if (fota_report_ack_pending) {
			if (current_fota.info.type != NRF_CLOUD_FOTA_TYPE__INVALID) {
				LOG_DBG("Sending job updated due to missing ACK");
				ret = publish_job_status(&current_fota);
			} else {
				/* No current job info, check for a validated job */
				ret = publish_validated_job_status();
				if (ret == 1) {
					/* No validated job either, clear flag */
					fota_report_ack_pending = false;
				}
			}
		}

		if (ret != 0) {
			/* Check for an update if job status was not published */
			nrf_cloud_fota_update_check();
		}

		break;
	}
	case MQTT_EVT_UNSUBACK:
	{
		if (evt->param.unsuback.message_id != NCT_MSG_ID_FOTA_UNSUB) {
			return 1;
		}
		LOG_DBG("MQTT_EVT_UNSUBACK");
		break;
	}
	case MQTT_EVT_PUBACK:
	{
		bool fota_report_ack = false;

		switch (evt->param.puback.message_id) {
		case NCT_MSG_ID_FOTA_REPORT:
			fota_report_ack = true;
		case NCT_MSG_ID_FOTA_REQUEST:
		case NCT_MSG_ID_FOTA_BLE_REPORT:
		case NCT_MSG_ID_FOTA_BLE_REQUEST:
			break;
		default:
			return 1;
		}

		LOG_DBG("MQTT_EVT_PUBACK: msg id %d",
			evt->param.puback.message_id);

		if (!fota_report_ack) {
			/* Nothing to do */
			break;
		}

		/* Job status updated on the cloud, now process the local saved status */
		fota_report_ack_pending = false;

		/* If job was updated to terminal status, save job ID and cleanup.
		 * This was likely due to an error, where no validation was required.
		 */
		if (is_job_status_terminal(current_fota.status)) {
			update_last_job(current_fota.info.id);
			cleanup_job(&current_fota);
		}

		switch (saved_job.validate) {
		case NRF_CLOUD_FOTA_VALIDATE_ERROR:
		case NRF_CLOUD_FOTA_VALIDATE_PASS:
		case NRF_CLOUD_FOTA_VALIDATE_UNKNOWN:
		case NRF_CLOUD_FOTA_VALIDATE_FAIL:
			save_validate_status(saved_job.id, saved_job.type,
					     NRF_CLOUD_FOTA_VALIDATE_DONE);
			break;
		case NRF_CLOUD_FOTA_VALIDATE_PENDING:
			send_fota_done_event_if_done();
			break;
		default:
			break;
		}

		break;
	}
	case MQTT_EVT_DISCONNECT:
		nrf_cloud_fota_endpoint_clear();
		/* Fall-through */
	case MQTT_EVT_CONNACK:
	case MQTT_EVT_PUBREC:
	case MQTT_EVT_PUBREL:
	case MQTT_EVT_PUBCOMP:
	case MQTT_EVT_PINGRESP:
		return 1;
	break;
	}

	return 0;
}

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
int nrf_cloud_fota_ble_set_handler(nrf_cloud_fota_ble_callback_t cb)
{
	if (!cb) {
		return -EINVAL;
	}

	ble_cb = cb;
	return 0;
}

int nrf_cloud_fota_ble_update_check(const bt_addr_t *const ble_id)
{
	if (ble_id == NULL) {
		return -EINVAL;
	} else if (client_mqtt == NULL) {
		return -ENXIO;
	} else if (topic_ble_req.topic.utf8 == NULL) {
		return -EHOSTUNREACH;
	}

	int ret;
	struct mqtt_publish_param param = {
		.message_id = NCT_MSG_ID_FOTA_BLE_REQUEST,
		.dup_flag = 0,
		.retain_flag = 0,
	};
	NRF_CLOUD_OBJ_JSON_DEFINE(check_obj);

	ret = nrf_cloud_obj_fota_ble_job_request_create(&check_obj, ble_id);
	if (ret) {
		LOG_ERR("Failed to create BLE FOTA job update check, error: %d", ret);
		return ret;
	}

	return publish_and_free_obj(&check_obj, &topic_ble_req, &param);
}

int nrf_cloud_fota_ble_job_update(const struct nrf_cloud_fota_ble_job *const ble_job,
				  const enum nrf_cloud_fota_status status)
{
	if (ble_job == NULL) {
		return -EINVAL;
	} else if (client_mqtt == NULL) {
		return -ENXIO;
	} else if (topic_ble_updt.topic.utf8 == NULL) {
		return -EHOSTUNREACH;
	}

	int ret;
	struct mqtt_publish_param param = {
		.message_id = NCT_MSG_ID_FOTA_BLE_REPORT,
		.dup_flag = 0,
		.retain_flag = 0,
	};
	NRF_CLOUD_OBJ_JSON_DEFINE(job_obj);

	ret = nrf_cloud_obj_fota_ble_job_update_create(&job_obj, ble_job, status);
	if (ret) {
		LOG_ERR("Failed to create BLE FOTA job update, error: %d", ret);
		return ret;
	}

	return publish_and_free_obj(&job_obj, &topic_ble_updt, &param);
}
#endif
