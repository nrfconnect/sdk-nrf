/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/aws_iot.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <stdio.h>

#if defined(CONFIG_AWS_FOTA)
#include <net/aws_fota.h>
#endif

#if defined(CONFIG_AWS_IOT_PROVISION_CERTIFICATES)
#include CONFIG_AWS_IOT_CERTIFICATES_FILE
#endif /* CONFIG_AWS_IOT_PROVISION_CERTIFICATES */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aws_iot, CONFIG_AWS_IOT_LOG_LEVEL);

/* Check if the static host name is set and if its buffer is larger enough if
 * static host name is used
 */
#if !defined(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP)
BUILD_ASSERT(sizeof(CONFIG_AWS_IOT_BROKER_HOST_NAME) > 1,
	    "AWS IoT hostname not set");
BUILD_ASSERT(CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN >=
	     sizeof(CONFIG_AWS_IOT_BROKER_HOST_NAME) - 1,
	     "AWS IoT host name static buffer too small "
	     "Increase CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN");
#endif /* !defined(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP) */

/* Check that the client ID buffer is large enough if a static ID is used. */
#if !defined(CONFIG_AWS_IOT_CLIENT_ID_APP)
BUILD_ASSERT(CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN >=
	     sizeof(CONFIG_AWS_IOT_CLIENT_ID_STATIC) - 1,
	     "AWS IoT client ID static buffer too small "
	     "Increase CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN");
#endif /* !defined(CONFIG_AWS_IOT_CLIENT_ID_APP */

#if defined(CONFIG_AWS_IOT_IPV6)
#define AWS_AF_FAMILY AF_INET6
#else
#define AWS_AF_FAMILY AF_INET
#endif

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

static char client_id_buf[AWS_CLIENT_ID_LEN_MAX + 1];
static char get_topic[GET_TOPIC_LEN + 1];
static char update_topic[UPDATE_TOPIC_LEN + 1];
static char delete_topic[DELETE_TOPIC_LEN + 1];

#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE)
#define UPDATE_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/update/accepted"
#define UPDATE_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)
static char update_accepted_topic[UPDATE_ACCEPTED_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE)
#define UPDATE_REJECTED_TOPIC AWS_TOPIC "%s/shadow/update/rejected"
#define UPDATE_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)
static char update_rejected_topic[UPDATE_REJECTED_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE)
#define UPDATE_DELTA_TOPIC AWS_TOPIC "%s/shadow/update/delta"
#define UPDATE_DELTA_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)
static char update_delta_topic[UPDATE_DELTA_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE)
#define GET_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/get/accepted"
#define GET_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)
static char get_accepted_topic[GET_ACCEPTED_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE)
#define GET_REJECTED_TOPIC AWS_TOPIC "%s/shadow/get/rejected"
#define GET_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 20)
static char get_rejected_topic[GET_REJECTED_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE)
#define DELETE_ACCEPTED_TOPIC AWS_TOPIC "%s/shadow/delete/accepted"
#define DELETE_ACCEPTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)
static char delete_accepted_topic[DELETE_ACCEPTED_TOPIC_LEN + 1];
#endif

#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE)
#define DELETE_REJECTED_TOPIC AWS_TOPIC "%s/shadow/delete/rejected"
#define DELETE_REJECTED_TOPIC_LEN (AWS_TOPIC_LEN + AWS_CLIENT_ID_LEN_MAX + 23)
static char delete_rejected_topic[DELETE_REJECTED_TOPIC_LEN + 1];
#endif

/* Empty string used to request the AWS IoT shadow document. */
#define AWS_IOT_SHADOW_REQUEST_STRING ""

static char aws_host_name_buf[CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN + 1];

static struct aws_iot_app_topic_data app_topic_data;
static struct mqtt_client client;
static struct sockaddr_storage broker;

static char rx_buffer[CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN];
static char tx_buffer[CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN];
static char payload_buf[CONFIG_AWS_IOT_MQTT_PAYLOAD_BUFFER_LEN];

static aws_iot_evt_handler_t module_evt_handler;

static atomic_t disconnect_requested;
static atomic_t connection_poll_active;

/* Flag that indicates if the client is disconnected from the
 * AWS IoT broker, or not.
 */
static atomic_t aws_iot_disconnected = ATOMIC_INIT(1);

/* Structure used to confirm successful subscriptions. */
static struct aws_iot_suback_confirmation {
	/* Subscription ID for application specific topics. */
	uint16_t app_subs_message_id;
	/* Subscription ID for AWS specific topics. */
	uint16_t aws_subs_message_id;
	/* Number of active topic lists subscribed to by the AWS IoT backend. */
	uint8_t active_topic_list_count;
} suback_conf;

static K_SEM_DEFINE(connection_poll_sem, 0, 1);

static int connect_error_translate(const int err)
{
	switch (err) {
	case 0:
		return AWS_IOT_CONNECT_RES_SUCCESS;
	case -ECHILD:
		return AWS_IOT_CONNECT_RES_ERR_NETWORK;
	case -EACCES:
		return AWS_IOT_CONNECT_RES_ERR_NOT_INITD;
	case -ENOEXEC:
		return AWS_IOT_CONNECT_RES_ERR_BACKEND;
	case -EINVAL:
		return AWS_IOT_CONNECT_RES_ERR_PRV_KEY;
	case -EOPNOTSUPP:
		return AWS_IOT_CONNECT_RES_ERR_CERT;
	case -ECONNREFUSED:
		return AWS_IOT_CONNECT_RES_ERR_CERT_MISC;
	case -ETIMEDOUT:
		return AWS_IOT_CONNECT_RES_ERR_TIMEOUT_NO_DATA;
	case -ENOMEM:
		return AWS_IOT_CONNECT_RES_ERR_NO_MEM;
	case -EINPROGRESS:
		return AWS_IOT_CONNECT_RES_ERR_ALREADY_CONNECTED;
	default:
		LOG_ERR("AWS broker connect failed %d", err);
		return AWS_IOT_CONNECT_RES_ERR_MISC;
	}
}

static void aws_iot_notify_event(const struct aws_iot_evt *aws_iot_evt)
{
	if ((module_evt_handler != NULL) && (aws_iot_evt != NULL)) {
		module_evt_handler(aws_iot_evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

#if defined(CONFIG_AWS_FOTA)
static void aws_fota_cb_handler(struct aws_fota_event *fota_evt)
{
	struct aws_iot_evt aws_iot_evt = { 0 };

	if (fota_evt == NULL) {
		return;
	}

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
		LOG_DBG("AWS_FOTA_EVT_DL_PROGRESS, (%d%%)",
			fota_evt->dl.progress);
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_DL_PROGRESS;
		aws_iot_evt.data.fota_progress = fota_evt->dl.progress;
		break;
	default:
		LOG_ERR("Unknown FOTA event");
		return;
	}

	aws_iot_notify_event(&aws_iot_evt);
}
#endif

static int aws_iot_set_host_name(char *const host_name, size_t host_name_len)
{
	if (host_name == NULL) {
		return -EINVAL;
	}

	if (host_name_len >= sizeof(aws_host_name_buf)) {
		return -ENOMEM;
	}

	memcpy(aws_host_name_buf, host_name, host_name_len);
	aws_host_name_buf[host_name_len] = '\0';

	return 0;
}

static int aws_iot_topics_populate(char *const id, size_t id_len)
{
	int err;
#if defined(CONFIG_AWS_IOT_CLIENT_ID_APP)
	err = snprintf(client_id_buf, sizeof(client_id_buf),
		       AWS_CLIENT_ID_PREFIX, id);
	if (err <= 0) {
		return -EINVAL;
	}
	if (err >= sizeof(client_id_buf)) {
		return -ENOMEM;
	}
#else
	err = snprintf(client_id_buf, sizeof(client_id_buf),
		       AWS_CLIENT_ID_PREFIX, CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	if (err <= 0) {
		return -EINVAL;
	}
	if (err >= sizeof(client_id_buf)) {
		return -ENOMEM;
	}
#endif
	err = snprintf(get_topic, sizeof(get_topic),
		       GET_TOPIC, client_id_buf);
	if (err >= GET_TOPIC_LEN) {
		return -ENOMEM;
	}

	err = snprintf(update_topic, sizeof(update_topic),
		       UPDATE_TOPIC, client_id_buf);
	if (err >= UPDATE_TOPIC_LEN) {
		return -ENOMEM;
	}

	err = snprintf(delete_topic, sizeof(delete_topic),
		       DELETE_TOPIC, client_id_buf);
	if (err >= DELETE_TOPIC_LEN) {
		return -ENOMEM;
	}

#if defined(CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE)
	err = snprintf(get_accepted_topic, sizeof(get_accepted_topic),
		       GET_ACCEPTED_TOPIC, client_id_buf);
	if (err >= GET_ACCEPTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE)
	err = snprintf(get_rejected_topic, sizeof(get_rejected_topic),
		       GET_REJECTED_TOPIC, client_id_buf);
	if (err >= GET_REJECTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE)
	err = snprintf(update_accepted_topic, sizeof(update_accepted_topic),
		       UPDATE_ACCEPTED_TOPIC, client_id_buf);
	if (err >= UPDATE_ACCEPTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE)
	err = snprintf(update_rejected_topic, sizeof(update_rejected_topic),
		       UPDATE_REJECTED_TOPIC, client_id_buf);
	if (err >= UPDATE_REJECTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE)
	err = snprintf(update_delta_topic, sizeof(update_delta_topic),
		       UPDATE_DELTA_TOPIC, client_id_buf);
	if (err >= UPDATE_DELTA_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE)
	err = snprintf(delete_accepted_topic, sizeof(delete_accepted_topic),
		       DELETE_ACCEPTED_TOPIC, client_id_buf);
	if (err >= DELETE_ACCEPTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE)
	err = snprintf(delete_rejected_topic, sizeof(delete_rejected_topic),
		       DELETE_REJECTED_TOPIC, client_id_buf);
	if (err >= DELETE_REJECTED_TOPIC_LEN) {
		return -ENOMEM;
	}
#endif
	return 0;
}

/* Returns the number of topics subscribed to (0 or greater),
 * or a negative error code.
 */
static int topic_subscribe(void)
{
	int err;
	const struct mqtt_topic aws_iot_rx_list[] = {
#if defined(CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = get_accepted_topic,
				.size = strlen(get_accepted_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = get_rejected_topic,
				.size = strlen(get_rejected_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = update_accepted_topic,
				.size = strlen(update_accepted_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = update_rejected_topic,
				.size = strlen(update_rejected_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE)
		{
			.topic = {
				.utf8 = update_delta_topic,
				.size = strlen(update_delta_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = delete_accepted_topic,
				.size = strlen(delete_accepted_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
#if defined(CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE)
		{
			.topic = {
				.utf8 = delete_rejected_topic,
				.size = strlen(delete_rejected_topic)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
#endif
	};

	if (app_topic_data.list_count > 0) {

		suback_conf.app_subs_message_id = k_cycle_get_32();

		const struct mqtt_subscription_list app_sub_list = {
			.list = app_topic_data.list,
			.list_count = app_topic_data.list_count,
			.message_id = suback_conf.app_subs_message_id
		};

		for (size_t i = 0; i < app_sub_list.list_count; i++) {
			LOG_DBG("Subscribing to application topic: %s",
				(char *)app_sub_list.list[i].topic.utf8);
		}

		err = mqtt_subscribe(&client, &app_sub_list);
		if (err) {
			LOG_ERR("Application topics subscribe, error: %d", err);
			return err;
		}

		suback_conf.active_topic_list_count++;
	}

	if (ARRAY_SIZE(aws_iot_rx_list) > 0) {

		suback_conf.aws_subs_message_id = k_cycle_get_32();

		const struct mqtt_subscription_list aws_sub_list = {
			.list = (struct mqtt_topic *)&aws_iot_rx_list,
			.list_count = ARRAY_SIZE(aws_iot_rx_list),
			.message_id = suback_conf.aws_subs_message_id
		};

		for (size_t i = 0; i < aws_sub_list.list_count; i++) {
			LOG_DBG("Subscribing to AWS shadow topic: %s",
				(char *)aws_sub_list.list[i].topic.utf8);
		}

		err = mqtt_subscribe(&client, &aws_sub_list);
		if (err) {
			LOG_ERR("AWS shadow topics subscribe, error: %d", err);
			return err;
		}

		suback_conf.active_topic_list_count++;
	}

	if (err < 0) {
		return err;
	}
	return app_topic_data.list_count + ARRAY_SIZE(aws_iot_rx_list);
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
		return;
	}

	LOG_DBG("Device shadow document requested");
}

/* Function that checks if all topics subscribed to by the client has been
 * acknowledged.
 */
static int subscription_list_id_check(uint16_t suback_message_id,
				      int suback_result)
{
	int err;
	static int suback_count;

	if (suback_result) {
		/* Error subscribing to topics. */
		err = suback_result;
		goto clean_exit;
	}

	if (suback_conf.app_subs_message_id == suback_message_id) {
		suback_count++;
	} else if (suback_conf.aws_subs_message_id == suback_message_id) {
		suback_count++;
	}

	if (suback_count >= suback_conf.active_topic_list_count &&
	    suback_conf.active_topic_list_count > 0) {
		/* All subscriptions are acknowledged. */
		err = 0;
		goto clean_exit;
	}

	/* Missing subscription list acknowledgment. Wait for next
	 * MQTT SUBACK.
	 */
	return -EAGAIN;

clean_exit:
	suback_count = 0;
	memset(&suback_conf, 0, sizeof(struct aws_iot_suback_confirmation));

	return err;
}

static int publish_get_payload(struct mqtt_client *const c, size_t length)
{
	if (length > sizeof(payload_buf)) {
		LOG_ERR("Incoming MQTT message too large for payload buffer");
		return -EMSGSIZE;
	}

	return mqtt_readall_publish_payload(c, payload_buf, length);
}

static void mqtt_evt_handler(struct mqtt_client *const c,
			     const struct mqtt_evt *mqtt_evt)
{
	int err;
	struct aws_iot_evt aws_iot_evt = { 0 };

#if defined(CONFIG_AWS_FOTA)
	err = aws_fota_mqtt_evt_handler(c, mqtt_evt);
	if (err == 0) {
		/* Event handled by FOTA library so it can be skipped. */
		return;
	} else if (err < 0) {
		LOG_ERR("aws_fota_mqtt_evt_handler, error: %d", err);
		aws_iot_evt.type = AWS_IOT_EVT_FOTA_ERROR;
		aws_iot_evt.data.err = err;
		aws_iot_notify_event(&aws_iot_evt);
	}
#endif

	switch (mqtt_evt->type) {
	case MQTT_EVT_CONNACK:
		if (mqtt_evt->param.connack.return_code) {
			LOG_ERR("MQTT_EVT_CONNACK, error: %d",
				mqtt_evt->param.connack.return_code);
			aws_iot_evt.data.err =
				mqtt_evt->param.connack.return_code;
			aws_iot_evt.type = AWS_IOT_EVT_ERROR;
			aws_iot_notify_event(&aws_iot_evt);
			break;
		}

		LOG_DBG("MQTT client connected!");

		aws_iot_evt.data.persistent_session =
				   !IS_ENABLED(CONFIG_MQTT_CLEAN_SESSION) &&
				   mqtt_evt->param.connack.session_present_flag;
		aws_iot_evt.type = AWS_IOT_EVT_CONNECTED;
		aws_iot_notify_event(&aws_iot_evt);

		if (!mqtt_evt->param.connack.session_present_flag ||
		    IS_ENABLED(CONFIG_MQTT_CLEAN_SESSION)) {
			err = topic_subscribe();

			if (err < 0) {
				aws_iot_evt.type = AWS_IOT_EVT_ERROR;
				aws_iot_evt.data.err = err;
				aws_iot_notify_event(&aws_iot_evt);
				break;
			}
			if (err == 0) {
				/* There were not topics to subscribe to. */
				aws_iot_evt.type = AWS_IOT_EVT_READY;
				aws_iot_notify_event(&aws_iot_evt);
			} /* else: wait for SUBACK */
		} else {
			/* pre-existing session:
			 * subscription is already established.
			 */
			aws_iot_evt.type = AWS_IOT_EVT_READY;
			aws_iot_notify_event(&aws_iot_evt);

			if (IS_ENABLED(
				CONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST)) {
				device_shadow_document_request();
			}
		}
		break;
	case MQTT_EVT_DISCONNECT:
		LOG_DBG("MQTT_EVT_DISCONNECT: result = %d", mqtt_evt->result);

		aws_iot_evt.data.err = AWS_IOT_DISCONNECT_MISC;

		if (atomic_get(&disconnect_requested)) {
			aws_iot_evt.data.err = AWS_IOT_DISCONNECT_USER_REQUEST;
		}

		atomic_set(&aws_iot_disconnected, 1);
		aws_iot_evt.type = AWS_IOT_EVT_DISCONNECTED;
		aws_iot_notify_event(&aws_iot_evt);
		break;
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &mqtt_evt->param.publish;

		LOG_DBG("MQTT_EVT_PUBLISH: id = %d len = %d ",
			p->message_id,
			p->message.payload.len);

		err = publish_get_payload(c, p->message.payload.len);
		if (err) {
			LOG_ERR("publish_get_payload, error: %d", err);
			break;
		}

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			mqtt_publish_qos1_ack(c, &ack);
		}

		aws_iot_evt.type = AWS_IOT_EVT_DATA_RECEIVED;
		aws_iot_evt.data.msg.ptr = payload_buf;
		aws_iot_evt.data.msg.len = p->message.payload.len;
		aws_iot_evt.data.msg.topic.str = p->message.topic.topic.utf8;
		aws_iot_evt.data.msg.topic.len = p->message.topic.topic.size;

		aws_iot_notify_event(&aws_iot_evt);
	} break;
	case MQTT_EVT_PUBACK:
		LOG_DBG("MQTT_EVT_PUBACK: id = %d result = %d",
			mqtt_evt->param.puback.message_id,
			mqtt_evt->result);

		aws_iot_evt.type = AWS_IOT_EVT_PUBACK;
		aws_iot_evt.data.message_id = mqtt_evt->param.puback.message_id;
		aws_iot_notify_event(&aws_iot_evt);
		break;
	case MQTT_EVT_PINGRESP:
		LOG_DBG("MQTT_EVT_PINGRESP");

		aws_iot_evt.type = AWS_IOT_EVT_PINGRESP;
		aws_iot_notify_event(&aws_iot_evt);
		break;
	case MQTT_EVT_SUBACK:
		LOG_DBG("MQTT_EVT_SUBACK: id = %d result = %d",
			mqtt_evt->param.suback.message_id,
			mqtt_evt->result);

		err = subscription_list_id_check(
					mqtt_evt->param.suback.message_id,
					mqtt_evt->result);
		if (err == 0) {
			/* All subscription list IDs confirmed. */
			if (IS_ENABLED(
				CONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST)) {
				device_shadow_document_request();
			}

			/* MQTT subscriptions established. */
			aws_iot_evt.type = AWS_IOT_EVT_READY;
			aws_iot_notify_event(&aws_iot_evt);
		} else if (err == -EAGAIN) {
			/* Subscriptions remaining to be acknowledged. */
		} else if (err < 0) {
			/* Error subscribing to topics. */
			aws_iot_evt.type = AWS_IOT_EVT_ERROR;
			aws_iot_evt.data.err = err;
			aws_iot_notify_event(&aws_iot_evt);
		}
		break;
	default:
		break;
	}
}

#if defined(CONFIG_AWS_IOT_PROVISION_CERTIFICATES)
static int certificates_provision(void)
{
	static bool certs_added;
	int err;

	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || certs_added) {
		return 0;
	}

	err = tls_credential_add(CONFIG_AWS_IOT_SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate,
				 sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d",
			err);
		return err;
	}

	err = tls_credential_add(CONFIG_AWS_IOT_SEC_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key,
				 sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
		return err;
	}

	err = tls_credential_add(CONFIG_AWS_IOT_SEC_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 device_certificate,
				 sizeof(device_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	certs_added = true;

	return 0;
}
#endif /* CONFIG_AWS_IOT_PROVISION_CERTIFICATES */

#if defined(CONFIG_AWS_IOT_STATIC_IPV4)
static int broker_init(void)
{
	struct sockaddr_in *broker4 =
		((struct sockaddr_in *)&broker);

	inet_pton(AF_INET, CONFIG_AWS_IOT_STATIC_IPV4_ADDR,
		  &broker4->sin_addr);
	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_AWS_IOT_PORT);

	LOG_DBG("IPv4 Address %s", CONFIG_AWS_IOT_STATIC_IPV4_ADDR);

	return 0;
}
#else
static int broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AWS_AF_FAMILY,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(aws_host_name_buf, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo, error %d", err);
		return -ECHILD;
	}

	addr = result;

	while (addr != NULL) {
		if ((addr->ai_addrlen == sizeof(struct sockaddr_in)) &&
		    (AWS_AF_FAMILY == AF_INET)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_AWS_IOT_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr, ipv4_addr,
				  sizeof(ipv4_addr));
			LOG_DBG("IPv4 Address found %s", ipv4_addr);
			break;
		} else if ((addr->ai_addrlen == sizeof(struct sockaddr_in6)) &&
			   (AWS_AF_FAMILY == AF_INET6)) {
			struct sockaddr_in6 *broker6 =
				((struct sockaddr_in6 *)&broker);
			char ipv6_addr[NET_IPV6_ADDR_LEN];

			memcpy(broker6->sin6_addr.s6_addr,
			       ((struct sockaddr_in6 *)addr->ai_addr)
			       ->sin6_addr.s6_addr,
			       sizeof(struct in6_addr));
			broker6->sin6_family = AF_INET6;
			broker6->sin6_port = htons(CONFIG_AWS_IOT_PORT);

			inet_ntop(AF_INET6, &broker6->sin6_addr.s6_addr,
				  ipv6_addr, sizeof(ipv6_addr));
			LOG_DBG("IPv4 Address found %s", ipv6_addr);
			break;
		}

		LOG_DBG("ai_addrlen = %u should be %u or %u",
			(unsigned int)addr->ai_addrlen,
			(unsigned int)sizeof(struct sockaddr_in),
			(unsigned int)sizeof(struct sockaddr_in6));

		addr = addr->ai_next;
		break;
	}

	freeaddrinfo(result);

	return err;
}
#endif /* !defined(CONFIG_AWS_IOT_STATIC_IP) */

static int client_broker_init(struct mqtt_client *const client)
{
	int err;

	mqtt_client_init(client);

	err = broker_init();
	if (err) {
		return err;
	}

	client->broker			= &broker;
	client->evt_cb			= mqtt_evt_handler;
	client->client_id.utf8		= (char *)client_id_buf;
	client->client_id.size		= strlen(client_id_buf);
	client->password		= NULL;
	client->user_name		= NULL;
	client->protocol_version	= MQTT_VERSION_3_1_1;
	client->rx_buf			= rx_buffer;
	client->rx_buf_size		= sizeof(rx_buffer);
	client->tx_buf			= tx_buffer;
	client->tx_buf_size		= sizeof(tx_buffer);
	client->transport.type		= MQTT_TRANSPORT_SECURE;

#if defined(CONFIG_AWS_IOT_LAST_WILL)
	static struct mqtt_topic last_will_topic = {
		.topic.utf8 = CONFIG_AWS_IOT_LAST_WILL_TOPIC,
		.topic.size = sizeof(CONFIG_AWS_IOT_LAST_WILL_TOPIC) - 1,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	static struct mqtt_utf8 last_will_message = {
		.utf8 = CONFIG_AWS_IOT_LAST_WILL_MESSAGE,
		.size = sizeof(CONFIG_AWS_IOT_LAST_WILL_MESSAGE) - 1
	};

	client->will_topic = &last_will_topic;
	client->will_message = &last_will_message;
#endif

	static sec_tag_t sec_tag_list[] = { CONFIG_AWS_IOT_SEC_TAG };
	struct mqtt_sec_config *tls_cfg = &(client->transport).tls.config;

	tls_cfg->peer_verify		= 2;
	tls_cfg->cipher_count		= 0;
	tls_cfg->cipher_list		= NULL;
	tls_cfg->sec_tag_count		= ARRAY_SIZE(sec_tag_list);
	tls_cfg->sec_tag_list		= sec_tag_list;
	tls_cfg->hostname		= aws_host_name_buf;
	tls_cfg->session_cache = TLS_SESSION_CACHE_DISABLED;

#if defined(CONFIG_AWS_IOT_PROVISION_CERTIFICATES)
	err = certificates_provision();
	if (err) {
		LOG_ERR("Could not provision certificates, error: %d", err);
		return err;
	}
#endif /* CONFIG_AWS_IOT_PROVISION_CERTIFICATES */

	return err;
}

static int connect_client(struct aws_iot_config *const config)
{
	int err;

	err = client_broker_init(&client);
	if (err) {
		LOG_ERR("client_broker_init, error: %d", err);
		return err;
	}

	err = mqtt_connect(&client);
	if (err) {
		LOG_ERR("mqtt_connect, error: %d", err);
		err = connect_error_translate(err);
		return err;
	}

#if defined(CONFIG_AWS_IOT_SEND_TIMEOUT)
		struct timeval timeout = {
			.tv_sec = CONFIG_AWS_IOT_SEND_TIMEOUT_SEC
		};

		err = setsockopt(client.transport.tls.sock,
				 SOL_SOCKET,
				 SO_SNDTIMEO,
				 &timeout,
				 sizeof(timeout));
		if (err == -1) {
			LOG_WRN("Failed to set timeout, errno: %d", errno);

			/* Don't propagate this as an error. */
			err = 0;
		} else {
			LOG_DBG("Using send socket timeout of %d seconds",
				CONFIG_AWS_IOT_SEND_TIMEOUT_SEC);
		}
#endif /* CONFIG_AWS_IOT_SEND_TIMEOUT */

	if (config != NULL) {
		config->socket = client.transport.tls.sock;
	}

	return 0;
}

static int connection_poll_start(void)
{
	if (atomic_get(&connection_poll_active)) {
		LOG_DBG("Connection poll in progress");
		return -EINPROGRESS;
	}

	atomic_set(&disconnect_requested, 0);
	k_sem_give(&connection_poll_sem);

	return 0;
}

int aws_iot_ping(void)
{
	if (client.unacked_ping) {
		LOG_DBG("Previous MQTT ping not acknowledged");
		return -ECONNRESET;
	}
	return mqtt_ping(&client);
}

int aws_iot_keepalive_time_left(void)
{
	return mqtt_keepalive_time_left(&client);
}

int aws_iot_input(void)
{
	return mqtt_input(&client);
}

int aws_iot_send(const struct aws_iot_data *const tx_data)
{
	struct aws_iot_data tx_data_pub = {
		.ptr	    = tx_data->ptr,
		.len	    = tx_data->len,
		.qos	    = tx_data->qos,
		.message_id = tx_data->message_id,
		.retain_flag = tx_data->retain_flag,
		.dup_flag = tx_data->dup_flag,
		.topic.type = tx_data->topic.type,
		.topic.str  = tx_data->topic.str,
		.topic.len  = tx_data->topic.len
	};

	switch (tx_data_pub.topic.type) {
	case AWS_IOT_SHADOW_TOPIC_GET:
		tx_data_pub.topic.str = get_topic;
		tx_data_pub.topic.len = strlen(get_topic);
		break;
	case AWS_IOT_SHADOW_TOPIC_UPDATE:
		tx_data_pub.topic.str = update_topic;
		tx_data_pub.topic.len = strlen(update_topic);
		break;
	case AWS_IOT_SHADOW_TOPIC_DELETE:
		tx_data_pub.topic.str = delete_topic;
		tx_data_pub.topic.len = strlen(delete_topic);
		break;
	default:
		if (tx_data_pub.topic.str == NULL ||
		    tx_data_pub.topic.len == 0) {
			LOG_ERR("No application topic present in tx_data");
			return -ENODATA;
		}
		break;
	}

	struct mqtt_publish_param param;

	param.message.topic.qos		= tx_data_pub.qos;
	param.message.topic.topic.utf8	= tx_data_pub.topic.str;
	param.message.topic.topic.size	= tx_data_pub.topic.len;
	param.message.payload.data	= tx_data_pub.ptr;
	param.message.payload.len	= tx_data_pub.len;
	param.dup_flag			= tx_data_pub.dup_flag;
	param.retain_flag		= tx_data_pub.retain_flag;

	/* If the message ID has not been set by the application, a random message ID is assigned
	 * to the packet.
	 */
	if (tx_data_pub.message_id == 0) {
		param.message_id = k_cycle_get_32();
		LOG_DBG("Using message ID %d set by the library", param.message_id);
	} else {
		param.message_id = tx_data_pub.message_id;
		LOG_DBG("Using message ID %d set by the application", param.message_id);
	}

	LOG_DBG("Publishing to topic: %s", (char *)param.message.topic.topic.utf8);

	return mqtt_publish(&client, &param);
}

int aws_iot_disconnect(void)
{
	atomic_set(&disconnect_requested, 1);
	return mqtt_disconnect(&client);
}

int aws_iot_connect(struct aws_iot_config *const config)
{
	int err;

	if (IS_ENABLED(CONFIG_AWS_IOT_CONNECTION_POLL_THREAD)) {
		err = connection_poll_start();
		if (err) {
			LOG_WRN("connection_poll_start failed, error: %d", err);
			return err;
		}
	} else {
		atomic_set(&disconnect_requested, 0);

		err = connect_client(config);
		if (err) {
			LOG_WRN("connect_client failed, error: %d", err);
			return err;
		}

		atomic_set(&aws_iot_disconnected, 0);
	}

	return 0;
}

int aws_iot_subscription_topics_add(
			const struct aws_iot_topic_data *const topic_list,
			size_t list_count)
{
	if (list_count == 0) {
		LOG_ERR("Application subscription list is 0");
		return -EMSGSIZE;
	}

	if (list_count != CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT) {
		LOG_ERR("Application subscription list count mismatch");
		return -EMSGSIZE;
	}

	for (size_t i = 0; i < list_count; i++) {
		app_topic_data.list[i].topic.utf8 = topic_list[i].str;
		app_topic_data.list[i].topic.size = topic_list[i].len;
		app_topic_data.list[i].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	}

	app_topic_data.list_count = list_count;

	return 0;
}

int aws_iot_init(const struct aws_iot_config *const config,
		 aws_iot_evt_handler_t event_handler)
{
	int err;

	if ((IS_ENABLED(CONFIG_AWS_IOT_CLIENT_ID_APP) ||
	     IS_ENABLED(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP)) &&
	     config == NULL) {
		LOG_ERR("config is NULL");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_CLIENT_ID_APP) &&
	    config->client_id_len >= CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN) {
		LOG_ERR("Client ID string too long");
		return -EMSGSIZE;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_CLIENT_ID_APP) &&
	    config->client_id == NULL) {
		LOG_ERR("Client ID not set in the application");
		return -ENODATA;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP) &&
	    config->host_name_len >= CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN) {
		LOG_ERR("AWS host name string too long");
		return -EMSGSIZE;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP) &&
	    config->host_name == NULL) {
		LOG_ERR("AWS host name not set in the application");
		return -ENODATA;
	}

	if (IS_ENABLED(CONFIG_AWS_IOT_CLIENT_ID_APP)) {
		err = aws_iot_topics_populate(config->client_id, config->client_id_len);
	} else {
		err = aws_iot_topics_populate(NULL, 0);
	}

	if (err) {
		LOG_ERR("aws_topics_populate, error: %d", err);
		return err;
	}

#if defined(CONFIG_AWS_IOT_BROKER_HOST_NAME_APP)
	err = aws_iot_set_host_name(config->host_name, config->host_name_len);
#else
	err = aws_iot_set_host_name(CONFIG_AWS_IOT_BROKER_HOST_NAME,
				    strlen(CONFIG_AWS_IOT_BROKER_HOST_NAME));
#endif /* CONFIG_AWS_IOT_BROKER_HOST_NAME_APP */
	if (err) {
		LOG_ERR("aws_iot_set_host_name, error: %d", err);
		return err;
	}

#if defined(CONFIG_AWS_FOTA)
	err = aws_fota_init(&client, aws_fota_cb_handler);
	if (err) {
		LOG_ERR("aws_fota_init, error: %d", err);
		return err;
	}
#endif

	module_evt_handler = event_handler;

	return err;
}

#if defined(CONFIG_AWS_IOT_CONNECTION_POLL_THREAD)
void aws_iot_cloud_poll(void)
{
	int err;
	struct pollfd fds[1];
	struct aws_iot_evt aws_iot_evt = {
		.type = AWS_IOT_EVT_DISCONNECTED,
		.data = { .err = AWS_IOT_DISCONNECT_MISC}
	};

start:
	k_sem_take(&connection_poll_sem, K_FOREVER);
	atomic_set(&connection_poll_active, 1);

	aws_iot_evt.data.err = AWS_IOT_CONNECT_RES_SUCCESS;
	aws_iot_evt.type = AWS_IOT_EVT_CONNECTING;
	aws_iot_notify_event(&aws_iot_evt);

	err = connect_client(NULL);
	if (err != AWS_IOT_CONNECT_RES_SUCCESS) {
		aws_iot_evt.data.err = err;
		aws_iot_evt.type = AWS_IOT_EVT_CONNECTING;
		aws_iot_notify_event(&aws_iot_evt);
		goto reset;
	}

	LOG_DBG("AWS IoT broker connection request sent");

	fds[0].fd = client.transport.tls.sock;
	fds[0].events = POLLIN;

	aws_iot_evt.type = AWS_IOT_EVT_DISCONNECTED;
	atomic_set(&aws_iot_disconnected, 0);

	while (true) {
		err = poll(fds, ARRAY_SIZE(fds), aws_iot_keepalive_time_left());

		/* If poll returns 0 the timeout has expired. */
		if (err == 0) {
			err = aws_iot_ping();
			if (err) {
				LOG_ERR("Cloud MQTT keepalive ping failed: %d", err);
				aws_iot_evt.data.err = AWS_IOT_DISCONNECT_MISC;
				break;
			}
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			err = aws_iot_input();
			if (err) {
				LOG_ERR("Cloud MQTT input error: %d", err);
				if (err == -ENOTCONN) {
					break;
				}
			}

			if (atomic_get(&aws_iot_disconnected) == 1) {
				LOG_DBG("The cloud socket is already closed.");
				break;
			}

			continue;
		}

		if (err < 0) {
			LOG_ERR("poll() returned an error: %d", err);
			aws_iot_evt.data.err = AWS_IOT_DISCONNECT_MISC;
			break;
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_DBG("Socket error: POLLNVAL");
			LOG_DBG("The cloud socket was unexpectedly closed.");
			aws_iot_evt.data.err =
					AWS_IOT_DISCONNECT_INVALID_REQUEST;
			break;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_DBG("Socket error: POLLHUP");
			LOG_DBG("Connection was closed by the cloud.");
			aws_iot_evt.data.err =
					AWS_IOT_DISCONNECT_CLOSED_BY_REMOTE;
			break;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_DBG("Socket error: POLLERR");
			LOG_DBG("Cloud connection was unexpectedly closed.");
			aws_iot_evt.data.err = AWS_IOT_DISCONNECT_MISC;
			break;
		}
	}

	/* Upon a socket error, disconnect the client and notify the
	 * application. If the client has already been disconnected this has
	 * occurred via a MQTT DISCONNECT event and the application has
	 * already been notified.
	 */
	if (atomic_get(&aws_iot_disconnected) == 0) {
		aws_iot_notify_event(&aws_iot_evt);
		aws_iot_disconnect();
	}

reset:
	atomic_set(&connection_poll_active, 0);
	k_sem_take(&connection_poll_sem, K_NO_WAIT);
	goto start;
}

K_THREAD_DEFINE(aws_connection_poll_thread, CONFIG_AWS_IOT_POLL_THREAD_STACK_SIZE,
		aws_iot_cloud_poll, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
#endif /* defined(CONFIG_AWS_IOT_CONNECTION_POLL_THREAD) */
