/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <net/aws_iot.h>
#include <net/aws_fota.h>
#include <net/mqtt_helper.h>
#include <stdio.h>

LOG_MODULE_REGISTER(aws_iot, CONFIG_AWS_IOT_LOG_LEVEL);

/* Empty string used to request the AWS IoT shadow document. */
#define AWS_IOT_SHADOW_REQUEST_STRING ""

#define AWS_TOPIC "$aws/things/"
#define AWS_TOPIC_LEN (sizeof(AWS_TOPIC))

#define AWS_CLIENT_ID_PREFIX "%s"
#define AWS_CLIENT_ID_LEN_MAX CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN

#define GET_TOPIC AWS_TOPIC "%s/shadow/get"
#define GET_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 11)

#define UPDATE_TOPIC AWS_TOPIC "%s/shadow/update"
#define UPDATE_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 14)

#define DELETE_TOPIC AWS_TOPIC "%s/shadow/delete"
#define DELETE_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 14)

#define UPDATE_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/update/accepted"
#define UPDATE_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)

#define UPDATE_REJECTED_TOPIC AWS_TOPIC "%s/shadow/update/rejected"
#define UPDATE_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)

#define UPDATE_DELTA_TOPIC AWS_TOPIC "%s/shadow/update/delta"
#define UPDATE_DELTA_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)

#define GET_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/get/accepted"
#define GET_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)

#define GET_REJECTED_TOPIC AWS_TOPIC "%s/shadow/get/rejected"
#define GET_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)

#define DELETE_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/delete/accepted"
#define DELETE_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)

#define DELETE_REJECTED_TOPIC AWS_TOPIC "%s/shadow/delete/rejected"
#define DELETE_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)

/* Message IDs used for AWS and application specific subscriptions. */
#define AWS_SUBS_MESSAGE_ID 1984
#define APP_SUBS_MESSAGE_ID 2469

/* Variables used to store AWS IoT shadow specific topics that the client can subscribe to. */
static char update_accepted_topic[UPDATE_ACCEPTED_TOPIC_LEN + 1];
static char update_rejected_topic[UPDATE_REJECTED_TOPIC_LEN + 1];
static char update_delta_topic[UPDATE_DELTA_TOPIC_LEN + 1];
static char get_accepted_topic[GET_ACCEPTED_TOPIC_LEN + 1];
static char get_rejected_topic[GET_REJECTED_TOPIC_LEN + 1];
static char delete_accepted_topic[DELETE_ACCEPTED_TOPIC_LEN + 1];
static char delete_rejected_topic[DELETE_REJECTED_TOPIC_LEN + 1];

/* Variables used to store AWS IoT shadow specific topics that the client can publish to.
 * These buffers are used when specifying the aws_iot_shadow_topic_type enumerator
 * when calling aws_iot_send().
 */
static char get_topic[GET_TOPIC_LEN + 1];
static char update_topic[UPDATE_TOPIC_LEN + 1];
static char delete_topic[DELETE_TOPIC_LEN + 1];

/* Structures used to reference application specific topics that have been passed in by calling
 * aws_iot_application_topics_set(), and topics that are specific to the AWS IoT shadow service.
 */
static struct mqtt_topic *app_topic_list;
static size_t app_topic_count;
static struct mqtt_topic aws_topic_list[
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE)    +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE)    +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE) +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE) +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE)    +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE) +
	IS_ENABLED(CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE)
];

/* Structure used to reference and invoke the library event handler that is
 * registered by the caller.
 */
static aws_iot_evt_handler_t module_evt_handler;

/* Structure used to confirm successful subscriptions.
 * The structure tracks subscription of two lists of topics, application specific and AWS IoT
 * shadow specific.
 */
static struct suback_conf {
	/* Message ID for MQTT SUBSCRIBE message that contains application specific topics. */
	uint16_t app_subs_id;

	/* Message ID for MQTT SUBSCRIBE message that contains AWS IoT specific topics. */
	uint16_t aws_subs_id;

	/* Flag used to indicate that the client is expecting
	 * acknowledgment for application specific topics.
	 */
	bool app_subs_needs_ack;

	/* Flag used to indicate that the client is expecting
	 * acknowledgment for AWS IoT shadow specific topics.
	 */
	bool aws_subs_needs_ack;

} suback_conf;

/* Semaphore used to wait for acknowledgment of MQTT subscriptions. */
static K_SEM_DEFINE(subs_acked_sem, 0, 1);

/* Forward declarations */
static void device_shadow_document_request(void);

/* Function used to notify the application that an irrecoverable error has occurred.
 *
 * @param err Error code indicating the reason of failure.
 */
static void error_notify(int err)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_ERROR,
		.data.err = err
	};

	/* Explicitly disconnect the client to clear any MQTT helper library state. */
	(void)aws_iot_disconnect();
	module_evt_handler(&evt);
}

/* Function used to notify the application that the client is connected and all subscriptions have
 * been acknowledged. The function also requests the AWS IoT shadow document in case
 * CONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST has been set.
 *
 * @param session_present Variable that indicates if a previous MQTT session is still valid.
 */
static void connected_notify(int session_present)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_CONNECTED,
		.data.persistent_session = session_present
	};

	if (IS_ENABLED(CONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST)) {
		device_shadow_document_request();
	}

	k_sem_give(&subs_acked_sem);
	module_evt_handler(&evt);
}

static void aws_fota_cb_handler(struct aws_fota_event *fota_evt)
{
	struct aws_iot_evt aws_iot_evt = { 0 };

	switch (fota_evt->id) {
	case AWS_FOTA_EVT_START:
		LOG_DBG("AWS_FOTA_EVT_START");
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_START;
		break;
	case AWS_FOTA_EVT_DONE:
		LOG_DBG("AWS_FOTA_EVT_DONE");
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_DONE;
		aws_iot_evt.data.image = fota_evt->image;
		break;
	case AWS_FOTA_EVT_ERASE_PENDING:
		LOG_DBG("AWS_FOTA_EVT_ERASE_PENDING");
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_ERASE_PENDING;
		break;
	case AWS_FOTA_EVT_ERASE_DONE:
		LOG_DBG("AWS_FOTA_EVT_ERASE_DONE");
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_ERASE_DONE;
		break;
	case AWS_FOTA_EVT_ERROR:
		LOG_ERR("AWS_FOTA_EVT_ERROR");
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_ERROR;
		break;
	case AWS_FOTA_EVT_DL_PROGRESS:
		LOG_DBG("AWS_FOTA_EVT_DL_PROGRESS, (%d%%)", fota_evt->dl.progress);
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_DL_PROGRESS;
		aws_iot_evt.data.fota_progress = fota_evt->dl.progress;
		break;
	default:
		LOG_ERR("Unknown FOTA event");
		return;
	}

	module_evt_handler(&aws_iot_evt);
}

/* Function that constructs the AWS IoT shadow topics that are used in the broker connection.
 * Shadow topics have the format $aws/things/<thing-name>/shadow/...
 * where <thing-name> is the MQTT client ID.
 *
 * @param client_id Pointer to null terminated MQTT client ID string.
 *
 * @return 0 on success, otherwise a negative error code is returned indicating reason of failure.
 */
static int shadow_topics_construct(const char *const client_id)
{
	int err;
	int index = 0;

	err = snprintk(get_topic, sizeof(get_topic), GET_TOPIC, client_id);
	if ((err < 0) && (err >= GET_TOPIC_LEN)) {
		return -ENOMEM;
	}

	err = snprintk(update_topic, sizeof(update_topic), UPDATE_TOPIC, client_id);
	if ((err < 0) && (err >= UPDATE_TOPIC_LEN)) {
		return -ENOMEM;
	}

	err = snprintk(delete_topic, sizeof(delete_topic), DELETE_TOPIC, client_id);
	if ((err < 0) && (err >= DELETE_TOPIC_LEN)) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE)) {
		err = snprintk(get_accepted_topic, sizeof(get_accepted_topic),
			       GET_ACCEPTED_TOPIC, client_id);
		if ((err < 0) && (err >= GET_ACCEPTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = get_accepted_topic;
		aws_topic_list[index].topic.size = strlen(get_accepted_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE)) {
		err = snprintk(get_rejected_topic, sizeof(get_rejected_topic),
			       GET_REJECTED_TOPIC, client_id);
		if ((err < 0) && (err >= GET_REJECTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = get_rejected_topic;
		aws_topic_list[index].topic.size = strlen(get_rejected_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE)) {
		err = snprintk(update_accepted_topic, sizeof(update_accepted_topic),
			       UPDATE_ACCEPTED_TOPIC, client_id);
		if ((err < 0) && (err >= UPDATE_ACCEPTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = update_accepted_topic;
		aws_topic_list[index].topic.size = strlen(update_accepted_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE)) {
		err = snprintk(update_rejected_topic, sizeof(update_rejected_topic),
			       UPDATE_REJECTED_TOPIC, client_id);
		if ((err < 0) && (err >= UPDATE_REJECTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = update_rejected_topic;
		aws_topic_list[index].topic.size = strlen(update_rejected_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE)) {
		err = snprintk(update_delta_topic, sizeof(update_delta_topic),
			       UPDATE_DELTA_TOPIC, client_id);
		if ((err < 0) && (err >= UPDATE_DELTA_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = update_delta_topic;
		aws_topic_list[index].topic.size = strlen(update_delta_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE)) {
		err = snprintk(delete_accepted_topic, sizeof(delete_accepted_topic),
			       DELETE_ACCEPTED_TOPIC, client_id);
		if ((err < 0) && (err >= DELETE_ACCEPTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = delete_accepted_topic;
		aws_topic_list[index].topic.size = strlen(delete_accepted_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;

		index++;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE)) {
		err = snprintk(delete_rejected_topic, sizeof(delete_rejected_topic),
			       DELETE_REJECTED_TOPIC, client_id);
		if ((err < 0) && (err >= DELETE_REJECTED_TOPIC_LEN)) {
			return -ENOMEM;
		}

		aws_topic_list[index].topic.utf8 = delete_rejected_topic;
		aws_topic_list[index].topic.size = strlen(delete_rejected_topic);
		aws_topic_list[index].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	}

	return 0;
}

/* Subscribe to topics.
 *
 * @param topic_list Pointer to list containing MQTT topics.
 * @param topic_count Number of entries in the list.
 * @param message_id ID that is used in the MQTT subscription call.
 *
 * @return 0 on success, otherwise a negative error code is returned indicating reason of failure.
 */
static int subscribe(struct mqtt_topic *topic_list, size_t topic_count, uint16_t message_id)
{
	int err;
	struct mqtt_subscription_list sub_list = {
		.list = topic_list,
		.list_count = topic_count,
		.message_id = message_id
	};

	for (size_t i = 0; i < sub_list.list_count; i++) {
		LOG_DBG("Subscribing to topic: %.*s", sub_list.list[i].topic.size,
						      (char *)sub_list.list[i].topic.utf8);
	}

	err = mqtt_helper_subscribe(&sub_list);
	if (err) {
		LOG_ERR("Topics subscribe, error: %d", err);
		return err;
	}

	return 0;
}

static void device_shadow_document_request(void)
{
	int err;

	/* The device shadow document is requested by sending an empty string to
	 * $aws/things/<thing-name>/shadow/get/. The shadow document is
	 * published to the $aws/things/<thing-name>/shadow/get/accepted topic.
	 * To subscribe to this topic set the
	 * CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE configurable option. If
	 * no AWS shadow document exist an error message will be published to
	 * the $aws/things/<thing-name>/shadow/get/rejected topic. Messages
	 * from this topic can be subscribed to by setting the
	 * CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE configurable option.
	 */
	struct aws_iot_data msg = {
		.topic.type = AWS_IOT_SHADOW_TOPIC_GET,
		.ptr = AWS_IOT_SHADOW_REQUEST_STRING,
		.len = strlen(AWS_IOT_SHADOW_REQUEST_STRING)
	};

	err = aws_iot_send(&msg);
	if (err) {
		LOG_ERR("Failed to send device shadow request, error: %d", err);
		error_notify(err);
		return;
	}

	LOG_DBG("Device shadow document requested");
}

/* MQTT helper callback handlers */

static bool on_all_events(struct mqtt_client *const client, const struct mqtt_evt *const event)
{
	int ret = aws_fota_mqtt_evt_handler(client, event);

	if (ret < 0) {
		LOG_ERR("aws_fota_mqtt_evt_handler, error: %d", ret);
		error_notify(ret);
		return false;
	}

	return ret;
}

static void on_connack(enum mqtt_conn_return_code return_code, bool session_present)
{
	int err;

	if (return_code) {
		LOG_ERR("MQTT connection failed, error: %d", return_code);
		error_notify(return_code);
		return;
	}

	LOG_DBG("MQTT client connected!");

	if (session_present) {
		connected_notify(true);
		return;
	}

	if (app_topic_count > 0) {
		suback_conf.app_subs_id = APP_SUBS_MESSAGE_ID;
		suback_conf.app_subs_needs_ack = true;

		err = subscribe(app_topic_list,
				app_topic_count,
				suback_conf.app_subs_id);
		if (err) {
			LOG_ERR("subscribe, error: %d", err);
			error_notify(err);
			return;
		}
	}

	if (ARRAY_SIZE(aws_topic_list) > 0) {
		suback_conf.aws_subs_id = AWS_SUBS_MESSAGE_ID;
		suback_conf.aws_subs_needs_ack = true;

		err = subscribe(aws_topic_list,
				ARRAY_SIZE(aws_topic_list),
				suback_conf.aws_subs_id);
		if (err) {
			LOG_ERR("subscribe, error: %d", err);
			error_notify(err);
			return;
		}
	}

	if (!suback_conf.app_subs_needs_ack && !suback_conf.aws_subs_needs_ack) {
		connected_notify(false);
		return;
	}

	/* Waiting for SUBACKs, handled in on_suback(). */
}

static void on_disconnect(int result)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_DISCONNECTED
	};

	LOG_DBG("MQTT client disconnected: result = %d", result);

	module_evt_handler(&evt);
}

static void on_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_DATA_RECEIVED,
		.data.msg.ptr = payload.ptr,
		.data.msg.len = payload.size,
		.data.msg.topic.str = topic.ptr,
		.data.msg.topic.len = topic.size
	};

	LOG_DBG("Received message: topic = %.*s and len = %d ", topic.size,
								topic.ptr,
								payload.size);

	module_evt_handler(&evt);
}

static void on_puback(uint16_t message_id, int result)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_PUBACK,
		.data.message_id = message_id
	};

	LOG_DBG("Received ACK for published message: id = %d result = %d", message_id, result);

	module_evt_handler(&evt);
}

static void on_suback(uint16_t message_id, int result)
{
	LOG_DBG("Received ACK for subscribe message: id = %d result = %d", message_id, result);

	if (result) {
		LOG_ERR("Error subscribing to topics, err: %d", result);
		error_notify(result);
		return;
	}

	if ((suback_conf.app_subs_id == message_id) && suback_conf.app_subs_needs_ack) {
		suback_conf.app_subs_needs_ack = false;
	} else if ((suback_conf.aws_subs_id == message_id) && suback_conf.aws_subs_needs_ack) {
		suback_conf.aws_subs_needs_ack = false;
	} else {
		LOG_ERR("No SUBACK expected with message ID: %d", message_id);
		error_notify(-ENOENT);
		return;
	}

	/* Notify that the library is connected when both subscription lists are acknowledged. */
	if (!suback_conf.app_subs_needs_ack && !suback_conf.aws_subs_needs_ack) {
		connected_notify(false);
		return;
	}
}

static void on_pingresp(void)
{
	struct aws_iot_evt evt = {
		.type = AWS_IOT_EVT_PINGRESP,
	};

	module_evt_handler(&evt);
}

static void on_error(enum mqtt_helper_error error)
{
	LOG_ERR("MQTT error occurred: %d", error);
	error_notify(error);
}

int aws_iot_send(const struct aws_iot_data *const tx_data)
{
	if (tx_data == NULL) {
		LOG_ERR("Invalid argument tx_data");
		return -EINVAL;
	}

	struct mqtt_publish_param param = {
		.message.topic.qos	  = tx_data->qos,
		.message.topic.topic.utf8 = tx_data->topic.str,
		.message.topic.topic.size = tx_data->topic.len,
		.message.payload.data	  = tx_data->ptr,
		.message.payload.len	  = tx_data->len,
		.dup_flag		  = tx_data->dup_flag,
		.retain_flag		  = tx_data->retain_flag
	};

	switch (tx_data->topic.type) {
	case AWS_IOT_SHADOW_TOPIC_GET:
		param.message.topic.topic.utf8 = get_topic;
		param.message.topic.topic.size = strlen(get_topic);
		break;
	case AWS_IOT_SHADOW_TOPIC_UPDATE:
		param.message.topic.topic.utf8 = update_topic;
		param.message.topic.topic.size = strlen(update_topic);
		break;
	case AWS_IOT_SHADOW_TOPIC_DELETE:
		param.message.topic.topic.utf8 = delete_topic;
		param.message.topic.topic.size = strlen(delete_topic);
		break;
	default:
		if ((tx_data->topic.str == NULL || tx_data->topic.len == 0)) {
			LOG_ERR("No application topic present in tx_data");
			return -ENODATA;
		}
		break;
	}

	/* Assign a random message ID if the ID has not been provided by the application. */
	param.message_id = (tx_data->message_id == 0) ? k_cycle_get_32() : tx_data->message_id;

	LOG_DBG("Using message ID %d", param.message_id);
	LOG_DBG("Publishing to topic: %s", (char *)param.message.topic.topic.utf8);

	return mqtt_helper_publish(&param);
}

int aws_iot_disconnect(void)
{
	int err = mqtt_helper_disconnect();

	if (err) {
		LOG_ERR("mqtt_helper_disconnect, error: %d", err);
		return err;
	}

	return 0;
}

int aws_iot_connect(const struct aws_iot_config *const config)
{
	int err;
	struct mqtt_helper_conn_params conn_params = {
		.hostname.ptr = (config && config->host_name) ? config->host_name : NULL,
		.hostname.size = (config && config->host_name) ? strlen(config->host_name) : 0,
		.device_id.ptr = (config && config->client_id) ? config->client_id : NULL,
		.device_id.size = (config && config->client_id) ? strlen(config->client_id) : 0,
	};

	/* Set the hostname to CONFIG_AWS_IOT_BROKER_HOST_NAME if it was not provided
	 * in the configuration structure.
	 */
	if (conn_params.hostname.size == 0) {
		LOG_DBG("No hostname provided, using Kconfig value: %s",
			CONFIG_AWS_IOT_BROKER_HOST_NAME);

		conn_params.hostname.ptr = CONFIG_AWS_IOT_BROKER_HOST_NAME;
		conn_params.hostname.size = strlen(CONFIG_AWS_IOT_BROKER_HOST_NAME);
	}

	/* Set the device ID to CONFIG_AWS_IOT_CLIENT_ID_STATIC if it was not provided
	 * in the configuration structure.
	 */
	if (conn_params.device_id.size == 0) {
		LOG_DBG("No device ID provided, using Kconfig value: %s",
			CONFIG_AWS_IOT_CLIENT_ID_STATIC);

		conn_params.device_id.ptr = CONFIG_AWS_IOT_CLIENT_ID_STATIC;
		conn_params.device_id.size = strlen(CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	}

	err = shadow_topics_construct(conn_params.device_id.ptr);
	if (err) {
		LOG_ERR("shadow_topics_construct, error: %d", err);
		return err;
	}

	err = mqtt_helper_connect(&conn_params);
	if (err) {
		LOG_ERR("mqtt_helper_connect, error: %d", err);
		return err;
	}

	memset(&suback_conf, 0, sizeof(struct suback_conf));

	err = k_sem_take(&subs_acked_sem, K_SECONDS(CONFIG_AWS_IOT_CONNECT_TIMEOUT_SECONDS));
	if (err == -EAGAIN) {
		LOG_ERR("Timed out waiting for subscription acknowledgments");

		/* Explicitly disconnect the client to clear any MQTT helper library state. */
		(void)aws_iot_disconnect();
	}

	return err;
}

int aws_iot_application_topics_set(const struct mqtt_topic *const list, size_t count)
{
	if ((list == NULL) || (count == 0) || (count > 8)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}

	app_topic_list = (struct mqtt_topic *)list;
	app_topic_count = count;
	return 0;
}

int aws_iot_init(aws_iot_evt_handler_t event_handler)
{
	int err;
	struct mqtt_helper_cfg cfg = {
		.cb = {
			.on_connack = on_connack,
			.on_disconnect = on_disconnect,
			.on_publish = on_publish,
			.on_puback = on_puback,
			.on_suback = on_suback,
			.on_pingresp = on_pingresp,
			.on_error = on_error,
		},
	};

	if (event_handler == NULL) {
		LOG_ERR("Event handler is NULL");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_AWS_FOTA)) {
		err = aws_fota_init(aws_fota_cb_handler);
		if (err) {
			LOG_ERR("aws_fota_init, error: %d", err);
			return err;
		}

		/* Set the MQTT helper library on_all_events handler to be able to intercept
		 * events from the MQTT stack and filter them into the AWS FOTA library.
		 */
		cfg.cb.on_all_events = on_all_events;
	}

	err = mqtt_helper_init(&cfg);
	if (err) {
		LOG_ERR("mqtt_helper_init, error: %d", err);
		return err;
	}

	module_evt_handler = event_handler;
	return 0;
}
