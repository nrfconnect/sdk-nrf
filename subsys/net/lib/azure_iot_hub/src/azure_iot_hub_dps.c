/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <net/azure_iot_hub.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <settings/settings.h>

#include "azure_iot_hub_dps.h"
#include "azure_iot_hub_topic.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub_dps, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

#define DPS_REG_STATUS_UPDATE_MAX_RETRIES	10
#define DPS_OPERATION_ID_MAX_LEN		60
#define DPS_TOPIC_OPERATION_ID_MAX_LEN		(DPS_OPERATION_ID_MAX_LEN + 100)
#define DPS_REGISTERED_HUB_MAX_LEN		100
#define DPS_SETTINGS_KEY			"azure_iot_hub"
#define DPS_SETTINGS_HOSTNAME_LEN_KEY		"hostname_len"
#define DPS_SETTINGS_HOSTNAME_KEY		"hostname"

/* Periodically send messages to receive registration status.
 * Keep polling as long as returned status code to $dps/registrations/res/#
 * is 202. If it's 200, registration succeeded.
 */
#define DPS_TOPIC_REG_STATUS	\
	"$dps/registrations/GET/iotdps-get-operationstatus/" \
	"?$rid=%s&operationId=%s"
#define DPS_TOPIC_REG		"$dps/registrations/res/"
#define DPS_TOPIC_REG_SUB	DPS_TOPIC_REG "#"
#define DPS_TOPIC_REG_PUB	"$dps/registrations/PUT/iotdps-register/" \
				"?$rid=%s"

#define DPS_REG_PAYLOAD "{registrationId:\"%s\"}"

struct dps_reg_status {
	enum dps_reg_state state;
	struct k_delayed_work poll_work;
	uint32_t retry;
	/* Add 1 to make sure null termination can be used */
	char reg_id[CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN + 1];
	char operation_id[DPS_OPERATION_ID_MAX_LEN];
	char assigned_hub[DPS_REGISTERED_HUB_MAX_LEN];
	uint32_t assigned_hub_len;
	struct k_sem registration_finished;
	struct k_sem settings_loaded;
};

static dps_handler_t cb_handler;
static struct dps_reg_status dps_reg_status;
static struct mqtt_client *mqtt_client;
static char dps_topic_reg_pub[sizeof(DPS_TOPIC_REG_PUB) +
			      sizeof(dps_reg_status.reg_id)];

/* Build time asserts */

/* If DPS is enabled, the ID scope must be defined */
BUILD_ASSERT(sizeof(CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE) - 1 > 0,
	     "The DPS ID scope must be defined");

/* Forward declarations */
static int dps_on_settings_loaded(void);
static int dps_settings_handler(const char *key, size_t len,
				settings_read_cb read_cb, void *cb_arg);

SETTINGS_STATIC_HANDLER_DEFINE(azure_iot_hub_dps, DPS_SETTINGS_KEY, NULL,
			       dps_settings_handler, dps_on_settings_loaded,
			       NULL);

/**@brief Callback when settings_load() is called. */
static int dps_settings_handler(const char *key, size_t len,
				settings_read_cb read_cb, void *cb_arg)
{
	int err;

	if (strcmp(key, DPS_SETTINGS_HOSTNAME_LEN_KEY) == 0) {
		err = read_cb(cb_arg, &dps_reg_status.assigned_hub_len,
			      sizeof(dps_reg_status.assigned_hub_len));
		if (err < 0) {
			LOG_ERR("Failed to read hostname length, error: %d",
				err);
			return err;
		}

		return 0;
	}

	if (strcmp(key, DPS_SETTINGS_HOSTNAME_KEY) == 0) {
		err = read_cb(cb_arg, dps_reg_status.assigned_hub,
			      dps_reg_status.assigned_hub_len);
		if (err < 0) {
			LOG_ERR("Failed to read hostname, error: %d", err);
			return err;
		}

		dps_reg_status.state = DPS_STATE_REGISTERED;

		LOG_DBG("Azure IoT Hub hostname found, DPS not needed: %s",
			log_strdup(dps_reg_status.assigned_hub));
	}

	return 0;
}

static int dps_save_hostname(const uint8_t *buf, uint32_t buf_len)
{
	int err;

	err = settings_save_one(
		DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_LEN_KEY,
		&buf_len, sizeof(buf_len));
	if (err) {
		return err;
	}

	err = settings_save_one(DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_KEY,
				buf, buf_len);
	if (err) {
		return err;
	}

	LOG_DBG("Azure IoT Hub acquired using DPS was successfully saved");

	return 0;
}

static int dps_on_settings_loaded(void)
{
	LOG_DBG("Settings fully loaded");

	k_sem_give(&dps_reg_status.settings_loaded);

	return 0;
}

static int dps_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Failed to initialize settings subsystem, error: %d",
			err);
		return err;
	}

	/* This module loads settings for all application modules */
	err = settings_load_subtree(DPS_SETTINGS_KEY);
	if (err) {
		LOG_ERR("Cannot load settings");
		return err;
	}

	return 0;
}

static void dps_reg_status_work_fn(struct k_work *work)
{
	int err;
	struct mqtt_publish_param param = {
		.message.payload.data = NULL,
		.message.payload.len = 0,
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = k_uptime_get_32(),
	};
	static uint32_t retry_count;
	struct dps_reg_status *reg_status =
		CONTAINER_OF(work, struct dps_reg_status, poll_work);
	char topic[DPS_TOPIC_OPERATION_ID_MAX_LEN];
	size_t topic_len;

	if (dps_reg_status.state != DPS_STATE_REGISTERING) {
		return;
	}

	retry_count++;

	if ((retry_count <= DPS_REG_STATUS_UPDATE_MAX_RETRIES) &&
	    (reg_status->state == DPS_STATE_REGISTERING)) {
		k_delayed_work_submit(
			(struct k_delayed_work *)&reg_status->poll_work,
			K_SECONDS(reg_status->retry));
	} else if (retry_count > DPS_REG_STATUS_UPDATE_MAX_RETRIES) {
		LOG_ERR("DPS retry max count (%d) exceeded",
			DPS_REG_STATUS_UPDATE_MAX_RETRIES);
		reg_status->state = DPS_STATE_FAILED;
		cb_handler(reg_status->state);

		return;
	}

	topic_len = snprintk(topic, sizeof(topic),
			     DPS_TOPIC_REG_STATUS,
			     dps_reg_status.reg_id,
			     dps_reg_status.operation_id);

	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = topic_len;

	LOG_DBG("Requesting DPS status update");

	err = mqtt_publish(mqtt_client, &param);
	if (err) {
		LOG_ERR("Failed to send status request, err: %d", err);
		return;
	}
}

static int dps_get_operation_id(char *json)
{
	cJSON *root_obj, *op_id_obj;
	char *op_id_str;
	int err = 0;

	root_obj = cJSON_Parse(json);
	if (root_obj == NULL) {
		LOG_DBG("[%s:%d] Unable to parse input", __func__, __LINE__);
		return -ENOMEM;
	}

	op_id_obj = cJSON_GetObjectItem(root_obj, "operationId");
	if (op_id_obj == NULL) {
		LOG_ERR("Could not find operationId object");
		err = -ENOMEM;
		goto exit;
	}

	op_id_str = cJSON_GetStringValue(op_id_obj);
	if (op_id_str == NULL) {
		LOG_ERR("Failed to get operation ID string");
		err = -ENOMEM;
		goto exit;
	}

	strncpy(dps_reg_status.operation_id, op_id_str,
		sizeof(dps_reg_status.operation_id));

exit:
	cJSON_Delete(root_obj);

	return err;
}

static int dps_check_reg_success(char *json)
{
	cJSON *root_obj, *reg_status_obj, *status_item, *assigned_hub_item;
	char *status_str, *assigned_hub_str;
	int err = 0;

	root_obj = cJSON_Parse(json);
	if (root_obj == NULL) {
		LOG_DBG("[%s:%d] Unable to parse input", __func__, __LINE__);
		return -ENOMEM;
	}

	reg_status_obj =
		cJSON_GetObjectItem(root_obj, "registrationState");
	if (reg_status_obj == NULL) {
		LOG_ERR("Did not find registrationState object");
		err = -ENOMEM;
		goto exit;
	}

	status_item =
		cJSON_GetObjectItem(reg_status_obj, "status");
	if (status_item == NULL) {
		LOG_ERR("Did not find status object");
		err = -ENOMEM;
		goto exit;
	}

	status_str = cJSON_GetStringValue(status_item);
	if (status_str == NULL) {
		LOG_ERR("Failed to get status string");
		err = -ENOMEM;
		goto exit;
	}

	if (strcmp("assigned", status_str) != 0) {
		LOG_ERR("The device was not assigned to hub, status: %s",
			log_strdup(status_str));
		goto exit;
	}

	LOG_DBG("Registration status: %s", log_strdup(status_str));

	assigned_hub_item =
		cJSON_GetObjectItem(reg_status_obj, "assignedHub");
	if (assigned_hub_item == NULL) {
		LOG_ERR("Did not find assignedHub object");
		err = -ENOMEM;
		goto exit;
	}

	assigned_hub_str = cJSON_GetStringValue(assigned_hub_item);
	if (assigned_hub_str == NULL) {
		LOG_ERR("Failed to get assignedHub string");
		err = -ENOMEM;
		goto exit;
	}

	strncpy(dps_reg_status.assigned_hub, assigned_hub_str,
		sizeof(dps_reg_status.assigned_hub));

	err = dps_save_hostname(assigned_hub_str, strlen(assigned_hub_str) + 1);
	if (err) {
		LOG_ERR("Failed to save hostname");
	}
exit:
	cJSON_Delete(root_obj);

	return err;
}

static void dps_parse_reg_update(struct topic_parser_data *topic, char *payload,
				 size_t payload_len)
{
	int err;
	char *reg_id_str = NULL;
	char *retry_str = NULL;
	char *end_ptr;

	LOG_DBG("DPS registration request response received");

	/* Response codes:
	 *	200: Device registration has been processed
	 *	202: Registration pending, keep polling
	 *	All others: Failure
	 */
	if (topic->status == 200) {
		k_delayed_work_cancel(&dps_reg_status.poll_work);

		/* The assigned IoT hub is stored as JSON in the payload */
		err = dps_check_reg_success(payload);
		if (err) {
			LOG_ERR("The registration was not successful");

			dps_reg_status.state = DPS_STATE_FAILED;
			cb_handler(dps_reg_status.state);

			return;
		}

		LOG_INF("Device ID %s is now registered to %s",
			log_strdup(dps_reg_status.reg_id),
			log_strdup(dps_reg_status.assigned_hub));

		dps_reg_status.state = DPS_STATE_REGISTERED;
		cb_handler(dps_reg_status.state);
		return;
	} else if (topic->status != 202) {
		/* Invalid response code */
		LOG_ERR("DPS registration request not successful");
		LOG_ERR("Code was %d, while 202 was expected", topic->status);
		return;
	}

	/* The property bag count shall be 2, and in its raw format, the
	 * property bag part of the topic looks like this:
	 *	?$rid={request_id}&retry-after=x
	 */
	if (topic->prop_bag_count != 2) {
		LOG_WRN("Invalid property bag count");
		return;
	}

	/* Usually the registration ID comes first. */
	if (strcmp("$rid", topic->prop_bag[0].key) == 0) {
		reg_id_str = topic->prop_bag[0].value;
	} else if (strcmp("retry-after", topic->prop_bag[0].key) == 0) {
		retry_str = topic->prop_bag[0].value;
	} else {
		LOG_WRN("Unexpected property bag keys: \"%s\", \"%s\"",
			log_strdup(topic->prop_bag[0].key),
			log_strdup(topic->prop_bag[1].key));
		return;
	}

	/* Usually the retry-after property comes second. */
	if (strcmp("retry-after", topic->prop_bag[1].key) == 0) {
		retry_str = topic->prop_bag[1].value;
	} else if (strcmp("$rid", topic->prop_bag[1].key) == 0) {
		reg_id_str = topic->prop_bag[1].value;
	} else {
		LOG_WRN("Unexpected property bag keys: \"%s\", \"%s\"",
			log_strdup(topic->prop_bag[0].key),
			log_strdup(topic->prop_bag[1].key));
		return;
	}

	if ((reg_id_str == NULL) || (retry_str == NULL)) {
		LOG_ERR("Retry value or request ID was not be found in topic");
		return;
	}

	err = strcmp(dps_reg_status.reg_id, reg_id_str);
	if (err != 0) {
		LOG_ERR("Mismatch in received registration ID");
		return;
	}

	LOG_DBG("Correct registration ID verified");

	/* Convert retry value from string to integer */
	errno = 0;
	dps_reg_status.retry = strtoul(retry_str, &end_ptr, 10);

	if ((errno == ERANGE) || (*end_ptr != '\0')) {
		LOG_WRN("Retry value could not be parsed and applied");
		return;
	}

	LOG_DBG("Received retry value: %d", dps_reg_status.retry);

	/* Get operation ID and store to global status struct */
	err = dps_get_operation_id(payload);
	if (err) {
		dps_reg_status.state = DPS_STATE_FAILED;
		LOG_ERR("Failed to get operation ID, error: %d", err);
		return;
	}

	LOG_DBG("Operation ID: %s", log_strdup(dps_reg_status.operation_id));
	k_delayed_work_submit(&dps_reg_status.poll_work,
			      K_SECONDS(dps_reg_status.retry));
}

static int dps_reg_id_set(const char *id, size_t id_len)
{
	ssize_t len;

	if (id_len > (sizeof(dps_reg_status.reg_id) - 1)) {
		LOG_ERR("The registration ID is too big for buffer");
		return -EMSGSIZE;
	}

	LOG_INF("Setting DPS registration ID: %s", log_strdup(id));

	memcpy(dps_reg_status.reg_id, id, id_len);
	dps_reg_status.reg_id[id_len] = '\0';

	LOG_DBG("Saved DPS registration ID: %s",
		log_strdup(dps_reg_status.reg_id));

	/* Populate registration status topic with ID */
	len = snprintk(dps_topic_reg_pub, sizeof(dps_topic_reg_pub),
		       DPS_TOPIC_REG_PUB, id);
	if ((len < 0) || (len > sizeof(dps_topic_reg_pub))) {
		LOG_ERR("The registration topic is too big for buffer");
		return -ENOMEM;
	}

	return 0;
}

/* Public interface */

int dps_init(struct dps_config *cfg)
{
	int err;

	mqtt_client = (struct mqtt_client *)cfg->mqtt_client;
	cb_handler = cfg->handler;
	dps_reg_status.state = DPS_STATE_NOT_STARTED;

	err = dps_reg_id_set(cfg->reg_id, cfg->reg_id_len);
	if (err) {
		return err;
	}

	k_delayed_work_init(&dps_reg_status.poll_work, dps_reg_status_work_fn);
	k_sem_init(&dps_reg_status.settings_loaded, 0, 1);
	cJSON_Init();

	return dps_settings_init();
}

int dps_start(void)
{
	int err;

	dps_reg_status.state = DPS_STATE_NOT_STARTED;

	err = k_sem_take(&dps_reg_status.settings_loaded, K_SECONDS(1));
	if (err) {
		LOG_ERR("Settings were not loaded in time");
		return -ETIMEDOUT;
	}

	if (strlen(dps_reg_status.assigned_hub) > 0) {
		LOG_INF("Device is assigned to IoT hub: %s",
			log_strdup(dps_reg_status.assigned_hub));

		dps_reg_status.state = DPS_STATE_REGISTERED;

		return -EALREADY;
	}

	dps_reg_status.state = DPS_STATE_REGISTERING;

	return 0;
}

char *dps_hostname_get(void)
{
	if (strlen(dps_reg_status.assigned_hub) == 0) {
		LOG_DBG("No assigned hub found");
		return NULL;
	}

	return dps_reg_status.assigned_hub;
}

int dps_subscribe(void)
{
	int err;
	struct mqtt_topic dps_sub_topics[] = {
		{
			.topic = {
				.utf8 = DPS_TOPIC_REG_SUB,
				.size = sizeof(DPS_TOPIC_REG_SUB) - 1,
			},
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = dps_sub_topics,
		.list_count = ARRAY_SIZE(dps_sub_topics),
		.message_id = k_uptime_get_32(),
	};

	for (size_t i = 0; i < sub_list.list_count; i++) {
		LOG_DBG("Subscribing to: %s",
			log_strdup(sub_list.list[i].topic.utf8));
	}

	err = mqtt_subscribe(mqtt_client, &sub_list);
	if (err) {
		LOG_ERR("Failed to subscribe to topic list, error: %d", err);
		return err;
	}

	LOG_DBG("Successfully subscribed to DPS topics");

	return 0;
}

int dps_send_reg_request(void)
{
	struct mqtt_publish_param param = {
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = k_uptime_get_32(),
	};
	size_t topic_len = sizeof(DPS_TOPIC_REG_PUB) +
			   CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN;
	size_t payload_len = sizeof(DPS_REG_PAYLOAD) +
			   CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN;
	char topic_buf[topic_len];
	char payload_buf[payload_len];

	topic_len = snprintk(topic_buf, sizeof(topic_buf),
		DPS_TOPIC_REG_PUB, dps_reg_status.reg_id);
	if (topic_len < 0 || topic_len > sizeof(topic_buf)) {
		LOG_ERR("Failed to create DPS request topic");
		return -EMSGSIZE;
	}

	payload_len = snprintk(payload_buf, sizeof(payload_buf),
		DPS_REG_PAYLOAD, dps_reg_status.reg_id);
	if (payload_len < 0 || payload_len > sizeof(payload_buf)) {
		LOG_ERR("Failed to create DPS request payload");
		return -EMSGSIZE;
	}

	param.message.topic.topic.utf8 = topic_buf;
	param.message.topic.topic.size = topic_len;
	param.message.payload.data = payload_buf;
	param.message.payload.len = payload_len;

	LOG_DBG("Publishing to DPS registration topic: %s, msg: %s",
		log_strdup(param.message.topic.topic.utf8),
		log_strdup(param.message.payload.data));

	return mqtt_publish(mqtt_client, &param);
}

bool dps_process_message(struct azure_iot_hub_evt *evt,
			 struct topic_parser_data *topic_data)
{
	if (evt->type == AZURE_IOT_HUB_EVT_DISCONNECTED) {
		LOG_WRN("DPS disconnected unexpectedly");

		dps_reg_status.state = DPS_STATE_FAILED;
		cb_handler(dps_reg_status.state);
		return true;
	}

	if (topic_data == NULL) {
		LOG_DBG("No topic data provided, message won't be processed");
		return false;
	}

	if (topic_data->type != TOPIC_TYPE_DPS_REG_RESULT) {
		LOG_DBG("Not a DPS result update, ignoring");
		return false;
	}

	dps_parse_reg_update(topic_data, evt->data.msg.ptr, evt->data.msg.len);

	LOG_DBG("Incoming DPS data consumed");

	return true;
}

bool dps_reg_in_progress(void)
{
	return dps_reg_status.state == DPS_STATE_REGISTERING;
}

enum dps_reg_state dps_get_reg_state(void)
{
	return dps_reg_status.state;
}
